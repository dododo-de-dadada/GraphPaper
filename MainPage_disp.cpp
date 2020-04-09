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
	void MainPage::disp_contents_invalidated(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender != DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		{
			mutex_wait();
			m_page_dx.ValidateDevice();
			m_sample_dx.ValidateDevice();
			mutex_unlock();
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
		if (scp_page_panel().IsLoaded()) {
			page_draw();
		}
	}

	// �f�C�X�v���C�� DPI ���ς����
	// sender	�C�x���g�����������f�C�X�v���C
	void MainPage::disp_dpi_changed(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender == DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		{
			mutex_wait();
			const auto dpi = sender.LogicalDpi();
			m_page_dx.SetDpi(dpi);
			m_sample_dx.SetDpi(dpi);
			mutex_unlock();
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
		if (scp_page_panel().IsLoaded()) {
			page_draw();
		}
	}

	// �f�C�X�v���C�̌������ς����
	// sender	�C�x���g�����������f�C�X�v���C
	void MainPage::disp_orientation_changed(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender != DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		{
			mutex_wait();
			const auto ori = sender.CurrentOrientation();
			m_page_dx.SetCurrentOrientation(ori);
			m_sample_dx.SetCurrentOrientation(ori);
			mutex_unlock();
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
		if (scp_page_panel().IsLoaded()) {
			page_draw();
		}
	}

}