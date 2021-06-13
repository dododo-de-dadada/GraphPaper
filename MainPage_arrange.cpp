//-------------------------------
// MainPage_arrange.cpp
// 図形の並び替え
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr auto SEND_TO_BACK = true;
	using SEND_BACKWARD = SHAPE_LIST::iterator;
	using BRING_FORWARD = SHAPE_LIST::reverse_iterator;
	constexpr auto BRING_TO_FRONT = false;

	// 編集メニューの「前面に移動」が選択された.
	void MainPage::arrange_bring_forward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択された図形を次または前の図形と入れ替える.
		arrange_order<BRING_FORWARD>();
	}

	// 編集メニューの「最前面に移動」が選択された.
	void MainPage::arrange_bring_to_front_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択された図形を最背面または最前面に移動する.
		arrange_to<BRING_TO_FRONT>();
	}

	// 選択された図形を次の図形と入れ替える.
	template void MainPage::arrange_order<BRING_FORWARD>(void);

	// 選択された図形を前の図形と入れ替える.
	template void MainPage::arrange_order<SEND_BACKWARD>(void);

	// 選択された図形を次または前の図形と入れ替える.
	// T	T が iterator の場合は背面の図形と入れ替え, reverse_iterator の場合は前面の図形と入れ替える. 
	template<typename T> void MainPage::arrange_order(void)
	{
		T it_end;	// 終端
		T it_src;	// 交換元反復子
		if constexpr (std::is_same<T, BRING_FORWARD>::value) {
			it_end = m_list_shapes.rend();
			it_src = m_list_shapes.rbegin();
		}
		else {
			if constexpr (std::is_same<T, SEND_BACKWARD>::value) {
				it_end = m_list_shapes.end();
				it_src = m_list_shapes.begin();
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
			if (!(*it_src)->is_deleted() && !(*it_src)->is_selected()) {
				break;
			}
			it_src++;
		}
		auto flag = false;	// 交換フラグ
		for (;;) {
			// 交換元反復子を交換先反復子に格納して,
			// 交換元反復子をインクリメントする.
			auto it_dst = it_src++;
			// 削除されてない次の図形を得る.
			for (;;) {
				// 次の図形がないか判定する.
				if (it_src == it_end) {
					// 交換フラグが立っているか判定する.
					if (flag == true) {
						undo_push_null();
						xcvd_is_enabled();
						sheet_draw();
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
			auto s = *it_src;
			if (s->is_selected() != true) {
				continue;
			}
			auto t = *it_dst;
			// 図形一覧の排他制御が true か判定する.
			if (m_summary_atomic.load(std::memory_order_acquire)) {
				summary_arrange(s, t);
			}
			undo_push_arrange(s, t);
			// 交換フラグを立てる.
			flag = true;
		}
	}

	// 編集メニューの「ひとつ背面に移動」が選択された.
	void MainPage::arrange_send_backward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択された図形を次または前の図形と入れ替える.
		arrange_order<SEND_BACKWARD>();
	}

	// 編集メニューの「最背面に移動」が選択された.
	void MainPage::arrange_send_to_back_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択された図形を最背面または最前面に移動する.
		arrange_to<SEND_TO_BACK>();
	}

	// 選択された図形を最背面または最前面に移動する.
	// B	B が true の場合は最背面, false の場合は最前面に移動
	template<bool B> void MainPage::arrange_to(void)
	{
		using winrt::Windows::UI::Xaml::Controls::ItemCollection;

		SHAPE_LIST slist;
		slist_selected<Shape>(m_list_shapes, slist);
		if (slist.size() == 0) {
			return;
		}
		if constexpr (B) {
			uint32_t i = 0;
			auto s = slist_front(m_list_shapes);
			for (auto t : slist) {
				// 図形一覧の排他制御が true か判定する.
				if (m_summary_atomic.load(std::memory_order_acquire)) {
					summary_remove(t);
					summary_insert_at(t, i++);
				}
				undo_push_remove(t);
				undo_push_insert(t, s);
			}
		}
		else {
			for (auto s : slist) {
				// 図形一覧の排他制御が true か判定する.
				if (m_summary_atomic.load(std::memory_order_acquire)) {
					summary_remove(s);
					summary_append(s);
				}
				undo_push_remove(s);
				undo_push_insert(s, nullptr);
			}
		}
		slist.clear();
		undo_push_null();
		xcvd_is_enabled();
		sheet_draw();
	}

	// 選択された図形を最前面に移動する.
	template void MainPage::arrange_to<BRING_TO_FRONT>(void);

	// 選択された図形を最背面に移動する.
	template void MainPage::arrange_to<SEND_TO_BACK>(void);

}