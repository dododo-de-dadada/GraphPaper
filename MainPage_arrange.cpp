//-------------------------------
// MainPage_arrange.cpp
// �}�`���X�g�̗v�f�̕��ёւ�
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �I�����ꂽ�}�`�ɂ���, �����邢�͑O�̐}�`�Ɠ���ւ���.
	template<typename T>
	void MainPage::arrange_order(void)
	{
		T it_end;	// �I�[�̔����q
		T it_src;	// �������̔����q
		auto flag = false;
		if constexpr (std::is_same<T, S_LIST_T::reverse_iterator>::value) {
			it_end = m_list_shapes.rend();
			it_src = m_list_shapes.rbegin();
		}
		else {
			if constexpr (std::is_same<T, S_LIST_T::iterator>::value) {
				it_end = m_list_shapes.end();
				it_src = m_list_shapes.begin();
			}
			else {
				throw winrt::hresult_not_implemented();
			}
		}
		for (;;) {
			if ((*it_src)->is_deleted() == false) {
				break;
			}
			it_src++;
		}
		for (;;) {
			auto it_dst = it_src++;
			while (it_src != it_end) {
				if ((*it_src)->is_deleted() == false) {
					break;
				}
				it_src++;
			}
			if (it_src == it_end) {
				//	�������̔����q���I�[�̏ꍇ,
				//	���f����.
				break;
			}
			auto s = *it_src;	// �������̐}�`
			if (s->is_deleted()) {
				it_src++;
				continue;
			}
			// �������̔����q���w���}�`���I������Ă���,
			// ������̔����q���w���}�`���I������ĂȂ��ꍇ,
			// �����̐}�`�̑g��g���X�g�ɕۑ���, ����ւ���.
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
			undo_push_null();
			enable_undo_menu();
			enable_edit_menu();
			page_draw();
		}
	}
	template void MainPage::arrange_order<S_LIST_T::iterator>(void);
	template void MainPage::arrange_order<S_LIST_T::reverse_iterator>(void);

	// �I�����ꂽ�}�`���Ŕw�ʂ܂��͍őO�ʂɈړ�����.
	template<bool TO_BACK>
	void MainPage::arrange_to(void)
	{
		using winrt::Windows::UI::Xaml::Controls::ItemCollection;

		// �I�����ꂽ�}�`�����X�g�ɒǉ�����.
		S_LIST_T sel_list;	// �I�����ꂽ�}�`�̃��X�g
		s_list_select<Shape>(m_list_shapes, sel_list);
		if (sel_list.size() == 0) {
			return;
		}
		if constexpr (TO_BACK) {
			uint32_t i = 0;	// �}�`��}������ʒu
			//auto s_pos = S_LIST::front(m_list_shapes, i);
			auto s_pos = s_list_front(m_list_shapes);
			for (auto s : sel_list) {
				if (m_summary_visible) {
					summary_remove(s);
					summary_insert(s, i++);
				}
				undo_push_remove(s);
				undo_push_insert(s, s_pos);
			}
		}
		else {
			for (auto s : sel_list) {
				if (m_summary_visible) {
					summary_remove(s);
					summary_append(s);
				}
				undo_push_remove(s);
				undo_push_insert(s, nullptr);
			}
		}
		sel_list.clear();
		undo_push_null();
		enable_undo_menu();
		enable_edit_menu();
		page_draw();
	}
	template void MainPage::arrange_to<true>(void);
	template void MainPage::arrange_to<false>(void);

	// �ҏW���j���[�́u�O�ʂɈړ��v���I�����ꂽ.
	void MainPage::mfi_bring_forward_click(IInspectable const&, RoutedEventArgs const&)
	{
		arrange_order<S_LIST_T::reverse_iterator>();
	}

	// �ҏW���j���[�́u�őO�ʂɈړ��v���I�����ꂽ.
	void MainPage::mfi_bring_to_front_click(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto FRONT = false;
		arrange_to<FRONT>();
	}

	// �ҏW���j���[�́u�ЂƂw�ʂɈړ��v���I�����ꂽ.
	void MainPage::mfi_send_backward_click(IInspectable const&, RoutedEventArgs const&)
	{
		arrange_order<S_LIST_T::iterator>();
	}

	// �ҏW���j���[�́u�Ŕw�ʂɈړ��v���I�����ꂽ.
	void MainPage::mfi_send_to_back_click(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto BACK = true;
		arrange_to<BACK>();
	}

}