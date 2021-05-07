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
	void MainPage::tool_bezi_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::BEZI;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「だ円」が選択された.
	void MainPage::tool_elli_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::ELLI;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「直線」が選択された.
	void MainPage::tool_line_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::LINE;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「四へん形」が選択された.
	void MainPage::tool_quad_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::QUAD;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「四へん形」が選択された.
	void MainPage::tool_poly_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::QUAD;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「方形」が選択された.
	void MainPage::tool_rect_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::RECT;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「角丸方形」が選択された.
	void MainPage::tool_rrect_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::RRCT;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「図形を選択」が選択された.
	void MainPage::tool_select_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::SELECT;
		pointer_state(STATE_TRAN::BEGIN);
		pointer_shape(nullptr);
		pointer_anchor(ANCH_WHICH::ANCH_OUTSIDE);
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「文字列」が選択された.
	void MainPage::tool_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::TEXT;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

	// 作図メニューの「定規」が選択された.
	void MainPage::tool_ruler_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::RULER;
		// 作図ツールをステータスバーに格納する.
		stbar_set_draw();
		pointer_set();
	}

}