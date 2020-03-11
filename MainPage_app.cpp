//-------------------------------
// MainPage_app.cpp
// アプリケーションの中断と再開
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t FILE_NAME[] = L"ji32k7au4a83";	// アプリケーションデータを格納するファイル名

	// アプリケーションデータを保存するフォルダーを得る.
	static auto data_folder(void);

	// アプリケーションデータを保存するフォルダーを得る.
	static auto data_folder(void)
	{
		using winrt::Windows::Storage::ApplicationData;
		return ApplicationData::Current().LocalCacheFolder();
	}

	// アプリケーションがバックグラウンドに移った.
	void MainPage::app_entered_background(IInspectable const&/*sender*/, EnteredBackgroundEventArgs const&/*args*/)
	{
		std::lock_guard<std::mutex> lock(m_dx_mutex);
		m_page_dx.Trim();
		m_sample_dx.Trim();
	}

	// アプリケーションの中断の処理を行う.
	// args	中断ハンドラーに渡された引数.
	// 戻り値	なし
	IAsyncAction MainPage::app_suspending_async(SuspendingEventArgs const& args)
	{
		using concurrency::cancellation_token_source;
		using winrt::Windows::Foundation::TypedEventHandler;// <winrt::Windows::Foundation::IInspectable const&, winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedEventArgs const&>;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionReason;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionResult;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionSession;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedEventArgs;
		using winrt::Windows::ApplicationModel::SuspendingDeferral;
		using winrt::Windows::Storage::StorageFolder;
		using winrt::Windows::Storage::CreationCollisionOption;

		auto const& s_op = args.SuspendingOperation();
		// コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// 延長処理を中断するためのトークンの元を得る.
		auto ct_source = cancellation_token_source();
		// アプリの中断を管理するため中断延期 SuspendingDeferral を中断操作から得る.
		auto s_deferral = s_op.GetDeferral();
		// 延長実行するためのセッションを得る.
		auto session = ExtendedExecutionSession();
		// SavingData をセッションの目的に格納する.
		session.Reason(ExtendedExecutionReason::SavingData);
		// 「To save data.」をセッションの理由に格納する.
		session.Description(L"To save data.");
		// セッションが取り消されたときのコルーチンを登録する.
		// RequestExtensionAsync の中でこのコルーチンは呼び出されるので, 
		// 上位関数のローカル変数は参照できる, はず.
		auto handler = [&ct_source, &s_deferral](IInspectable const&, ExtendedExecutionRevokedEventArgs const& args)
		{
			using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedReason;
			// セッションが取り消された場合,
			// トークンの元をキャンセルする.
			ct_source.cancel();
			switch (args.Reason()) {
			case ExtendedExecutionRevokedReason::Resumed:
				break;
			case ExtendedExecutionRevokedReason::SystemPolicy:
				break;
			}
			if (s_deferral != nullptr) {
				// 中断延期がヌルでない場合,
				// 中断延期を完了する.
				// 中断延長を完了し OS に通知しなければ, 
				// アプリは再び中断延期を要求できない.
				s_deferral.Complete();
				s_deferral = nullptr;
			}
		};
		auto s_token = session.Revoked(handler);

		// セッションを要求し, その結果を得る.
		auto hr = E_FAIL;
		switch (co_await session.RequestExtensionAsync()) {
		case ExtendedExecutionResult::Allowed:
			// セッションが許可された場合,
			// トークンを調べて, キャンセルされたか判定する.
			if (ct_source.get_token().is_canceled()) {
				// キャンセルされた場合, 中断する.
				break;
			}
			try {
				// キャンセルでない場合,
				// アプリケーションデータを格納するためのフォルダーを得る.
				// LocalFolder は「有効な範囲外のデータにアクセスしようとしました」内部エラーを起こすが,
				// とりあえず, ファイルを作成して保存はできている.
				auto folder{ data_folder() };
				// ストレージファイルをローカルフォルダに作成する.
				auto s_file{ co_await folder.CreateFileAsync(FILE_NAME, CreationCollisionOption::ReplaceExisting) };
				// 図形データをストレージファイルに非同期に書き込み, 結果を得る.
				hr = co_await file_write_gpf_async(s_file, true);
				// ファイルを破棄する.
				s_file = nullptr;
				// フォルダーを破棄する.
				folder = nullptr;
				// 操作スタックを消去し, 含まれる操作を破棄する.
				undo_clear();
				// 図形リストを消去し, 含まれる図形を破棄する.
				s_list_clear(m_list_shapes);
				// 有効な書体名の配列を破棄する.
				ShapeText::release_available_fonts();
			}
			catch (winrt::hresult_error const& e) {
				// エラーが発生した場合, エラーコードを結果に格納する.
				hr = e.code();
			}
			break;
		case ExtendedExecutionResult::Denied:
			break;
		}
		// スレッドをメインページの UI スレッドに変える.
		// スレッド変更は, セッションを閉じる前にでないとダメ.
		co_await winrt::resume_foreground(this->Dispatcher());
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			// 「メモリリーク」メッセージダイアログを表示する.
			cd_message_show(ICON_ALERT, L"Memory leak occurs", {});
		}
#endif
		if (hr == S_OK) {
			if (m_summary_visible) {
				summary_clear();
			}
		}
		// スレッドコンテキストを復元する.
		co_await context;
		if (session != nullptr) {
			// セッションがヌルでない場合,
			// 取り消しコルーチンをセッションから解放し, 
			// セッションを閉じる.
			session.Revoked(s_token);
			session.Close();
			session = nullptr;
		}
		if (s_deferral != nullptr) {
			// 中断延期がヌルでない場合,
			// 中断延期を完了する.
			// 中断延長を完了し OS に通知しなければ, 
			// アプリは再び中断延期を要求できない.
			s_deferral.Complete();
			s_deferral = nullptr;
		}
	}

	// アプリケーションがバックグラウンドから戻った.
	void MainPage::app_leaving_background(IInspectable const&/*sender*/, LeavingBackgroundEventArgs const&/*args*/)
	{
	}

	// アプリケーションの再開の処理を行う.
	// アプリ起動のときは呼ばれない.
	IAsyncAction MainPage::app_resuming_async(void)
	{
		// コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;

		// 有効な書体名の配列を破棄する.
		// ShapeText::release_available_fonts();
		// 有効な書体名の配列を設定する.
		ShapeText::set_available_fonts();

		// E_FAIL を結果に格納する.
		auto hr = E_FAIL;
		try {
			// アプリ用に作成されたローカルデータフォルダーを得る.
			auto folder{ data_folder() };
			auto s_file{ co_await folder.GetFileAsync(FILE_NAME) };
			// ストレージファイルを非同期に読む.
			co_await file_read_async(s_file, true);
			s_file = nullptr;
			folder = nullptr;
			// スレッドをメインページの UI スレッドに変える.
			co_await winrt::resume_foreground(this->Dispatcher());
			finish_file_read();
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, エラーコードを結果に格納する.
			hr = e.code();
		}
		// スレッドコンテキストを復元する.
		co_await context;
	}

}