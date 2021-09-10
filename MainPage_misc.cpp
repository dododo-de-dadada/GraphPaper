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
		m_sample_type = SAMPLE_TYPE::MISC;
		co_await cd_sample_dialog().ShowAsync();
		cd_sample_dialog().PrimaryButtonText(pri_text);
		cd_sample_dialog().CloseButtonText(close_text);
		cd_sample_dialog().DefaultButton(def_btn);
		tb_version().Visibility(UI_COLLAPSED);
		//delete m_sample_shape;
		delete m_sample_sheet.m_list_shapes.back();
		m_sample_sheet.m_list_shapes.clear();
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		//m_sample_shape = nullptr;
		// バージョン情報のメッセージダイアログを表示する.
		//message_show(ICON_INFO, L"str_appname", L"str_version");
	}

	// その他メニューの「色の表記」のサブ項目が選択された.
	void MainPage::misc_color_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == rmfi_misc_color_cent() || sender == rmfi_misc_color_cent_2()) {
			m_misc_color_code = COLOR_CODE::CENT;
		}
		else if (sender == rmfi_misc_color_dec() || sender == rmfi_misc_color_dec_2()) {
			m_misc_color_code = COLOR_CODE::DEC;
		}
		else if (sender == rmfi_misc_color_hex() || sender == rmfi_misc_color_hex_2()) {
			m_misc_color_code = COLOR_CODE::HEX;
		}
		else if (sender == rmfi_misc_color_real() || sender == rmfi_misc_color_real_2()) {
			m_misc_color_code = COLOR_CODE::REAL;
		}
	}

	// その他メニューの「色の表記」に印をつける.
	void MainPage::misc_color_is_checked(const COLOR_CODE value)
	{
		rmfi_misc_color_dec().IsChecked(value == COLOR_CODE::DEC);
		rmfi_misc_color_dec_2().IsChecked(value == COLOR_CODE::DEC);
		rmfi_misc_color_hex().IsChecked(value == COLOR_CODE::HEX);
		rmfi_misc_color_hex_2().IsChecked(value == COLOR_CODE::HEX);
		rmfi_misc_color_real().IsChecked(value == COLOR_CODE::REAL);
		rmfi_misc_color_real_2().IsChecked(value == COLOR_CODE::REAL);
		rmfi_misc_color_cent().IsChecked(value == COLOR_CODE::CENT);
		rmfi_misc_color_cent_2().IsChecked(value == COLOR_CODE::CENT);
	}

	// その他メニューの「長さの単位」のサブ項目が選択された.
	void MainPage::misc_len_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		LEN_UNIT l_unit;
		if (sender == rmfi_misc_len_grid() || sender == rmfi_misc_len_grid_2()) {
			l_unit = LEN_UNIT::GRID;
		}
		else if (sender == rmfi_misc_len_inch() || sender == rmfi_misc_len_inch_2()) {
			l_unit = LEN_UNIT::INCH;
		}
		else if (sender == rmfi_misc_len_milli() || sender == rmfi_misc_len_milli_2()) {
			l_unit = LEN_UNIT::MILLI;
		}
		else if (sender == rmfi_misc_len_pixel() || sender == rmfi_misc_len_pixel_2()) {
			l_unit = LEN_UNIT::PIXEL;
		}
		else if (sender == rmfi_misc_len_point() || sender == rmfi_misc_len_point_2()) {
			l_unit = LEN_UNIT::POINT;
		}
		else {
			return;
		}
		if (m_misc_len_unit == l_unit) {
			return;
		}
		m_misc_len_unit = l_unit;
		status_set_curs();
		status_set_grid();
		status_set_sheet();
		status_set_unit();
	}

	// その他メニューの「長さの単位」に印をつける.
	void MainPage::misc_len_is_checked(const LEN_UNIT value)
	{
		rmfi_misc_len_grid().IsChecked(value == LEN_UNIT::GRID);
		rmfi_misc_len_grid_2().IsChecked(value == LEN_UNIT::GRID);
		rmfi_misc_len_inch().IsChecked(value == LEN_UNIT::INCH);
		rmfi_misc_len_inch_2().IsChecked(value == LEN_UNIT::INCH);
		rmfi_misc_len_milli().IsChecked(value == LEN_UNIT::MILLI);
		rmfi_misc_len_milli_2().IsChecked(value == LEN_UNIT::MILLI);
		rmfi_misc_len_pixel().IsChecked(value == LEN_UNIT::PIXEL);
		rmfi_misc_len_pixel_2().IsChecked(value == LEN_UNIT::PIXEL);
		rmfi_misc_len_point().IsChecked(value == LEN_UNIT::POINT);
		rmfi_misc_len_point_2().IsChecked(value == LEN_UNIT::POINT);
	}

	// その他メニューの「頂点をくっつける...」が選択された.
	IAsyncAction MainPage::misc_vert_stick_click_async(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		misc_vert_stick_set_header(m_misc_vert_stick);
		sd_misc_vert_stick().Value(static_cast<double>(m_misc_vert_stick));
		const auto d_result{ co_await cd_misc_vert_stick().ShowAsync() };
		if (d_result == ContentDialogResult::Primary) {
			m_misc_vert_stick = static_cast<float>(sd_misc_vert_stick().Value());
		}
	}

	void MainPage::misc_vert_stick_set_header(const float value) noexcept
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_SHOW>(m_misc_len_unit, value, m_sheet_dx.m_logical_dpi, m_sample_sheet.m_grid_base + 1.0f, buf);
		const auto text = ResourceLoader::GetForCurrentView().GetString(L"str_misc_vert_stick") + L": " + buf;
		sd_misc_vert_stick().Header(box_value(text));
	}

	// スライダーの値が変更された.
	void MainPage::misc_vert_stick_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args) noexcept
	{
		misc_vert_stick_set_header(static_cast<float>(args.NewValue()));
	}


}