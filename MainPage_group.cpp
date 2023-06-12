//-------------------------------
// MainPage_group.cpp
// �O���[�v���ƃO���[�v�̉���
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	// �ҏW���j���[�́u�O���[�v���v���I�����ꂽ.
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
			// �}�`�̏����t���O�������Ă��邩���肷��.
			if (s->is_deleted()) {
				continue;
			}
			// �ꗗ���\������Ă邩���肷��.
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
		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			summary_append(g);
		}
		status_bar_set_pos();
	}

	// �ҏW���j���[�́u�O���[�v�̉����v���I�����ꂽ.
	void MainPage::ungroup_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�O���[�v�}�`�̃��X�g�𓾂�, ���X�g���󂩔��肷��.
		SHAPE_LIST group_list;
		slist_get_selected<ShapeGroup>(m_main_sheet.m_shape_list, group_list);
		if (group_list.empty()) {
			return;
		}
		undo_push_null();
		unselect_shape_all();
		// ����ꂽ���X�g�̊e�O���[�v�}�`�ɂ��Ĉȉ����J��Ԃ�.
		for (auto t : group_list) {
			auto g = static_cast<ShapeGroup*>(t);
			uint32_t i = 0;
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				i = summary_remove(t);
			}
			const auto at = slist_next(m_main_sheet.m_shape_list, g);
			// �܂����X�g����O���[�v���͂���.
			// ���̎��_��, �q�v�f�ɂ͍폜�t���O������.
			undo_push_remove(g);
			while (!g->m_list_grouped.empty()) {
				// �O���[�v�����ꂽ�}�`�̃��X�g����ŏ��̐}�`�𓾂�.
				auto s = g->m_list_grouped.front();
				// �ꗗ���\������Ă邩���肷��.
				if (summary_is_visible()) {
					summary_insert_at(s, i++);
					summary_select(s);
				}
				// ��苎�����}�`��}������.
				// �O���[�v�}�`����擪�̐}�`����苎��.
				undo_push_remove(g, s);
				// �}�`���X�g���̂��̃O���[�v�}�`�̒��O��,
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