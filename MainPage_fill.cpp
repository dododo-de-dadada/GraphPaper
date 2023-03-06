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
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

	constexpr wchar_t TITLE_FILL[] = L"str_fill_color";

	// 塗りつぶしメニューの「塗りつぶし色」が選択された.
	IAsyncAction MainPage::fill_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_dialog_page.set_attr_to(&m_main_page);
		D2D1_COLOR_F val;
		m_dialog_page.get_fill_color(val);
		const float val0 = static_cast<float>(conv_color_comp(val.r));
		const float val1 = static_cast<float>(conv_color_comp(val.g));
		const float val2 = static_cast<float>(conv_color_comp(val.b));
		const float val3 = static_cast<float>(conv_color_comp(val.a));

		//FindName(L"cd_setting_dialog");

		dialog_slider_0().Maximum(255.0);
		dialog_slider_0().TickFrequency(1.0);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val0);
		fill_slider_set_header<0>(val0);
		dialog_slider_1().Maximum(255.0);
		dialog_slider_1().TickFrequency(1.0);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(val1);
		fill_slider_set_header<1>(val1);
		dialog_slider_2().Maximum(255.0);
		dialog_slider_2().TickFrequency(1.0);
		dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_2().Value(val2);
		fill_slider_set_header<2>(val2);
		dialog_slider_3().Maximum(255.0);
		dialog_slider_3().TickFrequency(1.0);
		dialog_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_3().Value(val3);
		fill_slider_set_header<3>(val3);
		dialog_slider_0().Visibility(Visibility::Visible);
		dialog_slider_1().Visibility(Visibility::Visible);
		dialog_slider_2().Visibility(Visibility::Visible);
		dialog_slider_3().Visibility(Visibility::Visible);
		const auto slider_0_token = dialog_slider_0().ValueChanged(
			[=](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
				const float val = static_cast<float>(args.NewValue());
				fill_slider_set_header<0>(val);
				D2D1_COLOR_F f_color;
				m_dialog_page.m_shape_list.back()->get_fill_color(f_color);
				f_color.r = static_cast<FLOAT>(val / COLOR_MAX);
				if (scp_dialog_panel().IsLoaded() &&
					m_dialog_page.m_shape_list.back()->set_fill_color(f_color)) {
					dialog_draw();
				}
			}
		);
		const auto slider_1_token = dialog_slider_1().ValueChanged(
			[=](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
				const float val = static_cast<float>(args.NewValue());
				fill_slider_set_header<1>(val);
				D2D1_COLOR_F f_color;
				m_dialog_page.m_shape_list.back()->get_fill_color(f_color);
				f_color.g = static_cast<FLOAT>(val / COLOR_MAX);
				if (scp_dialog_panel().IsLoaded() &&
					m_dialog_page.m_shape_list.back()->set_fill_color(f_color)) {
					dialog_draw();
				}
			}
		);
		const auto slider_2_token = dialog_slider_2().ValueChanged(
			[=](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
				const float val = static_cast<float>(args.NewValue());
				fill_slider_set_header<2>(val);
				D2D1_COLOR_F f_color;
				m_dialog_page.m_shape_list.back()->get_fill_color(f_color);
				f_color.b = static_cast<FLOAT>(val / COLOR_MAX);
				if (scp_dialog_panel().IsLoaded() &&
					m_dialog_page.m_shape_list.back()->set_fill_color(f_color)) {
					dialog_draw();
				}
			}
		);
		const auto slider_3_token = dialog_slider_3().ValueChanged(
			[=](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
				const float val = static_cast<float>(args.NewValue());
				fill_slider_set_header<3>(val);
				D2D1_COLOR_F f_color;
				m_dialog_page.m_shape_list.back()->get_fill_color(f_color);
				f_color.a = static_cast<FLOAT>(val / COLOR_MAX);
				if (scp_dialog_panel().IsLoaded() &&
					m_dialog_page.m_shape_list.back()->set_fill_color(f_color)) {
					dialog_draw();
				}

			}
		);

		const auto p_width = scp_dialog_panel().Width();
		const auto p_height = scp_dialog_panel().Height();
		const auto padd = p_width * 0.125;
		const D2D1_POINT_2F start{
			static_cast<FLOAT>(padd), static_cast<FLOAT>(padd)
		};
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(p_width - 2.0 * padd), static_cast<FLOAT>(p_height - 2.0 * padd)
		};
		m_dialog_page.m_shape_list.push_back(new ShapeRect(start, pos, &m_dialog_page));
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		cd_setting_dialog().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_FILL)));
		m_mutex_event.lock();
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F samp_val;
			m_dialog_page.m_shape_list.back()->get_fill_color(samp_val);
			if (ustack_push_set<UNDO_ID::FILL_COLOR>(samp_val)) {
				ustack_push_null();
				xcvd_is_enabled();
				page_draw();
			}
		}
		slist_clear(m_dialog_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		dialog_slider_2().Visibility(Visibility::Collapsed);
		dialog_slider_3().Visibility(Visibility::Collapsed);
		dialog_slider_0().ValueChanged(slider_0_token);
		dialog_slider_1().ValueChanged(slider_1_token);
		dialog_slider_2().ValueChanged(slider_2_token);
		dialog_slider_3().ValueChanged(slider_3_token);

		//UnloadObject(cd_setting_dialog());

		page_draw();
		m_mutex_event.unlock();
	}

	// 値をスライダーのヘッダーに格納する.
	// U	操作の識別子
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <int S>
	void MainPage::fill_slider_set_header(const float val)
	{
		constexpr wchar_t* HEADER[]{ 
			L"str_color_r", L"str_color_g", L"str_color_b", L"str_opacity"
		};
		wchar_t buf[32];
		conv_col_to_str(m_color_code, val, buf);
		const winrt::hstring text{
			ResourceLoader::GetForCurrentView().GetString(HEADER[S]) + L": " + buf
		};
		dialog_set_slider_header<S>(text);
	}

	/*
	// スライダーの値が変更された.
	// U	操作の識別子
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_ID U, int S>
	void MainPage::fill_slider_val_changed(
		IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_ID::FILL_COLOR) {
			const float val = static_cast<float>(args.NewValue());
			fill_slider_set_header<U, S>(val);
			D2D1_COLOR_F f_color;
			m_dialog_page.m_shape_list.back()->get_fill_color(f_color);
			if constexpr (S == 0) {
				f_color.r = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 1) {
				f_color.g = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 2) {
				f_color.b = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 3) {
				f_color.a = static_cast<FLOAT>(val / COLOR_MAX);
			}
			m_dialog_page.m_shape_list.back()->set_fill_color(f_color);
			if (scp_dialog_panel().IsLoaded()) {
				dialog_draw();
			}
		}
	}
	*/

}