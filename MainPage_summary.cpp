//-------------------------------
// MainPage_summary.cpp
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

	// 図形リストをもとに図形の一覧を作成する.
	static void summary_create(ListView const& l_view, const SHAPE_LIST& slist, const ResourceDictionary& r_dict);
	// 図形の一覧から間隔を得る.
	static uint32_t summary_distance(ItemCollection const& items, const Shape* s);
	// 図形の一覧の要素を選択する.
	static void summary_select_at(ListView const& view, const uint32_t i);
	// 図形の一覧の要素から図形を得る.
	static Shape* const summary_shape(IInspectable const& item) noexcept;
	// 図形の一覧に含まれる図形を入れ替える.
	static void summary_swap(ListView const& view, Shape* const s, Shape* const t, ResourceDictionary const& r);
	// 図形の一覧の要素の選択を解除する.
	static void summary_unselect_at(ListView const& view, const uint32_t i);

	// 図形リストをもとに一覧を作成する.
	// l_view	リストビュー
	// slist	図形リスト
	// r_dict	リソースディレクトリ
	static void summary_create(ListView const& l_view, const SHAPE_LIST& slist, const ResourceDictionary& r_dict)
	{
		uint32_t i = 0;	// 要素の添え字
		for (const auto s : slist) {
			if (s->is_deleted()) {
				continue;
			}
			// リストビューの末尾に要素を追加する.
			l_view.Items().Append(winrt::make<Summary>(s, r_dict));
			// 図形が選択されているか判定する.
			if (s->is_selected()) {
				summary_select_at(l_view, i);
			}
			i++;
		}
	}

	// 図形の一覧から間隔を得る.
	static uint32_t summary_distance(ItemCollection const& items, const Shape* s)
	{
		// for (auto item : items) はエラーになるので使えない.
		// items.GetAt は空の要素を返すので使えない.
		// items.GetMany は期待した要素を返す.
		const auto k = items.Size();
		for (uint32_t i = 0; i < k; i++) {
			IInspectable item[1];
			items.GetMany(i, item);
			if (summary_shape(item[0]) == s) {
				return i;
			}
		}
		return k;
	}

	// 図形の一覧の要素を選択する.
	// i	選択する要素の添え字
	static void summary_select_at(ListView const& view, const uint32_t i)
	{
		IInspectable item[1];
		view.Items().GetMany(i, item);
		view.SelectedItems().Append(item[0]);
		view.ScrollIntoView(item[0]);
	}

	// 一覧の要素から図形を得る.
	// item	要素
	static Shape* const summary_shape(IInspectable const& item) noexcept
	{
		// try_as は例外を投げない.
		const auto& s = item.try_as<Summary>();
		if (s != nullptr) {
			return s->get_shape();
		}
		return static_cast<Shape*>(nullptr);
	}

	// 図形の一覧に含まれる図形を入れ替える.
	// l_view	リストビュー
	// s	入れ替える図形
	// t	もう一方の入れ替える図形
	// r_dict	リソースディレクトリ
	static void summary_swap(ListView const& l_view, Shape* const s, Shape* const t, ResourceDictionary const& r_dict)
	{
		auto const& items = l_view.Items();
		const auto i = summary_distance(items, s);
		const auto j = summary_distance(items, t);
		const auto t_item{ winrt::make<Summary>(t, r_dict) };
		const auto s_item{ winrt::make<Summary>(s, r_dict) };
		items.SetAt(i, t_item);
		items.SetAt(j, s_item);
		if (s->is_selected()) {
			summary_select_at(l_view, j);
		}
		if (t->is_selected()) {
			summary_select_at(l_view, i);
		}
	}

	// 図形の一覧の要素の選択を解除する.
	// l_view	リストビュー
	// i	図形の添え字
	static void summary_unselect_at(ListView const& l_view, const uint32_t i)
	{
		IInspectable s_item[1];
		l_view.Items().GetMany(i, s_item);
		const auto s = summary_shape(s_item[0]);
		const auto k = l_view.SelectedItems().Size();
		for (uint32_t j = 0; j < k; j++) {
			IInspectable t_item[1];
			l_view.SelectedItems().GetMany(j, t_item);
			if (summary_shape(t_item[0]) == s) {
				l_view.SelectedItems().RemoveAt(j);
				break;
			}
		}
	}

	// 一覧の末尾に図形を追加する.
	// s	図形
	void MainPage::summary_append(Shape* const s)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			// 一覧の末尾に要素を追加する.
			lv_summary_list().Items().Append(winrt::make<Summary>(s, Resources()));
			if (s->is_selected()) {
				summary_select_at(lv_summary_list(), lv_summary_list().Items().Size() - 1);
			}
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形の一覧の中で図形を入れ替える.
	// s	入れ替える図形
	// t	もう一方の図形
	void MainPage::summary_order(Shape* const s, Shape* const t)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);

			summary_swap(lv_summary_list(), s, t, Resources());
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形の一覧を消去する.
	void MainPage::summary_clear(void)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);

			lv_summary_list().Items().Clear();
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形の一覧パネルの「閉じる」ボタンが押された.
	void MainPage::summary_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);

			if (gd_summary_panel() != nullptr) {
				gd_summary_panel().Visibility(Visibility::Collapsed);
				lv_summary_list().Items().Clear();
				UnloadObject(gd_summary_panel());
			}
		}
		status_bar_set_pos();
	}

	// 図形の一覧の添え字の位置に図形を挿入する.
	// s	挿入する図形
	// i	挿入される位置 (添え字)
	void MainPage::summary_insert_at(Shape* const s, const uint32_t i)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			const auto& list = lv_summary_list();
			list.Items().InsertAt(i, winrt::make<Summary>(s, Resources()));
			if (s->is_selected()) {
				summary_select_at(list, i);
			}
			else {
				summary_unselect_at(list, i);
			}
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形の一覧の項目が選択された.
	void MainPage::summary_item_click(IInspectable const&, ItemClickEventArgs const& args)
	{
		const auto item = args.ClickedItem();
		const winrt::impl::com_ref<Summary> summary = item.try_as<Summary>();
		m_event_shape_pressed = summary->get_shape();
		m_event_shape_last = m_event_shape_pressed;
		if (typeid(*m_event_shape_pressed) == typeid(ShapeText) &&
			m_edit_text_shape != static_cast<ShapeText*>(m_event_shape_pressed)) {
			// 編集対象の図形があるならフォーカスをはずす.
			if (m_edit_text_shape != nullptr) {
				m_edit_context.NotifyFocusLeave();
				undo_push_text_unselect(m_edit_text_shape);
			}
			// 押された図形をあらたな編集対象の図形とする.
			m_edit_text_shape = static_cast<ShapeText*>(m_event_shape_pressed);
			m_edit_context.NotifyFocusEnter();
		}
		m_main_page.set_attr_to(m_event_shape_pressed);
		// メニューバーを更新する
		stroke_dash_is_checked(m_main_page.m_stroke_dash);
		stroke_width_is_checked(m_main_page.m_stroke_width);
		stroke_cap_is_checked(m_main_page.m_stroke_cap);
		stroke_join_is_checked(m_main_page.m_stroke_join);
		stroke_arrow_is_checked(m_main_page.m_arrow_style);
		font_weight_is_checked(m_main_page.m_font_weight);
		font_stretch_is_checked(m_main_page.m_font_stretch);
		font_style_is_checked(m_main_page.m_font_style);
		text_align_horz_is_checked(m_main_page.m_text_align_horz);
		text_align_vert_is_checked(m_main_page.m_text_align_vert);
		grid_emph_is_checked(m_main_page.m_grid_emph);
		grid_show_is_checked(m_main_page.m_grid_show);
	}

	// 編集メニューの「一覧を表示」が選択された.
	void MainPage::summary_list_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			// 図形の一覧を閉じる.
			summary_close_click(nullptr, nullptr);
		}
		else {
			// 検索パネルが表示されてるか判定する.
			if (sp_find_text_panel().Visibility() == Visibility::Visible) {
				// 検索パネルを非表示にする.
				find_text_click(nullptr, nullptr);
			}
			// リソースから図形の一覧パネルを見つける.
			auto _{ FindName(L"gd_summary_panel") };
			gd_summary_panel().Visibility(Visibility::Visible);
			m_summary_atomic.store(true, std::memory_order_release);
		}
		status_bar_set_pos();
	}

	// 図形の一覧がロードされた.
	void MainPage::summary_loaded(IInspectable const&, RoutedEventArgs const& /*e*/)
	{
		// 図形の一覧の排他制御を false と入れ替える.
		bool summary_visible = m_summary_atomic.exchange(false, std::memory_order_acq_rel); // 入れ替え前の排他制御
		summary_create(lv_summary_list(), m_main_page.m_shape_list, Resources());
		m_summary_atomic.store(summary_visible, std::memory_order_release);
	}

	// 操作を図形の一覧に反映する.
	// この関数は, 操作を実行する前に呼び出す.
	void MainPage::summary_reflect(const Undo* u)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			const auto& u_type = typeid(*u);
			if (u_type == typeid(UndoList)) {
				const auto v = static_cast<const UndoList*>(u);
				const auto& list = lv_summary_list();
				if (v->is_insert()) {
					if (v->shape_at() != nullptr) {
						const auto s = v->shape();
						auto i = summary_distance(list.Items(), v->shape_at());
						list.Items().InsertAt(i, winrt::make<Summary>(s, Resources()));
						if (s->is_selected()) {
							summary_select_at(list, i);
						}
					}
					else {
						const auto s = v->shape();
						list.Items().Append(winrt::make<Summary>(s, Resources()));
						const auto i = list.Items().Size() - 1;
						summary_select_at(list, i);
					}
				}
				else {
					const auto& items = list.Items();
					const auto s = v->shape();
					const auto i = summary_distance(items, s);
					if (i < items.Size()) {
						items.RemoveAt(i);
					}
				}
				list.UpdateLayout();
			}
			else if (u_type == typeid(UndoOrder)) {
				const auto v = static_cast<const UndoOrder*>(u);
				summary_swap(lv_summary_list(), v->shape(), v->dest(), Resources());
			}
			else if (u_type == typeid(UndoSelect)) {
				const auto s = u->shape();
				const auto i = summary_distance(lv_summary_list().Items(), s);
				if (!s->is_selected()) {
					summary_select_at(lv_summary_list(), i);
				}
				else {
					summary_unselect_at(lv_summary_list(), i);
				}
			}
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形の一覧を作成しなおす.
	void MainPage::summary_remake(void)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			const auto& list = lv_summary_list();
			list.Items().Clear();
			summary_create(list, m_main_page.m_shape_list, Resources());
			list.UpdateLayout();
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形を一覧から消去する.
	uint32_t MainPage::summary_remove(Shape* const s)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			const auto& list = lv_summary_list();
			const auto& items = list.Items();
			const uint32_t i = summary_distance(items, s);
			if (i < items.Size()) {
				summary_unselect_at(list, i);	// リムーブする前に選択を外さないとシステムが勝手に他の項目を選択する.
				items.RemoveAt(i);
			}
			m_summary_atomic.store(true, std::memory_order_release);
			return i;
		}
		return 0;
	}

	// 一覧の図形を選択する.
	// s	選択する図形
	void MainPage::summary_select(Shape* const s)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			summary_select_at(lv_summary_list(), summary_distance(lv_summary_list().Items(), s));
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形の一覧の項目を全て選択する.
	void MainPage::summary_select_all(void)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			lv_summary_list().SelectAll();
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形の一覧の最初の項目を選択する.
	void MainPage::summary_select_head(void)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			summary_select_at(lv_summary_list(), 0);
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形の一覧の最後の項目を選択する.
	void MainPage::summary_select_tail(void)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			summary_select_at(lv_summary_list(), lv_summary_list().Items().Size() - 1);
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形の一覧の項目が選択された.
	void MainPage::summary_selection_changed(IInspectable const&, SelectionChangedEventArgs const& e)
	{
		// 図形の一覧の排他制御が false か判定する.
		if (!m_summary_atomic) {
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
			const auto s = summary_shape(item[0]);
			if (s != nullptr && s->is_selected()) {
				undo_push_select(s);
			}
		}

		// 選択された項目があれば, 選択をつける.
		auto t = static_cast<Shape*>(nullptr);
		for (uint32_t i = 0; i < e.AddedItems().Size(); i++) {
			IInspectable item[1];
			e.AddedItems().GetMany(i, item);
			const auto s = summary_shape(item[0]);
			if (s != nullptr && !s->is_selected()) {
				undo_push_select(t = s);
			}
		}

		// 選択された図形があるか判定する.
		if (t != static_cast<const Shape*>(nullptr)) {
			// 図形が表示されるようスクロールする.
			scroll_to(t);
		}

		// 編集メニュー項目の使用の可否を設定する.
		xcvd_menu_is_enabled();
		main_draw();
	}

	// 一覧の図形を選択解除する.
	// s	選択解除する図形
	void MainPage::summary_unselect(Shape* const s)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			summary_unselect_at(lv_summary_list(), summary_distance(lv_summary_list().Items(), s));
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形の一覧の項目を全て選択解除する.
	void MainPage::summary_unselect_shape_all(void)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			lv_summary_list().SelectedIndex(static_cast<uint32_t>(-1));
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// 図形の一覧の表示を更新する.
	void MainPage::summary_update(void)
	{
		// 一覧が表示されてるか判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			lv_summary_list().UpdateLayout();
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

}