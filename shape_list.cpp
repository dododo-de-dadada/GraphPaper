//------------------------------
// Shape_list.cpp
// �}�`���X�g
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �}�`�̎��
	// �t�@�C���ւ̏������݂Ŏg�p����.
	enum SHAPE_TYPE {
		SHAPE_NULL,		// �k��
		SHAPE_BEZI,		// �Ȑ�
		SHAPE_ELLI,		// ���~
		SHAPE_LINE,		// ����
		SHAPE_QUAD,		// �l�ӌ`
		SHAPE_RECT,		// ���`
		SHAPE_RRCT,		// �p�ە��`
		SHAPE_TEXT,		// ������
		SHAPE_GROUP,	// �O���[�v
		SHAPE_RULER		// ��K
	};

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	static Shape* s_list_read_shape(DataReader const& dt_reader);
	// ���̐}�`�𓾂�.
	template <typename T> static Shape* s_list_next(T const& it_begin, T const& it_end, const Shape* s) noexcept;
	// ���̐}�`�Ƃ��̋����𓾂�.
	template <typename T> static Shape* s_list_next(T const& it_begin, T const& it_end, uint32_t& distance) noexcept;

	// �g�p�\�ȏ��̖������ׂ�.
	bool s_list_available_font(const S_LIST_T& s_list, wchar_t*& unavailable_font) noexcept
	{
		// �}�`���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
		for (auto s : s_list) {
			if (s->is_deleted()) {
				continue;
			}
			wchar_t* value;	// ���̖�
			if (s->get_font_family(value)) {
				if (ShapeText::is_available_font(value) != true) {
					unavailable_font = value;
					return false;
				}
			}
		}
		return true;
	}

	// �Ō�̐}�`�����X�g���瓾��.
	// s_list	�}�`���X�g
	// �߂�l	����ꂽ�}�`
	Shape* s_list_back(S_LIST_T const& s_list) noexcept
	{
		uint32_t _;
		return s_list_next(s_list.rbegin(), s_list.rend(), _);
	}

	// �}�`�S�̗̂̈�����X�g���瓾��.
	// s_list	�}�`���X�g
	// s_size	�p���̐��@
	// b_min	�̈�̍���ʒu
	// b_max	�̈�̉E���ʒu
	void s_list_bound(S_LIST_T const& s_list, const D2D1_SIZE_F s_size, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept
	{
		b_min = { 0.0F, 0.0F };	// ����ʒu
		b_max = { s_size.width, s_size.height };	// �E���ʒu
		for (auto s : s_list) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_min, b_max);
		}
	}

	// �}�`�S�̗̂̈�����X�g���瓾��.
	// s_list	�}�`���X�g
	// b_min	�̈�̍���ʒu
	// b_max	�̈�̉E���ʒu
	void s_list_bound(S_LIST_T const& s_list, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept
	{
		b_min = { FLT_MAX, FLT_MAX };	// ����ʒu
		b_max = { -FLT_MAX, -FLT_MAX };	// �E���ʒu
		for (auto s : s_list) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_min, b_max);
		}
	}

	// �}�`���X�g��������, �܂܂��}�`��j������.
	// s_list	�}�`���X�g
	void s_list_clear(S_LIST_T& s_list) noexcept
	{
		for (auto s : s_list) {
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		s_list.clear();
	}

	// �}�`���X�g�̐}�`�𐔂���
	// undeleted_cnt	�����t���O���Ȃ��}�`�̐�
	// selected_cnt	�I�����ꂽ�}�`�̐�
	// selected_group_cnt	�I�����ꂽ�O���[�v�}�`�̐�
	// runlength_cnt	�I�����ꂽ�}�`�̃��������O�X�̐�
	// selected_text_cnt	�I�����ꂽ������}�`�̐�
	// text_cnt	������}�`�̐�
	// fore_selected	�őO�ʂ̐}�`�̑I���t���O
	// back_selected	�Ŕw�ʂ̐}�`�̑I���t���O
	// prev_selected	�ЂƂw�ʂ̐}�`�̑I���t���O
	void s_list_count(
		const S_LIST_T& s_list,
		uint32_t& undeleted_cnt,
		uint32_t& selected_cnt,
		uint32_t& selected_group_cnt,
		uint32_t& runlength_cnt,
		uint32_t& selected_text_cnt,
		uint32_t& text_cnt,
		bool& fore_selected,
		bool& back_selected,
		bool& prev_selected
	) noexcept
	{
		undeleted_cnt = 0;	// �����t���O���Ȃ��}�`�̐�
		selected_cnt = 0;	// �I�����ꂽ�}�`�̐�
		selected_group_cnt = 0;	// �I�����ꂽ�O���[�v�}�`�̐�
		runlength_cnt = 0;	// �I�����ꂽ�}�`�̃��������O�X�̐�
		selected_text_cnt = 0;	// �I�����ꂽ������}�`�̐�
		text_cnt = 0;	// ������}�`�̐�
		fore_selected = false;	// �őO�ʂ̐}�`�̑I���t���O
		back_selected = false;	// �Ŕw�ʂ̐}�`�̑I���t���O
		prev_selected = false;	// �ЂƂw�ʂ̐}�`�̑I���t���O

		// �}�`���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
		for (auto s : s_list) {
			if (s->is_deleted()) {
				// �}�`�̏����t���O�������Ă���ꍇ,
				// �ȉ��𖳎�����.
				continue;
			}
			// �����t���O���Ȃ��}�`�̐����C���N�������g����.
			undeleted_cnt++;
			// �}�`�̓��I�Ȍ^�𓾂�.
			auto const& s_type = typeid(*s);
			if (s_type == typeid(ShapeText)) {
				// �^��������}�`�̏ꍇ,
				// ������}�`�̐����C���N�������g����.
				text_cnt++;
			}
			else if (s_type == typeid(ShapeGroup)) {
				if (static_cast<ShapeGroup*>(s)->has_text()) {
					// �^��������}�`�̏ꍇ,
					// ������}�`�̐����C���N�������g����.
					text_cnt++;
				}
			}
			// �}�`�̑I���t���O���őO�ʂ̐}�`�̑I���t���O�Ɋi�[����.
			fore_selected = s->is_selected();
			if (fore_selected) {
				// �őO�ʂ̐}�`�̑I���t���O�������Ă���ꍇ,
				// �I�����ꂽ�}�`�̐����C���N�������g����.
				selected_cnt++;
				if (undeleted_cnt == 1) {
					// �����t���O���Ȃ��}�`�̐��� 1 �̏ꍇ,
					// �Ŕw�ʂ̐}�`�̑I���t���O�𗧂Ă�.
					back_selected = true;
				}
				if (s_type == typeid(ShapeGroup)) {
					// �}�`�̌^���O���[�v�}�`�̏ꍇ,
					// �I�����ꂽ�O���[�v�}�`�̐����C���N�������g����.
					selected_group_cnt++;
				}
				else if (s_type == typeid(ShapeText)) {
					// �}�`�̌^��������}�`�̏ꍇ,
					// �I�����ꂽ������}�`�̐����C���N�������g����.
					selected_text_cnt++;
				}
				if (prev_selected != true) {
					// �ЂƂw�ʂ̐}�`���k��
					// �܂��͂ЂƂw�ʂ̐}�`�̑I���t���O���Ȃ��ꍇ,
					// �I�����ꂽ�}�`�̃��������O�X�̐����C���N�������g����.
					runlength_cnt++;
				}
			}
			prev_selected = fore_selected;
		}

	}

	// �}�`�̃��X�g��ł̏��Ԃ𓾂�.
	// s_list	�}�`���X�g
	// s	�}�`
	// �߂�l	�}�`�̏���
	// �����t���O�������Ă���}�`�͏��Ԃɓ���Ȃ�.
	uint32_t s_list_distance(S_LIST_T const& s_list, const Shape* s) noexcept
	{
		uint32_t i = 0;
		for (auto t : s_list) {
			if (t->is_deleted()) {
				continue;
			}
			else if (t == s) {
				break;
			}
			i++;
		}
		return i;
	}

	// �ŏ��̐}�`�����X�g���瓾��.
	// s_list	�}�`���X�g
	// �߂�l	�ŏ��̐}�`
	// �����t���O�������Ă���}�`�͊��肳��Ȃ�.
	Shape* s_list_front(S_LIST_T const& s_list) noexcept
	{
		uint32_t _;
		return s_list_next(s_list.begin(), s_list.end(), _);
	}

	// �}�`���X�g�̒�����, �ʒu���܂ސ}�`�Ƃ��̕��ʂ𒲂ׂ�.
	// s_list	�}�`���X�g
	// t_pos	���ׂ�ʒu
	// a_len	���ʂ̑傫��
	// s	�ʒu���܂ސ}�`
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t s_list_hit_test(S_LIST_T const& s_list, const D2D1_POINT_2F t_pos, const double a_len, Shape*& s) noexcept
	{
		// �O�ʂɂ���}�`����Ƀq�b�g����悤��, ���X�g���t���Ɍ�������.
		for (auto it = s_list.rbegin(); it != s_list.rend(); it++) {
			auto t = *it;
			if (t->is_deleted()) {
				continue;
			}
			//if (!t->is_selected()) {
			//	continue;
			//}
			const auto a = t->hit_test(t_pos, a_len);
			if (a != ANCH_WHICH::ANCH_OUTSIDE) {
				s = t;
				return a;
			}
		}
		// �O�ʂɂ���}�`����Ƀq�b�g����悤��, ���X�g���t���Ɍ�������.
		/*
		for (auto it = s_list.rbegin(); it != s_list.rend(); it++) {
			auto t = *it;
			if (t->is_deleted()) {
				continue;
			}
			if (t->is_selected()) {
				continue;
			}
			const auto a = t->hit_test(t_pos, a_len);
			if (a != ANCH_WHICH::ANCH_OUTSIDE) {
				s = t;
				return a;
			}
		}
		*/
		return ANCH_WHICH::ANCH_OUTSIDE;
	}

	// �}�`�����X�g�ɑ}������.
	// s_list	�}�`���X�g
	// s	�}������}�`
	// s_pos	�}������ʒu�ɂ���}�`
	void s_list_insert(S_LIST_T& s_list, Shape* s, const Shape* s_pos) noexcept
	{
		s_list.insert(std::find(s_list.begin(), s_list.end(), s_pos), s);
	}

	// ���X�g�̒��̐}�`�̏��Ԃ𓾂�.
	// S	�T������^
	// T	����ꂽ�^
	// s_list	�}�`���X�g
	// s	�T������l
	// t	����ꂽ�l
	template <typename S, typename T>
	bool s_list_match(S_LIST_T const& s_list, S s, T& t)
	{
		// ��v�t���O
		bool match = false;
		// �T�����鏇�ɐ}�`�𐔂���v��.
		uint32_t j = 0;
		// �X�^�b�N�J�E���^.
		uint32_t k = 0;
		// ���X�g��[���D��ŒT�����邽�߂̃X�^�b�N
		std::list<S_LIST_T::const_iterator> stack;
		// �}�`���X�g�̊J�n�ƏI�[���X�^�b�N�Ƀv�b�V������.
		stack.push_back(s_list.begin());
		stack.push_back(s_list.end());
		k++;
		while (k-- != 0) {
			// �X�^�b�N�J�E���^�� 0 �łȂ�������ȉ����J��Ԃ�.
			// �����q�ƏI�[���X�^�b�N����|�b�v����.
			auto it_end = stack.back();
			stack.pop_back();
			auto it = stack.back();
			stack.pop_back();
			while (it != it_end) {
				// �����q���I�[�łȂ��Ȃ�, �ȉ����J��Ԃ�.
				// ���X�g�Ɋ܂܂��}�`�𔽕��q���瓾��.
				// �����q�����ɐi�߂�.
				auto r = *it++;
				if constexpr (std::is_same<S, Shape*>::value) {
					// ���� s ���}�`�̏ꍇ,
					// ���X�g�Ɋ܂܂��}�`�ƈ��� s ����v�����Ȃ�,
					// ���� t �ɓY�������i�[��, �X�^�b�N�J�E���^�� 0 �ɂ���.
					// �����q�̌J��Ԃ��𒆒f����.
					match = (r == s);
					if (match) {
						t = j;
						k = 0;
						break;
					}
				}
				if constexpr (std::is_same<S, uint32_t>::value) {
					// ���� s ���Y�����̏ꍇ,
					// �W���ƈ��� s ����v�����Ȃ�,
					// ���� t �ɐ}�`���i�[��, �X�^�b�N�J�E���^�� 0 �ɂ���.
					// �����q�̌J��Ԃ��𒆒f����.
					match = (j == s);
					if (match) {
						t = r;
						k = 0;
						break;
					}
				}
				j++;
				if (typeid(*r) == typeid(ShapeGroup)) {
					// ���X�g�Ɋ܂܂��}�`���O���[�v�}�`�Ȃ�,
					// ���̐}�`���w���Ă��锽���q�ƏI�[���X�^�b�N�Ƀv�b�V������.
					// �O���[�v�Ɋ܂܂�郊�X�g�̊J�n�ƏI�[���X�^�b�N�Ƀv�b�V������.
					// �����q�̌J��Ԃ��𒆒f����.
					stack.push_back(it);
					stack.push_back(it_end);
					k++;
					auto& grp_list = static_cast<ShapeGroup*>(r)->m_list_grouped;
					stack.push_back(grp_list.begin());
					stack.push_back(grp_list.end());
					k++;
					break;
				}
			}
		}
		stack.clear();
		return match;
	}
	template bool winrt::GraphPaper::implementation::s_list_match<Shape*, uint32_t>(S_LIST_T const& s_list, Shape* s, uint32_t& t);
	template bool winrt::GraphPaper::implementation::s_list_match<uint32_t, Shape*>(S_LIST_T const& s_list, uint32_t s, Shape*& t);

	// �I���t���O�̗����ׂĂ̐}�`�����������ړ�����.
	// s_list	�}�`���X�g
	// diff	�ړ����鍷��
	void s_list_move(S_LIST_T const& s_list, const D2D1_POINT_2F diff) noexcept
	{
		for (auto s : s_list) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() != true) {
				continue;
			}
			s->move(diff);
		}
	}

	// ���̐}�`�����X�g���瓾��.
	// s_list	�}�`���X�g
	// s	���O�̐}�`
	// �߂�l	����ꂽ�}�`
	Shape* s_list_next(S_LIST_T const& s_list, const Shape* s) noexcept
	{
		return s_list_next(s_list.begin(), s_list.end(), s);
	}

	// ���̐}�`�����X�g���瓾��.
	// it_begin	���X�g�̎n�[
	// it_end	���X�g�̏I�[
	// s	���O�̐}�`
	// �߂�l	���̐}�`, �k���Ȃ�Ύ��̐}�`�͂Ȃ�.
	template <typename T>
	static Shape* s_list_next(T const& it_begin, T const& it_end, const Shape* s) noexcept
	{
		auto it{ std::find(it_begin, it_end, s) };
		if (it != it_end) {
			uint32_t _;
			return s_list_next(++it, it_end, _);
		}
		return static_cast<Shape*>(nullptr);
	}
	template Shape* winrt::GraphPaper::implementation::s_list_next(S_LIST_T::iterator const& it_begin, S_LIST_T::iterator const& it_end, const Shape* s) noexcept;
	template Shape* winrt::GraphPaper::implementation::s_list_next(S_LIST_T::reverse_iterator const& it_rbegin, S_LIST_T::reverse_iterator const& it_rend, const Shape* s) noexcept;

	// ���̐}�`�Ƃ��̒��������X�g���瓾��.
	// it_begin	���X�g�̎n�[
	// it_end	���X�g�̏I�[
	// distance	���̐}�`�Ƃ̒���
	// �߂�l	���̐}�`, �k���Ȃ�Ύ��̐}�`�͂Ȃ�.
	template <typename T>
	static Shape* s_list_next(T const& it_begin, T const& it_end, uint32_t& distance) noexcept
	{
		uint32_t i = 0;
		for (auto it = it_begin; it != it_end; it++) {
			auto s = *it;
			if (s->is_deleted() != true) {
				distance = i;
				return s;
			}
			i++;
		}
		return static_cast<Shape*>(nullptr);
	}
	template Shape* winrt::GraphPaper::implementation::s_list_next(S_LIST_T::iterator const& it_begin, S_LIST_T::iterator const& it_end, uint32_t& distance) noexcept;
	template Shape* winrt::GraphPaper::implementation::s_list_next(S_LIST_T::reverse_iterator const& it_rbegin, S_LIST_T::reverse_iterator const& it_rend, uint32_t& distance) noexcept;

	// �O�̐}�`�����X�g���瓾��.
	Shape* s_list_prev(S_LIST_T const& s_list, const Shape* s) noexcept
	{
		return s_list_next(s_list.rbegin(), s_list.rend(), s);
	}

	// �}�`���X�g���f�[�^���[�_�[����ǂݍ���.
	bool s_list_read(S_LIST_T& s_list, DataReader const& dt_reader)
	{
		Shape* s;
		while ((s = s_list_read_shape(dt_reader)) != static_cast<Shape*>(nullptr)) {
			if (s == reinterpret_cast<Shape*>(-1)) {
				return false;
			}
			s_list.push_back(s);
		}
		return true;
	}

	// �f�[�^���[�_�[����}�`���쐬����.
	static Shape* s_list_read_shape(DataReader const& dt_reader)
	{
		if (dt_reader.UnconsumedBufferLength() < sizeof(uint32_t)) {
			return nullptr;
		}
		Shape* s = nullptr;
		auto s_type = dt_reader.ReadUInt32();
		if (s_type == SHAPE_NULL) {
		}
		else if (s_type == SHAPE_BEZI) {
			s = new ShapeBezi(dt_reader);
		}
		else if (s_type == SHAPE_ELLI) {
			s = new ShapeElli(dt_reader);
		}
		else if (s_type == SHAPE_LINE) {
			s = new ShapeLine(dt_reader);
		}
		else if (s_type == SHAPE_QUAD) {
			s = new ShapeQuad(dt_reader);
		}
		else if (s_type == SHAPE_RECT) {
			s = new ShapeRect(dt_reader);
		}
		else if (s_type == SHAPE_RRCT) {
			s = new ShapeRRect(dt_reader);
		}
		else if (s_type == SHAPE_TEXT) {
			s = new ShapeText(dt_reader);
		}
		else if (s_type == SHAPE_GROUP) {
			s = new ShapeGroup(dt_reader);
		}
		else if (s_type == SHAPE_RULER) {
			s = new ShapeRuler(dt_reader);
		}
		else {
			s = reinterpret_cast<Shape*>(-1);
		}
#if defined(_DEBUG)
		if (s != nullptr && s != reinterpret_cast<Shape*>(-1)) {
			debug_leak_cnt++;
		}
#endif
		return s;
	}

	// �}�`�����X�g����폜��, �폜�����}�`�̎��̐}�`�𓾂�.
	// s_list	�}�`���X�g.
	// s	�폜����}�`.
	// s_pos	�}�`���폜�����ʒu.
	// �}�`���폜�����ʒu�Ƃ�, �폜���ꂽ�}�`�̎��̈ʒu�ɂ������}�`�ւ̃|�C���^�[.
	// �폜���ꂽ�}�`�����X�g�����ɂ������Ƃ��̓k���|�C���^�[�ƂȂ�.
	Shape* s_list_remove(S_LIST_T& s_list, const Shape* s) noexcept
	{
		auto it{ std::find(s_list.begin(), s_list.end(), s) };
		if (it != s_list.end()) {
			(*it)->set_delete(true);
			auto it_next = std::next(it, 1);
			if (it_next != s_list.end()) {
				return *it_next;
			}
		}
		return static_cast<Shape*>(nullptr);
	}

	// �I�����ꂽ�}�`����, ������S�č��킹��������𓾂�.
	winrt::hstring s_list_selected_all_text(S_LIST_T const& s_list) noexcept
	{
		winrt::hstring text;
		for (auto s : s_list) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() != true) {
				continue;
			}
			wchar_t* w;
			if (s->get_text(w) != true) {
				continue;
			}
			if (text.empty()) {
				text = w;
			}
			else {
				text = text + L"\n" + w;
			}
		}
		return text;
	}

	// �I�����ꂽ�}�`�̃��X�g�𓾂�.
	// S	�}�`�̌^. Shape �Ȃ炷�ׂĂ̎��, ShapeGroup �Ȃ�O���[�v�}�`�̂�.
	// s_list	�}�`���X�g
	// t_list	����ꂽ���X�g
	template <typename S>
	void s_list_selected(S_LIST_T const& s_list, S_LIST_T& t_list) noexcept
	{
		for (auto s : s_list) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() != true) {
				continue;
			}
			if constexpr (std::is_same<S, ShapeGroup>::value) {
				if (typeid(*s) != typeid(S)) {
					continue;
				}
			}
			t_list.push_back(s);
		}
	}
	template void s_list_selected<Shape>(const S_LIST_T& s_list, S_LIST_T& t_list) noexcept;
	template void s_list_selected<ShapeGroup>(const S_LIST_T& s_list, S_LIST_T& t_list) noexcept;

	// �}�`���X�g���f�[�^���C�^�[�ɏ�������.
	// REDUCE	�����t���O�������Ă���}�`������.
	// s_list	�������ސ}�`���X�g
	// dt_writer	�f�[�^���C�^�[
	template<bool REDUCE>
	void s_list_write(S_LIST_T const& s_list, DataWriter const& dt_writer)
	{
		for (const auto s : s_list) {
			if constexpr (REDUCE) {
				if (s->is_deleted()) {
					continue;
				}
			}
			// �}�`�̎�ނ𓾂�.
			auto const& s_type = typeid(*s);
			uint32_t s_int;
			if (s_type == typeid(ShapeBezi)) {
				s_int = SHAPE_BEZI;
			}
			else if (s_type == typeid(ShapeElli)) {
				s_int = SHAPE_ELLI;
			}
			else if (s_type == typeid(ShapeGroup)) {
				s_int = SHAPE_GROUP;
			}
			else if (s_type == typeid(ShapeLine)) {
				s_int = SHAPE_LINE;
			}
			else if (s_type == typeid(ShapeQuad)) {
				s_int = SHAPE_QUAD;
			}
			else if (s_type == typeid(ShapeRect)) {
				s_int = SHAPE_RECT;
			}
			else if (s_type == typeid(ShapeRRect)) {
				s_int = SHAPE_RRCT;
			}
			else if (s_type == typeid(ShapeRuler)) {
				s_int = SHAPE_RULER;
			}
			else if (s_type == typeid(ShapeText)) {
				s_int = SHAPE_TEXT;
			}
			else {
				continue;
			}
			// �}�`�̎�ނƐ}�`����������.
			dt_writer.WriteUInt32(s_int);
			s->write(dt_writer);
		}
		// �I�[����������.
		dt_writer.WriteUInt32(static_cast<uint32_t>(SHAPE_NULL));
	}
	constexpr auto REDUCE = true;
	template void s_list_write<!REDUCE>(S_LIST_T const& s_list, DataWriter const& dt_writer);
	template void s_list_write<REDUCE>(S_LIST_T const& s_list, DataWriter const& dt_writer);

}