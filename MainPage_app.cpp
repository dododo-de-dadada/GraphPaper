//-------------------------------
// MainPage_app.cpp
// アプリケーションの中断と再開
// Windows11 ではデバッガーで「中断」をすることはできるが「再開」ができない.
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using concurrency::cancellation_token_source;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionReason;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionResult;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionSession;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedReason;
	using winrt::Windows::ApplicationModel::SuspendingDeferral;
	using winrt::Windows::ApplicationModel::SuspendingOperation;
	using winrt::Windows::Foundation::TypedEventHandler;
	using winrt::Windows::Storage::ApplicationData;
	using winrt::Windows::Storage::CreationCollisionOption;
	using winrt::Windows::Storage::IStorageItem;
	using winrt::Windows::Storage::StorageFolder;

	constexpr wchar_t APP_DATA_FILE[] = L"ji32k7au4a83";	// アプリケーションデータを格納するファイル名

	//------------------------------
	// アプリケーションがバックグラウンドに移った.
	//------------------------------
	void MainPage::app_entered_background(IInspectable const&/*sender*/, EnteredBackgroundEventArgs const&/*args*/)
	{
		m_mutex_draw.lock();
		m_main_d2d.Trim();
		m_dialog_d2d.Trim();
		m_mutex_draw.unlock();
	}

	//------------------------------
	// アプリケーションがバックグラウンドから戻った.
	//------------------------------
	void MainPage::app_leaving_background(IInspectable const&/*sender*/, LeavingBackgroundEventArgs const&/*args*/)
	{
	}

	//------------------------------
	// アプリケーションの再開の処理を行う.
	// アプリ起動のときは呼ばれない.
	//------------------------------
	IAsyncAction MainPage::app_resuming_async(IInspectable const&, IInspectable const&)
	{
		winrt::apartment_context context;

		ShapeText::set_available_fonts();

		// アプリケーションデータを読み込む.
		IStorageItem app_data_item{
			co_await ApplicationData::Current().LocalCacheFolder().TryGetItemAsync(APP_DATA_FILE) 
		};
		if (app_data_item != nullptr) {
			StorageFile app_data_file = app_data_item.try_as<StorageFile>();
			if (app_data_file != nullptr) {
				constexpr bool RESUME = true;
				constexpr bool SETTING_ONLY = true;
				co_await file_read_gpf_async<RESUME, !SETTING_ONLY>(app_data_file);
				app_data_file = nullptr;
			}
			app_data_item = nullptr;
		}

		// スレッドコンテキストを復元する.
		co_await context;
	}

	//------------------------------
	// アプリケーションの中断の処理を行う.
	// args	中断ハンドラーに渡された引数.
	//------------------------------
	IAsyncAction MainPage::app_suspending_async(IInspectable const&, SuspendingEventArgs const& args)
	{
		SuspendingOperation const& sus_operation = args.SuspendingOperation();
		cancellation_token_source cancel_src = cancellation_token_source();
		SuspendingDeferral sus_deferral = sus_operation.GetDeferral();	// 延長実行セッションの中断	

		// 延長実行セッションに理由と説明を格納する.
		ExtendedExecutionSession ext_session = ExtendedExecutionSession();
		ext_session.Reason(ExtendedExecutionReason::SavingData);
		ext_session.Description(L"To save data.");

		// 延長実行セッションが取り消されたときのコルーチンを登録する.
		winrt::event_token ext_token = ext_session.Revoked(
			// RequestExtensionAsync の中でこのコルーチンは呼び出されるので, 上位関数のローカル変数は参照できる (たぶん).
			[&cancel_src, &sus_deferral](auto const&, auto const& args)
			// IInspectable const&, ExtendedExecutionRevokedEventArgs const& args
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
		switch (co_await ext_session.RequestExtensionAsync()) {
		// 延長実行セッションが許可された場合.
		case ExtendedExecutionResult::Allowed:
			// セッションがキャンセル以外なら
			if (!cancel_src.get_token().is_canceled()) {
				// アプリケーションデータを格納するストレージファイルを作成する.
				StorageFile app_data_file{
					co_await ApplicationData::Current().LocalCacheFolder().CreateFileAsync(APP_DATA_FILE, CreationCollisionOption::ReplaceExisting) 
				};
				// ファイルが作成できたなら, アプリケーションデータを書き込む.
				if (app_data_file != nullptr) {
					co_await file_write_gpf_async<true, false>(app_data_file);
					app_data_file = nullptr;
				}
				// ファイルが作成できないなら, エラーメッセージを表示する.
				else {
					message_show(ICON_ALERT, L"str_err_save", {});
				}
			}
			break;
		case ExtendedExecutionResult::Denied:
			break;
		}

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