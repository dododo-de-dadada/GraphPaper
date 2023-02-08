
// �~�ʂ�`�悷�� 3 �̕��@ - �ȉ~�̉~��
// https://learn.microsoft.com/ja-jp/xamarin/xamarin-forms/user-interface/graphics/skiasharp/curves/arcs
// �p�X - �~��
// https://developer.mozilla.org/ja/docs/Web/SVG/Tutorial/Paths

#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ���~�̒��S�_�𓾂�.
	// start	�~�ʂ̊J�n�_
	// vec	�~�ʂ̏I�_�ւ̃x�N�g��
	// rad	���~�̔��a (�W���`�ɂ����� X �������� Y ������)
	// rot	���~�̌X�� (���W�A��)
	// val	����ꂽ���S�_.
	bool ShapeQEllipse::get_pos_center(const D2D1_POINT_2F start, const D2D1_POINT_2F vec, const D2D1_SIZE_F rad, const double rot, D2D1_POINT_2F& val) noexcept
	{
		// ���~�̒��S�_�����߂�.
		// A = 1 / (rx^2)
		// B = 1 / (ry^2)
		// C = cos(��)
		// S = sin(��)
		// �_ p (px, py) ���~�̒��S (ox, oy) �����_�Ƃ�����W�ɕ��s�ړ�����, ��]����.
		// x = C�E(px - ox) + S�E(py - oy)
		// y =-S�E(px - ox) + C�E(py - oy)
		// �_ q �ɂ��Ă����l.
		// ������, ���~�̕W���` A�Ex^2 + B�Ey^2 = 1 �ɑ������.
		// A�E{ C�E(px - ox) + S�E(py - oy) }^2 + B�E{ -S�E(px - ox) + C�E(py - oy) }^2 = 1 ...[1]
		// A�E{ C�E(qx - ox) + S�E(qy - oy) }^2 + B�E{ -S�E(qx - ox) + C�E(qy - oy) }^2 = 1 ...[2]
		// [1] ���̑� 1 ���� ox, oy �ɂ��ēW�J����.
		// A�E{ C�E(px - ox) + S�E(py - oy) }^2 =
		// A�E{ C�Epx - C�Eox + S�Epy - S�Eoy }^2 = 
		// A�E{ -C�Eox - S�Eoy + (C�Epx + S�Epy) }^2 =
		// A�EC^2�Eox^2 + 2A�EC�ES�Eox�Eoy + A�ES^2�Eoy^2 - 2A�EC�E(C�Epx + S�Epy)�Eox - 2A�ES�E(C�Epx + S�Epy)�Eoy + A�E(C�Epx + S�Epy)^2
		// [1] ���̑� 2 �������l.
		// B�E{ -S�E(px - ox) + C�E(py - oy) }^2 =
		// B�E{ -S�Epx + S�Eox + C�Epy - C�Eoy }^2 = 
		// B�E{  S�Eox - C�Eoy - (S�Epx - C�Epy) }^2 =
		// B�ES^2�Eox^2 - 2B�ES�EC�Eox�Eoy + B�EC^2�Eoy^2 - 2B�ES�E(S�Epx - C�Epy)�Eox + 2B�EC�E(S�Epx - C�Epy)�Eoy + B�E(S�Epx - C�Epy)^2
		// ���������� [1] ����,
		// (A�EC^2 + B�ES^2)�Eox^2 + (2A�EC�ES - 2B�ES�EC)�Eox�Eoy + (A�ES^2 + B�EC^2)�Eoy^2 - (2A�EC�E(C�Epx + S�Epy) + 2B�ES�E(S�Epx - C�Epy))�Eox - (2A�ES�E(C�Epx + S�Epy) - 2B�EC�E(S�Epx - C�Epy))�Eoy + (A�E(C�Epx + S�Epy)^2 + B�E(S�Epx - C�Epy)^2)
		//  (A�E(C�Epx + S�Epy)^2 + B�E(S�Epx - C�Epy)^2)
		// [2] ����, [1] ���Ɋ܂܂�� px, py ��, qx, qy �ɒu�������邾��.
		// ox^2, ox�Eoy, oy^2 �̌W���́@px, py ���܂܂Ȃ�.
		// ���������Ă����̌W���� [1]-[2] �ŏ���, 1 ���̍��ł��� ox �� oy, �萔�����c��.
		// d = -(2A�EC�E(C�Epx + S�Epy) + 2B�ES�E(S�Epx - C�Epy)) ... [1] ���� ox �̍�
		// e = -(2A�ES�E(C�Epx + S�Epy) - 2B�EC�E(S�Epx - C�Epy)) ... [1] ���� oy �̍�
		// f = A�E(C�Epx + S�Epy)^2 + B�E(S�Epx - C�Epy)^2 ... [1] �萔��
		// d�Eox + e�Eoy + f = g�Eox + h�Eoy + i
		// oy = (g - d)/(e - h)�Eox + (i - f)/(e - h)
		// oy = j�Eox + k
		// ����� [1] ���ɑ������,
		// A�E{ C�E(px - ox) + S�E(py - oy) }^2 = 1
		// A�E{ C�E(px - ox) + S�E(py - j�Eox - k) }^2 + B�E{ S�E(px - ox) - C�E(py - j�Eox - k) }^2 - 1 = 0 ...[3]
		// [3] ���� ox �ɂ��ēW�J����
		// [3] ���̑� 1 ����,
		// A�E{ C�E(px - ox) + S�E(py - j�Eox - k) }^2 =
		// A�E{ C�Epx - C�Eox + S�Epy - S�Ej�Eox - S�Ek }^2 =
		// A�E{-(C + S�Ej)�Eox + (C�Epx + S�Epy - S�Ek) }^2 =
		// A�E(C + S�Ej)^2�Eox^2 - 2A�E(C + S�Ej)(C�Epx + S�Epy - S�Ek)�Eox + A�E(C�Epx + S�Epy - S�Ek)^2
		// [3] ���̑� 2 ����,
		// B�E{ S�E(px - ox) - C�E(py - j�Eox - k) }^2 =
		// B�E{ S�Epx - S�Eox - C�Epy + C�Ej�Eox + C�Ek) }^2 =
		// B�E{-(S - C�Ej)�Eox + (S�Epx - C�Epy + C�Ek) }^2 =
		// B�E(S - C�Ej)^2�Eox^2 - 2B�E(S - C�Ej)(S�Epx - C�Epy + C�Ek)�Eox + B�E(S�Epx - C�Epy + C�Ek)^2
		// [3] ���� a�Eox^2 + b�Eox + c = 0 �Ƃ����,
		// a = A�E(C + S�Ej)^2 + B�E(S - C�Ej)^2
		// b = -2A�E(C + S�Ej)(C�Epx + S�Epy - S�Ek) - 2B�E(S - C�Ej)(S�Epx - C�Epy + C�Ek)
		// c = A�E(C�Epx + S�Epy - S�Ek)^2 + B�E(S�Epx - C�Epy + C�Ek)^2 - 1
		// 2 ���������̉������ɑ�������, ox �����܂�.
		const double px = start.x;
		const double py = start.y;
		const double qx = start.x + vec.x;
		const double qy = start.y + vec.y;
		const double A = 1.0 / (rad.width * rad.width);
		const double B = 1.0 / (rad.height * rad.height);
		const double C = cos(rot);
		const double S = sin(rot);
		const double d = -2 * A * C * (C * px + S * py) - 2 * B * S * (S * px - C * py);
		const double e = -2 * A * S * (C * px + S * py) + 2 * B * C * (S * px - C * py);
		const double f = A * (C * px + S * py) * (C * px + S * py) + B * (S * px - C * py) * (S * px - C * py);
		const double g = -2 * A * C * (C * qx + S * qy) - 2 * B * S * (S * qx - C * qy);
		const double h = -2 * A * S * (C * qx + S * qy) + 2 * B * C * (S * qx - C * qy);
		const double i = A * (C * qx + S * qy) * (C * qx + S * qy) + B * (S * qx - C * qy) * (S * qx - C * qy);
		const double j = (g - d) / (e - h);
		const double k = (i - f) / (e - h);
		const double a = A * (C + S * j) * (C + S * j) + B * (S - C * j) * (S - C * j);
		const double b = -2 * A * (C + S * j) * (C * px + S * py - S * k) - 2 * B * (S - C * j) * (S * px - C * py + C * k);
		const double c = A * (C * px + S * py - S * k) * (C * px + S * py - S * k) + B * (S * px - C * py + C * k) * (S * px - C * py + C * k) - 1;
		const double bb_4ac = b * b - 4 * a * c;
		if (bb_4ac <= FLT_MIN) {
			return false;
			//__debugbreak();
		}
		const double s = bb_4ac <= FLT_MIN ? 0.0f : sqrt(bb_4ac);
		const double ox = (-b + s) / (a + a);
		const double oy = j * ox + k;
		const double vx = px - ox;
		const double vy = py - oy;
		const double wx = qx - ox;
		const double wy = qy - oy;
		if (vx * wy - vy * wx >= 0.0) {
			val.x = static_cast<FLOAT>(ox);
			val.y = static_cast<FLOAT>(oy);
		}
		else {
			const double x = (-b - s) / (a + a);
			val.x = static_cast<FLOAT>(x);
			val.y = static_cast<FLOAT>(j * x + k);
		}
		return true;
	}

	bool ShapeQEllipse::set_rotation(const float val) noexcept
	{
		if (equal(m_rot_degree, val)) {
			return false;
		}
		// ���̂��~�ł̒��S�_�𓾂�.
		const double old_r = M_PI * m_rot_degree / 180.0;
		D2D1_POINT_2F c_pos;
		get_pos_center(m_start, m_vec[0], m_radius, old_r, c_pos);
		// �V�������~�̎��𓾂�.
		const double new_r = M_PI * val / 180.0;
		const auto c = cos(-new_r);
		const auto s = sin(-new_r);
		double px, py, qx, qy;
		if (m_vec[0].x >= 0.0f && m_vec[0].y >= 0.0f) {
			px = 0.0;
			py = -m_radius.height;
			qx = m_radius.width;
			qy = 0.0;
			m_start.x = static_cast<FLOAT>(c * px + s * py + c_pos.x);
			m_start.y = static_cast<FLOAT>(-s * px + c * py + c_pos.y);
			m_vec.resize(1);
			m_vec[0].x = static_cast<FLOAT>(c * qx + s * qy + c_pos.x - m_start.x);
			m_vec[0].y = static_cast<FLOAT>(-s * qx + c * qy + c_pos.y - m_start.y);
		}
		else if (m_vec[0].x < 0.0f && m_vec[0].y >= 0.0f) {
			px = m_radius.width;
			py = 0.0f;
			qx = 0.0f;
			qy = m_radius.height;
			m_start.x = static_cast<FLOAT>(c * px + s * py + c_pos.x);
			m_start.y = static_cast<FLOAT>(-s * px + c * py + c_pos.y);
			m_vec.resize(1);
			m_vec[0].x = static_cast<FLOAT>(c * qx + s * qy + c_pos.x - m_start.x);
			m_vec[0].y = static_cast<FLOAT>(-s * qx + c * qy + c_pos.y - m_start.y);
		}
		else if (m_vec[0].x < 0.0f && m_vec[0].y < 0.0f) {
			px = 0.0;
			py = m_radius.height;
			qx = -m_radius.width;
			qy = 0.0;
			m_start.x = static_cast<FLOAT>(c * px + s * py + c_pos.x);
			m_start.y = static_cast<FLOAT>(-s * px + c * py + c_pos.y);
			m_vec.resize(1);
			m_vec[0].x = static_cast<FLOAT>(c * qx + s * qy + c_pos.x - m_start.x);
			m_vec[0].y = static_cast<FLOAT>(-s * qx + c * qy + c_pos.y - m_start.y);
		}
		else {
			px = -m_radius.width;
			py = 0.0f;
			qx = 0.0f;
			qy = -m_radius.height;
			m_start.x = static_cast<FLOAT>(c * px + s * py + c_pos.x);
			m_start.y = static_cast<FLOAT>(-s * px + c * py + c_pos.y);
			m_vec.resize(1);
			m_vec[0].x = static_cast<FLOAT>(c * qx + s * qy + c_pos.x - m_start.x);
			m_vec[0].y = static_cast<FLOAT>(-s * qx + c * qy + c_pos.y - m_start.y);
		}
		m_rot_degree = val;
		if (m_d2d_fill_geom != nullptr) {
			m_d2d_fill_geom = nullptr;
		}
		if (m_d2d_path_geom != nullptr) {
			m_d2d_path_geom = nullptr;
		}
		if (m_d2d_arrow_geom != nullptr) {
			m_d2d_arrow_geom = nullptr;
		}
		return true;
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����.
	bool ShapeQEllipse::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept
	{
		if (anc != ANC_TYPE::ANC_CENTER) {
			if (ShapePath::set_pos_anc(val, anc, limit, keep_aspect)) {
				/*
				const double px = m_start.x;
				const double py = m_start.y;
				const double qx = m_start.x + m_vec[0].x;
				const double qy = m_start.y + m_vec[0].y;
				const auto a = 1.0;
				const auto b0 = -(px + qx);
				const auto c0 = px * qx;
				const auto b1 = -(py + qy);
				const auto c1 = py + qy;
				const auto bb_4ac0 = b0 * b0 - 4.0 * a * c0;
				const auto bb_4ac1 = b1 * b1 - 4.0 * a * c1;
				const auto rx = (-b0 - sqrt(bb_4ac0)) / (2.0 * a);
				const auto ry = (-b1 - sqrt(bb_4ac1)) / (2.0 * a);
				m_radius.width = sqrt((qx - rx) * (qx - rx) + (qy - ry) * (qy - ry));
				m_radius.height = sqrt((px - rx) * (px - rx) + (py - ry) * (py - ry));
				D2D1_POINT_2F c_pos;
				*/
				const double rot = M_PI * m_rot_degree / 180.0;
				const double c = cos(rot);
				const double s = sin(rot);
				const double x = m_vec[0].x;
				const double y = m_vec[0].y;
				const double w =  c * x + s * y;
				const double h = -s * x + c * y;
				m_radius.width = static_cast<FLOAT>(fabs(w));
				m_radius.height = static_cast<FLOAT>(fabs(h));
				if (m_d2d_fill_geom != nullptr) {
					m_d2d_fill_geom = nullptr;
				}
				return true;
			}
		}
		else {
			const double rot = M_PI * m_rot_degree / 180.0;
			D2D1_POINT_2F c_pos;
			if (get_pos_center(m_start, m_vec[0], m_radius, rot, c_pos)) {
				const D2D1_POINT_2F s_pos{
					m_start.x + val.x - c_pos.x,
					m_start.y + val.y - c_pos.y
				};
				if (ShapePath::set_pos_start(s_pos)) {
					if (m_d2d_fill_geom != nullptr) {
						m_d2d_fill_geom = nullptr;
					}
					return true;
				}
			}
		}
		return false;
	}

	// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
	bool ShapeQEllipse::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		if (ShapePath::set_pos_start(val)) {
			m_d2d_fill_geom = nullptr;
			return true;
		}
		return false;
	}

	uint32_t ShapeQEllipse::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		// �A���J�[�|�C���g�Ɋ܂܂�邩���肷��.
		if (pt_in_anc(t_pos, m_start, a_len)) {
			return ANC_TYPE::ANC_P0;
		}
		else if (pt_in_anc(t_pos, D2D1_POINT_2F{ m_start.x + m_vec[0].x, m_start.y + m_vec[0].y }, a_len)) {
			return ANC_TYPE::ANC_P0 + 1;
		}
		// ���~�̒��S�_�Ɋ܂܂�邩���肷��.
		const double rot = M_PI * m_rot_degree / 180.0;
		D2D1_POINT_2F c_pos;
		get_pos_center(m_start, m_vec[0], m_radius, rot, c_pos);
		if (pt_in_anc(t_pos, c_pos, a_len)) {
			return  ANC_TYPE::ANC_CENTER;
		}
		// ��`�̓����ɂ��邩���肷��.
		const double px = m_start.x - c_pos.x;
		const double py = m_start.y - c_pos.y;
		const double qx = px + m_vec[0].x;
		const double qy = py + m_vec[0].y;
		const double tx = t_pos.x - c_pos.x;
		const double ty = t_pos.y - c_pos.y;
		const double pt = px * ty - py * tx;
		const double qt = qx * ty - qy * tx;
		const double rx = abs(m_radius.width);
		const double ry = abs(m_radius.height);
		// p �� t �̊O�ς� q �� t �̊O�ς̕������t�Ȃ�Γ���
		if (pt >= 0.0 && qt <= 0.0) {
			// ���g�����Ȃ�,
			if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
				const double e_width = m_stroke_width * 0.5;
				if (!pt_in_ellipse(t_pos, c_pos, rx - e_width, ry - e_width, rot)) {
					if (pt_in_ellipse(t_pos, c_pos, rx + e_width, ry + e_width, rot)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				else if (is_opaque(m_fill_color)) {
					return ANC_TYPE::ANC_FILL;
				}
			}
			else if (is_opaque(m_fill_color)) {
				if (pt_in_ellipse(t_pos, c_pos, rx, ry, rot)) {
					return ANC_TYPE::ANC_FILL;
				}
			}
		}
		// ��̊O���ɂ���, �����g�����Ȃ�,
		else if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			// �[�_�Ɋ܂܂�邩���肷��.
			// ���v���ɑ΂��Ă̂ݔ��肵�Ă���̂ŗv����.
			auto c_style = m_stroke_cap.m_start;
			const double e_width = m_stroke_width * 0.5;	// �ӂ̔����̕�.
			if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
				// ���肷��ʒu��, �[�_�����_�Ƃ�����W�ɕ��s�ړ�����.
				const D2D1_POINT_2F t{
					t_pos.x - m_start.x, t_pos.y - m_start.y
				};
				if (pt_in_circle(t, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			else {
				const D2D1_POINT_2F t{
					static_cast<FLOAT>(tx), static_cast<FLOAT>(ty)
				};
				// ���~ A�Ex^2 + B�Ey^2 = 1 �ɂ�����_ p { x0, y0 } �̐ڐ���
				// (A�Ex0)�Ex + (B�Ey0)�Ey = 1
				const double x0 = px;
				const double y0 = py;
				// p ����]�ړ���, ���~�̕W���`�ł̐ڐ��̌W���𓾂�.
				const double c = cos(rot);
				const double s = sin(rot);
				const double Ax0 = ( c * x0 + s * y0) / (rx * rx);
				const double By0 = (-s * x0 + c * y0) / (ry * ry);
				// ����ꂽ�ڐ����t�ɉ�]�ړ���, �X�������~�ɂ�����ڐ��𓾂�.
				const double a = c * Ax0 - s * By0;
				const double b = s * Ax0 + c * By0;
				// ����ꂽ�ڐ�����, �ڐ��ɐ����ȃx�N�g�� e (�����͕ӂ̔����̕�) �𓾂�.
				const double ab = sqrt(a * a + b * b);
				const double ex = e_width * a / ab;
				const double ey = e_width * b / ab;
				if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT) {
					const D2D1_POINT_2F ev[4]{
						{ static_cast<FLOAT>(x0 + ex), static_cast<FLOAT>(y0 + ey) },
						{ static_cast<FLOAT>(x0 - ex), static_cast<FLOAT>(y0 - ey) },
						{ static_cast<FLOAT>(x0 - ex - ey), static_cast<FLOAT>(y0 + ex - ey) },
						{ static_cast<FLOAT>(x0 + ex - ey), static_cast<FLOAT>(y0 + ex + ey) }
					};
					if (pt_in_poly(t, 4, ev)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				else if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
					//const D2D1_POINT_2F ev[4]{
					//	{ static_cast<FLOAT>(x0 + ex + ey), static_cast<FLOAT>(y0 - ex + ey) },
					//	{ static_cast<FLOAT>(x0 - ex + ey), static_cast<FLOAT>(y0 - ex - ey) },
					//	{ static_cast<FLOAT>(x0 - ex - ey), static_cast<FLOAT>(y0 + ex - ey) },
					//	{ static_cast<FLOAT>(x0 + ex - ey), static_cast<FLOAT>(y0 + ex + ey) },
					//};
					const D2D1_POINT_2F ev[4]{
						{ static_cast<FLOAT>(x0 + ex + ey), static_cast<FLOAT>(y0 - ex + ey) },
						{ static_cast<FLOAT>(x0 - ex + ey), static_cast<FLOAT>(y0 - ex - ey) },
						{ static_cast<FLOAT>(x0 - ex), static_cast<FLOAT>(y0 - ey) },
						{ static_cast<FLOAT>(x0 + ex), static_cast<FLOAT>(y0 + ey) },
					};
					if (pt_in_poly(t, 4, ev)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				else if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
					const D2D1_POINT_2F ev[3/*4*/]{
						{ static_cast<FLOAT>(x0 + ex), static_cast<FLOAT>(y0 + ey) },
						{ static_cast<FLOAT>(x0 + ey), static_cast<FLOAT>(y0 - ex) },
						{ static_cast<FLOAT>(x0 - ex), static_cast<FLOAT>(y0 - ey) },
						//{ static_cast<FLOAT>(x0 - ey), static_cast<FLOAT>(y0 + ex) }
					};
					if (pt_in_poly(t, 3/*4*/, ev)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
			}
			if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
				// ���肷��ʒu��, �[�_�����_�Ƃ�����W�ɕ��s�ړ�����.
				const D2D1_POINT_2F t{
					t_pos.x - (m_start.x + m_vec[0].x), t_pos.y - (m_start.y + m_vec[0].y)
				};
				if (pt_in_circle(t, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			else {
				const D2D1_POINT_2F t{
					static_cast<FLOAT>(tx), static_cast<FLOAT>(ty)
				};
				// ���~ A�Ex^2 + B�Ey^2 = 1 �ɂ�����_ p { x0, y0 } �̐ڐ���
				// (A�Ex0)�Ex + (B�Ey0)�Ey = 1
				const double x0 = qx;
				const double y0 = qy;
				// p ����]�ړ���, ���~�̕W���`�ł̐ڐ��̌W���𓾂�.
				const double c = cos(rot);
				const double s = sin(rot);
				const double Ax0 = (c * x0 + s * y0) / (rx * rx);
				const double By0 = (-s * x0 + c * y0) / (ry * ry);
				// ����ꂽ�ڐ����t�ɉ�]�ړ���, �X�������~�ɂ�����ڐ��𓾂�.
				const double a = c * Ax0 - s * By0;
				const double b = s * Ax0 + c * By0;
				// ����ꂽ�ڐ�����, �ڐ��ɐ����ȃx�N�g�� e (�����͕ӂ̔����̕�) �𓾂�.
				// e ��, ���~�̊O������.
				const double ab = sqrt(a * a + b * b);
				const double ex = e_width * a / ab;
				const double ey = e_width * b / ab;
				if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT) {
					const D2D1_POINT_2F ev[4]{
						{ static_cast<FLOAT>(x0 + ex + ey), static_cast<FLOAT>(y0 - ex + ey) },
						{ static_cast<FLOAT>(x0 - ex + ey), static_cast<FLOAT>(y0 - ex - ey) },
						{ static_cast<FLOAT>(x0 - ex), static_cast<FLOAT>(y0 - ey) },
						{ static_cast<FLOAT>(x0 + ex), static_cast<FLOAT>(y0 + ey) },
					};
					if (pt_in_poly(t, 4, ev)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				else if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
					//const D2D1_POINT_2F ev[4]{
					//	{ static_cast<FLOAT>(x0 + ex + ey), static_cast<FLOAT>(y0 - ex + ey) },
					//	{ static_cast<FLOAT>(x0 - ex + ey), static_cast<FLOAT>(y0 - ex - ey) },
					//	{ static_cast<FLOAT>(x0 - ex - ey), static_cast<FLOAT>(y0 + ex - ey) },
					//	{ static_cast<FLOAT>(x0 + ex - ey), static_cast<FLOAT>(y0 + ex + ey) }
					//};
					const D2D1_POINT_2F ev[4]{
						{ static_cast<FLOAT>(x0 + ex), static_cast<FLOAT>(y0 + ey) },
						{ static_cast<FLOAT>(x0 - ex), static_cast<FLOAT>(y0 - ey) },
						{ static_cast<FLOAT>(x0 - ex - ey), static_cast<FLOAT>(y0 + ex - ey) },
						{ static_cast<FLOAT>(x0 + ex - ey), static_cast<FLOAT>(y0 + ex + ey) }
					};
					if (pt_in_poly(t, 4, ev)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				else if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
					const D2D1_POINT_2F ev[3/*4*/]{
						{ static_cast<FLOAT>(x0 + ex), static_cast<FLOAT>(y0 + ey) },
						//{ static_cast<FLOAT>(x0 + ey), static_cast<FLOAT>(y0 - ex) },
						{ static_cast<FLOAT>(x0 - ex), static_cast<FLOAT>(y0 - ey) },
						{ static_cast<FLOAT>(x0 - ey), static_cast<FLOAT>(y0 + ex) }
					};
					if (pt_in_poly(t, 3/*4*/, ev)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// ���~���x�W�F�Ȑ��ŋߎ�����.
	// q	�ی� (1, 2, 3, 4)
	// ����ꂽ�x�W�F�Ȑ��̊J�n�_�Ɛ���_��, ���~�̒��S�_�����_�Ƃ�����W�œ�����.
	void ShapeQEllipse::qellipse_alternate(const int q, const D2D1_SIZE_F rad, const double rot, D2D1_POINT_2F& b_pos, D2D1_BEZIER_SEGMENT& b_seg)
	{
		// 3���x�W�F�Ȑ���p�����ȉ~�̋ߎ�
		// https://clown.cube-soft.jp/entry/20090606/p1
		// p0(x+r, y),  p1(x+r, y+a*r'),  p2(x+a*r, y+r'),  p3(x, y+r') ... (0 <= �� <= ��/2)
		// p3(x, y + r'), p4(x-a*r, y+r'), p5(x - r, y + a * r'),  p6(x-r, y)  ... (��/2 <= �� <= ��)
		// p6(x - r, y), p7(x - r, y - a * r'),  p8(x-a*r, y-r'), p9(x, y - r') ... (�� <= �� <= 3��/2)
		// p9(x, y - r'), p10(x+a*r, y-r'), p11(x + r, y - a * r'), p12(x+r, y) ... (3��/2 <= �� <= 2��)
		constexpr double a = 4.0 * (M_SQRT2 - 1.0) / 3.0;
		const double rx = rad.width;
		const double ry = rad.height;
		double b_pos_x;
		double b_pos_y;
		double b_seg1x;
		double b_seg1y;
		double b_seg2x;
		double b_seg2y;
		double b_seg3x;
		double b_seg3y;
		if (q == 1) {
			b_pos_x = 0.0f;
			b_pos_y = -ry;
			b_seg1x = a * rx;
			b_seg1y = -ry;
			b_seg2x = rx;
			b_seg2y = -a * ry;
			b_seg3x = rx;
			b_seg3y = 0.0f;
		}
		else if (q == 2) {
			b_pos_x = rx;
			b_pos_y = 0.0f;
			b_seg1x = rx;
			b_seg1y = a * ry;
			b_seg2x = a * rx;
			b_seg2y = ry;
			b_seg3x = 0.0f;
			b_seg3y = ry;
		}
		else if (q == 3) {
			b_pos_x = 0.0f;
			b_pos_y = ry;
			b_seg1x = -a * rx;
			b_seg1y = ry;
			b_seg2x = -rx;
			b_seg2y = a * ry;
			b_seg3x = -rx;
			b_seg3y = 0.0f;
		}
		else if (q == 4) {
			b_pos_x = -rx;
			b_pos_y = 0.0f;
			b_seg1x = -rx;
			b_seg1y = -a * ry;
			b_seg2x = -a * rx;
			b_seg2y = -ry;
			b_seg3x = 0.0f;
			b_seg3y = -ry;
		}
		else {
			return;
		}
		const double c = cos(-rot);
		const double s = sin(-rot);
		b_pos.x = static_cast<FLOAT>(c * b_pos_x + s * b_pos_y);
		b_pos.y = static_cast<FLOAT>(-s * b_pos_x + c * b_pos_y);
		b_seg.point1.x = static_cast<FLOAT>(c * b_seg1x + s * b_seg1y);
		b_seg.point1.y = static_cast<FLOAT>(-s * b_seg1x + c * b_seg1y);
		b_seg.point2.x = static_cast<FLOAT>(c * b_seg2x + s * b_seg2y);
		b_seg.point2.y = static_cast<FLOAT>(-s * b_seg2x + c * b_seg2y);
		b_seg.point3.x = static_cast<FLOAT>(c * b_seg3x + s * b_seg3y);
		b_seg.point3.y = static_cast<FLOAT>(-s * b_seg3x + c * b_seg3y);
	}

	// ���̕Ԃ��Ɛ�[�̈ʒu�𓾂�
	bool ShapeQEllipse::qellipse_calc_arrow(const int q, const D2D1_POINT_2F c_pos, const D2D1_SIZE_F rad, const double rot, const ARROW_SIZE a_size, D2D1_POINT_2F barbs[])
	{
		D2D1_POINT_2F b_pos{};
		D2D1_BEZIER_SEGMENT b_seg{};
		qellipse_alternate(q, rad, rot, b_pos, b_seg);
		D2D1_POINT_2F h[3];
		if (ShapeBezier::bezi_calc_arrow(b_pos, b_seg, a_size, h)) {
			// ���s�ړ�.
			barbs[0].x = h[0].x + c_pos.x;
			barbs[0].y = h[0].y + c_pos.y;
			barbs[1].x = h[1].x + c_pos.x;
			barbs[1].y = h[1].y + c_pos.y;
			barbs[2].x = h[2].x + c_pos.x;
			barbs[2].y = h[2].y + c_pos.y;
			return true;
		}
		return false;
	}

	void ShapeQEllipse::draw(void)
	{
		ID2D1Factory* factory = Shape::s_d2d_factory;
		ID2D1SolidColorBrush* brush = Shape::s_d2d_color_brush;
		ID2D1RenderTarget* target = Shape::s_d2d_target;

		D2D1_POINT_2F c_pos{};
		if ((!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color) &&
			m_arrow_style != ARROW_STYLE::NONE && m_d2d_arrow_geom == nullptr) ||
			(is_opaque(m_fill_color) && m_d2d_fill_geom == nullptr || is_selected())) {
			if (!get_pos_center(m_start, m_vec[0], m_radius, M_PI * m_rot_degree / 180.0, c_pos)) {
				__debugbreak();
			}
		}
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			if (m_d2d_stroke_style == nullptr) {
				create_stroke_style(factory);
			}
			if (m_d2d_path_geom == nullptr) {
				D2D1_ARC_SEGMENT arc{
					D2D1_POINT_2F{ m_start.x + m_vec[0].x, m_start.y + m_vec[0].y },
					D2D1_SIZE_F{ fabsf(m_radius.width), fabsf(m_radius.height) },
					m_rot_degree,
					m_sweep_flag,
					D2D1_ARC_SIZE::D2D1_ARC_SIZE_SMALL
				};
				winrt::com_ptr<ID2D1GeometrySink> sink;
				winrt::check_hresult(
					factory->CreatePathGeometry(m_d2d_path_geom.put())
				);
				winrt::check_hresult(
					m_d2d_path_geom->Open(sink.put())
				);
				sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
				const auto f_begin = (is_opaque(m_fill_color) ?
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
				sink->BeginFigure(D2D1_POINT_2F{ m_start.x, m_start.y }, f_begin);
				sink->AddArc(arc);
				sink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
				winrt::check_hresult(
					sink->Close()
				);
				sink = nullptr;
			}
			if (m_arrow_style != ARROW_STYLE::NONE) {
				if (m_d2d_arrow_geom == nullptr) {
					// ���~�̌ʒ������߂�̂͂���ǂ��̂�, �x�W�F�ŋߎ�
					D2D1_POINT_2F arrow[3];
					if (m_vec[0].x >= 0.0f && m_vec[0].y >= 0.0f) {
						qellipse_calc_arrow(1, c_pos, m_radius, M_PI * m_rot_degree / 180.0, m_arrow_size, arrow);
					}
					else if (m_vec[0].x < 0.0f && m_vec[0].y >= 0.0f) {
						qellipse_calc_arrow(2, c_pos, m_radius, M_PI * m_rot_degree / 180.0, m_arrow_size, arrow);
					}
					else if (m_vec[0].x < 0.0f && m_vec[0].y < 0.0f) {
						qellipse_calc_arrow(3, c_pos, m_radius, M_PI * m_rot_degree / 180.0, m_arrow_size, arrow);
					}
					else {
						qellipse_calc_arrow(4, c_pos, m_radius, M_PI * m_rot_degree / 180.0, m_arrow_size, arrow);
					}
					winrt::com_ptr<ID2D1GeometrySink> sink;
					const ARROW_STYLE a_style{ m_arrow_style };
					// �W�I���g���p�X���쐬����.
					winrt::check_hresult(
						factory->CreatePathGeometry(m_d2d_arrow_geom.put())
					);
					winrt::check_hresult(
						m_d2d_arrow_geom->Open(sink.put())
					);
					sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
					const auto f_begin = (a_style == ARROW_STYLE::FILLED
						? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
						: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
					const auto f_end = (a_style == ARROW_STYLE::FILLED
						? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED
						: D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
					sink->BeginFigure(arrow[0], f_begin);
					sink->AddLine(arrow[2]);
					sink->AddLine(arrow[1]);
					sink->EndFigure(f_end);
					winrt::check_hresult(
						sink->Close()
					);
					sink = nullptr;
				}
				if (m_d2d_arrow_style == nullptr) {
					const CAP_STYLE c_style{ m_stroke_cap };
					const D2D1_LINE_JOIN j_style{ m_join_style };
					const double j_miter_limit = m_join_miter_limit;
					// ��邵�̔j���̌`���͂��Ȃ炸����.
					const D2D1_STROKE_STYLE_PROPERTIES s_prop{
						c_style.m_start,	// startCap
						c_style.m_end,	// endCap
						D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// dashCap
						j_style,	// lineJoin
						static_cast<FLOAT>(j_miter_limit),	// miterLimit
						D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID,	// dashStyle
						0.0f	// dashOffset
					};
					winrt::check_hresult(
						factory->CreateStrokeStyle(s_prop, nullptr, 0, m_d2d_arrow_style.put())
					);
				}
			}
		}
		if (is_opaque(m_fill_color) && m_d2d_fill_geom == nullptr) {
			D2D1_ARC_SEGMENT arc{
				D2D1_POINT_2F{ m_start.x + m_vec[0].x, m_start.y + m_vec[0].y },
				D2D1_SIZE_F{ fabsf(m_radius.width), fabsf(m_radius.height) },
				m_rot_degree,
				m_sweep_flag,
				D2D1_ARC_SIZE::D2D1_ARC_SIZE_SMALL
			};
			winrt::com_ptr<ID2D1GeometrySink> sink;
			winrt::check_hresult(factory->CreatePathGeometry(m_d2d_fill_geom.put()));
			winrt::check_hresult(m_d2d_fill_geom->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			const auto f_begin = (is_opaque(m_fill_color) ?
				D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
				D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
			sink->BeginFigure(D2D1_POINT_2F{ m_start.x, m_start.y }, f_begin);
			sink->AddArc(arc);
			sink->AddLine(c_pos);
			sink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED);
			winrt::check_hresult(sink->Close());
			sink = nullptr;
		}
		if (m_d2d_fill_geom != nullptr) {
			brush->SetColor(m_fill_color);
			target->FillGeometry(m_d2d_fill_geom.get(), brush);
		}
		if (m_d2d_path_geom != nullptr) {
			brush->SetColor(m_stroke_color);
			target->DrawGeometry(m_d2d_path_geom.get(), brush, m_stroke_width, m_d2d_stroke_style.get());
			if (m_d2d_arrow_geom != nullptr) {
				if (m_arrow_style == ARROW_STYLE::FILLED) {
					target->FillGeometry(m_d2d_arrow_geom.get(), brush);
					target->DrawGeometry(m_d2d_arrow_geom.get(), brush, m_stroke_width, m_d2d_arrow_style.get());
				}
				if (m_arrow_style == ARROW_STYLE::OPENED) {
					target->DrawGeometry(m_d2d_arrow_geom.get(), brush, m_stroke_width, m_d2d_arrow_style.get());
				}
			}
		}
		if (is_selected()) {
			D2D1_POINT_2F p{ m_start.x, m_start.y };
			D2D1_POINT_2F q{ m_start.x + m_vec[0].x, m_start.y + m_vec[0].y };
			D2D1_MATRIX_3X2_F t32;
			target->GetTransform(&t32);
			anc_draw_rect(p, Shape::s_anc_len / t32._11, target, brush);
			anc_draw_rect(q, Shape::s_anc_len / t32._11, target, brush);
			anc_draw_rect(c_pos, Shape::s_anc_len / t32._11, target, brush);
		}
	}

	ShapeQEllipse::ShapeQEllipse(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const float rot, const Shape* page) :
		ShapePath(page, false),
		m_rot_degree(rot),
		// �����a�͐}�`���쐬���ꂽ�Ƃ��̏ی���ێ����邽��, �������̂܂܂Ŋi�[�����.
		m_radius(D2D1_SIZE_F{ fabs(b_vec.x), fabs(b_vec.y) }),
		m_sweep_flag(D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE),
		m_larg_flag(D2D1_ARC_SIZE_SMALL
	)
	{
		m_start = b_pos;	// �n�_
		m_vec.push_back(b_vec);	// �I�_
		/*
		// (p - r)�E(q - r) = 0
		// pq - pr - qr + r^2 = 0
		// r^2 - (p + q)�Er + pq = 0
		const double px = b_pos.x;
		const double py = b_pos.y;
		const double qx = b_pos.x + b_vec.x;
		const double qy = b_pos.y + b_vec.y;
		const auto a = 1.0;
		const auto b0 = -(px + qx);
		const auto c0 =   px * qx;
		const auto b1 = -(py + qy);
		const auto c1 = py + qy;
		const auto bb_4ac0 = b0 * b0 - 4.0 * a * c0;
		const auto bb_4ac1 = b1 * b1 - 4.0 * a * c1;
		const auto rx = (-b0 - sqrt(bb_4ac0)) / 2.0;
		const auto ry = (-b1 - sqrt(bb_4ac1)) / 2.0;
		m_radius.width = sqrt((qx - rx) * (qx - rx) + (qy - ry) * (qy - ry));
		m_radius.height = sqrt((px - rx) * (px - rx) + (py - ry) * (py - ry));
		*/
		/*
		D2D1_POINT_2F c_pos;
		get_pos_center(b_pos, b_vec, m_radius, 0.0, c_pos);
		const auto c = cos(-(rot * M_PI / 180.0));
		const auto s = sin(-(rot * M_PI / 180.0));

		m_start.x = static_cast<FLOAT>(c * (b_pos.x - c_pos.x) + s * (b_pos.y - c_pos.y) + c_pos.x);
		m_start.y = static_cast<FLOAT>(-s * (b_pos.x - c_pos.x) + c * (b_pos.y - c_pos.y) + c_pos.y);
		m_vec.resize(1);
		m_vec[0].x = static_cast<FLOAT>(c * (b_pos.x + b_vec.x - c_pos.x) + s * (b_pos.y + b_vec.y - c_pos.y) + c_pos.x - m_start.x);
		m_vec[0].y = static_cast<FLOAT>(-s * (b_pos.x + b_vec.x - c_pos.x) + c * (b_pos.y + b_vec.y - c_pos.y) + c_pos.y - m_start.y);
		*/
	}

	ShapeQEllipse::ShapeQEllipse(const Shape& page, const DataReader& dt_reader) :
		ShapePath(page, dt_reader),
		m_radius({ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_rot_degree(dt_reader.ReadSingle()),
		m_sweep_flag(static_cast<D2D1_SWEEP_DIRECTION>(dt_reader.ReadUInt32())),
		m_larg_flag(static_cast<D2D1_ARC_SIZE>(dt_reader.ReadUInt32()))
	{}
	void ShapeQEllipse::write(const DataWriter& dt_writer) const
	{
		ShapePath::write(dt_writer);
		dt_writer.WriteSingle(m_radius.width);
		dt_writer.WriteSingle(m_radius.height);
		dt_writer.WriteSingle(m_rot_degree);
		dt_writer.WriteUInt32(m_sweep_flag);
		dt_writer.WriteUInt32(m_larg_flag);
	}
}