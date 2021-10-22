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
	using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::Foundation::IAsyncOperation;
	using winrt::Windows::Graphics::Display::DisplayInformation;
	using winrt::Windows::UI::Color;
	using winrt::Windows::UI::Core::CoreWindow;
	using winrt::Windows::UI::Core::Preview::SystemNavigationManagerPreview;
	using winrt::Windows::UI::ViewManagement::UISettings;
	using winrt::Windows::UI::Xaml::Application;
	using winrt::Windows::UI::Xaml::Controls::ContentDialog;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogButton;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::MenuFlyout;
	using winrt::Windows::UI::Xaml::Media::Brush;
	using winrt::Windows::UI::Xaml::RoutedEventArgs;

	// UWP のブラシを D2D1_COLOR_F に変換する.
	//static bool conv_uwp_to_color(const Brush& a, D2D1_COLOR_F& b) noexcept;

	// UWP の色を D2D1_COLOR_F に変換する.
	//void conv_uwp_to_color(const Color& a, D2D1_COLOR_F& b) noexcept;

	// 色成分を文字列に変換する.
	void conv_col_to_str(const COLOR_CODE c_code, const double value, const size_t t_len, wchar_t t_buf[]) noexcept;

	// UWP のブラシを D2D1_COLOR_F に変換する.
	/*
	static bool conv_uwp_to_color(const Brush& a, D2D1_COLOR_F& b) noexcept
	{
		using winrt::Windows::UI::Xaml::Media::SolidColorBrush;

		const auto brush = a.try_as<SolidColorBrush>();
		if (brush == nullptr) {
			return false;
		}
		const auto color = brush.Color();
		conv_uwp_to_color(color, b);
		return true;
	}
	*/

	// 色成分を文字列に変換する.
	// c_code	色の表記
	// c_value	色成分の値
	// t_len	文字列の最大長 ('\0' を含む長さ)
	// t_buf	文字列の配列 [t_len]
	// 戻り値	なし
	void conv_col_to_str(const COLOR_CODE c_code, const double c_value, const size_t t_len, wchar_t t_buf[]) noexcept
	{
		// 色の表記が 10 進数か判定する.
		if (c_code == COLOR_CODE::DEC) {
			swprintf_s(t_buf, t_len, L"%.0lf", std::round(c_value));
		}
		// 色の表記が 16 進数か判定する.
		else if (c_code == COLOR_CODE::HEX) {
			swprintf_s(t_buf, t_len, L"%02X", static_cast<uint32_t>(std::round(c_value)));
		}
		// 色の表記が実数か判定する.
		else if (c_code == COLOR_CODE::REAL) {
			swprintf_s(t_buf, t_len, L"%.4lf", c_value / COLOR_MAX);
		}
		// 色の表記がパーセントか判定する.
		else if (c_code == COLOR_CODE::CENT) {
			swprintf_s(t_buf, t_len, L"%.1lf%%", c_value / COLOR_MAX * 100.0);
		}
		else {
			swprintf_s(t_buf, t_len, L"?");
		}
	}

	// 長さを文字列に変換する.
	// B	単位付加フラグ
	// len_unit	長さの単位
	// pixel_val	ピクセル単位の長さ
	// dpi	DPI
	// g_len	方眼の大きさ
	// t_len	文字列の最大長 ('\0' を含む長さ)
	// t_buf	文字列の配列
	template <bool B> void conv_len_to_str(const LEN_UNIT len_unit, const float pixel_val, const float dpi, const float g_len, const uint32_t t_len, wchar_t *t_buf) noexcept
	{
		// 長さの単位がピクセルか判定する.
		if (len_unit == LEN_UNIT::PIXEL) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_PIXEL_UNIT, pixel_val);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_PIXEL, pixel_val);
			}
		}
		// 長さの単位がインチか判定する.
		else if (len_unit == LEN_UNIT::INCH) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_INCH_UNIT, pixel_val / dpi);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_INCH, pixel_val / dpi);
			}
		}
		// 長さの単位がミリメートルか判定する.
		else if (len_unit == LEN_UNIT::MILLI) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_MILLI_UNIT, pixel_val * MM_PER_INCH / dpi);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_MILLI, pixel_val * MM_PER_INCH / dpi);
			}
		}
		// 長さの単位がポイントか判定する.
		else if (len_unit == LEN_UNIT::POINT) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_POINT_UNIT, pixel_val * PT_PER_INCH / dpi);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_POINT, pixel_val * PT_PER_INCH / dpi);
			}
		}
		// 長さの単位が方眼か判定する.
		else if (len_unit == LEN_UNIT::GRID) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_GRID_UNIT, pixel_val / g_len);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_GRID, pixel_val / g_len);
			}
		}
		else {
			swprintf_s(t_buf, t_len, L"?");
		}
	}

	// 長さを文字列に変換する (単位なし).
	template void conv_len_to_str<LEN_UNIT_HIDE>(const LEN_UNIT len_unit, const float pixel_val, const float dpi, const float g_len, const uint32_t t_len, wchar_t* t_buf) noexcept;

	// 長さを文字列に変換する (単位つき).
	template void conv_len_to_str<LEN_UNIT_SHOW>(const LEN_UNIT len_unit, const float pixel_val, const float dpi, const float g_len, const uint32_t t_len, wchar_t* t_buf) noexcept;

	// 確認ダイアログを表示してその応答を得る.
	// 戻り値	「保存する」または「保存しない」が押されたなら true を, 応答がキャンセルなら, または内容を保存できなかったなら false を返す.
	IAsyncOperation<bool> MainPage::ask_for_conf_async(void)
	{
		// 確認ダイアログを表示し, 応答を得る.
		const ContentDialogResult d_res = co_await cd_conf_save_dialog().ShowAsync();
		// 応答が「キャンセル」か判定する.
		if (d_res == ContentDialogResult::None) {
			co_return false;
		}
		// 応答が「保存する」か判定する.
		else if (d_res == ContentDialogResult::Primary) {
			// ファイルに非同期に保存し, 結果が S_OK 以外か判定する.
			if (co_await file_save_async() != S_OK) {
				co_return false;
			}
		}
		co_return true;
	}

	// ファイルメニューの「終了」が選択された
	IAsyncAction MainPage::exit_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		// スタックが更新された, かつ確認ダイアログの応答が「キャンセル」か判定する.
		if (m_ustack_updt && !co_await ask_for_conf_async()) {
			co_return;
		}
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_close_click(nullptr, nullptr);
		}
		ustack_clear();
		slist_clear(m_main_sheet.m_shape_list);
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
			m_menu_ruler = nullptr;
			m_menu_image = nullptr;
		}

		// コードビハインドで設定したハンドラーの設定を解除する.
		{
			auto const& app{ Application::Current() };
			app.Suspending(m_token_suspending);
			app.Resuming(m_token_resuming);
			app.EnteredBackground(m_token_entered_background);
			app.LeavingBackground(m_token_leaving_background);
			auto const& thread{ CoreWindow::GetForCurrentThread() };
			thread.Activated(m_token_activated);
			thread.VisibilityChanged(m_token_visibility_changed);
			//auto const& disp{ DisplayInformation::GetForCurrentView() };
			//disp.DpiChanged(m_token_dpi_changed);
			//disp.OrientationChanged(m_token_orientation_changed);
			//disp.DisplayContentsInvalidated(m_token_contents_invalidated);
			SystemNavigationManagerPreview::GetForCurrentView().CloseRequested(m_token_close_requested);
		}

		// 本来なら DirectX をコードビハインドでリリースしたいところだが,
		// このあとスワップチェーンパネルの SizeChanged が呼び出されることがあるため,
		// Trim を呼び出すだけにする.
		m_main_d2d.Trim();
		m_sample_d2d.Trim();

		// アプリケーションを終了する.
		Application::Current().Exit();
	}

	// メインページを作成する.
	MainPage::MainPage(void)
	{
		// お約束.
		InitializeComponent();

		// アプリケーションの中断・継続などのイベントハンドラーを設定する.
		{
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
			m_token_dpi_changed = disp.DpiChanged({ this, &MainPage::display_dpi_changed });
			m_token_orientation_changed = disp.OrientationChanged({ this, &MainPage::display_orientation_changed });
			m_token_contents_invalidated = disp.DisplayContentsInvalidated({ this, &MainPage::display_contents_invalidated });
		}

		// アプリケーションを閉じる前の確認のハンドラーを設定する.
		{
			m_token_close_requested = SystemNavigationManagerPreview::GetForCurrentView().CloseRequested({ this, &MainPage::navi_close_requested });
		}

		// D2D/DWRITE ファクトリを図形クラスに, 
		// 図形リストと用紙をアンドゥ操作に格納する.
		{
			//Shape::s_dx = &m_main_d2d;
			//Shape::s_d2d_factory = m_main_d2d.m_d2d_factory.get();
			//Shape::s_dwrite_factory = m_main_d2d.m_dwrite_factory.get();
			Shape::m_aux_style = nullptr;
			winrt::check_hresult(
				m_main_d2d.m_d2d_factory->CreateStrokeStyle(AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT, Shape::m_aux_style.put())
			);
			Undo::set(&m_main_sheet.m_shape_list, &m_main_sheet);
		}

		// クリックの判定時間と判定距離をシステムから得る.
		{
			m_event_click_time = static_cast<uint64_t>(UISettings().DoubleClickTime()) * 1000L;
			auto const raw_dpi = DisplayInformation::GetForCurrentView().RawDpiX();
			auto const log_dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			m_event_click_dist = 6.0 * raw_dpi / log_dpi;
		}

		// コンテキストメニューを静的リソースから読み込む.
		// ポップアップは静的なリソースとして定義して、複数の要素で使用することができる.
		{
			m_menu_stroke = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_stroke_menu")));
			m_menu_fill = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_fill_menu")));
			m_menu_font = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_front_menu")));
			m_menu_sheet = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_sheet_menu")));
			m_menu_ungroup = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_ungroup_menu")));
			m_menu_ruler = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_ruler_menu")));
			m_menu_image = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_image_menu")));
		}

		auto _{ new_click_async(nullptr, nullptr) };
	}

	// メッセージダイアログを表示する.
	// glyph_key	フォントアイコンのグリフの静的リソースのキー
	// message_key	メッセージのアプリケーションリソースのキー
	// desc_key		説明文のアプリケーションリソースのキー
	// 戻り値	なし
	void MainPage::message_show(winrt::hstring const& glyph_key, winrt::hstring const& message_key, winrt::hstring const& desc_key)
	{
		constexpr wchar_t QUOT[] = L"\"";	// 引用符
		constexpr wchar_t NEW_LINE[] = L"\u2028";	// テキストブロック内での改行

		ResourceLoader const& r_loader = ResourceLoader::GetForCurrentView();
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
			// 追加する文字列が空以外の場合,
			text = text + NEW_LINE + added_text;
		}
		else if (desc_key.empty() != true) {
			// 説明そのものが空以外の場合,
			text = text + NEW_LINE + QUOT + desc_key + QUOT;
		}
		const IInspectable glyph_val = Resources().TryLookup(box_value(glyph_key));
		fi_message().Glyph(glyph_val != nullptr ? unbox_value<winrt::hstring>(glyph_val) : glyph_key);
		tk_message().Text(text);
		auto _{ cd_message_dialog().ShowAsync() };
	}

	// ファイルメニューの「新規」が選択された
	IAsyncAction MainPage::new_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		// スタックが更新された, かつ確認ダイアログの応答が「キャンセル」か判定する.
		if (m_ustack_updt && !co_await ask_for_conf_async()) {
			co_return;
		}
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_close_click(nullptr, nullptr);
		}
		ustack_clear();
		slist_clear(m_main_sheet.m_shape_list);
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			// 「メモリリーク」メッセージダイアログを表示する.
			message_show(ICON_ALERT, DEBUG_MSG, {});
		}
