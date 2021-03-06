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
	static auto app_cache_folder(void);

	// アプリケーションデータを保存するフォルダーを得る.
	static auto app_cache_folder(void)
	{
		return winrt::Windows::Storage::ApplicationData::Current().LocalCacheFolder();
	}

	// アプリケーションがバックグラウンドに移った.
	void MainPage::app_entered_background(IInspectable const&/*sender*/, EnteredBackgroundEventArgs const&/*args*/)
	{
		m_dx_mutex.lock();
		m_sheet_dx.Trim();
		m_sample_dx.Trim();
		m_dx_mutex.unlock();
	}

	// アプリケーションがバックグラウンドから戻った.
	void MainPage::app_leaving_background(IInspectable const&/*sender*/, LeavingBackgroundEventArgs const&/*args*/)
	{
		//using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;
		//mfi_xcvd_paste().IsEnabled(xcvd_contains({ CBF_GPD, StandardDataFormats::Text(), StandardDataFormats::Bitmap() }) != static_cast<size_t>(-1));
	}

	// アプリケーションの再開の処理を行う.
	// アプリ起動のときは呼ばれない.
	IAsyncAction MainPage::app_resuming_async(IInspectable const&, IInspectable const&)
	{
		winrt::apartment_context context;

		ShapeText::set_available_fonts();

		auto hres = E_FAIL;
		auto item{ co_await app_cache_folder().TryGetItemAsync(FILE_NAME) };
		if (item != nullptr) {
			auto s_file = item.try_as<StorageFile>();
			if (s_file != nullptr) {
				// ストレージファイルを非同期に読む.
				try {
					hres = co_await file_read_async(s_file, true, false);
					// スレッドをメインページの UI スレッドに変える.
					//auto cd = this->Dispatcher();
					//co_await winrt::resume_foreground(cd);
					//file_finish_reading();
				}
				catch (winrt::hresult_error const& e) {
					hres = e.code();
				}
				s_file = nullptr;
			}
			item = nullptr;
		}
		// スレッドコンテキストを復元する.
		co_await context;
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
		// RequestExtensionAsync の中でこのコルーチンは呼び出されるので, 上位関数のローカル変数は参照できる (たぶん).
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
				// 中断延長を完了し OS に通知しなければ, アプリは再び中断延期を要求できない.
				s_deferral.Complete();
				s_deferral = nullptr;
			}
		};
		auto s_token = e_session.Revoked(handler);

		// 延長実行セッションを要求し, その結果を得る.
		auto hres = E_FAIL;
		switch (co_await e_session.RequestExtensionAsync()) {
		// 延長実行セッションが許可された場合.
		case ExtendedExecutionResult::Allowed:
			// セッションがキャンセルか判定する.
			if (ct_source.get_token().is_canceled()) {
				break;
			}
			try {
				// キャンセル以外ならば,
				auto s_file{ co_await app_cache_folder().CreateFileAsync(FILE_NAME, CreationCollisionOption::ReplaceExisting) };
				if (s_file != nullptr) {
					hres = co_await file_write_gpf_async(s_file, true, false);
					s_file = nullptr;
					ustack_clear();
					slist_clear(m_list_shapes);
					ShapeText::release_available_fonts();
				}
			}
			catch (winrt::hresult_error const& e) {
				hres = e.code();
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
			message_show(ICON_ALERT, DEBUG_MSG, {});
		}
#endif
		if (hres == S_OK) {
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
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

}