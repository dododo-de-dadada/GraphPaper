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
		ustack_push_set<UNDO_OP::BM_KEEP>(&m_sheet_main, !m_sheet_main.s_bm_keep_aspect);
	}

	// 画像メニューの「縦横比を変えない」が選択された.
	void MainPage::image_resize_origin_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		for (auto s : m_list_shapes) {
			if (s->is_deleted() || !s->is_selected() || typeid(*s) != typeid(ShapeBitmap)) {
				continue;
			}
			ShapeBitmap* b = static_cast<ShapeBitmap*>(s);
			ustack_push_anch(b, ANCH_TYPE::ANCH_SHEET);
			b->resize_origin();
		}
		ustack_push_null();
		sheet_draw();
	}

	// 画像メニューの「不透明度...」が選択された.
	IAsyncAction MainPage::image_opac_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		co_return;
	}

}