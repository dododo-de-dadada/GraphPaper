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
	void MainPage::select_all_click(IInspectable const&, RoutedEventArgs const&)
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
		if (flag != true) {
			return;
		}
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			summary_select_all();
		}
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		xcvd_is_enabled();
		sheet_draw();
	}

	// �͈͂Ɋ܂܂��}�`��I����, �܂܂�Ȃ��}�`�̑I������������.
	bool MainPage::select_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max)
	{
		bool flag = false;
		//uint32_t i = 0u;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->in_area(a_min, a_max)) {
				if (s->is_selected() != true) {
					undo_push_select(s);
					// �}�`�ꗗ�̔r�����䂪 true �����肷��.
					if (m_summary_atomic.load(std::memory_order_acquire)) {
						summary_select(s);
					}
					flag = true;
				}
			}
			else {
				if (s->is_selected()) {
					undo_push_select(s);
					// �}�`�ꗗ�̔r�����䂪 true �����肷��.
					if (m_summary_atomic.load(std::memory_order_acquire)) {
						summary_unselect(s);
					}
					flag = true;
				}
			}
			//i++;
		}
		return flag;
	}

	// ���̐}�`��I������.
	template <VirtualKeyModifiers M, VirtualKey K> void MainPage::select_next_shape(void)
	{
		Shape* s = static_cast<Shape*>(nullptr);
		if constexpr (K == VirtualKey::Down) {
			if (m_event_shape_prev == nullptr) {
				s = slist_front(m_list_shapes);
				m_event_shape_pressed = s;
			}
			else {
				s = slist_next(m_list_shapes, m_event_shape_prev);
			}
			if (s != nullptr) {
				//m_event_shape_summary = s;
				goto SEL;
			}
		}
		if constexpr (K == VirtualKey::Up) {
			if (m_event_shape_prev == nullptr) {
				s = slist_back(m_list_shapes);
				m_event_shape_pressed = s;
			}
			else {
				s = slist_prev(m_list_shapes, m_event_shape_prev);
			}
			if (s != nullptr) {
				//m_event_shape_summary = s;
				goto SEL;
			}
		}
		return;
	SEL:
		if constexpr (M == VirtualKeyModifiers::Shift) {
			select_range(m_event_shape_pressed, s);
			m_event_shape_prev = s;
		}
		if constexpr (M == VirtualKeyModifiers::None) {
			m_event_shape_pressed =
			m_event_shape_prev = s;
			unselect_all();
			undo_push_select(s);
			// �}�`�ꗗ�̔r�����䂪 true �����肷��.
			if (m_summary_atomic.load(std::memory_order_acquire)) {
				summary_select(s);
			}
		}
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		xcvd_is_enabled();
		sheet_draw();
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
					[[fallthrough]];
			case END:
					if (s->is_selected()) {
						flag = true;
						undo_push_select(s);
						// �}�`�ꗗ�̔r�����䂪 true �����肷��.
						if (m_summary_atomic.load(std::memory_order_acquire)) {
							summary_unselect(s);
						}
					}
					break;
				}
				[[fallthrough]];
			case NEXT:
				if (s->is_selected() != true) {
					flag = true;
					undo_push_select(s);
					// �}�`�ꗗ�̔r�����䂪 true �����肷��.
					if (m_summary_atomic.load(std::memory_order_acquire)) {
						summary_select(s);
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
	void MainPage::select_shape(Shape* const s, const VirtualKeyModifiers k_mod)
	{
		using winrt::Windows::UI::Xaml::Controls::ListViewItem;

		// �R���g���[���L�[��������Ă��邩���肷��.
		if (k_mod == VirtualKeyModifiers::Control) {
			undo_push_select(s);
			xcvd_is_enabled();
			sheet_draw();
			// �}�`�ꗗ�̔r�����䂪 true �����肷��.
			if (m_summary_atomic.load(std::memory_order_acquire)) {
				if (s->is_selected()) {
					summary_select(s);
				}
				else {
					summary_unselect(s);
				}
			}
			m_event_shape_prev = s;
		}
		// �V�t�g�L�[��������Ă��邩���肷��.
		else if (k_mod == VirtualKeyModifiers::Shift) {
			// �O��|�C���^�[�������ꂽ�}�`���獡�񉟂��ꂽ�}�`�܂ł�
			// �͈͂ɂ���}�`��I������, �����łȂ��}�`��I�����Ȃ�.
			if (m_event_shape_prev == nullptr) {
				m_event_shape_prev = m_list_shapes.front();
			}
			if (select_range(s, m_event_shape_prev)) {
				// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		else {
			// �V�t�g�L�[���R���g���[���L�[��������ĂȂ��Ȃ��,
			// �}�`�̑I���t���O�������ĂȂ������肷��.
			if (!s->is_selected()) {
				unselect_all();
				undo_push_select(s);
				xcvd_is_enabled();
				sheet_draw();
				// �}�`�ꗗ�̔r�����䂪 true �����肷��.
				if (m_summary_atomic.load(std::memory_order_acquire)) {
					summary_select(s);
				}
			}
			m_event_shape_prev = s;
		}
		if (s->is_selected()) {
			// �����ꂽ�}�`���I������Ă���ꍇ,
			m_sheet_main.set_attr_to(s);
			sheet_attr_is_checked();
		}
	}

	// �͈͂Ɋ܂܂��}�`�̑I���𔽓]����.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	bool MainPage::toggle_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max)
	{
		auto flag = false;
		//uint32_t i = 0;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->in_area(a_min, a_max)) {
				undo_push_select(s);
				// �}�`�ꗗ�̔r�����䂪 true �����肷��.
				if (m_summary_atomic.load(std::memory_order_acquire)) {
					if (s->is_selected() != true) {
						summary_select(s);
					}
					else {
						summary_unselect(s);
					}
				}
				flag = true;
			}
			//i++;
		}
		return flag;
	}

	// �}�`�̑I�������ׂĉ�������.
	// t_range_only	�����͈͂̂݉����t���O
	// �߂�l	�I�����������ꂽ�}�`������ꍇ true, �Ȃ��ꍇ false
	// �����͈͂̂݃t���O�������Ă���ꍇ, �����͈͂̑I���̂݉��������.
	// �����͈͂̂݃t���O���Ȃ��ꍇ, �}�`�̑I���������͈͂̑I�����������������.
	bool MainPage::unselect_all(const bool t_range_only)
	{
		auto flag = false;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (t_range_only != true && s->is_selected()) {
				// �����͈͂̂݉����t���O���Ȃ�, ���}�`�̑I���t���O�������Ă���ꍇ,
				undo_push_select(s);
				flag = true;
			}
			DWRITE_TEXT_RANGE d_range;
			if (s->get_text_range(d_range) != true) {
				// �����͈͂��擾�ł��Ȃ�
				// (������}�`�łȂ�) �ꍇ,
				// �ȉ��𖳎�����.
				continue;
			}
			const DWRITE_TEXT_RANGE s_range = DWRITE_TEXT_RANGE{ 0, 0 };
			if (equal(s_range, d_range)) {
				// ���������͈͂� { 0, 0 } �̏ꍇ,
				// �ȉ��𖳎�����.
				continue;
			}
			// { 0, 0 } ��}�`�Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
			undo_push_set<UNDO_OP::TEXT_RANGE>(s, s_range);
			flag = true;
		}
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			summary_unselect_all();
		}
		return flag;
	}

	// Shift + �����L�[�������ꂽ.
	void MainPage::kacc_range_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	}

	// Shift + ����L�[�������ꂽ.
	void MainPage::kacc_range_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();
	}

	// �����L�[�������ꂽ.
	void MainPage::kacc_select_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	}

	// ����L�[�������ꂽ.
	void MainPage::kacc_select_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::None, VirtualKey::Up>();
	}

}