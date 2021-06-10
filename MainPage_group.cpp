//-------------------------------
// MainPage_group.cpp
// グループ化とグループの解除
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 編集メニューの「グループ化」が選択された.
	void MainPage::group_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_cnt_selected <= 1) {
			return;
		}
		SHAPE_LIST slist;
		slist_selected<Shape>(m_list_shapes, slist);
		ShapeGroup* g = new ShapeGroup();
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		unselect_all();
		undo_push_append(g);
		for (const auto s : slist) {
			// 図形の消去フラグが立っているか判定する.
			if (s->is_deleted()) {
				continue;
			}
			// 図形一覧の排他制御が true か判定する.
			if (m_summary_atomic.load(std::memory_order_acquire)) {
				summary_remove(s);
			}
			undo_push_remove(s);
			undo_push_append(g, s);
		}
		undo_push_select(g);
		undo_push_null();
		edit_menu_is_enabled();
		sheet_draw();
		// 図形一覧の排他制御が true か判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			summary_append(g);
		}
	}

	// 編集メニューの「グループの解除」が選択された.
	void MainPage::ungroup_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択されたグループ図形のリストを得て, リストが空か判定する.
		SHAPE_LIST g_list;
		slist_selected<ShapeGroup>(m_list_shapes, g_list);
		if (g_list.empty()) {
			return;
		}
		unselect_all();
		// 得られたリストの各グループ図形について以下を繰り返す.
		for (auto t : g_list) {
			uint32_t i = 0;
			// 図形一覧の排他制御が true か判定する.
			if (m_summary_atomic.load(std::memory_order_acquire)) {
				i = summary_remove(t);
			}
			auto g = static_cast<ShapeGroup*>(t);
			while (g->m_list_grouped.empty() != true) {
				// グループ化された図形のリストから最初の図形を得る.
				auto s = g->m_list_grouped.front();
				// 図形の消去フラグが立っているか判定する.
				if (s->is_deleted()) {
					continue;
				}
				// 図形一覧の排他制御が true か判定する.
				if (m_summary_atomic.load(std::memory_order_acquire)) {
					summary_insert(s, i++);
				}
				undo_push_remove(g, s);
				undo_push_insert(s, g);
				//t = s;
			}
			undo_push_remove(g);
		}
		g_list.clear();
		undo_push_null();
		edit_menu_is_enabled();
		sheet_draw();
	}

}