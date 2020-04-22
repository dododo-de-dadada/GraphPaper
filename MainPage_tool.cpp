//-------------------------------
// MainPage_tool.cpp
// ��}�c�[��
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ��}���j���[�́u�Ȑ��v���I�����ꂽ.
	void MainPage::rmfi_tool_bezi_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::BEZI;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u���~�v���I�����ꂽ.
	void MainPage::rmfi_tool_elli_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::ELLI;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u�����v���I�����ꂽ.
	void MainPage::rmfi_tool_line_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::LINE;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u�l�ւ�`�v���I�����ꂽ.
	void MainPage::rmfi_tool_quad_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::QUAD;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u���`�v���I�����ꂽ.
	void MainPage::rmfi_tool_rect_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::RECT;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u�p�ە��`�v���I�����ꂽ.
	void MainPage::rmfi_tool_rrect_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::RRCT;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u�}�`��I���v���I�����ꂽ.
	void MainPage::rmfi_tool_select_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::SELECT;
		pointer_state(STATE_TRAN::BEGIN);
		pointer_shape(nullptr);
		pointer_anchor(ANCH_WHICH::ANCH_OUTSIDE);
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u������v���I�����ꂽ.
	void MainPage::rmfi_tool_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::TEXT;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u������v���I�����ꂽ.
	void MainPage::rmfi_tool_scale_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_draw_tool = DRAW_TOOL::SCALE;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

}