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
	static STAT_BAR stat_and(const STAT_BAR a, const STAT_BAR b) noexcept;
	// 列挙型をビットマスクする.
	static bool stat_mask(const STAT_BAR a, const STAT_BAR b) noexcept;
	// 列挙型を NOT 演算する.
	static STAT_BAR stat_not(const STAT_BAR a) noexcept;
	// ステータスバーの項目のビジビリティを設定する.
	static void stat_visiblity(const bool check, FrameworkElement const& f_elem) noexcept;

	// 列挙型を AND 演算する.
	static STAT_BAR stat_and(const STAT_BAR a, const STAT_BAR b) noexcept
	{
		return static_cast<STAT_BAR>(static_cast<uint32_t>(a)& static_cast<uint32_t>(b));
	}

	// 列挙型をビットマスクする.
	static bool stat_mask(const STAT_BAR a, const STAT_BAR b) noexcept
	{
		return static_cast<uint32_t>(a)& static_cast<uint32_t>(b);
	}

	// 列挙型を NOT 演算する.
	static STAT_BAR stat_not(const STAT_BAR a) noexcept
	{
		return static_cast<STAT_BAR>(~static_cast<uint32_t>(a));
	}

	// ステータスバーの項目のビジビリティを設定する.
	static void stat_visiblity(const bool check, FrameworkElement const& f_elem) noexcept
	{
		if (f_elem.Visibility() == (check ? COLLAPSED : VISIBLE)) {
			f_elem.Visibility(check ? VISIBLE : COLLAPSED);
		}
	}

	// ページメニューの「ステータスバー」が選択された.
	void MainPage::mi_stat_bar_click(IInspectable const& sender, RoutedEventArgs const& /*args*/)
	{
		STAT_BAR s_bar;	// ステータスバーの状態
		bool check;	// チェックマークの有無

		if (sender == tmfi_stat_grid()) {
			s_bar = STAT_BAR::GRID;
			check = tmfi_stat_grid().IsChecked();
			tmfi_stat_grid_2().IsChecked(check);
			stat_set_grid();
			stat_visiblity(check, tk_stat_grid());
		}
		else if (sender == tmfi_stat_grid_2()) {
			s_bar = STAT_BAR::GRID;
			check = tmfi_stat_grid_2().IsChecked();
			tmfi_stat_grid().IsChecked(check);
			stat_set_grid();
			stat_visiblity(check, tk_stat_grid());
		}
		else if (sender == tmfi_stat_page()) {
			s_bar = STAT_BAR::PAGE;
			check = tmfi_stat_page().IsChecked();
			tmfi_stat_page_2().IsChecked(check);
			stat_set_page();
			stat_visiblity(check, tk_stat_width());
			stat_visiblity(check, tk_stat_height());
		}
		else if (sender == tmfi_stat_page_2()) {
			s_bar = STAT_BAR::PAGE;
			check = tmfi_stat_page_2().IsChecked();
			tmfi_stat_page().IsChecked(check);
			stat_set_page();
			stat_visiblity(check, tk_stat_width());
			stat_visiblity(check, tk_stat_height());
		}
		else if (sender == tmfi_stat_curs()) {
			s_bar = STAT_BAR::CURS;
			check = tmfi_stat_curs().IsChecked();
			tmfi_stat_curs_2().IsChecked(check);
			stat_set_curs();
			stat_visiblity(check, tk_stat_pos_x());
			stat_visiblity(check, tk_stat_pos_y());
		}
		else if (sender == tmfi_stat_curs_2()) {
			s_bar = STAT_BAR::CURS;
			check = tmfi_stat_curs_2().IsChecked();
			tmfi_stat_curs().IsChecked(check);
			stat_set_curs();
			stat_visiblity(check, tk_stat_pos_x());
			stat_visiblity(check, tk_stat_pos_y());
		}
		else if (sender == tmfi_stat_zoom()) {
			s_bar = STAT_BAR::ZOOM;
			check = tmfi_stat_zoom().IsChecked();
			tmfi_stat_zoom_2().IsChecked(check);
			stat_set_zoom();
			stat_visiblity(check, tk_stat_zoom());
		}
		else if (sender == tmfi_stat_zoom_2()) {
			s_bar = STAT_BAR::ZOOM;
			check = tmfi_stat_zoom_2().IsChecked();
			tmfi_stat_zoom().IsChecked(check);
			stat_set_zoom();
			stat_visiblity(check, tk_stat_zoom());
		}
		else if (sender == tmfi_stat_tool()) {
			s_bar = STAT_BAR::DRAW;
			check = tmfi_stat_tool().IsChecked();
			tmfi_stat_tool_2().IsChecked(check);
			stat_set_draw();
			stat_visiblity(check, sp_stat_tool());
		}
		else if (sender == tmfi_stat_tool_2()) {
			s_bar = STAT_BAR::DRAW;
			check = tmfi_stat_tool_2().IsChecked();
			tmfi_stat_tool().IsChecked(check);
			stat_set_draw();
			stat_visiblity(check, sp_stat_tool());
		}
		else if (sender == tmfi_stat_unit()) {
			s_bar = STAT_BAR::UNIT;
			check = tmfi_stat_unit().IsChecked();
			tmfi_stat_unit_2().IsChecked(check);
			stat_set_unit();
			stat_visiblity(check, tk_stat_unit());
		}
		else if (sender == tmfi_stat_unit_2()) {
			s_bar = STAT_BAR::UNIT;
			check = tmfi_stat_unit_2().IsChecked();
			tmfi_stat_unit().IsChecked(check);
			stat_set_unit();
			stat_visiblity(check, tk_stat_unit());
		}
		else {
			return;
		}
		if (check) {
			m_stat_bar = stat_or(m_stat_bar, s_bar);
		}
		else {
			m_stat_bar = stat_and(m_stat_bar, stat_not(s_bar));
		}
		stat_visiblity(m_stat_bar != static_cast<STAT_BAR>(0), sp_stat_bar());
	}

	// ステータスバーのメニュー項目に印をつける.
	void MainPage::stat_check_menu(const STAT_BAR st_bar)
	{
		tmfi_stat_curs().IsChecked(stat_mask(st_bar, STAT_BAR::CURS));
		tmfi_stat_grid().IsChecked(stat_mask(st_bar, STAT_BAR::GRID));
		tmfi_stat_page().IsChecked(stat_mask(st_bar, STAT_BAR::PAGE));
		tmfi_stat_tool().IsChecked(stat_mask(st_bar, STAT_BAR::DRAW));
		tmfi_stat_unit().IsChecked(stat_mask(st_bar, STAT_BAR::UNIT));
		tmfi_stat_zoom().IsChecked(stat_mask(st_bar, STAT_BAR::ZOOM));
		tmfi_stat_curs_2().IsChecked(stat_mask(st_bar, STAT_BAR::CURS));
		tmfi_stat_grid_2().IsChecked(stat_mask(st_bar, STAT_BAR::GRID));
		tmfi_stat_page_2().IsChecked(stat_mask(st_bar, STAT_BAR::PAGE));
		tmfi_stat_tool_2().IsChecked(stat_mask(st_bar, STAT_BAR::DRAW));
		tmfi_stat_unit_2().IsChecked(stat_mask(st_bar, STAT_BAR::UNIT));
		tmfi_stat_zoom_2().IsChecked(stat_mask(st_bar, STAT_BAR::ZOOM));
	}

	// 列挙型を OR 演算する.
	STAT_BAR MainPage::stat_or(const STAT_BAR a, const STAT_BAR b) noexcept
	{
		return static_cast<STAT_BAR>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	// ステータスバーの状態をデータリーダーから読み込む.
	void MainPage::stat_read(DataReader const& dt_reader)
	{
		m_stat_bar = static_cast<STAT_BAR>(dt_reader.ReadUInt32());
	}

	// ポインターの位置をステータスバーに格納する.
	void MainPage::stat_set_curs(void)
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
		wchar_t buf[16];

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
		tk_stat_pos_x().Text(winrt::hstring{ L"x:" } + buf);
		swprintf_s(buf, format, y);
		tk_stat_pos_y().Text(winrt::hstring{ L"y:" } + buf);
swprintf_s(buf, L"%d", static_cast<uint32_t>(m_list_shapes.size()));
tk_stat_cnt().Text(winrt::hstring{ L"c:" } +buf);
	}

	// 方眼の大きさをステータスバーに格納する.
	void MainPage::stat_set_grid(void)
	{
		wchar_t buf[16];
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
		tk_stat_grid().Text(winrt::hstring{ L"g:" } +buf);
	}

	// ページの大きさをステータスバーに格納する.
	void MainPage::stat_set_page(void)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		const double g_len = m_page_panel.m_grid_size + 1.0;
		wchar_t buf[16];
		conv_val_to_len(m_page_unit, m_page_panel.m_page_size.width, dpi, g_len, buf, 16);
		tk_stat_width().Text(winrt::hstring{ L"w:" } + buf);
		conv_val_to_len(m_page_unit, m_page_panel.m_page_size.height, dpi, g_len, buf, 16);
		tk_stat_height().Text(winrt::hstring{ L"h:" } + buf);
	}

	// 図形ツールをステータスバーに格納する.
	void MainPage::stat_set_draw(void)
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
	void MainPage::stat_set_unit(void)
	{
		tk_stat_unit().Text(winrt::hstring{ L"u:" } + get_unit_name());
	}

	// 拡大率をステータスバーに格納する.
	void MainPage::stat_set_zoom(void)
	{
		wchar_t buf[16];
		swprintf_s(buf, FMT_ZOOM, m_page_panel.m_page_scale * 100.0);
		tk_stat_zoom().Text(winrt::hstring{ L"z:" } +buf);
	}

	// ステータスバーの表示を設定する.
	void MainPage::stat_visibility(void)
	{
		tk_stat_pos_x().Visibility(stat_mask(m_stat_bar, STAT_BAR::CURS) ? VISIBLE : COLLAPSED);
		tk_stat_pos_y().Visibility(stat_mask(m_stat_bar, STAT_BAR::CURS) ? VISIBLE : COLLAPSED);
		tk_stat_grid().Visibility(stat_mask(m_stat_bar, STAT_BAR::GRID) ? VISIBLE : COLLAPSED);
		tk_stat_width().Visibility(stat_mask(m_stat_bar, STAT_BAR::PAGE) ? VISIBLE : COLLAPSED);
		tk_stat_height().Visibility(stat_mask(m_stat_bar, STAT_BAR::PAGE) ? VISIBLE : COLLAPSED);
		//sp_stat_curs().Visibility(stat_mask(m_stat_bar, STAT_BAR::CURS) ? VISIBLE : COLLAPSED);
		//sp_stat_grid().Visibility(stat_mask(m_stat_bar, STAT_BAR::GRID) ? VISIBLE : COLLAPSED);
		//sp_stat_page().Visibility(stat_mask(m_stat_bar, STAT_BAR::PAGE) ? VISIBLE : COLLAPSED);
		sp_stat_tool().Visibility(stat_mask(m_stat_bar, STAT_BAR::DRAW) ? VISIBLE : COLLAPSED);
		tk_stat_unit().Visibility(stat_mask(m_stat_bar, STAT_BAR::UNIT) ? VISIBLE : COLLAPSED);
		tk_stat_zoom().Visibility(stat_mask(m_stat_bar, STAT_BAR::ZOOM) ? VISIBLE : COLLAPSED);
		//sp_stat_unit().Visibility(stat_mask(m_stat_bar, STAT_BAR::UNIT) ? VISIBLE : COLLAPSED);
		//sp_stat_zoom().Visibility(stat_mask(m_stat_bar, STAT_BAR::ZOOM) ? VISIBLE : COLLAPSED);
		sp_stat_bar().Visibility(m_stat_bar != static_cast<STAT_BAR>(0) ? VISIBLE : COLLAPSED);
	}

	// ステータスバーの状態をデータライターに書き込む.
	void MainPage::stat_write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_stat_bar));
	}

}