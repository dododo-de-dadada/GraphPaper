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
	using winrt::Windows::UI::Xaml::Visibility;

	constexpr wchar_t NOT_FOUND[] = L"str_info_not_found";	// �u�����񂪌�����܂���v���b�Z�[�W�̃��\�[�X�L�[

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

	// �����񌟍��p�l���́u���ׂĒu���v�{�^���������ꂽ.
	void MainPage::replace_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();

		// ���������񂪋�Ȃ璆�f����.
		const auto find_len = wchar_len(m_find_text);
		if (find_len == 0) {
			return;
		}
		undo_push_null();
		if (find_next()) {
			do {
				replace_text();
			} while (find_next());
		}
		else {
			while (m_undo_stack.size() > 0) {
				auto u = m_undo_stack.back();
				if (u != nullptr && typeid(*u) != typeid(UndoTextSelect)) {
					break;
				}
				m_undo_stack.pop_back();
				if (u == nullptr) {
					break;
				}
				u->exec();
				delete u;
			}
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
			status_bar_set_pointer();
		}
		main_sheet_draw();
	}

	// ������̑I��͈͂�����Βu������.
	bool MainPage::replace_text(void)
	{
		// �ҏW����}�`������.
		if (m_core_text_focused != nullptr) {
			//�I�����ꂽ�����͈͂�����.
			const auto len = m_core_text_focused->get_text_len();
			const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, len);
			const auto s = min(m_main_sheet.m_select_start, end);
			const auto e = max(m_main_sheet.m_select_start, end);
			if (s < e) {
				if (!m_core_text_focused->is_selected()) {
					undo_push_toggle(m_core_text_focused);
				}
				m_undo_stack.push_back(new UndoText2(m_core_text_focused, m_repl_text));
				const auto repl_len = wchar_len(m_repl_text);
				undo_push_text_select(m_core_text_focused, s + repl_len, s + repl_len, false);
				m_core_text_focused->create_text_layout();	// DWRITE �����񃌃C�A�E�g���X�V����K�v������.
				return true;
			}
		}
		return false;
	}

	// �����񌟍��p�l���́u�u�����Ď��Ɂv�{�^���������ꂽ.
	void MainPage::replace_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();

		// ���������񂪋�, �܂��͌���������ƒu�������񂪓����Ȃ璆�f����.
		const auto find_len = wchar_len(m_find_text);
		if (find_len == 0 || wcscmp(m_find_text, m_repl_text == nullptr ? L"" : m_repl_text) == 0) {
			return;
		}

		// �I��͈͂�����Βu������.
		if (m_core_text_focused != nullptr) {
			const auto end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
			if (m_main_sheet.m_select_start != end) {
				undo_push_null();
				replace_text();
			}
		}

		// ���̕��������������.
		const bool found = find_next();


		if (found) {
			scroll_to(m_core_text_focused);
			menu_is_enable();
			main_sheet_draw();
		}
		else {
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
			status_bar_set_pointer();
		}
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
			//bool exist_text = false;
			//for (const auto s : m_main_sheet.m_shape_list) {
			//	if (s->is_deleted()) {
			//		continue;
			//	}
			//	else if (typeid(*s) == typeid(ShapeText)) {
			//		exist_text = true;
			//		break;
			//	}
			//}
			//if (exist_text) {
			//	return;
			//}

			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_close_click(nullptr, nullptr);
			}
			tx_find_text_what().Text({ m_find_text == nullptr ? L"" : m_find_text });
			tx_find_replace_with().Text({ m_repl_text == nullptr ? L"" : m_repl_text });
			ck_find_text_case().IsChecked(m_find_text_case);
			ck_find_text_wrap().IsChecked(m_find_text_wrap);
			sp_find_text_panel().Visibility(Visibility::Visible);
		}
		status_bar_set_pointer();
	}

	// �����񌟍��p�l���́u����v�{�^���������ꂽ.
	void MainPage::find_text_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();
		sp_find_text_panel().Visibility(Visibility::Collapsed);
		status_bar_set_pointer();
	}

	// ���������������.
	bool MainPage::find_next(void)
	{
		// ���͒��̕����񂪂����,
		if (m_core_text_focused != nullptr && m_core_text_comp) {
			m_core_text.NotifyFocusLeave();
		}

		const auto find_len = wchar_len(m_find_text);
		// �Ō�ʂ���őO�ʂ̊e������}�`�ɂ���.
		for (auto it = m_main_sheet.m_shape_list.begin(); it != m_main_sheet.m_shape_list.end(); it++) {
			// �ҏW���̐}�`������Ȃ炻�̐}�`. �Ȃ���΂ł��邾���w�ʂɂ��镶����}�` s �𓾂�.
			if ((*it)->is_deleted() || typeid(*(*it)) != typeid(ShapeText)) {
				continue;
			}
			if (m_core_text_focused != nullptr && m_core_text_focused != *it) {
				continue;
			}
			Shape* s = *it;
			// �}�` s �� s ���O�ʂɂ���e������}�` t �ɂ���
			for (; it != m_main_sheet.m_shape_list.end(); it++) {
				if ((*it)->is_deleted() || typeid(*(*it)) != typeid(ShapeText)) {
					continue;
				}
				// s �� t �łȂ���΃L�����b�g�ʒu�͐擪�ɂ���.
				ShapeText* t = static_cast<ShapeText*>(*it);
				if (s != t) {
					undo_push_text_select(t, 0, 0, false);
				}
				// �}�`�̃L�����b�g��肤����Ɉ�v���镶���񂪂��邩���ׂ�.
				const auto text_len = t->get_text_len();
				const auto end = min(m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end, text_len);
				for (uint32_t i = end; i + find_len <= text_len; i++) {
					const int cmp = m_find_text_case ? wcsncmp(t->m_text + i, m_find_text, find_len) : _wcsnicmp(t->m_text + i, m_find_text, find_len);
					if (cmp == 0) {
						m_core_text_focused = static_cast<ShapeText*>(*it);
						select_shape(m_core_text_focused);
						//unselect_all_shape();
						//undo_push_toggle(m_core_text_focused);
						undo_push_text_select(m_core_text_focused, i, i + find_len, false);
						return true;
					}
				}
			}
			// ��荞�݌����łȂ���Β��f����.
			if (!m_find_text_wrap) {
				break;
			}
			// �Ō�ʂ̐}�`�ɖ߂���, �������n�߂��}�` s �̒��O�܂ł̊e�}�`�ɂ���,
			for (it = m_main_sheet.m_shape_list.begin(); it != m_main_sheet.m_shape_list.end() && *it != s; it++) {
				if ((*it)->is_deleted() || typeid(*(*it)) != typeid(ShapeText)) {
					continue;
				}
				// �}�`�̕�����̐擪�����v���镶���񂪂��邩���ׂ�.
				const ShapeText* t = static_cast<ShapeText*>(*it);
				const auto text_len = t->get_text_len();
				for (uint32_t i = 0; i + find_len <= text_len; i++) {
					// ��v���镶���񂪂����,
					const int cmp = m_find_text_case ?
						wcsncmp(t->m_text + i, m_find_text, find_len) : _wcsnicmp(t->m_text + i, m_find_text, find_len);
					if (cmp == 0) {
						m_core_text_focused = static_cast<ShapeText*>(*it);
						select_shape(m_core_text_focused);
						//unselect_all_shape();
						//undo_push_toggle(m_core_text_focused);
						undo_push_text_select(m_core_text_focused, i, i + find_len, false);
						return true;
					}
				}
			}
			// �������n�߂��}�` s �ɂ���, �}�` s �̃L�����b�g���O�Ɉ�v���镶���񂪂��邩���ׂ�.
			const ShapeText* t = static_cast<const ShapeText*>(s);
			const auto end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
			const auto e = min(m_main_sheet.m_select_start, end);
			for (uint32_t i = 0; i + find_len <= e; i++) {
				const int cmp = m_find_text_case ?
					wcsncmp(t->m_text + i, m_find_text, find_len) :
					_wcsnicmp(t->m_text + i, m_find_text, find_len);
				if (cmp == 0) {
					m_core_text_focused = static_cast<ShapeText*>(*it);
					select_shape(m_core_text_focused);
					//unselect_all_shape();
					//undo_push_toggle(m_core_text_focused);
					undo_push_text_select(m_core_text_focused, i, i + find_len, false);
					return true;
				}
			}
			break;
		}
		return false;
	}

	// �����񌟍��p�l���́u���������v�{�^���������ꂽ.
	void MainPage::find_next_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();
		const auto find_len = wchar_len(m_find_text);
		if (find_len == 0) {
			return;
		}
		const bool found = find_next();
		if (found) {
			scroll_to(m_core_text_focused);
		}
		main_sheet_draw();
		if (!found) {
			// �u������͌�����܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
			status_bar_set_pointer();
		}
	}

	// �����񌟍��p�l���̒l��ۑ�����.
	void MainPage::find_text_preserve(void)
	{
		if (m_find_text != nullptr) {
			delete[] m_find_text;
		}
		m_find_text = find_wchar_cpy(tx_find_text_what().Text().c_str());
		if (m_repl_text != nullptr) {
			delete[] m_repl_text;
		}
		m_repl_text = find_wchar_cpy(tx_find_replace_with().Text().c_str());
		m_find_text_case = ck_find_text_case().IsChecked().GetBoolean();
		m_find_text_wrap = ck_find_text_wrap().IsChecked().GetBoolean();
	}

	// ���������񂪕ύX���ꂽ.
	void MainPage::find_text_what_changed(IInspectable const&, TextChangedEventArgs const&)
	{
		const auto not_empty = !tx_find_text_what().Text().empty();
		btn_find_next().IsEnabled(not_empty);
		btn_find_replace().IsEnabled(not_empty);
		btn_find_replace_all().IsEnabled(not_empty);
	}
}