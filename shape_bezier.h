//
// �x�W�F�Ȑ��̕⏕�֐�
//
#pragma once
#include <stdint.h>
#include <cmath>
#include <cfloat>
#include <d2d1.h>

namespace winrt::GraphPaper::implementation
{
	constexpr uint32_t SIMPSON_CNT = 30;	// �V���v�\���@�̉�

	//------------------------------
// double �^�̒l�����ʒu
// ShapeBase, ShapeArc �Ŏg�p����.
//------------------------------
	struct POINT_2D {
		double x;
		double y;
		// ���̈ʒu���܂ނ悤���`���g������.
		inline void exp(POINT_2D& r_lt, POINT_2D& r_rb) const noexcept
		{
			if (x < r_lt.x) {
				r_lt.x = x;
			}
			if (x > r_rb.x) {
				r_rb.x = x;
			}
			if (y < r_lt.y) {
				r_lt.y = y;
			}
			if (y > r_rb.y) {
				r_rb.y = y;
			}
		}
		inline POINT_2D nextafter(const double d) const noexcept
		{
			return POINT_2D{ std::nextafter(x, x + d), std::nextafter(y, y + d) };
		}
		inline operator D2D1_POINT_2F(void) const noexcept
		{
			return D2D1_POINT_2F{ static_cast<FLOAT>(x), static_cast<FLOAT>(y) };
		}
		inline POINT_2D operator -(const POINT_2D& q) const noexcept
		{
			return POINT_2D{ x - q.x, y - q.y };
		}
		inline POINT_2D operator -(const D2D1_POINT_2F q) const noexcept
		{
			return POINT_2D{ x - q.x, y - q.y };
		}
		inline POINT_2D operator -(void) const noexcept
		{
			return POINT_2D{ -x, -y };
		}
		inline POINT_2D operator *(const double s) const noexcept
		{
			return POINT_2D{ x * s, y * s };
		}
		inline double operator *(const POINT_2D& q) const noexcept
		{
			return x * q.x + y * q.y;
		}
		inline POINT_2D operator +(const POINT_2D& q) const noexcept
		{
			return POINT_2D{ x + q.x, y + q.y };
		}
		inline POINT_2D operator +(const D2D1_POINT_2F p) const noexcept
		{
			return POINT_2D{ x + p.x, y + p.y };
		}
		inline bool operator <(const POINT_2D& q) const noexcept
		{
			return x < q.x && y < q.y;
		}
		inline POINT_2D operator =(const D2D1_POINT_2F p) noexcept
		{
			return POINT_2D{ x = p.x, y = p.y };
		}
		inline POINT_2D operator =(const double s) noexcept
		{
			return POINT_2D{ x = s, y = s };
		}
		inline bool operator >(const POINT_2D& q) const noexcept
		{
			return x > q.x && y > q.y;
		}
		inline bool operator ==(const POINT_2D& q) const noexcept
		{
			return x == q.x && y == q.y;
		}
		inline bool operator !=(const POINT_2D& q) const noexcept
		{
			return x != q.x || y != q.y;
		}
		inline double opro(const POINT_2D& q) const noexcept
		{
			return x * q.y - y * q.x;
		}
	};

	//------------------------------
	// �Ȑ���̏��ϐ������Ƃɐڐ��x�N�g�������߂�.
	// c	����_
	// t	���ϐ�
	// v	t �ɂ�����ڐ��x�N�g��
	//------------------------------
	static inline void bezi_tvec_by_param(const POINT_2D c[4], const double t, POINT_2D& v) noexcept
	{
		const double a = -3.0 * (1.0 - t) * (1.0 - t);
		const double b = 3.0 * (1.0 - t) * (1.0 - 3.0 * t);
		const double d = 3.0 * t * (2.0 - 3.0 * t);
		const double e = (3.0 * t * t);
		v = c[0] * a + c[1] * b + c[2] * d + c[3] * e;
	}

	//------------------------------
	// �Ȑ���̏��ϐ������Ƃɔ����l�����߂�.
	// c	����_ (�R���g���[���|�C���g)
	// t	���ϐ�
	// �߂�l	���܂��������l
	//------------------------------
	static inline double bezi_deriv_by_param(const POINT_2D c[4], const double t) noexcept
	{
		// ���ϐ������ƂɃx�W�F�Ȑ���̐ڐ��x�N�g��������, ���̐ڐ��x�N�g���̒�����Ԃ�.
		POINT_2D v;	// t �ɂ�����ڐ��x�N�g��
		bezi_tvec_by_param(c, t, v);
		return sqrt(v * v);
	}

	// 2 �̏��ϐ������ 0-1 �̊ԂŐ��������肷��.
	// t_min	���������̏��ϐ�
	// t_max	�傫�����̏��ϐ�
	static inline bool bezi_test_param(const double t_min, const double t_max) noexcept
	{
		// �͈͂̏�� t_max �� 1+DBL_EPSILON ��菬 ?
		// t_min ���傫���čł��߂��l�� t_max ��菬 ?
		return -DBL_MIN < t_min && t_max < 1.0 + DBL_EPSILON &&
			std::nextafter(t_min, t_min + 1.0) < t_max;
	}

