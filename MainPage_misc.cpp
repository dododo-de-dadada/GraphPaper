//-------------------------------
// MainPage_app.cpp
// アプリケーションの中断と再開
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// その他メニューの「色成分の表記」に印をつける.
	void MainPage::color_code_check_menu(void)
	{
		rmfi_color_dec().IsChecked(m_color_code == COLOR_CODE::DEC);
		rmfi_color_hex().IsChecked(m_color_code == COLOR_CODE::HEX);
		rmfi_color_real().IsChecked(m_color_code == COLOR_CODE::REAL);
		rmfi_color_cent().IsChecked(m_color_code == COLOR_CODE::CENT);
		rmfi_color_dec_2().IsChecked(m_color_code == COLOR_CODE::DEC);
		rmfi_color_hex_2().IsChecked(m_color_code == COLOR_CODE::HEX);
		rmfi_color_real_2().IsChecked(m_color_code == COLOR_CODE::REAL);
		rmfi_color_cent_2().IsChecked(m_color_code == COLOR_CODE::CENT);
	}

	// 色成分表記を選択する.
	template <COLOR_CODE C>
	void MainPage::color_code_click(void)
	{
		if (m_color_code == C) {
			return;
		}
		m_color_code = C;
	}

	// その他メニューの「長さの単位」に印をつける.
	void MainPage::len_unit_check_menu(void)
	{
		rmfi_len_grid().IsChecked(m_len_unit == LEN_UNIT::GRID);
		rmfi_len_inch().IsChecked(m_len_unit == LEN_UNIT::INCH);
		rmfi_len_milli().IsChecked(m_len_unit == LEN_UNIT::MILLI);
		rmfi_len_pixel().IsChecked(m_len_unit == LEN_UNIT::PIXEL);
		rmfi_len_point().IsChecked(m_len_unit == LEN_UNIT::POINT);
		rmfi_len_grid_2().IsChecked(m_len_unit == LEN_UNIT::GRID);
		rmfi_len_inch_2().IsChecked(m_len_unit == LEN_UNIT::INCH);
		rmfi_len_milli_2().IsChecked(m_len_unit == LEN_UNIT::MILLI);
		rmfi_len_pixel_2().IsChecked(m_len_unit == LEN_UNIT::PIXEL);
		rmfi_len_point_2().IsChecked(m_len_unit == LEN_UNIT::POINT);
	}

	// 長さの単位を選択する.
	template <LEN_UNIT L>
	void MainPage::len_unit_click(void)
	{
		if (m_len_unit == L) {
			return;
		}
		m_len_unit = L;
		status_bar_set_curs();
		status_bar_set_grid();
		status_bar_set_page();
		status_bar_set_unit();
	}

	// その他メニューの「色成分の表記」>「10進数」が選択された.
	void MainPage::rmfi_color_dec_click(IInspectable const&, RoutedEventArgs const&)
	{
		color_code_click<COLOR_CODE::DEC>();
	}

	// その他メニューの「色成分の表記」>「16進数」が選択された.
	void MainPage::rmfi_color_hex_click(IInspectable const&, RoutedEventArgs const&)
	{
		color_code_click<COLOR_CODE::HEX>();
	}

	// その他メニューの「色成分の表記」>「実数」が選択された.
	void MainPage::rmfi_color_real_click(IInspectable const&, RoutedEventArgs const&)
	{
		color_code_click<COLOR_CODE::REAL>();
	}

	// その他メニューの「色成分の表記」>「パーセント」が選択された.
	void MainPage::rmfi_color_cent_click(IInspectable const&, RoutedEventArgs const&)
	{
		color_code_click<COLOR_CODE::CENT>();
	}

	// その他メニューの「長さの単位」>「方眼」が選択された.
	void MainPage::rmfi_len_grid_click(IInspectable const&, RoutedEventArgs const&)
	{
		len_unit_click<LEN_UNIT::GRID>();
	}

	// その他メニューの「長さの単位」>「インチ」が選択された.
	void MainPage::rmfi_len_inch_click(IInspectable const&, RoutedEventArgs const&)
	{
		len_unit_click<LEN_UNIT::INCH>();
	}

	// その他メニューの「長さの単位」>「ポイント」が選択された.
	void MainPage::rmfi_len_milli_click(IInspectable const&, RoutedEventArgs const&)
	{
		len_unit_click<LEN_UNIT::MILLI>();
	}

	// その他メニューの「長さの単位」>「ピクセル」が選択された.
	void MainPage::rmfi_len_pixel_click(IInspectable const&, RoutedEventArgs const&)
	{
		len_unit_click<LEN_UNIT::PIXEL>();
	}

	// その他メニューの「長さの単位」>「ポイント」が選択された.
	void MainPage::rmfi_len_point_click(IInspectable const&, RoutedEventArgs const&)
	{
		len_unit_click<LEN_UNIT::POINT>();
	}

}