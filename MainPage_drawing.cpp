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
		if (sender == rmfi_selection_tool()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::SELECT);
			m_event_state = EVENT_STATE::BEGIN;
			m_event_shape_pressed = nullptr;
			m_event_loc_pressed = LOC_TYPE::LOC_PAGE;
		}
		else if (sender == rmfi_drawing_rect()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::RECT);
		}
		else if (sender == rmfi_drawing_rrect()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::RRECT);
		}
		else if (sender == rmfi_drawing_poly()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::POLY);
		}
		else if (sender == rmfi_drawing_ellipse()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::ELLIPSE);
		}
		else if (sender == rmfi_drawing_line()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::LINE);
		}
		else if (sender == rmfi_drawing_bezier()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::BEZIER);
		}
		else if (sender == rmfi_drawing_text()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::TEXT);
		}
		else if (sender == rmfi_drawing_ruler()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::RULER);
		}
		else if (sender == rmfi_drawing_arc()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::ARC);
		}
		else if (sender == rmfi_tool_eyedropper()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::EYEDROPPER);
		}
		else {
			if (sender == rmfi_drawing_poly_di()) {
				drawing_poly_vtx_is_checked(m_drawing_poly_opt.m_vertex_cnt = 2);
			}
			else if (sender == rmfi_drawing_poly_tri()) {
				drawing_poly_vtx_is_checked(m_drawing_poly_opt.m_vertex_cnt = 3);
			}
			else if (sender == rmfi_drawing_poly_quad()) {
				drawing_poly_vtx_is_checked(m_drawing_poly_opt.m_vertex_cnt = 4);
			}
			else if (sender == rmfi_drawing_poly_pent()) {
				drawing_poly_vtx_is_checked(m_drawing_poly_opt.m_vertex_cnt = 5);
			}
			else if (sender == rmfi_drawing_poly_hexa()) {
				drawing_poly_vtx_is_checked(m_drawing_poly_opt.m_vertex_cnt = 6);
			}
			else if (sender == rmfi_drawing_poly_hept()) {
				drawing_poly_vtx_is_checked(m_drawing_poly_opt.m_vertex_cnt = 7);
			}
			else if (sender == rmfi_drawing_poly_octa()) {
				drawing_poly_vtx_is_checked(m_drawing_poly_opt.m_vertex_cnt = 8);
			}
			else if (sender == rmfi_drawing_poly_nona()) {
				drawing_poly_vtx_is_checked(m_drawing_poly_opt.m_vertex_cnt = 9);
			}
			else if (sender == rmfi_drawing_poly_deca()) {
				drawing_poly_vtx_is_checked(m_drawing_poly_opt.m_vertex_cnt = 10);
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
				winrt::hresult_not_implemented();
			}
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::POLY);
		}
		status_bar_set_draw();
		status_bar_set_pos();
		event_set_cursor();
	}

	// 作図ツールのメニューに印をつける.
	void MainPage::drawing_tool_is_checked(const DRAWING_TOOL val)
	{
		rmfi_selection_tool().IsChecked(val == DRAWING_TOOL::SELECT);
		rmfi_drawing_rect().IsChecked(val == DRAWING_TOOL::RECT);
		rmfi_drawing_rrect().IsChecked(val == DRAWING_TOOL::RRECT);
		rmfi_drawing_poly().IsChecked(val == DRAWING_TOOL::POLY);
		rmfi_drawing_ellipse().IsChecked(val == DRAWING_TOOL::ELLIPSE);
		rmfi_drawing_line().IsChecked(val == DRAWING_TOOL::LINE);
		rmfi_drawing_bezier().IsChecked(val == DRAWING_TOOL::BEZIER);
		rmfi_drawing_text().IsChecked(val == DRAWING_TOOL::TEXT);
		rmfi_drawing_ruler().IsChecked(val == DRAWING_TOOL::RULER);
		rmfi_drawing_arc().IsChecked(val == DRAWING_TOOL::ARC);
		rmfi_tool_eyedropper().IsChecked(val == DRAWING_TOOL::EYEDROPPER);
	}

	// 多角形の選択肢メニューにチェックをつける.
	void MainPage::drawing_poly_opt_is_checked(const POLY_OPTION& val)
	{
		drawing_poly_vtx_is_checked(val.m_vertex_cnt);
		tmfi_tool_poly_regular().IsChecked(val.m_regular);
		tmfi_tool_poly_vertex_up().IsChecked(val.m_vertex_up);
		tmfi_tool_poly_end_close().IsChecked(val.m_end_closed);
		tmfi_tool_poly_clockwise().IsChecked(val.m_clockwise);
	}

	// 多角形の頂点数メニューにチェックをつける.
	void MainPage::drawing_poly_vtx_is_checked(const uint32_t val)
	{
		rmfi_drawing_poly_di().IsChecked(val == 2);
		rmfi_drawing_poly_tri().IsChecked(val == 3);
		rmfi_drawing_poly_quad().IsChecked(val == 4);
		rmfi_drawing_poly_pent().IsChecked(val == 5);
		rmfi_drawing_poly_hexa().IsChecked(val == 6);
		rmfi_drawing_poly_hept().IsChecked(val == 7);
		rmfi_drawing_poly_octa().IsChecked(val == 8);
		rmfi_drawing_poly_nona().IsChecked(val == 9);
		rmfi_drawing_poly_deca().IsChecked(val == 10);
	}

}