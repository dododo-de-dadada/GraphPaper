//-------------------------------
// MainPage_tool.cpp
// ��}�c�[���̐ݒ�
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ��}���j���[�́u�Ȑ��v���I�����ꂽ.
	void MainPage::rmfi_tool_bezi_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::TOOL_BEZI;
		status_set_draw();
		set_pointer();
	}

	// ��}���j���[�́u���~�v���I�����ꂽ.
	void MainPage::rmfi_tool_elli_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::TOOL_ELLI;
		status_set_draw();
		set_pointer();
	}

	// ��}���j���[�́u�����v���I�����ꂽ.
	void MainPage::rmfi_tool_line_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::TOOL_LINE;
		status_set_draw();
		set_pointer();
	}

	// ��}���j���[�́u�l�ւ�`�v���I�����ꂽ.
	void MainPage::rmfi_tool_quad_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::TOOL_QUAD;
		status_set_draw();
		set_pointer();
	}

	// ��}���j���[�́u���`�v���I�����ꂽ.
	void MainPage::rmfi_tool_rect_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::TOOL_RECT;
		status_set_draw();
		set_pointer();
	}

	// ��}���j���[�́u�p�ە��`�v���I�����ꂽ.
	void MainPage::rmfi_tool_rrect_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::TOOL_RRECT;
		status_set_draw();
		set_pointer();
	}

	// ��}���j���[�́u�}�`��I���v���I�����ꂽ.
	void MainPage::rmfi_tool_select_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::TOOL_SELECT;
		m_press_state = S_TRAN::BEGIN;
		m_press_shape = nullptr;
		m_press_anchor = ANCH_WHICH::ANCH_OUTSIDE;
		status_set_draw();
		set_pointer();
	}

	// ��}���j���[�́u������v���I�����ꂽ.
	void MainPage::rmfi_tool_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::TOOL_TEXT;
		status_set_draw();
		set_pointer();
	}

	// ��}���j���[�́u������v���I�����ꂽ.
	void MainPage::rmfi_tool_scale_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::TOOL_SCALE;
		status_set_draw();
		set_pointer();
	}

}