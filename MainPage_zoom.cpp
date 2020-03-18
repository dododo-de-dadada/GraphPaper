//-------------------------------
// MainPage_zoom.cpp
// 表示倍率の設定
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 表示倍率の配列
	constexpr double ZOOM_LEVEL[] = {
		0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0, 6.0
	};

	// ページメニューの「拡大縮小」>「拡大」が選択された.
	void MainPage::mfi_zoom_in_clicked(IInspectable const&, RoutedEventArgs const&)
	{
		const int s = 0;
		const int e = sizeof(ZOOM_LEVEL) / sizeof(ZOOM_LEVEL[0]) - 1;
		const int o = 1;

		for (int i = s; i < e; i++) {
			if (m_page_layout.m_page_scale <= ZOOM_LEVEL[i]) {
				m_page_layout.m_page_scale = ZOOM_LEVEL[i + o];
				set_page_panle_size();
				page_draw();
				status_bar_set_zoom();
				break;
			}
		}
	}

	// ページメニューの「拡大縮小」>「縮小」が選択された.
	void MainPage::mfi_zoom_out_clicked(IInspectable const&, RoutedEventArgs const&)
	{
		//using winrt::Windows::UI::Xaml::Controls::ToolTipService;

		const int s = 1;
		const int e = sizeof(ZOOM_LEVEL) / sizeof(ZOOM_LEVEL[0]);
		const int o = -1;

		for (int i = s; i < e; i++) {
			if (m_page_layout.m_page_scale <= ZOOM_LEVEL[i]) {
				m_page_layout.m_page_scale = ZOOM_LEVEL[i + o];
				set_page_panle_size();
				page_draw();
				status_bar_set_zoom();
				break;
			}
		}
	}

	// ページメニューの「拡大縮小」>「100%に戻す」が選択された.
	void MainPage::mfi_zoom_reset_click(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::UI::Xaml::Controls::ToolTipService;

		if (m_page_layout.m_page_scale == 1.0) {
			return;
		}
		m_page_layout.m_page_scale = 1.0;
		set_page_panle_size();
		page_draw();
		status_bar_set_zoom();
	}

}