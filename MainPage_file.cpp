//-------------------------------
// MainPage_file.cpp
// ファイルの読み書き
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;
/*
* ファイル関係の非同期の関数は, UI スレッドで実行されることを前提としている.
* UI スレッド以外が呼び出すと失敗する関数がある.

file_confirm_dialog
	+---file_save_click_async

file_exit_click_async
	+---file_confirm_dialog

file_export_as_click_async
	+---export_as_pdf_async (MainPage_export.cpp)
	+---export_as_svg_async (MainPage_export.cpp)
	+---export_as_raster_async (MainPage_export.cpp)

file_finish_reading

file_import_image_click_async
	+---file_wait_cursor

file_new_click_async
	+---file_confirm_dialog
	+---file_recent_add
	+---file_finish_reading

file_open_click_async
	+---file_confirm_dialog
	+---file_wait_cursor
	+---file_read_async

file_read_async
	+---file_recent_add
	+---file_finish_reading

file_recent_add
	+---file_recent_menu_update

file_recent_click_async
	+---file_confirm_dialog
	+---file_recent_token_async
	+---file_wait_cursor

file_recent_token_async
	+---file_recent_menu_update

file_recent_menu_update

file_save_as_click_async
	+---file_recent_token_async
	+---file_wait_cursor
	+---file_write_gpf_async

file_save_click_async
	+---file_recent_token_async
	+---file_save_as_click_async
	+---file_wait_cursor
	+---file_write_gpf_async

file_write_gpf_async
	+---file_recent_add

*/
namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Application;
	using winrt::Windows::Storage::AccessCache::AccessListEntry;
	using winrt::Windows::UI::ViewManagement::ApplicationView;
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapBufferAccessMode;
	using winrt::Windows::Graphics::Imaging::BitmapDecoder;
	using winrt::Windows::Graphics::Imaging::BitmapInterpolationMode;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::Graphics::Imaging::BitmapRotation;
	using winrt::Windows::Storage::CachedFileManager;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Core::CoreCursor;
	using winrt::Windows::UI::Core::CoreCursorType;
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Core::Preview::SystemNavigationManagerPreview;
	using winrt::Windows::Foundation::Collections::IVector;
	using winrt::Windows::Foundation::Uri;
	using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::Storage::Pickers::FileOpenPicker;
	using winrt::Windows::Storage::Pickers::FileSavePicker;
	using winrt::Windows::Storage::Pickers::PickerLocationId;
	using winrt::Windows::Storage::Provider::FileUpdateStatus;
	using winrt::Windows::Storage::StorageFolder;
	using winrt::Windows::System::Launcher;
	using winrt::Windows::UI::Xaml::Window;
	using winrt::Windows::Storage::ApplicationData;

	static const CoreCursor& CURS_WAIT = CoreCursor(CoreCursorType::Wait, 0);	// 待機カーソル.
	constexpr static uint32_t MRU_MAX = 25;	// 最近使ったリストの最大数.
	static const IVector<winrt::hstring> TYPE_BMP {
		winrt::single_threaded_vector<winrt::hstring>({ L".bmp" })
	};
	static const IVector<winrt::hstring> TYPE_GIF{
		winrt::single_threaded_vector<winrt::hstring>({ L".gif" })
	};
	static const IVector<winrt::hstring> TYPE_JPG{
		winrt::single_threaded_vector<winrt::hstring>({ L".jpg", L".jpeg" })
	};
	static const IVector<winrt::hstring> TYPE_PNG{
		winrt::single_threaded_vector<winrt::hstring>({ L".png" })
	};
	static const IVector<winrt::hstring> TYPE_TIF{
		winrt::single_threaded_vector<winrt::hstring>({ L".tif", L".tiff" })
	};
	static const IVector<winrt::hstring> TYPE_GPF{
		winrt::single_threaded_vector<winrt::hstring>({ L".gpf" })
	};
	static const IVector<winrt::hstring> TYPE_SVG{
		winrt::single_threaded_vector<winrt::hstring>({ L".svg" })
	};
	static const IVector<winrt::hstring> TYPE_PDF{
		winrt::single_threaded_vector<winrt::hstring>({ L".pdf" })
	};

	inline static CoreCursor file_wait_cursor(void);

	//-------------------------------
	// 待機カーソルを表示, 表示する前のカーソルを得る.
	//-------------------------------
	inline static CoreCursor file_wait_cursor(void)
	{
		CoreWindow const& core_win = Window::Current().CoreWindow();
		CoreCursor const& prev_cur = core_win.PointerCursor();
		if (prev_cur.Type() != CURS_WAIT.Type()) {
			core_win.PointerCursor(CURS_WAIT);
		}
		return prev_cur;
	}

	//-------------------------------
	// ファイルへの更新を確認する.
	// 戻り値	応答が「保存する」ならファイルに書き込んで true を,
	//			応答が「保存しない」なら何もせずに true を,
	//			応答が「キャンセル」なら false を返す.
	//-------------------------------
	IAsyncOperation<bool> MainPage::file_confirm_dialog(void)
	{
		// 確認ダイアログを表示し, 応答を得る.
		const ContentDialogResult d_res{	// 押されたボタン
			co_await cd_conf_save_dialog().ShowAsync()
		};

		// 応答が「保存する」か判定する.
		if (d_res == ContentDialogResult::Primary) {
			// ファイルに非同期に保存.
			// 保存に失敗しても, true を返す.
			co_await file_save_click_async(nullptr, nullptr);
			co_return true;
		}
		// 応答が「保存しない」か判定する.
		else if (d_res == ContentDialogResult::Secondary) {
			co_return true;
		}
		// 応答が「キャンセル」(上記以外) なら false を返す.
		co_return false;
	}

	//-------------------------------
	// ファイルメニューの「終了」が選択された
	//-------------------------------
	IAsyncAction MainPage::file_exit_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		// UWP の Windows10 では, ウィンドウのタイトルバーを変更することはできない.
		// 閉じるボタンの可否を変えることはできないので, 排他処理を行なう.
		// ちなみに Windows11 では, 「タイトル バーのカスタマイズ」を参照.
		static std::mutex recursion{};	// 終了の排他処理
		if (!recursion.try_lock()) {
			co_return;
		}

		// コンテキストメニューが開いているなら閉じる.
		if (m_menu_fill != nullptr && m_menu_fill.IsOpen()) {
			m_menu_fill.Hide();
			ContextFlyout(nullptr);
		}
		else if (m_menu_font != nullptr && m_menu_font.IsOpen()) {
			m_menu_font.Hide();
			ContextFlyout(nullptr);
		}
		else if (m_menu_image != nullptr && m_menu_image.IsOpen()) {
			m_menu_image.Hide();
			ContextFlyout(nullptr);
		}
		else if (m_menu_ruler != nullptr && m_menu_ruler.IsOpen()) {
			m_menu_ruler.Hide();
			ContextFlyout(nullptr);
		}
		else if (m_menu_page != nullptr && m_menu_page.IsOpen()) {
			m_menu_page.Hide();
			ContextFlyout(nullptr);
		}
		else if (m_menu_stroke != nullptr && m_menu_stroke.IsOpen()) {
			m_menu_stroke.Hide();
			ContextFlyout(nullptr);
		}
		else if (m_menu_ungroup != nullptr && m_menu_ungroup.IsOpen()) {
			m_menu_ungroup.Hide();
			ContextFlyout(nullptr);
		}

		// コンテキストダイアログを閉じる.
		cd_conf_save_dialog().Hide();
		cd_edit_text_dialog().Hide();
		cd_message_dialog().Hide();
		cd_misc_vert_stick().Hide();
		cd_setting_dialog().Hide();
		cd_page_size_dialog().Hide();

		// スタックが更新された, かつ確認ダイアログの応答が「キャンセル」か判定する.
		if (m_ustack_is_changed && !co_await file_confirm_dialog()) {
			// 「キャンセル」なら, 排他処理を解除して中断する.
			recursion.unlock();
			co_return;
		}

		// 保存ピッカーで「保存」を押し, ピッカーがストレージファイルを返すまでの間に,
		// 「終了」を押すことができる.
		// ファイルの書き込みが終わるまでブロックする.
		while (!m_mutex_exit.try_lock()) {
#ifdef _DEBUG
			__debugbreak();
#endif // _DEBUG
		}
		m_mutex_exit.unlock();

		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_close_click(nullptr, nullptr);
		}
		// 静的リソースから読み込んだコンテキストメニューを破棄する.
		{
			m_menu_stroke = nullptr;
			m_menu_fill = nullptr;
			m_menu_font = nullptr;
			m_menu_page = nullptr;
			m_menu_ruler = nullptr;
			m_menu_image = nullptr;
			m_menu_ungroup = nullptr;
		}

		// コードビハインドで設定したハンドラーの設定を解除する.
		{
			auto const& app{ Application::Current() };
			app.Suspending(m_token_suspending);
			app.Resuming(m_token_resuming);
			app.EnteredBackground(m_token_entered_background);
			app.LeavingBackground(m_token_leaving_background);
			auto const& thread{ CoreWindow::GetForCurrentThread() };
			thread.Activated(m_token_activated);
			thread.VisibilityChanged(m_token_visibility_changed);
			//auto const& disp{ DisplayInformation::GetForCurrentView() };
			//disp.DpiChanged(m_token_dpi_changed);
			//disp.OrientationChanged(m_token_orientation_changed);
			//disp.DisplayContentsInvalidated(m_token_contents_invalidated);
			SystemNavigationManagerPreview::GetForCurrentView().CloseRequested(m_token_close_requested);
		}

		// DirectX のオブジェクトを破棄する.
		{
			// ウィンドウに他のコントロールを表示していた場合 (例えばリストビュー),
			// この後, スワップチェーンパネルの SizeChanged が呼び出されてしまう.
			// その時, 描画処理しないよう排他制御をロックする.
			// winrt::com_ptr で確保されたオブジェクトは Release するとエラーになる.
			// 例えば, m_main_page.m_state_block->Release();
			m_mutex_draw.lock();
			if (m_main_page.m_state_block != nullptr) {
				m_main_page.m_state_block = nullptr;
			}
			if (m_main_page.m_color_brush != nullptr) {
				m_main_page.m_color_brush = nullptr;
			}
			if (m_main_page.m_range_brush != nullptr) {
				m_main_page.m_range_brush = nullptr;
			}
			m_main_d2d.Trim();
			if (m_dialog_page.m_state_block != nullptr) {
				m_dialog_page.m_state_block = nullptr;
			}
			if (m_dialog_page.m_color_brush != nullptr) {
				m_dialog_page.m_color_brush = nullptr;
			}
			if (m_dialog_page.m_range_brush != nullptr) {
				m_dialog_page.m_range_brush = nullptr;
			}
			m_dialog_d2d.Trim();
		}

		ustack_clear();
		slist_clear(m_main_page.m_shape_list);
		slist_clear(m_dialog_page.m_shape_list);
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			message_show(ICON_DEBUG, DEBUG_MSG, {});
		}
