//-----------------------------
// MainPage_misc.cpp
// 長さの単位, 色の表記, ステータスバー, バージョン情報
//-----------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// その他メニューの「色の表記」>「パーセント」が選択された.
	void MainPage::color_code_cent_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_color_code = COLOR_CODE::CENT;
	}

	// その他メニューの「色の表記」に印をつける.
	void MainPage::color_code_check_menu(void)
	{
		rmfi_color_code_dec().IsChecked(m_color_code == COLOR_CODE::DEC);
		rmfi_color_code_hex().IsChecked(m_color_code == COLOR_CODE::HEX);
		rmfi_color_code_real().IsChecked(m_color_code == COLOR_CODE::REAL);
		rmfi_color_code_cent().IsChecked(m_color_code == COLOR_CODE::CENT);
		rmfi_color_code_dec_2().IsChecked(m_color_code == COLOR_CODE::DEC);
		rmfi_color_code_hex_2().IsChecked(m_color_code == COLOR_CODE::HEX);
		rmfi_color_code_real_2().IsChecked(m_color_code == COLOR_CODE::REAL);
		rmfi_color_code_cent_2().IsChecked(m_color_code == COLOR_CODE::CENT);
	}

	// その他メニューの「色の表記」>「10進数」が選択された.
	void MainPage::color_code_dec_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_color_code = COLOR_CODE::DEC;
	}

	// その他メニューの「色の表記」>「16進数」が選択された.
	void MainPage::color_code_hex_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_color_code = COLOR_CODE::HEX;
	}

	// その他メニューの「色の表記」>「実数」が選択された.
	void MainPage::color_code_real_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_color_code = COLOR_CODE::REAL;
	}

	// その他メニューの「長さの単位」に印をつける.
	void MainPage::len_unit_check_menu(void)
	{
		rmfi_len_unit_grid().IsChecked(m_len_unit == LEN_UNIT::GRID);
		rmfi_len_unit_inch().IsChecked(m_len_unit == LEN_UNIT::INCH);
		rmfi_len_unit_milli().IsChecked(m_len_unit == LEN_UNIT::MILLI);
		rmfi_len_unit_pixel().IsChecked(m_len_unit == LEN_UNIT::PIXEL);
		rmfi_len_unit_point().IsChecked(m_len_unit == LEN_UNIT::POINT);
		rmfi_len_unit_grid_2().IsChecked(m_len_unit == LEN_UNIT::GRID);
		rmfi_len_unit_inch_2().IsChecked(m_len_unit == LEN_UNIT::INCH);
		rmfi_len_unit_milli_2().IsChecked(m_len_unit == LEN_UNIT::MILLI);
		rmfi_len_unit_pixel_2().IsChecked(m_len_unit == LEN_UNIT::PIXEL);
		rmfi_len_unit_point_2().IsChecked(m_len_unit == LEN_UNIT::POINT);
	}

	// その他メニューの「長さの単位」>「方眼」が選択された.
	void MainPage::len_unit_grid_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_len_unit == LEN_UNIT::GRID) {
			return;
		}
		m_len_unit = LEN_UNIT::GRID;
		stbar_set_curs();
		stbar_set_grid();
		stbar_set_page();
		stbar_set_unit();
	}

	// その他メニューの「長さの単位」>「インチ」が選択された.
	void MainPage::len_unit_inch_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_len_unit == LEN_UNIT::INCH) {
			return;
		}
		m_len_unit = LEN_UNIT::INCH;
		stbar_set_curs();
		stbar_set_grid();
		stbar_set_page();
		stbar_set_unit();
	}

	// その他メニューの「長さの単位」>「ポイント」が選択された.
	void MainPage::len_unit_milli_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_len_unit == LEN_UNIT::MILLI) {
			return;
		}
		m_len_unit = LEN_UNIT::MILLI;
		stbar_set_curs();
		stbar_set_grid();
		stbar_set_page();
		stbar_set_unit();
	}

	// その他メニューの「長さの単位」>「ピクセル」が選択された.
	void MainPage::len_unit_pixel_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_len_unit == LEN_UNIT::PIXEL) {
			return;
		}
		m_len_unit = LEN_UNIT::PIXEL;
		stbar_set_curs();
		stbar_set_grid();
		stbar_set_page();
		stbar_set_unit();
	}

	// その他メニューの「長さの単位」>「ポイント」が選択された.
	void MainPage::len_unit_point_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_len_unit == LEN_UNIT::POINT) {
			return;
		}
		m_len_unit = LEN_UNIT::POINT;
		stbar_set_curs();
		stbar_set_grid();
		stbar_set_page();
		stbar_set_unit();
	}

}