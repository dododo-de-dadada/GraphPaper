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
	// UWP のブラシを D2D1_COLOR_F に変換する.
	static bool conv_uwp_to_dx(const Brush& a, D2D1_COLOR_F& b) noexcept;

	// UWP の色を D2D1_COLOR_F に変換する.
	static void conv_uwp_to_dx(const Color& a, D2D1_COLOR_F& b) noexcept;

	// 色成分を文字列に変換する.
	void conv_col_to_str(const COLOR_CODE c_code, const double value, const size_t t_len, wchar_t t_buf[]);

	constexpr double UWP_COLOR_MAX = 255.0;	// UWP の色成分の最大値

	// UWP のブラシを D2D1_COLOR_F に変換する.
	static bool conv_uwp_to_dx(const Brush& a, D2D1_COLOR_F& b) noexcept
	{
		using winrt::Windows::UI::Xaml::Media::SolidColorBrush;

		const auto brush = a.try_as<SolidColorBrush>();
		if (brush == nullptr) {
			return false;
		}
		const auto color = brush.Color();
		b.r = static_cast<FLOAT>(static_cast<double>(color.R) / UWP_COLOR_MAX);
		b.g = static_cast<FLOAT>(static_cast<double>(color.G) / UWP_COLOR_MAX);
		b.b = static_cast<FLOAT>(static_cast<double>(color.B) / UWP_COLOR_MAX);
		b.a = static_cast<FLOAT>(static_cast<double>(color.A) / UWP_COLOR_MAX);
		return true;
	}

	// UWP の色を D2D1_COLOR_F に変換する.
	static void conv_uwp_to_dx(const Color& a, D2D1_COLOR_F& b) noexcept
	{
		b.r = static_cast<FLOAT>(static_cast<double>(a.R) / UWP_COLOR_MAX);
		b.g = static_cast<FLOAT>(static_cast<double>(a.G) / UWP_COLOR_MAX);
		b.b = static_cast<FLOAT>(static_cast<double>(a.B) / UWP_COLOR_MAX);
		b.a = static_cast<FLOAT>(static_cast<double>(a.A) / UWP_COLOR_MAX);
	}

	// 色成分を文字列に変換する.
	// c_code	色の表記
	// value	色成分の値
	// t_len	文字列の最大長 ('\0' を含む長さ)
	// t_buf	文字列の配列 [t_len]
	// 戻り値	なし
	void conv_col_to_str(const COLOR_CODE c_code, const double value, const size_t t_len, wchar_t t_buf[])
	{
		// 色の表記が 10 進数か判定する.
		if (c_code == COLOR_CODE::DEC) {
			swprintf_s(t_buf, t_len, L"%.0lf", std::round(value));
		}
		// 色の表記が 16 進数か判定する.
		else if (c_code == COLOR_CODE::HEX) {
			swprintf_s(t_buf, t_len, L"%02X", static_cast<uint32_t>(std::round(value)));
		}
		// 色の表記が実数か判定する.
		else if (c_code == COLOR_CODE::REAL) {
			swprintf_s(t_buf, t_len, L"%.4lf", value / COLOR_MAX);
		}
		// 色の表記がパーセントか判定する.
		else if (c_code == COLOR_CODE::CENT) {
			swprintf_s(t_buf, t_len, L"%.1lf%%", value / COLOR_MAX * 100.0);
		}
		else {
			throw hresult_not_implemented();
		}
	}

	// 長さを文字列に変換する.
	// B	単位付加フラグ
	// len_unit	長さの単位
	// value	ピクセル単位の長さ
	// dpi	DPI
	// g_len	方眼の大きさ
	// t_buf	文字列の配列
	// t_len	文字列の最大長 ('\0' を含む長さ)
	template <bool B> void conv_len_to_str(const LEN_UNIT len_unit, const double value, const double dpi, const double g_len, const uint32_t t_len, wchar_t *t_buf)
	{
		if (len_unit == LEN_UNIT::PIXEL) {
			if constexpr (B == LEN_UNIT_SHOW) {
				swprintf_s(t_buf, t_len, FMT_PIXEL_UNIT, value);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_PIXEL, value);
			}
		}
		else if (len_unit == LEN_UNIT::INCH) {
			if constexpr (B == LEN_UNIT_SHOW) {
				swprintf_s(t_buf, t_len, FMT_INCH_UNIT, value / dpi);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_INCH, value / dpi);
			}
		}
		else if (len_unit == LEN_UNIT::MILLI) {
			if constexpr (B == LEN_UNIT_SHOW) {
				swprintf_s(t_buf, t_len, FMT_MILLI_UNIT, value * MM_PER_INCH / dpi);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_MILLI, value * MM_PER_INCH / dpi);
			}
		}
		else if (len_unit == LEN_UNIT::POINT) {
			if constexpr (B == LEN_UNIT_SHOW) {
				swprintf_s(t_buf, t_len, FMT_POINT_UNIT, value * PT_PER_INCH / dpi);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_POINT, value * PT_PER_INCH / dpi);
			}
		}
		else if (len_unit == LEN_UNIT::GRID) {
			if constexpr (B == LEN_UNIT_SHOW) {
				swprintf_s(t_buf, t_len, FMT_GRID_UNIT, value / g_len);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_GRID, value / g_len);
			}
		}
		else {
			throw hresult_not_implemented();
		}
	}

	// 長さを文字列に変換する (単位なし).
	template void conv_len_to_str<LEN_UNIT_HIDE>(const LEN_UNIT len_unit, const double value, const double dpi, const double g_len, const uint32_t t_len, wchar_t* t_buf);

	// 長さを文字列に変換する (単位つき).
	template void conv_len_to_str<LEN_UNIT_SHOW>(const LEN_UNIT len_unit, const double value, const double dpi, const double g_len, const uint32_t t_len, wchar_t* t_buf);

	// 内容が変更されていたなら, 確認ダイアログを表示してその応答を得る.
	// 戻り値	確認前の処理を続行するなら true を, 応答がキャンセルなら, または内容を保存できなかったなら false を返す.
	IAsyncOperation<bool> MainPage::ask_for_conf_async(void)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		// 操作スタックの更新フラグが立っているか判定する.
		if (undo_pushed()) {
			// 確認ダイアログを表示し, 結果を得る.
			const auto dres = co_await cd_conf_save().ShowAsync();	// ダイアログの結果
			// ダイアログの結果がキャンセルか判定する.
			if (dres == ContentDialogResult::None) {
				co_return false;
			}
			// ダイアログの結果が「保存する」か判定する.
			else if (dres == ContentDialogResult::Primary) {
				// ファイルに非同期に保存し, 結果が S_OK 以外か判定する.
				if (co_await file_save_async() != S_OK) {
					co_return false;
				}
			}
		}
		co_return true;
	}

	// 編集メニュー項目の使用の可否を設定する.
	// 選択の有無やクラスごとに図形を数え, メニュー項目の可否を判定する.
	void MainPage::edit_menu_enable(void)
	{
		using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;

		// 元に戻す/やり直しメニュー項目の使用の可否を設定する.
		undo_menu_enable();

		uint32_t undeleted_cnt = 0;	// 消去フラグがない図形の数
		uint32_t selected_cnt = 0;	// 選択された図形の数
		uint32_t selected_group_cnt = 0;	// 選択されたグループ図形の数
		uint32_t runlength_cnt = 0;	// 選択された図形のランレングスの数
		uint32_t selected_text_cnt = 0;	// 選択された文字列図形の数
		uint32_t text_cnt = 0;	// 文字列図形の数
		bool fore_selected = false;	// 最前面の図形の選択フラグ
		bool back_selected = false;	// 最背面の図形の選択フラグ
		bool prev_selected = false;	// ひとつ背面の図形の選択フラグ
		s_list_count(m_list_shapes,
			undeleted_cnt,
			selected_cnt,
			selected_group_cnt,
			runlength_cnt,
			selected_text_cnt,
			text_cnt,
			fore_selected,
			back_selected,
			prev_selected
		);

		// 消去されていない図形がひとつ以上ある場合.
		const auto exists_undeleted = (undeleted_cnt > 0);
		// 選択された図形がひとつ以上ある場合.
		const auto exists_selected = (selected_cnt > 0);
		// 選択された文字列図形がひとつ以上ある場合.
		const auto exists_selected_text = (selected_text_cnt > 0);
		// 文字列図形がひとつ以上ある場合.
		const auto exists_text = (text_cnt > 0);
		// 選択されてない図形がひとつ以上ある場合.
		const auto exists_unselected = (selected_cnt < undeleted_cnt);
		// 選択された図形がふたつ以上ある場合.
		const auto exists_selected_2 = (selected_cnt > 1);
		// 選択されたグループ図形がひとつ以上ある場合.
		const auto exists_selected_group = (selected_group_cnt > 0);
		// 前面に配置可能か判定する.
		// 1. 複数のランレングスがある.
		// 2. または, 少なくとも 1 つは選択された図形があり, 
		//    かつ最前面の図形は選択されいない.
		const auto enable_forward = (runlength_cnt > 1 || (exists_selected && fore_selected != true));
		// 背面に配置可能か判定する.
		// 1. 複数のランレングスがある.
		// 2. または, 少なくとも 1 つは選択された図形があり, 
		//    かつ最背面の図形は選択されいない.
		const auto enable_backward = (runlength_cnt > 1 || (exists_selected && back_selected != true));

		mfi_xcvd_cut().IsEnabled(exists_selected);
		mfi_xcvd_copy().IsEnabled(exists_selected);
		mfi_xcvd_paste().IsEnabled(xcvd_contains({ CF_GPD, StandardDataFormats::Text() }));
		mfi_xcvd_delete().IsEnabled(exists_selected);
		mfi_select_all().IsEnabled(exists_unselected);
		mfi_group().IsEnabled(exists_selected_2);
		mfi_ungroup().IsEnabled(exists_selected_group);
		mfi_text_edit().IsEnabled(exists_selected_text);
		mfi_text_find().IsEnabled(exists_text);
		mfi_bring_forward().IsEnabled(enable_forward);
		mfi_bring_to_front().IsEnabled(enable_forward);
		mfi_send_to_back().IsEnabled(enable_backward);
		mfi_send_backward().IsEnabled(enable_backward);
		mfi_summary().IsEnabled(exists_undeleted);
		m_cnt_selected = selected_cnt;
	}

	// ファイルメニューの「終了」が選択された
	IAsyncAction MainPage::exit_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::UI::Xaml::Application;

		if ((co_await ask_for_conf_async()) == false) {
			co_return;
		}
		// 図形一覧の排他制御が true か判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			summary_close();
		}
		undo_clear();
		s_list_clear(m_list_shapes);
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			message_show(ICON_ALERT, DEBUG_MSG, {});
		}
