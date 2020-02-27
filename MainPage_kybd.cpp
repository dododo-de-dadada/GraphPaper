//-------------------------------
// MainPage_kybd.cpp
// キーボードアクセラレーターのハンドラー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// Shift + 下矢印キーが押された.
	void MainPage::ka_range_next_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	}

	// Shift + 上矢印キーが押された.
	void MainPage::ka_range_prev_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();
	}

	// 下矢印キーが押された.
	void MainPage::ka_select_next_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	}

	// 上矢印キーが押された.
	void MainPage::ka_select_prev_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		select_next_shape<VirtualKeyModifiers::None, VirtualKey::Up>();
	}

	// Escape が押された.
	void MainPage::ka_tool_select_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (m_tool_shape == TOOL_SELECT) {
			return;
		}
		rmfi_tool_select().IsChecked(true);
		// グループ名を指定していてもコードビハインドからは
		// 自動でチェックが外れない.
		switch (m_tool_shape) {
		case TOOL_BEZI:
			rmfi_tool_bezi().IsChecked(false);
			break;
		case TOOL_ELLI:
			rmfi_tool_elli().IsChecked(false);
			break;
		case TOOL_LINE:
			rmfi_tool_line().IsChecked(false);
			break;
		case TOOL_QUAD:
			rmfi_tool_quad().IsChecked(false);
			break;
		case TOOL_RECT:
			rmfi_tool_rect().IsChecked(false);
			break;
		case TOOL_RRECT:
			rmfi_tool_rrect().IsChecked(false);
			break;
		case TOOL_TEXT:
			rmfi_tool_text().IsChecked(false);
			break;
		case TOOL_RULER:
			rmfi_tool_ruler().IsChecked(false);
			break;
		}
		rmfi_tool_select_click(nullptr, nullptr);
	}

	// Cntrol + PgDn が押された.
	//void MainPage::ka_bring_forward_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (mfi_bring_forward().IsEnabled()) {
	//		//mfi_bring_forward_click(nullptr, nullptr);
	//		arrange_order<S_LIST_T::reverse_iterator>();
	//	}
	//}

	// Cntrol + End が押された.
	/*
	void MainPage::ka_bring_to_front_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_bring_to_front().IsEnabled()) {
			constexpr auto FRONT = false;
			arrange_to<FRONT>();
			//mfi_bring_to_front_click(nullptr, nullptr);
		}
	}
	*/

	// Cntrol + C が押された.
	/*
	void MainPage::ka_copy_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_copy().IsEnabled()) {
			constexpr uint32_t COPY = 1;
			auto _{ clipboard_copy_async<COPY>() };
			//mfi_copy_click(nullptr, nullptr);
		}
	}*/

	// Cntrol + X が押された.
	/*
	void MainPage::ka_cut_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_cut().IsEnabled()) {
			constexpr uint32_t CUT = 0;
			auto _{ clipboard_copy_async<CUT>() };
			//mfi_cut_click(nullptr, nullptr);
		}
	}
	*/
	// Delete が押された.
	/*
	void MainPage::ka_delete_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_delete().IsEnabled()) {
			mfi_delete_click(nullptr, nullptr);
		}
	}
	*/
	// Cntrol + E が押された.
	/*
	void MainPage::ka_edit_text_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (mfi_edit_text().IsEnabled()) {
			mfi_edit_text_click(nullptr, nullptr);
		}
	}
	*/
	// Cntrol + F が押された.
	//void MainPage::ka_find_text_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (mfi_find_text().IsEnabled()) {
	//		mfi_find_text_click(nullptr, nullptr);
	//	}
	//}

	// Cntrol + G が押された.
	//void MainPage::ka_group_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (mfi_group().IsEnabled()) {
	//		mfi_group_click(nullptr, nullptr);
	//	}
	//}

	// Cntrol + N が押された.
	//void MainPage::ka_new_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	mfi_new_click(nullptr, nullptr);
	//}

	// Cntrol + O が押された.
	//void MainPage::ka_open_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	mfi_open_click(nullptr, nullptr);
	//}

	// Cntrol + V が押された.
	//void MainPage::ka_paste_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (mfi_paste().IsEnabled()) {
	//		mfi_paste_click(nullptr, nullptr);
	//	}
	//}

	// Cntrol + Y が押された.
	//void MainPage::ka_redo_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (mfi_redo().IsEnabled()) {
	//		mfi_redo_click(nullptr, nullptr);
	//	}
	//}

	// Cntrol + Shift + S が押された.
	//void MainPage::ka_save_as_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (mfi_save_as().IsEnabled()) {
	//		mfi_save_as_click(nullptr, nullptr);
	//	}
	//}

	// Cntrol + S が押された.
	//void MainPage::ka_save_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (mfi_save().IsEnabled()) {
	//		mfi_save_click(nullptr, nullptr);
	//	}
	//}

	// Cntrol + A が押された.
	//void MainPage::ka_select_all_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (mfi_select_all().IsEnabled()) {
	//		mfi_select_all_click(nullptr, nullptr);
	//	}
	//}

	// Cntrol + PgUp が押された.
	//void MainPage::ka_send_backward_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (mfi_send_backward().IsEnabled()) {
	//		mfi_send_backward_click(nullptr, nullptr);
	//	}
	//}

	// Cntrol + Home が押された.
	//void MainPage::ka_send_to_back_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (mfi_send_to_back().IsEnabled()) {
	//		mfi_send_to_back_click(nullptr, nullptr);
	//	}
	//}

	// Cntrol + L が押された.
	//void MainPage::ka_summaty_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (mfi_summary().IsEnabled()) {
	//		mfi_summary_click(nullptr, nullptr);
	//	}
	//}

	// Cntrol + Z が押された.
	//void MainPage::ka_undo_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (mfi_undo().IsEnabled()) {
	//		mfi_undo_click(nullptr, nullptr);
	//	}
	//}

	// Cntrol + U が押された.
	//void MainPage::ka_ungroup_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (mfi_ungroup().IsEnabled()) {
	//		mfi_ungroup_click(nullptr, nullptr);
	//	}
	//}

	// Cntrol + 0 が押された.
	//void MainPage::ka_zoom_reset_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//	if (m_page_panel.m_page_scale != 1.0) {
	//		mfi_zoom_reset_click(nullptr, nullptr);
	//	}
	//}

	//void MainPage::ka_zoom_in_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//
	//}

	//void MainPage::scp_manipulation_delta(IInspectable const& /*sender*/, ManipulationDeltaRoutedEventArgs const& args)
	//{
	//
	//}

}