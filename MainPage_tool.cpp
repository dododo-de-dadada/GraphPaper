//-------------------------------
// MainPage_tool.cpp
// 図形ツールの設定
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 図形メニューの「曲線」が選択された.
	void MainPage::rmfi_draw_bezi_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_BEZI;
		stat_set_draw();
		set_pointer();
	}

	// 図形メニューの「だ円」が選択された.
	void MainPage::rmfi_draw_elli_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_ELLI;
		stat_set_draw();
		set_pointer();
	}

	// 図形メニューの「直線」が選択された.
	void MainPage::rmfi_draw_line_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_LINE;
		stat_set_draw();
		set_pointer();
	}

	// 図形メニューの「四へん形」が選択された.
	void MainPage::rmfi_draw_quad_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_QUAD;
		stat_set_draw();
		set_pointer();
	}

	// 図形メニューの「方形」が選択された.
	void MainPage::rmfi_draw_rect_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_RECT;
		stat_set_draw();
		set_pointer();
	}

	// 図形メニューの「角丸方形」が選択された.
	void MainPage::rmfi_draw_rrect_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_RRECT;
		stat_set_draw();
		set_pointer();
	}

	// 図形メニューの「図形を選択」が選択された.
	void MainPage::rmfi_tool_select_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_SELECT;
		m_press_state = S_TRAN::BEGIN;
		m_press_shape = nullptr;
		m_press_anchor = ANCH_OUTSIDE;
		stat_set_draw();
		set_pointer();
	}

	// 図形メニューの「文字列」が選択された.
	void MainPage::rmfi_draw_text_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_TEXT;
		stat_set_draw();
		set_pointer();
	}

	// 図形メニューの「文字列」が選択された.
	void MainPage::rmfi_draw_scale_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_SCALE;
		stat_set_draw();
		set_pointer();
	}

}