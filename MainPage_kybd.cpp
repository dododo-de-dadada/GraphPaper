//-------------------------------
// MainPage_kybd.cpp
// �L�[�{�[�h�A�N�Z�����[�^�[�̃n���h���[
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// Shift + �����L�[�������ꂽ.
	void MainPage::ka_range_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	}

	// Shift + ����L�[�������ꂽ.
	void MainPage::ka_range_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();
	}

	// �����L�[�������ꂽ.
	void MainPage::ka_select_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	}

	// ����L�[�������ꂽ.
	void MainPage::ka_select_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::None, VirtualKey::Up>();
	}

	// Escape �������ꂽ.
	void MainPage::ka_tool_select_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		const auto draw_tool = tool();
		if (draw_tool == DRAW_TOOL::SELECT) {
			return;
		}
		rmfi_tool_select().IsChecked(true);
		// �O���[�v�����w�肵�Ă��Ă��R�[�h�r�n�C���h�����
		// �����Ń`�F�b�N���O��Ȃ�.
		if (draw_tool == DRAW_TOOL::BEZI) {
			rmfi_tool_BEZI().IsChecked(false);
		}
		else if (draw_tool == DRAW_TOOL::ELLI) {
			rmfi_tool_elli().IsChecked(false);
		}
		else if (draw_tool == DRAW_TOOL::LINE) {
			rmfi_tool_line().IsChecked(false);
		}
		else if (draw_tool == DRAW_TOOL::QUAD) {
			rmfi_tool_quad().IsChecked(false);
		}
		else if (draw_tool == DRAW_TOOL::RECT) {
			rmfi_tool_rect().IsChecked(false);
		}
		else if (draw_tool == DRAW_TOOL::RRCT) {
			rmfi_tool_rrect().IsChecked(false);
		}
		else if (draw_tool == DRAW_TOOL::TEXT) {
			rmfi_tool_text().IsChecked(false);
		}
		else if (draw_tool == DRAW_TOOL::SCALE) {
			rmfi_tool_scale().IsChecked(false);
		}
		rmfi_tool_select_click(nullptr, nullptr);
	}

	// Cntrol + PgDn �������ꂽ.
	//void MainPage::ka_bring_forward_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_bring_forward().IsEnabled()) {
	// 	//bring_forward_click(nullptr, nullptr);
	// 	arrange_order<S_LIST_T::reverse_iterator>();
	// }
	//}

	// Cntrol + End �������ꂽ.
	/*
	void MainPage::ka_bring_to_front_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_bring_to_front().IsEnabled()) {
			constexpr auto FRONT = false;
			arrange_to<FRONT>();
			//bring_to_front_click(nullptr, nullptr);
		}
	}
	*/

	// Cntrol + C �������ꂽ.
	/*
	void MainPage::ka_copy_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_xcvd_copy().IsEnabled()) {
			constexpr uint32_t COPY = 1;
			auto _{ xcvd_copy_async<COPY>() };
			//mfi_xcvd_copy_click_async(nullptr, nullptr);
		}
	}*/

	// Cntrol + X �������ꂽ.
	/*
	void MainPage::ka_cut_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_xcvd_cut().IsEnabled()) {
			constexpr uint32_t CUT = 0;
			auto _{ xcvd_copy_async<CUT>() };
			//mfi_xcvd_cut_click_async(nullptr, nullptr);
		}
	}
	*/
	// Delete �������ꂽ.
	/*
	void MainPage::ka_delete_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_xcvd_delete().IsEnabled()) {
			mfi_xcvd_delete_click(nullptr, nullptr);
		}
	}
	*/
	// Cntrol + E �������ꂽ.
	/*
	void MainPage::ka_edit_text_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_text_edit().IsEnabled()) {
			mfi_text_edit_click(nullptr, nullptr);
		}
	}
	*/
	// Cntrol + F �������ꂽ.
	//void MainPage::ka_text_find_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_text_find().IsEnabled()) {
	// 	mfi_text_find_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + G �������ꂽ.
	//void MainPage::ka_group_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_group().IsEnabled()) {
	// 	group_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + N �������ꂽ.
	//void MainPage::ka_new_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// mfi_new_click(nullptr, nullptr);
	//}

	// Cntrol + O �������ꂽ.
	//void MainPage::ka_open_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// file_open_click_async(nullptr, nullptr);
	//}

	// Cntrol + V �������ꂽ.
	//void MainPage::ka_paste_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_xcvd_paste().IsEnabled()) {
	// 	mfi_xcvd_paste_click_async(nullptr, nullptr);
	// }
	//}

	// Cntrol + Y �������ꂽ.
	//void MainPage::ka_redo_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_redo().IsEnabled()) {
	// 	mfi_redo_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + Shift + S �������ꂽ.
	//void MainPage::ka_save_as_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_file_save_as().IsEnabled()) {
	// 	file_save_as_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + S �������ꂽ.
	//void MainPage::ka_save_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_file_save().IsEnabled()) {
	// 	file_save_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + A �������ꂽ.
	//void MainPage::ka_select_all_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_select_all().IsEnabled()) {
	// 	mfi_select_all_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + PgUp �������ꂽ.
	//void MainPage::ka_send_backward_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_send_backward().IsEnabled()) {
	// 	send_backward_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + Home �������ꂽ.
	//void MainPage::ka_send_to_back_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_send_to_back().IsEnabled()) {
	// 	send_to_back_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + L �������ꂽ.
	//void MainPage::ka_summaty_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_summary().IsEnabled()) {
	// 	mfi_summary_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + Z �������ꂽ.
	//void MainPage::ka_undo_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_undo().IsEnabled()) {
	// 	mfi_undo_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + U �������ꂽ.
	//void MainPage::ka_ungroup_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_ungroup().IsEnabled()) {
	// 	ungroup_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + 0 �������ꂽ.
	//void MainPage::ka_zoom_reset_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (m_page_layout.m_page_scale != 1.0) {
	// 	mfi_zoom_reset_click(nullptr, nullptr);
	// }
	//}

	//void MainPage::ka_zoom_in_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//
	//}

	//void MainPage::scp_manipulation_delta(IInspectable const&, ManipulationDeltaRoutedEventArgs const& args)
	//{
	//
	//}

}