//-------------------------------
//	MainPage_find.cpp
//	������̌���/�u��
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t NO_FOUND[] = L"str_err_found";

	//	��������������Č��������ʒu�𓾂�.
	static bool find_text(const wchar_t* w_text, const uint32_t w_len, const wchar_t* f_text, const uint32_t f_len, const bool match_case, uint32_t& pos) noexcept;
	//	�}�`���X�g�̒����當�����������, ���������}�`�ƈʒu�𓾂�.
	static bool find_text_whithin_shapes(S_LIST_T::iterator const& it_beg, S_LIST_T::iterator const& it_end, wchar_t* f_text, uint32_t f_len, bool match_case, ShapeText*& s, uint32_t& pos) noexcept;
	//	������̈ꕔ��u������.
	static wchar_t* replace_text(wchar_t const* w_text, const uint32_t w_pos, const uint32_t w_len, wchar_t const* r_text, const uint32_t r_len) noexcept;

	//	��������������Č��������ʒu�𓾂�.
	//	w_text	��������镶����
	//	w_len	��������镶����̕�����
	//	w_pos	�������J�n����ʒu
	//	f_text	����������
	//	f_len	����������̕�����
	//	match_case	�p�����̋�ʃt���O
	//	pos	���������ʒu
	static bool find_text(const wchar_t* w_text, const uint32_t w_len, const wchar_t* f_text, const uint32_t f_len, const bool match_case, uint32_t& pos) noexcept
	{
		if (w_text == nullptr
			|| f_text == nullptr
			|| f_len > w_len || f_len == 0) {
			//	��������镶���񂪃k��, 
			//	�܂��͌��������񂪃k��, 
			//	�܂��͕�����������������̕�������菬����,
			//	�܂��͌���������̕������� 0 �̏ꍇ
			//	false ��Ԃ�.
			return false;
		}
		if (match_case) {
			for (uint32_t i = 0; i <= w_len - f_len; i++) {
				if (wcsncmp(w_text + i, f_text, f_len) == 0) {
					pos = i;
					return true;
				}
			}
		}
		else {
			for (uint32_t i = 0; i <= w_len - f_len; i++) {
				if (_wcsnicmp(w_text + i, f_text, f_len) == 0) {
					pos = i;
					return true;
				}
			}
		}
		return false;
	}

	// �}�`���X�g�̒����當�����������, ���������}�`�ƈʒu�𓾂�.
	// it_beg	�͈͂̍ŏ��̔����q
	// it_end	�͈͂̍Ō�̎��̔����q
	// f_text	�������镶����
	// f_len	�������镶����̕�����
	// match_case	�p�����̋�ʃt���O
	// t	��������������}�`
	// pos	���������ʒu
	// �߂�l	���������� true
	static bool find_text_whithin_shapes(S_LIST_T::iterator const& it_beg, S_LIST_T::iterator const& it_end, wchar_t* f_text, uint32_t f_len, bool match_case, ShapeText*& t, uint32_t& pos) noexcept
	{
		for (auto it = it_beg; it != it_end; it++) {
			auto s = *it;
			if (s->is_deleted()) {
				//	�����t���O�������Ă���ꍇ,
				//	�p������.
				continue;
			}
			//	������𓾂�.
			wchar_t* w;
			if (s->get_text(w) == false) {
				//	�����Ȃ��ꍇ,
				//	�p������.
				continue;
			}
			// ��������������Č��������ʒu�𓾂�.
			if (find_text(w, wchar_len(w), f_text, f_len, match_case, pos)) {
				//	���������ꍇ,
				//	���������}�`�Ɋi�[����.
				//	true ��Ԃ�.
				t = static_cast<ShapeText*>(s);
				return true;
			}
		}
		return false;
	}

	//	������̈ꕔ��u������.
	//	w_text	�u�������O�̕�����
	//	f_pos	�u������镶����̊J�n�ʒu
	//	f_len	�u������镶����
	//	r_text	�u��������
	//	r_len	�u��������̕�����
	//	�߂�l	�u�����ꂽ������
	static wchar_t* replace_text(wchar_t const* w_text, const uint32_t f_pos, const uint32_t f_len, wchar_t const* r_text, const uint32_t r_len) noexcept
	{
		if (w_text != nullptr) {
			//	�u�������O�̕�����̕������𓾂�.
			const uint32_t w_len = wchar_len(w_text);
			//	�u������镶����̏I���ʒu�𓾂�.
			const uint32_t f_end = f_pos + f_len;
			if (f_end <= w_len) {
				//	�I���ʒu���������ȉ��̏ꍇ,
				//	�u�����ꂽ��̕�����̏I���ʒu�𓾂�.
				const size_t r_end = static_cast<size_t>(f_pos) + static_cast<size_t>(r_len);
				//	�u�����ꂽ��̕�����̕������𓾂�.
				const uint32_t n_len = static_cast<size_t>(w_len) + static_cast<size_t>(r_len) - static_cast<size_t>(f_len);
				//	�u�����ꂽ��̕�������i�[����z����m�ۂ���.
				wchar_t* const n_text = new wchar_t[n_len + 1];
				//	�J�n�ʒu�܂ł̕������z��Ɋi�[����.
				wcsncpy_s(n_text, n_len + 1, w_text, f_pos);
				//for (uint32_t i = 0; i < f_pos; i++) {
				//	n_text[i] = w_text[i];
				//}
				//	�u���������z��Ɋi�[����.
				wcsncpy_s(n_text + f_pos, n_len - f_pos + 1, r_text, r_len);
				//for (uint32_t i = f_pos, j = 0; i < n_len && i < r_end && j < r_len; i++, j++) {
				//	n_text[i] = r_text[j];
				//}
				//	�I���ʒu�����̕������z��Ɋi�[����.
				wcsncpy_s(n_text + r_end, n_len - r_end + 1, w_text + f_end, w_len - f_end);
				//for (uint32_t i = r_end, j = f_end; i < n_len && j < w_len; i++, j++) {
				//	n_text[i] = w_text[j];
				//}
				//n_text[n_len] = L'\0';
				return n_text;
			}
		}
		return wchar_cpy(r_text);
	}

	// �����p�l���́u����v�{�^���������ꂽ.
	void MainPage::btn_find_close_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	�����p�l������l���i�[����.
		find_set_to_panel();
		sp_find_text().Visibility(COLLAPSED);
	}

	// �����p�l���́u���������v�{�^���������ꂽ.
	void MainPage::btn_find_next_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		if (find_text_whithin_shapes()) {
			draw_page();
			return;
		}
		// �����ł��Ȃ����,
		// �u������܂���v���b�Z�[�W�_�C�A���O��\������.
		cd_message_show(NO_FOUND, tx_find_what().Text());
	}

	// �����p�l���́u���ׂĒu���v�{�^���������ꂽ.
	void MainPage::btn_replace_all_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	�����p�l������l���i�[����.
		find_set_to_panel();

		//	����������̕������𓾂�.
		const auto f_len = wchar_len(m_find_text);
		if (f_len == 0) {
			//	�������� 0 �̏ꍇ,
			//	�I������.
			return;
		}

		//	���炩���ߌ�����������܂ޕ�����}�`�����邩���ׂ�.
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
			if (find_text(w_text, wchar_len(w_text), m_find_text, f_len, m_find_case, f_pos)) {
				flag = true;
				break;
			}
		}
		if (flag == false) {
			//	�}�`���Ȃ����,
			// �u������܂���v���b�Z�[�W�_�C�A���O��\������.
			cd_message_show(NO_FOUND, tx_find_what().Text());
			return;
		}
		// �����͈͂̑I������������.
		unselect_all(true);

		const auto r_len = wchar_len(m_find_repl);
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (typeid(*s) != typeid(ShapeText)) {
				continue;
			}
			auto t = static_cast<ShapeText*>(s); // ��������镶����}�`
			auto w_text = wchar_cpy(t->m_text);	// ��������镶����
			auto w_len = wchar_len(w_text);// ��������镶����̕�����
			uint32_t w_pos = 0;	// ��������镶���񒆂̈ʒu
			uint32_t f_pos = 0;
			flag = false;	// ��v�t���O
			while (find_text(w_text + w_pos, w_len - w_pos, m_find_text, f_len, m_find_case, f_pos)) {
				flag = true;
				w_pos += f_pos;
				auto t_text = replace_text(w_text, w_pos, f_len, m_find_repl, r_len);
				delete[] w_text;
				w_text = t_text;
				w_len += r_len - f_len;
				w_pos += r_len;
			}
			if (flag) {
				undo_push_set<UNDO_OP::TEXT>(t, w_text);
			}
		}
		undo_push_null();
		enable_undo_menu();
		draw_page();
	}

	//	�����p�l������l���i�[����.
	void MainPage::find_set_to_panel(void)
	{
		//	������������i�[����.
		if (m_find_text != nullptr) {
			delete[] m_find_text;
		}
		m_find_text = wchar_cpy(tx_find_what().Text().c_str());
		//	�u����������i�[����.
		if (m_find_repl != nullptr) {
			delete[] m_find_repl;
		}
		m_find_repl = wchar_cpy(tx_repl_with().Text().c_str());
		//	�p�����̋�ʃt���O���i�[����.
		m_find_case = ck_match_case().IsChecked().GetBoolean();
		//	��荞�݌����t���O���i�[����.
		m_find_wrap = ck_wrap_around().IsChecked().GetBoolean();
	}

	//	�����p�l���́u�u���v�{�^���������ꂽ.
	void MainPage::btn_replace_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	�����p�l������l���i�[����.
		find_set_to_panel();
		//	����������̕������𓾂�.
		const auto f_len = wchar_len(m_find_text);
		if (f_len == 0) {
			//	�������� 0 �̏ꍇ,
			//	�I������.
			return;
		}

		//	�����͈͂��I�����ꂽ�}�`��������.
		DWRITE_TEXT_RANGE t_range;
		auto it = find_range_selected_shape(t_range);

		bool flag = false;	// ��v�܂��͒u���t���O.
		if (it != m_list_shapes.end()) {
			//	�}�`�����������ꍇ,
			//	�I�����ꂽ�͈͂ƌ��������񂪈�v���邩���ׂ�.
			auto t = static_cast<ShapeText*>(*it);
			const auto w_pos = t_range.startPosition;
			if (m_find_case) {
				//	�p�����̋�ʃt���O�������Ă���ꍇ,
				flag = wcsncmp(t->m_text + w_pos, m_find_text, f_len) == 0;
			}
			else {
				//	�p�����̋�ʃt���O���Ȃ��ꍇ,
				flag = _wcsnicmp(t->m_text + w_pos, m_find_text, f_len) == 0;
			}
			if (flag) {
				//	��v�����ꍇ,
				//	�u��������ƒu������.
				const auto r_len = wchar_len(m_find_repl);
				auto r_text = replace_text(t->m_text, w_pos, f_len, m_find_repl, r_len);
				undo_push_set<UNDO_OP::TEXT>(t, r_text);
				undo_push_set<UNDO_OP::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ w_pos, r_len });
				undo_push_null();
				enable_undo_menu();
			}
		}
		//	���̐}�`�̕��������������.
		if (find_text_whithin_shapes() || flag) {
			//	��������, �܂��͒u�����ꂽ�ꍇ
			//	�ĕ\������.
			draw_page();
			return;
		}
		//	�����ł��Ȃ�, ���u��������ĂȂ��ꍇ,
		// �u������܂���v���b�Z�[�W�_�C�A���O��\������.
		cd_message_show(NO_FOUND, tx_find_what().Text());
	}

	//	�����͈͂��I�����ꂽ�}�`�ƕ����͈͂�������.
	//	t_range	�������������͈�
	//	�߂�l	���������}�`�̔����q
	S_LIST_T::iterator MainPage::find_range_selected_shape(DWRITE_TEXT_RANGE& t_range)
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

	//	�����̒l���f�[�^���[�_�[����ǂݍ���.
	void MainPage::find_read(DataReader const& dt_reader)
	{
		read(m_find_text, dt_reader);
		read(m_find_repl, dt_reader);
		m_find_case = dt_reader.ReadBoolean();
		m_find_wrap = dt_reader.ReadBoolean();
	}

	//	�}�`�̕��������������.
	bool MainPage::find_text_whithin_shapes(void)
	{
		using winrt::GraphPaper::implementation::find_text;
		using winrt::GraphPaper::implementation::find_text_whithin_shapes;

		//	�����p�l������l���i�[����.
		find_set_to_panel();
		//	����������̕������𓾂�.
		const auto f_len = wchar_len(m_find_text);
		if (f_len == 0) {
			return false;
		}
		//	�����͈͂��I�����ꂽ�}�`�ƕ����͈͂�������.
		DWRITE_TEXT_RANGE t_range;
		auto it = find_range_selected_shape(t_range);

		auto t = static_cast<ShapeText*>(nullptr);
		auto const& it_end = m_list_shapes.end();
		uint32_t f_pos = 0;	// ���������ʒu
		if (it == it_end) {
			//	�I�����ꂽ�}�`��������Ȃ��ꍇ,
			//	���X�g�̍ŏ��̐}�`����Ō�܂Ō�������.
			auto const& it_begin = m_list_shapes.begin();
			if (find_text_whithin_shapes(it_begin, it_end, m_find_text, f_len, m_find_case, t, f_pos) == false) {
				//	������Ȃ��ꍇ false ��Ԃ�.
				return false;
			}
		}
		else {
			//	�I�����ꂽ�}�`���������ꍇ,
			t = static_cast<ShapeText*>(*it);
			const auto t_text = t->m_text;
			const auto t_pos = t_range.startPosition;
			const auto t_end = t_pos + t_range.length;

			//	�͈͑I�����ꂽ�}�`��, �����͈͂����̕��������������.
			if (find_text(t_text + t_end, wchar_len(t_text) - t_end, m_find_text, f_len, m_find_case, f_pos) == false) {
				//	�V���Ɍ�����Ȃ��ꍇ,
				//	�͈͑I�����ꂽ�}�`�̎�����Ō�܂Ō�������.
				auto const& it_next = std::next(it, 1);
				if (find_text_whithin_shapes(it_next, it_end, m_find_text, f_len, m_find_case, t, f_pos) == false) {
					if (m_find_wrap == false) {
						//	�V���Ɍ�����Ȃ�, ����荞�݌����łȂ��ꍇ,
						//	false ��Ԃ�.
						return false;
					}
					//	�V���Ɍ�����Ȃ�, ����荞�݌����̏ꍇ,
					//	���X�g�̍ŏ�����͈͑I�����ꂽ�}�`�̒��O�܂Ō�������.
					auto const& it_begin = m_list_shapes.begin();
					if (find_text_whithin_shapes(it_begin, it, m_find_text, f_len, m_find_case, t, f_pos) == false) {
						//	�V���Ɍ�����Ȃ��ꍇ,
						//	�����͈͂��O�ɂ��镶�������������.
						if (find_text(t_text, t_pos, m_find_text, f_len, m_find_case, f_pos) == false) {
							//	�V���Ɍ�����Ȃ��ꍇ,
							//	false ��Ԃ�.
							return false;
						}
					}
					else {
						//	�V���ɕʂ̐}�`�����������ꍇ,
						//	�͈͑I�����ꂽ�}�`�̕����͈͂���������.
						undo_push_set<UNDO_OP::TEXT_RANGE>(*it, DWRITE_TEXT_RANGE{ 0, 0 });
					}
				}
				else {
					//	�V���ɕʂ̐}�`�����������ꍇ,
					//	�͈͑I�����ꂽ�}�`�̕����͈͂���������.
					undo_push_set<UNDO_OP::TEXT_RANGE>(*it, DWRITE_TEXT_RANGE{ 0, 0 });
				}
			}
			else {
				//	�͈͑I�����ꂽ�}�`�̒��ɕ������V���Ɍ������ꍇ
				f_pos += t_end;
			}
		}
		//	�V���Ɍ������}�`�̕����͈͂Ɋi�[����.
		//	(����X�^�b�N�Ƀk���͐ς܂Ȃ�.)
		undo_push_set<UNDO_OP::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ f_pos, f_len });
		redo_clear();
		scroll_to_shape(t);
		return true;
	}

	//	�ҏW���j���[�́u������̌���/�u���v���I�����ꂽ.
	void MainPage::mfi_find_text_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		if (sp_find_text().Visibility() == VISIBLE) {
			//	�����p�l�����\������Ă���ꍇ,
			//	�����p�l�����\���ɂ���.
			sp_find_text().Visibility(COLLAPSED);
			//	�����p�l������l���i�[����.
			find_set_to_panel();
			return;
		}
		if (m_summary_visible) {
			//	�ꗗ�p�l�����\������Ă���ꍇ,
			//	�ꗗ�p�l�����\���ɂ���.
			summary_close();
		}
		tx_find_what().Text({ m_find_text == nullptr ? L"" : m_find_text });
		tx_repl_with().Text({ m_find_repl == nullptr ? L"" : m_find_repl });
		ck_match_case().IsChecked(m_find_case);
		ck_wrap_around().IsChecked(m_find_wrap);
		sp_find_text().Visibility(VISIBLE);
	}

	// �����̒l���f�[�^���[�_�[�ɏ�������.
	void MainPage::find_write(DataWriter const& dt_writer)
	{
		write(m_find_text, dt_writer);
		write(m_find_repl, dt_writer);
		dt_writer.WriteBoolean(m_find_case);
		dt_writer.WriteBoolean(m_find_wrap);
	}

	// ���������񂪕ύX���ꂽ.
	void MainPage::tx_find_what_text_changed(IInspectable const& /*sender*/, TextChangedEventArgs const& /*args*/)
	{
		const auto not_empty = (tx_find_what().Text().empty() == false);
		btn_find_next().IsEnabled(not_empty);
		btn_replace().IsEnabled(not_empty);
		btn_replace_all().IsEnabled(not_empty);
	}

}