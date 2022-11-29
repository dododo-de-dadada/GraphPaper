//-------------------------------
// MainPage_file.cpp
// ファイルの読み書き
//-------------------------------
#include "pch.h"
#include "MainPage.h"
#include <wincodec.h>
#include <shcore.h>

using namespace winrt;
/*
* ファイル関係の非同期の関数は, UI スレッドで実行されることを前提としている.
* UI スレッド以外が呼び出すと失敗する関数がある.

file_confirm_dialog
	+---file_save_click_async

file_exit_click_async
	+---file_confirm_dialog

file_export_as_image_click_async
	+---file_pick_save_image_async

file_finish_reading

file_pick_save_image_async

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
	+---file_write_svg_async
	+---file_write_gpf_async

file_save_click_async
	+---file_recent_token_async
	+---file_save_as_click_async
	+---file_wait_cursor
	+---file_write_gpf_async

file_write_gpf_async
	+---file_recent_add

file_write_svg_async
	+---file_pick_save_image_async

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

	static const CoreCursor& CURS_WAIT = CoreCursor(CoreCursorType::Wait, 0);	// 待機カーソル.
	constexpr uint32_t MRU_MAX = 25;	// 最近使ったリストの最大数.
	constexpr wchar_t RES_DESC_GPF[] = L"str_desc_gpf";	// 拡張子 gpf の説明
	constexpr wchar_t RES_DESC_SVG[] = L"str_desc_svg";	// 拡張子 svg の説明
	constexpr wchar_t RES_ERR_FONT[] = L"str_err_font";	// 有効でない書体のエラーメッセージのリソース名
	constexpr wchar_t RES_ERR_READ[] = L"str_err_read";	// 読み込みエラーメッセージのリソース名
	constexpr wchar_t RES_ERR_RECENT[] = L"str_err_recent";	// 最近使ったファイルのエラーメッセージのリソース名
	constexpr wchar_t RES_ERR_WRITE[] = L"str_err_write";	// 書き込みエラーメッセージのリソース名
	constexpr wchar_t RES_EXT_BMP[] = L".bmp";	// 画像ファイルの拡張子
	constexpr wchar_t RES_EXT_GIF[] = L".gif";	// 画像ファイルの拡張子
	constexpr wchar_t RES_EXT_GPF[] = L".gpf";	// 図形データファイルの拡張子
	constexpr wchar_t RES_EXT_JPEG[] = L".jpeg";	// 画像ファイルの拡張子
	constexpr wchar_t RES_EXT_JPG[] = L".jpg";	// 画像ファイルの拡張子
	constexpr wchar_t RES_EXT_PNG[] = L".png";	// 画像ファイルの拡張子
	constexpr wchar_t RES_EXT_SVG[] = L".svg";	// SVG ファイルの拡張子
	constexpr wchar_t RES_EXT_TIF[] = L".tif";	// 画像ファイルの拡張子
	constexpr wchar_t RES_EXT_TIFF[] = L".tiff";	// 画像ファイルの拡張子
	constexpr wchar_t UNTITLED[] = L"str_untitled";	// 無題のリソース名
	static const IVector<winrt::hstring> TYPE_BMP{
		winrt::single_threaded_vector<winrt::hstring>({ RES_EXT_BMP })
	};
	static const IVector<winrt::hstring> TYPE_GIF{
		winrt::single_threaded_vector<winrt::hstring>({ RES_EXT_GIF })
	};
	static const IVector<winrt::hstring> TYPE_JPG{
		winrt::single_threaded_vector<winrt::hstring>({ RES_EXT_JPG, RES_EXT_JPEG })
	};
	static const IVector<winrt::hstring> TYPE_PNG{
		winrt::single_threaded_vector<winrt::hstring>({ RES_EXT_PNG })
	};
	static const IVector<winrt::hstring> TYPE_TIF{
		winrt::single_threaded_vector<winrt::hstring>({ RES_EXT_TIF, RES_EXT_TIFF })
	};
	static const IVector<winrt::hstring> TYPE_GPF{
		winrt::single_threaded_vector<winrt::hstring>({ RES_EXT_GPF })
	};
	static const IVector<winrt::hstring> TYPE_SVG{
		winrt::single_threaded_vector<winrt::hstring>({ RES_EXT_SVG })
	};
	static const IVector<winrt::hstring> TYPE_PDF{
		winrt::single_threaded_vector<winrt::hstring>({ L".pdf" })
	};
	static winrt::guid enc_id_default = BitmapEncoder::BmpEncoderId();	// 既定のエンコード識別子

	inline static CoreCursor file_wait_cursor(void);

	//-------------------------------
	// 待機カーソルを表示, 表示する前のカーソルを得る.
	//-------------------------------
	inline static CoreCursor file_wait_cursor(void)
	{
		CoreWindow const& c_win = Window::Current().CoreWindow();
		CoreCursor const& prev_cur = c_win.PointerCursor();
		if (prev_cur.Type() != CURS_WAIT.Type()) {
			c_win.PointerCursor(CURS_WAIT);
		}
		return prev_cur;
	}

	//-------------------------------
	// ファイルへの更新を確認する.
	// 戻り値	「保存する」または「保存しない」が押されたなら true を, 応答がキャンセルなら, または内容を保存できなかったなら false を返す.
	//-------------------------------
	IAsyncOperation<bool> MainPage::file_confirm_dialog(void)
	{
		// 確認ダイアログを表示し, 応答を得る.
		const ContentDialogResult dr{
			co_await cd_conf_save_dialog().ShowAsync()
		};

		// 応答が「保存する」か判定する.
		if (dr == ContentDialogResult::Primary) {
			// ファイルに非同期に保存.
			// 保存に失敗しても, true を返す.
			co_await file_save_click_async(nullptr, nullptr);
			co_return true;
		}
		// 応答が「保存しない」か判定する.
		else if (dr == ContentDialogResult::Secondary) {
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
		static std::mutex exit{};	// 終了の排他処理
		if (!exit.try_lock()) {
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
		else if (m_menu_sheet != nullptr && m_menu_sheet.IsOpen()) {
			m_menu_sheet.Hide();
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
		cd_prop_dialog().Hide();
		cd_sheet_size_dialog().Hide();

		// スタックが更新された, かつ確認ダイアログの応答が「キャンセル」か判定する.
		if (m_ustack_is_changed && !co_await file_confirm_dialog()) {
			// 「キャンセル」なら, 排他処理を解除して中断する.
			exit.unlock();
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
			m_menu_sheet = nullptr;
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
			m_mutex_draw.lock();
			if (m_main_sheet.m_state_block != nullptr) {
				//m_main_sheet.m_state_block->Release();
				m_main_sheet.m_state_block = nullptr;
			}
			if (m_main_sheet.m_color_brush != nullptr) {
				//m_main_sheet.m_color_brush->Release();
				m_main_sheet.m_color_brush = nullptr;
			}
			if (m_main_sheet.m_range_brush != nullptr) {
				//m_main_sheet.m_range_brush->Release();
				m_main_sheet.m_range_brush = nullptr;
			}
			m_main_sheet.m_d2d.Trim();
			if (m_prop_sheet.m_state_block != nullptr) {
				//m_prop_sheet.m_state_block->Release();
				m_prop_sheet.m_state_block = nullptr;
			}
			if (m_prop_sheet.m_color_brush != nullptr) {
				//m_prop_sheet.m_color_brush->Release();
				m_prop_sheet.m_color_brush = nullptr;
			}
			if (m_prop_sheet.m_range_brush != nullptr) {
				//m_prop_sheet.m_range_brush->Release();
				m_prop_sheet.m_range_brush = nullptr;
			}
			m_prop_sheet.m_d2d.Trim();
		}

		ustack_clear();
		slist_clear(m_main_sheet.m_shape_list);
		slist_clear(m_prop_sheet.m_shape_list);
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
	// ファイルメニューの「用紙を画像としてエクスポートする」が選択された
	//------------------------------
	IAsyncAction MainPage::file_export_as_image_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		StorageFile s_file{ co_await file_pick_save_image_async(nullptr) };
		if (s_file == nullptr) {
			co_return;
		}

		// Direct2D コンテンツを画像ファイルに保存する方法
		const GUID& wic_fmt = [](const winrt::hstring& f_type)
		{
			if (f_type == L".png") {
				return GUID_ContainerFormatPng;
			}
			else if (f_type == L".tif") {
				return GUID_ContainerFormatTiff;
			}
			else if (f_type == L".jpg") {
				return GUID_ContainerFormatJpeg;
			}
			else if (f_type == L".bmp") {
				return GUID_ContainerFormatBmp;
			}
			return GUID_NULL;
		}(s_file.FileType());

		if (wic_fmt == GUID_NULL) {
			co_return;
		}

		IRandomAccessStream ra_stream{
			co_await s_file.OpenAsync(FileAccessMode::ReadWrite)
		};
		winrt::com_ptr<IStream> stream;
		winrt::hresult(
			CreateStreamOverRandomAccessStream(winrt::get_unknown(ra_stream), IID_PPV_ARGS(&stream))
		);

		winrt::com_ptr<IWICImagingFactory2> wic_factory;
		winrt::check_hresult(
			CoCreateInstance(
				CLSID_WICImagingFactory,
				nullptr,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&wic_factory)
			)
		);

		winrt::com_ptr<IWICBitmapEncoder> wic_enc;
		winrt::check_hresult(
			wic_factory->CreateEncoder(wic_fmt, nullptr, wic_enc.put())
		);
		winrt::check_hresult(
			wic_enc->Initialize(stream.get(), WICBitmapEncoderNoCache)
		);

		winrt::com_ptr<IWICBitmapFrameEncode> wic_frm;
		winrt::check_hresult(
			wic_enc->CreateNewFrame(wic_frm.put(), nullptr)
		);
		winrt::check_hresult(
			wic_frm->Initialize(nullptr)
		);
		/*
		D2D1_BITMAP_PROPERTIES bp{
			D2D1_PIXEL_FORMAT{ DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT },
			96.0f,
			96.0f
		};

		winrt::com_ptr<ID2D1Bitmap> bm;
		m_main_sheet.m_d2d.m_d2d_context->CreateBitmap(
			D2D1_SIZE_U{
				static_cast<uint32_t>(m_main_sheet.m_sheet_size.width),
				static_cast<uint32_t>(m_main_sheet.m_sheet_size.height)
			},
			NULL,
			0,
			bp,
			bm.put()
		);
		m_main_sheet.m_d2d.m_d2d_context->SetTarget(nullptr);
		m_main_sheet.m_d2d.m_d2d_context->SetTarget(bm.get());
		*/
		winrt::com_ptr<ID2D1Device> d2d_dev;
		m_main_sheet.m_d2d.m_d2d_context->GetDevice(d2d_dev.put());

		winrt::com_ptr<IWICImageEncoder> image_enc;
		winrt::check_hresult(
			wic_factory->CreateImageEncoder(d2d_dev.get(), image_enc.put())
		);

		winrt::com_ptr<ID2D1Image> d2d_image;
		m_main_sheet.m_d2d.m_d2d_context->GetTarget(d2d_image.put());
		winrt::check_hresult(
			image_enc->WriteFrame(d2d_image.get(), wic_frm.get(), nullptr)
		);

		//m_main_sheet.m_d2d.HandleDeviceLost();

		//scp_sheet_panel().Width(w);
		//scp_sheet_panel().Height(h);

		// スレッドをメインページの UI スレッドに変える.
		//co_await winrt::resume_foreground(Dispatcher());
		//m_main_sheet.m_d2d.SetSwapChainPanel(scp_sheet_panel());

		winrt::check_hresult(
			wic_frm->Commit()
		);
		winrt::check_hresult(
			wic_enc->Commit()
		);
		winrt::check_hresult(
			stream->Commit(STGC_DEFAULT)
		);
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
		
		sheet_attr_is_checked();

		wchar_t* unavailable_font;	// 無効な書体名
		if (!slist_test_avaiable_font(m_main_sheet.m_shape_list, unavailable_font)) {
			// 「無効な書体が使用されています」メッセージダイアログを表示する.
			message_show(ICON_ALERT, RES_ERR_FONT, unavailable_font);
		}

		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			if (m_main_sheet.m_shape_list.empty()) {
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

		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
		status_bar_set_pos();
		status_bar_set_draw();
		status_bar_set_grid();
		status_bar_set_sheet();
		status_bar_set_zoom();
		status_bar_set_unit();
	}

	//-------------------------------
	// ファイルメニューの「画像を図形としてインポートする...」が選択された.
	//-------------------------------
	IAsyncAction MainPage::file_import_image_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		// ファイル「オープン」ピッカーを取得して開く.
		FileOpenPicker open_picker{ FileOpenPicker() };
		open_picker.FileTypeFilter().Append(RES_EXT_BMP);
		open_picker.FileTypeFilter().Append(RES_EXT_GIF);
		open_picker.FileTypeFilter().Append(RES_EXT_JPG);
		open_picker.FileTypeFilter().Append(RES_EXT_JPEG);
		open_picker.FileTypeFilter().Append(RES_EXT_PNG);
		open_picker.FileTypeFilter().Append(RES_EXT_TIF);
		open_picker.FileTypeFilter().Append(RES_EXT_TIFF);

		// ピッカーを非同期に表示してストレージファイルを取得する.
		// (「閉じる」ボタンが押された場合ストレージファイルは nullptr.)
		// co_await してるにもかかわらず, ファイル開くピッカーが返値を戻すまで時間がかかる.
		// その間フォアグランドのスレッドが動作してしまう.
		m_mutex_event.lock();
		StorageFile open_file{
			co_await open_picker.PickSingleFileAsync()
		};
		m_mutex_event.unlock();
		open_picker = nullptr;
		// ストレージファイルがヌルポインターか判定する.
		if (open_file != nullptr) {
			// 待機カーソルを表示, 表示する前のカーソルを得る.
			const CoreCursor& prev_cur = file_wait_cursor();
			unselect_all();
			const double win_w = scp_sheet_panel().ActualWidth();
			const double win_h = scp_sheet_panel().ActualHeight();
			const double win_x = sb_horz().Value();
			const double win_y = sb_vert().Value();

			IRandomAccessStream stream{ co_await open_file.OpenAsync(FileAccessMode::Read) };
			BitmapDecoder decoder{ co_await BitmapDecoder::CreateAsync(stream) };
			SoftwareBitmap bitmap{ SoftwareBitmap::Convert(co_await decoder.GetSoftwareBitmapAsync(), BitmapPixelFormat::Bgra8) };

			// 用紙の表示された部分の中心の位置を求める.
			const double scale = m_main_sheet.m_sheet_scale;
			const double image_w = bitmap.PixelWidth();
			const double image_h = bitmap.PixelHeight();
			const D2D1_POINT_2F center_pos{
				static_cast<FLOAT>((win_x + (win_w - image_w) * 0.5) / scale),
				static_cast<FLOAT>((win_y + (win_h - image_h) * 0.5) / scale)
			};
			const D2D1_SIZE_F view_size{ static_cast<FLOAT>(image_w), static_cast<FLOAT>(image_h) };
			ShapeImage* s = new ShapeImage(center_pos, view_size, bitmap, m_main_sheet.m_image_opac);
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
			sheet_update_bbox(s);
			sheet_panle_size();
			sheet_draw();

			// カーソルを元に戻す.
			Window::Current().CoreWindow().PointerCursor(prev_cur);
		}
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
		// ファイル「オープン」ピッカーを取得して開く.
		FileOpenPicker open_picker{
			FileOpenPicker()
		};
		open_picker.FileTypeFilter().Append(RES_EXT_GPF);
		// ダブルクリックでファイルが選択された場合,
		// co_await が終了する前に, PonterReleased と PonterEntered が呼ばれる.
		// これはピッカーが 2 度目の Released を待たずにダブルクリックを成立させているためだと思われる.
		//scp_sheet_panel().PointerReleased(m_token_event_released);
		//scp_sheet_panel().PointerEntered(m_token_event_entered);

		// ピッカーを非同期で表示してストレージファイルを取得する.
		// (「閉じる」ボタンが押された場合ストレージファイルは nullptr.)
		m_mutex_event.lock();
		StorageFile open_file{
			co_await open_picker.PickSingleFileAsync()
		};
		m_mutex_event.unlock();
		open_picker = nullptr;
		// ストレージファイルがヌルポインターか判定する.
		if (open_file != nullptr) {
			// 待機カーソルを表示, 表示する前のカーソルを得る.
			const CoreCursor& prev_cur = file_wait_cursor();
			// ストレージファイルを非同期に読む.
			auto _{
				co_await file_read_async<false, false>(open_file)
			};
			// カーソルを元に戻す.
			Window::Current().CoreWindow().PointerCursor(prev_cur);
			// ストレージファイルを解放する.
			open_file = nullptr;
		}
	}

	//-------------------------------
	// ストレージファイルを非同期に読む.
	// SUSPEND	ライフサイクルが中断のとき true
	// SETTING	「用紙設定を保存」のとき true
	// s_file	読み込むストレージファイル
	// 戻り値	読み込めたら S_OK.
	//-------------------------------
	template <bool SUSPEND, bool SETTING>
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
			slist_clear(m_main_sheet.m_shape_list);
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
			dt_read(m_find_text, dt_reader);
			dt_read(m_find_repl, dt_reader);
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

			// メイン用紙を読み込む.
			m_main_sheet.read(dt_reader);

			// シートのみ読み込むか判定する.
			if constexpr (SETTING) {
				hr = S_OK;
			}
			else {
				if (slist_read(m_main_sheet.m_shape_list, dt_reader)) {
					// 中断されたか判定する.
					if constexpr (SUSPEND) {
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
		if (hr != S_OK) {
			message_show(ICON_ALERT, RES_ERR_READ, s_file.Path());
		}
		else {
			if constexpr (!SUSPEND && !SETTING) {
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
			ApplicationView::GetForCurrentView().Title(ResourceLoader::GetForCurrentView().GetString(UNTITLED));
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
				message_show(ICON_ALERT, RES_ERR_RECENT, to_hstring(n + 1));
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
				co_await file_read_async<false, false>(s_file);
				// カーソルを元に戻す.
				Window::Current().CoreWindow().PointerCursor(prev_cur);
				// ストレージファイルを破棄する.
				s_file = nullptr;
			}
			else {
				// 取得できないならば,
				message_show(ICON_ALERT, RES_ERR_RECENT, item[0].Metadata);
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
		auto const& mru_entries = StorageApplicationPermissions::MostRecentlyUsedList().Entries();
		const auto ent_size = mru_entries.Size();
		AccessListEntry items[MRU_MAX];
		winrt::hstring data[MRU_MAX];
		mru_entries.GetMany(0, items);
		// アクセスリストのファイル名を配列に格納する.
		for (uint32_t i = 0; i < MRU_MAX; i++) {
			if (i < ent_size) {
				data[i] = items[i].Metadata;
			}
			else {
				data[i] = L"";
			}
		}
		// 配列をメニュー項目に格納する.
		mfi_file_recent_1().Text(data[0]);
		mfi_file_recent_2().Text(data[1]);
		mfi_file_recent_3().Text(data[2]);
		mfi_file_recent_4().Text(data[3]);
		mfi_file_recent_5().Text(data[4]);
	}

	//-------------------------------
	// 名前を付けてファイルに非同期に保存する.
	// svg_allowed	SVG への保存を許す.
	//-------------------------------
	template <bool SVG_ARROWED>
	IAsyncAction MainPage::file_save_as_click_async(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		HRESULT hr = E_FAIL;
		try {
			// ファイル保存ピッカーを得る.
			// ファイルタイプに拡張子 GPF とその説明を追加する.
			FileSavePicker save_picker{ FileSavePicker() };
			const winrt::hstring desc_gpf{
				ResourceLoader::GetForCurrentView().GetString(RES_DESC_GPF)
			};
			save_picker.FileTypeChoices().Insert(desc_gpf, TYPE_GPF);
			// SVG への保存を許すか判定する.
			if constexpr (SVG_ARROWED) {
				// ファイルタイプに拡張子 SVG とその説明を追加する.
				const winrt::hstring desc_svg{
					ResourceLoader::GetForCurrentView().GetString(RES_DESC_SVG)
				};
				save_picker.FileTypeChoices().Insert(desc_svg, TYPE_SVG);
				const winrt::hstring desc_pdf{ L"" };
				save_picker.FileTypeChoices().Insert(desc_pdf, TYPE_PDF);
			}

			// ドキュメントライブラリーを保管場所に設定する.
			const PickerLocationId loc_id = PickerLocationId::DocumentsLibrary;
			save_picker.SuggestedStartLocation(loc_id);

			// 最近使ったファイルのトークンが空か判定する.
			if (m_file_token_mru.empty()) {
				// 提案されたファイル名に拡張子を格納する.
				save_picker.SuggestedFileName(RES_EXT_GPF);
			}
			else {
				// 最近使ったファイルのトークンからストレージファイルを得る.
				StorageFile recent_file{
					co_await file_recent_token_async(m_file_token_mru)
				};
				// ストレージファイルを得たなら,
				if (recent_file != nullptr) {
					// ファイルタイプが RES_EXT_GPF か判定する.
					if (recent_file.FileType() == RES_EXT_GPF) {
						// ファイル名を, 提案するファイル名に格納する.
						save_picker.SuggestedFileName(recent_file.Name());
					}
					recent_file = nullptr;
				}
			}
			// ファイル保存ピッカーを表示し, ストレージファイルを得る.
			m_mutex_event.lock();
			StorageFile save_file{
				co_await save_picker.PickSaveFileAsync()
			};
			m_mutex_event.unlock();
			// ピッカーを破棄する.
			save_picker = nullptr;
			// ストレージファイルを取得したか判定する.
			if (save_file != nullptr) {
				// 待機カーソルを表示, 表示する前のカーソルを得る.
				const CoreCursor& prev_cur = file_wait_cursor();
				// ファイルタイプが SVG か判定する.
				const auto f_type = save_file.FileType();
				if (f_type == RES_EXT_SVG) {
					// 図形データを SVG としてストレージファイルに非同期に書き込み, 結果を得る.
					hr = co_await file_write_svg_async(save_file);
				}
				else if (f_type == RES_EXT_GPF) {
					// 図形データをストレージファイルに非同期に書き込み, 結果を得る.
					hr = co_await file_write_gpf_async<false, false>(save_file);
				}
				else if (f_type == L".pdf") {
					hr = co_await file_write_pdf_async(save_file);
				}
				// カーソルを元に戻す.
				Window::Current().CoreWindow().PointerCursor(prev_cur);
				// ストレージファイルを破棄する.
				save_file = nullptr;
			}
			else {
				// ファイル保存ピッカーでキャンセルが押された.
				hr = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, エラーコードを結果に格納する.
			hr = e.code();
		}
		if (hr != S_OK) {
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, RES_ERR_WRITE, m_file_token_mru);
		}
	}
	template IAsyncAction MainPage::file_save_as_click_async<true>(IInspectable const&, RoutedEventArgs const&) noexcept;
	template IAsyncAction MainPage::file_save_as_click_async<false>(IInspectable const&, RoutedEventArgs const&) noexcept;

	//-------------------------------
	// ファイルメニューの「上書き保存」が選択された
	//-------------------------------
	IAsyncAction MainPage::file_save_click_async(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		// 最近使ったファイルのトークンからストレージファイルを得る.
		StorageFile recent_file{
			co_await file_recent_token_async(m_file_token_mru)
		};
		// ストレージファイルが空の場合,
		if (recent_file == nullptr) {
			// 名前を付けてファイルに非同期に保存する
			constexpr bool SVG_ALLOWED = true;
			co_await file_save_as_click_async<!SVG_ALLOWED>(nullptr, nullptr);
		}
		// ストレージファイルを得た場合,
		else {
			// 待機カーソルを表示, 表示する前のカーソルを得る.
			const CoreCursor& prev_cur = file_wait_cursor();	// 前のカーソル
			// 図形データをストレージファイルに非同期に書き込み, 結果を得る.
			const HRESULT hr = co_await file_write_gpf_async<false, false>(recent_file);
			recent_file = nullptr;
			// カーソルを元に戻す.
			Window::Current().CoreWindow().PointerCursor(prev_cur);
			// 結果が S_OK 以外か判定する.
			if (hr != S_OK) {
				// スレッドをメインページの UI スレッドに変える.
				//co_await winrt::resume_foreground(Dispatcher());
				// 「ファイルに書き込めません」メッセージダイアログを表示する.
				message_show(ICON_ALERT, RES_ERR_WRITE, m_file_token_mru);
			}
		}
	}

	//-------------------------------
	// 画像用のファイル保存ピッカーを開いて, ストレージファイルを得る.
	//-------------------------------
	IAsyncOperation <StorageFile> MainPage::file_pick_save_image_async(const wchar_t sug_name[])
	{
		// ResourceLoader::GetForCurrentView はフォアグラウンド.
		const ResourceLoader& res_loader = ResourceLoader::GetForCurrentView();
		const winrt::hstring desc_bmp{ res_loader.GetString(L"str_desc_bmp") };
		const winrt::hstring desc_gif{ res_loader.GetString(L"str_desc_gif") };
		const winrt::hstring desc_jpg{ res_loader.GetString(L"str_desc_jpg") };
		const winrt::hstring desc_png{ res_loader.GetString(L"str_desc_png") };
		const winrt::hstring desc_tif{ res_loader.GetString(L"str_desc_tif") };

		FileSavePicker image_picker{ FileSavePicker() };

		// 保存ピッカーに, 既定のエンコード識別子の説明を設定する.
		if (m_enc_id == BitmapEncoder::GifEncoderId()) {
			image_picker.FileTypeChoices().Insert(desc_gif, TYPE_GIF);
		}
		else if (m_enc_id == BitmapEncoder::JpegEncoderId()) {
			image_picker.FileTypeChoices().Insert(desc_jpg, TYPE_JPG);
		}
		else if (m_enc_id == BitmapEncoder::PngEncoderId()) {
			image_picker.FileTypeChoices().Insert(desc_png, TYPE_PNG);
		}
		else if (m_enc_id == BitmapEncoder::TiffEncoderId()) {
			image_picker.FileTypeChoices().Insert(desc_tif, TYPE_TIF);
		}
		else {
			image_picker.FileTypeChoices().Insert(desc_bmp, TYPE_BMP);
			m_enc_id = BitmapEncoder::BmpEncoderId();
		}

		// 上記以外のエンコード識別子の説明を設定する.
		if (m_enc_id != BitmapEncoder::BmpEncoderId()) {
			image_picker.FileTypeChoices().Insert(desc_bmp, TYPE_BMP);
		}
		if (m_enc_id != BitmapEncoder::GifEncoderId()) {
			image_picker.FileTypeChoices().Insert(desc_gif, TYPE_GIF);
		}
		if (m_enc_id != BitmapEncoder::JpegEncoderId()) {
			image_picker.FileTypeChoices().Insert(desc_jpg, TYPE_JPG);
		}
		if (m_enc_id != BitmapEncoder::PngEncoderId()) {
			image_picker.FileTypeChoices().Insert(desc_png, TYPE_PNG);
		}
		if (m_enc_id != BitmapEncoder::TiffEncoderId()) {
			image_picker.FileTypeChoices().Insert(desc_tif, TYPE_TIF);
		}

		// 画像ライブラリーを保管場所に設定する.
		const PickerLocationId loc_id = PickerLocationId::PicturesLibrary;
		image_picker.SuggestedStartLocation(loc_id);

		// ピッカーに, あらかじめ表示されるファイル名を設定する.
		if (sug_name != nullptr) {
			image_picker.SuggestedFileName(sug_name);
		}

		// ピッカーを表示しストレージファイルを得る.
		StorageFile img_file{
			co_await image_picker.PickSaveFileAsync()
		};
		image_picker = nullptr;
		if (img_file != nullptr) {

			// 得られたファイルの拡張子を既定のエンコード識別子に設定する.
			if (std::find(TYPE_BMP.begin(), TYPE_BMP.end(), img_file.FileType()) != TYPE_BMP.end()) {
				m_enc_id = BitmapEncoder::BmpEncoderId();
			}
			else if (std::find(TYPE_GIF.begin(), TYPE_GIF.end(), img_file.FileType()) != TYPE_GIF.end()) {
				m_enc_id = BitmapEncoder::GifEncoderId();
			}
			else if (std::find(TYPE_JPG.begin(), TYPE_JPG.end(), img_file.FileType()) != TYPE_JPG.end()) {
				m_enc_id = BitmapEncoder::JpegEncoderId();
			}
			else if (std::find(TYPE_PNG.begin(), TYPE_PNG.end(), img_file.FileType()) != TYPE_PNG.end()) {
				m_enc_id = BitmapEncoder::PngEncoderId();
			}
			else if (std::find(TYPE_TIF.begin(), TYPE_TIF.end(), img_file.FileType()) != TYPE_TIF.end()) {
				m_enc_id = BitmapEncoder::TiffEncoderId();
			}
			else {
				img_file = nullptr;
			}
		}
		co_return img_file;
	}

	//-------------------------------
	// 図形データをストレージファイルに非同期に書き込む.
	// SUSPEND	ライフサイクルが中断のとき true
	// SETTING	「用紙設定を保存」のとき true
	// s_file	ストレージファイル
	// 戻り値	書き込みに成功したら true
	//-------------------------------
	template <bool SUSPEND, bool SETTING>
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
			//dt_write(m_drawing_poly_opt, dt_writer);
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_drawing_poly_opt.m_vertex_cnt));
			dt_writer.WriteBoolean(m_drawing_poly_opt.m_regular);
			dt_writer.WriteBoolean(m_drawing_poly_opt.m_vertex_up);
			dt_writer.WriteBoolean(m_drawing_poly_opt.m_end_closed);
			dt_writer.WriteBoolean(m_drawing_poly_opt.m_clockwise);
			dt_write(m_find_text, dt_writer);
			dt_write(m_find_repl, dt_writer);
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

			m_main_sheet.write(dt_writer);
			if constexpr (SUSPEND) {
				// 消去された図形も含めて書き込む.
				// 操作スタックも書き込む.
				slist_write<!REDUCE>(m_main_sheet.m_shape_list, dt_writer);
				ustack_write(dt_writer);
			}
			else if constexpr (!SETTING) {
				// 消去された図形は省いて書き込む.
				// 操作スタックは消去する.
				slist_write<REDUCE>(m_main_sheet.m_shape_list, dt_writer);
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
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, RES_ERR_WRITE, s_file.Path());
		}
		else {
			// 中断ではない, かつ設定ではないか判定する.
			if constexpr (!SUSPEND && !SETTING) {
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

	//詳細PDF入門 ー 実装して学ぼう！PDFファイルの構造とその書き方読み方
	///F0 36 Tf
	//40 TL
	//(Hello, world!) Tj
	// Tfというフォント演算子
	// Tjという演算子に (Hello, world!) という引数を渡してテキストを表示
	//50 650 m 150 750 l S
	// m は移動 (move) 演算子
	//lは直線 (line) 演算子
	// Sはストローク (stroke) 演算子
	// 190 650 100 100 re f
	// reは矩形 (rectangle) 演算子
	// fは塗りつぶし (fill) を行う演算子
	// cはベジェ曲線を生成する演算子
	// rgは塗りつぶしに対する色で、RGはストロークに対する色
	// gは塗りつぶしの色を灰色の0(黒)〜1(白)で指定する演算子で、Gはそのストローク版
	IAsyncOperation<winrt::hresult> MainPage::file_write_pdf_async(StorageFile pdf_file)
	{
		// 
		HRESULT hr = S_OK;
		try {
			// D2D の論理 DPI (96dpi) を PDF の 72dpi に変換する.
			const float dpi = m_main_sheet.m_d2d.m_logical_dpi;
			const float w_pt = m_main_sheet.m_sheet_size.width * 72.0f / dpi;
			const float h_pt = m_main_sheet.m_sheet_size.height * 72.0f / dpi;
			// ファイル更新の遅延を設定する.
			CachedFileManager::DeferUpdates(pdf_file);
			// ストレージファイルを開いてランダムアクセスストリームを得る.
			const IRandomAccessStream& pdf_stream{
				co_await pdf_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			// ランダムアクセスストリームの先頭からデータライターを作成する.
			// 必ず先頭を指定する.
			DataWriter dt_writer{
				DataWriter(pdf_stream.GetOutputStreamAt(0))
			};
			// ヘッダー
			size_t len;
			len = dt_write(
				"%PDF-1.6\n"
				"%\xff\xff\xff\xff\n",
				dt_writer);
			// ページツリーディクショナリ
			const size_t obj_1 = len;
			len = dt_write(
				"1 0 obj\n"
				"<<\n"
				"/Type /Pages\n"
				"/Count 1\n"
				"/Kids[2 0 R]\n"
				">>\n"
				"endobj\n", 
				dt_writer
			);
			// ページオブジェクト
			const size_t obj_2 = obj_1 + len;
			char buf[1024];
			sprintf_s(buf,
				"2 0 obj\n"
				"<<\n"
				"/Type /Page\n"
				"/MediaBox[0 0 %f %f]\n"
				"/Resources 3 0 R\n"
				"/Parent 1 0 R\n"
				"/Contents[4 0 R]\n"
				">>\n"
				"endobj\n",
				w_pt, h_pt);
			len = dt_write(buf, dt_writer);
			// リソース
			const size_t obj_3 = obj_2 + len;
			len = dt_write(
				"3 0 obj\n"
				"<<\n"
				">>\n"
				"endobj\n",
				dt_writer
			);
			// コンテントオブジェクト
			const size_t obj_4 = obj_3 + len;
			sprintf_s(buf,
				"4 0 obj\n"
				"<<\n"
				">>\n"
				"stream\n"
				"%f 0 0 -%f 0 %f cm\n"
				"q\n",
				72.0f / dpi, 72.0f / dpi,
				h_pt);
			len = dt_write(buf, dt_writer);
			for (const auto s : m_main_sheet.m_shape_list) {
				len += s->write_pdf(dt_writer);
			}
			len += dt_write(
				"Q\n"
				"endstream\n"
				"endobj\n", dt_writer
			);
			// ドキュメントカタログ
			const size_t obj_5 = obj_4 + len;
			len = dt_write(
				"5 0 obj\n"
				"<<\n"
				"/Type /Catalog"
				"/Pages 1 0 R\n"
				">>\n"
				"endobj\n", dt_writer
			);
			// クロスリファレンス
			const size_t xref = obj_5 + len;
			sprintf_s(
				buf,
				"xref\n"
				"0 6\n"
				"0000000000 65535 f\n"
				"%010zu 00000 n\n"
				"%010zu 00000 n\n"
				"%010zu 00000 n\n"
				"%010zu 00000 n\n"
				"%010zu 00000 n\n",
				obj_1, obj_2, obj_3, obj_4, obj_5
			);
			// トレイラーと EOF
			dt_write(buf, dt_writer);
			sprintf_s(
				buf,
				"trailer\n"
				"<<\n"
				"/Size 6\n"
				"/Root 5 0 R\n"
				">>\n"
				"startxref\n"
				"%zu\n"
				"%%%%EOF\n",
				xref
			);
			dt_write(buf, dt_writer);
			// ストリームの現在位置をストリームの大きさに格納する.
			pdf_stream.Size(pdf_stream.Position());
			// バッファ内のデータをストリームに出力する.
			co_await dt_writer.StoreAsync();
			// ストリームをフラッシュする.
			co_await pdf_stream.FlushAsync();
			// 遅延させたファイル更新を完了し, 結果を判定する.
			if (co_await CachedFileManager::CompleteUpdatesAsync(pdf_file) == FileUpdateStatus::Complete) {
				// 完了した場合, S_OK を結果に格納する.
				hr = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			hr = e.code();
		}
		co_return hr;
	}

	//-------------------------------
	// 図形データを SVG としてストレージファイルに非同期に書き込む.
	// svg_file	書き込み先のファイル
	// 戻り値	書き込めた場合 S_OK
	//-------------------------------
	IAsyncOperation<winrt::hresult> MainPage::file_write_svg_async(StorageFile svg_file)
	{
		constexpr char XML_DEC[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" SVG_NEW_LINE;
		constexpr char DOCTYPE[] = "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" SVG_NEW_LINE;

		m_mutex_exit.lock();
		// コルーチンの開始時のスレッドコンテキストを保存する.
//		winrt::apartment_context context;
		// スレッドをバックグラウンドに変える.
//		co_await winrt::resume_background();
		HRESULT hr = E_FAIL;
		try {
			// ファイル更新の遅延を設定する.
			CachedFileManager::DeferUpdates(svg_file);
			// ストレージファイルを開いてランダムアクセスストリームを得る.
			const IRandomAccessStream& svg_stream{
				co_await svg_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			// ランダムアクセスストリームの先頭からデータライターを作成する.
			DataWriter dt_writer{
				DataWriter(svg_stream.GetOutputStreamAt(0))
			};
			// XML 宣言を書き込む.
			dt_write_svg(XML_DEC, dt_writer);
			// DOCTYPE を書き込む.
			dt_write_svg(DOCTYPE, dt_writer);
			// SVG 開始タグを書き込む.
			{
				const auto size = m_main_sheet.m_sheet_size;	// 用紙の大きさ
				const auto unit = m_len_unit;	// 長さの単位
				const auto dpi = m_main_sheet.m_d2d.m_logical_dpi;	// 論理 DPI
				const auto color = m_main_sheet.m_sheet_color;	// 背景色
				constexpr char SVG_TAG[] =
					"<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" ";
				constexpr char* SVG_UNIT_PX = "px";
				constexpr char* SVG_UNIT_IN = "in";
				constexpr char* SVG_UNIT_MM = "mm";
				constexpr char* SVG_UNIT_PT = "pt";

				// SVG タグの開始を書き込む.
				dt_write_svg(SVG_TAG, dt_writer);

				// 単位付きで幅と高さの属性を書き込む.
				char buf[256];
				double w;	// 単位変換後の幅
				double h;	// 単位変換後の高さ
				char* u;	// 単位
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
				// SVG で使用できる上記の単位以外はすべてピクセル.
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

			// 図形リストの各図形について以下を繰り返す.
			for (auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				// 図形が画像か判定する.
				if (typeid(*s) == typeid(ShapeImage)) {
					// 提案される画像ファイル名を得る.
					// SVG ファイル名を xxxx.svg とすると,
					// 得られるファイル名は xxxx_yyyymmddhhmmss_999.bmp になる
					const size_t NAME_LEN = 1024;
					wchar_t image_name[NAME_LEN];
					{
						static uint32_t magic_num = 0;	// ミリ秒の代わり
						const time_t t = time(nullptr);
						struct tm tm;
						localtime_s(&tm, &t);
						swprintf(image_name, NAME_LEN - 20, L"%s", svg_file.Name().data());
						const wchar_t* const dot_ptr = wcsrchr(image_name, L'.');
						const size_t dot_len = (dot_ptr != nullptr ? dot_ptr - image_name : wcslen(image_name));	// ピリオドまでの長さ
						const size_t tail_len = dot_len + wcsftime(image_name + dot_len, NAME_LEN - 8 - dot_len, L"_%Y%m%d%H%M%S_", &tm);
						swprintf(image_name + tail_len, NAME_LEN - tail_len, L"%03d", magic_num++);
					}

					// 画像用のファイル保存ピッカーを開いて, ストレージファイルを得る.
					// ピッカーを開くので UI スレッドに変える.
					ShapeImage* const t = static_cast<ShapeImage*>(s);
//					co_await winrt::resume_foreground(Dispatcher());
					StorageFile image_file{
						co_await file_pick_save_image_async(image_name)
					};
//					co_await winrt::resume_background();
					if (image_file != nullptr) {
						CachedFileManager::DeferUpdates(image_file);
						IRandomAccessStream image_stream{
							co_await image_file.OpenAsync(FileAccessMode::ReadWrite)
						};
						const bool ret = co_await t->copy_to(m_enc_id, image_stream);
						// 遅延させたファイル更新を完了し, 結果を判定する.
						if (ret && co_await CachedFileManager::CompleteUpdatesAsync(image_file) == FileUpdateStatus::Complete) {
							wcscpy_s(image_name, NAME_LEN, image_file.Path().c_str());
						}
						image_stream.Close();
						image_stream = nullptr;
						image_file = nullptr;

						// スレッドコンテキストを復元する.
						//co_await context;
						t->write_svg(image_name, dt_writer);
					}
					else {
						t->write_svg(dt_writer);
					}
				}
				else {
					s->write_svg(dt_writer);
				}
			}
			// SVG 終了タグを書き込む.
			dt_write_svg("</svg>" SVG_NEW_LINE, dt_writer);
			// ストリームの現在位置をストリームの大きさに格納する.
			svg_stream.Size(svg_stream.Position());
			// バッファ内のデータをストリームに出力する.
			co_await dt_writer.StoreAsync();
			// ストリームをフラッシュする.
			co_await svg_stream.FlushAsync();
			// 遅延させたファイル更新を完了し, 結果を判定する.
			if (co_await CachedFileManager::CompleteUpdatesAsync(svg_file) == FileUpdateStatus::Complete) {
				// 完了した場合, S_OK を結果に格納する.
				hr = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, エラーコードを結果に格納する.
			hr = e.code();
		}
		// 結果が S_OK 以外か判定する.
		if (hr != S_OK) {
			// スレッドをメインページの UI スレッドに変える.
//			co_await winrt::resume_foreground(Dispatcher());
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, RES_ERR_WRITE, svg_file.Path());
		}
		// スレッドコンテキストを復元する.
//		co_await context;
		m_mutex_exit.unlock();
		// 結果を返し終了する.
		co_return hr;
	}

	// ファイルメニューの「新規」が選択された
	IAsyncAction MainPage::file_new_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		// スタックが更新された, かつ確認ダイアログの応答が「キャンセル」か判定する.
		if (m_ustack_is_changed && !co_await file_confirm_dialog()) {
			co_return;
		}
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_close_click(nullptr, nullptr);
		}
		ustack_clear();
		slist_clear(m_main_sheet.m_shape_list);

#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			// 「メモリリーク」メッセージダイアログを表示する.
			message_show(ICON_DEBUG, DEBUG_MSG, {});
		}
#endif

		ShapeText::release_available_fonts();

		ShapeText::set_available_fonts(m_main_sheet.m_d2d);

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

		if (co_await sheet_prop_load_async() != S_OK) {
			// 読み込みに失敗した場合,
			sheet_init();
			m_len_unit = LEN_UNIT::PIXEL;
			m_color_code = COLOR_CODE::DEC;
			m_vert_stick = VERT_STICK_DEF_VAL;
			m_status_bar = STATUS_BAR_DEF_VAL;
		}

		// 用紙の左上位置と右下位置を初期化する.
		{
			m_main_min = D2D1_POINT_2F{ 0.0F, 0.0F };
			m_main_max = D2D1_POINT_2F{ m_main_sheet.m_sheet_size.width, m_main_sheet.m_sheet_size.height };
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

