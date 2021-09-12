//------------------------------
// Shape_group.cpp
// �O���[�v�}�`
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
	// �}�`��j������.
	ShapeGroup::~ShapeGroup(void)
	{
		slist_clear(m_list_grouped);
	}

	// �}�`��\������.
	void ShapeGroup::draw(SHAPE_DX& dx)
	{
		// �I���t���O�������Ă邩���肷��.
		if (is_selected()) {
			D2D1_POINT_2F b_min{ FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_max{ -FLT_MAX, -FLT_MAX };
			// �O���[�v�����ꂽ�e�}�`�ɂ��Ĉȉ����J��Ԃ�.
			for (const auto s : m_list_grouped) {
				// �����t���O�������Ă��邩���肷��.
				if (s->is_deleted()) {
					continue;
				}
				s->draw(dx);
				s->get_bound(b_min, b_max, b_min, b_max);
			}
			dx.m_d2dContext->DrawRectangle(D2D1_RECT_F{ b_min.x, b_min.y, b_max.x, b_max.y }, dx.m_shape_brush.get(), 1.0f, dx.m_aux_style.get());
		}
		else {
			for (const auto s : m_list_grouped) {
				if (s->is_deleted()) {
					continue;
				}
				s->draw(dx);
			}
		}
	}

	// �}�`���͂ޗ̈�𓾂�.
	// a_min	���̗̈�̍���ʒu.
	// a_man	���̗̈�̉E���ʒu.
	// b_min	����ꂽ�̈�̍���ʒu.
	// b_max	����ꂽ�̈�̉E���ʒu.
	void ShapeGroup::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		b_min = a_min;
		b_max = a_max;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_min, b_max, b_min, b_max);
		}
	}

	// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
	// value	�̈�̍���ʒu
	void ShapeGroup::get_pos_min(D2D1_POINT_2F& value) const noexcept
	{
		get_pos_start(value);
	}

	// �J�n�ʒu�𓾂�.
	// value	�J�n�ʒu
	// �O���[�v�}�`�̏ꍇ, �J�n�ʒu�͐}�`���͂ޗ̈�̍���ʒu.
	bool ShapeGroup::get_pos_start(D2D1_POINT_2F& value) const noexcept
	{
		//if (m_list_grouped.empty()) {
		//	return false;
		//}
		auto flag = false;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			D2D1_POINT_2F pos;
			s->get_pos_min(pos);
			if (!flag) {
				value = pos;
				flag = true;
			}
			else {
				pt_min(pos, value, value);
			}
		}
		return flag;
	}

	// ������}�`���܂ނ����肷��.
	bool ShapeGroup::has_text(void) noexcept
	{
		std::list<SHAPE_LIST::iterator> stack;
		stack.push_back(m_list_grouped.begin());
		stack.push_back(m_list_grouped.end());
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
				if (typeid(*s) == typeid(ShapeText)) {
					return true;
				}
				else if (typeid(*s) == typeid(ShapeGroup)) {
					stack.push_back(i);
					stack.push_back(j);
					i = static_cast<ShapeGroup*>(s)->m_list_grouped.begin();
					j = static_cast<ShapeGroup*>(s)->m_list_grouped.end();
					continue;
				}
			}
		}
		return false;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeGroup::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->hit_test(t_pos) != ANCH_TYPE::ANCH_SHEET) {
				return ANCH_TYPE::ANCH_FILL;
			}
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeGroup::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		for (const auto s : m_list_grouped) {
			if (s->in_area(a_min, a_max) != true) {
				return false;
			}
		}
		return true;
	}

	// �����t���O�𔻒肷��.
	bool ShapeGroup::is_deleted(void) const noexcept
	{
		return m_list_grouped.size() == 0 || m_list_grouped.back()->is_deleted();
	}

	// �I���t���O�𔻒肷��.
	bool ShapeGroup::is_selected(void) const noexcept
	{
		// �O���[�v�Ɋ܂܂��}�`���I������Ă邩���肷��.
		return m_list_grouped.size() > 0 && m_list_grouped.back()->is_selected();
	}

	// ���������ړ�����
	// d_vec	�����x�N�g��
	bool ShapeGroup::move(const D2D1_POINT_2F d_vec)
	{
		return slist_move(m_list_grouped, d_vec);
	}

	// �l�������t���O�Ɋi�[����.
	bool ShapeGroup::set_delete(const bool value) noexcept
	{
		bool flag = false;
		for (const auto s : m_list_grouped) {
			if (s->set_delete(value) && !flag) {
				flag = true;
			}
		}
		return flag;
	}

	// �l���J�n�ʒu�Ɋi�[����. ���̕��ʂ̈ʒu������.
	bool ShapeGroup::set_pos_start(const D2D1_POINT_2F value)
	{
		D2D1_POINT_2F b_min;
		if (get_pos_start(b_min) && !equal(value, b_min)) {
			D2D1_POINT_2F d_vec;
			pt_sub(value, b_min, d_vec);
			move(d_vec);
			return true;
		}
		return false;
	}

	// �l��I���t���O�Ɋi�[����.
	bool ShapeGroup::set_select(const bool value) noexcept
	{
		bool flag = false;
		for (const auto s : m_list_grouped) {
			if (s->set_select(value) && !flag) {
				flag = true;
			}
		}
		return flag;
	}

	// �f�[�^���[�_�[����}�`���쐬����.
	ShapeGroup::ShapeGroup(DataReader const& dt_reader)
	{
		slist_read(m_list_grouped, dt_reader);
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapeGroup::write(DataWriter const& dt_writer) const
	{
		constexpr bool REDUCED = true;
		slist_write<!REDUCED>(m_list_grouped, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void ShapeGroup::write_svg(DataWriter const& dt_writer) const
	{
		dt_write_svg("<g>" SVG_NEW_LINE, dt_writer);
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			s->write_svg(dt_writer);
		}
		dt_write_svg("</g>" SVG_NEW_LINE, dt_writer);
	}
}