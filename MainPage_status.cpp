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
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Core::CoreWindow;
	using winrt::Windows::UI::Xaml::RoutedEventArgs;

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

	// その他メニューの「ステータスバー」が選択された.
	void MainPage::status_bar_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		//using winrt::Windows::UI::Xaml::Controls::ToggleMenuFlyoutItem;
		STATUS_BAR s_bar;	// ステータスバーの状態
		bool check;

		if (sender == tmfi_status_bar_curs()) {
			s_bar = STATUS_BAR::CURS;
			check = tmfi_status_bar_curs().IsChecked();
			tmfi_status_bar_curs_2().IsChecked(tmfi_status_bar_curs().IsChecked());
			status_set_curs();
			status_visiblity(check, tk_misc_status_bar_pos_x());
			status_visiblity(check, tk_misc_status_bar_pos_y());
		}
		else if (sender == tmfi_status_bar_curs_2()) {
			s_bar = STATUS_BAR::CURS;
			check = tmfi_status_bar_curs_2().IsChecked();
			tmfi_status_bar_curs().IsChecked(check);
			status_set_curs();
			status_visiblity(check, tk_misc_status_bar_pos_x());
			status_visiblity(check, tk_misc_status_bar_pos_y());
		}
		else if (sender == tmfi_status_bar_grid()) {
			s_bar = STATUS_BAR::GRID;
			check = tmfi_status_bar_grid().IsChecked();
			tmfi_status_bar_grid_2().IsChecked(check);
			status_set_grid();
			status_visiblity(check, tk_misc_status_grid());
		}
		else if (sender == tmfi_status_bar_grid_2()) {
			s_bar = STATUS_BAR::GRID;
			check = tmfi_status_bar_grid_2().IsChecked();
			tmfi_status_bar_grid().IsChecked(check);
			status_set_grid();
			status_visiblity(check, tk_misc_status_grid());
		}
		else if (sender == tmfi_status_bar_sheet()) {
			s_bar = STATUS_BAR::SHEET;
			check = tmfi_status_bar_sheet().IsChecked();
			tmfi_status_bar_sheet_2().IsChecked(check);
			status_set_sheet();
			status_visiblity(check, tk_misc_status_bar_width());
			status_visiblity(check, tk_misc_status_bar_height());
		}
		else if (sender == tmfi_status_bar_sheet_2()) {
			s_bar = STATUS_BAR::SHEET;
			check = tmfi_status_bar_sheet_2().IsChecked();
			tmfi_status_bar_sheet().IsChecked(check);
			status_set_sheet();
			status_visiblity(check, tk_misc_status_bar_width());
			status_visiblity(check, tk_misc_status_bar_height());
		}
		else if (sender == tmfi_status_bar_zoom()) {
			s_bar = STATUS_BAR::ZOOM;
			check = tmfi_status_bar_zoom().IsChecked();
			tmfi_status_bar_zoom_2().IsChecked(check);
			status_set_zoom();
			status_visiblity(check, tk_misc_status_zoom());
		}
		else if (sender == tmfi_status_bar_zoom_2()) {
			s_bar = STATUS_BAR::ZOOM;
			check = tmfi_status_bar_zoom_2().IsChecked();
			tmfi_status_bar_zoom().IsChecked(check);
			status_set_zoom();
			status_visiblity(check, tk_misc_status_zoom());
		}
		else if (sender == tmfi_status_bar_draw()) {
			s_bar = STATUS_BAR::DRAW;
			check = tmfi_status_bar_draw().IsChecked();
			tmfi_status_bar_draw_2().IsChecked(check);
			status_set_draw();
			status_visiblity(check, sp_misc_status_bar_panel_draw());
		}
		else if (sender == tmfi_status_bar_draw_2()) {
			s_bar = STATUS_BAR::DRAW;
			check = tmfi_status_bar_draw_2().IsChecked();
			tmfi_status_bar_draw().IsChecked(check);
			status_set_draw();
			status_visiblity(check, sp_misc_status_bar_panel_draw());
		}
		else if (sender == tmfi_status_bar_unit()) {
			s_bar = STATUS_BAR::UNIT;
			check = tmfi_status_bar_unit().IsChecked();
			tmfi_status_bar_unit_2().IsChecked(check);
			status_set_unit();
			status_visiblity(check, tk_misc_status_unit());
		}
		else if (sender == tmfi_status_bar_unit_2()) {
			s_bar = STATUS_BAR::UNIT;
			check = tmfi_status_bar_unit_2().IsChecked();
			tmfi_status_bar_unit().IsChecked(check);
			status_set_unit();
			status_visiblity(check, tk_misc_status_unit());
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
		status_visiblity(m_status_bar != static_cast<STATUS_BAR>(0), sp_misc_status_bar_panel());
	}

	// ステータスバーのメニュー項目に印をつける.
	// s_bar	ステータスバーの状態
	void MainPage::status_bar_is_checked(const STATUS_BAR s_bar)
	{
		tmfi_status_bar_curs().IsChecked(status_mask(s_bar, STATUS_BAR::CURS));
		tmfi_status_bar_grid().IsChecked(status_mask(s_bar, STATUS_BAR::GRID));
		tmfi_status_bar_sheet().IsChecked(status_mask(s_bar, STATUS_BAR::SHEET));
		tmfi_status_bar_draw().IsChecked(status_mask(s_bar, STATUS_BAR::DRAW));
		tmfi_status_bar_unit().IsChecked(status_mask(s_bar, STATUS_BAR::UNIT));
		tmfi_status_bar_zoom().IsChecked(status_mask(s_bar, STATUS_BAR::ZOOM));
		tmfi_status_bar_curs_2().IsChecked(status_mask(s_bar, STATUS_BAR::CURS));
		tmfi_status_bar_grid_2().IsChecked(status_mask(s_bar, STATUS_BAR::GRID));
		tmfi_status_bar_sheet_2().IsChecked(status_mask(s_bar, STATUS_BAR::SHEET));
		tmfi_status_bar_draw_2().IsChecked(status_mask(s_bar, STATUS_BAR::DRAW));
		tmfi_status_bar_unit_2().IsChecked(status_mask(s_bar, STATUS_BAR::UNIT));
		tmfi_status_bar_zoom_2().IsChecked(status_mask(s_bar, STATUS_BAR::ZOOM));
	}

	// 列挙型を OR 演算する.
	STATUS_BAR MainPage::status_or(const STATUS_BAR a, const STATUS_BAR b) noexcept
	{
		return static_cast<STATUS_BAR>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	// ポインターの位置をステータスバーに格納する.
	void MainPage::status_set_curs(void)
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
		const double px = m_main_min.x;
		const double py = m_main_min.y;
		const double ps = m_main_sheet.m_sheet_scale;
		const float fx = static_cast<FLOAT>((wx - bx - tx) / ps + sx + px);
		const float fy = static_cast<FLOAT>((wy - by - ty) / ps + sy + py);
		const float g_len = m_main_sheet.m_grid_base + 1.0f;
		wchar_t buf_x[32];
		wchar_t buf_y[32];
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, fx, m_main_d2d.m_logical_dpi, g_len, buf_x);
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, fy, m_main_d2d.m_logical_dpi, g_len, buf_y);
		tk_misc_status_bar_pos_x().Text(winrt::hstring{ L"X:" } + buf_x);
		tk_misc_status_bar_pos_y().Text(winrt::hstring{ L"Y:" } + buf_y);
	}

	// 作図ツールをステータスバーに格納する.
	void MainPage::status_set_draw(void)
	{
		winrt::hstring data;
		if (m_drawing_tool == DRAWING_TOOL::BEZI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_bezi")));
		}
		else if (m_drawing_tool == DRAWING_TOOL::ELLI) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_elli")));
		}
		else if (m_drawing_tool == DRAWING_TOOL::LINE) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_line")));
		}
		else if (m_drawing_tool == DRAWING_TOOL::POLY) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_tri")));
		}
		else if (m_drawing_tool == DRAWING_TOOL::RECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rect")));
		}
		else if (m_drawing_tool == DRAWING_TOOL::RRECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rrect")));
		}
		else if (m_drawing_tool == DRAWING_TOOL::RULER) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_ruler")));
		}
		else if (m_drawing_tool == DRAWING_TOOL::SELECT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_select")));
		}
		else if (m_drawing_tool == DRAWING_TOOL::TEXT) {
			data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_text")));
		}
		else {
			throw winrt::hresult_invalid_argument();
		}
		pi_draw().Data(nullptr);
		pi_draw().Data(Summary::Data(data));
	}

	// 方眼の大きさをステータスバーに格納する.
	void MainPage::status_set_grid(void)
	{
		const float g_len = m_main_sheet.m_grid_base + 1.0f;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, g_len, m_main_d2d.m_logical_dpi, g_len, buf);
		tk_misc_status_grid().Text(winrt::hstring{ L"G:" } +buf);
	}

	// 用紙の大きさをステータスバーに格納する.
	void MainPage::status_set_sheet(void)
	{
		const float g_len = m_main_sheet.m_grid_base + 1.0f;
		wchar_t buf_w[32];
		wchar_t buf_h[32];
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, m_main_sheet.m_sheet_size.width, m_main_d2d.m_logical_dpi, g_len, buf_w);
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, m_main_sheet.m_sheet_size.height, m_main_d2d.m_logical_dpi, g_len, buf_h);
		tk_misc_status_bar_width().Text(winrt::hstring{ L"W:" } + buf_w);
		tk_misc_status_bar_height().Text(winrt::hstring{ L"H:" } + buf_h);
	}

	// 単位をステータスバーに格納する.
	void MainPage::status_set_unit(void)
	{
		//using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		if (m_len_unit == LEN_UNIT::GRID) {
			tk_misc_status_unit().Text(L"U:grid");
		}
		else if (m_len_unit == LEN_UNIT::INCH) {
			tk_misc_status_unit().Text(L"U:\u33CC");
		}
		else if (m_len_unit == LEN_UNIT::MILLI) {
			tk_misc_status_unit().Text(L"U:\u339C");
		}
		else if (m_len_unit == LEN_UNIT::PIXEL) {
			tk_misc_status_unit().Text(L"U:px");
		}
		else if (m_len_unit == LEN_UNIT::POINT) {
			tk_misc_status_unit().Text(L"U:pt");
		}
	}

	// 拡大率をステータスバーに格納する.
	void MainPage::status_set_zoom(void)
	{
		wchar_t buf[32];
		swprintf_s(buf, 31, FMT_ZOOM, m_main_sheet.m_sheet_scale * 100.0);
		tk_misc_status_zoom().Text(winrt::hstring{ L"Z:" } + buf);
	}

	// ステータスバーの表示を設定する.
	void MainPage::status_bar_visibility(void)
	{
		tk_misc_status_bar_pos_x().Visibility(status_mask(m_status_bar, STATUS_BAR::CURS) ? UI_VISIBLE : UI_COLLAPSED);
		tk_misc_status_bar_pos_y().Visibility(status_mask(m_status_bar, STATUS_BAR::CURS) ? UI_VISIBLE : UI_COLLAPSED);
		tk_misc_status_grid().Visibility(status_mask(m_status_bar, STATUS_BAR::GRID) ? UI_VISIBLE : UI_COLLAPSED);
		tk_misc_status_bar_width().Visibility(status_mask(m_status_bar, STATUS_BAR::SHEET) ? UI_VISIBLE : UI_COLLAPSED);
		tk_misc_status_bar_height().Visibility(status_mask(m_status_bar, STATUS_BAR::SHEET) ? UI_VISIBLE : UI_COLLAPSED);
		sp_misc_status_bar_panel_draw().Visibility(status_mask(m_status_bar, STATUS_BAR::DRAW) ? UI_VISIBLE : UI_COLLAPSED);
		tk_misc_status_unit().Visibility(status_mask(m_status_bar, STATUS_BAR::UNIT) ? UI_VISIBLE : UI_COLLAPSED);
		tk_misc_status_zoom().Visibility(status_mask(m_status_bar, STATUS_BAR::ZOOM) ? UI_VISIBLE : UI_COLLAPSED);
		sp_misc_status_bar_panel().Visibility(m_status_bar != static_cast<STATUS_BAR>(0) ? UI_VISIBLE : UI_COLLAPSED);
	}

}