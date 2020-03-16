//-------------------------------
// MainPage_summary.cpp
// �}�`�ꗗ�p�l���̕\��, �ݒ�
//-------------------------------
#include "pch.h"
#include "Summary.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::ItemCollection;
	using winrt::Windows::UI::Xaml::Controls::ListView;

	// �}�`�̒������ꗗ���瓾��.
	static uint32_t summary_distance(ItemCollection const& items, const Shape* s);
	// �ꗗ�̗v�f��I������.
	static void summary_select_item(ListView const& view, const uint32_t i);
	// �}�`���ꗗ�̗v�f���瓾��.
	static Shape* summary_shape(IInspectable const& item) noexcept;
	// �ꗗ�Ɋ܂܂��}�`�����ւ���.
	static void summary_swap(ListView const& view, Shape* s, Shape* t, ResourceDictionary const& r);
	// �ꗗ�̗v�f�̑I������������.
	static void summary_unselect_item(ListView const& view, const uint32_t i);

	// �}�`�̒������ꗗ���瓾��.
	static uint32_t summary_distance(ItemCollection const& items, const Shape* s)
	{
		// for (auto item : items) �̓G���[�ɂȂ�̂Ŏg���Ȃ�.
		// items.GetAt �͋�̗v�f��Ԃ��̂Ŏg���Ȃ�.
		// items.GetMany �͊��҂����v�f��Ԃ�.
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

	// �}�`���ꗗ�̗v�f���瓾��.
	static Shape* summary_shape(IInspectable const& item) noexcept
	{
		// try_as �͗�O�𓊂��Ȃ�.
		const auto s = item.try_as<Summary>();
		if (s != nullptr) {
			return reinterpret_cast<Shape*>(s->get_shape());
		}
		return nullptr;
	}

	// �ꗗ�̗v�f��I������.
	// i	�I������v�f�̓Y����
	static void summary_select_item(ListView const& view, const uint32_t i)
	{
		IInspectable item[1];
		view.Items().GetMany(i, item);
		view.SelectedItems().Append(item[0]);
		view.ScrollIntoView(item[0]);
	}

	// �ꗗ�Ɋ܂܂��}�`�����ւ���.
	// view	���X�g�r���[
	// s	����ւ���}�`
	// t	��������̓���ւ���}�`
	// r	���\�[�X�f�B���N�g��
	static void summary_swap(ListView const& view, Shape* s, Shape* t, ResourceDictionary const& r)
	{
		auto const& items = view.Items();
		const auto i = summary_distance(items, s);
		const auto j = summary_distance(items, t);
		const auto p = s->is_selected();
		const auto q = t->is_selected();
		auto t_item = winrt::make<Summary>(t, r);
		auto s_item = winrt::make<Summary>(s, r);
		items.SetAt(i, t_item);
		items.SetAt(j, s_item);
		if (p) {
			summary_select_item(view, j);
		}
		if (q) {
			summary_select_item(view, i);
		}
	}

	// �ꗗ�̗v�f�̑I������������.
	static void summary_unselect_item(ListView const& view, const uint32_t i)
	{
		IInspectable t_item[1];
		view.Items().GetMany(i, t_item);
		const auto t = summary_shape(t_item[0]);
		const auto k = view.SelectedItems().Size();
		for (uint32_t j = 0; j < k; j++) {
			IInspectable s_item[1];
			view.SelectedItems().GetMany(j, s_item);
			const auto s = summary_shape(s_item[0]);
			if (s == t) {
				view.SelectedItems().RemoveAt(j);
				break;
			}
		}
	}

	// �}�`�ꗗ�p�l���́u����v�{�^���������ꂽ.
	void MainPage::btn_summary_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		summary_close();
		//mfi_summary_click(nullptr, nullptr);
	}

	// �}�`�ꗗ�p�l�������[�h���ꂽ.
	void MainPage::lv_summary_loaded(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const& /*e*/)
	{
		bool s_visible = m_summary_visible;
		if (m_summary_visible) {
			m_summary_visible = false;
		}
		uint32_t i = 0;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			lv_summary().Items().Append(winrt::make<Summary>(s, Resources()));
			if (s->is_selected()) {
				summary_select_item(lv_summary(), i);
			}
			i++;
		}
		if (m_summary_visible != s_visible) {
			m_summary_visible = s_visible;
		}
	}

	// �}�`�ꗗ�̍��ڂ��I�����ꂽ.
	void MainPage::lv_summary_selection_changed(IInspectable const&, SelectionChangedEventArgs const& e)
	{
		if (m_summary_visible == false) {
			// �ꗗ�̕\���t���O���Ȃ���Β��f����.
			return;
		}
#if defined(_DEBUG)
		if (e.AddedItems().Size() + e.RemovedItems().Size() == 0) {
			// �I���܂��͔�I�����ꂽ���ڂ��Ȃ���Β��f����.
			return;
		}
#endif	
		for (uint32_t i = 0; i < e.RemovedItems().Size(); i++) {
			IInspectable item[1];
			e.RemovedItems().GetMany(i, item);
			auto s = summary_shape(item[0]);
			if (s != nullptr && s->is_selected()) {
				undo_push_select(s);
			}
		}
		auto t = static_cast<Shape*>(nullptr);
		for (uint32_t i = 0; i < e.AddedItems().Size(); i++) {
			IInspectable item[1];
			e.AddedItems().GetMany(i, item);
			auto s = summary_shape(item[0]);
			if (s != nullptr && s->is_selected() == false) {
				undo_push_select(t = s);
			}
		}
		if (t != nullptr) {
			// �}�`���\�������悤�y�[�W���X�N���[������.
			scroll_to_shape(t);
			m_page_layout.set_to_shape(t);
		}
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		enable_edit_menu();
		page_draw();
	}

	// �ҏW���j���[�́u���X�g��\���v���I�����ꂽ.
	void MainPage::mfi_summary_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_summary_visible) {
			summary_close();
			return;
		}
		if (sp_text_find().Visibility() == VISIBLE) {
			mfi_text_find_click(nullptr, nullptr);
		}
		auto _{ FindName(L"rp_summary") };
		rp_summary().Visibility(VISIBLE);
		m_summary_visible = true;
	}

	// �}�`���ꗗ�ɒǉ�����.
	void MainPage::summary_append(Shape* s)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		lv_summary().Items().Append(winrt::make<Summary>(s, Resources()));
		summary_select_item(lv_summary(), lv_summary().Items().Size() - 1);
		m_summary_visible = true;
	}

	// �ꗗ�̒��Ő}�`�����ւ���.
	void MainPage::summary_arrange(Shape* s, Shape* t)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		summary_swap(lv_summary(), s, t, Resources());
		m_summary_visible = true;
	}

	// �}�`�ꗗ����������.
	void MainPage::summary_clear(void)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		lv_summary().Items().Clear();
		m_summary_visible = true;
	}

	// �}�`�ꗗ�p�l������ď�������.
	void MainPage::summary_close(void)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		rp_summary().Visibility(COLLAPSED);
		lv_summary().Items().Clear();
		UnloadObject(rp_summary());
	}

	// �ꗗ�̓Y�����̈ʒu�ɐ}�`��}������.
	void MainPage::summary_insert(Shape* s, const uint32_t i)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		lv_summary().Items().InsertAt(i, winrt::make<Summary>(s, Resources()));
		summary_select_item(lv_summary(), i);
		m_summary_visible = true;
	}

	// �����}�`�ꗗ�ɔ��f����.
	// ���̊֐���, ��������s����O�ɌĂяo��.
	void MainPage::summary_reflect(const Undo* u)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		auto const& u_type = typeid(*u);
		if (u_type == typeid(UndoList)) {
			auto v = static_cast<const UndoList*>(u);
			auto s = v->shape();
			if (v->is_insert()) {
				if (v->item_pos() != nullptr) {
					const auto i = summary_distance(lv_summary().Items(), v->item_pos());
					lv_summary().Items().InsertAt(i, winrt::make<Summary>(s, Resources()));
					summary_select_item(lv_summary(), i);
				}
				else {
					lv_summary().Items().Append(winrt::make<Summary>(s, Resources()));
					const auto i = lv_summary().Items().Size() - 1;
					summary_select_item(lv_summary(), i);
				}
			}
			else {
				auto items = lv_summary().Items();
				const auto i = summary_distance(items, s);
				if (i < items.Size()) {
					items.RemoveAt(i);
				}
			}
			lv_summary().UpdateLayout();
		}
		else if (u_type == typeid(UndoArrange2)) {
			auto v = static_cast<const UndoArrange2*>(u);
			auto s = v->shape();
			auto t = v->dest();
			summary_swap(lv_summary(), s, t, Resources());
		}
		else if (u_type == typeid(UndoSelect)) {
			const auto s = u->shape();
			const auto i = summary_distance(lv_summary().Items(), s);
			if (s->is_selected() == false) {
				summary_select_item(lv_summary(), i);
			}
			else {
				summary_unselect_item(lv_summary(), i);
			}
		}
		m_summary_visible = true;
	}

	// �}�`�ꗗ���쐬���Ȃ���.
	void MainPage::summary_remake(void)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		lv_summary().Items().Clear();
		uint32_t i = 0;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			lv_summary().Items().Append(winrt::make<Summary>(s, Resources()));
			if (s->is_selected()) {
				summary_select_item(lv_summary(), i);
			}
			i++;
		}
		lv_summary().UpdateLayout();
		m_summary_visible = true;
	}

	// �}�`���ꗗ�����������.
	uint32_t MainPage::summary_remove(Shape* s)
	{
		if (m_summary_visible == false) {
			return 0;
		}
		m_summary_visible = false;
		auto items = lv_summary().Items();
		const auto i = summary_distance(items, s);
		if (i < items.Size()) {
			items.RemoveAt(i);
		}
		m_summary_visible = true;
		return i;
	}

	// �ꗗ�̐}�`��I������.
	void MainPage::summary_select(Shape* s)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		const auto i = summary_distance(lv_summary().Items(), s);
		summary_select_item(lv_summary(), i);
		m_summary_visible = true;
	}

	// �ꗗ�̍��ڂ�I������.
	void MainPage::summary_select(uint32_t i)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		summary_select_item(lv_summary(), i);
		m_summary_visible = true;
	}

	// �ꗗ�̍��ڂ�S�đI������.
	void MainPage::summary_select_all(void)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		lv_summary().SelectAll();
		m_summary_visible = true;
	}

	// �ꗗ�̍ŏ��̍��ڂ�I������.
	void MainPage::summary_select_head(void)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		summary_select_item(lv_summary(), 0);
		m_summary_visible = true;
	}

	// �ꗗ�̍Ō�̍��ڂ�I������.
	void MainPage::summary_select_tail(void)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		summary_select_item(lv_summary(), lv_summary().Items().Size() - 1);
		m_summary_visible = true;
	}

	// �ꗗ�̐}�`��I����������.
	void MainPage::summary_unselect(Shape* s)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		summary_unselect_item(lv_summary(), summary_distance(lv_summary().Items(), s));
		m_summary_visible = true;
	}

	// �ꗗ�̍��ڂ�I����������.
	void MainPage::summary_unselect(uint32_t i)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		summary_unselect_item(lv_summary(), i);
		m_summary_visible = true;
	}

	// �ꗗ�̍��ڂ�S�đI����������.
	void MainPage::summary_unselect_all(void)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		lv_summary().SelectedIndex(static_cast<uint32_t>(-1));
		m_summary_visible = true;
	}

	// �ꗗ�̕\�����X�V����.
	void MainPage::summary_update(void)
	{
		if (m_summary_visible == false) {
			return;
		}
		m_summary_visible = false;
		lv_summary().UpdateLayout();
		m_summary_visible = true;
	}

}