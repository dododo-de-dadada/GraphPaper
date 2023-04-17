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

	// 文字列の一部を置換する.
	static wchar_t* find_replace(wchar_t const* w_text, const uint32_t w_pos, const uint32_t w_len, wchar_t const* r_text, const uint32_t r_len) noexcept;
	// 文字列を検索して見つかった位置を得る.
	static bool find_text(const wchar_t* w_text, const uint32_t w_len, const wchar_t* f_text, const uint32_t f_len, const bool f_case, uint32_t& f_pos) noexcept;
	// 図形リストから, それまで文字範囲が選択されていた図形, 新たに選択される図形を得る.
	static bool find_text(SHAPE_LIST& slist, wchar_t* f_text, const bool f_case, const bool f_wrap, ShapeText*& t, Shape*& s, DWRITE_TEXT_RANGE& s_range);
	// 文字範囲が選択されている図形を見つける.
	static ShapeText* find_text_range_selected(SHAPE_LIST::iterator const& it_beg, SHAPE_LIST::iterator const& it_end, DWRITE_TEXT_RANGE& t_range);
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

	// 文字列の一部を置換する.
	// w_text	置換される前の文字列
	// f_pos	置換される文字列の開始位置
	// f_len	置換される文字数
	// r_text	置換文字列
	// r_len	置換文字列の文字数
	// 戻り値	置換された文字列
	static wchar_t* find_replace(wchar_t const* w_text, const uint32_t f_pos, const uint32_t f_len, wchar_t const* r_text, const uint32_t r_len) noexcept
	{
		if (w_text != nullptr) {
			// 置換される前の文字列の文字数を得る.
			const uint32_t w_len = wchar_len(w_text);
			// 置換される文字列の終了位置を得る.
			const uint32_t f_end = f_pos + f_len;
			if (f_end <= w_len) {
				// 終了位置が文字数以下の場合,
				// 置換された後の文字列の終了位置を得る.
				const size_t r_end = static_cast<size_t>(f_pos) + static_cast<size_t>(r_len);
				// 置換された後の文字列の文字数を得る.
				const size_t n_len = static_cast<size_t>(w_len) + static_cast<size_t>(r_len) - static_cast<size_t>(f_len);
				// 置換された後の文字列を格納する配列を確保する.
				wchar_t* const n_text = new wchar_t[n_len + 1];
				// 開始位置までの文字列を配列に格納する.
				wcsncpy_s(n_text, n_len + 1, w_text, static_cast<size_t>(f_pos));
				// 置換文字列を配列に格納する.
				wcsncpy_s(n_text + f_pos, n_len - f_pos + 1, r_text, r_len);
				// 終了位置から後の文字列を配列に格納する.
				wcsncpy_s(n_text + r_end, n_len - r_end + 1, w_text + f_end, w_len - f_end);
				// 置換された文字列を返す.
				return n_text;
			}
		}
		return wchar_cpy(r_text);
	}

	// 文字列を検索して見つかった位置を得る.
	// w_text	検索される文字列
	// w_len	検索される文字列の文字数
	// w_pos	検索を開始する位置
	// f_text	検索文字列
	// f_len	検索文字列の文字数
	// f_case	英文字を区別する
	// f_break	改行を無視する
	// f_pos	見つかった位置
	static bool find_text(const wchar_t* w_text, const uint32_t w_len, const wchar_t* f_text, const uint32_t f_len, const bool f_case, uint32_t& f_pos) noexcept
	{
		// 検索される文字列が非ヌル, かつ検索文字列が非ヌル, 
		// かつ検索文字列の文字数がゼロより大きく, 検索される文字列の文字数以下か判定する.
		if (w_text != nullptr && f_text != nullptr && f_len > 0 && f_len <= w_len) {
			// 英文字を区別するか判定する.
			if (f_case) {
				// 区別するなら,
				for (uint32_t i = 0; i <= w_len - f_len; i++) {
					if (wcsncmp(w_text + i, f_text, f_len) == 0) {
						f_pos = i;
						return true;
					}
				}
			}
			else {
				// 区別しないなら,
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

	// 図形リストから, それまで文字範囲が選択されていた図形, 新たに選択される図形を得る.
	// slist	図形リスト
	// f_text	検索文字列
	// f_len	検索文字列の文字数
	// f_case	英文字を区別する
	// f_wrap	回り込み検索する
	// t	それまで文字範囲が選択されていた図形
	// s_found	新たに選択される図形
	// s_range	新たに選択される文字範囲
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
		uint32_t t_pos = 0;	// 文字列中の位置
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
					// 文字範囲が選択された図形の, 文字範囲より後ろの文字列を検索する.
					t = static_cast<ShapeText*>(s);
					t_pos = t_range.startPosition;
					const auto t_end = t_pos + t_range.length;
					uint32_t f_pos;
					if (find_text(t->m_text + t_end, wchar_len(t->m_text) - t_end, f_text, f_len, f_case, f_pos)) {
						// 文字範囲より後ろの文字列の中で見つかった場合
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
		// 回り込み検索がない, かつ文字範囲が選択された図形が見つかったか判定する.
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

	// 文字範囲が選択されている図形を見つける.
	// it_beg	最初の列挙子
	// it_end	最後の列挙子 (の次）
	// t_range	見つかった文字範囲
	// 戻り値	見つかった文字列図形
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

	// 図形が持つ文字列を編集する.
	// s	文字列図形
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

	// 文字列検索パネルの「すべて置換」ボタンが押された.
	void MainPage::find_replace_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();
		// 検索文字列の文字数を得る.
		const uint32_t f_len = wchar_len(m_find_text);
		if (f_len == 0) {
			// 文字数が 0 の場合,
			return;
		}

		// あらかじめ検索文字列を含む文字列図形があるか判定する.
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
			// 図形がない場合,
			// 「文字列は見つかりません」メッセージダイアログを表示する.
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
			return;
		}
		// 文字範囲の選択のみを解除する.
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
				ShapeText* t = static_cast<ShapeText*>(s); // 検索される文字列図形
				wchar_t* w_text = wchar_cpy(t->m_text);	// 検索される文字列
				uint32_t w_len = wchar_len(w_text);// 検索される文字列の文字数
				uint32_t w_pos = 0;	// 検索される文字列中の位置
				uint32_t f_pos = 0;
				bool done2 = false;	// 一致したか判定する.
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

	// 文字列検索パネルの「置換して次に」ボタンが押された.
	void MainPage::find_replace_click(IInspectable const&, RoutedEventArgs const&)
	{
		find_text_preserve();
		const auto f_len = wchar_len(m_find_text);
		if (f_len == 0) {
			return;
		}

		bool flag = false;	// 一致または置換したか判定.
		DWRITE_TEXT_RANGE t_range;	// 文字範囲
		auto t = find_text_range_selected(m_main_page.m_shape_list.begin(), m_main_page.m_shape_list.end(), t_range);
		if (t != nullptr) {
			// 図形が見つかった場合,
			const auto w_pos = t_range.startPosition;
			// 英文字を区別するか判定する.
			if (m_find_text_case) {
				flag = wcsncmp(t->m_text + w_pos, m_find_text, f_len) == 0;
			}
			else {
				flag = _wcsnicmp(t->m_text + w_pos, m_find_text, f_len) == 0;
			}
			if (flag) {
				// 一致した場合
				const auto r_len = wchar_len(m_find_repl);
				const auto r_text = find_replace(t->m_text, w_pos, f_len, m_find_repl, r_len);
				undo_push_set<UNDO_T::TEXT_CONTENT>(t, r_text);
				undo_push_set<UNDO_T::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ w_pos, r_len });
				undo_push_null();
				undo_menu_is_enabled();
			}
		}
		// 次を検索する.
		Shape* s;
		DWRITE_TEXT_RANGE s_range;
		if (find_text(m_main_page.m_shape_list, m_find_text, m_find_text_case, m_find_text_wrap, t, s, s_range)) {
			// 検索できたならば,
			// 文字範囲が選択された図形があり, それが次の図形と異なるか判定する.
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
			// 検索できない, かつ置換もされてない場合,
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
		}
		status_bar_set_pos();
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

	// 文字列検索パネルの「次を検索」ボタンが押された.
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
			// 検索できない場合,
			// 「文字列は見つかりません」メッセージダイアログを表示する.
			message_show(ICON_INFO, NOT_FOUND, tx_find_text_what().Text());
		}
		status_bar_set_pos();
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
		btn_find_text_next().IsEnabled(not_empty);
		btn_find_replace().IsEnabled(not_empty);
		btn_find_replace_all().IsEnabled(not_empty);
	}

	// 検索の値をデータリーダーに書き込む.
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