#endif
		ShapeText::release_available_fonts();

		// 静的リソースから読み込んだコンテキストメニューを破棄する.
		{
			m_menu_stroke = nullptr;
			m_menu_fill = nullptr;
			m_menu_font = nullptr;
			m_menu_sheet = nullptr;
			m_menu_ungroup = nullptr;
		}

		// コードビハインドで設定したハンドラーの設定を解除する.
		{
			using winrt::Windows::UI::Xaml::Application;
			using winrt::Windows::UI::Core::Preview::SystemNavigationManagerPreview;

			auto const& app{ Application::Current() };
			app.Suspending(m_token_suspending);
			app.Resuming(m_token_resuming);
			app.EnteredBackground(m_token_entered_background);
			app.LeavingBackground(m_token_leaving_background);
			auto const& thread{ CoreWindow::GetForCurrentThread() };
			thread.Activated(m_token_activated);
			thread.VisibilityChanged(m_token_visibility_changed);
			auto const& disp{ DisplayInformation::GetForCurrentView() };
			disp.DpiChanged(m_token_dpi_changed);
			disp.OrientationChanged(m_token_orientation_changed);
			disp.DisplayContentsInvalidated(m_token_contents_invalidated);
			SystemNavigationManagerPreview::GetForCurrentView().CloseRequested(m_token_close_requested);
		}

		// 本来なら DirectX をコードビハインドでリリースしたいところだが,
		// このあとスワップチェーンパネルの SizeChanged が呼び出されることがあるため,
		// Trim を呼び出すだけにする.
		m_sheet_dx.Trim();
		m_sample_dx.Trim();

		// アプリケーションを終了する.
		Application::Current().Exit();
	}

	// メインページを作成する.
	MainPage::MainPage(void)
	{
		// お約束.
		InitializeComponent();

		// コンテキストメニューを静的リソースから読み込む.
		// ポップアップは静的なリソースとして定義して、複数の要素で使用することができる.
		{
			m_menu_stroke = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_stroke")));
			m_menu_fill = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_fill")));
			m_menu_font = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_font")));
			m_menu_sheet = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_sheet")));
			m_menu_ungroup = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_ungroup")));
		}

		// アプリケーションの中断・継続などのイベントハンドラーを設定する.
		{
			using winrt::Windows::UI::Xaml::Application;

			auto const& app{ Application::Current() };
			m_token_suspending = app.Suspending({ this, &MainPage::app_suspending_async });
			m_token_resuming = app.Resuming({ this, &MainPage::app_resuming_async });
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
			m_token_dpi_changed = disp.DpiChanged({ this, &MainPage::disp_dpi_changed });
			m_token_orientation_changed = disp.OrientationChanged({ this, &MainPage::disp_orientation_changed });
			m_token_contents_invalidated = disp.DisplayContentsInvalidated({ this, &MainPage::disp_contents_invalidated });
		}

		// アプリケーションを閉じる前の確認のハンドラーを設定する.
		{
			using winrt::Windows::UI::Core::Preview::SystemNavigationManagerPreview;
			m_token_close_requested = SystemNavigationManagerPreview::GetForCurrentView().CloseRequested({ this, &MainPage::navi_close_requested });
		}

		// クリップボードに受け入れ可能なフォーマットを設定する.
		{
			using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;
			using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;

			auto const& dp_view = Clipboard::GetContent();
			dp_view.SetAcceptedFormatId(CF_GPD);
			dp_view.SetAcceptedFormatId(StandardDataFormats::Text());
		}

		// D2D/DWRITE ファクトリを図形クラスに, 
		// 図形リストと用紙をアンドゥ操作に格納する.
		{
			Shape::s_d2d_factory = m_sheet_dx.m_d2dFactory.get();
			Shape::s_dwrite_factory = m_sheet_dx.m_dwriteFactory.get();
			Undo::set(&m_list_shapes, &m_sheet_main);
		}

		// クリックの判定時間と判定距離をシステムから得る.
		{
			using winrt::Windows::UI::ViewManagement::UISettings;

			m_pointer_click_time = static_cast<uint64_t>(UISettings().DoubleClickTime()) * 1000L;
			auto const raw_dpi = DisplayInformation::GetForCurrentView().RawDpiX();
			auto const log_dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			m_pointer_click_dist = 6.0 * raw_dpi / log_dpi;
		}

		/*{
			if (rmfi_tool_select().IsChecked()) {
				m_tool_draw = TOOL_DRAW::SELECT;
			}
			else if (rmfi_tool_draw_rect().IsChecked()) {
				m_tool_draw = TOOL_DRAW::RECT;
			}
			else if (rmfi_tool_draw_rrect().IsChecked()) {
				m_tool_draw = TOOL_DRAW::RRECT;
			}
			else if (rmfi_tool_draw_poly().IsChecked()) {
				m_tool_draw = TOOL_DRAW::POLY;
			}
			else if (rmfi_tool_draw_elli().IsChecked()) {
				m_tool_draw = TOOL_DRAW::ELLI;
			}
			else if (rmfi_tool_draw_line().IsChecked()) {
				m_tool_draw = TOOL_DRAW::LINE;
			}
			else if (rmfi_tool_draw_bezi().IsChecked()) {
				m_tool_draw = TOOL_DRAW::BEZI;
			}
			if (rmfi_tool_poly_tri().IsChecked()) {
				m_tool_poly.m_vertex_cnt = 3;
			}
			else if (rmfi_tool_poly_quad().IsChecked()) {
				m_tool_poly.m_vertex_cnt = 4;
			}
			else if (rmfi_tool_poly_pent().IsChecked()) {
				m_tool_poly.m_vertex_cnt = 5;
			}
			else if (rmfi_tool_poly_hexa().IsChecked()) {
				m_tool_poly.m_vertex_cnt = 6;
			}
			else if (rmfi_tool_poly_hept().IsChecked()) {
				m_tool_poly.m_vertex_cnt = 7;
			}
			else if (rmfi_tool_poly_octa().IsChecked()) {
				m_tool_poly.m_vertex_cnt = 8;
			}
			else if (rmfi_tool_poly_nona().IsChecked()) {
				m_tool_poly.m_vertex_cnt = 9;
			}
			else if (rmfi_tool_poly_deca().IsChecked()) {
				m_tool_poly.m_vertex_cnt = 10;
			}
			m_tool_poly.m_regular = tmfi_tool_poly_regular().IsChecked();
			m_tool_poly.m_vertex_up = tmfi_tool_poly_vertex_up().IsChecked();
			m_tool_poly.m_closed = tmfi_tool_poly_closed().IsChecked();
			m_tool_poly.m_clockwise = tmfi_tool_poly_clockwise().IsChecked();
		}
		*/
		auto _{ new_click_async(nullptr, nullptr) };
	}

	// メッセージダイアログを表示する.
	// glyph_key	フォントアイコンのグリフの静的リソースのキー
	// message_key	メッセージのアプリケーションリソースのキー
	// desc_key		説明文のアプリケーションリソースのキー
	// 戻り値	なし
	void MainPage::message_show(winrt::hstring const& glyph_key, winrt::hstring const& message_key, winrt::hstring const& desc_key)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialog;
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogButton;
		const wchar_t QUOT[] = L"\"";	// 引用符
		const wchar_t NEW_LINE[] = L"\u2028";	// テキストブロック内での改行

		auto const& r_loader = ResourceLoader::GetForCurrentView();
		winrt::hstring text;
		try {
			text = r_loader.GetString(message_key);
		}
		catch (winrt::hresult_error const&) {
		}
		if (text.empty()) {
			// 文字列が空の場合,
			text = message_key;
		}
		winrt::hstring added_text;
		try {
			added_text = r_loader.GetString(desc_key);
		}
		catch (winrt::hresult_error const&) {}
		if (added_text.empty() != true) {
			// 追加する文字列が空でない場合,
			text = text + NEW_LINE + added_text;
		}
		else if (desc_key.empty() != true) {
			// 説明そのものが空でない場合,
			text = text + NEW_LINE + QUOT + desc_key + QUOT;
		}
		const auto glyph = Resources().TryLookup(box_value(glyph_key));
		fi_message().Glyph(glyph != nullptr ? unbox_value<winrt::hstring>(glyph) : glyph_key);
		tk_message().Text(text);
		auto _{ cd_message().ShowAsync() };
	}

	// ファイルメニューの「新規」が選択された
	IAsyncAction MainPage::new_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		if ((co_await ask_for_conf_async()) == false) {
			co_return;
		}
		// 図形一覧の排他制御が true か判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			summary_close();
		}
		undo_clear();
		s_list_clear(m_list_shapes);
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			// 「メモリリーク」メッセージダイアログを表示する.
			message_show(ICON_ALERT, DEBUG_MSG, {});
		}
