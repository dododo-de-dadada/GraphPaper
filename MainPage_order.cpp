//-------------------------------
// MainPage_order.cpp
// �}�`�̕��ёւ�
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	constexpr bool SEND_TO_BACK = true;
	using SEND_BACKWARD = SHAPE_LIST::iterator;
	using BRING_FORWARD = SHAPE_LIST::reverse_iterator;
	constexpr bool BRING_TO_FRONT = false;

	// �ҏW���j���[�́u�O�ʂɈړ��v���I�����ꂽ.
	void MainPage::order_bring_forward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
		order_swap<BRING_FORWARD>();
		status_bar_set_pos();
	}

	// �ҏW���j���[�́u�őO�ʂɈړ��v���I�����ꂽ.
	void MainPage::order_bring_to_front_click(IInspectable const&, RoutedEventArgs const&)
	{
		SHAPE_LIST slist;	// �I�����ꂽ�}�`�̃��X�g
		slist_get_selected<Shape>(m_main_page.m_shape_list, slist);
		if (slist.size() > 0) {
			undo_push_null();
			// �őO�� (���X�g�ł͖���) �Ɉړ�
			for (Shape* const s : slist) {
				if (summary_is_visible()) {
					summary_remove(s);
					summary_append(s);
				}
				undo_push_remove(s);
				undo_push_insert(s, nullptr);
			}
		}
		slist.clear();
		//undo_menu_is_enabled();
		xcvd_menu_is_enabled();
		main_draw();
		status_bar_set_pos();
	}

	// �I�����ꂽ�}�`�����̐}�`�Ɠ���ւ���.
	template void MainPage::order_swap<BRING_FORWARD>(void);

	// �I�����ꂽ�}�`��O�̐}�`�Ɠ���ւ���.
	template void MainPage::order_swap<SEND_BACKWARD>(void);

	// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
	// T	T �� iterator �̏ꍇ�͔w�ʂ̐}�`�Ɠ���ւ�, reverse_iterator �̏ꍇ�͑O�ʂ̐}�`�Ɠ���ւ���. 
	template<typename T> void MainPage::order_swap(void)
	{
		T it_end;	// �I�[
		T it_src;	// ������
		if constexpr (std::is_same<T, BRING_FORWARD>::value) {
			it_end = m_main_page.m_shape_list.rend();
			it_src = m_main_page.m_shape_list.rbegin();
		}
		else {
			if constexpr (std::is_same<T, SEND_BACKWARD>::value) {
				it_end = m_main_page.m_shape_list.end();
				it_src = m_main_page.m_shape_list.begin();
			}
			else {
				throw winrt::hresult_not_implemented();
				return;
			}
		}
		// �I������Ă��Ȃ��}�`�̒�����ŏ��̐}�`�𓾂�.
		for (;;) {
			// ���̐}�`���Ȃ������肷��.
			if (it_src == it_end) {
				return;
			}
			// �����t���O���I���t���O�������Ă��Ȃ������肷��.
			const Shape* s = *it_src;
			if (!s->is_deleted() && !s->is_selected()) {
				break;
			}
			it_src++;
		}
		bool done = false;	// �����ς݂����肷��
		undo_push_null();
		for (;;) {
			// �����������q�������攽���q�Ɋi�[����,
			// �����������q���C���N�������g����.
			T it_dst = it_src++;
			// �폜����ĂȂ����̐}�`�𓾂�.
			for (;;) {
				// ���̐}�`���Ȃ������肷��.
				if (it_src == it_end) {
					// �����ς݂����肷��
					if (done) {
						//undo_menu_is_enabled();
						xcvd_menu_is_enabled();
						main_draw();
					}
					return;
				}
				// �������̍폜�t���O�������ĂȂ������肷��.
				if (!(*it_src)->is_deleted()) {
					break;
				}
				it_src++;
			}
			// ���̐}�`���I������ĂȂ��ꍇ,
			Shape* const s = *it_src;
			if (!s->is_selected()) {
				continue;
			}
			Shape* const t = *it_dst;
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_order(s, t);
			}
			undo_push_order(s, t);
			// �����ς݂ɂ���.
			done = true;
		}
	}

	// �ҏW���j���[�́u�ЂƂw�ʂɈړ��v���I�����ꂽ.
	void MainPage::order_send_backward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
		order_swap<SEND_BACKWARD>();
		status_bar_set_pos();
	}

	// �ҏW���j���[�́u�Ŕw�ʂɈړ��v���I�����ꂽ.
	void MainPage::order_send_to_back_click(IInspectable const&, RoutedEventArgs const&)
	{
		SHAPE_LIST slist;	// �I�����ꂽ�}�`�̃��X�g
		slist_get_selected<Shape>(m_main_page.m_shape_list, slist);
		if (slist.size() > 0) {
			undo_push_null();
			// �Ŕw�� (���X�g�ł͐擪) �Ɉړ�
			uint32_t i = 0;
			Shape* const s = slist_front(m_main_page.m_shape_list);
			for (Shape* const t : slist) {
				if (summary_is_visible()) {
					summary_remove(t);
					summary_insert_at(t, i++);
				}
				undo_push_remove(t);
				undo_push_insert(t, s);
			}
		}
		slist.clear();
		//undo_menu_is_enabled();
		xcvd_menu_is_enabled();
		main_draw();
		status_bar_set_pos();
	}

}