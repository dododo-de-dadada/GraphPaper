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
	// 図形リストの中から文字列を検索し, 見つかった図形と位置を得る.
	static bool text_find_whithin_shapes(S_LIST_T::iterator const& it_beg, S_LIST_T::iterator const& it_end, wchar_t* f_text, uint32_t f_len, bool f_case, ShapeText*& s, uint32_t& pos) noexcept;
	// 文字列の一部を置換する.
	static wchar_t* text_replace(wchar_t const* w_text, const uint32_t w_pos, const uint32_t w_len, wchar_t const* r_text, const uint32_t r_len) noexcept;

	// 文字列を検索して見つかった位置を得る.
	// w_text	検索される文字列
	// w_len	検索される文字列の文字数
	// w_pos	検索を開始する位置
	// f_text	検索文字列
	// f_len	検索文字列の文字数
	// f_case	英文字の区別フラグ
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

	// 図形リストの中から文字列を検索し, 見つかった図形と位置を得る.
	// it_beg	範囲の最初の反復子
	// it_end	範囲の最後の次の反復子
	// f_text	検索する文字列
	// f_len	検索する文字列の文字数
	// f_case	英文字の区別フラグ
	// t	見つかった文字列図形
	// pos	見つかった位置
	// 戻り値	見つかったら true
	static bool text_find_whithin_shapes(S_LIST_T::iterator const& it_beg, S_LIST_T::iterator const& it_end, wchar_t* f_text, uint32_t f_len, bool f_case, ShapeText*& t, uint32_t& pos) noexcept
	{
		for (auto it = it_beg; it != it_end; it++) {
			auto s = *it;
			if (s->is_deleted()) {
				// 消去フラグが立っている場合,
				// 継続する.
				continue;
			}
			// 文字列を得る.
			wchar_t* w;
			if (s->get_text(w) == false) {
				// 得られない場合,
				// 継続する.
				continue;
			}
			// 文字列を検索して見つかった位置を得る.
			if (text_find(w, wchar_len(w), f_text, f_len, f_case, pos)) {
				// 見つかった場合,
				// 見つかった図形に格納する.
				// true を返す.
				t = static_cast<ShapeText*>(s);
				return true;
			}
		}
		return false;
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
	void MainPage::btn_text_find_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 文字列検索パネルから値を格納する.
		text_find_set_to();
		sp_text_find().Visibility(COLLAPSED);
	}

	// 文字列検索パネルの「次を検索」ボタンが押された.
	void MainPage::btn_text_find_next_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 図形リストの中から文字列を検索する.
		if (text_find_whithin_shapes()) {
			// 検索できた場合,
			// ページと図形を表示する.
			page_draw();
			return;
		}
		// 検索できない場合,
		// 「文字列は見つかりません」メッセージダイアログを表示する.
		cd_message_show(ICON_INFO, NO_FOUND, tx_text_find_what().Text());
	}

	// 文字列検索パネルの「すべて置換」ボタンが押された.
	void MainPage::btn_text_replace_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 文字列検索パネルから値を格納する.
		text_find_set_to();

		// 検索文字列の文字数を得る.
		const auto f_len = wchar_len(m_text_find);
		if (f_len == 0) {
			// 文字数が 0 の場合,
			// 終了する.
			return;
		}

		// あらかじめ検索文字列を含む文字列図形があるか調べる.
		auto flag = false;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			wchar_t* w_text;
			if (s->get_text(w_text) == false) {
				continue;
			}
			uint32_t f_pos = 0;
			if (text_find(w_text, wchar_len(w_text), m_text_find, f_len, m_text_find_case, f_pos)) {
				flag = true;
				break;
			}
		}
		if (flag == false) {
			// 図形がない場合,
			// 「文字列は見つかりません」メッセージダイアログを表示する.
			cd_message_show(ICON_INFO, NO_FOUND, tx_text_find_what().Text());
			return;
		}
		// 文字範囲の選択のみを解除する.
		unselect_all(true);

		const auto r_len = wchar_len(m_text_repl);
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (typeid(*s) != typeid(ShapeText)) {
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
		// 一連の操作の区切としてヌル操作をスタックに積む.
		undo_push_null();
		// 元に戻す/やり直しメニュー項目の使用の可否を設定する.
		enable_undo_menu();
		page_draw();
	}

	// 文字列検索パネルの「置換」ボタンが押された.
	void MainPage::btn_text_replace_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 文字列検索パネルから値を格納する.
		text_find_set_to();
		// 検索文字列の文字数を得る.
		const auto f_len = wchar_len(m_text_find);
		if (f_len == 0) {
			// 文字数が 0 の場合,
			// 終了する.
			return;
		}

		// 文字範囲が選択された図形を見つける.
		DWRITE_TEXT_RANGE t_range;
		auto it = text_find_range_selected(t_range);

		bool flag = false;	// 一致または置換フラグ.
		if (it != m_list_shapes.end()) {
			// 図形が見つかった場合,
			// 選択された範囲と検索文字列が一致するか調べる.
			auto t = static_cast<ShapeText*>(*it);
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
				// 一致した場合,
				// 置換文字列と置換する.
				const auto r_len = wchar_len(m_text_repl);
				const auto r_text = text_replace(t->m_text, w_pos, f_len, m_text_repl, r_len);
				undo_push_set<UNDO_OP::TEXT_CONTENT>(t, r_text);
				// 文字範囲の値を図形に格納して, その操作をスタックに積む.
				undo_push_set<UNDO_OP::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ w_pos, r_len });
				// 一連の操作の区切としてヌル操作をスタックに積む.
				undo_push_null();
				// 元に戻す/やり直しメニュー項目の使用の可否を設定する.
				enable_undo_menu();
			}
		}
		// 次の図形の文字列を検索する.
		if (text_find_whithin_shapes() || flag) {
			// 見つかった, または置換された場合
			// 元に戻す/やり直しメニュー項目の使用の可否を設定する.
			enable_undo_menu();
			// 再表示する.
			page_draw();
			return;
		}
		// 検索できない, かつ置換もされてない場合,
		// 「文字列は見つかりません」メッセージダイアログを表示する.
		cd_message_show(ICON_INFO, NO_FOUND, tx_text_find_what().Text());
	}

	// 編集メニューの「文字列の編集」が選択された.
	void MainPage::mfi_text_edit_click(IInspectable const&, RoutedEventArgs const&)
	{
		ShapeText* s = nullptr;

		if (m_pointer_shape_prev != nullptr && typeid(*m_pointer_shape_prev) == typeid(ShapeText)) {
			// 前回ポインターが押されたのが文字列図形の場合,
			// その図形を得る.
			s = static_cast<ShapeText*>(m_pointer_shape_prev);
		}
		else {
			// 選択された図形のうち最前面にある文字列図形を得る.
			for (auto it = m_list_shapes.rbegin(); it != m_list_shapes.rend(); it++) {
				auto t = *it;
				if (t->is_deleted()) {
					continue;
				}
				if (t->is_selected() == false) {
					continue;
				}
				if (typeid(*t) == typeid(ShapeText)) {
					s = static_cast<ShapeText*>(t);
					break;
				}
			}
		}
		if (s != nullptr) {
			text_edit_in(s);
		}

	}

	// 編集メニューの「文字列の検索/置換」が選択された.
	void MainPage::mfi_text_find_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (sp_text_find().Visibility() == VISIBLE) {
			// 文字列検索パネルが表示されている場合,
			// 文字列検索パネルを非表示にする.
			sp_text_find().Visibility(COLLAPSED);
			// 文字列検索パネルから値を格納する.
			text_find_set_to();
			return;
		}
		if (m_summary_visible) {
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
	void MainPage::text_edit_in(ShapeText* s)
	{
		static winrt::event_token primary_token;
		static winrt::event_token closed_token;

		tx_edit().Text(s->m_text == nullptr ? L"" : s->m_text);
		tx_edit().SelectAll();
		primary_token = cd_edit_text().PrimaryButtonClick(
			[this, s](auto, auto)
			{
				auto text = wchar_cpy(tx_edit().Text().c_str());
				undo_push_set<UNDO_OP::TEXT_CONTENT>(s, text);
				if (ck_text_ignore_bottom_blank().IsChecked().GetBoolean()) {
					s->delete_bottom_blank();
				}
				// 一連の操作の区切としてヌル操作をスタックに積む.
				undo_push_null();
				// 編集メニュー項目の使用の可否を設定する.
				enable_edit_menu();
			}
		);
		closed_token = cd_edit_text().Closed(
			[this](auto, auto)
			{
				cd_edit_text().PrimaryButtonClick(primary_token);
				cd_edit_text().Closed(closed_token);
				page_draw();
			}
		);
		auto _{ cd_edit_text().ShowAsync() };
	}

	// 文字範囲が選択された図形と文字範囲を見つける.
	// t_range	見つかった文字範囲
	// 戻り値	見つかった図形の反復子
	S_LIST_T::iterator MainPage::text_find_range_selected(DWRITE_TEXT_RANGE& t_range)
	{
		auto const& it_end = m_list_shapes.end();
		for (auto it = m_list_shapes.begin(); it != it_end; it++) {
			auto s = *it;
			if (s->is_deleted()) {
				continue;
			}
			if (s->get_text_range(t_range) == false) {
				continue;
			}
			if (t_range.startPosition > 0 || t_range.length > 0) {
				return it;
			}
		}
		return it_end;
	}

	// 検索の値をデータリーダーから読み込む.
	void MainPage::text_find_read(DataReader const& dt_reader)
	{
		read(m_text_find, dt_reader);
		read(m_text_repl, dt_reader);
		m_text_find_case = dt_reader.ReadBoolean();
		m_text_find_wrap = dt_reader.ReadBoolean();
	}

	// 文字列検索パネルから値を格納する.
	void MainPage::text_find_set_to(void)
	{
		// 検索文字列を格納する.
		if (m_text_find != nullptr) {
			delete[] m_text_find;
		}
		m_text_find = wchar_cpy(tx_text_find_what().Text().c_str());
		// 置換文字列を格納する.
		if (m_text_repl != nullptr) {
			delete[] m_text_repl;
		}
		m_text_repl = wchar_cpy(tx_text_replace_with().Text().c_str());
		// 英文字の区別フラグを格納する.
		m_text_find_case = ck_text_find_case().IsChecked().GetBoolean();
		// 回り込み検索フラグを格納する.
		m_text_find_wrap = ck_text_find_wrap().IsChecked().GetBoolean();
	}

	// 図形リストの中から文字列を検索する.
	bool MainPage::text_find_whithin_shapes(void)
	{
		using winrt::GraphPaper::implementation::text_find;
		using winrt::GraphPaper::implementation::text_find_whithin_shapes;

		// 文字列検索パネルから値を格納する.
		text_find_set_to();
		// 検索文字列の文字数を得る.
		const auto f_len = wchar_len(m_text_find);
		if (f_len == 0) {
			return false;
		}
		// 文字範囲が選択された図形と文字範囲を見つける.
		DWRITE_TEXT_RANGE t_range;
		auto it = text_find_range_selected(t_range);

		auto t = static_cast<ShapeText*>(nullptr);
		auto const& it_end = m_list_shapes.end();
		uint32_t f_pos = 0;	// 見つかった位置
		if (it == it_end) {
			// 選択された図形が見つからない場合,
			// リストの最初の図形から最後まで検索する.
			auto const& it_begin = m_list_shapes.begin();
			if (text_find_whithin_shapes(it_begin, it_end, m_text_find, f_len, m_text_find_case, t, f_pos) == false) {
				// 見つからない場合 false を返す.
				return false;
			}
		}
		else {
			// 選択された図形を見つけた場合,
			t = static_cast<ShapeText*>(*it);
			const auto t_text = t->m_text;
			const auto t_pos = t_range.startPosition;
			const auto t_end = t_pos + t_range.length;

			// 範囲選択された図形の, 文字範囲より後ろの文字列を検索する.
			if (text_find(t_text + t_end, wchar_len(t_text) - t_end, m_text_find, f_len, m_text_find_case, f_pos) == false) {
				// 新たに見つからない場合,
				// 範囲選択された図形の次から最後まで検索する.
				auto const& it_next = std::next(it, 1);
				if (text_find_whithin_shapes(it_next, it_end, m_text_find, f_len, m_text_find_case, t, f_pos) == false) {
					if (m_text_find_wrap == false) {
						// 新たに見つからない, かつ回り込み検索でない場合,
						// false を返す.
						return false;
					}
					// 新たに見つからない, かつ回り込み検索の場合,
					// リストの最初から文字範囲が選択された図形の直前まで検索する.
					auto const& it_begin = m_list_shapes.begin();
					if (text_find_whithin_shapes(it_begin, it, m_text_find, f_len, m_text_find_case, t, f_pos) == false) {
						// 新たに見つからない場合,
						// 文字範囲より前にある文字列を検索する.
						if (text_find(t_text, t_pos, m_text_find, f_len, m_text_find_case, f_pos) == false) {
							// 新たに見つからない場合,
							// false を返す.
							return false;
						}
					}
				}
			}
			else {
				//  文字範囲が選択された図形の中に文字列を新たに見つけた場合
				f_pos += t_end;
			}
		}
		if (it != it_end && t != *it) {
			// 新たに別の図形が見つかった場合,
			// { 0, 0 } を図形に格納して, その操作をスタックに積む.
			// (元の図形の文字範囲の選択を消去する.)
			undo_push_set<UNDO_OP::TEXT_RANGE>(*it, DWRITE_TEXT_RANGE{ 0, 0 });
		}
		// 文字範囲を新たに見つけた図形にして, その操作をスタックに積む.
		// (操作スタックにヌルは積まない.)
		undo_push_set<UNDO_OP::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ f_pos, f_len });
		// 図形が表示されるようページをスクロールする.
		scroll_to_shape(t);
		// true を返す.
		return true;
	}

	// 検索の値をデータリーダーに書き込む.
	void MainPage::text_find_write(DataWriter const& dt_writer)
	{
		write(m_text_find, dt_writer);
		write(m_text_repl, dt_writer);
		dt_writer.WriteBoolean(m_text_find_case);
		dt_writer.WriteBoolean(m_text_find_wrap);
	}

	// 検索文字列が変更された.
	void MainPage::tx_text_find_what_changed(IInspectable const&, TextChangedEventArgs const&)
	{
		const auto not_empty = (tx_text_find_what().Text().empty() == false);
		btn_text_find_next().IsEnabled(not_empty);
		btn_text_replace().IsEnabled(not_empty);
		btn_text_replace_all().IsEnabled(not_empty);
	}

}