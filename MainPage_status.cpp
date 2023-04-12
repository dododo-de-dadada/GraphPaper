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
	//using winrt::Windows::UI::Core::CoreWindow;
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	// AND 演算する.
	static inline STATUS_BAR status_and(const STATUS_BAR a, const STATUS_BAR b) noexcept;
	// ビットマスクする.
	static inline bool status_mask(const STATUS_BAR a, const STATUS_BAR b) noexcept;
	// NOT 演算する.
	static inline STATUS_BAR status_not(const STATUS_BAR a) noexcept;
	// 列挙型を OR 演算する.
	static inline STATUS_BAR status_or(const STATUS_BAR a, const STATUS_BAR b) noexcept;

	// 列挙型を AND 演算する.
	static inline STATUS_BAR status_and(const STATUS_BAR a, const STATUS_BAR b) noexcept
	{
		return static_cast<STATUS_BAR>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	// 列挙型をビットマスクする.
	static inline bool status_mask(const STATUS_BAR a, const STATUS_BAR b) noexcept
	{
		return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);
	}

	// 列挙型を OR 演算する.
	static inline STATUS_BAR status_or(const STATUS_BAR a, const STATUS_BAR b) noexcept
	{
		return static_cast<STATUS_BAR>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	// 列挙型を NOT 演算する.
	static inline STATUS_BAR status_not(const STATUS_BAR a) noexcept
	{
		return static_cast<STATUS_BAR>(~static_cast<uint32_t>(a));
	}

	// その他メニューの「ステータスバー」が選択された.
	void MainPage::status_bar_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == tmfi_menu_status_bar_pos()) {
			const bool is_checked = tmfi_menu_status_bar_pos().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::POS) : status_and(m_status_bar, status_not(STATUS_BAR::POS));
			tmfi_popup_status_bar_pos().IsChecked(is_checked);
		}
		else if (sender == tmfi_menu_status_bar_grid()) {
			const bool is_checked = tmfi_menu_status_bar_grid().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::GRID) : status_and(m_status_bar, status_not(STATUS_BAR::GRID));
			tmfi_popup_status_bar_grid().IsChecked(is_checked);
			status_bar_set_grid();
		}
		else if (sender == tmfi_menu_status_bar_page()) {
			const bool is_checked = tmfi_menu_status_bar_page().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::PAGE) : status_and(m_status_bar, status_not(STATUS_BAR::PAGE));
			tmfi_popup_status_bar_page().IsChecked(is_checked);
			status_bar_set_page();
		}
		else if (sender == tmfi_menu_status_bar_zoom()) {
			const bool is_checked = tmfi_menu_status_bar_zoom().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::ZOOM) : status_and(m_status_bar, status_not(STATUS_BAR::ZOOM));
			tmfi_popup_status_bar_zoom().IsChecked(is_checked);
			status_bar_set_zoom();
		}
		else if (sender == tmfi_menu_status_bar_draw()) {
			const bool is_checked = tmfi_menu_status_bar_draw().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::DRAW) : status_and(m_status_bar, status_not(STATUS_BAR::DRAW));
			tmfi_popup_status_bar_draw().IsChecked(is_checked);
			status_bar_set_draw();
		}
		else if (sender == tmfi_menu_status_bar_unit()) {
			const bool is_checked = tmfi_menu_status_bar_unit().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::UNIT) : status_and(m_status_bar, status_not(STATUS_BAR::UNIT));
			tmfi_popup_status_bar_unit().IsChecked(is_checked);
			status_bar_set_unit();
		}
		else if (sender == tmfi_popup_status_bar_pos()) {
			const bool is_checked = tmfi_popup_status_bar_pos().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::POS) : status_and(m_status_bar, status_not(STATUS_BAR::POS));
			tmfi_menu_status_bar_pos().IsChecked(is_checked);
		}
		else if (sender == tmfi_popup_status_bar_grid()) {
			const bool is_checked = tmfi_popup_status_bar_grid().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::GRID) : status_and(m_status_bar, status_not(STATUS_BAR::GRID));
			tmfi_menu_status_bar_grid().IsChecked(is_checked);
			status_bar_set_grid();
		}
		else if (sender == tmfi_popup_status_bar_page()) {
			const bool is_checked = tmfi_popup_status_bar_page().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::PAGE) : status_and(m_status_bar, status_not(STATUS_BAR::PAGE));
			tmfi_menu_status_bar_page().IsChecked(is_checked);
			status_bar_set_page();
		}
		else if (sender == tmfi_popup_status_bar_zoom()) {
			const bool is_checked = tmfi_popup_status_bar_zoom().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::ZOOM) : status_and(m_status_bar, status_not(STATUS_BAR::ZOOM));
			tmfi_menu_status_bar_zoom().IsChecked(is_checked);
			status_bar_set_zoom();
		}
		else if (sender == tmfi_popup_status_bar_draw()) {
			const bool is_checked = tmfi_popup_status_bar_draw().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::DRAW) : status_and(m_status_bar, status_not(STATUS_BAR::DRAW));
			tmfi_menu_status_bar_draw().IsChecked(is_checked);
			status_bar_set_draw();
		}
		else if (sender == tmfi_popup_status_bar_unit()) {
			const bool is_checked = tmfi_popup_status_bar_unit().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::UNIT) : status_and(m_status_bar, status_not(STATUS_BAR::UNIT));
			tmfi_menu_status_bar_unit().IsChecked(is_checked);
			status_bar_set_unit();
		}
		else {
			auto _{ winrt::hresult_not_implemented() };
		}
		if (m_status_bar == static_cast<STATUS_BAR>(0)) {
			if (sp_status_bar_panel().Visibility() == Visibility::Visible) {
				sp_status_bar_panel().Visibility(Visibility::Collapsed);
			}
		}
		else {
			if (sp_status_bar_panel().Visibility() == Visibility::Collapsed) {
				sp_status_bar_panel().Visibility(Visibility::Visible);
			}
		}
		status_bar_set_pos();
	}

	// ステータスバーのメニュー項目に印をつける.
	// s_bar	ステータスバーの状態
	void MainPage::status_bar_is_checked(const STATUS_BAR s_bar)
	{
		tmfi_menu_status_bar_pos().IsChecked(status_mask(s_bar, STATUS_BAR::POS));
		tmfi_menu_status_bar_grid().IsChecked(status_mask(s_bar, STATUS_BAR::GRID));
		tmfi_menu_status_bar_page().IsChecked(status_mask(s_bar, STATUS_BAR::PAGE));
		tmfi_menu_status_bar_draw().IsChecked(status_mask(s_bar, STATUS_BAR::DRAW));
		tmfi_menu_status_bar_unit().IsChecked(status_mask(s_bar, STATUS_BAR::UNIT));
		tmfi_menu_status_bar_zoom().IsChecked(status_mask(s_bar, STATUS_BAR::ZOOM));

		tmfi_popup_status_bar_pos().IsChecked(status_mask(s_bar, STATUS_BAR::POS));
		tmfi_popup_status_bar_grid().IsChecked(status_mask(s_bar, STATUS_BAR::GRID));
		tmfi_popup_status_bar_page().IsChecked(status_mask(s_bar, STATUS_BAR::PAGE));
		tmfi_popup_status_bar_draw().IsChecked(status_mask(s_bar, STATUS_BAR::DRAW));
		tmfi_popup_status_bar_unit().IsChecked(status_mask(s_bar, STATUS_BAR::UNIT));
		tmfi_popup_status_bar_zoom().IsChecked(status_mask(s_bar, STATUS_BAR::ZOOM));
	}

	// ポインターの位置をステータスバーに格納する.
	void MainPage::status_bar_set_pos(void)
	{
		if (status_and(m_status_bar, STATUS_BAR::POS) == STATUS_BAR::POS) {
			const auto wp = CoreWindow::GetForCurrentThread().PointerPosition();
			const auto wb = CoreWindow::GetForCurrentThread().Bounds();
			const auto tr = scp_main_panel().TransformToVisual(nullptr);
			const auto tp = tr.TransformPoint({ 0.0f, 0.0f });
			const double sx = sb_horz().Value();
			const double sy = sb_vert().Value();
			const double wx = wp.X;
			const double tx = tp.X;
			const double bx = wb.X;
			const double wy = wp.Y;
			const double ty = tp.Y;
			const double by = wb.Y;
			const double px = m_main_bbox_lt.x;
			const double py = m_main_bbox_lt.y;
			const double ps = m_main_scale;
			const float fx = static_cast<FLOAT>((wx - bx - tx) / ps + sx + px);
			const float fy = static_cast<FLOAT>((wy - by - ty) / ps + sy + py);
			const float g_len = m_main_page.m_grid_base + 1.0f;
			wchar_t buf_x[32];
			wchar_t buf_y[32];
			conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, fx, m_main_d2d.m_logical_dpi, g_len, buf_x);
			conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, fy, m_main_d2d.m_logical_dpi, g_len, buf_y);
			tk_status_bar_pos_x().Text(buf_x);
			tk_status_bar_pos_y().Text(buf_y);
			if (sp_status_bar_pos().Visibility() != Visibility::Visible) {
				sp_status_bar_pos().Visibility(Visibility::Visible);
			}
		}
		else {
			if (sp_status_bar_pos().Visibility() != Visibility::Collapsed) {
				sp_status_bar_pos().Visibility(Visibility::Collapsed);
			}
		}
	}

	// 作図ツールをステータスバーに格納する.
	void MainPage::status_bar_set_draw(void)
	{
		if (status_and(m_status_bar, STATUS_BAR::DRAW) == STATUS_BAR::DRAW) {
			winrt::hstring data;
			if (m_drawing_tool == DRAWING_TOOL::BEZIER) {
				data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_bezier")));
				r_eyedropper().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::ELLIPSE) {
				data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_ellipse")));
				r_eyedropper().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::LINE) {
				data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_line")));
				r_eyedropper().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::POLY) {
				data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_polygon")));
				r_eyedropper().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::RECT) {
				data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rect")));
				r_eyedropper().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::RRECT) {
				data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rrect")));
				r_eyedropper().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::RULER) {
				data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_ruler")));
				r_eyedropper().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::SELECT) {
				data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_select")));
				r_eyedropper().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::TEXT) {
				data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_text")));
				r_eyedropper().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::ARC) {
				data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_arc")));
				r_eyedropper().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
				data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_eyedropper")));
				winrt::Windows::UI::Color c{
					static_cast<uint8_t>(conv_color_comp(m_eyedropper_color.a)),
					static_cast<uint8_t>(conv_color_comp(m_eyedropper_color.r)),
					static_cast<uint8_t>(conv_color_comp(m_eyedropper_color.g)),
					static_cast<uint8_t>(conv_color_comp(m_eyedropper_color.b))
				};
				r_eyedropper().Fill(winrt::Windows::UI::Xaml::Media::SolidColorBrush{ c });
				r_eyedropper().Visibility(Visibility::Visible);
			}
			else {
#ifdef _DEBUG
				__debugbreak();
#endif // _DEBUG

				throw winrt::hresult_invalid_argument();
			}
			pi_tool_icon().Data(nullptr);
			pi_tool_icon().Data(Summary::Geom(data));
			if (sp_status_bar_draw().Visibility() != Visibility::Visible) {
				sp_status_bar_draw().Visibility(Visibility::Visible);
			}
		}
		else {
			if (sp_status_bar_draw().Visibility() != Visibility::Collapsed) {
				sp_status_bar_draw().Visibility(Visibility::Collapsed);
			}
		}
	}

	// 方眼の大きさをステータスバーに格納する.
	void MainPage::status_bar_set_grid(void)
	{
		if (status_and(m_status_bar, STATUS_BAR::GRID) == STATUS_BAR::GRID) {
			const float g_len = m_main_page.m_grid_base + 1.0f;
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(
				m_len_unit, g_len, m_main_d2d.m_logical_dpi, g_len, buf);
			tk_status_bar_grid().Text(buf);
			if (sp_status_bar_grid().Visibility() != Visibility::Visible) {
				sp_status_bar_grid().Visibility(Visibility::Visible);
			}
		}
		else {
			if (sp_status_bar_grid().Visibility() != Visibility::Collapsed) {
				sp_status_bar_grid().Visibility(Visibility::Collapsed);
			}
		}
	}

	// ページの大きさをステータスバーに格納する.
	void MainPage::status_bar_set_page(void)
	{
		if (status_and(m_status_bar, STATUS_BAR::PAGE) == STATUS_BAR::PAGE) {
			const float g_len = m_main_page.m_grid_base + 1.0f;
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(
				m_len_unit, m_main_page.m_page_size.width, m_main_d2d.m_logical_dpi, g_len, buf);
			tk_status_bar_page_w().Text(buf);
			conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(
				m_len_unit, m_main_page.m_page_size.height, m_main_d2d.m_logical_dpi, g_len, buf);
			tk_status_bar_page_h().Text(buf);
			if (sp_status_bar_page().Visibility() != Visibility::Visible) {
				sp_status_bar_page().Visibility(Visibility::Visible);
			}
		}
		else {
			if (sp_status_bar_page().Visibility() != Visibility::Collapsed) {
				sp_status_bar_page().Visibility(Visibility::Collapsed);
			}
		}
	}

	// 単位をステータスバーに格納する.
	void MainPage::status_bar_set_unit(void)
	{
		if (status_and(m_status_bar, STATUS_BAR::UNIT) == STATUS_BAR::UNIT) {
			if (m_len_unit == LEN_UNIT::GRID) {
				tk_status_bar_unit().Text(L"grid");
			}
			else if (m_len_unit == LEN_UNIT::INCH) {
				tk_status_bar_unit().Text(L"\u33CC");
			}
			else if (m_len_unit == LEN_UNIT::MILLI) {
				tk_status_bar_unit().Text(L"\u339C");
			}
			else if (m_len_unit == LEN_UNIT::PIXEL) {
				tk_status_bar_unit().Text(L"px");
			}
			else if (m_len_unit == LEN_UNIT::POINT) {
				tk_status_bar_unit().Text(L"pt");
			}
			if (sp_status_bar_unit().Visibility() != Visibility::Visible) {
				sp_status_bar_unit().Visibility(Visibility::Visible);
			}
		}
		else {
			if (sp_status_bar_unit().Visibility() != Visibility::Collapsed) {
				sp_status_bar_unit().Visibility(Visibility::Collapsed);
			}
		}
	}

	// 拡大率をステータスバーに格納する.
	void MainPage::status_bar_set_zoom(void)
	{
		if (status_and(m_status_bar, STATUS_BAR::ZOOM) == STATUS_BAR::ZOOM) {
			constexpr auto FMT_ZOOM = L"%.f%%";	// 倍率の書式
			wchar_t buf[32];
			swprintf_s(buf, 31, FMT_ZOOM, m_main_scale * 100.0f);
			//swprintf_s(buf, 31, FMT_ZOOM, m_main_page.m_page_scale * 100.0);
			tk_status_bar_zoom().Text(buf);
			if (sp_status_bar_zoom().Visibility() != Visibility::Visible) {
				sp_status_bar_zoom().Visibility(Visibility::Visible);
			}
		}
		else {
			if (sp_status_bar_zoom().Visibility() != Visibility::Collapsed) {
				sp_status_bar_zoom().Visibility(Visibility::Collapsed);
			}
		}
	}

}