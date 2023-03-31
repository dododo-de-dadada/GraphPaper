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

	//-------------------------------
	// 待機カーソルを表示する.
	// 戻り値	それまで表示されていたカーソル.
	//-------------------------------
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
	void conv_col_to_str(const COLOR_CODE c_code, const double val, const size_t t_len, wchar_t t_buf[]) noexcept;

	//-------------------------------
	// 色成分を文字列に変換する.
	// c_code	色の基数
	// c_val	色成分の値
	// t_len	文字列の最大長 ('\0' を含む長さ)
	// t_buf	文字列の配列 [t_len]
	//-------------------------------
	void conv_col_to_str(const COLOR_CODE c_code, const double c_val, const size_t t_len, wchar_t t_buf[]) noexcept
	{
		// 色の基数が 10 進数か判定する.
		if (c_code == COLOR_CODE::DEC) {
			swprintf_s(t_buf, t_len, L"%.0lf", std::round(c_val));
		}
		// 色の基数が 16 進数か判定する.
		else if (c_code == COLOR_CODE::HEX) {
			swprintf_s(t_buf, t_len, L"x%02X", static_cast<uint32_t>(std::round(c_val)));
		}
		// 色の基数が実数か判定する.
		else if (c_code == COLOR_CODE::REAL) {
			swprintf_s(t_buf, t_len, L"%.4lf", c_val / COLOR_MAX);
		}
		// 色の基数がパーセントか判定する.
		else if (c_code == COLOR_CODE::PCT) {
			swprintf_s(t_buf, t_len, L"%.1lf%%", c_val * 100.0 / COLOR_MAX);
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
	template <bool B> void conv_len_to_str(
		const LEN_UNIT len_unit, const double val, const double dpi, const double g_len,
		const uint32_t t_len, wchar_t *t_buf) noexcept
	{
		// 長さの単位がピクセルか判定する.
		if (len_unit == LEN_UNIT::PIXEL) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_PIXEL_UNIT, val);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_PIXEL, val);
			}
		}
		// 長さの単位がインチか判定する.
		else if (len_unit == LEN_UNIT::INCH) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_INCH_UNIT, val / dpi);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_INCH, val / dpi);
			}
		}
		// 長さの単位がミリメートルか判定する.
		else if (len_unit == LEN_UNIT::MILLI) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_MILLI_UNIT, val * MM_PER_INCH / dpi);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_MILLI, val * MM_PER_INCH / dpi);
			}
		}
		// 長さの単位がポイントか判定する.
		else if (len_unit == LEN_UNIT::POINT) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_POINT_UNIT, val * PT_PER_INCH / dpi);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_POINT, val * PT_PER_INCH / dpi);
			}
		}
		// 長さの単位が方眼か判定する.
		else if (len_unit == LEN_UNIT::GRID) {
			if constexpr (B) {
				swprintf_s(t_buf, t_len, FMT_GRID_UNIT, val / g_len);
			}
			else {
				swprintf_s(t_buf, t_len, FMT_GRID, val / g_len);
			}
		}
		else {
			swprintf_s(t_buf, t_len, L"?");
		}
	}

	// 長さを文字列に変換する (単位なし).
	template void conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(
		const LEN_UNIT len_unit, const double len_val, const double dpi, const double g_len,
		const uint32_t t_len, wchar_t* t_buf) noexcept;

	// 長さを文字列に変換する (単位つき).
	template void conv_len_to_str<LEN_UNIT_NAME_APPEND>(
		const LEN_UNIT len_unit, const double len_val, const double dpi, const double g_len,
		const uint32_t t_len, wchar_t* t_buf) noexcept;

	// 長さををピクセル単位の値に変換する.
	// 変換された値は, 0.5 ピクセル単位に丸められる.
	// l_unit	長さの単位
	// l_val	長さの値
	// dpi	DPI
	// g_len	方眼の大きさ
	// 戻り値	ピクセル単位の値
	double conv_len_to_pixel(const LEN_UNIT l_unit, const double l_val, const double dpi, const double g_len) noexcept
	{
		double ret;

		if (l_unit == LEN_UNIT::INCH) {
			ret = l_val * dpi;
		}
		else if (l_unit == LEN_UNIT::MILLI) {
			ret = l_val * dpi / MM_PER_INCH;
		}
		else if (l_unit == LEN_UNIT::POINT) {
			ret = l_val * dpi / PT_PER_INCH;
		}
		else if (l_unit == LEN_UNIT::GRID) {
			ret = l_val * g_len;
		}
		else {
			ret = l_val;
		}
		return std::round(2.0 * ret) * 0.5;
	}

	//-------------------------------
	// メインページを作成する.
	//-------------------------------
	MainPage::MainPage(void)
	{
		// お約束.
		InitializeComponent();

		// 「印刷」メニューの可否を設定する.
		{
			//mfi_print().IsEnabled(PrintManager::IsSupported());
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
			m_token_orientation_changed = disp.OrientationChanged(
				{ this, &MainPage::display_orientation_changed });
			m_token_contents_invalidated = disp.DisplayContentsInvalidated(
				{ this, &MainPage::display_contents_invalidated });
		}

		// アプリケーションを閉じる前の確認のハンドラーを設定する.
		{
			m_token_close_requested = 
				SystemNavigationManagerPreview::GetForCurrentView().CloseRequested(
					{ this, &MainPage::navi_close_requested });
		}

		// D2D/DWRITE ファクトリを図形クラスに, 
		// 図形リストとページをアンドゥ操作に格納する.
		{
			Undo::set(&m_main_page.m_shape_list, &m_main_page);
		}

		// 背景パターン画像の読み込み.
		{
			background_get_brush();
		}

		auto _{ file_new_click_async(nullptr, nullptr) };
	}

	//-------------------------------
	// メッセージダイアログを表示する.
	// glyph	フォントアイコンのグリフの静的リソースのキー
	// message	メッセージのアプリケーションリソースのキー
	// desc		説明文のアプリケーションリソースのキー
	// 戻り値	なし
	//-------------------------------
	void MainPage::message_show(winrt::hstring const& glyph, winrt::hstring const& message, winrt::hstring const& desc)
	{
		constexpr wchar_t QUOT[] = L"\"";	// 引用符
		constexpr wchar_t NEW_LINE[] = L"\u2028";	// テキストブロック内での改行

		ResourceLoader const& r_loader = ResourceLoader::GetForCurrentView();
		winrt::hstring text;
		try {
			text = r_loader.GetString(message);
		}
		catch (winrt::hresult_error const&) {
		}
		if (text.empty()) {
			// 文字列が空の場合,
			text = message;
		}
		winrt::hstring added_text;
		try {
			added_text = r_loader.GetString(desc);
		}
		catch (winrt::hresult_error const&) {}
		if (!added_text.empty()) {
			// 追加する文字列が空以外の場合,
			text = text + NEW_LINE + added_text;
		}
		else if (!desc.empty()) {
			// 説明そのものが空以外の場合,
			text = text + NEW_LINE + QUOT + desc + QUOT;
		}
		const IInspectable glyph_val = Resources().TryLookup(box_value(glyph));
		const winrt::hstring font_icon{
			glyph_val != nullptr ? unbox_value<winrt::hstring>(glyph_val) : glyph
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
		m_main_d2d.SetCompositionScale(
			scp_main_panel().CompositionScaleX(), scp_main_panel().CompositionScaleY());
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

	// 図形をもとにメインのページ図形の境界矩形を更新する.
	// s	追加された図形
	void MainPage::main_bbox_update(const Shape* s) noexcept
	{
		s->get_bound(m_main_bbox_lt, m_main_bbox_rb, m_main_bbox_lt, m_main_bbox_rb);
	}

	// メインのページ図形の境界矩形を更新する.
	void MainPage::main_bbox_update(void) noexcept
	{
		// リスト中の図形を囲む矩形を得る.
		slist_bound_shape(m_main_page.m_shape_list, m_main_bbox_lt, m_main_bbox_rb);

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

	// メインのページ図形を表示する.
	void MainPage::main_draw(void)
	{
		if (!scp_main_panel().IsLoaded()) {
			return;
		}
		if (!m_mutex_draw.try_lock()) {
			// ロックできない場合
			return;
		}

		// ひな型に描画に必要な変数を格納する.
		//const auto p_scale = max(m_main_page.m_page_scale, 0.0f);
		const auto p_scale = max(m_main_scale, 0.0f);
		m_main_page.begin_draw(m_main_d2d.m_d2d_context.get(), true, m_wic_background.get(), p_scale);

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
		t.m11 = t.m22 = p_scale;
		t.dx = static_cast<FLOAT>(-(m_main_bbox_lt.x + sb_horz().Value()) * p_scale);
		t.dy = static_cast<FLOAT>(-(m_main_bbox_lt.y + sb_vert().Value()) * p_scale);
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
