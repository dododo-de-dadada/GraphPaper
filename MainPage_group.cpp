//-------------------------------
// MainPage_group.cpp
// グループ化とグループの解除
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::RoutedEventArgs;

	// 編集メニューの「グループ化」が選択された.
	void MainPage::group_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_list_sel_cnt <= 1) {
			return;
		}
		SHAPE_LIST slist;
		slist_get_selected<Shape>(m_main_sheet.m_shape_list, slist);
		ShapeGroup* g = new ShapeGroup();
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		unselect_all();
		ustack_push_append(g);
		for (const auto s : slist) {
			// 図形の消去フラグが立っているか判定する.
			if (s->is_deleted()) {
				continue;
			}
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_remove(s);
			}
			ustack_push_remove(s);
			ustack_push_append(g, s);
		}
		ustack_push_select(g);
		ustack_push_null();
		xcvd_is_enabled();
		sheet_draw();
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_append(g);
		}
	}

	// 編集メニューの「グループの解除」が選択された.
	void MainPage::ungroup_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択されたグループ図形のリストを得て, リストが空か判定する.
		SHAPE_LIST g_list;
		slist_get_selected<ShapeGroup>(m_main_sheet.m_shape_list, g_list);
		if (g_list.empty()) {
			return;
		}
		unselect_all();
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
				}
				ustack_push_remove(g, s);
				ustack_push_insert(s, g);
				//t = s;
			}
			ustack_push_remove(g);
		}
		g_list.clear();
		ustack_push_null();
		xcvd_is_enabled();
		sheet_draw();
	}

}