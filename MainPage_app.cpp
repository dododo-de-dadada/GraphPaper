//-------------------------------
// MainPage_app.cpp
// アプリケーションの中断と再開
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	static winrt::hstring app_data;	// アプリケーションデータを格納するファイル名
	constexpr auto APP_DATA = L"file_app_data";	// // アプリケーションデータを格納するファイル名のリソース名

	//	アプリケーションデータを保存するフォルダーを得る.
	static auto app_data_folder(void);

	//	アプリケーションデータを保存するフォルダーを得る.
	static auto app_data_folder(void)
	{
		using winrt::Windows::Storage::ApplicationData;
		return ApplicationData::Current().LocalFolder();
	}

	//	アプリケーションがバックグラウンドに移った.
	void MainPage::app_entered_background(IInspectable const&/*sender*/, EnteredBackgroundEventArgs const&/*args*/)
	{
		app_data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(APP_DATA)));
		std::lock_guard<std::mutex> lock(m_dx_mutex);
		m_page_dx.Trim();
		m_sample_dx.Trim();
	}

	//	アプリケーションを非同期に延長する.
	//	s_op	中断操作
	//	戻り値	なし
	//	中断操作は, アプリ中断のハンドラーの引数
	//	SuspendingEventArgs から得られる.
	IAsyncAction MainPage::app_extended_session_async(SuspendingOperation const& s_op)
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
		//static cancellation_token_source ct_source;
		//static SuspendingDeferral s_deferral;
		//static winrt::event_token s_token;

		//	コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;
		//	延長処理を中断するためのトークンの元を得る.
		auto ct_source = cancellation_token_source();
		//	アプリの中断を管理するため中断延期 SuspendingDeferral を中断操作から得る.
		auto s_deferral = s_op.GetDeferral();
		//	延長実行するためのセッションを得る.
		auto session = ExtendedExecutionSession();
		//	SavingData をセッションの目的に格納する.
		session.Reason(ExtendedExecutionReason::SavingData);
		//	「To save data.」をセッションの理由に格納する.
		session.Description(L"To save data.");
		//	セッションが取り消されたときのコルーチンを登録する.
		//	RequestExtensionAsync の中でこのコルーチンは呼び出されるので, 
		//	上位関数のローカル変数は参照できる, はず.
		auto handler = [&ct_source, &s_deferral](IInspectable const&, ExtendedExecutionRevokedEventArgs const& args)
		{
			using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedReason;
			//	セッションが取り消された場合,
			//	トークンの元をキャンセルする.
			ct_source.cancel();
			switch (args.Reason()) {
			case ExtendedExecutionRevokedReason::Resumed:
				break;
			case ExtendedExecutionRevokedReason::SystemPolicy:
				break;
			}
			if (s_deferral != nullptr) {
				//	中断延期がヌルでない場合,
				//	中断延期を完了する.
				//	中断延長を完了し OS に通知しなければ, 
				//	アプリは再び中断延期を要求できない.
				s_deferral.Complete();
				s_deferral = nullptr;
			}
		};
		auto s_token = session.Revoked(handler);

		//	セッションを要求し, その結果を得る.
		auto hr = E_FAIL;
		switch (co_await session.RequestExtensionAsync()) {
		case ExtendedExecutionResult::Allowed:
			//	セッションが許可された場合,
			//	トークンを調べて, キャンセルされたか判定する.
			if (ct_source.get_token().is_canceled()) {
				//	キャンセルされた場合, 中断する.
				break;
			}
			try {
				/*
				//	キャンセルでない場合,
				//	アプリケーションデータを格納するためのローカルフォルダーを得る.
				//	LocalFolder は「有効な範囲外のデータにアクセスしようとしました」内部エラーを起こすが,
				//	とりあえず, ファイルを作成して保存はできている.
				auto l_folder{ app_data_folder() };
				//	ストレージファイルをローカルフォルダに作成する.
				auto s_file{ co_await l_folder.CreateFileAsync(app_data, CreationCollisionOption::ReplaceExisting) };
				//	図形データをストレージファイルに非同期に書き込, 結果を得る.
				hr = co_await file_write_gpf_async(s_file, true);
				//	ファイルを破棄する.
				s_file = nullptr;
				//	フォルダーを破棄する.
				l_folder = nullptr;
				//	操作スタックを消去する.
				undo_clear();
				//	図形リストを消去する.
				s_list_clear(m_list_shapes);
				*/
			}
			catch (winrt::hresult_error const& e) {
				// エラーが発生した場合, エラーコードを結果に格納する.
				hr = e.code();
			}
			break;
		case ExtendedExecutionResult::Denied:
			break;
		}
		//	スレッドをメインページの UI スレッドに変える.
		//	スレッド変更は, セッションを閉じる前にでないとダメ.
		co_await winrt::resume_foreground(this->Dispatcher());
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			cd_message_show(L"icon_alert", L"Memory leak occurs", {});
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
			//	セッションがヌルでない場合,
			//	取り消しコルーチンをセッションから解放し, 
			//	セッションを閉じる.
			session.Revoked(s_token);
			session.Close();
			session = nullptr;
		}
		if (s_deferral != nullptr) {
			//	中断延期がヌルでない場合,
			//	中断延期を完了する.
			//	中断延長を完了し OS に通知しなければ, 
			//	アプリは再び中断延期を要求できない.
			s_deferral.Complete();
			s_deferral = nullptr;
		}
	}

	// アプリケーションがバックグラウンドから戻った.
	void MainPage::app_leaving_background(IInspectable const&/*sender*/, LeavingBackgroundEventArgs const&/*args*/)
	{
	}

	// アプリケーションが再開された.
	// アプリ起動のときは呼ばれない.
	void MainPage::app_resuming(IInspectable const&, IInspectable const& /*object*/)
	{
		// アプリケーションを非同期に再開する.
		auto _{ app_resuming_async() };
	}

	// アプリケーションを非同期に再開する.
	IAsyncAction MainPage::app_resuming_async(void)
	{
		// コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// E_FAIL を結果に格納する.
		auto hr = E_FAIL;
		try {
			// アプリ用に作成されたローカルデータフォルダーを得る.
			auto l_folder{ app_data_folder() };
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
	void MainPage::app_suspending(IInspectable const&, SuspendingEventArgs const& args)
	{
		// 引数から中断操作を得る.
		// 得られた中断操作を指定して,
		// アプリケーションを非同期に延長する.
		auto _{ app_extended_session_async(args.SuspendingOperation()) };
	}

}