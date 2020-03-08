//------------------------------
// MainPage_stroke.cpp
// 線枠の設定
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t TITLE_STROKE[] = L"str_stroke";

	// 線枠メニューの「色」が選択された.
	IAsyncAction MainPage::mfi_stroke_color_click(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		const double val0 = m_page_panel.m_stroke_color.r * COLOR_MAX;
		const double val1 = m_page_panel.m_stroke_color.g * COLOR_MAX;
		const double val2 = m_page_panel.m_stroke_color.b * COLOR_MAX;
		const double val3 = m_page_panel.m_stroke_color.a * COLOR_MAX;
		sample_slider_0().Value(val0);
		sample_slider_1().Value(val1);
		sample_slider_2().Value(val2);
		sample_slider_3().Value(val3);
		stroke_set_slider_header<UNDO_OP::STROKE_COLOR, 0>(val0);
		stroke_set_slider_header<UNDO_OP::STROKE_COLOR, 1>(val1);
		stroke_set_slider_header<UNDO_OP::STROKE_COLOR, 2>(val2);
		stroke_set_slider_header<UNDO_OP::STROKE_COLOR, 3>(val3);
		sample_slider_0().Visibility(VISIBLE);
		sample_slider_1().Visibility(VISIBLE);
		sample_slider_2().Visibility(VISIBLE);
		sample_slider_3().Visibility(VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_COLOR, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_COLOR, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_COLOR, 2> });
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_COLOR, 3> });
		m_sample_type = SAMP_TYPE::STROKE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_STROKE)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F sample_val;
			m_sample_shape->get_stroke_color(sample_val);
			undo_push_value<UNDO_OP::STROKE_COLOR>(sample_val);
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_1().Visibility(COLLAPSED);
		sample_slider_2().Visibility(COLLAPSED);
		sample_slider_3().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sample_slider_2().ValueChanged(slider_2_token);
		sample_slider_3().ValueChanged(slider_3_token);
		page_draw();
	}

	// 線枠メニューの「破線の配置」が選択された.
	IAsyncAction MainPage::mfi_stroke_pattern_click(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		const double val0 = m_page_panel.m_stroke_pattern.m_[0];
		const double val1 = m_page_panel.m_stroke_pattern.m_[1];
		const double val2 = m_page_panel.m_stroke_pattern.m_[2];
		const double val3 = m_page_panel.m_stroke_pattern.m_[3];
		sample_slider_0().Value(val0);
		sample_slider_1().Value(val1);
		sample_slider_2().Value(val2);
		sample_slider_3().Value(val3);
		sample_slider_0().Visibility(m_page_panel.m_stroke_style != D2D1_DASH_STYLE_DOT ? VISIBLE : COLLAPSED);
		sample_slider_1().Visibility(m_page_panel.m_stroke_style != D2D1_DASH_STYLE_DOT ? VISIBLE : COLLAPSED);
		sample_slider_2().Visibility(m_page_panel.m_stroke_style != D2D1_DASH_STYLE_DASH ? VISIBLE : COLLAPSED);
		sample_slider_3().Visibility(m_page_panel.m_stroke_style != D2D1_DASH_STYLE_DASH ? VISIBLE : COLLAPSED);
		stroke_set_slider_header<UNDO_OP::STROKE_PATTERN, 0>(val0);
		stroke_set_slider_header<UNDO_OP::STROKE_PATTERN, 1>(val1);
		stroke_set_slider_header<UNDO_OP::STROKE_PATTERN, 2>(val2);
		stroke_set_slider_header<UNDO_OP::STROKE_PATTERN, 3>(val3);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_PATTERN, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_PATTERN, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_PATTERN, 2> });
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_PATTERN, 3> });
		m_sample_type = SAMP_TYPE::STROKE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_STROKE)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			STROKE_PATTERN sample_val;
			m_sample_shape->get_stroke_pattern(sample_val);
			undo_push_value<UNDO_OP::STROKE_PATTERN>(sample_val);
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_1().Visibility(COLLAPSED);
		sample_slider_2().Visibility(COLLAPSED);
		sample_slider_3().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sample_slider_2().ValueChanged(slider_2_token);
		sample_slider_3().ValueChanged(slider_3_token);
		page_draw();
	}

	// 線枠メニューの「太さ」が選択された.
	IAsyncAction MainPage::mfi_stroke_width_click(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		const double val0 = m_page_panel.m_stroke_width;
		sample_slider_0().Value(val0);
		sample_slider_0().Visibility(VISIBLE);
		stroke_set_slider_header<UNDO_OP::STROKE_WIDTH, 0>(val0);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_WIDTH, 0> });
		m_sample_type = SAMP_TYPE::STROKE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_STROKE)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			double sample_val;
			m_sample_shape->get_stroke_width(sample_val);
			undo_push_value<UNDO_OP::STROKE_WIDTH>(sample_val);
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		page_draw();
	}

	// 線枠メニューの「破線」が選択された.
	void MainPage::rmfi_stroke_dash_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_panel.m_stroke_style == D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_pattern().IsEnabled(true);
			mfi_stroke_pattern_2().IsEnabled(true);
		}
		undo_push_value<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_DASH);
	}

	// 線枠メニューの「一点破線」が選択された.
	void MainPage::rmfi_stroke_dash_dot_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_panel.m_stroke_style == D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_pattern().IsEnabled(true);
			mfi_stroke_pattern_2().IsEnabled(true);
		}
		undo_push_value<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_DASH_DOT);
	}

	// 線枠メニューの「二点破線」が選択された.
	void MainPage::rmfi_stroke_dash_dot_dot_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_panel.m_stroke_style == D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_pattern().IsEnabled(true);
			mfi_stroke_pattern_2().IsEnabled(true);
		}
		undo_push_value<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_DASH_DOT_DOT);
	}

	// 線枠メニューの「点線」が選択された.
	void MainPage::rmfi_stroke_dot_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_panel.m_stroke_style == D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_pattern().IsEnabled(true);
			mfi_stroke_pattern_2().IsEnabled(true);
		}
		undo_push_value<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_DOT);
	}

	// 線枠メニューの「実線」が選択された.
	void MainPage::rmfi_stroke_solid_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_panel.m_stroke_style != D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_pattern().IsEnabled(false);
			mfi_stroke_pattern_2().IsEnabled(false);
		}
		undo_push_value<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_SOLID);
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S>
	void MainPage::stroke_set_slider_header(double val)
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
			val *= m_sample_panel.m_stroke_width;
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH || U == UNDO_OP::STROKE_PATTERN) {
			wchar_t buf[32];
			const auto dpi = m_sample_dx.m_logical_dpi;
			const auto g_len = m_page_panel.m_grid_size + 1.0;
			conv_val_to_len(m_page_unit, val, dpi, g_len, buf, 31);
			hdr = hdr + L": " + buf;
		}
		if constexpr (U == UNDO_OP::STROKE_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				conv_val_to_col(m_col_style, val, buf, 16);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				conv_val_to_col(m_col_style, val, buf, 16);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				conv_val_to_col(m_col_style, val, buf, 16);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_b") + L": " + buf;
			}
			if constexpr (S == 3) {
				wchar_t buf[32];
				conv_val_to_col(m_col_style, val, buf, 16);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_opacity") + L": " + buf;
			}
		}
		if constexpr (S == 0) {
			sample_slider_0().Header(box_value(hdr));
		}
		if constexpr (S == 1) {
			sample_slider_1().Header(box_value(hdr));
		}
		if constexpr (S == 2) {
			sample_slider_2().Header(box_value(hdr));
		}
		if constexpr (S == 3) {
			sample_slider_3().Header(box_value(hdr));
		}
	}

	// 値をスライダーのヘッダーと図形に格納する.
	template <UNDO_OP U, int S>
	void MainPage::stroke_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* s = m_sample_shape;
		const double val = args.NewValue();
		stroke_set_slider_header<U, S>(val);
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
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
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