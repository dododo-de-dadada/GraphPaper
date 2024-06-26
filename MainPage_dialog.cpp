//-------------------------------
// MainPage_prop.cpp
// 設定ダイアログ
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ふつうは次の順序で呼ばれる.
	// 1. opend
	// 2. size_changed
	// 3. loaded
	// 4. scale_changed
	// 5. closed
	// ちなみに, unloaded は呼ばれない.
	// ところが, デバッグを続けていると, たまに,
	// 1. size_changed
	// 2. opened
	// 3. loaded
	// あるいは,
	// 1. size_changed
	// 2. loaded
	// 3. opened
	// の順番で呼ばれることがある.
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::Foundation::Uri;
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapDecoder;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::UI::Xaml::Window;
	using winrt::Windows::UI::Xaml::Controls::ListView;

//#ifdef _DEBUG
//	enum struct DEBUG_DIALOG {
//		NIL,
//		LOADED,
//		OPENED,
//		CLOSED,
//		UNLOADED,
//		SIZE_CHANGED,
//		SCALE_CHANGED
//	} debug_dialog[124]{};
//	int debug_dialog_cnt = 0;
//#endif

	// 属性ダイアログの図形を表示する.
	void MainPage::dialog_draw(void)
	{
		if (!scp_dialog_panel().IsLoaded()) {
			return;
		}
		if (!m_mutex_draw.try_lock()) {
			// ロックできない場合
			return;
		}
		// 描画前に必要な変数を格納する.
		m_dialog_sheet.begin_draw(m_dialog_d2d.m_d2d_context.get(), true, m_background_wic.get(), 1.0f);
		m_dialog_d2d.m_d2d_context->SaveDrawingState(SHAPE::m_state_block.get());
		m_dialog_d2d.m_d2d_context->BeginDraw();
		m_dialog_d2d.m_d2d_context->Clear(m_background_color);
		const D2D1_RECT_F w_rect{
			0, 0, m_dialog_d2d.m_logical_width, m_dialog_d2d.m_logical_height
		};
		if (m_background_show) {
			// 背景パターンを描画する,
			m_dialog_d2d.m_d2d_context->FillRectangle(w_rect, SHAPE::m_d2d_bitmap_brush.get());
		}
		SHAPE::m_d2d_color_brush->SetColor(m_dialog_sheet.m_sheet_color);
		m_dialog_d2d.m_d2d_context->FillRectangle(w_rect, SHAPE::m_d2d_color_brush.get());

		const float offset = static_cast<FLOAT>(std::fmod(m_dialog_sheet.m_sheet_size.width * 0.5, m_dialog_sheet.m_grid_base + 1.0));
		m_dialog_sheet.m_grid_offset.x = offset;
		m_dialog_sheet.m_grid_offset.y = offset;
		m_dialog_sheet.m_sheet_padding.left = 0.0f;
		m_dialog_sheet.m_sheet_padding.top = 0.0f;
		m_dialog_sheet.m_sheet_padding.right = 0.0f;
		m_dialog_sheet.m_sheet_padding.bottom = 0.0f;
		m_dialog_sheet.draw();
		winrt::check_hresult(
			m_dialog_d2d.m_d2d_context->EndDraw()
		);
		m_dialog_d2d.m_d2d_context->RestoreDrawingState(SHAPE::m_state_block.get());
		m_dialog_d2d.Present();
		m_mutex_draw.unlock();
	}

	// 属性ダイアログのリストがロードされた.
	void MainPage::dialog_list_loaded(IInspectable const& sender, RoutedEventArgs const&)
	{
		// 選択された行が表示されるようリストをスクロールする.
		const auto lv{ unbox_value<ListView>(sender) };
		const auto it = lv.SelectedItem();
		if (it != nullptr) {
			lv.ScrollIntoView(it);
		}
	}

	// 属性ダイアログが開いた.
	void MainPage::dialog_prop_opened(ContentDialog const&, ContentDialogOpenedEventArgs const&)
	{
//#ifdef _DEBUG
//		debug_dialog[debug_dialog_cnt++] = DEBUG_DIALOG::OPENED;
//#endif
	}

	// 属性ダイアログが閉じた.
	void MainPage::dialog_prop_closed(ContentDialog const&, ContentDialogClosedEventArgs const&)
	{
//#ifdef _DEBUG
//		debug_dialog[debug_dialog_cnt++] = DEBUG_DIALOG::CLOSED;
//		debug_dialog_cnt = 0;
//		if (debug_dialog[0] != DEBUG_DIALOG::OPENED) {
//			__debugbreak();
//		}
//#endif
	}

	// 属性ダイアログがアンロードされた
	void MainPage::dialog_prop_unloaded(IInspectable const&, RoutedEventArgs const&)
	{
//#ifdef _DEBUG
//		debug_dialog[debug_dialog_cnt++] = DEBUG_DIALOG::UNLOADED;
//#endif
		m_dialog_d2d.Trim();
	}

	// 属性の画像を読み込む
	// p_width	パネルの幅
	// p_height	パネルの高さ
	IAsyncAction MainPage::dialog_image_load_async(const float p_width, const float p_height)
	{
		bool ok;
		winrt::apartment_context context;
		co_await winrt::resume_background();
		try {
			const StorageFile file{
				co_await StorageFile::GetFileFromApplicationUriAsync(Uri{ L"ms-appx:///Assets/4.1.05.tiff" })
			};
			const IRandomAccessStream stream{
				co_await file.OpenAsync(FileAccessMode::Read)
			};
			const BitmapDecoder decoder{
				co_await BitmapDecoder::CreateAsync(stream)
			};
			const SoftwareBitmap bitmap{
				co_await decoder.GetSoftwareBitmapAsync(BitmapPixelFormat::Bgra8, BitmapAlphaMode::Straight)
			};
			const D2D1_POINT_2F pt{
				static_cast<FLOAT>(p_width * 0.125), static_cast<FLOAT>(p_height * 0.125)
			};
			const D2D1_SIZE_F size{
				static_cast<float>(p_width * 0.75), static_cast<FLOAT>(p_height * 0.75)
			};
			SHAPE_IMAGE* s = new SHAPE_IMAGE(pt, size, bitmap, m_dialog_sheet.m_image_opac);
			bitmap.Close();

			m_dialog_sheet.m_shape_list.push_back(s);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			ok = true;
		}
		catch (winrt::hresult_error&) {
			ok = false;
		}
		co_await context;
		if (ok) {
			dialog_draw();
		}
	}

	// 属性のスワップチェーンパネルが読み込まれた.
	void MainPage::dialog_panel_loaded(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender != scp_dialog_panel()) {
			return;
		}
