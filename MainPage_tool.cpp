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
		tool_draw(static_cast<DRAW_TOOL>(dt_reader.ReadUInt32()));
		POLY_TOOL p_tool;
		dt_read(p_tool, dt_reader);
		tool_poly(p_tool.m_vertex_cnt);
		tmfi_tool_poly_regular().IsChecked(m_tool_poly.m_regular = p_tool.m_regular);
		tmfi_tool_poly_vertex_up().IsChecked(m_tool_poly.m_vertex_up = p_tool.m_vertex_up);
		tmfi_tool_poly_closed().IsChecked(m_tool_poly.m_closed = p_tool.m_closed);
		tmfi_tool_poly_clockwise().IsChecked(m_tool_poly.m_clockwise = p_tool.m_clockwise);
	}

	template <typename T, T C> constexpr void radio_menu_item_set_value(const T value, const RadioMenuFlyoutItem& item)
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
		radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::RECT>(value, rmfi_tool_draw_rect());
		radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::RRECT>(value, rmfi_tool_draw_rrect());
		radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::POLY>(value, rmfi_tool_draw_poly());
		radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::ELLI>(value, rmfi_tool_draw_elli());
		radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::LINE>(value, rmfi_tool_draw_line());
		radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::BEZI>(value, rmfi_tool_draw_bezi());
		radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::TEXT>(value, rmfi_tool_draw_text());
		radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::RULER>(value, rmfi_tool_draw_ruler());
		m_tool_draw = value;
	}

	void MainPage::tool_poly(const uint32_t value)
	{
		radio_menu_item_set_value<uint32_t, 2>(value, rmfi_tool_poly_line());
		radio_menu_item_set_value<uint32_t, 3>(value, rmfi_tool_poly_tri());
		radio_menu_item_set_value<uint32_t, 4>(value, rmfi_tool_poly_quad());
		radio_menu_item_set_value<uint32_t, 5>(value, rmfi_tool_poly_pent());
		radio_menu_item_set_value<uint32_t, 6>(value, rmfi_tool_poly_hexa());
		radio_menu_item_set_value<uint32_t, 7>(value, rmfi_tool_poly_hept());
		radio_menu_item_set_value<uint32_t, 8>(value, rmfi_tool_poly_octa());
		radio_menu_item_set_value<uint32_t, 9>(value, rmfi_tool_poly_nona());
		radio_menu_item_set_value<uint32_t, 10>(value, rmfi_tool_poly_deca());
		m_tool_poly.m_vertex_cnt = value;
	}

	void MainPage::tool_draw_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == rmfi_tool_select()) {
			tool_draw(DRAW_TOOL::SELECT);
			event_state(EVENT_STATE::BEGIN);
			event_shape(nullptr);
			event_anch_pressed(ANCH_TYPE::ANCH_SHEET);
		}
		else if (sender == rmfi_tool_draw_rect()) {
			tool_draw(DRAW_TOOL::RECT);
		}
		else if (sender == rmfi_tool_draw_rrect()) {
			tool_draw(DRAW_TOOL::RRECT);
		}
		else if (sender == rmfi_tool_draw_poly()) {
			tool_draw(DRAW_TOOL::POLY);
		}
		else if (sender == rmfi_tool_draw_elli()) {
			tool_draw(DRAW_TOOL::ELLI);
		}
		else if (sender == rmfi_tool_draw_line()) {
			tool_draw(DRAW_TOOL::LINE);
		}
		else if (sender == rmfi_tool_draw_bezi()) {
			tool_draw(DRAW_TOOL::BEZI);
		}
		else if (sender == rmfi_tool_draw_text()) {
			tool_draw(DRAW_TOOL::TEXT);
		}
		else if (sender == rmfi_tool_draw_ruler()) {
			tool_draw(DRAW_TOOL::RULER);
		}
		else {
			if (sender == rmfi_tool_poly_line()) {
				tool_poly(2);
			}
			else if (sender == rmfi_tool_poly_tri()) {
				tool_poly(3);
			}
			else if (sender == rmfi_tool_poly_quad()) {
				tool_poly(4);
			}
			else if (sender == rmfi_tool_poly_pent()) {
				tool_poly(5);
			}
			else if (sender == rmfi_tool_poly_hexa()) {
				tool_poly(6);
			}
			else if (sender == rmfi_tool_poly_hept()) {
				tool_poly(7);
			}
			else if (sender == rmfi_tool_poly_octa()) {
				tool_poly(8);
			}
			else if (sender == rmfi_tool_poly_nona()) {
				tool_poly(9);
			}
			else if (sender == rmfi_tool_poly_deca()) {
				tool_poly(10);
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
			tool_draw(DRAW_TOOL::POLY);
		}
		stbar_set_draw();
		event_curs_style();
	}

}