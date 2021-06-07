//-------------------------------
// MainPage_tool.cpp
// 作図ツール
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 作図ツールの状態を書き込む.
	void MainPage::tool_write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_tool_draw));
		dt_write(m_tool_poly, dt_writer);
	}

	// 作図ツールの状態を読み込む.
	void MainPage::tool_read(DataReader const& dt_reader)
	{
		m_tool_draw = static_cast<DRAW_TOOL>(dt_reader.ReadUInt32());
		dt_read(m_tool_poly, dt_reader);
	}

	void MainPage::tool_draw_is_checked(const DRAW_TOOL value)
	{
		menu_item_is_checked(value == DRAW_TOOL::SELECT, rmfi_tool_select());
		menu_item_is_checked(value == DRAW_TOOL::RECT, rmfi_tool_draw_rect());
		menu_item_is_checked(value == DRAW_TOOL::RRECT, rmfi_tool_draw_rrect());
		menu_item_is_checked(value == DRAW_TOOL::POLY, rmfi_tool_draw_poly());
		menu_item_is_checked(value == DRAW_TOOL::ELLI, rmfi_tool_draw_elli());
		menu_item_is_checked(value == DRAW_TOOL::LINE, rmfi_tool_draw_line());
		menu_item_is_checked(value == DRAW_TOOL::BEZI, rmfi_tool_draw_bezi());
		menu_item_is_checked(value == DRAW_TOOL::TEXT, rmfi_tool_draw_text());
		menu_item_is_checked(value == DRAW_TOOL::RULER, rmfi_tool_draw_ruler());
		//radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::SELECT>(value, rmfi_tool_select());
		//radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::RECT>(value, rmfi_tool_draw_rect());
		//radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::RRECT>(value, rmfi_tool_draw_rrect());
		//radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::POLY>(value, rmfi_tool_draw_poly());
		//radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::ELLI>(value, rmfi_tool_draw_elli());
		//radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::LINE>(value, rmfi_tool_draw_line());
		//radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::BEZI>(value, rmfi_tool_draw_bezi());
		//radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::TEXT>(value, rmfi_tool_draw_text());
		//radio_menu_item_set_value<DRAW_TOOL, DRAW_TOOL::RULER>(value, rmfi_tool_draw_ruler());
	}

	// 多角形ツールのメニューのチェックをつける.
	void MainPage::tool_poly_is_checked(const uint32_t value)
	{
		menu_item_is_checked(value == 2, rmfi_tool_poly_line());
		menu_item_is_checked(value == 3, rmfi_tool_poly_tri());
		menu_item_is_checked(value == 4, rmfi_tool_poly_quad());
		menu_item_is_checked(value == 5, rmfi_tool_poly_pent());
		menu_item_is_checked(value == 6, rmfi_tool_poly_hexa());
		menu_item_is_checked(value == 7, rmfi_tool_poly_hept());
		menu_item_is_checked(value == 8, rmfi_tool_poly_octa());
		menu_item_is_checked(value == 9, rmfi_tool_poly_nona());
		menu_item_is_checked(value == 10, rmfi_tool_poly_deca());
		//radio_menu_item_set_value<uint32_t, 2>(value, rmfi_tool_poly_line());
		//radio_menu_item_set_value<uint32_t, 3>(value, rmfi_tool_poly_tri());
		//radio_menu_item_set_value<uint32_t, 4>(value, rmfi_tool_poly_quad());
		//radio_menu_item_set_value<uint32_t, 5>(value, rmfi_tool_poly_pent());
		//radio_menu_item_set_value<uint32_t, 6>(value, rmfi_tool_poly_hexa());
		//radio_menu_item_set_value<uint32_t, 7>(value, rmfi_tool_poly_hept());
		//radio_menu_item_set_value<uint32_t, 8>(value, rmfi_tool_poly_octa());
		//radio_menu_item_set_value<uint32_t, 9>(value, rmfi_tool_poly_nona());
		//radio_menu_item_set_value<uint32_t, 10>(value, rmfi_tool_poly_deca());
	}

	// 多角形ツールのメニューのチェックをつける.
	void MainPage::tool_poly_is_checked(const POLY_TOOL& value)
	{
		tool_poly_is_checked(value.m_vertex_cnt);
		tmfi_tool_poly_regular().IsChecked(value.m_regular);
		tmfi_tool_poly_vertex_up().IsChecked(value.m_vertex_up);
		tmfi_tool_poly_closed().IsChecked(value.m_end_closed);
		tmfi_tool_poly_clockwise().IsChecked(value.m_clockwise);
	}

	// 作図メニューが選択された.
	void MainPage::tool_draw_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == rmfi_tool_select()) {
			tool_draw_is_checked(m_tool_draw = DRAW_TOOL::SELECT);
			event_state(EVENT_STATE::BEGIN);
			event_shape(nullptr);
			event_anch_pressed(ANCH_TYPE::ANCH_SHEET);
		}
		else if (sender == rmfi_tool_draw_rect()) {
			tool_draw_is_checked(m_tool_draw = DRAW_TOOL::RECT);
		}
		else if (sender == rmfi_tool_draw_rrect()) {
			tool_draw_is_checked(m_tool_draw = DRAW_TOOL::RRECT);
		}
		else if (sender == rmfi_tool_draw_poly()) {
			tool_draw_is_checked(m_tool_draw = DRAW_TOOL::POLY);
		}
		else if (sender == rmfi_tool_draw_elli()) {
			tool_draw_is_checked(m_tool_draw = DRAW_TOOL::ELLI);
		}
		else if (sender == rmfi_tool_draw_line()) {
			tool_draw_is_checked(m_tool_draw = DRAW_TOOL::LINE);
		}
		else if (sender == rmfi_tool_draw_bezi()) {
			tool_draw_is_checked(m_tool_draw = DRAW_TOOL::BEZI);
		}
		else if (sender == rmfi_tool_draw_text()) {
			tool_draw_is_checked(m_tool_draw = DRAW_TOOL::TEXT);
		}
		else if (sender == rmfi_tool_draw_ruler()) {
			tool_draw_is_checked(m_tool_draw = DRAW_TOOL::RULER);
		}
		else {
			if (sender == rmfi_tool_poly_line()) {
				tool_poly_is_checked(m_tool_poly.m_vertex_cnt = 2);
			}
			else if (sender == rmfi_tool_poly_tri()) {
				tool_poly_is_checked(m_tool_poly.m_vertex_cnt = 3);
			}
			else if (sender == rmfi_tool_poly_quad()) {
				tool_poly_is_checked(m_tool_poly.m_vertex_cnt = 4);
			}
			else if (sender == rmfi_tool_poly_pent()) {
				tool_poly_is_checked(m_tool_poly.m_vertex_cnt = 5);
			}
			else if (sender == rmfi_tool_poly_hexa()) {
				tool_poly_is_checked(m_tool_poly.m_vertex_cnt = 6);
			}
			else if (sender == rmfi_tool_poly_hept()) {
				tool_poly_is_checked(m_tool_poly.m_vertex_cnt = 7);
			}
			else if (sender == rmfi_tool_poly_octa()) {
				tool_poly_is_checked(m_tool_poly.m_vertex_cnt = 8);
			}
			else if (sender == rmfi_tool_poly_nona()) {
				tool_poly_is_checked(m_tool_poly.m_vertex_cnt = 9);
			}
			else if (sender == rmfi_tool_poly_deca()) {
				tool_poly_is_checked(m_tool_poly.m_vertex_cnt = 10);
			}
			else if (sender == tmfi_tool_poly_closed()) {
				m_tool_poly.m_end_closed = !m_tool_poly.m_end_closed;
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
			tool_draw_is_checked(m_tool_draw = DRAW_TOOL::POLY);
		}
		stbar_set_draw();
		event_curs_style();
	}

}