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
	void MainPage::rmfi_tool_bezi_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_tool_shape = TOOL_BEZI;
		stat_set_tool();
		set_pointer();
	}

	// 図形メニューの「だ円」が選択された.
	void MainPage::rmfi_tool_elli_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_tool_shape = TOOL_ELLI;
		stat_set_tool();
		set_pointer();
	}

	// 図形メニューの「直線」が選択された.
	void MainPage::rmfi_tool_line_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_tool_shape = TOOL_LINE;
		stat_set_tool();
		set_pointer();
	}

	// 図形メニューの「四へん形」が選択された.
	void MainPage::rmfi_tool_quad_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_tool_shape = TOOL_QUAD;
		stat_set_tool();
		set_pointer();
	}

	// 図形メニューの「方形」が選択された.
	void MainPage::rmfi_tool_rect_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_tool_shape = TOOL_RECT;
		stat_set_tool();
		set_pointer();
	}

	// 図形メニューの「角丸方形」が選択された.
	void MainPage::rmfi_tool_rrect_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_tool_shape = TOOL_RRECT;
		stat_set_tool();
		set_pointer();
	}

	// 図形メニューの「図形を選択」が選択された.
	void MainPage::rmfi_tool_select_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_tool_shape = TOOL_SELECT;
		m_press_state = S_TRAN::BEGIN;
		m_press_shape = nullptr;
		m_press_anchor = ANCH_OUTSIDE;
		stat_set_tool();
		set_pointer();
	}

	// 図形メニューの「文字列」が選択された.
	void MainPage::rmfi_tool_text_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_tool_shape = TOOL_TEXT;
		stat_set_tool();
		set_pointer();
	}

}