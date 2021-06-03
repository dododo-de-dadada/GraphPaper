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
	constexpr float SLIDER_STEP = 0.5f;

	// 線枠メニューの「色」が選択された.
	IAsyncAction MainPage::stroke_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		D2D1_COLOR_F s_color;
		m_sample_sheet.get_stroke_color(s_color);
		const float val0 = s_color.r * COLOR_MAX;
		const float val1 = s_color.g * COLOR_MAX;
		const float val2 = s_color.b * COLOR_MAX;
		const float val3 = s_color.a * COLOR_MAX;
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
			if (undo_push_set<UNDO_OP::STROKE_COLOR>(sample_value)) {
				undo_push_null();
				undo_menu_enable();
				edit_menu_enable();
				sheet_draw();
			}
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
	IAsyncAction MainPage::stroke_dash_patt_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		STROKE_DASH_PATT s_patt;
		m_sheet_main.get_stroke_dash_patt(s_patt);
		const float val0 = s_patt.m_[0] / SLIDER_STEP;
		const float val1 = s_patt.m_[1] / SLIDER_STEP;
		const float val2 = s_patt.m_[2] / SLIDER_STEP;
		const float val3 = s_patt.m_[3] / SLIDER_STEP;
		float s_width;
		m_sheet_main.get_stroke_width(s_width);
		const float val4 = s_width / SLIDER_STEP;
		sample_slider_0().Value(val0);
		sample_slider_1().Value(val1);
		sample_slider_2().Value(val2);
		sample_slider_3().Value(val3);
		sample_slider_4().Value(val4);
		D2D1_DASH_STYLE s_style;
		m_sheet_main.get_stroke_dash_style(s_style);
		sample_slider_0().Visibility(s_style != D2D1_DASH_STYLE_DOT ? VISIBLE : COLLAPSED);
		sample_slider_1().Visibility(s_style != D2D1_DASH_STYLE_DOT ? VISIBLE : COLLAPSED);
		sample_slider_2().Visibility(s_style != D2D1_DASH_STYLE_DASH ? VISIBLE : COLLAPSED);
		sample_slider_3().Visibility(s_style != D2D1_DASH_STYLE_DASH ? VISIBLE : COLLAPSED);
		sample_slider_4().Visibility(VISIBLE);
		stroke_set_slider_header<UNDO_OP::STROKE_DASH_PATT, 0>(val0);
		stroke_set_slider_header<UNDO_OP::STROKE_DASH_PATT, 1>(val1);
		stroke_set_slider_header<UNDO_OP::STROKE_DASH_PATT, 2>(val2);
		stroke_set_slider_header<UNDO_OP::STROKE_DASH_PATT, 3>(val3);
		stroke_set_slider_header<UNDO_OP::STROKE_WIDTH, 4>(val4);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_DASH_PATT, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_DASH_PATT, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_DASH_PATT, 2> });
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_DASH_PATT, 3> });
		const auto slider_4_token = sample_slider_4().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_WIDTH, 4> });
		m_sample_type = SAMP_TYPE::STROKE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_STROKE)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			STROKE_DASH_PATT sample_patt;
			float sample_width;
			m_sample_shape->get_stroke_dash_patt(sample_patt);
			m_sample_shape->get_stroke_width(sample_width);
			if (undo_push_set<UNDO_OP::STROKE_DASH_PATT>(sample_patt) ||
				undo_push_set<UNDO_OP::STROKE_WIDTH>(sample_width)) {
				undo_push_null();
				edit_menu_enable();
				sheet_draw();
			}
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
		sample_slider_4().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sample_slider_2().ValueChanged(slider_2_token);
		sample_slider_3().ValueChanged(slider_3_token);
		sample_slider_4().ValueChanged(slider_4_token);
		sheet_draw();
	}

	// 線枠メニューの「太さ」が選択された.
	IAsyncAction MainPage::stroke_width_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		float s_width;
		m_sample_sheet.get_stroke_width(s_width);
		const float val0 = s_width / SLIDER_STEP;
		sample_slider_0().Value(val0);
		sample_slider_0().Visibility(VISIBLE);
		stroke_set_slider_header<UNDO_OP::STROKE_WIDTH, 0>(val0);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_WIDTH, 0> });
		m_sample_type = SAMP_TYPE::STROKE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_STROKE)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_value;
			m_sample_shape->get_stroke_width(sample_value);
			if (undo_push_set<UNDO_OP::STROKE_WIDTH>(sample_value)) {
				undo_push_null();
				edit_menu_enable();
				sheet_draw();
			}
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

	// 線枠メニューの「種類」のサブ項目が選択された.
	void MainPage::stroke_dash_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		D2D1_DASH_STYLE d_style;
		if (sender == rmfi_stroke_dash_style_solid() || sender == rmfi_stroke_dash_style_solid_2()) {
			d_style = D2D1_DASH_STYLE_SOLID;
		}
		else if (sender == rmfi_stroke_dash_style_dash() || sender == rmfi_stroke_dash_style_dash_2()) {
			d_style = D2D1_DASH_STYLE_DASH;
		}
		else if (sender == rmfi_stroke_dash_style_dot() || sender == rmfi_stroke_dash_style_dot_2()) {
			d_style = D2D1_DASH_STYLE_DOT;
		}
		else if (sender == rmfi_stroke_dash_style_dash_dot() || sender == rmfi_stroke_dash_style_dash_dot_2()) {
			d_style = D2D1_DASH_STYLE_DASH_DOT;
		}
		else if (sender == rmfi_stroke_dash_style_dash_dot_dot() || sender == rmfi_stroke_dash_style_dash_dot_dot_2()) {
			d_style = D2D1_DASH_STYLE_DASH_DOT_DOT;
		}
		else {
			return;
		}
		D2D1_DASH_STYLE s_style;
		m_sheet_main.get_stroke_dash_style(s_style);
		mfi_stroke_dash_patt().IsEnabled(s_style == D2D1_DASH_STYLE_SOLID);
		mfi_stroke_dash_patt_2().IsEnabled(s_style == D2D1_DASH_STYLE_SOLID);
		if (undo_push_set<UNDO_OP::STROKE_DASH_STYLE>(d_style)) {
			undo_push_null();
			edit_menu_enable();
			sheet_draw();
		}
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S> void MainPage::stroke_set_slider_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring text;

		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			text = r_loader.GetString(L"str_stroke_width");
		}
		if constexpr (U == UNDO_OP::STROKE_DASH_PATT) {
			wchar_t buf[32];
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			//float s_width;
			//m_sample_sheet.get_stroke_width(s_width);
			conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), value * SLIDER_STEP, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			if constexpr (S == 0) {
				text = r_loader.GetString(L"str_dash_len") + L": " + buf + L"\u00D7";
			}
			if constexpr (S == 1) {
				text = r_loader.GetString(L"str_dash_gap") + L": " + buf + L"\u00D7";
			}
			if constexpr (S == 2) {
				text = r_loader.GetString(L"str_dot_len") + L": " + buf + L"\u00D7";
			}
			if constexpr (S == 3) {
				text = r_loader.GetString(L"str_dot_gap") + L": " + buf + L"\u00D7";
			}
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			wchar_t buf[32];
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), value * SLIDER_STEP, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			text = r_loader.GetString(L"str_stroke_width");
			text = text + L": " + buf;
		}
		if constexpr (U == UNDO_OP::STROKE_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				text = r_loader.GetString(L"str_col_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				text = r_loader.GetString(L"str_col_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				text = r_loader.GetString(L"str_col_b") + L": " + buf;
			}
			if constexpr (S == 3) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				text = r_loader.GetString(L"str_opacity") + L": " + buf;
			}
		}
		if constexpr (S == 0) {
			sample_slider_0().Header(box_value(text));
		}
		if constexpr (S == 1) {
			sample_slider_1().Header(box_value(text));
		}
		if constexpr (S == 2) {
			sample_slider_2().Header(box_value(text));
		}
		if constexpr (S == 3) {
			sample_slider_3().Header(box_value(text));
		}
		if constexpr (S == 4) {
			sample_slider_4().Header(box_value(text));
		}
	}

	// 値をスライダーのヘッダーと、見本の図形に格納する.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S> void MainPage::stroke_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* s = m_sample_shape;
		const float value = static_cast<float>(args.NewValue());
		stroke_set_slider_header<U, S>(value);
		if constexpr (U == UNDO_OP::STROKE_DASH_PATT) {
			if constexpr (S == 0) {
				STROKE_DASH_PATT patt;
				s->get_stroke_dash_patt(patt);
				patt.m_[0] = static_cast<FLOAT>(value * SLIDER_STEP);
				s->set_stroke_dash_patt(patt);
			}
			if constexpr (S == 1) {
				STROKE_DASH_PATT patt;
				s->get_stroke_dash_patt(patt);
				patt.m_[1] = static_cast<FLOAT>(value * SLIDER_STEP);
				s->set_stroke_dash_patt(patt);
			}
			if constexpr (S == 2) {
				STROKE_DASH_PATT patt;
				s->get_stroke_dash_patt(patt);
				patt.m_[2] = patt.m_[4] = static_cast<FLOAT>(value * SLIDER_STEP);
				s->set_stroke_dash_patt(patt);
			}
			if constexpr (S == 3) {
				STROKE_DASH_PATT patt;
				s->get_stroke_dash_patt(patt);
				patt.m_[3] = patt.m_[5] = static_cast<FLOAT>(value * SLIDER_STEP);
				s->set_stroke_dash_patt(patt);
			}
			//if constexpr (S == 4) {
			//	float s_width;
			//	s_width = static_cast<FLOAT>(value * SLIDER_STEP);
			//	s->set_stroke_width(s_width);
			//}
		}
		if constexpr (U == UNDO_OP::STROKE_JOIN_LIMIT) {
			s->set_stroke_join_limit(value);
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
	void MainPage::stroke_dash_style_check_menu(const D2D1_DASH_STYLE d_style)
	{
		if (rmfi_stroke_dash_style_solid().IsChecked()) {
			rmfi_stroke_dash_style_solid().IsChecked(false);
		}
		if (rmfi_stroke_dash_style_solid_2().IsChecked()) {
			rmfi_stroke_dash_style_solid_2().IsChecked(false);
		}
		if (rmfi_stroke_dash_style_dash().IsChecked()) {
			rmfi_stroke_dash_style_dash().IsChecked(false);
		}
		if (rmfi_stroke_dash_style_dash_2().IsChecked()) {
			rmfi_stroke_dash_style_dash_2().IsChecked(false);
		}
		if (rmfi_stroke_dash_style_dash_dot().IsChecked()) {
			rmfi_stroke_dash_style_dash_dot().IsChecked(false);
		}
		if (rmfi_stroke_dash_style_dash_dot_2().IsChecked()) {
			rmfi_stroke_dash_style_dash_dot_2().IsChecked(false);
		}
		if (rmfi_stroke_dash_style_dash_dot_dot().IsChecked()) {
			rmfi_stroke_dash_style_dash_dot_dot().IsChecked(false);
		}
		if (rmfi_stroke_dash_style_dash_dot_dot_2().IsChecked()) {
			rmfi_stroke_dash_style_dash_dot_dot_2().IsChecked(false);
		}
		if (rmfi_stroke_dash_style_dot().IsChecked()) {
			rmfi_stroke_dash_style_dot().IsChecked(false);
		}
		if (rmfi_stroke_dash_style_dot_2().IsChecked()) {
			rmfi_stroke_dash_style_dot_2().IsChecked(false);
		}
		if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID) {
			rmfi_stroke_dash_style_solid().IsChecked(true);
			rmfi_stroke_dash_style_solid_2().IsChecked(true);
			mfi_stroke_dash_patt().IsEnabled(true);
			mfi_stroke_dash_patt_2().IsEnabled(true);
		}
		else {
			if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH) {
				rmfi_stroke_dash_style_dash().IsChecked(true);
				rmfi_stroke_dash_style_dash_2().IsChecked(true);
			}
			else if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT) {
				rmfi_stroke_dash_style_dot().IsChecked(true);
				rmfi_stroke_dash_style_dot_2().IsChecked(true);
			}
			else if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT) {
				rmfi_stroke_dash_style_dash_dot().IsChecked(true);
				rmfi_stroke_dash_style_dash_dot_2().IsChecked(true);
			}
			else if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
				rmfi_stroke_dash_style_dash_dot_dot().IsChecked(true);
				rmfi_stroke_dash_style_dash_dot_dot_2().IsChecked(true);
			}
			mfi_stroke_dash_patt().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);
			mfi_stroke_dash_patt_2().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);
		}
	}

}