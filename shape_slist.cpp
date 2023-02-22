//------------------------------
// shape_slist.cpp
// �}�`���X�g
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	// �}�`�̎��
	// �t�@�C���ւ̏������݂Ŏg�p����.
	enum SHAPE_TYPE : uint32_t {
		SHAPE_NULL,	// �k��
		SHAPE_BEZIER,	// �Ȑ�
		SHAPE_IMAGE,	// �摜
		SHAPE_ELLIPSE,	// ���~
		SHAPE_GROUP,	// �O���[�v
		SHAPE_LINE,	// ����
		SHAPE_POLYGON,	// ���p�`
		SHAPE_RECT,	// ���`
		SHAPE_RRECT,	// �p�ە��`
		SHAPE_RULER,	// ��K
		SHAPE_TEXT,	// ������
		SHAPE_QELLIPSE	// �l���~
	};

	// �f�[�^���[�_�[����}�`��ǂݍ���.
	static Shape* slist_read_shape(const Shape& page, DataReader const& dt_reader);
	// ���̐}�`�𓾂�.
	template <typename T>
	static Shape* slist_next(T const& it_beg, T const& it_end, const Shape* s) noexcept;
	// ���̐}�`�Ƃ��̋����𓾂�.
	template <typename T>
	static Shape* slist_next(T const& it_beg, T const& it_end, uint32_t& distance) noexcept;

	// �}�`���X�g�̒��̕�����}�`��, ���p�ł��Ȃ����̂��������Ȃ�΂��̏��̖��𓾂�.
	// slist	�}�`���X�g
	// unavailable_font	���p�ł��Ȃ����̖�
	// �߂�l	���p�ł��Ȃ����̂��������Ȃ� true
	bool slist_test_avaiable_font(const SHAPE_LIST& slist, wchar_t*& unavailable_font) noexcept
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

	// �Ō�̐}�`�𓾂�.
	// slist	�}�`���X�g
	// �߂�l	����ꂽ�}�`
	Shape* slist_back(SHAPE_LIST const& slist) noexcept
	{
		uint32_t _;
		return slist_next(slist.rbegin(), slist.rend(), _);
	}

	// �}�`�ƕ\�����͂ޗ̈�𓾂�.
	// slist	�}�`���X�g
	// page_size	�\���̑傫��
	// b_lt	�̈�̍���ʒu
	// b_rb	�̈�̉E���ʒu
	void slist_bound_view(SHAPE_LIST const& slist, const D2D1_SIZE_F page_size, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept
	{
		b_lt = { 0.0F, 0.0F };	// ����ʒu
		b_rb = { page_size.width, page_size.height };	// �E���ʒu
		for (const Shape* s : slist) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_lt, b_rb, b_lt, b_rb);
		}
	}

	// ���ׂĂ̐}�`���͂ޗ̈�𓾂�.
	// slist	�}�`���X�g
	// b_lt	�̈�̍���ʒu
	// b_rb	�̈�̉E���ʒu
	void slist_bound_all(SHAPE_LIST const& slist, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept
	{
		b_lt = { FLT_MAX, FLT_MAX };	// ����ʒu
		b_rb = { -FLT_MAX, -FLT_MAX };	// �E���ʒu
		for (const Shape* s : slist) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_lt, b_rb, b_lt, b_rb);
		}
	}

	// �I�����ꂽ�}�`���͂ޗ̈�𓾂�.
	// slist	�}�`���X�g
	// b_lt	�̈�̍���ʒu
	// b_rb	�̈�̉E���ʒu
	bool slist_bound_selected(SHAPE_LIST const& slist, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept
	{
		bool done = false;
		b_lt = D2D1_POINT_2F{ FLT_MAX, FLT_MAX };	// ����ʒu
		b_rb = D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX };	// �E���ʒu
		for (const Shape* s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			s->get_bound(b_lt, b_rb, b_lt, b_rb);
			if (!done) {
				done = true;
			}
		}
		return done;
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
	// undeleted_cnt	�����t���O���Ȃ��}�`�̐�
	// selected_cnt	�I�����ꂽ�}�`�̐�
	// selected_group_cnt	�I�����ꂽ�O���[�v�}�`�̐�
	// runlength_cnt	�I�����ꂽ�}�`�̃��������O�X�̐�
	// selected_text_cnt	�I�����ꂽ������}�`�̐�
	// selected_image_cnt	�I�����ꂽ�摜�}�`�̐�
	// text_cnt	������}�`�̐�
	// fore_selected	�őO�ʂ̐}�`�̑I���t���O
	// back_selected	�Ŕw�ʂ̐}�`�̑I���t���O
	// prev_selected	�ЂƂw�ʂ̐}�`�̑I���t���O
	void slist_count(
		const SHAPE_LIST& slist,
		uint32_t& undeleted_cnt,
		uint32_t& selected_cnt,
		uint32_t& selected_group_cnt,
		uint32_t& runlength_cnt,
		uint32_t& selected_text_cnt,
		uint32_t& text_cnt,
		uint32_t& selected_image_cnt,
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
		selected_image_cnt = 0;
		text_cnt = 0;	// ������}�`�̐�
		fore_selected = false;	// �őO�ʂ̐}�`�̑I���t���O
		back_selected = false;	// �Ŕw�ʂ̐}�`�̑I���t���O
		prev_selected = false;	// �ЂƂw�ʂ̐}�`�̑I���t���O

		// �}�`���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
		for (auto s : slist) {
			// �}�`�̏����t���O�𔻒肷��.
			if (s->is_deleted()) {
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
				// �}�`�̌^���摜�����肷��.,
				if (s_type == typeid(ShapeImage)) {
					selected_image_cnt++;
				}
				else if (s_type == typeid(ShapeGroup)) {
					// �}�`�̌^���O���[�v�}�`�̏ꍇ,
					// �I�����ꂽ�O���[�v�}�`�̐����C���N�������g����.
					selected_group_cnt++;
				}
				else if (s_type == typeid(ShapeText)) {
					// �}�`�̌^��������}�`�̏ꍇ,
					// �I�����ꂽ������}�`�̐����C���N�������g����.
					selected_text_cnt++;
				}
				if (!prev_selected) {
					// �ЂƂw�ʂ̐}�`���k��
					// �܂��͂ЂƂw�ʂ̐}�`�̑I���t���O���Ȃ��ꍇ,
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

	// �ʒu���܂ސ}�`�Ƃ��̕��ʂ𓾂�.
	// slist	�}�`���X�g
	// t_pos	���肷��ʒu
	// s	�ʒu���܂ސ}�`
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t slist_hit_test(SHAPE_LIST const& slist, const D2D1_POINT_2F t_pos, Shape*& s) noexcept
	{
		// �O�ʂɂ���}�`����Ƀq�b�g����悤��, ���X�g���t���Ɍ�������.
		for (auto it = slist.rbegin(); it != slist.rend(); it++) {
			const auto t = *it;
			if (t->is_deleted()) {
				continue;
			}
			//if (!t->is_selected()) {
			//	continue;
			//}
			const uint32_t anc = t->hit_test(t_pos);
			if (anc != ANC_TYPE::ANC_PAGE) {
				s = t;
				return anc;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// �}�`��}������.
	// slist	�}�`���X�g
	// s	�}������}�`
	// s_at	�}������ʒu�ɂ���}�`
	void slist_insert(SHAPE_LIST& slist, Shape* const s_ins, const Shape* s_at) noexcept
	{
		slist.insert(std::find(slist.begin(), slist.end(), s_at), s_ins);
	}

	// ���X�g�̒��̐}�`�̏��Ԃ𓾂�.
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

	// �I���t���O�̗����ׂĂ̐}�`�����������ړ�����.
	// slist	�}�`���X�g
	// d_vec	�ړ����鍷��
	// �߂�l	�ړ������}�`������Ȃ� true
	bool slist_move(SHAPE_LIST const& slist, const D2D1_POINT_2F d_vec) noexcept
	{
		bool done = false;
		for (auto s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			if (s->move(d_vec) && !done) {
				done = true;
			}
		}
		return done;
	}

	// �}�`�̂��̎��̐}�`�����X�g���瓾��.
	// slist	�}�`���X�g
	// s	�}�`
	// �߂�l	���̎��̐}�`
	Shape* slist_next(SHAPE_LIST const& slist, const Shape* s) noexcept
	{
		return slist_next(slist.begin(), slist.end(), s);
	}

	// �}�`��, ���̐}�`�����X�g���瓾��.
	// it_beg	���X�g�̎n�[
	// it_end	���X�g�̏I�[
	// s	�}�`
	// �߂�l	���̐}�`. �k���Ȃ�Ύ��̐}�`�͂Ȃ�.
	template <typename T> static Shape* slist_next(T const& it_beg, T const& it_end, const Shape* s) noexcept
	{
		auto it{ std::find(it_beg, it_end, s) };
		if (it != it_end) {
			uint32_t _;
			return slist_next(++it, it_end, _);
		}
		return static_cast<Shape*>(nullptr);
	}
	template Shape* winrt::GraphPaper::implementation::slist_next(SHAPE_LIST::iterator const& it_beg, SHAPE_LIST::iterator const& it_end, const Shape* s) noexcept;
	template Shape* winrt::GraphPaper::implementation::slist_next(SHAPE_LIST::reverse_iterator const& it_rbeg, SHAPE_LIST::reverse_iterator const& it_rend, const Shape* s) noexcept;

	// �}�`�̎��̐}�`��, ���̊Ԋu�����X�g���瓾��.
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
	template Shape* winrt::GraphPaper::implementation::slist_next(SHAPE_LIST::iterator const& it_beg, SHAPE_LIST::iterator const& it_end, uint32_t& distance) noexcept;
	template Shape* winrt::GraphPaper::implementation::slist_next(SHAPE_LIST::reverse_iterator const& it_rbeg, SHAPE_LIST::reverse_iterator const& it_rend, uint32_t& distance) noexcept;

	// �O�̐}�`�����X�g���瓾��.
	Shape* slist_prev(SHAPE_LIST const& slist, const Shape* s) noexcept
	{
		return slist_next(slist.rbegin(), slist.rend(), s);
	}

	// �f�[�^���[�_�[����}�`���X�g��ǂݍ���.
	bool slist_read(SHAPE_LIST& slist, const Shape& page, DataReader const& dt_reader)
	{
		Shape* s;
		while ((s = slist_read_shape(page, dt_reader)) != static_cast<Shape*>(nullptr)) {
			if (s == reinterpret_cast<Shape*>(-1)) {
				return false;
			}
			slist.push_back(s);
		}
		return true;
	}

	// �f�[�^���[�_�[����}�`��ǂݍ���.
	static Shape* slist_read_shape(const Shape& page, DataReader const& dt_reader)
	{
		if (dt_reader.UnconsumedBufferLength() < sizeof(uint32_t)) {
			return nullptr;
		}
		Shape* s = nullptr;
		auto s_type = dt_reader.ReadUInt32();
		if (s_type == SHAPE_TYPE::SHAPE_NULL) {
		}
		else if (s_type == SHAPE_TYPE::SHAPE_BEZIER) {
			s = new ShapeBezier(page, dt_reader);
		}
		else if (s_type == SHAPE_TYPE::SHAPE_IMAGE) {
			s = new ShapeImage(dt_reader);
		}
		else if (s_type == SHAPE_TYPE::SHAPE_ELLIPSE) {
			s = new ShapeEllipse(page, dt_reader);
		}
		else if (s_type == SHAPE_TYPE::SHAPE_LINE) {
			s = new ShapeLine(page, dt_reader);
		}
		else if (s_type == SHAPE_TYPE::SHAPE_POLYGON) {
			s = new ShapePolygon(page, dt_reader);
		}
		else if (s_type == SHAPE_TYPE::SHAPE_RECT) {
			s = new ShapeRect(page, dt_reader);
		}
		else if (s_type == SHAPE_TYPE::SHAPE_RRECT) {
			s = new ShapeRRect(page, dt_reader);
		}
		else if (s_type == SHAPE_TYPE::SHAPE_TEXT) {
			s = new ShapeText(page, dt_reader);
		}
		else if (s_type == SHAPE_TYPE::SHAPE_GROUP) {
			s = new ShapeGroup(page, dt_reader);
		}
		else if (s_type == SHAPE_TYPE::SHAPE_RULER) {
			s = new ShapeRuler(page, dt_reader);
		}
		else if (s_type == SHAPE_TYPE::SHAPE_QELLIPSE) {
			s = new ShapeQEllipse(page, dt_reader);
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
			auto const& s_type = typeid(*s);
			uint32_t s_int;
			if (s_type == typeid(ShapeQEllipse)) {
				s_int = SHAPE_TYPE::SHAPE_QELLIPSE;
			}
			else if (s_type == typeid(ShapeBezier)) {
				s_int = SHAPE_TYPE::SHAPE_BEZIER;
			}
			else if (s_type == typeid(ShapeImage)) {
				s_int = SHAPE_TYPE::SHAPE_IMAGE;
			}
			else if (s_type == typeid(ShapeEllipse)) {
				s_int = SHAPE_TYPE::SHAPE_ELLIPSE;
			}
			else if (s_type == typeid(ShapeGroup)) {
				s_int = SHAPE_TYPE::SHAPE_GROUP;
			}
			else if (s_type == typeid(ShapeLine)) {
				s_int = SHAPE_TYPE::SHAPE_LINE;
			}
			else if (s_type == typeid(ShapePolygon)) {
				s_int = SHAPE_TYPE::SHAPE_POLYGON;
			}
			else if (s_type == typeid(ShapeRect)) {
				s_int = SHAPE_TYPE::SHAPE_RECT;
			}
			else if (s_type == typeid(ShapeRRect)) {
				s_int = SHAPE_TYPE::SHAPE_RRECT;
			}
			else if (s_type == typeid(ShapeRuler)) {
				s_int = SHAPE_TYPE::SHAPE_RULER;
			}
			else if (s_type == typeid(ShapeText)) {
				s_int = SHAPE_TYPE::SHAPE_TEXT;
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
	// �������ꂽ�}�`���܂߂�, �}�`���X�g����������.
	template void slist_write<!REDUCE>(SHAPE_LIST const& slist, DataWriter const& dt_writer);
	// �������ꂽ�}�`�͏Ȃ���, �}�`���X�g����������.
	template void slist_write<REDUCE>(SHAPE_LIST const& slist, DataWriter const& dt_writer);

	// �I������ĂȂ��}�`����, �w�肵�������ȉ���, �w�肵���ʒu�ɍł��߂����_�𓾂�.
	// slist	�}�`���X�g
	// p	�ʒu
	// d	����
	// v	�ł��߂����_
	bool slist_find_vertex_closest(const SHAPE_LIST& slist, const D2D1_POINT_2F& p, const float d, D2D1_POINT_2F& v) noexcept
	{
		bool flag = false;	// ���_�����������ǂ����̃t���O
		float dd = d * d;	// �����̓��
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