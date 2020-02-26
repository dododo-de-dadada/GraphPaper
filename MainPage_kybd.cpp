//-------------------------------
// MainPage_kybd.cpp
// �L�[�{�[�h�A�N�Z�����[�^�[�̃n���h���[
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// Cntrol + PgDn �������ꂽ.
	void MainPage::ka_bring_forward_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_bring_forward().IsEnabled()) {
			//mfi_bring_forward_click(nullptr, nullptr);
			arrange_order<S_LIST_T::reverse_iterator>();
		}
	}

	// Cntrol + End �������ꂽ.
	void MainPage::ka_bring_to_front_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_bring_to_front().IsEnabled()) {
			constexpr auto FRONT = false;
			arrange_to<FRONT>();
			//mfi_bring_to_front_click(nullptr, nullptr);
		}
	}

	// Cntrol + C �������ꂽ.
	void MainPage::ka_copy_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_copy().IsEnabled()) {
			constexpr uint32_t COPY = 1;
			auto _{ clipboard_copy_async<COPY>() };
			//mfi_copy_click(nullptr, nullptr);
		}
	}

	// Cntrol + X �������ꂽ.
	void MainPage::ka_cut_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_cut().IsEnabled()) {
			constexpr uint32_t CUT = 0;
			auto _{ clipboard_copy_async<CUT>() };
			//mfi_cut_click(nullptr, nullptr);
		}
	}

	// Delete �������ꂽ.
	void MainPage::ka_delete_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_delete().IsEnabled()) {
			delete_selected_shapes();
			//mfi_delete_click(nullptr, nullptr);
		}
	}

	// Cntrol + E �������ꂽ.
	void MainPage::ka_edit_text_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_edit_text().IsEnabled()) {
			edit_text_of_shape();
			//mfi_edit_text_click(nullptr, nullptr);
		}
	}

	// Cntrol + F �������ꂽ.
	void MainPage::ka_find_text_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_find_text().IsEnabled()) {
			find_show_or_hide_panel();
			//mfi_find_text_click(nullptr, nullptr);
		}
	}

	// Cntrol + G �������ꂽ.
	void MainPage::ka_group_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_group().IsEnabled()) {
			mfi_group_click(nullptr, nullptr);
		}
	}

	// Cntrol + N �������ꂽ.
	void MainPage::ka_new_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		mfi_new_click(nullptr, nullptr);
	}

	// Cntrol + O �������ꂽ.
	void MainPage::ka_open_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		mfi_open_click(nullptr, nullptr);
	}

	// Cntrol + V �������ꂽ.
	void MainPage::ka_paste_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_paste().IsEnabled()) {
			mfi_paste_click(nullptr, nullptr);
		}
	}

	// Shift + �����L�[�������ꂽ.
	void MainPage::ka_range_next_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	}

	// Shift + ����L�[�������ꂽ.
	void MainPage::ka_range_prev_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();
	}

	// Cntrol + Y �������ꂽ.
	void MainPage::ka_redo_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_redo().IsEnabled()) {
			mfi_redo_click(nullptr, nullptr);
		}
	}

	// Cntrol + Shift + S �������ꂽ.
	void MainPage::ka_save_as_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_save_as().IsEnabled()) {
			mfi_save_as_click(nullptr, nullptr);
		}
	}

	// Cntrol + S �������ꂽ.
	void MainPage::ka_save_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_save().IsEnabled()) {
			mfi_save_click(nullptr, nullptr);
		}
	}

	// Cntrol + A �������ꂽ.
	void MainPage::ka_select_all_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_select_all().IsEnabled()) {
			mfi_select_all_click(nullptr, nullptr);
		}
	}

	// �����L�[�������ꂽ.
	void MainPage::ka_select_next_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	}

	// ����L�[�������ꂽ.
	void MainPage::ka_select_prev_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		select_next_shape<VirtualKeyModifiers::None, VirtualKey::Up>();
	}

	// Cntrol + PgUp �������ꂽ.
	void MainPage::ka_send_backward_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_send_backward().IsEnabled()) {
			mfi_send_backward_click(nullptr, nullptr);
		}
	}

	// Cntrol + Home �������ꂽ.
	void MainPage::ka_send_to_back_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_send_to_back().IsEnabled()) {
			mfi_send_to_back_click(nullptr, nullptr);
		}
	}

	// Cntrol + L �������ꂽ.
	void MainPage::ka_summaty_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_summary().IsEnabled()) {
			//summary_show_or_hide_panel();
			//mfi_summary_click(nullptr, nullptr);
		}
	}

	// Escape �������ꂽ.
	void MainPage::ka_tool_select_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (m_tool_shape == TOOL_SELECT) {
			return;
		}
		rmfi_tool_select().IsChecked(true);
		// �O���[�v�����w�肵�Ă��Ă��R�[�h�r�n�C���h�����
		// �����Ń`�F�b�N���O��Ȃ�.
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

	// Cntrol + Z �������ꂽ.
	void MainPage::ka_undo_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_undo().IsEnabled()) {
			mfi_undo_click(nullptr, nullptr);
		}
	}

	// Cntrol + U �������ꂽ.
	void MainPage::ka_ungroup_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (mfi_ungroup().IsEnabled()) {
			mfi_ungroup_click(nullptr, nullptr);
		}
	}

	// Cntrol + 0 �������ꂽ.
	void MainPage::ka_zoom_reset_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	{
		if (m_page_panel.m_page_scale != 1.0) {
			mfi_zoom_reset_click(nullptr, nullptr);
		}
	}

	//void MainPage::ka_zoom_in_invoked(IInspectable const& /*sender*/, KeyboardAcceleratorInvokedEventArgs const& /*args*/)
	//{
	//
	//}

	//void MainPage::scp_manipulation_delta(IInspectable const& /*sender*/, ManipulationDeltaRoutedEventArgs const& args)
	//{
	//
	//}

}