#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	// �}�`��\������.
	void ShapeOblong::draw_loc(void) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		// ���ʂ�\������.
		// 0---1
		// |   |
		// 3---2
		D2D1_POINT_2F p[4]{	// ���`�̒��_
			{ m_start.x, m_start.y },
			{ m_start.x + m_lineto.x, m_start.y },
			{ m_start.x + m_lineto.x, m_start.y + m_lineto.y },
			{ m_start.x, m_start.y + m_lineto.y }
		};
		D2D1_POINT_2F mid_pt;	// ���`�̕ӂ̒��_
		pt_avg(p[0], p[3], mid_pt);
		loc_draw_square(mid_pt, target, brush);
		pt_avg(p[0], p[1], mid_pt);
		loc_draw_square(mid_pt, target, brush);
		pt_avg(p[1], p[2], mid_pt);
		loc_draw_square(mid_pt, target, brush);
		pt_avg(p[2], p[3], mid_pt);
		loc_draw_square(mid_pt, target, brush);
		loc_draw_square(p[0], target, brush);
		loc_draw_square(p[1], target, brush);
		loc_draw_square(p[3], target, brush);
		loc_draw_square(p[2], target, brush);
	}

	// �}�`��\������.
	void ShapeOblong::draw(void) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		if (m_d2d_stroke_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			create_stroke_style(factory);
		}

		const D2D1_RECT_F rect{
			m_start.x,
			m_start.y,
			m_start.x + m_lineto.x,
			m_start.y + m_lineto.y
		};
		// �h��Ԃ��F���s���������肷��.
		if (is_opaque(m_fill_color)) {
			// ���`��h��Ԃ�.
			brush->SetColor(m_fill_color);
			target->FillRectangle(rect, brush);
		}
		// ���g�̐F���s���������肷��.
		if (is_opaque(m_stroke_color)) {
			// ���`�̘g��\������.
			const auto w = m_stroke_width;
			brush->SetColor(m_stroke_color);
			target->DrawRectangle(rect, brush, w, m_d2d_stroke_style.get());
		}
		// ���̐}�`���I������Ă邩���肷��.
		if (m_loc_show && is_selected()) {
			// �⏕����`��
			if (m_stroke_width >= Shape::m_loc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawRectangle(rect, brush, 2.0f * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawRectangle(rect, brush, m_aux_width, m_aux_style.get());
			}
			// �}�`�̕��ʂ�`��.
			draw_loc();
		}
	}

	// �_��, ���`�̒��_�ƒ��ԓ_�̂���, �ǂ̕��ʂɊ܂܂�邩�𔻒肷��.
	uint32_t rect_loc_hit_test(
		const D2D1_POINT_2F start,	// �n�_
		const D2D1_POINT_2F end_to,	// �Ίp�ւ̈ʒu�x�N�g��
		const D2D1_POINT_2F test_pt,	// ���肳���_
		const double a_len	// �}�`�̕��ʂ̑傫��
	) noexcept
	{
		// 4----8----2
		// |         |
		// 7         6
		// |         |
		// 3----5----1
		const D2D1_POINT_2F se_pt{
			start.x + end_to.x, start.y + end_to.y
		};
		if (loc_hit_test(test_pt, se_pt, a_len)) {
			return LOC_TYPE::LOC_SE;
		}
		const D2D1_POINT_2F ne_pt{
			se_pt.x, start.y
		};
		if (loc_hit_test(test_pt, ne_pt, a_len)) {
			return LOC_TYPE::LOC_NE;
		}
		const D2D1_POINT_2F sw_pt{
			start.x, se_pt.y
		};
		if (loc_hit_test(test_pt, sw_pt, a_len)) {
			return LOC_TYPE::LOC_SW;
		}
		if (loc_hit_test(test_pt, start, a_len)) {
			return LOC_TYPE::LOC_NW;
		}
		const D2D1_POINT_2F s_pt{
			static_cast<FLOAT>(start.x + end_to.x * 0.5), se_pt.y
		};
		if (loc_hit_test(test_pt, s_pt, a_len)) {
			return LOC_TYPE::LOC_SOUTH;
		}
		const D2D1_POINT_2F e_pt{
			se_pt.x, static_cast<FLOAT>(start.y + end_to.y * 0.5)
		};
		if (loc_hit_test(test_pt, e_pt, a_len)) {
			return LOC_TYPE::LOC_EAST;
		}
		const D2D1_POINT_2F w_pt{
			start.x, e_pt.y
		};
		if (loc_hit_test(test_pt, w_pt, a_len)) {
			return LOC_TYPE::LOC_WEST;
		}
		const D2D1_POINT_2F n_pt{
			s_pt.x, start.y
		};
		if (loc_hit_test(test_pt, n_pt, a_len)) {
			return LOC_TYPE::LOC_NORTH;
		}
		return LOC_TYPE::LOC_SHEET;
	}

	// �}�`���_���܂ނ����肷��.
	// test_pt	���肳���_
	// �߂�l	�_���܂ޕ���
	uint32_t ShapeOblong::hit_test(const D2D1_POINT_2F test_pt, const bool/*ctrl_key*/) const noexcept
	{
		D2D1_POINT_2F p[4]{ m_start, };	// ���_�̔z��

		p[2].x = m_start.x + m_lineto.x;
		p[2].y = m_start.y + m_lineto.y;
		if (loc_hit_test(test_pt, p[2], m_loc_width)) {
			return LOC_TYPE::LOC_SE;
		}
		p[3].x = m_start.x;
		p[3].y = m_start.y + m_lineto.y;
		if (loc_hit_test(test_pt, p[3], m_loc_width)) {
			return LOC_TYPE::LOC_SW;
		}
		p[1].x = m_start.x + m_lineto.x;
		p[1].y = m_start.y;
		if (loc_hit_test(test_pt, p[1], m_loc_width)) {
			return LOC_TYPE::LOC_NE;
		}
		if (loc_hit_test(test_pt, p[0], m_loc_width)) {
			return LOC_TYPE::LOC_NW;
		}

		// �e�ӂ̒��_�̕��ʂɊ܂܂�邩���肷��.
		D2D1_POINT_2F bottom;
		pt_avg(p[2], p[3], bottom);
		if (loc_hit_test(test_pt, bottom, m_loc_width)) {
			return LOC_TYPE::LOC_SOUTH;
		}
		D2D1_POINT_2F right;
		pt_avg(p[1], p[2], right);
		if (loc_hit_test(test_pt, right, m_loc_width)) {
			return LOC_TYPE::LOC_EAST;
		}
		D2D1_POINT_2F left;
		pt_avg(p[0], p[3], left);
		if (loc_hit_test(test_pt, left, m_loc_width)) {
			return LOC_TYPE::LOC_WEST;
		}
		D2D1_POINT_2F top;
		pt_avg(p[0], p[1], top);
		if (loc_hit_test(test_pt, top, m_loc_width)) {
			return LOC_TYPE::LOC_NORTH;
		}

		// �Ίp�ɂ��钸�_�����Ƃ�, ���`�𓾂�.
		D2D1_POINT_2F r_lt, r_rb;
		if (p[0].x < p[2].x) {
			r_lt.x = p[0].x;
			r_rb.x = p[2].x;
		}
		else {
			r_lt.x = p[2].x;
			r_rb.x = p[0].x;
		}
		if (p[0].y < p[2].y) {
			r_lt.y = p[0].y;
			r_rb.y = p[2].y;
		}
		else {
			r_lt.y = p[2].y;
			r_rb.y = p[0].y;
		}

		if (!is_opaque(m_stroke_color) || m_stroke_width < FLT_MIN) {
			if (is_opaque(m_fill_color) && pt_in_rect2(test_pt, r_lt, r_rb)) {
				return LOC_TYPE::LOC_FILL;
			}
		}
		else {
			//      |                |
			//    +-+----------------+-+
			// ---+-+----------------+-+---
			//    | |                | |
			//    | |                | |
			// ---+-+----------------+-+---
			//    +-+----------------+-+
			//      |                |
			// ���g�̑����̔����̑傫�������O����, ���`���g�傷��.
			// ���������������ʂ̑傫�������Ȃ�, �����͕��ʂ̑傫���ɒ�������.
			D2D1_POINT_2F e_lb, e_rb;	// �g�債�����`
			const double s_thick = max(m_stroke_width, m_loc_width);
			const double e_thick = s_thick * 0.5;
			pt_add(r_lt, -e_thick, e_lb);
			pt_add(r_rb, e_thick, e_rb);
			// �g�債�����`�Ɋ܂܂�邩���肷��.
			if (pt_in_rect2(test_pt, e_lb, e_rb)) {
				// �����̑傫������������, �g�債�����`���k������.
				D2D1_POINT_2F s_lb, s_rb;	// �k���������`
				pt_add(e_lb, s_thick, s_lb);
				pt_add(e_rb, -s_thick, s_rb);
				// �k���������`�Ɋ܂܂�� (�ӂɊ܂܂�Ȃ�) �����肷��.
				if (pt_in_rect2(test_pt, s_lb, s_rb)) {
					if (is_opaque(m_fill_color)) {
						return LOC_TYPE::LOC_FILL;
					}
				}
				// �k���������`�����]���� (�g���������Đ}�`�𕢂�),
				// �܂���, ���`�̊p�Ɋ܂܂�ĂȂ� (�ӂɊ܂܂��) �����肷��.
				else if (s_rb.x <= s_lb.x || s_rb.y <= s_lb.y ||
					r_lt.x <= test_pt.x && test_pt.x <= r_rb.x || r_lt.y <= test_pt.y && test_pt.y <= r_rb.y) {
					return LOC_TYPE::LOC_STROKE;
				}
				// ���g�̌������ۂ߂����肷��.
				else if (m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
					if (pt_in_circle(test_pt, p[0], e_thick) ||
						pt_in_circle(test_pt, p[1], e_thick) ||
						pt_in_circle(test_pt, p[2], e_thick) ||
						pt_in_circle(test_pt, p[3], e_thick)) {
						return LOC_TYPE::LOC_STROKE;
					}
				}
				// ���g�̌������ʎ��, �܂���, ���E�ʎ��ł���萧������2 ���������肷��.
				else if (m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL ||
					(m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL && m_stroke_join_limit < M_SQRT2)) {
					const auto limit = static_cast<FLOAT>(e_thick);
					const D2D1_POINT_2F q[4]{
						D2D1_POINT_2F{ 0.0f, -limit }, D2D1_POINT_2F{ limit, 0.0f }, 
						D2D1_POINT_2F{ 0.0f, limit }, D2D1_POINT_2F{ -limit, 0.0f }
					};
					if (pt_in_poly(D2D1_POINT_2F{ test_pt.x - p[0].x, test_pt.y - p[0].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ test_pt.x - p[1].x, test_pt.y - p[1].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ test_pt.x - p[2].x, test_pt.y - p[2].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ test_pt.x - p[3].x, test_pt.y - p[3].y }, 4, q)) {
						return LOC_TYPE::LOC_STROKE;
					}
				}
				// ���g�̌��������, �܂���, ���/�ʎ��ł���萧������2 �ȏォ���肷��.
				else if (m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
					(m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL &&
						m_stroke_join_limit >= M_SQRT2)) {
					const auto limit = static_cast<FLOAT>(M_SQRT2 * 0.5 * m_stroke_width * m_stroke_join_limit);
					const D2D1_POINT_2F q[4]{
						D2D1_POINT_2F{ 0.0f, -limit }, D2D1_POINT_2F{ limit, 0.0f },
						D2D1_POINT_2F{ 0.0f, limit }, D2D1_POINT_2F{ -limit, 0.0f }
					};
					if (pt_in_poly(D2D1_POINT_2F{ test_pt.x - p[0].x, test_pt.y - p[0].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ test_pt.x - p[1].x, test_pt.y - p[1].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ test_pt.x - p[2].x, test_pt.y - p[2].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ test_pt.x - p[3].x, test_pt.y - p[3].y }, 4, q)) {
						return LOC_TYPE::LOC_STROKE;
					}
				}
			}
		}
		return LOC_TYPE::LOC_SHEET;
	}

	// ���E��`�𓾂�.
	// a_lt	���̗̈�̍���ʒu.
	// a_rb	���̗̈�̉E���ʒu.
	// b_lt	�͂ޗ̈�̍���ʒu.
	// b_rb	�͂ޗ̈�̉E���ʒu.
	void ShapeOblong::get_bbox(
		const D2D1_POINT_2F a_lt,
		const D2D1_POINT_2F a_rb,
		D2D1_POINT_2F& b_lt,
		D2D1_POINT_2F& b_rb
	) const noexcept
	{
		b_lt.x = m_start.x < a_lt.x ? m_start.x : a_lt.x;
		b_lt.y = m_start.y < a_lt.y ? m_start.y : a_lt.y;
		b_rb.x = m_start.x > a_rb.x ? m_start.x : a_rb.x;
		b_rb.y = m_start.y > a_rb.y ? m_start.y : a_rb.y;
		D2D1_POINT_2F end;
		pt_add(m_start, m_lineto, end);
		if (end.x < b_lt.x) {
			b_lt.x = end.x;
		}
		if (end.x > b_rb.x) {
			b_rb.x = end.x;
		}
		if (end.y < b_lt.y) {
			b_lt.y = end.y;
		}
		if (end.y > b_rb.y) {
			b_rb.y = end.y;
		}
	}

	// �h��Ԃ��F�𓾂�.
	// val	����ꂽ�l.
	bool ShapeOblong::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}

	// �h��Ԃ��F�Ɋi�[����.
	// val	�i�[����l.
	bool ShapeOblong::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_fill_color, val)) {
			m_fill_color = val;
			return true;
		}
		return false;
	}

	// �w�肵�����ʂ̓_�𓾂�.
	// loc	����
	// val	����ꂽ�l
	void ShapeOblong::get_pos_loc(const uint32_t loc, D2D1_POINT_2F& val) const noexcept
	{
		switch (loc) {
		case LOC_TYPE::LOC_NORTH:
			val.x = m_start.x + m_lineto.x * 0.5f;
			val.y = m_start.y;
			break;
		case LOC_TYPE::LOC_NE:
			val.x = m_start.x + m_lineto.x;
			val.y = m_start.y;
			break;
		case LOC_TYPE::LOC_WEST:
			val.x = m_start.x;
			val.y = m_start.y + m_lineto.y * 0.5f;
			break;
		case LOC_TYPE::LOC_EAST:
			val.x = m_start.x + m_lineto.x;
			val.y = m_start.y + m_lineto.y * 0.5f;
			break;
		case LOC_TYPE::LOC_SW:
			val.x = m_start.x;
			val.y = m_start.y + m_lineto.y;
			break;
		case LOC_TYPE::LOC_SOUTH:
			val.x = m_start.x + m_lineto.x * 0.5f;
			val.y = m_start.y + m_lineto.y;
			break;
		case LOC_TYPE::LOC_SE:
			val.x = m_start.x + m_lineto.x;
			val.y = m_start.y + m_lineto.y;
			break;
		default:
			val = m_start;
			break;
		}
	}

	// ���E��`�̍���_�𓾂�.
	void ShapeOblong::get_bbox_lt(D2D1_POINT_2F& val) const noexcept
	{
		val.x = (0.0f <= m_lineto.x ? m_start.x : m_start.x + m_lineto.x);
		val.y = (0.0f <= m_lineto.y ? m_start.y : m_start.y + m_lineto.y);
	}

	// �n�_�𓾂�
	// val	�n�_
	// �߂�l	�˂� true
	bool ShapeOblong::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		val = m_start;
		return true;
	}

	// ��`�Ɋ܂܂�邩���肷��.
	// lt	��`�̍���ʒu
	// rb	��`�̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeOblong::is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		D2D1_POINT_2F end_pt;
		pt_add(m_start, m_lineto, end_pt);
		return pt_in_rect(m_start, lt, rb) && pt_in_rect(end_pt, lt, rb);
	}

	// �ʒu���ړ�����.
	// to	�ړ���ւ̃x�N�g��
	bool ShapeOblong::move(const D2D1_POINT_2F to) noexcept
	{
		const D2D1_POINT_2F pt{
			m_start.x + to.x, m_start.y + to.y
		};
		if (set_pos_start(pt)) {
			return true;
		}
		return false;
	}

	// �l��, �w�肵�����ʂ̓_�Ɋi�[����.
	// val	�l
	// loc	����
	// snap_point
	bool ShapeOblong::set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool /*keep_aspect*/) noexcept
	{
		bool done = false;
		switch (loc) {
		case LOC_TYPE::LOC_SHEET:
		{
			D2D1_POINT_2F pt;
			pt_round(val, PT_ROUND, pt);
			D2D1_POINT_2F to;
			pt_sub(pt, m_start, to);
			if (pt_abs2(to) >= FLT_MIN) {
				pt_add(m_start, to, m_start);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_NW:
		{
			D2D1_POINT_2F pt;
			pt_round(val, PT_ROUND, pt);
			D2D1_POINT_2F to;
			pt_sub(pt, m_start, to);
			if (pt_abs2(to) >= FLT_MIN) {
				pt_add(m_start, to, m_start);
				pt_sub(m_lineto, to, m_lineto);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_SE:
		{
			D2D1_POINT_2F pt;
			pt_round(val, PT_ROUND, pt);
			D2D1_POINT_2F to;
			pt_sub(pt, D2D1_POINT_2F{ m_start.x + m_lineto.x, m_start.y + m_lineto.y }, to);
			if (pt_abs2(to) >= FLT_MIN) {
				pt_add(m_lineto, to, m_lineto);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_NE:
		{
			D2D1_POINT_2F pt;
			pt_round(val, PT_ROUND, pt);
			D2D1_POINT_2F to;
			pt_sub(pt, D2D1_POINT_2F{ m_start.x + m_lineto.x, m_start.y }, to);
			if (pt_abs2(to) >= FLT_MIN) {
				m_start.y += to.y;
				pt_add(m_lineto, to.x, -to.y, m_lineto);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_SW:
		{
			D2D1_POINT_2F pt;
			pt_round(val, PT_ROUND, pt);
			D2D1_POINT_2F to;
			pt_sub(pt, D2D1_POINT_2F{ m_start.x, m_start.y + m_lineto.y }, to);
			if (pt_abs2(to) >= FLT_MIN) {
				m_start.x += to.x;
				pt_add(m_lineto, -to.x, to.y, m_lineto);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_WEST:
		{
			const double to_x = std::round((static_cast<double>(val.x) - m_start.x) / PT_ROUND) * PT_ROUND;
			if (to_x <= -FLT_MIN || to_x >= FLT_MIN) {
				m_lineto.x = static_cast<FLOAT>(m_lineto.x - to_x);
				m_start.x = static_cast<FLOAT>(m_start.x + to_x);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_EAST:
		{
			const double to_x = std::round((static_cast<double>(val.x) - m_start.x - m_lineto.x) / PT_ROUND) * PT_ROUND;
			if (to_x <= -FLT_MIN || to_x >= FLT_MIN) {
				m_lineto.x += static_cast<FLOAT>(to_x);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_NORTH:
		{
			const double to_y = std::round((static_cast<double>(val.y) - m_start.y) / PT_ROUND) * PT_ROUND;
			if (to_y <= -FLT_MIN || to_y >= FLT_MIN) {
				m_lineto.y = static_cast<FLOAT>(m_lineto.y - to_y);
				m_start.y = static_cast<FLOAT>(m_start.y + to_y);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_SOUTH:
		{
			const double to_y = std::round(
				(static_cast<double>(val.y) - m_start.y - m_lineto.y) / PT_ROUND) * PT_ROUND;
			if (to_y <= -FLT_MIN || to_y >= FLT_MIN) {
				m_lineto.y += static_cast<FLOAT>(to_y);
				done = true;
			}
		}
		break;
		default:
			return false;
		}
		if (snap_point >= FLT_MIN) {
			// �I�_�ւ̍����� x �l��, ���E�������������肷��.
			if (m_lineto.x > -snap_point && m_lineto.x < snap_point) {
				if (loc == LOC_TYPE::LOC_NE) {
					m_start.x += m_lineto.x;
				}
				m_lineto.x = 0.0f;
				done = true;
			}
			if (m_lineto.y > -snap_point && m_lineto.y < snap_point) {
				if (loc == LOC_TYPE::LOC_NE) {
					m_start.y += m_lineto.y;
				}
				m_lineto.y = 0.0f;
				done = true;
			}
		}
		return done;
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	bool ShapeOblong::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F p;	// �ۂ߂��l.
		pt_round(val, PT_ROUND, p);
		if (!equal(m_start, p)) {
			m_start = p;
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// start	�n�_
	// end_to	�Ίp�_�ւ̈ʒu�x�N�g��
	// page	����
	ShapeOblong::ShapeOblong(const D2D1_POINT_2F start, const D2D1_POINT_2F end_to, const Shape* prop) :
		ShapeStroke(prop),
		m_start(start),
		m_lineto(end_to)
	{
		prop->get_fill_color(m_fill_color);
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapeOblong::ShapeOblong(DataReader const& dt_reader) :
		ShapeStroke(dt_reader)
	{
		m_start.x = dt_reader.ReadSingle();
		m_start.y = dt_reader.ReadSingle();
		m_lineto.x = dt_reader.ReadSingle();
		m_lineto.y = dt_reader.ReadSingle();
		const auto r = dt_reader.ReadSingle();
		const auto g = dt_reader.ReadSingle();
		const auto b = dt_reader.ReadSingle();
		const auto a = dt_reader.ReadSingle();
		m_fill_color.r = min(max(r, 0.0f), 1.0f);
		m_fill_color.g = min(max(g, 0.0f), 1.0f);
		m_fill_color.b = min(max(b, 0.0f), 1.0f);
		m_fill_color.a = min(max(a, 0.0f), 1.0f);
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapeOblong::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);

		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);
		dt_writer.WriteSingle(m_lineto.x);
		dt_writer.WriteSingle(m_lineto.y);
		dt_writer.WriteSingle(m_fill_color.r);
		dt_writer.WriteSingle(m_fill_color.g);
		dt_writer.WriteSingle(m_fill_color.b);
		dt_writer.WriteSingle(m_fill_color.a);
	}

	// �ߖT�̒��_��������.
	// pt	����_
	// dd	�ߖT�Ƃ݂Ȃ����� (�̓��l), �����藣�ꂽ���_�͋ߖT�Ƃ݂͂Ȃ��Ȃ�.
	// val	����ʒu�̋ߖT�ɂ��钸�_
	// �߂�l	���������� true
	bool ShapeOblong::get_pos_nearest(const D2D1_POINT_2F pt, double& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool flag = false;
		D2D1_POINT_2F q[4];
		const size_t q_cnt = get_verts(q);
		for (size_t i = 0; i < q_cnt; i++) {
			D2D1_POINT_2F r;
			pt_sub(q[i], pt, r);
			const double r_abs = pt_abs2(r);
			if (r_abs < dd) {
				dd = r_abs;
				val = q[i];
				if (!flag) {
					flag = true;
				}
			}
		}
		return flag;
	}

	// ���_�𓾂�.
	// p	���_���i�[����z��
	// �߂�l	���_�̌�
	size_t ShapeOblong::get_verts(D2D1_POINT_2F p[]) const noexcept
	{
		// ����
		p[0] = m_start;
		// �E��
		p[1].x = m_start.x + m_lineto.x;
		p[1].y = m_start.y;
		// �E��
		p[2].x = p[1].x;
		p[2].y = m_start.y + m_lineto.y;
		// �E��
		p[3].x = m_start.x;
		p[3].y = p[2].y;
		return 4;
	}

}