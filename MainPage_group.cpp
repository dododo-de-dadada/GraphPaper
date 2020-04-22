//-------------------------------
// MainPage_group.cpp
// �O���[�v���ƃO���[�v�̉���
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �ҏW���j���[�́u�O���[�v���v���I�����ꂽ.
	void MainPage::group_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_list_selected <= 1) {
			return;
		}
		S_LIST_T list_selected;
		s_list_selected<Shape>(m_list_shapes, list_selected);
		ShapeGroup* g = new ShapeGroup();
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		undo_push_append(g);
		for (const auto s : list_selected) {
			if (m_mutex_summary.load(std::memory_order_acquire)) {
			//if (m_summary_visible) {
				summary_remove(s);
			}
			// �}�`���폜����, ���̑�����X�^�b�N�ɐς�.
			undo_push_remove(s);
			// �}�`���O���[�v�}�`�ɒǉ�����, ���̑�����X�^�b�N�ɐς�.
			undo_push_append(g, s);
		}
		// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
		undo_push_null();
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		edit_menu_enable();
		page_draw();
		if (m_mutex_summary.load(std::memory_order_acquire)) {
		//if (m_summary_visible) {
			summary_append(g);
		}
	}

	// �ҏW���j���[�́u�O���[�v�̉����v���I�����ꂽ.
	void MainPage::ungroup_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�O���[�v�}�`�̃��X�g�𓾂�.
		S_LIST_T list_group;
		s_list_selected<ShapeGroup>(m_list_shapes, list_group);
		if (list_group.empty()) {
			// ����ꂽ���X�g����̏ꍇ,
			// �I������.
			return;
		}
		// ����ꂽ���X�g�̊e�O���[�v�}�`�ɂ��Ĉȉ����J��Ԃ�.
		for (auto t : list_group) {
			uint32_t i = 0;
			if (m_mutex_summary.load(std::memory_order_acquire)) {
			//if (m_summary_visible) {
				i = summary_remove(t);
			}
			auto g = static_cast<ShapeGroup*>(t);
			while (g->m_list_grouped.empty() != true) {
				// �O���[�v�����ꂽ�}�`�̃��X�g����ŏ��̐}�`�𓾂�.
				auto s = g->m_list_grouped.front();
				if (s->is_deleted()) {
					// �}�`�̏����t���O�������Ă���ꍇ,
					// �ȉ��𖳎�����.
					continue;
				}
				if (m_mutex_summary.load(std::memory_order_acquire)) {
				//if (m_summary_visible) {
					// �}�`���ꗗ�ɑ}������.
					summary_insert(s, i++);
				}
				// �}�`���O���[�v����폜����, ���̑�����X�^�b�N�ɐς�.
				undo_push_remove(g, s);
				// �}�`��}�`�̒��O�ɑ}������.
				undo_push_insert(s, g);
				//t = s;
			}
			// �}�`���폜����, ���̑�����X�^�b�N�ɐς�.
			undo_push_remove(g);
		}
		list_group.clear();
		// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
		undo_push_null();
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		edit_menu_enable();
		page_draw();
	}

}