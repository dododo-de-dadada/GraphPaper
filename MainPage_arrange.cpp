//-------------------------------
// MainPage_arrange.cpp
// �}�`�̕��ёւ�
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr auto SEND_TO_BACK = true;
	using SEND_BACKWARD = SHAPE_LIST::iterator;
	using BRING_FORWARD = SHAPE_LIST::reverse_iterator;
	constexpr auto BRING_TO_FRONT = false;

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
	template void MainPage::arrange_order<BRING_FORWARD>(void);

	// �I�����ꂽ�}�`��O�̐}�`�Ɠ���ւ���.
	template void MainPage::arrange_order<SEND_BACKWARD>(void);

	// �I�����ꂽ�}�`�����܂��͑O�̐}�`�Ɠ���ւ���.
	// T	T �� iterator �̏ꍇ�͔w�ʂ̐}�`�Ɠ���ւ�, reverse_iterator �̏ꍇ�͑O�ʂ̐}�`�Ɠ���ւ���. 
	template<typename T> void MainPage::arrange_order(void)
	{
		T it_end;	// �I�[
		T it_src;	// �����������q
		if constexpr (std::is_same<T, BRING_FORWARD>::value) {
			it_end = m_list_shapes.rend();
			it_src = m_list_shapes.rbegin();
		}
		else {
			if constexpr (std::is_same<T, SEND_BACKWARD>::value) {
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
			auto s = *it_src;
			if (s->is_selected() != true) {
				continue;
			}
			auto t = *it_dst;
			// �}�`�ꗗ�̔r�����䂪 true �����肷��.
			if (m_summary_atomic.load(std::memory_order_acquire)) {
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
	template<bool B> void MainPage::arrange_to(void)
	{
		using winrt::Windows::UI::Xaml::Controls::ItemCollection;

		SHAPE_LIST slist;
		slist_selected<Shape>(m_list_shapes, slist);
		if (slist.size() == 0) {
			return;
		}
		if constexpr (B) {
			uint32_t i = 0;
			auto s = slist_front(m_list_shapes);
			for (auto t : slist) {
				// �}�`�ꗗ�̔r�����䂪 true �����肷��.
				if (m_summary_atomic.load(std::memory_order_acquire)) {
					summary_remove(t);
					summary_insert_at(t, i++);
				}
				undo_push_remove(t);
				undo_push_insert(t, s);
			}
		}
		else {
			for (auto s : slist) {
				// �}�`�ꗗ�̔r�����䂪 true �����肷��.
				if (m_summary_atomic.load(std::memory_order_acquire)) {
					summary_remove(s);
					summary_append(s);
				}
				undo_push_remove(s);
				undo_push_insert(s, nullptr);
			}
		}
		slist.clear();
		undo_push_null();
		xcvd_is_enabled();
		sheet_draw();
	}

	// �I�����ꂽ�}�`���őO�ʂɈړ�����.
	template void MainPage::arrange_to<BRING_TO_FRONT>(void);

	// �I�����ꂽ�}�`���Ŕw�ʂɈړ�����.
	template void MainPage::arrange_to<SEND_TO_BACK>(void);

}