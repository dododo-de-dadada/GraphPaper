#pragma once
#include <d2d1.h>

namespace winrt::GraphPaper::implementation
{
	// �x�N�g���̒��� (�̎���l) �𓾂�
	// a	�x�N�g��
	// �߂�l	���� (�̎���l) 
	inline double pt_abs2(const D2D1_POINT_2F a) noexcept
	{
		const double ax = a.x;
		const double ay = a.y;
		return ax * ax + ay * ay;
	}

	// �_�ɓ_�𑫂�
	inline void pt_add(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x + b.x;
		c.y = a.y + b.y;
	}

	// �_�ɃX�J���[�l�𑫂�
	inline void pt_add(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(a.x + b);
		c.y = static_cast<FLOAT>(a.y + b);
	}

	// �_�� X �� Y �̒l�𑫂�
	inline void pt_add(const D2D1_POINT_2F a, const double x, const double y, D2D1_POINT_2F& b)
		noexcept
	{
		b.x = static_cast<FLOAT>(a.x + x);
		b.y = static_cast<FLOAT>(a.y + y);
	}

	// ��_�̒��_�����߂�.
	inline void pt_avg(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>((a.x + b.x) * 0.5);
		c.y = static_cast<FLOAT>((a.y + b.y) * 0.5);
	}

	// �~���_���܂ނ����肷��.
	// ctr	�~�̒��S�_
	// rad	�~�̔��a
	inline bool pt_in_circle(
		const D2D1_POINT_2F t, const D2D1_POINT_2F ctr, const double rad) noexcept
	{
		const double tx = static_cast<double>(t.x) - static_cast<double>(ctr.x);
		const double ty = static_cast<double>(t.y) - static_cast<double>(ctr.y);
		return tx * tx + ty * ty <= rad * rad;
	}

	// �~���_���܂ނ����肷��.
	// tx, ty	���肳���ʒu (�~�̒��S�_�����_�Ƃ���)
	// rad	�~�̔��a
	inline bool pt_in_circle(const double tx, const double ty, const double rad) noexcept
	{
		return tx * tx + ty * ty <= rad * rad;
	}

	// ���~���_���܂ނ����肷��.
	// t	���肳���_
	// ctr	���~�̒��S
	// rad_x	���~�� X �������̌a
	// rad_y	���~�� Y �������̌a
	// rot	���~�̌X�� (���W�A��)
	// �߂�l	�܂ޏꍇ true
	inline bool pt_in_ellipse(
		const D2D1_POINT_2F t, const D2D1_POINT_2F ctr, const double rad_x,
		const double rad_y, const double rot = 0.0) noexcept
	{
		// ���~�̌X���ɍ��킹�Ĕ��肳���_����].
		const double tx = static_cast<double>(t.x) - static_cast<double>(ctr.x);
		const double ty = static_cast<double>(t.y) - static_cast<double>(ctr.y);
		const double c = cos(rot);
		const double s = sin(rot);
		const double x = c * tx + s * ty;
		const double y = -s * tx + c * ty;
		const double aa = rad_x * rad_x;
		const double bb = rad_y * rad_y;
		return x * x / aa + y * y / bb <= 1.0;
	}

	// ���~���_���܂ނ����肷��.
	// tx, ty	���肳���_ (���~�̒��S�_�����_)
	// rad_x	���~�� X �������̌a
	// rad_y	���~�� Y �������̌a
	// rot	���~�̌X�� (���W�A��)
	// �߂�l	�܂ޏꍇ true
	inline bool pt_in_ellipse(
		const double tx, const double ty, const double rad_x, const double rad_y, const double rot)
		noexcept
	{
		// ���~�̌X���ɍ��킹�Ĕ��肳���_����].
		const double c = cos(rot);
		const double s = sin(rot);
		const double x = c * tx + s * ty;
		const double y = -s * tx + c * ty;
		const double aa = rad_x * rad_x;
		const double bb = rad_y * rad_y;
		return x * x / aa + y * y / bb <= 1.0;
	}

