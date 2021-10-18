#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;

	// �}�`��\������.
	// dx	�`���
	void ShapeRect::draw(D2D_UI& dx)
	{
		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(dx);
		}

		const D2D1_RECT_F rect{
			m_pos.x,
			m_pos.y,
			m_pos.x + m_vec[0].x,
			m_pos.y + m_vec[0].y
		};
		// �h��Ԃ��F���s���������肷��.
		if (is_opaque(m_fill_color)) {
			// ���`��h��Ԃ�.
			dx.m_solid_color_brush->SetColor(m_fill_color);
			dx.m_d2d_context->FillRectangle(rect, dx.m_solid_color_brush.get());
		}
		// ���g�̐F���s���������肷��.
		if (is_opaque(m_stroke_color)) {
			// ���`�̘g��\������.
			const auto w = m_stroke_width;
			dx.m_solid_color_brush->SetColor(m_stroke_color);
			dx.m_d2d_context->DrawRectangle(rect, dx.m_solid_color_brush.get(), w, m_d2d_stroke_style.get());
		}
		// ���̐}�`���I������Ă邩���肷��.
		if (is_selected()) {
			// ���ʂ�\������.
			D2D1_POINT_2F a_pos[4];	// ���`�̒��_
			a_pos[0] = m_pos;
			a_pos[1].y = rect.top;
			a_pos[1].x = rect.right;
			a_pos[2].x = rect.right;
			a_pos[2].y = rect.bottom;
			a_pos[3].y = rect.bottom;
			a_pos[3].x = rect.left;
			for (uint32_t i = 0, j = 3; i < 4; j = i++) {
				anchor_draw_rect(a_pos[i], dx);
				D2D1_POINT_2F a_mid;	// ���`�̕ӂ̒��_
				pt_avg(a_pos[j], a_pos[i], a_mid);
				anchor_draw_rect(a_mid, dx);
			}
		}
	}

	// �܂���̐}�`�̕��ʂ��ʒu���܂ނ����肷��.
	uint32_t ShapeRect::hit_test_anchor(const D2D1_POINT_2F t_pos) const noexcept
	{
		// �ǂ̒��_���ʒu���܂ނ����肷��.
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F a_pos;
			get_pos_anchor(ANCH_CORNER[i], a_pos);
			if (pt_in_anchor(t_pos, a_pos)) {
				return ANCH_CORNER[i];
			}
		}
		// �ǂ̒��_���ʒu���܂ނ����肷��.
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F a_pos;
			get_pos_anchor(ANCH_MIDDLE[i], a_pos);
			if (pt_in_anchor(t_pos, a_pos)) {
				return ANCH_MIDDLE[i];
			}
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肳���ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeRect::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		// �e���_�̕��ʂɊ܂܂�邩���肷��.
		D2D1_POINT_2F v_pos[4]{ m_pos, };
		v_pos[2].x = m_pos.x + m_vec[0].x;
		v_pos[2].y = m_pos.y + m_vec[0].y;
		if (pt_in_anchor(t_pos, v_pos[2])) {
			return ANCH_TYPE::ANCH_SE;
		}
		v_pos[3].x = m_pos.x;
		v_pos[3].y = m_pos.y + m_vec[0].y;
		if (pt_in_anchor(t_pos, v_pos[3])) {
			return ANCH_TYPE::ANCH_SW;
		}
		v_pos[1].x = m_pos.x + m_vec[0].x;
		v_pos[1].y = m_pos.y;
		if (pt_in_anchor(t_pos, v_pos[1])) {
			return ANCH_TYPE::ANCH_NE;
		}
		if (pt_in_anchor(t_pos, v_pos[0])) {
			return ANCH_TYPE::ANCH_NW;
		}

		// �e�ӂ̒��_�̕��ʂɊ܂܂�邩���肷��.
		D2D1_POINT_2F s_pos;
		pt_avg(v_pos[2], v_pos[3], s_pos);
		if (pt_in_anchor(t_pos, s_pos)) {
			return ANCH_TYPE::ANCH_SOUTH;
		}
		D2D1_POINT_2F e_pos;
		pt_avg(v_pos[1], v_pos[2], e_pos);
		if (pt_in_anchor(t_pos, e_pos)) {
			return ANCH_TYPE::ANCH_EAST;
		}
		D2D1_POINT_2F w_pos;
		pt_avg(v_pos[0], v_pos[3], w_pos);
		if (pt_in_anchor(t_pos, w_pos)) {
			return ANCH_TYPE::ANCH_WEST;
		}
		D2D1_POINT_2F n_pos;
		pt_avg(v_pos[0], v_pos[1], n_pos);
		if (pt_in_anchor(t_pos, n_pos)) {
			return ANCH_TYPE::ANCH_NORTH;
		}

		// �Ίp�ɂ��钸�_�����Ƃ�, ���`�𓾂�.
		D2D1_POINT_2F t_min, t_max;
		//pt_bound(v_pos[0], v_pos[2], r_min, r_max);
		if (v_pos[0].x < v_pos[2].x) {
			t_min.x = v_pos[0].x;
			t_max.x = v_pos[2].x;
		}
		else {
			t_min.x = v_pos[2].x;
			t_max.x = v_pos[0].x;
		}
		if (v_pos[0].y < v_pos[2].y) {
			t_min.y = v_pos[0].y;
			t_max.y = v_pos[2].y;
		}
		else {
			t_min.y = v_pos[2].y;
			t_max.y = v_pos[0].y;
		}

		if (is_opaque(m_stroke_color) && m_stroke_width > 0.0) {
			// �O���̕��`�Ɋ܂܂�邩���肷��.
			D2D1_POINT_2F s_min, s_max;
			const double s_width = m_stroke_width > 0.0 ? max(m_stroke_width, Shape::s_anchor_len) : 0.0;
			const double e_width = s_width * 0.5;
			pt_add(t_min, -e_width, s_min);
			pt_add(t_max, e_width, s_max);
			if (pt_in_rect(t_pos, s_min, s_max)) {
				// �����̕��`���v�Z����.
				D2D1_POINT_2F u_min, u_max;
				pt_add(s_min, s_width, u_min);
				pt_add(s_max, -s_width, u_max);
				// �����̕��`�����]���� (�g���������Đ}�`�𕢂�) �����肷��.
				if (u_max.x <= u_min.x || u_max.y <= u_min.y) {
					return ANCH_TYPE::ANCH_STROKE;
				}
				// �����̕��`�Ɋ܂܂�� (�ӂɊ܂܂�Ȃ�) �����肷��.
				else if (pt_in_rect(t_pos, u_min, u_max)) {
					if (is_opaque(m_fill_color)) {
						return ANCH_TYPE::ANCH_FILL;
					}
				}
				// ���`�̊p�Ɋ܂܂�ĂȂ� (�ӂɊ܂܂��) �����肷��.
				else if (t_min.x <= t_pos.x && t_pos.x <= t_max.x || t_min.y <= t_pos.y && t_pos.y <= t_max.y) {
					return ANCH_TYPE::ANCH_STROKE;
				}
				// ���g�̂Ȃ����ۂ߂����肷��.
				else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
					if (pt_in_circle(t_pos, v_pos[0], e_width) ||
						pt_in_circle(t_pos, v_pos[1], e_width) ||
						pt_in_circle(t_pos, v_pos[2], e_width) ||
						pt_in_circle(t_pos, v_pos[3], e_width)) {
						return ANCH_TYPE::ANCH_STROKE;
					}
				}
				// ���g�̂Ȃ����ʎ��, �܂���, �}�C�^�[�E�ʎ��ł��}�C�^�[��������2 ���������肷��.
				else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL ||
					(m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL && m_join_limit < M_SQRT2)) {
					const auto limit = static_cast<FLOAT>(e_width);
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
				// ���g�̂Ȃ����}�C�^�[, �܂���, �}�C�^�[/�ʎ��ł��}�C�^�[��������2 �ȏォ���肷��.
				else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
					(m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL && m_join_limit >= M_SQRT2)) {
					const auto limit = static_cast<FLOAT>(m_stroke_width * M_SQRT2 * 0.5 * m_join_limit);
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
		else if (is_opaque(m_fill_color) && pt_in_rect(t_pos, t_min, t_max)) {
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
	void ShapeRect::get_pos_anchor(const uint32_t anchor, D2D1_POINT_2F& value) const noexcept
	{
		switch (anchor) {
		case ANCH_TYPE::ANCH_NORTH:
			value.x = m_pos.x + m_vec[0].x * 0.5f;
			value.y = m_pos.y;
			break;
		case ANCH_TYPE::ANCH_NE:
			value.x = m_pos.x + m_vec[0].x;
			value.y = m_pos.y;
			break;
		case ANCH_TYPE::ANCH_WEST:
			value.x = m_pos.x;
			value.y = m_pos.y + m_vec[0].y * 0.5f;
			break;
		case ANCH_TYPE::ANCH_EAST:
			value.x = m_pos.x + m_vec[0].x;
			value.y = m_pos.y + m_vec[0].y * 0.5f;
			break;
		case ANCH_TYPE::ANCH_SW:
			value.x = m_pos.x;
			value.y = m_pos.y + m_vec[0].y;
			break;
		case ANCH_TYPE::ANCH_SOUTH:
			value.x = m_pos.x + m_vec[0].x * 0.5f;
			value.y = m_pos.y + m_vec[0].y;
			break;
		case ANCH_TYPE::ANCH_SE:
			value.x = m_pos.x + m_vec[0].x;
			value.y = m_pos.y + m_vec[0].y;
			break;
		default:
			value = m_pos;
			break;
		}
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// area_min	�͈͂̍���ʒu
	// area_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeRect::in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept
	{
		D2D1_POINT_2F pos;
		pt_add(m_pos, m_vec[0], pos);
		return pt_in_rect(m_pos, area_min, area_max) && pt_in_rect(pos, area_min, area_max);
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu������.
	// value	�l
	// anchor	�}�`�̕���
	// limit	���̒��_�Ƃ̌��E���� (���̒��_�Ƃ̋��������̒l�����ɂȂ�Ȃ�, ���̒��_�Ɉʒu�ɍ��킹��)
	bool ShapeRect::set_pos_anchor(const D2D1_POINT_2F value, const uint32_t anchor, const float limit, const bool /*keep_aspect*/) noexcept
	{
		bool done = false;
		switch (anchor) {
		case ANCH_TYPE::ANCH_SHEET:
		{
			D2D1_POINT_2F new_pos;
			pt_round(value, PT_ROUND, new_pos);
			D2D1_POINT_2F vec;
			pt_sub(new_pos, m_pos, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				pt_add(m_pos, vec, m_pos);
				done = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_NW:
		{
			D2D1_POINT_2F new_pos;
			pt_round(value, PT_ROUND, new_pos);
			D2D1_POINT_2F vec;
			pt_sub(new_pos, m_pos, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				pt_add(m_pos, vec, m_pos);
				pt_sub(m_vec[0], vec, m_vec[0]);
				done = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_SE:
		{
			D2D1_POINT_2F pos;
			pt_add(m_pos, m_vec[0], pos);
			D2D1_POINT_2F new_pos;
			pt_round(value, PT_ROUND, new_pos);
			D2D1_POINT_2F vec;
			pt_sub(new_pos, pos, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				pt_add(m_vec[0], vec, m_vec[0]);
				done = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_NE:
		{
			D2D1_POINT_2F pos;
			get_pos_anchor(ANCH_TYPE::ANCH_NE, pos);
			D2D1_POINT_2F new_pos;
			pt_round(value, PT_ROUND, new_pos);
			D2D1_POINT_2F vec;
			pt_sub(new_pos, pos, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				m_pos.y += vec.y;
				pt_add(m_vec[0], vec.x, -vec.y, m_vec[0]);
				done = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_SW:
		{
			D2D1_POINT_2F pos;
			get_pos_anchor(ANCH_TYPE::ANCH_SW, pos);
			D2D1_POINT_2F new_pos;
			pt_round(value, PT_ROUND, new_pos);
			D2D1_POINT_2F vec;
			pt_sub(new_pos, pos, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				m_pos.x += vec.x;
				pt_add(m_vec[0], -vec.x, vec.y, m_vec[0]);
				done = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_WEST:
		{
			const double vec_x = std::round((static_cast<double>(value.x) - m_pos.x) / PT_ROUND) * PT_ROUND;
			if (vec_x <= -FLT_MIN || vec_x >= FLT_MIN) {
				m_vec[0].x = static_cast<FLOAT>(m_vec[0].x - vec_x);
				m_pos.x = static_cast<FLOAT>(m_pos.x + vec_x);
				done = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_EAST:
		{
			const double vec_x = std::round((static_cast<double>(value.x) - m_pos.x - m_vec[0].x) / PT_ROUND) * PT_ROUND;
			if (vec_x <= -FLT_MIN || vec_x >= FLT_MIN) {
				m_vec[0].x += static_cast<FLOAT>(vec_x);
				done = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_NORTH:
		{
			const double vec_y = std::round((static_cast<double>(value.y) - m_pos.y) / PT_ROUND) * PT_ROUND;
			if (vec_y <= -FLT_MIN || vec_y >= FLT_MIN) {
				m_vec[0].y = static_cast<FLOAT>(m_vec[0].y - vec_y);
				m_pos.y = static_cast<FLOAT>(m_pos.y + vec_y);
				done = true;
			}
		}
		break;
		case ANCH_TYPE::ANCH_SOUTH:
		{
			const double vec_y = std::round((static_cast<double>(value.y) - m_pos.y - m_vec[0].y) / PT_ROUND) * PT_ROUND;
			if (vec_y <= -FLT_MIN || vec_y >= FLT_MIN) {
				m_vec[0].y += static_cast<FLOAT>(vec_y);
				done = true;
			}
		}
		break;
		default:
			return false;
		}
		if (limit >= FLT_MIN) {
			// �I�_�ւ̍����� x �l��, ���E�������������肷��.
			if (m_vec[0].x > -limit && m_vec[0].x < limit) {
				if (anchor == ANCH_TYPE::ANCH_NE) {
					m_pos.x += m_vec[0].x;
				}
				m_vec[0].x = 0.0f;
				done = true;
			}
			if (m_vec[0].y > -limit && m_vec[0].y < limit) {
				if (anchor == ANCH_TYPE::ANCH_NE) {
					m_pos.y += m_vec[0].y;
				}
				m_vec[0].y = 0.0f;
				done = true;
			}
		}
		return done;
	}

	// �}�`���쐬����.
	// b_pos	�͂ޗ̈�̎n�_
	// b_vec	�͂ޗ̈�̏I�_�ւ̍���
	// s_sttr	����
	ShapeRect::ShapeRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr) :
		ShapeStroke::ShapeStroke(s_attr),
		m_fill_color(s_attr->m_fill_color)
	{
		m_pos = b_pos;
		m_vec.resize(1, b_vec);
		m_vec.shrink_to_fit();
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

	// �ߖT�̒��_��������.
	// pos	����ʒu
	// dd	�ߖT�Ƃ݂Ȃ����� (�̓��l), �����藣�ꂽ���_�͋ߖT�Ƃ݂͂Ȃ��Ȃ�.
	// value	����ʒu�̋ߖT�ɂ��钸�_
	// �߂�l	���������� true
	bool ShapeRect::get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& value) const noexcept
	{
		bool found = false;
		D2D1_POINT_2F v_pos[4];
		const size_t v_cnt = get_verts(v_pos);
		for (size_t i = 0; i < v_cnt; i++) {
			D2D1_POINT_2F vec;
			pt_sub(v_pos[i], pos, vec);
			const float vv = static_cast<float>(pt_abs2(vec));
			if (vv < dd) {
				dd = vv;
				value = v_pos[i];
				if (!found) {
					found = true;
				}
			}
		}
		return found;
	}

	// ���_�𓾂�.
	size_t ShapeRect::get_verts(D2D1_POINT_2F v_pos[]) const noexcept
	{
		v_pos[0] = m_pos;
		v_pos[1].x = m_pos.x + m_vec[0].x;
		v_pos[1].y = m_pos.y;
		v_pos[2].x = v_pos[1].x;
		v_pos[2].y = m_pos.y + m_vec[0].y;
		v_pos[3].x = m_pos.x;
		v_pos[3].y = v_pos[2].y;
		return 4;
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeRect::write_svg(DataWriter const& dt_writer) const
	{
		dt_write_svg("<rect ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_vec[0], "width", "height", dt_writer);
		dt_write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
	}
}