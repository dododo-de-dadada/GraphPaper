//-------------------------------
// MainPage_fill.cpp
// 塗りつぶし
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t TITLE_FILL[] = L"str_fill";

	// 塗りつぶしメニューの「色」が選択された.
	IAsyncAction MainPage::fill_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		m_sample_sheet.set_attr_to(&m_main_sheet);
		D2D1_COLOR_F value;
		m_sample_sheet.get_fill_color(value);
		const float val0 = value.r * COLOR_MAX;
		const float val1 = value.g * COLOR_MAX;
		const float val2 = value.b * COLOR_MAX;
		const float val3 = value.a * COLOR_MAX;
		sample_slider_0().Maximum(255.0);
		sample_slider_0().TickFrequency(1.0);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(val0);
		fill_slider_set_header<UNDO_OP::FILL_COLOR, 0>(val0);
		sample_slider_1().Maximum(255.0);
		sample_slider_1().TickFrequency(1.0);
		sample_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_1().Value(val1);
		fill_slider_set_header<UNDO_OP::FILL_COLOR, 1>(val1);
		sample_slider_2().Maximum(255.0);
		sample_slider_2().TickFrequency(1.0);
		sample_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_2().Value(val2);
		fill_slider_set_header<UNDO_OP::FILL_COLOR, 2>(val2);
		sample_slider_3().Maximum(255.0);
		sample_slider_3().TickFrequency(1.0);
		sample_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_3().Value(val3);
		fill_slider_set_header<UNDO_OP::FILL_COLOR, 3>(val3);
		sample_slider_0().Visibility(UI_VISIBLE);
		sample_slider_1().Visibility(UI_VISIBLE);
		sample_slider_2().Visibility(UI_VISIBLE);
		sample_slider_3().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::fill_slider_value_changed<UNDO_OP::FILL_COLOR, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::fill_slider_value_changed<UNDO_OP::FILL_COLOR, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::fill_slider_value_changed<UNDO_OP::FILL_COLOR, 2> });
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::fill_slider_value_changed<UNDO_OP::FILL_COLOR, 3> });
		m_sample_type = SAMPLE_TYPE::FILL;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_FILL)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F sample_value;
			//m_sample_shape->get_fill_color(sample_value);
			m_sample_sheet.m_shape_list.back()->get_fill_color(sample_value);
			if (ustack_push_set<UNDO_OP::FILL_COLOR>(sample_value)) {
				ustack_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		//delete m_sample_shape;
		delete m_sample_sheet.m_shape_list.back();
		m_sample_sheet.m_shape_list.clear();
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		//m_sample_shape = nullptr;
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

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S> void MainPage::fill_slider_set_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring text;

		if constexpr (U == UNDO_OP::FILL_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				conv_col_to_str(m_misc_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_color_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				conv_col_to_str(m_misc_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_color_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				conv_col_to_str(m_misc_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_color_b") + L": " + buf;
			}
			if constexpr (S == 3) {
				wchar_t buf[32];
				conv_col_to_str(m_misc_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_opacity") + L": " + buf;
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
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S> void MainPage::fill_slider_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::FILL_COLOR) {
			const float value = static_cast<float>(args.NewValue());
			fill_slider_set_header<U, S>(value);
			D2D1_COLOR_F f_color;
			//m_sample_shape->get_fill_color(f_color);
			m_sample_sheet.m_shape_list.back()->get_fill_color(f_color);
			if constexpr (S == 0) {
				f_color.r = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 1) {
				f_color.g = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 2) {
				f_color.b = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (U != UNDO_OP::SHEET_COLOR && S == 3) {
				f_color.a = static_cast<FLOAT>(value / COLOR_MAX);
			}
			//m_sample_shape->set_fill_color(f_color);
			m_sample_sheet.m_shape_list.back()->set_fill_color(f_color);
			if (scp_sample_panel().IsLoaded()) {
				sample_draw();
			}
		}
	}

}