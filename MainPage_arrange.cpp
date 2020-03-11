//-------------------------------
// MainPage_arrange.cpp
// 図形リストの要素の並び替え
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 選択された図形を次または前の図形と入れ替える.
	template<typename T>
	void MainPage::arrange_order(void)
	{
		T it_end;	// 終端の反復子
		T it_src;	// 交換元の反復子
		auto flag = false;
		if constexpr (std::is_same<T, FORWARD>::value) {
			it_end = m_list_shapes.rend();
			it_src = m_list_shapes.rbegin();
		}
		else {
			if constexpr (std::is_same<T, BACKWARD>::value) {
				it_end = m_list_shapes.end();
				it_src = m_list_shapes.begin();
			}
			else {
				throw winrt::hresult_not_implemented();
			}
		}
		for (;;) {
			if ((*it_src)->is_deleted() == false) {
				break;
			}
			it_src++;
		}
		for (;;) {
			auto it_dst = it_src++;
			while (it_src != it_end) {
				if ((*it_src)->is_deleted() == false) {
					break;
				}
				it_src++;
			}
			if (it_src == it_end) {
				// 交換元の反復子が終端の場合,
				// 中断する.
				break;
			}
			auto s = *it_src;	// 交換元の図形
			if (s->is_deleted()) {
				// 消去フラグが立っている場合,
				// 交換元の反復子をインクリメントする.
				// 以下を無視する.
				it_src++;
				continue;
			}
			// 交換元の反復子が指す図形が選択されていて,
			// 交換先の反復子が指す図形が選択されてない場合,
			// それらの図形の組を組リストに保存し, 入れ替える.
			auto t = *it_dst;	// 交換先の図形
			if (s->is_selected() && !t->is_selected()) {
				flag = true;
				if (m_summary_visible) {
					summary_arrange(s, t);
				}
				undo_push_arrange(s, t);
			}
		}
		if (flag == true) {
			// 一連の操作の区切としてヌル操作をスタックに積む.
			undo_push_null();
			// 編集メニュー項目の使用の可否を設定する.
			enable_edit_menu();
			page_draw();
		}
	}
	using BACKWARD = S_LIST_T::iterator;
	using FORWARD = S_LIST_T::reverse_iterator;
	template void MainPage::arrange_order<BACKWARD>(void);
	template void MainPage::arrange_order<FORWARD>(void);

	// 選択された図形を最背面または最前面に移動する.
	// B	true の場合は最背面, false の場合は最前面に移動
	template<bool B>
	void MainPage::arrange_to(void)
	{
		using winrt::Windows::UI::Xaml::Controls::ItemCollection;

		// 選択された図形をリストに追加する.
		S_LIST_T sel_list;	// 選択された図形のリスト
		s_list_selected<Shape>(m_list_shapes, sel_list);
		if (sel_list.size() == 0) {
			return;
		}
		if constexpr (B) {
			uint32_t i = 0;	// 図形を挿入する位置
			//auto s_pos = S_LIST::front(m_list_shapes, i);
			auto s_pos = s_list_front(m_list_shapes);
			for (auto s : sel_list) {
				if (m_summary_visible) {
					summary_remove(s);
					summary_insert(s, i++);
				}
				// 図形を削除して, その操作をスタックに積む.
				undo_push_remove(s);
				undo_push_insert(s, s_pos);
			}
		}
		else {
			for (auto s : sel_list) {
				if (m_summary_visible) {
					summary_remove(s);
					summary_append(s);
				}
				// 図形を削除して, その操作をスタックに積む.
				undo_push_remove(s);
				undo_push_insert(s, nullptr);
			}
		}
		sel_list.clear();
		// 一連の操作の区切としてヌル操作をスタックに積む.
		undo_push_null();
		// 編集メニュー項目の使用の可否を設定する.
		enable_edit_menu();
		page_draw();
	}
	template void MainPage::arrange_to<true>(void);
	template void MainPage::arrange_to<false>(void);

	// 編集メニューの「前面に移動」が選択された.
	void MainPage::mfi_bring_forward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択された図形を次または前の図形と入れ替える.
		arrange_order<FORWARD>();
	}

	// 編集メニューの「最前面に移動」が選択された.
	void MainPage::mfi_bring_to_front_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択された図形を最背面または最前面に移動する.
		constexpr auto FRONT = false;
		arrange_to<FRONT>();
	}

	// 編集メニューの「ひとつ背面に移動」が選択された.
	void MainPage::mfi_send_backward_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択された図形を次または前の図形と入れ替える.
		arrange_order<BACKWARD>();
	}

	// 編集メニューの「最背面に移動」が選択された.
	void MainPage::mfi_send_to_back_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 選択された図形を最背面または最前面に移動する.
		constexpr auto BACK = true;
		arrange_to<BACK>();
	}

}