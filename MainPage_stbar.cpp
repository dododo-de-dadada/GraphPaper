//-------------------------------
// MainPage_stbar.cpp
// ステータスバーの設定
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

	// ページメニューの「ステータスバー」が選択された.
	void MainPage::mi_stbar_click(IInspectable const& sender, RoutedEventArgs const&)
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
		else if (sender == tmfi_stbar_page()) {
			stbar = STATUS_BAR::PAGE;
			check = tmfi_stbar_page().IsChecked();
			tmfi_stbar_page_2().IsChecked(check);
			// ページの大きさをステータスバーに格納する.
			stbar_set_page();
			stbar_visiblity(check, tk_stbar_width());
			stbar_visiblity(check, tk_stbar_height());
		}
		else if (sender == tmfi_stbar_page_2()) {
			stbar = STATUS_BAR::PAGE;
			check = tmfi_stbar_page_2().IsChecked();
			tmfi_stbar_page().IsChecked(check);
			// ページの大きさをステータスバーに格納する.
			stbar_set_page();
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
		else if (sender == tmfi_stbar_tool()) {
			stbar = STATUS_BAR::DRAW;
			check = tmfi_stbar_tool().IsChecked();
			tmfi_stbar_tool_2().IsChecked(check);
			// 作図ツールをステータスバーに格納する.
			stbar_set_draw();
			stbar_visiblity(check, sp_stbar_tool());
		}
		else if (sender == tmfi_stbar_tool_2()) {
			stbar = STATUS_BAR::DRAW;
			check = tmfi_stbar_tool_2().IsChecked();
			tmfi_stbar_tool().IsChecked(check);
			// 作図ツールをステータスバーに格納する.
			stbar_set_draw();
			stbar_visiblity(check, sp_stbar_tool());
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
			m_stbar = stbar_or(m_stbar, stbar);
		}
		else {
			m_stbar = stbar_and(m_stbar, stbar_not(stbar));
		}
		stbar_visiblity(m_stbar != static_cast<STATUS_BAR>(0), sp_stbar());
	}

	// ステータスバーのメニュー項目に印をつける.
	void MainPage::stbar_check_menu(const STATUS_BAR st_bar)
	{
		tmfi_stbar_curs().IsChecked(stbar_mask(st_bar, STATUS_BAR::CURS));
		tmfi_stbar_grid().IsChecked(stbar_mask(st_bar, STATUS_BAR::GRID));
		tmfi_stbar_page().IsChecked(stbar_mask(st_bar, STATUS_BAR::PAGE));
		tmfi_stbar_tool().IsChecked(stbar_mask(st_bar, STATUS_BAR::DRAW));
		tmfi_stbar_unit().IsChecked(stbar_mask(st_bar, STATUS_BAR::UNIT));
		tmfi_stbar_zoom().IsChecked(stbar_mask(st_bar, STATUS_BAR::ZOOM));
		tmfi_stbar_curs_2().IsChecked(stbar_mask(st_bar, STATUS_BAR::CURS));
		tmfi_stbar_grid_2().IsChecked(stbar_mask(st_bar, STATUS_BAR::GRID));
		tmfi_stbar_page_2().IsChecked(stbar_mask(st_bar, STATUS_BAR::PAGE));
		tmfi_stbar_tool_2().IsChecked(stbar_mask(st_bar, STATUS_BAR::DRAW));
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
		m_stbar = static_cast<STATUS_BAR>(dt_reader.ReadUInt32());
	}

	// ポインターの位置をステータスバーに格納する.
	void MainPage::stbar_set_curs(void)
	{
		const double dpi = m_page_dx.m_logical_dpi;
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
		const double ps = m_page_layout.m_page_scale;
		const double fx = (wx - bx - tx) / ps + sx + px;
		const double fy = (wy - by - ty) / ps + sy + py;
		const double g_len = m_page_layout.m_grid_base + 1.0;

		wchar_t buf[32];
		// ピクセル単位の長さを他の単位の文字列に変換する.
		conv_val_to_len<!WITH_UNIT_NAME>(m_len_unit, fx, dpi, g_len, buf);
		tk_stbar_pos_x().Text(winrt::hstring{ L"x:" } + buf);
		// ピクセル単位の長さを他の単位の文字列に変換する.
		conv_val_to_len<!WITH_UNIT_NAME>(m_len_unit, fy, dpi, g_len, buf);
		tk_stbar_pos_y().Text(winrt::hstring{ L"y:" } + buf);

swprintf_s(buf, L"%d", static_cast<uint32_t>(m_list_shapes.size()));
tk_stbar_cnt().Text(winrt::hstring{ L"c:" } + buf);
	}

	// 方眼の大きさをステータスバーに格納する.
	void MainPage::stbar_set_grid(void)
	{
		wchar_t buf[32];
		const double dpi = m_page_dx.m_logical_dpi;
		double g_len = m_page_layout.m_grid_base + 1.0;
		// ピクセル単位の長さを他の単位の文字列に変換する.
		conv_val_to_len<!WITH_UNIT_NAME>(m_len_unit, g_len, dpi, g_len, buf);
		tk_stbar_grid().Text(winrt::hstring{ L"g:" } +buf);
	}

	// ページの大きさをステータスバーに格納する.
	void MainPage::stbar_set_page(void)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		const double g_len = m_page_layout.m_grid_base + 1.0;
		wchar_t buf[32];
		// ピクセル単位の長さを他の単位の文字列に変換する.
		conv_val_to_len<!WITH_UNIT_NAME>(m_len_unit, m_page_layout.m_page_size.width, dpi, g_len, buf);
		tk_stbar_width().Text(winrt::hstring{ L"w:" } + buf);
		// ピクセル単位の長さを他の単位の文字列に変換する.
		conv_val_to_len<!WITH_UNIT_NAME>(m_len_unit, m_page_layout.m_page_size.height, dpi, g_len, buf);
		tk_stbar_height().Text(winrt::hstring{ L"h:" } + buf);
	}

	// 作図ツールをステータスバーに格納する.
	void MainPage::stbar_set_draw(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring data;
		if (m_draw_tool == DRAW_TOOL::BEZI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_bezi")));
		}
		else if (m_draw_tool == DRAW_TOOL::ELLI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_elli")));
		}
		else if (m_draw_tool == DRAW_TOOL::LINE) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_line")));
		}
		else if (m_draw_tool == DRAW_TOOL::QUAD) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_quad")));
		}
		else if (m_draw_tool == DRAW_TOOL::RECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rect")));
		}
		else if (m_draw_tool == DRAW_TOOL::RRCT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rrct")));
		}
		else if (m_draw_tool == DRAW_TOOL::SCALE) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_text")));
		}
		else if (m_draw_tool == DRAW_TOOL::SELECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_select")));
		}
		else if (m_draw_tool == DRAW_TOOL::TEXT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_text")));
		}
		else {
			throw winrt::hresult_not_implemented();
		}
		pi_tool().Data(nullptr);
		pi_tool().Data(Summary::Data(data));
	}

	// 単位をステータスバーに格納する.
	void MainPage::stbar_set_unit(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring unit_name{};
		if (m_len_unit == LEN_UNIT::GRID) {
			unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_grid/Text");
		}
		else if (m_len_unit == LEN_UNIT::INCH) {
			unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_inch/Text");
		}
		else if (m_len_unit == LEN_UNIT::MILLI) {
			unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_milli/Text");
		}
		else if (m_len_unit == LEN_UNIT::PIXEL) {
			unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_pixel/Text");
		}
		else if (m_len_unit == LEN_UNIT::POINT) {
			unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_point/Text");
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
		swprintf_s(buf, 31, FMT_ZOOM, m_page_layout.m_page_scale * 100.0);
		tk_stbar_zoom().Text(winrt::hstring{ L"z:" } +buf);
	}

	// ステータスバーの表示を設定する.
	void MainPage::stbar_visibility(void)
	{
		tk_stbar_pos_x().Visibility(stbar_mask(m_stbar, STATUS_BAR::CURS) ? VISIBLE : COLLAPSED);
		tk_stbar_pos_y().Visibility(stbar_mask(m_stbar, STATUS_BAR::CURS) ? VISIBLE : COLLAPSED);
		tk_stbar_grid().Visibility(stbar_mask(m_stbar, STATUS_BAR::GRID) ? VISIBLE : COLLAPSED);
		tk_stbar_width().Visibility(stbar_mask(m_stbar, STATUS_BAR::PAGE) ? VISIBLE : COLLAPSED);
		tk_stbar_height().Visibility(stbar_mask(m_stbar, STATUS_BAR::PAGE) ? VISIBLE : COLLAPSED);
		//sp_stbar_curs().Visibility(stbar_mask(m_stbar, STATUS_BAR::CURS) ? VISIBLE : COLLAPSED);
		//sp_stbar_grid().Visibility(stbar_mask(m_stbar, STATUS_BAR::GRID) ? VISIBLE : COLLAPSED);
		//sp_stbar_page().Visibility(stbar_mask(m_stbar, STATUS_BAR::PAGE) ? VISIBLE : COLLAPSED);
		sp_stbar_tool().Visibility(stbar_mask(m_stbar, STATUS_BAR::DRAW) ? VISIBLE : COLLAPSED);
		tk_stbar_unit().Visibility(stbar_mask(m_stbar, STATUS_BAR::UNIT) ? VISIBLE : COLLAPSED);
		tk_stbar_zoom().Visibility(stbar_mask(m_stbar, STATUS_BAR::ZOOM) ? VISIBLE : COLLAPSED);
		//sp_stbar_unit().Visibility(stbar_mask(m_stbar, STATUS_BAR::UNIT) ? VISIBLE : COLLAPSED);
		//sp_stbar_zoom().Visibility(stbar_mask(m_stbar, STATUS_BAR::ZOOM) ? VISIBLE : COLLAPSED);
		sp_stbar().Visibility(m_stbar != static_cast<STATUS_BAR>(0) ? VISIBLE : COLLAPSED);
	}

	// ステータスバーの状態をデータライターに書き込む.
	void MainPage::stbar_write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_stbar));
	}

}