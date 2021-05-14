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
	// sender	�C�x���g�����������\���f�o�C�X
	void MainPage::disp_contents_invalidated(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender != DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		{
			m_dx_mutex.lock();
			sheet_dx().ValidateDevice();
			sample_dx().ValidateDevice();
			m_dx_mutex.unlock();
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			sheet_draw();
		}
	}

	// �\���f�o�C�X�� DPI ���ς����
	// sender	�C�x���g�����������\���f�o�C�X
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
			sheet_dx().SetDpi(dpi);
			sample_dx().SetDpi(dpi);
			m_dx_mutex.unlock();
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			sheet_draw();
		}
	}

	// �\���f�o�C�X�̌������ς����
	// sender	�C�x���g�����������\���f�o�C�X
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
			sheet_dx().SetCurrentOrientation(ori);
			sample_dx().SetCurrentOrientation(ori);
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