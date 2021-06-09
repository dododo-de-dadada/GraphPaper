//-------------------------------
// MainPage_smry.cpp
// 図形の一覧
//-------------------------------
#include "pch.h"
#include "Summary.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::ItemCollection;
	using winrt::Windows::UI::Xaml::Controls::ListView;

	// 図形リストをもとにリストビューを作成する.
	static void smry_add_items(ListView const& l_view, const SHAPE_LIST& slist, const ResourceDictionary& r_dict);
	// 図形の間隔を一覧から得る.
	static uint32_t smry_distance(ItemCollection const& items, const Shape* s);
	// 一覧の要素を選択する.
	static void smry_select_item(ListView const& view, const uint32_t i);
	// 図形を一覧の要素から得る.
	static Shape* const smry_shape(IInspectable const& item) noexcept;
	// 一覧に含まれる図形を入れ替える.
	static void smry_swap(ListView const& view, Shape* const s, Shape* const t, ResourceDictionary const& r);
	// 一覧の要素の選択を解除する.
	static void smry_unselect_item(ListView const& view, const uint32_t i);

	// 図形リストをもとにリストビューを作成する.
	static void smry_add_items(ListView const& l_view, const SHAPE_LIST& slist, const ResourceDictionary& r_dict)
	{
		uint32_t i = 0;
		for (auto s : slist) {
			if (s->is_deleted()) {
				continue;
			}
			l_view.Items().Append(winrt::make<Summary>(s, r_dict));
			if (s->is_selected()) {
				smry_select_item(l_view, i);
			}
			i++;
		}
	}

	// 図形の間隔を一覧から得る.
	static uint32_t smry_distance(ItemCollection const& items, const Shape* s)
	{
		// for (auto item : items) はエラーになるので使えない.
		// items.GetAt は空の要素を返すので使えない.
		// items.GetMany は期待した要素を返す.
		const auto k = items.Size();
		for (uint32_t i = 0; i < k; i++) {
			IInspectable item[1];
			items.GetMany(i, item);
			if (smry_shape(item[0]) == s) {
				return i;
			}
		}
		return k;
	}

	// 図形を一覧の要素から得る.
	static Shape* const smry_shape(IInspectable const& item) noexcept
	{
		// try_as は例外を投げない.
		const auto s = item.try_as<Summary>();
		if (s != nullptr) {
			return s->get_shape();
		}
		return static_cast<Shape*>(nullptr);
	}

	// 一覧の要素を選択する.
	// i	選択する要素の添え字
	static void smry_select_item(ListView const& view, const uint32_t i)
	{
		IInspectable item[1];
		view.Items().GetMany(i, item);
		view.SelectedItems().Append(item[0]);
		view.ScrollIntoView(item[0]);
	}

	// 一覧に含まれる図形を入れ替える.
	// l_view	リストビュー
	// s	入れ替える図形
	// t	もう一方の入れ替える図形
	// r_dict	リソースディレクトリ
	static void smry_swap(ListView const& l_view, Shape* const s, Shape* const t, ResourceDictionary const& r_dict)
	{
		auto const& items = l_view.Items();
		const auto i = smry_distance(items, s);
		const auto j = smry_distance(items, t);
		const auto t_item{ winrt::make<Summary>(t, r_dict) };
		const auto s_item{ winrt::make<Summary>(s, r_dict) };
		items.SetAt(i, t_item);
		items.SetAt(j, s_item);
		if (s->is_selected()) {
			smry_select_item(l_view, j);
		}
		if (t->is_selected()) {
			smry_select_item(l_view, i);
		}
	}

	// 一覧の要素の選択を解除する.
	static void smry_unselect_item(ListView const& l_view, const uint32_t i)
	{
		IInspectable s_item[1];
		l_view.Items().GetMany(i, s_item);
		const auto s = smry_shape(s_item[0]);
		const auto k = l_view.SelectedItems().Size();
		for (uint32_t j = 0; j < k; j++) {
			IInspectable t_item[1];
			l_view.SelectedItems().GetMany(j, t_item);
			if (smry_shape(t_item[0]) == s) {
				l_view.SelectedItems().RemoveAt(j);
				return;
			}
		}
	}

	// 図形一覧パネルの「閉じる」ボタンが押された.
	void MainPage::smry_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		smry_close();
	}

	// 図形一覧パネルがロードされた.
	void MainPage::smry_loaded(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const& /*e*/)
	{
		// 図形一覧の排他制御を false と入れ替える.
		bool smry_visible = m_smry_atomic.exchange(false, std::memory_order_acq_rel); // 入れ替え前の排他制御
		smry_add_items(lv_smry(), m_list_shapes, Resources());
		m_smry_atomic.store(smry_visible, std::memory_order_release);
	}

	// 図形一覧の項目が選択された.
	void MainPage::smry_selection_changed(IInspectable const&, SelectionChangedEventArgs const& e)
	{
		// 図形一覧の排他制御が false か判定する.
		if (!m_smry_atomic) {
			return;
		}
#if defined(_DEBUG)
		if (e.AddedItems().Size() + e.RemovedItems().Size() == 0) {
			// 選択または非選択された項目がなければ中断する.
			return;
		}
#endif	

		// 非選択された項目があれば, 選択をはずす。
		for (uint32_t i = 0; i < e.RemovedItems().Size(); i++) {
			IInspectable item[1];
			e.RemovedItems().GetMany(i, item);
			const auto s = smry_shape(item[0]);
			if (s != nullptr && s->is_selected()) {
				undo_push_select(s);
				//if (m_event_shape_smry == s) {
				//	m_event_shape_smry = nullptr;
				//}
			}
		}
		// 選択された項目があれば, 選択をつける.
		auto t = static_cast<Shape*>(nullptr);
		for (uint32_t i = 0; i < e.AddedItems().Size(); i++) {
			IInspectable item[1];
			e.AddedItems().GetMany(i, item);
			const auto s = smry_shape(item[0]);
			if (s != nullptr && !s->is_selected()) {
				undo_push_select(t = s);
				//m_event_shape_smry = s;
			}
		}
		if (t != static_cast<const Shape*>(nullptr)) {
			// 図形が表示されるよう用紙をスクロールする.
			scroll_to(t);
			m_sheet_main.set_attr_to(t);
			sheet_attr_is_checked();
		}
		// 編集メニュー項目の使用の可否を設定する.
		edit_menu_is_enabled();
		sheet_draw();
	}

	// 編集メニューの「リストを表示」が選択された.
	void MainPage::mfi_smry_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 図形一覧の排他制御が true か判定する.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			smry_close();
			return;
		}
		if (sp_find_text_panel().Visibility() == UI_VISIBLE) {
			find_text_click(nullptr, nullptr);
		}
		auto _{ FindName(L"gd_smry_panel") };
		gd_smry_panel().Visibility(UI_VISIBLE);
		m_smry_atomic.store(true, std::memory_order_release);
	}

	// 図形を一覧に追加する.
	void MainPage::smry_append(Shape* const s)
	{
		// 図形一覧の排他制御が true か判定する.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			lv_smry().Items().Append(winrt::make<Summary>(s, Resources()));
			if (s->is_selected()) {
				smry_select_item(lv_smry(), lv_smry().Items().Size() - 1);
			}
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// 一覧の中で図形を入れ替える.
	void MainPage::smry_arrng(Shape* const s, Shape* const t)
	{
		// 図形一覧の排他制御が true か判定する.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			smry_swap(lv_smry(), s, t, Resources());
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形一覧を消去する.
	void MainPage::smry_clear(void)
	{
		// 図形一覧の排他制御が true か判定する.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			lv_smry().Items().Clear();
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形一覧パネルを閉じて消去する.
	void MainPage::smry_close(void)
	{
		// 図形一覧の排他制御が true か判定する.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			gd_smry_panel().Visibility(UI_COLLAPSED);
			lv_smry().Items().Clear();
			UnloadObject(gd_smry_panel());
		}
	}

	// 一覧の添え字の位置に図形を挿入する.
	void MainPage::smry_insert(Shape* const s, const uint32_t i)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			lv_smry().Items().InsertAt(i, winrt::make<Summary>(s, Resources()));
			if (s->is_selected()) {
				smry_select_item(lv_smry(), i);
			}
			else {
				smry_unselect_item(lv_smry(), i);
			}
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// 操作を図形一覧に反映する.
	// この関数は, 操作を実行する前に呼び出す.
	void MainPage::smry_reflect(const Undo* u)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			const auto& u_type = typeid(*u);
			if (u_type == typeid(UndoList)) {
				auto v = static_cast<const UndoList*>(u);
				auto s = v->shape();
				if (v->is_insert()) {
					if (v->shape_at() != nullptr) {
						auto i = smry_distance(lv_smry().Items(), v->shape_at());
						lv_smry().Items().InsertAt(i, winrt::make<Summary>(s, Resources()));
						if (s->is_selected()) {
							smry_select_item(lv_smry(), i);
						}
					}
					else {
						lv_smry().Items().Append(winrt::make<Summary>(s, Resources()));
						const auto i = lv_smry().Items().Size() - 1;
						smry_select_item(lv_smry(), i);
					}
				}
				else {
					auto items = lv_smry().Items();
					const auto i = smry_distance(items, s);
					if (i < items.Size()) {
						//smry_unselect_item(lv_smry(), i);	// リムーブする前に選択を外さないとシステムが勝手に他の項目を選択する.
						items.RemoveAt(i);
					}
				}
				lv_smry().UpdateLayout();
			}
			else if (u_type == typeid(UndoArrange2)) {
				auto v = static_cast<const UndoArrange2*>(u);
				auto s = v->shape();
				auto t = v->dest();
				smry_swap(lv_smry(), s, t, Resources());
			}
			else if (u_type == typeid(UndoSelect)) {
				const auto s = u->shape();
				const auto i = smry_distance(lv_smry().Items(), s);
				if (s->is_selected() != true) {
					smry_select_item(lv_smry(), i);
				}
				else {
					smry_unselect_item(lv_smry(), i);
				}
			}
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形一覧を作成しなおす.
	void MainPage::smry_remake(void)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			lv_smry().Items().Clear();
			smry_add_items(lv_smry(), m_list_shapes, Resources());
			lv_smry().UpdateLayout();
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形を一覧から消去する.
	uint32_t MainPage::smry_remove(Shape* const s)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			auto items = lv_smry().Items();
			const uint32_t i = smry_distance(items, s);
			if (i < items.Size()) {
				smry_unselect_item(lv_smry(), i);	// リムーブする前に選択を外さないとシステムが勝手に他の項目を選択する.
				items.RemoveAt(i);
			}
			m_smry_atomic.store(true, std::memory_order_release);
			return i;
		}
		return 0;
	}

	// 一覧の図形を選択する.
	void MainPage::smry_select(Shape* const s)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			smry_select_item(lv_smry(), smry_distance(lv_smry().Items(), s));
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// 一覧の項目を全て選択する.
	void MainPage::smry_select_all(void)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			lv_smry().SelectAll();
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// 一覧の最初の項目を選択する.
	void MainPage::smry_select_head(void)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			smry_select_item(lv_smry(), 0);
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// 一覧の最後の項目を選択する.
	void MainPage::smry_select_tail(void)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			smry_select_item(lv_smry(), lv_smry().Items().Size() - 1);
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// 一覧の図形を選択解除する.
	void MainPage::smry_unselect(Shape* const s)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			smry_unselect_item(lv_smry(), smry_distance(lv_smry().Items(), s));
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// 一覧の項目を全て選択解除する.
	void MainPage::smry_unselect_all(void)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			lv_smry().SelectedIndex(static_cast<uint32_t>(-1));
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// 一覧の表示を更新する.
	void MainPage::smry_update(void)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			lv_smry().UpdateLayout();
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	void MainPage::smry_item_click(IInspectable const&, ItemClickEventArgs const& args)
	{
		//if (m_smry_atomic.load(std::memory_order_acquire)) {
			//m_smry_atomic.store(false, std::memory_order_release);
			const auto item = args.ClickedItem();
			const auto smry = item.try_as<Summary>();
			m_event_shape_prev =
				m_event_shape_pressed = smry->get_shape();
		//}
	}
}