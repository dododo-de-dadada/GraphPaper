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
	using winrt::Windows::UI::Xaml::Visibility;

	// AND 演算する.
	//static inline STATUS_BAR status_and(const STATUS_BAR a, const STATUS_BAR b) noexcept;
	// ビットマスクする.
	static inline bool status_mask(const STATUS_BAR a, const STATUS_BAR b) noexcept;
	// NOT 演算する.
	static inline STATUS_BAR status_not(const STATUS_BAR a) noexcept;
	// 列挙型を OR 演算する.
	static inline STATUS_BAR status_or(const STATUS_BAR a, const STATUS_BAR b) noexcept;

	// 列挙型を AND 演算する.
	//static inline STATUS_BAR status_and(const STATUS_BAR a, const STATUS_BAR b) noexcept
	//{
	//	return static_cast<STATUS_BAR>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	//}

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
		else if (sender == tmfi_menu_status_bar_sheet()) {
			const bool is_checked = tmfi_menu_status_bar_sheet().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::SHEET) : status_and(m_status_bar, status_not(STATUS_BAR::SHEET));
			tmfi_popup_status_bar_sheet().IsChecked(is_checked);
			status_bar_set_sheet();
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
		else if (sender == tmfi_popup_status_bar_sheet()) {
			const bool is_checked = tmfi_popup_status_bar_sheet().IsChecked();
			m_status_bar = is_checked ? status_or(m_status_bar, STATUS_BAR::SHEET) : status_and(m_status_bar, status_not(STATUS_BAR::SHEET));
			tmfi_menu_status_bar_sheet().IsChecked(is_checked);
			status_bar_set_sheet();
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
			throw winrt::hresult_not_implemented();
			return;
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
			const float g_len = m_main_sheet.m_grid_base + 1.0f;
			wchar_t buf[64];
			wchar_t buf_x[32];
			wchar_t buf_y[32];
			conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, fx, m_main_d2d.m_logical_dpi, g_len, buf_x);
			conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, fy, m_main_d2d.m_logical_dpi, g_len, buf_y);
			swprintf_s(buf, L"%s %s", buf_x, buf_y);
			tk_status_bar_pos_x().Text(buf);
			//tk_status_bar_pos_y().Text(buf_y);
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
			if (m_drawing_tool == DRAWING_TOOL::BEZIER) {
				pi_tool_icon().Data(nullptr);
				pi_tool_icon().Data(Summary::Geom(unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_bezier")))));
				r_eyedropper().Visibility(Visibility::Collapsed);
				//tb_map_pointer().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::ELLIPSE) {
				pi_tool_icon().Data(nullptr);
				pi_tool_icon().Data(Summary::Geom(unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_ellipse")))));
				r_eyedropper().Visibility(Visibility::Collapsed);
				//tb_map_pointer().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::LINE) {
				pi_tool_icon().Data(nullptr);
				pi_tool_icon().Data(Summary::Geom(unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_line")))));
				r_eyedropper().Visibility(Visibility::Collapsed);
				//tb_map_pointer().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::POLY) {
				pi_tool_icon().Data(nullptr);
				pi_tool_icon().Data(Summary::Geom(unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_polygon")))));
				r_eyedropper().Visibility(Visibility::Collapsed);
				//tb_map_pointer().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::RECT) {
				pi_tool_icon().Data(nullptr);
				pi_tool_icon().Data(Summary::Geom(unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rect")))));
				r_eyedropper().Visibility(Visibility::Collapsed);
				//tb_map_pointer().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::RRECT) {
				pi_tool_icon().Data(nullptr);
				pi_tool_icon().Data(Summary::Geom(unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_rrect")))));
				r_eyedropper().Visibility(Visibility::Collapsed);
				//tb_map_pointer().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::RULER) {
				pi_tool_icon().Data(nullptr);
				pi_tool_icon().Data(Summary::Geom(unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_ruler")))));
				r_eyedropper().Visibility(Visibility::Collapsed);
				//tb_map_pointer().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::SELECT) {
				pi_tool_icon().Data(nullptr);
				pi_tool_icon().Data(Summary::Geom(unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_select")))));
				r_eyedropper().Visibility(Visibility::Collapsed);
				//tb_map_pointer().Visibility(Visibility::Visible);
			}
			else if (m_drawing_tool == DRAWING_TOOL::TEXT) {
				pi_tool_icon().Data(nullptr);
				pi_tool_icon().Data(Summary::Geom(unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_text")))));
				r_eyedropper().Visibility(Visibility::Collapsed);
				//tb_map_pointer().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::ARC) {
				pi_tool_icon().Data(nullptr);
				pi_tool_icon().Data(Summary::Geom(unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_arc")))));
				r_eyedropper().Visibility(Visibility::Collapsed);
				//tb_map_pointer().Visibility(Visibility::Collapsed);
			}
			else if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
				pi_tool_icon().Data(nullptr);
				pi_tool_icon().Data(Summary::Geom(unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_eyedropper")))));
				winrt::Windows::UI::Color c{
					static_cast<uint8_t>(conv_color_comp(m_eyedropper_color.a)),
					static_cast<uint8_t>(conv_color_comp(m_eyedropper_color.r)),
					static_cast<uint8_t>(conv_color_comp(m_eyedropper_color.g)),
					static_cast<uint8_t>(conv_color_comp(m_eyedropper_color.b))
				};
				r_eyedropper().Fill(winrt::Windows::UI::Xaml::Media::SolidColorBrush{ c });
				r_eyedropper().Visibility(Visibility::Visible);
				wchar_t r_buf[128];
				wchar_t g_buf[128];
				wchar_t b_buf[128];
				wchar_t a_buf[128];
				wchar_t c_buf[128];
				conv_col_to_str(m_color_code, conv_color_comp(m_eyedropper_color.r), r_buf);
				conv_col_to_str(m_color_code, conv_color_comp(m_eyedropper_color.g), g_buf);
				conv_col_to_str(m_color_code, conv_color_comp(m_eyedropper_color.b), b_buf);
				conv_col_to_str(m_color_code, conv_color_comp(m_eyedropper_color.a), a_buf);
				swprintf_s(c_buf, m_color_code != COLOR_CODE::HEX ? L"%s,%s,%s,%s" : L"#%s%s%s%s", r_buf, g_buf, b_buf, a_buf);
				tb_map_pointer().Text(c_buf);
				//tb_map_pointer().Visibility(Visibility::Visible);
			}
			else if (m_drawing_tool == DRAWING_TOOL::POINTER) {
				pi_tool_icon().Data(nullptr);
				pi_tool_icon().Data(Summary::Geom(unbox_value<winrt::hstring>(Resources().Lookup(box_value(L"data_map_pin")))));
				r_eyedropper().Visibility(Visibility::Collapsed);
				//tb_map_pointer().Visibility(Visibility::Visible);
			}
			else {
#ifdef _DEBUG
				__debugbreak();
#endif // _DEBUG

				throw winrt::hresult_invalid_argument();
			}
			if (status_bar_drawing_tool().Visibility() != Visibility::Visible) {
				status_bar_drawing_tool().Visibility(Visibility::Visible);
			}
		}
		else {
			if (status_bar_drawing_tool().Visibility() != Visibility::Collapsed) {
				status_bar_drawing_tool().Visibility(Visibility::Collapsed);
			}
		}
	}

	// 方眼の大きさをステータスバーに格納する.
	void MainPage::status_bar_set_grid(void)
	{
		if (status_and(m_status_bar, STATUS_BAR::GRID) == STATUS_BAR::GRID) {
			const float g_len = m_main_sheet.m_grid_base + 1.0f;
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

	// 用紙の大きさをステータスバーに格納する.
	void MainPage::status_bar_set_sheet(void)
	{
		if (status_and(m_status_bar, STATUS_BAR::SHEET) == STATUS_BAR::SHEET) {
			const float g_len = m_main_sheet.m_grid_base + 1.0f;
			wchar_t buf_w[32];
			conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_sheet.m_sheet_size.width, m_main_d2d.m_logical_dpi, g_len, buf_w);
			//tk_status_bar_sheet_w().Text(buf_w);
			wchar_t buf_h[32];
			conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_sheet.m_sheet_size.height, m_main_d2d.m_logical_dpi, g_len, buf_h);
			wchar_t buf[64];
			swprintf_s(buf, L"%s×%s", buf_w, buf_h);
			tk_status_bar_sheet().Text(buf);
			if (sp_status_bar_sheet().Visibility() != Visibility::Visible) {
				sp_status_bar_sheet().Visibility(Visibility::Visible);
			}
		}
		else {
			if (sp_status_bar_sheet().Visibility() != Visibility::Collapsed) {
				sp_status_bar_sheet().Visibility(Visibility::Collapsed);
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
			swprintf_s(buf, 31, FMT_ZOOM, m_main_scale * 100.0);
			//swprintf_s(buf, 31, FMT_ZOOM, m_main_sheet.m_sheet_scale * 100.0);
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