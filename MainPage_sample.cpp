//-------------------------------
// MainPage_sample.cpp
// 見本
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::Foundation::Uri;
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapDecoder;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::Graphics::Imaging::SoftwareBitmap;
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::Storage::StorageFile;
	using winrt::Windows::Storage::Streams::IRandomAccessStream;
	using winrt::Windows::UI::Xaml::Controls::ContentDialog;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogOpenedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogClosedEventArgs;
	using winrt::Windows::UI::Xaml::RoutedEventArgs;
	using winrt::Windows::UI::Xaml::SizeChangedEventArgs;

	// 見本を表示する
	void MainPage::sample_draw(void)
	{
#if defined(_DEBUG)
		if (!scp_sample_panel().IsLoaded()) {
			return;
		}
#endif
		if (m_d2d_mutex.try_lock() != true) {
			// ロックできない場合
			return;
		}

		m_sample_dx.m_d2d_context->SaveDrawingState(m_sample_dx.m_state_block.get());
		m_sample_dx.m_d2d_context->BeginDraw();
		m_sample_dx.m_d2d_context->Clear(m_sample_sheet.m_sheet_color);
		const float offset = static_cast<FLOAT>(std::fmod(m_sample_sheet.m_sheet_size.width * 0.5, m_sample_sheet.m_grid_base + 1.0));
		m_sample_sheet.m_grid_offset.x = offset;
		m_sample_sheet.m_grid_offset.y = offset;
		m_sample_sheet.draw(m_sample_dx);
		m_sample_dx.m_d2d_context->EndDraw();
		m_sample_dx.m_d2d_context->RestoreDrawingState(m_sample_dx.m_state_block.get());
		m_sample_dx.Present();
		m_d2d_mutex.unlock();
	}

	//　見本リストビューがロードされた.
	void MainPage::sample_list_loaded(IInspectable const&, RoutedEventArgs const&)
	{
		const auto item = lv_sample_list().SelectedItem();
		if (item != nullptr) {
			lv_sample_list().ScrollIntoView(item);
		}
	}

	// 見本ダイアログが開かれた.
	void MainPage::sample_opened(ContentDialog const&, ContentDialogOpenedEventArgs const&)
	{
	}

	// 見本ダイアログが閉じられた.
	void MainPage::sample_closed(ContentDialog const&, ContentDialogClosedEventArgs const&)
	{
	}

	IAsyncAction MainPage::sample_image_load_async(const float panel_w, const float panel_h)
	{
		bool ok;
		winrt::apartment_context context;
		co_await winrt::resume_background();
		try {
			const StorageFile samp_file{ co_await StorageFile::GetFileFromApplicationUriAsync(Uri{ L"ms-appx:///Assets/4.1.05.tiff" }) };
			const IRandomAccessStream samp_strm{ co_await samp_file.OpenAsync(FileAccessMode::Read) };
			const BitmapDecoder samp_dec{ co_await BitmapDecoder::CreateAsync(samp_strm) };
			const SoftwareBitmap samp_bmp{ co_await samp_dec.GetSoftwareBitmapAsync(BitmapPixelFormat::Bgra8, BitmapAlphaMode::Straight) };
			ShapeImage* samp_shape = new ShapeImage(
				D2D1_POINT_2F{ static_cast<FLOAT>(panel_w * 0.125), static_cast<FLOAT>(panel_h * 0.125) },
				D2D1_SIZE_F{ static_cast<float>(panel_w * 0.75), static_cast<FLOAT>(panel_h * 0.75) },
				samp_bmp,
				m_sample_sheet.m_image_opac);
			samp_bmp.Close();
			m_sample_sheet.m_shape_list.push_back(samp_shape);
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
			sample_draw();
		}
	}

	// 用紙のスワップチェーンパネルがロードされた.
#if defined(_DEBUG)
	void MainPage::sample_panel_loaded(IInspectable const& sender, RoutedEventArgs const& args)
	{
		if (sender != scp_sample_panel()) {
			return;
		}
#else
	void MainPage::sample_panel_loaded(IInspectable const&, RoutedEventArgs const& args)
	{
#endif // _DEBUG
		const auto& swap_chain_panel = scp_sample_panel();
		m_sample_dx.SetSwapChainPanel(swap_chain_panel);
		sample_draw();
	}

	// 用紙のスワップチェーンパネルの寸法が変わった.
#if defined(_DEBUG)
	void MainPage::sample_panel_size_changed(IInspectable const& sender, winrt::Windows::UI::Xaml::SizeChangedEventArgs const& args)
	{
		if (sender != scp_sample_panel()) {
			return;
		}
#else
	void MainPage::sample_panel_size_changed(IInspectable const&, SizeChangedEventArgs const& args)
	{
#endif	// _DEBUG
		m_sample_sheet.m_sheet_size.width = static_cast<FLOAT>(args.NewSize().Width);
		m_sample_sheet.m_sheet_size.height = static_cast<FLOAT>(args.NewSize().Height);
		m_sample_dx.SetLogicalSize2(m_sample_sheet.m_sheet_size);
		sample_draw();
	}

	void MainPage::sample_panel_scale_changed(IInspectable const&, IInspectable const&)
	{
		m_sample_dx.SetCompositionScale(scp_sample_panel().CompositionScaleX(), scp_sample_panel().CompositionScaleY());
	}

}
