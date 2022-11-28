//-------------------------------
// MainPage_fill.cpp
// 塗りつぶし
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	//using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	//using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	constexpr wchar_t TITLE_FILL[] = L"str_fill";

	// 塗りつぶしメニューの「色」が選択された.
	IAsyncAction MainPage::fill_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_prop_sheet.set_attr_to(&m_main_sheet);
		D2D1_COLOR_F val;
		m_prop_sheet.get_fill_color(val);
		const float val0 = val.r * COLOR_MAX;
		const float val1 = val.g * COLOR_MAX;
		const float val2 = val.b * COLOR_MAX;
		const float val3 = val.a * COLOR_MAX;

		//FindName(L"cd_prop_dialog");

		prop_slider_0().Maximum(255.0);
		prop_slider_0().TickFrequency(1.0);
		prop_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_0().Value(val0);
		fill_slider_set_header<UNDO_OP::FILL_COLOR, 0>(val0);
		prop_slider_1().Maximum(255.0);
		prop_slider_1().TickFrequency(1.0);
		prop_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_1().Value(val1);
		fill_slider_set_header<UNDO_OP::FILL_COLOR, 1>(val1);
		prop_slider_2().Maximum(255.0);
		prop_slider_2().TickFrequency(1.0);
		prop_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_2().Value(val2);
		fill_slider_set_header<UNDO_OP::FILL_COLOR, 2>(val2);
		prop_slider_3().Maximum(255.0);
		prop_slider_3().TickFrequency(1.0);
		prop_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_3().Value(val3);
		fill_slider_set_header<UNDO_OP::FILL_COLOR, 3>(val3);
		prop_slider_0().Visibility(Visibility::Visible);
		prop_slider_1().Visibility(Visibility::Visible);
		prop_slider_2().Visibility(Visibility::Visible);
		prop_slider_3().Visibility(Visibility::Visible);
		const auto slider_0_token = prop_slider_0().ValueChanged({ this, &MainPage::fill_slider_val_changed<UNDO_OP::FILL_COLOR, 0> });
		const auto slider_1_token = prop_slider_1().ValueChanged({ this, &MainPage::fill_slider_val_changed<UNDO_OP::FILL_COLOR, 1> });
		const auto slider_2_token = prop_slider_2().ValueChanged({ this, &MainPage::fill_slider_val_changed<UNDO_OP::FILL_COLOR, 2> });
		const auto slider_3_token = prop_slider_3().ValueChanged({ this, &MainPage::fill_slider_val_changed<UNDO_OP::FILL_COLOR, 3> });

		const auto panel_w = scp_prop_panel().Width();
		const auto panel_h = scp_prop_panel().Height();
		const auto padd = panel_w * 0.125;
		const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd), static_cast<FLOAT>(padd) };
		const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(panel_w - 2.0 * padd), static_cast<FLOAT>(panel_h - 2.0 * padd) };
		//m_sample_shape = new ShapeRect(b_pos, b_vec, &m_prop_sheet);
		m_prop_sheet.m_shape_list.push_back(new ShapeRect(b_pos, b_vec, &m_prop_sheet));
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		cd_prop_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_FILL)));
		const auto d_result = co_await cd_prop_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F samp_val;
			m_prop_sheet.m_shape_list.back()->get_fill_color(samp_val);
			if (ustack_push_set<UNDO_OP::FILL_COLOR>(samp_val)) {
				ustack_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		slist_clear(m_prop_sheet.m_shape_list);
		prop_slider_0().Visibility(Visibility::Collapsed);
		prop_slider_1().Visibility(Visibility::Collapsed);
		prop_slider_2().Visibility(Visibility::Collapsed);
		prop_slider_3().Visibility(Visibility::Collapsed);
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
	void MainPage::fill_slider_set_header(const float val)
	{
		winrt::hstring text;

		if constexpr (U == UNDO_OP::FILL_COLOR) {
			constexpr wchar_t* HEADER[]{ L"str_color_r", L"str_color_g",L"str_color_b", L"str_opacity" };
			wchar_t buf[32];
			conv_col_to_str(m_color_code, val, buf);
			text = ResourceLoader::GetForCurrentView().GetString(HEADER[S]) + L": " + buf;
		}
		prop_set_slider_header<S>(text);
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S>
	void MainPage::fill_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::FILL_COLOR) {
			const float val = static_cast<float>(args.NewValue());
			fill_slider_set_header<U, S>(val);
			D2D1_COLOR_F f_color;
			m_prop_sheet.m_shape_list.back()->get_fill_color(f_color);
			if constexpr (S == 0) {
				f_color.r = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 1) {
				f_color.g = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 2) {
				f_color.b = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (U != UNDO_OP::SHEET_COLOR && S == 3) {
				f_color.a = static_cast<FLOAT>(val / COLOR_MAX);
			}
			//m_sample_shape->set_fill_color(f_color);
			m_prop_sheet.m_shape_list.back()->set_fill_color(f_color);
			if (scp_prop_panel().IsLoaded()) {
				prop_sample_draw();
			}
		}
	}

}