//-------------------------------
// MainPage_stbar.cpp
// ステータスバー
//-------------------------------
#include "pch.h"
#include "Summary.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 列挙型を AND 演算する.
	static STATUS_BAR stbar_and(const STATUS_BAR a, const STATUS_BAR b) noexcept;
	// 列挙型をビットマスクする.
	static bool stbar_mask(const STATUS_BAR a, const STATUS_BAR b) noexcept;
	// 列挙型を NOT 演算する.
	static STATUS_BAR stbar_not(const STATUS_BAR a) noexcept;
	// ステータスバーの項目のビジビリティを設定する.
	static void stbar_visiblity(const bool check, FrameworkElement const& f_elem) noexcept;

	// 列挙型を AND 演算する.
	static STATUS_BAR stbar_and(const STATUS_BAR a, const STATUS_BAR b) noexcept
	{
		return static_cast<STATUS_BAR>(static_cast<uint32_t>(a)& static_cast<uint32_t>(b));
	}

	// 列挙型をビットマスクする.
	static bool stbar_mask(const STATUS_BAR a, const STATUS_BAR b) noexcept
	{
		return static_cast<uint32_t>(a)& static_cast<uint32_t>(b);
	}

	// 列挙型を NOT 演算する.
	static STATUS_BAR stbar_not(const STATUS_BAR a) noexcept
	{
		return static_cast<STATUS_BAR>(~static_cast<uint32_t>(a));
	}

	// ステータスバーの項目のビジビリティを設定する.
	static void stbar_visiblity(const bool check, FrameworkElement const& f_elem) noexcept
	{
		if (f_elem.Visibility() == (check ? COLLAPSED : VISIBLE)) {
			f_elem.Visibility(check ? VISIBLE : COLLAPSED);
		}
	}

	// 用紙メニューの「ステータスバー」が選択された.
	void MainPage::stbar_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		STATUS_BAR stbar;	// ステータスバーの状態
		bool check;	// チェックマークの有無

		if (sender == tmfi_stbar_curs()) {
			stbar = STATUS_BAR::CURS;
			check = tmfi_stbar_curs().IsChecked();
			tmfi_stbar_curs_2().IsChecked(check);
			// ポインターの位置をステータスバーに格納する.
			stbar_set_curs();
			stbar_visiblity(check, tk_stbar_pos_x());
			stbar_visiblity(check, tk_stbar_pos_y());
		}
		else if (sender == tmfi_stbar_curs_2()) {
			stbar = STATUS_BAR::CURS;
			check = tmfi_stbar_curs_2().IsChecked();
			tmfi_stbar_curs().IsChecked(check);
			// ポインターの位置をステータスバーに格納する.
			stbar_set_curs();
			stbar_visiblity(check, tk_stbar_pos_x());
			stbar_visiblity(check, tk_stbar_pos_y());
		}
		else if (sender == tmfi_stbar_grid()) {
			stbar = STATUS_BAR::GRID;
			check = tmfi_stbar_grid().IsChecked();
			tmfi_stbar_grid_2().IsChecked(check);
			// 方眼の大きさをステータスバーに格納する.
			stbar_set_grid();
			stbar_visiblity(check, tk_stbar_grid());
		}
		else if (sender == tmfi_stbar_grid_2()) {
			stbar = STATUS_BAR::GRID;
			check = tmfi_stbar_grid_2().IsChecked();
			tmfi_stbar_grid().IsChecked(check);
			// 方眼の大きさをステータスバーに格納する.
			stbar_set_grid();
			stbar_visiblity(check, tk_stbar_grid());
		}
		else if (sender == tmfi_stbar_sheet()) {
			stbar = STATUS_BAR::SHEET;
			check = tmfi_stbar_sheet().IsChecked();
			tmfi_stbar_sheet_2().IsChecked(check);
			// 用紙の大きさをステータスバーに格納する.
			stbar_set_sheet();
			stbar_visiblity(check, tk_stbar_width());
			stbar_visiblity(check, tk_stbar_height());
		}
		else if (sender == tmfi_stbar_sheet_2()) {
			stbar = STATUS_BAR::SHEET;
			check = tmfi_stbar_sheet_2().IsChecked();
			tmfi_stbar_sheet().IsChecked(check);
			// 用紙の大きさをステータスバーに格納する.
			stbar_set_sheet();
			stbar_visiblity(check, tk_stbar_width());
			stbar_visiblity(check, tk_stbar_height());
		}
		else if (sender == tmfi_stbar_zoom()) {
			stbar = STATUS_BAR::ZOOM;
			check = tmfi_stbar_zoom().IsChecked();
			tmfi_stbar_zoom_2().IsChecked(check);
			stbar_set_zoom();
			stbar_visiblity(check, tk_stbar_zoom());
		}
		else if (sender == tmfi_stbar_zoom_2()) {
			stbar = STATUS_BAR::ZOOM;
			check = tmfi_stbar_zoom_2().IsChecked();
			tmfi_stbar_zoom().IsChecked(check);
			stbar_set_zoom();
			stbar_visiblity(check, tk_stbar_zoom());
		}
		else if (sender == tmfi_stbar_draw()) {
			stbar = STATUS_BAR::DRAW;
			check = tmfi_stbar_draw().IsChecked();
			tmfi_stbar_draw_2().IsChecked(check);
			// 作図ツールをステータスバーに格納する.
			stbar_set_draw();
			stbar_visiblity(check, sp_stbar_draw());
		}
		else if (sender == tmfi_stbar_draw_2()) {
			stbar = STATUS_BAR::DRAW;
			check = tmfi_stbar_draw_2().IsChecked();
			tmfi_stbar_draw().IsChecked(check);
			// 作図ツールをステータスバーに格納する.
			stbar_set_draw();
			stbar_visiblity(check, sp_stbar_draw());
		}
		else if (sender == tmfi_stbar_unit()) {
			stbar = STATUS_BAR::UNIT;
			check = tmfi_stbar_unit().IsChecked();
			tmfi_stbar_unit_2().IsChecked(check);
			stbar_set_unit();
			stbar_visiblity(check, tk_stbar_unit());
		}
		else if (sender == tmfi_stbar_unit_2()) {
			stbar = STATUS_BAR::UNIT;
			check = tmfi_stbar_unit_2().IsChecked();
			tmfi_stbar_unit().IsChecked(check);
			stbar_set_unit();
			stbar_visiblity(check, tk_stbar_unit());
		}
		else {
			return;
		}
		if (check) {
			m_status_bar = stbar_or(m_status_bar, stbar);
		}
		else {
			m_status_bar = stbar_and(m_status_bar, stbar_not(stbar));
		}
		stbar_visiblity(m_status_bar != static_cast<STATUS_BAR>(0), sp_stbar());
	}

	// ステータスバーのメニュー項目に印をつける.
	void MainPage::stbar_check_menu(const STATUS_BAR st_bar)
	{
		tmfi_stbar_curs().IsChecked(stbar_mask(st_bar, STATUS_BAR::CURS));
		tmfi_stbar_grid().IsChecked(stbar_mask(st_bar, STATUS_BAR::GRID));
		tmfi_stbar_sheet().IsChecked(stbar_mask(st_bar, STATUS_BAR::SHEET));
		tmfi_stbar_draw().IsChecked(stbar_mask(st_bar, STATUS_BAR::DRAW));
		tmfi_stbar_unit().IsChecked(stbar_mask(st_bar, STATUS_BAR::UNIT));
		tmfi_stbar_zoom().IsChecked(stbar_mask(st_bar, STATUS_BAR::ZOOM));
		tmfi_stbar_curs_2().IsChecked(stbar_mask(st_bar, STATUS_BAR::CURS));
		tmfi_stbar_grid_2().IsChecked(stbar_mask(st_bar, STATUS_BAR::GRID));
		tmfi_stbar_sheet_2().IsChecked(stbar_mask(st_bar, STATUS_BAR::SHEET));
		tmfi_stbar_draw_2().IsChecked(stbar_mask(st_bar, STATUS_BAR::DRAW));
		tmfi_stbar_unit_2().IsChecked(stbar_mask(st_bar, STATUS_BAR::UNIT));
		tmfi_stbar_zoom_2().IsChecked(stbar_mask(st_bar, STATUS_BAR::ZOOM));
	}

	// 列挙型を OR 演算する.
	STATUS_BAR MainPage::stbar_or(const STATUS_BAR a, const STATUS_BAR b) noexcept
	{
		return static_cast<STATUS_BAR>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	// ステータスバーの状態をデータリーダーから読み込む.
	void MainPage::stbar_read(DataReader const& dt_reader)
	{
		m_status_bar = static_cast<STATUS_BAR>(dt_reader.ReadUInt32());
	}

	// ポインターの位置をステータスバーに格納する.
	void MainPage::stbar_set_curs(void)
	{
		const double dpi = sheet_dpi();
		const auto wp = CoreWindow::GetForCurrentThread().PointerPosition();
		const auto wb = CoreWindow::GetForCurrentThread().Bounds();
		const auto tr = scp_sheet_panel().TransformToVisual(nullptr);
		const auto tp = tr.TransformPoint({ 0.0f, 0.0f });
		const double sx = sb_horz().Value();
		const double sy = sb_vert().Value();
		const double wx = wp.X;
		const double tx = tp.X;
		const double bx = wb.X;
		const double wy = wp.Y;
		const double ty = tp.Y;
		const double by = wb.Y;
		const double px = sheet_min().x;
		const double py = sheet_min().y;
		const double ps = m_main_sheet.m_sheet_scale;
		const double fx = (wx - bx - tx) / ps + sx + px;
		const double fy = (wy - by - ty) / ps + sy + py;
		const double g_len = m_main_sheet.m_grid_base + 1.0;

		wchar_t buf[32];
		// ピクセル単位の長さを他の単位の文字列に変換する.
		conv_val_to_len<!WITH_UNIT_NAME>(len_unit(), fx, dpi, g_len, buf);
		tk_stbar_pos_x().Text(winrt::hstring{ L"x:" } + buf);
		// ピクセル単位の長さを他の単位の文字列に変換する.
		conv_val_to_len<!WITH_UNIT_NAME>(len_unit(), fy, dpi, g_len, buf);
		tk_stbar_pos_y().Text(winrt::hstring{ L"y:" } + buf);

swprintf_s(buf, L"%d", static_cast<uint32_t>(m_list_shapes.size()));
tk_stbar_cnt().Text(winrt::hstring{ L"c:" } + buf);
	}

	// 方眼の大きさをステータスバーに格納する.
	void MainPage::stbar_set_grid(void)
	{
		wchar_t buf[32];
		const double dpi = sheet_dpi();
		double g_len = m_main_sheet.m_grid_base + 1.0;
		// ピクセル単位の長さを他の単位の文字列に変換する.
		conv_val_to_len<!WITH_UNIT_NAME>(len_unit(), g_len, dpi, g_len, buf);
		tk_stbar_grid().Text(winrt::hstring{ L"g:" } +buf);
	}

	// 用紙の大きさをステータスバーに格納する.
	void MainPage::stbar_set_sheet(void)
	{
		const double dpi = sheet_dpi();
		const double g_len = m_main_sheet.m_grid_base + 1.0;
		wchar_t buf[32];
		// ピクセル単位の長さを他の単位の文字列に変換する.
		conv_val_to_len<!WITH_UNIT_NAME>(len_unit(), m_main_sheet.m_sheet_size.width, dpi, g_len, buf);
		tk_stbar_width().Text(winrt::hstring{ L"w:" } + buf);
		// ピクセル単位の長さを他の単位の文字列に変換する.
		conv_val_to_len<!WITH_UNIT_NAME>(len_unit(), m_main_sheet.m_sheet_size.height, dpi, g_len, buf);
		tk_stbar_height().Text(winrt::hstring{ L"h:" } + buf);
	}

	// 作図ツールをステータスバーに格納する.
	void MainPage::stbar_set_draw(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		const auto tool = tool_draw();
		winrt::hstring data;
		if (tool == TOOL_DRAW::BEZI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_bezi")));
		}
		else if (tool == TOOL_DRAW::ELLI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_elli")));
		}
		else if (tool == TOOL_DRAW::LINE) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_line")));
		}
		else if (tool == TOOL_DRAW::QUAD) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_quad")));
		}
		else if (tool == TOOL_DRAW::RECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rect")));
		}
		else if (tool == TOOL_DRAW::RRCT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rrct")));
		}
		else if (tool == TOOL_DRAW::RULER) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_text")));
		}
		else if (tool == TOOL_DRAW::SELECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_select")));
		}
		else if (tool == TOOL_DRAW::TEXT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_text")));
		}
		else {
			throw winrt::hresult_not_implemented();
		}
		pi_draw().Data(nullptr);
		pi_draw().Data(Summary::Data(data));
	}

	// 単位をステータスバーに格納する.
	void MainPage::stbar_set_unit(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		const auto unit = len_unit();
		winrt::hstring unit_name{};
		if (unit == LEN_UNIT::GRID) {
			unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_unit_grid/Text");
		}
		else if (unit == LEN_UNIT::INCH) {
			unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_unit_inch/Text");
		}
		else if (unit == LEN_UNIT::MILLI) {
			unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_unit_milli/Text");
		}
		else if (unit == LEN_UNIT::PIXEL) {
			unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_unit_pixel/Text");
		}
		else if (unit == LEN_UNIT::POINT) {
			unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_unit_point/Text");
		}
		else {
			return;
		}
		tk_stbar_unit().Text(winrt::hstring{ L"u:" } + unit_name);
	}

	// 拡大率をステータスバーに格納する.
	void MainPage::stbar_set_zoom(void)
	{
		wchar_t buf[32];
		swprintf_s(buf, 31, FMT_ZOOM, m_main_sheet.m_sheet_scale * 100.0);
		tk_stbar_zoom().Text(winrt::hstring{ L"z:" } +buf);
	}

	// ステータスバーの表示を設定する.
	void MainPage::stbar_visibility(void)
	{
		tk_stbar_pos_x().Visibility(stbar_mask(m_status_bar, STATUS_BAR::CURS) ? VISIBLE : COLLAPSED);
		tk_stbar_pos_y().Visibility(stbar_mask(m_status_bar, STATUS_BAR::CURS) ? VISIBLE : COLLAPSED);
		tk_stbar_grid().Visibility(stbar_mask(m_status_bar, STATUS_BAR::GRID) ? VISIBLE : COLLAPSED);
		tk_stbar_width().Visibility(stbar_mask(m_status_bar, STATUS_BAR::SHEET) ? VISIBLE : COLLAPSED);
		tk_stbar_height().Visibility(stbar_mask(m_status_bar, STATUS_BAR::SHEET) ? VISIBLE : COLLAPSED);
		//sp_stbar_curs().Visibility(stbar_mask(m_status_bar, STATUS_BAR::CURS) ? VISIBLE : COLLAPSED);
		//sp_stbar_grid().Visibility(stbar_mask(m_status_bar, STATUS_BAR::GRID) ? VISIBLE : COLLAPSED);
		//sp_stbar_sheet().Visibility(stbar_mask(m_status_bar, STATUS_BAR::SHEET) ? VISIBLE : COLLAPSED);
		sp_stbar_draw().Visibility(stbar_mask(m_status_bar, STATUS_BAR::DRAW) ? VISIBLE : COLLAPSED);
		tk_stbar_unit().Visibility(stbar_mask(m_status_bar, STATUS_BAR::UNIT) ? VISIBLE : COLLAPSED);
		tk_stbar_zoom().Visibility(stbar_mask(m_status_bar, STATUS_BAR::ZOOM) ? VISIBLE : COLLAPSED);
		//sp_stbar_unit().Visibility(stbar_mask(m_status_bar, STATUS_BAR::UNIT) ? VISIBLE : COLLAPSED);
		//sp_stbar_zoom().Visibility(stbar_mask(m_status_bar, STATUS_BAR::ZOOM) ? VISIBLE : COLLAPSED);
		sp_stbar().Visibility(m_status_bar != static_cast<STATUS_BAR>(0) ? VISIBLE : COLLAPSED);
	}

	// ステータスバーの状態をデータライターに書き込む.
	void MainPage::stbar_write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_status_bar));
	}

}