#endif
		ShapeText::release_available_fonts();

		ShapeText::set_available_fonts(m_main_d2d);

		// 背景色, 前景色, 選択された文字範囲の背景色, 文字色をリソースから得る.
		{
			const IInspectable sel_back_color = Resources().TryLookup(box_value(L"SystemAccentColor"));
			const IInspectable sel_text_color = Resources().TryLookup(box_value(L"SystemColorHighlightTextColor"));
			if (sel_back_color != nullptr && sel_text_color != nullptr) {
				conv_uwp_to_color(unbox_value<Color>(sel_back_color), ShapeText::s_text_selected_background);
				conv_uwp_to_color(unbox_value<Color>(sel_text_color), ShapeText::s_text_selected_foreground);
			}
			else {
				ShapeText::s_text_selected_background = { 0.0f, 0x00 / COLOR_MAX, 0x78 / COLOR_MAX, 0xD4 / COLOR_MAX };
				ShapeText::s_text_selected_foreground = COLOR_WHITE;
			}
			/*
			auto const& back_theme = Resources().TryLookup(box_value(L"ApplicationPageBackgroundThemeBrush"));
			auto const& fore_theme = Resources().TryLookup(box_value(L"ApplicationForegroundThemeBrush"));
			if (back_theme != nullptr && fore_theme != nullptr) {
				conv_uwp_to_color(unbox_value<Brush>(back_theme), Shape::s_background_color);
				conv_uwp_to_color(unbox_value<Brush>(fore_theme), Shape::s_foreground_color);
			}
			else {*/
				Shape::s_background_color = COLOR_WHITE;
				Shape::s_foreground_color = COLOR_BLACK;
			//}
			/*
			Shape::s_background_color = m_main_d2d.s_background_color;
			Shape::s_foreground_color = m_main_d2d.s_foreground_color;
			*/
		}

		if (co_await sheet_pref_load_async() != S_OK) {
			// 読み込みに失敗した場合,
			sheet_init();
			m_len_unit = LEN_UNIT::PIXEL;
			m_color_code = COLOR_CODE::DEC;
			m_vert_stick = VERT_STICK_DEF_VAL;
			m_status_bar = STATUS_BAR_DEF_VAL;
		}

		// 用紙の左上位置と右下位置を初期化する.
		{
			m_main_min = D2D1_POINT_2F{ 0.0F, 0.0F };
			m_main_max = D2D1_POINT_2F{ m_main_sheet.m_sheet_size.width, m_main_sheet.m_sheet_size.height };
		}
		file_recent_add(nullptr);
		file_finish_reading();
	}

}
