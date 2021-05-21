//-------------------------------
// MainPage_text.cpp
// 文字列の編集, または検索 / 置換
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 「文字列が見つかりません」メッセージのリソースキー
	constexpr wchar_t NO_FOUND[] = L"str_err_found";

	// 文字列を検索して見つかった位置を得る.
	static bool text_find(const wchar_t* w_text, const uint32_t w_len, const wchar_t* f_text, const uint32_t f_len, const bool f_case, uint32_t& f_pos) noexcept;
	// 図形リストの中から文字列を検索し, それまで文字範囲が選択されていた図形, 新たに文字列が見つかった図形とその文字範囲を得る.
	static bool text_find(S_LIST_T& s_list, wchar_t* f_text, const bool f_case, const bool f_wrap, ShapeText*& t, Shape*& s, DWRITE_TEXT_RANGE& s_range);
	// 文字列の一部を置換する.
	static wchar_t* text_replace(wchar_t const* w_text, const uint32_t w_pos, const uint32_t w_len, wchar_t const* r_text, const uint32_t r_len) noexcept;
	// 文字範囲が選択された図形と文字範囲を見つける.
	static ShapeText* text_find_range_selected(S_LIST_T::iterator const& it_beg, S_LIST_T::iterator const& it_end, DWRITE_TEXT_RANGE& t_range);

	// 文字列を検索して見つかった位置を得る.
	// w_text	検索される文字列
	// w_len	検索される文字列の文字数
	// w_pos	検索を開始する位置
	// f_text	検索文字列
	// f_len	検索文字列の文字数
	// f_case	英文字の区別フラグ
	// f_break	改行を無視フラグ
	// f_pos	見つかった位置
	static bool text_find(const wchar_t* w_text, const uint32_t w_len, const wchar_t* f_text, const uint32_t f_len, const bool f_case, uint32_t& f_pos) noexcept
	{
		if (w_text == nullptr
			|| f_text == nullptr
			|| f_len > w_len || f_len == 0) {
			// 検索される文字列がヌル, 
			// または検索文字列がヌル, 
			// または文字数が検索文字列の文字数より小さい,
			// または検索文字列の文字数が 0 の場合
			// false を返す.
			return false;
		}
		if (f_case) {
			// 英文字の区別フラグが立っている場合,
			for (uint32_t i = 0; i <= w_len - f_len; i++) {
				if (wcsncmp(w_text + i, f_text, f_len) == 0) {
					f_pos = i;
					return true;
				}
			}
		}
		else {
			// 英文字の区別フラグがない場合,
			for (uint32_t i = 0; i <= w_len - f_len; i++) {
				if (_wcsnicmp(w_text + i, f_text, f_len) == 0) {
					f_pos = i;
					return true;
				}
			}
		}
		return false;
	}

	// 図形リストの中から文字列を検索し, それまで文字範囲が選択されていた図形, 新たに文字列が見つかった図形とその文字範囲を得る.
	// s_list	図形リスト
	// f_text	検索文字列
	// f_len	検索文字列の文字数
	// f_case	英文字の区別フラグ
	// f_wrap	回り込み検索フラグ
	// t	それまで文字範囲が選択されていた図形
	// s_found	新たに文字列が見つかった図形
	// s_range	新たに文字列が見つかった図形の文字範囲
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
					// 文字範囲が選択された図形の, 文字範囲より後ろの文字列を検索する.
					t = static_cast<ShapeText*>(s);
					t_pos = t_range.startPosition;
					const auto t_end = t_pos + t_range.length;
					uint32_t f_pos;
					if (text_find(t->m_text + t_end, wchar_len(t->m_text) - t_end, f_text, f_len, f_case, f_pos)) {
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
				if (text_find(t_text, wchar_len(t_text), f_text, f_len, f_case, f_pos)) {
					s_found = s;
					s_range.startPosition = f_pos;
					s_range.length = f_len;
					return true;
				}
			}
		}
		if (f_wrap != true && t != nullptr) {
			// 回り込み検索がない, かつ文字範囲が選択された図形が見つかった場合,
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

	// 文字範囲が選択された図形と文字範囲を見つける.
	// t_range	見つかった文字範囲
	// 戻り値	見つかった図形
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

	// 文字列の一部を置換する.
	// w_text	置換される前の文字列
	// f_pos	置換される文字列の開始位置
	// f_len	置換される文字数
	// r_text	置換文字列
	// r_len	置換文字列の文字数
	// 戻り値	置換された文字列
	static wchar_t* text_replace(wchar_t const* w_text, const uint32_t f_pos, const uint32_t f_len, wchar_t const* r_text, const uint32_t r_len) noexcept
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

	// 文字列検索パネルの「閉じる」ボタンが押された.
	void MainPage::text_find_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		text_find_set();
		sp_text_find().Visibility(COLLAPSED);
	}

	// 文字列検索パネルの「次を検索」ボタンが押された.
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
		// 検索できない場合,
		// 「文字列は見つかりません」メッセージダイアログを表示する.
		message_show(ICON_INFO, NO_FOUND, tx_text_find_what().Text());
	}

	// 文字列検索パネルの「すべて置換」ボタンが押された.
	void MainPage::text_replace_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		text_find_set();
		// 検索文字列の文字数を得る.
		const auto f_len = wchar_len(m_text_find);
		if (f_len == 0) {
			// 文字数が 0 の場合,
			return;
		}

		// あらかじめ検索文字列を含む文字列図形があるか判定する.
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
			// 図形がない場合,
			// 「文字列は見つかりません」メッセージダイアログを表示する.
			message_show(ICON_INFO, NO_FOUND, tx_text_find_what().Text());
			return;
		}
		// 文字範囲の選択のみを解除する.
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
				auto t = static_cast<ShapeText*>(s); // 検索される文字列図形
				auto w_text = wchar_cpy(t->m_text);	// 検索される文字列
				auto w_len = wchar_len(w_text);// 検索される文字列の文字数
				uint32_t w_pos = 0;	// 検索される文字列中の位置
				uint32_t f_pos = 0;
				flag = false;	// 一致フラグ
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

	// 文字列検索パネルの「置換して次に」ボタンが押された.
	void MainPage::text_replace_click(IInspectable const&, RoutedEventArgs const&)
	{
		text_find_set();
		const auto f_len = wchar_len(m_text_find);
		if (f_len == 0) {
			return;
		}

		bool flag = false;	// 一致または置換フラグ.
		DWRITE_TEXT_RANGE t_range;
		auto t = text_find_range_selected(m_list_shapes.begin(), m_list_shapes.end(), t_range);
		if (t != nullptr) {
			// 図形が見つかった場合,
			const auto w_pos = t_range.startPosition;
			if (m_text_find_case) {
				// 英文字の区別フラグが立っている場合,
				flag = wcsncmp(t->m_text + w_pos, m_text_find, f_len) == 0;
			}
			else {
				// 英文字の区別フラグがない場合,
				flag = _wcsnicmp(t->m_text + w_pos, m_text_find, f_len) == 0;
			}
			if (flag) {
				// 一致した場合
				const auto r_len = wchar_len(m_text_repl);
				const auto r_text = text_replace(t->m_text, w_pos, f_len, m_text_repl, r_len);
				undo_push_set<UNDO_OP::TEXT_CONTENT>(t, r_text);
				undo_push_set<UNDO_OP::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ w_pos, r_len });
				undo_push_null();
				undo_menu_enable();
			}
		}
		// 次を検索する.
		Shape* s;
		DWRITE_TEXT_RANGE s_range;
		if (text_find(m_list_shapes, m_text_find, m_text_find_case, m_text_find_wrap, t, s, s_range)) {
			// 検索できた場合,
			if (t != nullptr && s != t) {
				// 文字範囲が選択された図形があり, それが次の図形と異なる場合,
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
		// 検索できない, かつ置換もされてない場合,
		message_show(ICON_INFO, NO_FOUND, tx_text_find_what().Text());
	}

	// 編集メニューの「文字列の編集」が選択された.
	void MainPage::text_edit_click(IInspectable const&, RoutedEventArgs const&)
	{
		ShapeText* s = nullptr;
		if (pointer_shape_prev() != nullptr && typeid(*pointer_shape_prev()) == typeid(ShapeText)) {
			// 前回ポインターが押されたのが文字列図形の場合,
			s = static_cast<ShapeText*>(pointer_shape_prev());
		}
		else {
			// 選択された図形のうち最前面にある文字列図形を得る.
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

	// 編集メニューの「文字列の検索/置換」が選択された.
	void MainPage::text_find_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (sp_text_find().Visibility() == VISIBLE) {
			// 文字列検索パネルが表示されている場合,
			// 文字列検索パネルを非表示にする.
			sp_text_find().Visibility(COLLAPSED);
			text_find_set();
			return;
		}
		if (m_summary_atomic.load(std::memory_order_acquire)) {
		//if (m_summary_visible) {
			// 図形一覧パネルが表示されている場合,
			// 図形一覧パネルを非表示にする.
			summary_close();
		}
		tx_text_find_what().Text({ m_text_find == nullptr ? L"" : m_text_find });
		tx_text_replace_with().Text({ m_text_repl == nullptr ? L"" : m_text_repl });
		ck_text_find_case().IsChecked(m_text_find_case);
		ck_text_find_wrap().IsChecked(m_text_find_wrap);
		sp_text_find().Visibility(VISIBLE);
	}

	// 図形が持つ文字列を編集する.
	// s	文字列図形
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
			// 一連の操作の区切としてヌル操作をスタックに積む.
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
				// 一連の操作の区切としてヌル操作をスタックに積む.
				undo_push_null();
				// 編集メニュー項目の使用の可否を設定する.
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

	// 検索の値をデータリーダーから読み込む.
	void MainPage::text_find_read(DataReader const& dt_reader)
	{
		read(m_text_find, dt_reader);
		read(m_text_repl, dt_reader);
		uint16_t bit = dt_reader.ReadUInt16();
		m_text_adjust = ((bit & 1) != 0);
		m_text_find_case = ((bit & 2) != 0);
		m_text_find_wrap = ((bit & 4) != 0);
	}

	// 文字列検索パネルから値を格納する.
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

	// 検索の値をデータリーダーに書き込む.
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

	// 検索文字列が変更された.
	void MainPage::text_find_what_changed(IInspectable const&, TextChangedEventArgs const&)
	{
		const auto not_empty = (tx_text_find_what().Text().empty() != true);
		btn_text_find_next().IsEnabled(not_empty);
		btn_text_replace().IsEnabled(not_empty);
		btn_text_replace_all().IsEnabled(not_empty);
	}

}