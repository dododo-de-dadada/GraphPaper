//-------------------------------
// MainPage_tool.cpp
// 作図ツール
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 作図メニューの「曲線」が選択された.
	void MainPage::rmfi_tool_bezi_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::BEZI;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「だ円」が選択された.
	void MainPage::rmfi_tool_elli_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::ELLI;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「直線」が選択された.
	void MainPage::rmfi_tool_line_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::LINE;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「四へん形」が選択された.
	void MainPage::rmfi_tool_quad_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::QUAD;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「方形」が選択された.
	void MainPage::rmfi_tool_rect_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::RECT;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「角丸方形」が選択された.
	void MainPage::rmfi_tool_rrect_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::RRCT;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「図形を選択」が選択された.
	void MainPage::rmfi_tool_select_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::SELECT;
		pointer_state(STATE_TRAN::BEGIN);
		pointer_shape(nullptr);
		pointer_anchor(ANCH_WHICH::ANCH_OUTSIDE);
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「文字列」が選択された.
	void MainPage::rmfi_tool_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::TEXT;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「文字列」が選択された.
	void MainPage::rmfi_tool_scale_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::SCALE;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

}