#endif
		ShapeText::release_available_fonts();

		// アプリケーションを終了する.
		Application::Current().Exit();
	}

	//------------------------------
	// ファイルメニューの「他の形式としてエクスポートする」が選択された
	//------------------------------
	IAsyncAction MainPage::file_export_as_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_mutex_event.lock();

		// ResourceLoader::GetForCurrentView はフォアグラウンド.
		const ResourceLoader& res_loader = ResourceLoader::GetForCurrentView();
		const winrt::hstring desc_bmp{ res_loader.GetString(L"str_desc_bmp") };
		const winrt::hstring desc_gif{ res_loader.GetString(L"str_desc_gif") };
		const winrt::hstring desc_jpg{ res_loader.GetString(L"str_desc_jpg") };
		const winrt::hstring desc_pdf{ res_loader.GetString(L"str_desc_pdf") };
		const winrt::hstring desc_png{ res_loader.GetString(L"str_desc_png") };
		const winrt::hstring desc_svg{ res_loader.GetString(L"str_desc_svg") };
		const winrt::hstring desc_tif{ res_loader.GetString(L"str_desc_tif") };

		FileSavePicker image_picker{ 
			FileSavePicker()
		};
		image_picker.FileTypeChoices().Insert(desc_svg, TYPE_SVG);
		image_picker.FileTypeChoices().Insert(desc_pdf, TYPE_PDF);
		image_picker.FileTypeChoices().Insert(desc_bmp, TYPE_BMP);
		image_picker.FileTypeChoices().Insert(desc_gif, TYPE_GIF);
		image_picker.FileTypeChoices().Insert(desc_jpg, TYPE_JPG);
		image_picker.FileTypeChoices().Insert(desc_png, TYPE_PNG);
		image_picker.FileTypeChoices().Insert(desc_tif, TYPE_TIF);

		// 画像ライブラリーを保管場所に設定する.
		const PickerLocationId loc_id = PickerLocationId::PicturesLibrary;
		image_picker.SuggestedStartLocation(loc_id);

		// 最近使ったファイルのトークンからストレージファイルを得る.
		StorageFile recent_file{
			co_await file_recent_token_async(m_file_token_mru)
		};
		// ストレージファイルを得たなら,
		if (recent_file != nullptr) {
			// かつ, それが方眼紙ファイルなら,
			if (recent_file.FileType() == L".gpf") {
				// ファイル名を, 提案するファイル名に格納する.
				auto sug_name = recent_file.DisplayName();
				image_picker.SuggestedFileName(sug_name);
			}
			recent_file = nullptr;
		}

		// ピッカーを表示しストレージファイルを得る.
		StorageFile image_file{
			co_await image_picker.PickSaveFileAsync()
		};

		if (image_file != nullptr) {
			// 待機カーソルを表示, 表示する前のカーソルを得る.
			const CoreCursor& prev_cur = file_wait_cursor();
			HRESULT hr;
			// ファイル更新の遅延を設定する.
			CachedFileManager::DeferUpdates(image_file);

			// SVG ファイル
			if (image_file.ContentType() == L"image/svg+xml") {
				hr = co_await export_as_svg_async(image_file);
			}
			// PDF ファイル
			else if (image_file.ContentType() == L"application/pdf") {
				hr = co_await export_as_pdf_async(image_file);
			}
			// ラスターファイル
			else {
				hr = co_await export_as_raster_async(image_file);
			}
			// 遅延させたファイル更新を完了し, 結果を判定する.
			if (co_await CachedFileManager::CompleteUpdatesAsync(image_file) != 
				FileUpdateStatus::Complete) {
				// 完了しなかったなら E_FAIL.
				hr = E_FAIL;
			}

			// カーソルを元に戻す.
			Window::Current().CoreWindow().PointerCursor(prev_cur);
			// 結果が S_OK 以外か判定する.
			if (hr != S_OK) {
				// 「ファイルの書き込みに失敗しました」メッセージダイアログを表示する.
				message_show(ICON_ALERT, L"str_err_write", image_file.Path());
			}

		}
		m_mutex_event.unlock();
	}

	//-------------------------------
	// ファイルの読み込みが終了した.
	//-------------------------------
	void MainPage::file_finish_reading(void)
	{
		xcvd_is_enabled();

		drawing_tool_is_checked(m_drawing_tool);
		drawing_poly_opt_is_checked(m_drawing_poly_opt);
		color_code_is_checked(m_color_code);
		status_bar_is_checked(m_status_bar);
		len_unit_is_checked(m_len_unit);
		image_keep_aspect_is_checked(m_image_keep_aspect);
		
		page_setting_is_checked();

		wchar_t* unavailable_font;	// 無効な書体名
		if (!slist_test_avaiable_font(m_main_page.m_shape_list, unavailable_font)) {
			// 「無効な書体が使用されています」メッセージダイアログを表示する.
			message_show(ICON_ALERT, L"str_err_font", unavailable_font);
		}

		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			if (m_main_page.m_shape_list.empty()) {
				summary_close_click(nullptr, nullptr);
			}
			else if (lv_summary_list() != nullptr) {
				summary_remake();
			}
			else {
				// リソースから図形の一覧パネルを見つける.
				auto _{
					FindName(L"gd_summary_panel")
				};
				gd_summary_panel().Visibility(Visibility::Visible);
			}
		}

		page_bbox_update();
		page_panel_size();
		page_draw();
		status_bar_set_pos();
		status_bar_set_draw();
		status_bar_set_grid();
		status_bar_set_page();
		status_bar_set_zoom();
		status_bar_set_unit();
	}

	//-------------------------------
	// ファイルメニューの「画像をインポートする...」が選択された.
	//-------------------------------
	IAsyncAction MainPage::file_import_image_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		// co_await してるにもかかわらず, ファイル開くピッカーが返値を戻すまで時間がかかる.
		// その間フォアグランドのスレッドが動作してしまう.
		m_mutex_event.lock();

		// ファイルオープンピッカーを取得して開く.
		FileOpenPicker open_picker{ FileOpenPicker() };
		open_picker.FileTypeFilter().Append(L".bmp");
		open_picker.FileTypeFilter().Append(L".gif");
		open_picker.FileTypeFilter().Append(L".jpg");
		open_picker.FileTypeFilter().Append(L".jpeg");
		open_picker.FileTypeFilter().Append(L".png");
		open_picker.FileTypeFilter().Append(L".tif");
		open_picker.FileTypeFilter().Append(L".tiff");

		// ピッカーを非同期に表示してストレージファイルを取得する.
		// (「閉じる」ボタンが押された場合ストレージファイルは nullptr.)
		StorageFile open_file{
			co_await open_picker.PickSingleFileAsync()
		};
		open_picker = nullptr;
		// ストレージファイルがヌルポインターか判定する.
		if (open_file != nullptr) {
			// 待機カーソルを表示, 表示する前のカーソルを得る.
			const CoreCursor& prev_cur = file_wait_cursor();
			unselect_all();

			IRandomAccessStream stream{ co_await open_file.OpenAsync(FileAccessMode::Read) };
			BitmapDecoder decoder{ co_await BitmapDecoder::CreateAsync(stream) };
			SoftwareBitmap bitmap{ SoftwareBitmap::Convert(co_await decoder.GetSoftwareBitmapAsync(), BitmapPixelFormat::Bgra8) };

			// 表示された部分の中心の位置を求める.
			const double scale = m_main_page.m_page_scale;
			const double win_x = sb_horz().Value();
			const double win_y = sb_vert().Value();
			const double win_w = scp_page_panel().ActualWidth();
			const double win_h = scp_page_panel().ActualHeight();
			const double image_w = bitmap.PixelWidth();
			const double image_h = bitmap.PixelHeight();
			const D2D1_POINT_2F center_pos{
				static_cast<FLOAT>((win_x + (win_w - image_w) * 0.5) / scale),
				static_cast<FLOAT>((win_y + (win_h - image_h) * 0.5) / scale)
			};
			const D2D1_SIZE_F page_size{ static_cast<FLOAT>(image_w), static_cast<FLOAT>(image_h) };
			ShapeImage* s = new ShapeImage(center_pos, page_size, bitmap, m_main_page.m_image_opac);
#if (_DEBUG)
			debug_leak_cnt++;
#endif
			bitmap.Close();
			bitmap = nullptr;
			decoder = nullptr;
			stream.Close();
			stream = nullptr;

			{
				m_mutex_draw.lock();
				ustack_push_append(s);
				ustack_push_select(s);
				ustack_push_null();
				m_mutex_draw.unlock();
			}

			ustack_is_enable();
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_append(s);
				summary_select(s);
			}
			xcvd_is_enabled();
			page_bbox_update(s);
			page_panel_size();
			page_draw();

			// カーソルを元に戻す.
			Window::Current().CoreWindow().PointerCursor(prev_cur);
		}
		m_mutex_event.unlock();
	};

	//-------------------------------
	// ファイルメニューの「開く...」が選択された
	//-------------------------------
	IAsyncAction MainPage::file_open_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		// スタックに操作の組が積まれている, かつ確認ダイアログの応答が「キャンセル」か判定する.
		if (m_ustack_is_changed && !co_await file_confirm_dialog()) {
			co_return;
		}
		// ダブルクリックでファイルが選択された場合,
		// co_await が終了する前に, PonterReleased と PonterEntered が呼ばれる.
		// これはピッカーが 2 度目の Released を待たずにダブルクリックを成立させているためだと思われる.

		m_mutex_event.lock();
		// ファイル「オープン」ピッカーを取得して開く.
		FileOpenPicker open_picker{
			FileOpenPicker()
		};
		open_picker.FileTypeFilter().Append(L".gpf");

		// ピッカーを非同期で表示してストレージファイルを取得する.
		// (「閉じる」ボタンが押された場合ストレージファイルは nullptr.)
		StorageFile open_file{
			co_await open_picker.PickSingleFileAsync()
		};
		open_picker = nullptr;
		// ストレージファイルがヌルポインターか判定する.
		if (open_file != nullptr) {
			// 待機カーソルを表示, 表示する前のカーソルを得る.
			const CoreCursor& prev_cur = file_wait_cursor();
			// ストレージファイルを非同期に読む.
			constexpr bool RESUME = true;
			constexpr bool SETTING_ONLY = true;
			co_await file_read_async<!RESUME, !SETTING_ONLY>(open_file);
			// カーソルを元に戻す.
			Window::Current().CoreWindow().PointerCursor(prev_cur);
			// ストレージファイルを解放する.
			open_file = nullptr;
		}
		m_mutex_event.unlock();
	}

	//-------------------------------
	// ストレージファイルを非同期に読む.
	// RESUME	ライフサイクルが再開のとき true
	// SETTING_ONLY	表示設定のみなら true
	// s_file	読み込むストレージファイル
	// 戻り値	読み込めたら S_OK.
	//-------------------------------
	template <bool RESUME, bool SETTING_ONLY>
	IAsyncOperation<winrt::hresult> MainPage::file_read_async(StorageFile s_file) noexcept
	{
		HRESULT hr = E_FAIL;
		m_mutex_draw.lock();
		try {
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				// 一覧を消去する.
				summary_close_click(nullptr, nullptr);
			}
			// 操作スタックと図形リストを消去する.
			ustack_clear();
			slist_clear(m_main_page.m_shape_list);
#if defined(_DEBUG)
			if (debug_leak_cnt != 0) {
				message_show(ICON_DEBUG, DEBUG_MSG, {});
				m_mutex_draw.unlock();
				co_return hr;
			}
#endif

			// ストリームファイルを開いてデータリーダーに読み込む.
			const auto& ra_stream{
				co_await s_file.OpenAsync(FileAccessMode::Read)
			};
			auto dt_reader{ 
				DataReader(ra_stream.GetInputStreamAt(0))
			};
			co_await dt_reader.LoadAsync(static_cast<uint32_t>(ra_stream.Size()));

			// メインページの作図の属性を読み込む.
			m_drawing_tool = static_cast<DRAWING_TOOL>(dt_reader.ReadUInt32());			
			m_drawing_poly_opt.m_vertex_cnt = dt_reader.ReadUInt32();
			m_drawing_poly_opt.m_regular = dt_reader.ReadBoolean();
			m_drawing_poly_opt.m_vertex_up = dt_reader.ReadBoolean();
			m_drawing_poly_opt.m_end_closed = dt_reader.ReadBoolean();
			m_drawing_poly_opt.m_clockwise = dt_reader.ReadBoolean();

			// メインページの文字検索の属性を読み込む.
			const size_t find_text_len = dt_reader.ReadUInt32();	// 文字数
			uint8_t* find_text_data = new uint8_t[2 * (find_text_len + 1)];
			dt_reader.ReadBytes(array_view(find_text_data, find_text_data + 2 * find_text_len));
			m_find_text = reinterpret_cast<wchar_t*>(find_text_data);
			m_find_text[find_text_len] = L'\0';

			const size_t find_repl_len = dt_reader.ReadUInt32();	// 文字数
			uint8_t* find_repl_data = new uint8_t[2 * (find_repl_len + 1)];
			dt_reader.ReadBytes(array_view(find_repl_data, find_repl_data + 2 * find_repl_len));
			m_find_repl = reinterpret_cast<wchar_t*>(find_repl_data);
			m_find_repl[find_repl_len] = L'\0';

			const uint16_t f_bit = dt_reader.ReadUInt16();
			m_text_frame_fit_text = ((f_bit & 1) != 0);
			m_find_text_case = ((f_bit & 2) != 0);
			m_find_text_wrap = ((f_bit & 4) != 0);

			// メインページのその他の属性を読み込む.
			m_len_unit = static_cast<LEN_UNIT>(dt_reader.ReadUInt32());
			m_color_code = static_cast<COLOR_CODE>(dt_reader.ReadUInt16());
			m_vert_stick = dt_reader.ReadSingle();
			m_status_bar = static_cast<STATUS_BAR>(dt_reader.ReadUInt16());
			m_image_keep_aspect = dt_reader.ReadBoolean();	// 画像の縦横比の維持

			const bool s_atom = dt_reader.ReadBoolean();
			m_summary_atomic.store(s_atom, std::memory_order_release);

			// メイン表示を読み込む.
			m_main_page.read(dt_reader);

			// メイン表示のみの読み込みか判定する.
			if constexpr (SETTING_ONLY) {
				hr = S_OK;
			}
			else {
				// 図形を読み込む.
				if (slist_read(m_main_page.m_shape_list, dt_reader)) {
					// 再開なら,
					if constexpr (RESUME) {
						// スタックも読み込む.
						ustack_read(dt_reader);
					}
					hr = S_OK;
				}
			}
			dt_reader.Close();
			ra_stream.Close();
		}
		catch (winrt::hresult_error const& e) {
			hr = e.code();
		}
		m_mutex_draw.unlock();
		// 読み込みに失敗した場合,
		if (hr != S_OK) {
			if constexpr (RESUME) {
				message_show(ICON_ALERT, L"str_err_resume", s_file.Path());
				// 表示設定を既定値に戻す.
				page_setting_init();
				m_len_unit = LEN_UNIT::PIXEL;
				m_color_code = COLOR_CODE::DEC;
				m_vert_stick = VERT_STICK_DEF_VAL;
				m_status_bar = STATUS_BAR_DEF_VAL;
			}
			else if constexpr (SETTING_ONLY) {
				message_show(ICON_ALERT, L"str_err_load", s_file.Path());
			}
			else {
				message_show(ICON_ALERT, L"str_err_read", s_file.Path());
			}
		}
		else {
			if constexpr (!RESUME && !SETTING_ONLY) {
				file_recent_add(s_file);
				file_finish_reading();
			}
		}
		// 結果を返し終了する.
		co_return hr;
	}

	template IAsyncOperation<winrt::hresult> MainPage::file_read_async<false, false>(StorageFile s_file) noexcept;
	template IAsyncOperation<winrt::hresult> MainPage::file_read_async<false, true>(StorageFile s_file) noexcept;
	template IAsyncOperation<winrt::hresult> MainPage::file_read_async<true, false>(StorageFile s_file) noexcept;

	//-------------------------------
	// ストレージファイルを最近使ったファイルに登録する.
	// s_file	ストレージファイル
	// 最近使ったファイルメニューとウィンドウタイトルも更新される.
	// ストレージファイルがヌルの場合, ウィンドウタイトルに無題が格納される.
	//-------------------------------
	void MainPage::file_recent_add(StorageFile const& s_file)
	{
		if (s_file != nullptr) {
			m_file_token_mru = StorageApplicationPermissions::MostRecentlyUsedList().Add(s_file, s_file.Path());
			ApplicationView::GetForCurrentView().Title(s_file.Name());
		}
		else {
			const auto untitle = ResourceLoader::GetForCurrentView().GetString(L"str_untitled");
			ApplicationView::GetForCurrentView().Title(untitle);
		}
		file_recent_menu_update();
	}

	//-------------------------------
	// ファイルメニューの「最近使ったファイル」のサブ項目が選択された.
	//-------------------------------
	IAsyncAction MainPage::file_recent_click_async(IInspectable const& sender, RoutedEventArgs const&)
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
			co_return;
		}
		// 最近使ったファイルを読み込む.
		{
			// SHCore.dll スレッド
			auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
			auto const& mru_entries = mru_list.Entries();
			// ファイルの番号が最近使ったファイルの数以上か判定する.
			// 数以上なら
			if (n >= mru_entries.Size()) {
				// 最近使ったファイルのエラーを表示する.
				message_show(ICON_ALERT, L"str_err_recent", to_hstring(n + 1));
				co_return;
			}
			// 最近使ったリストから i 番目の要素を得る.
			AccessListEntry item[1];
			mru_entries.GetMany(n, item);

			// スタックに操作の組が積まれている, かつ確認ダイアログの応答が「キャンセル」か判定する.
			if (m_ustack_is_changed && !co_await file_confirm_dialog()) {
				co_return;
			}

			// 最近使ったファイルのトークンからストレージファイルを得る.
			StorageFile s_file{ co_await file_recent_token_async(item[0].Token) };	// ストレージファイル
			// ストレージファイルが空でないか判定する.
			// 空でないなら
			if (s_file != nullptr) {
				// 待機カーソルを表示, 表示する前のカーソルを得る.
				const CoreCursor& prev_cur = file_wait_cursor();
				constexpr bool RESUME = true;
				constexpr bool SETTING_ONLY = true;
				co_await file_read_async<!RESUME, !SETTING_ONLY>(s_file);
				// ストレージファイルを破棄する.
				s_file = nullptr;
				// カーソルを元に戻す.
				Window::Current().CoreWindow().PointerCursor(prev_cur);
			}
			else {
				// 取得できないならば,
				message_show(ICON_ALERT, L"str_err_recent", item[0].Metadata);
			}
		}
	}

	//-------------------------------
	// 最近使ったファイルのトークンからストレージファイルを得る.
	// token	ファイルのトークン
	// 戻り値	ストレージファイル
	//-------------------------------
	IAsyncOperation<StorageFile> MainPage::file_recent_token_async(const winrt::hstring token)
	{
		// ストレージファイルにヌルを格納する.
		StorageFile recent_file = nullptr;	// ストレージファイル
		try {
			// トークンが空でないか判定する.
			// 空でないなら,
			if (!token.empty()) {
				// 最近使ったファイルのリストを得る.
				// リストにトークンが含まれているか判定する.
				// 含まれているなら,
				auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
				if (mru_list.ContainsItem(token)) {
					// リストからそのトークンがしめすストレージファイルを得る.
					recent_file = co_await mru_list.GetFileAsync(token);
				}
			}
		}
		catch (winrt::hresult_error) {
		}
		// 取得できても出来なくても最近使ったリストの順番は入れ替わるので,
		// 最近使ったファイルメニューを更新する.
		file_recent_menu_update();
		// ストレージファイルを返す.
		co_return recent_file;
	}

	//-------------------------------
	// 最近使ったファイルメニューを更新する.
	//-------------------------------
	void MainPage::file_recent_menu_update(void)
	{
		// 最近使ったファイルのアクセスリストを得る.
		auto const& mru_entries{
			StorageApplicationPermissions::MostRecentlyUsedList().Entries()
		};
		const auto mru_size = mru_entries.Size();
		AccessListEntry items[MRU_MAX];
		mru_entries.GetMany(0, items);
		// アクセスリストのファイル名をメニュー項目に格納する.
		mfi_file_recent_1().Text(mru_size > 0 ? items[0].Metadata : L"");
		mfi_file_recent_2().Text(mru_size > 1 ? items[1].Metadata : L"");
		mfi_file_recent_3().Text(mru_size > 2 ? items[2].Metadata : L"");
		mfi_file_recent_4().Text(mru_size > 3 ? items[3].Metadata : L"");
		mfi_file_recent_5().Text(mru_size > 4 ? items[4].Metadata : L"");
	}

	//-------------------------------
	// 名前を付けてファイルに非同期に保存する.
	//-------------------------------
	IAsyncAction MainPage::file_save_as_click_async(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		m_mutex_event.lock();

		// ファイル保存ピッカーを得る.
		FileSavePicker save_picker{
			FileSavePicker()
		};
		// ファイルタイプに拡張子とその説明を追加する.
		const winrt::hstring desc_gpf{
			ResourceLoader::GetForCurrentView().GetString(L"str_desc_gpf")
		};
		save_picker.FileTypeChoices().Insert(desc_gpf, TYPE_GPF);

		// ドキュメントライブラリーを保管場所に設定する.
		const PickerLocationId loc_id = PickerLocationId::DocumentsLibrary;
		save_picker.SuggestedStartLocation(loc_id);

		// 最近使ったファイルのトークンが空か判定する.
		if (m_file_token_mru.empty()) {
			// 提案されたファイル名に拡張子を格納する.
			save_picker.SuggestedFileName(L".gpf");
		}
		else {
			// 最近使ったファイルのトークンからストレージファイルを得る.
			StorageFile recent_file{
				co_await file_recent_token_async(m_file_token_mru)
			};
			// ストレージファイルを得たなら,
			if (recent_file != nullptr) {
				// ファイルタイプが ".gpf" か判定する.
				if (recent_file.FileType() == L".gpf") {
					// ファイル名を, 提案するファイル名に格納する.
					save_picker.SuggestedFileName(recent_file.Name());
				}
				recent_file = nullptr;
			}
		}
		// ファイル保存ピッカーを表示し, ストレージファイルを得る.
		StorageFile save_file{
			co_await save_picker.PickSaveFileAsync()
		};
		// ピッカーを破棄する.
		save_picker = nullptr;
		// ストレージファイルを取得したか判定する.
		if (save_file != nullptr) {
			// 待機カーソルを表示, 表示する前のカーソルを得る.
			const CoreCursor& prev_cur = file_wait_cursor();
			// ファイルタイプが方眼紙ファイルか判定する.
			if (save_file.FileType() == L".gpf") {
				// 図形データをストレージファイルに非同期に書き込み, 結果を得る.
				co_await file_write_gpf_async<false, false>(save_file);
			}
			// ストレージファイルを破棄する.
			save_file = nullptr;
			// カーソルを元に戻す.
			Window::Current().CoreWindow().PointerCursor(prev_cur);
		}
		m_mutex_event.unlock();
	}

	//-------------------------------
	// ファイルメニューの「上書き保存」が選択された
	//-------------------------------
	IAsyncAction MainPage::file_save_click_async(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		// 最近使ったファイルのトークンからストレージファイルを得る.
		StorageFile recent_file{
			co_await file_recent_token_async(m_file_token_mru)
		};
		// ストレージファイルがない場合,
		if (recent_file == nullptr) {
			// 名前を付けてファイルに非同期に保存する
			co_await file_save_as_click_async(nullptr, nullptr);
		}
		// ストレージファイルを得た場合,
		else {
			// 待機カーソルを表示, 表示する前のカーソルを得る.
			const CoreCursor& prev_cur = file_wait_cursor();	// 前のカーソル
			// 図形データをストレージファイルに非同期に書き込み, 結果を得る.
			co_await file_write_gpf_async<false, false>(recent_file);
			recent_file = nullptr;
			// カーソルを元に戻す.
			Window::Current().CoreWindow().PointerCursor(prev_cur);
		}
	}

	//-------------------------------
	// 画像用のファイル保存ピッカーを開いて, ストレージファイルを得る.
	//-------------------------------
	/*
	IAsyncOperation <StorageFile> MainPage::file_pick_save_image_async(const wchar_t sug_name[])
	{
		// ResourceLoader::GetForCurrentView はフォアグラウンド.
		const ResourceLoader& res_loader = ResourceLoader::GetForCurrentView();
		const winrt::hstring desc_bmp{ res_loader.GetString(L"str_desc_bmp") };
		const winrt::hstring desc_gif{ res_loader.GetString(L"str_desc_gif") };
		const winrt::hstring desc_jpg{ res_loader.GetString(L"str_desc_jpg") };
		const winrt::hstring desc_pdf{ res_loader.GetString(L"str_desc_pdf") };
		const winrt::hstring desc_png{ res_loader.GetString(L"str_desc_png") };
		const winrt::hstring desc_svg{ res_loader.GetString(L"str_desc_svg") };
		const winrt::hstring desc_tif{ res_loader.GetString(L"str_desc_tif") };

		FileSavePicker image_picker{ FileSavePicker() };
		image_picker.FileTypeChoices().Insert(desc_svg, TYPE_SVG);
		image_picker.FileTypeChoices().Insert(desc_pdf, TYPE_PDF);
		image_picker.FileTypeChoices().Insert(desc_bmp, TYPE_BMP);
		image_picker.FileTypeChoices().Insert(desc_gif, TYPE_GIF);
		image_picker.FileTypeChoices().Insert(desc_jpg, TYPE_JPG);
		image_picker.FileTypeChoices().Insert(desc_png, TYPE_PNG);
		image_picker.FileTypeChoices().Insert(desc_tif, TYPE_TIF);

		// 画像ライブラリーを保管場所に設定する.
		const PickerLocationId loc_id = PickerLocationId::PicturesLibrary;
		image_picker.SuggestedStartLocation(loc_id);

		// ピッカーに, あらかじめ表示されるファイル名を設定する.
		if (m_file_token_mru.empty()) {
			// 提案されたファイル名に拡張子を格納する.
			image_picker.SuggestedFileName(L"");
		}
		else {
			// 最近使ったファイルのトークンからストレージファイルを得る.
			StorageFile recent_file{
				co_await file_recent_token_async(m_file_token_mru)
			};
			// ストレージファイルを得たなら,
			if (recent_file != nullptr) {
				if (recent_file.FileType() == L".gpf") {
					// ファイル名を, 提案するファイル名に格納する.
					image_picker.SuggestedFileName(recent_file.Name());
				}
				recent_file = nullptr;
			}
		}

		// ピッカーを表示しストレージファイルを得る.
		StorageFile img_file{
			co_await image_picker.PickSaveFileAsync()
		};

		image_picker = nullptr;
		co_return img_file;
	}
	*/

	//-------------------------------
	// 図形データをストレージファイルに非同期に書き込む.
	// SUSPEND 	ライフサイクルが中断のとき true
	// SETTING_ONLY	「表示設定を保存」のとき true
	// s_file	ストレージファイル
	// 戻り値	書き込みに成功したら true
	//-------------------------------
	template <bool SUSPEND, bool SETTING_ONLY>
	IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async(StorageFile s_file)
	{
		constexpr auto REDUCE = true;

		// スレッドを変更する前に, 排他制御をロック
		m_mutex_exit.lock();
		// E_FAIL を結果に格納する.
		HRESULT hr = E_FAIL;
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
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_drawing_tool));
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_drawing_poly_opt.m_vertex_cnt));
			dt_writer.WriteBoolean(m_drawing_poly_opt.m_regular);
			dt_writer.WriteBoolean(m_drawing_poly_opt.m_vertex_up);
			dt_writer.WriteBoolean(m_drawing_poly_opt.m_end_closed);
			dt_writer.WriteBoolean(m_drawing_poly_opt.m_clockwise);
			//dt_write(m_find_text, dt_writer);
			const uint32_t find_text_len = wchar_len(m_find_text);
			dt_writer.WriteUInt32(find_text_len);
			const auto find_text_data = reinterpret_cast<const uint8_t*>(m_find_text);
			dt_writer.WriteBytes(array_view(find_text_data, find_text_data + 2 * find_text_len));
			//dt_write(m_find_repl, dt_writer);
			const uint32_t find_repl_len = wchar_len(m_find_repl);
			dt_writer.WriteUInt32(find_repl_len);
			const auto find_repl_data = reinterpret_cast<const uint8_t*>(m_find_repl);
			dt_writer.WriteBytes(array_view(find_repl_data, find_repl_data + 2 * find_repl_len));
			uint16_t f_bit = 0;
			if (m_text_frame_fit_text) {
				f_bit |= 1;
			}
			if (m_find_text_case) {
				f_bit |= 2;
			}
			if (m_find_text_wrap) {
				f_bit |= 4;
			}
			dt_writer.WriteUInt16(f_bit);
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_len_unit));
			dt_writer.WriteUInt16(static_cast<uint16_t>(m_color_code));
			dt_writer.WriteSingle(m_vert_stick);
			dt_writer.WriteUInt16(static_cast<uint16_t>(m_status_bar));
			dt_writer.WriteBoolean(m_image_keep_aspect);

			dt_writer.WriteBoolean(summary_is_visible());

			m_main_page.write(dt_writer);
			if constexpr (SUSPEND) {
				// 消去された図形も含めて書き込む.
				// 操作スタックも書き込む.
				slist_write<!REDUCE>(m_main_page.m_shape_list, dt_writer);
				ustack_write(dt_writer);
			}
			else if constexpr (!SETTING_ONLY) {
				// 消去された図形は省いて書き込む.
				// 操作スタックは消去する.
				slist_write<REDUCE>(m_main_page.m_shape_list, dt_writer);
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
				hr = S_OK;
			}
		}
		// エラーが発生した場合, 
		catch (winrt::hresult_error const& err) {
			hr = err.code();
		}

		// 結果が S_OK 以外か判定する.
		if (hr != S_OK) {
			if constexpr (SUSPEND) {
				// 「ファイルに書き込めません」メッセージダイアログを表示する.
				message_show(ICON_ALERT, L"str_err_suspend", s_file.Path());
				// 一覧が表示されてるなら閉じる.
				if (summary_is_visible()) {
					summary_clear();
				}
				ustack_clear();
				slist_clear(m_main_page.m_shape_list);
				slist_clear(m_dialog_page.m_shape_list);
#if defined(_DEBUG)
				if (debug_leak_cnt != 0) {
					// 「メモリリーク」メッセージダイアログを表示する.
					message_show(ICON_DEBUG, DEBUG_MSG, {});
				}
#endif
				page_draw();
			}
			else if constexpr (SETTING_ONLY) {
				message_show(ICON_ALERT, L"str_err_save", s_file.Path());
			}
			else {
				// 「ファイルに書き込めません」メッセージダイアログを表示する.
				message_show(ICON_ALERT, L"str_err_write", s_file.Path());
			}
		}
		else {
			// 中断ではない, かつ設定ではないか判定する.
			if constexpr (!SUSPEND && !SETTING_ONLY) {
				// ストレージファイルを最近使ったファイルに登録する.
				// ここでエラーが出る.
				file_recent_add(s_file);
				// false をスタックが更新されたか判定に格納する.
				m_ustack_is_changed = false;
			}
		}
		m_mutex_exit.unlock();
		// 結果を返し終了する.
		co_return hr;
	}
	template IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async<false, false>(StorageFile s_file);
	template IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async<true, false>(StorageFile s_file);
	template IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async<false, true>(StorageFile s_file);

	// ファイルメニューの「新規」が選択された
	IAsyncAction MainPage::file_new_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		// スタックが更新されたなら確認ダイアログを表示して, 
		// ダイアログの応答が「キャンセル」か判定する.
		if (m_ustack_is_changed && !co_await file_confirm_dialog()) {
			co_return;
		}
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_close_click(nullptr, nullptr);
		}
		ustack_clear();
		slist_clear(m_main_page.m_shape_list);

