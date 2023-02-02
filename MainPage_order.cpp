//-------------------------------
// MainPage_order.cpp
// 図形の並び替え
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	constexpr bool SEND_TO_BACK = true;
	using SEND_BACKWARD = SHAPE_LIST::iterator;
	using BRING_FORWARD = SHAPE_LIST::reverse_iterator;
	constexpr bool BRING_TO_FRONT = false;

	// 編集メニューの「前面に移動」が選択された.
	void MainPage::order_bring_forward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択された図形を次または前の図形と入れ替える.
		order_swap<BRING_FORWARD>();
		status_bar_set_pos();
	}

	// 編集メニューの「最前面に移動」が選択された.
	void MainPage::order_bring_to_front_click(IInspectable const&, RoutedEventArgs const&)
	{
		SHAPE_LIST slist;	// 選択された図形のリスト
		slist_get_selected<Shape>(m_main_page.m_shape_list, slist);
		if (slist.size() > 0) {
			// 最前面 (リストでは末尾) に移動
			for (Shape* const s : slist) {
				if (summary_is_visible()) {
					summary_remove(s);
					summary_append(s);
				}
				ustack_push_remove(s);
				ustack_push_insert(s, nullptr);
			}
		}
		slist.clear();
		ustack_push_null();
		xcvd_is_enabled();
		page_draw();
		status_bar_set_pos();
	}

	// 選択された図形を次の図形と入れ替える.
	template void MainPage::order_swap<BRING_FORWARD>(void);

	// 選択された図形を前の図形と入れ替える.
	template void MainPage::order_swap<SEND_BACKWARD>(void);

	// 選択された図形を次または前の図形と入れ替える.
	// T	T が iterator の場合は背面の図形と入れ替え, reverse_iterator の場合は前面の図形と入れ替える. 
	template<typename T> void MainPage::order_swap(void)
	{
		T it_end;	// 終端
		T it_src;	// 交換元
		if constexpr (std::is_same<T, BRING_FORWARD>::value) {
			it_end = m_main_page.m_shape_list.rend();
			it_src = m_main_page.m_shape_list.rbegin();
		}
		else {
			if constexpr (std::is_same<T, SEND_BACKWARD>::value) {
				it_end = m_main_page.m_shape_list.end();
				it_src = m_main_page.m_shape_list.begin();
			}
			else {
				throw winrt::hresult_not_implemented();
			}
		}
		// 選択されていない図形の中から最初の図形を得る.
		for (;;) {
			// 次の図形がないか判定する.
			if (it_src == it_end) {
				return;
			}
			// 消去フラグも選択フラグも立っていないか判定する.
			const Shape* s = *it_src;
			if (!s->is_deleted() && !s->is_selected()) {
				break;
			}
			it_src++;
		}
		bool done = false;	// 交換済みか判定する
		for (;;) {
			// 交換元反復子を交換先反復子に格納して,
			// 交換元反復子をインクリメントする.
			T it_dst = it_src++;
			// 削除されてない次の図形を得る.
			for (;;) {
				// 次の図形がないか判定する.
				if (it_src == it_end) {
					// 交換済みか判定する
					if (done) {
						ustack_push_null();
						xcvd_is_enabled();
						page_draw();
					}
					return;
				}
				// 交換元の削除フラグが立ってないか判定する.
				if (!(*it_src)->is_deleted()) {
					break;
				}
				it_src++;
			}
			// 次の図形が選択されてない場合,
			Shape* const s = *it_src;
			if (!s->is_selected()) {
				continue;
			}
			Shape* const t = *it_dst;
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_order(s, t);
			}
			ustack_push_order(s, t);
			// 交換済みにする.
			done = true;
		}
	}

	// 編集メニューの「ひとつ背面に移動」が選択された.
	void MainPage::order_send_backward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択された図形を次または前の図形と入れ替える.
		order_swap<SEND_BACKWARD>();
		status_bar_set_pos();
	}

	// 編集メニューの「最背面に移動」が選択された.
	void MainPage::order_send_to_back_click(IInspectable const&, RoutedEventArgs const&)
	{
		SHAPE_LIST slist;	// 選択された図形のリスト
		slist_get_selected<Shape>(m_main_page.m_shape_list, slist);
		if (slist.size() > 0) {
			// 最背面 (リストでは先頭) に移動
			uint32_t i = 0;
			Shape* const s = slist_front(m_main_page.m_shape_list);
			for (Shape* const t : slist) {
				if (summary_is_visible()) {
					summary_remove(t);
					summary_insert_at(t, i++);
				}
				ustack_push_remove(t);
				ustack_push_insert(t, s);
			}
		}
		slist.clear();
		ustack_push_null();
		xcvd_is_enabled();
		page_draw();
		status_bar_set_pos();
	}

}