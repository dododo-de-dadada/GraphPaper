//-------------------------------
// MainPage_tool.cpp
// �}�`�c�[���̐ݒ�
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �}�`���j���[�́u�Ȑ��v���I�����ꂽ.
	void MainPage::rmfi_draw_bezi_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_BEZI;
		stat_set_draw();
		set_pointer();
	}

	// �}�`���j���[�́u���~�v���I�����ꂽ.
	void MainPage::rmfi_draw_elli_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_ELLI;
		stat_set_draw();
		set_pointer();
	}

	// �}�`���j���[�́u�����v���I�����ꂽ.
	void MainPage::rmfi_draw_line_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_LINE;
		stat_set_draw();
		set_pointer();
	}

	// �}�`���j���[�́u�l�ւ�`�v���I�����ꂽ.
	void MainPage::rmfi_draw_quad_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_QUAD;
		stat_set_draw();
		set_pointer();
	}

	// �}�`���j���[�́u���`�v���I�����ꂽ.
	void MainPage::rmfi_draw_rect_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_RECT;
		stat_set_draw();
		set_pointer();
	}

	// �}�`���j���[�́u�p�ە��`�v���I�����ꂽ.
	void MainPage::rmfi_draw_rrect_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_RRECT;
		stat_set_draw();
		set_pointer();
	}

	// �}�`���j���[�́u�}�`��I���v���I�����ꂽ.
	void MainPage::rmfi_tool_select_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_SELECT;
		m_press_state = S_TRAN::BEGIN;
		m_press_shape = nullptr;
		m_press_anchor = ANCH_OUTSIDE;
		stat_set_draw();
		set_pointer();
	}

	// �}�`���j���[�́u������v���I�����ꂽ.
	void MainPage::rmfi_draw_text_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_TEXT;
		stat_set_draw();
		set_pointer();
	}

	// �}�`���j���[�́u������v���I�����ꂽ.
	void MainPage::rmfi_draw_scale_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		m_draw_shape = DRAW_SCALE;
		stat_set_draw();
		set_pointer();
	}

}