#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			// 「メモリリーク」メッセージダイアログを表示する.
			message_show(ICON_DEBUG, DEBUG_MSG, {});
		}
#endif

		ShapeText::release_available_fonts();

		ShapeText::set_available_fonts(m_main_d2d);

		// 背景色, 前景色, 選択された文字範囲の背景色, 文字色をリソースから得る.
		{
			const IInspectable sel_back_color = Resources().TryLookup(box_value(L"SystemAccentColor"));
			const IInspectable sel_text_color = Resources().TryLookup(box_value(L"SystemColorHighlightTextColor"));
			if (sel_back_color != nullptr && sel_text_color != nullptr) {
				conv_uwp_to_color(unbox_value<Color>(sel_back_color), ShapeText::s_text_selected_background);
				conv_uwp_to_color(unbox_value<Color>(sel_text_color), ShapeText::s_text_selected_foreground);
			}
			else {
				ShapeText::s_text_selected_background = { 0.0f, 0x00 / COLOR_MAX, 0x78 / COLOR_MAX, 0xD4 / COLOR_MAX };
				ShapeText::s_text_selected_foreground = COLOR_WHITE;
			}
			/*
			auto const& back_theme = Resources().TryLookup(box_value(L"ApplicationPageBackgroundThemeBrush"));
			auto const& fore_theme = Resources().TryLookup(box_value(L"ApplicationForegroundThemeBrush"));
			if (back_theme != nullptr && fore_theme != nullptr) {
				conv_uwp_to_color(unbox_value<Brush>(back_theme), Shape::s_background_color);
				conv_uwp_to_color(unbox_value<Brush>(fore_theme), Shape::s_foreground_color);
			}
			else {*/
			Shape::s_background_color = COLOR_WHITE;
			Shape::s_foreground_color = COLOR_BLACK;
			//}
		}

		// 設定データを保存するフォルダーを得る.
		mfi_page_setting_reset().IsEnabled(false);
		winrt::Windows::Storage::IStorageItem setting_item{
			co_await ApplicationData::Current().LocalFolder().TryGetItemAsync(PAGE_SETTING)
		};

		if (setting_item == nullptr) {
			// 表示設定を既定値に戻す.
			page_setting_init();
			m_len_unit = LEN_UNIT::PIXEL;
			m_color_code = COLOR_CODE::DEC;
			m_vert_stick = VERT_STICK_DEF_VAL;
			m_status_bar = STATUS_BAR_DEF_VAL;
		}
		else {
			auto setting_file = setting_item.try_as<StorageFile>();
			if (setting_file != nullptr) {
				constexpr bool RESUME = true;
				constexpr bool SETTING_ONLY = true;
				co_await file_read_async<!RESUME, SETTING_ONLY>(setting_file);
				setting_file = nullptr;
				mfi_page_setting_reset().IsEnabled(true);
			}
			setting_item = nullptr;
		}

		// 表示の左上位置と右下位置を初期化する.
		{
			m_main_min = D2D1_POINT_2F{ 0.0F, 0.0F };
			m_main_max = D2D1_POINT_2F{ m_main_page.m_page_size.width, m_main_page.m_page_size.height };
		}
		file_recent_add(nullptr);
		file_finish_reading();

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

