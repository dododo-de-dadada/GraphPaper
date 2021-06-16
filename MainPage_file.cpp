//-------------------------------
// MainPage_file.cpp
// ファイルの読み書き
//-------------------------------
#include "pch.h"
#include "MainPage.h"

/*
ただし、4つのWindowsランタイム非同期操作タイプ（IAsyncXxx）のいずれかを
co_awaitした場合、C ++ / WinRTはco_awaitの時点で呼び出しコンテキストをキャプチャします。
また、継続が再開されたときに、そのコンテキストにいることが保証されます。
C ++ / WinRTは、呼び出し側のコンテキストに既にいるかを確認し、
そうでない場合は切り替えます。
*/

// GetFileFromPathAsync を E_ACCESSDENIED なしに使うには以下が必要になる.
// 1. XAMLテキストエディタを使って, Pakage.appxmanifest に broadFileSystemAccess を追加する.
// Pakage.appxmanifest の <Package> のプロパティーに
// xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities" を追加する
// IgnorableNamespaces="uap mp" を IgnorableNamespaces="uap mp rescap" に変更する
// 2. もしなければ <Capabilities> タグを <Package> の子要素として追加する.
// <Capabilities> の子要素として <rescap:Capability Name="broadFileSystemAccess" /> を加える
// 3. コンパイルするたびに, 設定を使ってアプリにファイルアクセスを許可する.
// スタートメニュー > 設定 > プライバー > ファイル システム > ファイルシステムにアクセスできるアプリを選ぶ
// 表示されているアプリをオンにする.
// または,
// スタートメニュー > 設定 > アプリ > アプリと機能
// 表示されているアプリをクリックする.
// 詳細オプション > アプリのアクセス許可
// ファイル システムをオンにする.

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Core::CoreCursorType;

	static auto const& CUR_WAIT = CoreCursor(CoreCursorType::Wait, 0);	// 待機カーソル.
	constexpr wchar_t DESC_GPF[] = L"str_desc_gpf";	// 拡張子 gpf の説明
	constexpr wchar_t DESC_SVG[] = L"str_desc_svg";	// 拡張子 svg の説明
	constexpr wchar_t ERR_FONT[] = L"str_err_font";	// 有効でない書体のエラーメッセージのリソース名
	constexpr wchar_t ERR_READ[] = L"str_err_read";	// 読み込みエラーメッセージのリソース名
	constexpr wchar_t ERR_RECENT[] = L"str_err_recent";	// 最近使ったファイルのエラーメッセージのリソース名
	constexpr wchar_t ERR_WRITE[] = L"str_err_write";	// 書き込みエラーメッセージのリソース名
	constexpr wchar_t FT_GPF[] = L".gpf";	// 図形データファイルの拡張子
	constexpr wchar_t FT_SVG[] = L".svg";	// SVG ファイルの拡張子
	constexpr uint32_t MRU_MAX = 25;	// 最近使ったリストの最大数.
	constexpr wchar_t UNTITLED[] = L"str_untitled";	// 無題のリソース名

	// データライターに SVG 開始タグを書き込む.
	static void file_dt_write_svg_tag(D2D1_SIZE_F const& size, D2D1_COLOR_F const& color, const double dpi, const LEN_UNIT unit, DataWriter const& dt_writer);

	// データライターに SVG 開始タグを書き込む.
	static void file_dt_write_svg_tag(D2D1_SIZE_F const& size, D2D1_COLOR_F const& color, const double dpi, const LEN_UNIT unit, DataWriter const& dt_writer)
	{
		constexpr char SVG_TAG[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" ";
		constexpr char* SVG_UNIT_PX = "px";
		constexpr char* SVG_UNIT_IN = "in";
		constexpr char* SVG_UNIT_MM = "mm";
		constexpr char* SVG_UNIT_PT = "pt";

		// SVG タグの開始を書き込む.
		dt_write_svg(SVG_TAG, dt_writer);
		// 単位付きで幅と高さの属性を書き込む.
		char buf[256];
		double w, h;
		char* u;
		if (unit == LEN_UNIT::INCH) {
			w = size.width / dpi;
			h = size.height / dpi;
			u = SVG_UNIT_IN;
		}
		else if (unit == LEN_UNIT::MILLI) {
			w = size.width * MM_PER_INCH / dpi;
			h = size.height * MM_PER_INCH / dpi;
			u = SVG_UNIT_MM;
		}
		else if (unit == LEN_UNIT::POINT) {
			w = size.width * PT_PER_INCH / dpi;
			h = size.height * PT_PER_INCH / dpi;
			u = SVG_UNIT_PT;
		}
		else {
			w = size.width;
			h = size.height;
			u = SVG_UNIT_PX;
		}
		sprintf_s(buf, "width=\"%lf%s\" height=\"%lf%s\" ", w, u, h, u);
		dt_write_svg(buf, dt_writer);
		// ピクセル単位の幅と高さを viewBox 属性として書き込む.
		dt_write_svg("viewBox=\"0 0 ", dt_writer);
		dt_write_svg(size.width, dt_writer);
		dt_write_svg(size.height, dt_writer);
		dt_write_svg("\" ", dt_writer);
		// 背景色をスタイル属性として書き込む.
		dt_write_svg("style=\"background-color:", dt_writer);
		dt_write_svg(color, dt_writer);
		// svg 開始タグの終了を書き込む.
		dt_write_svg("\" >" SVG_NEW_LINE, dt_writer);
	}

	// ファイルの読み込みが終了した.
	void MainPage::file_finish_reading(void)
	{
		xcvd_is_enabled();

		tool_draw_is_checked(m_tool_draw);
		tool_poly_is_checked(m_tool_poly);
		//tool_vert_snap_is_checked(m_tool_vert_snap);

		color_code_is_checked(m_color_code);
		status_bar_is_checked(m_status_bar);
		len_unit_is_checked(m_len_unit);

		sheet_attr_is_checked();

		wchar_t* unavailable_font;	// 書体名
		if (slist_test_font(m_list_shapes, unavailable_font) != true) {
			// 「無効な書体が使用されています」メッセージダイアログを表示する.
			message_show(ICON_ALERT, ERR_FONT, unavailable_font);
		}
		// 図形一覧の排他制御が true か判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			if (m_list_shapes.empty()) {
				summary_close_click(nullptr, nullptr);
			}
			else {
				summary_remake();
			}
		}
		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
		status_bar_set_curs();
		status_bar_set_draw();
		status_bar_set_grid();
		status_bar_set_sheet();
		status_bar_set_zoom();
		status_bar_set_unit();
		status_bar_visibility();
	}

	// ファイルメニューの「開く」が選択された
	IAsyncAction MainPage::file_open_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::Storage::Pickers::FileOpenPicker;

		if (m_stack_updt && !co_await ask_for_conf_async()) {
			co_return;
		}
		// 待機カーソルを表示, 表示する前のカーソルを得る.
		auto const& prev_cur = file_wait_cursor();
		// ファイル「オープン」ピッカーを取得して開く.
		auto o_picker{FileOpenPicker()};
		o_picker.FileTypeFilter().Append(FT_GPF);
		// ダブルクリックでファイルが選択された場合,
		// co_await が終了する前に, PonterReleased と PonterEntered が呼ばれる.
		// これはピッカーが 2 度目の Released を待たずにダブルクリックを成立させているためだと思われる.
		//scp_sheet_panel().PointerReleased(m_token_event_released);
		//scp_sheet_panel().PointerEntered(m_token_event_entered);

		// ピッカーを非同期で表示してストレージファイルを取得する.
		// ストレージファイルがヌルポインターか判定する.
		// (「閉じる」ボタンが押された場合ストレージファイルは nullptr.)
		auto s_file{ co_await o_picker.PickSingleFileAsync() };
		if (s_file != nullptr) {
			// ストレージファイルを非同期に読む.
			co_await file_read_async(s_file);
			// ストレージファイルを解放する.
			s_file = nullptr;
			//file_finish_reading();
		}
		o_picker = nullptr;
		Window::Current().CoreWindow().PointerCursor(prev_cur);
	}

	// ストレージファイルを非同期に読む.
	// s_file	読み込むストレージファイル
	// suspend	中断フラグ
	// sheet	シートのみフラグ
	// 戻り値	読み込めたら S_OK.
	// 中断フラグが立っている場合, 操作スタックも保存する.
	IAsyncOperation<winrt::hresult> MainPage::file_read_async(StorageFile const& s_file, const bool suspend, const bool sheet) noexcept
	{
		using winrt::Windows::Storage::FileAccessMode;

		auto hres = E_FAIL;
		winrt::apartment_context context;
		m_dx_mutex.lock();
		try {
			const auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::Read) };
			auto dt_reader{ DataReader(ra_stream.GetInputStreamAt(0)) };
			const auto ra_size = static_cast<uint32_t>(ra_stream.Size());
			co_await dt_reader.LoadAsync(ra_size);

			tool_read(dt_reader);
			find_text_read(dt_reader);
			//m_status_bar = static_cast<STATUS_BAR>(dt_reader.ReadUInt32());
			m_len_unit = static_cast<LEN_UNIT>(dt_reader.ReadUInt32());
			m_color_code = static_cast<COLOR_CODE>(dt_reader.ReadUInt16());
			m_status_bar = static_cast<STATUS_BAR>(dt_reader.ReadUInt16());

			m_sheet_main.read(dt_reader);
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			m_sheet_main.set_grid_base(max(g_base, 0.0F));
			m_sheet_main.m_sheet_scale = min(max(m_sheet_main.m_sheet_scale, 0.25f), 4.0f);
			m_sheet_main.m_sheet_size.width = max(min(m_sheet_main.m_sheet_size.width, sheet_size_max()), 1.0F);
			m_sheet_main.m_sheet_size.height = max(min(m_sheet_main.m_sheet_size.height, sheet_size_max()), 1.0F);

			undo_clear();
			slist_clear(m_list_shapes);
