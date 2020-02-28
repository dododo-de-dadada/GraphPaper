//------------------------------
// MainPage_stroke.cpp
// 線枠の設定
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 線枠メニューの「色」が選択された.
	void MainPage::mfi_stroke_color_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		static winrt::event_token slider0_token;
		static winrt::event_token slider1_token;
		static winrt::event_token slider2_token;
		static winrt::event_token slider3_token;
		//static winrt::event_token c_style_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_samp();
		double val0 = m_page_panel.m_stroke_color.r * COLOR_MAX;
		double val1 = m_page_panel.m_stroke_color.g * COLOR_MAX;
		double val2 = m_page_panel.m_stroke_color.b * COLOR_MAX;
		double val3 = m_page_panel.m_stroke_color.a * COLOR_MAX;
		slider0().Value(val0);
		slider1().Value(val1);
		slider2().Value(val2);
		slider3().Value(val3);
		//cx_color_style().SelectedIndex(m_page_panel.m_col_style);
		stroke_set_slider<UNDO_OP::STROKE_COLOR, 0>(val0);
		stroke_set_slider<UNDO_OP::STROKE_COLOR, 1>(val1);
		stroke_set_slider<UNDO_OP::STROKE_COLOR, 2>(val2);
		stroke_set_slider<UNDO_OP::STROKE_COLOR, 3>(val3);
		slider0().Visibility(VISIBLE);
		slider1().Visibility(VISIBLE);
		slider2().Visibility(VISIBLE);
		slider3().Visibility(VISIBLE);
		//cx_color_style().Visibility(VISIBLE);
		loaded_token = scp_samp_panel().Loaded(
			[this](auto, auto)
			{
				samp_panel_loaded();
				stroke_create_samp();
				draw_samp();
			}
		);
		slider0_token = slider0().ValueChanged(
			[this](auto, auto args)
			{
				stroke_set_slider<UNDO_OP::STROKE_COLOR, 0>(m_samp_shape, args.NewValue());
			}
		);
		slider1_token = slider1().ValueChanged(
			[this](auto, auto args)
			{
				stroke_set_slider<UNDO_OP::STROKE_COLOR, 1>(m_samp_shape, args.NewValue());
			}
		);
		slider2_token = slider2().ValueChanged(
			[this](auto, auto args)
			{
				stroke_set_slider<UNDO_OP::STROKE_COLOR, 2>(m_samp_shape, args.NewValue());
			}
		);
		slider3_token = slider3().ValueChanged(
			[this](auto, auto args)
			{
				stroke_set_slider<UNDO_OP::STROKE_COLOR, 3>(m_samp_shape, args.NewValue());
			}
		);
		//c_style_token = cx_color_style().SelectionChanged(
		//	[this](auto, auto args)
		//	{
		//		m_samp_panel.m_col_style = static_cast<COL_STYLE>(cx_color_style().SelectedIndex());
		//		stroke_set_slider<UNDO_OP::STROKE_COLOR, 0>(m_samp_shape, slider0().Value());
		//		stroke_set_slider<UNDO_OP::STROKE_COLOR, 1>(m_samp_shape, slider1().Value());
		//		stroke_set_slider<UNDO_OP::STROKE_COLOR, 2>(m_samp_shape, slider2().Value());
		//		stroke_set_slider<UNDO_OP::STROKE_COLOR, 3>(m_samp_shape, slider3().Value());
		//	}
		//);
		primary_token = cd_samp().PrimaryButtonClick(
			[this](auto, auto)
			{
				//m_page_panel.m_col_style = m_samp_panel.m_col_style;
				D2D1_COLOR_F samp_val;
				m_samp_shape->get_stroke_color(samp_val);
				undo_push_value<UNDO_OP::STROKE_COLOR>(samp_val);
			}
		);
		closed_token = cd_samp().Closed(
			[this](auto, auto)
			{
				delete m_samp_shape;
#if defined(_DEBUG)
				debug_leak_cnt--;
#endif
				m_samp_shape = nullptr;
				slider0().Visibility(COLLAPSED);
				slider1().Visibility(COLLAPSED);
				slider2().Visibility(COLLAPSED);
				slider3().Visibility(COLLAPSED);
				//cx_color_style().Visibility(COLLAPSED);
				scp_samp_panel().Loaded(loaded_token);
				slider0().ValueChanged(slider0_token);
				slider1().ValueChanged(slider1_token);
				slider2().ValueChanged(slider2_token);
				slider3().ValueChanged(slider3_token);
				//cx_color_style().SelectionChanged(c_style_token);
				cd_samp().PrimaryButtonClick(primary_token);
				cd_samp().Closed(closed_token);
				UnloadObject(cd_samp());
				draw_page();
			}
		);
		auto const& r_loader = ResourceLoader::GetForCurrentView();
		tk_samp_caption().Text(r_loader.GetString(L"str_stroke"));
		show_cd_samp();
	}

	// 線枠メニューの「破線の配置」が選択された.
	void MainPage::mfi_stroke_pattern_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		static winrt::event_token slider0_token;
		static winrt::event_token slider1_token;
		static winrt::event_token slider2_token;
		static winrt::event_token slider3_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_samp();
		double val0 = m_page_panel.m_stroke_pattern.m_[0];
		double val1 = m_page_panel.m_stroke_pattern.m_[1];
		double val2 = m_page_panel.m_stroke_pattern.m_[2];
		double val3 = m_page_panel.m_stroke_pattern.m_[3];
		slider0().Value(val0);
		slider1().Value(val1);
		slider2().Value(val2);
		slider3().Value(val3);
		slider0().Visibility(m_page_panel.m_stroke_style != D2D1_DASH_STYLE_DOT ? VISIBLE : COLLAPSED);
		slider1().Visibility(m_page_panel.m_stroke_style != D2D1_DASH_STYLE_DOT ? VISIBLE : COLLAPSED);
		slider2().Visibility(m_page_panel.m_stroke_style != D2D1_DASH_STYLE_DASH ? VISIBLE : COLLAPSED);
		slider3().Visibility(m_page_panel.m_stroke_style != D2D1_DASH_STYLE_DASH ? VISIBLE : COLLAPSED);
		stroke_set_slider<UNDO_OP::STROKE_PATTERN, 0>(val0);
		stroke_set_slider<UNDO_OP::STROKE_PATTERN, 1>(val1);
		stroke_set_slider<UNDO_OP::STROKE_PATTERN, 2>(val2);
		stroke_set_slider<UNDO_OP::STROKE_PATTERN, 3>(val3);
		loaded_token = scp_samp_panel().Loaded(
			[this](auto, auto)
			{
				samp_panel_loaded();
				stroke_create_samp();
				draw_samp();
			}
		);
		slider0_token = slider0().ValueChanged(
			[this](auto, auto args)
			{
				stroke_set_slider<UNDO_OP::STROKE_PATTERN, 0>(m_samp_shape, args.NewValue());
			}
		);
		slider1_token = slider1().ValueChanged(
			[this](auto, auto args)
			{
				stroke_set_slider<UNDO_OP::STROKE_PATTERN, 1>(m_samp_shape, args.NewValue());
			}
		);
		slider2_token = slider2().ValueChanged(
			[this](auto, auto args)
			{
				stroke_set_slider<UNDO_OP::STROKE_PATTERN, 2>(m_samp_shape, args.NewValue());
			}
		);
		slider3_token = slider3().ValueChanged(
			[this](auto, auto args)
			{
				stroke_set_slider<UNDO_OP::STROKE_PATTERN, 3>(m_samp_shape, args.NewValue());
			}
		);
		primary_token = cd_samp().PrimaryButtonClick(
			[this](auto, auto)
			{
				STROKE_PATTERN samp_val;
				m_samp_shape->get_stroke_pattern(samp_val);
				undo_push_value<UNDO_OP::STROKE_PATTERN>(samp_val);
			}
		);
		closed_token = cd_samp().Closed(
			[this](auto, auto)
			{
				delete m_samp_shape;
#if defined(_DEBUG)
				debug_leak_cnt--;
#endif
				m_samp_shape = nullptr;
				slider0().Visibility(COLLAPSED);
				slider1().Visibility(COLLAPSED);
				slider2().Visibility(COLLAPSED);
				slider3().Visibility(COLLAPSED);
				scp_samp_panel().Loaded(loaded_token);
				slider0().ValueChanged(slider0_token);
				slider1().ValueChanged(slider1_token);
				slider2().ValueChanged(slider2_token);
				slider3().ValueChanged(slider3_token);
				//cd_samp().Opened(opened_token);
				cd_samp().PrimaryButtonClick(primary_token);
				cd_samp().Closed(closed_token);
				UnloadObject(cd_samp());
				draw_page();
			}
		);
		auto const& r_loader = ResourceLoader::GetForCurrentView();
		tk_samp_caption().Text(r_loader.GetString(L"str_stroke"));
		show_cd_samp();
	}

	// 線枠メニューの「太さ」が選択された.
	void MainPage::mfi_stroke_width_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		static winrt::event_token slider0_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_samp();
		double val0 = m_page_panel.m_stroke_width;
		slider0().Value(val0);
		slider0().Visibility(VISIBLE);
		stroke_set_slider<UNDO_OP::STROKE_WIDTH, 0>(val0);
		loaded_token = scp_samp_panel().Loaded(
			[this](auto, auto)
			{
				samp_panel_loaded();
				stroke_create_samp();
				draw_samp();
			}
		);
		slider0_token = slider0().ValueChanged(
			[this](auto, auto args)
			{
				stroke_set_slider<UNDO_OP::STROKE_WIDTH, 0>(m_samp_shape, args.NewValue());
			}
		);
		primary_token = cd_samp().PrimaryButtonClick(
			[this](auto, auto)
			{
				double samp_val;
				m_samp_shape->get_stroke_width(samp_val);
				undo_push_value<UNDO_OP::STROKE_WIDTH>(samp_val);
			}
		);
		closed_token = cd_samp().Closed(
			[this](auto, auto)
			{
				delete m_samp_shape;
#if defined(_DEBUG)
				debug_leak_cnt--;
#endif
				m_samp_shape = nullptr;
				scp_samp_panel().Loaded(loaded_token);
				slider0().Visibility(COLLAPSED);
				slider0().ValueChanged(slider0_token);
				//cd_samp().Opened(opened_token);
				cd_samp().PrimaryButtonClick(primary_token);
				cd_samp().Closed(closed_token);
				UnloadObject(cd_samp());
				draw_page();
			}
		);
		auto const& r_loader = ResourceLoader::GetForCurrentView();
		tk_samp_caption().Text(r_loader.GetString(L"str_stroke"));
		show_cd_samp();
	}

	// 線枠メニューの「破線」が選択された.
	void MainPage::rmfi_stroke_dash_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		if (m_page_panel.m_stroke_style == D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_pattern().IsEnabled(true);
			mfi_stroke_pattern_2().IsEnabled(true);
		}
		undo_push_value<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_DASH);
	}

	// 線枠メニューの「一点破線」が選択された.
	void MainPage::rmfi_stroke_dash_dot_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		if (m_page_panel.m_stroke_style == D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_pattern().IsEnabled(true);
			mfi_stroke_pattern_2().IsEnabled(true);
		}
		undo_push_value<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_DASH_DOT);
	}

	// 線枠メニューの「二点破線」が選択された.
	void MainPage::rmfi_stroke_dash_dot_dot_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		if (m_page_panel.m_stroke_style == D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_pattern().IsEnabled(true);
			mfi_stroke_pattern_2().IsEnabled(true);
		}
		undo_push_value<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_DASH_DOT_DOT);
	}

	// 線枠メニューの「点線」が選択された.
	void MainPage::rmfi_stroke_dot_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		if (m_page_panel.m_stroke_style == D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_pattern().IsEnabled(true);
			mfi_stroke_pattern_2().IsEnabled(true);
		}
		undo_push_value<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_DOT);
	}

	// 線枠メニューの「実線」が選択された.
	void MainPage::rmfi_stroke_solid_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		if (m_page_panel.m_stroke_style != D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_pattern().IsEnabled(false);
			mfi_stroke_pattern_2().IsEnabled(false);
		}
		undo_push_value<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_SOLID);
	}

	// 線枠の見本を作成する.
	void MainPage::stroke_create_samp(void)
	{
		const auto dpi = m_samp_dx.m_logical_dpi;
		const D2D1_POINT_2F pos = { GRIDLEN_PX, GRIDLEN_PX };
		const auto w = scp_samp_panel().ActualWidth();
		const auto h = scp_samp_panel().ActualHeight();
		const D2D1_POINT_2F vec = {
			static_cast<FLOAT>(w - 2.0 * GRIDLEN_PX),
			static_cast<FLOAT>(h - 2.0 * GRIDLEN_PX)
		};
		m_samp_shape = new ShapeLine(pos, vec, &m_samp_panel);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S>
	void MainPage::stroke_set_slider(double val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			hdr = r_loader.GetString(L"str_stroke_width");
		}
		if constexpr (U == UNDO_OP::STROKE_PATTERN) {
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			if constexpr (S == 0) {
				hdr = r_loader.GetString(L"str_dash_len");
			}
			if constexpr (S == 1) {
				hdr = r_loader.GetString(L"str_dash_gap");
			}
			if constexpr (S == 2) {
				hdr = r_loader.GetString(L"str_dot_len");
			}
			if constexpr (S == 3) {
				hdr = r_loader.GetString(L"str_dot_gap");
			}
			val *= m_samp_panel.m_stroke_width;
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH || U == UNDO_OP::STROKE_PATTERN) {
			if (m_page_unit == DIST_UNIT::PIXEL) {
				wchar_t buf[16];
				swprintf_s(buf, FMT_PIXEL_UNIT, val);
				hdr = hdr + L": " + buf;
			}
			else if (m_page_unit == DIST_UNIT::GRID) {
				wchar_t buf[16];
				swprintf_s(buf, FMT_GRID_UNIT, val / (m_page_panel.m_grid_len + 1.0));
				hdr = hdr + L": " + buf;
			}
			else {
				wchar_t buf[16];
				const double inch = val / m_samp_dx.m_logical_dpi;
				switch (m_page_unit) {
				case DIST_UNIT::INCH:
					swprintf_s(buf, FMT_INCH_UNIT, inch);
					hdr = hdr + L": " + buf;
					break;
				case DIST_UNIT::MILLI:
					swprintf_s(buf, FMT_MILLI_UNIT, inch * MM_PER_INCH);
					hdr = hdr + L": " + buf;
					break;
				case DIST_UNIT::POINT:
					swprintf_s(buf, FMT_POINT_UNIT, inch * PT_PER_INCH);
					hdr = hdr + L": " + buf;
					break;
				}
			}
		}
		if constexpr (U == UNDO_OP::STROKE_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[16];
				conv_val_to_col(m_col_style, val, buf, 16);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[16];
				conv_val_to_col(m_col_style, val, buf, 16);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[16];
				conv_val_to_col(m_col_style, val, buf, 16);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_b") + L": " + buf;
			}
			if constexpr (S == 3) {
				wchar_t buf[16];
				conv_val_to_col(m_col_style, val, buf, 16);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_opacity") + L": " + buf;
			}
		}
		if constexpr (S == 0) {
			slider0().Header(box_value(hdr));
		}
		if constexpr (S == 1) {
			slider1().Header(box_value(hdr));
		}
		if constexpr (S == 2) {
			slider2().Header(box_value(hdr));
		}
		if constexpr (S == 3) {
			slider3().Header(box_value(hdr));
		}
	}

	// 値をスライダーのヘッダーと図形に格納する.
	template <UNDO_OP U, int S>
	void MainPage::stroke_set_slider(Shape* s, const double val)
	{
		stroke_set_slider<U, S>(val);
		if constexpr (U == UNDO_OP::STROKE_PATTERN) {
			STROKE_PATTERN pat;
			s->get_stroke_pattern(pat);
			if constexpr (S == 0) {
				pat.m_[0] = static_cast<FLOAT>(val);
			}
			if constexpr (S == 1) {
				pat.m_[1] = static_cast<FLOAT>(val);
			}
			if constexpr (S == 2) {
				pat.m_[2] = pat.m_[4] = static_cast<FLOAT>(val);
			}
			if constexpr (S == 3) {
				pat.m_[3] = pat.m_[5] = static_cast<FLOAT>(val);
			}
			s->set_stroke_pattern(pat);
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			s->set_stroke_width(val);
		}
		if constexpr (U == UNDO_OP::STROKE_COLOR) {
			D2D1_COLOR_F col;
			s->get_stroke_color(col);
			if constexpr (S == 0) {
				col.r = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 1) {
				col.g = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 2) {
				col.b = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 3) {
				col.a = static_cast<FLOAT>(val / COLOR_MAX);
			}
			s->set_stroke_color(col);
		}
		if (scp_samp_panel().IsLoaded()) {
			draw_samp();
		}
	}

	// 線枠メニューの「形式」に印をつける.
	// d_style	破線の種別
	void MainPage::stroke_style_check_menu(const D2D1_DASH_STYLE d_style)
	{
		rmfi_stroke_solid().IsChecked(d_style == D2D1_DASH_STYLE_SOLID);
		rmfi_stroke_dash().IsChecked(d_style == D2D1_DASH_STYLE_DASH);
		rmfi_stroke_dot().IsChecked(d_style == D2D1_DASH_STYLE_DOT);
		rmfi_stroke_dash_dot().IsChecked(d_style == D2D1_DASH_STYLE_DASH_DOT);
		rmfi_stroke_dash_dot_dot().IsChecked(d_style == D2D1_DASH_STYLE_DASH_DOT_DOT);
		mfi_stroke_pattern().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);

		rmfi_stroke_solid_2().IsChecked(d_style == D2D1_DASH_STYLE_SOLID);
		rmfi_stroke_dash_2().IsChecked(d_style == D2D1_DASH_STYLE_DASH);
		rmfi_stroke_dot_2().IsChecked(d_style == D2D1_DASH_STYLE_DOT);
		rmfi_stroke_dash_dot_2().IsChecked(d_style == D2D1_DASH_STYLE_DASH_DOT);
		rmfi_stroke_dash_dot_dot_2().IsChecked(d_style == D2D1_DASH_STYLE_DASH_DOT_DOT);
		mfi_stroke_pattern_2().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);
	}

}