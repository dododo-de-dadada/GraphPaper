//------------------------------
// MainPage_dash.cpp
// 線枠
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t DLG_TITLE[] = L"str_dash_patt";

	// 線枠メニューの「破線の様式」が選択された.
	IAsyncAction MainPage::dash_patt_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_sample_sheet.set_attr_to(&m_sheet_main);
		DASH_PATT d_patt;
		m_sheet_main.get_dash_patt(d_patt);

		sample_slider_0().Maximum(MAX_VALUE);
		sample_slider_0().TickFrequency(TICK_FREQ);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(d_patt.m_[0]);
		dash_slider_set_header<UNDO_OP::DASH_PATT, 0>(d_patt.m_[0]);
		sample_slider_1().Maximum(MAX_VALUE);
		sample_slider_1().TickFrequency(TICK_FREQ);
		sample_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_1().Value(d_patt.m_[1]);
		dash_slider_set_header<UNDO_OP::DASH_PATT, 1>(d_patt.m_[1]);
		sample_slider_2().Maximum(MAX_VALUE);
		sample_slider_2().TickFrequency(TICK_FREQ);
		sample_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_2().Value(d_patt.m_[2]);
		dash_slider_set_header<UNDO_OP::DASH_PATT, 2>(d_patt.m_[2]);
		sample_slider_3().Maximum(MAX_VALUE);
		sample_slider_3().TickFrequency(TICK_FREQ);
		sample_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_3().Value(d_patt.m_[3]);
		dash_slider_set_header<UNDO_OP::DASH_PATT, 3>(d_patt.m_[3]);

		float s_width;
		m_sheet_main.get_stroke_width(s_width);

		sample_slider_4().Maximum(MAX_VALUE);
		sample_slider_4().TickFrequency(TICK_FREQ);
		sample_slider_4().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_4().Value(s_width);
		dash_slider_set_header<UNDO_OP::STROKE_WIDTH, 4>(s_width);

		D2D1_DASH_STYLE s_style;
		m_sheet_main.get_dash_style(s_style);
		sample_slider_0().Visibility(s_style != D2D1_DASH_STYLE_DOT ? UI_VISIBLE : UI_COLLAPSED);
		sample_slider_1().Visibility(s_style != D2D1_DASH_STYLE_DOT ? UI_VISIBLE : UI_COLLAPSED);
		sample_slider_2().Visibility(s_style != D2D1_DASH_STYLE_DASH ? UI_VISIBLE : UI_COLLAPSED);
		sample_slider_3().Visibility(s_style != D2D1_DASH_STYLE_DASH ? UI_VISIBLE : UI_COLLAPSED);
		sample_slider_4().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::dash_slider_value_changed<UNDO_OP::DASH_PATT, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::dash_slider_value_changed<UNDO_OP::DASH_PATT, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::dash_slider_value_changed<UNDO_OP::DASH_PATT, 2> });
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::dash_slider_value_changed<UNDO_OP::DASH_PATT, 3> });
		const auto slider_4_token = sample_slider_4().ValueChanged({ this, &MainPage::dash_slider_value_changed<UNDO_OP::STROKE_WIDTH, 4> });
		m_sample_type = SAMPLE_TYPE::STROKE;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			DASH_PATT sample_patt;
			float sample_width;
			m_sample_shape->get_dash_patt(sample_patt);
			m_sample_shape->get_stroke_width(sample_width);
			if (ustack_push_set<UNDO_OP::DASH_PATT>(sample_patt) ||
				ustack_push_set<UNDO_OP::STROKE_WIDTH>(sample_width)) {
				ustack_push_null();
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
	void MainPage::dash_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		D2D1_DASH_STYLE d_style;
		if (sender == rmfi_dash_style_solid() || sender == rmfi_dash_style_solid_2()) {
			d_style = D2D1_DASH_STYLE_SOLID;
		}
		else if (sender == rmfi_dash_style_dash() || sender == rmfi_dash_style_dash_2()) {
			d_style = D2D1_DASH_STYLE_DASH;
		}
		else if (sender == rmfi_dash_style_dot() || sender == rmfi_dash_style_dot_2()) {
			d_style = D2D1_DASH_STYLE_DOT;
		}
		else if (sender == rmfi_dash_style_dash_dot() || sender == rmfi_dash_style_dash_dot_2()) {
			d_style = D2D1_DASH_STYLE_DASH_DOT;
		}
		else if (sender == rmfi_dash_style_dash_dot_dot() || sender == rmfi_dash_style_dash_dot_dot_2()) {
			d_style = D2D1_DASH_STYLE_DASH_DOT_DOT;
		}
		else {
			return;
		}
		mfi_dash_patt().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);
		mfi_dash_patt_2().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);
		if (ustack_push_set<UNDO_OP::DASH_STYLE>(d_style)) {
			ustack_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// 線枠メニューの「種類」に印をつける.
	// d_style	破線の形式
	void MainPage::dash_style_is_checked(const D2D1_DASH_STYLE d_style)
	{
		rmfi_dash_style_solid().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		rmfi_dash_style_solid_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		rmfi_dash_style_dash().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH);
		rmfi_dash_style_dash_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH);
		rmfi_dash_style_dash_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT);
		rmfi_dash_style_dash_dot_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT);
		rmfi_dash_style_dash_dot_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT);
		rmfi_dash_style_dash_dot_dot_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT);
		rmfi_dash_style_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT);
		rmfi_dash_style_dot_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT);

		mfi_dash_patt().IsEnabled(d_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		mfi_dash_patt_2().IsEnabled(d_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S> void MainPage::dash_slider_set_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		if constexpr (U == UNDO_OP::DASH_PATT) {
			constexpr wchar_t* R[]{ L"str_dash_len", L"str_dash_gap", L"str_dot_len", L"str_dot_gap" };
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_misc_len_unit, value/* * SLIDER_STEP*/, m_sheet_dx.m_logical_dpi, m_sheet_main.m_grid_base + 1.0f, buf);
			const auto text = ResourceLoader::GetForCurrentView().GetString(R[S]) + L": " + buf;
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
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH && S == 4) {
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_misc_len_unit, value/* * SLIDER_STEP*/, m_sheet_dx.m_logical_dpi, m_sheet_main.m_grid_base + 1.0f, buf);
			const auto text = ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " + buf;
			sample_slider_4().Header(box_value(text));
		}
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S> void MainPage::dash_slider_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::DASH_PATT) {
			const float value = static_cast<float>(args.NewValue());
			DASH_PATT patt;
			m_sample_shape->get_dash_patt(patt);
			if constexpr (S == 0) {
				dash_slider_set_header<U, S>(value);
				patt.m_[0] = static_cast<FLOAT>(value);
			}
			else if constexpr (S == 1) {
				dash_slider_set_header<U, S>(value);
				patt.m_[1] = static_cast<FLOAT>(value);
			}
			else if constexpr (S == 2) {
				dash_slider_set_header<U, S>(value);
				patt.m_[2] = patt.m_[4] = static_cast<FLOAT>(value);
			}
			else if constexpr (S == 3) {
				dash_slider_set_header<U, S>(value);
				patt.m_[3] = patt.m_[5] = static_cast<FLOAT>(value);
			}
			m_sample_shape->set_dash_patt(patt);
		}
		else if constexpr (U == UNDO_OP::STROKE_WIDTH && S == 4) {
			const float value = static_cast<float>(args.NewValue());
			dash_slider_set_header<U, S>(value);
			m_sample_shape->set_stroke_width(value);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

}