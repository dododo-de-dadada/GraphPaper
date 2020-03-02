//-------------------------------
// MainPage_fill.cpp
// 塗りつぶしの設定
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 塗りつぶしメニューの「色」が選択された.
	void MainPage::mfi_fill_color_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		static winrt::event_token slider0_token;
		static winrt::event_token slider1_token;
		static winrt::event_token slider2_token;
		static winrt::event_token slider3_token;
		//static winrt::event_token c_style_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_sample();
		const double val0 = m_page_panel.m_fill_color.r * COLOR_MAX;
		const double val1 = m_page_panel.m_fill_color.g * COLOR_MAX;
		const double val2 = m_page_panel.m_fill_color.b * COLOR_MAX;
		const double val3 = m_page_panel.m_fill_color.a * COLOR_MAX;
		slider0().Value(val0);
		slider1().Value(val1);
		slider2().Value(val2);
		slider3().Value(val3);
		//cx_color_style().SelectedIndex(m_page_panel.m_col_style);
		fill_set_slider<UNDO_OP::FILL_COLOR, 0>(val0);
		fill_set_slider<UNDO_OP::FILL_COLOR, 1>(val1);
		fill_set_slider<UNDO_OP::FILL_COLOR, 2>(val2);
		fill_set_slider<UNDO_OP::FILL_COLOR, 3>(val3);
		slider0().Visibility(VISIBLE);
		slider1().Visibility(VISIBLE);
		slider2().Visibility(VISIBLE);
		slider3().Visibility(VISIBLE);
		//cx_color_style().Visibility(VISIBLE);
		loaded_token = scp_sample_panel().Loaded(
			[this](auto, auto)
			{
				sample_panel_loaded();
				fill_create_sample();
				sample_draw();
			}
		);
		slider0_token = slider0().ValueChanged(
			[this](auto, auto args)
			{
				fill_set_slider<UNDO_OP::FILL_COLOR, 0>(m_sample_shape, args.NewValue());
			}
		);
		slider1_token = slider1().ValueChanged(
			[this](auto, auto args)
			{
				fill_set_slider<UNDO_OP::FILL_COLOR, 1>(m_sample_shape, args.NewValue());
			}
		);
		slider2_token = slider2().ValueChanged(
			[this](auto, auto args)
			{
				fill_set_slider<UNDO_OP::FILL_COLOR, 2>(m_sample_shape, args.NewValue());
			}
		);
		slider3_token = slider3().ValueChanged(
			[this](auto, auto args)
			{
				fill_set_slider<UNDO_OP::FILL_COLOR, 3>(m_sample_shape, args.NewValue());
			}
		);
		//c_style_token = cx_color_style().SelectionChanged(
		//	[this](auto, auto args)
		//	{
		//		m_sample_panel.m_col_style = static_cast<COL_STYLE>(cx_color_style().SelectedIndex());
		//		fill_set_slider<UNDO_OP::FILL_COLOR, 0>(m_sample_shape, slider0().Value());
		//		fill_set_slider<UNDO_OP::FILL_COLOR, 1>(m_sample_shape, slider1().Value());
		//		fill_set_slider<UNDO_OP::FILL_COLOR, 2>(m_sample_shape, slider2().Value());
		//		fill_set_slider<UNDO_OP::FILL_COLOR, 3>(m_sample_shape, slider3().Value());
		//	}
		//);
		primary_token = cd_sample().PrimaryButtonClick(
			[this](auto, auto)
			{
				//m_page_panel.m_col_style = m_sample_panel.m_col_style;
				D2D1_COLOR_F sample_val;
				m_sample_shape->get_fill_color(sample_val);
				//D2D1_COLOR_F page_val;
				//m_page_shape->get_fill_color(page_val);
				//if (equal(sample_val, page_val)) {
				//	return;
				//}
				undo_push_value<UNDO_OP::FILL_COLOR>(sample_val);
			}
		);
		closed_token = cd_sample().Closed(
			[this](auto, auto)
			{
				delete m_sample_shape;
#if defined(_DEBUG)
				debug_leak_cnt--;
#endif
				m_sample_shape = nullptr;
				scp_sample_panel().Loaded(loaded_token);
				slider0().Visibility(COLLAPSED);
				slider1().Visibility(COLLAPSED);
				slider2().Visibility(COLLAPSED);
				slider3().Visibility(COLLAPSED);
				//cx_color_style().Visibility(COLLAPSED);
				slider0().ValueChanged(slider0_token);
				slider1().ValueChanged(slider1_token);
				slider2().ValueChanged(slider2_token);
				slider3().ValueChanged(slider3_token);
				//cx_color_style().SelectionChanged(c_style_token);
				cd_sample().PrimaryButtonClick(primary_token);
				cd_sample().Closed(closed_token);
				UnloadObject(cd_sample());
				page_draw();
			}
		);
		show_cd_sample(L"str_fill");
	}

	// 塗りつぶしの見本を作成する.
	void MainPage::fill_create_sample(void)
	{
		const auto dpi = m_sample_dx.m_logical_dpi;
		const auto w = scp_sample_panel().ActualWidth();
		const auto h = scp_sample_panel().ActualHeight();
		const auto padding = w * 0.125;
		const D2D1_POINT_2F pos = {
			static_cast<FLOAT>(padding),
			static_cast<FLOAT>(padding)
		};
		const D2D1_POINT_2F vec = {
			static_cast<FLOAT>(w - 2.0 * padding),
			static_cast<FLOAT>(h - 2.0 * padding)
		};
		m_sample_shape = new ShapeRect(pos, vec, &m_sample_panel);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S>
	void MainPage::fill_set_slider(double val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == UNDO_OP::FILL_COLOR) {
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
	void MainPage::fill_set_slider(Shape* s, const double val)
	{
		fill_set_slider<U, S>(val);
		if constexpr (U == UNDO_OP::FILL_COLOR) {
			D2D1_COLOR_F col;
			s->get_fill_color(col);
			if constexpr (S == 0) {
				col.r = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 1) {
				col.g = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 2) {
				col.b = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (U != UNDO_OP::PAGE_COLOR && S == 3) {
				col.a = static_cast<FLOAT>(val / COLOR_MAX);
			}
			s->set_fill_color(col);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

}