//------------------------------
// MainPage_stroke.cpp
// 線枠
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t DLG_TITLE[] = L"str_stroke";
	//constexpr float SLIDER_STEP = 0.5f;

	// 線枠メニューの「ストロークの色...」が選択された.
	IAsyncAction MainPage::stroke_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		D2D1_COLOR_F s_color;
		m_sample_sheet.get_stroke_color(s_color);
		const float val0 = s_color.r * COLOR_MAX;
		const float val1 = s_color.g * COLOR_MAX;
		const float val2 = s_color.b * COLOR_MAX;
		const float val3 = s_color.a * COLOR_MAX;

		sample_slider_0().Maximum(255.0);
		sample_slider_0().TickFrequency(1.0);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(val0);
		stroke_slider_set_header<UNDO_OP::STROKE_COLOR, 0>(val0);
		sample_slider_1().Maximum(255.0);
		sample_slider_1().TickFrequency(1.0);
		sample_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_1().Value(val1);
		stroke_slider_set_header<UNDO_OP::STROKE_COLOR, 1>(val1);
		sample_slider_2().Maximum(255.0);
		sample_slider_2().TickFrequency(1.0);
		sample_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_2().Value(val2);
		stroke_slider_set_header<UNDO_OP::STROKE_COLOR, 2>(val2);
		sample_slider_3().Maximum(255.0);
		sample_slider_3().TickFrequency(1.0);
		sample_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_3().Value(val3);
		stroke_slider_set_header<UNDO_OP::STROKE_COLOR, 3>(val3);
		sample_slider_0().Visibility(UI_VISIBLE);
		sample_slider_1().Visibility(UI_VISIBLE);
		sample_slider_2().Visibility(UI_VISIBLE);
		sample_slider_3().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::stroke_slider_value_changed<UNDO_OP::STROKE_COLOR, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::stroke_slider_value_changed<UNDO_OP::STROKE_COLOR, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::stroke_slider_value_changed<UNDO_OP::STROKE_COLOR, 2> });
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::stroke_slider_value_changed<UNDO_OP::STROKE_COLOR, 3> });
		m_sample_type = SAMPLE_TYPE::STROKE;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F sample_value;
			m_sample_shape->get_stroke_color(sample_value);
			if (undo_push_set<UNDO_OP::STROKE_COLOR>(sample_value)) {
				undo_push_null();
				undo_menu_enable();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_1().Visibility(UI_COLLAPSED);
		sample_slider_2().Visibility(UI_COLLAPSED);
		sample_slider_3().Visibility(UI_COLLAPSED);
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
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		m_sample_sheet.set_attr_to(&m_sheet_main);

		STROKE_DASH_PATT d_patt;
		m_sheet_main.get_stroke_dash_patt(d_patt);
		sample_slider_0().Maximum(127.5);
		sample_slider_0().TickFrequency(0.5);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(d_patt.m_[0]);
		stroke_slider_set_header<UNDO_OP::STROKE_DASH_PATT, 0>(d_patt.m_[0]);
		sample_slider_1().Maximum(127.5);
		sample_slider_1().TickFrequency(0.5);
		sample_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_1().Value(d_patt.m_[1]);
		stroke_slider_set_header<UNDO_OP::STROKE_DASH_PATT, 1>(d_patt.m_[1]);
		sample_slider_2().Maximum(127.5);
		sample_slider_2().TickFrequency(0.5);
		sample_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_2().Value(d_patt.m_[2]);
		stroke_slider_set_header<UNDO_OP::STROKE_DASH_PATT, 2>(d_patt.m_[2]);
		sample_slider_3().Maximum(127.5);
		sample_slider_3().TickFrequency(0.5);
		sample_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_3().Value(d_patt.m_[3]);
		stroke_slider_set_header<UNDO_OP::STROKE_DASH_PATT, 3>(d_patt.m_[3]);

		float s_width;
		m_sheet_main.get_stroke_width(s_width);
		sample_slider_4().Maximum(127.5);
		sample_slider_4().TickFrequency(0.5);
		sample_slider_4().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_4().Value(s_width);
		stroke_slider_set_header<UNDO_OP::STROKE_WIDTH, 4>(s_width);

		D2D1_DASH_STYLE s_style;
		m_sheet_main.get_stroke_dash_style(s_style);
		sample_slider_0().Visibility(s_style != D2D1_DASH_STYLE_DOT ? UI_VISIBLE : UI_COLLAPSED);
		sample_slider_1().Visibility(s_style != D2D1_DASH_STYLE_DOT ? UI_VISIBLE : UI_COLLAPSED);
		sample_slider_2().Visibility(s_style != D2D1_DASH_STYLE_DASH ? UI_VISIBLE : UI_COLLAPSED);
		sample_slider_3().Visibility(s_style != D2D1_DASH_STYLE_DASH ? UI_VISIBLE : UI_COLLAPSED);
		sample_slider_4().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::stroke_slider_value_changed<UNDO_OP::STROKE_DASH_PATT, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::stroke_slider_value_changed<UNDO_OP::STROKE_DASH_PATT, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::stroke_slider_value_changed<UNDO_OP::STROKE_DASH_PATT, 2> });
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::stroke_slider_value_changed<UNDO_OP::STROKE_DASH_PATT, 3> });
		const auto slider_4_token = sample_slider_4().ValueChanged({ this, &MainPage::stroke_slider_value_changed<UNDO_OP::STROKE_WIDTH, 4> });
		m_sample_type = SAMPLE_TYPE::STROKE;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			STROKE_DASH_PATT sample_patt;
			float sample_width;
			m_sample_shape->get_stroke_dash_patt(sample_patt);
			m_sample_shape->get_stroke_width(sample_width);
			if (undo_push_set<UNDO_OP::STROKE_DASH_PATT>(sample_patt) ||
				undo_push_set<UNDO_OP::STROKE_WIDTH>(sample_width)) {
				undo_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_1().Visibility(UI_COLLAPSED);
		sample_slider_2().Visibility(UI_COLLAPSED);
		sample_slider_3().Visibility(UI_COLLAPSED);
		sample_slider_4().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sample_slider_2().ValueChanged(slider_2_token);
		sample_slider_3().ValueChanged(slider_3_token);
		sample_slider_4().ValueChanged(slider_4_token);
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
		mfi_stroke_dash_patt().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);
		mfi_stroke_dash_patt_2().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);
		if (undo_push_set<UNDO_OP::STROKE_DASH_STYLE>(d_style)) {
			undo_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// 線枠メニューの「種類」に印をつける.
	// d_style	破線の形式
	void MainPage::stroke_dash_style_is_checked(const D2D1_DASH_STYLE d_style)
	{
		rmfi_stroke_dash_style_solid().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		rmfi_stroke_dash_style_solid_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		rmfi_stroke_dash_style_dash().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH);
		rmfi_stroke_dash_style_dash_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH);
		rmfi_stroke_dash_style_dash_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT);
		rmfi_stroke_dash_style_dash_dot_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT);
		rmfi_stroke_dash_style_dash_dot_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT);
		rmfi_stroke_dash_style_dash_dot_dot_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT);
		rmfi_stroke_dash_style_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT);
		rmfi_stroke_dash_style_dot_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT);

		mfi_stroke_dash_patt().IsEnabled(d_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		mfi_stroke_dash_patt_2().IsEnabled(d_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S> void MainPage::stroke_slider_set_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring text;

		if constexpr (U == UNDO_OP::STROKE_DASH_PATT) {
			constexpr wchar_t* R[]{ L"str_dash_len", L"str_dash_gap", L"str_dot_len", L"str_dot_gap" };
			wchar_t buf[32];
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			conv_len_to_str<LEN_UNIT_SHOW>(m_misc_len_unit, value/* * SLIDER_STEP*/, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
			text = ResourceLoader::GetForCurrentView().GetString(R[S]) + L": " + buf;
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			wchar_t buf[32];
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			conv_len_to_str<LEN_UNIT_SHOW>(m_misc_len_unit, value/* * SLIDER_STEP*/, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
			text = ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " + buf;
		}
		if constexpr (U == UNDO_OP::STROKE_COLOR) {
			constexpr wchar_t* R[]{ L"str_col_r", L"str_col_g", L"str_col_b", L"str_opacity" };
			wchar_t buf[32];
			conv_col_to_str(m_misc_color_code, value, buf);
			text = ResourceLoader::GetForCurrentView().GetString(R[S]) + L": " + buf;
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

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S> void MainPage::stroke_slider_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* s = m_sample_shape;
		const float value = static_cast<float>(args.NewValue());
		stroke_slider_set_header<U, S>(value);
		if constexpr (U == UNDO_OP::STROKE_DASH_PATT) {
			STROKE_DASH_PATT patt;
			s->get_stroke_dash_patt(patt);
			if constexpr (S == 0) {
				patt.m_[0] = static_cast<FLOAT>(value);// * SLIDER_STEP);
			}
			if constexpr (S == 1) {
				patt.m_[1] = static_cast<FLOAT>(value);// * SLIDER_STEP);
			}
			if constexpr (S == 2) {
				patt.m_[2] = patt.m_[4] = static_cast<FLOAT>(value);// * SLIDER_STEP);
			}
			if constexpr (S == 3) {
				patt.m_[3] = patt.m_[5] = static_cast<FLOAT>(value);// * SLIDER_STEP);
			}
			s->set_stroke_dash_patt(patt);
		}
		if constexpr (U == UNDO_OP::STROKE_JOIN_LIMIT) {
			s->set_stroke_join_limit(value);
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			s->set_stroke_width(value);// * SLIDER_STEP);
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

	// 線枠メニューの「太さ...」が選択された.
	IAsyncAction MainPage::stroke_width_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		float s_width;
		m_sample_sheet.get_stroke_width(s_width);
		sample_slider_0().Maximum(127.5);
		sample_slider_0().TickFrequency(0.5);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(s_width);
		stroke_slider_set_header<UNDO_OP::STROKE_WIDTH, 0>(s_width);
		sample_slider_0().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::stroke_slider_value_changed<UNDO_OP::STROKE_WIDTH, 0> });
		m_sample_type = SAMPLE_TYPE::STROKE;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_value;
			m_sample_shape->get_stroke_width(sample_value);
			if (undo_push_set<UNDO_OP::STROKE_WIDTH>(sample_value)) {
				undo_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_0().StepFrequency(1.0);
		sample_slider_0().Maximum(255.0);
		sheet_draw();
	}

}