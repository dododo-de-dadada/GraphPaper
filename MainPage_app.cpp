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
		m_mutex_page.lock();
		page_trim();
		m_sample_dx.Trim();
		m_mutex_page.unlock();
	}

	// アプリケーションの中断の処理を行う.
	// args	中断ハンドラーに渡された引数.
	// 戻り値	なし
	IAsyncAction MainPage::app_suspending_async(IInspectable const&, SuspendingEventArgs const& args)
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

		winrt::apartment_context context;
		auto const& s_operation = args.SuspendingOperation();
		auto ct_source = cancellation_token_source();
		auto s_deferral = s_operation.GetDeferral();
		auto e_session = ExtendedExecutionSession();
		e_session.Reason(ExtendedExecutionReason::SavingData);
		e_session.Description(L"To save data.");
		// 延長実行セッションが取り消されたときのコルーチンを登録する.
		// RequestExtensionAsync の中でこのコルーチンは呼び出されるので, 
		// 上位関数のローカル変数は参照できる (たぶん).
		auto handler = [&ct_source, &s_deferral](IInspectable const&, ExtendedExecutionRevokedEventArgs const& args)
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
			if (s_deferral != nullptr) {
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
			if (ct_source.get_token().is_canceled()) {
				// キャンセルされた場合, 中断する.
				break;
			}
			try {
				// キャンセルでない場合,
				auto s_file{ co_await cache_folder().CreateFileAsync(FILE_NAME, CreationCollisionOption::ReplaceExisting) };
				if (s_file != nullptr) {
					hr = co_await file_write_gpf_async(s_file, true, false);
					s_file = nullptr;
					undo_clear();
					s_list_clear(m_list_shapes);
					ShapeText::release_available_fonts();
				}
			}
			catch (winrt::hresult_error const& e) {
				hr = e.code();
			}
			break;
		case ExtendedExecutionResult::Denied:
			break;
		}
		// スレッドをメインページの UI スレッドに変える.
		// スレッド変更は, セッションを閉じる前にでないとダメ.
		auto cd = this->Dispatcher();
		co_await winrt::resume_foreground(cd);
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			// 「メモリリーク」メッセージダイアログを表示する.
			cd_message_show(ICON_ALERT, L"Memory leak occurs", {});
		}
#endif
		if (hr == S_OK) {
			if (m_mutex_summary.load(std::memory_order_acquire)) {
			//if (m_summary_visible) {
				summary_clear();
			}
		}
		// スレッドコンテキストを復元する.
		co_await context;
		if (e_session != nullptr) {
			e_session.Revoked(s_token);
			e_session.Close();
			e_session = nullptr;
		}
		if (s_deferral != nullptr) {
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
	IAsyncAction MainPage::app_resuming_async(IInspectable const&, IInspectable const&)
	{
		winrt::apartment_context context;

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
					//auto cd = this->Dispatcher();
					//co_await winrt::resume_foreground(cd);
					//file_finish_reading();
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