#endif
		ShapeText::release_available_fonts();

		ShapeText::set_available_fonts();

		// 背景色, 前景色, 文字範囲の背景色, 文字範囲の文字色をリソースから得る.
		{
			using winrt::Windows::UI::Color;
			using winrt::Windows::UI::Xaml::Media::Brush;

			auto sel_back_color = Resources().TryLookup(box_value(L"SystemColorHighlightColor"));
			auto sel_text_color = Resources().TryLookup(box_value(L"SystemColorHighlightTextColor"));
			if (sel_back_color != nullptr && sel_text_color != nullptr) {
				conv_uwp_to_dx(unbox_value<Color>(sel_back_color), m_sheet_dx.m_range_background);
				conv_uwp_to_dx(unbox_value<Color>(sel_text_color), m_sheet_dx.m_range_foreground);
			}
			else {
				m_sheet_dx.m_range_background = { 0.0f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f };
				m_sheet_dx.m_range_foreground = S_WHITE;
			}
			auto const& back_theme = Resources().TryLookup(box_value(L"ApplicationPageBackgroundThemeBrush"));
			auto const& fore_theme = Resources().TryLookup(box_value(L"ApplicationForegroundThemeBrush"));
			if (back_theme != nullptr && fore_theme != nullptr) {
				conv_uwp_to_dx(unbox_value<Brush>(back_theme), m_sheet_dx.m_theme_background);
				conv_uwp_to_dx(unbox_value<Brush>(fore_theme), m_sheet_dx.m_theme_foreground);
			}
			else {
				m_sheet_dx.m_theme_background = S_WHITE;
				m_sheet_dx.m_theme_foreground = S_BLACK;
			}
			m_sample_dx.m_range_background = m_sheet_dx.m_range_background;
			m_sample_dx.m_range_foreground = m_sheet_dx.m_range_foreground;
			m_sample_dx.m_theme_background = m_sheet_dx.m_theme_background;
			m_sample_dx.m_theme_foreground = m_sheet_dx.m_theme_foreground;
		}

		if (co_await pref_load_async() != S_OK) {
			// 読み込みに失敗した場合,
			sheet_init();
		}

		// 用紙の左上位置と右下位置を初期化する.
		{
			sheet_min(D2D1_POINT_2F{ 0.0F, 0.0F });
			sheet_max(D2D1_POINT_2F{ m_sheet_main.m_sheet_size.width, m_sheet_main.m_sheet_size.height });
		}
		file_recent_add(nullptr);
		file_finish_reading();
	}

}
