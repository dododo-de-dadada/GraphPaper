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
		dt_write(m_tool_poly, dt_writer);
	}

	void MainPage::tool_read(DataReader const& dt_reader)
	{
		m_tool_draw = static_cast<DRAW_TOOL>(dt_reader.ReadUInt32());
		dt_read(m_tool_poly, dt_reader);

		if (m_tool_draw == DRAW_TOOL::SELECT) {
			rmfi_tool_select().IsChecked(true);
		}
		else if (m_tool_draw == DRAW_TOOL::BEZI) {
			rmfi_tool_draw_bezi().IsChecked(true);
		}
		else if (m_tool_draw == DRAW_TOOL::ELLI) {
			rmfi_tool_draw_elli().IsChecked(true);
		}
		else if (m_tool_draw == DRAW_TOOL::LINE) {
			rmfi_tool_draw_line().IsChecked(true);
		}
		else if (m_tool_draw == DRAW_TOOL::POLY) {
			rmfi_tool_draw_poly().IsChecked(true);
		}
		else if (m_tool_draw == DRAW_TOOL::RECT) {
			rmfi_tool_draw_rect().IsChecked(true);
		}
		else if (m_tool_draw == DRAW_TOOL::RRECT) {
			rmfi_tool_draw_rrect().IsChecked(true);
		}
		else if (m_tool_draw == DRAW_TOOL::RULER) {
			rmfi_tool_draw_ruler().IsChecked(true);
		}
		else if (m_tool_draw == DRAW_TOOL::TEXT) {
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

	template <typename T, T C> void radio_menu_item_set_value(const T value, const RadioMenuFlyoutItem& item)
	{
		if (item.IsChecked()) {
			if (value != C) {
				item.IsChecked(true);
			}
		}
		else if (value == C) {
			item.IsChecked(false);
		}
	}

	void MainPage::tool_draw(const DRAW_TOOL value)
	{
		radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::SELECT>(value, rmfi_tool_select());
	}

	void MainPage::tool_draw_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == rmfi_tool_select()) {
			m_tool_draw = DRAW_TOOL::SELECT;
			event_state(EVENT_STATE::BEGIN);
			event_shape(nullptr);
			event_anch_pressed(ANCH_TYPE::ANCH_SHEET);
		}
		else if (sender == rmfi_tool_draw_rect()) {
			m_tool_draw = DRAW_TOOL::RECT;
		}
		else if (sender == rmfi_tool_draw_rrect()) {
			m_tool_draw = DRAW_TOOL::RRECT;
		}
		else if (sender == rmfi_tool_draw_poly()) {
			m_tool_draw = DRAW_TOOL::POLY;
		}
		else if (sender == rmfi_tool_draw_elli()) {
			m_tool_draw = DRAW_TOOL::ELLI;
		}
		else if (sender == rmfi_tool_draw_line()) {
			m_tool_draw = DRAW_TOOL::LINE;
		}
		else if (sender == rmfi_tool_draw_bezi()) {
			m_tool_draw = DRAW_TOOL::BEZI;
		}
		else if (sender == rmfi_tool_draw_text()) {
			m_tool_draw = DRAW_TOOL::TEXT;
		}
		else if (sender == rmfi_tool_draw_ruler()) {
			m_tool_draw = DRAW_TOOL::RULER;
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
			m_tool_draw = DRAW_TOOL::POLY;
		}
		stbar_set_draw();
		event_curs_style();
	}

}