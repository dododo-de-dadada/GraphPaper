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
	void MainPage::mfi_group_click(IInspectable const&, RoutedEventArgs const&)
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
			//	図形を削除して, その操作をスタックに積む.
			undo_push_remove(s);
			m_stack_undo.push_back(new UndoAppendG(g, s));
		}
		undo_push_null();
		//	編集メニュー項目の使用の可否を設定する.
		enable_edit_menu();
		page_draw();
		if (m_summary_visible) {
			summary_append(g);
		}
	}

	// 編集メニューの「グループの解除」が選択された.
	void MainPage::mfi_ungroup_click(IInspectable const&, RoutedEventArgs const&)
	{
		//	選択されたグループ図形のリストを得る.
		S_LIST_T list_group;
		s_list_select<ShapeGroup>(m_list_shapes, list_group);
		if (list_group.empty()) {
			//	得られたリストが空の場合,
			//	終了する.
			return;
		}
		//	得られたリストの各グループ図形について以下を繰り返す.
		for (auto t : list_group) {
			uint32_t i = 0;
			if (m_summary_visible) {
				i = summary_remove(t);
			}
			auto g = static_cast<ShapeGroup*>(t);
			while (g->m_list_grouped.empty() == false) {
				//	グループ化された図形のリストから最初の図形を得る.
				auto s = g->m_list_grouped.front();
				if (s->is_deleted()) {
					//	図形の消去フラグが立っている場合,
					//	以下を無視する.
					continue;
				}
				if (m_summary_visible) {
					//	図形を一覧に挿入する.
					summary_insert(s, i++);
				}
				//	図形をグループから削除して, その操作をスタックに積む.
				undo_push_remove(g, s);
				//m_stack_undo.push_back(new UndoRemoveG(g, s));
				// 図形を図形の直前に挿入する.
				undo_push_insert(s, g);
				//t = s;
			}
			//	図形を削除して, その操作をスタックに積む.
			undo_push_remove(g);
		}
		list_group.clear();
		undo_push_null();
		//	編集メニュー項目の使用の可否を設定する.
		enable_edit_menu();
		page_draw();
	}

}