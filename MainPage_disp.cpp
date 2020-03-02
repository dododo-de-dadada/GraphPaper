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
	// sender	イベントが発生したデイスプレイ
	void MainPage::disp_contents_invalidated(DisplayInformation const& sender, IInspectable const& /*args*/)
	{
#if defined(_DEBUG)
		if (sender != DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		{
			std::lock_guard<std::mutex> lock(m_dx_mutex);
			m_page_dx.ValidateDevice();
			m_samp_dx.ValidateDevice();
		}
		if (scp_samp_panel().IsLoaded()) {
			samp_draw();
		}
		if (scp_page_panel().IsLoaded()) {
			draw_page();
		}
	}

	// デイスプレイの DPI が変わった
	// sender	イベントが発生したデイスプレイ
	void MainPage::disp_dpi_changed(DisplayInformation const& sender, IInspectable const& /*args*/)
	{
#if defined(_DEBUG)
		if (sender == DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		{
			std::lock_guard<std::mutex> lock(m_dx_mutex);
			const auto dpi = sender.LogicalDpi();
			m_page_dx.SetDpi(dpi);
			m_samp_dx.SetDpi(dpi);
		}
		if (scp_samp_panel().IsLoaded()) {
			samp_draw();
		}
		if (scp_page_panel().IsLoaded()) {
			draw_page();
		}
	}

	// デイスプレイの向きが変わった
	// sender	イベントが発生したデイスプレイ
	void MainPage::disp_orientation_changed(DisplayInformation const& sender, IInspectable const& /*args*/)
	{
#if defined(_DEBUG)
		if (sender != DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		{
			std::lock_guard<std::mutex> lock(m_dx_mutex);
			const auto ori = sender.CurrentOrientation();
			m_page_dx.SetCurrentOrientation(ori);
			m_samp_dx.SetCurrentOrientation(ori);
		}
		if (scp_samp_panel().IsLoaded()) {
			samp_draw();
		}
		if (scp_page_panel().IsLoaded()) {
			draw_page();
		}
	}

}