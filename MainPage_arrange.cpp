//-------------------------------
// MainPage_arrange.cpp
// �}�`�̕��ёւ�
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr auto BACK = true;
	using BACKWARD = S_LIST_T::iterator;
	using FORWARD = S_LIST_T::reverse_iterator;
	constexpr auto FRONT = false;

	// �ҏW���j���[�́u�O�ʂɈړ��v���I�����ꂽ.
	void MainPage::arrange_bring_forward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
		arrange_order<FORWARD>();
	}

	// �ҏW���j���[�́u�őO�ʂɈړ��v���I�����ꂽ.
	void MainPage::arrange_bring_to_front_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`���Ŕw�ʂ܂��͍őO�ʂɈړ�����.
		arrange_to<FRONT>();
	}

	// �I�����ꂽ�}�`��O�̐}�`�Ɠ���ւ���.
	template void MainPage::arrange_order<BACKWARD>(void);

	// �I�����ꂽ�}�`�����̐}�`�Ɠ���ւ���.
	template void MainPage::arrange_order<FORWARD>(void);

	// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
	// T	S_LIST_T::iterator �̏ꍇ�͔w�ʂ̐}�`�Ɠ���ւ�, S_LIST_T::reverse_iterator �̏ꍇ�͑O�ʂ̐}�`�Ɠ���ւ���. 
	template<typename T> void MainPage::arrange_order(void)
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
			if ((*it_src)->is_deleted() != true && (*it_src)->is_selected() != true) {
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
						edit_menu_enable();
						sheet_draw();
					}
					return;
				}
				if ((*it_src)->is_deleted() != true) {
					break;
				}
				it_src++;
			}
			if ((*it_src)->is_selected() != true) {
				// ���̐}�`���I������ĂȂ��ꍇ,
				continue;
			}
			auto s = *it_src;
			auto t = *it_dst;
			if (m_mutex_summary.load(std::memory_order_acquire)) {
			//if (m_summary_visible) {
				summary_arrange(s, t);
			}
			undo_push_arrange(s, t);
			// �����t���O�𗧂Ă�.
			flag = true;
		}
	}

	// �ҏW���j���[�́u�ЂƂw�ʂɈړ��v���I�����ꂽ.
	void MainPage::arrange_send_backward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
		arrange_order<BACKWARD>();
	}

	// �ҏW���j���[�́u�Ŕw�ʂɈړ��v���I�����ꂽ.
	void MainPage::arrange_send_to_back_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`���Ŕw�ʂ܂��͍őO�ʂɈړ�����.
		arrange_to<BACK>();
	}

	// �I�����ꂽ�}�`���Ŕw�ʂɈړ�����.
	template void MainPage::arrange_to<BACK>(void);

	// �I�����ꂽ�}�`���őO�ʂɈړ�����.
	template void MainPage::arrange_to<FRONT>(void);

	// �I�����ꂽ�}�`���Ŕw�ʂ܂��͍őO�ʂɈړ�����.
	// T	true �̏ꍇ�͍Ŕw��, false �̏ꍇ�͍őO�ʂɈړ�
	template<bool T> void MainPage::arrange_to(void)
	{
		using winrt::Windows::UI::Xaml::Controls::ItemCollection;

		S_LIST_T list_selected;
		s_list_selected<Shape>(m_list_shapes, list_selected);
		if (list_selected.size() == 0) {
			return;
		}
		if constexpr (T) {
			uint32_t i = 0;
			auto s_pos = s_list_front(m_list_shapes);
			for (auto s : list_selected) {
				if (m_mutex_summary.load(std::memory_order_acquire)) {
					summary_remove(s);
					summary_insert(s, i++);
				}
				undo_push_remove(s);
				undo_push_insert(s, s_pos);
			}
		}
		else {
			for (auto s : list_selected) {
				if (m_mutex_summary.load(std::memory_order_acquire)) {
					summary_remove(s);
					summary_append(s);
				}
				undo_push_remove(s);
				undo_push_insert(s, nullptr);
			}
		}
		list_selected.clear();
		undo_push_null();
		edit_menu_enable();
		sheet_draw();
	}

}