#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 線枠メニューの「形式」に印をつける.
	// j_style	破線の種別
	void MainPage::join_style_check_menu(const D2D1_LINE_JOIN j_style)
	{
		if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
			rmfi_join_bevel().IsChecked(true);
			rmfi_join_bevel_2().IsChecked(true);
		}
		else if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER) {
			rmfi_join_miter().IsChecked(true);
			rmfi_join_miter_2().IsChecked(true);
		}
		else if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
			rmfi_join_round().IsChecked(true);
			rmfi_join_round_2().IsChecked(true);
		}
	}

	void MainPage::join_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		D2D1_LINE_JOIN new_value;
		if (sender == rmfi_join_bevel() || sender == rmfi_join_bevel_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;
		}
		else if (sender == rmfi_join_miter() || sender == rmfi_join_miter_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER;
		}
		else if (sender == rmfi_join_round() || sender == rmfi_join_round_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND;
		}
		else {
			return;
		}
		D2D1_LINE_JOIN old_value;
		m_sheet_main.get_stroke_join_style(old_value);
		undo_push_set<UNDO_OP::STROKE_JOIN_STYLE>(new_value);
	}

}