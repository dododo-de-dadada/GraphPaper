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

		// 得られた図形のキャレットを先頭に戻して検索する.
		undo_push_null();
		if (m_edit_text_shape != nullptr) {
			undo_push_text_select(m_edit_text_shape, 0, 0, false);
			m_edit_text_shape = nullptr;
		}
		if (find_next()) {
			do {
				replace_text();
			} while (find_next());
		}
		else {
			while (m_ustack_undo.size() > 0) {
				auto u = m_ustack_undo.back();
				if (u != nullptr && typeid(*u) != typeid(UndoTextSelect)) {
					break;
				}
				m_ustack_undo.pop_back();
				if (u == nullptr) {
					break;
				}
				u->exec();
				delete u;
			}
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
			status_bar_set_pos();
		}
		undo_menu_is_enabled();
		xcvd_menu_is_enabled();
		main_draw();
	}

	// 文字列の選択範囲があれば置換し, 次を検索する.
	// 選択範囲がなければ置換せず, 次を検索する.
	// 一致したなら true, そうでなければ false を返す.
	/*
	bool MainPage::replace_text(void)
	{
		// 編集する図形がある.
		if (m_edit_text_shape != nullptr) {
			//選択された文字範囲がある.
			const ShapePage* t = &m_main_page;
			const auto end = t->m_select_trail ? t->m_select_end + 1 : t->m_select_end;
			const auto s = min(t->m_select_start, end);
			const auto e = max(t->m_select_start, end);
			if (s < e) {
				// 置換して, 置換後の文字範囲を選択
				undo_push_select(m_edit_text_shape);
				m_ustack_undo.push_back(new UndoText2(m_edit_text_shape, m_find_repl));
				const auto repl_len = wchar_len(m_find_repl);
				undo_push_text_select(m_edit_text_shape, s + repl_len, s + repl_len, false);
				m_edit_text_shape->create_text_layout();	// DWRITE 文字列レイアウトを更新する必要もある.
			}
		}
		// 次の文字列を検索する.
		return find_next();
	}
	*/

	// 文字列の選択範囲があれば置換する.
	bool MainPage::replace_text(void)
	{
		// 編集する図形がある.
		if (m_edit_text_shape != nullptr) {
			//選択された文字範囲がある.
			const auto len = m_edit_text_shape->get_text_len();
			const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
			const auto s = min(m_main_page.m_select_start, end);
			const auto e = max(m_main_page.m_select_start, end);
			if (s < e) {
				if (!m_edit_text_shape->is_selected()) {
					undo_push_select(m_edit_text_shape);
				}
				m_ustack_undo.push_back(new UndoText2(m_edit_text_shape, m_find_repl));
				const auto repl_len = wchar_len(m_find_repl);
				undo_push_text_select(m_edit_text_shape, s + repl_len, s + repl_len, false);
				m_edit_text_shape->create_text_layout();	// DWRITE 文字列レイアウトを更新する必要もある.
				return true;
			}
		}
		return false;
	}

	// 文字列検索パネルの「置換して次に」ボタンが押された.
	void MainPage::replace_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();
		const auto find_len = wchar_len(m_find_text);
		if (find_len == 0 || wcscmp(m_find_text, m_find_repl) == 0) {
			return;
		}

		// 選択範囲があれば置換する.
		if (m_edit_text_shape != nullptr) {
			const auto end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
			if (m_main_page.m_select_start != end) {
				undo_push_null();
				replace_text();
				undo_menu_is_enabled();
			}
		}
		// 次の文字列を検索する.
		const bool found = find_next();
		if (found) {
			scroll_to(m_edit_text_shape);
		}
		xcvd_menu_is_enabled();
		main_draw();
		if (!found) {
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
			status_bar_set_pos();
		}
	}

	// 編集メニューの「文字列の検索/置換」が選択された.
	void MainPage::find_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 文字列検索パネルが表示されているか判定する.
		if (sp_find_text_panel().Visibility() == Visibility::Visible) {
			sp_find_text_panel().Visibility(Visibility::Collapsed);
			find_text_preserve();
		}
		else {
			//if (m_edit_text_shape != nullptr) {
			//	m_edit_context.NotifyFocusLeave();
			//	undo_push_text_unselect(m_edit_text_shape);
			//	m_edit_text_shape = nullptr;
			//	xcvd_menu_is_enabled();
			//}
			if (m_edit_text_shape != nullptr) {
				m_edit_context.NotifyFocusLeave();
			}
			// 一覧が表示されてるか判定する.
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

	// 文字列検索パネルの「閉じる」ボタンが押された.
	void MainPage::find_text_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();
		sp_find_text_panel().Visibility(Visibility::Collapsed);
		status_bar_set_pos();
	}

	// 文字列を検索する.
	bool MainPage::find_next(void)
	{
		const auto find_len = wchar_len(m_find_text);
		// 最後面から最前面の各文字列図形について.
		for (auto it = m_main_page.m_shape_list.begin(); it != m_main_page.m_shape_list.end(); it++) {
			// 編集中の図形があるならその図形. なければできるだけ背面にある文字列図形 s を得る.
			if ((*it)->is_deleted() || typeid(*(*it)) != typeid(ShapeText)) {
				continue;
			}
			if (m_edit_text_shape != nullptr && m_edit_text_shape != *it) {
				continue;
			}
			Shape* s = *it;
			// 図形 s と s より前面にある各文字列図形 t について
			for (; it != m_main_page.m_shape_list.end(); it++) {
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
				const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, text_len);
				for (uint32_t i = end; i + find_len <= text_len; i++) {
					const int cmp = m_find_text_case ?
						wcsncmp(t->m_text + i, m_find_text, find_len) : _wcsnicmp(t->m_text + i, m_find_text, find_len);
					if (cmp == 0) {
						unselect_shape_all();
						m_edit_text_shape = static_cast<ShapeText*>(*it);
						undo_push_select(m_edit_text_shape);
						undo_push_text_select(m_edit_text_shape, i, i + find_len, false);
						return true;
					}
				}
			}
			// 回り込み検索でなければ中断する.
			if (!m_find_text_wrap) {
				break;
			}
			// 最後面の図形に戻って, 検索を始めた図形 s の直前までの各図形について,
			for (it = m_main_page.m_shape_list.begin(); it != m_main_page.m_shape_list.end() && *it != s; it++) {
				if ((*it)->is_deleted() || typeid(*(*it)) != typeid(ShapeText)) {
					continue;
				}
				// 図形の文字列の先頭から一致する文字列があるか調べる.
				const ShapeText* t = static_cast<ShapeText*>(*it);
				const auto text_len = t->get_text_len();
				for (uint32_t i = 0; i + find_len <= text_len; i++) {
					// 一致する文字列があれば,
					const int cmp = m_find_text_case ?
						wcsncmp(t->m_text + i, m_find_text, find_len) : _wcsnicmp(t->m_text + i, m_find_text, find_len);
					if (cmp == 0) {
						unselect_shape_all();
						m_edit_text_shape = static_cast<ShapeText*>(*it);
						undo_push_select(m_edit_text_shape);
						undo_push_text_select(m_edit_text_shape, i, i + find_len, false);
						return true;
					}
				}
			}
			// 検索を始めた図形 s について, 図形 s のキャレットより前に一致する文字列があるか調べる.
			const ShapeText* t = static_cast<const ShapeText*>(s);
			const auto end = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
			const auto e = min(m_main_page.m_select_start, end);
			for (uint32_t i = 0; i + find_len <= e; i++) {
				const int cmp = m_find_text_case ?
					wcsncmp(t->m_text + i, m_find_text, find_len) :
					_wcsnicmp(t->m_text + i, m_find_text, find_len);
				if (cmp == 0) {
					unselect_shape_all();
					m_edit_text_shape = static_cast<ShapeText*>(*it);
					undo_push_select(m_edit_text_shape);
					undo_push_text_select(m_edit_text_shape, i, i + find_len, false);
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
		const bool found = find_next();
		if (found) {
			scroll_to(m_edit_text_shape);
		}
		xcvd_menu_is_enabled();
		main_draw();
		if (!found) {
			// 「文字列は見つかりません」メッセージダイアログを表示する.
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
			status_bar_set_pos();
		}
	}

	// 文字列検索パネルの値を保存する.
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

	// 検索文字列が変更された.
	void MainPage::find_text_what_changed(IInspectable const&, TextChangedEventArgs const&)
	{
		const auto not_empty = !tx_find_text_what().Text().empty();
		btn_find_next().IsEnabled(not_empty);
		btn_find_replace().IsEnabled(not_empty);
		btn_find_replace_all().IsEnabled(not_empty);
	}
}