#if defined(_DEBUG)
			if (debug_leak_cnt != 0) {
				auto cd = this->Dispatcher();
				co_await winrt::resume_foreground(cd);
				message_show(ICON_ALERT, DEBUG_MSG, {});
				co_await context;
				m_dx_mutex.unlock();
				co_return hres;
			}
#endif
			if (sheet) {
				// シートのみフラグが立っている場合,
				hres = S_OK;
			}
			else if (slist_read(m_list_shapes, dt_reader)) {
				if (suspend) {
					// 中断フラグが立っている場合,
					undo_read(dt_reader);
				}
				hres = S_OK;
			}
			dt_reader.Close();
			ra_stream.Close();
		}
		catch (winrt::hresult_error const& e) {
			hres = e.code();
		}
		m_dx_mutex.unlock();
		if (hres != S_OK) {
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			message_show(ICON_ALERT, ERR_READ, s_file.Path());
		}
		else if (!suspend && !sheet) {
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			file_recent_add(s_file);
			file_finish_reading();
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return hres;
	}

	// ストレージファイルを最近使ったファイルに登録する.
	// s_file	ストレージファイル
	// 戻り値	なし
	// 最近使ったファイルメニューとウィンドウタイトルも更新される.
	// ストレージファイルがヌルの場合, ウィンドウタイトルに無題が格納される.
	void MainPage::file_recent_add(StorageFile const& s_file)
	{
		using winrt::Windows::UI::ViewManagement::ApplicationView;
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		if (s_file != nullptr) {
			m_file_token_mru = StorageApplicationPermissions::MostRecentlyUsedList().Add(s_file, s_file.Path());
			ApplicationView::GetForCurrentView().Title(s_file.Name());
		}
		else {
			ApplicationView::GetForCurrentView().Title(ResourceLoader::GetForCurrentView().GetString(UNTITLED));
		}
		file_recent_update_menu();
	}

	// ファイルメニューの「最近使ったファイル」のサブ項目が選択された.
	void MainPage::file_recent_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		uint32_t n;
		if (sender == mfi_file_recent_1()) {
			n = 0;
		}
		else if (sender == mfi_file_recent_2()) {
			n = 1;
		}
		else if (sender == mfi_file_recent_3()) {
			n = 2;
		}
		else if (sender == mfi_file_recent_4()) {
			n = 3;
		}
		else if (sender == mfi_file_recent_5()) {
			n = 4;
		}
		else {
			return;
		}
		auto _{ file_recent_read_async(n) };
	}

	// 最近使ったファイルのトークンからストレージファイルを得る.
	// token	最近使ったファイルのトークン
	// 戻り値	ストレージファイル
	IAsyncOperation<StorageFile> MainPage::file_recent_get_async(const winrt::hstring token)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;

		// コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;

		// スレッドをメインページの UI スレッドに変える.
		//co_await winrt::resume_foreground(this->Dispatcher());
		// ストレージファイルにヌルを格納する.
		StorageFile s_file = nullptr;
		try {
			if (token.empty() != true) {
				// トークンが空でない場合,
				// トークンからストレージファイルを得る.
				auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
				if (mru_list.ContainsItem(token)) {
					s_file = co_await mru_list.GetFileAsync(token);
				}
			}
		}
		catch (winrt::hresult_error) {
		}
		// 取得できても出来なくても最近使ったリストの順番は入れ替わるので,
		// 最近使ったファイルメニュを更新する.
		file_recent_update_menu();
		// スレッドコンテキストを復元する.
		co_await context;
		// ストレージファイルを返す.
		co_return s_file;
	}

	// 最近使ったファイルを読み込む.
	// i	最近使ったファイルの番号 (最も直近が 0).
	IAsyncAction MainPage::file_recent_read_async(const uint32_t i)
	{
		//using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		// SHCore.dll スレッド
		auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		auto const& mru_entries = mru_list.Entries();
		if (i >= mru_entries.Size()) {
			// ファイルの番号が最近使ったファイルの数以上の場合,
			message_show(ICON_ALERT, ERR_RECENT, to_hstring(i + 1));
			co_return;
		}
		// 最近使ったリストから要素を得る.
		AccessListEntry item[1];
		mru_entries.GetMany(i, item);

		if (m_stack_updt && !co_await ask_for_conf_async()) {
			co_return;
		}
		/*
		// 操作スタックの更新フラグが立っているか判定する.
		if (m_stack_updt) {
			// 確認ダイアログを表示し, 結果を得る.
			const auto dres = co_await cd_conf_save_dialog().ShowAsync();
			// ダイアログの結果がキャンセルか判定する.
			if (dres == ContentDialogResult::None) {
				co_return;
			}
			// ダイアログの結果が「保存する」か判定する.
			else if (dres == ContentDialogResult::Primary) {
				// ファイルに非同期に保存し, 結果が S_OK 以外か判定する.
				if (co_await file_save_async() != S_OK) {
					co_return;
				}
			}
		}
		*/
		auto const prev_cur = file_wait_cursor();
		auto s_file{ co_await file_recent_get_async(item[0].Token) };
		if (s_file != nullptr) {
			// 取得できた場合,
			co_await file_read_async(s_file);
			s_file = nullptr;
		}
		else {
			// 取得できない場合,
			message_show(ICON_ALERT, ERR_RECENT, item[0].Metadata);
		}

		// ウィンドウのカーソルを復元する.
		Window::Current().CoreWindow().PointerCursor(prev_cur);
	}

	// 最近使ったファイルメニュを更新する.
	void MainPage::file_recent_update_menu(void)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		auto const& mru_entries = StorageApplicationPermissions::MostRecentlyUsedList().Entries();
		const auto ent_size = mru_entries.Size();
		AccessListEntry items[MRU_MAX];
		winrt::hstring data[MRU_MAX];
		mru_entries.GetMany(0, items);
		for (uint32_t i = MRU_MAX; i > 0; i--) {
			data[i - 1] = winrt::to_hstring(i) + L":";
			if (ent_size >= i) {
				data[i - 1] = data[i - 1] + L" " + items[i - 1].Metadata;
			}
		}
		mfi_file_recent_1().Text(data[0]);
		mfi_file_recent_2().Text(data[1]);
		mfi_file_recent_3().Text(data[2]);
		mfi_file_recent_4().Text(data[3]);
		mfi_file_recent_5().Text(data[4]);

	}

	// 名前を付けてファイルに非同期に保存する.
	// svg_allowed	SVG 許容フラグ.
	IAsyncOperation<winrt::hresult> MainPage::file_save_as_async(const bool svg_allowed) noexcept
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::Storage::Pickers::FileSavePicker;

		auto hres = E_FAIL;
		auto const& prev_cur = file_wait_cursor();	// 表示する前のカーソル
		// コルーチンの開始時のスレッドコンテキストを保存する.
		//winrt::apartment_context context;
		//co_await resume_background(this->Dispatcher());
		try {
			// ファイル保存ピッカーを得る.
			// ファイルタイプに拡張子 GPF とその説明を追加する.
			auto s_picker{ FileSavePicker() };
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			auto desc_gpf = r_loader.GetString(DESC_GPF);
			auto type_gpf = winrt::single_threaded_vector<winrt::hstring>({ FT_GPF });
			s_picker.FileTypeChoices().Insert(desc_gpf, type_gpf);
			// SVG 許容フラグが立っているか判定する.
			if (svg_allowed) {
				// ファイルタイプに拡張子 SVG とその説明を追加する.
				auto desc_svg = r_loader.GetString(DESC_SVG);
				auto type_svg = winrt::single_threaded_vector<winrt::hstring>({ FT_SVG });
				s_picker.FileTypeChoices().Insert(desc_svg, type_svg);
			}
			// 最近使ったファイルのトークンが空か判定する.
			if (m_file_token_mru.empty()) {
				// 提案されたファイル名に拡張子を格納する.
				s_picker.SuggestedFileName(FT_GPF);
			}
			else {
				// ストレージファイルを最近使ったファイルのトークンから得る.
				auto s_file{ co_await file_recent_get_async(m_file_token_mru) };
				if (s_file != nullptr) {
					// ファイルタイプが FT_GPF か判定する.
					if (s_file.FileType() == FT_GPF) {
						// ファイル名を, 提案するファイル名に格納する.
						s_picker.SuggestedFileName(s_file.Name());
					}
					s_file = nullptr;
				}
			}
			// ファイル保存ピッカーを表示し, ストレージファイルを得る.
			auto s_file{ co_await s_picker.PickSaveFileAsync() };
			// ストレージファイルを取得したか判定する.
			if (s_file != nullptr) {
				// ファイルタイプが SVG か判定する.
				if (s_file.FileType() == FT_SVG) {
					// 図形データを SVG としてストレージファイルに非同期に書き込み, 結果を得る.
					hres = co_await file_dt_write_svg_async(s_file);
				}
				else {
					// 図形データをストレージファイルに非同期に書き込み, 結果を得る.
					hres = co_await file_write_gpf_async(s_file);
				}
				// ストレージファイルを破棄する.
				s_file = nullptr;
			}
			// ピッカーを破棄する.
			s_picker = nullptr;
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, エラーコードを結果に格納する.
			hres = e.code();
		}
		// スレッドをメインページの UI スレッドに変える.
		//co_await winrt::resume_foreground(this->Dispatcher());
		// ウィンドウのカーソルを復元する.
		Window::Current().CoreWindow().PointerCursor(prev_cur);
		// スレッドコンテキストを復元する.
		//co_await context;
		// 結果を返し終了する.
		co_return hres;
	}

	// ファイルメニューの「名前を付けて保存」が選択された
	void MainPage::file_save_as_click(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr bool SVG_ALLOWED = true;
		auto _{ file_save_as_async(SVG_ALLOWED) };
	}

	// ファイルに非同期に保存する
	IAsyncOperation<winrt::hresult> MainPage::file_save_async(void) noexcept
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		// 最近使ったファイルのトークンからストレージファイルを得る.
		auto s_file{ co_await file_recent_get_async(m_file_token_mru) };
		if (s_file == nullptr) {
			// ストレージファイルが空の場合,
			constexpr bool SVG_ALLOWED = true;
			// 名前を付けてファイルに非同期に保存する
			co_return co_await file_save_as_async(!SVG_ALLOWED);
		}

		auto hres = E_FAIL;
		// 待機カーソルを表示, 表示する前のカーソルを得る.
		auto const& prev_cur = file_wait_cursor();
		// 図形データをストレージファイルに非同期に書き込み, 結果を得る.
		hres = co_await file_write_gpf_async(s_file);
		// 結果を判定する.
		if (hres != S_OK) {
			// 結果が S_OK でない場合,
			// スレッドをメインページの UI スレッドに変える.
			// 最近使ったファイルのエラーメッセージダイアログを表示する.
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, ERR_WRITE, m_file_token_mru);
		}
		// スレッドコンテキストを復元する.
		//co_await context;
		// ウィンドウのカーソルを復元する.
		Window::Current().CoreWindow().PointerCursor(prev_cur);
		// 結果を返し終了する.
		co_return hres;
	}

	// ファイルメニューの「上書き保存」が選択された
	void MainPage::file_save_click(IInspectable const&, RoutedEventArgs const&)
	{
		auto _{ file_save_async() };
	}

	// 図形データを SVG としてストレージファイルに非同期に書き込む.
	// file	書き込み先のファイル
	// 戻り値	書き込めた場合 S_OK
	IAsyncOperation<winrt::hresult> MainPage::file_dt_write_svg_async(StorageFile const& s_file)
	{
		using winrt::Windows::Storage::CachedFileManager;
		using winrt::Windows::Storage::FileAccessMode;
		using winrt::Windows::Storage::Provider::FileUpdateStatus;
		using winrt::GraphPaper::implementation::dt_write_svg;
		constexpr char XML_DEC[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" SVG_NEW_LINE;
		constexpr char DOCTYPE[] = "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" SVG_NEW_LINE;

		auto hres = E_FAIL;
		// コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// スレッドをバックグラウンドに変える.
		try {
			co_await winrt::resume_background();
			// ファイル更新の遅延を設定する.
			CachedFileManager::DeferUpdates(s_file);
			// ストレージファイルを開いてランダムアクセスストリームを得る.
			auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::ReadWrite) };
			// ランダムアクセスストリームの先頭からデータライターを作成する.
			auto dt_writer{ DataWriter(ra_stream.GetOutputStreamAt(0)) };
			// XML 宣言を書き込む.
			dt_write_svg(XML_DEC, dt_writer);
			// DOCTYPE を書き込む.
			dt_write_svg(DOCTYPE, dt_writer);
			// データライターに SVG 開始タグを書き込む.
			file_dt_write_svg_tag(m_sheet_main.m_sheet_size, m_sheet_main.m_sheet_color, m_sheet_dx.m_logical_dpi, m_len_unit, dt_writer);
			// 図形リストの各図形について以下を繰り返す.
			for (auto s : m_list_shapes) {
				if (s->is_deleted()) {
					// 消去フラグが立っている場合,
					// 以下を無視する.
					continue;
				}
				s->dt_write_svg(dt_writer);
			}
			// SVG 終了タグを書き込む.
			dt_write_svg("</svg>" SVG_NEW_LINE, dt_writer);
			// ストリームの現在位置をストリームの大きさに格納する.
			ra_stream.Size(ra_stream.Position());
			// バッファ内のデータをストリームに出力する.
			co_await dt_writer.StoreAsync();
			// ストリームをフラッシュする.
			co_await ra_stream.FlushAsync();
			// 遅延させたファイル更新を完了する.
			auto fu_status{ co_await CachedFileManager::CompleteUpdatesAsync(s_file) };
			if (fu_status == FileUpdateStatus::Complete) {
				// 完了した場合, S_OK を結果に格納する.
				hres = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, エラーコードを結果に格納する.
			hres = e.code();
		}
		if (hres != S_OK) {
			// 結果が S_OK でない場合,
			// スレッドをメインページの UI スレッドに変える.
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, ERR_WRITE, s_file.Path());
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return hres;
	}

	// 待機カーソルを表示, 表示する前のカーソルを得る.
	CoreCursor MainPage::file_wait_cursor(void) const
	{
		auto const& c_win = Window::Current().CoreWindow();
		auto prev_cur = c_win.PointerCursor();
		c_win.PointerCursor(CUR_WAIT);
		return prev_cur;
	}

	// 図形データをストレージファイルに非同期に書き込む.
	// s_file	ストレージファイル
	// suspend	中断フラグ
	// 戻り値	書き込みに成功したら true
	IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async(StorageFile const& s_file, const bool suspend, const bool layout)
	{
		using winrt::Windows::Storage::CachedFileManager;
		using winrt::Windows::Storage::FileAccessMode;
		using winrt::Windows::Storage::Provider::FileUpdateStatus;
		constexpr auto REDUCE = true;

		// E_FAIL を結果に格納する.
		auto hres = E_FAIL;
		// コルーチンの開始時のスレッドコンテキストを保存し
		winrt::apartment_context context;
		// スレッドをバックグラウンドに変える.
		co_await winrt::resume_background();
		try {
			// ファイル更新の遅延を設定する.
			// ストレージファイルを開いてランダムアクセスストリームを得る.
			// ランダムアクセスストリームからデータライターを作成する.
			CachedFileManager::DeferUpdates(s_file);
			auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::ReadWrite) };
			auto dt_writer{ DataWriter(ra_stream.GetOutputStreamAt(0)) };

			tool_write(dt_writer);
			find_text_write(dt_writer);
			//dt_writer.WriteUInt32(static_cast<uint32_t>(m_status_bar));
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_len_unit));
			dt_writer.WriteUInt16(static_cast<uint16_t>(m_color_code));
			dt_writer.WriteUInt16(static_cast<uint16_t>(m_status_bar));
			m_sheet_main.write(dt_writer);
			if (suspend) {
				slist_write<!REDUCE>(m_list_shapes, dt_writer);
				undo_write(dt_writer);
			}
			else if (layout) {
			}
			else {
				slist_write<REDUCE>(m_list_shapes, dt_writer);
			}
			ra_stream.Size(ra_stream.Position());
			co_await dt_writer.StoreAsync();
			co_await ra_stream.FlushAsync();
			// データライターを閉じる.
			dt_writer.Close();
			// ストリームを閉じる.
			ra_stream.Close();
			// 遅延させたファイル更新を完了する.
			auto fu_status{ co_await CachedFileManager::CompleteUpdatesAsync(s_file) };
			if (fu_status == FileUpdateStatus::Complete) {
				// 完了した場合, 
				// S_OK を結果に格納する.
				hres = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, 
			// エラーコードを結果に格納する.
			hres = e.code();
		}
		if (hres != S_OK) {
			// 結果が S_OK でない場合,
			// スレッドをメインページの UI スレッドに変える.
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, ERR_WRITE, s_file.Path());
		}
		else if (suspend != true && layout != true) {
			// 中断フラグがない場合,
			// スレッドをメインページの UI スレッドに変える.
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			// ストレージファイルを最近使ったファイルに登録する.
			// ここでエラーが出る.
			file_recent_add(s_file);
			// false を操作スタックの更新フラグに格納する.
			m_stack_updt = false;
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return hres;
	}

}