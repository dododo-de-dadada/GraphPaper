//-----------------------------
// MainPage_misc.cpp
// 長さの単位, 色の基数, ステータスバー, バージョン情報
//-----------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

	// その他メニューの「バージョン情報」が選択された.
	IAsyncAction MainPage::about_graph_paper_click(IInspectable const&, RoutedEventArgs const&)
	{
		tb_version().Visibility(Visibility::Visible);
		const auto def_text = cd_dialog_prop().DefaultButton();
		const auto pri_text = cd_dialog_prop().PrimaryButtonText();
		const auto close_text = cd_dialog_prop().CloseButtonText();
		cd_dialog_prop().PrimaryButtonText(L"");
		cd_dialog_prop().CloseButtonText(L"OK");
		cd_dialog_prop().Title(box_value(L"GraphPaper"));

		const auto samp_w = scp_dialog_panel().Width();
		const auto samp_h = scp_dialog_panel().Height();

		constexpr uint32_t misc_min = 3;
		constexpr uint32_t misc_max = 12;
		static uint32_t misc_cnt = misc_min;
		const auto mar = samp_w * 0.125;
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(samp_w - 2.0 * mar), static_cast<FLOAT>(samp_h - 2.0 * mar)
		};
		POLY_OPTION p_opt{ m_drawing_poly_opt };
		p_opt.m_vertex_cnt = (misc_cnt >= misc_max ? misc_min : misc_cnt++);
		Shape* s = new ShapePoly(D2D1_POINT_2F{ 0.0f, 0.0f }, pos, &m_prop_page, p_opt);
		D2D1_POINT_2F b_lt;
		D2D1_POINT_2F b_rb;
		D2D1_POINT_2F b_pos;
		s->get_bound(
			D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX }, b_lt, b_rb);
		pt_sub(b_rb, b_lt, b_pos);
		s->move(
			D2D1_POINT_2F{ static_cast<FLOAT>((samp_w - b_pos.x) * 0.5),
			static_cast<FLOAT>((samp_h - b_pos.y) * 0.5) });
		m_prop_page.m_shape_list.push_back(s);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		m_mutex_event.lock();
		co_await cd_dialog_prop().ShowAsync();

		cd_dialog_prop().PrimaryButtonText(pri_text);
		cd_dialog_prop().CloseButtonText(close_text);
		cd_dialog_prop().DefaultButton(def_text);
		tb_version().Visibility(Visibility::Collapsed);
		slist_clear(m_prop_page.m_shape_list);
		status_bar_set_pos();
		m_mutex_event.unlock();
	}

	// その他メニューの「色の基数」のサブ項目が選択された.
	void MainPage::color_code_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == rmfi_menu_color_code_pct()) {
			m_color_code = COLOR_CODE::PCT;
		}
		else if (sender == rmfi_menu_color_code_dec()) {
			m_color_code = COLOR_CODE::DEC;
		}
		else if (sender == rmfi_menu_color_code_hex()) {
			m_color_code = COLOR_CODE::HEX;
		}
		else if (sender == rmfi_menu_color_code_real()) {
			m_color_code = COLOR_CODE::REAL;
		}
		else {
			throw winrt::hresult_not_implemented();
			return;
		}
		color_code_is_checked(m_color_code);
		status_bar_set_pos();
	}

	// その他メニューの「色の基数」に印をつける.
	void MainPage::color_code_is_checked(const COLOR_CODE val)
	{
		rmfi_menu_color_code_dec().IsChecked(val == COLOR_CODE::DEC);
		rmfi_menu_color_code_hex().IsChecked(val == COLOR_CODE::HEX);
		rmfi_menu_color_code_real().IsChecked(val == COLOR_CODE::REAL);
		rmfi_menu_color_code_pct().IsChecked(val == COLOR_CODE::PCT);
	}

	// その他メニューの「長さの単位」のサブ項目が選択された.
	void MainPage::len_unit_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		const auto old_unit = m_len_unit;
		LEN_UNIT new_val;
		if (sender == rmfi_menu_len_unit_grid()) {
			new_val = LEN_UNIT::GRID;
		}
		else if (sender == rmfi_menu_len_unit_inch()) {
			new_val = LEN_UNIT::INCH;
		}
		else if (sender == rmfi_menu_len_unit_milli()) {
			new_val = LEN_UNIT::MILLI;
		}
		else if (sender == rmfi_menu_len_unit_pixel()) {
			new_val = LEN_UNIT::PIXEL;
		}
		else if (sender == rmfi_menu_len_unit_point()) {
			new_val = LEN_UNIT::POINT;
		}
		else {
			throw winrt::hresult_not_implemented();
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
		rmfi_menu_len_unit_grid().IsChecked(val == LEN_UNIT::GRID);
		rmfi_menu_len_unit_inch().IsChecked(val == LEN_UNIT::INCH);
		rmfi_menu_len_unit_milli().IsChecked(val == LEN_UNIT::MILLI);
		rmfi_menu_len_unit_pixel().IsChecked(val == LEN_UNIT::PIXEL);
		rmfi_menu_len_unit_point().IsChecked(val == LEN_UNIT::POINT);
		cbi_len_unit_grid().IsSelected(val == LEN_UNIT::GRID);
		cbi_len_unit_inch().IsSelected(val == LEN_UNIT::INCH);
		cbi_len_unit_milli().IsSelected(val == LEN_UNIT::MILLI);
		cbi_len_unit_pixel().IsSelected(val == LEN_UNIT::PIXEL);
		cbi_len_unit_point().IsSelected(val == LEN_UNIT::POINT);
	}

	// その他メニューの「点を方眼にくっつける」が選択された.
	void MainPage::snap_grid_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_snap_grid = tmfi_menu_snap_grid().IsChecked();
		//m_main_page.m_snap_grid = tmfi_menu_snap_grid().IsChecked();
		status_bar_set_pos();
	}

	// その他メニューの「頂点をくっつける...」が選択された.
	IAsyncAction MainPage::snap_point_click_async(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		const winrt::hstring str_snap_point{ ResourceLoader::GetForCurrentView().GetString(L"str_snap_point") + L": " };
		const auto val = m_snap_point;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(m_len_unit, val, m_main_d2d.m_logical_dpi, m_prop_page.m_grid_base + 1.0, buf);
		sd_snap_point().Header(box_value(str_snap_point + buf));
		sd_snap_point().Value(static_cast<double>(m_snap_point));
		m_mutex_event.lock();
		{
			const auto revoler{
				sd_snap_point().ValueChanged(winrt::auto_revoke, [this, str_snap_point](auto, auto args) {
					const auto val = args.NewValue();
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(m_len_unit, val, m_main_d2d.m_logical_dpi, m_prop_page.m_grid_base + 1.0, buf);
					sd_snap_point().Header(box_value(str_snap_point + buf));
				})
			};
			if (co_await cd_snap_point().ShowAsync() == ContentDialogResult::Primary) {
				m_snap_point = static_cast<float>(sd_snap_point().Value());
			}
		}
		m_mutex_event.unlock();
	}

	// その他メニューの「ズーム」のサブ項目に印をつける.
	void MainPage::zoom_is_cheched(float scale)
	{
		rmfi_menu_layout_zoom_100().IsChecked(equal(scale, 1.0f));
		rmfi_popup_layout_zoom_100().IsChecked(equal(scale, 1.0f));
		rmfi_menu_layout_zoom_150().IsChecked(equal(scale, 1.5f));
		rmfi_popup_layout_zoom_150().IsChecked(equal(scale, 1.5f));
		rmfi_menu_layout_zoom_200().IsChecked(equal(scale, 2.0f));
		rmfi_popup_layout_zoom_200().IsChecked(equal(scale, 2.0f));
		rmfi_menu_layout_zoom_300().IsChecked(equal(scale, 3.0f));
		rmfi_popup_layout_zoom_300().IsChecked(equal(scale, 3.0f));
		rmfi_menu_layout_zoom_400().IsChecked(equal(scale, 4.0f));
		rmfi_popup_layout_zoom_400().IsChecked(equal(scale, 4.0f));
		rmfi_menu_layout_zoom_075().IsChecked(equal(scale, 0.75f));
		rmfi_popup_layout_zoom_075().IsChecked(equal(scale, 0.75f));
		rmfi_menu_layout_zoom_050().IsChecked(equal(scale, 0.5f));
		rmfi_popup_layout_zoom_050().IsChecked(equal(scale, 0.5f));
		rmfi_menu_layout_zoom_025().IsChecked(equal(scale, 0.25f));
		rmfi_popup_layout_zoom_025().IsChecked(equal(scale, 0.25f));
	}

	// その他メニューの「ズーム」が選択された.
	void MainPage::zoom_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		float scale;
		if (sender == rmfi_menu_layout_zoom_100() || sender == rmfi_popup_layout_zoom_100()) {
			scale = 1.0f;
		}
		else if (sender == rmfi_menu_layout_zoom_150() || sender == rmfi_popup_layout_zoom_150()) {
			scale = 1.5f;
		}
		else if (sender == rmfi_menu_layout_zoom_200() || sender == rmfi_popup_layout_zoom_200()) {
			scale = 2.0f;
		}
		else if (sender == rmfi_menu_layout_zoom_300() || sender == rmfi_popup_layout_zoom_300()) {
			scale = 3.0f;
		}
		else if (sender == rmfi_menu_layout_zoom_400() || sender == rmfi_popup_layout_zoom_400()) {
			scale = 4.0f;
		}
		else if (sender == rmfi_menu_layout_zoom_075() || sender == rmfi_popup_layout_zoom_075()) {
			scale = 0.75f;
		}
		else if (sender == rmfi_menu_layout_zoom_050() || sender == rmfi_popup_layout_zoom_050()) {
			scale = 0.5f;
		}
		else if (sender == rmfi_menu_layout_zoom_025() || sender == rmfi_popup_layout_zoom_025()) {
			scale = 0.25f;
		}
		else {
			return;
		}
		zoom_is_cheched(scale);
		//if (scale != m_main_page.m_page_scale) {
		//	m_main_page.m_page_scale = scale;
		if (scale != m_main_scale) {
			m_main_scale = scale;
			main_panel_size();
			main_draw();
			status_bar_set_zoom();
		}
		status_bar_set_pos();
	}

}