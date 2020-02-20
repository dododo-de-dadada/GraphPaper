//-------------------------------
// MainPage_group.cpp
// グループ化とグループ化の解除
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 編集メニューの「グループ化」が選択された.
	void MainPage::mfi_group_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		S_LIST_T sel_list;
		s_list_select<Shape>(m_list_shapes, sel_list);
		if (sel_list.size() == 0) {
			return;
		}
		ShapeGroup* g = new ShapeGroup();
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		undo_push_append(g);
		for (const auto s : sel_list) {
			if (m_summary_visible) {
				summary_remove(s);
			}
			undo_push_remove(s);
			m_stack_undo.push_back(new UndoAppendG(g, s));
		}
		undo_push_null();
		enable_undo_menu();
		enable_edit_menu();
		draw_page();
		if (m_summary_visible) {
			summary_append(g);
		}
	}

	// 編集メニューの「グループの解除」が選択された.
	void MainPage::mfi_ungroup_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		// 選択されたグループ図形をすべて得る.
		S_LIST_T grp_list;
		s_list_select<ShapeGroup>(m_list_shapes, grp_list);
		if (grp_list.size() == 0) {
			return;
		}
		// 得られた各グループ図形について.
		for (auto t : grp_list) {
			//if (t->is_deleted()) {
			//	continue;
			//}
			uint32_t i = 0;
			if (m_summary_visible) {
				i = summary_remove(t);
			}
			// グループに含まれる各図形について.
			auto g = static_cast<ShapeGroup*>(t);
			while (g->m_grp_list.size() > 0) {
				auto s = g->m_grp_list.front();
				if (s->is_deleted()) {
					continue;
				}
				if (m_summary_visible) {
					// 図形を一覧に挿入する.
					summary_insert(s, i++);
				}
				// 図形をグループから取り除く.
				undo_push_remove(g, s);
				//m_stack_undo.push_back(new UndoRemoveG(g, s));
				// 図形を図形の直前に挿入する.
				undo_push_insert(s, g);
				//t = s;
			}
			// グループ図形を削除する.
			undo_push_remove(g);
		}
		grp_list.clear();
		undo_push_null();
		enable_undo_menu();
		enable_edit_menu();
		draw_page();
	}

}