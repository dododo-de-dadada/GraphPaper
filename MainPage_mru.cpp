//-------------------------------
// MainPage_mru.cpp
// 最近使ったファイル
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t ERR_RECENT[] = L"str_err_recent";	// 最近使ったファイルのエラーメッセージのリソース名
	constexpr uint32_t MRU_MAX = 25;	// 最近使ったリストの最大数.
	constexpr wchar_t UNTITLED[] = L"str_untitled";	// 無題のリソース名

	// 最近使ったファイルを読み込む.
	// i	最近使ったファイルの番号 (最も直近が 0).
	void MainPage::file_read_recent(const uint32_t i)
	{
		static winrt::event_token save_token;
		static winrt::event_token dont_token;
		static winrt::event_token closed_token;
		static winrt::hstring mru_token;

		//	最近使ったファイルからトークンを得る.
		mru_token = mru_get_token(i);
		if (mru_token.empty()) {
			// 要素が空の場合,
			// 最近使ったファイルのエラーメッセージダイアログを表示する.
			cd_message_show(ERR_RECENT, to_hstring(i + 1));
			return;
		}
		if (m_stack_push == false) {
			// 操作スタックの更新フラグがない場合,
			// 図形データは変更されていないので,
			// 最近使ったファイルを非同期に読み込む.
			auto _{ file_read_recent_async(mru_token) };
			return;
		}
		// 保存確認ダイアログをリソースからロードする.
		cd_load_conf_save();
		save_token = cd_conf_save().PrimaryButtonClick(
			[this](auto, auto)
			{
				// ファイルに非同期に保存して, 最近使ったファイルを非同期に読み込む.
				auto _{ file_save_and_read_recent_async(mru_token) };
			}
		);
		dont_token = cd_conf_save().SecondaryButtonClick(
			[this](auto, auto)
			{
				// 最近使ったファイルを非同期に読み込む.
				auto _{ file_read_recent_async(mru_token) };
			}
		);
		closed_token = cd_conf_save().Closed(
			[this](auto, auto)
			{
				// イベントハンドラーをすべて解除し,
				// 保存確認ダイアログを破棄する.
				cd_conf_save().PrimaryButtonClick(save_token);
				cd_conf_save().SecondaryButtonClick(dont_token);
				cd_conf_save().Closed(closed_token);
				UnloadObject(cd_conf_save());
			}
		);
		// 保存確認ダイアログを表示する.
		{ auto _{ cd_conf_save().ShowAsync() }; }
	}

	//	最近使ったファイルを非同期に読み込む.
	//	token	最近使ったファイルのトークン
	//	戻り値	なし
	IAsyncAction MainPage::file_read_recent_async(const winrt::hstring token)
	{
		auto hr = E_FAIL;
		//	待機カーソルを表示, 表示する前のカーソルを得る.
		auto const p_cur = file_wait_cursor();
		//	コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;
		//	ストレージファイルを最近使ったファイルのトークンから得る.
		auto s_file{ co_await mru_get_file(token) };
		if (s_file != nullptr) {
			//	ストレージファイルが空でない場合,
			//	ストレージファイルから非同期に読み込む.
			hr = co_await file_read_async(s_file);
			//	ストレージファイルを破棄する.
			s_file = nullptr;
			finish_file_read();
			co_await winrt::resume_foreground(this->Dispatcher());
		}
		else {
			//	ストレージファイルが空の場合,
			//	スレッドをメインページの UI スレッドに変える.
			//	最近使ったファイルのエラーメッセージダイアログを表示する.
			co_await winrt::resume_foreground(this->Dispatcher());
			cd_message_show(ERR_RECENT, token);
		}

		Window::Current().CoreWindow().PointerCursor(p_cur);
		// スレッドコンテキストを復元する.
		// ウィンドウのカーソルを復元する.
		co_await context;
	}

	//	ファイルに非同期に保存して, 最近使ったファイルを非同期に読み込む.
	//	token	最近使ったファイルのトークン
	//	戻り値	なし
	IAsyncAction MainPage::file_save_and_read_recent_async(winrt::hstring const& token)
	{
		//	ファイルに非同期に保存する
		if (co_await file_save_async() == S_OK) {
			//	保存できた場合,
			//	最近使ったファイルを非同期に読み込む.
			co_await file_read_recent_async(token);
		}
	}

	// ファイルメニューの「最近使ったファイル 1」が選択された
	void MainPage::mfi_recent_files_1_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	最近使ったファイル (0) を読み込む.
		file_read_recent(0);
	}

	// ファイルメニューの「最近使ったファイル 2」が選択された
	void MainPage::mfi_recent_files_2_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	最近使ったファイル (1) を読み込む.
		file_read_recent(1);
	}

	// ファイルメニューの「最近使ったファイル 3」が選択された
	void MainPage::mfi_recent_files_3_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	最近使ったファイル (2) を読み込む.
		file_read_recent(2);
	}

	// ファイルメニューの「最近使ったファイル 4」が選択された
	void MainPage::mfi_recent_files_4_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	最近使ったファイル (3) を読み込む.
		file_read_recent(3);
	}

	//	ファイルメニューの「最近使ったファイル 5」が選択された
	void MainPage::mfi_recent_files_5_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	最近使ったファイル (4) を読み込む.
		file_read_recent(4);
	}

	//	最近使ったファイルのトークンからストレージファイルを得る.
	//	token	最近使ったファイルのトークン
	//	戻り値	ストレージファイル
	IAsyncOperation<StorageFile> MainPage::mru_get_file(const winrt::hstring token)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;

		//	コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;

		//	ストレージファイルにヌルを格納する.
		StorageFile s_file = nullptr;
		try {
			if (token.empty() == false) {
				//	トークンが空でない場合,
				//	トークンからストレージファイルを得る.
				auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
				s_file = co_await mru_list.GetFileAsync(token);
			}
		}
		catch (winrt::hresult_error) {
		}
		//	スレッドをメインページの UI スレッドに変える.
		co_await winrt::resume_foreground(this->Dispatcher());
		//	取得できても出来なくても最近使ったリストの順番は入れ替わるので,
		//	最近使ったファイルメニュを更新する.
		mru_update_menu_items();
		//	スレッドコンテキストを復元する.
		co_await context;
		//	ストレージファイルを返す.
		co_return s_file;
	}

	//	ストレージファイルを最近使ったファイルに登録する.
	//	s_file	ストレージファイル
	//	戻り値	なし
	//	最近使ったファイルメニューとウィンドウタイトルも更新される.
	//	ストレージファイルがヌルの場合, 最近使ったファイルはそのままで, 
	//	ウィンドウタイトルに無題が格納される.
	void MainPage::mru_add_file(StorageFile const& s_file)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::UI::ViewManagement::ApplicationView;
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		if (s_file != nullptr) {
			m_mru_token = mru_list.Add(s_file, s_file.Path());
			ApplicationView::GetForCurrentView().Title(s_file.Name());
		}
		else {
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			ApplicationView::GetForCurrentView().Title(r_loader.GetString(UNTITLED));
		}
		//	最近使ったファイルメニュを更新する.
		mru_update_menu_items();
	}

	//	最近使ったファイルからトークンを得る.
	//	i	最近使ったファイルの番号 (最も直近が 0).
	//	戻り値	i　番目のファイルのトークン
	winrt::hstring MainPage::mru_get_token(const uint32_t i)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		auto const& mru_ent = mru_list.Entries();
		//	最近使ったファイルの番号と最近使ったリストの要素数を比較する.
		if (i < mru_ent.Size()) {
			//	番号が要素数より小さい場合,
			//	最近使ったリストから要素を得る.
			AccessListEntry item[1];
			mru_ent.GetMany(i, item);
			//	要素のトークンを返す.
			return item[0].Token;
		}
		// 空の文字列を返す.
		return {};
	}

	// 最近使ったファイルメニュを更新する.
	void MainPage::mru_update_menu_items(void)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		auto const& mru_ent = mru_list.Entries();
		const auto ent_size = mru_ent.Size();
		AccessListEntry items[MRU_MAX];
		winrt::hstring data[MRU_MAX];
		mru_ent.GetMany(0, items);
		for (uint32_t i = MRU_MAX; i > 0; i--) {
			if (ent_size >= i) {
				data[i - 1] = winrt::to_hstring(i) + L" " + items[i - 1].Metadata;
			}
			else {
				data[i - 1] = winrt::to_hstring(i);
			}
		}
		mfi_recent_files_1().Text(data[0]);
		mfi_recent_files_2().Text(data[1]);
		mfi_recent_files_3().Text(data[2]);
		mfi_recent_files_4().Text(data[3]);
		mfi_recent_files_5().Text(data[4]);
	}

}