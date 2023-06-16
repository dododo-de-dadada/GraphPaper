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
			m_mutex_draw.lock();
			m_main_d2d.ValidateDevice();
			m_dialog_d2d.ValidateDevice();
			m_mutex_draw.unlock();
		}
		if (scp_dialog_panel().IsLoaded()) {
			m_mutex_draw.lock();
			m_dialog_d2d.ValidateDevice();
			m_mutex_draw.unlock();
			dialog_draw();
		}
		if (scp_main_panel().IsLoaded()) {
			m_mutex_draw.lock();
			m_main_d2d.ValidateDevice();
			m_mutex_draw.unlock();
			main_sheet_draw();
		}
		status_bar_set_pos();
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
		if (scp_dialog_panel().IsLoaded()) {
			const float logical_dpi = sender.LogicalDpi();
			m_mutex_draw.lock();
			m_dialog_d2d.SetDpi(logical_dpi);
			m_mutex_draw.unlock();
			dialog_draw();
		}
		if (scp_main_panel().IsLoaded()) {
			const float logical_dpi = sender.LogicalDpi();
			m_mutex_draw.lock();
			m_main_d2d.SetDpi(logical_dpi);
			m_mutex_draw.unlock();
			main_sheet_draw();
		}
		m_event_click_dist = CLICK_DIST * DisplayInformation::GetForCurrentView().RawDpiX() / DisplayInformation::GetForCurrentView().LogicalDpi();
		status_bar_set_pos();
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
		if (scp_dialog_panel().IsLoaded()) {
			const auto ori = sender.CurrentOrientation();
			m_mutex_draw.lock();
			m_dialog_d2d.SetCurrentOrientation(ori);
			m_mutex_draw.unlock();
			dialog_draw();
		}
		if (scp_main_panel().IsLoaded()) {
			const auto ori = sender.CurrentOrientation();
			m_mutex_draw.lock();
			m_main_d2d.SetCurrentOrientation(ori);
			m_mutex_draw.unlock();
			main_sheet_draw();
		}
		status_bar_set_pos();
	}

}