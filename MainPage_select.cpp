//-------------------------------
// MainPage_select.cpp
// 図形の選択
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::ListViewItem;

	// Shift + 下矢印キーが押された.
	//void MainPage::select_range_shape_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//	select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	//}

	// Shift + 上矢印キーが押された.
	//void MainPage::select_range_shape_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//	select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();
	//}

	// 下矢印キーが押された.
	//void MainPage::select_shape_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//	select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	//}

	// 編集メニューの「すべて選択」が選択された.
	void MainPage::select_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (!m_main_sheet_focused) {
			return;
		}
		if (m_core_text_focused != nullptr) {
			undo_push_text_select(m_core_text_focused, 0, m_core_text_focused->get_text_len(), false);
			menu_is_enable();
			main_sheet_draw();
		}
		else {
			bool done = false;
			for (auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted() || s->is_selected()) {
					continue;
				}
				if (!done) {
					done = true;
				}
				undo_push_toggle(s);
			}
			if (!done) {
				return;
			}
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_select_all();
			}
			menu_is_enable();
			main_sheet_draw();
			status_bar_set_pointer();
		}
	}

	// 矩形に含まれる図形を選択し, 含まれない図形の選択を解除する.
	// lt	範囲の左上点
	// rb	範囲の右下点
	bool MainPage::select_inside_shape(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb)
	{
		bool change = false;
		for (auto s : m_main_sheet.m_shape_list) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_inside(lt, rb)) {
				if (!s->is_selected()) {
					undo_push_toggle(s);
					// 一覧が表示されてるか判定する.
					if (summary_is_visible()) {
						summary_select(s);
					}
					change = true;
				}
			}
			else if (s->is_selected()) {
				undo_push_toggle(s);
				// 一覧が表示されてるか判定する.
				if (summary_is_visible()) {
					summary_unselect(s);
				}
				change = true;
			}
		}
		return change;
	}

	// 指定した範囲の図形を選択, 範囲外の図形の選択を外す.
	// s_from	範囲の最初
	// s_to	範囲の最後
	// 戻り値	選択が変更された true
	bool MainPage::select_range_shape(Shape* const s_from, Shape* const s_to)
	{
		constexpr int BEGIN = 0;
		constexpr int NEXT = 1;
		constexpr int END = 2;
		auto done = false;
		auto st = BEGIN;
		auto s_end = static_cast<Shape*>(nullptr);
		auto i = 0u;
		for (auto s : m_main_sheet.m_shape_list) {
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
					undo_push_toggle(s);
					// 一覧が表示されてるか判定する.
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
					undo_push_toggle(s);
					// 一覧が表示されてるか判定する.
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

	// 矩形に含まれる図形の選択を反転する.
	// left_top	矩形の左上点
	// right_bot	矩形の右下点
	bool MainPage::toggle_inside_shape(const D2D1_POINT_2F left_top, const D2D1_POINT_2F right_bot)
	{
		bool done = false;
		for (auto s : m_main_sheet.m_shape_list) {
			if (s->is_deleted() || !s->is_inside(left_top, right_bot)) {
				continue;
			}
			undo_push_toggle(s);
			// 一覧が表示されてるか判定する.
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

	// 図形の選択をすべて解除する.
	// 戻り値	選択が解除された図形があるなら true
	bool MainPage::unselect_all_shape(void)
	{
		bool changed = false;
		if (m_core_text_focused != nullptr) {
			undo_push_text_unselect(m_core_text_focused);
			m_core_text.NotifyFocusLeave();
			m_core_text_focused = nullptr;
			changed = true;
		}
		for (auto s : m_main_sheet.m_shape_list) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			undo_push_toggle(s);
			changed = true;
		}
		// 一覧が表示されてるか判定する.
		if (changed && summary_is_visible()) {
			summary_unselect_all_shape();
		}
		return changed;
	}

	// 図形を選択して, それ以外の図形の選択をはずす.
	bool MainPage::select_shape(Shape* const s)
	{
		bool changed = false;
		if (m_core_text_focused != nullptr) {
			undo_push_text_unselect(m_core_text_focused);
			m_core_text.NotifyFocusLeave();
			m_core_text_focused = nullptr;
			changed = true;
		}
		for (auto t : m_main_sheet.m_shape_list) {
			if (t->is_deleted()) {
				continue;
			}
			if (t != s) {
				if (t->is_selected()) {
					undo_push_toggle(t);
					changed = true;
				}
			}
			else {
				if (!t->is_selected()) {
					undo_push_toggle(t);
					changed = true;
				}
			}
		}
		// 一覧が表示されてるか判定する.
		if (changed && summary_is_visible()) {
			summary_select_shape(s);
		}
		return changed;
	}
}