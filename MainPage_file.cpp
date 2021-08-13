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

/*
Yes the behavior changed between the April 2018 and October 2018 releases, and the default is now Disabled.
This is a privacy constraint - we're very focused on maintaining the user's privacy.
The documentation for this is up-to-date: https://docs.microsoft.com/en-us/windows/uwp/files/file-access-permissions#accessing-additional-locations.
As of right now, if you want to detect whether the setting is enabled or disabled, 
you can simply try to access some file/folder to which this setting would grant you permission if enabled and deny permission if disabled (eg, "C:\").
If disabled, you can then launch the Settings app on the File System privacy page.
For example:

protected override async void OnNavigatedTo(NavigationEventArgs e)
{
	try
	{
		StorageFolder folder = await StorageFolder.GetFolderFromPathAsync(@"C:\");
		// do work
	}
	catch
	{
		MessageDialog dlg = new MessageDialog(
			"It seems you have not granted permission for this app to access the file system broadly. " +
			"Without this permission, the app will only be able to access a very limited set of filesystem locations. " +
			"You can grant this permission in the Settings app, if you wish. You can do this now or later. " +
			"If you change the setting while this app is running, it will terminate the app so that the " +
			"setting can be applied. Do you want to do this now?",
			"File system permissions");
		dlg.Commands.Add(new UICommand("Yes", new UICommandInvokedHandler(InitMessageDialogHandler), 0));
		dlg.Commands.Add(new UICommand("No", new UICommandInvokedHandler(InitMessageDialogHandler), 1));
		dlg.DefaultCommandIndex = 0;
		dlg.CancelCommandIndex = 1;
		await dlg.ShowAsync();
	}
}

private async void InitMessageDialogHandler(IUICommand command)
{
	if ((int)command.Id == 0)
	{
		await Launcher.LaunchUriAsync(new Uri("ms-settings:privacy-broadfilesystemaccess"));
	}
}
*/

