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

	constexpr wchar_t TITLE_ARROWHEAD[] = L"str_arrow_size";

	//------------------------------
	// 値をスライダーのヘッダーに格納する.
	// U	操作の識別子
	// S	スライダーの番号
	// val	格納する値
	//------------------------------
	template <UNDO_ID U, int S>
	void MainPage::arrow_slider_set_header(const float val)
	{
		if constexpr (U == UNDO_ID::ARROW_SIZE) {
			constexpr wchar_t* SLIDER_HEADER[] = {
				L"str_arrow_width",
				L"str_arrow_length",
				L"str_arrow_offset"
			};
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, val, m_main_d2d.m_logical_dpi, m_main_page.m_grid_base + 1.0f, buf);
			const winrt::hstring text = ResourceLoader::GetForCurrentView().GetString(SLIDER_HEADER[S]) + L": " + buf;
			if constexpr (S == 0) {
				dialog_slider_0().Header(box_value(text));
			}
			if constexpr (S == 1) {
				dialog_slider_1().Header(box_value(text));
			}
			if constexpr (S == 2) {
				dialog_slider_2().Header(box_value(text));
			}
			if constexpr (S == 3) {
				dialog_slider_3().Header(box_value(text));
			}
		}
	}

	//------------------------------
	// スライダーの値が変更された.
	// U	操作の識別子
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	//------------------------------
	template <UNDO_ID U, int S> void MainPage::arrow_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		// 値をスライダーのヘッダーに格納する.
		if constexpr (U == UNDO_ID::ARROW_SIZE) {
			const float val = static_cast<float>(args.NewValue());
			ARROW_SIZE a_size;
			//m_sample_shape->get_arrow_size(a_size);
			m_dialog_page.m_shape_list.back()->get_arrow_size(a_size);
			if constexpr (S == 0) {
				arrow_slider_set_header<U, S>(val);
				a_size.m_width = static_cast<FLOAT>(val);
			}
			else if constexpr (S == 1) {
				arrow_slider_set_header<U, S>(val);
				a_size.m_length = static_cast<FLOAT>(val);
			}
			else if constexpr (S == 2) {
				arrow_slider_set_header<U, S>(val);
				a_size.m_offset = static_cast<FLOAT>(val);
			}
			//m_sample_shape->set_arrow_size(a_size);
			m_dialog_page.m_shape_list.back()->set_arrow_size(a_size);
		}
		if (scp_dialog_panel().IsLoaded()) {
			dialog_draw();
		}
	}

	//------------------------------
	// 線枠メニューの「矢じるしの寸法...」が選択された.
	//------------------------------
	IAsyncAction MainPage::arrow_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;

		m_dialog_page.set_attr_to(&m_main_page);
		ARROW_SIZE a_size;
		m_dialog_page.get_arrow_size(a_size);

		const auto ds0_max = dialog_slider_0().Maximum();
		const auto ds0_freq = dialog_slider_0().TickFrequency();
		const auto ds0_snap = dialog_slider_0().SnapsTo();
		const auto ds0_val = dialog_slider_0().Value();
		const auto ds0_vis = dialog_slider_0().Visibility();
		const auto ds1_max = dialog_slider_1().Maximum();
		const auto ds1_freq = dialog_slider_1().TickFrequency();
		const auto ds1_snap = dialog_slider_1().SnapsTo();
		const auto ds1_val = dialog_slider_1().Value();
		const auto ds1_vis = dialog_slider_1().Visibility();
		const auto ds2_max = dialog_slider_2().Maximum();
		const auto ds2_freq = dialog_slider_2().TickFrequency();
		const auto ds2_snap = dialog_slider_2().SnapsTo();
		const auto ds2_val = dialog_slider_2().Value();
		const auto ds2_vis = dialog_slider_2().Visibility();
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
		const winrt::event_token ds0_tok{
			dialog_slider_0().ValueChanged({ this, &MainPage::arrow_slider_val_changed<UNDO_ID::ARROW_SIZE, 0> })
		};
		const winrt::event_token ds1_tok{
			dialog_slider_1().ValueChanged({ this, &MainPage::arrow_slider_val_changed< UNDO_ID::ARROW_SIZE, 1> })
		};
		const winrt::event_token ds2_tok{
			dialog_slider_2().ValueChanged({ this, &MainPage::arrow_slider_val_changed< UNDO_ID::ARROW_SIZE, 2> })
		};
		arrow_slider_set_header<UNDO_ID::ARROW_SIZE, 0>(a_size.m_width);
		arrow_slider_set_header<UNDO_ID::ARROW_SIZE, 1>(a_size.m_length);
		arrow_slider_set_header<UNDO_ID::ARROW_SIZE, 2>(a_size.m_offset);
		const auto samp_w = scp_dialog_panel().Width();
		const auto samp_h = scp_dialog_panel().Height();
		const auto padd = samp_w * 0.125;
		const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd), static_cast<FLOAT>(padd) };
		const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(samp_w - 2.0 * padd), static_cast<FLOAT>(samp_h - 2.0 * padd) };
		m_dialog_page.m_shape_list.push_back(new ShapeLine(b_pos, b_vec, &m_dialog_page));
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif

		cd_setting_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_ARROWHEAD)));
		m_mutex_event.lock();
		const ContentDialogResult d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			ARROW_SIZE samp_val;
			//m_sample_shape->get_arrow_size(samp_val);
			m_dialog_page.m_shape_list.back()->get_arrow_size(samp_val);
			if (ustack_push_set<UNDO_ID::ARROW_SIZE>(samp_val)) {
				ustack_push_null();
				xcvd_is_enabled();
				page_draw();
			}
		}
		slist_clear(m_dialog_page.m_shape_list);
		dialog_slider_0().ValueChanged(ds0_tok);
		dialog_slider_1().ValueChanged(ds1_tok);
		dialog_slider_2().ValueChanged(ds2_tok);
		dialog_slider_0().Maximum(ds0_max);
		dialog_slider_0().TickFrequency(ds0_freq);
		dialog_slider_0().SnapsTo(ds0_snap);
		dialog_slider_0().Value(ds0_val);
		dialog_slider_0().Visibility(ds0_vis);
		dialog_slider_1().Maximum(ds1_max);
		dialog_slider_1().TickFrequency(ds1_freq);
		dialog_slider_1().SnapsTo(ds1_snap);
		dialog_slider_1().Value(ds1_val);
		dialog_slider_1().Visibility(ds1_vis);
		dialog_slider_2().Maximum(ds2_max);
		dialog_slider_2().TickFrequency(ds2_freq);
		dialog_slider_2().SnapsTo(ds2_snap);
		dialog_slider_2().Value(ds2_val);
		dialog_slider_2().Visibility(ds2_vis);
		//page_draw();
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