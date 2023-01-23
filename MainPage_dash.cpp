//------------------------------
// MainPage_dash.cpp
// 線枠
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	//using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	//using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	constexpr wchar_t DLG_TITLE[] = L"str_dash_pattern";

	// 線枠メニューの「破線の配置」が選択された.
	IAsyncAction MainPage::dash_patt_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_dialog_page.set_attr_to(&m_main_page);
		DASH_PATT d_patt;
		m_main_page.get_dash_patt(d_patt);

		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(d_patt.m_[0]);
		dash_slider_set_header<UNDO_ID::DASH_PATT, 0>(d_patt.m_[0]);
		dialog_slider_1().Maximum(MAX_VALUE);
		dialog_slider_1().TickFrequency(TICK_FREQ);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(d_patt.m_[1]);
		dash_slider_set_header<UNDO_ID::DASH_PATT, 1>(d_patt.m_[1]);
		dialog_slider_2().Maximum(MAX_VALUE);
		dialog_slider_2().TickFrequency(TICK_FREQ);
		dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_2().Value(d_patt.m_[2]);
		dash_slider_set_header<UNDO_ID::DASH_PATT, 2>(d_patt.m_[2]);
		dialog_slider_3().Maximum(MAX_VALUE);
		dialog_slider_3().TickFrequency(TICK_FREQ);
		dialog_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_3().Value(d_patt.m_[3]);
		dash_slider_set_header<UNDO_ID::DASH_PATT, 3>(d_patt.m_[3]);

		float s_width;
		m_main_page.get_stroke_width(s_width);

		dialog_slider_4().Maximum(MAX_VALUE);
		dialog_slider_4().TickFrequency(TICK_FREQ);
		dialog_slider_4().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_4().Value(s_width);
		dash_slider_set_header<UNDO_ID::STROKE_WIDTH, 4>(s_width);

		D2D1_DASH_STYLE s_style;
		m_main_page.get_dash_style(s_style);
		dialog_slider_0().Visibility(s_style != D2D1_DASH_STYLE_DOT ? Visibility::Visible : Visibility::Collapsed);
		dialog_slider_1().Visibility(s_style != D2D1_DASH_STYLE_DOT ? Visibility::Visible : Visibility::Collapsed);
		dialog_slider_2().Visibility(s_style != D2D1_DASH_STYLE_DASH ? Visibility::Visible : Visibility::Collapsed);
		dialog_slider_3().Visibility(s_style != D2D1_DASH_STYLE_DASH ? Visibility::Visible : Visibility::Collapsed);
		dialog_slider_4().Visibility(Visibility::Visible);
		const winrt::event_token slider_0_token{
			dialog_slider_0().ValueChanged({ this, &MainPage::dash_slider_val_changed<UNDO_ID::DASH_PATT, 0> })
		};
		const winrt::event_token slider_1_token{
			dialog_slider_1().ValueChanged({ this, &MainPage::dash_slider_val_changed<UNDO_ID::DASH_PATT, 1> })
		};
		const winrt::event_token slider_2_token{
			dialog_slider_2().ValueChanged({ this, &MainPage::dash_slider_val_changed<UNDO_ID::DASH_PATT, 2> })
		};
		const winrt::event_token slider_3_token{
			dialog_slider_3().ValueChanged({ this, &MainPage::dash_slider_val_changed<UNDO_ID::DASH_PATT, 3> })
		};
		const winrt::event_token slider_4_token{
			dialog_slider_4().ValueChanged({ this, &MainPage::dash_slider_val_changed<UNDO_ID::STROKE_WIDTH, 4> })
		};
		const auto p_width = scp_dialog_panel().Width();
		const auto p_height = scp_dialog_panel().Height();
		const auto padd = p_width * 0.125;
		const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd), static_cast<FLOAT>(padd) };
		const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(p_width - 2.0 * padd), static_cast<FLOAT>(p_height - 2.0 * padd) };
		m_dialog_page.m_shape_list.push_back(new ShapeLine(b_pos, b_vec, &m_dialog_page));
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif

		cd_setting_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const ContentDialogResult d_result{
			co_await cd_setting_dialog().ShowAsync()
		};
		if (d_result == ContentDialogResult::Primary) {
			DASH_PATT sample_patt;
			float sample_width;
			//m_sample_shape->get_dash_patt(sample_patt);
			//m_sample_shape->get_stroke_width(sample_width);
			m_dialog_page.m_shape_list.back()->get_dash_patt(sample_patt);
			m_dialog_page.m_shape_list.back()->get_stroke_width(sample_width);
			if (ustack_push_set<UNDO_ID::DASH_PATT>(sample_patt) ||
				ustack_push_set<UNDO_ID::STROKE_WIDTH>(sample_width)) {
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
		dialog_slider_4().Visibility(Visibility::Collapsed);
		dialog_slider_0().ValueChanged(slider_0_token);
		dialog_slider_1().ValueChanged(slider_1_token);
		dialog_slider_2().ValueChanged(slider_2_token);
		dialog_slider_3().ValueChanged(slider_3_token);
		dialog_slider_4().ValueChanged(slider_4_token);
		page_draw();
	}

	// 線枠メニューの「破線の形式」のサブ項目が選択された.
	void MainPage::dash_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		D2D1_DASH_STYLE d_style;
		if (sender == rmfi_dash_style_solid()) {
			d_style = D2D1_DASH_STYLE_SOLID;
		}
		else if (sender == rmfi_dash_style_dash()) {
			d_style = D2D1_DASH_STYLE_DASH;
		}
		else if (sender == rmfi_dash_style_dot()) {
			d_style = D2D1_DASH_STYLE_DOT;
		}
		else if (sender == rmfi_dash_style_dash_dot()) {
			d_style = D2D1_DASH_STYLE_DASH_DOT;
		}
		else if (sender == rmfi_dash_style_dash_dot_dot()) {
			d_style = D2D1_DASH_STYLE_DASH_DOT_DOT;
		}
		else {
			return;
		}
		mfi_dash_patt().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);
		//mfi_dash_patt_2().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);
		if (ustack_push_set<UNDO_ID::DASH_STYLE>(d_style)) {
			ustack_push_null();
			xcvd_is_enabled();
			page_draw();
		}
	}

	// 線枠メニューの「破線の形式」に印をつける.
	// d_style	破線の形式
	void MainPage::dash_style_is_checked(const D2D1_DASH_STYLE d_style)
	{
		rmfi_dash_style_solid().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		rmfi_dash_style_dash().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH);
		rmfi_dash_style_dash_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT);
		rmfi_dash_style_dash_dot_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT);
		rmfi_dash_style_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT);

		mfi_dash_patt().IsEnabled(d_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
	}

	// 値をスライダーのヘッダーに格納する.
	// U	操作の識別子
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <UNDO_ID U, int S>
	void MainPage::dash_slider_set_header(const float val)
	{
		if constexpr (U == UNDO_ID::DASH_PATT) {
			constexpr wchar_t* R[]{ L"str_dash_len", L"str_dash_gap", L"str_dot_len", L"str_dot_gap" };
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, val, m_main_d2d.m_logical_dpi, m_main_page.m_grid_base + 1.0f, buf);
			const winrt::hstring text{ ResourceLoader::GetForCurrentView().GetString(R[S]) + L": " + buf };
			dialog_set_slider_header<S>(text);
		}
		if constexpr (U == UNDO_ID::STROKE_WIDTH && S == 4) {
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, val, m_main_d2d.m_logical_dpi, m_main_page.m_grid_base + 1.0f, buf);
			const winrt::hstring text{ ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " + buf };
			dialog_slider_4().Header(box_value(text));
		}
	}

	// スライダーの値が変更された.
	// U	操作の識別子
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_ID U, int S> void MainPage::dash_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_ID::DASH_PATT) {
			const float val = static_cast<float>(args.NewValue());
			DASH_PATT patt;
			//m_sample_shape->get_dash_patt(patt);
			m_dialog_page.m_shape_list.back()->get_dash_patt(patt);
			if constexpr (S == 0) {
				dash_slider_set_header<U, S>(val);
				patt.m_[0] = static_cast<FLOAT>(val);
			}
			else if constexpr (S == 1) {
				dash_slider_set_header<U, S>(val);
				patt.m_[1] = static_cast<FLOAT>(val);
			}
			else if constexpr (S == 2) {
				dash_slider_set_header<U, S>(val);
				patt.m_[2] = patt.m_[4] = static_cast<FLOAT>(val);
			}
			else if constexpr (S == 3) {
				dash_slider_set_header<U, S>(val);
				patt.m_[3] = patt.m_[5] = static_cast<FLOAT>(val);
			}
			//m_sample_shape->set_dash_patt(patt);
			m_dialog_page.m_shape_list.back()->set_dash_patt(patt);
		}
		else if constexpr (U == UNDO_ID::STROKE_WIDTH && S == 4) {
			const float val = static_cast<float>(args.NewValue());
			dash_slider_set_header<U, S>(val);
			m_dialog_page.m_shape_list.back()->set_stroke_width(val);
		}
		if (scp_dialog_panel().IsLoaded()) {
			dialog_draw();
		}
	}

}