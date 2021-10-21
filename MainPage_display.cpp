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