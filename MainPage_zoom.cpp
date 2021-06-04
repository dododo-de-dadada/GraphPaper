//-------------------------------
// MainPage_zoom.cpp
// �\���{��
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �\���{���̔z��
	//constexpr float ZOOM_LEVEL[] = {
	//	0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f
	//};

	void MainPage::mfi_zoom_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		float scale;
		if (sender == mfi_zoom_100()) {
			scale = 1.0f;
		}
		else if (sender == mfi_zoom_150()) {
			scale = 1.5f;
		}
		else if (sender == mfi_zoom_200()) {
			scale = 2.0f;
		}
		else if (sender == mfi_zoom_300()) {
			scale = 3.0f;
		}
		else if (sender == mfi_zoom_400()) {
			scale = 4.0f;
		}
		else if (sender == mfi_zoom_075()) {
			scale = 0.75f;
		}
		else if (sender == mfi_zoom_050()) {
			scale = 0.5f;
		}
		else if (sender == mfi_zoom_025()) {
			scale = 0.25f;
		}
		else {
			return;
		}
		if (scale != m_sheet_main.m_sheet_scale) {
			m_sheet_main.m_sheet_scale = scale;
			sheet_panle_size();
			sheet_draw();
			stbar_set_zoom();
		}
	}
	/*
	// �p�����j���[�́u�g��k���v>�u�g��v���I�����ꂽ.
	void MainPage::mfi_zoom_in_clicked(IInspectable const&, RoutedEventArgs const&)
	{
		const int s = 0;
		const int e = sizeof(ZOOM_LEVEL) / sizeof(ZOOM_LEVEL[0]) - 1;
		const int o = 1;

		for (int i = s; i < e; i++) {
			if (m_sheet_main.m_sheet_scale <= ZOOM_LEVEL[i]) {
				m_sheet_main.m_sheet_scale = ZOOM_LEVEL[i + o];
				sheet_panle_size();
				sheet_draw();
				stbar_set_zoom();
				break;
			}
		}
	}

	// �p�����j���[�́u�g��k���v>�u�k���v���I�����ꂽ.
	void MainPage::mfi_zoom_out_clicked(IInspectable const&, RoutedEventArgs const&)
	{
		const int s = 1;
		const int e = sizeof(ZOOM_LEVEL) / sizeof(ZOOM_LEVEL[0]);
		const int o = -1;

		for (int i = s; i < e; i++) {
			if (m_sheet_main.m_sheet_scale <= ZOOM_LEVEL[i]) {
				m_sheet_main.m_sheet_scale = ZOOM_LEVEL[i + o];
				sheet_panle_size();
				sheet_draw();
				stbar_set_zoom();
				break;
			}
		}
	}

	// �p�����j���[�́u�g��k���v>�u100%�ɖ߂��v���I�����ꂽ.
	void MainPage::mfi_zoom_100_click(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::UI::Xaml::Controls::ToolTipService;

		if (m_sheet_main.m_sheet_scale == 1.0) {
			return;
		}
		m_sheet_main.m_sheet_scale = 1.0;
		sheet_panle_size();
		sheet_draw();
		stbar_set_zoom();
	}
	*/

}