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
	static auto cache_folder(void);

	// アプリケーションデータを保存するフォルダーを得る.
	static auto cache_folder(void)
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
		using winrt::Windows::Foundation::TypedEventHandler;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionReason;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionResult;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionSession;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedEventArgs;
		using winrt::Windows::ApplicationModel::SuspendingDeferral;
		using winrt::Windows::Storage::StorageFolder;
		using winrt::Windows::Storage::CreationCollisionOption;

		// コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// 引数から中断操作に関する情報を得る.
		auto const& s_operation = args.SuspendingOperation();
		// 延長処理を中断するためのトークンの元を得る.
		auto ct_source = cancellation_token_source();
		// 中断操作に関する情報からアプリの中断を管理するため中断延期 SuspendingDeferral を得る.
		auto s_deferral = s_operation.GetDeferral();
		// 延長実行するための延長実行セッションを得る.
		auto e_session = ExtendedExecutionSession();
		// SavingData を延長実行セッションの目的に格納する.
		e_session.Reason(ExtendedExecutionReason::SavingData);
		// 「To save data.」を延長実行セッションの理由に格納する.
		e_session.Description(L"To save data.");
		// 延長実行セッションが取り消されたときのコルーチンを登録する.
		// RequestExtensionAsync の中でこのコルーチンは呼び出されるので, 
		// 上位関数のローカル変数は参照できる (たぶん).
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
		auto s_token = e_session.Revoked(handler);

		// 延長実行セッションを要求し, その結果を得る.
		auto hr = E_FAIL;
		switch (co_await e_session.RequestExtensionAsync()) {
		case ExtendedExecutionResult::Allowed:
			// 延長実行セッションが許可された場合,
			// トークンを調べて, キャンセルされたか判定する.
			if (ct_source.get_token().is_canceled()) {
				// キャンセルされた場合, 中断する.
				break;
			}
			try {
				// キャンセルでない場合,
				auto s_file{ co_await cache_folder().CreateFileAsync(FILE_NAME, CreationCollisionOption::ReplaceExisting) };
				if (s_file != nullptr) {
					hr = co_await file_write_suspend_async(s_file);
					s_file = nullptr;
					undo_clear();
					s_list_clear(m_list_shapes);
					ShapeText::release_available_fonts();
				}
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
		if (e_session != nullptr) {
			// 延長実行セッションがヌルでない場合,
			// 取り消しコルーチンをセッションから解放し, 
			// セッションを閉じる.
			e_session.Revoked(s_token);
			e_session.Close();
			e_session = nullptr;
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

		// 有効な書体名の配列を設定する.
		ShapeText::set_available_fonts();

		auto hr = E_FAIL;
		auto item{ co_await cache_folder().TryGetItemAsync(FILE_NAME) };
		if (item != nullptr) {
			auto s_file = item.try_as<StorageFile>();
			if (s_file != nullptr) {
				// ストレージファイルを非同期に読む.
				try {
					hr = co_await file_read_async(s_file, true, false);
					// スレッドをメインページの UI スレッドに変える.
					co_await winrt::resume_foreground(this->Dispatcher());
					finish_file_read();
				}
				catch (winrt::hresult_error const& e) {
					hr = e.code();
				}
				s_file = nullptr;
			}
			item = nullptr;
		}
		// スレッドコンテキストを復元する.
		co_await context;
	}

}