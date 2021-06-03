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
			return reinterpret_cast<Shape*>(s->get_shape());
		}
		return nullptr;
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
	// view	���X�g�r���[
	// s	����ւ���}�`
	// t	��������̓���ւ���}�`
	// r	���\�[�X�f�B���N�g��
	static void smry_swap(ListView const& view, Shape* const s, Shape* const t, ResourceDictionary const& r)
	{
		auto const& items = view.Items();
		const auto i = smry_distance(items, s);
		const auto j = smry_distance(items, t);
		const auto p = s->is_selected();
		const auto q = t->is_selected();
		auto t_item = winrt::make<Summary>(t, r);
		auto s_item = winrt::make<Summary>(s, r);
		items.SetAt(i, t_item);
		items.SetAt(j, s_item);
		if (p) {
			smry_select_item(view, j);
		}
		if (q) {
			smry_select_item(view, i);
		}
	}

	// �ꗗ�̗v�f�̑I������������.
	static void smry_unselect_item(ListView const& view, const uint32_t i)
	{
		IInspectable t_item[1];
		view.Items().GetMany(i, t_item);
		const auto t = smry_shape(t_item[0]);
		const auto k = view.SelectedItems().Size();
		for (uint32_t j = 0; j < k; j++) {
			IInspectable s_item[1];
			view.SelectedItems().GetMany(j, s_item);
			const auto s = smry_shape(s_item[0]);
			if (s == t) {
				view.SelectedItems().RemoveAt(j);
				break;
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
		uint32_t i = 0;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (m_smry_descend) {
				lv_smry().Items().InsertAt(0, winrt::make<Summary>(s, Resources()));
				if (s->is_selected()) {
					smry_select_item(lv_smry(), 0);
				}
			}
			else {
				lv_smry().Items().Append(winrt::make<Summary>(s, Resources()));
				if (s->is_selected()) {
					smry_select_item(lv_smry(), i);
				}
			}
			i++;
		}
		// ����ւ��O�̔r������ɖ߂�.
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
		for (uint32_t i = 0; i < e.RemovedItems().Size(); i++) {
			IInspectable item[1];
			e.RemovedItems().GetMany(i, item);
			auto s = smry_shape(item[0]);
			if (s != nullptr && s->is_selected()) {
				undo_push_select(s);
			}
		}
		auto t = static_cast<Shape*>(nullptr);
		for (uint32_t i = 0; i < e.AddedItems().Size(); i++) {
			IInspectable item[1];
			e.AddedItems().GetMany(i, item);
			auto const s = smry_shape(item[0]);
			if (s != nullptr && s->is_selected() != true) {
				undo_push_select(t = s);
			}
		}
		if (t != static_cast<const Shape*>(nullptr)) {
			// �}�`���\�������悤�p�����X�N���[������.
			scroll_to(t);
			m_sheet_main.set_attr_to(t);
		}
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		edit_menu_enable();
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
		if (sp_text_find().Visibility() == VISIBLE) {
			text_find_click(nullptr, nullptr);
		}
		auto _{ FindName(L"rp_smry") };
		rp_smry().Visibility(VISIBLE);
		m_smry_atomic.store(true, std::memory_order_release);
	}

	// �}�`���ꗗ�ɒǉ�����.
	void MainPage::smry_append(Shape* const s)
	{
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			if (m_smry_descend) {
				lv_smry().Items().InsertAt(0, winrt::make<Summary>(s, Resources()));
				if (s->is_selected()) {
					smry_select_item(lv_smry(), 0);
				}
			}
			else {
				lv_smry().Items().Append(winrt::make<Summary>(s, Resources()));
				if (s->is_selected()) {
					smry_select_item(lv_smry(), lv_smry().Items().Size() - 1);
				}
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
			rp_smry().Visibility(COLLAPSED);
			lv_smry().Items().Clear();
			UnloadObject(rp_smry());
		}
	}

	// �ꗗ�̓Y�����̈ʒu�ɐ}�`��}������.
	void MainPage::smry_insert(Shape* const s, const uint32_t i)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			if (m_smry_descend) {
				const uint32_t j = lv_smry().Items().Size() - i;
				lv_smry().Items().InsertAt(j, winrt::make<Summary>(s, Resources()));
				if (s->is_selected()) {
					smry_select_item(lv_smry(), j);
				}
				else {
					smry_unselect_item(lv_smry(), j);
				}
			}
			else {
				lv_smry().Items().InsertAt(i, winrt::make<Summary>(s, Resources()));
				if (s->is_selected()) {
					smry_select_item(lv_smry(), i);
				}
				else {
					smry_unselect_item(lv_smry(), i);
				}
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
						if (m_smry_descend) {
							i++;
						}
						lv_smry().Items().InsertAt(i, winrt::make<Summary>(s, Resources()));
						if (s->is_selected()) {
							smry_select_item(lv_smry(), i);
						}
					}
					else {
						if (m_smry_descend) {
							lv_smry().Items().InsertAt(0, winrt::make<Summary>(s, Resources()));
							if (s->is_selected()) {
								smry_select_item(lv_smry(), 0);
							}
						}
						else {
							lv_smry().Items().Append(winrt::make<Summary>(s, Resources()));
							const auto i = lv_smry().Items().Size() - 1;
							smry_select_item(lv_smry(), i);
						}
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
			uint32_t i = 0;
			for (auto s : m_list_shapes) {
				if (s->is_deleted()) {
					continue;
				}
				lv_smry().Items().Append(winrt::make<Summary>(s, Resources()));
				if (s->is_selected()) {
					smry_select_item(lv_smry(), i);
				}
				i++;
			}
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

	// �ꗗ�̍��ڂ�I������.
	//void MainPage::smry_select(uint32_t i)
	//{
	//	if (m_smry_atomic.load(std::memory_order_acquire)) {
	//		m_smry_atomic.store(false, std::memory_order_release);
	//		smry_select_item(lv_smry(), i);
	//		m_smry_atomic.store(true, std::memory_order_release);
	//	}
	//}

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
			if (m_smry_descend) {
				smry_select_item(lv_smry(), lv_smry().Items().Size() - 1);
			}
			else {
				smry_select_item(lv_smry(), 0);
			}
			m_smry_atomic.store(true, std::memory_order_release);
		}
	}

	// �ꗗ�̍Ō�̍��ڂ�I������.
	void MainPage::smry_select_tail(void)
	{
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			m_smry_atomic.store(false, std::memory_order_release);
			if (m_smry_descend) {
				smry_select_item(lv_smry(), 0);
			}
			else {
				smry_select_item(lv_smry(), lv_smry().Items().Size() - 1);
			}
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

	// �ꗗ�̍��ڂ�I����������.
	//void MainPage::smry_unselect(uint32_t i)
	//{
	//	if (m_smry_atomic.load(std::memory_order_acquire)) {
	//		m_smry_atomic.store(false, std::memory_order_release);
	//		smry_unselect_item(lv_smry(), i);
	//		m_smry_atomic.store(true, std::memory_order_release);
	//	}
	//}

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

}