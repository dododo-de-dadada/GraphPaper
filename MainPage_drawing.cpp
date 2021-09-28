//-------------------------------
// MainPage_drawing.cpp
// 作図ツール
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 作図ツールのメニュー項目が選択された.
	void MainPage::drawing_tool_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == rmfi_select_tool()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::SELECT);
			m_event_state = EVENT_STATE::BEGIN;
			m_event_shape_pressed = nullptr;
			m_event_anch_pressed = ANCH_TYPE::ANCH_SHEET;
		}
		else if (sender == rmfi_drawing_tool_rect()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::RECT);
		}
		else if (sender == rmfi_drawing_tool_rrect()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::RRECT);
		}
		else if (sender == rmfi_drawing_tool_poly()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::POLY);
		}
		else if (sender == rmfi_drawing_tool_elli()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::ELLI);
		}
		else if (sender == rmfi_drawing_tool_line()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::LINE);
		}
		else if (sender == rmfi_drawing_tool_bezi()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::BEZI);
		}
		else if (sender == rmfi_drawing_tool_text()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::TEXT);
		}
		else if (sender == rmfi_drawing_tool_ruler()) {
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::RULER);
		}
		else {
			if (sender == rmfi_drawing_poly_line()) {
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
				return;
			}
			drawing_tool_is_checked(m_drawing_tool = DRAWING_TOOL::POLY);
		}
		status_set_draw();
		event_set_cursor();
	}

	// 作図ツールのメニューに印をつける.
	void MainPage::drawing_tool_is_checked(const DRAWING_TOOL value)
	{
		rmfi_select_tool().IsChecked(value == DRAWING_TOOL::SELECT);
		rmfi_drawing_tool_rect().IsChecked(value == DRAWING_TOOL::RECT);
		rmfi_drawing_tool_rrect().IsChecked(value == DRAWING_TOOL::RRECT);
		rmfi_drawing_tool_poly().IsChecked(value == DRAWING_TOOL::POLY);
		rmfi_drawing_tool_elli().IsChecked(value == DRAWING_TOOL::ELLI);
		rmfi_drawing_tool_line().IsChecked(value == DRAWING_TOOL::LINE);
		rmfi_drawing_tool_bezi().IsChecked(value == DRAWING_TOOL::BEZI);
		rmfi_drawing_tool_text().IsChecked(value == DRAWING_TOOL::TEXT);
		rmfi_drawing_tool_ruler().IsChecked(value == DRAWING_TOOL::RULER);
	}

	// 多角形の選択肢メニューにチェックをつける.
	void MainPage::drawing_poly_opt_is_checked(const POLY_OPTION& value)
	{
		drawing_poly_vtx_is_checked(value.m_vertex_cnt);
		tmfi_tool_poly_regular().IsChecked(value.m_regular);
		tmfi_tool_poly_vertex_up().IsChecked(value.m_vertex_up);
		tmfi_tool_poly_end_close().IsChecked(value.m_end_closed);
		tmfi_tool_poly_clockwise().IsChecked(value.m_clockwise);
	}

	// 多角形の頂点数メニューにチェックをつける.
	void MainPage::drawing_poly_vtx_is_checked(const uint32_t value)
	{
		rmfi_drawing_poly_line().IsChecked(value == 2);
		rmfi_drawing_poly_tri().IsChecked(value == 3);
		rmfi_drawing_poly_quad().IsChecked(value == 4);
		rmfi_drawing_poly_pent().IsChecked(value == 5);
		rmfi_drawing_poly_hexa().IsChecked(value == 6);
		rmfi_drawing_poly_hept().IsChecked(value == 7);
		rmfi_drawing_poly_octa().IsChecked(value == 8);
		rmfi_drawing_poly_nona().IsChecked(value == 9);
		rmfi_drawing_poly_deca().IsChecked(value == 10);
	}

}