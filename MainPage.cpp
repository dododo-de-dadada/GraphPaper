//-------------------------------
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
	using winrt::Windows::UI::Core::Preview::SystemNavigationManagerPreview;
	using winrt::Windows::UI::Xaml::Application;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogButton;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Media::Brush;
	using winrt::Windows::UI::Xaml::Window;
	using winrt::Windows::Foundation::Rect;
	using winrt::Windows::System::VirtualKey;
	using winrt::Windows::System::VirtualKeyModifiers;
	using winrt::Windows::UI::Core::CoreVirtualKeyStates;
	using winrt::Windows::UI::Xaml::FocusState;
	using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;
	using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;

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

	// 方眼を表示する.
	static void page_draw_grid(ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush, const float g_len, const D2D1_COLOR_F g_color, const GRID_EMPH g_emph, const D2D1_POINT_2F g_offset, const D2D1_SIZE_F g_size);

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
	void conv_len_to_str(const LEN_UNIT len_unit, const double len, const double dpi, const double grid_len, const uint32_t text_len, wchar_t *text_buf) noexcept
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

	//-------------------------------
	// メインページを作成する.
	//-------------------------------
	int _debug_edit = 0;
	MainPage::MainPage(void)
	{
		// お約束.
		InitializeComponent();

		mf_popup_menu().Opening([this](auto, auto) {
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
				m_main_page.m_shape_list,
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
			if (m_edit_text_shape != nullptr) {
				const auto len = m_edit_text_shape->get_text_len();
				const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, m_edit_text_shape->get_text_len());
				text_unselected_char_cnt = len - (end - m_main_page.m_select_start);
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
			mfi_popup_undo().Visibility(m_ustack_undo.size() > 0 ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_redo().Visibility(m_ustack_redo.size() > 0 ? Visibility::Visible : Visibility::Collapsed);

			mfs_popup_sepa_undo_xcvd().Visibility((m_ustack_undo.size() > 0 || m_ustack_redo.size() > 0) && (exists_selected || exists_clipboard_data) ? Visibility::Visible : Visibility::Collapsed);

			mfi_popup_xcvd_cut().Visibility(exists_selected ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_xcvd_copy().Visibility(exists_selected ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_xcvd_paste().Visibility(exists_clipboard_data ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_xcvd_delete().Visibility(exists_selected ? Visibility::Visible : Visibility::Collapsed);

			mfs_popup_sepa_xcvd_select().Visibility(
				(m_ustack_undo.size() > 0 || m_ustack_redo.size() > 0 || exists_selected || exists_clipboard_data) &&
				(exists_unselected || enable_forward || enable_backward) ? Visibility::Visible : Visibility::Collapsed);

			mfi_popup_select_all().Visibility(exists_unselected ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_bring_forward().Visibility(enable_forward ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_bring_to_front().Visibility(enable_forward ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_send_to_back().Visibility(enable_backward ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_send_backward().Visibility(enable_backward ? Visibility::Visible : Visibility::Collapsed);
			mfsi_popup_order().Visibility((exists_unselected || enable_forward || enable_backward) ? Visibility::Visible : Visibility::Collapsed);

			mfs_popup_sepa_select_group().Visibility(
				(m_ustack_undo.size() > 0 || m_ustack_redo.size() > 0 || exists_selected || exists_clipboard_data || exists_unselected || enable_forward || enable_backward) &&
				(exists_selected_2 || exists_selected_group) ? Visibility::Visible : Visibility::Collapsed);

			mfi_popup_group().Visibility(exists_selected_2 ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_ungroup().Visibility(exists_selected_group ? Visibility::Visible : Visibility::Collapsed);

			mfs_popup_sepa_group_edit().Visibility(
				(m_ustack_undo.size() > 0 || m_ustack_redo.size() > 0 || exists_selected || exists_clipboard_data || exists_unselected || enable_forward || enable_backward || exists_selected_2 || exists_selected_group) &&
				(exists_selected_cap || exists_selected_poly_close || exists_selected_text || exists_text || exists_selected_image) ? Visibility::Visible : Visibility::Collapsed);

			mfi_popup_reverse_path().Visibility(exists_selected_cap ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_meth_poly_end().Visibility(exists_selected_poly_close || exists_selected_poly_open ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_meth_text_edit().Visibility(exists_selected_text ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_meth_text_find().Visibility(exists_text ? Visibility::Visible : Visibility::Collapsed);
			mfi_popup_meth_image_revert().Visibility(exists_selected_image ? Visibility::Visible : Visibility::Collapsed);
			m_list_sel_cnt = selected_cnt;

			//Shape* pressed = m_event_shape_r_pressed;
			//uint32_t loc_pressed = m_event_loc_r_pressed;
			winrt::Windows::UI::Xaml::Visibility prop;
			winrt::Windows::UI::Xaml::Visibility font;
			winrt::Windows::UI::Xaml::Visibility image;
			winrt::Windows::UI::Xaml::Visibility layout;
			// 押された図形がヌル, または押された図形の部位が外側か判定する.
			if (m_event_shape_r_pressed == nullptr || m_event_loc_r_pressed == LOC_TYPE::LOC_PAGE) {
				prop = Visibility::Collapsed;
				font = Visibility::Collapsed;
				image = Visibility::Collapsed;
				layout = Visibility::Visible;
			}
			else {
				// 押された図形の属性値を表示に格納する.
				m_main_page.set_attr_to(m_event_shape_r_pressed);
				// メニューバーを更新する
				stroke_dash_is_checked(m_main_page.m_stroke_dash);
				stroke_width_is_checked(m_main_page.m_stroke_width);
				stroke_cap_is_checked(m_main_page.m_stroke_cap);
				stroke_join_is_checked(m_main_page.m_stroke_join);
				stroke_arrow_is_checked(m_main_page.m_arrow_style);
				font_weight_is_checked(m_main_page.m_font_weight);
				font_stretch_is_checked(m_main_page.m_font_stretch);
				font_style_is_checked(m_main_page.m_font_style);
				text_align_horz_is_checked(m_main_page.m_text_align_horz);
				text_align_vert_is_checked(m_main_page.m_text_align_vert);
				text_word_wrap_is_checked(m_main_page.m_text_word_wrap);
				grid_emph_is_checked(m_main_page.m_grid_emph);
				grid_show_is_checked(m_main_page.m_grid_show);

				// 押された部位が文字列なら, フォントのメニュー項目を表示する.
				if (m_event_loc_r_pressed == LOC_TYPE::LOC_TEXT) {
					prop = Visibility::Collapsed;
					font = Visibility::Visible;
					image = Visibility::Collapsed;
					layout = Visibility::Collapsed;
				}
				// 押された部位が画像なら, 画像のメニュー項目を表示する.
				else if (typeid(*m_event_shape_r_pressed) == typeid(ShapeImage)) {
					prop = Visibility::Collapsed;
					font = Visibility::Collapsed;
					image = Visibility::Visible;
					layout = Visibility::Collapsed;
				}
				// 上記以外なら, 属性のメニュー項目を表示する.
				else {
					prop = Visibility::Visible;
					font = Visibility::Collapsed;
					image = Visibility::Collapsed;
					layout = Visibility::Collapsed;
				}
			}
			mfsi_popup_stroke_dash().Visibility(prop);
			mfi_popup_stroke_dash_pat().Visibility(prop);
			mfsi_popup_stroke_width().Visibility(prop);
			if (prop == Visibility::Visible && m_event_shape_r_pressed->exist_cap()) {
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
			if (prop == Visibility::Visible && m_event_shape_r_pressed->exist_join()) {
				mfsi_popup_stroke_join().Visibility(Visibility::Visible);
			}
			else {
				mfsi_popup_stroke_join().Visibility(Visibility::Collapsed);
			}
			mfs_popup_sepa_arrow_color().Visibility(prop);
			mfi_popup_stroke_color().Visibility(prop);
			mfi_popup_fill_color().Visibility(prop);

			mfi_popup_font_family().Visibility(font);
			mfi_popup_font_size().Visibility(font);
			mfsi_popup_font_weight().Visibility(font);
			mfsi_popup_font_weight().Visibility(font);
			mfsi_popup_font_stretch().Visibility(font);
			mfsi_popup_font_style().Visibility(font);
			mfs_popup_sepa_font_text().Visibility(font);
			mfsi_popup_text_align_horz().Visibility(font);
			mfsi_popup_text_align_vert().Visibility(font);
			mfi_popup_text_line_sp().Visibility(font);
			mfi_popup_text_pad().Visibility(font);
			mfsi_popup_text_wrap().Visibility(font);
			mfi_popup_font_color().Visibility(font);

			tmfi_menu_meth_image_keep_asp_2().Visibility(image);
			mfi_popup_meth_image_revert().Visibility(image);
			mfi_popup_image_opac().Visibility(image);

			mfsi_popup_grid_show().Visibility(layout);
			mfsi_popup_grid_len().Visibility(layout);
			mfsi_popup_grid_emph().Visibility(layout);
			mfi_popup_grid_color().Visibility(layout);
			mfs_popup_sepa_grid_page().Visibility(layout);
			mfi_popup_page_size().Visibility(layout);
			mfi_popup_page_color().Visibility(layout);
			mfsi_popup_page_zoom().Visibility(layout);
			mfsi_popup_background_pattern().Visibility(layout);
			});

		mbi_menu_edit().as<winrt::Windows::UI::Xaml::Controls::Control>().GettingFocus([this](auto, auto) {
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
				m_main_page.m_shape_list,
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
			if (m_edit_text_shape != nullptr) {
				const auto len = m_edit_text_shape->get_text_len();
				const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, m_edit_text_shape->get_text_len());
				text_unselected_char_cnt = len - (end - m_main_page.m_select_start);
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
			mfi_menu_meth_poly_end().IsEnabled(exists_selected_poly_close || exists_selected_poly_open);
			mfi_menu_meth_text_edit().IsEnabled(exists_selected_text);
			mfi_menu_meth_text_find().IsEnabled(exists_text);
			mfi_menu_meth_text_fit_frame().IsEnabled(exists_selected_text);
			mfi_menu_meth_image_revert().IsEnabled(exists_selected_image);
			m_list_sel_cnt = selected_cnt;
			}
		);
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
		//	-SelectionRequested
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
			/*
			tx_find_text_what().GettingFocus([this](auto, auto) {
				if (m_edit_text_shape != nullptr) {
					m_edit_context.NotifyFocusLeave();
				}
				});
			tx_find_replace_with().GettingFocus([this](auto, auto) {
				if (m_edit_text_shape != nullptr) {
					m_edit_context.NotifyFocusLeave();
				}
				});
			auto core_win{ CoreWindow::GetForCurrentThread() };
			core_win.KeyDown([this](auto sender, auto args) {
				if (tx_find_text_what().FocusState() != FocusState::Unfocused ||
					tx_find_replace_with().FocusState() != FocusState::Unfocused ||
					btn_find_next().FocusState() != FocusState::Unfocused ||
					btn_find_replace().FocusState() != FocusState::Unfocused ||
					btn_find_replace_all().FocusState() != FocusState::Unfocused ||
					btn_summary_close().FocusState() != FocusState::Unfocused ||
					lv_summary_list().FocusState() != FocusState::Unfocused) {
					return;
				}
				if (m_edit_text_shape == nullptr ||
					m_edit_text_shape->is_deleted() || !m_edit_text_shape->is_selected()) {
					return;
				}
				const ShapeText* t = m_edit_text_shape;
				if (args.VirtualKey() == VirtualKey::Enter) {
					const auto len = t->get_text_len();
					const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
					const auto start = min(m_main_page.m_select_start, len);
					const auto s = min(start, end);
					undo_push_null();
					m_ustack_undo.push_back(new UndoText2(m_edit_text_shape, L"\r"));
					undo_push_text_select(m_edit_text_shape, s + 1, s + 1, false);
					//undo_menu_is_enabled();
					main_draw();

					winrt::Windows::UI::Text::Core::CoreTextRange modified_ran{
						static_cast<int32_t>(min(start, end)), static_cast<int32_t>(max(start, end))
					};
					const auto new_start = m_main_page.m_select_start;
					const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
					winrt::Windows::UI::Text::Core::CoreTextRange new_ran{
						static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
					};
					m_edit_context.NotifyTextChanged(modified_ran, 1, new_ran);
				}
				else if (args.VirtualKey() == VirtualKey::Back) {
					// 選択範囲がなくキャレット位置が文頭でないなら
					const auto len = t->get_text_len();
					const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
					const auto start = min(m_main_page.m_select_start, len);
					if (end == start && end > 0) {
						undo_push_null();
						undo_push_text_select(m_edit_text_shape, end - 1, end, false);
						m_ustack_undo.push_back(new UndoText2(m_edit_text_shape, nullptr));
						//undo_menu_is_enabled();
						main_draw();
					}
					// 選択範囲があるなら
					else if (end != start) {
						undo_push_null();
						m_ustack_undo.push_back(new UndoText2(m_edit_text_shape, nullptr));
						//undo_menu_is_enabled();
						main_draw();
					}
					winrt::Windows::UI::Text::Core::CoreTextRange modified_ran{
						static_cast<int32_t>(min(start, end)), static_cast<int32_t>(max(start, end))
					};
					const auto new_start = m_main_page.m_select_start;
					const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
					winrt::Windows::UI::Text::Core::CoreTextRange new_ran{
						static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
					};
					m_edit_context.NotifyTextChanged(modified_ran, 0, new_ran);
				}
				else if (args.VirtualKey() == VirtualKey::Delete) {
					const auto shift_key = ((sender.GetKeyState(VirtualKey::Shift) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
					//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
					//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
					text_char_delete(shift_key);
				}
				else if (args.VirtualKey() == VirtualKey::Left) {
					const auto shift_key = ((sender.GetKeyState(VirtualKey::Shift) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
					//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
					//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
					const auto len = t->get_text_len();
					const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
					const auto start = min(m_main_page.m_select_start, len);
					if (shift_key) {
						if (end > 0) {
							undo_push_text_select(m_edit_text_shape, start, end - 1, false);
							main_draw();
						}
					}
					else {
						if (end == start && end > 0) {
							undo_push_text_select(m_edit_text_shape, end - 1, end - 1, false);
							main_draw();
						}
						else if (end != start) {
							const auto new_end = min(start, end);
							undo_push_text_select(m_edit_text_shape, new_end, new_end, false);
							main_draw();
						}
					}
					const auto new_start = m_main_page.m_select_start;
					const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
					winrt::Windows::UI::Text::Core::CoreTextRange new_ran{
						static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
					};
					m_edit_context.NotifySelectionChanged(new_ran);
				}
				else if (args.VirtualKey() == VirtualKey::Right) {
					const auto shift_key = ((sender.GetKeyState(VirtualKey::Shift) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
					//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
					//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
					const auto len = t->get_text_len();
					const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
					const auto start = min(m_main_page.m_select_start, len);
					if (shift_key) {
						if (end < len) {
							undo_push_text_select(m_edit_text_shape, start, end + 1, false);
							main_draw();
						}
					}
					else {
						if (end == start && end < len) {
							undo_push_text_select(m_edit_text_shape, end + 1, end + 1, false);
							main_draw();
						}
						else if (end != start) {
							const auto new_end = max(start, end);
							undo_push_text_select(m_edit_text_shape, new_end, new_end, false);
							main_draw();
						}
					}
					const auto new_start = m_main_page.m_select_start;
					const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
					winrt::Windows::UI::Text::Core::CoreTextRange new_ran{
						static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
					};
					m_edit_context.NotifySelectionChanged(new_ran);
				}
				else if (args.VirtualKey() == VirtualKey::Up) {
					const auto len = t->get_text_len();
					const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
					const auto start = min(m_main_page.m_select_start, len);
					const auto row = t->get_text_row(m_main_page.m_select_end);
					if (end != start && row == 0) {
						const auto shift_key = ((sender.GetKeyState(VirtualKey::Shift) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
						//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
						//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
						if (shift_key) {
							undo_push_text_select(m_edit_text_shape, start, m_main_page.m_select_end, m_main_page.m_select_trail);
							main_draw();
						}
						else {
							undo_push_text_select(m_edit_text_shape, end, m_main_page.m_select_end, m_main_page.m_select_trail);
							main_draw();
						}
					}
					else if (row != 0) {
						//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
						//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
						const auto shift_key = ((sender.GetKeyState(VirtualKey::Shift) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
						const auto line_h = t->m_dwrite_test_metrics[row].top - t->m_dwrite_test_metrics[row - 1].top;
						D2D1_POINT_2F car;
						t->get_text_caret(end, row, m_main_page.m_select_trail, car);
						const D2D1_POINT_2F new_car{ car.x, car.y - line_h };
						bool new_trail;
						const auto new_end = t->get_text_pos(new_car, new_trail);
						if (shift_key) {
							undo_push_text_select(m_edit_text_shape, start, new_end, new_trail);
							main_draw();
						}
						else {
							const auto new_start = new_trail ? new_end + 1 : new_end;
							undo_push_text_select(m_edit_text_shape, new_start, new_end, new_trail);
							main_draw();
						}
					}
					const auto new_start = m_main_page.m_select_start;
					const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
					winrt::Windows::UI::Text::Core::CoreTextRange new_ran{
						static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
					};
					m_edit_context.NotifySelectionChanged(new_ran);
				}
				else if (args.VirtualKey() == VirtualKey::Down) {
					const auto len = t->get_text_len();
					const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
					const auto start = min(m_main_page.m_select_start, len);
					const auto row = t->get_text_row(m_main_page.m_select_end);	// キャレットがある行
					const auto last = t->m_dwrite_test_cnt - 1;	// 最終行
					if (end != start && row == last) {
						const bool shift_key = ((sender.GetKeyState(VirtualKey::Shift) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
						//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
						//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
						if (shift_key) {
							undo_push_text_select(m_edit_text_shape, start, m_main_page.m_select_end, m_main_page.m_select_trail);
							main_draw();
						}
						else {
							undo_push_text_select(m_edit_text_shape, end, m_main_page.m_select_end, m_main_page.m_select_trail);
							main_draw();
						}
					}
					else if (row != last) {
						const bool shift_key = ((sender.GetKeyState(VirtualKey::Shift) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
						//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
						//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
						const auto line_h = t->m_dwrite_test_metrics[row + 1].top - t->m_dwrite_test_metrics[row].top;
						D2D1_POINT_2F car;
						t->get_text_caret(end, row, m_main_page.m_select_trail, car);
						const D2D1_POINT_2F new_car{ car.x, car.y + line_h };
						bool new_trail;
						const auto new_end = t->get_text_pos(new_car, new_trail);
						if (shift_key) {
							undo_push_text_select(m_edit_text_shape, start, new_end, new_trail);
							main_draw();
						}
						else {
							const auto new_start = new_trail ? new_end + 1 : new_end;
							undo_push_text_select(m_edit_text_shape, new_start, new_end, new_trail);
							main_draw();
						}
					}
					const auto new_start = m_main_page.m_select_start;
					const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
					winrt::Windows::UI::Text::Core::CoreTextRange new_ran{
						static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
					};
					m_edit_context.NotifySelectionChanged(new_ran);
				}
			});
			*/
			m_edit_context.InputPaneDisplayPolicy(winrt::Windows::UI::Text::Core::CoreTextInputPaneDisplayPolicy::Manual);
			m_edit_context.InputScope(winrt::Windows::UI::Text::Core::CoreTextInputScope::Text);
			m_edit_context.TextRequested([this](auto, auto args) {
				//__debugbreak();
				using winrt::Windows::UI::Text::Core::CoreTextTextRequest;
				CoreTextTextRequest req{ args.Request() };
				const auto end = static_cast<uint32_t>(req.Range().EndCaretPosition);
				const auto text = (m_edit_text_shape->m_text == nullptr ? L"" : m_edit_text_shape->m_text);
				const auto sub_len = min(end, m_edit_text_shape->get_text_len()) - req.Range().StartCaretPosition;	// 部分文字列の長さ
				winrt::hstring sub_text{	// 部分文字列
					text + req.Range().StartCaretPosition,
					static_cast<winrt::hstring::size_type>(sub_len)
				};
				req.Text(sub_text);
			});
			m_edit_context.SelectionRequested([this](auto, auto args) {
				//__debugbreak();
				using winrt::Windows::UI::Text::Core::CoreTextSelectionRequest;
				using winrt::Windows::UI::Text::Core::CoreTextRange;
				CoreTextSelectionRequest req{ args.Request() };
				CoreTextRange ran{};
				const ShapeText* t = m_edit_text_shape;
				const auto len = t->get_text_len();
				const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
				const auto start = min(m_main_page.m_select_start, len);
				ran.StartCaretPosition = min(start, end);
				ran.EndCaretPosition = max(start, end);
				req.Selection(ran);
			});
			m_edit_context.FocusRemoved([this](auto, auto) {
				__debugbreak();
				if (m_edit_text_shape != nullptr) {
					m_edit_context.NotifyFocusLeave();
					undo_push_text_unselect(m_edit_text_shape);
					m_edit_text_shape = nullptr;
					xcvd_menu_is_enabled();
					main_draw();
				}
			});
			// 文字が入力される
			m_edit_context.TextUpdating([this](auto, auto args) {
				//__debugbreak();
				using winrt::Windows::UI::Text::Core::CoreTextRange;
				CoreTextRange ran{ args.Range() };
				const winrt::hstring ins_text{ args.Text() };
				text_sele_insert(ins_text.data(), static_cast<uint32_t>(ins_text.size()));
			});
			// 変換中, キャレットが移動した
			m_edit_context.SelectionUpdating([this](auto, auto args) {
				//__debugbreak();
				using winrt::Windows::UI::Text::Core::CoreTextRange;
				CoreTextRange ran{ args.Selection() };
				undo_push_text_select(m_edit_text_shape, ran.StartCaretPosition, ran.EndCaretPosition, false);
				xcvd_menu_is_enabled();
				main_draw();
			});
			m_edit_context.FormatUpdating([](auto, auto) {
				//__debugbreak();
			});
			m_edit_context.LayoutRequested([this](auto, auto args) {
				// __debugbreak();
				using winrt::Windows::UI::Text::Core::CoreTextLayoutRequest;
				using winrt::Windows::Graphics::Display::DisplayInformation;
				using winrt::Windows::UI::Xaml::Media::GeneralTransform;
				CoreTextLayoutRequest req{ args.Request() };
				Rect con_rect;	// テキストの矩形
				Rect sel_rect;	// 選択範囲の矩形
				D2D1_POINT_2F con_start, con_end;	// テキストの端
				D2D1_POINT_2F sel_start, sel_end;	// 選択範囲の端
				// キャレットがある行を得る.
				// キャレットは選択範囲の end の位置にある.
				const ShapeText* t = m_edit_text_shape;
				if (t->m_dwrite_text_layout == nullptr) {
					m_edit_text_shape->create_text_layout();
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
				const Point pane{	// スワップチェーンパネルの左上点
					tran.TransformPoint(Point{ 0.0f, 0.0f })
				};
				Rect win_rect{	// ウィンドウのクライアント矩形
					Window::Current().CoreWindow().Bounds()
				};
				con_rect.X = win_rect.X + pane.X + left / m_main_scale;
				con_rect.Y = win_rect.Y + pane.Y + top / m_main_scale;
				con_rect.Width = width / m_main_scale;
				con_rect.Height = height / m_main_scale;

				const auto end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
				const auto start = m_main_page.m_select_start;
				const auto car_row = t->get_text_row(end);	// キャレット行
				const auto row_start = t->m_dwrite_test_metrics[car_row].textPosition;	// キャレット行の開始位置
				const auto row_end = row_start + t->m_dwrite_test_metrics[car_row].length;	// キャレット行の終了位置

				// 選択範囲が複数行の場合, 選択範囲の矩形をどう設定すれば適切なのか分からない.
				// とりあえず選択範囲はキャレットがある行に限定する.
				t->get_text_caret(max(min(start, end), row_start), car_row, false, sel_start);
				t->get_text_caret(min(max(start, end), row_end), car_row, false, sel_end);
				sel_rect.X = win_rect.X + pane.X + sel_start.x / m_main_scale;
				sel_rect.Y = win_rect.Y + pane.Y + sel_end.y / m_main_scale;
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



				/*
				const auto end = (m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end);
				const auto row = t->get_text_row(end);	// キャレットがある行
				// キャレット行の両端の座標を得る.
				const auto row_start = t->m_dwrite_test_metrics[row].textPosition;
				const auto row_end = row_start + t->m_dwrite_test_metrics[row].length;
				t->get_text_caret(row_start, row, false, con_start);
				t->get_text_caret(row_end, row, false, con_end);
				// キャレット行と選択範囲の共通部分の両端の座標を得る.
				const auto start = m_main_page.m_select_start;
				t->get_text_caret(max(start, row_start), row, false, sel_start);
				t->get_text_caret(min(end, row_end), row, false, sel_end);
				Rect win_rect{	// ウィンドウのクライアント矩形
					Window::Current().CoreWindow().Bounds()
				};
				GeneralTransform tran{	// 変換子
					scp_main_panel().TransformToVisual(nullptr)
				};
				const Point pane{	// スワップチェーンパネルの左上点
					tran.TransformPoint(Point{ 0.0f, 0.0f })
				};
				con_rect.X = win_rect.X + pane.X + con_start.x / m_main_scale;
				con_rect.Y = win_rect.Y + pane.Y + con_start.y / m_main_scale;
				con_rect.Width = (con_end.x - con_start.x) / m_main_scale;
				con_rect.Height = t->m_font_size / m_main_scale;
				sel_rect.X = win_rect.X + pane.X + sel_start.x / m_main_scale;
				sel_rect.Y = win_rect.Y + pane.Y + sel_start.y / m_main_scale;
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
				req.LayoutBounds().TextBounds(sel_rect);
				req.LayoutBounds().ControlBounds(con_rect);
				*/
			});
			// 入力変換が開始された
			m_edit_context.CompositionStarted([this](auto, auto) {
				//__debugbreak();
				// 変換開始時の文字列の選択範囲を保存しておく.
				m_edit_text_comp = true;
				m_edit_text_start = m_main_page.m_select_start;
				m_edit_text_end = m_main_page.m_select_end;
				m_edit_text_trail = m_main_page.m_select_trail;
				undo_push_null();

				m_edit_context.NotifyLayoutChanged();
			});
			m_edit_context.CompositionCompleted([this](auto, auto) {
				//__debugbreak();				
				m_edit_text_comp = false;
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
		// 図形リストとページをアンドゥ操作に格納する.
		{
			Undo::begin(&m_main_page.m_shape_list, &m_main_page);
		}

		// 背景パターン画像の読み込み.
		{
			background_get_brush();
		}

		auto _{ file_new_click_async(nullptr, nullptr) };
	}

	// メッセージダイアログを表示する.
	// 戻り値	なし
	void MainPage::message_show(
		winrt::hstring const& glyph,	// フォントアイコンのグリフの静的リソースのキー
		winrt::hstring const& message,	// メッセージのアプリケーションリソースのキー
		winrt::hstring const& desc	// 説明のアプリケーションリソースのキー
	)
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

	// メインのページの境界矩形を更新する.
	void MainPage::main_bbox_update(void) noexcept
	{
		// リスト中の図形を囲む矩形を得る.
		slist_bbox_shape(m_main_page.m_shape_list, m_main_bbox_lt, m_main_bbox_rb);

		// 矩形の右下点がページの右下点より小さいなら, ページの右下点を格納する.
		const auto rb_x = m_main_page.m_page_size.width - m_main_page.m_page_margin.left;
		if (m_main_bbox_rb.x < rb_x) {
			m_main_bbox_rb.x = rb_x;
		}
		const auto rb_y = m_main_page.m_page_size.height - m_main_page.m_page_margin.top;
		if (m_main_bbox_rb.y < rb_y) {
			m_main_bbox_rb.y = rb_y;
		}

		// 矩形の左上点がページの左上点より大きいなら, ページの左上点を格納する.
		const auto lb_x = -m_main_page.m_page_margin.left;
		if (m_main_bbox_lt.x > lb_x) {
			m_main_bbox_lt.x = lb_x;
		}
		const auto lb_y = -m_main_page.m_page_margin.left;
		if (m_main_bbox_lt.y > lb_y) {
			m_main_bbox_lt.y = lb_y;
		}
	}

	// メインのページを表示する.
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
		m_main_page.begin_draw(m_main_d2d.m_d2d_context.get(), true, m_wic_background.get(), m_main_scale);

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

		// 図形を (ページも) 表示する.
		m_main_page.draw();

		// 矩形選択している状態なら, 作図ツールに応じた補助線を表示する.
		if (m_event_state == EVENT_STATE::PRESS_RECT) {
			if (m_drawing_tool == DRAWING_TOOL::SELECT ||
				m_drawing_tool == DRAWING_TOOL::RECT ||
				m_drawing_tool == DRAWING_TOOL::TEXT ||
				m_drawing_tool == DRAWING_TOOL::RULER) {
				m_main_page.auxiliary_draw_rect(m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::BEZIER) {
				m_main_page.auxiliary_draw_bezi(m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::ELLIPSE) {
				m_main_page.auxiliary_draw_elli(m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::LINE) {
				m_main_page.auxiliary_draw_line(m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::RRECT) {
				m_main_page.auxiliary_draw_rrect(m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::POLY) {
				m_main_page.auxiliary_draw_poly(m_event_pos_pressed, m_event_pos_curr, m_drawing_poly_opt);
			}
			else if (m_drawing_tool == DRAWING_TOOL::ARC) {
				m_main_page.auxiliary_draw_arc(m_event_pos_pressed, m_event_pos_curr);
			}
		}

		if (m_edit_text_shape != nullptr && !m_edit_text_shape->is_deleted() && m_edit_text_shape->is_selected()) {
			m_edit_text_shape->draw_selection(m_main_page.m_select_start, m_main_page.m_select_end, m_main_page.m_select_trail);

			const int row = m_edit_text_shape->get_text_row(m_main_page.m_select_end);
			//const int row = m_edit_text_shape->get_text_row(m_edit_text_end);
			D2D1_POINT_2F car;	// キャレットの点
			m_edit_text_shape->get_text_caret(m_main_page.m_select_end, row, m_main_page.m_select_trail, car);
			//m_edit_text_shape->get_text_caret(m_edit_text_end, row, m_edit_text_trail, car);
			D2D1_POINT_2F p{
				car.x - 0.5f, car.y
			};
			D2D1_POINT_2F q{
				car.x - 0.5f, car.y + m_edit_text_shape->m_font_size
			};
			D2D1_POINT_2F r{
				car.x, car.y
			};
			D2D1_POINT_2F s{
				car.x, car.y + m_edit_text_shape->m_font_size
			};
			m_main_page.m_d2d_color_brush->SetColor(COLOR_WHITE);
			m_main_d2d.m_d2d_context->DrawLine(p, q, m_main_page.m_d2d_color_brush.get(), 2.0f);
			m_main_page.m_d2d_color_brush->SetColor(COLOR_BLACK);
			m_main_d2d.m_d2d_context->DrawLine(r, s, m_main_page.m_d2d_color_brush.get(), 1.0f);
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
