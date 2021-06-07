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
	// AND 演算する.
	static STBAR_FLAG stbar_and(const STBAR_FLAG a, const STBAR_FLAG b) noexcept;
	// ビットマスクする.
	static bool stbar_mask(const STBAR_FLAG a, const STBAR_FLAG b) noexcept;
	// NOT 演算する.
	static STBAR_FLAG stbar_not(const STBAR_FLAG a) noexcept;
	// ステータスバーの項目の表示を設定する.
	static void stbar_visiblity(const bool check, FrameworkElement const& f_elem) noexcept;

	// 列挙型を AND 演算する.
	static STBAR_FLAG stbar_and(const STBAR_FLAG a, const STBAR_FLAG b) noexcept
	{
		return static_cast<STBAR_FLAG>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	// 列挙型をビットマスクする.
	static bool stbar_mask(const STBAR_FLAG a, const STBAR_FLAG b) noexcept
	{
		return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);
	}

	// 列挙型を NOT 演算する.
	static STBAR_FLAG stbar_not(const STBAR_FLAG a) noexcept
	{
		return static_cast<STBAR_FLAG>(~static_cast<uint32_t>(a));
	}

	// ステータスバーの項目の表示を設定する.
	static void stbar_visiblity(const bool check, FrameworkElement const& f_elem) noexcept
	{
		if (f_elem.Visibility() == (check ? UI_COLLAPSED : UI_VISIBLE)) {
			f_elem.Visibility(check ? UI_VISIBLE : UI_COLLAPSED);
		}
	}

	// 用紙メニューの「ステータスバー」が選択された.
	void MainPage::stbar_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		using winrt::Windows::UI::Xaml::Controls::ToggleMenuFlyoutItem;
		STBAR_FLAG stbar;	// ステータスバーの状態
		bool check;

		if (sender == tmfi_stbar_curs()) {
			stbar = STBAR_FLAG::CURS;
			check = tmfi_stbar_curs().IsChecked();
			tmfi_stbar_curs_2().IsChecked(tmfi_stbar_curs().IsChecked());
			stbar_set_curs();
			stbar_visiblity(check, tk_stbar_pos_x());
			stbar_visiblity(check, tk_stbar_pos_y());
		}
		else if (sender == tmfi_stbar_curs_2()) {
			stbar = STBAR_FLAG::CURS;
			//const ToggleMenuFlyoutItem& m_item = unbox_value<ToggleMenuFlyoutItem>(sender);
			check = tmfi_stbar_curs_2().IsChecked();
			tmfi_stbar_curs().IsChecked(check);
			stbar_set_curs();
			stbar_visiblity(check, tk_stbar_pos_x());
			stbar_visiblity(check, tk_stbar_pos_y());
		}
		else if (sender == tmfi_stbar_grid()) {
			stbar = STBAR_FLAG::GRID;
			check = tmfi_stbar_grid().IsChecked();
			tmfi_stbar_grid_2().IsChecked(check);
			stbar_set_grid();
			stbar_visiblity(check, tk_stbar_grid());
		}
		else if (sender == tmfi_stbar_grid_2()) {
			stbar = STBAR_FLAG::GRID;
			check = tmfi_stbar_grid_2().IsChecked();
			tmfi_stbar_grid().IsChecked(check);
			stbar_set_grid();
			stbar_visiblity(check, tk_stbar_grid());
		}
		else if (sender == tmfi_stbar_sheet()) {
			stbar = STBAR_FLAG::SHEET;
			check = tmfi_stbar_sheet().IsChecked();
			tmfi_stbar_sheet_2().IsChecked(check);
			stbar_set_sheet();
			stbar_visiblity(check, tk_stbar_width());
			stbar_visiblity(check, tk_stbar_height());
		}
		else if (sender == tmfi_stbar_sheet_2()) {
			stbar = STBAR_FLAG::SHEET;
			check = tmfi_stbar_sheet_2().IsChecked();
			tmfi_stbar_sheet().IsChecked(check);
			stbar_set_sheet();
			stbar_visiblity(check, tk_stbar_width());
			stbar_visiblity(check, tk_stbar_height());
		}
		else if (sender == tmfi_stbar_zoom()) {
			stbar = STBAR_FLAG::ZOOM;
			check = tmfi_stbar_zoom().IsChecked();
			tmfi_stbar_zoom_2().IsChecked(check);
			stbar_set_zoom();
			stbar_visiblity(check, tk_stbar_zoom());
		}
		else if (sender == tmfi_stbar_zoom_2()) {
			stbar = STBAR_FLAG::ZOOM;
			check = tmfi_stbar_zoom_2().IsChecked();
			tmfi_stbar_zoom().IsChecked(check);
			stbar_set_zoom();
			stbar_visiblity(check, tk_stbar_zoom());
		}
		else if (sender == tmfi_stbar_draw()) {
			stbar = STBAR_FLAG::DRAW;
			check = tmfi_stbar_draw().IsChecked();
			tmfi_stbar_draw_2().IsChecked(check);
			stbar_set_draw();
			stbar_visiblity(check, sp_stbar_draw());
		}
		else if (sender == tmfi_stbar_draw_2()) {
			stbar = STBAR_FLAG::DRAW;
			check = tmfi_stbar_draw_2().IsChecked();
			tmfi_stbar_draw().IsChecked(check);
			stbar_set_draw();
			stbar_visiblity(check, sp_stbar_draw());
		}
		else if (sender == tmfi_stbar_unit()) {
			stbar = STBAR_FLAG::UNIT;
			check = tmfi_stbar_unit().IsChecked();
			tmfi_stbar_unit_2().IsChecked(check);
			stbar_set_unit();
			stbar_visiblity(check, tk_stbar_unit());
		}
		else if (sender == tmfi_stbar_unit_2()) {
			stbar = STBAR_FLAG::UNIT;
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
		stbar_visiblity(m_status_bar != static_cast<STBAR_FLAG>(0), sp_stbar());
	}

	// ステータスバーのメニュー項目に印をつける.
	void MainPage::stbar_is_checked(const STBAR_FLAG st_bar)
	{
		tmfi_stbar_curs().IsChecked(stbar_mask(st_bar, STBAR_FLAG::CURS));
		tmfi_stbar_grid().IsChecked(stbar_mask(st_bar, STBAR_FLAG::GRID));
		tmfi_stbar_sheet().IsChecked(stbar_mask(st_bar, STBAR_FLAG::SHEET));
		tmfi_stbar_draw().IsChecked(stbar_mask(st_bar, STBAR_FLAG::DRAW));
		tmfi_stbar_unit().IsChecked(stbar_mask(st_bar, STBAR_FLAG::UNIT));
		tmfi_stbar_zoom().IsChecked(stbar_mask(st_bar, STBAR_FLAG::ZOOM));
		tmfi_stbar_curs_2().IsChecked(stbar_mask(st_bar, STBAR_FLAG::CURS));
		tmfi_stbar_grid_2().IsChecked(stbar_mask(st_bar, STBAR_FLAG::GRID));
		tmfi_stbar_sheet_2().IsChecked(stbar_mask(st_bar, STBAR_FLAG::SHEET));
		tmfi_stbar_draw_2().IsChecked(stbar_mask(st_bar, STBAR_FLAG::DRAW));
		tmfi_stbar_unit_2().IsChecked(stbar_mask(st_bar, STBAR_FLAG::UNIT));
		tmfi_stbar_zoom_2().IsChecked(stbar_mask(st_bar, STBAR_FLAG::ZOOM));
	}

	// 列挙型を OR 演算する.
	STBAR_FLAG MainPage::stbar_or(const STBAR_FLAG a, const STBAR_FLAG b) noexcept
	{
		return static_cast<STBAR_FLAG>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	// ステータスバーの状態をデータリーダーから読み込む.
	void MainPage::stbar_read(DataReader const& dt_reader)
	{
		m_status_bar = static_cast<STBAR_FLAG>(dt_reader.ReadUInt32());
		status_bar(m_status_bar);
	}

	// ポインターの位置をステータスバーに格納する.
	void MainPage::stbar_set_curs(void)
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
		const double px = sheet_min().x;
		const double py = sheet_min().y;
		const double ps = m_sheet_main.m_sheet_scale;
		const float fx = static_cast<FLOAT>((wx - bx - tx) / ps + sx + px);
		const float fy = static_cast<FLOAT>((wy - by - ty) / ps + sy + py);
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), fx, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
		tk_stbar_pos_x().Text(winrt::hstring{ L"X:" } + buf);
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), fy, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
		tk_stbar_pos_y().Text(winrt::hstring{ L"Y:" } + buf);
