﻿//-------------------------------
// MainPage.cpp
// メインページの作成と, ファイルメニューの「新規」と「終了」
//-------------------------------
#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;
	using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
	using winrt::Windows::Foundation::Rect;
	using winrt::Windows::UI::Core::Preview::SystemNavigationManagerPreview;
	using winrt::Windows::UI::Text::Core::CoreTextRange;
	using winrt::Windows::UI::Text::Core::CoreTextTextRequest;
	using winrt::Windows::UI::Text::Core::CoreTextSelectionRequest;
	using winrt::Windows::UI::Text::Core::CoreTextLayoutRequest;
	using winrt::Windows::UI::Xaml::Application;
	using winrt::Windows::UI::Xaml::Window;
	using winrt::Windows::UI::Xaml::Controls::Control;
	using winrt::Windows::UI::Xaml::Media::GeneralTransform;
	using winrt::Windows::UI::Core::CoreCursorType;
	using winrt::Windows::UI::ViewManagement::InputPane;
	using winrt::Windows::UI::Xaml::Visibility;

	// 書式文字列
	constexpr auto FMT_INCH = L"%.3lf";	// インチ単位の書式
	constexpr auto FMT_INCH_UNIT = L"%.3lf \u33CC";	// インチ単位の書式
	constexpr auto FMT_MILLI = L"%.3lf";	// ミリメートル単位の書式
	constexpr auto FMT_MILLI_UNIT = L"%.3lf \u339C";	// ミリメートル単位の書式
	constexpr auto FMT_POINT = L"%.2lf";	// ポイント単位の書式
	constexpr auto FMT_POINT_UNIT = L"%.2lf pt";	// ポイント単位の書式
	constexpr auto FMT_PIXEL = L"%.1lf";	// ピクセル単位の書式
	constexpr auto FMT_PIXEL_UNIT = L"%.1lf px";	// ピクセル単位の書式
	constexpr auto FMT_ZOOM = L"%.lf%%";	// 倍率の書式
	constexpr auto FMT_GRID = L"%.3lf";	// グリッド単位の書式
	constexpr auto FMT_GRID_UNIT = L"%.3lf grid";	// グリッド単位の書式
	static const auto& CURS_WAIT = CoreCursor(CoreCursorType::Wait, 0);	// 左右カーソル

	// 待機カーソルを表示する.
	// 戻り値	それまで表示されていたカーソル.
	const CoreCursor wait_cursor_show(void)
	{
		const CoreWindow& core_win = Window::Current().CoreWindow();
		const CoreCursor& prev_cur = core_win.PointerCursor();
		if (prev_cur.Type() != CURS_WAIT.Type()) {
			core_win.PointerCursor(CURS_WAIT);
		}
		return prev_cur;
	}

	// 色成分を文字列に変換する.
	// col_code	色成分の記法
	// col_comp	色成分の値
	// text_len	文字列の最大長 ('\0' を含む長さ)
	// text_buf	文字列の配列 [t_len]
	void conv_col_to_str(const COLOR_CODE col_code, const double col_comp, const size_t text_len, wchar_t text_buf[]) noexcept
	{
		// 色の基数が 10 進数か判定する.
		if (col_code == COLOR_CODE::DEC) {
			swprintf_s(text_buf, text_len, L"%.0lf", std::round(col_comp));
		}
		// 色の基数が 16 進数か判定する.
		else if (col_code == COLOR_CODE::HEX) {
			swprintf_s(text_buf, text_len, L"x%02X", static_cast<uint32_t>(std::round(col_comp)));
		}
		// 色の基数が実数か判定する.
		else if (col_code == COLOR_CODE::REAL) {
			swprintf_s(text_buf, text_len, L"%.4lf", col_comp / COLOR_MAX);
		}
		// 色の基数がパーセントか判定する.
		else if (col_code == COLOR_CODE::PCT) {
			swprintf_s(text_buf, text_len, L"%.1lf%%", col_comp * 100.0 / COLOR_MAX);
		}
		else {
			swprintf_s(text_buf, text_len, L"?");
		}
	}

	// 長さを文字列に変換する.
	// B	単位付加フラグ
	// len_unit	長さの単位
	// len	ピクセル単位の長さ
	// dpi	DPI
	// grid_len	方眼の大きさ
	// text_len	文字列の最大長 ('\0' を含む長さ)
	// text_buf	文字列の配列
	template <bool B>
	void conv_len_to_str(const LEN_UNIT len_unit, const double len, const double dpi, const double grid_len, const uint32_t text_len, wchar_t* text_buf) noexcept
	{
		// 長さの単位がピクセルか判定する.
		if (len_unit == LEN_UNIT::PIXEL) {
			if constexpr (B) {
				swprintf_s(text_buf, text_len, FMT_PIXEL_UNIT, len);
			}
			else {
				swprintf_s(text_buf, text_len, FMT_PIXEL, len);
			}
		}
		// 長さの単位がインチか判定する.
		else if (len_unit == LEN_UNIT::INCH) {
			if constexpr (B) {
				swprintf_s(text_buf, text_len, FMT_INCH_UNIT, len / dpi);
			}
			else {
				swprintf_s(text_buf, text_len, FMT_INCH, len / dpi);
			}
		}
		// 長さの単位がミリメートルか判定する.
		else if (len_unit == LEN_UNIT::MILLI) {
			if constexpr (B) {
				swprintf_s(text_buf, text_len, FMT_MILLI_UNIT, len * MM_PER_INCH / dpi);
			}
			else {
				swprintf_s(text_buf, text_len, FMT_MILLI, len * MM_PER_INCH / dpi);
			}
		}
		// 長さの単位がポイントか判定する.
		else if (len_unit == LEN_UNIT::POINT) {
			if constexpr (B) {
				swprintf_s(text_buf, text_len, FMT_POINT_UNIT, len * PT_PER_INCH / dpi);
			}
			else {
				swprintf_s(text_buf, text_len, FMT_POINT, len * PT_PER_INCH / dpi);
			}
		}
		// 長さの単位が方眼か判定する.
		else if (len_unit == LEN_UNIT::GRID) {
			if constexpr (B) {
				swprintf_s(text_buf, text_len, FMT_GRID_UNIT, len / grid_len);
			}
			else {
				swprintf_s(text_buf, text_len, FMT_GRID, len / grid_len);
			}
		}
		else {
			swprintf_s(text_buf, text_len, L"?");
		}
	}

	// 長さを文字列に変換する (単位なし).
	template void conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(const LEN_UNIT len_unit, const double len_val, const double dpi, const double g_len, const uint32_t t_len, wchar_t* t_buf) noexcept;
	// 長さを文字列に変換する (単位つき).
	template void conv_len_to_str<LEN_UNIT_NAME_APPEND>(const LEN_UNIT len_unit, const double len_val, const double dpi, const double g_len, const uint32_t t_len, wchar_t* t_buf) noexcept;

	// 長さををピクセル単位の値に変換する.
	// len_unit	長さの単位
	// len	長さの値
	// dpi	DPI
	// grid_len	方眼の大きさ
	// 戻り値	ピクセル単位の値
	double conv_len_to_pixel(const LEN_UNIT len_unit, const double len, const double dpi, const double grid_len) noexcept
	{
		double ret;

		if (len_unit == LEN_UNIT::INCH) {
			ret = len * dpi;
		}
		else if (len_unit == LEN_UNIT::MILLI) {
			ret = len * dpi / MM_PER_INCH;
		}
		else if (len_unit == LEN_UNIT::POINT) {
			ret = len * dpi / PT_PER_INCH;
		}
		else if (len_unit == LEN_UNIT::GRID) {
			ret = len * grid_len;
		}
		else {
			ret = len;
		}
		return std::round(2.0 * ret) * 0.5;
	}

	// 文字列の選択範囲の文字列を得る.
	winrt::hstring MainPage::core_text_substr(void) const noexcept
	{
		const auto len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
		const auto s = min(m_main_sheet.m_select_start, end);
		const auto e = max(m_main_sheet.m_select_start, end);
		return winrt::hstring{ m_core_text_shape->m_text + s, e - s };
	}

	// 文字列の選択範囲の文字を削除する.
	void MainPage::core_text_delete(void) noexcept
	{
		const auto len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
		const auto start = min(m_main_sheet.m_select_start, len);
		// 選択範囲があるなら
		if (end != start) {
			undo_push_null();
			m_ustack_undo.push_back(new UndoText2(m_core_text_shape, nullptr));
			main_draw();
		}

	}

	// 文字列の選択範囲に文字列を挿入する.
	void MainPage::core_text_insert(const wchar_t* ins_text, const uint32_t ins_len) noexcept
	{
		//const ShapeText* t = m_core_text_shape;
		const auto old_len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, old_len);
		const auto start = min(m_core_text_comp ? m_core_text_start : m_main_sheet.m_select_start, old_len);
		const auto s = min(start, end);
		const auto e = max(start, end);
		if (s < e) {
			if (!m_core_text_comp) {
				undo_push_null();
			}
			else {
				for (Undo* u = m_ustack_undo.back(); u != nullptr; u = m_ustack_undo.back()) {
					m_ustack_undo.pop_back();
					u->exec();
					delete u;
				}
			}
			m_ustack_undo.push_back(new UndoText2(m_core_text_shape, ins_text));
			undo_push_text_select(m_core_text_shape, s + ins_len, s + ins_len, false);
			main_draw();
		}
		else if (ins_len > 0) {
			if (!m_core_text_comp) {
				undo_push_null();
			}
			else {
				for (Undo* u = m_ustack_undo.back(); u != nullptr; u = m_ustack_undo.back()) {
					m_ustack_undo.pop_back();
					u->exec();
					delete u;
				}
			}
			m_ustack_undo.push_back(new UndoText2(m_core_text_shape, ins_text));
			undo_push_text_select(m_core_text_shape, s + ins_len, s + ins_len, false);
			main_draw();
		}
	}

	// 1 文字削除.
	void MainPage::core_text_del_c(const bool shift_key) noexcept
	{
		// シフトキー押下でなく選択範囲がなくキャレット位置が文末でないなら
		const auto len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
		const auto start = min(m_main_sheet.m_select_start, len);
		if (!shift_key && end == start && end < len) {
			undo_push_null();
			undo_push_text_select(m_core_text_shape, end, end + 1, false);
			m_ustack_undo.push_back(new UndoText2(m_core_text_shape, nullptr));
			main_draw();
		}
		// 選択範囲があるなら
		else if (end != start) {
			undo_push_null();
			m_ustack_undo.push_back(new UndoText2(m_core_text_shape, nullptr));
			main_draw();
		}
		winrt::Windows::UI::Text::Core::CoreTextRange modified_ran{
			static_cast<const int32_t>(start), static_cast<const int32_t>(end)
		};
		winrt::Windows::UI::Text::Core::CoreTextRange new_ran{
			static_cast<int32_t>(m_main_sheet.m_select_start),
				static_cast<int32_t>(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end)
		};
		m_core_text.NotifyTextChanged(modified_ran, 0, new_ran);
	}

	//-------------------------------
	// メインページを作成する.
	//-------------------------------
	int _debug_edit = 0;
	MainPage::MainPage(void)
	{
		// お約束.
		InitializeComponent();

		// スワップチェーンパネル右クリックのコンテキストメニューを設定する.
		mf_popup_menu().Opening([this](auto const&, auto const&) {
			if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID) {
				rmfi_popup_stroke_dash_solid().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(false);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH) {
				rmfi_popup_stroke_dash_dash().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(true);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT) {
				rmfi_popup_stroke_dash_dot().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(true);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT) {
				rmfi_popup_stroke_dash_dash_dot().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(true);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
				rmfi_popup_stroke_dash_dash_dot_dot().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(true);
			}
			if (equal(m_main_sheet.m_stroke_width, 0.0f)) {
				rmfi_popup_stroke_width_0px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 1.0f)) {
				rmfi_popup_stroke_width_1px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 2.0f)) {
				rmfi_popup_stroke_width_2px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 3.0f)) {
				rmfi_popup_stroke_width_3px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 4.0f)) {
				rmfi_popup_stroke_width_4px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 8.0f)) {
				rmfi_popup_stroke_width_8px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 12.0f)) {
				rmfi_popup_stroke_width_12px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 16.0f)) {
				rmfi_popup_stroke_width_16px().IsChecked(true);
			}
			else {
				rmfi_popup_stroke_width_other().IsChecked(true);
			}
			if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT) {
				rmfi_popup_stroke_cap_flat().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
				rmfi_popup_stroke_cap_square().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
				rmfi_popup_stroke_cap_round().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
				rmfi_popup_stroke_cap_triangle().IsChecked(true);
			}
			if (m_main_sheet.m_arrow_style == ARROW_STYLE::ARROW_NONE) {
				rmfi_popup_stroke_arrow_none().IsChecked(true);
				mfi_popup_stroke_arrow_size().IsEnabled(false);
			}
			else if (m_main_sheet.m_arrow_style == ARROW_STYLE::ARROW_OPENED) {
				rmfi_popup_stroke_arrow_opened().IsChecked(true);
				mfi_popup_stroke_arrow_size().IsEnabled(true);
			}
			else if (m_main_sheet.m_arrow_style == ARROW_STYLE::ARROW_FILLED) {
				rmfi_popup_stroke_arrow_filled().IsChecked(true);
				mfi_popup_stroke_arrow_size().IsEnabled(true);
			}

			if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				rmfi_popup_stroke_join_bevel().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER) {
				rmfi_popup_stroke_join_miter().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				rmfi_popup_stroke_join_miter_or_bevel().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				rmfi_popup_stroke_join_round().IsChecked(true);
			}

			if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_CONDENSED) {
				rmfi_popup_font_stretch_ultra_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_CONDENSED) {
				rmfi_popup_font_stretch_extra_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_CONDENSED) {
				rmfi_popup_font_stretch_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_CONDENSED) {
				rmfi_popup_font_stretch_semi_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL) {
				rmfi_popup_font_stretch_normal().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_EXPANDED) {
				rmfi_popup_font_stretch_semi_expanded().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXPANDED) {
				rmfi_popup_font_stretch_expanded().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_EXPANDED) {
				rmfi_popup_font_stretch_extra_expanded().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_EXPANDED) {
				rmfi_popup_font_stretch_ultra_expanded().IsChecked(true);
			}

			if (m_main_sheet.m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC) {
				rmfi_popup_font_style_italic().IsChecked(true);
			}
			else if (m_main_sheet.m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL) {
				rmfi_popup_font_style_normal().IsChecked(true);
			}
			else if (m_main_sheet.m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE) {
				rmfi_popup_font_style_oblique().IsChecked(true);
			}

			if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_THIN) {
				rmfi_popup_font_weight_thin().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_LIGHT) {
				rmfi_popup_font_weight_extra_light().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_LIGHT) {
				rmfi_popup_font_weight_light().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL) {
				rmfi_popup_font_weight_normal().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_MEDIUM) {
				rmfi_popup_font_weight_medium().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_SEMI_BOLD) {
				rmfi_popup_font_weight_semi_bold().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BOLD) {
				rmfi_popup_font_weight_bold().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BOLD) {
				rmfi_popup_font_weight_extra_bold().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BLACK) {
				rmfi_popup_font_weight_black().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BLACK) {
				rmfi_popup_font_weight_extra_black().IsChecked(true);
			}

			if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING) {
				rmfi_popup_text_align_left().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING) {
				rmfi_popup_text_align_right().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER) {
				rmfi_popup_text_align_center().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED) {
				rmfi_popup_text_align_just().IsChecked(true);
			}
			if (m_main_sheet.m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR) {
				rmfi_popup_text_align_top().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR) {
				rmfi_popup_text_align_bot().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER) {
				rmfi_popup_text_align_mid().IsChecked(true);
			}
			if (m_main_sheet.m_text_word_wrap == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_WRAP) {
				rmfi_popup_text_wrap().IsChecked(true);
			}
			else if (m_main_sheet.m_text_word_wrap == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_NO_WRAP) {
				rmfi_popup_text_no_wrap().IsChecked(true);
			}
			else if (m_main_sheet.m_text_word_wrap == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_CHARACTER) {
				rmfi_popup_text_wrap_char().IsChecked(true);
			}

			if (m_main_sheet.m_grid_emph.m_gauge_1 == 0 && m_main_sheet.m_grid_emph.m_gauge_2 == 0) {
				rmfi_popup_grid_emph_1().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_emph.m_gauge_1 != 0 && m_main_sheet.m_grid_emph.m_gauge_2 == 0) {
				rmfi_popup_grid_emph_2().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_emph.m_gauge_1 != 0 && m_main_sheet.m_grid_emph.m_gauge_2 != 0) {
				rmfi_popup_grid_emph_3().IsChecked(true);
			}

			if (m_main_sheet.m_grid_show == GRID_SHOW::BACK) {
				rmfi_popup_grid_show_back().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_show == GRID_SHOW::FRONT) {
				rmfi_popup_grid_show_front().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_show == GRID_SHOW::HIDE) {
				rmfi_popup_grid_show_hide().IsChecked(true);
			}

			if (equal(m_main_scale, 1.0f)) {
				rmfi_popup_sheet_zoom_100().IsChecked(true);
			}
			else if (equal(m_main_scale, 1.5f)) {
				rmfi_popup_sheet_zoom_150().IsChecked(true);
			}
			else if (equal(m_main_scale, 2.0f)) {
				rmfi_popup_sheet_zoom_200().IsChecked(true);
			}
			else if (equal(m_main_scale, 3.0f)) {
				rmfi_popup_sheet_zoom_300().IsChecked(true);
			}
			else if (equal(m_main_scale, 4.0f)) {
				rmfi_popup_sheet_zoom_400().IsChecked(true);
			}
			else if (equal(m_main_scale, 0.75f)) {
				rmfi_popup_sheet_zoom_075().IsChecked(true);
			}
			else if (equal(m_main_scale, 0.5f)) {
				rmfi_popup_sheet_zoom_050().IsChecked(true);
			}
			else if (equal(m_main_scale, 0.25f)) {
				rmfi_popup_sheet_zoom_025().IsChecked(true);
			}

			if (m_background_show) {
				tmfi_popup_background_show().IsChecked(true);
			}
			else {
				tmfi_popup_background_show().IsChecked(false);
			}
			if (equal(m_background_color, COLOR_BLACK)) {
				rmfi_popup_background_black().IsChecked(true);
			}
			else {
				rmfi_popup_background_white().IsChecked(true);
			}

			uint32_t undeleted_cnt = 0;	// 消去フラグがない図形の数
			uint32_t selected_cnt = 0;	// 選択された図形の数
			uint32_t selected_group_cnt = 0;	// 選択されたグループ図形の数
			uint32_t runlength_cnt = 0;	// 選択された図形の連続の数
			uint32_t selected_text_cnt = 0;	// 選択された文字列図形の数
			uint32_t text_cnt = 0;	// 文字列図形の数
			uint32_t selected_image_cnt = 0;	// 選択された画像図形の数
			uint32_t selected_arc_cnt = 0;	// 選択された円弧図形の数
			uint32_t selected_poly_open_cnt = 0;	// 選択された開いた多角形図形の数
			uint32_t selected_poly_close_cnt = 0;	// 選択された閉じた多角形図形の数
			uint32_t selected_exist_cap_cnt = 0;	// 選択された端をもつ図形の数
			bool fore_selected = false;	// 最前面の図形の選択フラグ
			bool back_selected = false;	// 最背面の図形の選択フラグ
			bool prev_selected = false;	// ひとつ背面の図形の選択フラグ
			slist_count(
				m_main_sheet.m_shape_list,
				undeleted_cnt,
				selected_cnt,
				selected_group_cnt,
				runlength_cnt,
				selected_text_cnt,
				text_cnt,
				selected_image_cnt,
				selected_arc_cnt,
				selected_poly_open_cnt,
				selected_poly_close_cnt,
				selected_exist_cap_cnt,
				fore_selected,
				back_selected,
				prev_selected
			);
			// 選択された図形がひとつ以上ある場合.
			const auto exists_selected = (selected_cnt > 0);
			// 選択された文字列がひとつ以上ある場合.
			const auto exists_selected_text = (selected_text_cnt > 0);
			// 文字列がひとつ以上ある場合.
			const auto exists_text = (text_cnt > 0);
			// 選択された画像がひとつ以上ある場合.
			const auto exists_selected_image = (selected_image_cnt > 0);
			// 選択された円弧がひとつ以上ある場合.
			const auto exists_selected_arc = (selected_arc_cnt > 0);
			// 選択された開いた多角形がひとつ以上ある場合.
			const auto exists_selected_poly_open = (selected_poly_open_cnt > 0);
			// 選択された閉じた多角形がひとつ以上ある場合.
			const auto exists_selected_poly_close = (selected_poly_close_cnt > 0);
			// 選択されてない図形がひとつ以上ある場合, または選択されてない文字がひとつ以上ある場合.
			uint32_t text_unselected_char_cnt;
			if (m_core_text_shape != nullptr) {
				const auto len = m_core_text_shape->get_text_len();
				const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, m_core_text_shape->get_text_len());
				text_unselected_char_cnt = len - (end - m_main_sheet.m_select_start);
			}
			else {
				text_unselected_char_cnt = 0;
			}
			const auto exists_unselected = (selected_cnt < undeleted_cnt || text_unselected_char_cnt > 0);
			// 選択された図形がふたつ以上ある場合.
			const auto exists_selected_2 = (selected_cnt > 1);
			// 選択されたグループがひとつ以上ある場合.
			const auto exists_selected_group = (selected_group_cnt > 0);
			// 選択された端のある図形がひとつ以上ある場合.
			const auto exists_selected_cap = (selected_exist_cap_cnt > 0);
			// 前面に配置可能か判定する.
			// 1. 複数のランレングスがある.
			// 2. または, 少なくとも 1 つは選択された図形があり, 
			//    かつ最前面の図形は選択されいない.
			const auto enable_forward = (runlength_cnt > 1 || (exists_selected && !fore_selected));
			// 背面に配置可能か判定する.
			// 1. 複数のランレングスがある.
			// 2. または, 少なくとも 1 つは選択された図形があり, 
			//    かつ最背面の図形は選択されいない.
			const auto enable_backward = (runlength_cnt > 1 || (exists_selected && !back_selected));
			const auto& dp_view = Clipboard::GetContent();
			const bool exists_clipboard_data = (dp_view.Contains(CLIPBOARD_FORMAT_SHAPES) ||
				dp_view.Contains(StandardDataFormats::Text()) || dp_view.Contains(StandardDataFormats::Bitmap()));

			// 元に戻すメニューの表示/非表示を設定する.
			mfi_popup_undo().Visibility(m_ustack_undo.size() > 0 ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_redo().Visibility(m_ustack_redo.size() > 0 ? Visibility::Visible : Visibility::Collapsed);

			// 元に戻すメニューとカット＆ペーストメニュー間のセパレーターの表示/非表示を設定する.
			bool sepa_over = false;	// セパレーターより上の項目フラグ
			bool sepa_under = false;	// セパレーターより下の項目フラグ
			sepa_over = (m_ustack_undo.size() > 0 || m_ustack_redo.size() > 0);
			sepa_under = (exists_selected || exists_clipboard_data);
			mfs_popup_sepa_undo_xcvd().Visibility(sepa_over && sepa_under ? Visibility::Visible : Visibility::Collapsed);

			// カット＆ペーストメニューの表示/非表示を設定する.
			mfi_popup_xcvd_cut().Visibility(exists_selected ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_xcvd_copy().Visibility(exists_selected ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_xcvd_paste().Visibility(exists_clipboard_data ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_xcvd_delete().Visibility(exists_selected ? Visibility::Visible : Visibility::Collapsed);

			// カット＆ペーストメニューと並び替えメニュー間のセパレーターの表示/非表示を設定する.
			sepa_over = sepa_over || sepa_under;
			sepa_under = (exists_unselected || enable_forward || enable_backward);
			mfs_popup_sepa_xcvd_select().Visibility(sepa_over && sepa_under ? Visibility::Visible : Visibility::Collapsed);

			// 並び替えメニューの表示/非表示を設定する.
			mfi_popup_select_all().Visibility(exists_unselected ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_bring_forward().Visibility(enable_forward ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_bring_to_front().Visibility(enable_forward ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_send_to_back().Visibility(enable_backward ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_send_backward().Visibility(enable_backward ? Visibility::Visible : Visibility::Collapsed);
			mfsi_popup_order().Visibility((exists_unselected || enable_forward || enable_backward) ? Visibility::Visible : Visibility::Collapsed);

			// 並び替えメニューとグループ操作メニュー間のセパレーターの表示/非表示を設定する.
			sepa_over = sepa_over || sepa_under;
			sepa_under = (exists_selected_2 || exists_selected_group);
			mfs_popup_sepa_select_group().Visibility(sepa_over && sepa_under ? Visibility::Visible : Visibility::Collapsed);

			// グループ操作メニューの表示/非表示を設定する.
			mfi_popup_group().Visibility(exists_selected_2 ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_ungroup().Visibility(exists_selected_group ? Visibility::Visible : Visibility::Collapsed);

			// グループメニューと図形編集メニュー間のセパレーターの表示/非表示を設定する.
			sepa_over = sepa_over || sepa_under;
			sepa_under = (exists_selected_cap || exists_selected_poly_close || exists_selected_text || exists_text || exists_selected_image);
			mfs_popup_sepa_group_edit().Visibility(sepa_over && sepa_under ? Visibility::Visible : Visibility::Collapsed);

			// 図形編集メニューの表示/非表示を設定する.
			mfi_popup_reverse_path().Visibility(exists_selected_cap ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_open_close_polygon().Visibility(exists_selected_poly_close || exists_selected_poly_open ? Visibility::Visible : Visibility::Collapsed);
			//mfi_popup_edit_text().Visibility(exists_selected_text ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_find_text().Visibility(exists_text ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_revert_image().Visibility(exists_selected_image ? Visibility::Visible : Visibility::Collapsed);
			//m_list_sel_cnt = selected_cnt;

			winrt::Windows::UI::Xaml::Visibility menu_prop;	// 図形属性メニューの表示/非表示
			winrt::Windows::UI::Xaml::Visibility menu_font;	// 書体メニューの表示/非表示
			winrt::Windows::UI::Xaml::Visibility menu_text;	// 文字列属性メニューの表示/非表示
			winrt::Windows::UI::Xaml::Visibility menu_image;	// 画像メニューの表示/非表示
			winrt::Windows::UI::Xaml::Visibility menu_layout;	// レイアウトメニューの表示/非表示

			// 押された図形が空なら, レイアウトメニューを表示に, それ以外を非表示に設定する.
			if (m_event_shape_pressed == nullptr) {
				menu_prop = Visibility::Collapsed;
				menu_font = Visibility::Collapsed;
				menu_text = Visibility::Collapsed;
				menu_image = Visibility::Collapsed;
				menu_layout = Visibility::Visible;
			}
			else {
				// 押された図形の属性値を表示に格納する.
				m_main_sheet.set_attr_to(m_event_shape_pressed);

				// 押された部位が文字列なら, 書体と文字列属性メニューを表示に, それ以外を非表示に設定する.
				if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
					menu_prop = Visibility::Collapsed;
					menu_font = Visibility::Visible;
					menu_text = Visibility::Visible;
					menu_image = Visibility::Collapsed;
					menu_layout = Visibility::Collapsed;
				}
				// 押された部位が画像なら, 画像メニューを表示に, それ以外を非表示に設定する.
				else if (typeid(*m_event_shape_pressed) == typeid(ShapeImage)) {
					menu_prop = Visibility::Collapsed;
					menu_font = Visibility::Collapsed;
					menu_text = Visibility::Collapsed;
					menu_image = Visibility::Visible;
					menu_layout = Visibility::Collapsed;
				}
				// 押された部位が定規なら, 図形属性と文字列属性メニューを表示に, それ以外を非表示に設定する.
				else if (typeid(*m_event_shape_pressed) == typeid(ShapeRuler)) {
					menu_prop = Visibility::Visible;
					menu_font = Visibility::Visible;
					menu_text = Visibility::Collapsed;
					menu_image = Visibility::Collapsed;
					menu_layout = Visibility::Collapsed;
				}
				// 上記以外なら, 図形属性メニューを表示に, それ以外を非表示に設定する.
				else {
					menu_prop = Visibility::Visible;
					menu_font = Visibility::Collapsed;
					menu_text = Visibility::Collapsed;
					menu_image = Visibility::Collapsed;
					menu_layout = Visibility::Collapsed;
				}
			}

			mfsi_popup_stroke_dash().Visibility(menu_prop);
			mfi_popup_stroke_dash_pat().Visibility(menu_prop);
			mfsi_popup_stroke_width().Visibility(menu_prop);
			if (menu_prop == Visibility::Visible && m_event_shape_pressed->exist_cap()) {
				mfsi_popup_stroke_cap().Visibility(Visibility::Visible);
				mfs_popup_sepa_stroke_arrow().Visibility(Visibility::Visible);
				mfsi_popup_stroke_arrow().Visibility(Visibility::Visible);
				mfi_popup_stroke_arrow_size().Visibility(Visibility::Visible);
			}
			else {
				mfsi_popup_stroke_cap().Visibility(Visibility::Collapsed);
				mfs_popup_sepa_stroke_arrow().Visibility(Visibility::Collapsed);
				mfsi_popup_stroke_arrow().Visibility(Visibility::Collapsed);
				mfi_popup_stroke_arrow_size().Visibility(Visibility::Collapsed);
			}
			if (menu_prop == Visibility::Visible && m_event_shape_pressed->exist_join()) {
				mfsi_popup_stroke_join().Visibility(Visibility::Visible);
			}
			else {
				mfsi_popup_stroke_join().Visibility(Visibility::Collapsed);
			}
			mfs_popup_sepa_arrow_color().Visibility(menu_prop);
			mfi_popup_stroke_color().Visibility(menu_prop);
			mfi_popup_fill_color().Visibility(menu_prop);

			mfi_popup_font_family().Visibility(menu_font);
			mfi_popup_font_size().Visibility(menu_font);
			mfsi_popup_font_weight().Visibility(menu_font);
			mfsi_popup_font_stretch().Visibility(menu_font);
			mfsi_popup_font_style().Visibility(menu_font);
			sepa_over = (menu_font == Visibility::Visible);
			sepa_under = (menu_text == Visibility::Visible);
			mfs_popup_sepa_font_text().Visibility(sepa_over && sepa_under ? Visibility::Visible : Visibility::Collapsed);
			mfsi_popup_text_align_horz().Visibility(menu_text);
			mfsi_popup_text_align_vert().Visibility(menu_text);
			mfi_popup_text_line_sp().Visibility(menu_text);
			mfi_popup_text_pad().Visibility(menu_text);
			mfsi_popup_text_wrap().Visibility(menu_text);
			mfi_popup_font_color().Visibility(menu_font);

			mfi_popup_revert_image().Visibility(menu_image);
			mfi_popup_image_opac().Visibility(menu_image);

			mfsi_popup_grid_show().Visibility(menu_layout);
			mfsi_popup_grid_len().Visibility(menu_layout);
			mfsi_popup_grid_emph().Visibility(menu_layout);
			mfi_popup_grid_color().Visibility(menu_layout);
			mfs_popup_sepa_grid_sheet().Visibility(menu_layout);
			mfi_popup_sheet_size().Visibility(menu_layout);
			mfi_popup_sheet_color().Visibility(menu_layout);
			mfsi_popup_sheet_zoom().Visibility(menu_layout);
			mfsi_popup_background_pattern().Visibility(menu_layout);
		});

		// ステータスバー右クリックのコンテキストメニューを設定する.
		mf_popup_status().Opening([this](auto const&, auto const&) {
			if (status_and(m_status_bar, STATUS_BAR::DRAW) == STATUS_BAR::DRAW) {
				tmfi_popup_status_bar_draw().IsChecked(true);
			}
			else {
				tmfi_popup_status_bar_draw().IsChecked(false);
			}
			if (status_and(m_status_bar, STATUS_BAR::GRID) == STATUS_BAR::GRID) {
				tmfi_popup_status_bar_grid().IsChecked(true);
			}
			else {
				tmfi_popup_status_bar_grid().IsChecked(false);
			}
			if (status_and(m_status_bar, STATUS_BAR::SHEET) == STATUS_BAR::SHEET) {
				tmfi_popup_status_bar_sheet().IsChecked(true);
			}
			else {
				tmfi_popup_status_bar_sheet().IsChecked(false);
			}
			if (status_and(m_status_bar, STATUS_BAR::POS) == STATUS_BAR::POS) {
				tmfi_popup_status_bar_pos().IsChecked(true);
			}
			else {
				tmfi_popup_status_bar_pos().IsChecked(false);
			}
			if (status_and(m_status_bar, STATUS_BAR::UNIT) == STATUS_BAR::UNIT) {
				tmfi_popup_status_bar_unit().IsChecked(true);
			}
			else {
				tmfi_popup_status_bar_unit().IsChecked(false);
			}
			if (status_and(m_status_bar, STATUS_BAR::ZOOM) == STATUS_BAR::ZOOM) {
				tmfi_popup_status_bar_zoom().IsChecked(true);
			}
			else {
				tmfi_popup_status_bar_zoom().IsChecked(false);
			}
		});

		// 作図メニューにフォーカスが移る直前.
		mbi_menu_drawing().GettingFocus([this](auto const&, auto const&) {
			if (m_drawing_tool == DRAWING_TOOL::SELECT) {
				rmfi_menu_selection_tool().IsChecked(true);
			}
			else if (m_drawing_tool == DRAWING_TOOL::RECT) {
				rmfi_menu_drawing_rect().IsChecked(true);
			}
			else if (m_drawing_tool == DRAWING_TOOL::RRECT) {
				rmfi_menu_drawing_rrect().IsChecked(true);
			}
			else if (m_drawing_tool == DRAWING_TOOL::POLY) {
				rmfi_menu_drawing_poly().IsChecked(true);
			}
			else if (m_drawing_tool == DRAWING_TOOL::ELLIPSE) {
				rmfi_menu_drawing_ellipse().IsChecked(true);
			}
			else if (m_drawing_tool == DRAWING_TOOL::ARC) {
				rmfi_menu_drawing_arc().IsChecked(true);
			}
			else if (m_drawing_tool == DRAWING_TOOL::LINE) {
				rmfi_menu_drawing_line().IsChecked(true);
			}
			else if (m_drawing_tool == DRAWING_TOOL::BEZIER) {
				rmfi_menu_drawing_bezier().IsChecked(true);
			}
			else if (m_drawing_tool == DRAWING_TOOL::TEXT) {
				rmfi_menu_drawing_text().IsChecked(true);
			}
			else if (m_drawing_tool == DRAWING_TOOL::RULER) {
				rmfi_menu_drawing_ruler().IsChecked(true);
			}
			else if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
				rmfi_menu_eyedropper().IsChecked(true);
			}
			if (m_drawing_poly_opt.m_vertex_cnt == 2) {
				rmfi_menu_drawing_poly_di().IsChecked(true);
			}
			else if (m_drawing_poly_opt.m_vertex_cnt == 3) {
				rmfi_menu_drawing_poly_tri().IsChecked(true);
			}
			else if (m_drawing_poly_opt.m_vertex_cnt == 4) {
				rmfi_menu_drawing_poly_quad().IsChecked(true);
			}
			else if (m_drawing_poly_opt.m_vertex_cnt == 5) {
				rmfi_menu_drawing_poly_pent().IsChecked(true);
			}
			else if (m_drawing_poly_opt.m_vertex_cnt == 6) {
				rmfi_menu_drawing_poly_hexa().IsChecked(true);
			}
			else if (m_drawing_poly_opt.m_vertex_cnt == 7) {
				rmfi_menu_drawing_poly_hept().IsChecked(true);
			}
			else if (m_drawing_poly_opt.m_vertex_cnt == 8) {
				rmfi_menu_drawing_poly_octa().IsChecked(true);
			}
			else if (m_drawing_poly_opt.m_vertex_cnt == 9) {
				rmfi_menu_drawing_poly_nona().IsChecked(true);
			}
			else if (m_drawing_poly_opt.m_vertex_cnt == 10) {
				rmfi_menu_drawing_poly_deca().IsChecked(true);
			}
			else if (m_drawing_poly_opt.m_vertex_cnt == 11) {
				rmfi_menu_drawing_poly_hendeca().IsChecked(true);
			}
			else if (m_drawing_poly_opt.m_vertex_cnt == 12) {
				rmfi_menu_drawing_poly_dodeca().IsChecked(true);
			}
			if (m_drawing_poly_opt.m_regular) {
				tmfi_tool_poly_regular().IsChecked(true);
			}
			else {
				tmfi_tool_poly_regular().IsChecked(false);
			}
			if (m_drawing_poly_opt.m_vertex_up) {
				tmfi_tool_poly_vertex_up().IsChecked(true);
			}
			else {
				tmfi_tool_poly_vertex_up().IsChecked(false);
			}
			if (m_drawing_poly_opt.m_end_closed) {
				tmfi_tool_poly_end_close().IsChecked(true);
			}
			else {
				tmfi_tool_poly_end_close().IsChecked(false);
			}
			if (m_drawing_poly_opt.m_clockwise) {
				tmfi_tool_poly_clockwise().IsChecked(true);
			}
			else {
				tmfi_tool_poly_clockwise().IsChecked(false);
			}
		});

		// 属性メニューにフォーカスが移る直前.
		mbi_menu_property().as<Control>().GettingFocus([this](auto const&, auto const&) {
			if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID) {
				rmfi_menu_stroke_dash_solid().IsChecked(true);
				mfi_menu_stroke_dash_pat().IsEnabled(false);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH) {
				rmfi_menu_stroke_dash_dash().IsChecked(true);
				mfi_menu_stroke_dash_pat().IsEnabled(true);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT) {
				rmfi_menu_stroke_dash_dot().IsChecked(true);
				mfi_menu_stroke_dash_pat().IsEnabled(true);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT) {
				rmfi_menu_stroke_dash_dash_dot().IsChecked(true);
				mfi_menu_stroke_dash_pat().IsEnabled(true);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
				rmfi_menu_stroke_dash_dash_dot_dot().IsChecked(true);
				mfi_menu_stroke_dash_pat().IsEnabled(true);
			}
			if (equal(m_main_sheet.m_stroke_width, 0.0f)) {
				rmfi_menu_stroke_width_0px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 1.0f)) {
				rmfi_menu_stroke_width_1px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 2.0f)) {
				rmfi_menu_stroke_width_2px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 3.0f)) {
				rmfi_menu_stroke_width_3px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 4.0f)) {
				rmfi_menu_stroke_width_4px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 8.0f)) {
				rmfi_menu_stroke_width_8px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 12.0f)) {
				rmfi_menu_stroke_width_12px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 16.0f)) {
				rmfi_menu_stroke_width_16px().IsChecked(true);
			}
			else {
				rmfi_menu_stroke_width_other().IsChecked(true);
			}
			if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT) {
				rmfi_menu_stroke_cap_flat().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
				rmfi_menu_stroke_cap_square().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
				rmfi_menu_stroke_cap_round().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
				rmfi_menu_stroke_cap_triangle().IsChecked(true);
			}
			if (m_main_sheet.m_arrow_style == ARROW_STYLE::ARROW_NONE) {
				rmfi_menu_stroke_arrow_none().IsChecked(true);
				mfi_menu_stroke_arrow_size().IsEnabled(false);
			}
			else if (m_main_sheet.m_arrow_style == ARROW_STYLE::ARROW_OPENED) {
				rmfi_menu_stroke_arrow_opened().IsChecked(true);
				mfi_menu_stroke_arrow_size().IsEnabled(true);
			}
			else if (m_main_sheet.m_arrow_style == ARROW_STYLE::ARROW_FILLED) {
				rmfi_menu_stroke_arrow_filled().IsChecked(true);
				mfi_menu_stroke_arrow_size().IsEnabled(true);
			}

			if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				rmfi_menu_stroke_join_bevel().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER) {
				rmfi_menu_stroke_join_miter().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				rmfi_menu_stroke_join_miter_or_bevel().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				rmfi_menu_stroke_join_round().IsChecked(true);
			}
		});

		// 書体メニューにフォーカスが移る直前.
		mbi_menu_font().as<Control>().GettingFocus([this](auto const&, auto const&) {
			if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_CONDENSED) {
				rmfi_menu_font_stretch_ultra_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_CONDENSED) {
				rmfi_menu_font_stretch_extra_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_CONDENSED) {
				rmfi_menu_font_stretch_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_CONDENSED) {
				rmfi_menu_font_stretch_semi_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL) {
				rmfi_menu_font_stretch_normal().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_EXPANDED) {
				rmfi_menu_font_stretch_semi_expanded().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXPANDED) {
				rmfi_menu_font_stretch_expanded().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_EXPANDED) {
				rmfi_menu_font_stretch_extra_expanded().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_EXPANDED) {
				rmfi_menu_font_stretch_ultra_expanded().IsChecked(true);
			}

			if (m_main_sheet.m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC) {
				rmfi_menu_font_style_italic().IsChecked(true);
			}
			else if (m_main_sheet.m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL) {
				rmfi_menu_font_style_normal().IsChecked(true);
			}
			else if (m_main_sheet.m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE) {
				rmfi_menu_font_style_oblique().IsChecked(true);
			}

			if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_THIN) {
				rmfi_menu_font_weight_thin().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_LIGHT) {
				rmfi_menu_font_weight_extra_light().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_LIGHT) {
				rmfi_menu_font_weight_light().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL) {
				rmfi_menu_font_weight_normal().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_MEDIUM) {
				rmfi_menu_font_weight_medium().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_SEMI_BOLD) {
				rmfi_menu_font_weight_semi_bold().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BOLD) {
				rmfi_menu_font_weight_bold().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BOLD) {
				rmfi_menu_font_weight_extra_bold().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BLACK) {
				rmfi_menu_font_weight_black().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BLACK) {
				rmfi_menu_font_weight_extra_black().IsChecked(true);
			}

			if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING) {
				rmfi_menu_text_align_left().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING) {
				rmfi_menu_text_align_right().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER) {
				rmfi_menu_text_align_center().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED) {
				rmfi_menu_text_align_just().IsChecked(true);
			}

			if (m_main_sheet.m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR) {
				rmfi_menu_text_align_top().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR) {
				rmfi_menu_text_align_bot().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER) {
				rmfi_menu_text_align_mid().IsChecked(true);
			}
			if (m_main_sheet.m_text_word_wrap == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_WRAP) {
				rmfi_menu_text_wrap().IsChecked(true);
			}
			else if (m_main_sheet.m_text_word_wrap == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_NO_WRAP) {
				rmfi_menu_text_no_wrap().IsChecked(true);
			}
			else if (m_main_sheet.m_text_word_wrap == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_CHARACTER) {
				rmfi_menu_text_wrap_char().IsChecked(true);
			}

		});

		// レイアウトメニューにフォーカスが移る直前.
		mbi_menu_layout().as<Control>().GettingFocus([this](auto const&, auto const&) {
			if (m_main_sheet.m_grid_emph.m_gauge_1 == 0 && m_main_sheet.m_grid_emph.m_gauge_2 == 0) {
				rmfi_menu_grid_emph_1().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_emph.m_gauge_1 != 0 && m_main_sheet.m_grid_emph.m_gauge_2 == 0) {
				rmfi_menu_grid_emph_2().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_emph.m_gauge_1 != 0 && m_main_sheet.m_grid_emph.m_gauge_2 != 0) {
				rmfi_menu_grid_emph_3().IsChecked(true);
			}

			if (m_main_sheet.m_grid_show == GRID_SHOW::BACK) {
				rmfi_menu_grid_show_back().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_show == GRID_SHOW::FRONT) {
				rmfi_menu_grid_show_front().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_show == GRID_SHOW::HIDE) {
				rmfi_menu_grid_show_hide().IsChecked(true);
			}

			if (equal(m_main_scale, 1.0f)) {
				rmfi_menu_sheet_zoom_100().IsChecked(true);
			}
			else if (equal(m_main_scale, 1.5f)) {
				rmfi_menu_sheet_zoom_150().IsChecked(true);
			}
			else if (equal(m_main_scale, 2.0f)) {
				rmfi_menu_sheet_zoom_200().IsChecked(true);
			}
			else if (equal(m_main_scale, 3.0f)) {
				rmfi_menu_sheet_zoom_300().IsChecked(true);
			}
			else if (equal(m_main_scale, 4.0f)) {
				rmfi_menu_sheet_zoom_400().IsChecked(true);
			}
			else if (equal(m_main_scale, 0.75f)) {
				rmfi_menu_sheet_zoom_075().IsChecked(true);
			}
			else if (equal(m_main_scale, 0.5f)) {
				rmfi_menu_sheet_zoom_050().IsChecked(true);
			}
			else if (equal(m_main_scale, 0.25f)) {
				rmfi_menu_sheet_zoom_025().IsChecked(true);
			}

			if (m_background_show) {
				tmfi_menu_background_show().IsChecked(true);
			}
			else {
				tmfi_menu_background_show().IsChecked(false);
			}
			if (equal(m_background_color, COLOR_BLACK)) {
				rmfi_menu_background_black().IsChecked(true);
			}
			else {
				rmfi_menu_background_white().IsChecked(true);
			}
		});

		// その他メニューにフォーカスが移る直前.
		mbi_menu_misc().as<Control>().GettingFocus([this](auto const&, auto const&) {
			if (m_len_unit == LEN_UNIT::PIXEL) {
				rmfi_menu_len_unit_pixel().IsChecked(true);
			}
			else if (m_len_unit == LEN_UNIT::INCH) {
				rmfi_menu_len_unit_inch().IsChecked(true);
			}
			else if (m_len_unit == LEN_UNIT::MILLI) {
				rmfi_menu_len_unit_milli().IsChecked(true);
			}
			else if (m_len_unit == LEN_UNIT::POINT) {
				rmfi_menu_len_unit_point().IsChecked(true);
			}
			else if (m_len_unit == LEN_UNIT::GRID) {
				rmfi_menu_len_unit_grid().IsChecked(true);
			}
			if (m_color_code == COLOR_CODE::DEC) {
				rmfi_menu_color_code_dec().IsChecked(true);
			}
			else if (m_color_code == COLOR_CODE::HEX) {
				rmfi_menu_color_code_hex().IsChecked(true);
			}
			else if (m_color_code == COLOR_CODE::PCT) {
				rmfi_menu_color_code_pct().IsChecked(true);
			}
			else if (m_color_code == COLOR_CODE::REAL) {
				rmfi_menu_color_code_real().IsChecked(true);
			}
			if (m_snap_grid) {
				tmfi_menu_snap_grid().IsChecked(true);
			}
			else {
				tmfi_menu_snap_grid().IsChecked(false);
			}

			if (status_and(m_status_bar, STATUS_BAR::DRAW) == STATUS_BAR::DRAW) {
				tmfi_menu_status_bar_draw().IsChecked(true);
			}
			else {
				tmfi_menu_status_bar_draw().IsChecked(false);
			}
			if (status_and(m_status_bar, STATUS_BAR::GRID) == STATUS_BAR::GRID) {
				tmfi_menu_status_bar_grid().IsChecked(true);
			}
			else {
				tmfi_menu_status_bar_grid().IsChecked(false);
			}
			if (status_and(m_status_bar, STATUS_BAR::SHEET) == STATUS_BAR::SHEET) {
				tmfi_menu_status_bar_sheet().IsChecked(true);
			}
			else {
				tmfi_menu_status_bar_sheet().IsChecked(false);
			}
			if (status_and(m_status_bar, STATUS_BAR::POS) == STATUS_BAR::POS) {
				tmfi_menu_status_bar_pos().IsChecked(true);
			}
			else {
				tmfi_menu_status_bar_pos().IsChecked(false);
			}
			if (status_and(m_status_bar, STATUS_BAR::UNIT) == STATUS_BAR::UNIT) {
				tmfi_menu_status_bar_unit().IsChecked(true);
			}
			else {
				tmfi_menu_status_bar_unit().IsChecked(false);
			}
			if (status_and(m_status_bar, STATUS_BAR::ZOOM) == STATUS_BAR::ZOOM) {
				tmfi_menu_status_bar_zoom().IsChecked(true);
			}
			else {
				tmfi_menu_status_bar_zoom().IsChecked(false);
			}
		});

		// 編集メニューにフォーカスが移る直前.
		mbi_menu_edit().as<Control>().GettingFocus([this](auto const&, auto const&) {
			uint32_t undeleted_cnt = 0;	// 消去フラグがない図形の数
			uint32_t selected_cnt = 0;	// 選択された図形の数
			uint32_t selected_group_cnt = 0;	// 選択されたグループ図形の数
			uint32_t runlength_cnt = 0;	// 選択された図形の連続の数
			uint32_t selected_text_cnt = 0;	// 選択された文字列図形の数
			uint32_t text_cnt = 0;	// 文字列図形の数
			uint32_t selected_image_cnt = 0;	// 選択された画像図形の数
			uint32_t selected_arc_cnt = 0;	// 選択された円弧図形の数
			uint32_t selected_poly_open_cnt = 0;	// 選択された開いた多角形図形の数
			uint32_t selected_poly_close_cnt = 0;	// 選択された閉じた多角形図形の数
			uint32_t selected_exist_cap_cnt = 0;	// 選択された端をもつ図形の数
			bool fore_selected = false;	// 最前面の図形の選択フラグ
			bool back_selected = false;	// 最背面の図形の選択フラグ
			bool prev_selected = false;	// ひとつ背面の図形の選択フラグ
			slist_count(
				m_main_sheet.m_shape_list,
				undeleted_cnt,
				selected_cnt,
				selected_group_cnt,
				runlength_cnt,
				selected_text_cnt,
				text_cnt,
				selected_image_cnt,
				selected_arc_cnt,
				selected_poly_open_cnt,
				selected_poly_close_cnt,
				selected_exist_cap_cnt,
				fore_selected,
				back_selected,
				prev_selected
			);

			// 選択された図形がひとつ以上ある場合.
			const auto exists_selected = (selected_cnt > 0);
			// 選択された文字列がひとつ以上ある場合.
			const auto exists_selected_text = (selected_text_cnt > 0);
			// 文字列がひとつ以上ある場合.
			const auto exists_text = (text_cnt > 0);
			// 選択された画像がひとつ以上ある場合.
			const auto exists_selected_image = (selected_image_cnt > 0);
			// 選択された円弧がひとつ以上ある場合.
			const auto exists_selected_arc = (selected_arc_cnt > 0);
			// 選択された開いた多角形がひとつ以上ある場合.
			const auto exists_selected_poly_open = (selected_poly_open_cnt > 0);
			// 選択された閉じた多角形がひとつ以上ある場合.
			const auto exists_selected_poly_close = (selected_poly_close_cnt > 0);
			// 選択されてない図形がひとつ以上ある場合, または選択されてない文字がひとつ以上ある場合.
			uint32_t text_unselected_char_cnt;
			if (m_core_text_shape != nullptr) {
				const auto len = m_core_text_shape->get_text_len();
				const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, m_core_text_shape->get_text_len());
				text_unselected_char_cnt = len - (end - m_main_sheet.m_select_start);
			}
			else {
				text_unselected_char_cnt = 0;
			}
			const auto exists_unselected = (selected_cnt < undeleted_cnt || text_unselected_char_cnt > 0);
			// 選択された図形がふたつ以上ある場合.
			const auto exists_selected_2 = (selected_cnt > 1);
			// 選択されたグループがひとつ以上ある場合.
			const auto exists_selected_group = (selected_group_cnt > 0);
			// 選択された端のある図形がひとつ以上ある場合.
			const auto exists_selected_cap = (selected_exist_cap_cnt > 0);
			// 前面に配置可能か判定する.
			// 1. 複数のランレングスがある.
			// 2. または, 少なくとも 1 つは選択された図形があり, 
			//    かつ最前面の図形は選択されいない.
			const auto enable_forward = (runlength_cnt > 1 || (exists_selected && !fore_selected));
			// 背面に配置可能か判定する.
			// 1. 複数のランレングスがある.
			// 2. または, 少なくとも 1 つは選択された図形があり, 
			//    かつ最背面の図形は選択されいない.
			const auto enable_backward = (runlength_cnt > 1 || (exists_selected && !back_selected));
			const auto& dp_view = Clipboard::GetContent();
			const bool exists_clipboard_data = (dp_view.Contains(CLIPBOARD_FORMAT_SHAPES) ||
				dp_view.Contains(StandardDataFormats::Text()) || dp_view.Contains(StandardDataFormats::Bitmap()));

			// まずサブ項目をもつメニューの可否を設定してから, 子の項目を設定する.
			// そうしないと, 子の項目の可否がただちに反映しない.

			mbi_menu_undo().IsEnabled(m_ustack_undo.size() > 0);
			mfi_menu_redo().IsEnabled(m_ustack_redo.size() > 0);

			mfi_menu_xcvd_cut().IsEnabled(exists_selected);
			mfi_menu_xcvd_copy().IsEnabled(exists_selected);
			mfi_menu_xcvd_paste().IsEnabled(exists_clipboard_data);
			mfi_menu_xcvd_delete().IsEnabled(exists_selected);

			mfi_menu_select_all().IsEnabled(exists_unselected);
			mfi_menu_bring_forward().IsEnabled(enable_forward);
			mfi_menu_bring_to_front().IsEnabled(enable_forward);
			mfi_menu_send_to_back().IsEnabled(enable_backward);
			mfi_menu_send_backward().IsEnabled(enable_backward);
			mfsi_menu_order().IsEnabled(enable_forward || enable_backward);

			mfi_menu_group().IsEnabled(exists_selected_2);
			mfi_menu_ungroup().IsEnabled(exists_selected_group);

			mfi_menu_reverse_path().IsEnabled(exists_selected_cap);
			mfi_menu_open_close_polygon().IsEnabled(exists_selected_poly_close || exists_selected_poly_open);
			//mfi_menu_edit_text().IsEnabled(exists_selected_text);
			mfi_menu_find_text().IsEnabled(exists_text);
			mfi_menu_revert_image().IsEnabled(exists_selected_image);
			//m_list_sel_cnt = selected_cnt;
		});

		//	winrt::Windows::UI::Xaml::UIElement::PointerPressedEvent(), box_value(winrt::Windows::UI::Xaml::Input::PointerEventHandler(
		//		[](auto) {
		//	int test = 0;
		//	})), true);
		// 「印刷」メニューの可否を設定する.
		//{
			//mfi_print().IsEnabled(PrintManager::IsSupported());
		//}

		// テキスト入力
		// CoreTextEditContext::NotifyFocusEnter
		//	- SelectionRequested
		//	- TextRequested
		// 半角文字入力
		//	- TextUpdating
		//	- TextRequested
		// 全角文字入力
		//	- CompositionStarted
		//	- TextUpdating
		//	- TextRequested
		//	- FormatUpdating
		//	- CompositionCompleted
		{
			m_core_text.InputPaneDisplayPolicy(winrt::Windows::UI::Text::Core::CoreTextInputPaneDisplayPolicy::Manual);
			m_core_text.InputScope(winrt::Windows::UI::Text::Core::CoreTextInputScope::Text);
			m_core_text.TextRequested([this](auto const&, auto const& args) {
				if (m_core_text_shape == nullptr) {
					return;
				}
				//__debugbreak();
				CoreTextTextRequest req{ args.Request() };
				const CoreTextRange ran{ req.Range() };
				const auto end = static_cast<uint32_t>(ran.EndCaretPosition);
				const auto text = (m_core_text_shape->m_text == nullptr ? L"" : m_core_text_shape->m_text);
				const auto sub_len = min(end, m_core_text_shape->get_text_len()) - ran.StartCaretPosition;	// 部分文字列の長さ
				winrt::hstring sub_text{	// 部分文字列
					text + ran.StartCaretPosition, static_cast<winrt::hstring::size_type>(sub_len)
				};
				req.Text(sub_text);
			});
			m_core_text.SelectionRequested([this](auto const&, auto const& args) {
				if (m_core_text_shape == nullptr) {
					return;
				}
				//__debugbreak();
				CoreTextSelectionRequest req{ args.Request() };
				const ShapeText* t = m_core_text_shape;
				const auto len = t->get_text_len();
				const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
				const auto start = min(m_main_sheet.m_select_start, len);
				CoreTextRange ran{};
				ran.StartCaretPosition = min(start, end);
				ran.EndCaretPosition = max(start, end);
				req.Selection(ran);
				});
			m_core_text.FocusRemoved([this](auto const&, auto const&) {
				if (m_core_text_shape == nullptr) {
					return;
				}
				__debugbreak();
				m_core_text.NotifyFocusLeave();
				undo_push_text_unselect(m_core_text_shape);
				m_core_text_shape = nullptr;
				m_core_text_comp = false;
				main_draw();
				});
			// 文字が入力される
			m_core_text.TextUpdating([this](auto const&, auto const& args) {
				//__debugbreak();
				CoreTextRange ran{ args.Range() };
				const winrt::hstring ins_text{ args.Text() };
				core_text_insert(ins_text.data(), static_cast<uint32_t>(ins_text.size()));
				});
			// 変換中, キャレットが移動した
			m_core_text.SelectionUpdating([this](auto const&, auto const& args) {
				CoreTextRange ran{ args.Selection() };
				undo_push_text_select(m_core_text_shape, ran.StartCaretPosition, ran.EndCaretPosition, false);
				main_draw();
				});
			// 変換候補などの書式
			m_core_text.FormatUpdating([](auto const&, auto const&) {
				//__debugbreak();
				});
			m_core_text.LayoutRequested([this](auto const&, auto const& args) {
				// __debugbreak();
				if (m_core_text_shape == nullptr) {
					return;
				}
				CoreTextLayoutRequest req{ args.Request() };
				Rect con_rect;	// テキストの矩形
				Rect sel_rect;	// 選択範囲の矩形
				D2D1_POINT_2F con_start, con_end;	// テキストの端
				D2D1_POINT_2F sel_start, sel_end;	// 選択範囲の端
				// キャレットがある行を得る.
				// キャレットは選択範囲の end の位置にある.
				const ShapeText* t = m_core_text_shape;
				if (t->m_dwrite_text_layout == nullptr) {
					m_core_text_shape->create_text_layout();
				}

				float height = t->m_font_size;
				for (uint32_t i = 0; i + 1 < t->m_dwrite_line_cnt; i++) {
					height += t->m_dwrite_line_metrics[i].height;
				}
				height = max(height, t->m_dwrite_text_layout->GetMaxHeight());
				float width = t->m_dwrite_text_layout->GetMaxWidth();
				float left = (t->m_pos.x < 0.0 ? t->m_start.x + t->m_pos.x : t->m_start.x);
				float top = (t->m_pos.y < 0.0 ? t->m_start.y + t->m_pos.y : t->m_start.y);
				GeneralTransform tran{	// 変換子
					scp_main_panel().TransformToVisual(nullptr)
				};
				const Point panel{	// スワップチェーンパネルの左上点
					tran.TransformPoint(Point{ 0.0f, 0.0f })
				};
				Rect win_rect{	// ウィンドウのクライアント矩形
					Window::Current().CoreWindow().Bounds()
				};
				con_rect.X = win_rect.X + panel.X + left / m_main_scale;
				con_rect.Y = win_rect.Y + panel.Y + top / m_main_scale;
				con_rect.Width = width / m_main_scale;
				con_rect.Height = height / m_main_scale;

				const auto end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
				const auto start = m_main_sheet.m_select_start;
				const auto car_row = t->get_text_row(end);	// キャレット行
				const auto row_start = t->m_dwrite_test_metrics[car_row].textPosition;	// キャレット行の開始位置
				const auto row_end = row_start + t->m_dwrite_test_metrics[car_row].length;	// キャレット行の終了位置

				// 選択範囲が複数行の場合, 選択範囲の矩形をどう設定すれば適切なのか分からない.
				// とりあえず選択範囲はキャレットがある行に限定する.
				t->get_text_caret(max(min(start, end), row_start), car_row, false, sel_start);
				t->get_text_caret(min(max(start, end), row_end), car_row, false, sel_end);
				sel_rect.X = win_rect.X + panel.X + sel_start.x / m_main_scale;
				sel_rect.Y = win_rect.Y + panel.Y + sel_end.y / m_main_scale;
				sel_rect.Width = (sel_end.x - sel_start.x) / m_main_scale;
				sel_rect.Height = t->m_font_size / m_main_scale;
				const auto disp_scale = DisplayInformation::GetForCurrentView().RawPixelsPerViewPixel();
				con_rect.X = static_cast<FLOAT>(con_rect.X * disp_scale);
				con_rect.Y = static_cast<FLOAT>(con_rect.Y * disp_scale);
				con_rect.Width = static_cast<FLOAT>(con_rect.Width * disp_scale);
				con_rect.Height = static_cast<FLOAT>(con_rect.Height * disp_scale);
				sel_rect.X = static_cast<FLOAT>(sel_rect.X * disp_scale);
				sel_rect.Y = static_cast<FLOAT>(sel_rect.Y * disp_scale);
				sel_rect.Width = static_cast<FLOAT>(sel_rect.Width * disp_scale);
				sel_rect.Height = static_cast<FLOAT>(sel_rect.Height * disp_scale);
				req.LayoutBounds().ControlBounds(con_rect);
				req.LayoutBounds().TextBounds(sel_rect);
			});
			// 入力変換が開始されたとき呼び出される.
			// 入力変換フラグを立て, 変換開始時の文字列の選択範囲を保存する.
			// 選択範囲を保存するのは, 変換終了して文字列を挿入するとき必要となるため.
			// 入力変換フラグが立ってないときはその時点の選択範囲を置き換えればいいが,
			// 入力変換フラグが立っているならあらかじめ保存した選択範囲で置き換える.
			m_core_text.CompositionStarted([this](auto const&, auto const&) {
				//__debugbreak();
				m_core_text_comp = true;
				m_core_text_start = m_main_sheet.m_select_start;
				m_core_text_end = m_main_sheet.m_select_end;
				m_core_text_trail = m_main_sheet.m_select_trail;
				undo_push_null();

				m_core_text.NotifyLayoutChanged();
				});
			// 変換終了 (中断) のとき呼び出される.
			// 入力変換フラグを下ろす.
			// 後退キーなどで変換中の文字列が空にされた場合.
			// 改行キー押下や漢字変換キー, エスケープキーなどで変換が終了場合.
			// LayoutRequested で設定したコンテキスト矩形以外でマウスボタンを使った (押下のあと離した後に呼び出される) 場合.
			// NotifyFocusLeave が呼び出された場合.
			m_core_text.CompositionCompleted([this](auto const&, auto const&) {
				//__debugbreak();				
				m_core_text_comp = false;
				});
		}

		// アプリケーションの中断・継続などのイベントハンドラーを設定する.
		{
			auto const& app{ Application::Current() };
			m_token_resuming = app.Resuming({ this, &MainPage::app_resuming_async });
			m_token_suspending = app.Suspending({ this, &MainPage::app_suspending_async });
			m_token_entered_background = app.EnteredBackground({ this, &MainPage::app_entered_background });
			m_token_leaving_background = app.LeavingBackground({ this, &MainPage::app_leaving_background });
		}

		// ウィンドウの表示が変わったときのイベントハンドラーを設定する.
		{
			auto const& win{ CoreWindow::GetForCurrentThread() };
			m_token_activated = win.Activated({ this, &MainPage::thread_activated });
			m_token_visibility_changed = win.VisibilityChanged({ this, &MainPage::thread_visibility_changed });
		}

		// ディスプレイの状態が変わったときのイベントハンドラーを設定する.
		{
			auto const& disp{ DisplayInformation::GetForCurrentView() };
			m_token_dpi_changed = disp.DpiChanged({ this, &MainPage::display_dpi_changed });
			m_token_orientation_changed = disp.OrientationChanged({ this, &MainPage::display_orientation_changed });
			m_token_contents_invalidated = disp.DisplayContentsInvalidated({ this, &MainPage::display_contents_invalidated });
		}

		// アプリケーションを閉じる前の確認のハンドラーを設定する.
		{
			m_token_close_requested = SystemNavigationManagerPreview::GetForCurrentView().CloseRequested({
				this, &MainPage::navi_close_requested });
		}

		// D2D/DWRITE ファクトリを図形クラスに, 
		// 用紙図形をアンドゥ操作に格納する.
		{
			Undo::begin(&m_main_sheet);
		}

		// 背景パターン画像の読み込み.
		{
			background_get_brush();
		}

		auto _{ file_new_click_async(nullptr, nullptr) };
	}

	// メッセージダイアログを表示する.
	// glyph	フォントアイコンのグリフの静的リソースのキー
	// message	メッセージのアプリケーションリソースのキー
	// desc	説明のアプリケーションリソースのキー
	// 戻り値	なし
	void MainPage::message_show(winrt::hstring const& glyph, winrt::hstring const& message, winrt::hstring const& desc)
	{
		constexpr wchar_t QUOT[] = L"\"";	// 引用符
		constexpr wchar_t NEW_LINE[] = L"\u2028";	// テキストブロック内での改行
		ResourceLoader const& r_loader = ResourceLoader::GetForCurrentView();

		// メッセージをキーとしてリソースから文字列を得る.
		// 文字列が空なら, メッセージをそのまま文字列に格納する.
		winrt::hstring text;	// 文字列
		try {
			text = r_loader.GetString(message);
		}
		catch (winrt::hresult_error const&) {}
		if (text.empty()) {
			text = message;
		}
		// 説明をキーとしてリソースから文字列を得る.
		// 文字列が空なら, 説明をそのまま文字列に格納する.
		winrt::hstring added_text;	// 追加の文字列
		try {
			added_text = r_loader.GetString(desc);
		}
		catch (winrt::hresult_error const&) {}
		if (!added_text.empty()) {
			text = text + NEW_LINE + added_text;
		}
		else if (!desc.empty()) {
			text = text + NEW_LINE + QUOT + desc + QUOT;
		}
		const IInspectable glyph_val{
			Resources().TryLookup(box_value(glyph))
		};
		const winrt::hstring font_icon{
			glyph_val != nullptr ? unbox_value<winrt::hstring>(glyph_val) : glyph
		};
		fi_message().Glyph(font_icon);
		tk_message().Text(text);
		// メッセージダイアログを起動時からでも表示できるよう, RunIdleAsync を使用する.
		// マイクロソフトによると,
		// > アプリでの最も低速なステージとして、起動や、ビューの切り替えなどがあります。
		// > ユーザーに最初に表示される UI を起動するために必要なもの以上の作業を実行しないでください。
		// > たとえば、段階的に公開される UI の UI や、ポップアップのコンテンツなどは作成しないでください。
		Dispatcher().RunIdleAsync([=](winrt::Windows::UI::Core::IdleDispatchedHandlerArgs) {
			auto _{ cd_message_dialog().ShowAsync() };
			});
	}

	/*
	*/
	// アプリからの印刷
	// https://learn.microsoft.com/ja-jp/windows/uwp/devices-sensors/print-from-your-app
	// https://github.com/microsoft/Windows-universal-samples/tree/main/Samples/Printing/cpp
	IAsyncAction MainPage::print_click_async(const IInspectable&, const RoutedEventArgs&)
	{
		co_return;
		/*
		if (!PrintManager::IsSupported()) {
			__debugbreak();
		}
		if (!co_await PrintManager::ShowPrintUIAsync()) {
			message_show(ICON_ALERT, L"File to Print", {});
		}
		*/
	}

	//------------------------------
	// メインのスワップチェーンパネルの寸法が変わった.
	//------------------------------
	void MainPage::main_panel_scale_changed(IInspectable const&, IInspectable const&)
	{
		m_main_d2d.SetCompositionScale(scp_main_panel().CompositionScaleX(), scp_main_panel().CompositionScaleY());
	}

	//------------------------------
	// メインのスワップチェーンパネルがロードされた.
	//------------------------------
	void MainPage::main_panel_loaded(IInspectable const& sender, RoutedEventArgs const&)
	{
#if defined(_DEBUG)
		if (sender != scp_main_panel()) {
			return;
		}
#endif // _DEBUG

		m_main_d2d.SetSwapChainPanel(scp_main_panel());
		main_draw();
	}

	//------------------------------
	// メインのスワップチェーンパネルの大きさが変わった.
	// args	イベントの引数
	//------------------------------
	void MainPage::main_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
	{
		if (sender != scp_main_panel()) {
			return;
		}
		const auto z = args.NewSize();
		const float w = z.Width;
		const float h = z.Height;
		scroll_set(w, h);
		if (scp_main_panel().IsLoaded()) {
			m_main_d2d.SetLogicalSize2(D2D1_SIZE_F{ w, h });
			main_draw();
		}
		status_bar_set_pos();
	}

	//------------------------------
	// メインのスワップチェーンパネルの大きさを設定する.
	//------------------------------
	void MainPage::main_panel_size(void)
	{
		const float w = static_cast<float>(scp_main_panel().ActualWidth());
		const float h = static_cast<float>(scp_main_panel().ActualHeight());
		if (w > 0.0f && h > 0.0f) {
			scroll_set(w, h);
			m_main_d2d.SetLogicalSize2(D2D1_SIZE_F{ w, h });
		}
	}

	// メインの用紙の境界矩形を更新する.
	void MainPage::main_bbox_update(void) noexcept
	{
		// リスト中の図形を囲む矩形を得る.
		slist_bbox_shape(m_main_sheet.m_shape_list, m_main_bbox_lt, m_main_bbox_rb);

		// 矩形の右下点が用紙の右下点より小さいなら, 用紙の右下点を格納する.
		const auto rb_x = m_main_sheet.m_sheet_size.width - m_main_sheet.m_sheet_margin.left;
		if (m_main_bbox_rb.x < rb_x) {
			m_main_bbox_rb.x = rb_x;
		}
		const auto rb_y = m_main_sheet.m_sheet_size.height - m_main_sheet.m_sheet_margin.top;
		if (m_main_bbox_rb.y < rb_y) {
			m_main_bbox_rb.y = rb_y;
		}

		// 矩形の左上点が用紙の左上点より大きいなら, 用紙の左上点を格納する.
		const auto lb_x = -m_main_sheet.m_sheet_margin.left;
		if (m_main_bbox_lt.x > lb_x) {
			m_main_bbox_lt.x = lb_x;
		}
		const auto lb_y = -m_main_sheet.m_sheet_margin.left;
		if (m_main_bbox_lt.y > lb_y) {
			m_main_bbox_lt.y = lb_y;
		}
	}

	// メインの用紙を表示する.
	void MainPage::main_draw(void)
	{
		if (!scp_main_panel().IsLoaded()) {
			return;
		}
		// ロックできないなら中断する.
		if (!m_mutex_draw.try_lock()) {
			return;
		}

		// 描画前に必要な変数を格納する.
		m_main_sheet.begin_draw(m_main_d2d.m_d2d_context.get(), true, m_wic_background.get(), m_main_scale);

		// 描画環境を保存, 描画を開始する.
		m_main_d2d.m_d2d_context->SaveDrawingState(Shape::m_state_block.get());
		m_main_d2d.m_d2d_context->BeginDraw();
		m_main_d2d.m_d2d_context->Clear(m_background_color);

		// 背景パターンを描画する,
		if (m_background_show) {
			const D2D1_RECT_F w_rect{	// ウィンドウの矩形
				0, 0, m_main_d2d.m_logical_width, m_main_d2d.m_logical_height
			};
			m_main_d2d.m_d2d_context->FillRectangle(w_rect, Shape::m_d2d_bitmap_brush.get());
		}

		// 変換行列に拡大縮小と平行移動を設定する.
		D2D1_MATRIX_3X2_F t{};	// 変換行列
		t.m11 = t.m22 = m_main_scale;
		t.dx = static_cast<FLOAT>(-(m_main_bbox_lt.x + sb_horz().Value()) * m_main_scale);
		t.dy = static_cast<FLOAT>(-(m_main_bbox_lt.y + sb_vert().Value()) * m_main_scale);
		m_main_d2d.m_d2d_context->SetTransform(&t);

		// 図形を (用紙も) 表示する.
		m_main_sheet.draw();

		// 矩形選択している状態なら, 作図ツールに応じた補助線を表示する.
		if (m_event_state == EVENT_STATE::PRESS_RECT) {
			if (m_drawing_tool == DRAWING_TOOL::SELECT ||
				m_drawing_tool == DRAWING_TOOL::RECT ||
				m_drawing_tool == DRAWING_TOOL::TEXT ||
				m_drawing_tool == DRAWING_TOOL::RULER) {
				m_main_sheet.auxiliary_draw_rect(m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::BEZIER) {
				m_main_sheet.auxiliary_draw_bezi(m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::ELLIPSE) {
				m_main_sheet.auxiliary_draw_elli(m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::LINE) {
				m_main_sheet.auxiliary_draw_line(m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::RRECT) {
				m_main_sheet.auxiliary_draw_rrect(m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::POLY) {
				m_main_sheet.auxiliary_draw_poly(m_event_pos_pressed, m_event_pos_curr, m_drawing_poly_opt);
			}
			else if (m_drawing_tool == DRAWING_TOOL::ARC) {
				m_main_sheet.auxiliary_draw_arc(m_event_pos_pressed, m_event_pos_curr);
			}
		}

		if (m_core_text_shape != nullptr && !m_core_text_shape->is_deleted() && m_core_text_shape->is_selected()) {
			m_core_text_shape->draw_selection(m_main_sheet.m_select_start, m_main_sheet.m_select_end, m_main_sheet.m_select_trail);

			const int row = m_core_text_shape->get_text_row(m_main_sheet.m_select_end);
			//const int row = m_core_text_shape->get_text_row(m_core_text_end);
			D2D1_POINT_2F car;	// キャレットの点
			m_core_text_shape->get_text_caret(m_main_sheet.m_select_end, row, m_main_sheet.m_select_trail, car);
			//m_core_text_shape->get_text_caret(m_core_text_end, row, m_core_text_trail, car);
			D2D1_POINT_2F p{
				car.x - 0.5f, car.y
			};
			D2D1_POINT_2F q{
				car.x - 0.5f, car.y + m_core_text_shape->m_font_size
			};
			D2D1_POINT_2F r{
				car.x, car.y
			};
			D2D1_POINT_2F s{
				car.x, car.y + m_core_text_shape->m_font_size
			};
			m_main_sheet.m_d2d_color_brush->SetColor(COLOR_WHITE);
			m_main_d2d.m_d2d_context->DrawLine(p, q, m_main_sheet.m_d2d_color_brush.get(), 2.0f);
			m_main_sheet.m_d2d_color_brush->SetColor(COLOR_BLACK);
			m_main_d2d.m_d2d_context->DrawLine(r, s, m_main_sheet.m_d2d_color_brush.get(), 1.0f);
		}

		// 描画を終了し結果を得る. 保存された描画環境を元に戻す.
		const HRESULT hres = m_main_d2d.m_d2d_context->EndDraw();
		m_main_d2d.m_d2d_context->RestoreDrawingState(Shape::m_state_block.get());

		// 結果が S_OK でない場合,
		if (hres != S_OK) {
			// 「描画できません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, L"str_err_draw", {});
		}
		// 結果が S_OK の場合,
		else {
			// スワップチェーンの内容を画面に表示する.
			m_main_d2d.Present();
		}
		m_mutex_draw.unlock();
	}
}
