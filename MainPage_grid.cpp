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
	// レイアウトメニューの「方眼の強調」が選択された.
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
			ustack_push_null();
			ustack_is_enable();
			main_draw();
		}
		status_bar_set_pos();
	}

	// レイアウトメニューの「方眼の強調」に印をつける.
	// g_emph	方眼の強調
	void MainPage::grid_emph_is_checked(const GRID_EMPH& g_emph)
	{
		rmfi_grid_emph_1().IsChecked(g_emph.m_gauge_1 == 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_2().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_3().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 != 0);
	}

	// レイアウトメニューの「方眼の大きさ」>「大きさ」が選択された.
	IAsyncAction MainPage::grid_len_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		const auto str_grid_length{ ResourceLoader::GetForCurrentView().GetString(L"str_grid_length") + L": " };
		const auto str_title{ ResourceLoader::GetForCurrentView().GetString(L"str_grid_length") };
		m_prop_page.set_attr_to(&m_main_page);
		const auto dpi = m_prop_d2d.m_logical_dpi;
		const auto g_len = m_prop_page.m_grid_base + 1.0;
		float g_base;
		m_prop_page.get_grid_base(g_base);
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(m_len_unit, g_base + 1.0f, dpi, g_len, buf);

		dialog_slider_0().Minimum(0.0);
		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(g_base);
		dialog_slider_0().Header(box_value(str_grid_length + buf));
		dialog_slider_0().Visibility(Visibility::Visible);

		dialog_combo_box().Items().Append(box_value(rmfi_len_unit_pixel().Text()));
		dialog_combo_box().Items().Append(box_value(rmfi_len_unit_inch().Text()));
		dialog_combo_box().Items().Append(box_value(rmfi_len_unit_milli().Text()));
		dialog_combo_box().Items().Append(box_value(rmfi_len_unit_point().Text()));
		dialog_combo_box().Items().Append(box_value(rmfi_len_unit_grid().Text()));
		if (m_len_unit == LEN_UNIT::PIXEL) {
			dialog_combo_box().SelectedIndex(0);
		}
		else if (m_len_unit == LEN_UNIT::INCH) {
			dialog_combo_box().SelectedIndex(1);
		}
		else if (m_len_unit == LEN_UNIT::MILLI) {
			dialog_combo_box().SelectedIndex(2);
		}
		else if (m_len_unit == LEN_UNIT::POINT) {
			dialog_combo_box().SelectedIndex(3);
		}
		else if (m_len_unit == LEN_UNIT::GRID) {
			dialog_combo_box().SelectedIndex(4);
		}
		dialog_combo_box().Visibility(Visibility::Visible);

		cd_setting_dialog().Title(box_value(str_title));
		m_mutex_event.lock();
		{
			const auto revoker0{
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_grid_length](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_main_page.m_grid_base + 1.0f;	// <---
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val + 1.0f, dpi, g_len, buf);
					dialog_slider_0().Header(box_value(str_grid_length + buf));
					if (m_prop_page.set_grid_base(val)) {
						prop_dialog_draw();
					}
				})
			};
			const auto revoker1{
				dialog_combo_box().SelectionChanged(winrt::auto_revoke, [this, str_grid_length](IInspectable const&, SelectionChangedEventArgs const&) {
					if (dialog_combo_box().SelectedIndex() == 0) {
						if (m_len_unit != LEN_UNIT::PIXEL) {
							m_len_unit = LEN_UNIT::PIXEL;
							const auto unit = m_len_unit;
							const auto dpi = m_prop_d2d.m_logical_dpi;
							const auto g_len = m_main_page.m_grid_base + 1.0f;	// <---
							const auto val = m_prop_page.m_grid_base + 1.0f;
							wchar_t buf[32];
							conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
							dialog_slider_0().Header(box_value(str_grid_length + buf));
						}
					}
					else if (dialog_combo_box().SelectedIndex() == 1) {
						if (m_len_unit != LEN_UNIT::INCH) {
							m_len_unit = LEN_UNIT::INCH;
							const auto unit = m_len_unit;
							const auto dpi = m_prop_d2d.m_logical_dpi;
							const auto g_len = m_main_page.m_grid_base + 1.0f;	// <---
							const auto val = m_prop_page.m_grid_base + 1.0f;
							wchar_t buf[32];
							conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
							dialog_slider_0().Header(box_value(str_grid_length + buf));
						}
					}
					else if (dialog_combo_box().SelectedIndex() == 2) {
						if (m_len_unit != LEN_UNIT::MILLI) {
							m_len_unit = LEN_UNIT::MILLI;
							const auto unit = m_len_unit;
							const auto dpi = m_prop_d2d.m_logical_dpi;
							const auto g_len = m_main_page.m_grid_base + 1.0f;	// <---
							const auto val = m_prop_page.m_grid_base + 1.0f;
							wchar_t buf[32];
							conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
							dialog_slider_0().Header(box_value(str_grid_length + buf));
						}
					}
					else if (dialog_combo_box().SelectedIndex() == 3) {
						if (m_len_unit != LEN_UNIT::POINT) {
							m_len_unit = LEN_UNIT::POINT;
							const auto unit = m_len_unit;
							const auto dpi = m_prop_d2d.m_logical_dpi;
							const auto g_len = m_main_page.m_grid_base + 1.0f;	// <---
							const auto val = m_prop_page.m_grid_base + 1.0f;
							wchar_t buf[32];
							conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
							dialog_slider_0().Header(box_value(str_grid_length + buf));
						}
					}
					else if (dialog_combo_box().SelectedIndex() == 4) {
						if (m_len_unit != LEN_UNIT::GRID) {
							m_len_unit = LEN_UNIT::GRID;
							const auto unit = m_len_unit;
							const auto dpi = m_prop_d2d.m_logical_dpi;
							const auto g_len = m_main_page.m_grid_base + 1.0f;	// <---
							const auto val = m_prop_page.m_grid_base + 1.0f;
							wchar_t buf[32];
							conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val , dpi, g_len, buf);
							dialog_slider_0().Header(box_value(str_grid_length + buf));
						}
					}

				})
			};
			if (co_await cd_setting_dialog().ShowAsync() == ContentDialogResult::Primary) {
				float setting_val;
				float page_val;

				m_main_page.get_grid_base(page_val);
				m_prop_page.get_grid_base(setting_val);
				if (!equal(page_val, setting_val)) {
					ustack_push_set<UNDO_T::GRID_BASE>(&m_main_page, setting_val);
					ustack_push_null();
					ustack_is_enable();
					xcvd_is_enabled();
					main_draw();
				}

			}
		}
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_combo_box().Items().Clear();
		dialog_combo_box().Visibility(Visibility::Collapsed);
		status_bar_set_pos();
		m_mutex_event.unlock();
	}

	// レイアウトメニューの「方眼の大きさ」>「狭める」が選択された.
	void MainPage::grid_len_con_click(IInspectable const&, RoutedEventArgs const&)
	{
		float g_base;
		m_main_page.get_grid_base(g_base);
		const float val = (g_base + 1.0f) * 0.5f - 1.0f;
		if (val >= 1.0f) {
			ustack_push_set<UNDO_T::GRID_BASE>(&m_main_page, val);
			ustack_push_null();
			ustack_is_enable();
			main_draw();
		}
		status_bar_set_pos();
	}

	// レイアウトメニューの「方眼の大きさ」>「広げる」が選択された.
	void MainPage::grid_len_exp_click(IInspectable const&, RoutedEventArgs const&)
	{
		float g_base;
		m_main_page.get_grid_base(g_base);
		const float val = (g_base + 1.0f) * 2.0f - 1.0f;
		if (val <= max(m_main_page.m_page_size.width, m_main_page.m_page_size.height)) {
			ustack_push_set<UNDO_T::GRID_BASE>(&m_main_page, val);
			ustack_push_null();
			ustack_is_enable();
			main_draw();
		}
		status_bar_set_pos();
	}

	// レイアウトメニューの「方眼の表示」>「最背面」が選択された.
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
			ustack_push_null();
			ustack_is_enable();
			main_draw();
		}
		status_bar_set_pos();
	}

	// レイアウトメニューの「方眼の表示」に印をつける.
	// g_show	方眼の表示
	void MainPage::grid_show_is_checked(const GRID_SHOW g_show)
	{
		rmfi_grid_show_back().IsChecked(g_show == GRID_SHOW::BACK);
		rmfi_grid_show_front().IsChecked(g_show == GRID_SHOW::FRONT);
		rmfi_grid_show_hide().IsChecked(g_show == GRID_SHOW::HIDE);
	}

}