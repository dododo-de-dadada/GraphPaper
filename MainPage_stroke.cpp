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
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

	// 見本の図形を作成する.
	static void stroke_create_sample_shape(
		const float p_width, const float p_height, ShapePage& page);

	// 見本の図形を作成する.
	// p_width	見本を表示するパネルの幅
	// p_height	見本を表示するパネルの高さ
	// page	見本を表示するシート
	static void stroke_create_sample_shape(
		const float p_width, const float p_height, ShapePage& page)
	{
		const auto padd = p_width * 0.125;
		const D2D1_POINT_2F start{
			static_cast<FLOAT>(padd), static_cast<FLOAT>(padd)
		};
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(p_width - 2.0 * padd), static_cast<FLOAT>(p_height - 2.0 * padd)
		};
		page.m_shape_list.push_back(new ShapeLine(start, pos, &page));
		page.m_shape_list.back()->set_select(true);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
	}

	// 線枠メニューの「線枠の色...」が選択された.
	IAsyncAction MainPage::stroke_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_dialog_page.set_attr_to(&m_main_page);
		D2D1_COLOR_F s_color;
		m_dialog_page.get_stroke_color(s_color);

		const float val0 = static_cast<float>(conv_color_comp(s_color.r));
		const float val1 = static_cast<float>(conv_color_comp(s_color.g));
		const float val2 = static_cast<float>(conv_color_comp(s_color.b));
		const float val3 = static_cast<float>(conv_color_comp(s_color.a));

		dialog_slider_0().Maximum(255.0);
		dialog_slider_0().TickFrequency(1.0);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val0);
		stroke_slider_set_header<UNDO_ID::STROKE_COLOR, 0>(val0);

		dialog_slider_1().Maximum(255.0);
		dialog_slider_1().TickFrequency(1.0);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(val1);
		stroke_slider_set_header<UNDO_ID::STROKE_COLOR, 1>(val1);

		dialog_slider_2().Maximum(255.0);
		dialog_slider_2().TickFrequency(1.0);
		dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_2().Value(val2);
		stroke_slider_set_header<UNDO_ID::STROKE_COLOR, 2>(val2);

		dialog_slider_3().Maximum(255.0);
		dialog_slider_3().TickFrequency(1.0);
		dialog_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_3().Value(val3);
		stroke_slider_set_header<UNDO_ID::STROKE_COLOR, 3>(val3);

		dialog_slider_0().Visibility(Visibility::Visible);
		dialog_slider_1().Visibility(Visibility::Visible);
		dialog_slider_2().Visibility(Visibility::Visible);
		dialog_slider_3().Visibility(Visibility::Visible);
		const auto token0{
			dialog_slider_0().ValueChanged(
				{ this, &MainPage::stroke_slider_value_changed<UNDO_ID::STROKE_COLOR, 0> })
		};
		const auto token1{
			dialog_slider_1().ValueChanged(
				{ this, &MainPage::stroke_slider_value_changed<UNDO_ID::STROKE_COLOR, 1> })
		};
		const auto token2{
			dialog_slider_2().ValueChanged(
				{ this, &MainPage::stroke_slider_value_changed<UNDO_ID::STROKE_COLOR, 2> })
		};
		const auto token3{
			dialog_slider_3().ValueChanged(
				{ this, &MainPage::stroke_slider_value_changed<UNDO_ID::STROKE_COLOR, 3> })
		};
		stroke_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), m_dialog_page);

		cd_setting_dialog().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(L"str_stroke_color")));
		m_mutex_event.lock();
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F val;
			m_dialog_page.m_shape_list.back()->get_stroke_color(val);
			if (ustack_push_set<UNDO_ID::STROKE_COLOR>(val)) {
				ustack_push_null();
				ustack_is_enable();
				xcvd_is_enabled();
				page_draw();
			}
		}
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		dialog_slider_2().Visibility(Visibility::Collapsed);
		dialog_slider_3().Visibility(Visibility::Collapsed);
		dialog_slider_0().ValueChanged(token0);
		dialog_slider_1().ValueChanged(token1);
		dialog_slider_2().ValueChanged(token2);
		dialog_slider_3().ValueChanged(token3);

		page_draw();
		m_mutex_event.unlock();
		slist_clear(m_dialog_page.m_shape_list);
	}

	// 値をスライダーのヘッダーに格納する.
	// U	操作の識別子
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <UNDO_ID U, int S>
	void MainPage::stroke_slider_set_header(const float val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring text;

		if constexpr (U == UNDO_ID::STROKE_WIDTH) {
			wchar_t buf[32];
			float g_base;
			m_main_page.get_grid_base(g_base);
			conv_len_to_str<LEN_UNIT_NAME_APPEND>(
				m_len_unit, val, m_main_d2d.m_logical_dpi, g_base + 1.0f, buf);
			dialog_set_slider_header<S>(
				ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " + buf);
		}
		else if constexpr (U == UNDO_ID::STROKE_COLOR) {
			constexpr wchar_t* R[]{
				L"str_color_r", L"str_color_g", L"str_color_b", L"str_opacity"
			};
			wchar_t buf[32];
			conv_col_to_str(m_color_code, val, buf);
			dialog_set_slider_header<S>(
				ResourceLoader::GetForCurrentView().GetString(R[S]) + L": " + buf);
		}
	}

	// スライダーの値が変更された.
	// U	操作の識別子
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_ID U, int S>
	void MainPage::stroke_slider_value_changed(
		IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_ID::STROKE_WIDTH && S == 0) {
			const float val = static_cast<float>(args.NewValue());
			stroke_slider_set_header<UNDO_ID::STROKE_WIDTH, 0>(val);
			if (m_dialog_page.m_shape_list.back()->set_stroke_width(val)) {
				dialog_draw();
			}
		}
		else if constexpr (U == UNDO_ID::STROKE_COLOR && S == 0) {
			const float val = static_cast<float>(args.NewValue());
			D2D1_COLOR_F color;
			m_dialog_page.m_shape_list.back()->get_stroke_color(color);
			stroke_slider_set_header<U, S>(val);
			color.r = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_dialog_page.m_shape_list.back()->set_stroke_color(color)) {
				dialog_draw();
			}
		}
		else if constexpr (U == UNDO_ID::STROKE_COLOR && S == 1) {
			const float val = static_cast<float>(args.NewValue());
			D2D1_COLOR_F color;
			m_dialog_page.m_shape_list.back()->get_stroke_color(color);
			stroke_slider_set_header<U, S>(val);
			color.g = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_dialog_page.m_shape_list.back()->set_stroke_color(color)) {
				dialog_draw();
			}
		}
		else if constexpr (U == UNDO_ID::STROKE_COLOR && S == 2) {
			const float val = static_cast<float>(args.NewValue());
			D2D1_COLOR_F color;
			m_dialog_page.m_shape_list.back()->get_stroke_color(color);
			stroke_slider_set_header<U, S>(val);
			color.b = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_dialog_page.m_shape_list.back()->set_stroke_color(color)) {
				dialog_draw();
			}
		}
		else if constexpr (U == UNDO_ID::STROKE_COLOR && S == 3) {
			const float val = static_cast<float>(args.NewValue());
			D2D1_COLOR_F color;
			m_dialog_page.m_shape_list.back()->get_stroke_color(color);
			stroke_slider_set_header<U, S>(val);
			color.a = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_dialog_page.m_shape_list.back()->set_stroke_color(color)) {
				dialog_draw();
			}
		}
	}

	// 線枠メニューの「太さ」のサブ項目が選択された.
	void MainPage::stroke_width_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		float s_width;
		if (sender == rmfi_stroke_width_0px()) {
			s_width = 0.0f;
		}
		else if (sender == rmfi_stroke_width_1px()) {
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
			winrt::hresult_not_implemented();
			return;
		}
		stroke_width_is_checked(s_width);
		if (ustack_push_set<UNDO_ID::STROKE_WIDTH>(s_width)) {
			ustack_push_null();
			xcvd_is_enabled();
			page_draw();
		}
		status_bar_set_pos();
	}

	void MainPage::stroke_width_is_checked(const float s_width) noexcept
	{
		rmfi_stroke_width_0px().IsChecked(s_width == 0.0f);
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

		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_dialog_page.set_attr_to(&m_main_page);
		float s_width;
		m_dialog_page.get_stroke_width(s_width);
		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().StepFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::StepValues);
		dialog_slider_0().Value(s_width);
		stroke_slider_set_header<UNDO_ID::STROKE_WIDTH, 0>(s_width);
		dialog_slider_0().Visibility(Visibility::Visible);
		const auto token0{
			dialog_slider_0().ValueChanged(
				{ this, &MainPage::stroke_slider_value_changed<UNDO_ID::STROKE_WIDTH, 0> })
		};
		stroke_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), m_dialog_page);

		cd_setting_dialog().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width")));
		m_mutex_event.lock();
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float samp_val;
			m_dialog_page.m_shape_list.back()->get_stroke_width(samp_val);
			stroke_width_is_checked(samp_val);
			if (ustack_push_set<UNDO_ID::STROKE_WIDTH>(samp_val)) {
				ustack_push_null();
				xcvd_is_enabled();
				page_draw();
			}
		}
		slist_clear(m_dialog_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_0().ValueChanged(token0);
		dialog_slider_0().StepFrequency(1.0);
		dialog_slider_0().Maximum(255.0);
		m_mutex_event.unlock();

	}

}