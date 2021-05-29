#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 線枠メニューの「形式」に印をつける.
	// j_style	破線の種別
	void MainPage::join_style_check_menu(const D2D1_LINE_JOIN j_style)
	{
		// コードビハインドではグループ名による切り替えが効かない？
		if (rmfi_join_bevel().IsChecked()) {
			rmfi_join_bevel().IsChecked(false);
		}
		if (rmfi_join_bevel_2().IsChecked()) {
			rmfi_join_bevel_2().IsChecked(false);
		}
		if (rmfi_join_miter().IsChecked()) {
			rmfi_join_miter().IsChecked(false);
		}
		if (rmfi_join_miter_2().IsChecked()) {
			rmfi_join_miter_2().IsChecked(false);
		}
		if (rmfi_join_m_or_b().IsChecked()) {
			rmfi_join_m_or_b().IsChecked(false);
		}
		if (rmfi_join_m_or_b_2().IsChecked()) {
			rmfi_join_m_or_b_2().IsChecked(false);
		}
		if (rmfi_join_round().IsChecked()) {
			rmfi_join_round().IsChecked(false);
		}
		if (rmfi_join_round_2().IsChecked()) {
			rmfi_join_round_2().IsChecked(false);
		}
		if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
			rmfi_join_bevel().IsChecked(true);
			rmfi_join_bevel_2().IsChecked(true);
		}
		else if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER) {
			rmfi_join_miter().IsChecked(true);
			rmfi_join_miter_2().IsChecked(true);
		}
		else if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
			rmfi_join_m_or_b().IsChecked(true);
			rmfi_join_m_or_b_2().IsChecked(true);
		}
		else if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
			rmfi_join_round().IsChecked(true);
			rmfi_join_round_2().IsChecked(true);
		}
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
		undo_push_set<UNDO_OP::STROKE_JOIN_STYLE>(new_value);
	}

	constexpr float SLIDER_STEP = 0.5f;
	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S> void MainPage::join_set_slider_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == UNDO_OP::STROKE_JOIN_LIMIT) {
			constexpr size_t LEN = 32;
			wchar_t buf[LEN + 1];
			swprintf_s(buf, LEN, L"%.1f", value * SLIDER_STEP + 1.0F);
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			hdr = r_loader.GetString(L"str_stroke_join_limit") + L": " + buf;
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			const double dpi = m_sheet_dx.m_logical_dpi;
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			const double g_len = g_base + 1.0;
			constexpr size_t LEN = 32;
			wchar_t buf[LEN + 1];
			conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), value * SLIDER_STEP, dpi, g_len, buf);
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			hdr = r_loader.GetString(L"str_stroke_width") + L": " + buf;
		}
		if constexpr (S == 0) {
			sample_slider_0().Header(box_value(hdr));
		}
		if constexpr (S == 1) {
			sample_slider_1().Header(box_value(hdr));
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

		m_sample_sheet.set_to(&m_sheet_main);
		float s_limit;
		m_sample_sheet.get_stroke_join_limit(s_limit);
		const float val0 = (s_limit - 1.0F) / SLIDER_STEP;
		sample_slider_0().Value(val0);
		sample_slider_0().Visibility(VISIBLE);
		float s_width;
		m_sample_sheet.get_stroke_width(s_width);
		const float val1 = s_width / SLIDER_STEP;
		sample_slider_1().Value(val1);
		sample_slider_1().Visibility(VISIBLE);
		join_set_slider_header<UNDO_OP::STROKE_JOIN_LIMIT, 0>(val0);
		join_set_slider_header<UNDO_OP::STROKE_WIDTH, 1>(val1);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::join_set_slider<UNDO_OP::STROKE_JOIN_LIMIT, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::join_set_slider<UNDO_OP::STROKE_WIDTH, 1> });
		m_sample_type = SAMP_TYPE::JOIN;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_strole_join")));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_value;
			m_sample_shape->get_stroke_join_limit(sample_value);
			undo_push_set<UNDO_OP::STROKE_JOIN_LIMIT>(sample_value);
			sheet_draw();
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().Visibility(COLLAPSED);
		sample_slider_1().ValueChanged(slider_1_token);
		co_return;
	}
}