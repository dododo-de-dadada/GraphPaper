//-------------------------------
// MainPage_select.cpp
// �}�`�̑I��
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �ҏW���j���[�́u���ׂđI���v���I�����ꂽ.
	void MainPage::mfi_select_all_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		bool flag = false;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected()) {
				continue;
			}
			flag = true;
			undo_push_select(s);
		}
		if (flag == false) {
			return;
		}
		if (m_summary_visible) {
			summary_select_all();
		}
		redo_clear();
		enable_undo_menu();
		enable_edit_menu();
		draw_page();
	}

	// �͈͂Ɋ܂܂��}�`��I����, �܂܂�Ȃ��}�`�̑I������������.
	bool MainPage::select_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max)
	{
		bool flag = false;
		uint32_t i = 0u;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->in_area(a_min, a_max)) {
				if (s->is_selected() == false) {
					undo_push_select(s);
					if (m_summary_visible) {
						summary_select(i);
					}
					flag = true;
				}
			}
			else {
				if (s->is_selected()) {
					undo_push_select(s);
					if (m_summary_visible) {
						summary_unselect(i);
					}
					flag = true;
				}
			}
			i++;
		}
		return flag;
	}

	// ���̐}�`��I������.
	template <VirtualKeyModifiers M, VirtualKey K>
	void MainPage::select_next_shape(void)
	{
		if (m_press_shape_summary == nullptr) {
			auto s_prev = m_press_shape_prev;
			if (s_prev != nullptr && s_prev->is_selected()) {
				m_press_shape_summary = s_prev;
			}
			else {
				if (s_prev == nullptr) {
					if constexpr (K == VirtualKey::Down) {
						m_press_shape_summary = s_list_front(m_list_shapes);
					}
					if constexpr (K == VirtualKey::Up) {
						m_press_shape_summary = s_list_back(m_list_shapes);
					}
					m_press_shape_prev = m_press_shape_summary;
				}
				else {
					m_press_shape_summary = s_prev;
				}
				undo_push_select(m_press_shape_summary);
				enable_undo_menu();
				enable_edit_menu();
				draw_page();
				if constexpr (K == VirtualKey::Down) {
					if (m_summary_visible) {
						summary_select_head();
					}
				}
				if constexpr (K == VirtualKey::Up) {
					if (m_summary_visible) {
						summary_select_tail();
					}
				}
				return;
			}
		}
		if constexpr (K == VirtualKey::Down) {
			auto s = s_list_next(m_list_shapes, m_press_shape_summary);
			if (s != nullptr) {
				m_press_shape_summary = s;
				goto SEL;
			}
		}
		if constexpr (K == VirtualKey::Up) {
			auto s = s_list_prev(m_list_shapes, m_press_shape_summary);
			if (s != nullptr) {
				m_press_shape_summary = s;
				goto SEL;
			}
		}
		return;
	SEL:
		if constexpr (M == VirtualKeyModifiers::Shift) {
			select_range(m_press_shape_prev, m_press_shape_summary);
			redo_clear();
		}
		if constexpr (M == VirtualKeyModifiers::None) {
			m_press_shape_prev = m_press_shape_summary;
			unselect_all();
			undo_push_select(m_press_shape_summary);
			if (m_summary_visible) {
				summary_select(m_press_shape_summary);
			}
		}
		enable_undo_menu();
		enable_edit_menu();
		draw_page();
	}
	template void MainPage::select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::None, VirtualKey::Up>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();


	// �w�肵���͈͂ɂ���}�`��I������, �����łȂ��}�`�͑I�����Ȃ�.
	// s_from	�ŏ��̐}�`
	// s_to	�Ō�̐}�`
	// �߂�l	�I�����ύX���ꂽ true
	bool MainPage::select_range(Shape* const s_from, Shape* const s_to)
	{
		constexpr int BEGIN = 0;
		constexpr int NEXT = 1;
		constexpr int END = 2;
		auto flag = false;
		auto st = BEGIN;
		auto s_end = static_cast<Shape*>(nullptr);
		auto i = 0u;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			switch (st) {
			case BEGIN:
				if (s == s_from) {
					s_end = s_to;
					st = NEXT;
				}
				else if (s == s_to) {
					s_end = s_from;
					st = NEXT;
				}
				else {
			case END:
				if (s->is_selected()) {
					flag = true;
					undo_push_select(s);
					if (m_summary_visible) {
						summary_unselect(i);
					}
				}
				break;
				}
				// no break.
			case NEXT:
				if (s->is_selected() == false) {
					flag = true;
					undo_push_select(s);
					if (m_summary_visible) {
						summary_select(i);
					}
				}
				if (s == s_end) {
					st = END;
				}
				break;
			}
			i++;
		}
		return flag;
	}

	// �}�`��I������.
	void MainPage::select_shape(Shape* s, const VirtualKeyModifiers vk)
	{
		using winrt::Windows::UI::Xaml::Controls::ListViewItem;
		if (vk == VirtualKeyModifiers::Control) {
			// �R���g���[���L�[��������Ă���ꍇ,
			// �|�C���^�[�������ꂽ�}�`�̑I���𔽓]������.
			undo_push_select(s);
			enable_undo_menu();
			enable_edit_menu();
			draw_page();
			if (s->is_selected()) {
				// �����ꂽ�}�`���I������Ă���ꍇ,
				// �����ꂽ�}�`�̑������y�[�W�̃p�l���Ɋi�[����.
				m_page_panel.set_to_shape(s);
				arrow_style_check_menu(m_page_panel.m_arrow_style);
				font_style_check_menu(m_page_panel.m_font_style);
				text_align_p_check_menu(m_page_panel.m_text_align_p);
				stroke_style_check_menu(m_page_panel.m_stroke_style);
				text_align_t_check_menu(m_page_panel.m_text_align_t);
				if (m_summary_visible) {
					summary_select(s);
				}
			}
			else {
				if (m_summary_visible) {
					summary_unselect(s);
				}
			}
			m_press_shape_prev = s;
		}
		else if (vk == VirtualKeyModifiers::Shift) {
			// �V�t�g�L�[��������Ă���ꍇ
			// �O��|�C���^�[�������ꂽ�}�`���獡�񉟂��ꂽ�}�`�܂ł�
			// �͈͂ɂ���}�`��I������, �����łȂ��}�`��I�����Ȃ�.
			if (m_press_shape_prev == nullptr) {
				m_press_shape_prev = m_list_shapes.front();
			}
			if (select_range(s, m_press_shape_prev)) {
				redo_clear();
				enable_undo_menu();
				enable_edit_menu();
				draw_page();
			}
			// �����ꂽ�}�`�̑������y�[�W�̃p�l���Ɋi�[����.
			m_page_panel.set_to_shape(s);
			arrow_style_check_menu(m_page_panel.m_arrow_style);
			font_style_check_menu(m_page_panel.m_font_style);
			text_align_p_check_menu(m_page_panel.m_text_align_p);
			stroke_style_check_menu(m_page_panel.m_stroke_style);
			text_align_t_check_menu(m_page_panel.m_text_align_t);
		}
		else {
			if (s->is_selected() == false) {
				// �V�t�g�L�[���R���g���[���L�[���ǂ����������Ă��Ȃ�,
				// �����ꂽ�}�`���I������Ă��Ȃ����, 
				// �}�`�̑I�������ׂĉ�����.
				// �����ꂽ�}�`�̑I���𔽓]����.
				// ���]��������ɖ߂�����X�^�b�N�ɐς�.
				// ��蒼���X�^�b�N��������, �y�[�W�Ɛ}�`��\������.
				unselect_all();
				undo_push_select(s);
				redo_clear();
				enable_undo_menu();
				enable_edit_menu();
				draw_page();
				arrow_style_check_menu(m_page_panel.m_arrow_style);
				font_style_check_menu(m_page_panel.m_font_style);
				text_align_p_check_menu(m_page_panel.m_text_align_p);
				stroke_style_check_menu(m_page_panel.m_stroke_style);
				text_align_t_check_menu(m_page_panel.m_text_align_t);
				if (m_summary_visible) {
					summary_select(s);
				}
			}
			// �����ꂽ�}�`�̑������y�[�W�̃p�l���Ɋi�[����.
			m_page_panel.set_to_shape(s);
			m_press_shape_prev = s;
		}
	}

	// �͈͂Ɋ܂܂��}�`�̑I���𔽓]����.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	bool MainPage::toggle_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max)
	{
		auto flag = false;
		uint32_t i = 0;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->in_area(a_min, a_max)) {
				undo_push_select(s);
				if (m_summary_visible) {
					if (s->is_selected() == false) {
						summary_select(i);
					}
					else {
						summary_unselect(i);
					}
				}
				flag = true;
			}
			i++;
		}
		return flag;
	}

	//	�}�`�̑I�������ׂĉ�������.
	//	t_range_only	�����͈͂̂݃t���O
	//	�߂�l	�I�����������ꂽ�}�`������ꍇ true, �Ȃ��ꍇ false
	//	�����͈͂̂݃t���O�������Ă���ꍇ, �����͈͂̑I���̂݉��������.
	//	�����͈͂̂݃t���O���Ȃ��ꍇ, �}�`�̑I���������͈͂̑I�����������������.
	bool MainPage::unselect_all(const bool t_range_only)
	{
		auto flag = false;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (t_range_only == false && s->is_selected()) {
				//	�����͈̓t���O���Ȃ�, ���}�`�̑I���t���O�������Ă���ꍇ,
				//	
				undo_push_select(s);
				flag = true;
			}
			DWRITE_TEXT_RANGE t_range;
			if (s->get_text_range(t_range) == false) {
				continue;
			}
			if (t_range.length > 0 || t_range.startPosition > 0) {
				undo_push_set<U_OP::TEXT_RANGE>(s, DWRITE_TEXT_RANGE{ 0, 0 });
				flag = true;
			}
		}
		if (m_summary_visible) {
			summary_unselect_all();
		}
		return flag;
	}

}