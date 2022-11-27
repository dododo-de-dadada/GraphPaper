//-------------------------------
// MainPage_disp.cpp
// �\���f�o�C�X�̃n���h���[
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Graphics::Display::DisplayInformation;

	// �ĕ`�悪�K�v�ɂȂ���
	// sender	�C�x���g�����������\���f�o�C�X
	void MainPage::display_contents_invalidated(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender != DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		{
			m_mutex_d2d.lock();
			m_main_sheet.m_d2d.ValidateDevice();
			m_prop_sheet.m_d2d.ValidateDevice();
			m_mutex_d2d.unlock();
		}
		if (scp_prop_panel().IsLoaded()) {
			m_mutex_d2d.lock();
			m_prop_sheet.m_d2d.ValidateDevice();
			m_mutex_d2d.unlock();
			prop_sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			m_mutex_d2d.lock();
			m_main_sheet.m_d2d.ValidateDevice();
			m_mutex_d2d.unlock();
			sheet_draw();
		}
	}

	// �\���f�o�C�X�� DPI ���ς����
	// sender	�C�x���g�����������\���f�o�C�X
	void MainPage::display_dpi_changed(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender == DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		if (scp_prop_panel().IsLoaded()) {
			const float logical_dpi = sender.LogicalDpi();
			m_mutex_d2d.lock();
			m_prop_sheet.m_d2d.SetDpi(logical_dpi);
			m_mutex_d2d.unlock();
			prop_sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			const float logical_dpi = sender.LogicalDpi();
			m_mutex_d2d.lock();
			m_main_sheet.m_d2d.SetDpi(logical_dpi);
			m_mutex_d2d.unlock();
			sheet_draw();
		}
	}

	// �\���f�o�C�X�̌������ς����
	// sender	�C�x���g�����������\���f�o�C�X
	void MainPage::display_orientation_changed(DisplayInformation const& sender, IInspectable const&)
	{
#if defined(_DEBUG)
		if (sender != DisplayInformation::GetForCurrentView()) {
			return;
		}
#endif
		if (scp_prop_panel().IsLoaded()) {
			const auto ori = sender.CurrentOrientation();
			m_mutex_d2d.lock();
			m_prop_sheet.m_d2d.SetCurrentOrientation(ori);
			m_mutex_d2d.unlock();
			prop_sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			const auto ori = sender.CurrentOrientation();
			m_mutex_d2d.lock();
			m_main_sheet.m_d2d.SetCurrentOrientation(ori);
			m_mutex_d2d.unlock();
			sheet_draw();
		}
	}

}