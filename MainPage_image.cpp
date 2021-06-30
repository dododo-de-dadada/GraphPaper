//-------------------------------
// MainPage_image.cpp
// �摜
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �摜���j���[�́u�c�����ς��Ȃ��v���I�����ꂽ.
	void MainPage::image_keep_aspect_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		m_sheet_main.m_bm_keep_aspect = !m_sheet_main.m_bm_keep_aspect;
		ustack_push_set<UNDO_OP::BM_KEEP>(&m_sheet_main, m_sheet_main.m_bm_keep_aspect);
	}

	// �摜���j���[�́u�s�����x...�v���I�����ꂽ.
	IAsyncAction MainPage::image_opac_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		co_return;
	}

}