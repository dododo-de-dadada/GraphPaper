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
		S_LIST_T s_list;
		s_list_selected<Shape>(m_list_shapes, s_list);
		ShapeGroup* g = new ShapeGroup();
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		unselect_all();
		undo_push_append(g);
		for (const auto s : s_list) {
			// 図形一覧の排他制御が true か判定する.
			if (m_smry_atomic.load(std::memory_order_acquire)) {
				smry_remove(s);
			}
			// 図形を削除して, その操作をスタックに積む.
			undo_push_remove(s);
			// 図形をグループ図形に追加して, その操作をスタックに積む.
			undo_push_append(g, s);
		}
		undo_push_select(g);
		// 一連の操作の区切としてヌル操作をスタックに積む.
		undo_push_null();
		// 編集メニュー項目の使用の可否を設定する.
		edit_menu_enable();
		sheet_draw();
		// 図形一覧の排他制御が true か判定する.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			smry_append(g);
		}
	}

	// 編集メニューの「グループの解除」が選択された.
	void MainPage::ungroup_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択されたグループ図形のリストを得る.
		S_LIST_T g_list;
		s_list_selected<ShapeGroup>(m_list_shapes, g_list);
		if (g_list.empty()) {
			// 得られたリストが空の場合,
			// 終了する.
			return;
		}
		unselect_all();
		// 得られたリストの各グループ図形について以下を繰り返す.
		for (auto t : g_list) {
			uint32_t i = 0;
			// 図形一覧の排他制御が true か判定する.
			if (m_smry_atomic.load(std::memory_order_acquire)) {
				i = smry_remove(t);
			}
			auto g = static_cast<ShapeGroup*>(t);
			while (g->m_list_grouped.empty() != true) {
				// グループ化された図形のリストから最初の図形を得る.
				auto s = g->m_list_grouped.front();
				if (s->is_deleted()) {
					// 図形の消去フラグが立っている場合,
					// 以下を無視する.
					continue;
				}
				// 図形一覧の排他制御が true か判定する.
				if (m_smry_atomic.load(std::memory_order_acquire)) {
					// 図形を一覧に挿入する.
					smry_insert(s, i++);
				}
				// 図形をグループから削除して, その操作をスタックに積む.
				undo_push_remove(g, s);
				// 図形を図形の直前に挿入する.
				undo_push_insert(s, g);
				//t = s;
			}
			// 図形を削除して, その操作をスタックに積む.
			undo_push_remove(g);
		}
		g_list.clear();
		// 一連の操作の区切としてヌル操作をスタックに積む.
		undo_push_null();
		// 編集メニュー項目の使用の可否を設定する.
		edit_menu_enable();
		sheet_draw();
	}

}