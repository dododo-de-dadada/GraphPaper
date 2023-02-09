//-------------------------------
// MainPage_dialog.cpp
// 設定ダイアログ
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::Foundation::Uri;
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapDecoder;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::Storage::FileAccessMode;

	// 設定ダイアログのスライダーヘッダーに文字列を格納する.
	// S	スライダーの番号
	// text	文字列
	template<int S> void MainPage::dialog_set_slider_header(const winrt::hstring& text)
	{
		if constexpr (S == 0) {
			dialog_slider_0().Header(box_value(text));
		}
		if constexpr (S == 1) {
			dialog_slider_1().Header(box_value(text));
		}
		if constexpr (S == 2) {
			dialog_slider_2().Header(box_value(text));
		}
		if constexpr (S == 3) {
			dialog_slider_3().Header(box_value(text));
		}
	}
	template void MainPage::dialog_set_slider_header<0>(const winrt::hstring& text);
	template void MainPage::dialog_set_slider_header<1>(const winrt::hstring& text);
	template void MainPage::dialog_set_slider_header<2>(const winrt::hstring& text);
	template void MainPage::dialog_set_slider_header<3>(const winrt::hstring& text);


	// 設定ダイアログのスワップチェーンパネルを表示する
	void MainPage::dialog_draw(void)
	{
		if (!scp_dialog_panel().IsLoaded()) {
			return;
		}
		if (!m_mutex_draw.try_lock()) {
			// ロックできない場合
			return;
		}

		Shape::s_d2d_factory = m_dialog_d2d.m_d2d_factory.get();
		Shape::s_d2d_target = m_dialog_d2d.m_d2d_context.get();
		Shape::s_dwrite_factory = m_dialog_d2d.m_dwrite_factory.get();
		Shape::s_d2d_color_brush = m_dialog_page.m_color_brush.get();
		Shape::s_d2d_range_brush = m_dialog_page.m_range_brush.get();

		m_dialog_d2d.m_d2d_context->SaveDrawingState(m_dialog_page.m_state_block.get());
		m_dialog_d2d.m_d2d_context->BeginDraw();
		m_dialog_d2d.m_d2d_context->Clear(m_dialog_page.m_page_color);
		const float offset = static_cast<FLOAT>(std::fmod(m_dialog_page.m_page_size.width * 0.5, m_dialog_page.m_grid_base + 1.0));
		m_dialog_page.m_grid_offset.x = offset;
		m_dialog_page.m_grid_offset.y = offset;
		m_dialog_page.draw();
		const auto hr = m_dialog_d2d.m_d2d_context->EndDraw();
		m_dialog_d2d.m_d2d_context->RestoreDrawingState(m_dialog_page.m_state_block.get());
		m_dialog_d2d.Present();
		m_mutex_draw.unlock();
	}

	// ダイアログのリストビューがロードされた.
	void MainPage::dialog_list_loaded(IInspectable const&, RoutedEventArgs const&)
	{
		const auto item = lv_dialog_list().SelectedItem();
		if (item != nullptr) {
			lv_dialog_list().ScrollIntoView(item);
		}
	}

	// 属性ダイアログが開かれた.
	void MainPage::setting_dialog_opened(ContentDialog const& sender, ContentDialogOpenedEventArgs const&)
	{
	}

	// 属性ダイアログが閉じられた.
	void MainPage::setting_dialog_closed(ContentDialog const&, ContentDialogClosedEventArgs const&)
	{
	}

	// 属性ダイアログが閉じられた.
	void MainPage::setting_dialog_unloaded(IInspectable const&, RoutedEventArgs const&)
	{
		m_dialog_page.m_state_block = nullptr;
		m_dialog_page.m_color_brush = nullptr;
		m_dialog_page.m_range_brush = nullptr;
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
			const StorageFile file{ co_await StorageFile::GetFileFromApplicationUriAsync(Uri{ L"ms-appx:///Assets/4.1.05.tiff" }) };
			const IRandomAccessStream stream{ co_await file.OpenAsync(FileAccessMode::Read) };
			const BitmapDecoder decoder{ co_await BitmapDecoder::CreateAsync(stream) };
			const SoftwareBitmap bitmap{ co_await decoder.GetSoftwareBitmapAsync(BitmapPixelFormat::Bgra8, BitmapAlphaMode::Straight) };
			const D2D1_POINT_2F pos{ static_cast<FLOAT>(p_width * 0.125), static_cast<FLOAT>(p_height * 0.125) };
			const D2D1_SIZE_F size{ static_cast<float>(p_width * 0.75), static_cast<FLOAT>(p_height * 0.75) };
			ShapeImage* s = new ShapeImage(pos, size, bitmap, m_dialog_page.m_image_opac);
			bitmap.Close();

			m_dialog_page.m_shape_list.push_back(s);
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
		m_dialog_d2d.SetSwapChainPanel(scp_dialog_panel());
		m_dialog_d2d.m_d2d_factory->CreateDrawingStateBlock(m_dialog_page.m_state_block.put());
		m_dialog_d2d.m_d2d_context->CreateSolidColorBrush({}, m_dialog_page.m_color_brush.put());
		m_dialog_d2d.m_d2d_context->CreateSolidColorBrush({}, m_dialog_page.m_range_brush.put());

		dialog_draw();
	}

	// 属性のスワップチェーンパネルの寸法が変わった.
	void MainPage::dialog_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
	{
		if (sender != scp_dialog_panel()) {
			return;
		}
		m_dialog_page.m_page_size.width = static_cast<FLOAT>(args.NewSize().Width);
		m_dialog_page.m_page_size.height = static_cast<FLOAT>(args.NewSize().Height);
		m_dialog_d2d.SetLogicalSize2(m_dialog_page.m_page_size);

		dialog_draw();
	}

	// 属性のスワップチェーンパネルの倍率が変わった.
	void MainPage::dialog_panel_scale_changed(IInspectable const&, IInspectable const&)
	{
		const float comp_x = scp_dialog_panel().CompositionScaleX();
		const float comp_y = scp_dialog_panel().CompositionScaleY();
		m_dialog_d2d.SetCompositionScale(comp_x, comp_y);

		dialog_draw();
	}

}
