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
	// e	��_
	// �߂�l	��_�����܂�� true, �����łȂ���� false.
	static bool poly_find_intersection(
		const D2D1_POINT_2F a, const D2D1_POINT_2F b, const D2D1_POINT_2F c, const D2D1_POINT_2F d,
		double& s, double& t, D2D1_POINT_2F& e)
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
		e.x = static_cast<FLOAT>(static_cast<double>(a.x) + ab_x * s);
		e.y = static_cast<FLOAT>(static_cast<double>(a.y) + ab_y * s);
		return true;
	}

	static bool poly_test_cap_square(
		const D2D1_POINT_2F test, const D2D1_POINT_2F v_end, const size_t d_cnt, 
		const D2D1_POINT_2F d_vec[], const double e_len[], const double e_width)
	{
		for (size_t i = 0; i < d_cnt; i++) {
			if (e_len[i] >= FLT_MIN) {
				D2D1_POINT_2F direction;	// �Ӄx�N�g��
				pt_mul(d_vec[i], -e_width / e_len[i], direction);
				const D2D1_POINT_2F orthogonal{ direction.y, -direction.x };
				D2D1_POINT_2F quadrilateral[4];
				pt_add(direction, orthogonal, quadrilateral[0]);
				pt_sub(direction, orthogonal, quadrilateral[1]);
				quadrilateral[2].x = -orthogonal.x;
				quadrilateral[2].y = -orthogonal.y;
				quadrilateral[3] = orthogonal;
				if (pt_in_poly(test, 4, quadrilateral)) {
					return true;
				}
				break;
			}
		}
		D2D1_POINT_2F t;
		pt_sub(test, v_end, t);
		for (size_t i = d_cnt; i > 0; i--) {
			if (e_len[i - 1] >= FLT_MIN) {
				D2D1_POINT_2F direction;	// �Ӄx�N�g��
				pt_mul(d_vec[i - 1], e_width / e_len[i - 1], direction);
				const D2D1_POINT_2F orthogonal{ direction.y, -direction.x };
				D2D1_POINT_2F quadrilateral[4];
				pt_add(direction, orthogonal, quadrilateral[0]);
				pt_sub(direction, orthogonal, quadrilateral[1]);
				quadrilateral[2].x = -orthogonal.x;
				quadrilateral[2].y = -orthogonal.y;
				quadrilateral[3] = orthogonal;
				if (pt_in_poly(t, 4, quadrilateral)) {
					return true;
				}
				break;
			}
		}
		return false;
	}

	static bool poly_test_cap_triangle(
		const D2D1_POINT_2F test, const D2D1_POINT_2F v_end, const size_t d_cnt,
		const D2D1_POINT_2F d_vec[], const double e_len[], const double e_width)
	{
		for (size_t i = 0; i < d_cnt; i++) {
			if (e_len[i] >= FLT_MIN) {
				D2D1_POINT_2F direction;
				pt_mul(d_vec[i], -e_width / e_len[i], direction);
				const D2D1_POINT_2F orthogonal{ direction.y, -direction.x };
				D2D1_POINT_2F triangle[3];
				triangle[0] = direction;
				triangle[1].x = -orthogonal.x;
				triangle[1].y = -orthogonal.y;
				triangle[2] = orthogonal;
				if (pt_in_poly(test, 3, triangle)) {
					return true;
				}
				break;
			}
		}
		D2D1_POINT_2F u_pos;
		pt_sub(test, v_end, u_pos);
		for (size_t i = d_cnt; i > 0; i--) {
			if (e_len[i - 1] >= FLT_MIN) {
				D2D1_POINT_2F direction;
				pt_mul(d_vec[i - 1], e_width / e_len[i - 1], direction);
				const D2D1_POINT_2F orthogonal{ direction.y, -direction.x };
				D2D1_POINT_2F triangle[3];	// �O�p�`
				triangle[0] = direction;
				//pt_neg(o_vec, tri_pos[1]);
				triangle[1].x = -orthogonal.x;
				triangle[1].y = -orthogonal.y;
				triangle[2] = orthogonal;
				if (pt_in_poly(u_pos, 3, triangle)) {
					return true;
				}
				break;
			}
		}
		return false;
	}

	// ���p�`�̊p���ʒu���܂ނ����肷�� (�ʎ��)
	static bool poly_test_join_bevel(
		const D2D1_POINT_2F test,
		const size_t v_cnt,
		const bool v_close,
		const D2D1_POINT_2F e_side[][4 + 1]) noexcept
	{
		// { 0, 1 }, { 1, 2 }, { v_cnt-2, v_cnt-1 }
		for (size_t i = 1; i < v_cnt; i++) {
			const D2D1_POINT_2F beveled[4]{
				e_side[i - 1][3], e_side[i][0], e_side[i][1], e_side[i - 1][2]
			};
			if (pt_in_poly(test, 4, beveled)) {
				return true;
			}
		}
		if (v_close) {
			const D2D1_POINT_2F beveled[4]{
				e_side[v_cnt - 1][3], e_side[0][0], e_side[0][1], e_side[v_cnt - 1][2] 
			};
			if (pt_in_poly(test, 4, beveled)) {
				return true;
			}
		}
		return false;
	}

	// ���p�`�̊p���ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// e_cnt	�g�������ӂ̐�
	// e_close	�g�������ӂ����Ă��邩����
	// e_width	�ӂ̑����̔���.
	// e	�g�������ӂ̔z�� [exp_cnt][4+1]
	// miter_limit	���̐�萧��
	// j_style	���̌������@
	static bool poly_test_join_miter(
		const D2D1_POINT_2F test,
		const size_t e_cnt,
		const bool e_close,
		const double e_width,
		const D2D1_POINT_2F e[][4 + 1],
		const double miter_limit,
		const D2D1_LINE_JOIN j_style) noexcept
	{
		for (size_t i = (e_close ? 0 : 1), j = (e_close ? e_cnt - 1 : 0); i < e_cnt; j = i++) {
			// �g�����ꂽ�ӂɂ��Ċp�̕��������߂�.
			//
			// �_ vi �łȂ���, �g�����ꂽ�� j �� i �̃C���[�W
			// j0                     j3      i0                     i3
			//  +---expanded side[j]-->+  vi   +---expanded side[i]-->+ 
			// j1                     j2      i1                     i2
			//
			// �g�����ꂽ�� j �� i �����s�����肷��.
			if (equal(e[j][3], e[i][0])) {
				// ���s�Ȃ�Ώd�Ȃ镔���͂Ȃ��̂�, ���̕ӂ�����.
				continue;
			}
			// �g�����ꂽ�� j �� i ���d�Ȃ邩���肷��.
			if (equal(e[j][3], e[i][1])) {
				// ���̌�������肩���肷��.
				if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER) {
					//���Ȃ��, �� j ���萧���̒����������������l�ӌ`�����߂�.
					D2D1_POINT_2F direction;	// �Ӄx�N�g��
					pt_sub(e[j][3], e[j][0], direction);
					pt_mul(direction, e_width * miter_limit / sqrt(pt_abs2(direction)), direction);
					D2D1_POINT_2F quadrilateral[4];
					quadrilateral[0] = e[j][3];
					quadrilateral[1] = e[j][2];
					pt_add(e[j][2], direction, quadrilateral[2]);
					pt_add(e[j][3], direction, quadrilateral[3]);
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
			double s, t;	// ��_�̏��ϐ�
			D2D1_POINT_2F q[4 + 1];	// �l�ӌ` (��萧���𒴂���Ȃ�Ό܊p�`)
			if (!poly_find_intersection(e[j][0], e[j][3], e[i][0], e[i][3], s, t, q[0])) {
				continue;
			}
			if (s < 1.0 || t > 0.0) {
				if (!poly_find_intersection(e[j][1], e[j][2], e[i][1], e[i][2], s, t, q[0])) {
					continue;
				}
				if (s < 1.0 || t > 0.0) {
					continue;
				}
				q[1] = e[j][2];
				q[2] = e[i][4];
				q[3] = e[i][1];
			}
			else {
				q[1] = e[i][0];
				q[2] = e[i][4];
				q[3] = e[j][3];
			}

			// ��_�ɂ���������x�N�g���Ƃ��̒��������߂�.
			// �����x�N�g���̒�����, ��萧���ȉ������肷��.
			D2D1_POINT_2F d;	// �����x�N�g��
			pt_sub(q[0], e[i][4], d);
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
			pt_mul_add(d, limit_len / sqrt(d_abs2), e[i][4], mitered);
			D2D1_POINT_2F orthogonal;
			pt_add(mitered, D2D1_POINT_2F{ d.y, -d.x }, orthogonal);
			poly_find_intersection(q[3], q[0], mitered, orthogonal, s, t, q[4]);
			poly_find_intersection(q[0], q[1], mitered, orthogonal, s, t, q[0]);
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
		const D2D1_POINT_2F& test, const size_t v_cnt, const D2D1_POINT_2F v_pos[], const double e_width)
	{
		for (size_t i = 0; i < v_cnt; i++) {
			if (pt_in_circle(test, v_pos[i], e_width)) {
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
		double e_len[N_GON_MAX];	// �ӂ̒���
		size_t nz_cnt = 0;	// �����̂���ӂ̐�
		size_t k = static_cast<size_t>(-1);	// �����������_
		for (size_t i = 0; i < p_cnt; i++) {
			// ���肷��ʒu��, ���_�̕��ʂɊ܂܂�邩���肷��.
			if (pt_in_anc(test, q[i], a_len)) {
				k = i;
			}
			// �ӂ̒��������߂�.
			e_len[i] = sqrt(pt_abs2(p[i]));
			// �ӂ̒��������邩���肷��.
			if (e_len[i] >= FLT_MIN) {
				nz_cnt++;
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
			if (nz_cnt == 0) {
				// �[���Ȃ��, ���肷��ʒu��, �g�����镝�𔼌a�Ƃ���~�Ɋ܂܂�邩���肷��.
				if (pt_in_circle(test, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
				return ANC_TYPE::ANC_PAGE;
			}
			// �ӂ����Ă��邩���肷��.
			if (e_closed) {
				// ���Ă���Ȃ�, �n�_�� { 0, 0 } �Ȃ̂ŏI�_�ւ̃x�N�g����, ���̂܂܍Ō�̕ӂ̒����Ƃ���.
				e_len[p_cnt] = sqrt(pt_abs2(q[p_cnt]));
			}
			// ���ĂȂ��Ȃ�, �[�̌`�����~�`�����肷��.
			else if (equal(s_cap, CAP_ROUND)) {
				if (pt_in_circle(test, e_width) || pt_in_circle(test, q[p_cnt], e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// ���ĂȂ��Ȃ�, �[�̌`���������`�����肷��.
			else if (equal(s_cap, CAP_SQUARE)) {
				if (poly_test_cap_square(test, q[p_cnt], p_cnt, p, e_len, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// ���ĂȂ��Ȃ�, �[�̌`�����O�p�`�����肷��.
			else if (equal(s_cap, CAP_TRIANGLE)) {
				if (poly_test_cap_triangle(test, q[p_cnt], p_cnt, p, e_len, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			D2D1_POINT_2F e[N_GON_MAX][4 + 1];	// �������g�����ꂽ�� (+���_)
			size_t e_cnt = 0;
			for (size_t i = 0; i < p_cnt; i++) {
				// �� i �̒������Ȃ������肷��.
				if (e_len[i] < FLT_MIN) {
					// �_ i ����~����, �����̂���� m ��������.
					size_t m = static_cast<size_t>(-1);
					for (size_t h = i; h > 0; h--) {
						if (e_len[h - 1] >= FLT_MIN) {
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
					else if (e_closed && e_len[p_cnt] >= FLT_MIN) {
						// �Ō�̒��_�̔��΃x�N�g��������, ���O�̕Ӄx�N�g���Ƃ���.
						prev = D2D1_POINT_2F{ -q[p_cnt].x, -q[p_cnt].y };
					}
					else {
						continue;
					}
					// �_ i ���珸����, �����̂���ӂ�������.
					size_t n = static_cast<size_t>(-1);
					for (size_t j = i + 1; j < p_cnt; j++) {
						if (e_len[j] >= FLT_MIN) {
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
					else if (e_closed && e_len[p_cnt] >= FLT_MIN) {
						// �Ō�̒��_�̔��΃x�N�g��������, ����̕Ӄx�N�g���Ƃ���.
						next = D2D1_POINT_2F{ -q[p_cnt].x, -q[p_cnt].y };
					}
					else {
						continue;
					}
					// ���O�ƒ���̕Ӄx�N�g��������, ���ʃx�N�g�������߂�.
					D2D1_POINT_2F resultant;
					pt_add(prev, next, resultant);
					// �����x�N�g���̒������Ȃ������肷��.
					double r_abs = pt_abs2(resultant);
					if (r_abs < FLT_MIN) {
						// ���O�̕Ӄx�N�g���������x�N�g���Ƃ���.
						resultant = prev;
						r_abs = pt_abs2(resultant);
					}
					// �����x�N�g���̒����x�N�g�������߂�.
					// ���x�N�g���Ƃ�������, �g�����镝�Ƃ���.
					pt_mul(resultant, e_width / sqrt(r_abs), resultant);
					const D2D1_POINT_2F orthogonal{ resultant.y, -resultant.x };
					// ���_ i �𒼌��x�N�g���ɉ����Ďl���Ɋg����, �g�����ꂽ�� i �Ɋi�[����.
					const double cx = resultant.x;
					const double cy = resultant.y;
					const double ox = orthogonal.x;
					const double oy = orthogonal.y;
					pt_add(q[i], -cx - ox, -cy - oy, e[e_cnt][0]);
					pt_add(q[i], -cx + ox, -cy + oy, e[e_cnt][1]);
					pt_add(q[i], cx + ox, cy + oy, e[e_cnt][2]);
					pt_add(q[i], cx - ox, cy - oy, e[e_cnt][3]);
					e[e_cnt][4] = q[i];
				}
				else {
					// �Ӄx�N�g���ɒ�������x�N�g�������߂�.
					// �����x�N�g���̒�����, �g�����镝�Ƃ���.
					D2D1_POINT_2F direction;	// �����x�N�g��
					pt_mul(p[i], e_width / e_len[i], direction);
					const D2D1_POINT_2F orthogonal{ direction.y, -direction.x };	// ��������x�N�g��
					// ���_ i �� i+1 �𒼌��x�N�g���ɉ����Đ��t�Ɋg����, �g�����ꂽ�� i �Ɋi�[����.
					pt_sub(q[i], orthogonal, e[e_cnt][0]);
					pt_add(q[i], orthogonal, e[e_cnt][1]);
					pt_add(q[i + 1], orthogonal, e[e_cnt][2]);
					pt_sub(q[i + 1], orthogonal, e[e_cnt][3]);
					e[e_cnt][4] = q[i];
				}
				// ���ׂ�ʒu��, �g�����ꂽ�ӂɊ܂܂�邩���肷��.
				if (pt_in_poly(test, 4, e[e_cnt++])) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// �ӂ����Ă��邩, �����ӂɒ��������邩���肷��.
			if (e_closed && e_len[p_cnt] >= FLT_MIN) {
				// �Ō�̕ӂ̈ʒu�𔽓]����, �g�����镝�̒����ɍ��킹, �Ӄx�N�g�������߂�.
				// �Ӄx�N�g���ɒ�������x�N�g�������߂�.
				// �n�_�ƏI�_�𒼌��x�N�g���ɉ����Đ��t�Ɋg����, �g�����ꂽ�ӂɊi�[����.
				D2D1_POINT_2F direction;
				pt_mul(q[p_cnt], -e_width / e_len[p_cnt], direction);
				const D2D1_POINT_2F orthogonal{ direction.y, -direction.x };
				pt_sub(q[p_cnt], orthogonal, e[e_cnt][0]);
				pt_add(q[p_cnt], orthogonal, e[e_cnt][1]);
				e[e_cnt][2] = orthogonal; // v0 + o_vec
				//pt_neg(o_vec, e_side[e_cnt][3]); // v0 - o_vec
				e[e_cnt][3].x = -orthogonal.x;
				e[e_cnt][3].y = -orthogonal.y;
				e[e_cnt][4] = q[p_cnt];
				// ���肷��ʒu���g�����ꂽ�ӂɊ܂܂�邩���肷��.
				if (pt_in_poly(test, 4, e[e_cnt++])) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				if (poly_test_join_bevel(test, e_cnt, e_closed, e)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			else if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER
				|| s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				if (poly_test_join_miter(test, e_cnt, e_closed, e_width, e, s_limit, s_join)) {
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

	// �̈�����Ƃɑ��p�`���쐬����.
	// start	�̈�̎n�_
	// b_vec	�̈�̏I�_�ւ̍���
	// p_opt	���p�`�̍쐬���@
	// v_pos	���_�̔z�� [v_cnt]
	void ShapePolygon::poly_by_bbox(
		const D2D1_POINT_2F start, const D2D1_POINT_2F b_vec, const POLY_OPTION& p_opt,
		D2D1_POINT_2F v_pos[]) noexcept
	{
		// v_cnt	���p�`�̒��_�̐�
		// v_up	���_����ɍ쐬���邩����
		// v_reg	�����p�`���쐬���邩����
		// v_clock	���v����ō쐬���邩����
		const auto v_cnt = p_opt.m_vertex_cnt;
		if (v_cnt == 0) {
			return;
		}
		const auto v_up = p_opt.m_vertex_up;
		const auto v_reg = p_opt.m_regular;
		const auto v_clock = p_opt.m_clockwise;

		// ���_�𒆐S�Ƃ��锼�a 1 �̉~�����Ƃɐ����p�`���쐬����.
		D2D1_POINT_2F v_lt{ 0.0, 0.0 };	// ���p�`���͂ޗ̈�̍���_
		D2D1_POINT_2F v_rb{ 0.0, 0.0 };	// ���p�`���͂ޗ̈�̉E���_
		const double s = v_up ? (M_PI / 2.0) : (M_PI / 2.0 + M_PI / v_cnt);	// �n�_�̊p�x
		const double pi2 = v_clock ? -2 * M_PI : 2 * M_PI;	// �񂷑S��
		for (uint32_t i = 0; i < v_cnt; i++) {
			const double t = s + pi2 * i / v_cnt;	// i �Ԗڂ̒��_�̊p�x
			v_pos[i].x = static_cast<FLOAT>(cos(t));
			v_pos[i].y = static_cast<FLOAT>(-sin(t));
			if (v_pos[i].x < v_lt.x) {
				v_lt.x = v_pos[i].x;
			}
			if (v_pos[i].y < v_lt.y) {
				v_lt.y = v_pos[i].y;
			}
			if (v_pos[i].x > v_rb.x) {
				v_rb.x = v_pos[i].x;
			}
			if (v_pos[i].y > v_rb.y) {
				v_rb.y = v_pos[i].y;
			}
		}

		// �����p�`��̈�̑傫���ɍ��킹��.
		D2D1_POINT_2F v_vec;
		pt_sub(v_rb, v_lt, v_vec);
		const double rate_x = v_reg ? fmin(b_vec.x, b_vec.y) / fmax(v_vec.x, v_vec.y) : b_vec.x / v_vec.x;
		const double rate_y = v_reg ? rate_x : b_vec.y / v_vec.y;
		v_vec.x = static_cast<FLOAT>(roundl(v_vec.x * rate_x));
		v_vec.y = static_cast<FLOAT>(roundl(v_vec.y * rate_y));
		for (uint32_t i = 0; i < v_cnt; i++) {
			pt_sub(v_pos[i], v_lt, v_pos[i]);
			v_pos[i].x = static_cast<FLOAT>(roundl(v_pos[i].x * rate_x));
			v_pos[i].y = static_cast<FLOAT>(roundl(v_pos[i].y * rate_y));
			pt_add(v_pos[i], start, v_pos[i]);
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
			if (m_vec.size() < 1) {
				return;
			}

			// �J�n�ʒu��, �����̔z������Ƃ�, ���_�����߂�.
			const size_t v_cnt = m_vec.size() + 1;	// ���_�̐� (�����̐� + 1)
			D2D1_POINT_2F v_pos[N_GON_MAX];
			v_pos[0] = m_start;
			for (size_t i = 1; i < v_cnt; i++) {
				pt_add(v_pos[i - 1], m_vec[i - 1], v_pos[i]);
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
			sink->BeginFigure(v_pos[0], f_begin);
			for (size_t i = 1; i < v_cnt; i++) {
				sink->AddLine(v_pos[i]);
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
				if (poly_get_pos_arrow(v_cnt, v_pos, m_arrow_size, tip, barb)) {

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
			D2D1_POINT_2F a_pos{ m_start };	// �}�`�̕��ʂ̈ʒu
			anc_draw_square(a_pos, target, brush);
			const size_t d_cnt = m_vec.size();	// �����̐�
			for (size_t i = 0; i < d_cnt; i++) {
				pt_add(a_pos, m_vec[i], a_pos);
				anc_draw_square(a_pos, target, brush);
			}
		}
	}

	/*
	// �h��Ԃ��F�𓾂�.
	// val	����ꂽ�l
	// �߂�l	����ꂽ�Ȃ� true
	bool ShapePolygon::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}
	*/

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// a_len	�A���J�[�̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapePolygon::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		D2D1_POINT_2F t;
		pt_sub(test, m_start, t);
		return poly_hit_test(
			t, m_vec.size(), m_vec.data(), is_opaque(m_stroke_color), m_stroke_width, m_end_closed,
			m_stroke_cap, m_join_style, m_join_miter_limit, is_opaque(m_fill_color), m_anc_width);
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// a_lt	�͈͂̍���ʒu
	// a_rb	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapePolygon::in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept
	{
		if (!pt_in_rect(m_start, area_lt, area_rb)) {
			return false;
		}
		const size_t d_cnt = m_vec.size();	// �����̐�
		D2D1_POINT_2F e_pos = m_start;
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(e_pos, m_vec[i], e_pos);	// ���̈ʒu
			if (!pt_in_rect(e_pos, area_lt, area_rb)) {
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

	/*
	// �h��Ԃ��F�Ɋi�[����.
	bool ShapePolygon::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_fill_color, val)) {
			m_fill_color = val;
			m_d2d_path_geom = nullptr;
			m_d2d_arrow_geom = nullptr;
			return true;
		}
		return false;
	}
	*/

	// �}�`���쐬����.
	// start	�͂ޗ̈�̎n�_
	// b_vec	�͂ޗ̈�̏I�_�ւ̍���
	// page	�y�[�W
	// p_opt	���p�`�̑I����
	ShapePolygon::ShapePolygon(
		const D2D1_POINT_2F start, const D2D1_POINT_2F b_vec, const Shape* page, const POLY_OPTION& p_opt) :
		ShapePath::ShapePath(page, p_opt.m_end_closed),
		m_end_closed(p_opt.m_end_closed)
	{
		D2D1_POINT_2F v_pos[N_GON_MAX];

		poly_by_bbox(start, b_vec, p_opt, v_pos);
		m_start = v_pos[0];
		m_vec.resize(p_opt.m_vertex_cnt - 1);
		m_vec.shrink_to_fit();
		for (size_t i = 1; i < p_opt.m_vertex_cnt; i++) {
			pt_sub(v_pos[i], v_pos[i - 1], m_vec[i - 1]);
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