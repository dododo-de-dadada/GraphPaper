//-------------------------------
// MainPage_zoom.cpp
// 表示倍率
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 表示倍率の配列
	//constexpr float ZOOM_LEVEL[] = {
	//	0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f
	//};

	/*
	// 用紙メニューの「拡大縮小」>「拡大」が選択された.
	void MainPage::mfi_sheet_zoom_in_clicked(IInspectable const&, RoutedEventArgs const&)
	{
		const int s = 0;
		const int e = sizeof(ZOOM_LEVEL) / sizeof(ZOOM_LEVEL[0]) - 1;
		const int o = 1;

		for (int i = s; i < e; i++) {
			if (m_sheet_main.m_sheet_scale <= ZOOM_LEVEL[i]) {
				m_sheet_main.m_sheet_scale = ZOOM_LEVEL[i + o];
				sheet_panle_size();
				sheet_draw();
				status_bar_set_zoom();
				break;
			}
		}
	}

	// 用紙メニューの「拡大縮小」>「縮小」が選択された.
	void MainPage::mfi_sheet_zoom_out_clicked(IInspectable const&, RoutedEventArgs const&)
	{
		const int s = 1;
		const int e = sizeof(ZOOM_LEVEL) / sizeof(ZOOM_LEVEL[0]);
		const int o = -1;

		for (int i = s; i < e; i++) {
			if (m_sheet_main.m_sheet_scale <= ZOOM_LEVEL[i]) {
				m_sheet_main.m_sheet_scale = ZOOM_LEVEL[i + o];
				sheet_panle_size();
				sheet_draw();
				status_bar_set_zoom();
				break;
			}
		}
	}

	// 用紙メニューの「拡大縮小」>「100%に戻す」が選択された.
	void MainPage::mfi_sheet_zoom_100_click(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::UI::Xaml::Controls::ToolTipService;

		if (m_sheet_main.m_sheet_scale == 1.0) {
			return;
		}
		m_sheet_main.m_sheet_scale = 1.0;
		sheet_panle_size();
		sheet_draw();
		status_bar_set_zoom();
	}
	*/

}