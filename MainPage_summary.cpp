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
	static void summary_create(ListView const& l_view, const SHAPE_LIST& slist, const ResourceDictionary& r_dict);
	// �}�`�̈ꗗ����Ԋu�𓾂�.
	static uint32_t summary_distance(ItemCollection const& items, const Shape* s);
	// �}�`�̈ꗗ�̗v�f��I������.
	static void summary_select_at(ListView const& view, const uint32_t i);
	// �}�`�̈ꗗ�̗v�f����}�`�𓾂�.
	static Shape* const summary_shape(IInspectable const& item) noexcept;
	// �}�`�̈ꗗ�Ɋ܂܂��}�`�����ւ���.
	static void summary_swap(ListView const& view, Shape* const s, Shape* const t, ResourceDictionary const& r);
	// �}�`�̈ꗗ�̗v�f�̑I������������.
	static void summary_unselect_at(ListView const& view, const uint32_t i);

	// �}�`���X�g�����ƂɈꗗ���쐬����.
	// l_view	���X�g�r���[
	// slist	�}�`���X�g
	// r_dict	���\�[�X�f�B���N�g��
	static void summary_create(ListView const& l_view, const SHAPE_LIST& slist, const ResourceDictionary& r_dict)
	{
		uint32_t i = 0;	// �v�f�̓Y����
		for (const auto s : slist) {
			if (s->is_deleted()) {
				continue;
			}
			// ���X�g�r���[�̖����ɗv�f��ǉ�����.
			l_view.Items().Append(winrt::make<Summary>(s, r_dict));
			// �}�`���I������Ă��邩���肷��.
			if (s->is_selected()) {
				summary_select_at(l_view, i);
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
	static void summary_select_at(ListView const& view, const uint32_t i)
	{
		IInspectable item[1];
		view.Items().GetMany(i, item);
		view.SelectedItems().Append(item[0]);
		view.ScrollIntoView(item[0]);
	}

	// �ꗗ�̗v�f����}�`�𓾂�.
	// item	�v�f
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
			summary_select_at(l_view, j);
		}
		if (t->is_selected()) {
			summary_select_at(l_view, i);
		}
	}

	// �}�`�̈ꗗ�̗v�f�̑I������������.
	// l_view	���X�g�r���[
	// i	�}�`�̓Y����
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

	// �ꗗ�̖����ɐ}�`��ǉ�����.
	// s	�}�`
	void MainPage::summary_append(Shape* const s)
	{
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			// �ꗗ�̖����ɗv�f��ǉ�����.
			lv_summary_list().Items().Append(winrt::make<Summary>(s, Resources()));
			if (s->is_selected()) {
				summary_select_at(lv_summary_list(), lv_summary_list().Items().Size() - 1);
			}
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�̈ꗗ�̒��Ő}�`�����ւ���.
	// s	����ւ���}�`
	// t	��������̐}�`
	void MainPage::summary_arrange(Shape* const s, Shape* const t)
	{
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);

			summary_swap(lv_summary_list(), s, t, Resources());
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�̈ꗗ����������.
	void MainPage::summary_clear(void)
	{
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);

			lv_summary_list().Items().Clear();
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�̈ꗗ�p�l���́u����v�{�^���������ꂽ.
	void MainPage::summary_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);

			if (gd_summary_panel() != nullptr) {
				gd_summary_panel().Visibility(UI_COLLAPSED);
				lv_summary_list().Items().Clear();
				UnloadObject(gd_summary_panel());
			}
		}
	}

	// �}�`�̈ꗗ�̓Y�����̈ʒu�ɐ}�`��}������.
	// s	�}�`
	// i	�Y����
	void MainPage::summary_insert_at(Shape* const s, const uint32_t i)
	{
		// �ꗗ���\������Ă邩���肷��.
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

	// �}�`�̈ꗗ�̍��ڂ��I�����ꂽ.
	void MainPage::summary_item_click(IInspectable const&, ItemClickEventArgs const& args)
	{
		const auto item = args.ClickedItem();
		const auto summary = item.try_as<Summary>();
		m_event_shape_prev =
			m_event_shape_pressed = summary->get_shape();
	}

	// �ҏW���j���[�́u�ꗗ��\���v���I�����ꂽ.
	void MainPage::summary_list_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			// �}�`�̈ꗗ�����.
			summary_close_click(nullptr, nullptr);
			return;
		}
		// �����p�l�����\������Ă邩���肷��.
		if (sp_find_text_panel().Visibility() == UI_VISIBLE) {
			// �����p�l�����\���ɂ���.
			find_text_click(nullptr, nullptr);
		}
		// ���\�[�X����}�`�̈ꗗ�p�l����������.
		auto _{ FindName(L"gd_summary_panel") };
		gd_summary_panel().Visibility(UI_VISIBLE);
		m_summary_atomic.store(true, std::memory_order_release);
	}

	// �}�`�̈ꗗ�����[�h���ꂽ.
	void MainPage::summary_loaded(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const& /*e*/)
	{
		// �}�`�̈ꗗ�̔r������� false �Ɠ���ւ���.
		bool summary_visible = m_summary_atomic.exchange(false, std::memory_order_acq_rel); // ����ւ��O�̔r������
		summary_create(lv_summary_list(), m_main_sheet.m_shape_list, Resources());
		m_summary_atomic.store(summary_visible, std::memory_order_release);
	}

	// �����}�`�̈ꗗ�ɔ��f����.
	// ���̊֐���, ��������s����O�ɌĂяo��.
	void MainPage::summary_reflect(const Undo* u)
	{
		// �ꗗ���\������Ă邩���肷��.
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
			else if (u_type == typeid(UndoArrange)) {
				const auto v = static_cast<const UndoArrange*>(u);
				summary_swap(lv_summary_list(), v->shape(), v->dest(), Resources());
			}
			else if (u_type == typeid(UndoSelect)) {
				const auto s = u->shape();
				const auto i = summary_distance(lv_summary_list().Items(), s);
				if (s->is_selected() != true) {
					summary_select_at(lv_summary_list(), i);
				}
				else {
					summary_unselect_at(lv_summary_list(), i);
				}
			}
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�̈ꗗ���쐬���Ȃ���.
	void MainPage::summary_remake(void)
	{
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			const auto& list = lv_summary_list();
			list.Items().Clear();
			summary_create(list, m_main_sheet.m_shape_list, Resources());
			list.UpdateLayout();
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`���ꗗ�����������.
	uint32_t MainPage::summary_remove(Shape* const s)
	{
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			const auto& list = lv_summary_list();
			const auto& items = list.Items();
			const uint32_t i = summary_distance(items, s);
			if (i < items.Size()) {
				summary_unselect_at(list, i);	// �����[�u����O�ɑI�����O���Ȃ��ƃV�X�e��������ɑ��̍��ڂ�I������.
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
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			summary_select_at(lv_summary_list(), summary_distance(lv_summary_list().Items(), s));
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�̈ꗗ�̍��ڂ�S�đI������.
	void MainPage::summary_select_all(void)
	{
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			lv_summary_list().SelectAll();
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�̈ꗗ�̍ŏ��̍��ڂ�I������.
	void MainPage::summary_select_head(void)
	{
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			summary_select_at(lv_summary_list(), 0);
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�̈ꗗ�̍Ō�̍��ڂ�I������.
	void MainPage::summary_select_tail(void)
	{
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			summary_select_at(lv_summary_list(), lv_summary_list().Items().Size() - 1);
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�̈ꗗ�̍��ڂ��I�����ꂽ.
	void MainPage::summary_selection_changed(IInspectable const&, SelectionChangedEventArgs const& e)
	{
		// �}�`�̈ꗗ�̔r�����䂪 false �����肷��.
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
				ustack_push_select(s);
			}
		}

		// �I�����ꂽ���ڂ������, �I��������.
		auto t = static_cast<Shape*>(nullptr);
		for (uint32_t i = 0; i < e.AddedItems().Size(); i++) {
			IInspectable item[1];
			e.AddedItems().GetMany(i, item);
			const auto s = summary_shape(item[0]);
			if (s != nullptr && !s->is_selected()) {
				ustack_push_select(t = s);
			}
		}

		// �I�����ꂽ�}�`�����邩���肷��.
		if (t != static_cast<const Shape*>(nullptr)) {
			// �}�`���\�������悤�p�����X�N���[������.
			scroll_to(t);
			m_main_sheet.set_attr_to(t);
			sheet_attr_is_checked();
		}

		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		xcvd_is_enabled();
		sheet_draw();
	}

	// �ꗗ�̐}�`��I����������.
	void MainPage::summary_unselect(Shape* const s)
	{
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			summary_unselect_at(lv_summary_list(), summary_distance(lv_summary_list().Items(), s));
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�̈ꗗ�̍��ڂ�S�đI����������.
	void MainPage::summary_unselect_all(void)
	{
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			lv_summary_list().SelectedIndex(static_cast<uint32_t>(-1));
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�̈ꗗ�̕\�����X�V����.
	void MainPage::summary_update(void)
	{
		// �ꗗ���\������Ă邩���肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			m_summary_atomic.store(false, std::memory_order_release);
			lv_summary_list().UpdateLayout();
			m_summary_atomic.store(true, std::memory_order_release);
		}
	}

}