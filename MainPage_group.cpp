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
		if (m_list_sel_cnt <= 1) {
			return;
		}
		SHAPE_LIST slist;
		slist_get_selected<Shape>(m_main_page.m_shape_list, slist);
		ShapeGroup* g = new ShapeGroup();
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		unselect_shape_all();
		undo_push_append(g);
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
		undo_push_select(g);
		undo_push_null();
		undo_menu_is_enabled();
		xcvd_menu_is_enabled();
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
		SHAPE_LIST g_list;
		slist_get_selected<ShapeGroup>(m_main_page.m_shape_list, g_list);
		if (g_list.empty()) {
			return;
		}
		unselect_shape_all();
		// 得られたリストの各グループ図形について以下を繰り返す.
		for (auto t : g_list) {
			uint32_t i = 0;
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				i = summary_remove(t);
			}
			auto g = static_cast<ShapeGroup*>(t);
			while (!g->m_list_grouped.empty()) {
				// グループ化された図形のリストから最初の図形を得る.
				auto s = g->m_list_grouped.front();
				// 図形の消去フラグが立っているか判定する.
				if (s->is_deleted()) {
					continue;
				}
				// 一覧が表示されてるか判定する.
				if (summary_is_visible()) {
					summary_insert_at(s, i++);
					summary_select(s);
				}
				// グループ図形から先頭の図形を取り去り,
				// 図形リスト中のそのグループ図形の直前に,
				// 取り去った図形を挿入する.
				undo_push_remove(g, s);
				undo_push_insert(s, g);
				undo_push_select(s);
				//t = s;
			}
			undo_push_remove(g);
		}
		undo_push_null();
		undo_menu_is_enabled();
		xcvd_menu_is_enabled();
		main_draw();
		status_bar_set_pos();
	}

}