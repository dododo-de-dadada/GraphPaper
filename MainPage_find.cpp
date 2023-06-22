//-------------------------------
// MainPage_find.cpp
// 文字列の編集, または検索 / 置換
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Visibility;

	constexpr wchar_t NOT_FOUND[] = L"str_info_not_found";	// 「文字列が見つかりません」メッセージのリソースキー

	// 文字列を複製する. エスケープ文字列は文字コードに変換する.
	static wchar_t* find_wchar_cpy(const wchar_t* const s) noexcept;
	// 文字が '0'...'9' または 'A'...'F', 'a'...'f' か判定する.
	static bool find_wchar_is_hex(const wchar_t w, uint32_t& x) noexcept;

	// 文字が 0...9 または A...F, a...f か判定する
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

	// 文字列を複製する.
	// エスケープ文字列は文字コードに変換する.
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

	// 文字列検索パネルの「すべて置換」ボタンが押された.
	void MainPage::replace_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();

		// 検索文字列が空なら中断する.
		const auto find_len = wchar_len(m_find_text);
		if (find_len == 0) {
			return;
		}
		if (m_core_text_focused != nullptr) {
			m_core_text.NotifyFocusLeave();
		}

		// 折り返し検索を有効にする.
		const auto wrap_around = m_find_wrap_around;
		m_find_wrap_around = true;

		undo_push_null();
		bool found = false;
		if (find_next()) {
			do {
				replace_text();
			} while (find_next());
		}
		else {
			while (!m_undo_stack.empty() && m_undo_stack.back() == nullptr) {
				m_undo_stack.pop_back();
			}
			message_show(ICON_INFO, NOT_FOUND, find_what().Text());
			status_bar_set_pointer();
		}
		// 折り返し検索を元に戻す.
		m_find_wrap_around = wrap_around;

		// 見つからなければ, m_core_text_focused は最後に見つかった文字列.
		if (m_core_text_focused != nullptr) {
			m_core_text.NotifyFocusEnter();
		}
		main_sheet_draw();
	}

	// 文字列の選択範囲があれば置換する.
	bool MainPage::replace_text(void)
	{
		// 編集する図形がある.
		if (m_core_text_focused != nullptr) {
			//選択された文字範囲がある.
			const auto len = m_core_text_focused->get_text_len();
			const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
			const auto s = min(m_main_sheet.m_core_text_range.m_start, end);
			const auto e = max(m_main_sheet.m_core_text_range.m_start, end);
			if (s < e) {
				if (!m_core_text_focused->is_selected()) {
					undo_push_toggle(m_core_text_focused);
				}
				m_undo_stack.push_back(new UndoText2(m_core_text_focused, m_repl_text));
				const auto repl_len = wchar_len(m_repl_text);
				undo_push_text_select(m_core_text_focused, s + repl_len, s + repl_len, false);
				m_core_text_focused->create_text_layout();
				return true;
			}
		}
		return false;
	}

	// 文字列検索パネルの「置換して次に」ボタンが押された.
	void MainPage::replace_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();

		// 検索文字列が空, または検索文字列と置換文字列が同じなら中断する.
		const auto find_len = wchar_len(m_find_text);
		if (find_len == 0 || wcscmp(m_find_text, m_repl_text == nullptr ? L"" : m_repl_text) == 0) {
			return;
		}

		auto focused = m_core_text_focused;
		if (focused != nullptr) {
			m_core_text.NotifyFocusLeave();
		}

		// 選択範囲があれば置換する.
		undo_push_null();
		bool replaced = false;
		if (m_core_text_focused != nullptr) {
			const auto end = m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end;
			if (m_main_sheet.m_core_text_range.m_start != end) {
				replaced = replace_text();
			}
		}

		// 次の文字列を検索する.
		const bool found = find_next();
		if (!found) {
			message_show(ICON_INFO, NOT_FOUND, find_what().Text());
			status_bar_set_pointer();
		}
		if (m_core_text_focused != nullptr) {
			m_core_text.NotifyFocusEnter();
		}
		if (found || replaced) {
			scroll_to(m_core_text_focused);
			menu_is_enable();
			main_sheet_draw();
		}
	}

	// 編集メニューの「文字列の検索/置換」が選択された.
	void MainPage::find_and_replace_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 文字列検索パネルが表示されているか判定する.
		if (find_and_replace_panel().Visibility() == Visibility::Visible) {
			find_and_replace_panel().Visibility(Visibility::Collapsed);
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

			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_close_click(nullptr, nullptr);
			}
			find_what().Text({ m_find_text == nullptr ? L"" : m_find_text });
			replace_with().Text({ m_repl_text == nullptr ? L"" : m_repl_text });
			find_case_sensitive().IsChecked(m_find_case_sensitive);
			find_wrap_around().IsChecked(m_find_wrap_around);
			find_and_replace_panel().Visibility(Visibility::Visible);
		}
		status_bar_set_pointer();
	}

	// 文字列検索パネルの「閉じる」ボタンが押された.
	void MainPage::find_and_replace_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();
		find_and_replace_panel().Visibility(Visibility::Collapsed);
		status_bar_set_pointer();
	}

	// 文字列を検索する.
	// 見つかったら入力中の文字列に格納される.
	bool MainPage::find_next(void)
	{
		//ShapeText* focused = m_core_text_focused;
		const auto find_len = wchar_len(m_find_text);
		// 最後面から最前面の各文字列図形について.
		for (auto it = m_main_sheet.m_shape_list.begin(); it != m_main_sheet.m_shape_list.end(); it++) {
			// 編集中の図形があるならその図形. なければできるだけ背面にある文字列図形 s を得る.
			if ((*it)->is_deleted() || typeid(*(*it)) != typeid(ShapeText)) {
				continue;
			}
			if (m_core_text_focused != nullptr && m_core_text_focused != *it) {
				continue;
			}
			Shape* s = *it;
			// 図形 s と s より前面にある各文字列図形 t について
			for (; it != m_main_sheet.m_shape_list.end(); it++) {
				if ((*it)->is_deleted() || typeid(*(*it)) != typeid(ShapeText)) {
					continue;
				}
				// s が t でなければキャレット位置は先頭にする.
				ShapeText* t = static_cast<ShapeText*>(*it);
				if (s != t) {
					undo_push_text_select(t, 0, 0, false);
				}
				// 図形のキャレットよりうしろに一致する文字列があるか調べる.
				const auto text_len = t->get_text_len();
				const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, text_len);
				for (uint32_t i = end; i + find_len <= text_len; i++) {
					const int cmp = m_find_case_sensitive ? wcsncmp(t->m_text + i, m_find_text, find_len) : _wcsnicmp(t->m_text + i, m_find_text, find_len);
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
			// 回り込み検索でなければ中断する.
			if (!m_find_wrap_around) {
				break;
			}
			// 最後面の図形に戻って, 検索を始めた図形 s の直前までの各図形について,
			for (it = m_main_sheet.m_shape_list.begin(); it != m_main_sheet.m_shape_list.end() && *it != s; it++) {
				if ((*it)->is_deleted() || typeid(*(*it)) != typeid(ShapeText)) {
					continue;
				}
				// 図形の文字列の先頭から一致する文字列があるか調べる.
				const ShapeText* t = static_cast<ShapeText*>(*it);
				const auto text_len = t->get_text_len();
				for (uint32_t i = 0; i + find_len <= text_len; i++) {
					// 一致する文字列があれば,
					const int cmp = m_find_case_sensitive ?
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
			// 検索を始めた図形 s について, 図形 s のキャレットより前に一致する文字列があるか調べる.
			const ShapeText* t = static_cast<const ShapeText*>(s);
			const auto end = m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end;
			const auto e = min(m_main_sheet.m_core_text_range.m_start, end);
			for (uint32_t i = 0; i + find_len <= e; i++) {
				const int cmp = m_find_case_sensitive ?
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

	// 文字列検索パネルの「次を検索」ボタンが押された.
	void MainPage::find_next_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();
		const auto find_len = wchar_len(m_find_text);
		if (find_len == 0) {
			return;
		}
		// 入力中の文字列があれば,
		if (m_core_text_focused != nullptr) {
			m_core_text.NotifyFocusLeave();
		}
		const bool found = find_next();
		if (found) {
			scroll_to(m_core_text_focused);
			if (m_core_text_focused != nullptr) {
				m_core_text.NotifyFocusEnter();
			}
		}
		main_sheet_draw();
		if (!found) {
			// 「文字列は見つかりません」メッセージダイアログを表示する.
			message_show(ICON_INFO, NOT_FOUND, find_what().Text());
			status_bar_set_pointer();
		}
	}

	// 文字列検索パネルの値を保存する.
	void MainPage::find_text_preserve(void)
	{
		if (m_find_text != nullptr) {
			delete[] m_find_text;
		}
		m_find_text = find_wchar_cpy(find_what().Text().c_str());

		if (m_repl_text != nullptr) {
			delete[] m_repl_text;
		}
		m_repl_text = find_wchar_cpy(replace_with().Text().c_str());

		m_find_case_sensitive = find_case_sensitive().IsChecked().GetBoolean();
		m_find_wrap_around = find_wrap_around().IsChecked().GetBoolean();
		m_find_use_escseq = find_use_escseq().IsChecked().GetBoolean();
	}

	// 検索文字列が変更された.
	void MainPage::find_text_what_changed(IInspectable const&, TextChangedEventArgs const&)
	{
		const auto not_empty = !find_what().Text().empty();
		btn_find_next().IsEnabled(not_empty);
		btn_find_replace().IsEnabled(not_empty);
		btn_find_replace_all().IsEnabled(not_empty);
	}
}