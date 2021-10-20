//------------------------------
// shape_rrect.cpp
// �p�ە��`�}�`
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;

	// �p�۔��a���v�Z����.
	static void calc_corner_radius(const D2D1_POINT_2F d_vec, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad);

	// �p�۔��a�̏c�܂��͉��̐������v�Z����.
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad);

	// �p�۔��a���v�Z����.
	// d_vec	�p�ە��`�̑Ίp�x�N�g��
	// d_rad	����̊p�۔��a
	// c_rad	�v�Z���ꂽ�p�۔��a
	static void calc_corner_radius(const D2D1_POINT_2F d_vec, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad)
	{
		calc_corner_radius(d_vec.x, d_rad.x, c_rad.x);
		calc_corner_radius(d_vec.y, d_rad.y, c_rad.y);
	}

	// �p�۔��a�̏c�܂��͉��̐������v�Z����.
	// r_len	�p�ە��`�̈�ӂ̑傫��
	// d_rad	���Ƃ̊p�۔��a
	// c_rad	����ꂽ�p�۔��a
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad)
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
	void ShapeRRect::draw(D2D_UI& dx)
	{
		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(dx);
		}

		auto s_brush = dx.m_solid_color_brush.get();
		auto s_style = m_d2d_stroke_style.get();
		auto s_width = m_stroke_width;
		auto dc = dx.m_d2d_context;

		D2D1_POINT_2F r_min;
		pt_add(m_pos, min(m_vec[0].x, 0.0), min(m_vec[0].y, 0.0), r_min);
		float rx = std::fabsf(m_corner_rad.x);
		float ry = std::fabsf(m_corner_rad.y);
		float vx = std::fabsf(m_vec[0].x);
		float vy = std::fabsf(m_vec[0].y);
		if (rx > vx * 0.5f) {
			rx = vx * 0.5f;
		}
		if (ry > vy * 0.5f) {
			ry = vy * 0.5f;
		}
		D2D1_ROUNDED_RECT r_rec;
		r_rec.rect.left = r_min.x;
		r_rec.rect.top = r_min.y;
		r_rec.rect.right = r_min.x + vx;
		r_rec.rect.bottom = r_min.y + vy;
		r_rec.radiusX = rx;
		r_rec.radiusY = ry;
		if (is_opaque(m_fill_color)) {
			s_brush->SetColor(m_fill_color);
			dc->FillRoundedRectangle(r_rec, s_brush);
		}
		s_brush->SetColor(m_stroke_color);
		dc->DrawRoundedRectangle(r_rec, s_brush, s_width, s_style);
		if (is_selected()) {
			const auto zero = (std::abs(m_vec[0].x) >= FLT_MIN && std::abs(m_vec[0].y) >= FLT_MIN);
			//if (zero) {
			// D2D1_POINT_2F c_pos;
			// pt_add(r_min, rx, ry, c_pos);
			// anp_draw_ellipse(c_pos, dx);
			// c_pos.x = r_rec.rect.right - rx;
			// anp_draw_ellipse(c_pos, dx);
			// c_pos.y = r_rec.rect.bottom - ry;
			// anp_draw_ellipse(c_pos, dx);
			// c_pos.x = r_min.x + rx;
			// anp_draw_ellipse(c_pos, dx);
			//}
			D2D1_POINT_2F r_pos[4];
			r_pos[0] = r_min;
			r_pos[1].x = r_rec.rect.right;
			r_pos[1].y = r_rec.rect.top;
			r_pos[2].x = r_rec.rect.right;
			r_pos[2].y = r_rec.rect.bottom;
			r_pos[3].x = r_rec.rect.left;
			r_pos[3].y = r_rec.rect.bottom;
			for (uint32_t i = 0, j = 3; i < 4; j = i++) {
				D2D1_POINT_2F r_mid;
				// ���`�̒��_�̃A���J�[��\������.
				// �ӂ̒��_������, ���̃A���J�[��\������.
				pt_avg(r_pos[j], r_pos[i], r_mid);
				anp_draw_rect(r_pos[i], dx);
				anp_draw_rect(r_mid, dx);
			}
			//if (zero != true) {
				D2D1_POINT_2F c_pos;
				pt_add(r_min, rx, ry, c_pos);
				anp_draw_ellipse(c_pos, dx);
				c_pos.x = r_rec.rect.right - rx;
				anp_draw_ellipse(c_pos, dx);
				c_pos.y = r_rec.rect.bottom - ry;
				anp_draw_ellipse(c_pos, dx);
				c_pos.x = r_min.x + rx;
				anp_draw_ellipse(c_pos, dx);
			//}
		}
	}

	// �p�۔��a�𓾂�.
	bool ShapeRRect::get_corner_radius(D2D1_POINT_2F& value) const noexcept
	{
		value = m_corner_rad;
		return true;
	}

	//	���ʂ̈ʒu�𓾂�.
	//	anp	�}�`�̕���.
	//	value	����ꂽ�ʒu.
	//	�߂�l	�Ȃ�
	void ShapeRRect::get_pos_anp(const uint32_t anp, D2D1_POINT_2F& value) const noexcept
	{
		const double dx = m_vec[0].x;
		const double dy = m_vec[0].y;
		const double mx = dx * 0.5;	// ���_
		const double my = dy * 0.5;	// ���_
		const double rx = fabs(mx) < fabs(m_corner_rad.x) ? mx : m_corner_rad.x;	// �p��
		const double ry = fabs(my) < fabs(m_corner_rad.y) ? my : m_corner_rad.y;	// �p��
		switch (anp) {
		case ANP_TYPE::ANP_R_NW:
			// ����̊p�ے��S�_�����߂�
			pt_add(m_pos, rx, ry, value);
			break;
		case ANP_TYPE::ANP_R_NE:
			// �E��̊p�ے��S�_�����߂�
			pt_add(m_pos, dx - rx, ry, value);
			break;
		case ANP_TYPE::ANP_R_SE:
			// �E���̊p�ے��S�_�����߂�
			pt_add(m_pos, dx - rx, dy - ry, value);
			break;
		case ANP_TYPE::ANP_R_SW:
			// �����̊p�ے��S�_�����߂�
			pt_add(m_pos, rx, dy - ry, value);
			break;
		default:
			ShapeRect::get_pos_anp(anp, value);
			break;
		}
	}

	// �ʒu���p�ە��`�Ɋ܂܂�邩���肷��.
	// t_pos	���肷��ʒu
	// r_min	�p�ە��`�̍���ʒu
	// r_max	�p�ە��`�̉E���ʒu
	// r_rad	�p�ۂ̔��a
	// �߂�l	�܂܂��Ȃ� true ��Ԃ�.
	static bool pt_in_rrect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max, const D2D1_POINT_2F r_rad)
	{
		if (t_pos.x < r_min.x) {
			return false;
		}
		if (t_pos.x > r_max.x) {
			return false;
		}
		if (t_pos.y < r_min.y) {
			return false;
		}
		if (t_pos.y > r_max.y) {
			return false;
		}
		D2D1_POINT_2F c_pos;
		pt_add(r_min, r_rad, c_pos);
		if (t_pos.x < c_pos.x) {
			if (t_pos.y < c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_max.x - r_rad.x;
		c_pos.y = r_min.y + r_rad.y;
		if (t_pos.x > c_pos.x) {
			if (t_pos.y < c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_max.x - r_rad.x;
		c_pos.y = r_max.y - r_rad.y;
		if (t_pos.x > c_pos.x) {
			if (t_pos.y > c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_min.x + r_rad.x;
		c_pos.y = r_max.y - r_rad.y;
		if (t_pos.x < c_pos.x) {
			if (t_pos.y > c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		return true;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeRRect::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		// �p�ۂ̉~�ʂ̒��S�_�Ɋ܂܂�邩���肷��.
		// +---------+
		// | 1     3 |
		// |         |
		// | 4     2 |
		// +---------+
		uint32_t anp_r;
		const double mx = m_vec[0].x * 0.5;	// ���_
		const double my = m_vec[0].y * 0.5;	// ���_
		const double rx = fabs(mx) < fabs(m_corner_rad.x) ? mx : m_corner_rad.x;	// �p��
		const double ry = fabs(my) < fabs(m_corner_rad.y) ? my : m_corner_rad.y;	// �p��
		const D2D1_POINT_2F anp_r_nw{ static_cast<FLOAT>(m_pos.x + rx), static_cast<FLOAT>(m_pos.y + ry) };
		if (pt_in_anp(t_pos, anp_r_nw)) {
			anp_r = ANP_TYPE::ANP_R_NW;
		}
		else {
			const D2D1_POINT_2F anp_r_se{ static_cast<FLOAT>(m_pos.x + m_vec[0].x - rx), static_cast<FLOAT>(m_pos.y + m_vec[0].y - ry) };
			if (pt_in_anp(t_pos, anp_r_se)) {
				anp_r = ANP_TYPE::ANP_R_SE;
			}
			else {
				const D2D1_POINT_2F anp_r_ne{ anp_r_se.x, anp_r_nw.y };
				if (pt_in_anp(t_pos, anp_r_ne)) {
					anp_r = ANP_TYPE::ANP_R_NE;
				}
				else {
					const D2D1_POINT_2F anp_r_sw{ anp_r_nw.x, anp_r_se.y };
					if (pt_in_anp(t_pos, anp_r_ne)) {
						anp_r = ANP_TYPE::ANP_R_NE;
					}
					else {
						anp_r = ANP_TYPE::ANP_SHEET;
					}
				}
			}
		}
		// �p�ۂ̉~�ʂ̒��S�_�Ɋ܂܂��,
		if (anp_r != ANP_TYPE::ANP_SHEET &&
			// ����, ���`�̑傫�����}�`�̕��ʂ̑傫�����傫�������肷��.
			fabs(m_vec[0].x) > Shape::s_anp_len && fabs(m_vec[0].y) > Shape::s_anp_len) {
			return anp_r;
		}
		// ���`�̊e���_�Ɋ܂܂�邩���肷��.
		const uint32_t anp_v = hit_test_anp(t_pos);
		if (anp_v != ANP_TYPE::ANP_SHEET) {
			return anp_v;
		}
		// ���_�Ɋ܂܂ꂸ, �p�ۂ̉~�ʂ̒��S�_�Ɋ܂܂�邩���肷��.
		else if (anp_r != ANP_TYPE::ANP_SHEET) {
			return anp_r;
		}

		// �p�ە��`�𐳋K������.
		D2D1_POINT_2F r_min;
		D2D1_POINT_2F r_max;
		D2D1_POINT_2F r_rad;
		if (m_vec[0].x > 0.0f) {
			r_min.x = m_pos.x;
			r_max.x = m_pos.x + m_vec[0].x;
		}
		else {
			r_min.x = m_pos.x + m_vec[0].x;
			r_max.x = m_pos.x;
		}
		if (m_vec[0].y > 0.0f) {
			r_min.y = m_pos.y;
			r_max.y = m_pos.y + m_vec[0].y;
		}
		else {
			r_min.y = m_pos.y + m_vec[0].y;
			r_max.y = m_pos.y;
		}
		r_rad.x = std::abs(m_corner_rad.x);
		r_rad.y = std::abs(m_corner_rad.y);

		// ���g�������܂��͑��� 0 �����肷��.
		if (!is_opaque(m_stroke_color) || m_stroke_width < FLT_MIN) {
			// �h��Ԃ��F���s����, ���p�ە��`���̂��̂Ɋ܂܂�邩���肷��.
			if (is_opaque(m_fill_color) && pt_in_rrect(t_pos, r_min, r_max, r_rad)) {
				return ANP_TYPE::ANP_FILL;
			}
		}
		// ���g�̐F���s����, �������� 0 ���傫��.
		else {
			// �O���̊p�ە��`�Ɋ܂܂�邩����
			const double s_width = max(m_stroke_width, Shape::s_anp_len);
			D2D1_POINT_2F s_min, s_max, s_rad;
			pt_add(r_min, -s_width * 0.5, s_min);
			pt_add(r_max, s_width * 0.5, s_max);
			pt_add(r_rad, s_width * 0.5, s_rad);
			if (pt_in_rrect(t_pos, s_min, s_max, s_rad)) {
				// �����̊p�ە��`���t�]���ĂȂ�, ���ʒu���p�ە��`�Ɋ܂܂�邩���肷��.
				D2D1_POINT_2F u_min, u_max, u_rad;
				pt_add(s_min, s_width, u_min);
				pt_add(s_max, -s_width, u_max);
				pt_add(s_rad, -s_width, u_rad);
				if (u_min.x < u_max.x && u_min.y < u_max.y && pt_in_rrect(t_pos, r_min, r_max, r_rad)) {
					// �����̊p�ە��`�Ɋ܂܂��ꍇ, �h��Ԃ��F���s�����Ȃ�, ANP_FILL ��Ԃ�.
					if (is_opaque(m_fill_color)) {
						return ANP_TYPE::ANP_FILL;
					}
				}
				else {
					// �O���Ɋ܂܂�, �����Ɋ܂܂�Ȃ��Ȃ� ANP_STROKE ��Ԃ�.
					return ANP_TYPE::ANP_STROKE;
				}
			}
		}
		return ANP_TYPE::ANP_SHEET;
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu������.
	// value	�l
	// anp	�}�`�̕���
	// limit	���E���� (���̒��_�Ƃ̋��������̒l�����ɂȂ�Ȃ�, ���̒��_�Ɉʒu�ɍ��킹��)
	bool ShapeRRect::set_pos_anp(const D2D1_POINT_2F value, const uint32_t anp, const float limit, const bool /*keep_aspect*/) noexcept
	{
		D2D1_POINT_2F c_pos;
		D2D1_POINT_2F vec;
		D2D1_POINT_2F rad;
		D2D1_POINT_2F new_pos;

		switch (anp) {
		case ANP_TYPE::ANP_R_NW:
			ShapeRRect::get_pos_anp(anp, c_pos);
			pt_round(value, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			pt_add(m_corner_rad, vec, rad);
			calc_corner_radius(m_vec[0], rad, m_corner_rad);
			break;
		case ANP_TYPE::ANP_R_NE:
			ShapeRRect::get_pos_anp(anp, c_pos);
			pt_round(value, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x - vec.x;
			rad.y = m_corner_rad.y + vec.y;
			calc_corner_radius(m_vec[0], rad, m_corner_rad);
			break;
		case ANP_TYPE::ANP_R_SE:
			ShapeRRect::get_pos_anp(anp, c_pos);
			pt_round(value, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x - vec.x;
			rad.y = m_corner_rad.y - vec.y;
			calc_corner_radius(m_vec[0], rad, m_corner_rad);
			break;
		case ANP_TYPE::ANP_R_SW:
			ShapeRRect::get_pos_anp(anp, c_pos);
			pt_round(value, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x + vec.x;
			rad.y = m_corner_rad.y - vec.y;
			calc_corner_radius(m_vec[0], rad, m_corner_rad);
			break;
		default:
			if (!ShapeRect::set_pos_anp(value, anp, limit, false)) {
				return false;
			}
			if (m_vec[0].x * m_corner_rad.x < 0.0f) {
				m_corner_rad.x = -m_corner_rad.x;
			}
			if (m_vec[0].y * m_corner_rad.y < 0.0f) {
				m_corner_rad.y = -m_corner_rad.y;
			}
			break;
		}
		const double d = static_cast<double>(limit);
		if (pt_abs2(m_corner_rad) < d * d) {
			m_corner_rad.x = m_corner_rad.y = 0.0f;
		}
		return true;
	}

	// �}�`���쐬����.
	// b_pos	�͂ޗ̈�̎n�_
	// b_vec	�͂ޗ̈�̏I�_�ւ̍���
	// s_attr	����
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr) :
		ShapeRect::ShapeRect(b_pos, b_vec, s_attr)
	{
		calc_corner_radius(m_vec[0], s_attr->m_corner_rad, m_corner_rad);
	}

	// �f�[�^���[�_�[����}�`��ǂݍ���.
	ShapeRRect::ShapeRRect(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		dt_read(m_corner_rad, dt_reader);
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapeRRect::write(DataWriter const& dt_writer) const
	{
		ShapeRect::write(dt_writer);
		dt_write(m_corner_rad, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeRRect::write_svg(DataWriter const& dt_writer) const
	{
		dt_write_svg("<rect ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_vec[0], "width", "height", dt_writer);
		if (std::round(m_corner_rad.x) != 0.0f && std::round(m_corner_rad.y) != 0.0f) {
			dt_write_svg(m_corner_rad, "rx", "ry", dt_writer);
		}
		dt_write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		dt_write_svg("/>", dt_writer);
	}
}