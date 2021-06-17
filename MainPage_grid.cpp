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

	constexpr float SLIDER_STEP = 0.5f;
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
		m_sheet_main.get_grid_emph(g_emph);
		if (!equal(g_emph, value)) {
			undo_push_set<UNDO_OP::GRID_EMPH>(&m_sheet_main, value);
			undo_menu_enable();
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

	// 用紙メニューの「方眼の濃さ」が選択された.
	IAsyncAction MainPage::grid_gray_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		float value;
		m_sample_sheet.get_grid_gray(value);
		const float val3 = value * COLOR_MAX;
		sample_slider_3().Value(val3);
		grid_slider_set_header<UNDO_OP::GRID_GRAY, 3>(val3);
		sample_slider_3().Visibility(UI_VISIBLE);
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::grid_slider_value_changed< UNDO_OP::GRID_GRAY, 3> });
		m_sample_type = SAMPLE_TYPE::NONE;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_GRID)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_value;
			m_sample_sheet.get_grid_gray(sample_value);
			float sheet_value;
			m_sheet_main.get_grid_gray(sheet_value);
			if (!equal(sheet_value, sample_value)) {
				undo_push_set<UNDO_OP::GRID_GRAY>(&m_sheet_main, sample_value);
				undo_menu_enable();
				sheet_draw();
			}
		}
		sample_slider_3().Visibility(UI_COLLAPSED);
		sample_slider_3().ValueChanged(slider_3_token);
	}

	// 用紙メニューの「方眼の大きさ」>「大きさ」が選択された.
	IAsyncAction MainPage::grid_len_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		float value;
		m_sample_sheet.get_grid_base(value);
		const float val0 = value / SLIDER_STEP;
		sample_slider_0().Value(val0);
		grid_slider_set_header<UNDO_OP::GRID_BASE, 0>(val0);
		sample_slider_0().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::grid_slider_value_changed<UNDO_OP::GRID_BASE, 0> });
		m_sample_type = SAMPLE_TYPE::NONE;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_GRID)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_value;
			float sheet_value;

			m_sheet_main.get_grid_base(sheet_value);
			m_sample_sheet.get_grid_base(sample_value);
			if (!equal(sheet_value, sample_value)) {
				undo_push_set<UNDO_OP::GRID_BASE>(&m_sheet_main, sample_value);
				undo_menu_enable();
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
		m_sheet_main.get_grid_base(g_base);
		const float value = (g_base + 1.0f) * 0.5f - 1.0f;
		if (value >= 1.0f) {
			undo_push_set<UNDO_OP::GRID_BASE>(&m_sheet_main, value);
			undo_menu_enable();
			sheet_draw();
		}
	}

	// 用紙メニューの「方眼の大きさ」>「広げる」が選択された.
	void MainPage::grid_len_exp_click(IInspectable const&, RoutedEventArgs const&)
	{
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		const float value = (g_base + 1.0f) * 2.0f - 1.0f;
		if (value <= max(m_sheet_main.m_sheet_size.width, m_sheet_main.m_sheet_size.height)) {
			undo_push_set<UNDO_OP::GRID_BASE>(&m_sheet_main, value);
			undo_menu_enable();
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
			m_sheet_main.get_grid_base(g_base);
			const float g_len = g_base + 1.0f;
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_misc_len_unit, value * SLIDER_STEP + 1.0f, m_sheet_dx.m_logical_dpi, g_len, buf);
			text = ResourceLoader::GetForCurrentView().GetString(L"str_grid_length") + L": " + buf;
		}
		if constexpr (U == UNDO_OP::GRID_GRAY) {
			if constexpr (S == 3) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(m_misc_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_gray_scale") + L": " + buf;
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
		Shape* const s = &m_sample_sheet;
		const float value = static_cast<float>(args.NewValue());

		grid_slider_set_header<U, S>(value);
		if constexpr (U == UNDO_OP::GRID_BASE) {
			s->set_grid_base(value * SLIDER_STEP);
		}
		if constexpr (U == UNDO_OP::GRID_GRAY) {
			s->set_grid_gray(value / COLOR_MAX);
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
		m_sheet_main.get_grid_show(g_show);
		if (g_show != value) {
			undo_push_set<UNDO_OP::GRID_SHOW>(&m_sheet_main, value);
			undo_menu_enable();
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
		m_sheet_main.m_grid_snap = unbox_value<ToggleMenuFlyoutItem>(sender).IsChecked();
	}

}