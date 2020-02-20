//-------------------------------
// MainPage_group.cpp
// �O���[�v���ƃO���[�v���̉���
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �ҏW���j���[�́u�O���[�v���v���I�����ꂽ.
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

	// �ҏW���j���[�́u�O���[�v�̉����v���I�����ꂽ.
	void MainPage::mfi_ungroup_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		// �I�����ꂽ�O���[�v�}�`�����ׂē���.
		S_LIST_T grp_list;
		s_list_select<ShapeGroup>(m_list_shapes, grp_list);
		if (grp_list.size() == 0) {
			return;
		}
		// ����ꂽ�e�O���[�v�}�`�ɂ���.
		for (auto t : grp_list) {
			//if (t->is_deleted()) {
			//	continue;
			//}
			uint32_t i = 0;
			if (m_summary_visible) {
				i = summary_remove(t);
			}
			// �O���[�v�Ɋ܂܂��e�}�`�ɂ���.
			auto g = static_cast<ShapeGroup*>(t);
			while (g->m_grp_list.size() > 0) {
				auto s = g->m_grp_list.front();
				if (s->is_deleted()) {
					continue;
				}
				if (m_summary_visible) {
					// �}�`���ꗗ�ɑ}������.
					summary_insert(s, i++);
				}
				// �}�`���O���[�v�����菜��.
				undo_push_remove(g, s);
				//m_stack_undo.push_back(new UndoRemoveG(g, s));
				// �}�`��}�`�̒��O�ɑ}������.
				undo_push_insert(s, g);
				//t = s;
			}
			// �O���[�v�}�`���폜����.
			undo_push_remove(g);
		}
		grp_list.clear();
		undo_push_null();
		enable_undo_menu();
		enable_edit_menu();
		draw_page();
	}

}