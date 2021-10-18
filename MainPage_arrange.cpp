//-------------------------------
// MainPage_arrange.cpp
// �}�`�̕��ёւ�
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::RoutedEventArgs;

	constexpr bool SEND_TO_BACK = true;
	using SEND_BACKWARD = SHAPE_LIST::iterator;
	using BRING_FORWARD = SHAPE_LIST::reverse_iterator;
	constexpr bool BRING_TO_FRONT = false;

	// �ҏW���j���[�́u�O�ʂɈړ��v���I�����ꂽ.
	void MainPage::arrange_bring_forward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
		arrange_order<BRING_FORWARD>();
	}

	// �ҏW���j���[�́u�őO�ʂɈړ��v���I�����ꂽ.
	void MainPage::arrange_bring_to_front_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`���Ŕw�ʂ܂��͍őO�ʂɈړ�����.
		arrange_to<BRING_TO_FRONT>();
	}

	// �I�����ꂽ�}�`�����̐}�`�Ɠ���ւ���.
	template 
	void MainPage::arrange_order<BRING_FORWARD>(void);

	// �I�����ꂽ�}�`��O�̐}�`�Ɠ���ւ���.
	template 
	void MainPage::arrange_order<SEND_BACKWARD>(void);

	// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
	// T	T �� iterator �̏ꍇ�͔w�ʂ̐}�`�Ɠ���ւ�, reverse_iterator �̏ꍇ�͑O�ʂ̐}�`�Ɠ���ւ���. 
	template<typename T>
	void MainPage::arrange_order(void)
	{
		T it_end;	// �I�[
		T it_src;	// ������
		if constexpr (std::is_same<T, BRING_FORWARD>::value) {
			it_end = m_main_sheet.m_shape_list.rend();
			it_src = m_main_sheet.m_shape_list.rbegin();
		}
		else {
			if constexpr (std::is_same<T, SEND_BACKWARD>::value) {
				it_end = m_main_sheet.m_shape_list.end();
				it_src = m_main_sheet.m_shape_list.begin();
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
			const Shape* s = *it_src;
			if (!s->is_deleted() && !s->is_selected()) {
				break;
			}
			it_src++;
		}
		bool done = false;	// �����ς݂����肷��
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
						ustack_push_null();
						xcvd_is_enabled();
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
			Shape* const s = *it_src;
			if (s->is_selected() != true) {
				continue;
			}
			Shape* const t = *it_dst;
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_arrange(s, t);
			}
			ustack_push_arrange(s, t);
			// �����ς݂ɂ���.
			done = true;
		}
	}

	// �ҏW���j���[�́u�ЂƂw�ʂɈړ��v���I�����ꂽ.
	void MainPage::arrange_send_backward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
		arrange_order<SEND_BACKWARD>();
	}

	// �ҏW���j���[�́u�Ŕw�ʂɈړ��v���I�����ꂽ.
	void MainPage::arrange_send_to_back_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`���Ŕw�ʂ܂��͍őO�ʂɈړ�����.
		arrange_to<SEND_TO_BACK>();
	}

	// �I�����ꂽ�}�`���Ŕw�ʂ܂��͍őO�ʂɈړ�����.
	// B	B �� true �̏ꍇ�͍Ŕw��, false �̏ꍇ�͍őO�ʂɈړ�
	template<bool B> 
	void MainPage::arrange_to(void)
	{
		SHAPE_LIST slist;
		slist_get_selected<Shape>(m_main_sheet.m_shape_list, slist);
		if (slist.size() == 0) {
			return;
		}
		if constexpr (B) {
			uint32_t i = 0;
			Shape* const s = slist_front(m_main_sheet.m_shape_list);
			for (Shape* const t : slist) {
				// �ꗗ���\������Ă邩���肷��.
				if (summary_is_visible()) {
					summary_remove(t);
					summary_insert_at(t, i++);
				}
				ustack_push_remove(t);
				ustack_push_insert(t, s);
			}
		}
		else {
			for (Shape* const s : slist) {
				// �ꗗ���\������Ă邩���肷��.
				if (summary_is_visible()) {
					summary_remove(s);
					summary_append(s);
				}
				ustack_push_remove(s);
				ustack_push_insert(s, nullptr);
			}
		}
		slist.clear();
		ustack_push_null();
		xcvd_is_enabled();
		sheet_draw();
	}

	// �I�����ꂽ�}�`���őO�ʂɈړ�����.
	template
	void MainPage::arrange_to<BRING_TO_FRONT>(void);

	// �I�����ꂽ�}�`���Ŕw�ʂɈړ�����.
	template 
	void MainPage::arrange_to<SEND_TO_BACK>(void);

}