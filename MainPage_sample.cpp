//-------------------------------
// MainPage_sample.cpp
// 属性
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

	// 属性ダイアログのスライダーヘッダーに文字列を格納する.
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


	// 属性の見本を表示する
	void MainPage::dialog_draw(void)
	{
#if defined(_DEBUG)
		if (!scp_dialog_panel().IsLoaded()) {
			return;
		}
#endif
		if (!m_mutex_draw.try_lock()) {
			// ロックできない場合
			return;
		}

		Shape::s_factory = m_dialog_d2d.m_d2d_factory.get();
		Shape::s_target = m_dialog_d2d.m_d2d_context.get();
		Shape::s_dw_factory = m_dialog_d2d.m_dwrite_factory.get();
		Shape::s_color_brush = m_dialog_page.m_color_brush.get();
		Shape::s_range_brush = m_dialog_page.m_range_brush.get();

		m_dialog_d2d.m_d2d_context->SaveDrawingState(m_dialog_page.m_state_block.get());
		m_dialog_d2d.m_d2d_context->BeginDraw();
		m_dialog_d2d.m_d2d_context->Clear(m_dialog_page.m_page_color);
		const float offset = static_cast<FLOAT>(std::fmod(m_dialog_page.m_page_size.width * 0.5, m_dialog_page.m_grid_base + 1.0));
		m_dialog_page.m_grid_offset.x = offset;
		m_dialog_page.m_grid_offset.y = offset;
		m_dialog_page.draw();
		m_dialog_d2d.m_d2d_context->EndDraw();
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
	void MainPage::setting_dialog_opened(ContentDialog const&, ContentDialogOpenedEventArgs const&)
	{
	}

	// 属性ダイアログが閉じられた.
	void MainPage::setting_dialog_closed(ContentDialog const&, ContentDialogClosedEventArgs const&)
	{
	}

	// 属性ダイアログが閉じられた.
	void MainPage::setting_dialog_unloaded(IInspectable const&, RoutedEventArgs const&)
	{
	}

	// 属性の画像を読み込む
	// panel_w	表示の幅
	// panel_h	表示の高さ
	IAsyncAction MainPage::dialog_image_load_async(const float panel_w, const float panel_h)
	{
		bool ok;
		winrt::apartment_context context;
		co_await winrt::resume_background();
		try {
			const StorageFile samp_file{ co_await StorageFile::GetFileFromApplicationUriAsync(Uri{ L"ms-appx:///Assets/4.1.05.tiff" }) };
			const IRandomAccessStream samp_stream{ co_await samp_file.OpenAsync(FileAccessMode::Read) };
			const BitmapDecoder samp_decoder{ co_await BitmapDecoder::CreateAsync(samp_stream) };
			const SoftwareBitmap samp_bmp{ co_await samp_decoder.GetSoftwareBitmapAsync(BitmapPixelFormat::Bgra8, BitmapAlphaMode::Straight) };
			const D2D1_POINT_2F pos{ static_cast<FLOAT>(panel_w * 0.125), static_cast<FLOAT>(panel_h * 0.125) };
			const D2D1_SIZE_F size{ static_cast<float>(panel_w * 0.75), static_cast<FLOAT>(panel_h * 0.75) };
			ShapeImage* samp_shape = new ShapeImage(pos, size, samp_bmp, m_dialog_page.m_image_opac);
			samp_bmp.Close();
			m_dialog_page.m_shape_list.push_back(samp_shape);
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

#if defined(_DEBUG)
	// 属性のスワップチェーンパネルが読み込まれた.
	void MainPage::dialog_panel_loaded(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender != scp_dialog_panel()) {
			return;
		}
#else
	// 属性のスワップチェーンパネルが読み込まれた.
	void MainPage::dialog_panel_loaded(IInspectable const&, RoutedEventArgs const& args)
	{
#endif // _DEBUG
		const auto& swap_chain_panel = scp_dialog_panel();
		m_dialog_d2d.SetSwapChainPanel(swap_chain_panel);
		m_dialog_d2d.m_d2d_factory->CreateDrawingStateBlock(m_dialog_page.m_state_block.put());
		m_dialog_d2d.m_d2d_context->CreateSolidColorBrush({}, m_dialog_page.m_color_brush.put());
		m_dialog_d2d.m_d2d_context->CreateSolidColorBrush({}, m_dialog_page.m_range_brush.put());

		dialog_draw();
	}

#if defined(_DEBUG)
	// 属性のスワップチェーンパネルの寸法が変わった.
	void MainPage::dialog_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
	{
		if (sender != scp_dialog_panel()) {
			return;
		}
#else
	// 属性のスワップチェーンパネルの寸法が変わった.
	void MainPage::dialog_panel_size_changed(IInspectable const&, SizeChangedEventArgs const& args)
	{
#endif	// _DEBUG
		m_dialog_page.m_page_size.width = static_cast<FLOAT>(args.NewSize().Width);
		m_dialog_page.m_page_size.height = static_cast<FLOAT>(args.NewSize().Height);
		m_dialog_d2d.SetLogicalSize2(m_dialog_page.m_page_size);

		dialog_draw();
	}

	// 属性のスワップチェーンパネルの倍率が変わった.
	void MainPage::dialog_panel_scale_changed(IInspectable const&, IInspectable const&)
	{
		m_dialog_d2d.SetCompositionScale(scp_dialog_panel().CompositionScaleX(), scp_dialog_panel().CompositionScaleY());
	}

}
