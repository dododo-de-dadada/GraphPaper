//-------------------------------
// MainPage_select.cpp
// �}�`�̑I��
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::System::VirtualKeyModifiers;
	using winrt::Windows::UI::Xaml::Controls::ListViewItem;
	//using winrt::Windows::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs;
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	// Escape �������ꂽ.
	void MainPage::select_tool_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_drawing_tool == DRAWING_TOOL::SELECT) {
			unselect_all();
			page_draw();
		}
		else {
			drawing_tool_click(rmfi_tool_selecting(), nullptr);
		}
	}

	// Shift + �����L�[�������ꂽ.
	//void MainPage::select_range_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//	select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	//}

	// Shift + ����L�[�������ꂽ.
	//void MainPage::select_range_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//	select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();
	//}

	// �����L�[�������ꂽ.
	//void MainPage::select_shape_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//	select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	//}

	// ����L�[�������ꂽ.
	//void MainPage::select_shape_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//	select_next_shape<VirtualKeyModifiers::None, VirtualKey::Up>();
	//}

	// �ҏW���j���[�́u���ׂđI���v���I�����ꂽ.
	void MainPage::select_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		bool done = false;
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted() || s->is_selected()) {
				continue;
			}
			if (!done) {
				done = true;
			}
			ustack_push_select(s);
		}
		if (!done) {
			return;
		}
		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			summary_select_all();
		}
		xcvd_is_enabled();
		page_draw();
	}

	// �͈͂Ɋ܂܂��}�`��I����, �܂܂�Ȃ��}�`�̑I������������.
	bool MainPage::select_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb)
	{
		bool done = false;
		//uint32_t i = 0u;
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->in_area(area_lt, area_rb)) {
				if (!s->is_selected()) {
					ustack_push_select(s);
					// �ꗗ���\������Ă邩���肷��.
					if (summary_is_visible()) {
						summary_select(s);
					}
					if (!done) {
						done = true;
					}
				}
			}
			else if (s->is_selected()) {
				ustack_push_select(s);
				// �ꗗ���\������Ă邩���肷��.
				if (summary_is_visible()) {
					summary_unselect(s);
				}
				if (!done) {
					done = true;
				}
			}
		}
		return done;
	}

	// ���̐}�`��I������.
	/*
	template <VirtualKeyModifiers M, VirtualKey K> void MainPage::select_next_shape(void)
	{
		Shape* s = static_cast<Shape*>(nullptr);
		if constexpr (K == VirtualKey::Down) {
			if (m_event_shape_prev == nullptr) {
				s = slist_front(m_main_page.m_shape_list);
				m_event_shape_pressed = s;
			}
			else {
				s = slist_next(m_main_page.m_shape_list, m_event_shape_prev);
			}
			if (s != nullptr) {
				//m_event_shape_summary = s;
				goto SEL;
			}
		}
		if constexpr (K == VirtualKey::Up) {
			if (m_event_shape_prev == nullptr) {
				s = slist_back(m_main_page.m_shape_list);
				m_event_shape_pressed = s;
			}
			else {
				s = slist_prev(m_main_page.m_shape_list, m_event_shape_prev);
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
			ustack_push_select(s);
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_select(s);
			}
		}
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		xcvd_is_enabled();
		page_draw();
	}
	template void MainPage::select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::None, VirtualKey::Up>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();
	*/

	// �͈͂̒��̐}�`��I������, ����ȊO�̐}�`�͑I�����͂���.
	// s_from	�ŏ��̐}�`
	// s_to	�Ō�̐}�`
	// �߂�l	�I�����ύX���ꂽ true
	bool MainPage::select_range(Shape* const s_from, Shape* const s_to)
	{
		constexpr int BEGIN = 0;
		constexpr int NEXT = 1;
		constexpr int END = 2;
		auto done = false;
		auto st = BEGIN;
		auto s_end = static_cast<Shape*>(nullptr);
		auto i = 0u;
		for (auto s : m_main_page.m_shape_list) {
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
						ustack_push_select(s);
						// �ꗗ���\������Ă邩���肷��.
						if (summary_is_visible()) {
							summary_unselect(s);
						}
						if (!done) {
							done = true;
						}
					}
					break;
				}
				[[fallthrough]];
			case NEXT:
				if (!s->is_selected()) {
					ustack_push_select(s);
					// �ꗗ���\������Ă邩���肷��.
					if (summary_is_visible()) {
						summary_select(s);
					}
					if (!done) {
						done = true;
					}
				}
				if (s == s_end) {
					st = END;
				}
				break;
			}
			i++;
		}
		return done;
	}

	// �}�`��I������.
	void MainPage::select_shape(Shape* const s, const VirtualKeyModifiers k_mod)
	{
		// �R���g���[���L�[��������Ă��邩���肷��.
		if (k_mod == VirtualKeyModifiers::Control) {
			ustack_push_select(s);
			xcvd_is_enabled();
			page_draw();
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				if (s->is_selected()) {
					summary_select(s);
				}
				else {
					summary_unselect(s);
				}
			}
			m_event_shape_prev = s;
		}
		// �V�t�g�L�[���������, �����O�ɉ����ꂽ�}�`������, ���̐}�`���I������Ă��邩���肷��.
		else if (k_mod == VirtualKeyModifiers::Shift &&
			m_event_shape_prev != nullptr && m_event_shape_prev->is_selected() && !m_event_shape_prev->is_deleted()) {
			// �O��|�C���^�[�������ꂽ�}�`���󂩔��肷��.
			//if (m_event_shape_prev == nullptr) {
			//	// �}�`���X�g�̐擪��O��|�C���^�[�������ꂽ�}�`�Ɋi�[����.
			//	m_event_shape_prev = m_main_page.m_shape_list.front();
			//}
			// �͈͂̒��̐}�`�͑I������, ����ȊO�̐}�`�̑I�����͂���.
			if (select_range(s, m_event_shape_prev)) {
				xcvd_is_enabled();
				page_draw();
			}
		}
		else {
			// ��L�ȊO�Ȃ�,
			// �}�`���I������Ă邩���肷��.
			if (!s->is_selected()) {
				unselect_all();
				ustack_push_select(s);
				xcvd_is_enabled();
				page_draw();
				// �ꗗ���\������Ă邩���肷��.
				if (summary_is_visible()) {
					summary_select(s);
				}
			}
			m_event_shape_prev = s;
		}
		if (s->is_selected()) {
			// �����ꂽ�}�`���I������Ă���ꍇ,
//			m_main_page.set_attr_to(s);
//			page_setting_is_checked();
		}
	}

	// �͈͂Ɋ܂܂��}�`�̑I���𔽓]����.
	// area_lt	�͈͂̍���ʒu
	// area_rb	�͈͂̉E���ʒu
	bool MainPage::toggle_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb)
	{
		bool done = false;
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted() || !s->in_area(area_lt, area_rb)) {
				continue;
			}
			ustack_push_select(s);
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				if (!s->is_selected()) {
					summary_select(s);
				}
				else {
					summary_unselect(s);
				}
			}
			if (!done) {
				done = true;
			}
		}
		return done;
	}

	// �}�`�̑I�������ׂĉ�������.
	// t_range_only	�����͈͂������� (�����͈͂����łȂ��Ȃ�, �}�`�̑I�������������).
	// �߂�l	�I�����������ꂽ�}�`������Ȃ� true
	bool MainPage::unselect_all(const bool t_range_only)
	{
		bool done = false;
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted()) {
				continue;
			}
			// �����͈͂��������łȂ�, ���}�`���I������Ă��邩���肷��.
			if (!t_range_only && s->is_selected()) {
				ustack_push_select(s);
				if (!done) {
					done = true;
				}
			}
			// �����͈͂��擾�ł��Ȃ� (������}�`�łȂ��ꍇ���܂�) �����肷��.
			DWRITE_TEXT_RANGE d_range;
			if (!s->get_text_selected(d_range)) {
				continue;
			}
			// ���������͈͂� { 0, 0 } �����肷��.
			constexpr DWRITE_TEXT_RANGE s_range = DWRITE_TEXT_RANGE{ 0, 0 };
			if (equal(s_range, d_range)) {
				continue;
			}
			// { 0, 0 } ��}�`�Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
			ustack_push_set<UNDO_ID::TEXT_RANGE>(s, s_range);
			if (!done) {
				done = true;
			}
		}
		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			summary_unselect_all();
		}
		return done;
	}

}