//swprintf_s(buf, L"%d", static_cast<uint32_t>(m_list_shapes.size()));
//tk_stbar_cnt().Text(winrt::hstring{ L"c:" } + buf);
	}

	// 方眼の大きさをステータスバーに格納する.
	void MainPage::stbar_set_grid(void)
	{
		wchar_t buf[32];
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), g_base + 1.0f, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
		tk_stbar_grid().Text(winrt::hstring{ L"G:" } +buf);
	}

	// 用紙の大きさをステータスバーに格納する.
	void MainPage::stbar_set_sheet(void)
	{
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), m_sheet_main.m_sheet_size.width, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
		tk_stbar_width().Text(winrt::hstring{ L"W:" } + buf);
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), m_sheet_main.m_sheet_size.height, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
		tk_stbar_height().Text(winrt::hstring{ L"H:" } + buf);
	}

	// 作図ツールをステータスバーに格納する.
	void MainPage::stbar_set_draw(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		const auto t_draw = tool_draw();
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

	// 単位をステータスバーに格納する.
	void MainPage::stbar_set_unit(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		const auto unit = len_unit();
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
		tk_stbar_unit().Text(winrt::hstring{ L"U:" } + unit_name);
	}

	// 拡大率をステータスバーに格納する.
	void MainPage::stbar_set_zoom(void)
	{
		wchar_t buf[32];
		swprintf_s(buf, 31, FMT_ZOOM, m_sheet_main.m_sheet_scale * 100.0);
		tk_stbar_zoom().Text(winrt::hstring{ L"Z:" } + buf);
	}

	// ステータスバーの表示を設定する.
	void MainPage::stbar_visibility(void)
	{
		tk_stbar_pos_x().Visibility(stbar_mask(m_status_bar, STBAR_FLAG::CURS) ? UI_VISIBLE : UI_COLLAPSED);
		tk_stbar_pos_y().Visibility(stbar_mask(m_status_bar, STBAR_FLAG::CURS) ? UI_VISIBLE : UI_COLLAPSED);
		tk_stbar_grid().Visibility(stbar_mask(m_status_bar, STBAR_FLAG::GRID) ? UI_VISIBLE : UI_COLLAPSED);
		tk_stbar_width().Visibility(stbar_mask(m_status_bar, STBAR_FLAG::SHEET) ? UI_VISIBLE : UI_COLLAPSED);
		tk_stbar_height().Visibility(stbar_mask(m_status_bar, STBAR_FLAG::SHEET) ? UI_VISIBLE : UI_COLLAPSED);
		//sp_stbar_curs().Visibility(stbar_mask(m_status_bar, STBAR_FLAG::CURS) ? UI_VISIBLE : UI_COLLAPSED);
		//sp_stbar_grid().Visibility(stbar_mask(m_status_bar, STBAR_FLAG::GRID) ? UI_VISIBLE : UI_COLLAPSED);
		//sp_stbar_sheet().Visibility(stbar_mask(m_status_bar, STBAR_FLAG::SHEET) ? UI_VISIBLE : UI_COLLAPSED);
		sp_stbar_draw().Visibility(stbar_mask(m_status_bar, STBAR_FLAG::DRAW) ? UI_VISIBLE : UI_COLLAPSED);
		tk_stbar_unit().Visibility(stbar_mask(m_status_bar, STBAR_FLAG::UNIT) ? UI_VISIBLE : UI_COLLAPSED);
		tk_stbar_zoom().Visibility(stbar_mask(m_status_bar, STBAR_FLAG::ZOOM) ? UI_VISIBLE : UI_COLLAPSED);
		//sp_stbar_unit().Visibility(stbar_mask(m_status_bar, STBAR_FLAG::UNIT) ? UI_VISIBLE : UI_COLLAPSED);
		//sp_stbar_zoom().Visibility(stbar_mask(m_status_bar, STBAR_FLAG::ZOOM) ? UI_VISIBLE : UI_COLLAPSED);
		sp_stbar().Visibility(m_status_bar != static_cast<STBAR_FLAG>(0) ? UI_VISIBLE : UI_COLLAPSED);
	}

	// ステータスバーの状態をデータライターに書き込む.
	void MainPage::stbar_write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_status_bar));
	}

}