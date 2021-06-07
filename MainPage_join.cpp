#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t DLG_TITLE[] = L"str_line_join";

	// 線枠メニューの「単点の形式」に印をつける.
	// s_cap	線の単点
	void MainPage::cap_style_is_checked(const CAP_STYLE& value)
	{
		menu_item_is_checked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT }), rmfi_cap_flat());
		menu_item_is_checked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT }), rmfi_cap_flat_2());
		menu_item_is_checked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE }), rmfi_cap_square());
		menu_item_is_checked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE }), rmfi_cap_square_2());
		menu_item_is_checked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND }), rmfi_cap_round());
		menu_item_is_checked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND }), rmfi_cap_round_2());
		menu_item_is_checked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE }), rmfi_cap_triangle());
		menu_item_is_checked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE }), rmfi_cap_triangle_2());
	}

	void MainPage::cap_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		CAP_STYLE new_value;
		if (sender == rmfi_cap_flat() || sender == rmfi_cap_flat_2()) {
			new_value = CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };
		}
		else if (sender == rmfi_cap_square() || sender == rmfi_cap_square_2()) {
			new_value = CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE };
		}
		else if (sender == rmfi_cap_round() || sender == rmfi_cap_round_2()) {
			new_value = CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND };
		}
		else if (sender == rmfi_cap_triangle() || sender == rmfi_cap_triangle_2()) {
			new_value = CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE };
		}
		else {
			return;
		}
		CAP_STYLE old_value;
		m_sheet_main.get_stroke_cap_style(old_value);
		if (undo_push_set<UNDO_OP::STROKE_CAP_STYLE>(new_value)) {
			undo_push_null();
			undo_menu_enable();
			sheet_draw();
		}
	}

	// 線枠メニューの「つながり」に印をつける.
	// s_join	線のつながり
	void MainPage::join_style_is_checked(const D2D1_LINE_JOIN value)
	{
		menu_item_is_checked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL, rmfi_join_bevel());
		menu_item_is_checked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL, rmfi_join_bevel_2());
		menu_item_is_checked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER, rmfi_join_miter());
		menu_item_is_checked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER, rmfi_join_miter_2());
		menu_item_is_checked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL, rmfi_join_m_or_b());
		menu_item_is_checked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL, rmfi_join_m_or_b_2());
		menu_item_is_checked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND, rmfi_join_round());
		menu_item_is_checked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND, rmfi_join_round_2());
		//radio_menu_item_set_value< D2D1_LINE_JOIN, D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL>(s_join, rmfi_join_bevel_2());
		//radio_menu_item_set_value< D2D1_LINE_JOIN, D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER>(s_join, rmfi_join_miter());
		//radio_menu_item_set_value< D2D1_LINE_JOIN, D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER>(s_join, rmfi_join_miter_2());
		//radio_menu_item_set_value< D2D1_LINE_JOIN, D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL>(s_join, rmfi_join_m_or_b());
		//radio_menu_item_set_value< D2D1_LINE_JOIN, D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL>(s_join, rmfi_join_m_or_b_2());
		//radio_menu_item_set_value< D2D1_LINE_JOIN, D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND>(s_join, rmfi_join_round());
		//radio_menu_item_set_value< D2D1_LINE_JOIN, D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND>(s_join, rmfi_join_round_2());
	}

	void MainPage::join_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		D2D1_LINE_JOIN new_value;
		if (sender == rmfi_join_bevel() || sender == rmfi_join_bevel_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;
		}
		else if (sender == rmfi_join_miter() || sender == rmfi_join_miter_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER;
		}
		else if (sender == rmfi_join_m_or_b() || sender == rmfi_join_m_or_b_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL;
		}
		else if (sender == rmfi_join_round() || sender == rmfi_join_round_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND;
		}
		else {
			return;
		}
		D2D1_LINE_JOIN old_value;
		m_sheet_main.get_stroke_join_style(old_value);
		if (undo_push_set<UNDO_OP::STROKE_JOIN_STYLE>(new_value)) {
			undo_push_null();
			undo_menu_enable();
			sheet_draw();
		}
	}

	constexpr float SLIDER_STEP = 0.5f;
	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S> void MainPage::join_set_slider_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring text;

		if constexpr (U == UNDO_OP::STROKE_JOIN_LIMIT) {
			constexpr size_t LEN = 32;
			wchar_t buf[LEN + 1];
			const float limit = value * SLIDER_STEP + 1.0f;
			swprintf_s(buf, LEN, L"%.1f", limit);
			text = ResourceLoader::GetForCurrentView().GetString(L"str_stroke_join_limit") + L": " + buf;
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			const float g_len = g_base + 1.0f;
			constexpr size_t LEN = 32;
			wchar_t buf[LEN + 1];
			conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), value * SLIDER_STEP, m_sheet_dx.m_logical_dpi, g_len, buf);
			text = ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " + buf;
		}
		if constexpr (S == 0) {
			sample_slider_0().Header(box_value(text));
		}
		if constexpr (S == 1) {
			sample_slider_1().Header(box_value(text));
		}
	}

	// 値をスライダーのヘッダーと、見本の図形に格納する.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S> void MainPage::join_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* sample = m_sample_shape;
		const float value = static_cast<float>(args.NewValue());
		join_set_slider_header<U, S>(value);
		if constexpr (U == UNDO_OP::STROKE_JOIN_LIMIT) {
			sample->set_stroke_join_limit(value * SLIDER_STEP + 1.0f);
		}
		else if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			sample->set_stroke_width(value * SLIDER_STEP);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	IAsyncAction MainPage::join_limit_click_async(IInspectable const& sender, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		float value;
		m_sample_sheet.get_stroke_join_limit(value);
		const float val0 = (value - 1.0F) / SLIDER_STEP;
		sample_slider_0().Value(val0);
		sample_slider_0().Visibility(UI_VISIBLE);
		float s_width;
		m_sample_sheet.get_stroke_width(s_width);
		const float val1 = s_width / SLIDER_STEP;
		sample_slider_1().Value(val1);
		sample_slider_1().Visibility(UI_VISIBLE);
		join_set_slider_header<UNDO_OP::STROKE_JOIN_LIMIT, 0>(val0);
		join_set_slider_header<UNDO_OP::STROKE_WIDTH, 1>(val1);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::join_set_slider<UNDO_OP::STROKE_JOIN_LIMIT, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::join_set_slider<UNDO_OP::STROKE_WIDTH, 1> });
		m_sample_type = SAMP_TYPE::JOIN;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_line_join")));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_limit;
			float sample_width;
			m_sample_shape->get_stroke_join_limit(sample_limit);
			m_sample_shape->get_stroke_width(sample_width);
			if (undo_push_set<UNDO_OP::STROKE_JOIN_LIMIT>(sample_limit) ||
				undo_push_set<UNDO_OP::STROKE_WIDTH>(sample_width)) {
				undo_push_null();
				undo_menu_enable();
				sheet_draw();
			}
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().Visibility(UI_COLLAPSED);
		sample_slider_1().ValueChanged(slider_1_token);
		co_return;
	}
}