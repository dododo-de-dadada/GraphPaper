//-------------------------------
// MainPage_image.cpp
// 画像
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 画像メニューの「縦横比を変えない」が選択された.
	void MainPage::image_keep_aspect_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		m_sheet_main.m_bm_keep_aspect = !m_sheet_main.m_bm_keep_aspect;
		ustack_push_set<UNDO_OP::BM_KEEP>(&m_sheet_main, m_sheet_main.m_bm_keep_aspect);
	}

	// 画像メニューの「不透明度...」が選択された.
	IAsyncAction MainPage::image_opac_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		co_return;
	}

}