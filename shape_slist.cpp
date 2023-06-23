//------------------------------
// shape_slist.cpp
// �}�`���X�g
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �}�`�̎��
	// �t�@�C���ւ̏������݂Ŏg�p����.
	enum SHAPE_T : uint32_t {
		SHAPE_NULL,	// �k��
		SHAPE_ARC,	// �~��
		SHAPE_BEZIER,	// �Ȑ�
		SHAPE_ELLIPSE,	// ���~
		SHAPE_GROUP,	// �O���[�v
		SHAPE_IMAGE,	// �摜
		SHAPE_LINE,	// ����
		SHAPE_POLY,	// ���p�`
		SHAPE_RECT,	// ���`
		SHAPE_RRECT,	// �p�ە��`
		SHAPE_RULER,	// ��K
		SHAPE_TEXT,	// ������
	};

	// �f�[�^���[�_�[����}�`��ǂݍ���.
	static Shape* slist_read_shape(DataReader const& dt_reader);
	// ���X�g���̐}�`�̂��̎��̐}�`�𓾂�.
	template <typename T>
	static Shape* slist_next(T const& it_beg, T const& it_end, const Shape* s) noexcept;
	// ���X�g���̐}�`�̂��̎��̐}�`��, ���̊Ԋu�����X�g���瓾��.
	template <typename T>
	static Shape* slist_next(T const& it_beg, T const& it_end, uint32_t& distance) noexcept;

	// ���X�g�̒��̕�����}�`��, ���p�ł��Ȃ����̂��������Ȃ�΂��̏��̖��𓾂�.
	// slist	�}�`���X�g
	// unavailable_font	���p�ł��Ȃ����̖�
	// �߂�l	���p�ł��Ȃ����̂��������Ȃ� true
	bool slist_check_avaiable_font(const SHAPE_LIST& slist, wchar_t*& unavailable_font) noexcept
	{
		for (const Shape* s : slist) {
			if (s->is_deleted()) {
				continue;
			}
			wchar_t* fam;	// ���̖�
			if (!s->get_font_family(fam)) {
				continue;
			}
			// �󕶎���͊���̏��̖��Ƃ��Ă�������.
			if (wchar_len(fam) > 0 && !ShapeText::is_available_font(fam)) {
				unavailable_font = fam;
				return false;
			}
		}
		return true;
	}

	// ���X�g���̍Ō�̐}�`�𓾂�.
	// slist	�}�`���X�g
	// �߂�l	����ꂽ�}�`
	Shape* slist_back(SHAPE_LIST const& slist) noexcept
	{
		uint32_t _;
		return slist_next(slist.rbegin(), slist.rend(), _);
	}

	// ���X�g���̐}�`�̋��E��`�𓾂�.
	// slist	�}�`���X�g
	// b_lt	�̈�̍���_
	// b_rb	�̈�̉E���_
	void slist_bbox_shape(SHAPE_LIST const& slist, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept
	{
		b_lt = { FLT_MAX, FLT_MAX };	// ����_
		b_rb = { -FLT_MAX, -FLT_MAX };	// �E���_
		for (const Shape* s : slist) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bbox(b_lt, b_rb, b_lt, b_rb);
		}
	}

	// ���X�g���̑I�����ꂽ�}�`�̋��E��`�𓾂�.
	// slist	�}�`���X�g
	// b_lt	�̈�̍���_
	// b_rb	�̈�̉E���_
	// �߂�l	�I�����ꂽ�}�`���������Ȃ� true.
	bool slist_bbox_selected(SHAPE_LIST const& slist, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept
	{
		bool flag = false;
		b_lt = D2D1_POINT_2F{ FLT_MAX, FLT_MAX };	// ����_
		b_rb = D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX };	// �E���_
		for (const Shape* s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			s->get_bbox(b_lt, b_rb, b_lt, b_rb);
			flag = true;
		}
		return flag;
	}

	// �}�`���X�g��������, �܂܂��}�`��j������.
	// slist	�}�`���X�g
	void slist_clear(SHAPE_LIST& slist) noexcept
	{
		for (auto s : slist) {
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		slist.clear();
	}

	// �}�`����ޕʂɐ�����.
	void slist_count(const SHAPE_LIST& slist, uint32_t& selected_cnt, uint32_t& runlength_cnt, bool& fore_selected, bool& back_selected) noexcept
	{
		uint32_t undeleted_cnt = 0;
		bool prev_selected = false;

		selected_cnt = 0;
		runlength_cnt = 0;
		fore_selected = false;

		// �}�`���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
		for (const auto s : slist) {
			// �}�`�̏����t���O�𔻒肷��.
			if (s->is_deleted()) {
				continue;
			}
			// �����t���O���Ȃ��}�`�̐����C���N�������g����.
			undeleted_cnt++;
			// �}�`�̑I���t���O���őO�ʂ̐}�`�̑I���t���O�Ɋi�[����.
			fore_selected = s->is_selected();
			// �őO�ʂ̐}�`�̑I���t���O�������Ă���ꍇ,
			if (fore_selected) {
				// �I�����ꂽ�}�`�̐����C���N�������g����.
				selected_cnt++;
				// �����t���O���Ȃ��}�`�̐��� 1 �̏ꍇ,
				if (undeleted_cnt == 1) {
					// �Ŕw�ʂ̐}�`�̑I���t���O�𗧂Ă�.
					back_selected = true;
				}
				// �ЂƂw�ʂ̐}�`���k���܂��͑I������ĂȂ��ꍇ,
				if (!prev_selected) {
					// �I�����ꂽ�}�`�̃��������O�X�̐����C���N�������g����.
					runlength_cnt++;
				}
			}
			prev_selected = fore_selected;
		}
	}

	// �}�`����ޕʂɐ�����.
	// undeleted_cnt	�����t���O���Ȃ��}�`�̐�
	// selected_cnt	�I�����ꂽ�}�`�̐�
	// selected_group_cnt	�I�����ꂽ�O���[�v�}�`�̐�
	// runlength_cnt	�I�����ꂽ�}�`�̃��������O�X�̐�
	// selected_text_cnt	�I�����ꂽ������}�`�̐�
	// text_cnt	������}�`�̐�
	// selecred_line_cnt	�I�����ꂽ�����̐�
	// selected_image_cnt	�I�����ꂽ�摜�̐�
	// selected_arc_cnt	�I�����ꂽ�~�ʂ̐�
	// fore_selected	�őO�ʂ̐}�`�̑I���t���O
	// back_selected	�Ŕw�ʂ̐}�`�̑I���t���O
	// prev_selected	�ЂƂw�ʂ̐}�`�̑I���t���O
	void slist_count(
		const SHAPE_LIST& slist, uint32_t& undeleted_cnt, uint32_t& selected_cnt,
		uint32_t& selected_group_cnt, uint32_t& runlength_cnt, uint32_t& selected_text_cnt,
		uint32_t& text_cnt, uint32_t& selected_line_cnt, uint32_t& selected_image_cnt, uint32_t& selected_ruler_cnt,
		uint32_t& selected_clockwise, uint32_t& selected_counter_clockwise,
		uint32_t& selected_poly_open_cnt, uint32_t& selected_poly_close_cnt, uint32_t& selected_exist_cap_cnt,
		bool& fore_selected,
		bool& back_selected/*, bool& prev_selected*/) noexcept
	{
		undeleted_cnt = 0;	// �����t���O���Ȃ��}�`�̐�
		selected_cnt = 0;	// �I�����ꂽ�}�`�̐�
		selected_group_cnt = 0;	// �I�����ꂽ�O���[�v�}�`�̐�
		runlength_cnt = 0;	// �I�����ꂽ�}�`�̃��������O�X�̐�
		selected_text_cnt = 0;	// �I�����ꂽ������}�`�̐�
		selected_line_cnt = 0;	// �I�����ꂽ�����̐�
		selected_image_cnt = 0;	// �I�����ꂽ�摜�̐�
		selected_ruler_cnt = 0;	// �I�����ꂽ��K�̐�
		selected_poly_open_cnt = 0;	// �I�����ꂽ�J�������p�`�̐�
		selected_poly_close_cnt = 0;	// �I�����ꂽ�������p�`�̐�
		selected_clockwise = 0;	// �I�����ꂽ�~�ʐ}�`�̐�
		selected_counter_clockwise = 0;	// �I�����ꂽ�~�ʐ}�`�̐�
		selected_exist_cap_cnt = 0;	// �I�����ꂽ�[�����}�`�̐�
		text_cnt = 0;	// ������}�`�̐�
		fore_selected = false;	// �őO�ʂ̐}�`�̑I���t���O
		back_selected = false;	// �Ŕw�ʂ̐}�`�̑I���t���O
		bool prev_selected = false;	// �ЂƂw�ʂ̐}�`�̑I���t���O

		// �}�`���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
		for (auto s : slist) {
			// �}�`�̏����t���O�𔻒肷��.
			if (s->is_deleted()) {
				continue;
			}
			// �����t���O���Ȃ��}�`�̐����C���N�������g����.
			undeleted_cnt++;
			// �}�`�̓��I�Ȍ^�𓾂�.
			auto const& tid = typeid(*s);
			if (tid == typeid(ShapeText)) {
				// �^��������}�`�̏ꍇ,
				// ������}�`�̐����C���N�������g����.
				text_cnt++;
			}
			else if (tid == typeid(ShapeGroup)) {
				if (static_cast<ShapeGroup*>(s)->has_text()) {
					// �^��������}�`�̏ꍇ,
					// ������}�`�̐����C���N�������g����.
					text_cnt++;
				}
			}
			// �}�`�̑I���t���O���őO�ʂ̐}�`�̑I���t���O�Ɋi�[����.
			fore_selected = s->is_selected();
			// �őO�ʂ̐}�`�̑I���t���O�������Ă���ꍇ,
			if (fore_selected) {
				// �I�����ꂽ�}�`�̐����C���N�������g����.
				selected_cnt++;
				// �����t���O���Ȃ��}�`�̐��� 1 �̏ꍇ,
				if (undeleted_cnt == 1) {
					// �Ŕw�ʂ̐}�`�̑I���t���O�𗧂Ă�.
					back_selected = true;
				}
				// �[�����}�`�����肷��.
				if (s->exist_cap()) {
					selected_exist_cap_cnt++;
				}
				// �}�`�̌^���摜�����肷��.,
				if (tid == typeid(ShapeImage)) {
					selected_image_cnt++;
				}
				// �}�`�̌^���摜�����肷��.,
				else if (tid == typeid(ShapeLine)) {
					selected_line_cnt++;
				}
				// �}�`�̌^���摜�����肷��.,
				else if (tid == typeid(ShapeArc)) {
					D2D1_SWEEP_DIRECTION dir;
					s->get_arc_dir(dir);
					if (dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE) {
						selected_counter_clockwise++;
					}
					else {
						selected_clockwise++;
					}
				}
				// �}�`�̌^���O���[�v�}�`�����肷��.,
				else if (tid == typeid(ShapeGroup)) {
					// �I�����ꂽ�O���[�v�}�`�̐����C���N�������g����.
					selected_group_cnt++;
				}
				else if (tid == typeid(ShapePoly)) {
					// �}�`�̌^�����p�`�}�`�̏ꍇ,
					D2D1_FIGURE_END end;
					s->get_poly_end(end);
					if (end == D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED) {
						// �I�����ꂽ�������p�`�}�`�̐����C���N�������g����.
						selected_poly_close_cnt++;
					}
					else {
						// �I�����ꂽ�J�������p�`�}�`�̐����C���N�������g����.
						selected_poly_open_cnt++;
					}
				}
				// �}�`�̌^��������}�`�̏ꍇ,
				else if (tid == typeid(ShapeText)) {
					// �I�����ꂽ������}�`�̐����C���N�������g����.
					selected_text_cnt++;
				}
				// �}�`�̌^��������}�`�̏ꍇ,
				else if (tid == typeid(ShapeRuler)) {
					// �I�����ꂽ������}�`�̐����C���N�������g����.
					selected_ruler_cnt++;
				}
				// �ЂƂw�ʂ̐}�`���k���܂��͑I������ĂȂ��ꍇ,
				if (!prev_selected) {
					// �I�����ꂽ�}�`�̃��������O�X�̐����C���N�������g����.
					runlength_cnt++;
				}
			}
			prev_selected = fore_selected;
		}

	}

	// �擪����}�`�܂Ő�����.
	// slist	�}�`���X�g
	// s	�}�`
	// �߂�l	�}�`�̏���
	// �����t���O�������Ă���}�`�͏��Ԃɓ���Ȃ�.
	uint32_t slist_count(SHAPE_LIST const& slist, const Shape* s) noexcept
	{
		uint32_t i = 0;
		for (auto t : slist) {
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

	// �ŏ��̐}�`�𓾂�.
	// slist	�}�`���X�g
	// �߂�l	�ŏ��̐}�`
	// �����t���O�������Ă���}�`�͊��肳��Ȃ�.
	Shape* slist_front(SHAPE_LIST const& slist) noexcept
	{
		uint32_t _;
		return slist_next(slist.begin(), slist.end(), _);
	}

	// ���X�g���̐}�`���_���܂ނ����肷��.
	// slist	�}�`���X�g
	// pt	���肳���_
	// s	�_���܂ސ}�`
	// �߂�l	�_���܂ސ}�`�̕���
	uint32_t slist_hit_test(SHAPE_LIST const& slist, const D2D1_POINT_2F pt, const bool ctrl_key, Shape*& s) noexcept
	{
		// �O�ʂɂ���}�`����Ƀq�b�g����悤��, ���X�g���t���Ɍ�������.
		for (auto it = slist.rbegin(); it != slist.rend(); it++) {
			const auto t = *it;
			if (t->is_deleted()) {
				continue;
			}
			const uint32_t loc = t->hit_test(pt, ctrl_key);
			if (loc != LOCUS_TYPE::LOCUS_SHEET) {
				s = t;
				return loc;
			}
		}
		return LOCUS_TYPE::LOCUS_SHEET;
	}

	// ���X�g�ɐ}�`��}������.
	// slist	�}�`���X�g
	// s_ins	�}�������}�`
	// s_at	�}������ʒu�ɂ���}�`
	void slist_insert(SHAPE_LIST& slist, Shape* const s_ins, const Shape* s_at) noexcept
	{
		slist.insert(std::find(slist.begin(), slist.end(), s_at), s_ins);
	}

	// ���X�g���̐}�`�̏��Ԃ𓾂�.
	// S	�T������^
	// T	����ꂽ�^
	// slist	�}�`���X�g
	// s	�T������l
	// t	����ꂽ�l
	template <typename S, typename T> bool slist_match(SHAPE_LIST const& slist, S s, T& t)
	{
		bool f = false;	// ��v�t���O
		uint32_t j = 0;	// �T�����鏇�ɐ}�`�𐔂���Y����.
		uint32_t k = 0;	// �X�^�b�N�J�E���^.
		std::list<SHAPE_LIST::const_iterator> stack;	// ���X�g��[���D��ŒT�����邽�߂̃X�^�b�N
		// �}�`���X�g�̊J�n�ƏI�[���X�^�b�N�Ƀv�b�V������.
		stack.push_back(slist.begin());
		stack.push_back(slist.end());
		k++;
		while (k-- != 0) {
			// �X�^�b�N�J�E���^�� 0 �łȂ�������ȉ����J��Ԃ�.
			// �����q�ƏI�[���X�^�b�N����|�b�v����.
			SHAPE_LIST::const_iterator it_end = stack.back();
			stack.pop_back();
			SHAPE_LIST::const_iterator it = stack.back();
			stack.pop_back();
			while (it != it_end) {
				// �����q���I�[�łȂ��Ȃ�, �ȉ����J��Ԃ�.
				// ���X�g�Ɋ܂܂��}�`�𔽕��q���瓾��.
				// �����q�����ɐi�߂�.
				auto r = *it++;	// ���X�g�Ɋ܂܂��}�`
				if constexpr (std::is_same<S, Shape* const>::value) {
					// ���� s ���}�`�̏ꍇ,
					// ���X�g�Ɋ܂܂��}�`�ƈ��� s ����v�����Ȃ�,
					// �t���O�𗧂Ă�.
					// ���� t �ɓY�������i�[��, �X�^�b�N�J�E���^�� 0 �ɂ���.
					// �����q�̌J��Ԃ��𒆒f����.
					f = (r == s);
					if (f) {
						t = j;
						k = 0;
						break;
					}
				}
				if constexpr (std::is_same<S, const uint32_t>::value) {
					// ���� s ���Y�����̏ꍇ,
					// �W���ƈ��� s ����v�����Ȃ�, �t���O�𗧂Ă�.
					// ���� t �ɐ}�`���i�[��, �X�^�b�N�J�E���^�� 0 �ɂ���.
					// �����q�̌J��Ԃ��𒆒f����.
					f = (j == s);
					if (f) {
						t = r;
						k = 0;
						break;
					}
				}
				// �Y�������ЂƂ��Z.
				j++;
				if (typeid(*r) == typeid(ShapeGroup)) {
					// ���X�g�Ɋ܂܂��}�`���O���[�v�}�`�Ȃ�,
					// ���̐}�`���w���Ă��锽���q�ƏI�[���X�^�b�N�Ƀv�b�V������.
					// �O���[�v�Ɋ܂܂�郊�X�g�̊J�n�ƏI�[���X�^�b�N�Ƀv�b�V������.
					// �����q�̌J��Ԃ��𒆒f����.
					stack.push_back(it);
					stack.push_back(it_end);
					k++;
					auto& glist = static_cast<ShapeGroup*>(r)->m_list_grouped;
					stack.push_back(glist.begin());
					stack.push_back(glist.end());
					k++;
					break;
				}
			}
		}
		stack.clear();
		return f;
	}
	template bool winrt::GraphPaper::implementation::slist_match<Shape* const, uint32_t>(SHAPE_LIST const& slist, Shape* const s, uint32_t& t);
	template bool winrt::GraphPaper::implementation::slist_match<const uint32_t, Shape*>(SHAPE_LIST const& slist, const uint32_t s, Shape*& t);

	// �I�����ꂽ�}�`���ړ�����.
	// slist	�}�`���X�g
	// pos	�ʒu�x�N�g��
	// �߂�l	�ړ������}�`������Ȃ� true
	bool slist_move_selected(SHAPE_LIST const& slist, const D2D1_POINT_2F pos) noexcept
	{
		bool flag = false;
		for (auto s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			if (s->move(pos)) {
				flag = true;
			}
		}
		return flag;
	}

	// ���X�g���̐}�`�̂��̎��̐}�`�𓾂�.
	// slist	�}�`���X�g
	// s	�}�`
	// �߂�l	���̎��̐}�`
	Shape* slist_next(SHAPE_LIST const& slist, const Shape* s) noexcept
	{
		return slist_next(slist.begin(), slist.end(), s);
	}

	// ���X�g���̐}�`�̂��̎��̐}�`�𓾂�.
	// it_beg	���X�g�̎n�[
	// it_end	���X�g�̏I�[
	// s	�}�`
	// �߂�l	���̐}�`. �k���Ȃ�Ύ��̐}�`�͂Ȃ�.
	template <typename T>
	static Shape* slist_next(T const& it_beg, T const& it_end, const Shape* s) noexcept
	{
		auto it{ std::find(it_beg, it_end, s) };
		if (it != it_end) {
			uint32_t _;
			return slist_next(++it, it_end, _);
		}
		return static_cast<Shape*>(nullptr);
	}
	template Shape* winrt::GraphPaper::implementation::slist_next(
		SHAPE_LIST::iterator const& it_beg, SHAPE_LIST::iterator const& it_end, const Shape* s) noexcept;
	template Shape* winrt::GraphPaper::implementation::slist_next(
		SHAPE_LIST::reverse_iterator const& it_rbeg, SHAPE_LIST::reverse_iterator const& it_rend, const Shape* s) noexcept;

	// ���X�g���̐}�`�̂��̎��̐}�`��, ���̊Ԋu�����X�g���瓾��.
	// it_beg	���X�g�̎n�[
	// it_end	���X�g�̏I�[
	// distance	���̐}�`�Ƃ̊Ԋu
	// �߂�l	���̐}�`, �k���Ȃ�Ύ��̐}�`�͂Ȃ�.
	template <typename T>
	static Shape* slist_next(T const& it_beg, T const& it_end, uint32_t& distance) noexcept
	{
		uint32_t i = 0;
		for (auto it = it_beg; it != it_end; it++) {
			auto s = *it;
			if (!s->is_deleted()) {
				distance = i;
				return s;
			}
			i++;
		}
		return static_cast<Shape*>(nullptr);
	}
	template Shape* winrt::GraphPaper::implementation::slist_next(
		SHAPE_LIST::iterator const& it_beg, SHAPE_LIST::iterator const& it_end, uint32_t& distance) noexcept;
	template Shape* winrt::GraphPaper::implementation::slist_next(
		SHAPE_LIST::reverse_iterator const& it_rbeg, SHAPE_LIST::reverse_iterator const& it_rend, uint32_t& distance) noexcept;

	// �O�̐}�`�����X�g���瓾��.
	Shape* slist_prev(SHAPE_LIST const& slist, const Shape* s) noexcept
	{
		return slist_next(slist.rbegin(), slist.rend(), s);
	}

	// �f�[�^���[�_�[����}�`���X�g��ǂݍ���.
	bool slist_read(SHAPE_LIST& slist, DataReader const& dt_reader)
	{
		Shape* s;
		while ((s = slist_read_shape(dt_reader)) != static_cast<Shape*>(nullptr)) {
			if (s == reinterpret_cast<Shape*>(-1)) {
				return false;
			}
			slist.push_back(s);
		}
		return true;
	}

	// �f�[�^���[�_�[����}�`��ǂݍ���.
	static Shape* slist_read_shape(DataReader const& dt_reader)
	{
		if (dt_reader.UnconsumedBufferLength() < sizeof(uint32_t)) {
			return nullptr;
		}
		Shape* s = nullptr;
		auto s_tid = dt_reader.ReadUInt32();
		if (s_tid == SHAPE_T::SHAPE_NULL) {
		}
		else if (s_tid == SHAPE_T::SHAPE_BEZIER) {
			s = new ShapeBezier(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_IMAGE) {
			s = new ShapeImage(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_ELLIPSE) {
			s = new ShapeEllipse(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_LINE) {
			s = new ShapeLine(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_POLY) {
			s = new ShapePoly(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_RECT) {
			s = new ShapeRect(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_RRECT) {
			s = new ShapeRRect(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_TEXT) {
			s = new ShapeText(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_GROUP) {
			s = new ShapeGroup(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_RULER) {
			s = new ShapeRuler(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_ARC) {
			s = new ShapeArc(dt_reader);
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
	// slist	�}�`���X�g.
	// s	�}�`.
	// �߂�l	�}�`�̎��ɂ������}�`. �}�`�����X�g�����ɂ������Ƃ��̓k���|�C���^�[.
	/*
	Shape* slist_remove(SHAPE_LIST& slist, const Shape* s) noexcept
	{
		auto it{ std::find(slist.begin(), slist.end(), s) };
		if (it != slist.end()) {
			(*it)->set_delete(true);
			auto it_next = std::next(it, 1);
			if (it_next != slist.end()) {
				return *it_next;
			}
		}
		return static_cast<Shape*>(nullptr);
	}
	*/

	// �I�����ꂽ�}�`�̃��X�g�𓾂�.
	// T	�}�`�̌^. Shape �Ȃ炷�ׂĂ̎��, ShapeGroup �Ȃ�O���[�v�}�`�̂�.
	// slist	�}�`���X�g
	// t_list	����ꂽ���X�g
	template <typename T> void slist_get_selected(const SHAPE_LIST& slist, SHAPE_LIST& t_list) noexcept
	{
		for (auto s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			if constexpr (std::is_same<T, ShapeGroup>::value) {
				if (typeid(*s) != typeid(T)) {
					continue;
				}
			}
			t_list.push_back(s);
		}
	}
	template void slist_get_selected<Shape>(const SHAPE_LIST& slist, SHAPE_LIST& t_list) noexcept;
	template void slist_get_selected<ShapeGroup>(const SHAPE_LIST& slist, SHAPE_LIST& t_list) noexcept;

	// �f�[�^���C�^�[�ɐ}�`���X�g����������.
	// REDUCE	�������ꂽ�}�`�͏Ȃ�.
	// slist	�}�`���X�g
	// dt_writer	�f�[�^���C�^�[
	template<bool REDUCE> void slist_write(SHAPE_LIST const& slist, DataWriter const& dt_writer)
	{
		for (const auto s : slist) {
			if constexpr (REDUCE) {
				if (s->is_deleted()) {
					continue;
				}
			}
			auto const& s_tid = typeid(*s);
			if (s_tid == typeid(ShapeArc)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_ARC);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeBezier)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_BEZIER);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeEllipse)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_ELLIPSE);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeGroup)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_GROUP);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeImage)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_IMAGE);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeLine)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_LINE);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapePoly)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_POLY);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeRect)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_RECT);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeRRect)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_RRECT);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeRuler)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_RULER);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeText)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_TEXT);
				s->write(dt_writer);
			}
		}
		// �I�[����������.
		dt_writer.WriteUInt32(static_cast<uint32_t>(SHAPE_NULL));
	}
	constexpr auto REDUCE = true;
	// �������ꂽ�}�`���܂߂�, �}�`���X�g����������.
	template void slist_write<!REDUCE>(SHAPE_LIST const& slist, DataWriter const& dt_writer);
	// �������ꂽ�}�`�͏Ȃ���, �}�`���X�g����������.
	template void slist_write<REDUCE>(SHAPE_LIST const& slist, DataWriter const& dt_writer);

	// �I������ĂȂ��}�`����, �w�肵�������ȉ���, �w�肵���_�ɍł��߂��_�𓾂�.
	// slist	�}�`���X�g
	// p	�w�肵���_
	// d	�w�肵������
	// v	�ł��߂����_
	bool slist_find_vertex_closest(
		const SHAPE_LIST& slist,
		const D2D1_POINT_2F& p,
		const double d, 
		D2D1_POINT_2F& v
	) noexcept
	{
		bool flag = false;	// ���_�����������ǂ����̃t���O
		double dd = d * d;	// �����̓��
		for (const auto s : slist) {
			if (s->is_deleted() || s->is_selected()) {
				continue;
			}
			if (s->get_pos_nearest(p, dd, v) && !flag) {
				flag = true;
			}
		}
		return flag;
	}

}