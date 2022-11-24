#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	using winrt::Windows::UI::Xaml::RoutedEventArgs;

	constexpr wchar_t DLG_TITLE[] = L"str_line_join";

	// 線枠メニューの「端の種類」が選択された.
	void MainPage::cap_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		CAP_STYLE new_val;
		if (sender == rmfi_cap_style_flat()) {
			new_val = CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };
		}
		else if (sender == rmfi_cap_style_square()) {
			new_val = CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE };
		}
		else if (sender == rmfi_cap_style_round()) {
			new_val = CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND };
		}
		else if (sender == rmfi_cap_style_triangle()) {
			new_val = CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE };
		}
		else {
			return;
		}
		cap_style_is_checked(new_val);
		if (ustack_push_set<UNDO_OP::STROKE_CAP>(new_val)) {
			ustack_push_null();
			ustack_is_enable();
			sheet_draw();
		}
	}

	// 線枠メニューの「端の種類」に印をつける.
	// s_cap	端の形式
	void MainPage::cap_style_is_checked(const CAP_STYLE& val)
	{
		rmfi_cap_style_flat().IsChecked(equal(val, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT }));
		//rmfi_cap_style_flat_2().IsChecked(equal(val, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT }));
		rmfi_cap_style_square().IsChecked(equal(val, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE }));
		//rmfi_cap_style_square_2().IsChecked(equal(val, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE }));
		rmfi_cap_style_round().IsChecked(equal(val, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND }));
		//rmfi_cap_style_round_2().IsChecked(equal(val, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND }));
		rmfi_cap_style_triangle().IsChecked(equal(val, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE }));
		//rmfi_cap_style_triangle_2().IsChecked(equal(val, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE }));
	}

	// 線枠メニューの「つなぎの種類」>「額ぶちの制限」が選択された.
	IAsyncAction MainPage::join_limit_click_async(IInspectable const& sender, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_prop_sheet.set_attr_to(&m_main_sheet);
		float j_limit;
		m_prop_sheet.get_join_limit(j_limit);
		j_limit -= 1.0f;

		prop_slider_0().Maximum(MAX_VALUE);
		prop_slider_0().TickFrequency(TICK_FREQ);
		prop_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_0().Value(j_limit);
		prop_slider_0().Visibility(UI_VISIBLE);
		join_slider_set_header<UNDO_OP::JOIN_LIMIT, 0>(j_limit);

		float s_width;
		m_prop_sheet.get_stroke_width(s_width);

		prop_slider_1().Maximum(MAX_VALUE);
		prop_slider_1().TickFrequency(TICK_FREQ);
		prop_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_1().Value(s_width);
		prop_slider_1().Visibility(UI_VISIBLE);
		join_slider_set_header<UNDO_OP::STROKE_WIDTH, 1>(s_width);

		const auto slider_0_token = prop_slider_0().ValueChanged({ this, &MainPage::join_slider_val_changed<UNDO_OP::JOIN_LIMIT, 0> });
		const auto slider_1_token = prop_slider_1().ValueChanged({ this, &MainPage::join_slider_val_changed<UNDO_OP::STROKE_WIDTH, 1> });
		const auto samp_w = scp_prop_panel().Width();
		const auto samp_h = scp_prop_panel().Height();
		const auto padd = samp_w * 0.125;
		const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd), static_cast<FLOAT>(padd) };
		const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(samp_w - 2.0 * padd), static_cast<FLOAT>(samp_h - 2.0 * padd) };
		POLY_OPTION p_opt{ 3, true, true, false, true };
		auto s = new ShapePoly(b_pos, b_vec, &m_prop_sheet, p_opt);
		const float offset = static_cast<float>(samp_h / 16.0);
		const float samp_x = static_cast<float>(samp_w * 0.25);
		const float samp_y = static_cast<float>(samp_h * 0.5);
		s->set_select(true);
		s->set_pos_anc(D2D1_POINT_2F{ -samp_x, samp_y - offset }, ANC_TYPE::ANC_P0, m_vert_stick, false);
		s->set_pos_anc(D2D1_POINT_2F{ samp_x, samp_y }, ANC_TYPE::ANC_P0 + 1, m_vert_stick, false);
		s->set_pos_anc(D2D1_POINT_2F{ -samp_x, samp_y + offset }, ANC_TYPE::ANC_P0 + 2, m_vert_stick, false);
		m_prop_sheet.m_shape_list.push_back(s);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif

		cd_prop_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_line_join")));
		const auto d_result = co_await cd_prop_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float samp_limit;
			float samp_width;
			m_prop_sheet.m_shape_list.back()->get_join_limit(samp_limit);
			m_prop_sheet.m_shape_list.back()->get_stroke_width(samp_width);
			if (ustack_push_set<UNDO_OP::JOIN_LIMIT>(samp_limit) ||
				ustack_push_set<UNDO_OP::STROKE_WIDTH>(samp_width)) {
				ustack_push_null();
				ustack_is_enable();
				sheet_draw();
			}
		}
		slist_clear(m_prop_sheet.m_shape_list);
		prop_slider_0().Visibility(UI_COLLAPSED);
		prop_slider_0().ValueChanged(slider_0_token);
		prop_slider_1().Visibility(UI_COLLAPSED);
		prop_slider_1().ValueChanged(slider_1_token);
		co_return;
	}

	// 値をスライダーのヘッダーに格納する.
	// U	操作の種類
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <UNDO_OP U, int S>
	void MainPage::join_slider_set_header(const float val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		if constexpr (U == UNDO_OP::JOIN_LIMIT && S == 0) {
			constexpr size_t LEN = 32;
			wchar_t buf[LEN + 1];
			swprintf_s(buf, LEN, L"%.1lf", static_cast<double>(val) + 1.0);
			const auto text = ResourceLoader::GetForCurrentView().GetString(L"str_join_limit") + L": " + buf;
			prop_slider_0().Header(box_value(text));
		}
		else if constexpr (U == UNDO_OP::STROKE_WIDTH && S == 1) {
			constexpr size_t LEN = 32;
			wchar_t buf[LEN + 1];
			conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, val, m_main_sheet.m_d2d.m_logical_dpi, m_main_sheet.m_grid_base + 1.0f, buf);
			const auto text = ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " + buf;
			prop_slider_1().Header(box_value(text));
		}
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S>
	void MainPage::join_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::JOIN_LIMIT && S == 0) {
			const float val = static_cast<float>(args.NewValue());
			join_slider_set_header<U, S>(val);
			m_prop_sheet.m_shape_list.back()->set_join_limit(val + 1.0f);
		}
		else if constexpr (U == UNDO_OP::STROKE_WIDTH && S == 1) {
			const float val = static_cast<float>(args.NewValue());
			join_slider_set_header<U, S>(val);
			m_prop_sheet.m_shape_list.back()->set_stroke_width(val);
		}
		if (scp_prop_panel().IsLoaded()) {
			prop_sample_draw();
		}
	}

	// 線枠メニューの「つなぎの種類」が選択された.
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
			return;
		}
		join_style_is_checked(new_val);
		if (ustack_push_set<UNDO_OP::JOIN_STYLE>(new_val)) {
			ustack_push_null();
			ustack_is_enable();
			sheet_draw();
		}
	}

	// 線枠メニューの「つなぎ」に印をつける.
	// s_join	線のつなぎ
	void MainPage::join_style_is_checked(const D2D1_LINE_JOIN val)
	{
		rmfi_join_style_bevel().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL);
		rmfi_join_style_miter().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
		rmfi_join_style_miter_or_bevel().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL);
		rmfi_join_style_round().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND);
		mfi_join_limit().IsEnabled(rmfi_join_style_miter().IsChecked() || rmfi_join_style_miter_or_bevel().IsChecked());
	}

}