//-------------------------------
// MainPage_kacc.cpp
// �L�[�{�[�h�A�N�Z�����[�^�̃n���h���[
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
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

	// Escape �������ꂽ.
	void MainPage::kacc_tool_select_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		const auto t_draw = tool_draw();
		if (t_draw == DRAW_TOOL::SELECT) {
			unselect_all();
			sheet_draw();
			return;
		}
		rmfi_tool_select().IsChecked(true);
		// �O���[�v�����w�肵�Ă��Ă��R�[�h�r�n�C���h�����
		// �����Ń`�F�b�N���O��Ȃ�.
		if (t_draw == DRAW_TOOL::BEZI) {
			rmfi_tool_draw_bezi().IsChecked(false);
		}
		else if (t_draw == DRAW_TOOL::ELLI) {
			rmfi_tool_draw_elli().IsChecked(false);
		}
		else if (t_draw == DRAW_TOOL::LINE) {
			rmfi_tool_draw_line().IsChecked(false);
		}
		else if (t_draw == DRAW_TOOL::POLY) {
			rmfi_tool_draw_poly().IsChecked(false);
		}
		else if (t_draw == DRAW_TOOL::RECT) {
			rmfi_tool_draw_rect().IsChecked(false);
		}
		else if (t_draw == DRAW_TOOL::RRECT) {
			rmfi_tool_draw_rrect().IsChecked(false);
		}
		else if (t_draw == DRAW_TOOL::TEXT) {
			rmfi_tool_draw_text().IsChecked(false);
		}
		else if (t_draw == DRAW_TOOL::RULER) {
			rmfi_tool_draw_ruler().IsChecked(false);
		}
		tool_draw_click(rmfi_tool_select(), nullptr);
	}

	// Cntrol + PgDn �������ꂽ.
	//void MainPage::kacc_bring_forward_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_bring_forward().IsEnabled()) {
	// 	//arrng_bring_forward_click(nullptr, nullptr);
	// 	arrng_order<SHAPE_LIST::reverse_iterator>();
	// }
	//}

	// Cntrol + End �������ꂽ.
	/*
	void MainPage::kacc_bring_to_front_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_bring_to_front().IsEnabled()) {
			constexpr auto FRONT = false;
			arrng_to<FRONT>();
			//arrng_bring_to_front_click(nullptr, nullptr);
		}
	}
	*/

	// Cntrol + C �������ꂽ.
	/*
	void MainPage::kacc_copy_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_xcvd_copy().IsEnabled()) {
			constexpr uint32_t COPY = 1;
			auto _{ xcvd_copy_async<COPY>() };
			//xcvd_copy_click_async(nullptr, nullptr);
		}
	}*/

	// Cntrol + X �������ꂽ.
	/*
	void MainPage::kacc_cut_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_xcvd_cut().IsEnabled()) {
			constexpr uint32_t CUT = 0;
			auto _{ xcvd_copy_async<CUT>() };
			//xcvd_cut_click_async(nullptr, nullptr);
		}
	}
	*/
	// Delete �������ꂽ.
	/*
	void MainPage::kacc_delete_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_xcvd_delete().IsEnabled()) {
			xcvd_delete_click(nullptr, nullptr);
		}
	}
	*/
	// Cntrol + E �������ꂽ.
	/*
	void MainPage::kacc_edit_text_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_edit_text().IsEnabled()) {
			edit_text_click(nullptr, nullptr);
		}
	}
	*/
	// Cntrol + F �������ꂽ.
	//void MainPage::kacc_find_text_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_find_text().IsEnabled()) {
	// 	find_text_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + G �������ꂽ.
	//void MainPage::kacc_group_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_group().IsEnabled()) {
	// 	group_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + N �������ꂽ.
	//void MainPage::kacc_new_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// new_click_async(nullptr, nullptr);
	//}

	// Cntrol + O �������ꂽ.
	//void MainPage::kacc_open_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// file_open_click_async(nullptr, nullptr);
	//}

	// Cntrol + V �������ꂽ.
	//void MainPage::kacc_paste_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_xcvd_paste().IsEnabled()) {
	// 	xcvd_paste_click_async(nullptr, nullptr);
	// }
	//}

	// Cntrol + Y �������ꂽ.
	//void MainPage::kacc_redo_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_redo().IsEnabled()) {
	// 	redo_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + Shift + S �������ꂽ.
	//void MainPage::kacc_save_as_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_file_save_as().IsEnabled()) {
	// 	file_save_as_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + S �������ꂽ.
	//void MainPage::kacc_save_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_file_save().IsEnabled()) {
	// 	file_save_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + A �������ꂽ.
	//void MainPage::kacc_select_all_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_select_all().IsEnabled()) {
	// 	select_all_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + PgUp �������ꂽ.
	//void MainPage::kacc_send_backward_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_send_backward().IsEnabled()) {
	// 	arrng_send_backward_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + Home �������ꂽ.
	//void MainPage::kacc_send_to_back_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_send_to_back().IsEnabled()) {
	// 	arrng_send_to_back_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + L �������ꂽ.
	//void MainPage::kacc_summaty_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_smry().IsEnabled()) {
	// 	mfi_smry_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + Z �������ꂽ.
	//void MainPage::kacc_undo_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_undo().IsEnabled()) {
	// 	undo_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + U �������ꂽ.
	//void MainPage::kacc_ungroup_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_ungroup().IsEnabled()) {
	// 	ungroup_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + 0 �������ꂽ.
	//void MainPage::kacc_zoom_reset_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (m_sheet_main.m_sheet_scale != 1.0) {
	// 	mfi_sheet_zoom_100_click(nullptr, nullptr);
	// }
	//}

	//void MainPage::kacc_zoom_in_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//
	//}

	//void MainPage::scp_manipulation_delta(IInspectable const&, ManipulationDeltaRoutedEventArgs const& args)
	//{
	//
	//}

}