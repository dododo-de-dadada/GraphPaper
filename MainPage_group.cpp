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
		if (m_cnt_selected <= 1) {
			return;
		}
		S_LIST_T s_list;
		s_list_selected<Shape>(m_list_shapes, s_list);
		ShapeGroup* g = new ShapeGroup();
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		unselect_all();
		undo_push_append(g);
		for (const auto s : s_list) {
			// �}�`�ꗗ�̔r�����䂪 true �����肷��.
			if (m_smry_atomic.load(std::memory_order_acquire)) {
				smry_remove(s);
			}
			// �}�`���폜����, ���̑�����X�^�b�N�ɐς�.
			undo_push_remove(s);
			// �}�`���O���[�v�}�`�ɒǉ�����, ���̑�����X�^�b�N�ɐς�.
			undo_push_append(g, s);
		}
		undo_push_select(g);
		// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
		undo_push_null();
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		edit_menu_enable();
		sheet_draw();
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			smry_append(g);
		}
	}

	// �ҏW���j���[�́u�O���[�v�̉����v���I�����ꂽ.
	void MainPage::ungroup_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�O���[�v�}�`�̃��X�g�𓾂�.
		S_LIST_T g_list;
		s_list_selected<ShapeGroup>(m_list_shapes, g_list);
		if (g_list.empty()) {
			// ����ꂽ���X�g����̏ꍇ,
			// �I������.
			return;
		}
		unselect_all();
		// ����ꂽ���X�g�̊e�O���[�v�}�`�ɂ��Ĉȉ����J��Ԃ�.
		for (auto t : g_list) {
			uint32_t i = 0;
			// �}�`�ꗗ�̔r�����䂪 true �����肷��.
			if (m_smry_atomic.load(std::memory_order_acquire)) {
				i = smry_remove(t);
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
				// �}�`�ꗗ�̔r�����䂪 true �����肷��.
				if (m_smry_atomic.load(std::memory_order_acquire)) {
					// �}�`���ꗗ�ɑ}������.
					smry_insert(s, i++);
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
		g_list.clear();
		// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
		undo_push_null();
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		edit_menu_enable();
		sheet_draw();
	}

}