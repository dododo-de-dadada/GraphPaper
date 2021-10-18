//-------------------------------
// MainPage_app.cpp
// アプリケーションの中断と再開
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using concurrency::cancellation_token_source;
	using winrt::Windows::ApplicationModel::EnteredBackgroundEventArgs;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionReason;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionResult;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionSession;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedEventArgs;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedReason;
	using winrt::Windows::ApplicationModel::LeavingBackgroundEventArgs;
	using winrt::Windows::ApplicationModel::SuspendingDeferral;
	using winrt::Windows::ApplicationModel::SuspendingEventArgs;
	using winrt::Windows::ApplicationModel::SuspendingOperation;
	using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::Foundation::TypedEventHandler;
	using winrt::Windows::Storage::ApplicationData;
	using winrt::Windows::Storage::CreationCollisionOption;
	using winrt::Windows::Storage::IStorageItem;
	using winrt::Windows::Storage::StorageFile;
	using winrt::Windows::Storage::StorageFolder;

	constexpr wchar_t FILE_NAME[] = L"ji32k7au4a83";	// アプリケーションデータを格納するファイル名

	// アプリケーションがバックグラウンドに移った.
	void MainPage::app_entered_background(IInspectable const&/*sender*/, EnteredBackgroundEventArgs const&/*args*/)
	{
		m_d2d_mutex.lock();
		m_main_d2d.Trim();
		m_sample_dx.Trim();
		m_d2d_mutex.unlock();
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

		ShapeText::set_available_fonts(m_main_d2d);

		HRESULT ok = E_FAIL;
		IStorageItem data_storage{ co_await ApplicationData::Current().LocalCacheFolder().TryGetItemAsync(FILE_NAME) };
		if (data_storage != nullptr) {
			StorageFile data_file = data_storage.try_as<StorageFile>();
			if (data_file != nullptr) {
				// ストレージファイルを非同期に読む.
				try {
					ok = co_await file_read_async<true, false>(data_file);
					// スレッドをメインページの UI スレッドに変える.
					//auto cd = this->Dispatcher();
					//co_await winrt::resume_foreground(cd);
					//file_finish_reading();
				}
				catch (winrt::hresult_error const& e) {
					ok = e.code();
				}
				data_file = nullptr;
			}
			data_storage = nullptr;
		}
		// スレッドコンテキストを復元する.
		co_await context;
	}

	// アプリケーションの中断の処理を行う.
	// args	中断ハンドラーに渡された引数.
	// 戻り値	なし
	IAsyncAction MainPage::app_suspending_async(IInspectable const&, SuspendingEventArgs const& args)
	{
		winrt::apartment_context context;
		SuspendingOperation const& sus_operation = args.SuspendingOperation();
		cancellation_token_source cancel_src = cancellation_token_source();
		SuspendingDeferral sus_deferral = sus_operation.GetDeferral();
		ExtendedExecutionSession ext_session = ExtendedExecutionSession();
		ext_session.Reason(ExtendedExecutionReason::SavingData);
		ext_session.Description(L"To save data.");
		// 延長実行セッションが取り消されたときのコルーチンを登録する.
		winrt::event_token ext_token = ext_session.Revoked(
			// RequestExtensionAsync の中でこのコルーチンは呼び出されるので, 上位関数のローカル変数は参照できる (たぶん).
			[&cancel_src, &sus_deferral](IInspectable const&, ExtendedExecutionRevokedEventArgs const& args)
			{
				// トークンの元をキャンセルする.
				cancel_src.cancel();
				switch (args.Reason()) {
				case ExtendedExecutionRevokedReason::Resumed:
					break;
				case ExtendedExecutionRevokedReason::SystemPolicy:
					break;
				}
				if (sus_deferral != nullptr) {
					// 中断延長を完了し OS に通知しなければ, アプリは再び中断延期を要求できない.
					sus_deferral.Complete();
					sus_deferral = nullptr;
				}
			}
		);

		// 延長実行セッションを要求し, その結果を得る.
		HRESULT ok = E_FAIL;
		switch (co_await ext_session.RequestExtensionAsync()) {
		// 延長実行セッションが許可された場合.
		case ExtendedExecutionResult::Allowed:
			// セッションがキャンセルか判定する.
			if (cancel_src.get_token().is_canceled()) {
				break;
			}
			try {
				// キャンセル以外ならば,
				StorageFile data_file{ co_await ApplicationData::Current().LocalCacheFolder().CreateFileAsync(FILE_NAME, CreationCollisionOption::ReplaceExisting) };
				if (data_file != nullptr) {
					ok = co_await file_write_gpf_async<true, false>(data_file);
					data_file = nullptr;
					ustack_clear();
					slist_clear(m_main_sheet.m_shape_list);
					ShapeText::release_available_fonts();
				}
			}
			catch (winrt::hresult_error const& e) {
				ok = e.code();
			}
			break;
		case ExtendedExecutionResult::Denied:
			break;
		}
		// スレッドをメインページの UI スレッドに変える.
		// スレッド変更は, セッションを閉じる前にでないとダメ.
		//auto cd = this->Dispatcher();
		co_await winrt::resume_foreground(Dispatcher());
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			// 「メモリリーク」メッセージダイアログを表示する.
			message_show(ICON_ALERT, DEBUG_MSG, {});
		}
#endif
		if (ok == S_OK) {
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_clear();
			}
		}
		// スレッドコンテキストを復元する.
		co_await context;
		if (ext_session != nullptr) {
			ext_session.Revoked(ext_token);
			ext_session.Close();
			ext_session = nullptr;
		}
		if (sus_deferral != nullptr) {
			// 中断延長を完了し OS に通知しなければ, 
			// アプリは再び中断延期を要求できない.
			sus_deferral.Complete();
			sus_deferral = nullptr;
		}
	}

}