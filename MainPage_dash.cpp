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

	constexpr wchar_t DLG_TITLE[] = L"str_dash_patt";

	// 線枠メニューの「破線の配置」が選択された.
	IAsyncAction MainPage::dash_patt_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_prop_sheet.set_attr_to(&m_main_sheet);
		DASH_PATT d_patt;
		m_main_sheet.get_dash_patt(d_patt);

		prop_slider_0().Maximum(MAX_VALUE);
		prop_slider_0().TickFrequency(TICK_FREQ);
		prop_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_0().Value(d_patt.m_[0]);
		dash_slider_set_header<UNDO_OP::DASH_PATT, 0>(d_patt.m_[0]);
		prop_slider_1().Maximum(MAX_VALUE);
		prop_slider_1().TickFrequency(TICK_FREQ);
		prop_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_1().Value(d_patt.m_[1]);
		dash_slider_set_header<UNDO_OP::DASH_PATT, 1>(d_patt.m_[1]);
		prop_slider_2().Maximum(MAX_VALUE);
		prop_slider_2().TickFrequency(TICK_FREQ);
		prop_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_2().Value(d_patt.m_[2]);
		dash_slider_set_header<UNDO_OP::DASH_PATT, 2>(d_patt.m_[2]);
		prop_slider_3().Maximum(MAX_VALUE);
		prop_slider_3().TickFrequency(TICK_FREQ);
		prop_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_3().Value(d_patt.m_[3]);
		dash_slider_set_header<UNDO_OP::DASH_PATT, 3>(d_patt.m_[3]);

		float s_width;
		m_main_sheet.get_stroke_width(s_width);

		prop_slider_4().Maximum(MAX_VALUE);
		prop_slider_4().TickFrequency(TICK_FREQ);
		prop_slider_4().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_4().Value(s_width);
		dash_slider_set_header<UNDO_OP::STROKE_WIDTH, 4>(s_width);

		D2D1_DASH_STYLE s_style;
		m_main_sheet.get_dash_style(s_style);
		prop_slider_0().Visibility(s_style != D2D1_DASH_STYLE_DOT ? UI_VISIBLE : UI_COLLAPSED);
		prop_slider_1().Visibility(s_style != D2D1_DASH_STYLE_DOT ? UI_VISIBLE : UI_COLLAPSED);
		prop_slider_2().Visibility(s_style != D2D1_DASH_STYLE_DASH ? UI_VISIBLE : UI_COLLAPSED);
		prop_slider_3().Visibility(s_style != D2D1_DASH_STYLE_DASH ? UI_VISIBLE : UI_COLLAPSED);
		prop_slider_4().Visibility(UI_VISIBLE);
		const winrt::event_token slider_0_token{
			prop_slider_0().ValueChanged({ this, &MainPage::dash_slider_val_changed<UNDO_OP::DASH_PATT, 0> })
		};
		const winrt::event_token slider_1_token{
			prop_slider_1().ValueChanged({ this, &MainPage::dash_slider_val_changed<UNDO_OP::DASH_PATT, 1> })
		};
		const winrt::event_token slider_2_token{
			prop_slider_2().ValueChanged({ this, &MainPage::dash_slider_val_changed<UNDO_OP::DASH_PATT, 2> })
		};
		const winrt::event_token slider_3_token{
			prop_slider_3().ValueChanged({ this, &MainPage::dash_slider_val_changed<UNDO_OP::DASH_PATT, 3> })
		};
		const winrt::event_token slider_4_token{
			prop_slider_4().ValueChanged({ this, &MainPage::dash_slider_val_changed<UNDO_OP::STROKE_WIDTH, 4> })
		};
		const auto panel_w = scp_prop_panel().Width();
		const auto panel_h = scp_prop_panel().Height();
		const auto padd = panel_w * 0.125;
		const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd), static_cast<FLOAT>(padd) };
		const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(panel_w - 2.0 * padd), static_cast<FLOAT>(panel_h - 2.0 * padd) };
		m_prop_sheet.m_shape_list.push_back(new ShapeLine(b_pos, b_vec, &m_prop_sheet));
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif

		cd_prop_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const ContentDialogResult d_result{
			co_await cd_prop_dialog().ShowAsync()
		};
		if (d_result == ContentDialogResult::Primary) {
			DASH_PATT sample_patt;
			float sample_width;
			//m_sample_shape->get_dash_patt(sample_patt);
			//m_sample_shape->get_stroke_width(sample_width);
			m_prop_sheet.m_shape_list.back()->get_dash_patt(sample_patt);
			m_prop_sheet.m_shape_list.back()->get_stroke_width(sample_width);
			if (ustack_push_set<UNDO_OP::DASH_PATT>(sample_patt) ||
				ustack_push_set<UNDO_OP::STROKE_WIDTH>(sample_width)) {
				ustack_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		slist_clear(m_prop_sheet.m_shape_list);
		prop_slider_0().Visibility(UI_COLLAPSED);
		prop_slider_1().Visibility(UI_COLLAPSED);
		prop_slider_2().Visibility(UI_COLLAPSED);
		prop_slider_3().Visibility(UI_COLLAPSED);
		prop_slider_4().Visibility(UI_COLLAPSED);
		prop_slider_0().ValueChanged(slider_0_token);
		prop_slider_1().ValueChanged(slider_1_token);
		prop_slider_2().ValueChanged(slider_2_token);
		prop_slider_3().ValueChanged(slider_3_token);
		prop_slider_4().ValueChanged(slider_4_token);
		sheet_draw();
	}

	// 線枠メニューの「種類」のサブ項目が選択された.
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
		if (ustack_push_set<UNDO_OP::DASH_STYLE>(d_style)) {
			ustack_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// 線枠メニューの「種類」に印をつける.
	// d_style	破線の形式
	void MainPage::dash_style_is_checked(const D2D1_DASH_STYLE d_style)
	{
		rmfi_dash_style_solid().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		//rmfi_dash_style_solid_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		rmfi_dash_style_dash().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH);
		//rmfi_dash_style_dash_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH);
		rmfi_dash_style_dash_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT);
		//rmfi_dash_style_dash_dot_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT);
		rmfi_dash_style_dash_dot_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT);
		//rmfi_dash_style_dash_dot_dot_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT);
		rmfi_dash_style_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT);
		//rmfi_dash_style_dot_2().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT);

		mfi_dash_patt().IsEnabled(d_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		//mfi_dash_patt_2().IsEnabled(d_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
	}

	// 値をスライダーのヘッダーに格納する.
	// U	操作の種類
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <UNDO_OP U, int S>
	void MainPage::dash_slider_set_header(const float val)
	{
		if constexpr (U == UNDO_OP::DASH_PATT) {
			constexpr wchar_t* R[]{ L"str_dash_len", L"str_dash_gap", L"str_dot_len", L"str_dot_gap" };
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, val, m_main_sheet.m_d2d.m_logical_dpi, m_main_sheet.m_grid_base + 1.0f, buf);
			const winrt::hstring text{ ResourceLoader::GetForCurrentView().GetString(R[S]) + L": " + buf };
			prop_set_slider_header<S>(text);
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH && S == 4) {
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, val, m_main_sheet.m_d2d.m_logical_dpi, m_main_sheet.m_grid_base + 1.0f, buf);
			const winrt::hstring text{ ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " + buf };
			prop_slider_4().Header(box_value(text));
		}
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S> void MainPage::dash_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::DASH_PATT) {
			const float val = static_cast<float>(args.NewValue());
			DASH_PATT patt;
			//m_sample_shape->get_dash_patt(patt);
			m_prop_sheet.m_shape_list.back()->get_dash_patt(patt);
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
			m_prop_sheet.m_shape_list.back()->set_dash_patt(patt);
		}
		else if constexpr (U == UNDO_OP::STROKE_WIDTH && S == 4) {
			const float val = static_cast<float>(args.NewValue());
			dash_slider_set_header<U, S>(val);
			m_prop_sheet.m_shape_list.back()->set_stroke_width(val);
		}
		if (scp_prop_panel().IsLoaded()) {
			prop_sample_draw();
		}
	}

}