	//------------------------------
	// �Ȑ���̏��ϐ��̋�Ԃ����Ƃɒ��������߂�.
	// �V���v�\���@��p����.
	// c	����_
	// t_min	��Ԃ̎n�[
	// t_max	��Ԃ̏I�[
	// s_cnt	�V���v�\���@�̉�
	// �߂�l	���܂�������
	//------------------------------
	static double bezi_len_by_param(
		const POINT_2D c[4], const double t_min, const double t_max, const uint32_t s_cnt) noexcept
	{
		double t_len;
		uint32_t n;
		double h;
		double a, b;
		double t;
		double b0, b2;
		double s;

		/* �͈͂̏�������͐��������肷��. */
		/* ���� ? */
		if (bezi_test_param(t_min, t_max)) {
			/* �͈͏�� t_max -�͈͉��� t_min ������ t_len �Ɋi�[����. */
			t_len = t_max - t_min;
			/* ��Ԃ̕����� s_cnt �� t_len ����Z����. */
			/* ���̌��ʂ�؂�グ�Đ����l����. */
			/* �����l����Ԃ̔��� n �Ɋi�[����. */
			n = (int)std::ceil(t_len * (double)s_cnt);
			/* t_len / 2n ���K�� h �Ɋi�[����. */
			h = t_len / (2.0 * n);
			/* 0 ����Ԗڂ̕�����Ԃ̍��v�l a �Ɋi�[����. */
			a = 0.0;
			/* 0 �������Ԗڂ̕�����Ԃ̍��v�l b �Ɋi�[����. */
			b = 0.0;
			/* t_min+h �����ϐ� t �Ɋi�[����. */
			t = t_min + h;
			/* 1 ��Y���� i �Ɋi�[����. */
			/* i �� n ��菬 ? */
			for (uint32_t i = 1; i < n; i++) {
				/* 2i-1 �Ԗڂ̕�����Ԃ̔����l������, a �ɉ�����. */
				a += bezi_deriv_by_param(c, t);
				/* �K�� h �����ϐ� t �ɉ�����. */
				t += h;
				/* 2i �Ԗڂ̕�����Ԃ̔����l������, b �ɉ�����. */
				b += bezi_deriv_by_param(c, t);
				/* �K�� h �����ϐ� t �ɉ�����. */
				t += h;
				/* i ���C���N�������g����. */
			}
			/* 2n-1 �Ԗڂ̕�����Ԃ̔����l������, a �ɉ�����. */
			a += bezi_deriv_by_param(c, t);
			/* 0 �Ԗڂ̕�����Ԃł̔����l������, b0 �Ɋi�[����. */
			b0 = bezi_deriv_by_param(c, t_min);
			/* 2n �Ԗڂ̕�����Ԃł̔����l������, b2 �Ɋi�[����. */
			b2 = bezi_deriv_by_param(c, t_max);
			/* (b0+4a+2b+b2)h/3 ������, �ϕ��l s �Ɋi�[����. */
			s = (b0 + 4.0 * a + 2.0 * b + b2) * h / 3.0f;
		}
		else {
			/* 0 ��ϕ��l s �Ɋi�[����. */
			s = 0.0;
		}
		/* s ��Ԃ�. */
		return s;
	}

	//------------------------------
	// �Ȑ���̒��������Ƃɏ��ϐ������߂�.
	// c	����_
	// len	����
	// �߂�l	����ꂽ���ϐ��̒l
	//------------------------------
	static double bezi_param_by_len(const POINT_2D c[4], const double len) noexcept
	{
		double t;	// ���ϐ�
		double d;	// ���ϐ��̕ϕ�
		double e;	// �덷

		/* ��Ԃ̒��Ԓl 0.5 �����ϐ��Ɋi�[����. */
		t = 0.5;
		/* 0.25 �����ϐ��̕ϕ��Ɋi�[����. */
		/* ���ϐ��̕ϕ��� 0.001953125 �ȏ� ? */
		for (d = 0.25; d >= 0.001953125; d *= 0.5) {
			/* 0-���ϐ��͈̔͂������V���v�\�������Őϕ���, �Ȑ��̒��������߂�. */
			/* ���߂������Ǝw�肳�ꂽ�����̍������덷�Ɋi�[����. */
			e = bezi_len_by_param(c, 0.0, t, SIMPSON_CNT) - len;
			/* �덷�̐�΂� 0.125 ��菬 ? */
			if (fabs(e) < 0.125) {
				break;
			}
			/* �덷�� 0 ���� ? */
			else if (e > 0.0) {
				/* �ϕ������ϐ��������. */
				t -= d;
			}
			else {
				/* �ϕ������ϐ��ɑ���. */
				t += d;
			}
		}
		return t;
	}

}