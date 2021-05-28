//-------------------------------
// MainPage_sbar.cpp
// ステータスバー
//-------------------------------
#include "pch.h"
#include "Summary.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// AND 演算する.
	static SBAR_FLAG sbar_and(const SBAR_FLAG a, const SBAR_FLAG b) noexcept;
	// ビットマスクする.
	static bool sbar_mask(const SBAR_FLAG a, const SBAR_FLAG b) noexcept;
	// NOT 演算する.
	static SBAR_FLAG sbar_not(const SBAR_FLAG a) noexcept;
	// ステータスバーの項目の表示を設定する.
	static void sbar_visiblity(const bool check, FrameworkElement const& f_elem) noexcept;

	// 列挙型を AND 演算する.
	static SBAR_FLAG sbar_and(const SBAR_FLAG a, const SBAR_FLAG b) noexcept
	{
		return static_cast<SBAR_FLAG>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	// 列挙型をビットマスクする.
	static bool sbar_mask(const SBAR_FLAG a, const SBAR_FLAG b) noexcept
	{
		return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);
	}

	// 列挙型を NOT 演算する.
	static SBAR_FLAG sbar_not(const SBAR_FLAG a) noexcept
	{
		return static_cast<SBAR_FLAG>(~static_cast<uint32_t>(a));
	}

	// ステータスバーの項目の表示を設定する.
	static void sbar_visiblity(const bool check, FrameworkElement const& f_elem) noexcept
	{
		if (f_elem.Visibility() == (check ? COLLAPSED : VISIBLE)) {
			f_elem.Visibility(check ? VISIBLE : COLLAPSED);
		}
	}

	// 用紙メニューの「ステータスバー」が選択された.
	void MainPage::sbar_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		SBAR_FLAG stbar;	// ステータスバーの状態
		bool check;	// チェックマークの有無

		if (sender == tmfi_sbar_curs()) {
			stbar = SBAR_FLAG::CURS;
			check = tmfi_sbar_curs().IsChecked();
			tmfi_sbar_curs_2().IsChecked(check);
			// ポインターの位置をステータスバーに格納する.
			sbar_set_curs();
			sbar_visiblity(check, tk_sbar_pos_x());
			sbar_visiblity(check, tk_sbar_pos_y());
		}
		else if (sender == tmfi_sbar_curs_2()) {
			stbar = SBAR_FLAG::CURS;
			check = tmfi_sbar_curs_2().IsChecked();
			tmfi_sbar_curs().IsChecked(check);
			// ポインターの位置をステータスバーに格納する.
			sbar_set_curs();
			sbar_visiblity(check, tk_sbar_pos_x());
			sbar_visiblity(check, tk_sbar_pos_y());
		}
		else if (sender == tmfi_sbar_grid()) {
			stbar = SBAR_FLAG::GRID;
			check = tmfi_sbar_grid().IsChecked();
			tmfi_sbar_grid_2().IsChecked(check);
			// 方眼の大きさをステータスバーに格納する.
			sbar_set_grid();
			sbar_visiblity(check, tk_sbar_grid());
		}
		else if (sender == tmfi_sbar_grid_2()) {
			stbar = SBAR_FLAG::GRID;
			check = tmfi_sbar_grid_2().IsChecked();
			tmfi_sbar_grid().IsChecked(check);
			// 方眼の大きさをステータスバーに格納する.
			sbar_set_grid();
			sbar_visiblity(check, tk_sbar_grid());
		}
		else if (sender == tmfi_sbar_sheet()) {
			stbar = SBAR_FLAG::SHEET;
			check = tmfi_sbar_sheet().IsChecked();
			tmfi_sbar_sheet_2().IsChecked(check);
			// 用紙の大きさをステータスバーに格納する.
			sbar_set_sheet();
			sbar_visiblity(check, tk_sbar_width());
			sbar_visiblity(check, tk_sbar_height());
		}
		else if (sender == tmfi_sbar_sheet_2()) {
			stbar = SBAR_FLAG::SHEET;
			check = tmfi_sbar_sheet_2().IsChecked();
			tmfi_sbar_sheet().IsChecked(check);
			// 用紙の大きさをステータスバーに格納する.
			sbar_set_sheet();
			sbar_visiblity(check, tk_sbar_width());
			sbar_visiblity(check, tk_sbar_height());
		}
		else if (sender == tmfi_sbar_zoom()) {
			stbar = SBAR_FLAG::ZOOM;
			check = tmfi_sbar_zoom().IsChecked();
			tmfi_sbar_zoom_2().IsChecked(check);
			sbar_set_zoom();
			sbar_visiblity(check, tk_sbar_zoom());
		}
		else if (sender == tmfi_sbar_zoom_2()) {
			stbar = SBAR_FLAG::ZOOM;
			check = tmfi_sbar_zoom_2().IsChecked();
			tmfi_sbar_zoom().IsChecked(check);
			sbar_set_zoom();
			sbar_visiblity(check, tk_sbar_zoom());
		}
		else if (sender == tmfi_sbar_draw()) {
			stbar = SBAR_FLAG::DRAW;
			check = tmfi_sbar_draw().IsChecked();
			tmfi_sbar_draw_2().IsChecked(check);
			// 作図ツールをステータスバーに格納する.
			sbar_set_draw();
			sbar_visiblity(check, sp_sbar_draw());
		}
		else if (sender == tmfi_sbar_draw_2()) {
			stbar = SBAR_FLAG::DRAW;
			check = tmfi_sbar_draw_2().IsChecked();
			tmfi_sbar_draw().IsChecked(check);
			// 作図ツールをステータスバーに格納する.
			sbar_set_draw();
			sbar_visiblity(check, sp_sbar_draw());
		}
		else if (sender == tmfi_sbar_unit()) {
			stbar = SBAR_FLAG::UNIT;
			check = tmfi_sbar_unit().IsChecked();
			tmfi_sbar_unit_2().IsChecked(check);
			sbar_set_unit();
			sbar_visiblity(check, tk_sbar_unit());
		}
		else if (sender == tmfi_sbar_unit_2()) {
			stbar = SBAR_FLAG::UNIT;
			check = tmfi_sbar_unit_2().IsChecked();
			tmfi_sbar_unit().IsChecked(check);
			sbar_set_unit();
			sbar_visiblity(check, tk_sbar_unit());
		}
		else {
			return;
		}
		if (check) {
			m_status_bar = sbar_or(m_status_bar, stbar);
		}
		else {
			m_status_bar = sbar_and(m_status_bar, sbar_not(stbar));
		}
		sbar_visiblity(m_status_bar != static_cast<SBAR_FLAG>(0), sp_stbar());
	}

	// ステータスバーのメニュー項目に印をつける.
	void MainPage::sbar_check_menu(const SBAR_FLAG st_bar)
	{
		tmfi_sbar_curs().IsChecked(sbar_mask(st_bar, SBAR_FLAG::CURS));
		tmfi_sbar_grid().IsChecked(sbar_mask(st_bar, SBAR_FLAG::GRID));
		tmfi_sbar_sheet().IsChecked(sbar_mask(st_bar, SBAR_FLAG::SHEET));
		tmfi_sbar_draw().IsChecked(sbar_mask(st_bar, SBAR_FLAG::DRAW));
		tmfi_sbar_unit().IsChecked(sbar_mask(st_bar, SBAR_FLAG::UNIT));
		tmfi_sbar_zoom().IsChecked(sbar_mask(st_bar, SBAR_FLAG::ZOOM));
		tmfi_sbar_curs_2().IsChecked(sbar_mask(st_bar, SBAR_FLAG::CURS));
		tmfi_sbar_grid_2().IsChecked(sbar_mask(st_bar, SBAR_FLAG::GRID));
		tmfi_sbar_sheet_2().IsChecked(sbar_mask(st_bar, SBAR_FLAG::SHEET));
		tmfi_sbar_draw_2().IsChecked(sbar_mask(st_bar, SBAR_FLAG::DRAW));
		tmfi_sbar_unit_2().IsChecked(sbar_mask(st_bar, SBAR_FLAG::UNIT));
		tmfi_sbar_zoom_2().IsChecked(sbar_mask(st_bar, SBAR_FLAG::ZOOM));
	}

	// 列挙型を OR 演算する.
	SBAR_FLAG MainPage::sbar_or(const SBAR_FLAG a, const SBAR_FLAG b) noexcept
	{
		return static_cast<SBAR_FLAG>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	// ステータスバーの状態をデータリーダーから読み込む.
	void MainPage::sbar_read(DataReader const& dt_reader)
	{
		m_status_bar = static_cast<SBAR_FLAG>(dt_reader.ReadUInt32());
		status_bar(m_status_bar);
	}

	// ポインターの位置をステータスバーに格納する.
	void MainPage::sbar_set_curs(void)
	{
		const double dpi = m_sheet_dx.m_logical_dpi;
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
		const double ps = m_sheet_main.m_sheet_scale;
		const double fx = (wx - bx - tx) / ps + sx + px;
		const double fy = (wy - by - ty) / ps + sy + py;
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		const double g_len = g_base + 1.0;

		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), fx, dpi, g_len, buf);
		tk_sbar_pos_x().Text(winrt::hstring{ L"x:" } + buf);
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), fy, dpi, g_len, buf);
		tk_sbar_pos_y().Text(winrt::hstring{ L"y:" } + buf);

