//------------------------------
// shape_rrect.cpp
// �p�ە��`�}�`
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �p�ە��`�̒��_�̔z��
	constexpr ANCH_WHICH ANCH_ROUND[4]{
		ANCH_WHICH::ANCH_R_SE,	// �E���p
		ANCH_WHICH::ANCH_R_NE,	// �E��p
		ANCH_WHICH::ANCH_R_SW,	// �����p
		ANCH_WHICH::ANCH_R_NW	// ����p
	};

	// �p�۔��a���v�Z����.
	static void calc_corner_radius(const D2D1_POINT_2F diff, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad);
	// �p�۔��a�̏c�܂��͉��̐������v�Z����.
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad);

	// �p�۔��a���v�Z����.
	// diff	�p�ە��`�̑Ίp�x�N�g��
	// d_rad	����̊p�۔��a
	// c_rad	�v�Z���ꂽ�p�۔��a
	static void calc_corner_radius(const D2D1_POINT_2F diff, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad)
	{
		calc_corner_radius(diff.x, d_rad.x, c_rad.x);
		calc_corner_radius(diff.y, d_rad.y, c_rad.y);
	}

	// �p�۔��a�̏c�܂��͉��̐������v�Z����.
	// r_len	�p�ە��`�̈�ӂ̑傫��
	// d_rad	���Ƃ̊p�۔��a
	// c_rad	����ꂽ�p�۔��a
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad)
	{
		const double r = r_len * 0.5;
		if (fabs(d_rad) > fabs(r)) {
			// ���Ƃ̊p�۔��a�����`�̑傫���̔����𒴂���Ȃ�,
			// �傫���̔����𓾂�ꂽ�p�۔��a�Ɋi�[����.
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
	void ShapeRRect::draw(SHAPE_DX& dx)
	{
		auto sb = dx.m_shape_brush.get();
		auto ss = m_d2d_stroke_style.get();
		auto sw = static_cast<FLOAT>(m_stroke_width);
		auto dc = dx.m_d2dContext;

		D2D1_POINT_2F r_min;
		pt_add(m_pos, min(m_diff.x, 0.0), min(m_diff.y, 0.0), r_min);
		float rx = std::fabsf(m_corner_rad.x);
		float ry = std::fabsf(m_corner_rad.y);
		float vx = std::fabsf(m_diff.x);
		float vy = std::fabsf(m_diff.y);
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
			sb->SetColor(m_fill_color);
			dc->FillRoundedRectangle(r_rec, sb);
		}
		sb->SetColor(m_stroke_color);
		dc->DrawRoundedRectangle(r_rec, sb, sw, ss);
		if (is_selected()) {
			const auto flag = (std::abs(m_diff.x) > FLT_MIN && std::abs(m_diff.y) > FLT_MIN);
			//if (flag) {
			// D2D1_POINT_2F c_pos;
			// pt_add(r_min, rx, ry, c_pos);
			// anchor_draw_ellipse(c_pos, dx);
			// c_pos.x = r_rec.rect.right - rx;
			// anchor_draw_ellipse(c_pos, dx);
			// c_pos.y = r_rec.rect.bottom - ry;
			// anchor_draw_ellipse(c_pos, dx);
			// c_pos.x = r_min.x + rx;
			// anchor_draw_ellipse(c_pos, dx);
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
				anchor_draw_rect(r_pos[i], dx);
				anchor_draw_rect(r_mid, dx);
			}
			//if (flag != true) {
				D2D1_POINT_2F c_pos;
				pt_add(r_min, rx, ry, c_pos);
				anchor_draw_ellipse(c_pos, dx);
				c_pos.x = r_rec.rect.right - rx;
				anchor_draw_ellipse(c_pos, dx);
				c_pos.y = r_rec.rect.bottom - ry;
				anchor_draw_ellipse(c_pos, dx);
				c_pos.x = r_min.x + rx;
				anchor_draw_ellipse(c_pos, dx);
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
	//	anch	�}�`�̕���.
	//	value	����ꂽ�ʒu.
	//	�߂�l	�Ȃ�
	void ShapeRRect::get_anch_pos(const ANCH_WHICH anch, D2D1_POINT_2F& value) const noexcept
	{
		const double dx = m_diff.x;
		const double dy = m_diff.y;
		const double mx = dx * 0.5;	// ���_
		const double my = dy * 0.5;	// ���_
		const double rx = fabs(mx) < fabs(m_corner_rad.x) ? mx : m_corner_rad.x;	// �p��
		const double ry = fabs(my) < fabs(m_corner_rad.y) ? my : m_corner_rad.y;	// �p��
		switch (anch) {
		case ANCH_WHICH::ANCH_R_NW:
			// ����̊p�ے��S�_�����߂�
			pt_add(m_pos, rx, ry, value);
			break;
		case ANCH_WHICH::ANCH_R_NE:
			// �E��̊p�ے��S�_�����߂�
			pt_add(m_pos, dx - rx, ry, value);
			break;
		case ANCH_WHICH::ANCH_R_SE:
			// �E���̊p�ے��S�_�����߂�
			pt_add(m_pos, dx - rx, dy - ry, value);
			break;
		case ANCH_WHICH::ANCH_R_SW:
			// �����̊p�ے��S�_�����߂�
			pt_add(m_pos, rx, dy - ry, value);
			break;
		default:
			ShapeRect::get_anch_pos(anch, value);
			break;
		}
	}

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
				return pt_in_elli(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_max.x - r_rad.x;
		c_pos.y = r_min.y + r_rad.y;
		if (t_pos.x > c_pos.x) {
			if (t_pos.y < c_pos.y) {
				return pt_in_elli(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_max.x - r_rad.x;
		c_pos.y = r_max.y - r_rad.y;
		if (t_pos.x > c_pos.x) {
			if (t_pos.y > c_pos.y) {
				return pt_in_elli(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_min.x + r_rad.x;
		c_pos.y = r_max.y - r_rad.y;
		if (t_pos.x < c_pos.x) {
			if (t_pos.y > c_pos.y) {
				return pt_in_elli(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		return true;
	}

	// �ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// a_len	���ʂ̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	ANCH_WHICH ShapeRRect::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const auto flag = (fabs(m_diff.x) > FLT_MIN&& fabs(m_diff.y) > FLT_MIN);
		if (flag) {
			for (uint32_t i = 0; i < 4; i++) {
				// �p�ۂ̒��S�_�𓾂�.
				D2D1_POINT_2F r_cen;
				get_anch_pos(ANCH_ROUND[i], r_cen);
				// �ʒu���p�ۂ̕��ʂɊ܂܂�邩���ׂ�.
				if (pt_in_anch(t_pos, r_cen, a_len)) {
					// �܂܂��Ȃ�p�ۂ̕��ʂ�Ԃ�.
					return ANCH_ROUND[i];
				}
			}
		}
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F r_pos;	// ���`�̒��_
			get_anch_pos(ANCH_CORNER[i], r_pos);
			if (pt_in_anch(t_pos, r_pos, a_len)) {
				return ANCH_CORNER[i];
			}
		}
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F r_pos;	// ���`�̕ӂ̒��_
			get_anch_pos(ANCH_MIDDLE[i], r_pos);
			if (pt_in_anch(t_pos, r_pos, a_len)) {
				return ANCH_MIDDLE[i];
			}
		}
		if (flag != true) {
			for (uint32_t i = 0; i < 4; i++) {
				D2D1_POINT_2F r_cen;	// �p�ە����̒��S�_
				get_anch_pos(ANCH_ROUND[i], r_cen);
				if (pt_in_anch(t_pos, r_cen, a_len)) {
					return ANCH_ROUND[i];
				}
			}
		}
		D2D1_POINT_2F r_min;
		D2D1_POINT_2F r_max;
		D2D1_POINT_2F r_rad;
		if (m_diff.x > 0.0f) {
			r_min.x = m_pos.x;
			r_max.x = m_pos.x + m_diff.x;
		}
		else {
			r_min.x = m_pos.x + m_diff.x;
			r_max.x = m_pos.x;
		}
		if (m_diff.y > 0.0f) {
			r_min.y = m_pos.y;
			r_max.y = m_pos.y + m_diff.y;
		}
		else {
			r_min.y = m_pos.y + m_diff.y;
			r_max.y = m_pos.y;
		}
		r_rad.x = std::abs(m_corner_rad.x);
		r_rad.y = std::abs(m_corner_rad.y);
		if (is_opaque(m_stroke_color) != true) {
			return is_opaque(m_fill_color) && pt_in_rrect(t_pos, r_min, r_max, r_rad) ? ANCH_WHICH::ANCH_INSIDE : ANCH_WHICH::ANCH_OUTSIDE;
		}
		const double s_width = max(m_stroke_width, a_len);
		// �O���̊p�ە��`�̔���
		pt_add(r_min, -s_width * 0.5, r_min);
		pt_add(r_max, s_width * 0.5, r_max);
		pt_add(r_rad, s_width * 0.5, r_rad);
		if (pt_in_rrect(t_pos, r_min, r_max, r_rad) != true) {
			return ANCH_WHICH::ANCH_OUTSIDE;
		}
		// �����̊p�ە��`�̔���
		pt_add(r_min, s_width, r_min);
		pt_add(r_max, -s_width, r_max);
		pt_add(r_rad, -s_width, r_rad);
		if (pt_in_rrect(t_pos, r_min, r_max, r_rad) != true) {
			return ANCH_WHICH::ANCH_FRAME;
		}
		return is_opaque(m_fill_color) ? ANCH_WHICH::ANCH_INSIDE : ANCH_WHICH::ANCH_OUTSIDE;
	}

	//	�l��, ���ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
	//	value	�i�[����l
	//	abch	�}�`�̕���
	void ShapeRRect::set_anch_pos(const D2D1_POINT_2F value, const ANCH_WHICH anch)
	{
		D2D1_POINT_2F c_pos;
		D2D1_POINT_2F diff;
		D2D1_POINT_2F rad;

		switch (anch) {
		case ANCH_WHICH::ANCH_R_NW:
			ShapeRRect::get_anch_pos(anch, c_pos);
			pt_sub(value, c_pos, diff);
			pt_add(m_corner_rad, diff, rad);
			calc_corner_radius(m_diff, rad, m_corner_rad);
			break;
		case ANCH_WHICH::ANCH_R_NE:
			ShapeRRect::get_anch_pos(anch, c_pos);
			pt_sub(value, c_pos, diff);
			rad.x = m_corner_rad.x - diff.x;
			rad.y = m_corner_rad.y + diff.y;
			calc_corner_radius(m_diff, rad, m_corner_rad);
			break;
		case ANCH_WHICH::ANCH_R_SE:
			ShapeRRect::get_anch_pos(anch, c_pos);
			pt_sub(value, c_pos, diff);
			rad.x = m_corner_rad.x - diff.x;
			rad.y = m_corner_rad.y - diff.y;
			calc_corner_radius(m_diff, rad, m_corner_rad);
			break;
		case ANCH_WHICH::ANCH_R_SW:
			ShapeRRect::get_anch_pos(anch, c_pos);
			pt_sub(value, c_pos, diff);
			rad.x = m_corner_rad.x + diff.x;
			rad.y = m_corner_rad.y - diff.y;
			calc_corner_radius(m_diff, rad, m_corner_rad);
			break;
		default:
			ShapeRect::set_anch_pos(value, anch);
			if (m_diff.x * m_corner_rad.x < 0.0f) {
				m_corner_rad.x = -m_corner_rad.x;
			}
			if (m_diff.y * m_corner_rad.y < 0.0f) {
				m_corner_rad.y = -m_corner_rad.y;
			}
			break;
		}
	}

	// �}�`���쐬����.
	// s_pos	�J�n�ʒu
	// diff	�J�n�ʒu����̍���
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, const ShapeSheet* attr) :
		ShapeRect::ShapeRect(s_pos, diff, attr)
	{
		calc_corner_radius(m_diff, attr->m_corner_rad, m_corner_rad);
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapeRRect::ShapeRRect(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_corner_rad, dt_reader);
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapeRRect::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeRect::write(dt_writer);
		write(m_corner_rad, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeRRect::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<rect ", dt_writer);
		write_svg(m_pos, "x", "y", dt_writer);
		write_svg(m_diff, "width", "height", dt_writer);
		if (std::round(m_corner_rad.x) != 0.0f && std::round(m_corner_rad.y) != 0.0f) {
			write_svg(m_corner_rad, "rx", "ry", dt_writer);
		}
		write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>", dt_writer);
	}
}