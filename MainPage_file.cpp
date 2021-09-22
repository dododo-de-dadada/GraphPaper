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

	static CoreCursor const& CUR_WAIT = CoreCursor(CoreCursorType::Wait, 0);	// 待機カーソル.
	constexpr wchar_t DESC_GPF[] = L"str_desc_gpf";	// 拡張子 gpf の説明
	constexpr wchar_t DESC_SVG[] = L"str_desc_svg";	// 拡張子 svg の説明
	constexpr wchar_t ERR_FONT[] = L"str_err_font";	// 有効でない書体のエラーメッセージのリソース名
	constexpr wchar_t ERR_READ[] = L"str_err_read";	// 読み込みエラーメッセージのリソース名
	constexpr wchar_t ERR_RECENT[] = L"str_err_recent";	// 最近使ったファイルのエラーメッセージのリソース名
	constexpr wchar_t ERR_WRITE[] = L"str_err_write";	// 書き込みエラーメッセージのリソース名
	constexpr wchar_t EXT_BMP[] = L".bmp";	// 画像ファイルの拡張子
	constexpr wchar_t EXT_GIF[] = L".gif";	// 画像ファイルの拡張子
	constexpr wchar_t EXT_GPF[] = L".gpf";	// 図形データファイルの拡張子
	constexpr wchar_t EXT_JPEG[] = L".jpeg";	// 画像ファイルの拡張子
	constexpr wchar_t EXT_JPG[] = L".jpg";	// 画像ファイルの拡張子
	constexpr wchar_t EXT_PNG[] = L".png";	// 画像ファイルの拡張子
	constexpr wchar_t EXT_SVG[] = L".svg";	// SVG ファイルの拡張子
	constexpr wchar_t EXT_TIF[] = L".tif";	// 画像ファイルの拡張子
	constexpr wchar_t EXT_TIFF[] = L".tiff";	// 画像ファイルの拡張子
	constexpr uint32_t MRU_MAX = 25;	// 最近使ったリストの最大数.
	constexpr wchar_t UNTITLED[] = L"str_untitled";	// 無題のリソース名
	static const IVector<winrt::hstring> TYPE_BMP{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_BMP })
	};
	static const IVector<winrt::hstring> TYPE_GIF{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_GIF })
	};
	static const IVector<winrt::hstring> TYPE_JPG{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_JPG, EXT_JPEG })
	};
	static const IVector<winrt::hstring> TYPE_PNG{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_PNG })
	};
	static const IVector<winrt::hstring> TYPE_TIF{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_TIF, EXT_TIFF })
	};
	static const IVector<winrt::hstring> TYPE_GPF{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_GPF })
	};
	static const IVector<winrt::hstring> TYPE_SVG{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_SVG })
	};
	static winrt::guid enc_id = BitmapEncoder::BmpEncoderId();

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
		if (slist_test_font(m_sheet_main.m_list_shapes, unavailable_font) != true) {
			// 「無効な書体が使用されています」メッセージダイアログを表示する.
			message_show(ICON_ALERT, ERR_FONT, unavailable_font);
		}

		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			if (m_sheet_main.m_list_shapes.empty()) {
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
		FileOpenPicker open_picker{ FileOpenPicker() };
		open_picker.FileTypeFilter().Append(EXT_BMP);
		open_picker.FileTypeFilter().Append(EXT_GIF);
		open_picker.FileTypeFilter().Append(EXT_JPG);
		open_picker.FileTypeFilter().Append(EXT_JPEG);
		open_picker.FileTypeFilter().Append(EXT_PNG);
		open_picker.FileTypeFilter().Append(EXT_TIF);
		open_picker.FileTypeFilter().Append(EXT_TIFF);
		// ピッカーを非同期に表示してストレージファイルを取得する.
		// (「閉じる」ボタンが押された場合ストレージファイルは nullptr.)
		StorageFile open_file{ co_await open_picker.PickSingleFileAsync() };
		// ストレージファイルがヌルポインターか判定する.
		if (open_file != nullptr) {
			CoreCursor const& prev_cur = file_wait_cursor();
			unselect_all();

			const double win_w = scp_sheet_panel().ActualWidth();
			const double win_h = scp_sheet_panel().ActualHeight();
			const double win_x = sb_horz().Value();
			const double win_y = sb_vert().Value();
			co_await winrt::resume_background();

			IRandomAccessStream stream{ co_await open_file.OpenAsync(FileAccessMode::Read) };
			BitmapDecoder decoder{ co_await BitmapDecoder::CreateAsync(stream) };
			SoftwareBitmap bitmap{ SoftwareBitmap::Convert(co_await decoder.GetSoftwareBitmapAsync(), BitmapPixelFormat::Bgra8) };

			// 用紙の表示された部分の中心の位置を求める.
			const double scale = m_sheet_main.m_sheet_scale;
			const double img_w = bitmap.PixelWidth();
			const double img_h = bitmap.PixelHeight();
			const D2D1_POINT_2F center_pos{
				static_cast<FLOAT>((win_x + (win_w - img_w) * 0.5) / scale),
				static_cast<FLOAT>((win_y + (win_h - img_h) * 0.5) / scale)
			};
			const D2D1_SIZE_F view_size{ static_cast<FLOAT>(img_w), static_cast<FLOAT>(img_h) };
			ShapeImage* img = new ShapeImage(center_pos, view_size, bitmap, m_sheet_main.m_image_opac);
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
		CoreCursor const& prev_cur = file_wait_cursor();
		// ファイル「オープン」ピッカーを取得して開く.
		FileOpenPicker open_picker{ FileOpenPicker() };
		open_picker.FileTypeFilter().Append(EXT_GPF);
		// ダブルクリックでファイルが選択された場合,
		// co_await が終了する前に, PonterReleased と PonterEntered が呼ばれる.
		// これはピッカーが 2 度目の Released を待たずにダブルクリックを成立させているためだと思われる.
		//scp_sheet_panel().PointerReleased(m_token_event_released);
		//scp_sheet_panel().PointerEntered(m_token_event_entered);

		// ピッカーを非同期で表示してストレージファイルを取得する.
		// (「閉じる」ボタンが押された場合ストレージファイルは nullptr.)
		StorageFile open_file{ co_await open_picker.PickSingleFileAsync() };
		// ストレージファイルがヌルポインターか判定する.
		if (open_file != nullptr) {
			// ストレージファイルを非同期に読む.
			co_await file_read_async<false, false>(open_file);
			// ストレージファイルを解放する.
			open_file = nullptr;
		}
		open_picker = nullptr;
		Window::Current().CoreWindow().PointerCursor(prev_cur);
	}

	// ストレージファイルを非同期に読む.
	// SUSPEND	ライフサイクルが中断のとき true
	// SETTING	「設定を保存」のとき true
	// s_file	読み込むストレージファイル
	// 戻り値	読み込めたら S_OK.
	template <bool SUSPEND, bool SETTING>
	IAsyncOperation<winrt::hresult> MainPage::file_read_async(StorageFile s_file) noexcept
	{
		HRESULT ok = E_FAIL;
		winrt::apartment_context context;
		m_dx_mutex.lock();
		try {
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_close_click(nullptr, nullptr);
			}
			ustack_clear();
			slist_clear(m_sheet_main.m_list_shapes);

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
				co_return ok;
			}
#endif
			// シートのみ読み込むか判定する.
			if constexpr (SETTING) {
				ok = S_OK;
			}
			else {
				if (slist_read(m_sheet_main.m_list_shapes, dt_reader)) {
					// 中断されたか判定する.
					if constexpr (SUSPEND) {
						ustack_read(dt_reader);
					}
					ok = S_OK;
				}
			}
			dt_reader.Close();
			ra_stream.Close();
		}
		catch (winrt::hresult_error const& e) {
			ok = e.code();
		}
		m_dx_mutex.unlock();
		if (ok != S_OK) {
			co_await winrt::resume_foreground(Dispatcher());
			message_show(ICON_ALERT, ERR_READ, s_file.Path());
		}
		else {
			if constexpr (!SUSPEND && !SETTING) {
				co_await winrt::resume_foreground(Dispatcher());
				file_recent_add(s_file);
				file_finish_reading();
			}
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return ok;
	}
	template IAsyncOperation<winrt::hresult> MainPage::file_read_async<false, false>(StorageFile s_file) noexcept;
	template IAsyncOperation<winrt::hresult> MainPage::file_read_async<false, true>(StorageFile s_file) noexcept;
	template IAsyncOperation<winrt::hresult> MainPage::file_read_async<true, false>(StorageFile s_file) noexcept;

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
			co_await file_read_async<false, false>(s_file);
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
		HRESULT ok = E_FAIL;
		auto const& prev_cur = file_wait_cursor();	// 表示する前のカーソル
		// コルーチンの開始時のスレッドコンテキストを保存する.
		//winrt::apartment_context context;
		//co_await winrt::resume_background();
		try {
			// ファイル保存ピッカーを得る.
			// ファイルタイプに拡張子 GPF とその説明を追加する.
			FileSavePicker s_picker{
				FileSavePicker()
			};
			const winrt::hstring desc_gpf{
				ResourceLoader::GetForCurrentView().GetString(DESC_GPF)
			};
			s_picker.FileTypeChoices().Insert(desc_gpf, TYPE_GPF);
			const PickerLocationId loc_id = PickerLocationId::DocumentsLibrary;
			s_picker.SuggestedStartLocation(loc_id);
			// SVG への保存を許すか判定する.
			if (svg_allowed) {
				// ファイルタイプに拡張子 SVG とその説明を追加する.
				const winrt::hstring desc_svg{
					ResourceLoader::GetForCurrentView().GetString(DESC_SVG)
				};
				s_picker.FileTypeChoices().Insert(desc_svg, TYPE_SVG);
			}
			// 最近使ったファイルのトークンが空か判定する.
			if (m_file_token_mru.empty()) {
				// 提案されたファイル名に拡張子を格納する.
				s_picker.SuggestedFileName(EXT_GPF);
			}
			else {
				// ストレージファイルを最近使ったファイルのトークンから得る.
				auto s_file{ co_await file_recent_get_async(m_file_token_mru) };
				if (s_file != nullptr) {
					// ファイルタイプが EXT_GPF か判定する.
					if (s_file.FileType() == EXT_GPF) {
						// ファイル名を, 提案するファイル名に格納する.
						s_picker.SuggestedFileName(s_file.Name());
					}
					s_file = nullptr;
				}
			}
			// ファイル保存ピッカーを表示し, ストレージファイルを得る.
			StorageFile s_file{
				co_await s_picker.PickSaveFileAsync()
			};
			// ストレージファイルを取得したか判定する.
			if (s_file != nullptr) {
				// ファイルタイプが SVG か判定する.
				if (s_file.FileType() == EXT_SVG) {
					//for (const auto s : m_sheet_main.m_list_shapes) {
					//	if (s->is_deleted() || typeid(*s) != typeid(ShapeImage)) {
					//		continue;
					//	}
					//	message_show(ICON_INFO, L"str_info_image_found", tx_find_text_what().Text());
					//	break;
					//}
					// 図形データを SVG としてストレージファイルに非同期に書き込み, 結果を得る.
					ok = co_await file_write_svg_async(s_file);
				}
				else {
					// 図形データをストレージファイルに非同期に書き込み, 結果を得る.
					ok = co_await file_write_gpf_async<false, false>(s_file);
				}
				// ストレージファイルを破棄する.
				s_file = nullptr;
			}
			// ピッカーを破棄する.
			s_picker = nullptr;
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, エラーコードを結果に格納する.
			ok = e.code();
		}
		// スレッドをメインページの UI スレッドに変える.
		//co_await winrt::resume_foreground(Dispatcher());
		// ウィンドウのカーソルを復元する.
		Window::Current().CoreWindow().PointerCursor(prev_cur);
		// スレッドコンテキストを復元する.
		//co_await context;
		// 結果を返し終了する.
		co_return ok;
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
		StorageFile s_file{
			co_await file_recent_get_async(m_file_token_mru)
		};
		if (s_file == nullptr) {
			// ストレージファイルが空の場合,
			constexpr bool SVG_ALLOWED = true;
			// 名前を付けてファイルに非同期に保存する
			co_return co_await file_save_as_async(!SVG_ALLOWED);
		}

		HRESULT ok = E_FAIL;	// 結果
		// 待機カーソルを表示, 表示する前のカーソルを得る.
		CoreCursor const& prev_cur = file_wait_cursor();
		// 図形データをストレージファイルに非同期に書き込み, 結果を得る.
		ok = co_await file_write_gpf_async<false, false>(s_file);
		// 結果が S_OK 以外か判定する.
		if (ok != S_OK) {
			// スレッドをメインページの UI スレッドに変える.
			// 最近使ったファイルのエラーメッセージダイアログを表示する.
			co_await winrt::resume_foreground(Dispatcher());
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, ERR_WRITE, m_file_token_mru);
		}
		// ウィンドウのカーソルを復元する.
		Window::Current().CoreWindow().PointerCursor(prev_cur);
		// 結果を返し終了する.
		co_return ok;
	}

	// ファイルメニューの「上書き保存」が選択された
	void MainPage::file_save_click(IInspectable const&, RoutedEventArgs const&)
	{
		auto _{ file_save_async() };
	}

	// ファイルに画像図形の画像を保存する.
	// s	画像図形
	// sug_name	ファイルセーブピッカーに渡されるファイル名
	// img_name	ファイルセーブピッカーで選択されたファイル名
	IAsyncOperation<winrt::hresult> MainPage::file_save_img_async(ShapeImage* s, const wchar_t sug_name[], wchar_t img_name[], const size_t name_len)
	{
		// コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;
		FileSavePicker img_picker{
			FileSavePicker()
		};
		// ResourceLoader::GetForCurrentView はフォアグラウンド.
		co_await winrt::resume_foreground(Dispatcher());
		const ResourceLoader& res_loader = ResourceLoader::GetForCurrentView();
		const winrt::hstring desc_bmp{
			res_loader.GetString(L"str_desc_bmp")
		};
		const winrt::hstring desc_gif{
			res_loader.GetString(L"str_desc_gif")
		};
		const winrt::hstring desc_jpg{
			res_loader.GetString(L"str_desc_jpg")
		};
		const winrt::hstring desc_png{
			res_loader.GetString(L"str_desc_png")
		};
		const winrt::hstring desc_tif{
			res_loader.GetString(L"str_desc_tif")
		};
		if (enc_id == BitmapEncoder::GifEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_gif, TYPE_GIF);
		}
		else if (enc_id == BitmapEncoder::JpegEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_jpg, TYPE_JPG);
		}
		else if (enc_id == BitmapEncoder::PngEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_png, TYPE_PNG);
		}
		else if (enc_id == BitmapEncoder::TiffEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_tif, TYPE_TIF);
		}
		else {
			img_picker.FileTypeChoices().Insert(desc_bmp, TYPE_BMP);
			enc_id = BitmapEncoder::BmpEncoderId();
		}
		if (enc_id != BitmapEncoder::BmpEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_bmp, TYPE_BMP);
		}
		if (enc_id != BitmapEncoder::GifEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_gif, TYPE_GIF);
		}
		if (enc_id != BitmapEncoder::JpegEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_jpg, TYPE_JPG);
		}
		if (enc_id != BitmapEncoder::PngEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_png, TYPE_PNG);
		}
		if (enc_id != BitmapEncoder::TiffEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_tif, TYPE_TIF);
		}
		img_picker.SuggestedFileName(sug_name);
		StorageFile img_file{
			co_await img_picker.PickSaveFileAsync()
		};
		if (img_file == nullptr) {
			co_await context;
			co_return E_FAIL;
		}
		// 保存するファイル形式
		if (std::find(TYPE_BMP.begin(), TYPE_BMP.end(), img_file.FileType()) != TYPE_BMP.end()) {
			enc_id = BitmapEncoder::BmpEncoderId();
		}
		else if (std::find(TYPE_GIF.begin(), TYPE_GIF.end(), img_file.FileType()) != TYPE_GIF.end()) {
			enc_id = BitmapEncoder::GifEncoderId();
		}
		else if (std::find(TYPE_JPG.begin(), TYPE_JPG.end(), img_file.FileType()) != TYPE_JPG.end()) {
			enc_id = BitmapEncoder::JpegEncoderId();
		}
		else if (std::find(TYPE_PNG.begin(), TYPE_PNG.end(), img_file.FileType()) != TYPE_PNG.end()) {
			enc_id = BitmapEncoder::PngEncoderId();
		}
		else if (std::find(TYPE_TIF.begin(), TYPE_TIF.end(), img_file.FileType()) != TYPE_TIF.end()) {
			enc_id = BitmapEncoder::TiffEncoderId();
		}
		else {
			co_await context;
			co_return E_FAIL;
		}
		HRESULT ok = E_FAIL;
		co_await winrt::resume_background();
		CachedFileManager::DeferUpdates(img_file);
		IRandomAccessStream img_stream{
			co_await img_file.OpenAsync(FileAccessMode::ReadWrite)
		};
		co_await s->copy_to(enc_id, img_stream);
		// 遅延させたファイル更新を完了し, 結果を判定する.
		if (co_await CachedFileManager::CompleteUpdatesAsync(img_file) == FileUpdateStatus::Complete) {
			ok = S_OK;
			wcscpy_s(img_name, name_len, img_file.Path().c_str());
		}
		img_stream.Close();
		img_stream = nullptr;
		img_file = nullptr;
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return ok;
	}

	// 待機カーソルを表示, 表示する前のカーソルを得る.
	CoreCursor MainPage::file_wait_cursor(void) const
	{
		CoreWindow const& c_win = Window::Current().CoreWindow();
		CoreCursor prev_cur = c_win.PointerCursor();
		c_win.PointerCursor(CUR_WAIT);
		return prev_cur;
	}

	// 図形データをストレージファイルに非同期に書き込む.
	// SUSPEND	ライフサイクルが中断のとき true
	// SETTING	「設定を保存」のとき true
	// s_file	ストレージファイル
	// 戻り値	書き込みに成功したら true
	template <bool SUSPEND, bool SETTING>
	IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async(StorageFile s_file)
	{
		constexpr auto REDUCE = true;

		// E_FAIL を結果に格納する.
		HRESULT ok = E_FAIL;
		// コルーチンの開始時のスレッドコンテキストを保存し
		winrt::apartment_context context;
		// スレッドをバックグラウンドに変える.
		co_await winrt::resume_background();
		try {
			// ファイル更新の遅延を設定する.
			// ストレージファイル->ランダムアクセスストリーム->データライターを作成する.
			CachedFileManager::DeferUpdates(s_file);
			const IRandomAccessStream& ra_stream{
				co_await s_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			DataWriter dt_writer{
				DataWriter(ra_stream.GetOutputStreamAt(0))
			};
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_tool_draw));
			dt_write(m_tool_poly, dt_writer);
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
			if constexpr (SUSPEND) {
				// 消去された図形も含めて書き込む.
				slist_write<!REDUCE>(m_sheet_main.m_list_shapes, dt_writer);
				ustack_write(dt_writer);
			}
			else if constexpr (!SETTING) {
				// 消去された図形は省いて書き込む.
				slist_write<REDUCE>(m_sheet_main.m_list_shapes, dt_writer);
			}
			ra_stream.Size(ra_stream.Position());
			co_await dt_writer.StoreAsync();
			co_await ra_stream.FlushAsync();
			// データライターを閉じる.
			dt_writer.Close();
			dt_writer = nullptr;
			// ストリームを閉じる.
			ra_stream.Close();
			// 遅延させたファイル更新を完了する.
			if (co_await CachedFileManager::CompleteUpdatesAsync(s_file) == FileUpdateStatus::Complete) {
				// 完了した場合, 
				// S_OK を結果に格納する.
				ok = S_OK;
			}
		}
		// エラーが発生した場合, 
		catch (winrt::hresult_error const& err) {
			ok = err.code();
		}
		// 結果が S_OK 以外か判定する.
		if (ok != S_OK) {
			// スレッドをメインページの UI スレッドに変える.
			co_await winrt::resume_foreground(Dispatcher());
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, ERR_WRITE, s_file.Path());
		}
		else {
			// 中断ではない, かつ設定ではないか判定する.
			if constexpr (!SUSPEND && !SETTING) {
				// スレッドをメインページの UI スレッドに変える.
				co_await winrt::resume_foreground(Dispatcher());
				// ストレージファイルを最近使ったファイルに登録する.
				// ここでエラーが出る.
				file_recent_add(s_file);
				// false をスタックが更新されたか判定に格納する.
				m_ustack_updt = false;
			}
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return ok;
	}
	template IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async<false, false>(StorageFile s_file);
	template IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async<true, false>(StorageFile s_file);
	template IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async<false, true>(StorageFile s_file);

	// 図形データを SVG としてストレージファイルに非同期に書き込む.
	// file	書き込み先のファイル
	// 戻り値	書き込めた場合 S_OK
	IAsyncOperation<winrt::hresult> MainPage::file_write_svg_async(StorageFile s_file)
	{
		constexpr char XML_DEC[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" SVG_NEW_LINE;
		constexpr char DOCTYPE[] = "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" SVG_NEW_LINE;
		HRESULT ok = E_FAIL;
		// コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// スレッドをバックグラウンドに変える.
		co_await winrt::resume_background();
		try {
			// ファイル更新の遅延を設定する.
			CachedFileManager::DeferUpdates(s_file);
			// ストレージファイルを開いてランダムアクセスストリームを得る.
			const IRandomAccessStream& ra_stream{
				co_await s_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			// ランダムアクセスストリームの先頭からデータライターを作成する.
			DataWriter dt_writer{
				DataWriter(ra_stream.GetOutputStreamAt(0))
			};
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
			for (auto s : m_sheet_main.m_list_shapes) {
				if (s->is_deleted()) {
					continue;
				}
				// 図形が画像か判定する.
				if (typeid(*s) == typeid(ShapeImage)) {
					static uint32_t magic_num = 0;	// ミリ秒の代わり
					constexpr size_t MAX_LEN = 1024 - 8;
					wchar_t img_name[MAX_LEN + 8];
					const time_t t = time(nullptr);
					struct tm tm;
					localtime_s(&tm, &t);

					swprintf(img_name, MAX_LEN, L"%s", s_file.Name().data());
					const wchar_t* const dot_ptr = wcsrchr(img_name, L'.');
					const size_t dot_len = (dot_ptr != nullptr ? dot_ptr - img_name : wcslen(img_name));
					const size_t tail_len = dot_len + wcsftime(img_name + dot_len, MAX_LEN - dot_len, L"%Y%m%d%H%M%S", &tm);
					swprintf(img_name + tail_len, 8, L"%03d", magic_num++);
					//const auto s_folder{ co_await s_file.GetParentAsync() };
					ShapeImage* const img = static_cast<ShapeImage*>(s);
					if (co_await file_save_img_async(img, img_name, img_name, MAX_LEN) == S_OK) {
						img->write_svg(img_name, dt_writer);
					}
					else {
						img->write_svg(dt_writer);
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
			if (co_await CachedFileManager::CompleteUpdatesAsync(s_file) == FileUpdateStatus::Complete) {
				// 完了した場合, S_OK を結果に格納する.
				ok = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, エラーコードを結果に格納する.
			ok = e.code();
		}
		// 結果が S_OK 以外か判定する.
		if (ok != S_OK) {
			// スレッドをメインページの UI スレッドに変える.
			co_await winrt::resume_foreground(Dispatcher());
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, ERR_WRITE, s_file.Path());
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return ok;
	}

	// ファイルシステムへのアクセス権を確認して, 設定を促す.
	/*
	IAsyncAction MainPage::file_check_broad_access(void) const
	{
		bool err = false;
		try {
			co_await StorageFolder::GetFolderFromPathAsync(L"C:\\");
		}
		catch (winrt::hresult_error const&) {
			err = true;
		}
		if (err) {
			Uri ms_setting{ L"ms-settings:privacy-broadfilesystemaccess" };
			co_await Launcher::LaunchUriAsync(ms_setting);
		}
		co_return;
	}
	*/

}