// 以下の情報は古い.
// GetFileFromPathAsync, CreateFileAsync を E_ACCESSDENIED なしに使うには以下が必要になる.
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
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::Foundation::Uri;
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapBufferAccessMode;
	using winrt::Windows::Graphics::Imaging::BitmapDecoder;
	using winrt::Windows::Graphics::Imaging::BitmapEncoder;
	using winrt::Windows::Graphics::Imaging::BitmapInterpolationMode;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::Graphics::Imaging::BitmapRotation;
	using winrt::Windows::Graphics::Imaging::SoftwareBitmap;
	using winrt::Windows::Storage::AccessCache::AccessListEntry;
	using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
	using winrt::Windows::Storage::CachedFileManager;
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::Storage::Pickers::FileOpenPicker;
	using winrt::Windows::Storage::Pickers::FileSavePicker;
	using winrt::Windows::Storage::Pickers::PickerLocationId;
	using winrt::Windows::Storage::Provider::FileUpdateStatus;
	using winrt::Windows::Storage::StorageFolder;
	using winrt::Windows::System::Launcher;
	using winrt::Windows::UI::Core::CoreCursorType;
	using winrt::Windows::UI::ViewManagement::ApplicationView;

	static auto const& CUR_WAIT = CoreCursor(CoreCursorType::Wait, 0);	// 待機カーソル.
	constexpr wchar_t DESC_GPF[] = L"str_desc_gpf";	// 拡張子 gpf の説明
	constexpr wchar_t DESC_SVG[] = L"str_desc_svg";	// 拡張子 svg の説明
	constexpr wchar_t ERR_FONT[] = L"str_err_font";	// 有効でない書体のエラーメッセージのリソース名
	constexpr wchar_t ERR_READ[] = L"str_err_read";	// 読み込みエラーメッセージのリソース名
	constexpr wchar_t ERR_RECENT[] = L"str_err_recent";	// 最近使ったファイルのエラーメッセージのリソース名
	constexpr wchar_t ERR_WRITE[] = L"str_err_write";	// 書き込みエラーメッセージのリソース名
	constexpr wchar_t DOT_GPF[] = L".gpf";	// 図形データファイルの拡張子
	constexpr wchar_t DOT_SVG[] = L".svg";	// SVG ファイルの拡張子
	constexpr wchar_t DOT_BMP[] = L".bmp";	// 画像ファイルの拡張子
	constexpr wchar_t DOT_GIF[] = L".gif";	// 画像ファイルの拡張子
	constexpr wchar_t DOT_JPG[] = L".jpg";	// 画像ファイルの拡張子
	constexpr wchar_t DOT_PNG[] = L".png";	// 画像ファイルの拡張子
	constexpr wchar_t DOT_TIF[] = L".tif";	// 画像ファイルの拡張子
	constexpr uint32_t MRU_MAX = 25;	// 最近使ったリストの最大数.
	constexpr wchar_t UNTITLED[] = L"str_untitled";	// 無題のリソース名

	// データライターに SVG 開始タグを書き込む.
	static void file_write_svg_tag(D2D1_SIZE_F const& size, D2D1_COLOR_F const& color, const double dpi, const LEN_UNIT unit, DataWriter const& dt_writer);

	// データライターに SVG 開始タグを書き込む.
	static void file_write_svg_tag(D2D1_SIZE_F const& size, D2D1_COLOR_F const& color, const double dpi, const LEN_UNIT unit, DataWriter const& dt_writer)
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
		misc_color_is_checked(m_misc_color_code);
		status_bar_is_checked(m_misc_status_bar);
		misc_len_is_checked(m_misc_len_unit);
		image_keep_aspect_is_checked(m_image_keep_aspect);
		
		sheet_attr_is_checked();

		wchar_t* unavailable_font;	// 書体名
		if (slist_test_font(m_list_shapes, unavailable_font) != true) {
			// 「無効な書体が使用されています」メッセージダイアログを表示する.
			message_show(ICON_ALERT, ERR_FONT, unavailable_font);
		}

		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			if (m_list_shapes.empty()) {
				summary_close_click(nullptr, nullptr);
			}
			else if (lv_summary_list() != nullptr) {
				summary_remake();
			}
			else {
				// リソースから図形の一覧パネルを見つける.
				auto _{ FindName(L"gd_summary_panel") };
				gd_summary_panel().Visibility(UI_VISIBLE);
			}
		}

		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
		status_set_curs();
		status_set_draw();
		status_set_grid();
		status_set_sheet();
		status_set_zoom();
		status_set_unit();
		status_bar_visibility();
	}

	// ファイルメニューの「画像をインポートする...」が選択された.
	IAsyncAction MainPage::file_import_img_click(IInspectable const&, RoutedEventArgs const&)
	{
		winrt::apartment_context context;
		// ファイル「オープン」ピッカーを取得して開く.
		auto open_picker{ FileOpenPicker() };
		open_picker.FileTypeFilter().Append(DOT_BMP);
		open_picker.FileTypeFilter().Append(DOT_GIF);
		open_picker.FileTypeFilter().Append(DOT_JPG);
		open_picker.FileTypeFilter().Append(DOT_PNG);
		open_picker.FileTypeFilter().Append(DOT_TIF);
		// ピッカーを非同期に表示してストレージファイルを取得する.
		// (「閉じる」ボタンが押された場合ストレージファイルは nullptr.)
		auto open_file{ co_await open_picker.PickSingleFileAsync() };
		// ストレージファイルがヌルポインターか判定する.
		if (open_file != nullptr) {
			auto const& prev_cur = file_wait_cursor();
			unselect_all();

			const float win_w = static_cast<float>(scp_sheet_panel().ActualWidth());
			const float win_h = static_cast<float>(scp_sheet_panel().ActualHeight());
			const float win_x = static_cast<float>(sb_horz().Value());
			const float win_y = static_cast<FLOAT>(sb_vert().Value());
			co_await winrt::resume_background();

			auto stream{ co_await open_file.OpenAsync(FileAccessMode::Read) };
			auto decoder{ co_await BitmapDecoder::CreateAsync(stream) };
			auto bitmap{ SoftwareBitmap::Convert(co_await decoder .GetSoftwareBitmapAsync(), BitmapPixelFormat::Bgra8) };

			// 用紙の表示された部分の中心の位置を求める.
			const float scale = m_sheet_main.m_sheet_scale;
			const float img_w = static_cast<float>(bitmap.PixelWidth());
			const float img_h = static_cast<float>(bitmap.PixelHeight());
			const D2D1_POINT_2F center_pos{
				static_cast<FLOAT>((win_x + (win_w - img_w) * 0.5) / scale),
				static_cast<FLOAT>((win_y + (win_h - img_h) * 0.5) / scale)
			};
			const D2D1_SIZE_F view_size{ img_w, img_h };
			ShapeImage* img = new ShapeImage(center_pos, view_size, bitmap);
#if (_DEBUG)
			debug_leak_cnt++;
#endif
			bitmap.Close();
			bitmap = nullptr;
			decoder = nullptr;
			stream.Close();
			stream = nullptr;

			m_dx_mutex.lock();
			ustack_push_append(img);
			ustack_push_select(img);
			ustack_push_null();
			m_dx_mutex.unlock();
			co_await winrt::resume_foreground(Dispatcher());

			ustack_is_enable();
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_append(img);
				summary_select(img);
			}
			xcvd_is_enabled();
			sheet_update_bbox(img);
			sheet_panle_size();
			sheet_draw();

			// カーソルを元に戻す.
			Window::Current().CoreWindow().PointerCursor(prev_cur);
		}

		// スレッドコンテキストを復元する.
		co_await context;
	};

	// ファイルメニューの「開く...」が選択された
	IAsyncAction MainPage::file_open_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		// スタックに操作の組が積まれている, かつ確認ダイアログの応答が「キャンセル」か判定する.
		if (m_ustack_updt && !co_await ask_for_conf_async()) {
			co_return;
		}
		// 待機カーソルを表示, 表示する前のカーソルを得る.
		auto const& prev_cur = file_wait_cursor();
		// ファイル「オープン」ピッカーを取得して開く.
		auto open_picker{ FileOpenPicker() };
		open_picker.FileTypeFilter().Append(DOT_GPF);
		// ダブルクリックでファイルが選択された場合,
		// co_await が終了する前に, PonterReleased と PonterEntered が呼ばれる.
		// これはピッカーが 2 度目の Released を待たずにダブルクリックを成立させているためだと思われる.
		//scp_sheet_panel().PointerReleased(m_token_event_released);
		//scp_sheet_panel().PointerEntered(m_token_event_entered);

		// ピッカーを非同期で表示してストレージファイルを取得する.
		// (「閉じる」ボタンが押された場合ストレージファイルは nullptr.)
		auto open_file{ co_await open_picker.PickSingleFileAsync() };
		// ストレージファイルがヌルポインターか判定する.
		if (open_file != nullptr) {
			// ストレージファイルを非同期に読む.
			co_await file_read_async(open_file);
			// ストレージファイルを解放する.
			open_file = nullptr;
		}
		open_picker = nullptr;
		Window::Current().CoreWindow().PointerCursor(prev_cur);
	}

	// ストレージファイルを非同期に読む.
	// s_file	読み込むストレージファイル
	// suspend	中断したか判定する
	// sheet	シートのみ読み込む
	// 戻り値	読み込めたら S_OK.
	// 中断したなら, 操作スタックも保存する.
	IAsyncOperation<winrt::hresult> MainPage::file_read_async(StorageFile const& s_file, const bool suspend, const bool sheet) noexcept
	{
		auto hres = E_FAIL;
		winrt::apartment_context context;
		m_dx_mutex.lock();
		try {
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_close_click(nullptr, nullptr);
			}
			ustack_clear();
			slist_clear(m_list_shapes);

			const auto& ra_stream{ co_await s_file.OpenAsync(FileAccessMode::Read) };
			auto dt_reader{ DataReader(ra_stream.GetInputStreamAt(0)) };
			co_await dt_reader.LoadAsync(static_cast<uint32_t>(ra_stream.Size()));

			//tool_read(dt_reader);
			m_tool_draw = static_cast<DRAW_TOOL>(dt_reader.ReadUInt32());
			dt_read(m_tool_poly, dt_reader);
			//find_text_read(dt_reader);
			dt_read(m_find_text, dt_reader);
			dt_read(m_find_repl, dt_reader);
			uint16_t f_bit = dt_reader.ReadUInt16();
			m_edit_text_frame = ((f_bit & 1) != 0);
			m_find_text_case = ((f_bit & 2) != 0);
			m_find_text_wrap = ((f_bit & 4) != 0);

			m_misc_len_unit = static_cast<LEN_UNIT>(dt_reader.ReadUInt32());
			m_misc_color_code = static_cast<COLOR_CODE>(dt_reader.ReadUInt16());
			m_misc_vert_stick = dt_reader.ReadSingle();
			m_misc_status_bar = static_cast<STATUS_BAR>(dt_reader.ReadUInt16());
			m_image_keep_aspect = dt_reader.ReadBoolean();	// 画像の縦横比の維持

			const auto s_atom = dt_reader.ReadBoolean();
			m_summary_atomic.store(s_atom, std::memory_order_release);

			m_sheet_main.read(dt_reader);

			m_sheet_main.m_grid_base = max(m_sheet_main.m_grid_base, 0.0f);
			m_sheet_main.m_sheet_scale = min(max(m_sheet_main.m_sheet_scale, 0.25f), 4.0f);
			m_sheet_main.m_sheet_size.width = max(min(m_sheet_main.m_sheet_size.width, sheet_size_max()), 1.0F);
			m_sheet_main.m_sheet_size.height = max(min(m_sheet_main.m_sheet_size.height, sheet_size_max()), 1.0F);

#if defined(_DEBUG)
			if (debug_leak_cnt != 0) {
				co_await winrt::resume_foreground(Dispatcher());
				message_show(ICON_ALERT, DEBUG_MSG, {});
				co_await context;
				m_dx_mutex.unlock();
				co_return hres;
			}
#endif
			// シートのみ読み込むか判定する.
			if (sheet) {
				hres = S_OK;
			}
			else if (slist_read(m_list_shapes, dt_reader)) {
				// 中断されたか判定する.
				if (suspend) {
					ustack_read(dt_reader);
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
			co_await winrt::resume_foreground(Dispatcher());
			message_show(ICON_ALERT, ERR_READ, s_file.Path());
		}
		else if (!suspend && !sheet) {
			co_await winrt::resume_foreground(Dispatcher());
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
		// コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;

		// スレッドをメインページの UI スレッドに変える.
		//co_await winrt::resume_foreground(this->Dispatcher());
		// ストレージファイルにヌルを格納する.
		StorageFile s_file = nullptr;
		try {
			// トークンが空以外か判定する.
			if (!token.empty()) {
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

		// スタックに操作の組が積まれている, かつ確認ダイアログの応答が「キャンセル」か判定する.
		if (m_ustack_updt && !co_await ask_for_conf_async()) {
			co_return;
		}
		auto const prev_cur = file_wait_cursor();
		auto s_file{ co_await file_recent_get_async(item[0].Token) };
		// ストレージファイルが得られたか判定する.
		if (s_file != nullptr) {
			co_await file_read_async(s_file);
			s_file = nullptr;
		}
		else {
			// 取得できないならば,
			message_show(ICON_ALERT, ERR_RECENT, item[0].Metadata);
		}

		// ウィンドウのカーソルを復元する.
		Window::Current().CoreWindow().PointerCursor(prev_cur);
	}

	// 最近使ったファイルメニュを更新する.
	void MainPage::file_recent_update_menu(void)
	{
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
	// svg_allowed	SVG への保存を許す.
	IAsyncOperation<winrt::hresult> MainPage::file_save_as_async(const bool svg_allowed) noexcept
	{
		auto hres = E_FAIL;
		auto const& prev_cur = file_wait_cursor();	// 表示する前のカーソル
		// コルーチンの開始時のスレッドコンテキストを保存する.
		//winrt::apartment_context context;
		//co_await winrt::resume_background();
		try {
			// ファイル保存ピッカーを得る.
			// ファイルタイプに拡張子 GPF とその説明を追加する.
			auto s_picker{ FileSavePicker() };
			//auto const& r_loader = ResourceLoader::GetForCurrentView();
			const auto desc_gpf = ResourceLoader::GetForCurrentView().GetString(DESC_GPF);
			const auto type_gpf = winrt::single_threaded_vector<winrt::hstring>({ DOT_GPF });
			s_picker.FileTypeChoices().Insert(desc_gpf, type_gpf);
			const auto loc_id = PickerLocationId::DocumentsLibrary;
			s_picker.SuggestedStartLocation(loc_id);
			// SVG への保存を許すか判定する.
			if (svg_allowed) {
				// ファイルタイプに拡張子 SVG とその説明を追加する.
				const auto desc_svg = ResourceLoader::GetForCurrentView().GetString(DESC_SVG);
				const auto type_svg = winrt::single_threaded_vector<winrt::hstring>({ DOT_SVG });
				s_picker.FileTypeChoices().Insert(desc_svg, type_svg);
			}
			// 最近使ったファイルのトークンが空か判定する.
			if (m_file_token_mru.empty()) {
				// 提案されたファイル名に拡張子を格納する.
				s_picker.SuggestedFileName(DOT_GPF);
			}
			else {
				// ストレージファイルを最近使ったファイルのトークンから得る.
				auto s_file{ co_await file_recent_get_async(m_file_token_mru) };
				if (s_file != nullptr) {
					// ファイルタイプが DOT_GPF か判定する.
					if (s_file.FileType() == DOT_GPF) {
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
				if (s_file.FileType() == DOT_SVG) {
					//for (const auto s : m_list_shapes) {
					//	if (s->is_deleted() || typeid(*s) != typeid(ShapeImage)) {
					//		continue;
					//	}
					//	message_show(ICON_INFO, L"str_info_image_found", tx_find_text_what().Text());
					//	break;
					//}
					// 図形データを SVG としてストレージファイルに非同期に書き込み, 結果を得る.
					hres = co_await file_write_svg_async(s_file);
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
		// 結果が S_OK 以外か判定する.
		if (hres != S_OK) {
			// スレッドをメインページの UI スレッドに変える.
			// 最近使ったファイルのエラーメッセージダイアログを表示する.
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, ERR_WRITE, m_file_token_mru);
		}
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

	IAsyncOperation<winrt::hresult> MainPage::file_write_img_async(ShapeImage* s, const wchar_t suggested_name[], wchar_t img_name[], const size_t name_len)
	{
		// コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;
		FileSavePicker img_picker{ FileSavePicker() };
		const auto bmp_type = winrt::single_threaded_vector<winrt::hstring>({ L".bmp" });
		const auto gif_type = winrt::single_threaded_vector<winrt::hstring>({ L".gif" });
		const auto jpg_type = winrt::single_threaded_vector<winrt::hstring>({ L".jpg", L".jpeg" });
		const auto png_type = winrt::single_threaded_vector<winrt::hstring>({ L".png" });
		const auto tif_type = winrt::single_threaded_vector<winrt::hstring>({ L".tif", L".tiff" });
		// ResourceLoader::GetForCurrentView はフォアグラウンド.
		co_await winrt::resume_foreground(Dispatcher());
		const auto bmp_desc{ ResourceLoader::GetForCurrentView().GetString(L"str_desc_bmp") };
		const auto gif_desc{ ResourceLoader::GetForCurrentView().GetString(L"str_desc_gif") };
		const auto jpg_desc{ ResourceLoader::GetForCurrentView().GetString(L"str_desc_jpg") };
		const auto png_desc{ ResourceLoader::GetForCurrentView().GetString(L"str_desc_png") };
		const auto tif_desc{ ResourceLoader::GetForCurrentView().GetString(L"str_desc_tif") };
		img_picker.FileTypeChoices().Insert(bmp_desc, bmp_type);
		img_picker.FileTypeChoices().Insert(gif_desc, gif_type);
		img_picker.FileTypeChoices().Insert(jpg_desc, jpg_type);
		img_picker.FileTypeChoices().Insert(png_desc, png_type);
		img_picker.FileTypeChoices().Insert(tif_desc, tif_type);
		img_picker.SuggestedFileName(suggested_name);
		auto img_file{ co_await img_picker.PickSaveFileAsync() };
		if (img_file == nullptr) {
			co_await context;
			co_return E_FAIL;
		}
		// 保存するファイル形式
		winrt::guid enc_id;
		if (std::find(bmp_type.begin(), bmp_type.end(), img_file.FileType()) != bmp_type.end()) {
			enc_id = BitmapEncoder::BmpEncoderId();
		}
		else if (std::find(gif_type.begin(), gif_type.end(), img_file.FileType()) != gif_type.end()) {
		//if (img_file.FileType() == L".gif") {
			enc_id = BitmapEncoder::GifEncoderId();
		}
		else if (std::find(jpg_type.begin(), jpg_type.end(), img_file.FileType()) != jpg_type.end()) {
		//else if (img_file.FileType() == L".jpg") {
			enc_id = BitmapEncoder::JpegEncoderId();
		}
		else if (std::find(png_type.begin(), png_type.end(), img_file.FileType()) != png_type.end()) {
		//else if (img_file.FileType() == L".png") {
			enc_id = BitmapEncoder::PngEncoderId();
		}
		else if (std::find(tif_type.begin(), tif_type.end(), img_file.FileType()) != tif_type.end()) {
		//else if (img_file.FileType() == L".tif") {
			enc_id = BitmapEncoder::TiffEncoderId();
		}
		else {
			co_await context;
			co_return E_FAIL;
		}
		HRESULT hres = E_FAIL;
		co_await winrt::resume_background();
		CachedFileManager::DeferUpdates(img_file);
		IRandomAccessStream img_stream{ co_await img_file.OpenAsync(FileAccessMode::ReadWrite) };
		co_await s->copy_to(enc_id, img_stream);
		// 遅延させたファイル更新を完了し, 結果を判定する.
		if (co_await CachedFileManager::CompleteUpdatesAsync(img_file) == FileUpdateStatus::Complete) {
			hres = S_OK;
			wcscpy_s(img_name, name_len, img_file.Path().c_str());
		}
		img_stream.Close();
		img_stream = nullptr;
		img_file = nullptr;
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return hres;
	}

	// 図形データを SVG としてストレージファイルに非同期に書き込む.
	// file	書き込み先のファイル
	// 戻り値	書き込めた場合 S_OK
	IAsyncOperation<winrt::hresult> MainPage::file_write_svg_async(StorageFile const& s_file)
	{
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
			const auto& ra_stream{ co_await s_file.OpenAsync(FileAccessMode::ReadWrite) };
			// ランダムアクセスストリームの先頭からデータライターを作成する.
			auto dt_writer{ DataWriter(ra_stream.GetOutputStreamAt(0)) };
			// XML 宣言を書き込む.
			dt_write_svg(XML_DEC, dt_writer);
			// DOCTYPE を書き込む.
			dt_write_svg(DOCTYPE, dt_writer);
			// データライターに SVG 開始タグを書き込む.
			file_write_svg_tag(m_sheet_main.m_sheet_size, m_sheet_main.m_sheet_color, m_sheet_dx.m_logical_dpi, m_misc_len_unit, dt_writer);
			// 日時 (注釈) を書き込む.
			//char buf[64];
			//const auto t = time(nullptr);
			//struct tm tm;
			//localtime_s(&tm, &t);
			//strftime(buf, 64, "<!-- %m/%d/%Y %H:%M:%S -->" SVG_NEW_LINE, &tm);
			//dt_write_svg(buf, dt_writer);
			// 図形リストの各図形について以下を繰り返す.
			for (auto s : m_list_shapes) {
				if (s->is_deleted()) {
					continue;
				}
				// 図形が画像か判定する.
				if (typeid(*s) == typeid(ShapeImage)) {
					static uint32_t magic_num = 0;	// ミリ秒の代わり
					constexpr size_t MAX_LEN = 1024 - 8;
					wchar_t img_name[MAX_LEN + 8];
					const auto t = time(nullptr);
					struct tm tm;
					localtime_s(&tm, &t);
					
					swprintf(img_name, MAX_LEN, L"%s", s_file.Name().data());
					const auto dot_ptr = wcsrchr(img_name, L'.');
					const size_t dot_len = (dot_ptr != nullptr ? dot_ptr - img_name : wcslen(img_name));
					const size_t tail_len = dot_len + wcsftime(img_name + dot_len, MAX_LEN - dot_len, L"%Y%m%d%H%M%S", &tm);
					swprintf(img_name + tail_len, 8, L"%03d.bmp", magic_num++);
					//const auto s_folder{ co_await s_file.GetParentAsync() };
					const auto b = static_cast<ShapeImage*>(s);
					if (co_await file_write_img_async(b, img_name, img_name, MAX_LEN) == S_OK) {
						b->write_svg(img_name, dt_writer);
					}
					else {
						b->write_svg(dt_writer);
					}
				}
				else {
					s->write_svg(dt_writer);
				}
			}
			// SVG 終了タグを書き込む.
			dt_write_svg("</svg>" SVG_NEW_LINE, dt_writer);
			// ストリームの現在位置をストリームの大きさに格納する.
			ra_stream.Size(ra_stream.Position());
			// バッファ内のデータをストリームに出力する.
			co_await dt_writer.StoreAsync();
			// ストリームをフラッシュする.
			co_await ra_stream.FlushAsync();
			// 遅延させたファイル更新を完了し, 結果を判定する.
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
		// 結果が S_OK 以外か判定する.
		if (hres != S_OK) {
			// スレッドをメインページの UI スレッドに変える.
			co_await winrt::resume_foreground(Dispatcher());
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
	// suspend	中断されたか判定
	// layout	
	// 戻り値	書き込みに成功したら true
	IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async(StorageFile const& s_file, const bool suspend, const bool layout)
	{
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
			const auto& ra_stream{ co_await s_file.OpenAsync(FileAccessMode::ReadWrite) };
			auto dt_writer{ DataWriter(ra_stream.GetOutputStreamAt(0)) };

			//tool_write(dt_writer);
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_tool_draw));
			dt_write(m_tool_poly, dt_writer);
			//find_text_write(dt_writer);
			dt_write(m_find_text, dt_writer);
			dt_write(m_find_repl, dt_writer);
			uint16_t f_bit = 0;
			if (m_edit_text_frame) {
				f_bit |= 1;
			}
			if (m_find_text_case) {
				f_bit |= 2;
			}
			if (m_find_text_wrap) {
				f_bit |= 4;
			}
			dt_writer.WriteUInt16(f_bit);
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_misc_len_unit));
			dt_writer.WriteUInt16(static_cast<uint16_t>(m_misc_color_code));
			dt_writer.WriteSingle(m_misc_vert_stick);
			dt_writer.WriteUInt16(static_cast<uint16_t>(m_misc_status_bar));
			dt_writer.WriteBoolean(m_image_keep_aspect);

			dt_writer.WriteBoolean(summary_is_visible());

			m_sheet_main.write(dt_writer);
			if (suspend) {
				slist_write<!REDUCE>(m_list_shapes, dt_writer);
				ustack_write(dt_writer);
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
		// エラーが発生した場合, 
		catch (winrt::hresult_error const& err) {
			hres = err.code();
		}
		// 結果が S_OK 以外か判定する.
		if (hres != S_OK) {
			// スレッドをメインページの UI スレッドに変える.
			co_await winrt::resume_foreground(this->Dispatcher());
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, ERR_WRITE, s_file.Path());
		}
		// 中断されてない, かつレイアウト以外か判定する.
		else if (!suspend && !layout) {
			// スレッドをメインページの UI スレッドに変える.
			co_await winrt::resume_foreground(Dispatcher());
			// ストレージファイルを最近使ったファイルに登録する.
			// ここでエラーが出る.
			file_recent_add(s_file);
			// false をスタックが更新されたか判定に格納する.
			m_ustack_updt = false;
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return hres;
	}

	IAsyncAction MainPage::file_check_access(void) const
	{
		bool flag = false;
		try {
			co_await StorageFolder::GetFolderFromPathAsync(L"C:\\");
		}
		catch (winrt::hresult_error const&) {
			flag = true;
		}
		if (flag) {
			Uri ms_setting{ L"ms-settings:privacy-broadfilesystemaccess" };
			co_await Launcher::LaunchUriAsync(ms_setting);
		}
		co_return;
	}

}