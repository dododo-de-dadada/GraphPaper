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
		tb_map_pointer().Text(L"");

		// 選択ツールが選択されている.
		if (sender == rmfi_menu_selection_tool()) {
			// XAML のキーボードアクセラレーターにエスケープキーが設定されている.
			//if (m_core_text_focused != nullptr) {
			//	m_core_text.NotifyFocusLeave();
			//	undo_push_text_unselect(m_core_text_focused);
			//	m_core_text_focused = nullptr;
			//}
			m_drawing_tool = DRAWING_TOOL::SELECT;
			m_event_state = EVENT_STATE::BEGIN;
			m_event_shape_pressed = nullptr;
			m_event_loc_pressed = LOC_TYPE::LOC_SHEET;
			unselect_shape_all();
			main_draw();
		}
		else if (sender == rmfi_menu_drawing_rect()) {
			m_drawing_tool = DRAWING_TOOL::RECT;
		}
		else if (sender == rmfi_menu_drawing_rrect()) {
			m_drawing_tool = DRAWING_TOOL::RRECT;
		}
		else if (sender == rmfi_menu_drawing_poly()) {
			m_drawing_tool = DRAWING_TOOL::POLY;
		}
		else if (sender == rmfi_menu_drawing_ellipse()) {
			m_drawing_tool = DRAWING_TOOL::ELLIPSE;
		}
		else if (sender == rmfi_menu_drawing_line()) {
			m_drawing_tool = DRAWING_TOOL::LINE;
		}
		else if (sender == rmfi_menu_drawing_bezier()) {
			m_drawing_tool = DRAWING_TOOL::BEZIER;
		}
		else if (sender == rmfi_menu_drawing_text()) {
			m_drawing_tool = DRAWING_TOOL::TEXT;
		}
		else if (sender == rmfi_menu_drawing_ruler()) {
			m_drawing_tool = DRAWING_TOOL::RULER;
		}
		else if (sender == rmfi_menu_drawing_arc()) {
			m_drawing_tool = DRAWING_TOOL::ARC;
		}
		else if (sender == rmfi_menu_eyedropper()) {
			m_drawing_tool = DRAWING_TOOL::EYEDROPPER;
		}
		else if (sender == rmfi_menu_pointer()) {
			m_drawing_tool = DRAWING_TOOL::POINTER;
		}
		else {
			if (sender == rmfi_menu_drawing_poly_di()) {
				m_drawing_poly_opt.m_vertex_cnt = 2;
			}
			else if (sender == rmfi_menu_drawing_poly_tri()) {
				m_drawing_poly_opt.m_vertex_cnt = 3;
			}
			else if (sender == rmfi_menu_drawing_poly_quad()) {
				m_drawing_poly_opt.m_vertex_cnt = 4;
			}
			else if (sender == rmfi_menu_drawing_poly_pent()) {
				m_drawing_poly_opt.m_vertex_cnt = 5;
			}
			else if (sender == rmfi_menu_drawing_poly_hexa()) {
				m_drawing_poly_opt.m_vertex_cnt = 6;
			}
			else if (sender == rmfi_menu_drawing_poly_hept()) {
				m_drawing_poly_opt.m_vertex_cnt = 7;
			}
			else if (sender == rmfi_menu_drawing_poly_octa()) {
				m_drawing_poly_opt.m_vertex_cnt = 8;
			}
			else if (sender == rmfi_menu_drawing_poly_nona()) {
				m_drawing_poly_opt.m_vertex_cnt = 9;
			}
			else if (sender == rmfi_menu_drawing_poly_deca()) {
				m_drawing_poly_opt.m_vertex_cnt = 10;
			}
			else if (sender == rmfi_menu_drawing_poly_hendeca()) {
				m_drawing_poly_opt.m_vertex_cnt = 11;
			}
			else if (sender == rmfi_menu_drawing_poly_dodeca()) {
				m_drawing_poly_opt.m_vertex_cnt = 12;
			}
			else if (sender == tmfi_tool_poly_end_close()) {
				m_drawing_poly_opt.m_end_closed = !m_drawing_poly_opt.m_end_closed;
			}
			else if (sender == tmfi_tool_poly_regular()) {
				m_drawing_poly_opt.m_regular = !m_drawing_poly_opt.m_regular;
			}
			else if (sender == tmfi_tool_poly_vertex_up()) {
				m_drawing_poly_opt.m_vertex_up = !m_drawing_poly_opt.m_vertex_up;
			}
			else if (sender == tmfi_tool_poly_clockwise()) {
				m_drawing_poly_opt.m_clockwise = !m_drawing_poly_opt.m_clockwise;
			}
			else {
				throw winrt::hresult_not_implemented();
				return;
			}
			m_drawing_tool = DRAWING_TOOL::POLY;
			rmfi_menu_drawing_poly().IsChecked(true);
		}
		status_bar_set_draw();
		status_bar_set_pos();
		event_set_cursor();
	}

}