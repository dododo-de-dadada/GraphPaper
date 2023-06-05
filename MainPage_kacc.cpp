#include "pch.h"
#include "MainPage.h"
#include "undo.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Text::Core::CoreTextRange;

	void MainPage::kacc_back_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_core_text_shape == nullptr) {
			return;
		}
		// 選択範囲がなくキャレット位置が文頭でないなら
		const auto len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
		const auto start = min(m_main_sheet.m_select_start, len);
		if (end == start && end > 0) {
			undo_push_null();
			undo_push_text_select(m_core_text_shape, end - 1, end, false);
			m_undo_stack.push_back(new UndoText2(m_core_text_shape, nullptr));
			main_draw();
		}
		// 選択範囲があるなら
		else if (end != start) {
			undo_push_null();
			m_undo_stack.push_back(new UndoText2(m_core_text_shape, nullptr));
			main_draw();
		}
		CoreTextRange modified_ran{
			static_cast<int32_t>(min(start, end)), static_cast<int32_t>(max(start, end))
		};
		const auto new_start = m_main_sheet.m_select_start;
		const auto new_end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifyTextChanged(modified_ran, 0, new_ran);
	}

	void MainPage::kacc_delete_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_core_text_shape == nullptr) {
			xcvd_delete_click(nullptr, nullptr);
		}
		else {
			core_text_del_c(false);
		}
	}

	void MainPage::kacc_delete_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_core_text_shape == nullptr) {
			xcvd_delete_click(nullptr, nullptr);
		}
		else {
			core_text_del_c(true);
		}
	}

	void MainPage::kacc_down_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_core_text_shape == nullptr) {
			return;
		}

		const auto len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
		const auto start = min(m_main_sheet.m_select_start, len);
		const auto row = m_core_text_shape->get_text_row(m_main_sheet.m_select_end);	// キャレットがある行
		const auto last = m_core_text_shape->m_dwrite_test_cnt - 1;	// 最終行
		if (end != start && row == last) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			//if (shift_key) {
			//	undo_push_text_select(m_core_text_shape, start, m_main_sheet.m_select_end, m_main_sheet.m_select_trail);
			//	main_draw();
			//}
			//else {
				undo_push_text_select(m_core_text_shape, end, m_main_sheet.m_select_end, m_main_sheet.m_select_trail);
				//xcvd_menu_is_enabled();
				main_draw();
			//}
		}
		else if (row != last) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			const auto h = m_core_text_shape->m_dwrite_test_metrics[row + 1].top - m_core_text_shape->m_dwrite_test_metrics[row].top;
			D2D1_POINT_2F car;
			m_core_text_shape->get_text_caret(end, row, m_main_sheet.m_select_trail, car);
			const D2D1_POINT_2F new_car{ car.x, car.y + h };
			bool new_trail;
			const auto new_end = m_core_text_shape->get_text_pos(new_car, new_trail);
			//if (shift_key) {
			//	undo_push_text_select(m_core_text_shape, start, new_end, new_trail);
			//	main_draw();
			//}
			//else {
				const auto new_start = new_trail ? new_end + 1 : new_end;
				undo_push_text_select(m_core_text_shape, new_start, new_end, new_trail);
				//xcvd_menu_is_enabled();
				main_draw();
			//}
		}
		const auto new_start = m_main_sheet.m_select_start;
		const auto new_end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_down_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_core_text_shape == nullptr || m_core_text_comp) {
			return;
		}
		const auto len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
		const auto start = min(m_main_sheet.m_select_start, len);
		const auto row = m_core_text_shape->get_text_row(m_main_sheet.m_select_end);	// キャレットがある行
		const auto last = m_core_text_shape->m_dwrite_test_cnt - 1;	// 最終行
		if (end != start && row == last) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			//if (shift_key) {
				undo_push_text_select(m_core_text_shape, start, m_main_sheet.m_select_end, m_main_sheet.m_select_trail);
				//xcvd_menu_is_enabled();
				main_draw();
			//}
			//else {
			//undo_push_text_select(m_core_text_shape, end, m_main_sheet.m_select_end, m_main_sheet.m_select_trail);
			//main_draw();
			//}
		}
		else if (row != last) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			const auto h = m_core_text_shape->m_dwrite_test_metrics[row + 1].top - m_core_text_shape->m_dwrite_test_metrics[row].top;
			D2D1_POINT_2F car;
			m_core_text_shape->get_text_caret(end, row, m_main_sheet.m_select_trail, car);
			const D2D1_POINT_2F new_car{ car.x, car.y + h };
			bool new_trail;
			const auto new_end = m_core_text_shape->get_text_pos(new_car, new_trail);
			//if (shift_key) {
				undo_push_text_select(m_core_text_shape, start, new_end, new_trail);
				//xcvd_menu_is_enabled();
				main_draw();
			//}
			//else {
			//const auto new_start = new_trail ? new_end + 1 : new_end;
			//undo_push_text_select(m_core_text_shape, new_start, new_end, new_trail);
			//main_draw();
			//}
		}
		const auto new_start = m_main_sheet.m_select_start;
		const auto new_end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_enter_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_core_text_shape == nullptr) {
			return;
		}
		const auto len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
		const auto start = min(m_main_sheet.m_select_start, len);
		const auto s = min(start, end);
		undo_push_null();
		// 改行を挿入する.
		m_undo_stack.push_back(new UndoText2(m_core_text_shape, L"\r"));
		undo_push_text_select(m_core_text_shape, s + 1, s + 1, false);
		//undo_menu_is_enabled();
		//xcvd_menu_is_enabled();
		main_draw();

		CoreTextRange modified_ran{
			static_cast<int32_t>(min(start, end)), static_cast<int32_t>(max(start, end))
		};
		const auto new_start = m_main_sheet.m_select_start;
		const auto new_end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifyTextChanged(modified_ran, 1, new_ran);
	}

	// Escape が押された.
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
		m_event_loc_pressed = LOC_TYPE::LOC_SHEET;
	}

	void MainPage::kacc_left_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_core_text_shape == nullptr) {
			return;
		}
		//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
		//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
		const auto len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
		const auto start = min(m_main_sheet.m_select_start, len);
		//if (shift_key) {
		//	if (end > 0) {
		//		undo_push_text_select(m_core_text_shape, start, end - 1, false);
		//		main_draw();
		//	}
		//}
		//else {
		if (end == start && end > 0) {
			undo_push_text_select(m_core_text_shape, end - 1, end - 1, false);
			//xcvd_menu_is_enabled();
			main_draw();
		}
		else if (end != start) {
			const auto new_end = min(start, end);
			undo_push_text_select(m_core_text_shape, new_end, new_end, false);
			//xcvd_menu_is_enabled();
			main_draw();
		}
		//}
		const auto new_start = m_main_sheet.m_select_start;
		const auto new_end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_left_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_core_text_shape == nullptr || m_core_text_comp) {
			return;
		}
		//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
		//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
		const auto len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
		const auto start = min(m_main_sheet.m_select_start, len);
		//if (shift_key) {
		if (end > 0) {
			undo_push_text_select(m_core_text_shape, start, end - 1, false);
			//xcvd_menu_is_enabled();
			main_draw();
		}
		//}
		//else {
		//	if (end == start && end > 0) {
		//		undo_push_text_select(m_core_text_shape, end - 1, end - 1, false);
		//		main_draw();
		//	}
		//	else if (end != start) {
		//		const auto new_end = min(start, end);
		//		undo_push_text_select(m_core_text_shape, new_end, new_end, false);
		//		main_draw();
		//	}
		//}
		const auto new_start = m_main_sheet.m_select_start;
		const auto new_end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_right_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_core_text_shape == nullptr) {
			return;
		}
		//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
		//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
		const auto len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
		const auto start = min(m_main_sheet.m_select_start, len);
		//if (shift_key) {
		//	if (end < len) {
		//		undo_push_text_select(m_core_text_shape, start, end + 1, false);
		//		main_draw();
		//	}
		//}
		//else {
		if (end == start && end < len) {
			undo_push_text_select(m_core_text_shape, end + 1, end + 1, false);
			//xcvd_menu_is_enabled();
			main_draw();
		}
		else if (end != start) {
			const auto new_end = max(start, end);
			undo_push_text_select(m_core_text_shape, new_end, new_end, false);
			//xcvd_menu_is_enabled();
			main_draw();
		}
		//}
		const auto new_start = m_main_sheet.m_select_start;
		const auto new_end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_right_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_core_text_shape == nullptr || m_core_text_comp) {
			return;
		}
		//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
		//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
		const auto len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
		const auto start = min(m_main_sheet.m_select_start, len);
		//if (shift_key) {
		if (end < len) {
			undo_push_text_select(m_core_text_shape, start, end + 1, false);
			//xcvd_menu_is_enabled();
			main_draw();
		}
		//}
		//else {
		//	if (end == start && end < len) {
		//		undo_push_text_select(m_core_text_shape, end + 1, end + 1, false);
		//		main_draw();
		//	}
		//	else if (end != start) {
		//		const auto new_end = max(start, end);
		//		undo_push_text_select(m_core_text_shape, new_end, new_end, false);
		//		main_draw();
		//	}
		//}
		const auto new_start = m_main_sheet.m_select_start;
		const auto new_end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_up_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_core_text_shape == nullptr) {
			return;
		}
		const auto len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
		const auto start = min(m_main_sheet.m_select_start, len);
		const auto row = m_core_text_shape->get_text_row(m_main_sheet.m_select_end);
		if (end != start && row == 0) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			//if (shift_key) {
			//	undo_push_text_select(m_core_text_shape, start, m_main_sheet.m_select_end, m_main_sheet.m_select_trail);
			//	main_draw();
			//}
			//else {
			undo_push_text_select(m_core_text_shape, end, m_main_sheet.m_select_end, m_main_sheet.m_select_trail);
			//xcvd_menu_is_enabled();
			main_draw();
			//}
		}
		else if (row != 0) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			const auto h = m_core_text_shape->m_dwrite_test_metrics[row].top - m_core_text_shape->m_dwrite_test_metrics[row - 1].top;
			D2D1_POINT_2F car;
			m_core_text_shape->get_text_caret(end, row, m_main_sheet.m_select_trail, car);
			const D2D1_POINT_2F new_car{ car.x, car.y - h };
			bool new_trail;
			const auto new_end = m_core_text_shape->get_text_pos(new_car, new_trail);
			//if (shift_key) {
			//	undo_push_text_select(m_core_text_shape, start, new_end, new_trail);
			//	main_draw();
			//}
			//else {
			const auto new_start = new_trail ? new_end + 1 : new_end;
			undo_push_text_select(m_core_text_shape, new_start, new_end, new_trail);
			//xcvd_menu_is_enabled();
			main_draw();
			//}
		}
		const auto new_start = m_main_sheet.m_select_start;
		const auto new_end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifySelectionChanged(new_ran);
	}

	void MainPage::kacc_up_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		// 漢字変換中は文字列選択しない.
		if (m_core_text_shape == nullptr || m_core_text_comp) {
			return;
		}
		const auto len = m_core_text_shape->get_text_len();
		const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
		const auto start = min(m_main_sheet.m_select_start, len);
		const auto row = m_core_text_shape->get_text_row(m_main_sheet.m_select_end);
		if (end != start && row == 0) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			//if (shift_key) {
			undo_push_text_select(m_core_text_shape, start, m_main_sheet.m_select_end, m_main_sheet.m_select_trail);
			//xcvd_menu_is_enabled();
			main_draw();
			//}
			//else {
			//	undo_push_text_select(m_core_text_shape, end, m_main_sheet.m_select_end, m_main_sheet.m_select_trail);
			//	main_draw();
			//}
		}
		else if (row != 0) {
			//const auto key_state = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift);
			//const auto shift_key = ((key_state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			const auto h = m_core_text_shape->m_dwrite_test_metrics[row].top - m_core_text_shape->m_dwrite_test_metrics[row - 1].top;
			D2D1_POINT_2F car;
			m_core_text_shape->get_text_caret(end, row, m_main_sheet.m_select_trail, car);
			const D2D1_POINT_2F new_car{ car.x, car.y - h };
			bool new_trail;
			const auto new_end = m_core_text_shape->get_text_pos(new_car, new_trail);
			//if (shift_key) {
			undo_push_text_select(m_core_text_shape, start, new_end, new_trail);
			//xcvd_menu_is_enabled();
			main_draw();
			//}
			//else {
			//	const auto new_start = new_trail ? new_end + 1 : new_end;
			//	undo_push_text_select(m_core_text_shape, new_start, new_end, new_trail);
			//	main_draw();
			//}
		}
		const auto new_start = m_main_sheet.m_select_start;
		const auto new_end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifySelectionChanged(new_ran);
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