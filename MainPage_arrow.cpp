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
	using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	using winrt::Windows::UI::Xaml::RoutedEventArgs;

	constexpr wchar_t TITLE_ARROWHEAD[] = L"str_arrow_size";

	// 値をスライダーのヘッダーに格納する.
	// U	操作の種類
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <UNDO_OP U, int S>
	void MainPage::arrow_slider_set_header(const float val)
	{
		if constexpr (U == UNDO_OP::ARROW_SIZE) {
			constexpr wchar_t* SLIDER_HEADER[] = {
				L"str_arrow_width",
				L"str_arrow_length",
				L"str_arrow_offset"
			};
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, val, m_main_sheet.m_d2d.m_logical_dpi, m_main_sheet.m_grid_base + 1.0f, buf);
			const winrt::hstring text = ResourceLoader::GetForCurrentView().GetString(SLIDER_HEADER[S]) + L": " + buf;
			if constexpr (S == 0) {
				prop_slider_0().Header(box_value(text));
			}
			if constexpr (S == 1) {
				prop_slider_1().Header(box_value(text));
			}
			if constexpr (S == 2) {
				prop_slider_2().Header(box_value(text));
			}
			if constexpr (S == 3) {
				prop_slider_3().Header(box_value(text));
			}
		}
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S> void MainPage::arrow_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		// 値をスライダーのヘッダーに格納する.
		if constexpr (U == UNDO_OP::ARROW_SIZE) {
			const float val = static_cast<float>(args.NewValue());
			ARROW_SIZE a_size;
			//m_sample_shape->get_arrow_size(a_size);
			m_prop_sheet.m_shape_list.back()->get_arrow_size(a_size);
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
			m_prop_sheet.m_shape_list.back()->set_arrow_size(a_size);
		}
		if (scp_prop_panel().IsLoaded()) {
			prop_sample_draw();
		}
	}

	// 線枠メニューの「矢じるしの大きさ」が選択された.
	IAsyncAction MainPage::arrow_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;

		m_prop_sheet.set_attr_to(&m_main_sheet);
		ARROW_SIZE a_size;
		m_prop_sheet.get_arrow_size(a_size);

		prop_slider_0().Maximum(MAX_VALUE);
		prop_slider_0().TickFrequency(TICK_FREQ);
		prop_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_0().Value(a_size.m_width);
		arrow_slider_set_header<UNDO_OP::ARROW_SIZE, 0>(a_size.m_width);
		prop_slider_1().Maximum(MAX_VALUE);
		prop_slider_1().TickFrequency(TICK_FREQ);
		prop_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_1().Value(a_size.m_length);
		arrow_slider_set_header<UNDO_OP::ARROW_SIZE, 1>(a_size.m_length);
		prop_slider_2().Maximum(MAX_VALUE);
		prop_slider_2().TickFrequency(TICK_FREQ);
		prop_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_2().Value(a_size.m_offset);
		arrow_slider_set_header<UNDO_OP::ARROW_SIZE, 2>(a_size.m_offset);

		prop_slider_0().Visibility(UI_VISIBLE);
		prop_slider_1().Visibility(UI_VISIBLE);
		prop_slider_2().Visibility(UI_VISIBLE);
		const winrt::event_token slider_0_token{
			prop_slider_0().ValueChanged({ this, &MainPage::arrow_slider_val_changed<UNDO_OP::ARROW_SIZE, 0> })
		};
		const winrt::event_token slider_1_token{
			prop_slider_1().ValueChanged({ this, &MainPage::arrow_slider_val_changed< UNDO_OP::ARROW_SIZE, 1> })
		};
		const winrt::event_token slider_2_token{
			prop_slider_2().ValueChanged({ this, &MainPage::arrow_slider_val_changed< UNDO_OP::ARROW_SIZE, 2> })
		};
		const auto samp_w = scp_prop_panel().Width();
		const auto samp_h = scp_prop_panel().Height();
		const auto padd = samp_w * 0.125;
		const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd), static_cast<FLOAT>(padd) };
		const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(samp_w - 2.0 * padd), static_cast<FLOAT>(samp_h - 2.0 * padd) };
		m_prop_sheet.m_shape_list.push_back(new ShapeLine(b_pos, b_vec, &m_prop_sheet));
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif

		cd_prop_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_ARROWHEAD)));
		const ContentDialogResult d_result{
			co_await cd_prop_dialog().ShowAsync()
		};
		if (d_result == ContentDialogResult::Primary) {
			ARROW_SIZE samp_val;
			//m_sample_shape->get_arrow_size(samp_val);
			m_prop_sheet.m_shape_list.back()->get_arrow_size(samp_val);
			if (ustack_push_set<UNDO_OP::ARROW_SIZE>(samp_val)) {
				ustack_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		slist_clear(m_prop_sheet.m_shape_list);
		prop_slider_0().Visibility(UI_COLLAPSED);
		prop_slider_1().Visibility(UI_COLLAPSED);
		prop_slider_2().Visibility(UI_COLLAPSED);
		prop_slider_0().ValueChanged(slider_0_token);
		prop_slider_1().ValueChanged(slider_1_token);
		prop_slider_2().ValueChanged(slider_2_token);
		sheet_draw();
	}

	// 線枠メニューの「矢じるしの種類」のサブ項目が選択された.
	void MainPage::arrow_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		ARROW_STYLE a_style;
		if (sender == rmfi_arrow_style_none() || sender == rmfi_arrow_style_none_2()) {
			a_style = ARROW_STYLE::NONE;
		}
		else if (sender == rmfi_arrow_style_opened() || sender == rmfi_arrow_style_opened_2()) {
			a_style = ARROW_STYLE::OPENED;
		}
		else if (sender == rmfi_arrow_style_filled() || sender == rmfi_arrow_style_filled_2()) {
			a_style = ARROW_STYLE::FILLED;
		}
		else {
			return;
		}
		mfi_arrow_size().IsEnabled(a_style != ARROW_STYLE::NONE);
		mfi_arrow_size_2().IsEnabled(a_style != ARROW_STYLE::NONE);
		if (ustack_push_set<UNDO_OP::ARROW_STYLE>(a_style)) {
			ustack_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// 線枠メニューの「矢じるしの種類」に印をつける.
	// a_style	矢じるしの形式
	void MainPage::arrow_style_is_checked(const ARROW_STYLE val)
	{
		rmfi_arrow_style_none().IsChecked(val == ARROW_STYLE::NONE);
		rmfi_arrow_style_none_2().IsChecked(val == ARROW_STYLE::NONE);
		rmfi_arrow_style_opened().IsChecked(val == ARROW_STYLE::OPENED);
		rmfi_arrow_style_opened_2().IsChecked(val == ARROW_STYLE::OPENED);
		rmfi_arrow_style_filled().IsChecked(val == ARROW_STYLE::FILLED);
		rmfi_arrow_style_filled_2().IsChecked(val == ARROW_STYLE::FILLED);
		mfi_arrow_size().IsEnabled(val != ARROW_STYLE::NONE);
		mfi_arrow_size_2().IsEnabled(val != ARROW_STYLE::NONE);
	}

}