#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �}�`��\������.
	// dx	�`���
	void ShapeRect::draw(SHAPE_DX& dx)
	{
		const D2D1_RECT_F rect{
			m_pos.x,
			m_pos.y,
			m_pos.x + m_diff[0].x,
			m_pos.y + m_diff[0].y
		};
		// �h��Ԃ��F���s���������肷��.
		if (is_opaque(m_fill_color)) {
			// ���`��h��Ԃ�.
			dx.m_shape_brush->SetColor(m_fill_color);
			dx.m_d2dContext->FillRectangle(&rect, dx.m_shape_brush.get());
		}
		// ���g�̐F���s���������肷��.
		if (is_opaque(m_stroke_color)) {
			// ���`�̘g��\������.
			const auto w = m_stroke_width;
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawRectangle(
				rect, dx.m_shape_brush.get(), w, m_d2d_stroke_style.get());
		}
		// ���̐}�`���I������Ă邩���肷��.
		if (is_selected()) {
			// ���ʂ�\������.
			D2D1_POINT_2F r_pos[4];	// ���`�̒��_
			r_pos[0] = m_pos;
			r_pos[1].y = rect.top;
			r_pos[1].x = rect.right;
			r_pos[2].x = rect.right;
			r_pos[2].y = rect.bottom;
			r_pos[3].y = rect.bottom;
			r_pos[3].x = rect.left;
			for (uint32_t i = 0, j = 3; i < 4; j = i++) {
				anch_draw_rect(r_pos[i], dx);
				D2D1_POINT_2F r_mid;	// ���`�̕ӂ̒��_
				pt_avg(r_pos[j], r_pos[i], r_mid);
				anch_draw_rect(r_mid, dx);
			}
		}
	}

	// �܂���̐}�`�̕��ʂ��ʒu���܂ނ����肷��.
	uint32_t ShapeRect::hit_test_anch(const D2D1_POINT_2F t_pos) const noexcept
	{
		// �ǂ̒��_���ʒu���܂ނ����肷��.
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F a_pos;
			get_anch_pos(ANCH_CORNER[i], a_pos);
			if (pt_in_anch(t_pos, a_pos)) {
				return ANCH_CORNER[i];
			}
		}
		// �ǂ̒��_���ʒu���܂ނ����肷��.
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F a_pos;
			get_anch_pos(ANCH_MIDDLE[i], a_pos);
			if (pt_in_anch(t_pos, a_pos)) {
				return ANCH_MIDDLE[i];
			}
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeRect::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		// ���`�̊e���_������, ���肷��ʒu�����ꂼ��̕��ʂɊ܂܂�邩���肷��.
		D2D1_POINT_2F v_pos[4]{ m_pos, };
		v_pos[2].x = m_pos.x + m_diff[0].x;
		v_pos[2].y = m_pos.y + m_diff[0].y;
		if (pt_in_anch(t_pos, v_pos[2])) {
			return ANCH_TYPE::ANCH_SE;
		}
		v_pos[3].x = m_pos.x;
		v_pos[3].y = m_pos.y + m_diff[0].y;
		if (pt_in_anch(t_pos, v_pos[3])) {
			return ANCH_TYPE::ANCH_SW;
		}
		v_pos[1].x = m_pos.x + m_diff[0].x;
		v_pos[1].y = m_pos.y;
		if (pt_in_anch(t_pos, v_pos[1])) {
			return ANCH_TYPE::ANCH_NE;
		}
		if (pt_in_anch(t_pos, v_pos[0])) {
			return ANCH_TYPE::ANCH_NW;
		}

		// �e�ӂ̒��_������, ���肷��ʒu�����ꂼ��̕��ʂɊ܂܂�邩���肷��.
		D2D1_POINT_2F s_pos;
		pt_avg(v_pos[2], v_pos[3], s_pos);
		if (pt_in_anch(t_pos, s_pos)) {
			return ANCH_TYPE::ANCH_SOUTH;
		}
		D2D1_POINT_2F e_pos;
		pt_avg(v_pos[1], v_pos[2], e_pos);
		if (pt_in_anch(t_pos, e_pos)) {
			return ANCH_TYPE::ANCH_EAST;
		}
		D2D1_POINT_2F w_pos;
		pt_avg(v_pos[0], v_pos[3], w_pos);
		if (pt_in_anch(t_pos, w_pos)) {
			return ANCH_TYPE::ANCH_WEST;
		}
		D2D1_POINT_2F n_pos;
		pt_avg(v_pos[0], v_pos[1], n_pos);
		if (pt_in_anch(t_pos, n_pos)) {
			return ANCH_TYPE::ANCH_NORTH;
		}

		D2D1_POINT_2F r_min;
		D2D1_POINT_2F r_max;
		pt_bound(v_pos[0], v_pos[2], r_min, r_max);
		D2D1_POINT_2F o_min;
		D2D1_POINT_2F o_max;
		const double e_width = m_stroke_width * 0.5;
		pt_add(r_min, -e_width, -e_width, o_min);
		pt_add(r_max, e_width, e_width, o_max);
		if (!pt_in_rect(t_pos, o_min, o_max)) {
			return ANCH_TYPE::ANCH_SHEET;
		}
		if (is_opaque(m_stroke_color)) {
			D2D1_POINT_2F i_min;
			D2D1_POINT_2F i_max;
			pt_add(o_min, m_stroke_width, m_stroke_width, i_min);
			pt_add(o_max, -m_stroke_width, -m_stroke_width, i_max);
			if (i_max.x <= i_min.x || i_max.y <= i_min.y ||
				!pt_in_rect(t_pos, i_min, i_max)) {
				if (r_min.x <= t_pos.x && t_pos.x <= r_max.x ||
					r_min.y <= t_pos.y && t_pos.y <= r_max.y) {
					return ANCH_TYPE::ANCH_STROKE;
				}
				else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
					if (pt_in_elli(t_pos, v_pos[0], e_width, e_width) ||
						pt_in_elli(t_pos, v_pos[1], e_width, e_width) ||
						pt_in_elli(t_pos, v_pos[2], e_width, e_width) ||
						pt_in_elli(t_pos, v_pos[3], e_width, e_width)) {
						return ANCH_TYPE::ANCH_STROKE;
					}
				}
				else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL ||
					(m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL &&
						m_stroke_join_limit < M_SQRT2)) {
					//const auto e_width = static_cast<FLOAT>(m_stroke_width * 0.5);
					const D2D1_POINT_2F q_pos[4]{
						D2D1_POINT_2F{ 0.0f, -static_cast<FLOAT>(e_width) },
						D2D1_POINT_2F{ static_cast<FLOAT>(e_width), 0.0f },
						D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
						D2D1_POINT_2F{ -static_cast<FLOAT>(e_width), 0.0f }
					};
					if (pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[0].x, t_pos.y - v_pos[0].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[1].x, t_pos.y - v_pos[1].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[2].x, t_pos.y - v_pos[2].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[3].x, t_pos.y - v_pos[3].y }, 4, q_pos)) {
						return ANCH_TYPE::ANCH_STROKE;
					}
				}
				else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
					(m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL &&
						m_stroke_join_limit >= M_SQRT2)) {
					const auto limit = static_cast<FLOAT>(m_stroke_width * M_SQRT2 * 0.5 * m_stroke_join_limit);
					const D2D1_POINT_2F q_pos[4]{
						D2D1_POINT_2F{ 0.0f, -limit }, D2D1_POINT_2F{ limit, 0.0f }, D2D1_POINT_2F{ 0.0f, limit }, D2D1_POINT_2F{ -limit, 0.0f }
					};
					if (pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[0].x, t_pos.y - v_pos[0].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[1].x, t_pos.y - v_pos[1].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[2].x, t_pos.y - v_pos[2].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[3].x, t_pos.y - v_pos[3].y }, 4, q_pos)) {
						return ANCH_TYPE::ANCH_STROKE;
					}
				}
			}
		}
		if (is_opaque(m_fill_color) && pt_in_rect(t_pos, r_min, r_max)) {
			return ANCH_TYPE::ANCH_FILL;
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// �h��Ԃ��̐F�𓾂�.
	// val	����ꂽ�l.
	bool ShapeRect::get_fill_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_fill_color;
		return true;
	}

	// �h��Ԃ��̐F�Ɋi�[����.
	// val	�i�[����l.
	bool ShapeRect::set_fill_color(const D2D1_COLOR_F& value) noexcept
	{
		if (!equal(m_fill_color, value)) {
			m_fill_color = value;
			return true;
		}
		return false;
	}

	//	���ʂ̈ʒu�𓾂�.
	//	anch	�}�`�̕���.
	//	value	����ꂽ�ʒu.
	//	�߂�l	�Ȃ�
	void ShapeRect::get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept
	{
		switch (anch) {
		case ANCH_TYPE::ANCH_NORTH:
			value.x = m_pos.x + m_diff[0].x * 0.5f;
			value.y = m_pos.y;
			break;
		case ANCH_TYPE::ANCH_NE:
			value.x = m_pos.x + m_diff[0].x;
			value.y = m_pos.y;
			break;
		case ANCH_TYPE::ANCH_WEST:
			value.x = m_pos.x;
			value.y = m_pos.y + m_diff[0].y * 0.5f;
			break;
		case ANCH_TYPE::ANCH_EAST:
			value.x = m_pos.x + m_diff[0].x;
			value.y = m_pos.y + m_diff[0].y * 0.5f;
			break;
		case ANCH_TYPE::ANCH_SW:
			value.x = m_pos.x;
			value.y = m_pos.y + m_diff[0].y;
			break;
		case ANCH_TYPE::ANCH_SOUTH:
			value.x = m_pos.x + m_diff[0].x * 0.5f;
			value.y = m_pos.y + m_diff[0].y;
			break;
		case ANCH_TYPE::ANCH_SE:
			value.x = m_pos.x + m_diff[0].x;
			value.y = m_pos.y + m_diff[0].y;
			break;
		default:
			value = m_pos;
			break;
		}
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeRect::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		D2D1_POINT_2F e_pos;

		pt_add(m_pos, m_diff[0], e_pos);
		return pt_in_rect(m_pos, a_min, a_max) && pt_in_rect(e_pos, a_min, a_max);
	}

	//	�l��, ���ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu������.
	//	value	�i�[����l
	//	abch	�}�`�̕���
	bool ShapeRect::set_anch_pos(const D2D1_POINT_2F value, const uint32_t anch, const float dist)
	{
		bool flag = false;
		switch (anch) {
		case ANCH_TYPE::ANCH_SHEET:
			{
			D2D1_POINT_2F vec;
			pt_sub(value, m_pos, vec);
			pt_round(vec, PT_ROUND, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				pt_add(m_pos, vec, m_pos);
				flag = true;
			}
			}
			break;
		case ANCH_TYPE::ANCH_NW:
			{
			D2D1_POINT_2F vec;
			pt_sub(value, m_pos, vec);
			pt_round(vec, PT_ROUND, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				pt_add(m_pos, vec, m_pos);
				pt_sub(m_diff[0], vec, m_diff[0]);
				flag = true;
			}
			}
			break;
		case ANCH_TYPE::ANCH_SE:
		{
			D2D1_POINT_2F vec;
			pt_sub(value, m_pos, vec);
			pt_round(vec, PT_ROUND, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				m_diff[0] = vec;
				flag = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_NE:
		{
			D2D1_POINT_2F a_pos;
			get_anch_pos(ANCH_TYPE::ANCH_NE, a_pos);
			D2D1_POINT_2F vec;
			pt_sub(value, a_pos, vec);
			pt_round(vec, PT_ROUND, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				m_pos.y += vec.y;
				pt_add(m_diff[0], vec.x, -vec.y, m_diff[0]);
				flag = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_SW:
		{
			D2D1_POINT_2F a_pos;
			get_anch_pos(ANCH_TYPE::ANCH_SW, a_pos);
			D2D1_POINT_2F vec;
			pt_sub(value, a_pos, vec);
			pt_round(vec, PT_ROUND, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				m_pos.x += vec.x;
				pt_add(m_diff[0], -vec.x, vec.y, m_diff[0]);
				flag = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_WEST:
		{
			const double vec_x = std::round((static_cast<double>(value.x) - m_pos.x) / PT_ROUND) * PT_ROUND;
			if (abs(vec_x) >= FLT_MIN) {
				m_diff[0].x = static_cast<FLOAT>(m_diff[0].x - vec_x);
				m_pos.x = static_cast<FLOAT>(m_pos.x + vec_x);
				flag = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_EAST:
		{
			const double vec_x = std::round((static_cast<double>(value.x) - m_pos.x) / PT_ROUND) * PT_ROUND;
			if (abs(vec_x) >= FLT_MIN) {
				m_diff[0].x = static_cast<FLOAT>(vec_x);
				flag = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_NORTH:
		{
			const double vec_y = std::round((static_cast<double>(value.y) - m_pos.y) / PT_ROUND) * PT_ROUND;
			if (fabs(vec_y) >= FLT_MIN) {
				m_diff[0].y = static_cast<FLOAT>(m_diff[0].y - vec_y);
				m_pos.y = static_cast<FLOAT>(m_pos.y + vec_y);
				flag = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_SOUTH:
		{
			const double vec_y = std::round((static_cast<double>(value.y) - m_pos.y) / PT_ROUND) * PT_ROUND;
			if (abs(vec_y) >= FLT_MIN) {
				m_diff[0].y = static_cast<FLOAT>(vec_y);
				flag = true;
			}
		}
		break;
		default:
			return false;
		}
		if (dist >= FLT_MIN) {
			if (m_diff[0].x < dist) {
				if (anch == ANCH_TYPE::ANCH_NE) {
					m_pos.x += m_diff[0].x;
				}
				m_diff[0].x = 0.0f;
				flag = true;
			}
			if (m_diff[0].y < dist) {
				if (anch == ANCH_TYPE::ANCH_NE) {
					m_pos.y += m_diff[0].y;
				}
				m_diff[0].y = 0.0f;
				flag = true;
			}
		}
		return flag;
	}

	// �}�`���쐬����.
	// b_pos	�͂ޗ̈�̎n�_
	// b_vec	�͂ޗ̈�̏I�_�ւ̍���
	// s_sttr	����
	ShapeRect::ShapeRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr) :
		ShapeStroke::ShapeStroke(1, s_attr),
		m_fill_color(s_attr->m_fill_color)
	{
		m_pos = b_pos;
		m_diff[0] = b_vec;
	}

	// �f�[�^���[�_�[����}�`��ǂݍ���.
	ShapeRect::ShapeRect(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader)
	{
		dt_read(m_fill_color, dt_reader);
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapeRect::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);
		dt_write(m_fill_color, dt_writer);
	}

	// �ߖT�̒��_�𓾂�.
	bool ShapeRect::get_neighbor(const D2D1_POINT_2F a_pos, float& dd, D2D1_POINT_2F& value) const noexcept
	{
		bool flag = false;
		D2D1_POINT_2F v_pos[4];
		const size_t v_cnt = get_verts(v_pos);
		for (size_t i = 0; i < v_cnt; i++) {
			D2D1_POINT_2F vec;
			pt_sub(v_pos[i], a_pos, vec);
			const float abs2 = static_cast<float>(pt_abs2(vec));
			if (abs2 < dd) {
				dd = abs2;
				value = v_pos[i];
				flag = true;
			}
		}
		return flag;
	}

	// ���_�𓾂�.
	size_t ShapeRect::get_verts(D2D1_POINT_2F v_pos[]) const noexcept
	{
		v_pos[0] = m_pos;
		v_pos[1].x = m_pos.x + m_diff[0].x;
		v_pos[1].y = m_pos.y;
		v_pos[2].x = v_pos[1].x;
		v_pos[2].y = m_pos.y + m_diff[0].y;
		v_pos[3].x = m_pos.x;
		v_pos[3].y = v_pos[2].y;
		return 4;
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeRect::dt_write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::dt_write_svg;

		dt_write_svg("<rect ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_diff[0], "width", "height", dt_writer);
		dt_write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::dt_write_svg(dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
	}
}