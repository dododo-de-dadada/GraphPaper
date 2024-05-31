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
		slist_get_selected<SHAPE>(m_main_sheet.m_shape_list, slist);
		SHAPE_GROUP* g = new SHAPE_GROUP();
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		undo_push_null();
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
		select_shape(g);
		//unselect_all_shape();
		//undo_push_toggle(g);
		menu_is_enable();
		main_sheet_draw();
		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			summary_append(g);
		}
		status_bar_set_pointer();
	}

	// �ҏW���j���[�́u�O���[�v�̉����v���I�����ꂽ.
	void MainPage::ungroup_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�O���[�v�}�`�̃��X�g�𓾂�, ���X�g���󂩔��肷��.
		SHAPE_LIST group_list;
		slist_get_selected<SHAPE_GROUP>(m_main_sheet.m_shape_list, group_list);
		if (group_list.empty()) {
			return;
		}
		undo_push_null();
		unselect_all_shape();
		// ����ꂽ���X�g�̊e�O���[�v�}�`�ɂ��Ĉȉ����J��Ԃ�.
		for (auto t : group_list) {
			auto g = static_cast<SHAPE_GROUP*>(t);
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
				undo_push_toggle(s);
				//t = s;
			}
			//undo_push_remove(g);
		}
		menu_is_enable();
		main_sheet_draw();
		status_bar_set_pointer();
	}

}