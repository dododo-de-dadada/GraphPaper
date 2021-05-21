//-------------------------------
// MainPage_arrng.cpp
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
	void MainPage::arrng_bring_forward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
		arrng_order<FORWARD>();
	}

	// �ҏW���j���[�́u�őO�ʂɈړ��v���I�����ꂽ.
	void MainPage::arrng_bring_to_front_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`���Ŕw�ʂ܂��͍őO�ʂɈړ�����.
		arrng_to<FRONT>();
	}

	// �I�����ꂽ�}�`��O�̐}�`�Ɠ���ւ���.
	template void MainPage::arrng_order<BACKWARD>(void);

	// �I�����ꂽ�}�`�����̐}�`�Ɠ���ւ���.
	template void MainPage::arrng_order<FORWARD>(void);

	// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
	// T	T �� iterator �̏ꍇ�͔w�ʂ̐}�`�Ɠ���ւ�, reverse_iterator �̏ꍇ�͑O�ʂ̐}�`�Ɠ���ւ���. 
	template<typename T> void MainPage::arrng_order(void)
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
			// ���̐}�`���Ȃ������肷��.
			if (it_src == it_end) {
				return;
			}
			// �����t���O���I���t���O�������Ă��Ȃ������肷��.
			if (!(*it_src)->is_deleted() && !(*it_src)->is_selected()) {
				break;
			}
			it_src++;
		}
		auto flag = false;	// �����t���O
		for (;;) {
			// �����������q�������攽���q�Ɋi�[����,
			// �����������q���C���N�������g����.
			auto it_dst = it_src++;
			// �폜����ĂȂ����̐}�`�𓾂�.
			for (;;) {
				// ���̐}�`���Ȃ������肷��.
				if (it_src == it_end) {
					// �����t���O�������Ă��邩���肷��.
					if (flag == true) {
						undo_push_null();
						edit_menu_enable();
						sheet_draw();
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
			auto s = *it_src;
			if (s->is_selected() != true) {
				continue;
			}
			auto t = *it_dst;
			// �}�`�ꗗ�̔r�����䂪 true �����肷��.
			if (m_summary_atomic.load(std::memory_order_acquire)) {
				summary_arrng(s, t);
			}
			undo_push_arrng(s, t);
			// �����t���O�𗧂Ă�.
			flag = true;
		}
	}

	// �ҏW���j���[�́u�ЂƂw�ʂɈړ��v���I�����ꂽ.
	void MainPage::arrng_send_backward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
		arrng_order<BACKWARD>();
	}

	// �ҏW���j���[�́u�Ŕw�ʂɈړ��v���I�����ꂽ.
	void MainPage::arrng_send_to_back_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`���Ŕw�ʂ܂��͍őO�ʂɈړ�����.
		arrng_to<BACK>();
	}

	// �I�����ꂽ�}�`���Ŕw�ʂɈړ�����.
	template void MainPage::arrng_to<BACK>(void);

	// �I�����ꂽ�}�`���őO�ʂɈړ�����.
	template void MainPage::arrng_to<FRONT>(void);

	// �I�����ꂽ�}�`���Ŕw�ʂ܂��͍őO�ʂɈړ�����.
	// T	T �� true �̏ꍇ�͍Ŕw��, false �̏ꍇ�͍őO�ʂɈړ�
	template<bool B> void MainPage::arrng_to(void)
	{
		using winrt::Windows::UI::Xaml::Controls::ItemCollection;

		S_LIST_T s_list;
		s_list_selected<Shape>(m_list_shapes, s_list);
		if (s_list.size() == 0) {
			return;
		}
		if constexpr (B) {
			uint32_t i = 0;
			auto s = s_list_front(m_list_shapes);
			for (auto t : s_list) {
				// �}�`�ꗗ�̔r�����䂪 true �����肷��.
				if (m_summary_atomic.load(std::memory_order_acquire)) {
					summary_remove(t);
					summary_insert(t, i++);
				}
				undo_push_remove(t);
				undo_push_insert(t, s);
			}
		}
		else {
			for (auto s : s_list) {
				// �}�`�ꗗ�̔r�����䂪 true �����肷��.
				if (m_summary_atomic.load(std::memory_order_acquire)) {
					summary_remove(s);
					summary_append(s);
				}
				undo_push_remove(s);
				undo_push_insert(s, nullptr);
			}
		}
		s_list.clear();
		undo_push_null();
		edit_menu_enable();
		sheet_draw();
	}

}