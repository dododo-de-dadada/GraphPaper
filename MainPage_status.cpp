//-------------------------------
// MainPage_status.cpp
// ステータスバーの設定
//-------------------------------
#include "pch.h"
#include "Summary.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 列挙型を AND 演算する.
	static STAT_BAR status_and(const STAT_BAR a, const STAT_BAR b) noexcept;
	// 列挙型をビットマスクする.
	static bool status_mask(const STAT_BAR a, const STAT_BAR b) noexcept;
	// 列挙型を NOT 演算する.
	static STAT_BAR status_not(const STAT_BAR a) noexcept;
	// ステータスバーの項目のビジビリティを設定する.
	static void status_visiblity(const bool check, FrameworkElement const& f_elem) noexcept;

	// 列挙型を AND 演算する.
	static STAT_BAR status_and(const STAT_BAR a, const STAT_BAR b) noexcept
	{
		return static_cast<STAT_BAR>(static_cast<uint32_t>(a)& static_cast<uint32_t>(b));
	}

	// 列挙型をビットマスクする.
	static bool status_mask(const STAT_BAR a, const STAT_BAR b) noexcept
	{
		return static_cast<uint32_t>(a)& static_cast<uint32_t>(b);
	}

	// 列挙型を NOT 演算する.
	static STAT_BAR status_not(const STAT_BAR a) noexcept
	{
		return static_cast<STAT_BAR>(~static_cast<uint32_t>(a));
	}

	// ステータスバーの項目のビジビリティを設定する.
	static void status_visiblity(const bool check, FrameworkElement const& f_elem) noexcept
	{
		if (f_elem.Visibility() == (check ? COLLAPSED : VISIBLE)) {
			f_elem.Visibility(check ? VISIBLE : COLLAPSED);
		}
	}

	// ページメニューの「ステータスバー」が選択された.
	void MainPage::mi_status_bar_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		STAT_BAR s_bar;	// ステータスバーの状態
		bool check;	// チェックマークの有無

		if (sender == tmfi_status_grid()) {
			s_bar = STAT_BAR::GRID;
			check = tmfi_status_grid().IsChecked();
			tmfi_status_grid_2().IsChecked(check);
			status_set_grid();
			status_visiblity(check, tk_status_grid());
		}
		else if (sender == tmfi_status_grid_2()) {
			s_bar = STAT_BAR::GRID;
			check = tmfi_status_grid_2().IsChecked();
			tmfi_status_grid().IsChecked(check);
			status_set_grid();
			status_visiblity(check, tk_status_grid());
		}
		else if (sender == tmfi_status_page()) {
			s_bar = STAT_BAR::PAGE;
			check = tmfi_status_page().IsChecked();
			tmfi_status_page_2().IsChecked(check);
			status_set_page();
			status_visiblity(check, tk_status_width());
			status_visiblity(check, tk_status_height());
		}
		else if (sender == tmfi_status_page_2()) {
			s_bar = STAT_BAR::PAGE;
			check = tmfi_status_page_2().IsChecked();
			tmfi_status_page().IsChecked(check);
			status_set_page();
			status_visiblity(check, tk_status_width());
			status_visiblity(check, tk_status_height());
		}
		else if (sender == tmfi_status_curs()) {
			s_bar = STAT_BAR::CURS;
			check = tmfi_status_curs().IsChecked();
			tmfi_status_curs_2().IsChecked(check);
			status_set_curs();
			status_visiblity(check, tk_status_pos_x());
			status_visiblity(check, tk_status_pos_y());
		}
		else if (sender == tmfi_status_curs_2()) {
			s_bar = STAT_BAR::CURS;
			check = tmfi_status_curs_2().IsChecked();
			tmfi_status_curs().IsChecked(check);
			status_set_curs();
			status_visiblity(check, tk_status_pos_x());
			status_visiblity(check, tk_status_pos_y());
		}
		else if (sender == tmfi_status_zoom()) {
			s_bar = STAT_BAR::ZOOM;
			check = tmfi_status_zoom().IsChecked();
			tmfi_status_zoom_2().IsChecked(check);
			status_set_zoom();
			status_visiblity(check, tk_status_zoom());
		}
		else if (sender == tmfi_status_zoom_2()) {
			s_bar = STAT_BAR::ZOOM;
			check = tmfi_status_zoom_2().IsChecked();
			tmfi_status_zoom().IsChecked(check);
			status_set_zoom();
			status_visiblity(check, tk_status_zoom());
		}
		else if (sender == tmfi_status_tool()) {
			s_bar = STAT_BAR::DRAW;
			check = tmfi_status_tool().IsChecked();
			tmfi_status_tool_2().IsChecked(check);
			status_set_draw();
			status_visiblity(check, sp_status_tool());
		}
		else if (sender == tmfi_status_tool_2()) {
			s_bar = STAT_BAR::DRAW;
			check = tmfi_status_tool_2().IsChecked();
			tmfi_status_tool().IsChecked(check);
			status_set_draw();
			status_visiblity(check, sp_status_tool());
		}
		else if (sender == tmfi_status_unit()) {
			s_bar = STAT_BAR::UNIT;
			check = tmfi_status_unit().IsChecked();
			tmfi_status_unit_2().IsChecked(check);
			status_set_unit();
			status_visiblity(check, tk_status_unit());
		}
		else if (sender == tmfi_status_unit_2()) {
			s_bar = STAT_BAR::UNIT;
			check = tmfi_status_unit_2().IsChecked();
			tmfi_status_unit().IsChecked(check);
			status_set_unit();
			status_visiblity(check, tk_status_unit());
		}
		else {
			return;
		}
		if (check) {
			m_status_bar = status_or(m_status_bar, s_bar);
		}
		else {
			m_status_bar = status_and(m_status_bar, status_not(s_bar));
		}
		status_visiblity(m_status_bar != static_cast<STAT_BAR>(0), sp_status_bar());
	}

	// ステータスバーのメニュー項目に印をつける.
	void MainPage::status_check_menu(const STAT_BAR st_bar)
	{
		tmfi_status_curs().IsChecked(status_mask(st_bar, STAT_BAR::CURS));
		tmfi_status_grid().IsChecked(status_mask(st_bar, STAT_BAR::GRID));
		tmfi_status_page().IsChecked(status_mask(st_bar, STAT_BAR::PAGE));
		tmfi_status_tool().IsChecked(status_mask(st_bar, STAT_BAR::DRAW));
		tmfi_status_unit().IsChecked(status_mask(st_bar, STAT_BAR::UNIT));
		tmfi_status_zoom().IsChecked(status_mask(st_bar, STAT_BAR::ZOOM));
		tmfi_status_curs_2().IsChecked(status_mask(st_bar, STAT_BAR::CURS));
		tmfi_status_grid_2().IsChecked(status_mask(st_bar, STAT_BAR::GRID));
		tmfi_status_page_2().IsChecked(status_mask(st_bar, STAT_BAR::PAGE));
		tmfi_status_tool_2().IsChecked(status_mask(st_bar, STAT_BAR::DRAW));
		tmfi_status_unit_2().IsChecked(status_mask(st_bar, STAT_BAR::UNIT));
		tmfi_status_zoom_2().IsChecked(status_mask(st_bar, STAT_BAR::ZOOM));
	}

	// 列挙型を OR 演算する.
	STAT_BAR MainPage::status_or(const STAT_BAR a, const STAT_BAR b) noexcept
	{
		return static_cast<STAT_BAR>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	// ステータスバーの状態をデータリーダーから読み込む.
	void MainPage::status_read(DataReader const& dt_reader)
	{
		m_status_bar = static_cast<STAT_BAR>(dt_reader.ReadUInt32());
	}

	// ポインターの位置をステータスバーに格納する.
	void MainPage::status_set_curs(void)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		//const double dpi = m_page_panel.m_dx.m_logical_dpi;
		const auto wp = CoreWindow::GetForCurrentThread().PointerPosition();
		const auto wb = CoreWindow::GetForCurrentThread().Bounds();
		const auto tr = scp_page_panel().TransformToVisual(nullptr);
		const auto tp = tr.TransformPoint({ 0.0f, 0.0f });
		const double sx = sb_horz().Value();
		const double sy = sb_vert().Value();
		const double wx = wp.X;
		const double tx = tp.X;
		const double bx = wb.X;
		const double wy = wp.Y;
		const double ty = tp.Y;
		const double by = wb.Y;
		const double px = m_page_min.x;
		const double py = m_page_min.y;
		const double ps = m_page_panel.m_page_scale;
		const double fx = (wx - bx - tx) / ps + sx + px;
		const double fy = (wy - by - ty) / ps + sy + py;
		double x, y;
		wchar_t const* format;
		wchar_t buf[32];

		switch (m_page_unit) {
		case LEN_UNIT::INCH:
			format = FMT_INCH;
			x = fx / dpi;
			y = fy / dpi;
			break;
		case LEN_UNIT::MILLI:
			format = FMT_MILLI;
			x = fx / dpi * MM_PER_INCH;
			y = fy / dpi * MM_PER_INCH;
			break;
		case LEN_UNIT::POINT:
			format = FMT_POINT;
			x = fx / dpi * PT_PER_INCH;
			y = fy / dpi * PT_PER_INCH;
			break;
		case LEN_UNIT::GRID:
			format = FMT_GRID;
			x = fx / (m_page_panel.m_grid_size + 1.0);
			y = fy / (m_page_panel.m_grid_size + 1.0);
			break;
		default:
			format = FMT_PIXEL;
			x = fx;
			y = fy;
			break;
		}
		swprintf_s(buf, format, x);
		tk_status_pos_x().Text(winrt::hstring{ L"x:" } + buf);
		swprintf_s(buf, format, y);
		tk_status_pos_y().Text(winrt::hstring{ L"y:" } + buf);
swprintf_s(buf, L"%d", static_cast<uint32_t>(m_list_shapes.size()));
tk_status_cnt().Text(winrt::hstring{ L"c:" } +buf);
	}

	// 方眼の大きさをステータスバーに格納する.
	void MainPage::status_set_grid(void)
	{
		wchar_t buf[32];
		const double dpi = m_page_dx.m_logical_dpi;
		double g_len = m_page_panel.m_grid_size + 1.0;
		wchar_t const* format;
		switch (m_page_unit) {
		case LEN_UNIT::INCH:
			format = FMT_INCH;
			g_len = g_len / dpi;
			break;
		case LEN_UNIT::MILLI:
			format = FMT_MILLI;
			g_len = g_len * MM_PER_INCH / dpi;
			break;
		case LEN_UNIT::POINT:
			format = FMT_POINT;
			g_len = g_len * PT_PER_INCH / dpi;
			break;
		case LEN_UNIT::GRID:
			format = FMT_GRID;
			g_len = 1.0;
			break;
		default:
			format = FMT_PIXEL;
			break;
		}
		swprintf_s(buf, format, g_len);
		tk_status_grid().Text(winrt::hstring{ L"g:" } +buf);
	}

	// ページの大きさをステータスバーに格納する.
	void MainPage::status_set_page(void)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		const double g_len = m_page_panel.m_grid_size + 1.0;
		wchar_t buf[32];
		conv_val_to_len(m_page_unit, m_page_panel.m_page_size.width, dpi, g_len, buf, 31);
		tk_status_width().Text(winrt::hstring{ L"w:" } + buf);
		conv_val_to_len(m_page_unit, m_page_panel.m_page_size.height, dpi, g_len, buf, 31);
		tk_status_height().Text(winrt::hstring{ L"h:" } + buf);
	}

	// 作図ツールをステータスバーに格納する.
	void MainPage::status_set_draw(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring data;
		if (m_draw_tool == DRAW_TOOL::TOOL_BEZI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_bezi")));
		}
		else if (m_draw_tool == DRAW_TOOL::TOOL_ELLI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_elli")));
		}
		else if (m_draw_tool == DRAW_TOOL::TOOL_LINE) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_line")));
		}
		else if (m_draw_tool == DRAW_TOOL::TOOL_QUAD) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_quad")));
		}
		else if (m_draw_tool == DRAW_TOOL::TOOL_RECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rect")));
		}
		else if (m_draw_tool == DRAW_TOOL::TOOL_RRECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rrect")));
		}
		else if (m_draw_tool == DRAW_TOOL::TOOL_TEXT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_text")));
		}
		else if (m_draw_tool == DRAW_TOOL::TOOL_SCALE) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_text")));
		}
		else {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_select")));
		}
		pi_tool().Data(nullptr);
		pi_tool().Data(Summary::Data(data));
	}

	// 単位をステータスバーに格納する.
	void MainPage::status_set_unit(void)
	{
		tk_status_unit().Text(winrt::hstring{ L"u:" } + get_unit_name());
	}

	// 拡大率をステータスバーに格納する.
	void MainPage::status_set_zoom(void)
	{
		wchar_t buf[32];
		swprintf_s(buf, 31, FMT_ZOOM, m_page_panel.m_page_scale * 100.0);
		tk_status_zoom().Text(winrt::hstring{ L"z:" } +buf);
	}

	// ステータスバーの表示を設定する.
	void MainPage::status_visibility(void)
	{
		tk_status_pos_x().Visibility(status_mask(m_status_bar, STAT_BAR::CURS) ? VISIBLE : COLLAPSED);
		tk_status_pos_y().Visibility(status_mask(m_status_bar, STAT_BAR::CURS) ? VISIBLE : COLLAPSED);
		tk_status_grid().Visibility(status_mask(m_status_bar, STAT_BAR::GRID) ? VISIBLE : COLLAPSED);
		tk_status_width().Visibility(status_mask(m_status_bar, STAT_BAR::PAGE) ? VISIBLE : COLLAPSED);
		tk_status_height().Visibility(status_mask(m_status_bar, STAT_BAR::PAGE) ? VISIBLE : COLLAPSED);
		//sp_status_curs().Visibility(status_mask(m_status_bar, STAT_BAR::CURS) ? VISIBLE : COLLAPSED);
		//sp_status_grid().Visibility(status_mask(m_status_bar, STAT_BAR::GRID) ? VISIBLE : COLLAPSED);
		//sp_status_page().Visibility(status_mask(m_status_bar, STAT_BAR::PAGE) ? VISIBLE : COLLAPSED);
		sp_status_tool().Visibility(status_mask(m_status_bar, STAT_BAR::DRAW) ? VISIBLE : COLLAPSED);
		tk_status_unit().Visibility(status_mask(m_status_bar, STAT_BAR::UNIT) ? VISIBLE : COLLAPSED);
		tk_status_zoom().Visibility(status_mask(m_status_bar, STAT_BAR::ZOOM) ? VISIBLE : COLLAPSED);
		//sp_status_unit().Visibility(status_mask(m_status_bar, STAT_BAR::UNIT) ? VISIBLE : COLLAPSED);
		//sp_status_zoom().Visibility(status_mask(m_status_bar, STAT_BAR::ZOOM) ? VISIBLE : COLLAPSED);
		sp_status_bar().Visibility(m_status_bar != static_cast<STAT_BAR>(0) ? VISIBLE : COLLAPSED);
	}

	// ステータスバーの状態をデータライターに書き込む.
	void MainPage::status_write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_status_bar));
	}

}