//-------------------------------
// MainPage_disp.cpp
// 表示デバイスのハンドラー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Graphics::Display::DisplayInformation;

	// 再描画が必要になった
	// sender	イベントが発生した表示デバイス
	void MainPage::display_contents_invalidated(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender != DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		{
			m_d2d_mutex.lock();
			m_main_d2d.ValidateDevice();
			m_sample_d2d.ValidateDevice();
			m_d2d_mutex.unlock();
		}
		if (scp_sample_panel().IsLoaded()) {
			m_d2d_mutex.lock();
			m_sample_d2d.ValidateDevice();
			m_d2d_mutex.unlock();
			sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			m_d2d_mutex.lock();
			m_main_d2d.ValidateDevice();
			m_d2d_mutex.unlock();
			sheet_draw();
		}
	}

	// 表示デバイスの DPI が変わった
	// sender	イベントが発生した表示デバイス
	void MainPage::display_dpi_changed(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender == DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		if (scp_sample_panel().IsLoaded()) {
			m_d2d_mutex.lock();
			const float logical_dpi = sender.LogicalDpi();
			m_sample_d2d.SetDpi(logical_dpi);
			m_d2d_mutex.unlock();
			sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			m_d2d_mutex.lock();
			const float logical_dpi = sender.LogicalDpi();
			m_main_d2d.SetDpi(logical_dpi);
			m_d2d_mutex.unlock();
			sheet_draw();
		}
	}

	// 表示デバイスの向きが変わった
	// sender	イベントが発生した表示デバイス
	void MainPage::display_orientation_changed(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender != DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		if (scp_sample_panel().IsLoaded()) {
			m_d2d_mutex.lock();
			const auto ori = sender.CurrentOrientation();
			m_sample_d2d.SetCurrentOrientation(ori);
			m_d2d_mutex.unlock();
			sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			m_d2d_mutex.lock();
			const auto ori = sender.CurrentOrientation();
			m_main_d2d.SetCurrentOrientation(ori);
			m_d2d_mutex.unlock();
			sheet_draw();
		}
	}

}