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
	constexpr float SLIDER_STEP = 0.5f;

	// 線枠メニューの「色」が選択された.
	IAsyncAction MainPage::stroke_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		D2D1_COLOR_F value;
		m_sample_sheet.get_stroke_color(value);
		const float val0 = value.r * COLOR_MAX;
		const float val1 = value.g * COLOR_MAX;
		const float val2 = value.b * COLOR_MAX;
		const float val3 = value.a * COLOR_MAX;
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
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
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
		STROKE_DASH_PATT value;
		m_sheet_main.get_stroke_dash_patt(value);
		const float val0 = value.m_[0] / SLIDER_STEP;
		const float val1 = value.m_[1] / SLIDER_STEP;
		const float val2 = value.m_[2] / SLIDER_STEP;
		const float val3 = value.m_[3] / SLIDER_STEP;
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
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
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
		float value;
		m_sample_sheet.get_stroke_width(value);
		const float val0 = value / SLIDER_STEP;
		sample_slider_0().Value(val0);
		sample_slider_0().Visibility(VISIBLE);
		stroke_set_slider_header<UNDO_OP::STROKE_WIDTH, 0>(val0);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::stroke_set_slider<UNDO_OP::STROKE_WIDTH, 0> });
		m_sample_type = SAMP_TYPE::STROKE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
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

		if constexpr (U == UNDO_OP::STROKE_DASH_PATT) {
			constexpr wchar_t* R[]{ L"str_dash_len", L"str_dash_gap", L"str_dot_len", L"str_dot_gap" };
			wchar_t buf[32];
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), value * SLIDER_STEP, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
			text = ResourceLoader::GetForCurrentView().GetString(R[S]) + L": " + buf;
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			wchar_t buf[32];
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), value * SLIDER_STEP, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
			text = ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " + buf;
		}
		if constexpr (U == UNDO_OP::STROKE_COLOR) {
			constexpr wchar_t* R[]{ L"str_col_r", L"str_col_g", L"str_col_b", L"str_opacity" };
			wchar_t buf[32];
			conv_col_to_str(color_code(), value, buf);
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
			STROKE_DASH_PATT patt;
			s->get_stroke_dash_patt(patt);
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
			s->set_stroke_dash_patt(patt);
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
	void MainPage::stroke_dash_style_is_checked(const D2D1_DASH_STYLE d_style)
	{
		radio_menu_item_is_checked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID, rmfi_stroke_dash_style_solid());
		radio_menu_item_is_checked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID, rmfi_stroke_dash_style_solid_2());
		radio_menu_item_is_checked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH, rmfi_stroke_dash_style_dash());
		radio_menu_item_is_checked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH, rmfi_stroke_dash_style_dash_2());
		radio_menu_item_is_checked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT, rmfi_stroke_dash_style_dash_dot());
		radio_menu_item_is_checked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT, rmfi_stroke_dash_style_dash_dot_2());
		radio_menu_item_is_checked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT, rmfi_stroke_dash_style_dash_dot_dot());
		radio_menu_item_is_checked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT, rmfi_stroke_dash_style_dash_dot_dot_2());
		radio_menu_item_is_checked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT, rmfi_stroke_dash_style_dot());
		radio_menu_item_is_checked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT, rmfi_stroke_dash_style_dot_2());
		//radio_menu_item_set_value<D2D1_DASH_STYLE, D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID>(d_style, rmfi_stroke_dash_style_solid());
		//radio_menu_item_set_value<D2D1_DASH_STYLE, D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID>(d_style, rmfi_stroke_dash_style_solid_2());
		//radio_menu_item_set_value<D2D1_DASH_STYLE, D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH>(d_style, rmfi_stroke_dash_style_dash());
		//radio_menu_item_set_value<D2D1_DASH_STYLE, D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH>(d_style, rmfi_stroke_dash_style_dash_2());
		//radio_menu_item_set_value<D2D1_DASH_STYLE, D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT>(d_style, rmfi_stroke_dash_style_dash_dot());
		//radio_menu_item_set_value<D2D1_DASH_STYLE, D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT>(d_style, rmfi_stroke_dash_style_dash_dot_2());
		//radio_menu_item_set_value<D2D1_DASH_STYLE, D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT>(d_style, rmfi_stroke_dash_style_dash_dot_dot());
		//radio_menu_item_set_value<D2D1_DASH_STYLE, D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT>(d_style, rmfi_stroke_dash_style_dash_dot_dot_2());
		//radio_menu_item_set_value<D2D1_DASH_STYLE, D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT>(d_style, rmfi_stroke_dash_style_dot());
		//radio_menu_item_set_value<D2D1_DASH_STYLE, D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT>(d_style, rmfi_stroke_dash_style_dot_2());
		menu_item_is_enabled(d_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID, mfi_stroke_dash_patt());
		menu_item_is_enabled(d_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID, mfi_stroke_dash_patt_2());
	}

}