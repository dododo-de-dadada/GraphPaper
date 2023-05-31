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
	//void MainPage::select_shape_range_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//	select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	//}

	// Shift + 上矢印キーが押された.
	//void MainPage::select_shape_range_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//	select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();
	//}

	// 下矢印キーが押された.
	//void MainPage::select_shape_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//	select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	//}

	// 上矢印キーが押された.
	//void MainPage::select_shape_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//	select_next_shape<VirtualKeyModifiers::None, VirtualKey::Up>();
	//}

	// 編集メニューの「すべて選択」が選択された.
	void MainPage::select_shape_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_core_text_shape != nullptr) {
			undo_push_text_select(m_core_text_shape, 0, m_core_text_shape->get_text_len(), false);
			//xcvd_menu_is_enabled();
			main_draw();
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
				undo_push_select(s);
			}
			if (!done) {
				return;
			}
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_select_all();
			}
			//xcvd_menu_is_enabled();
			main_draw();
			status_bar_set_pos();
		}
	}

	// 矩形に含まれる図形を選択し, 含まれない図形の選択を解除する.
	// lt	範囲の左上点
	// rb	範囲の右下点
	bool MainPage::select_shape_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb)
	{
		bool done = false;
		for (auto s : m_main_sheet.m_shape_list) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_inside(lt, rb)) {
				if (!s->is_selected()) {
					undo_push_select(s);
					// 一覧が表示されてるか判定する.
					if (summary_is_visible()) {
						summary_select(s);
					}
					if (!done) {
						done = true;
					}
				}
			}
			else if (s->is_selected()) {
				undo_push_select(s);
				// 一覧が表示されてるか判定する.
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

	// 次の図形を選択する.
	/*
	template <VirtualKeyModifiers M, VirtualKey K> void MainPage::select_next_shape(void)
	{
		Shape* s = static_cast<Shape*>(nullptr);
		if constexpr (K == VirtualKey::Down) {
			if (m_event_shape_last == nullptr) {
				s = slist_front(m_main_sheet.m_shape_list);
				m_event_shape_pressed = s;
			}
			else {
				s = slist_next(m_main_sheet.m_shape_list, m_event_shape_last);
			}
			if (s != nullptr) {
				//m_event_shape_summary = s;
				goto SEL;
			}
		}
		if constexpr (K == VirtualKey::Up) {
			if (m_event_shape_last == nullptr) {
				s = slist_back(m_main_sheet.m_shape_list);
				m_event_shape_pressed = s;
			}
			else {
				s = slist_prev(m_main_sheet.m_shape_list, m_event_shape_last);
			}
			if (s != nullptr) {
				//m_event_shape_summary = s;
				goto SEL;
			}
		}
		return;
	SEL:
		if constexpr (M == VirtualKeyModifiers::Shift) {
			select_shape_range(m_event_shape_pressed, s);
			m_event_shape_last = s;
		}
		if constexpr (M == VirtualKeyModifiers::None) {
			m_event_shape_pressed =
			m_event_shape_last = s;
			unselect_shape_all();
			undo_push_select(s);
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_select(s);
			}
		}
		// 編集メニュー項目の使用の可否を設定する.
		//xcvd_menu_is_enabled();
		main_draw();
	}
	template void MainPage::select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::None, VirtualKey::Up>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();
	*/

	// 指定した範囲の図形を選択, 範囲外の図形の選択を外す.
	// s_from	範囲の最初
	// s_to	範囲の最後
	// 戻り値	選択が変更された true
	bool MainPage::select_shape_range(Shape* const s_from, Shape* const s_to)
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
					undo_push_select(s);
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
					undo_push_select(s);
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
	bool MainPage::toggle_shape_inside(const D2D1_POINT_2F left_top, const D2D1_POINT_2F right_bot)
	{
		bool done = false;
		for (auto s : m_main_sheet.m_shape_list) {
			if (s->is_deleted() || !s->is_inside(left_top, right_bot)) {
				continue;
			}
			undo_push_select(s);
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
	// t_range_only	true なら文字列選択だけを解除. false なら図形の選択も解除.
	// 戻り値	選択が解除された図形があるなら true
	bool MainPage::unselect_shape_all(const bool t_range_only)
	{
		bool changed = false;
		if (m_core_text_shape == nullptr) {
			undo_push_text_unselect(m_core_text_shape);
			m_core_text.NotifyFocusLeave();
			m_core_text_shape = nullptr;
			changed = true;
		}
		for (auto s : m_main_sheet.m_shape_list) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			// 文字列選択だけを解除ではない, かつ選択された図形か判定する.
			if (!t_range_only) {
				undo_push_select(s);
				changed = true;
			}
			if (typeid(*s) != typeid(ShapeText)) {
				continue;
			}
			const ShapeText* t = static_cast<ShapeText*>(s);
			if (m_main_sheet.m_select_start != 0 ||
				m_main_sheet.m_select_end != 0 ||
				m_main_sheet.m_select_trail != false) {
				undo_push_text_select(s, 0, 0, false);
				changed = true;
			}
		}
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_unselect_shape_all();
		}
		return changed;
	}

}