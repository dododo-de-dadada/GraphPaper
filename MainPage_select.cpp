//-------------------------------
// MainPage_select.cpp
// 図形の選択
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 編集メニューの「すべて選択」が選択された.
	void MainPage::select_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		bool flag = false;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected()) {
				continue;
			}
			flag = true;
			undo_push_select(s);
		}
		if (flag != true) {
			return;
		}
		// 図形一覧の排他制御が true か判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			summary_select_all();
		}
		// 編集メニュー項目の使用の可否を設定する.
		xcvd_is_enabled();
		sheet_draw();
	}

	// 範囲に含まれる図形を選択し, 含まれない図形の選択を解除する.
	bool MainPage::select_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max)
	{
		bool flag = false;
		//uint32_t i = 0u;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->in_area(a_min, a_max)) {
				if (s->is_selected() != true) {
					undo_push_select(s);
					// 図形一覧の排他制御が true か判定する.
					if (m_summary_atomic.load(std::memory_order_acquire)) {
						summary_select(s);
					}
					flag = true;
				}
			}
			else {
				if (s->is_selected()) {
					undo_push_select(s);
					// 図形一覧の排他制御が true か判定する.
					if (m_summary_atomic.load(std::memory_order_acquire)) {
						summary_unselect(s);
					}
					flag = true;
				}
			}
			//i++;
		}
		return flag;
	}

	// 次の図形を選択する.
	template <VirtualKeyModifiers M, VirtualKey K> void MainPage::select_next_shape(void)
	{
		Shape* s = static_cast<Shape*>(nullptr);
		if constexpr (K == VirtualKey::Down) {
			if (m_event_shape_prev == nullptr) {
				s = slist_front(m_list_shapes);
				m_event_shape_pressed = s;
			}
			else {
				s = slist_next(m_list_shapes, m_event_shape_prev);
			}
			if (s != nullptr) {
				//m_event_shape_summary = s;
				goto SEL;
			}
		}
		if constexpr (K == VirtualKey::Up) {
			if (m_event_shape_prev == nullptr) {
				s = slist_back(m_list_shapes);
				m_event_shape_pressed = s;
			}
			else {
				s = slist_prev(m_list_shapes, m_event_shape_prev);
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
			undo_push_select(s);
			// 図形一覧の排他制御が true か判定する.
			if (m_summary_atomic.load(std::memory_order_acquire)) {
				summary_select(s);
			}
		}
		// 編集メニュー項目の使用の可否を設定する.
		xcvd_is_enabled();
		sheet_draw();
	}
	template void MainPage::select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::None, VirtualKey::Up>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	template void MainPage::select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();


	// 指定した範囲にある図形を選択して, そうでない図形は選択しない.
	// s_from	最初の図形
	// s_to	最後の図形
	// 戻り値	選択が変更された true
	bool MainPage::select_range(Shape* const s_from, Shape* const s_to)
	{
		constexpr int BEGIN = 0;
		constexpr int NEXT = 1;
		constexpr int END = 2;
		auto flag = false;
		auto st = BEGIN;
		auto s_end = static_cast<Shape*>(nullptr);
		auto i = 0u;
		for (auto s : m_list_shapes) {
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
						flag = true;
						undo_push_select(s);
						// 図形一覧の排他制御が true か判定する.
						if (m_summary_atomic.load(std::memory_order_acquire)) {
							summary_unselect(s);
						}
					}
					break;
				}
				[[fallthrough]];
			case NEXT:
				if (s->is_selected() != true) {
					flag = true;
					undo_push_select(s);
					// 図形一覧の排他制御が true か判定する.
					if (m_summary_atomic.load(std::memory_order_acquire)) {
						summary_select(s);
					}
				}
				if (s == s_end) {
					st = END;
				}
				break;
			}
			i++;
		}
		return flag;
	}

	// 図形を選択する.
	void MainPage::select_shape(Shape* const s, const VirtualKeyModifiers k_mod)
	{
		using winrt::Windows::UI::Xaml::Controls::ListViewItem;

		// コントロールキーが押されているか判定する.
		if (k_mod == VirtualKeyModifiers::Control) {
			undo_push_select(s);
			xcvd_is_enabled();
			sheet_draw();
			// 図形一覧の排他制御が true か判定する.
			if (m_summary_atomic.load(std::memory_order_acquire)) {
				if (s->is_selected()) {
					summary_select(s);
				}
				else {
					summary_unselect(s);
				}
			}
			m_event_shape_prev = s;
		}
		// シフトキーが押されているか判定する.
		else if (k_mod == VirtualKeyModifiers::Shift) {
			// 前回ポインターが押された図形から今回押された図形までの
			// 範囲にある図形を選択して, そうでない図形を選択しない.
			if (m_event_shape_prev == nullptr) {
				m_event_shape_prev = m_list_shapes.front();
			}
			if (select_range(s, m_event_shape_prev)) {
				// 編集メニュー項目の使用の可否を設定する.
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		else {
			// シフトキーもコントロールキーも押されてないならば,
			// 図形の選択フラグが立ってないか判定する.
			if (!s->is_selected()) {
				unselect_all();
				undo_push_select(s);
				xcvd_is_enabled();
				sheet_draw();
				// 図形一覧の排他制御が true か判定する.
				if (m_summary_atomic.load(std::memory_order_acquire)) {
					summary_select(s);
				}
			}
			m_event_shape_prev = s;
		}
		if (s->is_selected()) {
			// 押された図形が選択されている場合,
			m_sheet_main.set_attr_to(s);
			sheet_attr_is_checked();
		}
	}

	// 範囲に含まれる図形の選択を反転する.
	// a_min	範囲の左上位置
	// a_max	範囲の右下位置
	bool MainPage::toggle_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max)
	{
		auto flag = false;
		//uint32_t i = 0;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->in_area(a_min, a_max)) {
				undo_push_select(s);
				// 図形一覧の排他制御が true か判定する.
				if (m_summary_atomic.load(std::memory_order_acquire)) {
					if (s->is_selected() != true) {
						summary_select(s);
					}
					else {
						summary_unselect(s);
					}
				}
				flag = true;
			}
			//i++;
		}
		return flag;
	}

	// 図形の選択をすべて解除する.
	// t_range_only	文字範囲のみ解除フラグ
	// 戻り値	選択が解除された図形がある場合 true, ない場合 false
	// 文字範囲のみフラグが立っている場合, 文字範囲の選択のみ解除される.
	// 文字範囲のみフラグがない場合, 図形の選択も文字範囲の選択も両方解除される.
	bool MainPage::unselect_all(const bool t_range_only)
	{
		auto flag = false;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (t_range_only != true && s->is_selected()) {
				// 文字範囲のみ解除フラグがない, かつ図形の選択フラグが立っている場合,
				undo_push_select(s);
				flag = true;
			}
			DWRITE_TEXT_RANGE d_range;
			if (s->get_text_range(d_range) != true) {
				// 文字範囲が取得できない
				// (文字列図形でない) 場合,
				// 以下を無視する.
				continue;
			}
			const DWRITE_TEXT_RANGE s_range = DWRITE_TEXT_RANGE{ 0, 0 };
			if (equal(s_range, d_range)) {
				// 得た文字範囲が { 0, 0 } の場合,
				// 以下を無視する.
				continue;
			}
			// { 0, 0 } を図形に格納して, その操作をスタックに積む.
			undo_push_set<UNDO_OP::TEXT_RANGE>(s, s_range);
			flag = true;
		}
		// 図形一覧の排他制御が true か判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			summary_unselect_all();
		}
		return flag;
	}

	// Shift + 下矢印キーが押された.
	void MainPage::kacc_range_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Down>();
	}

	// Shift + 上矢印キーが押された.
	void MainPage::kacc_range_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::Shift, VirtualKey::Up>();
	}

	// 下矢印キーが押された.
	void MainPage::kacc_select_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::None, VirtualKey::Down>();
	}

	// 上矢印キーが押された.
	void MainPage::kacc_select_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
	{
		select_next_shape<VirtualKeyModifiers::None, VirtualKey::Up>();
	}

}