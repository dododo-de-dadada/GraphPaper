//-------------------------------
// MainPage_select.cpp
// 図形の選択
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::System::VirtualKeyModifiers;
	using winrt::Windows::UI::Xaml::Controls::ListViewItem;
	//using winrt::Windows::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs;
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	// Escape が押された.
	void MainPage::select_tool_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		if (m_drawing_tool == DRAWING_TOOL::SELECT) {
			unselect_all();
			page_draw();
		}
		else {
			drawing_tool_click(rmfi_tool_selecting(), nullptr);
		}
	}

	// Shift + 下矢印キーが押された.
	//void MainPage::select_range_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	//{
	//	select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	//}

	// Shift + 上矢印キーが押された.
	//void MainPage::select_range_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
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
	void MainPage::select_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		bool done = false;
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted() || s->is_selected()) {
				continue;
			}
			if (!done) {
				done = true;
			}
			ustack_push_select(s);
		}
		if (!done) {
			return;
		}
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_select_all();
		}
		xcvd_is_enabled();
		page_draw();
	}

	// 範囲に含まれる図形を選択し, 含まれない図形の選択を解除する.
	bool MainPage::select_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb)
	{
		bool done = false;
		//uint32_t i = 0u;
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->in_area(area_lt, area_rb)) {
				if (!s->is_selected()) {
					ustack_push_select(s);
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
				ustack_push_select(s);
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
			if (m_event_shape_prev == nullptr) {
				s = slist_front(m_main_page.m_shape_list);
				m_event_shape_pressed = s;
			}
			else {
				s = slist_next(m_main_page.m_shape_list, m_event_shape_prev);
			}
			if (s != nullptr) {
				//m_event_shape_summary = s;
				goto SEL;
			}
		}
		if constexpr (K == VirtualKey::Up) {
			if (m_event_shape_prev == nullptr) {
				s = slist_back(m_main_page.m_shape_list);
				m_event_shape_pressed = s;
			}
			else {
				s = slist_prev(m_main_page.m_shape_list, m_event_shape_prev);
			}
			if (s != nullptr) {
				//m_event_shape_summary = s;
				goto SEL;
			}
		}
		return;
	SEL:
		if constexpr (M == VirtualKeyModifiers::Shift) {
			select_range(m_event_shape_pressed, s);
			m_event_shape_prev = s;
		}
		if constexpr (M == VirtualKeyModifiers::None) {
			m_event_shape_pressed =
			m_event_shape_prev = s;
			unselect_all();
			ustack_push_select(s);
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_select(s);
			}
		}
		// 編集メニュー項目の使用の可否を設定する.
		xcvd_is_enabled();
		page_draw();
	}
	template void MainPage::select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::None, VirtualKey::Up>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();
	*/

	// 範囲の中の図形を選択して, それ以外の図形は選択をはずす.
	// s_from	最初の図形
	// s_to	最後の図形
	// 戻り値	選択が変更された true
	bool MainPage::select_range(Shape* const s_from, Shape* const s_to)
	{
		constexpr int BEGIN = 0;
		constexpr int NEXT = 1;
		constexpr int END = 2;
		auto done = false;
		auto st = BEGIN;
		auto s_end = static_cast<Shape*>(nullptr);
		auto i = 0u;
		for (auto s : m_main_page.m_shape_list) {
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
						ustack_push_select(s);
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
					ustack_push_select(s);
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

	// 図形を選択する.
	void MainPage::select_shape(Shape* const s, const VirtualKeyModifiers k_mod)
	{
		// コントロールキーが押されているか判定する.
		if (k_mod == VirtualKeyModifiers::Control) {
			ustack_push_select(s);
			xcvd_is_enabled();
			page_draw();
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				if (s->is_selected()) {
					summary_select(s);
				}
				else {
					summary_unselect(s);
				}
			}
			m_event_shape_prev = s;
		}
		// シフトキーが押されて, かつ直前に押された図形があり, その図形が選択されているか判定する.
		else if (k_mod == VirtualKeyModifiers::Shift &&
			m_event_shape_prev != nullptr && m_event_shape_prev->is_selected() && !m_event_shape_prev->is_deleted()) {
			// 前回ポインターが押された図形が空か判定する.
			//if (m_event_shape_prev == nullptr) {
			//	// 図形リストの先頭を前回ポインターが押された図形に格納する.
			//	m_event_shape_prev = m_main_page.m_shape_list.front();
			//}
			// 範囲の中の図形は選択して, それ以外の図形の選択をはずす.
			if (select_range(s, m_event_shape_prev)) {
				xcvd_is_enabled();
				page_draw();
			}
		}
		else {
			// 上記以外なら,
			// 図形が選択されてるか判定する.
			if (!s->is_selected()) {
				unselect_all();
				ustack_push_select(s);
				xcvd_is_enabled();
				page_draw();
				// 一覧が表示されてるか判定する.
				if (summary_is_visible()) {
					summary_select(s);
				}
			}
			m_event_shape_prev = s;
		}
		if (s->is_selected()) {
			// 押された図形が選択されている場合,
//			m_main_page.set_attr_to(s);
//			page_setting_is_checked();
		}
	}

	// 範囲に含まれる図形の選択を反転する.
	// area_lt	範囲の左上位置
	// area_rb	範囲の右下位置
	bool MainPage::toggle_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb)
	{
		bool done = false;
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted() || !s->in_area(area_lt, area_rb)) {
				continue;
			}
			ustack_push_select(s);
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
	// t_range_only	文字範囲だけ解除 (文字範囲だけでないなら, 図形の選択も解除される).
	// 戻り値	選択が解除された図形があるなら true
	bool MainPage::unselect_all(const bool t_range_only)
	{
		bool done = false;
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted()) {
				continue;
			}
			// 文字範囲だけ解除でない, かつ図形が選択されているか判定する.
			if (!t_range_only && s->is_selected()) {
				ustack_push_select(s);
				if (!done) {
					done = true;
				}
			}
			// 文字範囲が取得できない (文字列図形でない場合も含む) か判定する.
			DWRITE_TEXT_RANGE d_range;
			if (!s->get_text_selected(d_range)) {
				continue;
			}
			// 得た文字範囲が { 0, 0 } か判定する.
			constexpr DWRITE_TEXT_RANGE s_range = DWRITE_TEXT_RANGE{ 0, 0 };
			if (equal(s_range, d_range)) {
				continue;
			}
			// { 0, 0 } を図形に格納して, その操作をスタックに積む.
			ustack_push_set<UNDO_ID::TEXT_RANGE>(s, s_range);
			if (!done) {
				done = true;
			}
		}
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_unselect_all();
		}
		return done;
	}

}