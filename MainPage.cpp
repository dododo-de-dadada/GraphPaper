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
	using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;
	using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
	using winrt::Windows::UI::Core::Preview::SystemNavigationManagerPreview;
	using winrt::Windows::UI::Xaml::Application;
	using winrt::Windows::UI::Xaml::Controls::Control;

	// 書式文字列
	constexpr auto FMT_INCH = L"%.3lf";	// インチ単位の書式
	constexpr auto FMT_INCH_UNIT = L"%.3lf in";// \u33CC";	// インチ単位の書式
	constexpr auto FMT_MILLI = L"%.3lf";	// ミリメートル単位の書式
	constexpr auto FMT_MILLI_UNIT = L"%.3lf mm";// \u339C";	// ミリメートル単位の書式
	constexpr auto FMT_POINT = L"%.2lf";	// ポイント単位の書式
	constexpr auto FMT_POINT_UNIT = L"%.2lf pt";	// ポイント単位の書式
	constexpr auto FMT_PIXEL = L"%.1lf";	// ピクセル単位の書式
	constexpr auto FMT_PIXEL_UNIT = L"%.1lf px";	// ピクセル単位の書式
	constexpr auto FMT_ZOOM = L"%.lf%%";	// 倍率の書式
	constexpr auto FMT_GRID = L"%.3lf";	// 方眼単位の書式
	constexpr auto FMT_GRID_UNIT = L"%.3lf grid";	// 方眼単位の書式

	// 色成分を文字列に変換する.
	// col_code	色成分の記法
	// col_comp	色成分の値 (0...255)
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
			swprintf_s(text_buf, text_len, L"%02x", static_cast<uint32_t>(std::round(col_comp)));
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
	// U	単位フラグ (true なら単位を付ける, false なら値のみ)
	// len_unit	長さの単位
	// len	ピクセル単位の長さ
	// dpi	DPI
	// grid_len	方眼の大きさ
	// text_len	文字列の最大長 ('\0' を含む長さ)
	// text_buf	文字列の配列
	template <bool U>
	void conv_len_to_str(const LEN_UNIT len_unit, const double len, const double dpi, const double grid_len, const uint32_t text_len, wchar_t* text_buf) noexcept
	{
		// 長さの単位がピクセルか判定する.
		if (len_unit == LEN_UNIT::PIXEL) {
			if constexpr (U) {
				swprintf_s(text_buf, text_len, FMT_PIXEL_UNIT, len);
			}
			else {
				swprintf_s(text_buf, text_len, FMT_PIXEL, len);
			}
		}
		// 長さの単位がインチか判定する.
		else if (len_unit == LEN_UNIT::INCH) {
			if constexpr (U) {
				swprintf_s(text_buf, text_len, FMT_INCH_UNIT, len / dpi);
			}
			else {
				swprintf_s(text_buf, text_len, FMT_INCH, len / dpi);
			}
		}
		// 長さの単位がミリメートルか判定する.
		else if (len_unit == LEN_UNIT::MILLI) {
			if constexpr (U) {
				swprintf_s(text_buf, text_len, FMT_MILLI_UNIT, len * MM_PER_INCH / dpi);
			}
			else {
				swprintf_s(text_buf, text_len, FMT_MILLI, len * MM_PER_INCH / dpi);
			}
		}
		// 長さの単位がポイントか判定する.
		else if (len_unit == LEN_UNIT::POINT) {
			if constexpr (U) {
				swprintf_s(text_buf, text_len, FMT_POINT_UNIT, len * PT_PER_INCH / dpi);
			}
			else {
				swprintf_s(text_buf, text_len, FMT_POINT, len * PT_PER_INCH / dpi);
			}
		}
		// 長さの単位が方眼か判定する.
		else if (len_unit == LEN_UNIT::GRID) {
			if constexpr (U) {
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

	void MainPage::menu_is_enable(void) noexcept
	{
		uint32_t undeleted_cnt = 0;	// 消去フラグがない図形の数
		uint32_t selected_cnt = 0;	// 選択された図形の数
		uint32_t selected_group = 0;	// 選択されたグループ図形の数
		uint32_t runlength_cnt = 0;	// 選択された図形の連続の数
		uint32_t selected_text = 0;	// 選択された文字列図形の数
		uint32_t undeleted_text = 0;	// 文字列図形の数
		uint32_t selected_line = 0;	// 選択された直線の数
		uint32_t selected_image = 0;	// 選択された画像図形の数
		uint32_t selected_ruler = 0;	// 選択された定規図形の数
		uint32_t selected_clockwise = 0;	// 選択された円弧図形の数
		uint32_t selected_counter_clockwise = 0;	// 選択された円弧図形の数
		uint32_t selected_polyline = 0;	// 選択された開いた多角形図形の数
		uint32_t selected_polygon = 0;	// 選択された閉じた多角形図形の数
		uint32_t selected_exist_cap = 0;	// 選択された端をもつ図形の数
		bool fore_selected = false;	// 最前面にある図形の選択フラグ
		bool back_selected = false;	// 最背面にある図形の選択フラグ
		//bool prev_selected = false;	// ひとつ背面の図形の選択フラグ
		slist_count(
			m_main_sheet.m_shape_list,
			undeleted_cnt,
			selected_cnt,
			selected_group,
			runlength_cnt,
			selected_text,
			undeleted_text,
			selected_line,
			selected_image,
			selected_ruler,
			selected_clockwise,
			selected_counter_clockwise,
			selected_polyline,
			selected_polygon,
			selected_exist_cap,
			fore_selected,
			back_selected//,
			//prev_selected
		);
		// 選択された図形がひとつ以上ある場合.
		const auto exists_selected = (m_undo_select_cnt > 0);
		// 選択された文字列がひとつ以上ある場合.
		const auto exists_selected_text = (selected_text > 0);
		// 文字列がひとつ以上ある場合.
		const auto exists_text = (undeleted_text > 0);
		// 選択された画像がひとつ以上ある場合.
		const auto exists_selected_image = (selected_image > 0);
		// 選択された定規がひとつ以上ある場合.
		const auto exists_selected_ruler = (selected_ruler > 0);
		// 選択された円弧がひとつ以上ある場合.
		const auto exists_selected_counter_clockwise = (selected_counter_clockwise > 0);
		// 選択された円弧がひとつ以上ある場合.
		const auto exists_selected_clockwise = (selected_clockwise > 0);
		// 選択された開いた多角形がひとつ以上ある場合.
		const auto exists_selected_polyline = (selected_polyline > 0);
		// 選択された閉じた多角形がひとつ以上ある場合.
		const auto exists_selected_polygon = (selected_polygon > 0);
		// 選択されてない図形がひとつ以上ある場合, または選択されてない文字がひとつ以上ある場合.
		const auto exists_unselected = (m_undo_select_cnt < m_undo_undeleted_cnt || core_text_len() - core_text_selected_len() > 0);
		// 選択された図形がふたつ以上ある場合.
		const auto exists_selected_2 = (m_undo_select_cnt > 1);
		// 選択されたグループがひとつ以上ある場合.
		const auto exists_selected_group = (selected_group > 0);
		// 選択された端のある図形がひとつ以上ある場合.
		const auto exists_selected_cap = (selected_exist_cap > 0);
		// 前面に配置可能か判定する.
		// 1. 複数のランレングスがある.
		// 2. または, 少なくとも 1 つは選択された図形があり, 
		//    かつ最前面の図形は選択されいない.
		const auto enable_forward = (runlength_cnt > 1 || (m_undo_select_cnt > 0 && !fore_selected));
		// 背面に配置可能か判定する.
		// 1. 複数のランレングスがある.
		// 2. または, 少なくとも 1 つは選択された図形があり, 
		//    かつ最背面の図形は選択されいない.
		const auto enable_backward = (runlength_cnt > 1 || (m_undo_select_cnt > 0 && !back_selected));
		const auto& dp_view = Clipboard::GetContent();
		const bool exists_data = (dp_view.Contains(CLIPBOARD_FORMAT_SHAPES) || dp_view.Contains(StandardDataFormats::Text()) || dp_view.Contains(StandardDataFormats::Bitmap()));
		const bool exists_fill = m_undo_select_cnt > selected_line + selected_image + selected_group;
		//const bool exists_stroke = m_undo_select_cnt > selected_group + selected_image + selected_ruler;

		// 元に戻すメニューの可否を設定する.
		popup_undo().IsEnabled(m_undo_stack.size() > 0);
		menu_undo().IsEnabled(m_undo_stack.size() > 0);
		popup_redo().IsEnabled(m_redo_stack.size() > 0);
		menu_redo().IsEnabled(m_redo_stack.size() > 0);

		// カット＆ペーストメニューの可否を設定する.
		popup_cut().IsEnabled(exists_selected);
		menu_cut().IsEnabled(exists_selected);
		popup_copy().IsEnabled(exists_selected);
		menu_copy().IsEnabled(exists_selected);
		popup_paste().IsEnabled(exists_data);
		menu_paste().IsEnabled(exists_data);
		popup_delete().IsEnabled(exists_selected);
		menu_delete().IsEnabled(exists_selected);
		popup_select_all().IsEnabled(exists_unselected);
		menu_select_all().IsEnabled(exists_unselected);

		// 並び替えメニューの可否を設定する.
		popup_order().IsEnabled(enable_forward || enable_backward);
		menu_order().IsEnabled(enable_forward || enable_backward);
		popup_bring_forward().IsEnabled(enable_forward);
		menu_bring_forward().IsEnabled(enable_forward);
		popup_bring_to_front().IsEnabled(enable_forward);
		menu_bring_to_front().IsEnabled(enable_forward);
		popup_send_to_back().IsEnabled(enable_backward);
		menu_send_to_back().IsEnabled(enable_backward);
		popup_send_backward().IsEnabled(enable_backward);
		menu_send_backward().IsEnabled(enable_backward);

		// グループ操作メニューの可否を設定する.
		popup_group().IsEnabled(exists_selected_2);
		menu_group().IsEnabled(exists_selected_2);
		popup_ungroup().IsEnabled(exists_selected_group);
		menu_ungroup().IsEnabled(exists_selected_group);

		popup_find_and_replace().IsEnabled(exists_text);
		menu_find_and_replace().IsEnabled(exists_text);

		// 図形編集メニューの可否を設定する.
		if (m_event_shape_pressed != nullptr && m_event_hit_pressed != HIT_TYPE::HIT_SHEET && 
			(exists_selected_cap || exists_selected_counter_clockwise || exists_selected_polygon || exists_selected_polyline || exists_selected_image)) {
			popup_edit_shape().Visibility(Visibility::Visible);
		}
		else {
			popup_edit_shape().Visibility(Visibility::Collapsed);
		}
		popup_reverse_path().IsEnabled(exists_selected_cap);
		menu_reverse_path().IsEnabled(exists_selected_cap);
		popup_draw_arc_cw().IsEnabled(exists_selected_counter_clockwise);
		menu_draw_arc_cw().IsEnabled(exists_selected_counter_clockwise);
		popup_draw_arc_ccw().IsEnabled(exists_selected_clockwise);
		menu_draw_arc_ccw().IsEnabled(exists_selected_clockwise);
		popup_open_polygon().IsEnabled(exists_selected_polygon);
		menu_open_polygon().IsEnabled(exists_selected_polygon);
		popup_close_polyline().IsEnabled(exists_selected_polyline);
		menu_close_polyline().IsEnabled(exists_selected_polyline);
		popup_revert_image().IsEnabled(exists_selected_image);
		menu_revert_image().IsEnabled(exists_selected_image);

		// 線枠メニューの可否を設定する.
		if (m_event_shape_pressed != nullptr && (m_event_shape_pressed->exist_cap() || m_event_shape_pressed->exist_join())) {
			popup_stroke().Visibility(Visibility::Visible);
		}
		else {
			popup_stroke().Visibility(Visibility::Collapsed);
		}
		mfi_popup_stroke_dash_pat().IsEnabled(m_main_sheet.m_stroke_dash != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		mfi_menu_stroke_dash_pat().IsEnabled(m_main_sheet.m_stroke_dash != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		mfi_popup_stroke_arrow_size().IsEnabled(m_main_sheet.m_arrow_style != ARROW_STYLE::ARROW_NONE);
		mfi_menu_stroke_arrow_size().IsEnabled(m_main_sheet.m_arrow_style != ARROW_STYLE::ARROW_NONE);

		// 塗りメニューの可否を設定する.
		if (m_event_shape_pressed != nullptr && m_event_shape_pressed->exist_fill()) {
			popup_fill().Visibility(Visibility::Visible);
		}
		else {
			popup_fill().Visibility(Visibility::Collapsed);
		}
		mfi_popup_fill_color().IsEnabled(exists_fill);
		mfi_popup_image_opacity().IsEnabled(exists_selected_image);

		// 書体メニューの可否を設定する.
		if (m_event_shape_pressed != nullptr && typeid(*m_event_shape_pressed) == typeid(ShapeText)) {
			popup_font().Visibility(Visibility::Visible);
		}
		else {
			popup_font().Visibility(Visibility::Collapsed);
		}
		mfi_popup_font_family().IsEnabled(exists_selected_text || exists_selected_ruler);
		mfi_popup_font_size().IsEnabled(exists_selected_text || exists_selected_ruler);
		mfsi_popup_font_weight().IsEnabled(exists_selected_text || exists_selected_ruler);
		mfsi_popup_font_stretch().IsEnabled(exists_selected_text || exists_selected_ruler);
		mfsi_popup_font_style().IsEnabled(exists_selected_text || exists_selected_ruler);
		mfsi_popup_text_align_horz().IsEnabled(exists_selected_text);
		mfsi_popup_text_align_vert().IsEnabled(exists_selected_text);
		popup_text_line_space().IsEnabled(exists_selected_text);
		mfi_popup_text_pad().IsEnabled(exists_selected_text);
		mfsi_popup_text_wrap().IsEnabled(exists_selected_text);
		mfi_popup_font_color().IsEnabled(exists_selected_text);

		// レイアウトメニューの可否を設定する.
		if (m_event_shape_pressed == nullptr || m_event_hit_pressed == HIT_TYPE::HIT_SHEET) {
			popup_layout().Visibility(Visibility::Visible);
		}
		else {
			popup_layout().Visibility(Visibility::Collapsed);
		}
	}

	//-------------------------------
	// メインページを作成する.
	//-------------------------------
	int _debug_edit = 0;
	MainPage::MainPage(void)
	{
		// お約束.
		InitializeComponent();

		//	winrt::Windows::UI::Xaml::UIElement::PointerPressedEvent(), box_value(winrt::Windows::UI::Xaml::Input::PointerEventHandler(
		//		[](auto) {
		//	int test = 0;
		//	})), true);
		// 「印刷」メニューの可否を設定する.
		//{
			//mfi_print().IsEnabled(PrintManager::IsSupported());
		//}

		// 文字入力のためのハンドラーを設定する.
		{
			core_text_setup_handler();
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
		m_main_sheet_focused = true;
		status_bar_debug().Text(L"m_main_sheet_focused = true");
		m_main_d2d.SetSwapChainPanel(scp_main_panel());
		main_sheet_draw();
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
			main_sheet_draw();
		}
		status_bar_set_pointer();
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
		const auto rb_x = m_main_sheet.m_sheet_size.width - m_main_sheet.m_sheet_padding.left;
		if (m_main_bbox_rb.x < rb_x) {
			m_main_bbox_rb.x = rb_x;
		}
		const auto rb_y = m_main_sheet.m_sheet_size.height - m_main_sheet.m_sheet_padding.top;
		if (m_main_bbox_rb.y < rb_y) {
			m_main_bbox_rb.y = rb_y;
		}

		// 矩形の左上点が用紙の左上点より大きいなら, 用紙の左上点を格納する.
		const auto lb_x = -m_main_sheet.m_sheet_padding.left;
		if (m_main_bbox_lt.x > lb_x) {
			m_main_bbox_lt.x = lb_x;
		}
		const auto lb_y = -m_main_sheet.m_sheet_padding.left;
		if (m_main_bbox_lt.y > lb_y) {
			m_main_bbox_lt.y = lb_y;
		}
	}

	// メインの用紙を表示する.
	void MainPage::main_sheet_draw(void)
	{
		if (!scp_main_panel().IsLoaded()) {
			return;
		}
		// ロックできないなら中断する.
		if (!m_mutex_draw.try_lock()) {
			return;
		}

		// 描画前に必要な変数を格納する.
		m_main_sheet.begin_draw(m_main_d2d.m_d2d_context.get(), true, m_background_wic.get(), m_main_scale);

		// 描画環境を保存, 描画を開始する.
		m_main_d2d.m_d2d_context->SaveDrawingState(SHAPE::m_state_block.get());
		m_main_d2d.m_d2d_context->BeginDraw();
		m_main_d2d.m_d2d_context->Clear(m_background_color);

		// 背景パターンを描画する,
		if (m_background_show) {
			const D2D1_RECT_F w_rect{	// ウィンドウの矩形
				0, 0, m_main_d2d.m_logical_width, m_main_d2d.m_logical_height
			};
			m_main_d2d.m_d2d_context->FillRectangle(w_rect, SHAPE::m_d2d_bitmap_brush.get());
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
			if (m_tool == DRAWING_TOOL::SELECT ||
				m_tool == DRAWING_TOOL::RECT ||
				m_tool == DRAWING_TOOL::TEXT ||
				m_tool == DRAWING_TOOL::RULER) {
				m_main_sheet.auxiliary_draw_rect(m_event_point_pressed, m_event_point_curr);
			}
			else if (m_tool == DRAWING_TOOL::BEZIER) {
				m_main_sheet.auxiliary_draw_bezi(m_event_point_pressed, m_event_point_curr);
			}
			else if (m_tool == DRAWING_TOOL::ELLIPSE) {
				m_main_sheet.auxiliary_draw_elli(m_event_point_pressed, m_event_point_curr);
			}
			else if (m_tool == DRAWING_TOOL::LINE || m_tool == DRAWING_TOOL::POINTER) {
				m_main_sheet.auxiliary_draw_line(m_event_point_pressed, m_event_point_curr);
			}
			else if (m_tool == DRAWING_TOOL::RRECT) {
				m_main_sheet.auxiliary_draw_rrect(m_event_point_pressed, m_event_point_curr);
			}
			else if (m_tool == DRAWING_TOOL::POLY) {
				m_main_sheet.auxiliary_draw_poly(m_event_point_pressed, m_event_point_curr, m_tool_polygon);
			}
			else if (m_tool == DRAWING_TOOL::ARC) {
				m_main_sheet.auxiliary_draw_arc(m_event_point_pressed, m_event_point_curr);
			}
		}

		if (m_core_text_focused != nullptr && !m_core_text_focused->is_deleted() && m_core_text_focused->is_selected()) {
			m_core_text_focused->draw_selection(m_main_sheet.m_core_text_range.m_start, m_main_sheet.m_core_text_range.m_end, m_main_sheet.m_core_text_range.m_trail);

			const int row = m_core_text_focused->get_text_row(m_main_sheet.m_core_text_range.m_end);
			const float hight = m_core_text_focused->m_font_size;
			D2D1_POINT_2F car;	// キャレットの点
			m_core_text_focused->get_text_caret(m_main_sheet.m_core_text_range.m_end, row, m_main_sheet.m_core_text_range.m_trail, car);
			D2D1_POINT_2F p{
				car.x - 0.5f, car.y
			};
			D2D1_POINT_2F q{
				car.x - 0.5f, car.y + hight
			};
			D2D1_POINT_2F r{
				car.x, car.y
			};
			D2D1_POINT_2F s{
				car.x, car.y + hight
			};
			m_main_sheet.m_d2d_color_brush->SetColor(COLOR_WHITE);
			m_main_d2d.m_d2d_context->DrawLine(p, q, m_main_sheet.m_d2d_color_brush.get(), 2.0f);
			m_main_sheet.m_d2d_color_brush->SetColor(COLOR_BLACK);
			m_main_d2d.m_d2d_context->DrawLine(r, s, m_main_sheet.m_d2d_color_brush.get(), 1.0f);
		}

		// 描画を終了し結果を得る. 保存された描画環境を元に戻す.
		const HRESULT hres = m_main_d2d.m_d2d_context->EndDraw();
		m_main_d2d.m_d2d_context->RestoreDrawingState(SHAPE::m_state_block.get());

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


void winrt::GraphPaper::implementation::MainPage::menu_getting_focus(UIElement const& sender, GettingFocusEventArgs const&)
{
		if (m_core_text_focused != nullptr && m_core_text_comp) {
			m_core_text.NotifyFocusLeave();
		}
}
