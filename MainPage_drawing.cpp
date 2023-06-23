//-------------------------------
// MainPage_drawing.cpp
// 作図ツール
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	// 作図ツールのメニュー項目が選択された.
	void MainPage::drawing_tool_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		status_bar_tool_value().Text(L"- -");

		// 選択ツールが選択されている.
		if (sender == menu_tool_selection()) {
			// XAML のキーボードアクセラレーターにエスケープキーが設定されている.
			//if (m_core_text_focused != nullptr) {
			//	m_core_text.NotifyFocusLeave();
			//	undo_push_text_unselect(m_core_text_focused);
			//	m_core_text_focused = nullptr;
			//}
			m_tool = DRAWING_TOOL::SELECT;
			m_event_state = EVENT_STATE::BEGIN;
			m_event_shape_pressed = nullptr;
			m_event_locus_pressed = LOCUS_TYPE::LOCUS_SHEET;
			unselect_all_shape();
			main_sheet_draw();
		}
		else if (sender == menu_tool_rect()) {
			m_tool = DRAWING_TOOL::RECT;
		}
		else if (sender == menu_tool_rrect()) {
			m_tool = DRAWING_TOOL::RRECT;
		}
		else if (sender == menu_tool_polygon()) {
			m_tool = DRAWING_TOOL::POLY;
		}
		else if (sender == menu_tool_ellipse()) {
			m_tool = DRAWING_TOOL::ELLIPSE;
		}
		else if (sender == menu_tool_line()) {
			m_tool = DRAWING_TOOL::LINE;
		}
		else if (sender == menu_tool_bezier()) {
			m_tool = DRAWING_TOOL::BEZIER;
		}
		else if (sender == menu_tool_text()) {
			m_tool = DRAWING_TOOL::TEXT;
		}
		else if (sender == menu_tool_ruler()) {
			m_tool = DRAWING_TOOL::RULER;
		}
		else if (sender == menu_tool_arc()) {
			m_tool = DRAWING_TOOL::ARC;
		}
		else if (sender == menu_tool_eyedropper()) {
			m_tool = DRAWING_TOOL::EYEDROPPER;
		}
		//else if (sender == rmfi_menu_pointer()) {
		//	m_tool = DRAWING_TOOL::POINTER;
		//}
		else {
			if (sender == menu_tool_polygon_di()) {
				m_tool_polygon.m_vertex_cnt = 2;
			}
			else if (sender == menu_tool_polygon_tri()) {
				m_tool_polygon.m_vertex_cnt = 3;
			}
			else if (sender == menu_tool_polygon_quad()) {
				m_tool_polygon.m_vertex_cnt = 4;
			}
			else if (sender == menu_tool_polygon_pent()) {
				m_tool_polygon.m_vertex_cnt = 5;
			}
			else if (sender == menu_tool_polygon_hexa()) {
				m_tool_polygon.m_vertex_cnt = 6;
			}
			else if (sender == menu_tool_polygon_hept()) {
				m_tool_polygon.m_vertex_cnt = 7;
			}
			else if (sender == menu_tool_polygon_octa()) {
				m_tool_polygon.m_vertex_cnt = 8;
			}
			else if (sender == menu_tool_polygon_nona()) {
				m_tool_polygon.m_vertex_cnt = 9;
			}
			else if (sender == menu_tool_polygon_deca()) {
				m_tool_polygon.m_vertex_cnt = 10;
			}
			else if (sender == menu_tool_polygon_hendeca()) {
				m_tool_polygon.m_vertex_cnt = 11;
			}
			else if (sender == menu_tool_polygon_dodeca()) {
				m_tool_polygon.m_vertex_cnt = 12;
			}
			else if (sender == tmfi_tool_poly_end_close()) {
				m_tool_polygon.m_end_closed = !m_tool_polygon.m_end_closed;
			}
			else if (sender == tmfi_tool_poly_regular()) {
				m_tool_polygon.m_regular = !m_tool_polygon.m_regular;
			}
			else if (sender == tmfi_tool_poly_vertex_up()) {
				m_tool_polygon.m_vertex_up = !m_tool_polygon.m_vertex_up;
			}
			else if (sender == tmfi_tool_poly_clockwise()) {
				m_tool_polygon.m_clockwise = !m_tool_polygon.m_clockwise;
			}
			else {
				throw winrt::hresult_not_implemented();
				return;
			}
			m_tool = DRAWING_TOOL::POLY;
			menu_tool_polygon().IsChecked(true);
		}
		status_bar_set_drawing_tool();
		status_bar_set_pointer();
		event_set_cursor();
	}

}