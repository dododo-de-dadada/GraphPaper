//-------------------------------
// MainPage_arrange.cpp
// �}�`���X�g�̗v�f�̕��ёւ�
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
	// T	S_LIST_T::iterator �̏ꍇ�͔w�ʂ̐}�`�Ɠ���ւ�, S_LIST_T::reverse_iterator �̏ꍇ�͑O�ʂ̐}�`�Ɠ���ւ���. 
	template<typename T>
	void MainPage::arrange_order(void)
	{
		T it_end;	// �I�[
		T it_src;	// �����������q
		if constexpr (std::is_same<T, FORWARD>::value) {
			it_end = m_list_shapes.rend();
			it_src = m_list_shapes.rbegin();
		}
		else {
			if constexpr (std::is_same<T, BACKWARD>::value) {
				it_end = m_list_shapes.end();
				it_src = m_list_shapes.begin();
			}
			else {
				throw winrt::hresult_not_implemented();
			}
		}
		// �I������Ă��Ȃ��}�`�̒�����ŏ��̐}�`�𓾂�.
		for (;;) {
			if (it_src == it_end) {
				// �}�`���Ȃ��ꍇ
				return;
			}
			if ((*it_src)->is_deleted() == false && (*it_src)->is_selected() == false) {
				// �����t���O���Ȃ�, ���I���t���O���Ȃ��ꍇ,
				break;
			}
			it_src++;
		}
		// �����t���O����������.
		auto flag = false;
		for (;;) {
			// �����������q�������攽���q�Ɋi�[����,
			// �����������q���C���N�������g����.
			auto it_dst = it_src++;
			// ���̐}�`�𓾂�.
			for (;;) {
				if (it_src == it_end) {
					// ���̐}�`���Ȃ��ꍇ,
					if (flag == true) {
						// �����t���O�������Ă���ꍇ,
						undo_push_null();
						enable_edit_menu();
						page_draw();
					}
					return;
				}
				if ((*it_src)->is_deleted() == false) {
					break;
				}
				it_src++;
			}
			if ((*it_src)->is_selected() == false) {
				// ���̐}�`���I������ĂȂ��ꍇ,
				continue;
			}
			auto s = *it_src;
			auto t = *it_dst;
			if (m_summary_visible) {
				summary_arrange(s, t);
			}
			undo_push_arrange(s, t);
			// �����t���O�𗧂Ă�.
			flag = true;
		}
		/*
		for (;;) {
			if ((*it_src)->is_deleted() == false) {
				break;
			}
			it_src++;
		}
		for (;;) {
			// �����������q�������攽���Ɋi�[����, �����������q���C���N�������g����.
			// �����������q���I�[�łȂ�
			auto it_dst = it_src++;
			while (it_src != it_end) {
				if ((*it_src)->is_deleted() == false) {
					break;
				}
				it_src++;
			}
			if (it_src == it_end) {
				// �������̔����q���I�[�̏ꍇ,
				// ���f����.
				break;
			}
			auto s = *it_src;	// �������̐}�`
			if (s->is_deleted()) {
				// �����t���O�������Ă���ꍇ,
				// �������̔����q���C���N�������g����.
				// �ȉ��𖳎�����.
				it_src++;
				continue;
			}
			// �����������q���w���}�`���I������Ă���,
			// �����攽���q���w���}�`���I������ĂȂ��ꍇ,
			// �����̐}�`�����ւ���.
			auto t = *it_dst;	// ������̐}�`
			if (s->is_selected() && !t->is_selected()) {
				flag = true;
				if (m_summary_visible) {
					summary_arrange(s, t);
				}
				undo_push_arrange(s, t);
			}
		}
		if (flag == true) {
			// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
			undo_push_null();
			// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
			enable_edit_menu();
			page_draw();
		}
		*/
	}
	using BACKWARD = S_LIST_T::iterator;
	using FORWARD = S_LIST_T::reverse_iterator;
	template void MainPage::arrange_order<BACKWARD>(void);
	template void MainPage::arrange_order<FORWARD>(void);

	// �I�����ꂽ�}�`���Ŕw�ʂ܂��͍őO�ʂɈړ�����.
	// B	true �̏ꍇ�͍Ŕw��, false �̏ꍇ�͍őO�ʂɈړ�
	template<bool B>
	void MainPage::arrange_to(void)
	{
		using winrt::Windows::UI::Xaml::Controls::ItemCollection;

		S_LIST_T list_selected;
		s_list_selected<Shape>(m_list_shapes, list_selected);
		if (list_selected.size() == 0) {
			return;
		}
		if constexpr (B) {
			uint32_t i = 0;
			auto s_pos = s_list_front(m_list_shapes);
			for (auto s : list_selected) {
				if (m_summary_visible) {
					summary_remove(s);
					summary_insert(s, i++);
				}
				undo_push_remove(s);
				undo_push_insert(s, s_pos);
			}
		}
		else {
			for (auto s : list_selected) {
				if (m_summary_visible) {
					summary_remove(s);
					summary_append(s);
				}
				undo_push_remove(s);
				undo_push_insert(s, nullptr);
			}
		}
		list_selected.clear();
		undo_push_null();
		enable_edit_menu();
		page_draw();
	}
	constexpr auto BACK = true;
	template void MainPage::arrange_to<BACK>(void);
	constexpr auto FRONT = false;
	template void MainPage::arrange_to<FRONT>(void);

	// �ҏW���j���[�́u�O�ʂɈړ��v���I�����ꂽ.
	void MainPage::mfi_bring_forward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
		arrange_order<FORWARD>();
	}

	// �ҏW���j���[�́u�őO�ʂɈړ��v���I�����ꂽ.
	void MainPage::mfi_bring_to_front_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`���Ŕw�ʂ܂��͍őO�ʂɈړ�����.
		arrange_to<FRONT>();
	}

	// �ҏW���j���[�́u�ЂƂw�ʂɈړ��v���I�����ꂽ.
	void MainPage::mfi_send_backward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
		arrange_order<BACKWARD>();
	}

	// �ҏW���j���[�́u�Ŕw�ʂɈړ��v���I�����ꂽ.
	void MainPage::mfi_send_to_back_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`���Ŕw�ʂ܂��͍őO�ʂɈړ�����.
		arrange_to<BACK>();
	}

}