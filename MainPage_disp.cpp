//-------------------------------
// MainPage_disp.cpp
// 表示デバイスのハンドラー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 再描画が必要になった
	// sender	イベントが発生した表示デバイス
	void MainPage::disp_contents_invalidated(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender != DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		{
			m_dx_mutex.lock();
			m_sheet_dx.ValidateDevice();
			m_sample_dx.ValidateDevice();
			m_dx_mutex.unlock();
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			sheet_draw();
		}
	}

	// 表示デバイスの DPI が変わった
	// sender	イベントが発生した表示デバイス
	void MainPage::disp_dpi_changed(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender == DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		{
			m_dx_mutex.lock();
			const auto dpi = sender.LogicalDpi();
			m_sheet_dx.SetDpi(dpi);
			m_sample_dx.SetDpi(dpi);
			m_dx_mutex.unlock();
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			sheet_draw();
		}
	}

	// 表示デバイスの向きが変わった
	// sender	イベントが発生した表示デバイス
	void MainPage::disp_orientation_changed(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender != DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		{
			m_dx_mutex.lock();
			const auto ori = sender.CurrentOrientation();
			m_sheet_dx.SetCurrentOrientation(ori);
			m_sample_dx.SetCurrentOrientation(ori);
			m_dx_mutex.unlock();
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			sheet_draw();
		}
	}

}