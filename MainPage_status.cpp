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
			s_bar = STAT_BAR::TOOL;
			check = tmfi_stat_tool().IsChecked();
			tmfi_stat_tool_2().IsChecked(check);
			stat_set_tool();
			stat_visiblity(check, sp_stat_tool());
		}
		else if (sender == tmfi_stat_tool_2()) {
			s_bar = STAT_BAR::TOOL;
			check = tmfi_stat_tool_2().IsChecked();
			tmfi_stat_tool().IsChecked(check);
			stat_set_tool();
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
		tmfi_stat_tool().IsChecked(stat_mask(st_bar, STAT_BAR::TOOL));
		tmfi_stat_unit().IsChecked(stat_mask(st_bar, STAT_BAR::UNIT));
		tmfi_stat_zoom().IsChecked(stat_mask(st_bar, STAT_BAR::ZOOM));
		tmfi_stat_curs_2().IsChecked(stat_mask(st_bar, STAT_BAR::CURS));
		tmfi_stat_grid_2().IsChecked(stat_mask(st_bar, STAT_BAR::GRID));
		tmfi_stat_page_2().IsChecked(stat_mask(st_bar, STAT_BAR::PAGE));
		tmfi_stat_tool_2().IsChecked(stat_mask(st_bar, STAT_BAR::TOOL));
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
		wchar_t const* fmt;
		wchar_t buf[16];

		switch (m_page_panel.m_page_unit) {
		case UNIT::INCH:
			fmt = FMT_IN;
			x = fx / dpi;
			y = fy / dpi;
			break;
		case UNIT::MILLI:
			fmt = FMT_MM;
			x = fx / dpi * MM_PER_INCH;
			y = fy / dpi * MM_PER_INCH;
			break;
		case UNIT::POINT:
			fmt = FMT_PT;
			x = fx / dpi * PT_PER_INCH;
			y = fy / dpi * PT_PER_INCH;
			break;
		case UNIT::GRID:
			fmt = FMT_GD;
			x = fx / (m_page_panel.m_grid_len + 1.0);
			y = fy / (m_page_panel.m_grid_len + 1.0);
			break;
		default:
			fmt = FMT_PX;
			x = fx;
			y = fy;
			break;
		}
		swprintf_s(buf, fmt, x);
		tk_stat_pos_x().Text(winrt::hstring{ L"x:" } +buf);
		swprintf_s(buf, fmt, y);
		tk_stat_pos_y().Text(winrt::hstring{ L"y:" } +buf);
		swprintf_s(buf, L"%d", static_cast<uint32_t>(m_list_shapes.size()));
		tk_stat_cnt().Text(winrt::hstring{ L"c:" } +buf);
	}

	// 方眼の大きさをステータスバーに格納する.
	void MainPage::stat_set_grid(void)
	{
		wchar_t buf[16];
		const double dpi = m_page_dx.m_logical_dpi;
		double g = m_page_panel.m_grid_len + 1.0;
		wchar_t const* fmt;
		switch (m_page_panel.m_page_unit) {
		case UNIT::INCH:
			fmt = FMT_IN;
			g = g / dpi;
			break;
		case UNIT::MILLI:
			fmt = FMT_MM;
			g = g / dpi * MM_PER_INCH;
			break;
		case UNIT::POINT:
			fmt = FMT_PT;
			g = g / dpi * PT_PER_INCH;
			break;
		case UNIT::GRID:
			fmt = FMT_GD;
			g = 1.0;
			break;
		default:
			fmt = FMT_PX;
			break;
		}
		swprintf_s(buf, fmt, g);
		tk_stat_grid().Text(winrt::hstring{ L"g:" } +buf);
	}

	// ページの大きさをステータスバーに格納する.
	void MainPage::stat_set_page(void)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		//const double dpi = m_page_panel.m_dx.m_logical_dpi;
		double w = m_page_panel.m_page_size.width;// m_page_max.x - m_page_min.x;
		double h = m_page_panel.m_page_size.height;// m_page_max.y - m_page_min.y;
		wchar_t const* fmt;
		switch (m_page_panel.m_page_unit) {
		case UNIT::INCH:
			fmt = FMT_IN;
			w = w / dpi;
			h = h / dpi;
			break;
		case UNIT::MILLI:
			fmt = FMT_MM;
			w = w / dpi * MM_PER_INCH;
			h = h / dpi * MM_PER_INCH;
			break;
		case UNIT::POINT:
			fmt = FMT_PT;
			w = w / dpi * PT_PER_INCH;
			h = h / dpi * PT_PER_INCH;
			break;
		case UNIT::GRID:
			fmt = FMT_GD;
			w /= m_page_panel.m_grid_len + 1.0;
			h /= m_page_panel.m_grid_len + 1.0;
			break;

		default:
			fmt = FMT_PX;
			break;
		}
		wchar_t buf[16];
		swprintf_s(buf, fmt, w);
		tk_stat_width().Text(winrt::hstring{ L"w:" } +buf);
		swprintf_s(buf, fmt, h);
		tk_stat_height().Text(winrt::hstring{ L"h:" } +buf);
	}

	// 図形ツールをステータスバーに格納する.
	void MainPage::stat_set_tool(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring data;
		if (m_tool_shape == TOOL_BEZI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_bezi")));
		}
		else if (m_tool_shape == TOOL_ELLI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_elli")));
		}
		else if (m_tool_shape == TOOL_LINE) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_line")));
		}
		else if (m_tool_shape == TOOL_QUAD) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_quad")));
		}
		else if (m_tool_shape == TOOL_RECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rect")));
		}
		else if (m_tool_shape == TOOL_RRECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rrect")));
		}
		else if (m_tool_shape == TOOL_TEXT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_text")));
		}
		else if (m_tool_shape == TOOL_RULER) {
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
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		wchar_t* r_key;
		if (m_page_panel.m_page_unit == UNIT::GRID) {
			r_key = L"cxi_unit_grid/Content";
		}
		else if (m_page_panel.m_page_unit == UNIT::INCH) {
			r_key = L"cxi_unit_inch/Content";
		}
		else if (m_page_panel.m_page_unit == UNIT::MILLI) {
			r_key = L"cxi_unit_milli/Content";
		}
		else if (m_page_panel.m_page_unit == UNIT::PIXEL) {
			r_key = L"cxi_unit_pixel/Content";
		}
		else if (m_page_panel.m_page_unit == UNIT::POINT) {
			r_key = L"cxi_unit_point/Content";
		}
		else {
			return;
		}
		auto r_loader{ ResourceLoader::GetForCurrentView() };
		auto unit = winrt::hstring{ L"u:" } +r_loader.GetString(r_key);
		tk_stat_unit().Text(unit);
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
		sp_stat_tool().Visibility(stat_mask(m_stat_bar, STAT_BAR::TOOL) ? VISIBLE : COLLAPSED);
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