//-----------------------------
// MainPage_misc.cpp
// 長さの単位, 色の表記, ステータスバー, バージョン情報
//-----------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

	// その他メニューの「バージョン情報」が選択された.
	IAsyncAction MainPage::about_graph_paper_click(IInspectable const&, RoutedEventArgs const&)
	{
		tb_version().Visibility(Visibility::Visible);
		const auto def_text = cd_setting_dialog().DefaultButton();
		const auto pri_text = cd_setting_dialog().PrimaryButtonText();
		const auto close_text = cd_setting_dialog().CloseButtonText();
		cd_setting_dialog().PrimaryButtonText(L"");
		cd_setting_dialog().CloseButtonText(L"OK");
		cd_setting_dialog().Title(box_value(L"GraphPaper"));

		const auto samp_w = scp_dialog_panel().Width();
		const auto samp_h = scp_dialog_panel().Height();

		constexpr uint32_t misc_min = 3;
		constexpr uint32_t misc_max = 12;
		static uint32_t misc_cnt = misc_min;
		const auto pad = samp_w * 0.125;
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(samp_w - 2.0 * pad), static_cast<FLOAT>(samp_h - 2.0 * pad)
		};
		POLY_OPTION p_opt{ m_drawing_poly_opt };
		p_opt.m_vertex_cnt = (misc_cnt >= misc_max ? misc_min : misc_cnt++);
		Shape* s = new ShapePoly(D2D1_POINT_2F{ 0.0f, 0.0f }, pos, &m_dialog_page, p_opt);
		D2D1_POINT_2F b_lt;
		D2D1_POINT_2F b_rb;
		D2D1_POINT_2F b_pos;
		s->get_bound(
			D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX }, b_lt, b_rb);
		pt_sub(b_rb, b_lt, b_pos);
		s->move(
			D2D1_POINT_2F{ static_cast<FLOAT>((samp_w - b_pos.x) * 0.5),
			static_cast<FLOAT>((samp_h - b_pos.y) * 0.5) });
		m_dialog_page.m_shape_list.push_back(s);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		m_mutex_event.lock();
		co_await cd_setting_dialog().ShowAsync();

		cd_setting_dialog().PrimaryButtonText(pri_text);
		cd_setting_dialog().CloseButtonText(close_text);
		cd_setting_dialog().DefaultButton(def_text);
		tb_version().Visibility(Visibility::Collapsed);
		slist_clear(m_dialog_page.m_shape_list);
		status_bar_set_pos();
		m_mutex_event.unlock();
	}

	// その他メニューの「色の表記」のサブ項目が選択された.
	void MainPage::color_code_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == rmfi_color_code_pct()) {
			m_color_code = COLOR_CODE::PCT;
		}
		else if (sender == rmfi_color_code_dec()) {
			m_color_code = COLOR_CODE::DEC;
		}
		else if (sender == rmfi_color_code_hex()) {
			m_color_code = COLOR_CODE::HEX;
		}
		else if (sender == rmfi_color_code_real()) {
			m_color_code = COLOR_CODE::REAL;
		}
		else {
			winrt::hresult_not_implemented();
		}
		color_code_is_checked(m_color_code);
		status_bar_set_pos();
	}

	// その他メニューの「色の表記」に印をつける.
	void MainPage::color_code_is_checked(const COLOR_CODE val)
	{
		rmfi_color_code_dec().IsChecked(val == COLOR_CODE::DEC);
		rmfi_color_code_hex().IsChecked(val == COLOR_CODE::HEX);
		rmfi_color_code_real().IsChecked(val == COLOR_CODE::REAL);
		rmfi_color_code_pct().IsChecked(val == COLOR_CODE::PCT);
	}

	// その他メニューの「長さの単位」のサブ項目が選択された.
	void MainPage::len_unit_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		const auto old_unit = m_len_unit;
		LEN_UNIT new_val;
		if (sender == rmfi_len_unit_grid()) {
			new_val = LEN_UNIT::GRID;
		}
		else if (sender == rmfi_len_unit_inch()) {
			new_val = LEN_UNIT::INCH;
		}
		else if (sender == rmfi_len_unit_milli()) {
			new_val = LEN_UNIT::MILLI;
		}
		else if (sender == rmfi_len_unit_pixel()) {
			new_val = LEN_UNIT::PIXEL;
		}
		else if (sender == rmfi_len_unit_point()) {
			new_val = LEN_UNIT::POINT;
		}
		else {
			winrt::hresult_not_implemented();
			return;
		}
		m_len_unit = new_val;
		status_bar_set_pos();
		if (old_unit != new_val) {
			status_bar_set_grid();
			status_bar_set_page();
			status_bar_set_unit();
		}
	}

	// その他メニューの「長さの単位」に印をつける.
	void MainPage::len_unit_is_checked(const LEN_UNIT val)
	{
		rmfi_len_unit_grid().IsChecked(val == LEN_UNIT::GRID);
		rmfi_len_unit_inch().IsChecked(val == LEN_UNIT::INCH);
		rmfi_len_unit_milli().IsChecked(val == LEN_UNIT::MILLI);
		rmfi_len_unit_pixel().IsChecked(val == LEN_UNIT::PIXEL);
		rmfi_len_unit_point().IsChecked(val == LEN_UNIT::POINT);
		cbi_len_unit_grid().IsSelected(val == LEN_UNIT::GRID);
		cbi_len_unit_inch().IsSelected(val == LEN_UNIT::INCH);
		cbi_len_unit_milli().IsSelected(val == LEN_UNIT::MILLI);
		cbi_len_unit_pixel().IsSelected(val == LEN_UNIT::PIXEL);
		cbi_len_unit_point().IsSelected(val == LEN_UNIT::POINT);
	}

	void MainPage::len_unit_selection_changed(
		IInspectable const&, SelectionChangedEventArgs const& args) noexcept
	{
		LEN_UNIT old_unit = LEN_UNIT::PIXEL;
		for (const auto i : args.RemovedItems()) {
			if (i == cbi_len_unit_grid()) {
				old_unit = LEN_UNIT::GRID;
			}
			else if (i == cbi_len_unit_inch()) {
				old_unit = LEN_UNIT::INCH;
			}
			else if (i == cbi_len_unit_milli()) {
				old_unit = LEN_UNIT::MILLI;
			}
			else if (i == cbi_len_unit_pixel()) {
				old_unit = LEN_UNIT::PIXEL;
			}
			else if (i == cbi_len_unit_point()) {
				old_unit = LEN_UNIT::POINT;
			}
			else {
				return;
			}
		}
		LEN_UNIT new_unit = LEN_UNIT::PIXEL;
		for (const auto i : args.AddedItems()) {
			if (i == cbi_len_unit_grid()) {
				new_unit = LEN_UNIT::GRID;
			}
			else if (i == cbi_len_unit_inch()) {
				new_unit = LEN_UNIT::INCH;
			}
			else if (i == cbi_len_unit_milli()) {
				new_unit = LEN_UNIT::MILLI;
			}
			else if (i == cbi_len_unit_pixel()) {
				new_unit = LEN_UNIT::PIXEL;
			}
			else if (i == cbi_len_unit_point()) {
				new_unit = LEN_UNIT::POINT;
			}
			else {
				return;
			}
		}
		if (old_unit != new_unit) {
			const double dpi = m_main_d2d.m_logical_dpi;
			const double g_len = m_main_page.m_grid_base + 1.0;
			double val;
			if (swscanf_s(tx_page_size_width().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_size_width().Text(buf);
			}
			if (swscanf_s(tx_page_size_height().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_size_height().Text(buf);
			}
			if (swscanf_s(tx_page_pad_left().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_pad_left().Text(buf);
			}
			if (swscanf_s(tx_page_pad_top().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_pad_top().Text(buf);
			}
			if (swscanf_s(tx_page_pad_right().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_pad_right().Text(buf);
			}
			if (swscanf_s(tx_page_pad_bottom().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_pad_bottom().Text(buf);
			}
		}
	}

	// その他メニューの「頂点をくっつける...」が選択された.
	IAsyncAction MainPage::stick_to_vertex_click_async(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		vert_stick_set_header(m_vert_stick);
		sd_vert_stick().Value(static_cast<double>(m_vert_stick));
		m_mutex_event.lock();
		const auto d_result = co_await cd_vert_stick().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			m_vert_stick = static_cast<float>(sd_vert_stick().Value());
		}
		m_mutex_event.unlock();
	}

	void MainPage::vert_stick_set_header(const float val) noexcept
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(
			m_len_unit, val, m_main_d2d.m_logical_dpi, m_dialog_page.m_grid_base + 1.0f, buf);
		const auto text = ResourceLoader::GetForCurrentView().GetString(L"str_vert_stick") + L": " + buf;
		sd_vert_stick().Header(box_value(text));
	}

	// スライダーの値が変更された.
	void MainPage::vert_stick_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args) noexcept
	{
		vert_stick_set_header(static_cast<float>(args.NewValue()));
	}

}