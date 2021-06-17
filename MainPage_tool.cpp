//-------------------------------
// MainPage_tool.cpp
// 作図ツール
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// Escape が押された.
	void MainPage::kacc_tool_select_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_tool_draw == DRAW_TOOL::SELECT) {
			unselect_all();
			sheet_draw();
		}
		else {
			tool_draw_click(rmfi_tool_select(), nullptr);
		}
	}

	// 作図ツールのメニューが選択された.
	void MainPage::tool_draw_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == rmfi_tool_select()) {
			tool_draw_is_checked(m_tool_draw = DRAW_TOOL::SELECT);
			m_event_state = EVENT_STATE::BEGIN;
			m_event_shape_pressed = nullptr;
			m_event_anch_pressed = ANCH_TYPE::ANCH_SHEET;
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
				tool_poly_n_is_checked(m_tool_poly.m_vertex_cnt = 2);
			}
			else if (sender == rmfi_tool_poly_tri()) {
				tool_poly_n_is_checked(m_tool_poly.m_vertex_cnt = 3);
			}
			else if (sender == rmfi_tool_poly_quad()) {
				tool_poly_n_is_checked(m_tool_poly.m_vertex_cnt = 4);
			}
			else if (sender == rmfi_tool_poly_pent()) {
				tool_poly_n_is_checked(m_tool_poly.m_vertex_cnt = 5);
			}
			else if (sender == rmfi_tool_poly_hexa()) {
				tool_poly_n_is_checked(m_tool_poly.m_vertex_cnt = 6);
			}
			else if (sender == rmfi_tool_poly_hept()) {
				tool_poly_n_is_checked(m_tool_poly.m_vertex_cnt = 7);
			}
			else if (sender == rmfi_tool_poly_octa()) {
				tool_poly_n_is_checked(m_tool_poly.m_vertex_cnt = 8);
			}
			else if (sender == rmfi_tool_poly_nona()) {
				tool_poly_n_is_checked(m_tool_poly.m_vertex_cnt = 9);
			}
			else if (sender == rmfi_tool_poly_deca()) {
				tool_poly_n_is_checked(m_tool_poly.m_vertex_cnt = 10);
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
		status_bar_set_draw();
		event_set_cursor();
	}

	// 作図ツールのメニューに印をつける.
	void MainPage::tool_draw_is_checked(const DRAW_TOOL value)
	{
		rmfi_tool_select().IsChecked(value == DRAW_TOOL::SELECT);
		rmfi_tool_draw_rect().IsChecked(value == DRAW_TOOL::RECT);
		rmfi_tool_draw_rrect().IsChecked(value == DRAW_TOOL::RRECT);
		rmfi_tool_draw_poly().IsChecked(value == DRAW_TOOL::POLY);
		rmfi_tool_draw_elli().IsChecked(value == DRAW_TOOL::ELLI);
		rmfi_tool_draw_line().IsChecked(value == DRAW_TOOL::LINE);
		rmfi_tool_draw_bezi().IsChecked(value == DRAW_TOOL::BEZI);
		rmfi_tool_draw_text().IsChecked(value == DRAW_TOOL::TEXT);
		rmfi_tool_draw_ruler().IsChecked(value == DRAW_TOOL::RULER);
	}

	// 多角形ツールのメニューのチェックをつける.
	void MainPage::tool_poly_is_checked(const POLY_TOOL& value)
	{
		tool_poly_n_is_checked(value.m_vertex_cnt);
		tmfi_tool_poly_regular().IsChecked(value.m_regular);
		tmfi_tool_poly_vertex_up().IsChecked(value.m_vertex_up);
		tmfi_tool_poly_closed().IsChecked(value.m_end_closed);
		tmfi_tool_poly_clockwise().IsChecked(value.m_clockwise);
	}

	// 多角形ツールのメニューのチェックをつける.
	void MainPage::tool_poly_n_is_checked(const uint32_t value)
	{
		rmfi_tool_poly_line().IsChecked(value == 2);
		rmfi_tool_poly_tri().IsChecked(value == 3);
		rmfi_tool_poly_quad().IsChecked(value == 4);
		rmfi_tool_poly_pent().IsChecked(value == 5);
		rmfi_tool_poly_hexa().IsChecked(value == 6);
		rmfi_tool_poly_hept().IsChecked(value == 7);
		rmfi_tool_poly_octa().IsChecked(value == 8);
		rmfi_tool_poly_nona().IsChecked(value == 9);
		rmfi_tool_poly_deca().IsChecked(value == 10);
	}

	// 作図ツールの状態を読み込む.
	void MainPage::tool_read(DataReader const& dt_reader)
	{
		m_tool_draw = static_cast<DRAW_TOOL>(dt_reader.ReadUInt32());
		dt_read(m_tool_poly, dt_reader);
		//m_pile_up_vert = dt_reader.ReadSingle();
	}

	// 作図ツールの状態を書き込む.
	void MainPage::tool_write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_tool_draw));
		dt_write(m_tool_poly, dt_writer);
		//dt_writer.WriteSingle(m_pile_up_vert);
	}

}