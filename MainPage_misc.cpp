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
	void MainPage::color_code_is_checked(const COLOR_CODE c_code)
	{
		radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::DEC>(c_code, rmfi_color_code_dec());
		radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::DEC>(c_code, rmfi_color_code_dec_2());
		radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::HEX>(c_code, rmfi_color_code_hex());
		radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::HEX>(c_code, rmfi_color_code_hex_2());
		radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::REAL>(c_code, rmfi_color_code_real());
		radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::REAL>(c_code, rmfi_color_code_real_2());
		radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::CENT>(c_code, rmfi_color_code_cent());
		radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::CENT>(c_code, rmfi_color_code_cent_2());
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
	void MainPage::len_unit_is_checked(const LEN_UNIT l_unit)
	{
		radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::GRID>(l_unit, rmfi_len_unit_grid());
		radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::GRID>(l_unit, rmfi_len_unit_grid_2());
		radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::INCH>(l_unit, rmfi_len_unit_inch());
		radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::INCH>(l_unit, rmfi_len_unit_inch_2());
		radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::MILLI>(l_unit, rmfi_len_unit_milli());
		radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::MILLI>(l_unit, rmfi_len_unit_milli_2());
		radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::PIXEL>(l_unit, rmfi_len_unit_pixel());
		radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::PIXEL>(l_unit, rmfi_len_unit_pixel_2());
		radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::POINT>(l_unit, rmfi_len_unit_point());
		radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::POINT>(l_unit, rmfi_len_unit_point_2());
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