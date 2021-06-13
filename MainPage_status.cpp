//-------------------------------
// MainPage_status.cpp
// ステータスバー
//-------------------------------
#include "pch.h"
#include "Summary.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// AND 演算する.
	static STATUS_BAR status_and(const STATUS_BAR a, const STATUS_BAR b) noexcept;
	// ビットマスクする.
	static bool status_mask(const STATUS_BAR a, const STATUS_BAR b) noexcept;
	// NOT 演算する.
	static STATUS_BAR status_not(const STATUS_BAR a) noexcept;
	// ステータスバーの項目の表示を設定する.
	static void status_visiblity(const bool check, FrameworkElement const& f_elem) noexcept;

	// 列挙型を AND 演算する.
	static STATUS_BAR status_and(const STATUS_BAR a, const STATUS_BAR b) noexcept
	{
		return static_cast<STATUS_BAR>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	// 列挙型をビットマスクする.
	static bool status_mask(const STATUS_BAR a, const STATUS_BAR b) noexcept
	{
		return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);
	}

	// 列挙型を NOT 演算する.
	static STATUS_BAR status_not(const STATUS_BAR a) noexcept
	{
		return static_cast<STATUS_BAR>(~static_cast<uint32_t>(a));
	}

	// ステータスバーの項目の表示を設定する.
	static void status_visiblity(const bool check, FrameworkElement const& f_elem) noexcept
	{
		if (f_elem.Visibility() == (check ? UI_COLLAPSED : UI_VISIBLE)) {
			f_elem.Visibility(check ? UI_VISIBLE : UI_COLLAPSED);
		}
	}

	// 用紙メニューの「ステータスバー」が選択された.
	void MainPage::status_bar_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		using winrt::Windows::UI::Xaml::Controls::ToggleMenuFlyoutItem;
		STATUS_BAR stbar;	// ステータスバーの状態
		bool check;

		if (sender == tmfi_status_bar_curs()) {
			stbar = STATUS_BAR::CURS;
			check = tmfi_status_bar_curs().IsChecked();
			tmfi_status_bar_curs_2().IsChecked(tmfi_status_bar_curs().IsChecked());
			status_bar_set_curs();
			status_visiblity(check, tk_status_bar_pos_x());
			status_visiblity(check, tk_status_bar_pos_y());
		}
		else if (sender == tmfi_status_bar_curs_2()) {
			stbar = STATUS_BAR::CURS;
			//const ToggleMenuFlyoutItem& m_item = unbox_value<ToggleMenuFlyoutItem>(sender);
			check = tmfi_status_bar_curs_2().IsChecked();
			tmfi_status_bar_curs().IsChecked(check);
			status_bar_set_curs();
			status_visiblity(check, tk_status_bar_pos_x());
			status_visiblity(check, tk_status_bar_pos_y());
		}
		else if (sender == tmfi_status_bar_grid()) {
			stbar = STATUS_BAR::GRID;
			check = tmfi_status_bar_grid().IsChecked();
			tmfi_status_bar_grid_2().IsChecked(check);
			status_bar_set_grid();
			status_visiblity(check, tk_status_bar_grid());
		}
		else if (sender == tmfi_status_bar_grid_2()) {
			stbar = STATUS_BAR::GRID;
			check = tmfi_status_bar_grid_2().IsChecked();
			tmfi_status_bar_grid().IsChecked(check);
			status_bar_set_grid();
			status_visiblity(check, tk_status_bar_grid());
		}
		else if (sender == tmfi_status_bar_sheet()) {
			stbar = STATUS_BAR::SHEET;
			check = tmfi_status_bar_sheet().IsChecked();
			tmfi_status_bar_sheet_2().IsChecked(check);
			status_bar_set_sheet();
			status_visiblity(check, tk_status_bar_width());
			status_visiblity(check, tk_status_bar_height());
		}
		else if (sender == tmfi_status_bar_sheet_2()) {
			stbar = STATUS_BAR::SHEET;
			check = tmfi_status_bar_sheet_2().IsChecked();
			tmfi_status_bar_sheet().IsChecked(check);
			status_bar_set_sheet();
			status_visiblity(check, tk_status_bar_width());
			status_visiblity(check, tk_status_bar_height());
		}
		else if (sender == tmfi_status_bar_zoom()) {
			stbar = STATUS_BAR::ZOOM;
			check = tmfi_status_bar_zoom().IsChecked();
			tmfi_status_bar_zoom_2().IsChecked(check);
			status_bar_set_zoom();
			status_visiblity(check, tk_status_bar_zoom());
		}
		else if (sender == tmfi_status_bar_zoom_2()) {
			stbar = STATUS_BAR::ZOOM;
			check = tmfi_status_bar_zoom_2().IsChecked();
			tmfi_status_bar_zoom().IsChecked(check);
			status_bar_set_zoom();
			status_visiblity(check, tk_status_bar_zoom());
		}
		else if (sender == tmfi_status_bar_draw()) {
			stbar = STATUS_BAR::DRAW;
			check = tmfi_status_bar_draw().IsChecked();
			tmfi_status_bar_draw_2().IsChecked(check);
			status_bar_set_draw();
			status_visiblity(check, sp_status_bar_panel_draw());
		}
		else if (sender == tmfi_status_bar_draw_2()) {
			stbar = STATUS_BAR::DRAW;
			check = tmfi_status_bar_draw_2().IsChecked();
			tmfi_status_bar_draw().IsChecked(check);
			status_bar_set_draw();
			status_visiblity(check, sp_status_bar_panel_draw());
		}
		else if (sender == tmfi_status_bar_unit()) {
			stbar = STATUS_BAR::UNIT;
			check = tmfi_status_bar_unit().IsChecked();
			tmfi_status_bar_unit_2().IsChecked(check);
			status_bar_set_unit();
			status_visiblity(check, tk_status_bar_unit());
		}
		else if (sender == tmfi_status_bar_unit_2()) {
			stbar = STATUS_BAR::UNIT;
			check = tmfi_status_bar_unit_2().IsChecked();
			tmfi_status_bar_unit().IsChecked(check);
			status_bar_set_unit();
			status_visiblity(check, tk_status_bar_unit());
		}
		else {
			return;
		}
		if (check) {
			m_status_bar = status_bar_or(m_status_bar, stbar);
		}
		else {
			m_status_bar = status_and(m_status_bar, status_not(stbar));
		}
		status_visiblity(m_status_bar != static_cast<STATUS_BAR>(0), sp_status_bar_panel());
	}

	// ステータスバーのメニュー項目に印をつける.
	void MainPage::status_bar_is_checked(const STATUS_BAR st_bar)
	{
		tmfi_status_bar_curs().IsChecked(status_mask(st_bar, STATUS_BAR::CURS));
		tmfi_status_bar_grid().IsChecked(status_mask(st_bar, STATUS_BAR::GRID));
		tmfi_status_bar_sheet().IsChecked(status_mask(st_bar, STATUS_BAR::SHEET));
		tmfi_status_bar_draw().IsChecked(status_mask(st_bar, STATUS_BAR::DRAW));
		tmfi_status_bar_unit().IsChecked(status_mask(st_bar, STATUS_BAR::UNIT));
		tmfi_status_bar_zoom().IsChecked(status_mask(st_bar, STATUS_BAR::ZOOM));
		tmfi_status_bar_curs_2().IsChecked(status_mask(st_bar, STATUS_BAR::CURS));
		tmfi_status_bar_grid_2().IsChecked(status_mask(st_bar, STATUS_BAR::GRID));
		tmfi_status_bar_sheet_2().IsChecked(status_mask(st_bar, STATUS_BAR::SHEET));
		tmfi_status_bar_draw_2().IsChecked(status_mask(st_bar, STATUS_BAR::DRAW));
		tmfi_status_bar_unit_2().IsChecked(status_mask(st_bar, STATUS_BAR::UNIT));
		tmfi_status_bar_zoom_2().IsChecked(status_mask(st_bar, STATUS_BAR::ZOOM));
	}

	// 列挙型を OR 演算する.
	STATUS_BAR MainPage::status_bar_or(const STATUS_BAR a, const STATUS_BAR b) noexcept
	{
		return static_cast<STATUS_BAR>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	// データリーダーからステータスバーの状態を読み込む.
	//void MainPage::status_bar_read(DataReader const& dt_reader)
	//{
	//	m_status_bar = static_cast<STATUS_BAR>(dt_reader.ReadUInt32());
	//}

	// ポインターの位置をステータスバーに格納する.
	void MainPage::status_bar_set_curs(void)
	{
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
		const double px = m_sheet_min.x;
		const double py = m_sheet_min.y;
		const double ps = m_sheet_main.m_sheet_scale;
		const float fx = static_cast<FLOAT>((wx - bx - tx) / ps + sx + px);
		const float fy = static_cast<FLOAT>((wy - by - ty) / ps + sy + py);
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, fx, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
		tk_status_bar_pos_x().Text(winrt::hstring{ L"X:" } + buf);
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, fy, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
		tk_status_bar_pos_y().Text(winrt::hstring{ L"Y:" } + buf);
//swprintf_s(buf, L"%d", static_cast<uint32_t>(m_list_shapes.size()));
//tk_status_bar_cnt().Text(winrt::hstring{ L"c:" } + buf);
	}

	// 作図ツールをステータスバーに格納する.
	void MainPage::status_bar_set_draw(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		const auto t_draw = m_tool_draw;
		winrt::hstring data;
		if (t_draw == DRAW_TOOL::BEZI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_bezi")));
		}
		else if (t_draw == DRAW_TOOL::ELLI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_elli")));
		}
		else if (t_draw == DRAW_TOOL::LINE) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_line")));
		}
		else if (t_draw == DRAW_TOOL::POLY) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_tri")));
		}
		else if (t_draw == DRAW_TOOL::RECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rect")));
		}
		else if (t_draw == DRAW_TOOL::RRECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rrect")));
		}
		else if (t_draw == DRAW_TOOL::RULER) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_ruler")));
		}
		else if (t_draw == DRAW_TOOL::SELECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_select")));
		}
		else if (t_draw == DRAW_TOOL::TEXT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_text")));
		}
		else {
			throw winrt::hresult_invalid_argument();
		}
		pi_draw().Data(nullptr);
		pi_draw().Data(Summary::Data(data));
	}

	// 方眼の大きさをステータスバーに格納する.
	void MainPage::status_bar_set_grid(void)
	{
		wchar_t buf[32];
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, g_base + 1.0f, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
		tk_status_bar_grid().Text(winrt::hstring{ L"G:" } +buf);
	}

	// 用紙の大きさをステータスバーに格納する.
	void MainPage::status_bar_set_sheet(void)
	{
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, m_sheet_main.m_sheet_size.width, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
		tk_status_bar_width().Text(winrt::hstring{ L"W:" } + buf);
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, m_sheet_main.m_sheet_size.height, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
		tk_status_bar_height().Text(winrt::hstring{ L"H:" } + buf);
	}

	// 単位をステータスバーに格納する.
	void MainPage::status_bar_set_unit(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		const auto unit = m_len_unit;
		winrt::hstring unit_name{};
		if (unit == LEN_UNIT::GRID) {
			//unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_unit_grid/Text");
			unit_name = L"g";
		}
		else if (unit == LEN_UNIT::INCH) {
			//unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_unit_inch/Text");
			unit_name = L"\u33CC";
		}
		else if (unit == LEN_UNIT::MILLI) {
			//unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_unit_milli/Text");
			unit_name = L"\u339C";
		}
		else if (unit == LEN_UNIT::PIXEL) {
			//unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_unit_pixel/Text");
			unit_name = L"px";
		}
		else if (unit == LEN_UNIT::POINT) {
			//unit_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_len_unit_point/Text");
			unit_name = L"pt";
		}
		else {
			return;
		}
		tk_status_bar_unit().Text(winrt::hstring{ L"U:" } + unit_name);
	}

	// 拡大率をステータスバーに格納する.
	void MainPage::status_bar_set_zoom(void)
	{
		wchar_t buf[32];
		swprintf_s(buf, 31, FMT_ZOOM, m_sheet_main.m_sheet_scale * 100.0);
		tk_status_bar_zoom().Text(winrt::hstring{ L"Z:" } + buf);
	}

	// ステータスバーの表示を設定する.
	void MainPage::status_bar_visibility(void)
	{
		tk_status_bar_pos_x().Visibility(status_mask(m_status_bar, STATUS_BAR::CURS) ? UI_VISIBLE : UI_COLLAPSED);
		tk_status_bar_pos_y().Visibility(status_mask(m_status_bar, STATUS_BAR::CURS) ? UI_VISIBLE : UI_COLLAPSED);
		tk_status_bar_grid().Visibility(status_mask(m_status_bar, STATUS_BAR::GRID) ? UI_VISIBLE : UI_COLLAPSED);
		tk_status_bar_width().Visibility(status_mask(m_status_bar, STATUS_BAR::SHEET) ? UI_VISIBLE : UI_COLLAPSED);
		tk_status_bar_height().Visibility(status_mask(m_status_bar, STATUS_BAR::SHEET) ? UI_VISIBLE : UI_COLLAPSED);
		sp_status_bar_panel_draw().Visibility(status_mask(m_status_bar, STATUS_BAR::DRAW) ? UI_VISIBLE : UI_COLLAPSED);
		tk_status_bar_unit().Visibility(status_mask(m_status_bar, STATUS_BAR::UNIT) ? UI_VISIBLE : UI_COLLAPSED);
		tk_status_bar_zoom().Visibility(status_mask(m_status_bar, STATUS_BAR::ZOOM) ? UI_VISIBLE : UI_COLLAPSED);
		sp_status_bar_panel().Visibility(m_status_bar != static_cast<STATUS_BAR>(0) ? UI_VISIBLE : UI_COLLAPSED);
	}

	// データライターにステータスバーの状態を書き込む.
	//void MainPage::status_bar_write(DataWriter const& dt_writer)
	//{
	//	dt_writer.WriteUInt32(static_cast<uint32_t>(m_status_bar));
	//}

}