//#ifdef _DEBUG
//		debug_dialog[debug_dialog_cnt++] = DEBUG_DIALOG::LOADED;
//#endif
//		dialog_draw();
	}

	// 属性のスワップチェーンパネルの寸法が変わった.
	void MainPage::dialog_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
	{
		if (sender != scp_dialog_panel()) {
			return;
		}
//#ifdef _DEBUG
//		debug_dialog[debug_dialog_cnt++] = DEBUG_DIALOG::SIZE_CHANGED;
//#endif
		m_dialog_d2d.SetSwapChainPanel(scp_dialog_panel());
		const float w = args.NewSize().Width;
		const float h = args.NewSize().Height;
		m_dialog_sheet.m_sheet_size.width = w;
		m_dialog_sheet.m_sheet_size.height = h;
		m_dialog_d2d.SetLogicalSize2({ w, h });
		dialog_draw();
	}

	// 属性のスワップチェーンパネルの倍率が変わった.
	void MainPage::dialog_panel_scale_changed(IInspectable const&, IInspectable const&)
	{
//#ifdef _DEBUG
//		debug_dialog[debug_dialog_cnt++] = DEBUG_DIALOG::SCALE_CHANGED;
//#endif
		const float x = scp_dialog_panel().CompositionScaleX();
		const float y = scp_dialog_panel().CompositionScaleY();
		m_dialog_d2d.SetCompositionScale(x, y);

		dialog_draw();
	}

}
