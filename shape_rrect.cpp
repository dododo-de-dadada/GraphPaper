//------------------------------
// shape_rrect.cpp
// �p�ە��`�}�`
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �p�۔��a���v�Z����.
	static void rrect_corner_radius(const D2D1_POINT_2F pos, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad);

	// �p�۔��a�̏c�܂��͉��̐������v�Z����.
	static void rrect_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad);

	// �p�۔��a���v�Z����.
	// pos	�Ίp�_�ւ̈ʒu�x�N�g��
	// d_rad	����̊p�۔��a
	// c_rad	�v�Z���ꂽ�p�۔��a
	static void rrect_corner_radius(const D2D1_POINT_2F pos, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad)
	{
		rrect_corner_radius(pos.x, d_rad.x, c_rad.x);
		rrect_corner_radius(pos.y, d_rad.y, c_rad.y);
	}

	// �p�۔��a��
	// r_len	�p�ە��`�̈�ӂ̑傫��
	// d_rad	���Ƃ̊p�۔��a
	// c_rad	����ꂽ�p�۔��a
	static void rrect_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad)
	{
		const double r = r_len * 0.5;
		// ���Ƃ̊p�۔��a�����`�̑傫���̔����𒴂��Ȃ��悤�ɂ���.
		if (fabs(d_rad) > fabs(r)) {
			c_rad = static_cast<FLOAT>(r);
		}
		else if (r_len * d_rad < 0.0f) {
			// �p�ە��`�̑傫���Ƃ��Ƃ̊p�۔��a�̕������t�Ȃ�,
			// ���Ƃ̊p�۔��a�̕������t�ɂ����l��
			// ����ꂽ�p�۔��a�Ɋi�[����.
			c_rad = -d_rad;
		}
		else {
			c_rad = d_rad;
		}
	}

	// �}�`��\������.
	void ShapeRRect::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		if (m_d2d_stroke_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			create_stroke_style(factory);
		}

		D2D1_POINT_2F r_lt;
		pt_add(m_start, min(m_pos.x, 0.0), min(m_pos.y, 0.0), r_lt);
		float rx = std::fabsf(m_corner_radius.x);
		float ry = std::fabsf(m_corner_radius.y);
		float vx = std::fabsf(m_pos.x);
		float vy = std::fabsf(m_pos.y);
		if (rx > vx * 0.5f) {
			rx = vx * 0.5f;
		}
		if (ry > vy * 0.5f) {
			ry = vy * 0.5f;
		}
		const D2D1_ROUNDED_RECT r_rec{
			{ r_lt.x, r_lt.y, r_lt.x + vx,  r_lt.y + vy },
			rx, ry
		};
		/*
		r_rec.rect.left = r_lt.x;
		r_rec.rect.top = r_lt.y;
		r_rec.rect.right = r_lt.x + vx;
		r_rec.rect.bottom = r_lt.y + vy;
		r_rec.radiusX = rx;
		r_rec.radiusY = ry;
		*/
		if (is_opaque(m_fill_color)) {
			brush->SetColor(m_fill_color);
			target->FillRoundedRectangle(r_rec, brush);
		}
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			brush->SetColor(m_stroke_color);
			target->DrawRoundedRectangle(r_rec, brush, m_stroke_width, m_d2d_stroke_style.get());
		}

		if (m_loc_show && is_selected()) {
			// �⏕����`��
			if (m_stroke_width >= Shape::m_loc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawRoundedRectangle(r_rec, brush, 2.0f * Shape::m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawRoundedRectangle(r_rec, brush, Shape::m_aux_width, m_aux_style.get());
			}
			// �p�ۂ̒��S�_��`��.
			D2D1_POINT_2F circle[4]{	// �~��̓_
				{ r_lt.x + rx, r_lt.y + ry },
				{ r_lt.x + vx - rx, r_lt.y + ry },
				{ r_lt.x + vx - rx, r_lt.y + vy - ry },
				{ r_lt.x + rx, r_lt.y + vy - ry }
			};
			loc_draw_circle(circle[2], target, brush);
			loc_draw_circle(circle[3], target, brush);
			loc_draw_circle(circle[1], target, brush);
			loc_draw_circle(circle[0], target, brush);
			// �}�`�̕��ʂ�`��.
			draw_loc();
		}
	}

	// �p�۔��a�𓾂�.
	bool ShapeRRect::get_corner_radius(D2D1_POINT_2F& val) const noexcept
	{
		val = m_corner_radius;
		return true;
	}

	// �w�肵�����ʂ̓_�𓾂�.
	void ShapeRRect::get_pos_loc(
		const uint32_t loc,	// ����
		D2D1_POINT_2F& val	// ����ꂽ�l
	) const noexcept
	{
		const double dx = m_pos.x;	// ���� x
		const double dy = m_pos.y;	// ���� y
		const double mx = dx * 0.5;	// ���_ x
		const double my = dy * 0.5;	// ���_ y
		const double rx = fabs(mx) < fabs(m_corner_radius.x) ? mx : m_corner_radius.x;	// �p�� x
		const double ry = fabs(my) < fabs(m_corner_radius.y) ? my : m_corner_radius.y;	// �p�� y

		switch (loc) {
		case LOC_TYPE::LOC_R_NW:
			// ����̊p�ے��S�_�����߂�
			pt_add(m_start, rx, ry, val);
			break;
		case LOC_TYPE::LOC_R_NE:
			// �E��̊p�ے��S�_�����߂�
			pt_add(m_start, dx - rx, ry, val);
			break;
		case LOC_TYPE::LOC_R_SE:
			// �E���̊p�ے��S�_�����߂�
			pt_add(m_start, dx - rx, dy - ry, val);
			break;
		case LOC_TYPE::LOC_R_SW:
			// �����̊p�ے��S�_�����߂�
			pt_add(m_start, rx, dy - ry, val);
			break;
		default:
			ShapeRect::get_pos_loc(loc, val);
			break;
		}
	}

	// �ʒu���p�ە��`�Ɋ܂܂�邩���肷��.
	// test	���肳���_
	// r_lt	�p�ە��`�̍���ʒu
	// r_rb	�p�ە��`�̉E���ʒu
	// r_rad	�p�ۂ̔��a
	// �߂�l	�܂܂��Ȃ� true ��Ԃ�.
	static bool pt_in_rrect(const D2D1_POINT_2F test, const D2D1_POINT_2F r_lt, const D2D1_POINT_2F r_rb, const D2D1_POINT_2F r_rad)
	{
		if (test.x < r_lt.x) {
			return false;
		}
		if (test.x > r_rb.x) {
			return false;
		}
		if (test.y < r_lt.y) {
			return false;
		}
		if (test.y > r_rb.y) {
			return false;
		}
		D2D1_POINT_2F ctr;	// �p�ۂ̒��S�_
		pt_add(r_lt, r_rad, ctr);
		if (test.x < ctr.x) {
			if (test.y < ctr.y) {
				return pt_in_ellipse(test, ctr, r_rad.x, r_rad.y);
			}
		}
		ctr.x = r_rb.x - r_rad.x;
		ctr.y = r_lt.y + r_rad.y;
		if (test.x > ctr.x) {
			if (test.y < ctr.y) {
				return pt_in_ellipse(test, ctr, r_rad.x, r_rad.y);
			}
		}
		ctr.x = r_rb.x - r_rad.x;
		ctr.y = r_rb.y - r_rad.y;
		if (test.x > ctr.x) {
			if (test.y > ctr.y) {
				return pt_in_ellipse(test, ctr, r_rad.x, r_rad.y);
			}
		}
		ctr.x = r_lt.x + r_rad.x;
		ctr.y = r_rb.y - r_rad.y;
		if (test.x < ctr.x) {
			if (test.y > ctr.y) {
				return pt_in_ellipse(test, ctr, r_rad.x, r_rad.y);
			}
		}
		return true;
	}

	// �}�`���_���܂ނ����肷��.
	// �߂�l	�_���܂ޕ���
	uint32_t ShapeRRect::hit_test(
		const D2D1_POINT_2F t	// ���肳���_
	) const noexcept
	{
		// �p�ۂ̉~�ʂ̒��S�_�Ɋ܂܂�邩���肷��.
		// +---------+
		// | 2     4 |
		// |         |
		// | 3     1 |
		// +---------+
		uint32_t loc_r;
		const double mx = m_pos.x * 0.5;	// ���ԓ_
		const double my = m_pos.y * 0.5;	// ���ԓ_
		const double rx = fabs(mx) < fabs(m_corner_radius.x) ? mx : m_corner_radius.x;	// �p��
		const double ry = fabs(my) < fabs(m_corner_radius.y) ? my : m_corner_radius.y;	// �p��
		const D2D1_POINT_2F loc_r_nw{
			static_cast<FLOAT>(m_start.x + rx), 
			static_cast<FLOAT>(m_start.y + ry)
		};
		const D2D1_POINT_2F loc_r_se{
			static_cast<FLOAT>(m_start.x + m_pos.x - rx),
			static_cast<FLOAT>(m_start.y + m_pos.y - ry)
		};
		const D2D1_POINT_2F loc_r_ne{ loc_r_se.x, loc_r_nw.y };
		const D2D1_POINT_2F loc_r_sw{ loc_r_nw.x, loc_r_se.y };
		if (loc_hit_test(t, loc_r_se, m_loc_width)) {
			loc_r = LOC_TYPE::LOC_R_SE;
		}
		else if (loc_hit_test(t, loc_r_nw, m_loc_width)) {
			loc_r = LOC_TYPE::LOC_R_NW;
		}
		else if (loc_hit_test(t, loc_r_sw, m_loc_width)) {
			loc_r = LOC_TYPE::LOC_R_SW;
		}
		else if (loc_hit_test(t, loc_r_ne, m_loc_width)) {
			loc_r = LOC_TYPE::LOC_R_NE;
		}
		else {
			loc_r = LOC_TYPE::LOC_PAGE;
		}

		// �p�ۂ̂����ꂩ�̒��S�_�Ɋ܂܂��,
		if (loc_r != LOC_TYPE::LOC_PAGE &&
			// ����, ���`�̑傫�����}�`�̕��ʂ̔{�̑傫�����傫�������肷��.
			fabs(m_pos.x) > m_loc_width && fabs(m_pos.y) > 2.0f * m_loc_width) {
			return loc_r;
		}
		const uint32_t loc_v = rect_loc_hit_test(m_start, m_pos, t, m_loc_width);
		if (loc_v != LOC_TYPE::LOC_PAGE) {
			return loc_v;
		}
		// ���_�Ɋ܂܂ꂸ, �p�ۂ̉~�ʂ̒��S�_�Ɋ܂܂�邩���肷��.
		else if (loc_r != LOC_TYPE::LOC_PAGE) {
			return loc_r;
		}

		D2D1_POINT_2F r_lt;	// ����ʒu
		D2D1_POINT_2F r_rb;	// �E���ʒu
		D2D1_POINT_2F r_rad;	// �p�ۂ̔��a
		if (m_pos.x > 0.0f) {
			r_lt.x = m_start.x;
			r_rb.x = m_start.x + m_pos.x;
		}
		else {
			r_lt.x = m_start.x + m_pos.x;
			r_rb.x = m_start.x;
		}
		if (m_pos.y > 0.0f) {
			r_lt.y = m_start.y;
			r_rb.y = m_start.y + m_pos.y;
		}
		else {
			r_lt.y = m_start.y + m_pos.y;
			r_rb.y = m_start.y;
		}
		r_rad.x = std::abs(m_corner_radius.x);
		r_rad.y = std::abs(m_corner_radius.y);

		// ���g�������܂��͑��� 0 �����肷��.
		if (!is_opaque(m_stroke_color) || m_stroke_width < FLT_MIN) {
			// �h��Ԃ��F���s����, ���p�ە��`���̂��̂Ɋ܂܂�邩���肷��.
			if (is_opaque(m_fill_color) && pt_in_rrect(t, r_lt, r_rb, r_rad)) {
				return LOC_TYPE::LOC_FILL;
			}
		}
		// ���g�̐F���s����, �������� 0 ���傫��.
		else {
			// �g�債���p�ە��`�Ɋ܂܂�邩����
			const double ew = max(m_stroke_width, m_loc_width);
			D2D1_POINT_2F e_lt, e_rb, e_rad;	// �g�債���p�ە��`
			pt_add(r_lt, -ew * 0.5, e_lt);
			pt_add(r_rb, ew * 0.5, e_rb);
			pt_add(r_rad, ew * 0.5, e_rad);
			if (pt_in_rrect(t, e_lt, e_rb, e_rad)) {
				// �k�������p�ە��`���t�]���ĂȂ�, ���ʒu���k�������p�ە��`�Ɋ܂܂�邩���肷��.
				D2D1_POINT_2F s_lt, s_rb, s_rad;	// �k�������p�ە��`
				pt_add(e_lt, ew, s_lt);
				pt_add(e_rb, -ew, s_rb);
				pt_add(e_rad, -ew, s_rad);
				if (s_lt.x < s_rb.x && s_lt.y < s_rb.y && pt_in_rrect(t, s_lt, s_rb, s_rad)) {
					// �h��Ԃ��F���s�����Ȃ�, LOC_FILL ��Ԃ�.
					if (is_opaque(m_fill_color)) {
						return LOC_TYPE::LOC_FILL;
					}
				}
				else {
					// �g�債���p�ە��`�Ɋ܂܂�, �k�������p�ە��`�Ɋ܂܂�Ȃ��Ȃ� LOC_STROKE ��Ԃ�.
					return LOC_TYPE::LOC_STROKE;
				}
			}
		}
		return LOC_TYPE::LOC_PAGE;
	}

	// �l��, �w�肵�����ʂ̓_�Ɋi�[����.
	// snap_point	���̓_�Ƃ̊Ԋu (���̒l��藣�ꂽ�_�͖�������)
	bool ShapeRRect::set_pos_loc(
		const D2D1_POINT_2F val,	// �l
		const uint32_t loc,	// ����
		const float snap_point,
		const bool /*keep_aspect*/
	) noexcept
	{
		D2D1_POINT_2F a;	// �}�`�̕��ʂ̈ʒu
		D2D1_POINT_2F p;	// �ʒu�x�N�g��
		D2D1_POINT_2F r;	// �p�۔��a
		D2D1_POINT_2F q;	// �V�����_

		switch (loc) {
		case LOC_TYPE::LOC_R_NW:
			ShapeRRect::get_pos_loc(loc, a);
			pt_round(val, PT_ROUND, q);
			pt_sub(q, a, p);
			if (pt_abs2(p) < FLT_MIN) {
				return false;
			}
			pt_add(m_corner_radius, p, r);
			rrect_corner_radius(m_pos, r, m_corner_radius);
			break;
		case LOC_TYPE::LOC_R_NE:
			ShapeRRect::get_pos_loc(loc, a);
			pt_round(val, PT_ROUND, q);
			pt_sub(q, a, p);
			if (pt_abs2(p) < FLT_MIN) {
				return false;
			}
			r.x = m_corner_radius.x - p.x;
			r.y = m_corner_radius.y + p.y;
			rrect_corner_radius(m_pos, r, m_corner_radius);
			break;
		case LOC_TYPE::LOC_R_SE:
			ShapeRRect::get_pos_loc(loc, a);
			pt_round(val, PT_ROUND, q);
			pt_sub(q, a, p);
			if (pt_abs2(p) < FLT_MIN) {
				return false;
			}
			r.x = m_corner_radius.x - p.x;
			r.y = m_corner_radius.y - p.y;
			rrect_corner_radius(m_pos, r, m_corner_radius);
			break;
		case LOC_TYPE::LOC_R_SW:
			ShapeRRect::get_pos_loc(loc, a);
			pt_round(val, PT_ROUND, q);
			pt_sub(q, a, p);
			if (pt_abs2(p) < FLT_MIN) {
				return false;
			}
			r.x = m_corner_radius.x + p.x;
			r.y = m_corner_radius.y - p.y;
			rrect_corner_radius(m_pos, r, m_corner_radius);
			break;
		default:
			if (!ShapeRect::set_pos_loc(val, loc, snap_point, false)) {
				return false;
			}
			if (m_pos.x * m_corner_radius.x < 0.0f) {
				m_corner_radius.x = -m_corner_radius.x;
			}
			if (m_pos.y * m_corner_radius.y < 0.0f) {
				m_corner_radius.y = -m_corner_radius.y;
			}
			break;
		}
		const double dd = static_cast<double>(snap_point) * static_cast<double>(snap_point);
		if (pt_abs2(m_corner_radius) < dd) {
			m_corner_radius.x = m_corner_radius.y = 0.0f;
		}
		return true;
	}

	// �}�`���쐬����.
	// start	�n�_
	// pos	�Ίp�_�ւ̈ʒu�x�N�g��
	// page	����
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page) :
		ShapeRect::ShapeRect(start, pos, page)
	{
		float g_base;
		page->get_grid_base(g_base);
		rrect_corner_radius(pos, D2D1_POINT_2F{ g_base + 1.0f, g_base + 1.0f }, m_corner_radius);
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapeRRect::ShapeRRect(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		m_corner_radius = D2D1_POINT_2F{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (m_corner_radius.x < 0.0f || m_corner_radius.x > 0.5f * fabs(m_pos.x)) {
			m_corner_radius.x = 0.0f;
		}
		if (m_corner_radius.y < 0.0f || m_corner_radius.y > 0.5f * fabs(m_pos.y)) {
			m_corner_radius.y = 0.0f;
		}
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapeRRect::write(DataWriter const& dt_writer) const
	{
		ShapeRect::write(dt_writer);
		dt_writer.WriteSingle(m_corner_radius.x);
		dt_writer.WriteSingle(m_corner_radius.y);
	}

}