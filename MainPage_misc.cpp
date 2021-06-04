//-----------------------------
// MainPage_misc.cpp
// 長さの単位, 色の表記, ステータスバー, バージョン情報
//-----------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// その他メニューの「色の表記」に印をつける.
	void MainPage::color_code_check_menu(void)
	{
		if (m_color_code == COLOR_CODE::DEC) {
			rmfi_color_code_dec().IsChecked(true);
			rmfi_color_code_dec_2().IsChecked(true);
		}
		else if (m_color_code == COLOR_CODE::HEX) {
			rmfi_color_code_hex().IsChecked(true);
			rmfi_color_code_hex_2().IsChecked(true);
		}
		else if (m_color_code == COLOR_CODE::REAL) {
			rmfi_color_code_real().IsChecked(true);
			rmfi_color_code_real_2().IsChecked(true);
		}
		else if (m_color_code == COLOR_CODE::CENT) {
			rmfi_color_code_cent().IsChecked(true);
			rmfi_color_code_cent_2().IsChecked(true);
		}
	}

	// その他メニューの「色の表記」のサブ項目が選択された.
	void MainPage::color_code_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == rmfi_color_code_cent() || sender == rmfi_color_code_cent_2()) {
			m_color_code = COLOR_CODE::CENT;
		}
		else if (sender == rmfi_color_code_dec() || sender == rmfi_color_code_dec_2()) {
			m_color_code = COLOR_CODE::DEC;
		}
		else if (sender == rmfi_color_code_hex() || sender == rmfi_color_code_hex_2()) {
			m_color_code = COLOR_CODE::HEX;
		}
		else if (sender == rmfi_color_code_real() || sender == rmfi_color_code_real_2()) {
			m_color_code = COLOR_CODE::REAL;
		}
	}

	// その他メニューの「長さの単位」に印をつける.
	void MainPage::len_unit_check_menu(void)
	{
		if (m_len_unit == LEN_UNIT::GRID) {
			rmfi_len_unit_grid().IsChecked(true);
			rmfi_len_unit_grid_2().IsChecked(true);
		}
		else if (m_len_unit == LEN_UNIT::INCH) {
			rmfi_len_unit_inch().IsChecked(true);
			rmfi_len_unit_inch_2().IsChecked(true);
		}
		else if (m_len_unit == LEN_UNIT::MILLI) {
			rmfi_len_unit_milli().IsChecked(true);
			rmfi_len_unit_milli_2().IsChecked(true);
		}
		else if (m_len_unit == LEN_UNIT::PIXEL) {
			rmfi_len_unit_pixel().IsChecked(true);
			rmfi_len_unit_pixel_2().IsChecked(true);
		}
		else if (m_len_unit == LEN_UNIT::POINT) {
			rmfi_len_unit_point().IsChecked(true);
			rmfi_len_unit_point_2().IsChecked(true);
		}
	}

	// その他メニューの「長さの単位」のサブ項目が選択された.
	void MainPage::len_unit_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		LEN_UNIT l_unit;
		if (sender == rmfi_len_unit_grid() || sender == rmfi_len_unit_grid_2()) {
			l_unit = LEN_UNIT::GRID;
		}
		else if (sender == rmfi_len_unit_inch() || sender == rmfi_len_unit_inch_2()) {
			l_unit = LEN_UNIT::INCH;
		}
		else if (sender == rmfi_len_unit_milli() || sender == rmfi_len_unit_milli_2()) {
			l_unit = LEN_UNIT::MILLI;
		}
		else if (sender == rmfi_len_unit_pixel() || sender == rmfi_len_unit_pixel_2()) {
			l_unit = LEN_UNIT::PIXEL;
		}
		else if (sender == rmfi_len_unit_point() || sender == rmfi_len_unit_point_2()) {
			l_unit = LEN_UNIT::POINT;
		}
		else {
			return;
		}
		if (m_len_unit == l_unit) {
			return;
		}
		m_len_unit = l_unit;
		stbar_set_curs();
		stbar_set_grid();
		stbar_set_sheet();
		stbar_set_unit();
	}

}