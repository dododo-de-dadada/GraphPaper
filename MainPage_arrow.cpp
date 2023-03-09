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

	void MainPage::arrow_selection_changed(IInspectable const&, SelectionChangedEventArgs const& args) noexcept
	{
		if (dialog_combo_box().SelectedIndex() == 0) {
			if (m_dialog_page.m_shape_list.back()->set_arrow_style(ARROW_STYLE::OPENED)) {
				dialog_draw();
			}
		}
		else if (dialog_combo_box().SelectedIndex() == 1) {
			if (m_dialog_page.m_shape_list.back()->set_arrow_style(ARROW_STYLE::FILLED)) {
				dialog_draw();
			}
		}
	}

	//------------------------------
	// 値をスライダーのヘッダーに格納する.
	// U	操作の識別子
	// S	スライダーの番号
	// val	格納する値
	//------------------------------
	template <int S>
	void MainPage::arrow_slider_set_header(const float val)
	{
		constexpr wchar_t* SLIDER_HEADER[] = {
			L"str_arrow_width",
			L"str_arrow_length",
			L"str_arrow_offset"
		};
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(
			m_len_unit, val, m_main_d2d.m_logical_dpi, m_main_page.m_grid_base + 1.0f, buf);
		const winrt::hstring text{
			ResourceLoader::GetForCurrentView().GetString(SLIDER_HEADER[S]) + L": " + buf
		};
		dialog_set_slider_header<S>(text);
	}

	//------------------------------
	// スライダーの値が変更された.
	// U	操作の識別子
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	//------------------------------
	template <int S>
	void MainPage::arrow_slider_val_changed(
		IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (S == 0) {
			const float val = static_cast<float>(args.NewValue());
			ARROW_SIZE a_size;
			m_dialog_page.m_shape_list.back()->get_arrow_size(a_size);
			arrow_slider_set_header<0>(val);
			a_size.m_width = static_cast<FLOAT>(val);
			if (m_dialog_page.m_shape_list.back()->set_arrow_size(a_size)) {
				dialog_draw();
			}
		}
		else if constexpr (S == 1) {
			const float val = static_cast<float>(args.NewValue());
			ARROW_SIZE a_size;
			m_dialog_page.m_shape_list.back()->get_arrow_size(a_size);
			arrow_slider_set_header<1>(val);
			a_size.m_length = static_cast<FLOAT>(val);
			if (m_dialog_page.m_shape_list.back()->set_arrow_size(a_size)) {
				dialog_draw();
			}
		}
		else if constexpr (S == 2) {
			const float val = static_cast<float>(args.NewValue());
			ARROW_SIZE a_size;
			m_dialog_page.m_shape_list.back()->get_arrow_size(a_size);
			arrow_slider_set_header<2>(val);
			a_size.m_offset = static_cast<FLOAT>(val);
			if (m_dialog_page.m_shape_list.back()->set_arrow_size(a_size)) {
				dialog_draw();
			}
		}
	}

	//------------------------------
	// 線枠メニューの「矢じるしの寸法...」が選択された.
	//------------------------------
	IAsyncAction MainPage::arrow_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;

		m_mutex_event.lock();
		ARROW_SIZE a_size;
		ARROW_STYLE a_style;
		m_dialog_page.set_attr_to(&m_main_page);
		m_dialog_page.get_arrow_size(a_size);
		m_dialog_page.get_arrow_style(a_style);

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
		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(a_size.m_width);
		dialog_slider_0().Visibility(Visibility::Visible);
		dialog_slider_1().Maximum(MAX_VALUE);
		dialog_slider_1().TickFrequency(TICK_FREQ);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(a_size.m_length);
		dialog_slider_1().Visibility(Visibility::Visible);
		dialog_slider_2().Maximum(MAX_VALUE);
		dialog_slider_2().TickFrequency(TICK_FREQ);
		dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_2().Value(a_size.m_offset);
		dialog_slider_2().Visibility(Visibility::Visible);
		dialog_combo_box().Header(box_value(mfsi_arrow_style().Text()));
		dialog_combo_box().Items().Append(box_value(rmfi_arrow_style_opened().Text()));
		dialog_combo_box().Items().Append(box_value(rmfi_arrow_style_filled().Text()));
		dialog_combo_box().Visibility(Visibility::Visible);
		if (a_style == ARROW_STYLE::OPENED) {
			dialog_combo_box().SelectedIndex(0);
		}
		else if (a_style == ARROW_STYLE::FILLED) {
			dialog_combo_box().SelectedIndex(1);
		}
		const winrt::event_token token0{
			dialog_slider_0().ValueChanged({ this, &MainPage::arrow_slider_val_changed<0> })
		};
		const winrt::event_token token1{
			dialog_slider_1().ValueChanged({ this, &MainPage::arrow_slider_val_changed<1> })
		};
		const winrt::event_token token2{
			dialog_slider_2().ValueChanged({ this, &MainPage::arrow_slider_val_changed<2> })
		};
		const winrt::event_token token3{
			dialog_combo_box().SelectionChanged({ this, &MainPage::arrow_selection_changed })
		};
		arrow_slider_set_header<0>(a_size.m_width);
		arrow_slider_set_header<1>(a_size.m_length);
		arrow_slider_set_header<2>(a_size.m_offset);
		const auto samp_w = scp_dialog_panel().Width();
		const auto samp_h = scp_dialog_panel().Height();
		const auto padd = samp_w * 0.125;
		const D2D1_POINT_2F start{	// 始点
			static_cast<FLOAT>(padd), static_cast<FLOAT>(padd)
		};
		const D2D1_POINT_2F pos{	// 対角点の位置ベクトル
			static_cast<FLOAT>(samp_w - 2.0 * padd), static_cast<FLOAT>(samp_h - 2.0 * padd)
		};
		m_dialog_page.m_shape_list.push_back(new ShapeLine(start, pos, &m_dialog_page));
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif

		cd_setting_dialog().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(L"str_arrow_size")));
		const ContentDialogResult d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			ARROW_SIZE new_size;
			ARROW_STYLE new_style;
			m_dialog_page.m_shape_list.back()->get_arrow_size(new_size);
			m_dialog_page.m_shape_list.back()->get_arrow_style(new_style);
			arrow_style_is_checked(new_style);
			const bool flag_size = ustack_push_set<UNDO_ID::ARROW_SIZE>(new_size);
			const bool flag_style = ustack_push_set<UNDO_ID::ARROW_STYLE>(new_style);
			if (flag_size || flag_style) {
				ustack_push_null();
				xcvd_is_enabled();
				page_draw();
			}
		}
		dialog_slider_0().ValueChanged(token0);
		dialog_slider_1().ValueChanged(token1);
		dialog_slider_2().ValueChanged(token2);
		dialog_combo_box().SelectionChanged(token3);
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
		dialog_combo_box().Visibility(Visibility::Collapsed);
		dialog_combo_box().Items().Clear();
		slist_clear(m_dialog_page.m_shape_list);
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
		if (ustack_push_set<UNDO_ID::ARROW_STYLE>(a_style)) {
			ustack_push_null();
			xcvd_is_enabled();
			page_draw();
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