	// ���p�`���_���܂ނ����肷��.
	// tx, ty	���肳���ʒu
	// p_cnt	���_�̐�
	// p	���_�̔z�� [v_cnt]
	// �߂�l	�܂ޏꍇ true
	// ���p�`�̊e�ӂ�, �w�肳�ꂽ�_���J�n�_�Ƃ��鐅�������������鐔�����߂�.
	inline bool pt_in_poly(
		const double tx, const double ty, const size_t p_cnt, const D2D1_POINT_2F p[]) noexcept
	{
		int i_cnt;	// ��_�̐�

		double px = p[p_cnt - 1].x;
		double py = p[p_cnt - 1].y;
		i_cnt = 0;
		for (size_t i = 0; i < p_cnt; i++) {
			const double qx = p[i].x;
			const double qy = p[i].y;
			// ���[�� 1. ������̕�. �_�� y �������ɂ��āA�n�_�ƏI�_�̊Ԃɂ��� (�������A�I�_�͊܂܂Ȃ�).
			// ���[�� 2. �������̕�. �_�� y �������ɂ��āA�n�_�ƏI�_�̊Ԃɂ��� (�������A�n�_�͊܂܂Ȃ�).
			if ((py <= ty && qy > ty) || (py > ty && qy <= ty)) {
				// ���[�� 3. �_��ʂ鐅�������ӂƏd�Ȃ� (���[�� 1, ���[�� 2 ���m�F���邱�Ƃ�, ���[�� 3 ���m�F�ł��Ă���).
				// ���[�� 4. �ӂ͓_�����E���ɂ���. ������, �d�Ȃ�Ȃ�.
				// �ӂ��_�Ɠ��������ɂȂ�ʒu����肵, ���̎���x�̒l�Ɠ_��x�̒l���r����.
				if (tx < px + (ty - py) / (qy - py) * (qx - px)) {
					i_cnt++;
				}
			}
			px = qx;
			py = qy;
		}
		return static_cast<bool>(i_cnt & 1);
	}

	// ���p�`���_���܂ނ����肷��.
	inline bool pt_in_poly(
		const D2D1_POINT_2F t, const size_t p_cnt, const D2D1_POINT_2F p[]) noexcept
	{
		return pt_in_poly(t.x, t.y, p_cnt, p);
	}

	// ���`���_���܂ނ����肷��.
	// t	���肳���ʒu
	// r_lt	���`�̍���ʒu
	// r_rb	���`�̉E���ʒu
	// �߂�l	�܂ޏꍇ true
	inline bool pt_in_rect2(
		const D2D1_POINT_2F t, const D2D1_POINT_2F r_lt, const D2D1_POINT_2F r_rb) noexcept
	{
		return r_lt.x <= t.x && t.x <= r_rb.x && r_lt.y <= t.y && t.y <= r_rb.y;
	}

	// ���`���_���܂ނ����肷��.
	// t	���肳���_
	// r_lt	���`�̂����ꂩ�̒��_
	// r_rb	r_lt �ɑ΂��đΊp�ɂ��钸�_
	// �߂�l	�܂ޏꍇ true
	inline bool pt_in_rect(
		const D2D1_POINT_2F t, const D2D1_POINT_2F r_lt, const D2D1_POINT_2F r_rb) noexcept
	{
		const double lt_x = r_lt.x < r_rb.x ? r_lt.x : r_rb.x;	// ����� x
		const double lt_y = r_lt.y < r_rb.y ? r_lt.y : r_rb.y;	// ����� y
		const double rb_x = r_lt.x < r_rb.x ? r_rb.x : r_lt.x;	// �E���� x
		const double rb_y = r_lt.y < r_rb.y ? r_rb.y : r_lt.y;	// �E���� y
		return lt_x <= t.x && t.x <= rb_x && lt_y <= t.y && t.y <= rb_y;
	}

	// �_�ɃX�J���[���|����, �ʂ̓_�𑫂�
	// a	�_
	// b	�X�J���[�l
	// c	�ʂ̓_
	// d	����
	inline void pt_mul_add(
		const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.x * b + c.x);
		d.y = static_cast<FLOAT>(a.y * b + c.y);
	}

	// �_�ɃX�J���[���|����.
	// a	�ʒu
	// b	�X�J���[�l
	// c	����
	inline void pt_mul(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(a.x * b);
		c.y = static_cast<FLOAT>(a.y * b);
	}

	// ���@�ɃX�J���[�l���|����.
	// a	���@
	// b	�X�J���[�l
	// c	����
	inline void pt_mul(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& c) noexcept
	{
		c.width = static_cast<FLOAT>(a.width * b);
		c.height = static_cast<FLOAT>(a.height * b);
	}

	// �_���X�J���[�{�Ɋۂ߂�.
	// a	�ʒu
	// b	�X�J���[�l
	// c	����
	inline void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(std::round(a.x / b) * b);
		c.y = static_cast<FLOAT>(std::round(a.y / b) * b);
	}

	// �ʒu����ʒu������.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.x;
		c.y = a.y - b.y;
	}

	// �ʒu����傫��������.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.width;
		c.y = a.y - b.height;
	}

}