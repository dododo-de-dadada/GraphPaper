//------------------------------
// MainPage_stroke.cpp
// 線枠
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	//using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	//using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	// 見本の図形を作成する.
	static void stroke_create_sample_shape(const float panel_w, const float panel_h, ShapeSheet& prop_sheet);

	// 見本の図形を作成する.
	// panel_w	見本を表示するパネルの幅
	// panel_h	見本を表示するパネルの高さ
	// prop_sheet	見本を表示するシート
	static void stroke_create_sample_shape(const float panel_w, const float panel_h, ShapeSheet& prop_sheet)
	{
		const auto padd = panel_w * 0.125;
		const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd), static_cast<FLOAT>(padd) };
		const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(panel_w - 2.0 * padd), static_cast<FLOAT>(panel_h - 2.0 * padd) };
		prop_sheet.m_shape_list.push_back(new ShapeLine(b_pos, b_vec, &prop_sheet));
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
	}

	// 線枠メニューの「線枠の色...」が選択された.
	IAsyncAction MainPage::stroke_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_prop_sheet.set_attr_to(&m_main_sheet);
		D2D1_COLOR_F s_color;
		m_prop_sheet.get_stroke_color(s_color);

		const float val0 = s_color.r * COLOR_MAX;
		const float val1 = s_color.g * COLOR_MAX;
		const float val2 = s_color.b * COLOR_MAX;
		const float val3 = s_color.a * COLOR_MAX;

		//FindName(L"cd_prop_dialog");

		prop_slider_0().Maximum(255.0);
		prop_slider_0().TickFrequency(1.0);
		prop_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_0().Value(val0);
		stroke_slider_set_header<UNDO_OP::STROKE_COLOR, 0>(val0);

		prop_slider_1().Maximum(255.0);
		prop_slider_1().TickFrequency(1.0);
		prop_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_1().Value(val1);
		stroke_slider_set_header<UNDO_OP::STROKE_COLOR, 1>(val1);

		prop_slider_2().Maximum(255.0);
		prop_slider_2().TickFrequency(1.0);
		prop_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_2().Value(val2);
		stroke_slider_set_header<UNDO_OP::STROKE_COLOR, 2>(val2);

		prop_slider_3().Maximum(255.0);
		prop_slider_3().TickFrequency(1.0);
		prop_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_3().Value(val3);
		stroke_slider_set_header<UNDO_OP::STROKE_COLOR, 3>(val3);

		prop_slider_0().Visibility(UI_VISIBLE);
		prop_slider_1().Visibility(UI_VISIBLE);
		prop_slider_2().Visibility(UI_VISIBLE);
		prop_slider_3().Visibility(UI_VISIBLE);
		const auto slider_0_token = prop_slider_0().ValueChanged({ this, &MainPage::stroke_slider_val_changed<UNDO_OP::STROKE_COLOR, 0> });
		const auto slider_1_token = prop_slider_1().ValueChanged({ this, &MainPage::stroke_slider_val_changed<UNDO_OP::STROKE_COLOR, 1> });
		const auto slider_2_token = prop_slider_2().ValueChanged({ this, &MainPage::stroke_slider_val_changed<UNDO_OP::STROKE_COLOR, 2> });
		const auto slider_3_token = prop_slider_3().ValueChanged({ this, &MainPage::stroke_slider_val_changed<UNDO_OP::STROKE_COLOR, 3> });

		stroke_create_sample_shape(static_cast<float>(scp_prop_panel().Width()), static_cast<float>(scp_prop_panel().Height()), m_prop_sheet);

		cd_prop_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_stroke_color")));
		const auto d_result = co_await cd_prop_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F samp_val;
			//m_sample_shape->get_stroke_color(samp_val);
			m_prop_sheet.m_shape_list.back()->get_stroke_color(samp_val);
			if (ustack_push_set<UNDO_OP::STROKE_COLOR>(samp_val)) {
				ustack_push_null();
				ustack_is_enable();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		slist_clear(m_prop_sheet.m_shape_list);
		prop_slider_0().Visibility(UI_COLLAPSED);
		prop_slider_1().Visibility(UI_COLLAPSED);
		prop_slider_2().Visibility(UI_COLLAPSED);
		prop_slider_3().Visibility(UI_COLLAPSED);
		prop_slider_0().ValueChanged(slider_0_token);
		prop_slider_1().ValueChanged(slider_1_token);
		prop_slider_2().ValueChanged(slider_2_token);
		prop_slider_3().ValueChanged(slider_3_token);

		//UnloadObject(cd_prop_dialog());

		sheet_draw();
	}

	// 値をスライダーのヘッダーに格納する.
	// U	操作の種類
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <UNDO_OP U, int S>
	void MainPage::stroke_slider_set_header(const float val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring text;

		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			wchar_t buf[32];
			float g_base;
			m_main_sheet.get_grid_base(g_base);
			conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, val, m_main_sheet.m_d2d.m_logical_dpi, g_base + 1.0f, buf);
			text = ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " + buf;
		}
		if constexpr (U == UNDO_OP::STROKE_COLOR) {
			constexpr wchar_t* R[]{ L"str_color_r", L"str_color_g", L"str_color_b", L"str_opacity" };
			wchar_t buf[32];
			conv_col_to_str(m_color_code, val, buf);
			text = ResourceLoader::GetForCurrentView().GetString(R[S]) + L": " + buf;
		}
		prop_set_slider_header<S>(text);
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S>
	void MainPage::stroke_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			const float val = static_cast<float>(args.NewValue());
			if constexpr (S == 0) {
				stroke_slider_set_header<U, S>(val);
				//m_sample_shape->set_stroke_width(val);
				m_prop_sheet.m_shape_list.back()->set_stroke_width(val);
			}
		}
		if constexpr (U == UNDO_OP::STROKE_COLOR) {
			const float val = static_cast<float>(args.NewValue());
			D2D1_COLOR_F color;
			//m_sample_shape->get_stroke_color(color);
			m_prop_sheet.m_shape_list.back()->get_stroke_color(color);
			if constexpr (S == 0) {
				stroke_slider_set_header<U, S>(val);
				color.r = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 1) {
				stroke_slider_set_header<U, S>(val);
				color.g = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 2) {
				stroke_slider_set_header<U, S>(val);
				color.b = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 3) {
				stroke_slider_set_header<U, S>(val);
				color.a = static_cast<FLOAT>(val / COLOR_MAX);
			}
			//m_sample_shape->set_stroke_color(color);
			m_prop_sheet.m_shape_list.back()->set_stroke_color(color);
		}
		if (scp_prop_panel().IsLoaded()) {
			prop_sample_draw();
		}
	}

	// 線枠メニューの「太さ」のサブ項目が選択された.
	void MainPage::stroke_width_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		float s_width;
		if (sender == rmfi_stroke_width_1px()) {
			s_width = 1.0f;
		}
		else if (sender == rmfi_stroke_width_2px()) {
			s_width = 2.0f;
		}
		else if (sender == rmfi_stroke_width_3px()) {
			s_width = 3.0f;
		}
		else if (sender == rmfi_stroke_width_4px()) {
			s_width = 4.0f;
		}
		else {
			return;
		}
		stroke_width_is_checked(s_width);
		if (ustack_push_set<UNDO_OP::STROKE_WIDTH>(s_width)) {
			ustack_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	void MainPage::stroke_width_is_checked(const float s_width) noexcept
	{
		rmfi_stroke_width_1px().IsChecked(s_width == 1.0f);
		rmfi_stroke_width_2px().IsChecked(s_width == 2.0f);
		rmfi_stroke_width_3px().IsChecked(s_width == 3.0f);
		rmfi_stroke_width_4px().IsChecked(s_width == 4.0f);
		rmfi_stroke_width_other().IsChecked(s_width != 1.0f && s_width != 2.0f && s_width != 3.0f && s_width != 4.0f);
	}

	// 線枠メニューの「太さ」が選択された.
	IAsyncAction MainPage::stroke_width_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		//FindName(L"cd_prop_dialog");

		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_prop_sheet.set_attr_to(&m_main_sheet);
		float s_width;
		m_prop_sheet.get_stroke_width(s_width);
		prop_slider_0().Maximum(MAX_VALUE);
		prop_slider_0().StepFrequency(TICK_FREQ);
		prop_slider_0().SnapsTo(SliderSnapsTo::StepValues);
		prop_slider_0().Value(s_width);
		stroke_slider_set_header<UNDO_OP::STROKE_WIDTH, 0>(s_width);
		prop_slider_0().Visibility(UI_VISIBLE);
		const auto slider_0_token = prop_slider_0().ValueChanged({ this, &MainPage::stroke_slider_val_changed<UNDO_OP::STROKE_WIDTH, 0> });

		stroke_create_sample_shape(static_cast<float>(scp_prop_panel().Width()), static_cast<float>(scp_prop_panel().Height()), m_prop_sheet);

		cd_prop_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width")));
		const auto d_result = co_await cd_prop_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float samp_val;
			m_prop_sheet.m_shape_list.back()->get_stroke_width(samp_val);
			stroke_width_is_checked(samp_val);
			if (ustack_push_set<UNDO_OP::STROKE_WIDTH>(samp_val)) {
				ustack_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		slist_clear(m_prop_sheet.m_shape_list);
		prop_slider_0().Visibility(UI_COLLAPSED);
		prop_slider_0().ValueChanged(slider_0_token);
		prop_slider_0().StepFrequency(1.0);
		prop_slider_0().Maximum(255.0);

		//m_prop_sheet.m_d2d.Trim();
		//UnloadObject(cd_prop_dialog());

		sheet_draw();
	}

}