//-------------------------------
// MainPage_tool.cpp
// çÏê}ÉcÅ[Éã
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	void MainPage::tool_write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_tool_draw));
		write(m_tool_poly, dt_writer);
	}

	void MainPage::tool_read(DataReader const& dt_reader)
	{
		m_tool_draw = static_cast<TOOL_DRAW>(dt_reader.ReadUInt32());
		read(m_tool_poly, dt_reader);

		if (m_tool_draw == TOOL_DRAW::SELECT) {
			rmfi_tool_select().IsChecked(true);
		}
		else if (m_tool_draw == TOOL_DRAW::BEZI) {
			rmfi_tool_draw_bezi().IsChecked(true);
		}
		else if (m_tool_draw == TOOL_DRAW::ELLI) {
			rmfi_tool_draw_elli().IsChecked(true);
		}
		else if (m_tool_draw == TOOL_DRAW::LINE) {
			rmfi_tool_draw_line().IsChecked(true);
		}
		else if (m_tool_draw == TOOL_DRAW::POLY) {
			rmfi_tool_draw_poly().IsChecked(true);
		}
		else if (m_tool_draw == TOOL_DRAW::RECT) {
			rmfi_tool_draw_rect().IsChecked(true);
		}
		else if (m_tool_draw == TOOL_DRAW::RRECT) {
			rmfi_tool_draw_rrect().IsChecked(true);
		}
		else if (m_tool_draw == TOOL_DRAW::RULER) {
			rmfi_tool_draw_ruler().IsChecked(true);
		}
		else if (m_tool_draw == TOOL_DRAW::TEXT) {
			rmfi_tool_draw_text().IsChecked(true);
		}
		if (m_tool_poly.m_vertex_cnt == 2) {
			rmfi_tool_poly_line().IsChecked(true);
		}
		else if (m_tool_poly.m_vertex_cnt == 3) {
			rmfi_tool_poly_tri().IsChecked(true);
		}
		else if (m_tool_poly.m_vertex_cnt == 4) {
			rmfi_tool_poly_quad().IsChecked(true);
		}
		else if (m_tool_poly.m_vertex_cnt == 5) {
			rmfi_tool_poly_pent().IsChecked(true);
		}
		else if (m_tool_poly.m_vertex_cnt == 6) {
			rmfi_tool_poly_hexa().IsChecked(true);
		}
		else if (m_tool_poly.m_vertex_cnt == 7) {
			rmfi_tool_poly_hept().IsChecked(true);
		}
		else if (m_tool_poly.m_vertex_cnt == 8) {
			rmfi_tool_poly_octa().IsChecked(true);
		}
		else if (m_tool_poly.m_vertex_cnt == 9) {
			rmfi_tool_poly_nona().IsChecked(true);
		}
		else if (m_tool_poly.m_vertex_cnt == 10) {
			rmfi_tool_poly_deca().IsChecked(true);
		}
		tmfi_tool_poly_regular().IsChecked(m_tool_poly.m_regular);
		tmfi_tool_poly_vertex_up().IsChecked(m_tool_poly.m_vertex_up);
		tmfi_tool_poly_closed().IsChecked(m_tool_poly.m_closed);
		tmfi_tool_poly_clockwise().IsChecked(m_tool_poly.m_clockwise);
	}

	void MainPage::tool_draw_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == rmfi_tool_select()) {
			m_tool_draw = TOOL_DRAW::SELECT;
			pointer_state(PBTN_STATE::BEGIN);
			pointer_shape(nullptr);
			pointer_anchor(ANCH_TYPE::ANCH_SHEET);
		}
		else if (sender == rmfi_tool_draw_rect()) {
			m_tool_draw = TOOL_DRAW::RECT;
		}
		else if (sender == rmfi_tool_draw_rrect()) {
			m_tool_draw = TOOL_DRAW::RRECT;
		}
		else if (sender == rmfi_tool_draw_poly()) {
			m_tool_draw = TOOL_DRAW::POLY;
		}
		else if (sender == rmfi_tool_draw_elli()) {
			m_tool_draw = TOOL_DRAW::ELLI;
		}
		else if (sender == rmfi_tool_draw_line()) {
			m_tool_draw = TOOL_DRAW::LINE;
		}
		else if (sender == rmfi_tool_draw_bezi()) {
			m_tool_draw = TOOL_DRAW::BEZI;
		}
		else if (sender == rmfi_tool_draw_text()) {
			m_tool_draw = TOOL_DRAW::TEXT;
		}
		else if (sender == rmfi_tool_draw_ruler()) {
			m_tool_draw = TOOL_DRAW::RULER;
		}
		else {
			if (sender == rmfi_tool_poly_line()) {
				m_tool_poly.m_vertex_cnt = 2;
			}
			else if (sender == rmfi_tool_poly_tri()) {
				m_tool_poly.m_vertex_cnt = 3;
			}
			else if (sender == rmfi_tool_poly_quad()) {
				m_tool_poly.m_vertex_cnt = 4;
			}
			else if (sender == rmfi_tool_poly_pent()) {
				m_tool_poly.m_vertex_cnt = 5;
			}
			else if (sender == rmfi_tool_poly_hexa()) {
				m_tool_poly.m_vertex_cnt = 6;
			}
			else if (sender == rmfi_tool_poly_hept()) {
				m_tool_poly.m_vertex_cnt = 7;
			}
			else if (sender == rmfi_tool_poly_octa()) {
				m_tool_poly.m_vertex_cnt = 8;
			}
			else if (sender == rmfi_tool_poly_nona()) {
				m_tool_poly.m_vertex_cnt = 9;
			}
			else if (sender == rmfi_tool_poly_deca()) {
				m_tool_poly.m_vertex_cnt = 10;
			}
			else if (sender == tmfi_tool_poly_closed()) {
				m_tool_poly.m_closed = !m_tool_poly.m_closed;
			}
			else if (sender == tmfi_tool_poly_regular()) {
				m_tool_poly.m_regular = !m_tool_poly.m_regular;
			}
			else if (sender == tmfi_tool_poly_vertex_up()) {
				m_tool_poly.m_vertex_up = !m_tool_poly.m_vertex_up;
			}
			else if (sender == tmfi_tool_poly_clockwise()) {
				m_tool_poly.m_clockwise = !m_tool_poly.m_clockwise;
			}
			else {
				return;
			}
			rmfi_tool_draw_poly().IsChecked(true);
			m_tool_draw = TOOL_DRAW::POLY;
		}
		sbar_set_draw();
		pointer_set();
	}

}