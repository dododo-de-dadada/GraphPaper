//-------------------------------
// MainPage_disp.cpp
// �\���f�o�C�X�̃n���h���[
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Graphics::Display::DisplayInformation;

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
			m_d2d_mutex.lock();
			m_main_d2d.ValidateDevice(scp_sheet_panel());
			m_sample_dx.ValidateDevice(scp_sheet_panel());
			m_d2d_mutex.unlock();
		}
		if (scp_sample_panel().IsLoaded()) {
			m_d2d_mutex.lock();
			m_sample_dx.ValidateDevice(scp_sheet_panel());
			m_d2d_mutex.unlock();
			sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			m_d2d_mutex.lock();
			m_main_d2d.ValidateDevice(scp_sheet_panel());
			m_d2d_mutex.unlock();
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
		if (scp_sample_panel().IsLoaded()) {
			m_d2d_mutex.lock();
			m_sample_dx.SetDpi(scp_sample_panel(), sender.LogicalDpi());
			m_d2d_mutex.unlock();
			sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			m_d2d_mutex.lock();
			m_main_d2d.SetDpi(scp_sheet_panel(), sender.LogicalDpi());
			m_d2d_mutex.unlock();
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
		if (scp_sample_panel().IsLoaded()) {
			m_d2d_mutex.lock();
			const auto ori = sender.CurrentOrientation();
			m_sample_dx.SetCurrentOrientation(scp_sample_panel(), ori);
			m_d2d_mutex.unlock();
			sample_draw();
		}
		if (scp_sheet_panel().IsLoaded()) {
			m_d2d_mutex.lock();
			const auto ori = sender.CurrentOrientation();
			m_main_d2d.SetCurrentOrientation(scp_sheet_panel(), ori);
			m_d2d_mutex.unlock();
			sheet_draw();
		}
	}

}