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

	// ���p�`�̊p���}�`���_���܂ނ����肷�� (�ʎ��)
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

	// ���p�`�̊p���}�`���_���܂ނ����肷��.
	// test_pt	���肳���_
	// s_cnt	�ӂ̐�
	// e_close	�ӂ����Ă��邩����
	// e_width	�ӂ̑����̔���.
	// s	�ӂ̔z�� [exp_cnt][4+1]
	// miter_limit	���̐�萧��
	// j_style	���̌������@
	static bool poly_test_join_miter(const D2D1_POINT_2F test_pt, const size_t s_cnt, const bool e_close, const double e_width, const D2D1_POINT_2F s[][4 + 1], const double miter_limit, const D2D1_LINE_JOIN j_style) noexcept
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
					if (pt_in_poly(test_pt, 4, quadrilateral)) {
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
				if (pt_in_poly(test_pt, 4, q)) {
					// �ʒu���܂ނȂ� true ��Ԃ�.
					return true;
				}
				continue;
			}
			// ��萧���𒴂���Ȃ��, ���̌��������܂��͖ʎ�肩���肷��.
			if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				// ���̌��������܂��͖ʎ��Ȃ��, ���ׂ�ʒu���O�p�` { q1, q2, q3 } ���܂ނ����肷��.
				const D2D1_POINT_2F* triangle = q + 1;
				if (pt_in_poly(test_pt, 3, triangle)) {
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
			if (pt_in_poly(test_pt, 5, pentagon)) {
				// �ʒu���܂ނȂ� true ��Ԃ�.
				return true;
			}
		}
		return false;
	}

	// ���p�`�̊p���}�`���_���܂ނ����肷�� (�ۂ܂����p)
	// s_cnt	�ӂ̐�
	// s_pt	���_�̔z��
	// e_width	�ӂ̔����̑���
	static bool poly_test_join_round(const D2D1_POINT_2F& test_pt, const size_t s_cnt, const D2D1_POINT_2F s_pt[], const double e_width)
	{
		for (size_t i = 0; i < s_cnt; i++) {
			if (pt_in_circle(test_pt, s_pt[i], e_width)) {
				return true;
			}
		}
		return false;
	}

	// �ʒu��, �����Ɋ܂܂�邩���肷��.
	// t	���肳���_ (�����̎n�_�����_�Ƃ���)
	// p_cnt	�n�_�������ʒu�x�N�g���̐�
	// p	�n�_�������ʒu�x�N�g���̔z��
	// s_opaque	�����s����������
	// s_width	���̑���
	// end_closed	�������Ă��邩����
	// s_join	���̌���
	// miter_limit	��萧��
	// f_opa	�h��Ԃ����s����������
	static uint32_t poly_hit_test(
		const D2D1_POINT_2F test_pt,	// ���肳���_ (�����̎n�_�����_�Ƃ���)
		const size_t to_cnt,	// �n�_�������ʒu�x�N�g���̐�
		const D2D1_POINT_2F to[],	// �n�_�������ʒu�x�N�g���̔z��
		const bool stroke_opa,	// �����s����������
		const double s_width,	// ���̑���
		const bool end_closed,	// �������Ă��邩����
		const D2D1_CAP_STYLE& s_cap,	// ���̒[�̌`��
		const D2D1_LINE_JOIN s_join,	// ���̌����̌`��
		const double miter_limit,	// ��萧��
		const bool fill_opa,	// �h��Ԃ����s����������
		const double a_len
	)
	{
		D2D1_POINT_2F pt[N_GON_MAX]{ { 0.0f, 0.0f }, };	// ���_ (�n�_ { 0,0 } ���܂߂�)
		double side_len[N_GON_MAX];	// �ӂ̒���
		size_t n_cnt = 0;	// �����̂���ӂ̐�
		size_t k = static_cast<size_t>(-1);	// �����������_
		for (size_t i = 0; i < to_cnt; i++) {
			// ���肳���_��, ���_�̕��ʂɊ܂܂�邩���肷��.
			if (hit_text_pt(test_pt, pt[i], a_len)) {
				k = i;
			}
			// �ӂ̒��������߂�.
			side_len[i] = sqrt(pt_abs2(to[i]));
			// �ӂ̒��������邩���肷��.
			if (side_len[i] >= FLT_MIN) {
				n_cnt++;
			}
			// ���_�ɕӃx�N�g��������, ���̒��_�����߂�.
			pt_add(pt[i], to[i], pt[i + 1]);
		}
		// ���肳���_��, �I�_�̕��ʂɊ܂܂�邩���肷��.
		if (hit_text_pt(test_pt, pt[to_cnt], a_len)) {
			k = to_cnt;
		}
		// ���_���������������肷��.
		if (k != -1) {
			return HIT_TYPE::HIT_P0 + static_cast<uint32_t>(k);
		}
		// �����s���������肷��.
		if (stroke_opa) {
			// �s�����Ȃ��, ���̑����̔����̕�������, �g�����镝�Ɋi�[����.
			const auto e_width = max(max(static_cast<double>(s_width), a_len) * 0.5, 0.5);	// �g�����镝
			// �S�Ă̕ӂ̒������[�������肷��.
			if (n_cnt == 0) {
				// �[���Ȃ��, ���肳���_��, �g�����镝�𔼌a�Ƃ���~�Ɋ܂܂�邩���肷��.
				if (pt_in_circle(test_pt.x, test_pt.y, e_width)) {
					return HIT_TYPE::HIT_STROKE;
				}
				return HIT_TYPE::HIT_SHEET;
			}
			// �ӂ����Ă��邩���肷��.
			if (end_closed) {
				// ���Ă���Ȃ�, �n�_�� { 0, 0 } �Ȃ̂ŏI�_�ւ̃x�N�g����, ���̂܂܍Ō�̕ӂ̒����Ƃ���.
				side_len[to_cnt] = sqrt(pt_abs2(pt[to_cnt]));
			}
			// ���ĂȂ��Ȃ�, �[�̌`�����~�`�����肷��.
			else if (equal(s_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND)) {
				if (pt_in_circle(test_pt.x, test_pt.y, e_width) ||
					pt_in_circle(test_pt, pt[to_cnt], e_width)) {
					return HIT_TYPE::HIT_STROKE;
				}
			}
			// ���ĂȂ��Ȃ�, �[�̌`���������`�����肷��.
			else if (equal(s_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE)) {
				if (poly_test_cap_square(test_pt, pt[to_cnt], to_cnt, to, side_len, e_width)) {
					return HIT_TYPE::HIT_STROKE;
				}
			}
			// ���ĂȂ��Ȃ�, �[�̌`�����O�p�`�����肷��.
			else if (equal(s_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE)) {
				if (poly_test_cap_triangle(test_pt, pt[to_cnt], to_cnt, to, side_len, e_width)) {
					return HIT_TYPE::HIT_STROKE;
				}
			}
			D2D1_POINT_2F s[N_GON_MAX][4 + 1]{};	// �������g�����ꂽ�� (+���_)
			size_t s_cnt = 0;
			for (size_t i = 0; i < to_cnt; i++) {
				// �� i �̒������Ȃ������肷��.
				if (side_len[i] < FLT_MIN) {
					// �_ i ����~����, �����̂���� m ��������.
					size_t m = static_cast<size_t>(-1);
					for (size_t h = i; h > 0; h--) {
						if (side_len[h - 1] >= FLT_MIN) {
							m = h - 1;
							break;
						}
					}
					// �~���̕ӂ��������������肷��.
					D2D1_POINT_2F prev;	// ���O�̕Ӄx�N�g��
					if (m != static_cast<size_t>(-1)) {
						// ���������ӂ𒼑O�̕Ӄx�N�g���Ɋi�[����.
						prev = to[m];
					}
					// ������Ȃ������Ȃ��,
					// �ӂ����Ă���, ���Ō�̕ӂ̒�������[�������肷��.
					else if (end_closed && side_len[to_cnt] >= FLT_MIN) {
						// �Ō�̒��_�̔��΃x�N�g��������, ���O�̕Ӄx�N�g���Ƃ���.
						prev = D2D1_POINT_2F{ -pt[to_cnt].x, -pt[to_cnt].y };
					}
					else {
						continue;
					}
					// �_ i ���珸����, �����̂���ӂ�������.
					size_t n = static_cast<size_t>(-1);
					for (size_t j = i + 1; j < to_cnt; j++) {
						if (side_len[j] >= FLT_MIN) {
							n = j;
							break;
						}
					}
					// �����̕ӂ��������������肷��.
					D2D1_POINT_2F next;	// ����̕Ӄx�N�g��
					if (n != -1) {
						// ���������ӂ𒼌�̕Ӄx�N�g���Ɋi�[����.
						next = to[n];
					}
					// ������Ȃ������Ȃ��,
					// �ӂ����Ă���, ���Ō�̕ӂɒ��������邩���肷��.
					else if (end_closed && side_len[to_cnt] >= FLT_MIN) {
						// �Ō�̒��_�̔��΃x�N�g��������, ����̕Ӄx�N�g���Ƃ���.
						next = D2D1_POINT_2F{ -pt[to_cnt].x, -pt[to_cnt].y };
					}
					else {
						continue;
					}
					// ���O�ƒ���̕Ӄx�N�g�����������x�N�g�� (resultant) �����߂�.
					D2D1_POINT_2F res;
					pt_add(prev, next, res);
					// �����x�N�g���̒������Ȃ������肷��.
					double r_abs = pt_abs2(res);
					if (r_abs < FLT_MIN) {
						// ���O�̕Ӄx�N�g���������x�N�g���Ƃ���.
						res = prev;
						r_abs = pt_abs2(res);
					}
					// �����x�N�g���̒����x�N�g�������߂�.
					// ���x�N�g���Ƃ�������, �g�����镝�Ƃ���.
					pt_mul(res, e_width / sqrt(r_abs), res);
					const D2D1_POINT_2F orthogonal{ res.y, -res.x };
					// ���_ i �𒼌��x�N�g���ɉ����Ďl���Ɋg����, �g�����ꂽ�� i �Ɋi�[����.
					const double cx = res.x;
					const double cy = res.y;
					const double ox = orthogonal.x;
					const double oy = orthogonal.y;
					pt_add(pt[i], -cx - ox, -cy - oy, s[s_cnt][0]);
					pt_add(pt[i], -cx + ox, -cy + oy, s[s_cnt][1]);
					pt_add(pt[i], cx + ox, cy + oy, s[s_cnt][2]);
					pt_add(pt[i], cx - ox, cy - oy, s[s_cnt][3]);
					s[s_cnt][4] = pt[i];
				}
				else {
					// �Ӄx�N�g���ɒ�������x�N�g�������߂�.
					// �����x�N�g���̒�����, �g�����镝�Ƃ���.
					D2D1_POINT_2F direction;	// �����x�N�g��
					pt_mul(to[i], e_width / side_len[i], direction);
					const D2D1_POINT_2F orthogonal{ direction.y, -direction.x };	// ��������x�N�g��
					// ���_ i �� i+1 �𒼌��x�N�g���ɉ����Đ��t�Ɋg����, �g�����ꂽ�� i �Ɋi�[����.
					pt_sub(pt[i], orthogonal, s[s_cnt][0]);
					pt_add(pt[i], orthogonal, s[s_cnt][1]);
					pt_add(pt[i + 1], orthogonal, s[s_cnt][2]);
					pt_sub(pt[i + 1], orthogonal, s[s_cnt][3]);
					s[s_cnt][4] = pt[i];
				}
				// ���ׂ�ʒu��, �g�����ꂽ�ӂɊ܂܂�邩���肷��.
				if (pt_in_poly(test_pt, 4, s[s_cnt++])) {
					return HIT_TYPE::HIT_STROKE;
				}
			}
			// �ӂ����Ă��邩, �����ӂɒ��������邩���肷��.
			if (end_closed && side_len[to_cnt] >= FLT_MIN) {
				// �Ō�̕ӂ̈ʒu�𔽓]����, �g�����镝�̒����ɍ��킹, �Ӄx�N�g�������߂�.
				// �Ӄx�N�g���ɒ�������x�N�g�������߂�.
				// �n�_�ƏI�_�𒼌��x�N�g���ɉ����Đ��t�Ɋg����, �g�����ꂽ�ӂɊi�[����.
				D2D1_POINT_2F direction;
				pt_mul(pt[to_cnt], -e_width / side_len[to_cnt], direction);
				const D2D1_POINT_2F orthogonal{ direction.y, -direction.x };
				pt_sub(pt[to_cnt], orthogonal, s[s_cnt][0]);
				pt_add(pt[to_cnt], orthogonal, s[s_cnt][1]);
				s[s_cnt][2] = orthogonal;
				s[s_cnt][3].x = -orthogonal.x;
				s[s_cnt][3].y = -orthogonal.y;
				s[s_cnt][4] = pt[to_cnt];
				// ���肳���_���g�����ꂽ�ӂɊ܂܂�邩���肷��.
				if (pt_in_poly(test_pt, 4, s[s_cnt++])) {
					return HIT_TYPE::HIT_STROKE;
				}
			}
			if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				if (poly_test_join_bevel(test_pt, s_cnt, end_closed, s)) {
					return HIT_TYPE::HIT_STROKE;
				}
			}
			else if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER
				|| s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				if (poly_test_join_miter(test_pt, s_cnt, end_closed, e_width, s, miter_limit, s_join)) {
					return HIT_TYPE::HIT_STROKE;
				}
			}
			else if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				if (poly_test_join_round(test_pt, to_cnt + 1, pt, e_width)) {
					return HIT_TYPE::HIT_STROKE;
				}
			}
		}
		if (fill_opa) {
			if (pt_in_poly(test_pt, to_cnt + 1, pt)) {
				return HIT_TYPE::HIT_FILL;
			}
		}
		return HIT_TYPE::HIT_SHEET;
	}

	// ���̕Ԃ��Ɛ�[�̈ʒu�𓾂�.
	// p_cnt	�܂���̒��_ (�[�_���܂�) �̐�
	// p	���_�̔z��
	// a_size	���̑傫��
	// tip	���̐�[�̈ʒu
	// barb	���̕Ԃ��̈ʒu
	bool ShapePoly::poly_get_pos_arrow(
		const size_t p_cnt, const D2D1_POINT_2F p[], const ARROW_SIZE& a_size,
		D2D1_POINT_2F barb[], D2D1_POINT_2F& tip) noexcept
	{
		double a_offset = a_size.m_offset;	// ���̐�[�̈ʒu
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

			// ��̒�������邵��[�̈ʒu���Z�������肷��.
			if (a_len < a_offset) {
				// ���̍��������邩���肷��.
				if (i > 1) {
					// ��[�̈ʒu���̒��������Z������.
					a_offset -= a_len;
					continue;
				}
				a_offset = a_len;
			}

			// ���̕Ԃ��̈ʒu�����߂�.
			const auto a_end = p[i - 1];		// ��̏I�[
			const auto b_len = a_size.m_length;	// ���̒���
			const auto b_width = a_size.m_width;	// ���̕Ԃ��̕�
			get_barbs(q, a_len, b_width, b_len, barb);
			pt_mul_add(q, 1.0 - a_offset / a_len, a_end, tip);
			pt_add(barb[0], tip, barb[0]);
			pt_add(barb[1], tip, barb[1]);
			return true;
		}
		return false;
	}

	// ��`�����Ƃɑ��p�`���쐬����.
	void ShapePoly::poly_create_by_box(
		const D2D1_POINT_2F start,	// �n�_
		const D2D1_POINT_2F end_to,	// �I�_�̈ʒu�x�N�g��
		const POLY_OPTION& p_opt,	// ���p�`�̍쐬���@
		D2D1_POINT_2F p[]	// �e�_�̔z��
	) noexcept
	{
		// v_cnt	���p�`�̒��_�̐�
		// v_up	���_����ɍ쐬���邩����
		// v_reg	�����p�`���쐬���邩����
		// v_clock	���v����ō쐬���邩����
		const auto p_cnt = p_opt.m_vertex_cnt;
		if (p_cnt < 2) {
			return;
		}
		else if (p_cnt == 2) {
			p[0] = start;
			p[1].x = start.x + end_to.x;
			p[1].y = start.y + end_to.y;
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
		const double px = fabs(end_to.x);
		const double py = fabs(end_to.y);
		const double bw = static_cast<double>(box_rb.x) - static_cast<double>(box_lt.x);
		const double bh = static_cast<double>(box_rb.y) - static_cast<double>(box_lt.y);
		double sx;	// X �����̊g�嗦
		double sy;	// Y �����̊g�嗦
		if (px <= py) {
			sx = px / bw;
			if (p_reg) {
				sy = sx;
			}
			else {
				sy = py / bh;
			}
		}
		else{
			sy = py / bh;
			if (p_reg) {
				sx = sy;
			}
			else {
				sx = px / bw;
			}
		}

		// �ʒu�x�N�g���̐����ɂ����, ���E��`�̂ǂ̒��_����_�ɂ��邩���߂�.
		double bx;	// ��_ X ���W
		double by;	// ��_ Y ���W
		if (end_to.x >= 0.0f && end_to.y >= 0.0f) {
			bx = box_lt.x;
			by = box_lt.y;
		}
		else if (end_to.x < 0.0f && end_to.y >= 0.0f) {
			bx = box_rb.x;
			by = box_lt.y;
		}
		else if (end_to.x < 0.0f && end_to.y <= 0.0f) {
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
	void ShapePoly::draw(void) noexcept
	{
		ID2D1RenderTarget* const target = SHAPE::m_d2d_target;
		ID2D1SolidColorBrush* const brush = SHAPE::m_d2d_color_brush.get();
		ID2D1Factory* factory;
		target->GetFactory(&factory);
		D2D1_POINT_2F p[N_GON_MAX];
		size_t p_cnt = static_cast<size_t>(-1);
		const bool exist_stroke = (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color));
		HRESULT hr = S_OK;

		if (exist_stroke && m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}
		if (exist_stroke && m_arrow_style != ARROW_STYLE::ARROW_NONE && m_d2d_arrow_stroke == nullptr) {
			create_arrow_stroke();
		}
		if ((exist_stroke || is_opaque(m_fill_color)) && m_d2d_path_geom == nullptr) {
			if (p_cnt == static_cast<size_t>(-1)) {
				p_cnt = get_verts(p);
			}
			if (p_cnt != static_cast<size_t>(-1)) {
				// �܂���̃p�X�W�I���g�����쐬����.
				const auto f_begin = is_opaque(m_fill_color) ?
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW;
				const auto f_end = (m_end == D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED ?
					D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED :
					D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
				winrt::com_ptr<ID2D1GeometrySink> sink;
				hr = factory->CreatePathGeometry(m_d2d_path_geom.put());
				if (hr == S_OK) {
					hr = m_d2d_path_geom->Open(sink.put());
				}
				if (hr == S_OK) {
					sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
					sink->BeginFigure(p[0], f_begin);
					for (size_t i = 1; i < p_cnt; i++) {
						sink->AddLine(p[i]);
					}
					sink->EndFigure(f_end);
					hr = sink->Close();
				}
				sink = nullptr;
			}
		}
		if (exist_stroke && m_arrow_style != ARROW_STYLE::ARROW_NONE && m_d2d_arrow_geom == nullptr) {
			if (p_cnt == static_cast<size_t>(-1)) {
				p_cnt = get_verts(p);
			}
			if (p_cnt != static_cast<size_t>(-1)) {
				// ��邵�̈ʒu�����߂�.
				D2D1_POINT_2F tip;
				D2D1_POINT_2F barb[2];
				if (poly_get_pos_arrow(p_cnt, p, m_arrow_size, barb, tip)) {
					winrt::com_ptr<ID2D1GeometrySink> sink;
					// ��邵�̃p�X�W�I���g�����쐬����.
					const auto a_begin = (m_arrow_style == ARROW_STYLE::ARROW_FILLED ?
						D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
						D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
					const auto a_end = (m_arrow_style == ARROW_STYLE::ARROW_FILLED ?
						D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED :
						D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
					if (hr == S_OK) {
						hr = factory->CreatePathGeometry(m_d2d_arrow_geom.put());
					}
					if (hr == S_OK) {
						hr = m_d2d_arrow_geom->Open(sink.put());
					}
					if (hr == S_OK) {
						sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
						sink->BeginFigure(barb[0], a_begin);
						sink->AddLine(tip);
						sink->AddLine(barb[1]);
						sink->EndFigure(a_end);
						hr = sink->Close();
					}
					sink = nullptr;
				}
			}
		}
		if (p_cnt > 2 && is_opaque(m_fill_color)) {
			const auto p_geom = m_d2d_path_geom.get();	// �p�X�̃W�I���g��
			if (p_geom != nullptr) {
				brush->SetColor(m_fill_color);
				target->FillGeometry(p_geom, brush, nullptr);
			}
		}
		if (exist_stroke) {
			const auto p_geom = m_d2d_path_geom.get();	// �p�X�̃W�I���g��
			if (p_geom != nullptr) {
				brush->SetColor(m_stroke_color);
				target->DrawGeometry(p_geom, brush, m_stroke_width, m_d2d_stroke_style.get());
			}
			const auto a_geom = m_d2d_arrow_geom.get();	// ��邵�̃W�I���g��
			if (a_geom != nullptr && m_arrow_style == ARROW_STYLE::ARROW_OPENED) {
				target->DrawGeometry(a_geom, brush, m_stroke_width, m_d2d_arrow_stroke.get());
			}
			else if (a_geom != nullptr && m_arrow_style == ARROW_STYLE::ARROW_FILLED) {
				target->FillGeometry(a_geom, brush, nullptr);
				target->DrawGeometry(a_geom, brush, m_stroke_width, m_d2d_arrow_stroke.get());
			}
		}
		if (m_hit_show && is_selected()) {
			if (p_cnt == static_cast<size_t>(-1)) {
				p_cnt = get_verts(p);
			}
			if (p_cnt != static_cast<size_t>(-1)) {
				// �⏕����`��
				if (m_stroke_width >= SHAPE::m_hit_square_inner) {
					const auto p_geom = m_d2d_path_geom.get();	// �p�X�̃W�I���g��
					brush->SetColor(COLOR_WHITE);
					target->DrawGeometry(p_geom, brush, 2.0f * m_aux_width, nullptr);
					brush->SetColor(COLOR_BLACK);
					target->DrawGeometry(p_geom, brush, m_aux_width, m_aux_style.get());
				}
				// �}�`�̕��ʂ�`��.
				for (size_t i = 0; i < p_cnt; i++) {
					hit_draw_square(p[i], target, brush);
				}
			}
		}
	}

	// �}�`���_���܂ނ����肷��.
	// test_pt	���肳���_
	// �߂�l	�_���܂ޕ���
	uint32_t ShapePoly::hit_test(const D2D1_POINT_2F test_pt, const bool/*ctrl_key*/) const noexcept
	{
		const D2D1_POINT_2F u{ 
			test_pt.x - m_start.x, test_pt.y - m_start.y
		};
		return poly_hit_test(u, m_lineto.size(), m_lineto.data(), is_opaque(m_stroke_color), m_stroke_width, m_end == D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED, m_stroke_cap, m_stroke_join, m_stroke_join_limit, is_opaque(m_fill_color), m_hit_width);
	}

	// ��`�Ɋ܂܂�邩���肷��.
	// lt	��`�̍���ʒu
	// rb	��`�̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapePoly::is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		if (!pt_in_rect(m_start, lt, rb)) {
			return false;
		}
		const size_t p_cnt = m_lineto.size();	// �����̐�
		D2D1_POINT_2F p{ m_start };
		for (size_t i = 0; i < p_cnt; i++) {
			p.x += m_lineto[i].x;
			p.y += m_lineto[i].y;
			if (!pt_in_rect(p, lt, rb)) {
				return false;
			}
		}
		return true;
	}

	bool ShapePoly::set_arrow_style(const ARROW_STYLE val) noexcept
	{
		if (m_end == D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN) {
			return SHAPE_PATH::set_arrow_style(val);
		}
		return false;
	}

	// �}�`���쐬����.
	ShapePoly::ShapePoly(
		const D2D1_POINT_2F start,	// ��`�̎n�_
		const D2D1_POINT_2F end_to,	// ��`�̏I�_�ւ̈ʒu�x�N�g��
		const SHAPE* prop,	// ����
		const POLY_OPTION& p_opt	// �쐬���@
	) :
		SHAPE_PATH::SHAPE_PATH(prop, p_opt.m_end_closed),
		m_end(p_opt.m_end_closed ? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN)
	{
		D2D1_POINT_2F p[N_GON_MAX];
		poly_create_by_box(start, end_to, p_opt, p);

		m_start = p[0];
		m_lineto.resize(p_opt.m_vertex_cnt - 1);
		m_lineto.shrink_to_fit();
		for (size_t i = 1; i < p_opt.m_vertex_cnt; i++) {
			m_lineto[i - 1].x = p[i].x - p[i - 1].x;
			m_lineto[i - 1].y = p[i].y - p[i - 1].y;
		}
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	// dt_reader	�f�[�^���[�_�[
	ShapePoly::ShapePoly(DataReader const& dt_reader) :
		SHAPE_PATH::SHAPE_PATH(dt_reader)
	{
		const auto end = static_cast<D2D1_FIGURE_END>(dt_reader.ReadUInt32());
		if (end == D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED || end == D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN) {
			m_end = end;
		}
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapePoly::write(DataWriter const& dt_writer) const
	{
		SHAPE_PATH::write(dt_writer);
		dt_writer.WriteUInt32(m_end);
	}

}