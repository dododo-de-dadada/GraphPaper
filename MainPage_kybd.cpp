//-------------------------------
// MainPage_kybd.cpp
// キーボードアクセラレータのハンドラー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// Shift + 下矢印キーが押された.
	void MainPage::ka_range_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	}

	// Shift + 上矢印キーが押された.
	void MainPage::ka_range_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();
	}

	// 下矢印キーが押された.
	void MainPage::ka_select_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	}

	// 上矢印キーが押された.
	void MainPage::ka_select_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::None, VirtualKey::Up>();
	}

	// Escape が押された.
	void MainPage::ka_tool_select_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		const auto t_draw = tool_draw();
		if (t_draw == TOOL_DRAW::SELECT) {
			return;
		}
		rmfi_tool_select().IsChecked(true);
		// グループ名を指定していてもコードビハインドからは
		// 自動でチェックが外れない.
		if (t_draw == TOOL_DRAW::BEZI) {
			rmfi_tool_draw_bezi().IsChecked(false);
		}
		else if (t_draw == TOOL_DRAW::ELLI) {
			rmfi_tool_draw_elli().IsChecked(false);
		}
		else if (t_draw == TOOL_DRAW::LINE) {
			rmfi_tool_draw_line().IsChecked(false);
		}
		else if (t_draw == TOOL_DRAW::POLY) {
			rmfi_tool_draw_poly().IsChecked(false);
		}
		else if (t_draw == TOOL_DRAW::RECT) {
			rmfi_tool_draw_rect().IsChecked(false);
		}
		else if (t_draw == TOOL_DRAW::RRECT) {
			rmfi_tool_draw_rrect().IsChecked(false);
		}
		else if (t_draw == TOOL_DRAW::TEXT) {
			rmfi_tool_draw_text().IsChecked(false);
		}
		else if (t_draw == TOOL_DRAW::RULER) {
			rmfi_tool_draw_ruler().IsChecked(false);
		}
		tool_draw_click(rmfi_tool_select(), nullptr);
	}

	// Cntrol + PgDn が押された.
	//void MainPage::ka_bring_forward_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_bring_forward().IsEnabled()) {
	// 	//arrng_bring_forward_click(nullptr, nullptr);
	// 	arrng_order<S_LIST_T::reverse_iterator>();
	// }
	//}

	// Cntrol + End が押された.
	/*
	void MainPage::ka_bring_to_front_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_bring_to_front().IsEnabled()) {
			constexpr auto FRONT = false;
			arrng_to<FRONT>();
			//arrng_bring_to_front_click(nullptr, nullptr);
		}
	}
	*/

	// Cntrol + C が押された.
	/*
	void MainPage::ka_copy_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_xcvd_copy().IsEnabled()) {
			constexpr uint32_t COPY = 1;
			auto _{ xcvd_copy_async<COPY>() };
			//xcvd_copy_click_async(nullptr, nullptr);
		}
	}*/

	// Cntrol + X が押された.
	/*
	void MainPage::ka_cut_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_xcvd_cut().IsEnabled()) {
			constexpr uint32_t CUT = 0;
			auto _{ xcvd_copy_async<CUT>() };
			//xcvd_cut_click_async(nullptr, nullptr);
		}
	}
	*/
	// Delete が押された.
	/*
	void MainPage::ka_delete_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_xcvd_delete().IsEnabled()) {
			xcvd_delete_click(nullptr, nullptr);
		}
	}
	*/
	// Cntrol + E が押された.
	/*
	void MainPage::ka_edit_text_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_text_edit().IsEnabled()) {
			text_edit_click(nullptr, nullptr);
		}
	}
	*/
	// Cntrol + F が押された.
	//void MainPage::ka_text_find_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_text_find().IsEnabled()) {
	// 	text_find_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + G が押された.
	//void MainPage::ka_group_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_group().IsEnabled()) {
	// 	group_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + N が押された.
	//void MainPage::ka_new_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// new_click_async(nullptr, nullptr);
	//}

	// Cntrol + O が押された.
	//void MainPage::ka_open_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// file_open_click_async(nullptr, nullptr);
	//}

	// Cntrol + V が押された.
	//void MainPage::ka_paste_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_xcvd_paste().IsEnabled()) {
	// 	xcvd_paste_click_async(nullptr, nullptr);
	// }
	//}

	// Cntrol + Y が押された.
	//void MainPage::ka_redo_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_redo().IsEnabled()) {
	// 	redo_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + Shift + S が押された.
	//void MainPage::ka_save_as_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_file_save_as().IsEnabled()) {
	// 	file_save_as_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + S が押された.
	//void MainPage::ka_save_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_file_save().IsEnabled()) {
	// 	file_save_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + A が押された.
	//void MainPage::ka_select_all_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_select_all().IsEnabled()) {
	// 	select_all_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + PgUp が押された.
	//void MainPage::ka_send_backward_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_send_backward().IsEnabled()) {
	// 	arrng_send_backward_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + Home が押された.
	//void MainPage::ka_send_to_back_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_send_to_back().IsEnabled()) {
	// 	arrng_send_to_back_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + L が押された.
	//void MainPage::ka_summaty_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_summary().IsEnabled()) {
	// 	mfi_summary_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + Z が押された.
	//void MainPage::ka_undo_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_undo().IsEnabled()) {
	// 	undo_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + U が押された.
	//void MainPage::ka_ungroup_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (mfi_ungroup().IsEnabled()) {
	// 	ungroup_click(nullptr, nullptr);
	// }
	//}

	// Cntrol + 0 が押された.
	//void MainPage::ka_zoom_reset_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	// if (m_sheet_main.m_sheet_scale != 1.0) {
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