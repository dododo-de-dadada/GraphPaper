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
	void MainPage::disp_contents_invalidated(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender != DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		{
			std::lock_guard<std::mutex> lock(m_dx_mutex);
			m_page_dx.ValidateDevice();
			m_sample_dx.ValidateDevice();
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
		if (scp_page_panel().IsLoaded()) {
			page_draw();
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
			m_sample_dx.SetDpi(dpi);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
		if (scp_page_panel().IsLoaded()) {
			page_draw();
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
			m_sample_dx.SetCurrentOrientation(ori);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
		if (scp_page_panel().IsLoaded()) {
			page_draw();
		}
	}

}