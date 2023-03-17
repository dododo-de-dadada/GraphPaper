//-------------------------------
// MainPage_grid.cpp
// 方眼
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::UI::Xaml::Controls::ToggleMenuFlyoutItem;

	//constexpr float SLIDER_STEP = 0.5f;
	// 方眼メニューの「方眼の強調」が選択された.
	void MainPage::grid_emph_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		GRID_EMPH val;
		if (sender == rmfi_grid_emph_1()) {
			val = GRID_EMPH_0;
		}
		else if (sender == rmfi_grid_emph_2()) {
			val = GRID_EMPH_2;
		}
		else if (sender == rmfi_grid_emph_3()) {
			val = GRID_EMPH_3;
		}
		else {
			winrt::hresult_not_implemented();
		}
		grid_emph_is_checked(val);
		GRID_EMPH g_emph;
		m_main_page.get_grid_emph(g_emph);
		if (!equal(g_emph, val)) {
			ustack_push_set<UNDO_T::GRID_EMPH>(&m_main_page, val);
			ustack_is_enable();
			page_draw();
		}
		status_bar_set_pos();
	}

	// 方眼メニューの「方眼の強調」に印をつける.
	// g_emph	方眼の強調
	void MainPage::grid_emph_is_checked(const GRID_EMPH& g_emph)
	{
		rmfi_grid_emph_1().IsChecked(g_emph.m_gauge_1 == 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_2().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_3().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 != 0);
	}

	// 方眼メニューの「方眼の色」が選択された.
	IAsyncAction MainPage::grid_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_dialog_page.set_attr_to(&m_main_page);
		const auto val0 = static_cast<float>(conv_color_comp(m_dialog_page.m_grid_color.r));
		const auto val1 = static_cast<float>(conv_color_comp(m_dialog_page.m_grid_color.g));
		const auto val2 = static_cast<float>(conv_color_comp(m_dialog_page.m_grid_color.b));
		const auto val3 = static_cast<float>(conv_color_comp(m_dialog_page.m_grid_color.a));

		dialog_slider_0().Maximum(255.0);
		dialog_slider_0().TickFrequency(1.0);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val0);
		grid_slider_set_header<UNDO_T::GRID_COLOR, 0>(val0);
		dialog_slider_1().Maximum(255.0);
		dialog_slider_1().TickFrequency(1.0);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(val1);
		grid_slider_set_header<UNDO_T::GRID_COLOR, 1>(val1);
		dialog_slider_2().Maximum(255.0);
		dialog_slider_2().TickFrequency(1.0);
		dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_2().Value(val2);
		grid_slider_set_header<UNDO_T::GRID_COLOR, 2>(val2);
		dialog_slider_3().Maximum(255.0);
		dialog_slider_3().TickFrequency(1.0);
		dialog_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_3().Value(val3);
		grid_slider_set_header<UNDO_T::GRID_COLOR, 3>(val3);

		dialog_slider_0().Visibility(Visibility::Visible);
		dialog_slider_1().Visibility(Visibility::Visible);
		dialog_slider_2().Visibility(Visibility::Visible);
		dialog_slider_3().Visibility(Visibility::Visible);
		const auto token0{
			dialog_slider_0().ValueChanged(
				{ this, &MainPage::grid_slider_value_changed<UNDO_T::GRID_COLOR, 0> })
		};
		const auto token1{
			dialog_slider_1().ValueChanged(
				{ this, &MainPage::grid_slider_value_changed<UNDO_T::GRID_COLOR, 1>})
		};
		const auto token2{
			dialog_slider_2().ValueChanged(
				{ this, &MainPage::grid_slider_value_changed<UNDO_T::GRID_COLOR, 2> })
		};
		const auto token3{
			dialog_slider_3().ValueChanged(
				{ this, &MainPage::grid_slider_value_changed<UNDO_T::GRID_COLOR, 3> })
		};

		cd_setting_dialog().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(L"str_grid_color")));
		m_mutex_event.lock();
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			if (!equal(m_main_page.m_grid_color, m_dialog_page.m_grid_color)) {
				ustack_push_set<UNDO_T::GRID_COLOR>(&m_main_page, m_dialog_page.m_grid_color);
				ustack_is_enable();
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
		status_bar_set_pos();
		m_mutex_event.unlock();
	}

	// 方眼メニューの「方眼の大きさ」>「大きさ」が選択された.
	IAsyncAction MainPage::grid_len_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_dialog_page.set_attr_to(&m_main_page);
		float g_base;
		m_dialog_page.get_grid_base(g_base);

		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(g_base);
		grid_slider_set_header<UNDO_T::GRID_BASE, 0>(g_base);
		dialog_slider_0().Visibility(Visibility::Visible);
		const auto token0{
			dialog_slider_0().ValueChanged(
				{ this, &MainPage::grid_slider_value_changed<UNDO_T::GRID_BASE, 0> })
		};
		cd_setting_dialog().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(L"str_grid_length")));
		m_mutex_event.lock();
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float setting_val;
			float page_val;

			m_main_page.get_grid_base(page_val);
			m_dialog_page.get_grid_base(setting_val);
			if (!equal(page_val, setting_val)) {
				ustack_push_set<UNDO_T::GRID_BASE>(&m_main_page, setting_val);
				ustack_is_enable();
				xcvd_is_enabled();
				page_draw();
			}

		}
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_0().ValueChanged(token0);
		status_bar_set_pos();
		m_mutex_event.unlock();
	}

	// 方眼メニューの「方眼の大きさ」>「狭める」が選択された.
	void MainPage::grid_len_con_click(IInspectable const&, RoutedEventArgs const&)
	{
		float g_base;
		m_main_page.get_grid_base(g_base);
		const float val = (g_base + 1.0f) * 0.5f - 1.0f;
		if (val >= 1.0f) {
			ustack_push_set<UNDO_T::GRID_BASE>(&m_main_page, val);
			ustack_is_enable();
			page_draw();
		}
		status_bar_set_pos();
	}

	// 方眼メニューの「方眼の大きさ」>「広げる」が選択された.
	void MainPage::grid_len_exp_click(IInspectable const&, RoutedEventArgs const&)
	{
		float g_base;
		m_main_page.get_grid_base(g_base);
		const float val = (g_base + 1.0f) * 2.0f - 1.0f;
		if (val <= max(m_main_page.m_page_size.width, m_main_page.m_page_size.height)) {
			ustack_push_set<UNDO_T::GRID_BASE>(&m_main_page, val);
			ustack_is_enable();
			page_draw();
		}
		status_bar_set_pos();
	}

	// 値をスライダーのヘッダーに格納する.
	// U	操作の識別子
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <UNDO_T U, int S>
	void MainPage::grid_slider_set_header(const float val)
	{
		if constexpr (U == UNDO_T::GRID_BASE) {
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_NAME_APPEND>(
				m_len_unit, val + 1.0f, m_main_d2d.m_logical_dpi, val + 1.0, buf);
			dialog_set_slider_header<S>(
				ResourceLoader::GetForCurrentView().GetString(L"str_grid_length") + L": " + buf);
		}
		if constexpr (U == UNDO_T::GRID_COLOR) {
			constexpr wchar_t* HEADER[]{
				L"str_color_r", L"str_color_g",L"str_color_b", L"str_opacity"
			};
			wchar_t buf[32];
			conv_col_to_str(m_color_code, val, buf);
			dialog_set_slider_header<S>(
				ResourceLoader::GetForCurrentView().GetString(HEADER[S]) + L": " + buf);
		}
	}

	// スライダーの値が変更された.
	// U	操作の識別子
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_T U, int S>
	void MainPage::grid_slider_value_changed(
		IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_T::GRID_BASE) {
			const float val = static_cast<float>(args.NewValue());
			grid_slider_set_header<U, S>(val);
			if (m_dialog_page.set_grid_base(val)) {
				dialog_draw();
			}
		}
		else if constexpr (U == UNDO_T::GRID_COLOR && S == 0) {
			const float val = static_cast<float>(args.NewValue());
			grid_slider_set_header<U, S>(val);
			D2D1_COLOR_F g_color;
			m_dialog_page.get_grid_color(g_color);
			g_color.r = val / COLOR_MAX;
			if (m_dialog_page.set_grid_color(g_color)) {
				dialog_draw();
			}
		}
		else if constexpr (U == UNDO_T::GRID_COLOR && S == 1) {
			const float val = static_cast<float>(args.NewValue());
			grid_slider_set_header<U, S>(val);
			D2D1_COLOR_F g_color;
			m_dialog_page.get_grid_color(g_color);
			g_color.g = val / COLOR_MAX;
			if (m_dialog_page.set_grid_color(g_color)) {
				dialog_draw();
			}
		}
		else if constexpr (U == UNDO_T::GRID_COLOR && S == 2) {
			const float val = static_cast<float>(args.NewValue());
			grid_slider_set_header<U, S>(val);
			D2D1_COLOR_F g_color;
			m_dialog_page.get_grid_color(g_color);
			g_color.b = val / COLOR_MAX;
			if (m_dialog_page.set_grid_color(g_color)) {
				dialog_draw();
			}
		}
		else if constexpr (U == UNDO_T::GRID_COLOR && S == 3) {
			const float val = static_cast<float>(args.NewValue());
			grid_slider_set_header<U, S>(val);
			D2D1_COLOR_F g_color;
			m_dialog_page.get_grid_color(g_color);
			g_color.a = val / COLOR_MAX;
			if (m_dialog_page.set_grid_color(g_color)) {
				dialog_draw();
			}
		}
	}

	// 方眼メニューの「方眼の表示」>「最背面」が選択された.
	void MainPage::grid_show_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		GRID_SHOW new_val;
		if (sender == rmfi_grid_show_back()) {
			new_val = GRID_SHOW::BACK;
		}
		else if (sender == rmfi_grid_show_front()) {
			new_val = GRID_SHOW::FRONT;
		}
		else if (sender == rmfi_grid_show_hide()) {
			new_val = GRID_SHOW::HIDE;
		}
		else {
			winrt::hresult_not_implemented();
			return;
		}
		grid_show_is_checked(new_val);
		if (m_main_page.m_grid_show != new_val) {
			ustack_push_set<UNDO_T::GRID_SHOW>(&m_main_page, new_val);
			ustack_is_enable();
			page_draw();
		}
		status_bar_set_pos();
	}

	// 方眼メニューの「方眼の表示」に印をつける.
	// g_show	方眼の表示
	void MainPage::grid_show_is_checked(const GRID_SHOW g_show)
	{
		rmfi_grid_show_back().IsChecked(g_show == GRID_SHOW::BACK);
		rmfi_grid_show_front().IsChecked(g_show == GRID_SHOW::FRONT);
		rmfi_grid_show_hide().IsChecked(g_show == GRID_SHOW::HIDE);
		//rmfi_grid_show_back_2().IsChecked(g_show == GRID_SHOW::BACK);
		//rmfi_grid_show_front_2().IsChecked(g_show == GRID_SHOW::FRONT);
		//rmfi_grid_show_hide_2().IsChecked(g_show == GRID_SHOW::HIDE);
	}

	// 方眼メニューの「方眼に合わせる」が選択された.
	void MainPage::grid_snap_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		m_main_page.m_grid_snap = unbox_value<ToggleMenuFlyoutItem>(sender).IsChecked();
		status_bar_set_pos();
	}

}