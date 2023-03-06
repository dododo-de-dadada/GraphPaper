//------------------------------
// Shape_poly.cpp
// ���p�`
//------------------------------
#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �����̌�_��, ���̏��ϐ������߂�.
	// a	���� ab �̎n�_
	// b	���� ab �̏I�_
	// c	���� cd �̎n�_
	// d	���� cd �̏I�_
	// s	���� ab �ɑ΂����_�̏��ϐ�
	// t	���� cd �ɑ΂����_�̏��ϐ�
	// i	��_
	// �߂�l	��_�����܂�� true, �����łȂ���� false.
	static bool poly_find_intersection(
		const D2D1_POINT_2F a, const D2D1_POINT_2F b, const D2D1_POINT_2F c, const D2D1_POINT_2F d,
		double& s, double& t, D2D1_POINT_2F& i)
	{
		const double ab_x = static_cast<double>(b.x) - static_cast<double>(a.x);
		const double ab_y = static_cast<double>(b.y) - static_cast<double>(a.y);
		const double cd_x = static_cast<double>(d.x) - static_cast<double>(c.x);
		const double cd_y = static_cast<double>(d.y) - static_cast<double>(c.y);
		const double ac_x = static_cast<double>(c.x) - static_cast<double>(a.x);
		const double ac_y = static_cast<double>(c.y) - static_cast<double>(a.y);
		const double r = ab_x * cd_y - ab_y * cd_x;
		if (fabs(r) < FLT_MIN) {
			return false;
		}
		const double sr = cd_y * ac_x - cd_x * ac_y;
		if (fabs(sr) < FLT_MIN) {
			return false;
		}
		const double tr = ab_y * ac_x - ab_x * ac_y;
		if (fabs(tr) < FLT_MIN) {
			return false;
		}
		s = sr / r;
		t = tr / r;
		i.x = static_cast<FLOAT>(static_cast<double>(a.x) + ab_x * s);
		i.y = static_cast<FLOAT>(static_cast<double>(a.y) + ab_y * s);
		return true;
	}

	// test	���肷��_
	// p_end	�Ō�̒��_
	// s_cnt	�ӂ̐�
	// s[s_cnt]	�ӂ̃x�N�g�� (���̒��_�ւ̈ʒu�x�N�g��)
	// s_len[s_cnt]	�ӂ̒���
	// e_width	�ӂ̔����̑���
	static bool poly_test_cap_square(
		const D2D1_POINT_2F test, const D2D1_POINT_2F p_end, const size_t s_cnt, 
		const D2D1_POINT_2F s[], const double s_len[], const double e_width)
	{
		for (size_t i = 0; i < s_cnt; i++) {
			if (s_len[i] >= FLT_MIN) {
				D2D1_POINT_2F d;	// �ӂ̑����ɍ��킹���ӂ̕����x�N�g��
				pt_mul(s[i], -e_width / s_len[i], d);
				const D2D1_POINT_2F o{ d.y, -d.x };	// �ӂ̒����x�N�g��
				D2D1_POINT_2F q[4];	// ���点���ӂ̒[�_�̎l�ӌ`
				pt_add(d, o, q[0]);
				pt_sub(d, o, q[1]);
				q[2].x = -o.x;
				q[2].y = -o.y;
				q[3] = o;
				if (pt_in_poly(test, 4, q)) {
					return true;
				}
				break;
			}
		}
		D2D1_POINT_2F t;
		pt_sub(test, p_end, t);
		for (size_t i = s_cnt; i > 0; i--) {
			if (s_len[i - 1] >= FLT_MIN) {
				D2D1_POINT_2F d;	// �ӂ̑����ɍ��킹���ӂ̕����x�N�g��
				pt_mul(s[i - 1], e_width / s_len[i - 1], d);
				const D2D1_POINT_2F o{ d.y, -d.x };	// �ӂ̒����x�N�g��
				D2D1_POINT_2F q[4];	// ���点���ӂ̒[�_�̎l�ӌ`
				pt_add(d, o, q[0]);
				pt_sub(d, o, q[1]);
				q[2].x = -o.x;
				q[2].y = -o.y;
				q[3] = o;
				if (pt_in_poly(t, 4, q)) {
					return true;
				}
				break;
			}
		}
		return false;
	}

	// test	���肷��_
	// p_end	�Ō�̒��_
	// s_cnt	�ӂ̐�
	// s[s_cnt]	�ӂ̃x�N�g�� (���̒��_�ւ̈ʒu�x�N�g��)
	// s_len[s_cnt]	�ӂ̒���
	// e_width	�ӂ̔����̑���
	static bool poly_test_cap_triangle(
		const D2D1_POINT_2F test, const D2D1_POINT_2F p_end, const size_t s_cnt,
		const D2D1_POINT_2F s[], const double s_len[], const double e_width)
	{
		for (size_t i = 0; i < s_cnt; i++) {
			if (s_len[i] >= FLT_MIN) {
				D2D1_POINT_2F d;
				pt_mul(s[i], -e_width / s_len[i], d);
				const D2D1_POINT_2F o{ d.y, -d.x };
				D2D1_POINT_2F t[3];	//  ���点���ӂ̒[�_�̎O�p�`
				t[0] = d;
				t[1].x = -o.x;
				t[1].y = -o.y;
				t[2] = o;
				if (pt_in_poly(test, 3, t)) {
					return true;
				}
				break;
			}
		}
		D2D1_POINT_2F u;
		pt_sub(test, p_end, u);
		for (size_t i = s_cnt; i > 0; i--) {
			if (s_len[i - 1] >= FLT_MIN) {
				D2D1_POINT_2F d;
				pt_mul(s[i - 1], e_width / s_len[i - 1], d);
				const D2D1_POINT_2F o{ d.y, -d.x };
				D2D1_POINT_2F t[3];	// �O�p�`
				t[0] = d;
				t[1].x = -o.x;
				t[1].y = -o.y;
				t[2] = o;
				if (pt_in_poly(u, 3, t)) {
					return true;
				}
				break;
			}
		}
		return false;
	}

	// ���p�`�̊p���ʒu���܂ނ����肷�� (�ʎ��)
	// e_side	�������g�����ꂽ�ӂ̔z��
	static bool poly_test_join_bevel(
		const D2D1_POINT_2F test,
		const size_t p_cnt,
		const bool e_close,
		const D2D1_POINT_2F s[][4 + 1]) noexcept
	{
		// { 0, 1 }, { 1, 2 }, { v_cnt-2, v_cnt-1 }
		for (size_t i = 1; i < p_cnt; i++) {
			const D2D1_POINT_2F beveled[4]{
				s[i - 1][3], s[i][0], s[i][1], s[i - 1][2]
			};
			if (pt_in_poly(test, 4, beveled)) {
				return true;
			}
		}
		if (e_close) {
			const D2D1_POINT_2F beveled[4]{
				s[p_cnt - 1][3], s[0][0], s[0][1], s[p_cnt - 1][2]
			};
			if (pt_in_poly(test, 4, beveled)) {
				return true;
			}
		}
		return false;
	}

	// ���p�`�̊p���ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// s_cnt	�ӂ̐�
	// e_close	�ӂ����Ă��邩����
	// e_width	�ӂ̑����̔���.
	// s	�ӂ̔z�� [exp_cnt][4+1]
	// miter_limit	���̐�萧��
	// j_style	���̌������@
	static bool poly_test_join_miter(
		const D2D1_POINT_2F test,
		const size_t s_cnt,
		const bool e_close,
		const double e_width,
		const D2D1_POINT_2F s[][4 + 1],
		const double miter_limit,
		const D2D1_LINE_JOIN j_style) noexcept
	{
		for (size_t i = (e_close ? 0 : 1), j = (e_close ? s_cnt - 1 : 0); i < s_cnt; j = i++) {
			// �g�����ꂽ�ӂɂ��Ċp�̕��������߂�.
			//
			// �_ vi �łȂ���, �g�����ꂽ�� j �� i �̃C���[�W
			// j0                     j3      i0                     i3
			//  +---expanded side[j]-->+  vi   +---expanded side[i]-->+ 
			// j1                     j2      i1                     i2
			//
			// �g�����ꂽ�� j �� i �����s�����肷��.
			if (equal(s[j][3], s[i][0])) {
				// ���s�Ȃ�Ώd�Ȃ镔���͂Ȃ��̂�, ���̕ӂ�����.
				continue;
			}
			// �g�����ꂽ�� j �� i ���d�Ȃ邩���肷��.
			if (equal(s[j][3], s[i][1])) {
				// ���̌�������肩���肷��.
				if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER) {
					//���Ȃ��, �� j ���萧���̒����������������l�ӌ`�����߂�.
					D2D1_POINT_2F direction;	// �Ӄx�N�g��
					pt_sub(s[j][3], s[j][0], direction);
					pt_mul(direction, e_width * miter_limit / sqrt(pt_abs2(direction)), direction);
					D2D1_POINT_2F quadrilateral[4];
					quadrilateral[0] = s[j][3];
					quadrilateral[1] = s[j][2];
					pt_add(s[j][2], direction, quadrilateral[2]);
					pt_add(s[j][3], direction, quadrilateral[3]);
					// ���ׂ�ʒu���l�ӌ`�Ɋ܂܂�邩���肷��.
					if (pt_in_poly(test, 4, quadrilateral)) {
						return true;
					}
				}
				continue;
			}
			// �g�����ꂽ�� i �� j ���d�Ȃ镔��������, �l�ӌ`�Ɋi�[����.
			// ���� j0 j3 �� i0 i3 (��������� 0..3 ���ƌĂ�) �Ƃ̌�_�����߂�.
			// 0..3 ���Ɍ�_���Ȃ��Ȃ��, ���̊g�����ꂽ�ӂ�����.
			// 0..3 ���Ɍ�_��������, �g�����ꂽ�ӂ̊O���Ȃ��, �l�ӌ` { ��_, j3, v[i], i0 } �𓾂�.
			// 0..3 ���Ɍ�_��������, �g�����ꂽ�ӂ̓����Ȃ��, ���� j1 j2 �� i1 i2 (1..2 ���ƌĂ�) �Ƃ̌�_�����߂�.
			// 1..2 ���Ɍ�_���Ȃ�, �܂���, ��_�������Ă��g�����ꂽ�ӂ̓����Ȃ��, ���̊g�����ꂽ�ӂ�����.
			// 1..2 ���Ɍ�_��������, �g�����ꂽ�ӂ̊O���Ȃ��, �l�ӌ` { ��_, j2, v[i], i1 } �𓾂�.
			double t, u;	// ��_�̏��ϐ�
			D2D1_POINT_2F q[4 + 1];	// �l�ӌ` (��萧���𒴂���Ȃ�Ό܊p�`)
			if (!poly_find_intersection(s[j][0], s[j][3], s[i][0], s[i][3], t, u, q[0])) {
				continue;
			}
			if (t < 1.0 || u > 0.0) {
				if (!poly_find_intersection(s[j][1], s[j][2], s[i][1], s[i][2], t, u, q[0])) {
					continue;
				}
				if (t < 1.0 || u > 0.0) {
					continue;
				}
				q[1] = s[j][2];
				q[2] = s[i][4];
				q[3] = s[i][1];
			}
			else {
				q[1] = s[i][0];
				q[2] = s[i][4];
				q[3] = s[j][3];
			}

			// ��_�ɂ���������x�N�g���Ƃ��̒��������߂�.
			// �����x�N�g���̒�����, ��萧���ȉ������肷��.
			D2D1_POINT_2F d;	// �����x�N�g��
			pt_sub(q[0], s[i][4], d);
			const double d_abs2 = pt_abs2(d);	// �����x�N�g���̒����̓��
			const double limit_len = e_width * miter_limit;
			if (d_abs2 <= limit_len * limit_len) {
				// ��萧���ȉ��Ȃ��, ���ׂ�ʒu���l�ӌ` { q0, q1, q2, q3 } ���܂ނ����肷��.
				if (pt_in_poly(test, 4, q)) {
					// �ʒu���܂ނȂ� true ��Ԃ�.
					return true;
				}
				continue;
			}
			// ��萧���𒴂���Ȃ��, ���̌��������܂��͖ʎ�肩���肷��.
			if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				// ���̌��������܂��͖ʎ��Ȃ��, ���ׂ�ʒu���O�p�` { q1, q2, q3 } ���܂ނ����肷��.
				const D2D1_POINT_2F* triangle = q + 1;
				if (pt_in_poly(test, 3, triangle)) {
					// �ʒu���܂ނȂ� true ��Ԃ�.
					return true;
				}
				continue;
			}
			// �����x�N�g����ɂ�����, ��_�Ƃ̋��������傤�ǐ�萧���̒����ɂȂ�_ m ��,
			// �����x�N�g���ƒ��s����x�N�g����ɂ�����, �_ m ��ʂ钼����̓_ n �����߂�.
			// ���� q3 q0 �� m n �Ƃ̌�_������, q4 �Ɋi�[����.
			// ���� q0 q1 �� m n �Ƃ̌�_������, �����V���� q0 �Ƃ���.
			// ���ׂ�ʒu���܊p�` { q0, q1, q2, q3, q4 } ���܂ނ����肷��.
			D2D1_POINT_2F mitered;
			pt_mul_add(d, limit_len / sqrt(d_abs2), s[i][4], mitered);
			D2D1_POINT_2F orthogonal;
			pt_add(mitered, D2D1_POINT_2F{ d.y, -d.x }, orthogonal);
			poly_find_intersection(q[3], q[0], mitered, orthogonal, t, u, q[4]);
			poly_find_intersection(q[0], q[1], mitered, orthogonal, t, u, q[0]);
			const D2D1_POINT_2F* pentagon = q;
			if (pt_in_poly(test, 5, pentagon)) {
				// �ʒu���܂ނȂ� true ��Ԃ�.
				return true;
			}
		}
		return false;
	}

	// ���p�`�̊p���ʒu���܂ނ����肷�� (�ۂ܂����p)
	// e_width	�ӂ̔����̑���
	static bool poly_test_join_round(
		const D2D1_POINT_2F& test, const size_t s_cnt, const D2D1_POINT_2F s[], const double e_width)
	{
		for (size_t i = 0; i < s_cnt; i++) {
			if (pt_in_circle(test, s[i], e_width)) {
				return true;
			}
		}
		return false;
	}

	// �ʒu��, �����Ɋ܂܂�邩���肷��.
	// test	���肷��ʒu (�����̎n�_�����_�Ƃ���)
	// p_cnt	�n�_�������ʒu�x�N�g���̐�
	// p	�n�_�������ʒu�x�N�g���̔z��
	// s_opaque	�����s����������
	// s_width	���̑���
	// e_closed	�������Ă��邩����
	// s_join	���̌���
	// s_limit	��萧��
	// f_opa	�h��Ԃ����s����������
	static uint32_t poly_hit_test(
		const D2D1_POINT_2F test, const size_t p_cnt, const D2D1_POINT_2F p[], const bool s_opaque,
		const double s_width, const bool e_closed, const CAP_STYLE& s_cap,
		const D2D1_LINE_JOIN s_join, const double s_limit, const bool f_opaque, const double a_len)
	{
		D2D1_POINT_2F q[N_GON_MAX]{ { 0.0f, 0.0f }, };	// ���_ (�n�_ { 0,0 } ���܂߂�)
		double s_len[N_GON_MAX];	// �ӂ̒���
		size_t n_cnt = 0;	// �����̂���ӂ̐�
		size_t k = static_cast<size_t>(-1);	// �����������_
		for (size_t i = 0; i < p_cnt; i++) {
			// ���肷��ʒu��, ���_�̕��ʂɊ܂܂�邩���肷��.
			if (pt_in_anc(test, q[i], a_len)) {
				k = i;
			}
			// �ӂ̒��������߂�.
			s_len[i] = sqrt(pt_abs2(p[i]));
			// �ӂ̒��������邩���肷��.
			if (s_len[i] >= FLT_MIN) {
				n_cnt++;
			}
			// ���_�ɕӃx�N�g��������, ���̒��_�����߂�.
			pt_add(q[i], p[i], q[i + 1]);
		}
		// ���肷��ʒu��, �I�_�̕��ʂɊ܂܂�邩���肷��.
		if (pt_in_anc(test, q[p_cnt], a_len)) {
			k = p_cnt;
		}
		// ���_���������������肷��.
		if (k != -1) {
			return ANC_TYPE::ANC_P0 + static_cast<uint32_t>(k);
		}
		// �����s���������肷��.
		if (s_opaque) {
			// �s�����Ȃ��, ���̑����̔����̕�������, �g�����镝�Ɋi�[����.
			const auto e_width = max(max(static_cast<double>(s_width), a_len) * 0.5, 0.5);	// �g�����镝
			// �S�Ă̕ӂ̒������[�������肷��.
			if (n_cnt == 0) {
				// �[���Ȃ��, ���肷��ʒu��, �g�����镝�𔼌a�Ƃ���~�Ɋ܂܂�邩���肷��.
				if (pt_in_circle(test, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
				return ANC_TYPE::ANC_PAGE;
			}
			// �ӂ����Ă��邩���肷��.
			if (e_closed) {
				// ���Ă���Ȃ�, �n�_�� { 0, 0 } �Ȃ̂ŏI�_�ւ̃x�N�g����, ���̂܂܍Ō�̕ӂ̒����Ƃ���.
				s_len[p_cnt] = sqrt(pt_abs2(q[p_cnt]));
			}
			// ���ĂȂ��Ȃ�, �[�̌`�����~�`�����肷��.
			else if (equal(s_cap, CAP_ROUND)) {
				if (pt_in_circle(test, e_width) || pt_in_circle(test, q[p_cnt], e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// ���ĂȂ��Ȃ�, �[�̌`���������`�����肷��.
			else if (equal(s_cap, CAP_SQUARE)) {
				if (poly_test_cap_square(test, q[p_cnt], p_cnt, p, s_len, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// ���ĂȂ��Ȃ�, �[�̌`�����O�p�`�����肷��.
			else if (equal(s_cap, CAP_TRIANGLE)) {
				if (poly_test_cap_triangle(test, q[p_cnt], p_cnt, p, s_len, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			D2D1_POINT_2F s[N_GON_MAX][4 + 1];	// �������g�����ꂽ�� (+���_)
			size_t s_cnt = 0;
			for (size_t i = 0; i < p_cnt; i++) {
				// �� i �̒������Ȃ������肷��.
				if (s_len[i] < FLT_MIN) {
					// �_ i ����~����, �����̂���� m ��������.
					size_t m = static_cast<size_t>(-1);
					for (size_t h = i; h > 0; h--) {
						if (s_len[h - 1] >= FLT_MIN) {
							m = h - 1;
							break;
						}
					}
					// �~���̕ӂ��������������肷��.
					D2D1_POINT_2F prev;	// ���O�̕Ӄx�N�g��
					if (m != static_cast<size_t>(-1)) {
						// ���������ӂ𒼑O�̕Ӄx�N�g���Ɋi�[����.
						prev = p[m];
					}
					// ������Ȃ������Ȃ��,
					// �ӂ����Ă���, ���Ō�̕ӂ̒�������[�������肷��.
					else if (e_closed && s_len[p_cnt] >= FLT_MIN) {
						// �Ō�̒��_�̔��΃x�N�g��������, ���O�̕Ӄx�N�g���Ƃ���.
						prev = D2D1_POINT_2F{ -q[p_cnt].x, -q[p_cnt].y };
					}
					else {
						continue;
					}
					// �_ i ���珸����, �����̂���ӂ�������.
					size_t n = static_cast<size_t>(-1);
					for (size_t j = i + 1; j < p_cnt; j++) {
						if (s_len[j] >= FLT_MIN) {
							n = j;
							break;
						}
					}
					// �����̕ӂ��������������肷��.
					D2D1_POINT_2F next;	// ����̕Ӄx�N�g��
					if (n != -1) {
						// ���������ӂ𒼌�̕Ӄx�N�g���Ɋi�[����.
						next = p[n];
					}
					// ������Ȃ������Ȃ��,
					// �ӂ����Ă���, ���Ō�̕ӂɒ��������邩���肷��.
					else if (e_closed && s_len[p_cnt] >= FLT_MIN) {
						// �Ō�̒��_�̔��΃x�N�g��������, ����̕Ӄx�N�g���Ƃ���.
						next = D2D1_POINT_2F{ -q[p_cnt].x, -q[p_cnt].y };
					}
					else {
						continue;
					}
					// ���O�ƒ���̕Ӄx�N�g�����������x�N�g�� (resultant) �����߂�.
					D2D1_POINT_2F r;
					pt_add(prev, next, r);
					// �����x�N�g���̒������Ȃ������肷��.
					double r_abs = pt_abs2(r);
					if (r_abs < FLT_MIN) {
						// ���O�̕Ӄx�N�g���������x�N�g���Ƃ���.
						r = prev;
						r_abs = pt_abs2(r);
					}
					// �����x�N�g���̒����x�N�g�������߂�.
					// ���x�N�g���Ƃ�������, �g�����镝�Ƃ���.
					pt_mul(r, e_width / sqrt(r_abs), r);
					const D2D1_POINT_2F orthogonal{ r.y, -r.x };
					// ���_ i �𒼌��x�N�g���ɉ����Ďl���Ɋg����, �g�����ꂽ�� i �Ɋi�[����.
					const double cx = r.x;
					const double cy = r.y;
					const double ox = orthogonal.x;
					const double oy = orthogonal.y;
					pt_add(q[i], -cx - ox, -cy - oy, s[s_cnt][0]);
					pt_add(q[i], -cx + ox, -cy + oy, s[s_cnt][1]);
					pt_add(q[i], cx + ox, cy + oy, s[s_cnt][2]);
					pt_add(q[i], cx - ox, cy - oy, s[s_cnt][3]);
					s[s_cnt][4] = q[i];
				}
				else {
					// �Ӄx�N�g���ɒ�������x�N�g�������߂�.
					// �����x�N�g���̒�����, �g�����镝�Ƃ���.
					D2D1_POINT_2F direction;	// �����x�N�g��
					pt_mul(p[i], e_width / s_len[i], direction);
					const D2D1_POINT_2F orthogonal{ direction.y, -direction.x };	// ��������x�N�g��
					// ���_ i �� i+1 �𒼌��x�N�g���ɉ����Đ��t�Ɋg����, �g�����ꂽ�� i �Ɋi�[����.
					pt_sub(q[i], orthogonal, s[s_cnt][0]);
					pt_add(q[i], orthogonal, s[s_cnt][1]);
					pt_add(q[i + 1], orthogonal, s[s_cnt][2]);
					pt_sub(q[i + 1], orthogonal, s[s_cnt][3]);
					s[s_cnt][4] = q[i];
				}
				// ���ׂ�ʒu��, �g�����ꂽ�ӂɊ܂܂�邩���肷��.
				if (pt_in_poly(test, 4, s[s_cnt++])) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// �ӂ����Ă��邩, �����ӂɒ��������邩���肷��.
			if (e_closed && s_len[p_cnt] >= FLT_MIN) {
				// �Ō�̕ӂ̈ʒu�𔽓]����, �g�����镝�̒����ɍ��킹, �Ӄx�N�g�������߂�.
				// �Ӄx�N�g���ɒ�������x�N�g�������߂�.
				// �n�_�ƏI�_�𒼌��x�N�g���ɉ����Đ��t�Ɋg����, �g�����ꂽ�ӂɊi�[����.
				D2D1_POINT_2F direction;
				pt_mul(q[p_cnt], -e_width / s_len[p_cnt], direction);
				const D2D1_POINT_2F orthogonal{ direction.y, -direction.x };
				pt_sub(q[p_cnt], orthogonal, s[s_cnt][0]);
				pt_add(q[p_cnt], orthogonal, s[s_cnt][1]);
				s[s_cnt][2] = orthogonal; // v0 + o_vec
				//pt_neg(o_vec, e_side[e_cnt][3]); // v0 - o_vec
				s[s_cnt][3].x = -orthogonal.x;
				s[s_cnt][3].y = -orthogonal.y;
				s[s_cnt][4] = q[p_cnt];
				// ���肷��ʒu���g�����ꂽ�ӂɊ܂܂�邩���肷��.
				if (pt_in_poly(test, 4, s[s_cnt++])) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				if (poly_test_join_bevel(test, s_cnt, e_closed, s)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			else if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER
				|| s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				if (poly_test_join_miter(test, s_cnt, e_closed, e_width, s, s_limit, s_join)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			else if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				if (poly_test_join_round(test, p_cnt + 1, q, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
		}
		if (f_opaque) {
			if (pt_in_poly(test, p_cnt + 1, q)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// ���̕Ԃ��Ɛ�[�̈ʒu�𓾂�.
	// p_cnt	�܂���̒��_ (�[�_���܂�) �̐�
	// p	���_�̔z��
	// a_size	���̑傫��
	// tip	���̐�[�̈ʒu
	// barb	���̕Ԃ��̈ʒu
	bool ShapePolygon::poly_get_pos_arrow(
		const size_t p_cnt, const D2D1_POINT_2F p[], const ARROW_SIZE& a_size, D2D1_POINT_2F& tip,
		D2D1_POINT_2F barb[]) noexcept
	{
		double a_offset = a_size.m_offset;	// ���̐�[�̃I�t�Z�b�g
		for (size_t i = p_cnt - 1; i > 0; i--) {

			// ���_�Ԃ̍��������Ƃ��̒��������߂�.
			// ��̒������قڃ[�������肷��.
			// �����[���Ȃ炱�̒��_�͖�������.
			D2D1_POINT_2F q;
			pt_sub(p[i], p[i - 1], q);	// ���_�Ԃ̍���
			const auto a_len = sqrt(pt_abs2(q));	// ��̒���
			if (a_len < FLT_MIN) {
				continue;
			}

			// ��̒�������邵��[�̃I�t�Z�b�g���Z�������肷��.
			if (a_len < a_offset) {
				// ���̍��������邩���肷��.
				if (i > 1) {
					// �I�t�Z�b�g���̒��������Z������.
					a_offset -= a_len;
					continue;
				}
				a_offset = a_len;
			}

			// ���̕Ԃ��̈ʒu�����߂�.
			const auto a_end = p[i - 1];		// ��̏I�[
			const auto b_len = a_size.m_length;	// ���̒���
			const auto b_width = a_size.m_width;	// ���̕Ԃ��̕�
			get_pos_barbs(q, a_len, b_width, b_len, barb);
			pt_mul_add(q, 1.0 - a_offset / a_len, a_end, tip);
			pt_add(barb[0], tip, barb[0]);
			pt_add(barb[1], tip, barb[1]);
			return true;
		}
		return false;
	}

	// ��`�����Ƃɑ��p�`���쐬����.
	// start	�n�_
	// pos	�I�_�̈ʒu�x�N�g��
	// p_opt	���p�`�̍쐬���@
	// p	���_�̔z��
	void ShapePolygon::poly_create_by_box(
		const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const POLY_OPTION& p_opt,
		D2D1_POINT_2F p[]) noexcept
	{
		// v_cnt	���p�`�̒��_�̐�
		// v_up	���_����ɍ쐬���邩����
		// v_reg	�����p�`���쐬���邩����
		// v_clock	���v����ō쐬���邩����
		const auto p_cnt = p_opt.m_vertex_cnt;
		if (p_cnt == 0) {
			return;
		}
		const auto p_up = p_opt.m_vertex_up;
		const auto p_reg = p_opt.m_regular;
		const auto p_clock = p_opt.m_clockwise;

		// { 0, 0 } �𒆐S�Ƃ��锼�a 1 �̉~�����Ƃɐ����p�`���쐬��,
		// ���ł�, ���̒��_�����傤�Ǌ܂�, ���E��`�𓾂�.
		D2D1_POINT_2F box_lt{ 0.0f, 0.0f };	// ���E��`�̍���_
		D2D1_POINT_2F box_rb{ 0.0f, 0.0f };	// ���E��`�̉E���_
		const double r = (p_clock ? -2.0 * M_PI : 2.0 * M_PI);	// �S��
		const double s = p_up ? (M_PI_2) : (M_PI_2 + M_PI / p_cnt);	// �n�_�̊p�x
		for (uint32_t i = 0; i < p_cnt; i++) {
			const double t = s + r * i / p_cnt;	// i �Ԗڂ̒��_�̊p�x
			p[i].x = static_cast<FLOAT>(cos(t));
			p[i].y = static_cast<FLOAT>(-sin(t));
			if (p[i].x < box_lt.x) {
				box_lt.x = p[i].x;
			}
			if (p[i].y < box_lt.y) {
				box_lt.y = p[i].y;
			}
			if (p[i].x > box_rb.x) {
				box_rb.x = p[i].x;
			}
			if (p[i].y > box_rb.y) {
				box_rb.y = p[i].y;
			}
		}

		// ���E��`���ʒu�x�N�g���ŕ\�������`�ɍ��v�����邽�߂�, �g�嗦�𓾂�.
		// �����p�`�̏ꍇ, X ������ Y ������, �ǂ��炩���������̊g�嗦�Ɉ�v������.
		const double px = fabs(pos.x);
		const double py = fabs(pos.y);
		double sx;	// X �����̊g�嗦
		double sy;	// Y �����̊g�嗦
		if (px <= py) {
			sx = px / (box_rb.x - box_lt.x);
			if (p_reg) {
				sy = sx;
			}
			else {
				sy = py / (box_rb.y - box_lt.y);
			}
		}
		else{
			sy = py / (box_rb.y - box_lt.y);
			if (p_reg) {
				sx = sy;
			}
			else {
				sx = px / (box_rb.x - box_lt.x);
			}
		}

		// �ʒu�x�N�g���̐����ɂ����, ���E��`�̂ǂ̒��_����_�ɂ��邩���߂�.
		double bx;	// ��_ X ���W
		double by;	// ��_ Y ���W
		if (pos.x >= 0.0f && pos.y >= 0.0f) {
			bx = box_lt.x;
			by = box_lt.y;
		}
		else if (pos.x < 0.0f && pos.y >= 0.0f) {
			bx = box_rb.x;
			by = box_lt.y;
		}
		else if (pos.x < 0.0f && pos.y <= 0.0f) {
			bx = box_rb.x;
			by = box_rb.y;
		}
		else {
			bx = box_lt.x;
			by = box_rb.y;
		}

		// �����p�`��, ��_�����_�ƂȂ�悤���s�ړ��������Ɗg�債,
		// �n�_�ɕ��s�ړ�����.
		for (uint32_t i = 0; i < p_cnt; i++) {
			p[i].x = static_cast<FLOAT>(start.x + sx * (p[i].x - bx));
			p[i].y = static_cast<FLOAT>(start.y + sy * (p[i].y - by));
			pt_round(p[i], PT_ROUND, p[i]);
		}

	}

	// �}�`��\������.
	void ShapePolygon::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
		ID2D1Factory* factory;
		target->GetFactory(&factory);

		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}
		if ((m_arrow_style != ARROW_STYLE::NONE && m_d2d_arrow_geom == nullptr) ||
			m_d2d_path_geom == nullptr) {
			if (m_d2d_path_geom != nullptr) {
				m_d2d_path_geom = nullptr;
			}
			else if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			if (m_pos.size() < 1) {
				return;
			}

			// �J�n�ʒu��, �����̔z������Ƃ�, ���_�����߂�.
			const size_t p_cnt = m_pos.size() + 1;	// ���_�̐� (�����̐� + 1)
			D2D1_POINT_2F p[N_GON_MAX];
			p[0] = m_start;
			for (size_t i = 1; i < p_cnt; i++) {
				pt_add(p[i - 1], m_pos[i - 1], p[i]);
			}

			// �܂���̃p�X�W�I���g�����쐬����.
			const auto f_begin = is_opaque(m_fill_color) ?
				D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
				D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW;
			const auto f_end = (m_end_closed ?
				D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED :
				D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
			winrt::com_ptr<ID2D1GeometrySink> sink;
			winrt::check_hresult(factory->CreatePathGeometry(m_d2d_path_geom.put()));
			winrt::check_hresult(m_d2d_path_geom->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(p[0], f_begin);
			for (size_t i = 1; i < p_cnt; i++) {
				sink->AddLine(p[i]);
			}
			sink->EndFigure(f_end);
			winrt::check_hresult(sink->Close());
			sink = nullptr;

			// ��邵�̌`�����Ȃ������肷��.
			const auto a_style = m_arrow_style;
			if (a_style != ARROW_STYLE::NONE) {

				// ��邵�̈ʒu�����߂�.
				D2D1_POINT_2F tip;
				D2D1_POINT_2F barb[2];
				if (poly_get_pos_arrow(p_cnt, p, m_arrow_size, tip, barb)) {

					// ��邵�̃p�X�W�I���g�����쐬����.
					const auto a_begin = (a_style == ARROW_STYLE::FILLED ?
						D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
						D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
					const auto a_end = (a_style == ARROW_STYLE::FILLED ?
						D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED :
						D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
					winrt::check_hresult(factory->CreatePathGeometry(m_d2d_arrow_geom.put()));
					winrt::check_hresult(m_d2d_arrow_geom->Open(sink.put()));
					sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
					sink->BeginFigure(barb[0], a_begin);
					sink->AddLine(tip);
					sink->AddLine(barb[1]);
					sink->EndFigure(a_end);
					winrt::check_hresult(sink->Close());
					sink = nullptr;
				}
			}
		}
		if (is_opaque(m_fill_color)) {
			const auto p_geom = m_d2d_path_geom.get();
			if (p_geom != nullptr) {
				brush->SetColor(m_fill_color);
				target->FillGeometry(p_geom, brush, nullptr);
			}
		}
		if (is_opaque(m_stroke_color)) {
			const auto p_geom = m_d2d_path_geom.get();	// �p�X�̃W�I���g��
			const auto s_width = m_stroke_width;	// �܂���̑���
			const auto s_style = m_d2d_stroke_style.get();	// �܂���̌`��
			brush->SetColor(m_stroke_color);
			target->DrawGeometry(p_geom, brush, s_width, s_style);
			if (m_arrow_style != ARROW_STYLE::NONE) {
				const auto a_geom = m_d2d_arrow_geom.get();
				if (a_geom != nullptr) {
					target->FillGeometry(a_geom, brush, nullptr);
					if (m_arrow_style != ARROW_STYLE::FILLED) {
						target->DrawGeometry(a_geom, brush, s_width, m_d2d_arrow_style.get());
					}
				}
			}
		}
		if (m_anc_show && is_selected()) {
			D2D1_POINT_2F a{ m_start };	// �}�`�̕��ʂ̈ʒu
			anc_draw_square(a, target, brush);
			const size_t d_cnt = m_pos.size();	// �����̐�
			for (size_t i = 0; i < d_cnt; i++) {
				pt_add(a, m_pos[i], a);
				anc_draw_square(a, target, brush);
			}
		}
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// a_len	�A���J�[�̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapePolygon::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		D2D1_POINT_2F t;
		pt_sub(test, m_start, t);
		return poly_hit_test(
			t, m_pos.size(), m_pos.data(), is_opaque(m_stroke_color), m_stroke_width, m_end_closed,
			m_stroke_cap, m_join_style, m_join_miter_limit, is_opaque(m_fill_color), m_anc_width);
	}

	// ��`�͈͂Ɋ܂܂�邩���肷��.
	// lt	��`�̍���ʒu
	// rb	��`�̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapePolygon::in_area(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		if (!pt_in_rect(m_start, lt, rb)) {
			return false;
		}
		const size_t p_cnt = m_pos.size();	// �����̐�
		D2D1_POINT_2F p{ m_start };
		for (size_t i = 0; i < p_cnt; i++) {
			p.x += m_pos[i].x;
			p.y += m_pos[i].y;
			if (!pt_in_rect(p, lt, rb)) {
				return false;
			}
		}
		return true;
	}

	bool ShapePolygon::set_arrow_style(const ARROW_STYLE val) noexcept
	{
		if (!m_end_closed) {
			return ShapePath::set_arrow_style(val);
		}
		return false;
	}

	// �}�`���쐬����.
	// start	�͂ޗ̈�̎n�_
	// pos	�͂ޗ̈�̏I�_�ւ̈ʒu�x�N�g��
	// page	�y�[�W
	// p_opt	���p�`�̑I����
	ShapePolygon::ShapePolygon(
		const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page, const POLY_OPTION& p_opt) :
		ShapePath::ShapePath(page, p_opt.m_end_closed),
		m_end_closed(p_opt.m_end_closed)
	{
		D2D1_POINT_2F p[N_GON_MAX];
		poly_create_by_box(start, pos, p_opt, p);

		m_start = p[0];
		m_pos.resize(p_opt.m_vertex_cnt - 1);
		m_pos.shrink_to_fit();
		for (size_t i = 1; i < p_opt.m_vertex_cnt; i++) {
			m_pos[i - 1].x = p[i].x - p[i - 1].x;
			m_pos[i - 1].y = p[i].y - p[i - 1].y;
		}
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	// dt_reader	�f�[�^���[�_�[
	ShapePolygon::ShapePolygon(const Shape& page, DataReader const& dt_reader) :
		ShapePath::ShapePath(page, dt_reader)
	{
		m_end_closed = dt_reader.ReadBoolean();
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapePolygon::write(DataWriter const& dt_writer) const
	{
		ShapePath::write(dt_writer);
		dt_writer.WriteBoolean(m_end_closed);
	}

}