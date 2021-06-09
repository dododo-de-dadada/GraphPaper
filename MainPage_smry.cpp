//-------------------------------
// MainPage_smry.cpp
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

	// �}�`���X�g�����ƂɃ��X�g�r���[���쐬����.
	static void smry_add_items(ListView const& l_view, const SHAPE_LIST& slist, const ResourceDictionary& r_dict);
	// �}�`�̊Ԋu���ꗗ���瓾��.
	static uint32_t smry_distance(ItemCollection const& items, const Shape* s);
	// �ꗗ�̗v�f��I������.
	static void smry_select_item(ListView const& view, const uint32_t i);
	// �}�`���ꗗ�̗v�f���瓾��.
	static Shape* const smry_shape(IInspectable const& item) noexcept;
	// �ꗗ�Ɋ܂܂��}�`�����ւ���.
	static void smry_swap(ListView const& view, Shape* const s, Shape* const t, ResourceDictionary const& r);
	// �ꗗ�̗v�f�̑I������������.
	static void smry_unselect_item(ListView const& view, const uint32_t i);

	// �}�`���X�g�����ƂɃ��X�g�r���[���쐬����.
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

	// �}�`�̊Ԋu���ꗗ���瓾��.
	static uint32_t smry_distance(ItemCollection const& items, const Shape* s)
	{
		// for (auto item : items) �̓G���[�ɂȂ�̂Ŏg���Ȃ�.
		// items.GetAt �͋�̗v�f��Ԃ��̂Ŏg���Ȃ�.
		// items.GetMany �͊��҂����v�f��Ԃ�.
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

	// �}�`���ꗗ�̗v�f���瓾��.
	static Shape* const smry_shape(IInspectable const& item) noexcept
	{
		// try_as �͗�O�𓊂��Ȃ�.
		const auto s = item.try_as<Summary>();
		if (s != nullptr) {
			return s->get_shape();
		}
		return static_cast<Shape*>(nullptr);
	}

	// �ꗗ�̗v�f��I������.
	// i	�I������v�f�̓Y����
	static void smry_select_item(ListView const& view, const uint32_t i)
	{
		IInspectable item[1];
		view.Items().GetMany(i, item);
		view.SelectedItems().Append(item[0]);
		view.ScrollIntoView(item[0]);
	}

	// �ꗗ�Ɋ܂܂��}�`�����ւ���.
	// l_view	���X�g�r���[
	// s	����ւ���}�`
	// t	��������̓���ւ���}�`
	// r_dict	���\�[�X�f�B���N�g��
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

	// �ꗗ�̗v�f�̑I������������.
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

	// �}�`�ꗗ�p�l���́u����v�{�^���������ꂽ.
	void MainPage::smry_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		smry_close();
	}

	// �}�`�ꗗ�p�l�������[�h���ꂽ.
	void MainPage::smry_loaded(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const& /*e*/)
	{
		// �}�`�ꗗ�̔r������� false �Ɠ���ւ���.
		bool smry_visible = m_smry_atomic.exchange(false, std::memory_order_acq_rel); // ����ւ��O�̔r������
		smry_add_items(lv_smry(), m_list_shapes, Resources());
		m_smry_atomic.store(smry_visible, std::memory_order_release);
	}

	// �}�`�ꗗ�̍��ڂ��I�����ꂽ.
	void MainPage::smry_selection_changed(IInspectable const&, SelectionChangedEventArgs const& e)
	{
		// �}�`�ꗗ�̔r�����䂪 false �����肷��.
		if (!m_smry_atomic) {
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
			const auto s = smry_shape(item[0]);
			if (s != nullptr && s->is_selected()) {
				undo_push_select(s);
				//if (m_event_shape_smry == s) {
				//	m_event_shape_smry = nullptr;
				//}
			}
		}
		// �I�����ꂽ���ڂ������, �I��������.
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
			// �}�`���\�������悤�p�����X�N���[������.
			scroll_to(t);
			m_sheet_main.set_attr_to(t);
			sheet_attr_is_checked();
		}
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		edit_menu_is_enabled();
		sheet_draw();
	}

	// �ҏW���j���[�́u���X�g��\���v���I�����ꂽ.
	void MainPage::mfi_smry_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
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

	// �}�`���ꗗ�ɒǉ�����.
	void MainPage::smry_append(Shape* const s)
	{
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			lv_smry().Items().Append(winrt::make<Summary>(s, Resources()));
			if (s->is_selected()) {
				smry_select_item(lv_smry(), lv_smry().Items().Size() - 1);
			}
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̒��Ő}�`�����ւ���.
	void MainPage::smry_arrng(Shape* const s, Shape* const t)
	{
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			smry_swap(lv_smry(), s, t, Resources());
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�ꗗ����������.
	void MainPage::smry_clear(void)
	{
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			lv_smry().Items().Clear();
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// �}�`�ꗗ�p�l������ď�������.
	void MainPage::smry_close(void)
	{
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			gd_smry_panel().Visibility(UI_COLLAPSED);
			lv_smry().Items().Clear();
			UnloadObject(gd_smry_panel());
		}
	}

	// �ꗗ�̓Y�����̈ʒu�ɐ}�`��}������.
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

	// �����}�`�ꗗ�ɔ��f����.
	// ���̊֐���, ��������s����O�ɌĂяo��.
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
						//smry_unselect_item(lv_smry(), i);	// �����[�u����O�ɑI�����O���Ȃ��ƃV�X�e��������ɑ��̍��ڂ�I������.
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

	// �}�`�ꗗ���쐬���Ȃ���.
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

	// �}�`���ꗗ�����������.
	uint32_t MainPage::smry_remove(Shape* const s)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			auto items = lv_smry().Items();
			const uint32_t i = smry_distance(items, s);
			if (i < items.Size()) {
				smry_unselect_item(lv_smry(), i);	// �����[�u����O�ɑI�����O���Ȃ��ƃV�X�e��������ɑ��̍��ڂ�I������.
				items.RemoveAt(i);
			}
			m_smry_atomic.store(true, std::memory_order_release);
			return i;
		}
		return 0;
	}

	// �ꗗ�̐}�`��I������.
	void MainPage::smry_select(Shape* const s)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			smry_select_item(lv_smry(), smry_distance(lv_smry().Items(), s));
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̍��ڂ�S�đI������.
	void MainPage::smry_select_all(void)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			lv_smry().SelectAll();
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̍ŏ��̍��ڂ�I������.
	void MainPage::smry_select_head(void)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			smry_select_item(lv_smry(), 0);
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̍Ō�̍��ڂ�I������.
	void MainPage::smry_select_tail(void)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			smry_select_item(lv_smry(), lv_smry().Items().Size() - 1);
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̐}�`��I����������.
	void MainPage::smry_unselect(Shape* const s)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			smry_unselect_item(lv_smry(), smry_distance(lv_smry().Items(), s));
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̍��ڂ�S�đI����������.
	void MainPage::smry_unselect_all(void)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			lv_smry().SelectedIndex(static_cast<uint32_t>(-1));
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̕\�����X�V����.
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