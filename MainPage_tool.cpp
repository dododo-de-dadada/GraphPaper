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
	void MainPage::tool_bezi_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::BEZI;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u���~�v���I�����ꂽ.
	void MainPage::tool_elli_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::ELLI;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u�����v���I�����ꂽ.
	void MainPage::tool_line_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::LINE;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u�l�ւ�`�v���I�����ꂽ.
	void MainPage::tool_quad_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::QUAD;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u�l�ւ�`�v���I�����ꂽ.
	void MainPage::tool_poly_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::QUAD;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u���`�v���I�����ꂽ.
	void MainPage::tool_rect_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::RECT;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u�p�ە��`�v���I�����ꂽ.
	void MainPage::tool_rrect_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::RRCT;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u�}�`��I���v���I�����ꂽ.
	void MainPage::tool_select_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::SELECT;
		pointer_state(STATE_TRAN::BEGIN);
		pointer_shape(nullptr);
		pointer_anchor(ANCH_WHICH::ANCH_OUTSIDE);
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u������v���I�����ꂽ.
	void MainPage::tool_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::TEXT;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

	// ��}���j���[�́u��K�v���I�����ꂽ.
	void MainPage::tool_ruler_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_tool_draw = TOOL_DRAW::RULER;
		// ��}�c�[�����X�e�[�^�X�o�[�Ɋi�[����.
		stbar_set_draw();
		pointer_set();
	}

}