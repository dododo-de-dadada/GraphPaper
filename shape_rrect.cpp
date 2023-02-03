//------------------------------
// shape_rrect.cpp
// �p�ە��`�}�`
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

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
	void ShapeRRect::draw(void)
	{
		ID2D1Factory* const factory = Shape::s_d2d_factory;
		ID2D1RenderTarget* const target = Shape::s_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::s_d2d_color_brush;

		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}

		D2D1_POINT_2F r_lt;
		pt_add(m_start, min(m_vec[0].x, 0.0), min(m_vec[0].y, 0.0), r_lt);
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
		r_rec.rect.left = r_lt.x;
		r_rec.rect.top = r_lt.y;
		r_rec.rect.right = r_lt.x + vx;
		r_rec.rect.bottom = r_lt.y + vy;
		r_rec.radiusX = rx;
		r_rec.radiusY = ry;
		if (is_opaque(m_fill_color)) {
			brush->SetColor(m_fill_color);
			target->FillRoundedRectangle(r_rec, brush);
		}
		brush->SetColor(m_stroke_color);
		target->DrawRoundedRectangle(r_rec, brush, m_stroke_width, m_d2d_stroke_style.get());
		if (is_selected()) {
			D2D1_MATRIX_3X2_F t32;
			target->GetTransform(&t32);
			const auto a_len = Shape::s_anc_len / t32._11;
			D2D1_POINT_2F c_pos[4]{
				{ r_lt.x + rx, r_lt.y + ry },
				{ r_lt.x + vx - rx, r_lt.y + ry },
				{ r_lt.x + vx - rx, r_lt.y + vy - ry },
				{ r_lt.x + rx, r_lt.y + vy - ry }
			};
			anc_draw_ellipse(c_pos[2], a_len, target, brush);
			anc_draw_ellipse(c_pos[3], a_len, target, brush);
			anc_draw_ellipse(c_pos[1], a_len, target, brush);
			anc_draw_ellipse(c_pos[0], a_len, target, brush);
			draw_anc(a_len);
		}
	}

	// �p�۔��a�𓾂�.
	bool ShapeRRect::get_corner_radius(D2D1_POINT_2F& val) const noexcept
	{
		val = m_corner_rad;
		return true;
	}

	//	���ʂ̈ʒu�𓾂�.
	//	anc	�}�`�̕���.
	//	val	����ꂽ�ʒu.
	//	�߂�l	�Ȃ�
	void ShapeRRect::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		const double dx = m_vec[0].x;	// ���� x
		const double dy = m_vec[0].y;	// ���� y
		const double mx = dx * 0.5;	// ���_ x
		const double my = dy * 0.5;	// ���_ y
		const double rx = fabs(mx) < fabs(m_corner_rad.x) ? mx : m_corner_rad.x;	// �p�� x
		const double ry = fabs(my) < fabs(m_corner_rad.y) ? my : m_corner_rad.y;	// �p�� y

		switch (anc) {
		case ANC_TYPE::ANC_R_NW:
			// ����̊p�ے��S�_�����߂�
			pt_add(m_start, rx, ry, val);
			break;
		case ANC_TYPE::ANC_R_NE:
			// �E��̊p�ے��S�_�����߂�
			pt_add(m_start, dx - rx, ry, val);
			break;
		case ANC_TYPE::ANC_R_SE:
			// �E���̊p�ے��S�_�����߂�
			pt_add(m_start, dx - rx, dy - ry, val);
			break;
		case ANC_TYPE::ANC_R_SW:
			// �����̊p�ے��S�_�����߂�
			pt_add(m_start, rx, dy - ry, val);
			break;
		default:
			ShapeRect::get_pos_anc(anc, val);
			break;
		}
	}

	// �ʒu���p�ە��`�Ɋ܂܂�邩���肷��.
	// t_pos	���肷��ʒu
	// r_lt	�p�ە��`�̍���ʒu
	// r_rb	�p�ە��`�̉E���ʒu
	// r_rad	�p�ۂ̔��a
	// �߂�l	�܂܂��Ȃ� true ��Ԃ�.
	static bool pt_in_rrect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_lt, const D2D1_POINT_2F r_rb, const D2D1_POINT_2F r_rad)
	{
		if (t_pos.x < r_lt.x) {
			return false;
		}
		if (t_pos.x > r_rb.x) {
			return false;
		}
		if (t_pos.y < r_lt.y) {
			return false;
		}
		if (t_pos.y > r_rb.y) {
			return false;
		}
		D2D1_POINT_2F c_pos;
		pt_add(r_lt, r_rad, c_pos);
		if (t_pos.x < c_pos.x) {
			if (t_pos.y < c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_rb.x - r_rad.x;
		c_pos.y = r_lt.y + r_rad.y;
		if (t_pos.x > c_pos.x) {
			if (t_pos.y < c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_rb.x - r_rad.x;
		c_pos.y = r_rb.y - r_rad.y;
		if (t_pos.x > c_pos.x) {
			if (t_pos.y > c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_lt.x + r_rad.x;
		c_pos.y = r_rb.y - r_rad.y;
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
	uint32_t ShapeRRect::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		// �p�ۂ̉~�ʂ̒��S�_�Ɋ܂܂�邩���肷��.
		// +---------+
		// | 1     3 |
		// |         |
		// | 4     2 |
		// +---------+
		uint32_t anc_r;
		const double mx = m_vec[0].x * 0.5;	// ���_
		const double my = m_vec[0].y * 0.5;	// ���_
		const double rx = fabs(mx) < fabs(m_corner_rad.x) ? mx : m_corner_rad.x;	// �p��
		const double ry = fabs(my) < fabs(m_corner_rad.y) ? my : m_corner_rad.y;	// �p��
		const D2D1_POINT_2F anc_r_nw{
			static_cast<FLOAT>(m_start.x + rx), 
			static_cast<FLOAT>(m_start.y + ry)
		};
		if (pt_in_anc(t_pos, anc_r_nw, a_len)) {
			anc_r = ANC_TYPE::ANC_R_NW;
		}
		else {
			const D2D1_POINT_2F anc_r_se{
				static_cast<FLOAT>(m_start.x + m_vec[0].x - rx),
				static_cast<FLOAT>(m_start.y + m_vec[0].y - ry)
			};
			if (pt_in_anc(t_pos, anc_r_se, a_len)) {
				anc_r = ANC_TYPE::ANC_R_SE;
			}
			else {
				const D2D1_POINT_2F anc_r_ne{ anc_r_se.x, anc_r_nw.y };
				if (pt_in_anc(t_pos, anc_r_ne, a_len)) {
					anc_r = ANC_TYPE::ANC_R_NE;
				}
				else {
					const D2D1_POINT_2F anc_r_sw{ anc_r_nw.x, anc_r_se.y };
					if (pt_in_anc(t_pos, anc_r_sw, a_len)) {
						anc_r = ANC_TYPE::ANC_R_SW;
					}
					else {
						anc_r = ANC_TYPE::ANC_PAGE;
					}
				}
			}
		}
		// �p�ۂ̉~�ʂ̒��S�_�Ɋ܂܂��,
		if (anc_r != ANC_TYPE::ANC_PAGE &&
			// ����, ���`�̑傫�����}�`�̕��ʂ̑傫�����傫�������肷��.
			fabs(m_vec[0].x) > a_len && fabs(m_vec[0].y) > a_len) {
			return anc_r;
		}
		// ���`�̊e���_�Ɋ܂܂�邩���肷��.
		const uint32_t anc_v = rect_hit_test_anc(m_start, m_vec[0], t_pos, a_len);
		if (anc_v != ANC_TYPE::ANC_PAGE) {
			return anc_v;
		}
		// ���_�Ɋ܂܂ꂸ, �p�ۂ̉~�ʂ̒��S�_�Ɋ܂܂�邩���肷��.
		else if (anc_r != ANC_TYPE::ANC_PAGE) {
			return anc_r;
		}

		// �p�ە��`�𐳋K������.
		D2D1_POINT_2F r_lt;
		D2D1_POINT_2F r_rb;
		D2D1_POINT_2F r_rad;
		if (m_vec[0].x > 0.0f) {
			r_lt.x = m_start.x;
			r_rb.x = m_start.x + m_vec[0].x;
		}
		else {
			r_lt.x = m_start.x + m_vec[0].x;
			r_rb.x = m_start.x;
		}
		if (m_vec[0].y > 0.0f) {
			r_lt.y = m_start.y;
			r_rb.y = m_start.y + m_vec[0].y;
		}
		else {
			r_lt.y = m_start.y + m_vec[0].y;
			r_rb.y = m_start.y;
		}
		r_rad.x = std::abs(m_corner_rad.x);
		r_rad.y = std::abs(m_corner_rad.y);

		// ���g�������܂��͑��� 0 �����肷��.
		if (!is_opaque(m_stroke_color) || m_stroke_width < FLT_MIN) {
			// �h��Ԃ��F���s����, ���p�ە��`���̂��̂Ɋ܂܂�邩���肷��.
			if (is_opaque(m_fill_color) && pt_in_rrect(t_pos, r_lt, r_rb, r_rad)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		// ���g�̐F���s����, �������� 0 ���傫��.
		else {
			// �ȉ��̎菇��, �k�������p�ە��`�̊O����, �p�ی�_������P�[�X�ł��܂������Ȃ�.
			// �g�債���p�ە��`�Ɋ܂܂�邩����
			const double s_thick = max(m_stroke_width, a_len);
			D2D1_POINT_2F e_lt, e_rb, e_rad;	// �g�債���p�ە��`
			pt_add(r_lt, -s_thick * 0.5, e_lt);
			pt_add(r_rb, s_thick * 0.5, e_rb);
			pt_add(r_rad, s_thick * 0.5, e_rad);
			if (pt_in_rrect(t_pos, e_lt, e_rb, e_rad)) {
				// �k�������p�ە��`���t�]���ĂȂ�, ���ʒu���k�������p�ە��`�Ɋ܂܂�邩���肷��.
				D2D1_POINT_2F s_lt, s_rb, s_rad;	// �k�������p�ە��`
				pt_add(e_lt, s_thick, s_lt);
				pt_add(e_rb, -s_thick, s_rb);
				pt_add(e_rad, -s_thick, s_rad);
				if (s_lt.x < s_rb.x && s_lt.y < s_rb.y && pt_in_rrect(t_pos, s_lt, s_rb, s_rad)) {
					// �h��Ԃ��F���s�����Ȃ�, ANC_FILL ��Ԃ�.
					if (is_opaque(m_fill_color)) {
						return ANC_TYPE::ANC_FILL;
					}
				}
				else {
					// �g�債���p�ە��`�Ɋ܂܂�, �k�������p�ە��`�Ɋ܂܂�Ȃ��Ȃ� ANC_STROKE ��Ԃ�.
					return ANC_TYPE::ANC_STROKE;
				}
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu������.
	// val	�l
	// anc	�}�`�̕���
	// limit	���E���� (���̒��_�Ƃ̋��������̒l�����ɂȂ�Ȃ�, ���̒��_�Ɉʒu�ɍ��킹��)
	bool ShapeRRect::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool /*keep_aspect*/) noexcept
	{
		D2D1_POINT_2F c_pos;
		D2D1_POINT_2F vec;
		D2D1_POINT_2F rad;
		D2D1_POINT_2F new_pos;

		switch (anc) {
		case ANC_TYPE::ANC_R_NW:
			ShapeRRect::get_pos_anc(anc, c_pos);
			pt_round(val, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			pt_add(m_corner_rad, vec, rad);
			calc_corner_radius(m_vec[0], rad, m_corner_rad);
			break;
		case ANC_TYPE::ANC_R_NE:
			ShapeRRect::get_pos_anc(anc, c_pos);
			pt_round(val, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x - vec.x;
			rad.y = m_corner_rad.y + vec.y;
			calc_corner_radius(m_vec[0], rad, m_corner_rad);
			break;
		case ANC_TYPE::ANC_R_SE:
			ShapeRRect::get_pos_anc(anc, c_pos);
			pt_round(val, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x - vec.x;
			rad.y = m_corner_rad.y - vec.y;
			calc_corner_radius(m_vec[0], rad, m_corner_rad);
			break;
		case ANC_TYPE::ANC_R_SW:
			ShapeRRect::get_pos_anc(anc, c_pos);
			pt_round(val, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x + vec.x;
			rad.y = m_corner_rad.y - vec.y;
			calc_corner_radius(m_vec[0], rad, m_corner_rad);
			break;
		default:
			if (!ShapeRect::set_pos_anc(val, anc, limit, false)) {
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
	// page	����
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapePage* page) :
		ShapeRect::ShapeRect(b_pos, b_vec, page)
	{
		D2D1_POINT_2F c_rad{
			page->m_grid_base + 1.0, page->m_grid_base + 1.0
		};
		calc_corner_radius(m_vec[0], c_rad, m_corner_rad);
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapeRRect::ShapeRRect(const ShapePage& page, DataReader const& dt_reader) :
		ShapeRect::ShapeRect(page, dt_reader)
	{
		m_corner_rad = D2D1_POINT_2F{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapeRRect::write(DataWriter const& dt_writer) const
	{
		ShapeRect::write(dt_writer);
		dt_writer.WriteSingle(m_corner_rad.x);
		dt_writer.WriteSingle(m_corner_rad.y);
	}

}