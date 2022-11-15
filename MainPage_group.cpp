//-------------------------------
// MainPage_group.cpp
// �O���[�v���ƃO���[�v�̉���
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::RoutedEventArgs;

	// �ҏW���j���[�́u�O���[�v���v���I�����ꂽ.
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
			// �}�`�̏����t���O�������Ă��邩���肷��.
			if (s->is_deleted()) {
				continue;
			}
			// �ꗗ���\������Ă邩���肷��.
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
		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			summary_append(g);
		}
	}

	// �ҏW���j���[�́u�O���[�v�̉����v���I�����ꂽ.
	void MainPage::ungroup_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�O���[�v�}�`�̃��X�g�𓾂�, ���X�g���󂩔��肷��.
		SHAPE_LIST g_list;
		slist_get_selected<ShapeGroup>(m_main_sheet.m_shape_list, g_list);
		if (g_list.empty()) {
			return;
		}
		unselect_all();
		// ����ꂽ���X�g�̊e�O���[�v�}�`�ɂ��Ĉȉ����J��Ԃ�.
		for (auto t : g_list) {
			uint32_t i = 0;
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				i = summary_remove(t);
			}
			auto g = static_cast<ShapeGroup*>(t);
			while (!g->m_list_grouped.empty()) {
				// �O���[�v�����ꂽ�}�`�̃��X�g����ŏ��̐}�`�𓾂�.
				auto s = g->m_list_grouped.front();
				// �}�`�̏����t���O�������Ă��邩���肷��.
				if (s->is_deleted()) {
					continue;
				}
				// �ꗗ���\������Ă邩���肷��.
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