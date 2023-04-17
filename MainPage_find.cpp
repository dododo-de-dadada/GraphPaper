//-------------------------------
// MainPage_find.cpp
// ������̕ҏW, �܂��͌��� / �u��
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

	constexpr wchar_t NOT_FOUND[] = L"str_info_not_found";	// �u�����񂪌�����܂���v���b�Z�[�W�̃��\�[�X�L�[

	// ������̈ꕔ��u������.
	static wchar_t* find_replace(wchar_t const* w_text, const uint32_t w_pos, const uint32_t w_len, wchar_t const* r_text, const uint32_t r_len) noexcept;
	// ��������������Č��������ʒu�𓾂�.
	static bool find_text(const wchar_t* w_text, const uint32_t w_len, const wchar_t* f_text, const uint32_t f_len, const bool f_case, uint32_t& f_pos) noexcept;
	// �}�`���X�g����, ����܂ŕ����͈͂��I������Ă����}�`, �V���ɑI�������}�`�𓾂�.
	static bool find_text(SHAPE_LIST& slist, wchar_t* f_text, const bool f_case, const bool f_wrap, ShapeText*& t, Shape*& s, DWRITE_TEXT_RANGE& s_range);
	// �����͈͂��I������Ă���}�`��������.
	static ShapeText* find_text_range_selected(SHAPE_LIST::iterator const& it_beg, SHAPE_LIST::iterator const& it_end, DWRITE_TEXT_RANGE& t_range);
	// ������𕡐�����. �G�X�P�[�v������͕����R�[�h�ɕϊ�����.
	static wchar_t* find_wchar_cpy(const wchar_t* const s) noexcept;
	// ������ '0'...'9' �܂��� 'A'...'F', 'a'...'f' �����肷��.
	static bool find_wchar_is_hex(const wchar_t w, uint32_t& x) noexcept;

	// ������ 0...9 �܂��� A...F, a...f �����肷��
	static bool find_wchar_is_hex(const wchar_t w, uint32_t& x) noexcept
	{
		if (isdigit(w)) {
			x = w - '0';
		}
		else if (w >= 'a' && w <= 'f') {
			x = w - 'a' + 10;
		}
		else if (w >= 'A' && w <= 'F') {
			x = w - 'A' + 10;
		}
		else {
			return false;
		}
		return true;
	}

	// ������𕡐�����.
	// �G�X�P�[�v������͕����R�[�h�ɕϊ�����.
	static wchar_t* find_wchar_cpy(const wchar_t* const s) noexcept
	{
		const auto s_len = wchar_len(s);
		if (s_len == 0) {
			return nullptr;
		}
		auto t = new wchar_t[static_cast<size_t>(s_len) + 1];
		auto st = 0;
		uint32_t j = 0;
		for (uint32_t i = 0; i < s_len && s[i] != '\0' && j < s_len; i++) {
			if (st == 0) {
				if (s[i] == '\\') {
					st = 1;
				}
				else {
					t[j++] = s[i];
				}
			}
			else if (st == 1) {
				// \0-9
				if (s[i] >= '0' && s[i] <= '8') {
					t[j] = s[i] - '0';
					st = 2;
				}
				// \x
				else if (s[i] == 'x') {
					st = 4;
				}
				// \u
				else if (s[i] == 'u') {
					st = 6;
				}
				// \a
				else if (s[i] == 'a') {
					t[j++] = '\a';
					st = 0;
				}
				// \b
				else if (s[i] == 'b') {
					t[j++] = '\b';
					st = 0;
				}
				// \f
				else if (s[i] == 'f') {
					t[j++] = '\f';
					st = 0;
				}
				// \n
				else if (s[i] == 'n') {
					t[j++] = '\n';
					st = 0;
				}
				// \r
				else if (s[i] == 'r') {
					t[j++] = '\r';
					st = 0;
				}
				// \s
				else if (s[i] == 's') {
					t[j++] = ' ';
					st = 0;
				}
				else if (s[i] == 't') {
					t[j++] = '\t';
					st = 0;
				}
				else if (s[i] == 'v') {
					t[j++] = '\v';
					st = 0;
				}
				else {
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 2) {
				if (s[i] >= '0' && s[i] <= '8') {
					t[j] = t[j] * 8 + s[i] - '0';
					st = 3;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 3) {
				if (s[i] >= '0' && s[i] <= '8') {
					t[j++] = t[j] * 8 + s[i] - '0';
					st = 0;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 4) {
				uint32_t x;
				if (find_wchar_is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(x);
					st = 5;
				}
				else if (s[i] == '\\') {
					st = 1;
				}
				else {
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 5) {
				uint32_t x;
				if (find_wchar_is_hex(s[i], x)) {
					t[j++] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 0;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 6) {
				uint32_t x;
				if (find_wchar_is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(x);
					st = 7;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 7) {
				uint32_t x;
				if (find_wchar_is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 8;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 8) {
				uint32_t x;
				if (find_wchar_is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 9;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 9) {
				uint32_t x;
				if (find_wchar_is_hex(s[i], x)) {
					t[j++] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 0;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
		}
		t[j] = '\0';
		return t;
	}

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

	// �}�`���X�g����, ����܂ŕ����͈͂��I������Ă����}�`, �V���ɑI�������}�`�𓾂�.
	// slist	�}�`���X�g
	// f_text	����������
	// f_len	����������̕�����
	// f_case	�p��������ʂ���
	// f_wrap	��荞�݌�������
	// t	����܂ŕ����͈͂��I������Ă����}�`
	// s_found	�V���ɑI�������}�`
	// s_range	�V���ɑI������镶���͈�
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
		uint32_t t_pos = 0;	// �����񒆂̈ʒu
		while (!stack.empty()) {
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
				if (!s->get_text_selected(t_range)) {
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
		if (!f_wrap && t != nullptr) {
			return false;
		}
		stack.push_back(slist.begin());
		stack.push_back(slist.end());
		while (!stack.empty()) {
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

	// �����͈͂��I������Ă���}�`��������.
	// it_beg	�ŏ��̗񋓎q
	// it_end	�Ō�̗񋓎q (�̎��j
	// t_range	�������������͈�
	// �߂�l	��������������}�`
	static ShapeText* find_text_range_selected(SHAPE_LIST::iterator const& it_beg, SHAPE_LIST::iterator const& it_end, DWRITE_TEXT_RANGE& t_range)
	{
		std::list<SHAPE_LIST::iterator> stack;
		stack.push_back(it_beg);
		stack.push_back(it_end);
		while (!stack.empty()) {
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
				if (!s->get_text_selected(t_range)) {
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
	/*
	IAsyncAction MainPage::edit_text_async(ShapeText* s)
	{
		static winrt::event_token primary_token;
		static winrt::event_token closed_token;

		tx_edit_text().Text(s->m_text == nullptr ? L"" : s->m_text);
		tx_edit_text().SelectAll();
		ck_fit_text_frame().IsChecked(m_fit_text_frame);
		if (co_await cd_edit_text_dialog().ShowAsync() == ContentDialogResult::Primary) {
			auto text = wchar_cpy(tx_edit_text().Text().c_str());
			undo_push_set<UNDO_T::TEXT_CONTENT>(s, text);
			m_fit_text_frame = ck_fit_text_frame().IsChecked().GetBoolean();
			if (m_fit_text_frame) {
				undo_push_position(s, LOC_TYPE::LOC_SE);
				s->fit_frame_to_text(m_main_page.m_snap_grid ? m_main_page.m_grid_base + 1.0f : 0.0f);
			}
			undo_push_null();
			xcvd_menu_is_enabled();
			main_draw();
		}
	}
	*/

	// �����񌟍��p�l���́u���ׂĒu���v�{�^���������ꂽ.
	void MainPage::find_replace_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();
		// ����������̕������𓾂�.
		const uint32_t f_len = wchar_len(m_find_text);
		if (f_len == 0) {
			// �������� 0 �̏ꍇ,
			return;
		}

		// ���炩���ߌ�����������܂ޕ�����}�`�����邩���肷��.
		bool done = false;
		std::list <SHAPE_LIST::iterator> it_stack;
		it_stack.push_back(m_main_page.m_shape_list.begin());
		it_stack.push_back(m_main_page.m_shape_list.end());
		while (!it_stack.empty()) {
			SHAPE_LIST::iterator j = it_stack.back();
			it_stack.pop_back();
			SHAPE_LIST::iterator i = it_stack.back();
			it_stack.pop_back();
			while (i != j) {
				auto s = *i++;
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeGroup)) {
					it_stack.push_back(i);
					it_stack.push_back(j);
					ShapeGroup* g = static_cast<ShapeGroup*>(s);
					i = g->m_list_grouped.begin();
					j = g->m_list_grouped.end();
					continue;
				}
				wchar_t* w_text;
				if (!s->get_text_content(w_text)) {
					continue;
				}
				uint32_t f_pos = 0;
				if (find_text(w_text, wchar_len(w_text), m_find_text, f_len, m_find_text_case, f_pos)) {
					if (!done) {
						done = true;
					}
					break;
				}
			}
		}
		it_stack.clear();
		if (!done) {
			// �}�`���Ȃ��ꍇ,
			// �u������͌�����܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
			return;
		}
		// �����͈͂̑I���݂̂���������.
		unselect_all(true);

		const uint32_t r_len = wchar_len(m_find_repl);
		it_stack.push_back(m_main_page.m_shape_list.begin());
		it_stack.push_back(m_main_page.m_shape_list.end());
		while (!it_stack.empty()) {
			SHAPE_LIST::iterator j = it_stack.back();
			it_stack.pop_back();
			SHAPE_LIST::iterator i = it_stack.back();
			it_stack.pop_back();
			while (i != j) {
				Shape* s = *i++;
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeGroup)) {
					it_stack.push_back(i);
					it_stack.push_back(j);
					ShapeGroup* g = static_cast<ShapeGroup*>(s);
					i = g->m_list_grouped.begin();
					j = g->m_list_grouped.end();
					continue;
				}
				else if (typeid(*s) != typeid(ShapeText)) {
					continue;
				}
				ShapeText* t = static_cast<ShapeText*>(s); // ��������镶����}�`
				wchar_t* w_text = wchar_cpy(t->m_text);	// ��������镶����
				uint32_t w_len = wchar_len(w_text);// ��������镶����̕�����
				uint32_t w_pos = 0;	// ��������镶���񒆂̈ʒu
				uint32_t f_pos = 0;
				bool done2 = false;	// ��v���������肷��.
				while (find_text(w_text + w_pos, w_len - w_pos, m_find_text, f_len, m_find_text_case, f_pos)) {
					w_pos += f_pos;
					wchar_t* t_text = find_replace(w_text, w_pos, f_len, m_find_repl, r_len);
					delete[] w_text;
					w_text = t_text;
					w_len += r_len - f_len;
					w_pos += r_len;
					if (!done2) {
						done2 = true;
					}
				}
				if (done2) {
					undo_push_set<UNDO_T::TEXT_CONTENT>(t, w_text);
				}
			}
		}
		undo_push_null();
		undo_menu_is_enabled();
		main_draw();
		status_bar_set_pos();
	}

	// �����񌟍��p�l���́u�u�����Ď��Ɂv�{�^���������ꂽ.
	void MainPage::find_replace_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();
		const auto f_len = wchar_len(m_find_text);
		if (f_len == 0) {
			return;
		}

		bool flag = false;	// ��v�܂��͒u������������.
		DWRITE_TEXT_RANGE t_range;	// �����͈�
		auto t = find_text_range_selected(m_main_page.m_shape_list.begin(), m_main_page.m_shape_list.end(), t_range);
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
				undo_push_set<UNDO_T::TEXT_CONTENT>(t, r_text);
				undo_push_set<UNDO_T::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ w_pos, r_len });
				undo_push_null();
				undo_menu_is_enabled();
			}
		}
		// ������������.
		Shape* s;
		DWRITE_TEXT_RANGE s_range;
		if (find_text(m_main_page.m_shape_list, m_find_text, m_find_text_case, m_find_text_wrap, t, s, s_range)) {
			// �����ł����Ȃ��,
			// �����͈͂��I�����ꂽ�}�`������, ���ꂪ���̐}�`�ƈقȂ邩���肷��.
			if (t != nullptr && s != t) {
				undo_push_set<UNDO_T::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ 0, 0 });
			}
			undo_push_set<UNDO_T::TEXT_RANGE>(s, s_range);
			scroll_to(s);
			flag = true;
		}
		if (flag) {
			main_draw();
		}
		else {
			// �����ł��Ȃ�, ���u��������ĂȂ��ꍇ,
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
		}
		status_bar_set_pos();
	}

	// �ҏW���j���[�́u������̌���/�u���v���I�����ꂽ.
	void MainPage::find_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �����񌟍��p�l�����\������Ă��邩���肷��.
		if (sp_find_text_panel().Visibility() == Visibility::Visible) {
			sp_find_text_panel().Visibility(Visibility::Collapsed);
			find_text_preserve();
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
			sp_find_text_panel().Visibility(Visibility::Visible);
		}
		status_bar_set_pos();
	}

	// �����񌟍��p�l���́u����v�{�^���������ꂽ.
	void MainPage::find_text_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();
		sp_find_text_panel().Visibility(Visibility::Collapsed);
		status_bar_set_pos();
	}

	// �����񌟍��p�l���́u���������v�{�^���������ꂽ.
	void MainPage::find_text_next_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();
		ShapeText* t;
		Shape* s;
		DWRITE_TEXT_RANGE s_range;
		if (find_text(m_main_page.m_shape_list, m_find_text, m_find_text_case, m_find_text_wrap, t, s, s_range)) {
			if (t != nullptr && s != t) {
				undo_push_set<UNDO_T::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ 0, 0 });
			}
			undo_push_set<UNDO_T::TEXT_RANGE>(s, s_range);
			scroll_to(s);
			main_draw();
		}
		else {
			// �����ł��Ȃ��ꍇ,
			// �u������͌�����܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
		}
		status_bar_set_pos();
	}

	// �����񌟍��p�l���̒l��ۑ�����.
	void MainPage::find_text_preserve(void)
	{
		if (m_find_text != nullptr) {
			delete[] m_find_text;
		}
		m_find_text = find_wchar_cpy(tx_find_text_what().Text().c_str());
		if (m_find_repl != nullptr) {
			delete[] m_find_repl;
		}
		m_find_repl = find_wchar_cpy(tx_find_replace_with().Text().c_str());
		m_find_text_case = ck_find_text_case().IsChecked().GetBoolean();
		m_find_text_wrap = ck_find_text_wrap().IsChecked().GetBoolean();
	}

	// ���������񂪕ύX���ꂽ.
	void MainPage::find_text_what_changed(IInspectable const&, TextChangedEventArgs const&)
	{
		const auto not_empty = !tx_find_text_what().Text().empty();
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
	//	if (m_fit_text_frame) {
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