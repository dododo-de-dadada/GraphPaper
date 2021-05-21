//-------------------------------
// MainPage_text.cpp
// ������̕ҏW, �܂��͌��� / �u��
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �u�����񂪌�����܂���v���b�Z�[�W�̃��\�[�X�L�[
	constexpr wchar_t NO_FOUND[] = L"str_err_found";

	// ��������������Č��������ʒu�𓾂�.
	static bool text_find(const wchar_t* w_text, const uint32_t w_len, const wchar_t* f_text, const uint32_t f_len, const bool f_case, uint32_t& f_pos) noexcept;
	// �}�`���X�g�̒����當�����������, ����܂ŕ����͈͂��I������Ă����}�`, �V���ɕ����񂪌��������}�`�Ƃ��̕����͈͂𓾂�.
	static bool text_find(S_LIST_T& s_list, wchar_t* f_text, const bool f_case, const bool f_wrap, ShapeText*& t, Shape*& s, DWRITE_TEXT_RANGE& s_range);
	// ������̈ꕔ��u������.
	static wchar_t* text_replace(wchar_t const* w_text, const uint32_t w_pos, const uint32_t w_len, wchar_t const* r_text, const uint32_t r_len) noexcept;
	// �����͈͂��I�����ꂽ�}�`�ƕ����͈͂�������.
	static ShapeText* text_find_range_selected(S_LIST_T::iterator const& it_beg, S_LIST_T::iterator const& it_end, DWRITE_TEXT_RANGE& t_range);

	// ��������������Č��������ʒu�𓾂�.
	// w_text	��������镶����
	// w_len	��������镶����̕�����
	// w_pos	�������J�n����ʒu
	// f_text	����������
	// f_len	����������̕�����
	// f_case	�p�����̋�ʃt���O
	// f_break	���s�𖳎��t���O
	// f_pos	���������ʒu
	static bool text_find(const wchar_t* w_text, const uint32_t w_len, const wchar_t* f_text, const uint32_t f_len, const bool f_case, uint32_t& f_pos) noexcept
	{
		if (w_text == nullptr
			|| f_text == nullptr
			|| f_len > w_len || f_len == 0) {
			// ��������镶���񂪃k��, 
			// �܂��͌��������񂪃k��, 
			// �܂��͕�����������������̕�������菬����,
			// �܂��͌���������̕������� 0 �̏ꍇ
			// false ��Ԃ�.
			return false;
		}
		if (f_case) {
			// �p�����̋�ʃt���O�������Ă���ꍇ,
			for (uint32_t i = 0; i <= w_len - f_len; i++) {
				if (wcsncmp(w_text + i, f_text, f_len) == 0) {
					f_pos = i;
					return true;
				}
			}
		}
		else {
			// �p�����̋�ʃt���O���Ȃ��ꍇ,
			for (uint32_t i = 0; i <= w_len - f_len; i++) {
				if (_wcsnicmp(w_text + i, f_text, f_len) == 0) {
					f_pos = i;
					return true;
				}
			}
		}
		return false;
	}

	// �}�`���X�g�̒����當�����������, ����܂ŕ����͈͂��I������Ă����}�`, �V���ɕ����񂪌��������}�`�Ƃ��̕����͈͂𓾂�.
	// s_list	�}�`���X�g
	// f_text	����������
	// f_len	����������̕�����
	// f_case	�p�����̋�ʃt���O
	// f_wrap	��荞�݌����t���O
	// t	����܂ŕ����͈͂��I������Ă����}�`
	// s_found	�V���ɕ����񂪌��������}�`
	// s_range	�V���ɕ����񂪌��������}�`�̕����͈�
	static bool text_find(S_LIST_T& s_list, wchar_t* f_text, const bool f_case, const bool f_wrap, ShapeText*& t, Shape*& s_found, DWRITE_TEXT_RANGE& s_range)
	{
		const auto f_len = wchar_len(f_text);
		if (f_len == 0) {
			return false;
		}
		std::list<S_LIST_T::iterator> stack;
		stack.push_back(s_list.begin());
		stack.push_back(s_list.end());
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
					if (text_find(t->m_text + t_end, wchar_len(t->m_text) - t_end, f_text, f_len, f_case, f_pos)) {
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
				if (text_find(t_text, wchar_len(t_text), f_text, f_len, f_case, f_pos)) {
					s_found = s;
					s_range.startPosition = f_pos;
					s_range.length = f_len;
					return true;
				}
			}
		}
		if (f_wrap != true && t != nullptr) {
			// ��荞�݌������Ȃ�, �������͈͂��I�����ꂽ�}�`�����������ꍇ,
			return false;
		}
		stack.push_back(s_list.begin());
		stack.push_back(s_list.end());
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
					if (t_pos != 0 && text_find(t_text, t_pos, f_text, f_len, f_case, f_pos)) {
						s_found = s;
						s_range.startPosition = f_pos;
						s_range.length = f_len;
						return true;
					}
					return false;
				}
				uint32_t f_pos;
				const auto t_text = static_cast<ShapeText*>(s)->m_text;
				if (text_find(t_text, wchar_len(t_text), f_text, f_len, f_case, f_pos)) {
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
	static ShapeText* text_find_range_selected(S_LIST_T::iterator const& it_beg, S_LIST_T::iterator const& it_end, DWRITE_TEXT_RANGE& t_range)
	{
		std::list<S_LIST_T::iterator> stack;
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

	// ������̈ꕔ��u������.
	// w_text	�u�������O�̕�����
	// f_pos	�u������镶����̊J�n�ʒu
	// f_len	�u������镶����
	// r_text	�u��������
	// r_len	�u��������̕�����
	// �߂�l	�u�����ꂽ������
	static wchar_t* text_replace(wchar_t const* w_text, const uint32_t f_pos, const uint32_t f_len, wchar_t const* r_text, const uint32_t r_len) noexcept
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

	// �����񌟍��p�l���́u����v�{�^���������ꂽ.
	void MainPage::text_find_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		text_find_set();
		sp_text_find().Visibility(COLLAPSED);
	}

	// �����񌟍��p�l���́u���������v�{�^���������ꂽ.
	void MainPage::text_find_next_click(IInspectable const&, RoutedEventArgs const&)
	{
		text_find_set();
		ShapeText* t;
		Shape* s;
		DWRITE_TEXT_RANGE s_range;
		if (text_find(m_list_shapes, m_text_find, m_text_find_case, m_text_find_wrap, t, s, s_range)) {
			if (t != nullptr && s != t) {
				undo_push_set<UNDO_OP::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ 0, 0 });
			}
			undo_push_set<UNDO_OP::TEXT_RANGE>(s, s_range);
			scroll_to(s);
			sheet_draw();
			return;
		}
		// �����ł��Ȃ��ꍇ,
		// �u������͌�����܂���v���b�Z�[�W�_�C�A���O��\������.
		message_show(ICON_INFO, NO_FOUND, tx_text_find_what().Text());
	}

	// �����񌟍��p�l���́u���ׂĒu���v�{�^���������ꂽ.
	void MainPage::text_replace_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		text_find_set();
		// ����������̕������𓾂�.
		const auto f_len = wchar_len(m_text_find);
		if (f_len == 0) {
			// �������� 0 �̏ꍇ,
			return;
		}

		// ���炩���ߌ�����������܂ޕ�����}�`�����邩���肷��.
		auto flag = false;
		std::list <S_LIST_T::iterator> stack;
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
				if (s->get_text(w_text) != true) {
					continue;
				}
				uint32_t f_pos = 0;
				if (text_find(w_text, wchar_len(w_text), m_text_find, f_len, m_text_find_case, f_pos)) {
					flag = true;
					break;
				}
			}
		}
		stack.clear();
		if (flag != true) {
			// �}�`���Ȃ��ꍇ,
			// �u������͌�����܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_INFO, NO_FOUND, tx_text_find_what().Text());
			return;
		}
		// �����͈͂̑I���݂̂���������.
		unselect_all(true);

		const auto r_len = wchar_len(m_text_repl);
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
				flag = false;	// ��v�t���O
				while (text_find(w_text + w_pos, w_len - w_pos, m_text_find, f_len, m_text_find_case, f_pos)) {
					flag = true;
					w_pos += f_pos;
					auto t_text = text_replace(w_text, w_pos, f_len, m_text_repl, r_len);
					delete[] w_text;
					w_text = t_text;
					w_len += r_len - f_len;
					w_pos += r_len;
				}
				if (flag) {
					undo_push_set<UNDO_OP::TEXT_CONTENT>(t, w_text);
				}
			}
		}
		undo_push_null();
		undo_menu_enable();
		sheet_draw();
	}

	// �����񌟍��p�l���́u�u�����Ď��Ɂv�{�^���������ꂽ.
	void MainPage::text_replace_click(IInspectable const&, RoutedEventArgs const&)
	{
		text_find_set();
		const auto f_len = wchar_len(m_text_find);
		if (f_len == 0) {
			return;
		}

		bool flag = false;	// ��v�܂��͒u���t���O.
		DWRITE_TEXT_RANGE t_range;
		auto t = text_find_range_selected(m_list_shapes.begin(), m_list_shapes.end(), t_range);
		if (t != nullptr) {
			// �}�`�����������ꍇ,
			const auto w_pos = t_range.startPosition;
			if (m_text_find_case) {
				// �p�����̋�ʃt���O�������Ă���ꍇ,
				flag = wcsncmp(t->m_text + w_pos, m_text_find, f_len) == 0;
			}
			else {
				// �p�����̋�ʃt���O���Ȃ��ꍇ,
				flag = _wcsnicmp(t->m_text + w_pos, m_text_find, f_len) == 0;
			}
			if (flag) {
				// ��v�����ꍇ
				const auto r_len = wchar_len(m_text_repl);
				const auto r_text = text_replace(t->m_text, w_pos, f_len, m_text_repl, r_len);
				undo_push_set<UNDO_OP::TEXT_CONTENT>(t, r_text);
				undo_push_set<UNDO_OP::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ w_pos, r_len });
				undo_push_null();
				undo_menu_enable();
			}
		}
		// ������������.
		Shape* s;
		DWRITE_TEXT_RANGE s_range;
		if (text_find(m_list_shapes, m_text_find, m_text_find_case, m_text_find_wrap, t, s, s_range)) {
			// �����ł����ꍇ,
			if (t != nullptr && s != t) {
				// �����͈͂��I�����ꂽ�}�`������, ���ꂪ���̐}�`�ƈقȂ�ꍇ,
				undo_push_set<UNDO_OP::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ 0, 0 });
			}
			undo_push_set<UNDO_OP::TEXT_RANGE>(s, s_range);
			scroll_to(s);
			flag = true;
		}
		if (flag) {
			sheet_draw();
			return;
		}
		// �����ł��Ȃ�, ���u��������ĂȂ��ꍇ,
		message_show(ICON_INFO, NO_FOUND, tx_text_find_what().Text());
	}

	// �ҏW���j���[�́u������̕ҏW�v���I�����ꂽ.
	void MainPage::text_edit_click(IInspectable const&, RoutedEventArgs const&)
	{
		ShapeText* s = nullptr;
		if (pointer_shape_prev() != nullptr && typeid(*pointer_shape_prev()) == typeid(ShapeText)) {
			// �O��|�C���^�[�������ꂽ�̂�������}�`�̏ꍇ,
			s = static_cast<ShapeText*>(pointer_shape_prev());
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
			text_edit_async(s);
		}

	}

	// �ҏW���j���[�́u������̌���/�u���v���I�����ꂽ.
	void MainPage::text_find_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (sp_text_find().Visibility() == VISIBLE) {
			// �����񌟍��p�l�����\������Ă���ꍇ,
			// �����񌟍��p�l�����\���ɂ���.
			sp_text_find().Visibility(COLLAPSED);
			text_find_set();
			return;
		}
		if (m_summary_atomic.load(std::memory_order_acquire)) {
		//if (m_summary_visible) {
			// �}�`�ꗗ�p�l�����\������Ă���ꍇ,
			// �}�`�ꗗ�p�l�����\���ɂ���.
			summary_close();
		}
		tx_text_find_what().Text({ m_text_find == nullptr ? L"" : m_text_find });
		tx_text_replace_with().Text({ m_text_repl == nullptr ? L"" : m_text_repl });
		ck_text_find_case().IsChecked(m_text_find_case);
		ck_text_find_wrap().IsChecked(m_text_find_wrap);
		sp_text_find().Visibility(VISIBLE);
	}

	// �}�`�����������ҏW����.
	// s	������}�`
	IAsyncAction MainPage::text_edit_async(ShapeText* s)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		static winrt::event_token primary_token;
		static winrt::event_token closed_token;

		tx_edit().Text(s->m_text == nullptr ? L"" : s->m_text);
		tx_edit().SelectAll();
		ck_text_adjust_bbox().IsChecked(m_text_adjust);
		if (co_await cd_edit_text().ShowAsync() == ContentDialogResult::Primary) {
			auto text = wchar_cpy(tx_edit().Text().c_str());
			undo_push_set<UNDO_OP::TEXT_CONTENT>(s, text);
			if (m_text_adjust = ck_text_adjust_bbox().IsChecked().GetBoolean()) {
				undo_push_anchor(s, ANCH_TYPE::ANCH_SE);
				s->adjust_bbox();
			}
			// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
			undo_push_null();
			edit_menu_enable();
			sheet_draw();
		}
		/*
		primary_token = cd_edit_text().PrimaryButtonClick(
			[this, s](auto, auto)
			{
				auto text = wchar_cpy(tx_edit().Text().c_str());
				undo_push_set<UNDO_OP::TEXT_CONTENT>(s, text);
				if (ck_text_adjust_bbox().IsChecked().GetBoolean()) {
					undo_push_anchor(s, ANCH_TYPE::ANCH_SE);
					s->adjust_bbox();
				}
				// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
				undo_push_null();
				// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
				edit_menu_enable();
			}
		);
		closed_token = cd_edit_text().Closed(
			[this](auto, auto)
			{
				cd_edit_text().PrimaryButtonClick(primary_token);
				cd_edit_text().Closed(closed_token);
				sheet_draw();
			}
		);
		auto _{ cd_edit_text().ShowAsync() };
		*/
	}

	// �����̒l���f�[�^���[�_�[����ǂݍ���.
	void MainPage::text_find_read(DataReader const& dt_reader)
	{
		read(m_text_find, dt_reader);
		read(m_text_repl, dt_reader);
		uint16_t bit = dt_reader.ReadUInt16();
		m_text_adjust = ((bit & 1) != 0);
		m_text_find_case = ((bit & 2) != 0);
		m_text_find_wrap = ((bit & 4) != 0);
	}

	// �����񌟍��p�l������l���i�[����.
	void MainPage::text_find_set(void)
	{
		if (m_text_find != nullptr) {
			delete[] m_text_find;
		}
		m_text_find = wchar_cpy_esc(tx_text_find_what().Text().c_str());
		if (m_text_repl != nullptr) {
			delete[] m_text_repl;
		}
		m_text_repl = wchar_cpy_esc(tx_text_replace_with().Text().c_str());
		m_text_find_case = ck_text_find_case().IsChecked().GetBoolean();
		m_text_find_wrap = ck_text_find_wrap().IsChecked().GetBoolean();
	}

	// �����̒l���f�[�^���[�_�[�ɏ�������.
	void MainPage::text_find_write(DataWriter const& dt_writer)
	{
		write(m_text_find, dt_writer);
		write(m_text_repl, dt_writer);
		uint16_t bit = 0;
		if (m_text_adjust) {
			bit |= 1;
		}
		if (m_text_find_case) {
			bit |= 2;
		}
		if (m_text_find_wrap) {
			bit |= 4;
		}
		dt_writer.WriteUInt16(bit);
	}

	// ���������񂪕ύX���ꂽ.
	void MainPage::text_find_what_changed(IInspectable const&, TextChangedEventArgs const&)
	{
		const auto not_empty = (tx_text_find_what().Text().empty() != true);
		btn_text_find_next().IsEnabled(not_empty);
		btn_text_replace().IsEnabled(not_empty);
		btn_text_replace_all().IsEnabled(not_empty);
	}

}