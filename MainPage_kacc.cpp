#include "pch.h"
#include "MainPage.h"
#include "undo.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Text::Core::CoreTextRange;

	void MainPage::kacc_back_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_edit_context_shape == nullptr) {
			return;
		}
		// �I��͈͂��Ȃ��L�����b�g�ʒu�������łȂ��Ȃ�
		const auto len = m_edit_context_shape->get_text_len();
		const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
		const auto start = min(m_main_page.m_select_start, len);
		if (end == start && end > 0) {
			undo_push_null();
			undo_push_text_select(m_edit_context_shape, end - 1, end, false);
			m_ustack_undo.push_back(new UndoText2(m_edit_context_shape, nullptr));
			main_draw();
		}
		// �I��͈͂�����Ȃ�
		else if (end != start) {
			undo_push_null();
			m_ustack_undo.push_back(new UndoText2(m_edit_context_shape, nullptr));
			main_draw();
		}
		CoreTextRange modified_ran{
			static_cast<int32_t>(min(start, end)), static_cast<int32_t>(max(start, end))
		};
		const auto new_start = m_main_page.m_select_start;
		const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_edit_context.NotifyTextChanged(modified_ran, 0, new_ran);
	}

	void MainPage::kacc_delete_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_edit_context_shape == nullptr) {
			xcvd_delete_click(nullptr, nullptr);
		}
		else {
			text_char_delete(false);
		}
	}

	void MainPage::kacc_delete_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_edit_context_shape == nullptr) {
			xcvd_delete_click(nullptr, nullptr);
		}
		else {
			text_char_delete(true);
		}
	}

	void MainPage::kacc_down_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_edit_context_shape == nullptr) {
			return;
		}

		const auto len = m_edit_context_shape->get_text_len();
		const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
		const auto start = min(m_main_page.m_select_start, len);
		const auto row = m_edit_context_shape->get_text_row(m_main_page.m_select_end);	// �L�����b�g������s
		const auto last = m_edit_context_shape->m_dwrite_test_cnt - 1;	// �ŏI�s
		if (end != start && row == last) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			//if (shift_key) {
			//	undo_push_text_select(m_edit_context_shape, start, m_main_page.m_select_end, m_main_page.m_select_trail);
			//	main_draw();
			//}
			//else {
				undo_push_text_select(m_edit_context_shape, end, m_main_page.m_select_end, m_main_page.m_select_trail);
				//xcvd_menu_is_enabled();
				main_draw();
			//}
		}
		else if (row != last) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			const auto line_h = m_edit_context_shape->m_dwrite_test_metrics[row + 1].top - m_edit_context_shape->m_dwrite_test_metrics[row].top;
			D2D1_POINT_2F car;
			m_edit_context_shape->get_text_caret(end, row, m_main_page.m_select_trail, car);
			const D2D1_POINT_2F new_car{ car.x, car.y + line_h };
			bool new_trail;
			const auto new_end = m_edit_context_shape->get_text_pos(new_car, new_trail);
			//if (shift_key) {
			//	undo_push_text_select(m_edit_context_shape, start, new_end, new_trail);
			//	main_draw();
			//}
			//else {
				const auto new_start = new_trail ? new_end + 1 : new_end;
				undo_push_text_select(m_edit_context_shape, new_start, new_end, new_trail);
				//xcvd_menu_is_enabled();
				main_draw();
			//}
		}
		const auto new_start = m_main_page.m_select_start;
		const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_edit_context.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_down_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_edit_context_shape == nullptr || m_edit_context_comp) {
			return;
		}
		const auto len = m_edit_context_shape->get_text_len();
		const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
		const auto start = min(m_main_page.m_select_start, len);
		const auto row = m_edit_context_shape->get_text_row(m_main_page.m_select_end);	// �L�����b�g������s
		const auto last = m_edit_context_shape->m_dwrite_test_cnt - 1;	// �ŏI�s
		if (end != start && row == last) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			//if (shift_key) {
				undo_push_text_select(m_edit_context_shape, start, m_main_page.m_select_end, m_main_page.m_select_trail);
				//xcvd_menu_is_enabled();
				main_draw();
			//}
			//else {
			//undo_push_text_select(m_edit_context_shape, end, m_main_page.m_select_end, m_main_page.m_select_trail);
			//main_draw();
			//}
		}
		else if (row != last) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			const auto line_h = m_edit_context_shape->m_dwrite_test_metrics[row + 1].top - m_edit_context_shape->m_dwrite_test_metrics[row].top;
			D2D1_POINT_2F car;
			m_edit_context_shape->get_text_caret(end, row, m_main_page.m_select_trail, car);
			const D2D1_POINT_2F new_car{ car.x, car.y + line_h };
			bool new_trail;
			const auto new_end = m_edit_context_shape->get_text_pos(new_car, new_trail);
			//if (shift_key) {
				undo_push_text_select(m_edit_context_shape, start, new_end, new_trail);
				//xcvd_menu_is_enabled();
				main_draw();
			//}
			//else {
			//const auto new_start = new_trail ? new_end + 1 : new_end;
			//undo_push_text_select(m_edit_context_shape, new_start, new_end, new_trail);
			//main_draw();
			//}
		}
		const auto new_start = m_main_page.m_select_start;
		const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_edit_context.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_enter_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_edit_context_shape == nullptr) {
			return;
		}
		const auto len = m_edit_context_shape->get_text_len();
		const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
		const auto start = min(m_main_page.m_select_start, len);
		const auto s = min(start, end);
		undo_push_null();
		// ���s��}������.
		m_ustack_undo.push_back(new UndoText2(m_edit_context_shape, L"\r"));
		undo_push_text_select(m_edit_context_shape, s + 1, s + 1, false);
		//undo_menu_is_enabled();
		//xcvd_menu_is_enabled();
		main_draw();

		CoreTextRange modified_ran{
			static_cast<int32_t>(min(start, end)), static_cast<int32_t>(max(start, end))
		};
		const auto new_start = m_main_page.m_select_start;
		const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_edit_context.NotifyTextChanged(modified_ran, 1, new_ran);
	}

	// Escape �������ꂽ.
	void MainPage::kacc_escape_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_drawing_tool == DRAWING_TOOL::SELECT) {
			unselect_shape_all();
			main_draw();
		}
		else {
			m_drawing_tool = DRAWING_TOOL::SELECT;
			rmfi_menu_selection_tool().IsChecked(true);
		}
		m_event_state = EVENT_STATE::BEGIN;
		m_event_shape_pressed = nullptr;
		m_event_loc_pressed = LOC_TYPE::LOC_PAGE;
	}

	void MainPage::kacc_left_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_edit_context_shape == nullptr) {
			return;
		}
		//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
		//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
		const auto len = m_edit_context_shape->get_text_len();
		const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
		const auto start = min(m_main_page.m_select_start, len);
		//if (shift_key) {
		//	if (end > 0) {
		//		undo_push_text_select(m_edit_context_shape, start, end - 1, false);
		//		main_draw();
		//	}
		//}
		//else {
		if (end == start && end > 0) {
			undo_push_text_select(m_edit_context_shape, end - 1, end - 1, false);
			//xcvd_menu_is_enabled();
			main_draw();
		}
		else if (end != start) {
			const auto new_end = min(start, end);
			undo_push_text_select(m_edit_context_shape, new_end, new_end, false);
			//xcvd_menu_is_enabled();
			main_draw();
		}
		//}
		const auto new_start = m_main_page.m_select_start;
		const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_edit_context.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_left_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_edit_context_shape == nullptr || m_edit_context_comp) {
			return;
		}
		//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
		//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
		const auto len = m_edit_context_shape->get_text_len();
		const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
		const auto start = min(m_main_page.m_select_start, len);
		//if (shift_key) {
		if (end > 0) {
			undo_push_text_select(m_edit_context_shape, start, end - 1, false);
			//xcvd_menu_is_enabled();
			main_draw();
		}
		//}
		//else {
		//	if (end == start && end > 0) {
		//		undo_push_text_select(m_edit_context_shape, end - 1, end - 1, false);
		//		main_draw();
		//	}
		//	else if (end != start) {
		//		const auto new_end = min(start, end);
		//		undo_push_text_select(m_edit_context_shape, new_end, new_end, false);
		//		main_draw();
		//	}
		//}
		const auto new_start = m_main_page.m_select_start;
		const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_edit_context.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_right_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_edit_context_shape == nullptr) {
			return;
		}
		//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
		//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
		const auto len = m_edit_context_shape->get_text_len();
		const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
		const auto start = min(m_main_page.m_select_start, len);
		//if (shift_key) {
		//	if (end < len) {
		//		undo_push_text_select(m_edit_context_shape, start, end + 1, false);
		//		main_draw();
		//	}
		//}
		//else {
		if (end == start && end < len) {
			undo_push_text_select(m_edit_context_shape, end + 1, end + 1, false);
			//xcvd_menu_is_enabled();
			main_draw();
		}
		else if (end != start) {
			const auto new_end = max(start, end);
			undo_push_text_select(m_edit_context_shape, new_end, new_end, false);
			//xcvd_menu_is_enabled();
			main_draw();
		}
		//}
		const auto new_start = m_main_page.m_select_start;
		const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_edit_context.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_right_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_edit_context_shape == nullptr || m_edit_context_comp) {
			return;
		}
		//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
		//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
		const auto len = m_edit_context_shape->get_text_len();
		const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
		const auto start = min(m_main_page.m_select_start, len);
		//if (shift_key) {
		if (end < len) {
			undo_push_text_select(m_edit_context_shape, start, end + 1, false);
			//xcvd_menu_is_enabled();
			main_draw();
		}
		//}
		//else {
		//	if (end == start && end < len) {
		//		undo_push_text_select(m_edit_context_shape, end + 1, end + 1, false);
		//		main_draw();
		//	}
		//	else if (end != start) {
		//		const auto new_end = max(start, end);
		//		undo_push_text_select(m_edit_context_shape, new_end, new_end, false);
		//		main_draw();
		//	}
		//}
		const auto new_start = m_main_page.m_select_start;
		const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_edit_context.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_up_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_edit_context_shape == nullptr) {
			return;
		}
		const auto len = m_edit_context_shape->get_text_len();
		const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
		const auto start = min(m_main_page.m_select_start, len);
		const auto row = m_edit_context_shape->get_text_row(m_main_page.m_select_end);
		if (end != start && row == 0) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			//if (shift_key) {
			//	undo_push_text_select(m_edit_context_shape, start, m_main_page.m_select_end, m_main_page.m_select_trail);
			//	main_draw();
			//}
			//else {
			undo_push_text_select(m_edit_context_shape, end, m_main_page.m_select_end, m_main_page.m_select_trail);
			//xcvd_menu_is_enabled();
			main_draw();
			//}
		}
		else if (row != 0) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			const auto line_h = m_edit_context_shape->m_dwrite_test_metrics[row].top - m_edit_context_shape->m_dwrite_test_metrics[row - 1].top;
			D2D1_POINT_2F car;
			m_edit_context_shape->get_text_caret(end, row, m_main_page.m_select_trail, car);
			const D2D1_POINT_2F new_car{ car.x, car.y - line_h };
			bool new_trail;
			const auto new_end = m_edit_context_shape->get_text_pos(new_car, new_trail);
			//if (shift_key) {
			//	undo_push_text_select(m_edit_context_shape, start, new_end, new_trail);
			//	main_draw();
			//}
			//else {
			const auto new_start = new_trail ? new_end + 1 : new_end;
			undo_push_text_select(m_edit_context_shape, new_start, new_end, new_trail);
			//xcvd_menu_is_enabled();
			main_draw();
			//}
		}
		const auto new_start = m_main_page.m_select_start;
		const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_edit_context.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_up_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		// �����ϊ����͕�����I�����Ȃ�.
		if (m_edit_context_shape == nullptr || m_edit_context_comp) {
			return;
		}
		const auto len = m_edit_context_shape->get_text_len();
		const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
		const auto start = min(m_main_page.m_select_start, len);
		const auto row = m_edit_context_shape->get_text_row(m_main_page.m_select_end);
		if (end != start && row == 0) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			//if (shift_key) {
			undo_push_text_select(m_edit_context_shape, start, m_main_page.m_select_end, m_main_page.m_select_trail);
			//xcvd_menu_is_enabled();
			main_draw();
			//}
			//else {
			//	undo_push_text_select(m_edit_context_shape, end, m_main_page.m_select_end, m_main_page.m_select_trail);
			//	main_draw();
			//}
		}
		else if (row != 0) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			const auto line_h = m_edit_context_shape->m_dwrite_test_metrics[row].top - m_edit_context_shape->m_dwrite_test_metrics[row - 1].top;
			D2D1_POINT_2F car;
			m_edit_context_shape->get_text_caret(end, row, m_main_page.m_select_trail, car);
			const D2D1_POINT_2F new_car{ car.x, car.y - line_h };
			bool new_trail;
			const auto new_end = m_edit_context_shape->get_text_pos(new_car, new_trail);
			//if (shift_key) {
			undo_push_text_select(m_edit_context_shape, start, new_end, new_trail);
			//xcvd_menu_is_enabled();
			main_draw();
			//}
			//else {
			//	const auto new_start = new_trail ? new_end + 1 : new_end;
			//	undo_push_text_select(m_edit_context_shape, new_start, new_end, new_trail);
			//	main_draw();
			//}
		}
		const auto new_start = m_main_page.m_select_start;
		const auto new_end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_edit_context.NotifySelectionChanged(new_ran);
	}
	/*
	*/
	/*
	*/
	/*
	*/
	/*
	*/

}