swprintf_s(buf, L"%d", static_cast<uint32_t>(m_list_shapes.size()));
tk_sbar_cnt().Text(winrt::hstring{ L"c:" } + buf);
	}

	// 方眼の大きさをステータスバーに格納する.
	void MainPage::sbar_set_grid(void)
	{
		wchar_t buf[32];
		const double dpi = m_sheet_dx.m_logical_dpi;
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		const double g_len = g_base + 1.0;
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), g_len, dpi, g_len, buf);
		tk_sbar_grid().Text(winrt::hstring{ L"g:" } +buf);
	}

	// 用紙の大きさをステータスバーに格納する.
	void MainPage::sbar_set_sheet(void)
	{
		const double dpi = m_sheet_dx.m_logical_dpi;
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		const double g_len = g_base + 1.0;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), m_sheet_main.m_sheet_size.width, dpi, g_len, buf);
		tk_sbar_width().Text(winrt::hstring{ L"w:" } + buf);
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), m_sheet_main.m_sheet_size.height, dpi, g_len, buf);
		tk_sbar_height().Text(winrt::hstring{ L"h:" } + buf);
	}

	// 作図ツールをステータスバーに格納する.
	void MainPage::sbar_set_draw(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		const auto t_draw = tool_draw();
		winrt::hstring data;
		if (t_draw == TOOL_DRAW::BEZI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_bezi")));
		}
		else if (t_draw == TOOL_DRAW::ELLI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_elli")));
		}
		else if (t_draw == TOOL_DRAW::LINE) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_line")));
		}
		else if (t_draw == TOOL_DRAW::POLY) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_tri")));
		}
		else if (t_draw == TOOL_DRAW::RECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rect")));
		}
		else if (t_draw == TOOL_DRAW::RRECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rrct")));
		}
		else if (t_draw == TOOL_DRAW::RULER) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_text")));
		}
		else if (t_draw == TOOL_DRAW::SELECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_select")));
		}
		else if (t_draw == TOOL_DRAW::TEXT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_text")));
		}
		else {
			throw winrt::hresult_invalid_argument();
		}
		pi_draw().Data(nullptr);
		pi_draw().Data(Summary::Data(data));
	}

	// 単位をステータスバーに格納する.
	void MainPage::sbar_set_unit(void)
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
		tk_sbar_unit().Text(winrt::hstring{ L"u:" } + unit_name);
	}

	// 拡大率をステータスバーに格納する.
	void MainPage::sbar_set_zoom(void)
	{
		wchar_t buf[32];
		swprintf_s(buf, 31, FMT_ZOOM, m_sheet_main.m_sheet_scale * 100.0);
		tk_sbar_zoom().Text(winrt::hstring{ L"z:" } + buf);
	}

	// ステータスバーの表示を設定する.
	void MainPage::sbar_visibility(void)
	{
		tk_sbar_pos_x().Visibility(sbar_mask(m_status_bar, SBAR_FLAG::CURS) ? VISIBLE : COLLAPSED);
		tk_sbar_pos_y().Visibility(sbar_mask(m_status_bar, SBAR_FLAG::CURS) ? VISIBLE : COLLAPSED);
		tk_sbar_grid().Visibility(sbar_mask(m_status_bar, SBAR_FLAG::GRID) ? VISIBLE : COLLAPSED);
		tk_sbar_width().Visibility(sbar_mask(m_status_bar, SBAR_FLAG::SHEET) ? VISIBLE : COLLAPSED);
		tk_sbar_height().Visibility(sbar_mask(m_status_bar, SBAR_FLAG::SHEET) ? VISIBLE : COLLAPSED);
		//sp_sbar_curs().Visibility(sbar_mask(m_status_bar, SBAR_FLAG::CURS) ? VISIBLE : COLLAPSED);
		//sp_sbar_grid().Visibility(sbar_mask(m_status_bar, SBAR_FLAG::GRID) ? VISIBLE : COLLAPSED);
		//sp_sbar_sheet().Visibility(sbar_mask(m_status_bar, SBAR_FLAG::SHEET) ? VISIBLE : COLLAPSED);
		sp_sbar_draw().Visibility(sbar_mask(m_status_bar, SBAR_FLAG::DRAW) ? VISIBLE : COLLAPSED);
		tk_sbar_unit().Visibility(sbar_mask(m_status_bar, SBAR_FLAG::UNIT) ? VISIBLE : COLLAPSED);
		tk_sbar_zoom().Visibility(sbar_mask(m_status_bar, SBAR_FLAG::ZOOM) ? VISIBLE : COLLAPSED);
		//sp_sbar_unit().Visibility(sbar_mask(m_status_bar, SBAR_FLAG::UNIT) ? VISIBLE : COLLAPSED);
		//sp_sbar_zoom().Visibility(sbar_mask(m_status_bar, SBAR_FLAG::ZOOM) ? VISIBLE : COLLAPSED);
		sp_stbar().Visibility(m_status_bar != static_cast<SBAR_FLAG>(0) ? VISIBLE : COLLAPSED);
	}

	// ステータスバーの状態をデータライターに書き込む.
	void MainPage::sbar_write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_status_bar));
	}

}