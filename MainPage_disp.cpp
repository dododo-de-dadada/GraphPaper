//-------------------------------
// MainPage_disp.cpp
// �\���f�o�C�X�̃n���h���[
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �ĕ`�悪�K�v�ɂȂ���
	// sender	�C�x���g�����������f�C�X�v���C
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

	// �f�C�X�v���C�� DPI ���ς����
	// sender	�C�x���g�����������f�C�X�v���C
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

	// �f�C�X�v���C�̌������ς����
	// sender	�C�x���g�����������f�C�X�v���C
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