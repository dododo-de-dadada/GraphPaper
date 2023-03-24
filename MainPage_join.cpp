#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Foundation::IAsyncAction;
	//using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	constexpr wchar_t DLG_TITLE[] = L"str_join_miter_limit";

	// 線枠メニューの「端の形式」が選択された.
	void MainPage::cap_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		CAP_STYLE new_val;
		if (sender == rmfi_cap_style_flat()) {
			new_val = CAP_STYLE_FLAT;
		}
		else if (sender == rmfi_cap_style_square()) {
			new_val = CAP_STYLE_SQUARE;
		}
		else if (sender == rmfi_cap_style_round()) {
			new_val = CAP_STYLE_ROUND;
		}
		else if (sender == rmfi_cap_style_triangle()) {
			new_val = CAP_STYLE_TRIANGLE;
		}
		else {
			winrt::hresult_not_implemented();
		}
		cap_style_is_checked(new_val);
		if (ustack_push_set<UNDO_T::STROKE_CAP>(new_val)) {
			ustack_push_null();
			ustack_is_enable();
			page_draw();
		}
		status_bar_set_pos();
	}

	// 線枠メニューの「端の形式」に印をつける.
	// s_cap	端の形式
	void MainPage::cap_style_is_checked(const CAP_STYLE& val)
	{
		rmfi_cap_style_flat().IsChecked(equal(val, CAP_STYLE_FLAT));
		rmfi_cap_style_square().IsChecked(equal(val, CAP_STYLE_SQUARE));
		rmfi_cap_style_round().IsChecked(equal(val, CAP_STYLE_ROUND));
		rmfi_cap_style_triangle().IsChecked(equal(val, CAP_STYLE_TRIANGLE));
	}

	// 線枠メニューの「線の結合の結合」>「尖り制限」が選択された.
	IAsyncAction MainPage::join_miter_limit_click_async(IInspectable const& sender, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		const auto str_join_miter_limit{ ResourceLoader::GetForCurrentView().GetString(L"str_join_miter_limit") + L": " };
		const auto str_stroke_width{ ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " };
		const auto str_title{ ResourceLoader::GetForCurrentView().GetString(L"str_join_miter_limit") };
		wchar_t buf[32];

		m_dialog_page.set_attr_to(&m_main_page);
		const auto unit = m_len_unit;
		const auto dpi = m_dialog_d2d.m_logical_dpi;
		const auto g_len = m_dialog_page.m_grid_base + 1.0f;
		float j_limit;
		m_dialog_page.get_join_miter_limit(j_limit);
		j_limit -= 1.0f;

		dialog_slider_0().Minimum(0.0);
		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(j_limit);
		dialog_slider_0().Visibility(Visibility::Visible);
		//join_slider_set_header<0>(j_limit);
		swprintf_s(buf, L"%.1lf", static_cast<double>(j_limit) + 1.0);
		dialog_slider_0().Header(box_value(str_join_miter_limit + buf));

		float s_width;
		m_dialog_page.get_stroke_width(s_width);

		dialog_slider_1().Minimum(0.0);
		dialog_slider_1().Maximum(MAX_VALUE);
		dialog_slider_1().TickFrequency(TICK_FREQ);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(s_width);
		dialog_slider_1().Visibility(Visibility::Visible);
		//join_slider_set_header<1>(s_width);
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, s_width, dpi, g_len, buf);
		dialog_slider_1().Header(box_value(str_stroke_width + buf));

		const auto samp_w = scp_dialog_panel().Width();
		const auto samp_h = scp_dialog_panel().Height();
		const auto pad = samp_w * 0.125;
		const D2D1_POINT_2F start{
			static_cast<FLOAT>(pad), static_cast<FLOAT>(pad) 
		};
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(samp_w - 2.0 * pad), static_cast<FLOAT>(samp_h - 2.0 * pad)
		};
		POLY_OPTION p_opt{ 3, true, true, false, true };
		auto s = new ShapePoly(start, pos, &m_dialog_page, p_opt);
		const float offset = static_cast<float>(samp_h / 16.0);
		const float samp_x = static_cast<float>(samp_w * 0.25);
		const float samp_y = static_cast<float>(samp_h * 0.5);
		s->set_select(true);
		s->set_pos_anc(D2D1_POINT_2F{ -samp_x, samp_y - offset }, ANC_TYPE::ANC_P0, m_snap_interval, false);
		s->set_pos_anc(D2D1_POINT_2F{ samp_x, samp_y }, ANC_TYPE::ANC_P0 + 1, m_snap_interval, false);
		s->set_pos_anc(D2D1_POINT_2F{ -samp_x, samp_y + offset }, ANC_TYPE::ANC_P0 + 2, m_snap_interval, false);
		m_dialog_page.m_shape_list.push_back(s);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif

		cd_setting_dialog().Title(box_value(str_title));
		m_mutex_event.lock();
		{
			const auto revoker0{
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_join_miter_limit](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					wchar_t buf[32];
					const float val = static_cast<float>(args.NewValue());
					swprintf_s(buf, L"%.1lf", static_cast<double>(val) + 1.0);
					dialog_slider_0().Header(box_value(str_join_miter_limit + buf));
					if (m_dialog_page.m_shape_list.back()->set_join_miter_limit(val + 1.0f)) {
						dialog_draw();
					}
				})
			};
			const auto revoker1{
				dialog_slider_1().ValueChanged(winrt::auto_revoke, [this, str_stroke_width](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_dialog_d2d.m_logical_dpi;
					const auto g_len = m_dialog_page.m_grid_base + 1.0f;
					wchar_t buf[32];
					const float val = static_cast<float>(args.NewValue());
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_1().Header(box_value(str_stroke_width + buf));
					if (m_dialog_page.m_shape_list.back()->set_stroke_width(val)) {
						dialog_draw();
					}
				})
			};
			if (co_await cd_setting_dialog().ShowAsync() == ContentDialogResult::Primary) {
				float new_limit;
				float new_width;
				m_dialog_page.m_shape_list.back()->get_join_miter_limit(new_limit);
				m_dialog_page.m_shape_list.back()->get_stroke_width(new_width);
				const bool flag_limit = ustack_push_set<UNDO_T::JOIN_LIMIT>(new_limit);
				const bool flag_width = ustack_push_set<UNDO_T::STROKE_WIDTH>(new_width);
				if (flag_limit || flag_width) {
					ustack_push_null();
					ustack_is_enable();
					page_draw();
				}
			}
		}
		slist_clear(m_dialog_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		m_mutex_event.unlock();
	}

	/*
	// 値をスライダーのヘッダーに格納する.
	// U	操作の識別子
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <int S>
	void MainPage::join_slider_set_header(const float val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		if constexpr (S == 0) {
			constexpr size_t LEN = 32;
			wchar_t buf[LEN + 1];
			swprintf_s(buf, LEN, L"%.1lf", static_cast<double>(val) + 1.0);
			dialog_slider_0().Header(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_join_miter_limit") + L": " + buf));
		}
		else if constexpr (S == 1) {
			constexpr size_t LEN = 32;
			wchar_t buf[LEN + 1];
			const auto unit = m_len_unit;
			const auto dpi = m_dialog_d2d.m_logical_dpi;
			const auto g_len = m_main_page.m_grid_base + 1.0f;
			conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
			const auto str_stroke_width{ ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " };
			dialog_slider_1().Header(box_value(str_stroke_width + buf));
		}
	}
	*/

	/*
	// スライダーの値が変更された.
	// U	操作の識別子
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <int S>
	void MainPage::join_slider_val_changed(
		IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (S == 0) {
			const float val = static_cast<float>(args.NewValue());
			join_slider_set_header<0>(val);
			if (m_dialog_page.m_shape_list.back()->set_join_miter_limit(val + 1.0f)) {
				dialog_draw();
			}
		}
		else if constexpr (S == 1) {
			const float val = static_cast<float>(args.NewValue());
			join_slider_set_header<1>(val);
			if (m_dialog_page.m_shape_list.back()->set_stroke_width(val)) {
				dialog_draw();
			}
		}
	}
	*/

	// 線枠メニューの「線の結合の形式」が選択された.
	void MainPage::join_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		D2D1_LINE_JOIN new_val;
		if (sender == rmfi_join_style_bevel()) {
			new_val = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;
		}
		else if (sender == rmfi_join_style_miter()) {
			new_val = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER;
		}
		else if (sender == rmfi_join_style_miter_or_bevel()) {
			new_val = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL;
		}
		else if (sender == rmfi_join_style_round()) {
			new_val = D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND;
		}
		else {
			winrt::hresult_not_implemented();
			return;
		}
		join_style_is_checked(new_val);
		if (ustack_push_set<UNDO_T::JOIN_STYLE>(new_val)) {
			ustack_push_null();
			ustack_is_enable();
			page_draw();
		}
		status_bar_set_pos();
	}

	// 線枠メニューの「結合」に印をつける.
	// s_join	線の結合
	void MainPage::join_style_is_checked(const D2D1_LINE_JOIN val)
	{
		rmfi_join_style_bevel().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL);
		rmfi_join_style_miter().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
		rmfi_join_style_miter_or_bevel().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL);
		rmfi_join_style_round().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND);
		mfi_join_miter_limit().IsEnabled(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER || val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL);
	}

}