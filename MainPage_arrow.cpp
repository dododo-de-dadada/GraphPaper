//-------------------------------
// MainPage_arrow.cpp
// 矢じるしの形式と寸法
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::UI::Xaml::Controls::ComboBoxItem;
	using winrt::Windows::UI::Xaml::Controls::Slider;

	//------------------------------
	// 線枠メニューの「矢じるしの寸法...」が選択された.
	//------------------------------
	IAsyncAction MainPage::arrow_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		const winrt::hstring str_arrow_width{ ResourceLoader::GetForCurrentView().GetString(L"str_arrow_width") + L": " };
		const winrt::hstring str_arrow_length{ ResourceLoader::GetForCurrentView().GetString(L"str_arrow_length") + L": " };
		const winrt::hstring str_arrow_offset{ ResourceLoader::GetForCurrentView().GetString(L"str_arrow_offset") + L": " };
		const winrt::hstring str_title{ ResourceLoader::GetForCurrentView().GetString(L"str_arrow_size") };

		m_mutex_event.lock();
		ARROW_SIZE a_size;
		ARROW_STYLE a_style;
		m_prop_page.set_attr_to(&m_main_page);
		m_prop_page.get_arrow_size(a_size);
		m_prop_page.get_arrow_style(a_style);

		const auto max0 = dialog_slider_0().Maximum();
		const auto freq0 = dialog_slider_0().TickFrequency();
		const auto snap0 = dialog_slider_0().SnapsTo();
		const auto val0 = dialog_slider_0().Value();
		const auto vis0 = dialog_slider_0().Visibility();
		const auto max1 = dialog_slider_1().Maximum();
		const auto freq1 = dialog_slider_1().TickFrequency();
		const auto snap1 = dialog_slider_1().SnapsTo();
		const auto val1 = dialog_slider_1().Value();
		const auto vis1 = dialog_slider_1().Visibility();
		const auto max2 = dialog_slider_2().Maximum();
		const auto freq2 = dialog_slider_2().TickFrequency();
		const auto snap2 = dialog_slider_2().SnapsTo();
		const auto val2 = dialog_slider_2().Value();
		const auto vis2 = dialog_slider_2().Visibility();

		dialog_slider_0().Minimum(0.0);
		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(a_size.m_width);
		dialog_slider_0().Visibility(Visibility::Visible);

		dialog_slider_1().Minimum(0.0);
		dialog_slider_1().Maximum(MAX_VALUE);
		dialog_slider_1().TickFrequency(TICK_FREQ);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(a_size.m_length);
		dialog_slider_1().Visibility(Visibility::Visible);

		dialog_slider_2().Minimum(0.0);
		dialog_slider_2().Maximum(MAX_VALUE);
		dialog_slider_2().TickFrequency(TICK_FREQ);
		dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_2().Value(a_size.m_offset);
		dialog_slider_2().Visibility(Visibility::Visible);

		dialog_radio_btns().Header(box_value(mfsi_arrow_style().Text()));
		dialog_radio_btn_0().Content(box_value(rmfi_arrow_style_opened().Text()));
		dialog_radio_btn_1().Content(box_value(rmfi_arrow_style_filled().Text()));
		dialog_radio_btns().Visibility(Visibility::Visible);
		if (a_style == ARROW_STYLE::OPENED) {
			dialog_radio_btns().SelectedIndex(0);
		}
		else if (a_style == ARROW_STYLE::FILLED) {
			dialog_radio_btns().SelectedIndex(1);
		}

		const auto unit = m_len_unit;
		const auto dpi = m_prop_d2d.m_logical_dpi;
		const auto g_len = m_prop_page.m_grid_base + 1.0f;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, a_size.m_width, dpi, g_len, buf);
		dialog_slider_0().Header(box_value(str_arrow_width + buf));
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, a_size.m_length, dpi, g_len, buf);
		dialog_slider_1().Header(box_value(str_arrow_length + buf));
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, a_size.m_offset, dpi, g_len, buf);
		dialog_slider_2().Header(box_value(str_arrow_offset + buf));
		const auto samp_w = scp_prop_panel().Width();
		const auto samp_h = scp_prop_panel().Height();
		const auto mar = samp_w * 0.125;
		const D2D1_POINT_2F start{	// 始点
			static_cast<FLOAT>(mar), static_cast<FLOAT>(mar)
		};
		const D2D1_POINT_2F pos{	// 対角点の位置ベクトル
			static_cast<FLOAT>(samp_w - 2.0 * mar), static_cast<FLOAT>(samp_h - 2.0 * mar)
		};
		m_prop_page.m_shape_list.push_back(new ShapeLine(start, pos, &m_prop_page));
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif

		cd_setting_dialog().Title(box_value(str_title));
		{
			const auto revoker0{
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_arrow_width](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					ARROW_SIZE a_size;
					m_prop_page.m_shape_list.back()->get_arrow_size(a_size);
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_0().Header(box_value(str_arrow_width + buf));
					a_size.m_width = static_cast<FLOAT>(val);
					if (m_prop_page.m_shape_list.back()->set_arrow_size(a_size)) {
						prop_dialog_draw();
					}
				})
			};
			const auto revoker1{
				dialog_slider_1().ValueChanged(winrt::auto_revoke, [this, str_arrow_length](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					ARROW_SIZE a_size;
					m_prop_page.m_shape_list.back()->get_arrow_size(a_size);
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_1().Header(box_value(str_arrow_length + buf));
					a_size.m_length = static_cast<FLOAT>(val);
					if (m_prop_page.m_shape_list.back()->set_arrow_size(a_size)) {
						prop_dialog_draw();
					}
				})
			};
			const auto revoker2{
				dialog_slider_2().ValueChanged(winrt::auto_revoke, [this, str_arrow_offset](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					ARROW_SIZE a_size;
					m_prop_page.m_shape_list.back()->get_arrow_size(a_size);
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_2().Header(box_value(str_arrow_offset + buf));
					a_size.m_offset = static_cast<FLOAT>(val);
					if (m_prop_page.m_shape_list.back()->set_arrow_size(a_size)) {
						prop_dialog_draw();
					}
				})
			};
			const auto revoker3{
				dialog_radio_btns().SelectionChanged(winrt::auto_revoke, [this](IInspectable const&, SelectionChangedEventArgs const&) {
					if (dialog_radio_btns().SelectedIndex() == 0) {
						if (m_prop_page.m_shape_list.back()->set_arrow_style(ARROW_STYLE::OPENED)) {
							prop_dialog_draw();
						}
					}
					else if (dialog_radio_btns().SelectedIndex() == 1) {
						if (m_prop_page.m_shape_list.back()->set_arrow_style(ARROW_STYLE::FILLED)) {
							prop_dialog_draw();
						}
					}
				})
			};
			if (co_await cd_setting_dialog().ShowAsync() == ContentDialogResult::Primary) {
				ARROW_SIZE new_size;
				ARROW_STYLE new_style;
				m_prop_page.m_shape_list.back()->get_arrow_size(new_size);
				m_prop_page.m_shape_list.back()->get_arrow_style(new_style);
				arrow_style_is_checked(new_style);
				const bool flag_size = ustack_push_set<UNDO_T::ARROW_SIZE>(new_size);
				const bool flag_style = ustack_push_set<UNDO_T::ARROW_STYLE>(new_style);
				if (flag_size || flag_style) {
					ustack_push_null();
					xcvd_is_enabled();
					main_draw();
				}
			}
		}

		dialog_slider_0().Maximum(max0);
		dialog_slider_0().TickFrequency(freq0);
		dialog_slider_0().SnapsTo(snap0);
		dialog_slider_0().Value(val0);
		dialog_slider_0().Visibility(vis0);
		dialog_slider_1().Maximum(max1);
		dialog_slider_1().TickFrequency(freq1);
		dialog_slider_1().SnapsTo(snap1);
		dialog_slider_1().Value(val1);
		dialog_slider_1().Visibility(vis1);
		dialog_slider_2().Maximum(max2);
		dialog_slider_2().TickFrequency(freq2);
		dialog_slider_2().SnapsTo(snap2);
		dialog_slider_2().Value(val2);
		dialog_slider_2().Visibility(vis2);
		dialog_radio_btns().Visibility(Visibility::Collapsed);
		//dialog_combo_box().Visibility(Visibility::Collapsed);
		//dialog_combo_box().Items().Clear();
		slist_clear(m_prop_page.m_shape_list);
		status_bar_set_pos();
		m_mutex_event.unlock();
	}

	//------------------------------
	// 線枠メニューの「矢じるしの形式」のサブ項目が選択された.
	//------------------------------
	void MainPage::arrow_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		ARROW_STYLE a_style;
		if (sender == rmfi_arrow_style_none()) {
			a_style = ARROW_STYLE::NONE;
		}
		else if (sender == rmfi_arrow_style_opened()) {
			a_style = ARROW_STYLE::OPENED;
		}
		else if (sender == rmfi_arrow_style_filled()) {
			a_style = ARROW_STYLE::FILLED;
		}
		else {
			return;
		}
		arrow_style_is_checked(a_style);
		if (ustack_push_set<UNDO_T::ARROW_STYLE>(a_style)) {
			ustack_push_null();
			xcvd_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	//------------------------------
	// 線枠メニューの「矢じるしの形式」に印をつける.
	// a_style	矢じるしの形式
	//------------------------------
	void MainPage::arrow_style_is_checked(const ARROW_STYLE val)
	{
		rmfi_arrow_style_none().IsChecked(val == ARROW_STYLE::NONE);
		rmfi_arrow_style_opened().IsChecked(val == ARROW_STYLE::OPENED);
		rmfi_arrow_style_filled().IsChecked(val == ARROW_STYLE::FILLED);
		mfi_arrow_size().IsEnabled(val != ARROW_STYLE::NONE);
	}

}