//-----------------------------
// MainPage_misc.cpp
// 長さの単位, 色の表記, ステータスバー, バージョン情報
//-----------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// その他メニューの「バージョン情報」が選択された.
	IAsyncAction MainPage::about_graph_paper_click(IInspectable const&, RoutedEventArgs const&)
	{
		//using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		tb_version().Visibility(UI_VISIBLE);
		const auto def_btn = cd_sample_dialog().DefaultButton();
		const auto pri_text = cd_sample_dialog().PrimaryButtonText();
		const auto close_text = cd_sample_dialog().CloseButtonText();
		cd_sample_dialog().PrimaryButtonText(L"");
		cd_sample_dialog().CloseButtonText(L"OK");
		cd_sample_dialog().Title(box_value(L"GraphPaper"));
		m_sample_type = SAMP_TYPE::MISC;
		co_await cd_sample_dialog().ShowAsync();
		cd_sample_dialog().PrimaryButtonText(pri_text);
		cd_sample_dialog().CloseButtonText(close_text);
		cd_sample_dialog().DefaultButton(def_btn);
		tb_version().Visibility(UI_COLLAPSED);
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		// バージョン情報のメッセージダイアログを表示する.
		//message_show(ICON_INFO, L"str_appname", L"str_version");
	}

	// その他メニューの「色の表記」に印をつける.
	void MainPage::color_code_is_checked(const COLOR_CODE value)
	{
		rmfi_color_code_dec().IsChecked(value == COLOR_CODE::DEC);
		rmfi_color_code_dec_2().IsChecked(value == COLOR_CODE::DEC);
		rmfi_color_code_hex().IsChecked(value == COLOR_CODE::HEX);
		rmfi_color_code_hex_2().IsChecked(value == COLOR_CODE::HEX);
		rmfi_color_code_real().IsChecked(value == COLOR_CODE::REAL);
		rmfi_color_code_real_2().IsChecked(value == COLOR_CODE::REAL);
		rmfi_color_code_cent().IsChecked(value == COLOR_CODE::CENT);
		rmfi_color_code_cent_2().IsChecked(value == COLOR_CODE::CENT);
		//radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::DEC>(c_code, rmfi_color_code_dec());
		//radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::DEC>(c_code, rmfi_color_code_dec_2());
		//radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::HEX>(c_code, rmfi_color_code_hex());
		//radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::HEX>(c_code, rmfi_color_code_hex_2());
		//radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::REAL>(c_code, rmfi_color_code_real());
		//radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::REAL>(c_code, rmfi_color_code_real_2());
		//radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::CENT>(c_code, rmfi_color_code_cent());
		//radio_menu_item_set_value<COLOR_CODE, COLOR_CODE::CENT>(c_code, rmfi_color_code_cent_2());
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
	void MainPage::len_unit_is_checked(const LEN_UNIT value)
	{
		rmfi_len_unit_grid().IsChecked(value == LEN_UNIT::GRID);
		rmfi_len_unit_grid_2().IsChecked(value == LEN_UNIT::GRID);
		rmfi_len_unit_inch().IsChecked(value == LEN_UNIT::INCH);
		rmfi_len_unit_inch_2().IsChecked(value == LEN_UNIT::INCH);
		rmfi_len_unit_milli().IsChecked(value == LEN_UNIT::MILLI);
		rmfi_len_unit_milli_2().IsChecked(value == LEN_UNIT::MILLI);
		rmfi_len_unit_pixel().IsChecked(value == LEN_UNIT::PIXEL);
		rmfi_len_unit_pixel_2().IsChecked(value == LEN_UNIT::PIXEL);
		rmfi_len_unit_point().IsChecked(value == LEN_UNIT::POINT);
		rmfi_len_unit_point_2().IsChecked(value == LEN_UNIT::POINT);
		//radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::GRID>(l_unit, rmfi_len_unit_grid());
		//radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::GRID>(l_unit, rmfi_len_unit_grid_2());
		//radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::INCH>(l_unit, rmfi_len_unit_inch());
		//radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::INCH>(l_unit, rmfi_len_unit_inch_2());
		//radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::MILLI>(l_unit, rmfi_len_unit_milli());
		//radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::MILLI>(l_unit, rmfi_len_unit_milli_2());
		//radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::PIXEL>(l_unit, rmfi_len_unit_pixel());
		//radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::PIXEL>(l_unit, rmfi_len_unit_pixel_2());
		//radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::POINT>(l_unit, rmfi_len_unit_point());
		//radio_menu_item_set_value<LEN_UNIT, LEN_UNIT::POINT>(l_unit, rmfi_len_unit_point_2());
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
		status_bar_set_curs();
		status_bar_set_grid();
		status_bar_set_sheet();
		status_bar_set_unit();
	}

}