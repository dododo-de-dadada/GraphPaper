//-----------------------------
// MainPage_misc.cpp
// 長さの単位, 色の表記, ステータスバー, バージョン情報
//-----------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	using winrt::Windows::UI::Xaml::RoutedEventArgs;

	// その他メニューの「バージョン情報」が選択された.
	IAsyncAction MainPage::about_graph_paper_click(IInspectable const&, RoutedEventArgs const&)
	{
		tb_version().Visibility(UI_VISIBLE);
		const auto def_btn = cd_sample_dialog().DefaultButton();
		const auto pri_text = cd_sample_dialog().PrimaryButtonText();
		const auto close_text = cd_sample_dialog().CloseButtonText();
		cd_sample_dialog().PrimaryButtonText(L"");
		cd_sample_dialog().CloseButtonText(L"OK");
		cd_sample_dialog().Title(box_value(L"GraphPaper"));

		//m_sample_type = SAMPLE_TYPE::MISC;
		//m_sample_type = SAMPLE_TYPE::MISC;
		//m_sample_d2d.SetSwapChainPanel(scp_sample_panel());
		const auto samp_w = scp_sample_panel().Width();
		const auto samp_h = scp_sample_panel().Height();
		//m_sample_sheet.m_sheet_size.width = static_cast<FLOAT>(samp_w);
		//m_sample_sheet.m_sheet_size.height = static_cast<FLOAT>(samp_h);
		constexpr uint32_t misc_min = 3;
		constexpr uint32_t misc_max = 12;
		static uint32_t misc_cnt = misc_min;
		const auto padd = samp_w * 0.125;
		const D2D1_POINT_2F samp_vec{ static_cast<FLOAT>(samp_w - 2.0 * padd), static_cast<FLOAT>(samp_h - 2.0 * padd) };
		POLY_OPTION p_opt{ m_drawing_poly_opt };
		p_opt.m_vertex_cnt = (misc_cnt >= misc_max ? misc_min : misc_cnt++);
		Shape* s = new ShapePoly(D2D1_POINT_2F{ 0.0f, 0.0f }, samp_vec, &m_sample_sheet, p_opt);
		D2D1_POINT_2F b_min;
		D2D1_POINT_2F b_max;
		D2D1_POINT_2F b_vec;
		s->get_bound(D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX }, b_min, b_max);
		pt_sub(b_max, b_min, b_vec);
		s->move(D2D1_POINT_2F{ static_cast<FLOAT>((samp_w - b_vec.x) * 0.5), static_cast<FLOAT>((samp_h - b_vec.y) * 0.5) });
		m_sample_sheet.m_shape_list.push_back(s);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		co_await cd_sample_dialog().ShowAsync();
		cd_sample_dialog().PrimaryButtonText(pri_text);
		cd_sample_dialog().CloseButtonText(close_text);
		cd_sample_dialog().DefaultButton(def_btn);
		tb_version().Visibility(UI_COLLAPSED);
		slist_clear(m_sample_sheet.m_shape_list);
		// バージョン情報のメッセージダイアログを表示する.
		//message_show(ICON_INFO, L"str_appname", L"str_version");
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

	// その他メニューの「色の表記」に印をつける.
	void MainPage::misc_color_is_checked(const COLOR_CODE value)
	{
		rmfi_color_code_dec().IsChecked(value == COLOR_CODE::DEC);
		rmfi_color_code_dec_2().IsChecked(value == COLOR_CODE::DEC);
		rmfi_color_code_hex().IsChecked(value == COLOR_CODE::HEX);
		rmfi_color_code_hex_2().IsChecked(value == COLOR_CODE::HEX);
		rmfi_color_code_real().IsChecked(value == COLOR_CODE::REAL);
		rmfi_color_code_real_2().IsChecked(value == COLOR_CODE::REAL);
		rmfi_color_code_cent().IsChecked(value == COLOR_CODE::CENT);
		rmfi_color_code_cent_2().IsChecked(value == COLOR_CODE::CENT);
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
		status_set_curs();
		status_set_grid();
		status_set_sheet();
		status_set_unit();
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
	}

	// その他メニューの「頂点をくっつける...」が選択された.
	IAsyncAction MainPage::stick_to_vertex_click_async(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		misc_vert_stick_set_header(m_vert_stick);
		sd_misc_vert_stick().Value(static_cast<double>(m_vert_stick));
		const auto d_result{ co_await cd_misc_vert_stick().ShowAsync() };
		if (d_result == ContentDialogResult::Primary) {
			m_vert_stick = static_cast<float>(sd_misc_vert_stick().Value());
		}
	}

	void MainPage::misc_vert_stick_set_header(const float value) noexcept
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, value, m_main_d2d.m_logical_dpi, m_sample_sheet.m_grid_base + 1.0f, buf);
		const auto text = ResourceLoader::GetForCurrentView().GetString(L"str_misc_vert_stick") + L": " + buf;
		sd_misc_vert_stick().Header(box_value(text));
	}

	// スライダーの値が変更された.
	void MainPage::misc_vert_stick_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args) noexcept
	{
		misc_vert_stick_set_header(static_cast<float>(args.NewValue()));
	}


}