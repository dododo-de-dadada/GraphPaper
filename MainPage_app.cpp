//-------------------------------
// MainPage_app.cpp
// アプリケーションの中断と再開
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Foundation::IAsyncAction;

	constexpr auto APP_DATA = L"file_app_data";	// // アプリケーションデータを格納するファイル名のリソース名
	static winrt::hstring app_data;	// アプリケーションデータを格納するファイル名

	// アプリケーションがバックグラウンドに移った.
	void MainPage::app_entered_background(IInspectable const&/*sender*/, EnteredBackgroundEventArgs const&/*args*/)
	{
		app_data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(APP_DATA)));
		std::lock_guard<std::mutex> lock(m_dx_mutex);
		m_page_dx.Trim();
		m_samp_dx.Trim();
	}

	// アプリケーションを非同期に延長する.
	// s_op	中断操作
	// 戻り値	なし
	// 中断操作は, アプリ中断のハンドラーの引数
	// SuspendingEventArgs から得られる.
	IAsyncAction MainPage::app_extended_session_async(SuspendingOperation const& s_op)
	{
		using concurrency::cancellation_token_source;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionReason;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionResult;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionSession;
		using winrt::Windows::ApplicationModel::SuspendingDeferral;
		using winrt::Windows::Storage::StorageFolder;
		using winrt::Windows::Storage::ApplicationData;
		using winrt::Windows::Storage::CreationCollisionOption;

		// コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// 延長処理を中断するためのトークンの元を得る.
		static auto ct_source = concurrency::cancellation_token_source();
		// アプリの中断を管理するため中断延期 SuspendingDeferral を中断操作から得る.
		static auto s_deferral = s_op.GetDeferral();
		// 延長実行セッションを得る.
		auto session = ExtendedExecutionSession();
		// SavingData をセッションの目的に格納する.
		session.Reason(ExtendedExecutionReason::SavingData);
		// 「...」をセッションの理由に格納する.
		session.Description(L"...");
		// セッションが取り消されたときのコルーチンを登録する.
		static auto s_token = session.Revoked(
			[this](auto, auto args)
			{
				using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedReason;
				// トークンの元をキャンセルする.
				ct_source.cancel();
				switch (args.Reason()) {
				case ExtendedExecutionRevokedReason::Resumed:
					break;
				case ExtendedExecutionRevokedReason::SystemPolicy:
					break;
				}
				// 中断延期を完了する.
				if (s_deferral != nullptr) {
					s_deferral.Complete();
					s_deferral = nullptr;
				}
			}
		);
		// セッションを要求し, その結果を得る.
		auto hr = E_FAIL;
		switch (co_await session.RequestExtensionAsync()) {
		case ExtendedExecutionResult::Allowed:
			// セッションが許可された場合,
			// トークンがキャンセルされたか調べる.
			if (ct_source.get_token().is_canceled()) {
				// キャンセルされた場合, 中断する.
				break;
			}
			try {
				// キャンセルでない場合,
				// アプリ用に作成されたローカルデータフォルダーを得る.
				// ストレージファイルをローカルフォルダに作成する.
				// 図形データをストレージファイルに非同期に書き込, 結果を得る.
				auto l_folder = ApplicationData::Current().LocalFolder();
				auto s_file = co_await l_folder.CreateFileAsync(app_data, CreationCollisionOption::ReplaceExisting);
				hr = co_await file_write_gpf_async(s_file, true);
				// ファイルを破棄する.
				// フォルダーを破棄する.
				s_file = nullptr;
				l_folder = nullptr;
				// 操作スタックを消去する.
				undo_clear();
				s_list_clear(m_list_shapes);
			}
			catch (winrt::hresult_error const& e) {
				// エラーが発生した場合, エラーコードを結果に格納する.
				hr = e.code();
			}
			break;
		case ExtendedExecutionResult::Denied:
			break;
		}
		// 取り消しコルーチンをセッションから解放し, 
		// セッションを閉じる.
		if (session != nullptr) {
			session.Revoked(s_token);
			session.Close();
			session = nullptr;
		}
		// 中断延期を完了する.
		// 中断延長を完了し OS に通知しなければ, 
		// アプリは再び中断延期を要求できない.
		if (s_deferral != nullptr) {
			s_deferral.Complete();
			s_deferral = nullptr;
		}
		// スレッドをメインページの UI スレッドに変える.
		co_await winrt::resume_foreground(this->Dispatcher());
		if (hr == S_OK) {
			if (m_summary_visible) {
				summary_clear();
			}
		}
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			cd_message_show(L"Memory leak occurs", {});
		}
#endif
		// スレッドコンテキストを復元する.
		co_await context;
	}

	// アプリケーションがバックグラウンドから戻った.
	void MainPage::app_leaving_background(IInspectable const&/*sender*/, LeavingBackgroundEventArgs const&/*args*/)
	{
	}

	// アプリケーションが再開された.
	// アプリ起動のときは呼ばれない.
	void MainPage::app_resuming(IInspectable const& /*sender*/, IInspectable const& /*object*/)
	{
		// アプリケーションを非同期に再開する.
		auto _{ app_resuming_async() };
	}

	// アプリケーションを非同期に再開する.
	IAsyncAction MainPage::app_resuming_async(void)
	{
		using winrt::Windows::Storage::ApplicationData;

		// コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// E_FAIL を結果に格納する.
		auto hr = E_FAIL;
		try {
			// アプリ用に作成されたローカルデータフォルダーを得る.
			auto l_folder{ ApplicationData::Current().LocalFolder() };
			auto s_file{ co_await l_folder.GetFileAsync(app_data) };
			co_await file_read_async(s_file, true);
			s_file = nullptr;
			l_folder = nullptr;
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

	// アプリケーションが中断された.
	// アプリの現在の状態を保存する.
	// EnteredBackground が先.
	void MainPage::app_suspending(IInspectable const& /*sender*/, SuspendingEventArgs const& args)
	{
		// 引数から中断操作を得る.
		// 得られた中断操作を指定して,
		// アプリケーションを非同期に延長する.
		auto _{ app_extended_session_async(args.SuspendingOperation()) };
	}

}