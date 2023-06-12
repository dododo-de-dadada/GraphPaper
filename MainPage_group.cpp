//-------------------------------
// MainPage_group.cpp
// グループ化とグループの解除
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	// 編集メニューの「グループ化」が選択された.
	void MainPage::group_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_undo_select_cnt <= 1) {
			return;
		}
		SHAPE_LIST slist;
		slist_get_selected<Shape>(m_main_sheet.m_shape_list, slist);
		ShapeGroup* g = new ShapeGroup();
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		undo_push_null();
		unselect_shape_all();
		for (const auto s : slist) {
			// 図形の消去フラグが立っているか判定する.
			if (s->is_deleted()) {
				continue;
			}
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_remove(s);
			}
			undo_push_remove(s);
			undo_push_append(g, s);
		}
		undo_push_append(g);
		undo_push_select(g);
		menu_is_enable();
		main_draw();
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_append(g);
		}
		status_bar_set_pos();
	}

	// 編集メニューの「グループの解除」が選択された.
	void MainPage::ungroup_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択されたグループ図形のリストを得て, リストが空か判定する.
		SHAPE_LIST group_list;
		slist_get_selected<ShapeGroup>(m_main_sheet.m_shape_list, group_list);
		if (group_list.empty()) {
			return;
		}
		undo_push_null();
		unselect_shape_all();
		// 得られたリストの各グループ図形について以下を繰り返す.
		for (auto t : group_list) {
			auto g = static_cast<ShapeGroup*>(t);
			uint32_t i = 0;
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				i = summary_remove(t);
			}
			const auto at = slist_next(m_main_sheet.m_shape_list, g);
			// まずリストからグループをはずす.
			// この時点で, 子要素には削除フラグが立つ.
			undo_push_remove(g);
			while (!g->m_list_grouped.empty()) {
				// グループ化された図形のリストから最初の図形を得る.
				auto s = g->m_list_grouped.front();
				// 一覧が表示されてるか判定する.
				if (summary_is_visible()) {
					summary_insert_at(s, i++);
					summary_select(s);
				}
				// 取り去った図形を挿入する.
				// グループ図形から先頭の図形を取り去る.
				undo_push_remove(g, s);
				// 図形リスト中のそのグループ図形の直前に,
				undo_push_insert(s, at);
				undo_push_select(s);
				//t = s;
			}
			//undo_push_remove(g);
		}
		menu_is_enable();
		main_draw();
		status_bar_set_pos();
	}

}