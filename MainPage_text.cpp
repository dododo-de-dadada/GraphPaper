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
	// �}�`���X�g�̒����當�����������, ���������}�`�ƈʒu�𓾂�.
	static bool text_find_whithin_shapes(S_LIST_T::iterator const& it_beg, S_LIST_T::iterator const& it_end, wchar_t* f_text, uint32_t f_len, bool f_case, ShapeText*& s, uint32_t& pos) noexcept;
	// ������̈ꕔ��u������.
	static wchar_t* text_replace(wchar_t const* w_text, const uint32_t w_pos, const uint32_t w_len, wchar_t const* r_text, const uint32_t r_len) noexcept;

	// ��������������Č��������ʒu�𓾂�.
	// w_text	��������镶����
	// w_len	��������镶����̕�����
	// w_pos	�������J�n����ʒu
	// f_text	����������
	// f_len	����������̕�����
	// f_case	�p�����̋�ʃt���O
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

	// �}�`���X�g�̒����當�����������, ���������}�`�ƈʒu�𓾂�.
	// it_beg	�͈͂̍ŏ��̔����q
	// it_end	�͈͂̍Ō�̎��̔����q
	// f_text	�������镶����
	// f_len	�������镶����̕�����
	// f_case	�p�����̋�ʃt���O
	// t	��������������}�`
	// pos	���������ʒu
	// �߂�l	���������� true
	static bool text_find_whithin_shapes(S_LIST_T::iterator const& it_beg, S_LIST_T::iterator const& it_end, wchar_t* f_text, uint32_t f_len, bool f_case, ShapeText*& t, uint32_t& pos) noexcept
	{
		for (auto it = it_beg; it != it_end; it++) {
			auto s = *it;
			if (s->is_deleted()) {
				// �����t���O�������Ă���ꍇ,
				// �p������.
				continue;
			}
			// ������𓾂�.
			wchar_t* w;
			if (s->get_text(w) == false) {
				// �����Ȃ��ꍇ,
				// �p������.
				continue;
			}
			// ��������������Č��������ʒu�𓾂�.
			if (text_find(w, wchar_len(w), f_text, f_len, f_case, pos)) {
				// ���������ꍇ,
				// ���������}�`�Ɋi�[����.
				// true ��Ԃ�.
				t = static_cast<ShapeText*>(s);
				return true;
			}
		}
		return false;
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
	void MainPage::btn_text_find_close_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �����񌟍��p�l������l���i�[����.
		text_find_set_to();
		sp_text_find().Visibility(COLLAPSED);
	}

	// �����񌟍��p�l���́u���������v�{�^���������ꂽ.
	void MainPage::btn_text_find_next_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �}�`���X�g�̒����當�������������.
		if (text_find_whithin_shapes()) {
			// �����ł����ꍇ,
			// �y�[�W�Ɛ}�`��\������.
			page_draw();
			return;
		}
		// �����ł��Ȃ��ꍇ,
		// �u������͌�����܂���v���b�Z�[�W�_�C�A���O��\������.
		cd_message_show(ICON_INFO, NO_FOUND, tx_text_find_what().Text());
	}

	// �����񌟍��p�l���́u���ׂĒu���v�{�^���������ꂽ.
	void MainPage::btn_text_replace_all_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �����񌟍��p�l������l���i�[����.
		text_find_set_to();

		// ����������̕������𓾂�.
		const auto f_len = wchar_len(m_text_find);
		if (f_len == 0) {
			// �������� 0 �̏ꍇ,
			// �I������.
			return;
		}

		// ���炩���ߌ�����������܂ޕ�����}�`�����邩���ׂ�.
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
			// �}�`���Ȃ��ꍇ,
			// �u������͌�����܂���v���b�Z�[�W�_�C�A���O��\������.
			cd_message_show(ICON_INFO, NO_FOUND, tx_text_find_what().Text());
			return;
		}
		// �����͈͂̑I���݂̂���������.
		unselect_all(true);

		const auto r_len = wchar_len(m_text_repl);
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
		// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
		undo_push_null();
		// ���ɖ߂�/��蒼�����j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		enable_undo_menu();
		page_draw();
	}

	// �����񌟍��p�l���́u�u���v�{�^���������ꂽ.
	void MainPage::btn_text_replace_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �����񌟍��p�l������l���i�[����.
		text_find_set_to();
		// ����������̕������𓾂�.
		const auto f_len = wchar_len(m_text_find);
		if (f_len == 0) {
			// �������� 0 �̏ꍇ,
			// �I������.
			return;
		}

		// �����͈͂��I�����ꂽ�}�`��������.
		DWRITE_TEXT_RANGE t_range;
		auto it = text_find_range_selected(t_range);

		bool flag = false;	// ��v�܂��͒u���t���O.
		if (it != m_list_shapes.end()) {
			// �}�`�����������ꍇ,
			// �I�����ꂽ�͈͂ƌ��������񂪈�v���邩���ׂ�.
			auto t = static_cast<ShapeText*>(*it);
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
				// ��v�����ꍇ,
				// �u��������ƒu������.
				const auto r_len = wchar_len(m_text_repl);
				const auto r_text = text_replace(t->m_text, w_pos, f_len, m_text_repl, r_len);
				undo_push_set<UNDO_OP::TEXT_CONTENT>(t, r_text);
				// �����͈͂̒l��}�`�Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
				undo_push_set<UNDO_OP::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ w_pos, r_len });
				// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
				undo_push_null();
				// ���ɖ߂�/��蒼�����j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
				enable_undo_menu();
			}
		}
		// ���̐}�`�̕��������������.
		if (text_find_whithin_shapes() || flag) {
			// ��������, �܂��͒u�����ꂽ�ꍇ
			// ���ɖ߂�/��蒼�����j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
			enable_undo_menu();
			// �ĕ\������.
			page_draw();
			return;
		}
		// �����ł��Ȃ�, ���u��������ĂȂ��ꍇ,
		// �u������͌�����܂���v���b�Z�[�W�_�C�A���O��\������.
		cd_message_show(ICON_INFO, NO_FOUND, tx_text_find_what().Text());
	}

	// �ҏW���j���[�́u������̕ҏW�v���I�����ꂽ.
	void MainPage::mfi_text_edit_click(IInspectable const&, RoutedEventArgs const&)
	{
		ShapeText* s = nullptr;

		if (m_pointer_shape_prev != nullptr && typeid(*m_pointer_shape_prev) == typeid(ShapeText)) {
			// �O��|�C���^�[�������ꂽ�̂�������}�`�̏ꍇ,
			// ���̐}�`�𓾂�.
			s = static_cast<ShapeText*>(m_pointer_shape_prev);
		}
		else {
			// �I�����ꂽ�}�`�̂����őO�ʂɂ��镶����}�`�𓾂�.
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

	// �ҏW���j���[�́u������̌���/�u���v���I�����ꂽ.
	void MainPage::mfi_text_find_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (sp_text_find().Visibility() == VISIBLE) {
			// �����񌟍��p�l�����\������Ă���ꍇ,
			// �����񌟍��p�l�����\���ɂ���.
			sp_text_find().Visibility(COLLAPSED);
			// �����񌟍��p�l������l���i�[����.
			text_find_set_to();
			return;
		}
		if (m_summary_visible) {
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
				// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
				undo_push_null();
				// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
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

	// �����͈͂��I�����ꂽ�}�`�ƕ����͈͂�������.
	// t_range	�������������͈�
	// �߂�l	���������}�`�̔����q
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

	// �����̒l���f�[�^���[�_�[����ǂݍ���.
	void MainPage::text_find_read(DataReader const& dt_reader)
	{
		read(m_text_find, dt_reader);
		read(m_text_repl, dt_reader);
		m_text_find_case = dt_reader.ReadBoolean();
		m_text_find_wrap = dt_reader.ReadBoolean();
	}

	// �����񌟍��p�l������l���i�[����.
	void MainPage::text_find_set_to(void)
	{
		// ������������i�[����.
		if (m_text_find != nullptr) {
			delete[] m_text_find;
		}
		m_text_find = wchar_cpy(tx_text_find_what().Text().c_str());
		// �u����������i�[����.
		if (m_text_repl != nullptr) {
			delete[] m_text_repl;
		}
		m_text_repl = wchar_cpy(tx_text_replace_with().Text().c_str());
		// �p�����̋�ʃt���O���i�[����.
		m_text_find_case = ck_text_find_case().IsChecked().GetBoolean();
		// ��荞�݌����t���O���i�[����.
		m_text_find_wrap = ck_text_find_wrap().IsChecked().GetBoolean();
	}

	// �}�`���X�g�̒����當�������������.
	bool MainPage::text_find_whithin_shapes(void)
	{
		using winrt::GraphPaper::implementation::text_find;
		using winrt::GraphPaper::implementation::text_find_whithin_shapes;

		// �����񌟍��p�l������l���i�[����.
		text_find_set_to();
		// ����������̕������𓾂�.
		const auto f_len = wchar_len(m_text_find);
		if (f_len == 0) {
			return false;
		}
		// �����͈͂��I�����ꂽ�}�`�ƕ����͈͂�������.
		DWRITE_TEXT_RANGE t_range;
		auto it = text_find_range_selected(t_range);

		auto t = static_cast<ShapeText*>(nullptr);
		auto const& it_end = m_list_shapes.end();
		uint32_t f_pos = 0;	// ���������ʒu
		if (it == it_end) {
			// �I�����ꂽ�}�`��������Ȃ��ꍇ,
			// ���X�g�̍ŏ��̐}�`����Ō�܂Ō�������.
			auto const& it_begin = m_list_shapes.begin();
			if (text_find_whithin_shapes(it_begin, it_end, m_text_find, f_len, m_text_find_case, t, f_pos) == false) {
				// ������Ȃ��ꍇ false ��Ԃ�.
				return false;
			}
		}
		else {
			// �I�����ꂽ�}�`���������ꍇ,
			t = static_cast<ShapeText*>(*it);
			const auto t_text = t->m_text;
			const auto t_pos = t_range.startPosition;
			const auto t_end = t_pos + t_range.length;

			// �͈͑I�����ꂽ�}�`��, �����͈͂����̕��������������.
			if (text_find(t_text + t_end, wchar_len(t_text) - t_end, m_text_find, f_len, m_text_find_case, f_pos) == false) {
				// �V���Ɍ�����Ȃ��ꍇ,
				// �͈͑I�����ꂽ�}�`�̎�����Ō�܂Ō�������.
				auto const& it_next = std::next(it, 1);
				if (text_find_whithin_shapes(it_next, it_end, m_text_find, f_len, m_text_find_case, t, f_pos) == false) {
					if (m_text_find_wrap == false) {
						// �V���Ɍ�����Ȃ�, ����荞�݌����łȂ��ꍇ,
						// false ��Ԃ�.
						return false;
					}
					// �V���Ɍ�����Ȃ�, ����荞�݌����̏ꍇ,
					// ���X�g�̍ŏ����當���͈͂��I�����ꂽ�}�`�̒��O�܂Ō�������.
					auto const& it_begin = m_list_shapes.begin();
					if (text_find_whithin_shapes(it_begin, it, m_text_find, f_len, m_text_find_case, t, f_pos) == false) {
						// �V���Ɍ�����Ȃ��ꍇ,
						// �����͈͂��O�ɂ��镶�������������.
						if (text_find(t_text, t_pos, m_text_find, f_len, m_text_find_case, f_pos) == false) {
							// �V���Ɍ�����Ȃ��ꍇ,
							// false ��Ԃ�.
							return false;
						}
					}
				}
			}
			else {
				//  �����͈͂��I�����ꂽ�}�`�̒��ɕ������V���Ɍ������ꍇ
				f_pos += t_end;
			}
		}
		if (it != it_end && t != *it) {
			// �V���ɕʂ̐}�`�����������ꍇ,
			// { 0, 0 } ��}�`�Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
			// (���̐}�`�̕����͈͂̑I������������.)
			undo_push_set<UNDO_OP::TEXT_RANGE>(*it, DWRITE_TEXT_RANGE{ 0, 0 });
		}
		// �����͈͂�V���Ɍ������}�`�ɂ���, ���̑�����X�^�b�N�ɐς�.
		// (����X�^�b�N�Ƀk���͐ς܂Ȃ�.)
		undo_push_set<UNDO_OP::TEXT_RANGE>(t, DWRITE_TEXT_RANGE{ f_pos, f_len });
		// �}�`���\�������悤�y�[�W���X�N���[������.
		scroll_to_shape(t);
		// true ��Ԃ�.
		return true;
	}

	// �����̒l���f�[�^���[�_�[�ɏ�������.
	void MainPage::text_find_write(DataWriter const& dt_writer)
	{
		write(m_text_find, dt_writer);
		write(m_text_repl, dt_writer);
		dt_writer.WriteBoolean(m_text_find_case);
		dt_writer.WriteBoolean(m_text_find_wrap);
	}

	// ���������񂪕ύX���ꂽ.
	void MainPage::tx_text_find_what_changed(IInspectable const&, TextChangedEventArgs const&)
	{
		const auto not_empty = (tx_text_find_what().Text().empty() == false);
		btn_text_find_next().IsEnabled(not_empty);
		btn_text_replace().IsEnabled(not_empty);
		btn_text_replace_all().IsEnabled(not_empty);
	}

}