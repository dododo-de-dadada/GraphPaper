//-------------------------------
// MainPage_summary.cpp
// �}�`�̈ꗗ
//-------------------------------
#include "pch.h"
#include "Summary.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::ItemCollection;
	using winrt::Windows::UI::Xaml::Controls::ListView;

	// �}�`���X�g�����Ƃɐ}�`�̈ꗗ���쐬����.
	static void summary_add_items(ListView const& l_view, const SHAPE_LIST& slist, const ResourceDictionary& r_dict);
	// �}�`�̈ꗗ����Ԋu�𓾂�.
	static uint32_t summary_distance(ItemCollection const& items, const Shape* s);
	// �}�`�̈ꗗ�̗v�f��I������.
	static void summary_select_item(ListView const& view, const uint32_t i);
	// �}�`�̈ꗗ�̗v�f����}�`�𓾂�.
	static Shape* const summary_shape(IInspectable const& item) noexcept;
	// �}�`�̈ꗗ�Ɋ܂܂��}�`�����ւ���.
	static void summary_swap(ListView const& view, Shape* const s, Shape* const t, ResourceDictionary const& r);
	// �}�`�̈ꗗ�̗v�f�̑I������������.
	static void summary_unselect_item(ListView const& view, const uint32_t i);

	// �}�`���X�g�����Ƃɐ}�`�̈ꗗ���쐬����.
	static void summary_add_items(ListView const& l_view, const SHAPE_LIST& slist, const ResourceDictionary& r_dict)
	{
		uint32_t i = 0;
		for (auto s : slist) {
			if (s->is_deleted()) {
				continue;
			}
			l_view.Items().Append(winrt::make<Summary>(s, r_dict));
			if (s->is_selected()) {
				summary_select_item(l_view, i);
			}
			i++;
		}
	}

	// �}�`�̈ꗗ����Ԋu�𓾂�.
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

	// �}�`�̈ꗗ�̗v�f��I������.
	// i	�I������v�f�̓Y����
	static void summary_select_item(ListView const& view, const uint32_t i)
	{
		IInspectable item[1];
		view.Items().GetMany(i, item);
		view.SelectedItems().Append(item[0]);
		view.ScrollIntoView(item[0]);
	}

	// �}�`�̈ꗗ�̗v�f����}�`�𓾂�.
	static Shape* const summary_shape(IInspectable const& item) noexcept
	{
		// try_as �͗�O�𓊂��Ȃ�.
		const auto& s = item.try_as<Summary>();
		if (s != nullptr) {
			return s->get_shape();
		}
		return static_cast<Shape*>(nullptr);
	}

	// �}�`�̈ꗗ�Ɋ܂܂��}�`�����ւ���.
	// l_view	���X�g�r���[
	// s	����ւ���}�`
	// t	��������̓���ւ���}�`
	// r_dict	���\�[�X�f�B���N�g��
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
			summary_select_item(l_view, j);
		}
		if (t->is_selected()) {
			summary_select_item(l_view, i);
		}
	}

	// �}�`�̈ꗗ�̗v�f�̑I������������.
	static void summary_unselect_item(ListView const& l_view, const uint32_t i)
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
				return;
			}
		}
	}

	// �}�`�ꗗ�p�l���́u����v�{�^���������ꂽ.
	void MainPage::summary_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		summary_close();
	}

	// �}�`�ꗗ�p�l�������[�h���ꂽ.
	void MainPage::summary_loaded(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const& /*e*/)
	{
		// �}�`�ꗗ�̔r������� false �Ɠ���ւ���.
		bool summary_visible = m_summary_atomic.exchange(false, std::memory_order_acq_rel); // ����ւ��O�̔r������
		summary_add_items(lv_summary(), m_list_shapes, Resources());
		m_summary_atomic.store(summary_visible, std::memory_order_release);
	}

	// �}�`�ꗗ�̍��ڂ��I�����ꂽ.
	void MainPage::summary_selection_changed(IInspectable const&, SelectionChangedEventArgs const& e)
	{
		// �}�`�ꗗ�̔r�����䂪 false �����肷��.
		if (!m_summary_atomic) {
			return;
		}
#if defined(_DEBUG)
		if (e.AddedItems().Size() + e.RemovedItems().Size() == 0) {
			// �I���܂��͔�I�����ꂽ���ڂ��Ȃ���Β��f����.
			return;
		}
#endif	

		// ��I�����ꂽ���ڂ������, �I�����͂����B
		for (uint32_t i = 0; i < e.RemovedItems().Size(); i++) {
			IInspectable item[1];
			e.RemovedItems().GetMany(i, item);
			const auto s = summary_shape(item[0]);
			if (s != nullptr && s->is_selected()) {
				undo_push_select(s);
				//if (m_event_shape_summary == s) {
				//	m_event_shape_summary = nullptr;
				//}
			}
		}
		// �I�����ꂽ���ڂ������, �I��������.
		auto t = static_cast<Shape*>(nullptr);
		for (uint32_t i = 0; i < e.AddedItems().Size(); i++) {
			IInspectable item[1];
			e.AddedItems().GetMany(i, item);
			const auto s = summary_shape(item[0]);
			if (s != nullptr && !s->is_selected()) {
				undo_push_select(t = s);
				//m_event_shape_summary = s;
			}
		}
		if (t != static_cast<const Shape*>(nullptr)) {
			// �}�`���\�������悤�p�����X�N���[������.
			scroll_to(t);
			m_sheet_main.set_attr_to(t);
			sheet_attr_is_checked();
		}
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		xcvd_is_enabled();
		sheet_draw();
	}

	// �ҏW���j���[�́u���X�g��\���v���I�����ꂽ.
	void MainPage::summary_list_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			summary_close();
			return;
		}
		if (sp_find_text_panel().Visibility() == UI_VISIBLE) {
			find_text_click(nullptr, nullptr);
		}
		auto _{ FindName(L"gd_summary_panel") };
		gd_summary_panel().Visibility(UI_VISIBLE);
		m_summary_atomic.store(true, std::memory_order_release);
	}

	// �}�`���ꗗ�ɒǉ�����.
	void MainPage::summary_append(Shape* const s)
	{
		// �}�`�̈ꗗ�̔r�����䂪 true �����肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			lv_summary().Items().Append(winrt::make<Summary>(s, Resources()));
			if (s->is_selected()) {
				summary_select_item(lv_summary(), lv_summary().Items().Size() - 1);
			}
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̒��Ő}�`�����ւ���.
	void MainPage::summary_arrange(Shape* const s, Shape* const t)
	{
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			summary_swap(lv_summary(), s, t, Resources());
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�ꗗ����������.
	void MainPage::summary_clear(void)
	{
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			lv_summary().Items().Clear();
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�ꗗ�p�l������ď�������.
	void MainPage::summary_close(void)
	{
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			gd_summary_panel().Visibility(UI_COLLAPSED);
			lv_summary().Items().Clear();
			UnloadObject(gd_summary_panel());
		}
	}

	// �ꗗ�̓Y�����̈ʒu�ɐ}�`��}������.
	void MainPage::summary_insert(Shape* const s, const uint32_t i)
	{
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			lv_summary().Items().InsertAt(i, winrt::make<Summary>(s, Resources()));
			if (s->is_selected()) {
				summary_select_item(lv_summary(), i);
			}
			else {
				summary_unselect_item(lv_summary(), i);
			}
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �����}�`�ꗗ�ɔ��f����.
	// ���̊֐���, ��������s����O�ɌĂяo��.
	void MainPage::summary_reflect(const Undo* u)
	{
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			const auto& u_type = typeid(*u);
			if (u_type == typeid(UndoList)) {
				auto v = static_cast<const UndoList*>(u);
				auto s = v->shape();
				if (v->is_insert()) {
					if (v->shape_at() != nullptr) {
						auto i = summary_distance(lv_summary().Items(), v->shape_at());
						lv_summary().Items().InsertAt(i, winrt::make<Summary>(s, Resources()));
						if (s->is_selected()) {
							summary_select_item(lv_summary(), i);
						}
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
						//summary_unselect_item(lv_summary(), i);	// �����[�u����O�ɑI�����O���Ȃ��ƃV�X�e��������ɑ��̍��ڂ�I������.
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
				if (s->is_selected() != true) {
					summary_select_item(lv_summary(), i);
				}
				else {
					summary_unselect_item(lv_summary(), i);
				}
			}
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�ꗗ���쐬���Ȃ���.
	void MainPage::summary_remake(void)
	{
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			lv_summary().Items().Clear();
			summary_add_items(lv_summary(), m_list_shapes, Resources());
			lv_summary().UpdateLayout();
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`���ꗗ�����������.
	uint32_t MainPage::summary_remove(Shape* const s)
	{
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			auto items = lv_summary().Items();
			const uint32_t i = summary_distance(items, s);
			if (i < items.Size()) {
				summary_unselect_item(lv_summary(), i);	// �����[�u����O�ɑI�����O���Ȃ��ƃV�X�e��������ɑ��̍��ڂ�I������.
				items.RemoveAt(i);
			}
			m_summary_atomic.store(true, std::memory_order_release);
			return i;
		}
		return 0;
	}

	// �ꗗ�̐}�`��I������.
	void MainPage::summary_select(Shape* const s)
	{
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			summary_select_item(lv_summary(), summary_distance(lv_summary().Items(), s));
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̍��ڂ�S�đI������.
	void MainPage::summary_select_all(void)
	{
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			lv_summary().SelectAll();
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̍ŏ��̍��ڂ�I������.
	void MainPage::summary_select_head(void)
	{
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			summary_select_item(lv_summary(), 0);
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̍Ō�̍��ڂ�I������.
	void MainPage::summary_select_tail(void)
	{
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			summary_select_item(lv_summary(), lv_summary().Items().Size() - 1);
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̐}�`��I����������.
	void MainPage::summary_unselect(Shape* const s)
	{
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			summary_unselect_item(lv_summary(), summary_distance(lv_summary().Items(), s));
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̍��ڂ�S�đI����������.
	void MainPage::summary_unselect_all(void)
	{
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			lv_summary().SelectedIndex(static_cast<uint32_t>(-1));
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̕\�����X�V����.
	void MainPage::summary_update(void)
	{
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			lv_summary().UpdateLayout();
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	void MainPage::summary_item_click(IInspectable const&, ItemClickEventArgs const& args)
	{
		//if (m_summary_atomic.load(std::memory_order_acquire)) {
			//m_summary_atomic.store(false, std::memory_order_release);
			const auto item = args.ClickedItem();
			const auto summary = item.try_as<Summary>();
			m_event_shape_prev =
				m_event_shape_pressed = summary->get_shape();
		//}
	}
}