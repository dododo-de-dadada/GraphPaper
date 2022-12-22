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
	using winrt::Windows::UI::ViewManagement::UISettings;
	using winrt::Windows::UI::Xaml::Application;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogButton;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Media::Brush;

	// 書式文字列
	constexpr auto FMT_INCH = L"%.3f";	// インチ単位の書式
	constexpr auto FMT_INCH_UNIT = L"%.3f \u33CC";	// インチ単位の書式
	constexpr auto FMT_MILLI = L"%.3f";	// ミリメートル単位の書式
	constexpr auto FMT_MILLI_UNIT = L"%.3f \u339C";	// ミリメートル単位の書式
	constexpr auto FMT_POINT = L"%.2f";	// ポイント単位の書式
	constexpr auto FMT_POINT_UNIT = L"%.2f pt";	// ポイント単位の書式
	constexpr auto FMT_PIXEL = L"%.1f";	// ピクセル単位の書式
	constexpr auto FMT_PIXEL_UNIT = L"%.1f px";	// ピクセル単位の書式
	constexpr auto FMT_ZOOM = L"%.f%%";	// 倍率の書式
	constexpr auto FMT_GRID = L"%.3f";	// グリッド単位の書式
	constexpr auto FMT_GRID_UNIT = L"%.3f gd";	// グリッド単位の書式

	// 色成分を文字列に変換する.
	void conv_col_to_str(const COLOR_CODE c_code, const double val, const size_t t_len, wchar_t t_buf[]) noexcept;

	//-------------------------------
	// 色成分を文字列に変換する.
	// c_code	色の表記
	// c_val	色成分の値
	// t_len	文字列の最大長 ('\0' を含む長さ)
	// t_buf	文字列の配列 [t_len]
	//-------------------------------
	void conv_col_to_str(const COLOR_CODE c_code, const double c_val, const size_t t_len, wchar_t t_buf[]) noexcept
	{
		// 色の表記が 10 進数か判定する.
		if (c_code == COLOR_CODE::DEC) {
			swprintf_s(t_buf, t_len, L"%.0lf", std::round(c_val));
		}
		// 色の表記が 16 進数か判定する.
		else if (c_code == COLOR_CODE::HEX) {
			swprintf_s(t_buf, t_len, L"%02X", static_cast<uint32_t>(std::round(c_val)));
		}
		// 色の表記が実数か判定する.
		else if (c_code == COLOR_CODE::REAL) {
			swprintf_s(t_buf, t_len, L"%.4lf", c_val / COLOR_MAX);
		}
		// 色の表記がパーセントか判定する.
		else if (c_code == COLOR_CODE::CENT) {
			swprintf_s(t_buf, t_len, L"%.1lf%%", c_val / COLOR_MAX * 100.0);
		}
		else {
			swprintf_s(t_buf, t_len, L"?");
		}
	}

	//-------------------------------
	// 長さを文字列に変換する.
	// B	単位付加フラグ
	// len_unit	長さの単位
	// len_val	ピクセル単位の長さ
	// dpi	DPI
	// g_len	方眼の大きさ
	// t_len	文字列の最大長 ('\0' を含む長さ)
	// t_buf	文字列の配列
	//-------------------------------
	template <bool B> void conv_len_to_str(const LEN_UNIT len_unit, const float len_val, const float dpi, const float g_len, const uint32_t t_len, wchar_t *t_buf) noexcept
	{
		// 長さの単位がピクセルか判定する.
		if (len_unit == LEN_UNIT::PIXEL) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_PIXEL_UNIT, len_val);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_PIXEL, len_val);
			}
		}
		// 長さの単位がインチか判定する.
		else if (len_unit == LEN_UNIT::INCH) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_INCH_UNIT, len_val / dpi);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_INCH, len_val / dpi);
			}
		}
		// 長さの単位がミリメートルか判定する.
		else if (len_unit == LEN_UNIT::MILLI) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_MILLI_UNIT, len_val * MM_PER_INCH / dpi);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_MILLI, len_val * MM_PER_INCH / dpi);
			}
		}
		// 長さの単位がポイントか判定する.
		else if (len_unit == LEN_UNIT::POINT) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_POINT_UNIT, len_val * PT_PER_INCH / dpi);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_POINT, len_val * PT_PER_INCH / dpi);
			}
		}
		// 長さの単位が方眼か判定する.
		else if (len_unit == LEN_UNIT::GRID) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_GRID_UNIT, len_val / g_len);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_GRID, len_val / g_len);
			}
		}
		else {
			swprintf_s(t_buf, t_len, L"?");
		}
	}

	// 長さを文字列に変換する (単位なし).
	template void conv_len_to_str<LEN_UNIT_HIDE>(const LEN_UNIT len_unit, const float len_val, const float dpi, const float g_len, const uint32_t t_len, wchar_t* t_buf) noexcept;

	// 長さを文字列に変換する (単位つき).
	template void conv_len_to_str<LEN_UNIT_SHOW>(const LEN_UNIT len_unit, const float len_val, const float dpi, const float g_len, const uint32_t t_len, wchar_t* t_buf) noexcept;

	//-------------------------------
	// メインページを作成する.
	//-------------------------------
	MainPage::MainPage(void)
	{
		Shape* s = new ShapeText(D2D1_POINT_2F{ 0, 0 }, D2D1_POINT_2F{ 10, 10 }, wchar_cpy(L""), & m_main_sheet);
		delete s;

		// お約束.
		InitializeComponent();

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
			m_token_close_requested = SystemNavigationManagerPreview::GetForCurrentView().CloseRequested({ this, &MainPage::navi_close_requested });
		}

		// D2D/DWRITE ファクトリを図形クラスに, 
		// 図形リストと用紙をアンドゥ操作に格納する.
		{
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
			//using winrt::Windows::UI::Xaml::Controls::MenuFlyoutSubItem;
			//m_menu_fill = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_fill_menu")));
			//m_menu_font = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_front_menu")));
			//m_menu_sheet = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_sheet_menu")));
			//m_menu_ungroup = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_ungroup_menu")));
			//m_menu_ruler = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_ruler_menu")));
			//m_menu_image = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_image_menu")));
		}

		auto _{ file_new_click_async(nullptr, nullptr) };
	}

	//-------------------------------
	// メッセージダイアログを表示する.
	// glyph_key	フォントアイコンのグリフの静的リソースのキー
	// message_key	メッセージのアプリケーションリソースのキー
	// desc_key		説明文のアプリケーションリソースのキー
	// 戻り値	なし
	//-------------------------------
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
		if (!added_text.empty()) {
			// 追加する文字列が空以外の場合,
			text = text + NEW_LINE + added_text;
		}
		else if (!desc_key.empty()) {
			// 説明そのものが空以外の場合,
			text = text + NEW_LINE + QUOT + desc_key + QUOT;
		}
		const IInspectable glyph_val = Resources().TryLookup(box_value(glyph_key));
		const winrt::hstring font_icon{
			glyph_val != nullptr ? unbox_value<winrt::hstring>(glyph_val) : glyph_key
		};
		fi_message().Glyph(font_icon);
		tk_message().Text(text);
		// マイクロソフトによると,
		// > アプリでの最も低速なステージとして、起動や、ビューの切り替えなどがあります。
		// > ユーザーに最初に表示される UI を起動するために必要なもの以上の作業を実行しないでください。
		// > たとえば、段階的に公開される UI の UI や、ポップアップのコンテンツなどは作成しないでください。
		// メッセージダイアログを起動時からでも表示できるよう, RunIdleAsync を使用する.
		Dispatcher().RunIdleAsync([=](
			winrt::Windows::UI::Core::IdleDispatchedHandlerArgs) {
			auto _{ cd_message_dialog().ShowAsync() };
		});
	}

}
