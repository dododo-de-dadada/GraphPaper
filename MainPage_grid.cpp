//-------------------------------
// MainPage_grid.cpp
// 方眼
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::ToggleMenuFlyoutItem;

	//constexpr float SLIDER_STEP = 0.5f;
	constexpr wchar_t TITLE_GRID[] = L"str_grid";

	// 用紙メニューの「方眼の強調」が選択された.
	void MainPage::grid_emph_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		GRID_EMPH value;
		if (sender == rmfi_grid_emph_1() || sender == rmfi_grid_emph_1_2()) {
			value = GRID_EMPH_0;
		}
		else if (sender == rmfi_grid_emph_2() || sender == rmfi_grid_emph_2_2()) {
			value = GRID_EMPH_2;
		}
		else if (sender == rmfi_grid_emph_3() || sender == rmfi_grid_emph_3_2()) {
			value = GRID_EMPH_3;
		}
		else {
			return;
		}
		GRID_EMPH g_emph;
		m_main_sheet.get_grid_emph(g_emph);
		if (!equal(g_emph, value)) {
			ustack_push_set<UNDO_OP::GRID_EMPH>(&m_main_sheet, value);
			ustack_is_enable();
			sheet_draw();
		}
	}

	// 用紙メニューの「方眼の強調」に印をつける.
	// g_emph	方眼の強調
	void MainPage::grid_emph_is_checked(const GRID_EMPH& g_emph)
	{
		rmfi_grid_emph_1().IsChecked(g_emph.m_gauge_1 == 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_2().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_3().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 != 0);

		rmfi_grid_emph_1_2().IsChecked(g_emph.m_gauge_1 == 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_2_2().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_3_2().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 != 0);
	}

	// 用紙メニューの「方眼の色」が選択された.
	IAsyncAction MainPage::grid_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		m_sample_sheet.set_attr_to(&m_main_sheet);
		const auto val0 = m_sample_sheet.m_grid_color.r * COLOR_MAX;
		const auto val1 = m_sample_sheet.m_grid_color.g * COLOR_MAX;
		const auto val2 = m_sample_sheet.m_grid_color.b * COLOR_MAX;
		const auto val3 = m_sample_sheet.m_grid_color.a * COLOR_MAX;

		sample_slider_0().Maximum(255.0);
		sample_slider_0().TickFrequency(1.0);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(val0);
		grid_slider_set_header<UNDO_OP::GRID_COLOR, 0>(val0);
		sample_slider_1().Maximum(255.0);
		sample_slider_1().TickFrequency(1.0);
		sample_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_1().Value(val1);
		grid_slider_set_header<UNDO_OP::GRID_COLOR, 1>(val1);
		sample_slider_2().Maximum(255.0);
		sample_slider_2().TickFrequency(1.0);
		sample_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_2().Value(val2);
		grid_slider_set_header<UNDO_OP::GRID_COLOR, 2>(val2);
		sample_slider_3().Maximum(255.0);
		sample_slider_3().TickFrequency(1.0);
		sample_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_3().Value(val3);
		grid_slider_set_header<UNDO_OP::GRID_COLOR, 3>(val3);

		sample_slider_0().Visibility(UI_VISIBLE);
		sample_slider_1().Visibility(UI_VISIBLE);
		sample_slider_2().Visibility(UI_VISIBLE);
		sample_slider_3().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::grid_slider_value_changed< UNDO_OP::GRID_COLOR, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::grid_slider_value_changed< UNDO_OP::GRID_COLOR, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::grid_slider_value_changed< UNDO_OP::GRID_COLOR, 2> });
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::grid_slider_value_changed< UNDO_OP::GRID_COLOR, 3> });
		m_sample_type = SAMPLE_TYPE::NONE;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_GRID)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			if (!equal(m_main_sheet.m_grid_color, m_sample_sheet.m_grid_color)) {
				ustack_push_set<UNDO_OP::GRID_COLOR>(&m_main_sheet, m_sample_sheet.m_grid_color);
				ustack_is_enable();
				sheet_draw();
			}
		}
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_1().Visibility(UI_COLLAPSED);
		sample_slider_2().Visibility(UI_COLLAPSED);
		sample_slider_3().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sample_slider_2().ValueChanged(slider_2_token);
		sample_slider_3().ValueChanged(slider_3_token);
	}

	// 用紙メニューの「方眼の大きさ」>「大きさ」が選択された.
	IAsyncAction MainPage::grid_len_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_sample_sheet.set_attr_to(&m_main_sheet);
		float g_base;
		m_sample_sheet.get_grid_base(g_base);

		sample_slider_0().Maximum(MAX_VALUE);
		sample_slider_0().TickFrequency(TICK_FREQ);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(g_base);
		grid_slider_set_header<UNDO_OP::GRID_BASE, 0>(g_base);
		sample_slider_0().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::grid_slider_value_changed<UNDO_OP::GRID_BASE, 0> });
		m_sample_type = SAMPLE_TYPE::NONE;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_GRID)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_value;
			float sheet_value;

			m_main_sheet.get_grid_base(sheet_value);
			m_sample_sheet.get_grid_base(sample_value);
			if (!equal(sheet_value, sample_value)) {
				ustack_push_set<UNDO_OP::GRID_BASE>(&m_main_sheet, sample_value);
				ustack_is_enable();
				xcvd_is_enabled();
				sheet_draw();
			}

		}
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
	}

	// 用紙メニューの「方眼の大きさ」>「狭める」が選択された.
	void MainPage::grid_len_con_click(IInspectable const&, RoutedEventArgs const&)
	{
		float g_base;
		m_main_sheet.get_grid_base(g_base);
		const float value = (g_base + 1.0f) * 0.5f - 1.0f;
		if (value >= 1.0f) {
			ustack_push_set<UNDO_OP::GRID_BASE>(&m_main_sheet, value);
			ustack_is_enable();
			sheet_draw();
		}
	}

	// 用紙メニューの「方眼の大きさ」>「広げる」が選択された.
	void MainPage::grid_len_exp_click(IInspectable const&, RoutedEventArgs const&)
	{
		float g_base;
		m_main_sheet.get_grid_base(g_base);
		const float value = (g_base + 1.0f) * 2.0f - 1.0f;
		if (value <= max(m_main_sheet.m_sheet_size.width, m_main_sheet.m_sheet_size.height)) {
			ustack_push_set<UNDO_OP::GRID_BASE>(&m_main_sheet, value);
			ustack_is_enable();
			sheet_draw();
		}
	}

	// 値をスライダーのヘッダーに格納する.
	// U	操作
	// S	スライダー
	// value	値
	template <UNDO_OP U, int S> void MainPage::grid_slider_set_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring text;

		if constexpr (U == UNDO_OP::GRID_BASE) {
			float g_base;
			m_main_sheet.get_grid_base(g_base);
			const float g_len = g_base + 1.0f;
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_misc_len_unit, value/* * SLIDER_STEP*/ + 1.0f, m_main_d2d.m_logical_dpi, g_len, buf);
			text = ResourceLoader::GetForCurrentView().GetString(L"str_grid_length") + L": " + buf;
		}
		if constexpr (U == UNDO_OP::GRID_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(m_misc_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_color_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(m_misc_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_color_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(m_misc_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_color_b") + L": " + buf;
			}
			if constexpr (S == 3) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
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
	template <UNDO_OP U, int S> void MainPage::grid_slider_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::GRID_BASE) {
			const float value = static_cast<float>(args.NewValue());
			grid_slider_set_header<U, S>(value);
			m_sample_sheet.set_grid_base(value/* * SLIDER_STEP*/);
		}
		else if constexpr (U == UNDO_OP::GRID_COLOR) {
			const float value = static_cast<float>(args.NewValue());
			grid_slider_set_header<U, S>(value);
			D2D1_COLOR_F g_color;
			m_sample_sheet.get_grid_color(g_color);
			if constexpr (S == 0) {
				g_color.r = value / COLOR_MAX;
			}
			else if constexpr (S == 1) {
				g_color.g = value / COLOR_MAX;
			}
			else if constexpr (S == 2) {
				g_color.b = value / COLOR_MAX;
			}
			else if constexpr (S == 3) {
				g_color.a = value / COLOR_MAX;
			}
			m_sample_sheet.set_grid_color(g_color);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// 用紙メニューの「方眼の表示」>「最背面」が選択された.
	void MainPage::grid_show_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		GRID_SHOW value;
		if (sender == rmfi_grid_show_back() || sender == rmfi_grid_show_back_2()) {
			value = GRID_SHOW::BACK;
		}
		else if (sender == rmfi_grid_show_front() || sender == rmfi_grid_show_front_2()) {
			value = GRID_SHOW::FRONT;
		}
		else if (sender == rmfi_grid_show_hide() || sender == rmfi_grid_show_hide_2()) {
			value = GRID_SHOW::HIDE;
		}
		else {
			return;
		}
		GRID_SHOW g_show;
		m_main_sheet.get_grid_show(g_show);
		if (g_show != value) {
			ustack_push_set<UNDO_OP::GRID_SHOW>(&m_main_sheet, value);
			ustack_is_enable();
			sheet_draw();
		}
	}

	// 用紙メニューの「方眼の表示」に印をつける.
	// g_show	方眼の表示
	void MainPage::grid_show_is_checked(const GRID_SHOW g_show)
	{
		rmfi_grid_show_back().IsChecked(g_show == GRID_SHOW::BACK);
		rmfi_grid_show_front().IsChecked(g_show == GRID_SHOW::FRONT);
		rmfi_grid_show_hide().IsChecked(g_show == GRID_SHOW::HIDE);
		rmfi_grid_show_back_2().IsChecked(g_show == GRID_SHOW::BACK);
		rmfi_grid_show_front_2().IsChecked(g_show == GRID_SHOW::FRONT);
		rmfi_grid_show_hide_2().IsChecked(g_show == GRID_SHOW::HIDE);
	}

	// 用紙メニューの「方眼に合わせる」が選択された.
	void MainPage::grid_snap_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		m_main_sheet.m_grid_snap = unbox_value<ToggleMenuFlyoutItem>(sender).IsChecked();
	}

}