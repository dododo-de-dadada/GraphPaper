//-------------------------------
// MainPage_find.cpp
// ������̕ҏW, �܂��͌��� / �u��
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t NOT_FOUND[] = L"str_info_not_found";	// �u�����񂪌�����܂���v���b�Z�[�W�̃��\�[�X�L�[

	// ������̈ꕔ��u������.
	static wchar_t* find_replace(wchar_t const* w_text, const uint32_t w_pos, const uint32_t w_len, wchar_t const* r_text, const uint32_t r_len) noexcept;
	// ��������������Č��������ʒu�𓾂�.
	static bool find_text(const wchar_t* w_text, const uint32_t w_len, const wchar_t* f_text, const uint32_t f_len, const bool f_case, uint32_t& f_pos) noexcept;
	// �}�`���X�g�̒����當�����������, ����܂ŕ����͈͂��I������Ă����}�`, �V���ɕ����񂪌��������}�`�Ƃ��̕����͈͂𓾂�.
	static bool find_text(SHAPE_LIST& slist, wchar_t* f_text, const bool f_case, const bool f_wrap, ShapeText*& t, Shape*& s, DWRITE_TEXT_RANGE& s_range);
	// �����͈͂��I�����ꂽ�}�`�ƕ����͈͂�������.
	static ShapeText* find_text_range_selected(SHAPE_LIST::iterator const& it_beg, SHAPE_LIST::iterator const& it_end, DWRITE_TEXT_RANGE& t_range);

	// ������̈ꕔ��u������.
	// w_text	�u�������O�̕�����
	// f_pos	�u������镶����̊J�n�ʒu
	// f_len	�u������镶����
	// r_text	�u��������
	// r_len	�u��������̕�����
	// �߂�l	�u�����ꂽ������
	static wchar_t* find_replace(wchar_t const* w_text, const uint32_t f_pos, const uint32_t f_len, wchar_t const* r_text, const uint32_t r_len) noexcept
	{
		if (w_text != nullptr) {
			// �u�������O�̕�����̕������𓾂�.
			const uint32_t w_len = wchar_len(w_text);
			// �u������镶����̏I���ʒu�𓾂�.
			const uint32_t f_end = f_pos + f_len;
			if (f_end <= w_len) {
				// �I���ʒu���������ȉ��̏ꍇ,
				// �u�����ꂽ��̕�����̏I���ʒu�𓾂�.
				const size_t r_end = static_cast<size_t>(f_pos) + static_cast<size_t>(r_len);
				// �u�����ꂽ��̕�����̕������𓾂�.
				const size_t n_len = static_cast<size_t>(w_len) + static_cast<size_t>(r_len) - static_cast<size_t>(f_len);
				// �u�����ꂽ��̕�������i�[����z����m�ۂ���.
				wchar_t* const n_text = new wchar_t[n_len + 1];
				// �J�n�ʒu�܂ł̕������z��Ɋi�[����.
				wcsncpy_s(n_text, n_len + 1, w_text, static_cast<size_t>(f_pos));
				// �u���������z��Ɋi�[����.
				wcsncpy_s(n_text + f_pos, n_len - f_pos + 1, r_text, r_len);
				// �I���ʒu�����̕������z��Ɋi�[����.
				wcsncpy_s(n_text + r_end, n_len - r_end + 1, w_text + f_end, w_len - f_end);
				// �u�����ꂽ�������Ԃ�.
				return n_text;
			}
		}
		return wchar_cpy(r_text);
	}

	// ��������������Č��������ʒu�𓾂�.
	// w_text	��������镶����
	// w_len	��������镶����̕�����
	// w_pos	�������J�n����ʒu
	// f_text	����������
	// f_len	����������̕�����
	// f_case	�p��������ʂ���
	// f_break	���s�𖳎�����
	// f_pos	���������ʒu
	static bool find_text(const wchar_t* w_text, const uint32_t w_len, const wchar_t* f_text, const uint32_t f_len, const bool f_case, uint32_t& f_pos) noexcept
	{
		// ��������镶���񂪔�k��, �����������񂪔�k��, 
		// ������������̕��������[�����傫��, ��������镶����̕������ȉ������肷��.
		if (w_text != nullptr && f_text != nullptr && f_len > 0 && f_len <= w_len) {
			// �p��������ʂ��邩���肷��.
			if (f_case) {
				// ��ʂ���Ȃ�,
				for (uint32_t i = 0; i <= w_len - f_len; i++) {
					if (wcsncmp(w_text + i, f_text, f_len) == 0) {
						f_pos = i;
						return true;
					}
				}
			}
			else {
				// ��ʂ��Ȃ��Ȃ�,
				for (uint32_t i = 0; i <= w_len - f_len; i++) {
					if (_wcsnicmp(w_text + i, f_text, f_len) == 0) {
						f_pos = i;
						return true;
					}
				}
			}
		}
		return false;
	}

	// �}�`���X�g�̒����當�����������, ����܂ŕ����͈͂��I������Ă����}�`, �V���ɕ����񂪌��������}�`�Ƃ��̕����͈͂𓾂�.
	// slist	�}�`���X�g
	// f_text	����������
	// f_len	����������̕�����
	// f_case	�p��������ʂ���
	// f_wrap	��荞�݌�������
	// t	����܂ŕ����͈͂��I������Ă����}�`
	// s_found	�V���ɕ����񂪌��������}�`
	// s_range	�V���ɕ����񂪌��������}�`�̕����͈�
	static bool find_text(SHAPE_LIST& slist, wchar_t* f_text, const bool f_case, const bool f_wrap, ShapeText*& t, Shape*& s_found, DWRITE_TEXT_RANGE& s_range)
	{
		const auto f_len = wchar_len(f_text);
		if (f_len == 0) {
			return false;
		}
		std::list<SHAPE_LIST::iterator> stack;
		stack.push_back(slist.begin());
		stack.push_back(slist.end());
		t = static_cast<ShapeText*>(nullptr);
		uint32_t t_pos = 0;
		while (stack.empty() != true) {
			auto j = stack.back();
			stack.pop_back();
			auto i = stack.back();
			stack.pop_back();
			while (i != j) {
				auto s = *i++;
				if (s == nullptr || s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeGroup)) {
					stack.push_back(i);
					stack.push_back(j);
					i = static_cast<ShapeGroup*>(s)->m_list_grouped.begin();
					j = static_cast<ShapeGroup*>(s)->m_list_grouped.end();
					continue;
				}
				DWRITE_TEXT_RANGE t_range;
				if (s->get_text_range(t_range) != true) {
					continue;
				}
				if (t == nullptr) {
					if (t_range.length == 0 && t_range.startPosition == 0) {
						continue;
					}
					// �����͈͂��I�����ꂽ�}�`��, �����͈͂����̕��������������.
					t = static_cast<ShapeText*>(s);
					t_pos = t_range.startPosition;
					const auto t_end = t_pos + t_range.length;
					uint32_t f_pos;
					if (find_text(t->m_text + t_end, wchar_len(t->m_text) - t_end, f_text, f_len, f_case, f_pos)) {
						// �����͈͂����̕�����̒��Ō��������ꍇ
						s_found = s;
						s_range.startPosition = t_end + f_pos;
						s_range.length = f_len;
						return true;
					}
					continue;
				}
				uint32_t f_pos;
				const auto t_text = static_cast<ShapeText*>(s)->m_text;
				if (find_text(t_text, wchar_len(t_text), f_text, f_len, f_case, f_pos)) {
					s_found = s;
					s_range.startPosition = f_pos;
					s_range.length = f_len;
					return true;
				}
			}
		}
		// ��荞�݌������Ȃ�, �������͈͂��I�����ꂽ�}�`���������������肷��.
		if (f_wrap != true && t != nullptr) {
			return false;
		}
		stack.push_back(slist.begin());
		stack.push_back(slist.end());
		while (stack.empty() != true) {
			auto j = stack.back();
			stack.pop_back();
			auto i = stack.back();
			stack.pop_back();
			while (i != j) {
				auto s = *i++;
				if (s == nullptr || s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeGroup)) {
					stack.push_back(i);
					stack.push_back(j);
					i = static_cast<ShapeGroup*>(s)->m_list_grouped.begin();
					j = static_cast<ShapeGroup*>(s)->m_list_grouped.end();
					continue;
				}
				if (typeid(*s) != typeid(ShapeText)) {
					continue;
				}
				if (s == t) {
					uint32_t f_pos;
					const auto t_text = static_cast<ShapeText*>(s)->m_text;
					if (t_pos != 0 && find_text(t_text, t_pos, f_text, f_len, f_case, f_pos)) {
						s_found = s;
						s_range.startPosition = f_pos;
						s_range.length = f_len;
						return true;
					}
					return false;
				}
				uint32_t f_pos;
				const auto t_text = static_cast<ShapeText*>(s)->m_text;
				if (find_text(t_text, wchar_len(t_text), f_text, f_len, f_case, f_pos)) {
					s_found = s;
					s_range.startPosition = f_pos;
					s_range.length = f_len;
					return true;
				}
			}
		}
		return false;
	}

	// �����͈͂��I�����ꂽ�}�`�ƕ����͈͂�������.
	// t_range	�������������͈�
	// �߂�l	���������}�`
	static ShapeText* find_text_range_selected(SHAPE_LIST::iterator const& it_beg, SHAPE_LIST::iterator const& it_end, DWRITE_TEXT_RANGE& t_range)
	{
		std::list<SHAPE_LIST::iterator> stack;
		stack.push_back(it_beg);
		stack.push_back(it_end);
		while (stack.empty() != true) {
			auto j = stack.back();
			stack.pop_back();
			auto i = stack.back();
			stack.pop_back();
			while (i != j) {
				auto k = i;
				auto s = *i++;
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeGroup)) {
					stack.push_back(i);
					stack.push_back(j);
					i = static_cast<ShapeGroup*>(s)->m_list_grouped.begin();
					j = static_cast<ShapeGroup*>(s)->m_list_grouped.end();
					continue;
				}
				if (s->get_text_range(t_range) != true) {
					continue;
				}
				if (t_range.startPosition > 0 || t_range.length > 0) {
					return static_cast<ShapeText*>(s);
				}
			}
		}
		return static_cast<ShapeText*>(nullptr);
	}

	// �}�`�����������ҏW����.
	// s	������}�`
	IAsyncAction MainPage::edit_text_async(ShapeText* s)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		static winrt::event_token primary_token;
		static winrt::event_token closed_token;

		tx_edit().Text(s->m_text == nullptr ? L"" : s->m_text);
		tx_edit().SelectAll();
		ck_edit_text_frame().IsChecked(m_edit_text_frame);
		if (co_await cd_edit_text_dialog().ShowAsync() == ContentDialogResult::Primary) {
			auto text = wchar_cpy(tx_edit().Text().c_str());
			ustack_push_set<UNDO_OP::TEXT_CONTENT>(s, text);
			m_edit_text_frame = ck_edit_text_frame().IsChecked().GetBoolean();
			if (m_edit_text_frame) {
				ustack_push_anch(s, ANCH_TYPE::ANCH_SE);
				s->adjust_bbox(m_sheet_main.m_grid_snap ? m_sheet_main.m_grid_base + 1.0f : 0.0f);
			}
			ustack_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// �ҏW���j���[�́u������̕ҏW�v���I�����ꂽ.
	void MainPage::edit_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		ShapeText* s = static_cast<ShapeText*>(nullptr);
		if (m_event_shape_prev != nullptr && typeid(*m_event_shape_prev) == typeid(ShapeText)) {
			// �O��|�C���^�[�������ꂽ�̂�������}�`�̏ꍇ,
			s = static_cast<ShapeText*>(m_event_shape_prev);
		}
		else {
			// �I�����ꂽ�}�`�̂����őO�ʂɂ��镶����}�`�𓾂�.
			for (auto it = m_list_shapes.rbegin(); it != m_list_shapes.rend(); it++) {
				auto t = *it;
				if (t->is_deleted()) {
					continue;
				}
				if (t->is_selected() != true) {
					continue;
				}
				if (typeid(*t) == typeid(ShapeText)) {
					s = static_cast<ShapeText*>(t);
					break;
				}
			}
		}
		if (s != nullptr) {
			edit_text_async(s);
		}

	}

	// �����񌟍��p�l���́u���ׂĒu���v�{�^���������ꂽ.
	void MainPage::find_replace_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_set();
		// ����������̕������𓾂�.
		const auto f_len = wchar_len(m_find_text);
		if (f_len == 0) {
			// �������� 0 �̏ꍇ,
			return;
		}

		// ���炩���ߌ�����������܂ޕ�����}�`�����邩���肷��.
		auto flag = false;
		std::list <SHAPE_LIST::iterator> stack;
		stack.push_back(m_list_shapes.begin());
		stack.push_back(m_list_shapes.end());
		while (stack.empty() != true) {
			auto j = stack.back();
			stack.pop_back();
			auto i = stack.back();
			stack.pop_back();
			while (i != j) {
				auto s = *i++;
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeGroup)) {
					stack.push_back(i);
					stack.push_back(j);
					i = static_cast<ShapeGroup*>(s)->m_list_grouped.begin();
					j = static_cast<ShapeGroup*>(s)->m_list_grouped.end();
					continue;
				}
				wchar_t* w_text;
				if (s->get_text_content(w_text) != true) {
					continue;
				}
				uint32_t f_pos = 0;
				if (find_text(w_text, wchar_len(w_text), m_find_text, f_len, m_find_text_case, f_pos)) {
					flag = true;
					break;
				}
			}
		}
		stack.clear();
		if (flag != true) {
			// �}�`���Ȃ��ꍇ,
			// �u������͌�����܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
			return;
		}
		// �����͈͂̑I���݂̂���������.
		unselect_all(true);

		const auto r_len = wchar_len(m_find_repl);
		stack.push_back(m_list_shapes.begin());
		stack.push_back(m_list_shapes.end());
		while (stack.empty() != true) {
			auto j = stack.back();
			stack.pop_back();
			auto i = stack.back();
			stack.pop_back();
			while (i != j) {
				auto s = *i++;
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeGroup)) {
					stack.push_back(i);
					stack.push_back(j);
					i = static_cast<ShapeGroup*>(s)->m_list_grouped.begin();
					j = static_cast<ShapeGroup*>(s)->m_list_grouped.end();
					continue;
				}
				else if (typeid(*s) != typeid(ShapeText)) {
					continue;
				}
				auto t = static_cast<ShapeText*>(s); // ��������镶����}�`
				auto w_text = wchar_cpy(t->m_text);	// ��������镶����
				auto w_len = wchar_len(w_text);// ��������镶����̕�����
				uint32_t w_pos = 0;	// ��������镶���񒆂̈ʒu
				uint32_t f_pos = 0;
				flag = false;	// ��v���������肷��.
				while (find_text(w_text + w_pos, w_len - w_pos, m_find_text, f_len, m_find_text_case, f_pos)) {
					flag = true;
					w_pos += f_pos;
					auto t_text = find_replace(w_text, w_pos, f_len, m_find_repl, r_len);
					delete[] w_text;
					w_text = t_text;
					w_len += r_len - f_len;
					w_pos += r_len;
				}
				if (flag) {
					ustack_push_set<UNDO_OP::TEXT_CONTENT>(t, w_text);
				}
			}
		}
		ustack_push_null();
		ustack_is_enable();
		sheet_draw();
	}

	// �����񌟍��p�l���́u�u�����Ď��Ɂv�{�^���������ꂽ.
	void MainPage::find_replace_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_set();
		const auto f_len = wchar_len(m_find_text);
		if (f_len == 0) {
			return;
		}

		bool flag = false;	// ��v�܂��͒u������������.
		DWRITE_TEXT_RANGE t_range;	// ������͈�
		auto t = find_text_range_selected(m_list_shapes.begin(), m_list_shapes.end(), t_range);
		if (t != nullptr) {
			// �}�`�����������ꍇ,
			const auto w_pos = t_range.startPosition;
			// �p��������ʂ��邩���肷��.
			if (m_find_text_case) {
				flag = wcsncmp(t->m_text + w_pos, m_find_text, f_len) == 0;
			}
			else {
				flag = _wcsnicmp(t->m_text + w_pos, m_find_text, f_len) == 0;
			}
			if (flag) {
				// ��v�����ꍇ
				const auto r_len = wchar_len(m_find_repl);
				const auto r_text = find_replace(t->m_text, w_pos, f_len, m_find_repl, r_len);
				ustack_push_set<UNDO_OP::TEXT_CONTENT>(t, r_text);
				ustack_push_set<UNDO_OP::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ w_pos, r_len });
				ustack_push_null();
				ustack_is_enable();
			}
		}
		// ������������.
		Shape* s;
		DWRITE_TEXT_RANGE s_range;
		if (find_text(m_list_shapes, m_find_text, m_find_text_case, m_find_text_wrap, t, s, s_range)) {
			// �����ł����Ȃ��,
			// �����͈͂��I�����ꂽ�}�`������, ���ꂪ���̐}�`�ƈقȂ邩���肷��.
			if (t != nullptr && s != t) {
				ustack_push_set<UNDO_OP::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ 0, 0 });
			}
			ustack_push_set<UNDO_OP::TEXT_RANGE>(s, s_range);
			scroll_to(s);
			flag = true;
		}
		if (flag) {
			sheet_draw();
		}
		else {
			// �����ł��Ȃ�, ���u��������ĂȂ��ꍇ,
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
		}
	}

	// �ҏW���j���[�́u������̌���/�u���v���I�����ꂽ.
	void MainPage::find_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �����񌟍��p�l�����\������Ă��邩���肷��.
		if (sp_find_text_panel().Visibility() == UI_VISIBLE) {
			sp_find_text_panel().Visibility(UI_COLLAPSED);
			find_text_set();
		}
		else {
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_close_click(nullptr, nullptr);
			}
			tx_find_text_what().Text({ m_find_text == nullptr ? L"" : m_find_text });
			tx_find_replace_with().Text({ m_find_repl == nullptr ? L"" : m_find_repl });
			ck_find_text_case().IsChecked(m_find_text_case);
			ck_find_text_wrap().IsChecked(m_find_text_wrap);
			sp_find_text_panel().Visibility(UI_VISIBLE);
		}
	}

	// �����񌟍��p�l���́u����v�{�^���������ꂽ.
	void MainPage::find_text_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_set();
		sp_find_text_panel().Visibility(UI_COLLAPSED);
	}

	// �����񌟍��p�l���́u���������v�{�^���������ꂽ.
	void MainPage::find_text_next_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_set();
		ShapeText* t;
		Shape* s;
		DWRITE_TEXT_RANGE s_range;
		if (find_text(m_list_shapes, m_find_text, m_find_text_case, m_find_text_wrap, t, s, s_range)) {
			if (t != nullptr && s != t) {
				ustack_push_set<UNDO_OP::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ 0, 0 });
			}
			ustack_push_set<UNDO_OP::TEXT_RANGE>(s, s_range);
			scroll_to(s);
			sheet_draw();
		}
		else {
			// �����ł��Ȃ��ꍇ,
			// �u������͌�����܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
		}
	}

	// �f�[�^���[�_�[���猟���̒l��ǂݍ���.
	//void MainPage::find_text_read(DataReader const& dt_reader)
	//{
	//	dt_read(m_find_text, dt_reader);
	//	dt_read(m_find_repl, dt_reader);
	//	uint16_t bit = dt_reader.ReadUInt16();
	//	m_edit_text_frame = ((bit & 1) != 0);
	//	m_find_text_case = ((bit & 2) != 0);
	//	m_find_text_wrap = ((bit & 4) != 0);
	//}

	// �����񌟍��p�l������l���i�[����.
	void MainPage::find_text_set(void)
	{
		if (m_find_text != nullptr) {
			delete[] m_find_text;
		}
		m_find_text = wchar_cpy_esc(tx_find_text_what().Text().c_str());
		if (m_find_repl != nullptr) {
			delete[] m_find_repl;
		}
		m_find_repl = wchar_cpy_esc(tx_find_replace_with().Text().c_str());
		m_find_text_case = ck_find_text_case().IsChecked().GetBoolean();
		m_find_text_wrap = ck_find_text_wrap().IsChecked().GetBoolean();
	}

	// ���������񂪕ύX���ꂽ.
	void MainPage::find_text_what_changed(IInspectable const&, TextChangedEventArgs const&)
	{
		const auto not_empty = (tx_find_text_what().Text().empty() != true);
		btn_find_text_next().IsEnabled(not_empty);
		btn_find_replace().IsEnabled(not_empty);
		btn_find_replace_all().IsEnabled(not_empty);
	}

	// �����̒l���f�[�^���[�_�[�ɏ�������.
	//void MainPage::find_text_write(DataWriter const& dt_writer)
	//{
	//	dt_write(m_find_text, dt_writer);
	//	dt_write(m_find_repl, dt_writer);
	//	uint16_t bit = 0;
	//	if (m_edit_text_frame) {
	//		bit |= 1;
	//	}
	//	if (m_find_text_case) {
	//		bit |= 2;
	//	}
	//	if (m_find_text_wrap) {
	//		bit |= 4;
	//	}
	//	dt_writer.WriteUInt16(bit);
	//}

}