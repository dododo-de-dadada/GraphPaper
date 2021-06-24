#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t DLG_TITLE[] = L"str_line_join";

	// 線枠メニューの「端の種類」が選択された.
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
		m_sheet_main.get_cap_style(old_value);
		if (undo_push_set<UNDO_OP::CAP_STYLE>(new_value)) {
			undo_push_null();
			undo_is_enable();
			sheet_draw();
		}
	}

	// 線枠メニューの「端の種類」に印をつける.
	// s_cap	端の形式
	void MainPage::cap_style_is_checked(const CAP_STYLE& value)
	{
		rmfi_cap_flat().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT }));
		rmfi_cap_flat_2().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT }));
		rmfi_cap_square().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE }));
		rmfi_cap_square_2().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE }));
		rmfi_cap_round().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND }));
		rmfi_cap_round_2().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND }));
		rmfi_cap_triangle().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE }));
		rmfi_cap_triangle_2().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE }));
	}

	// 線枠メニューの「つなぎの種類」>「額ぶちの制限」が選択された.
	IAsyncAction MainPage::join_limit_click_async(IInspectable const& sender, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_sample_sheet.set_attr_to(&m_sheet_main);
		float j_limit;
		m_sample_sheet.get_join_limit(j_limit);
		j_limit -= 1.0f;

		sample_slider_0().Maximum(MAX_VALUE);
		sample_slider_0().TickFrequency(TICK_FREQ);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(j_limit);
		sample_slider_0().Visibility(UI_VISIBLE);
		join_slider_set_header<UNDO_OP::JOIN_LIMIT, 0>(j_limit);

		float s_width;
		m_sample_sheet.get_stroke_width(s_width);

		sample_slider_1().Maximum(MAX_VALUE);
		sample_slider_1().TickFrequency(TICK_FREQ);
		sample_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_1().Value(s_width);
		sample_slider_1().Visibility(UI_VISIBLE);
		join_slider_set_header<UNDO_OP::STROKE_WIDTH, 1>(s_width);

		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::join_slider_value_changed<UNDO_OP::JOIN_LIMIT, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::join_slider_value_changed<UNDO_OP::STROKE_WIDTH, 1> });
		m_sample_type = SAMPLE_TYPE::JOIN;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_line_join")));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_limit;
			float sample_width;
			m_sample_shape->get_join_limit(sample_limit);
			m_sample_shape->get_stroke_width(sample_width);
			if (undo_push_set<UNDO_OP::JOIN_LIMIT>(sample_limit) ||
				undo_push_set<UNDO_OP::STROKE_WIDTH>(sample_width)) {
				undo_push_null();
				undo_is_enable();
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

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S> void MainPage::join_slider_set_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		if constexpr (U == UNDO_OP::JOIN_LIMIT && S == 0) {
			constexpr size_t LEN = 32;
			wchar_t buf[LEN + 1];
			swprintf_s(buf, LEN, L"%.1f", value + 1.0f);
			const auto text = ResourceLoader::GetForCurrentView().GetString(L"str_join_limit") + L": " + buf;
			sample_slider_0().Header(box_value(text));
		}
		else if constexpr (U == UNDO_OP::STROKE_WIDTH && S == 1) {
			constexpr size_t LEN = 32;
			wchar_t buf[LEN + 1];
			conv_len_to_str<LEN_UNIT_SHOW>(m_misc_len_unit, value, m_sheet_dx.m_logical_dpi, m_sheet_main.m_grid_base + 1.0f, buf);
			const auto text = ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " + buf;
			sample_slider_1().Header(box_value(text));
		}
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S> void MainPage::join_slider_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::JOIN_LIMIT && S == 0) {
			const float value = static_cast<float>(args.NewValue());
			join_slider_set_header<U, S>(value);
			m_sample_shape->set_join_limit(value + 1.0f);
		}
		else if constexpr (U == UNDO_OP::STROKE_WIDTH && S == 1) {
			const float value = static_cast<float>(args.NewValue());
			join_slider_set_header<U, S>(value);
			m_sample_shape->set_stroke_width(value);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// 線枠メニューの「つなぎの種類」が選択された.
	void MainPage::join_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		D2D1_LINE_JOIN new_value;
		if (sender == rmfi_join_bevel() || sender == rmfi_join_bevel_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;
			mfi_join_limit().IsEnabled(false);
			mfi_join_limit_2().IsEnabled(false);
		}
		else if (sender == rmfi_join_miter() || sender == rmfi_join_miter_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER;
			mfi_join_limit().IsEnabled(true);
			mfi_join_limit_2().IsEnabled(true);
		}
		else if (sender == rmfi_join_m_or_b() || sender == rmfi_join_m_or_b_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL;
			mfi_join_limit().IsEnabled(true);
			mfi_join_limit_2().IsEnabled(true);
		}
		else if (sender == rmfi_join_round() || sender == rmfi_join_round_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND;
			mfi_join_limit().IsEnabled(false);
			mfi_join_limit_2().IsEnabled(false);
		}
		else {
			return;
		}
		D2D1_LINE_JOIN old_value;
		m_sheet_main.get_join_style(old_value);
		if (undo_push_set<UNDO_OP::JOIN_STYLE>(new_value)) {
			undo_push_null();
			undo_is_enable();
			sheet_draw();
		}
	}

	// 線枠メニューの「つなぎ」に印をつける.
	// s_join	線のつなぎ
	void MainPage::join_style_is_checked(const D2D1_LINE_JOIN value)
	{
		rmfi_join_bevel().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL);
		rmfi_join_bevel_2().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL);
		rmfi_join_miter().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
		rmfi_join_miter_2().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
		rmfi_join_m_or_b().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL);
		rmfi_join_m_or_b_2().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL);
		rmfi_join_round().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND);
		rmfi_join_round_2().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND);
		mfi_join_limit().IsEnabled(rmfi_join_miter().IsChecked() || rmfi_join_m_or_b().IsChecked());
		mfi_join_limit_2().IsEnabled(rmfi_join_miter_2().IsChecked() || rmfi_join_m_or_b_2().IsChecked());
	}

}