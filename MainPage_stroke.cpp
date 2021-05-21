//------------------------------
// MainPage_stroke.cpp
// 線枠
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t TITLE_STROKE[] = L"str_stroke";
	constexpr double SLIDER_STEP = 0.5;

	// 線枠メニューの「色」が選択された.
	IAsyncAction MainPage::stroke_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		const double val0 = m_sample_sheet.m_stroke_color.r * COLOR_MAX;
		const double val1 = m_sample_sheet.m_stroke_color.g * COLOR_MAX;
		const double val2 = m_sample_sheet.m_stroke_color.b * COLOR_MAX;
		const double val3 = m_sample_sheet.m_stroke_color.a * COLOR_MAX;
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
			D2D1_COLOR_F sample_value;
			m_sample_shape->get_stroke_color(sample_value);
			undo_push_set<UNDO_OP::STROKE_COLOR>(sample_value);
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
		sheet_draw();
	}

	// 線枠メニューの「破線の配置」が選択された.
	IAsyncAction MainPage::stroke_patt_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		const double val0 = m_sheet_main.m_stroke_patt.m_[0] / SLIDER_STEP;
		const double val1 = m_sheet_main.m_stroke_patt.m_[1] / SLIDER_STEP;
		const double val2 = m_sheet_main.m_stroke_patt.m_[2] / SLIDER_STEP;
		const double val3 = m_sheet_main.m_stroke_patt.m_[3] / SLIDER_STEP;
		sample_slider_0().Value(val0);
		sample_slider_1().Value(val1);
		sample_slider_2().Value(val2);
		sample_slider_3().Value(val3);
		sample_slider_0().Visibility(m_sheet_main.m_stroke_style != D2D1_DASH_STYLE_DOT ? VISIBLE : COLLAPSED);
		sample_slider_1().Visibility(m_sheet_main.m_stroke_style != D2D1_DASH_STYLE_DOT ? VISIBLE : COLLAPSED);
		sample_slider_2().Visibility(m_sheet_main.m_stroke_style != D2D1_DASH_STYLE_DASH ? VISIBLE : COLLAPSED);
		sample_slider_3().Visibility(m_sheet_main.m_stroke_style != D2D1_DASH_STYLE_DASH ? VISIBLE : COLLAPSED);
		stroke_set_slider_header<UNDO_OP::STROKE_PATT, 0>(val0);
		stroke_set_slider_header<UNDO_OP::STROKE_PATT, 1>(val1);
		stroke_set_slider_header<UNDO_OP::STROKE_PATT, 2>(val2);
		stroke_set_slider_header<UNDO_OP::STROKE_PATT, 3>(val3);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_PATT, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_PATT, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_PATT, 2> });
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_PATT, 3> });
		m_sample_type = SAMP_TYPE::STROKE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_STROKE)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			STROKE_PATT sample_value;
			m_sample_shape->get_stroke_patt(sample_value);
			undo_push_set<UNDO_OP::STROKE_PATT>(sample_value);
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
		sheet_draw();
	}

	// 線枠メニューの「太さ」が選択された.
	IAsyncAction MainPage::stroke_width_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		const double val0 = m_sample_sheet.m_stroke_width / SLIDER_STEP;
		sample_slider_0().Value(val0);
		sample_slider_0().Visibility(VISIBLE);
		stroke_set_slider_header<UNDO_OP::STROKE_WIDTH, 0>(val0);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_WIDTH, 0> });
		m_sample_type = SAMP_TYPE::STROKE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_STROKE)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			double sample_value;
			m_sample_shape->get_stroke_width(sample_value);
			undo_push_set<UNDO_OP::STROKE_WIDTH>(sample_value);
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sheet_draw();
	}

	/*
	// 線枠メニューの「破線」が選択された.
	void MainPage::stroke_dash_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_sheet_main.m_stroke_style == D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_patt().IsEnabled(true);
			mfi_stroke_patt_2().IsEnabled(true);
		}
		undo_push_set<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_DASH);
	}

	// 線枠メニューの「一点破線」が選択された.
	void MainPage::stroke_dash_dot_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_sheet_main.m_stroke_style == D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_patt().IsEnabled(true);
			mfi_stroke_patt_2().IsEnabled(true);
		}
		undo_push_set<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_DASH_DOT);
	}

	// 線枠メニューの「二点破線」が選択された.
	void MainPage::stroke_dash_dot_dot_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_sheet_main.m_stroke_style == D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_patt().IsEnabled(true);
			mfi_stroke_patt_2().IsEnabled(true);
		}
		undo_push_set<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_DASH_DOT_DOT);
	}

	// 線枠メニューの「点線」が選択された.
	void MainPage::stroke_dot_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_sheet_main.m_stroke_style == D2D1_DASH_STYLE_SOLID) {
			mfi_stroke_patt().IsEnabled(true);
			mfi_stroke_patt_2().IsEnabled(true);
		}
		undo_push_set<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE_DOT);
	}
	*/

	// 線枠メニューの「実線」が選択された.
	void MainPage::stroke_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		D2D1_DASH_STYLE d_style;
		if (sender == rmfi_stroke_style_solid() || sender == rmfi_stroke_style_solid_2()) {
			d_style = D2D1_DASH_STYLE_SOLID;
		}
		else if (sender == rmfi_stroke_style_dash() || sender == rmfi_stroke_style_dash_2()) {
			d_style = D2D1_DASH_STYLE_DASH;
		}
		else if (sender == rmfi_stroke_style_dot() || sender == rmfi_stroke_style_dot_2()) {
			d_style = D2D1_DASH_STYLE_DOT;
		}
		else if (sender == rmfi_stroke_style_dash_dot() || sender == rmfi_stroke_style_dash_dot_2()) {
			d_style = D2D1_DASH_STYLE_DASH_DOT;
		}
		else if (sender == rmfi_stroke_style_dash_dot_dot() || sender == rmfi_stroke_style_dash_dot_dot_2()) {
			d_style = D2D1_DASH_STYLE_DASH_DOT_DOT;
		}
		else {
			return;
		}
		mfi_stroke_patt().IsEnabled(m_sheet_main.m_stroke_style == D2D1_DASH_STYLE_SOLID);
		mfi_stroke_patt_2().IsEnabled(m_sheet_main.m_stroke_style == D2D1_DASH_STYLE_SOLID);
		undo_push_set<UNDO_OP::STROKE_STYLE>(d_style);
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S> void MainPage::stroke_set_slider_header(const double value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			hdr = r_loader.GetString(L"str_stroke_width");
		}
		if constexpr (U == UNDO_OP::STROKE_PATT) {
			wchar_t buf[32];
			const double dpi = sheet_dx().m_logical_dpi;
			const double g_len = m_sheet_main.m_grid_base + 1.0;
			conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), value * SLIDER_STEP * m_sample_sheet.m_stroke_width, dpi, g_len, buf);
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			if constexpr (S == 0) {
				hdr = r_loader.GetString(L"str_dash_len") + L": " + buf;
			}
			if constexpr (S == 1) {
				hdr = r_loader.GetString(L"str_dash_gap") + L": " + buf;
			}
			if constexpr (S == 2) {
				hdr = r_loader.GetString(L"str_dot_len") + L": " + buf;
			}
			if constexpr (S == 3) {
				hdr = r_loader.GetString(L"str_dot_gap") + L": " + buf;
			}
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			wchar_t buf[32];
			const double dpi = sheet_dx().m_logical_dpi;
			const double g_len = m_sheet_main.m_grid_base + 1.0;
			conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), value * SLIDER_STEP, dpi, g_len, buf);
			hdr = hdr + L": " + buf;
		}
		if constexpr (U == UNDO_OP::STROKE_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_b") + L": " + buf;
			}
			if constexpr (S == 3) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
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

	// 値をスライダーのヘッダーと、見本の図形に格納する.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S>
	void MainPage::stroke_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* s = m_sample_shape;
		const double value = args.NewValue();
		stroke_set_slider_header<U, S>(value);
		if constexpr (U == UNDO_OP::STROKE_PATT) {
			STROKE_PATT patt;
			s->get_stroke_patt(patt);
			if constexpr (S == 0) {
				patt.m_[0] = static_cast<FLOAT>(value * SLIDER_STEP);
			}
			if constexpr (S == 1) {
				patt.m_[1] = static_cast<FLOAT>(value * SLIDER_STEP);
			}
			if constexpr (S == 2) {
				patt.m_[2] = patt.m_[4] = static_cast<FLOAT>(value * SLIDER_STEP);
			}
			if constexpr (S == 3) {
				patt.m_[3] = patt.m_[5] = static_cast<FLOAT>(value * SLIDER_STEP);
			}
			s->set_stroke_patt(patt);
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			s->set_stroke_width(value * SLIDER_STEP);
		}
		if constexpr (U == UNDO_OP::STROKE_COLOR) {
			D2D1_COLOR_F color;
			s->get_stroke_color(color);
			if constexpr (S == 0) {
				color.r = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 1) {
				color.g = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 2) {
				color.b = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 3) {
				color.a = static_cast<FLOAT>(value / COLOR_MAX);
			}
			s->set_stroke_color(color);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// 線枠メニューの「形式」に印をつける.
	// d_style	破線の種別
	void MainPage::stroke_style_check_menu(const D2D1_DASH_STYLE d_style)
	{
		rmfi_stroke_style_solid().IsChecked(d_style == D2D1_DASH_STYLE_SOLID);
		rmfi_stroke_style_dash().IsChecked(d_style == D2D1_DASH_STYLE_DASH);
		rmfi_stroke_style_dot().IsChecked(d_style == D2D1_DASH_STYLE_DOT);
		rmfi_stroke_style_dash_dot().IsChecked(d_style == D2D1_DASH_STYLE_DASH_DOT);
		rmfi_stroke_style_dash_dot_dot().IsChecked(d_style == D2D1_DASH_STYLE_DASH_DOT_DOT);
		mfi_stroke_patt().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);

		rmfi_stroke_style_solid_2().IsChecked(d_style == D2D1_DASH_STYLE_SOLID);
		rmfi_stroke_style_dash_2().IsChecked(d_style == D2D1_DASH_STYLE_DASH);
		rmfi_stroke_style_dot_2().IsChecked(d_style == D2D1_DASH_STYLE_DOT);
		rmfi_stroke_style_dash_dot_2().IsChecked(d_style == D2D1_DASH_STYLE_DASH_DOT);
		rmfi_stroke_style_dash_dot_dot_2().IsChecked(d_style == D2D1_DASH_STYLE_DASH_DOT_DOT);
		mfi_stroke_patt_2().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);
	}

}