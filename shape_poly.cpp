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
	static bool stroke_find_intersection(const D2D1_POINT_2F a, const D2D1_POINT_2F b, const D2D1_POINT_2F c, const D2D1_POINT_2F d, double& s, double& t, D2D1_POINT_2F& e)
	{
		const double ax = a.x;
		const double ay = a.y;
		//const double bx = b.x;
		//const double by = b.y;
		const double cx = c.x;
		const double cy = c.y;
		//const double dx = d.x;
		//const double dy = d.y;
		const double ab_x = static_cast<double>(b.x) - ax;
		const double ab_y = static_cast<double>(b.y) - ay;
		const double cd_x = static_cast<double>(d.x) - cx;
		const double cd_y = static_cast<double>(d.y) - cy;
		const double ac_x = cx - ax;
		const double ac_y = cy - ay;
		const double r = ab_x * cd_y - ab_y * cd_x;
		if (fabs(r) <= FLT_MIN) {
			return false;
		}
		const double sr = cd_y * ac_x - cd_x * ac_y;
		if (fabs(sr) <= FLT_MIN) {
			return false;
		}
		const double tr = ab_y * ac_x - ab_x * ac_y;
		if (fabs(tr) <= FLT_MIN) {
			return false;
		}
		s = sr / r;
		t = tr / r;
		e.x = static_cast<FLOAT>(ax + ab_x * s);
		e.y = static_cast<FLOAT>(ay + ab_y * s);
		return true;
	}

	// �ʒu��, �����̒[�_ (�~�`) �Ɋ܂܂�邩���肷��.
	// t_pos	�����̎n�_�����_�Ƃ���, ���肷��ʒu.
	// v_end	�����̏I�_
	// �߂�l	�܂܂��Ȃ� true
	static bool stroke_test_cap_round(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F v_end, const double e_width)
	{
		return pt_in_circle(t_pos, e_width) || pt_in_circle(t_pos, v_end, e_width);
		// ���ׂ�ʒu��, �ŏ��̒��_�𒆐S�Ƃ�, �g�����镝�𔼌a�Ƃ���, �~�Ɋ܂܂�邩���肷��.
		//if (pt_abs2(t_pos) <= e_width * e_width) {
		//	return true;
		//}
		// ���ׂ�ʒu��, �Ō�̒��_�𒆐S�Ƃ�, �g�����镝�𔼌a�Ƃ���, �~�Ɋ܂܂�邩���肷��.
		//D2D1_POINT_2F u_pos;
		//pt_sub(t_pos, v_end, u_pos);
		//return pt_abs2(u_pos) <= e_width * e_width;
	}

	static bool stroke_test_cap_square(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F v_end, const size_t d_cnt, const D2D1_POINT_2F diff[], const double s_len[], const double e_width)
	{
		for (size_t i = 0; i < d_cnt; i++) {
			if (s_len[i] > FLT_MIN) {
				D2D1_POINT_2F d_vec;
				pt_mul(diff[i], -e_width / s_len[i], d_vec);
				const D2D1_POINT_2F o_vec{ d_vec.y, -d_vec.x };
				D2D1_POINT_2F q_pos[4];
				pt_add(d_vec, o_vec, q_pos[0]);
				pt_sub(d_vec, o_vec, q_pos[1]);
				pt_neg(o_vec, q_pos[2]);
				q_pos[3] = o_vec;
				if (pt_in_poly(t_pos, 4, q_pos)) {
					return true;
				}
				break;
			}
		}
		D2D1_POINT_2F u_pos;
		pt_sub(t_pos, v_end, u_pos);
		for (size_t i = d_cnt; i > 0; i--) {
			if (s_len[i - 1] > FLT_MIN) {
				D2D1_POINT_2F d_vec;
				pt_mul(diff[i - 1], e_width / s_len[i - 1], d_vec);
				const D2D1_POINT_2F o_vec{ d_vec.y, -d_vec.x };
				D2D1_POINT_2F q_pos[4];
				pt_add(d_vec, o_vec, q_pos[0]);
				pt_sub(d_vec, o_vec, q_pos[1]);
				pt_neg(o_vec, q_pos[2]);
				q_pos[3] = o_vec;
				if (pt_in_poly(u_pos, 4, q_pos)) {
					return true;
				}
				break;
			}
		}
		return false;
	}

	static bool stroke_test_cap_triangle(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F v_end, const size_t d_cnt, const D2D1_POINT_2F diff[], const double s_len[], const double e_width)
	{
		for (size_t i = 0; i < d_cnt; i++) {
			if (s_len[i] > FLT_MIN) {
				D2D1_POINT_2F d_vec;
				pt_mul(diff[i], -e_width / s_len[i], d_vec);
				const D2D1_POINT_2F o_vec{ d_vec.y, -d_vec.x };
				D2D1_POINT_2F tri_pos[3];
				tri_pos[0] = d_vec;
				pt_neg(o_vec, tri_pos[1]);
				tri_pos[2] = o_vec;
				if (pt_in_poly(t_pos, 3, tri_pos)) {
					return true;
				}
				break;
			}
		}
		D2D1_POINT_2F u_pos;
		pt_sub(t_pos, v_end, u_pos);
		for (size_t i = d_cnt; i > 0; i--) {
			if (s_len[i - 1] > FLT_MIN) {
				D2D1_POINT_2F d_vec;
				pt_mul(diff[i - 1], e_width / s_len[i - 1], d_vec);
				const D2D1_POINT_2F o_vec{ d_vec.y, -d_vec.x };
				D2D1_POINT_2F tri_pos[3];
				tri_pos[0] = d_vec;
				pt_neg(o_vec, tri_pos[1]);
				tri_pos[2] = o_vec;
				if (pt_in_poly(u_pos, 3, tri_pos)) {
					return true;
				}
				break;
			}
		}
		return false;
	}

	// ���p�`�̊p���ʒu���܂ނ����肷�� (�ʎ��)
	static bool stroke_test_join_bevel(
		const D2D1_POINT_2F t_pos,
		const size_t v_cnt,
		const bool v_close,
		const D2D1_POINT_2F e_side[][4 + 1]) noexcept
	{
		// { 0, 1 }, { 1, 2 }, { v_cnt-2, v_cnt-1 }
		for (size_t i = 1; i < v_cnt; i++) {
			//for (size_t i = (v_close ? v_cnt - 1 : 0), j = (v_close ? 0 : 1); j < v_cnt; i = j++) {
				//const D2D1_POINT_2F bev_pos[4]{ e_side[i][3], e_side[j][0], e_side[j][1], e_side[i][2] };
			const D2D1_POINT_2F bev_pos[4]{ e_side[i - 1][3], e_side[i][0], e_side[i][1], e_side[i - 1][2] };
			if (pt_in_poly(t_pos, 4, bev_pos)) {
				return true;
			}
		}
		// { v_cnt-1, 0 }
		if (v_close) {
			const D2D1_POINT_2F bev_pos[4]{ e_side[v_cnt - 1][3], e_side[0][0], e_side[0][1], e_side[v_cnt - 1][2] };
		}
		return false;
	}

	// ���p�`�̊p���ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// exp_cnt	�g�������ӂ̐�
	// exp_close	�g�������ӂ����Ă��邩����
	// exp_width	�ӂ̑����̔���.
	// exp_side	�g�������ӂ̔z�� [exp_cnt][4+1]
	// mit_limit	���̃}�C�^�[����
	// s_join	���̂Ȃ�����@
	static bool stroke_test_join_miter(
		const D2D1_POINT_2F t_pos,
		const size_t exp_cnt,
		const bool exp_close,
		const double exp_width,
		const D2D1_POINT_2F exp_side[][4 + 1],
		const double s_limit,
		const D2D1_LINE_JOIN s_join) noexcept
	{
		for (size_t i = (exp_close ? 0 : 1), j = (exp_close ? exp_cnt - 1 : 0); i < exp_cnt; j = i++) {
			// �g�����ꂽ�ӂɂ��Ċp�̕��������߂�.
			//
			// �_ vi �łȂ���, �g�����ꂽ�� j �� i �̃C���[�W
			// j0                     j3      i0                     i3
			//  +---expanded side[j]-->+  vi   +---expanded side[i]-->+ 
			// j1                     j2      i1                     i2
			//
			// �g�����ꂽ�� j �� i �����s�����肷��.
			if (equal(exp_side[j][3], exp_side[i][0])) {
				// ���s�Ȃ�Ώd�Ȃ镔���͂Ȃ��̂�, ���̕ӂ�����.
				continue;
			}
			// �g�����ꂽ�� j �� i ���d�Ȃ邩���肷��.
			if (equal(exp_side[j][3], exp_side[i][1])) {
				// ���̂Ȃ��肪�}�C�^�[�����肷��.
				if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER) {
					// �}�C�^�[�Ȃ��, �� j ���}�C�^�[�����̒����������������l�ӌ`�����߂�.
					D2D1_POINT_2F d_vec;
					pt_sub(exp_side[j][3], exp_side[j][0], d_vec);
					pt_mul(d_vec, exp_width * s_limit / sqrt(pt_abs2(d_vec)), d_vec);
					//D2D1_POINT_2F c_pos[4];
					D2D1_POINT_2F q_pos[4];
					q_pos[0] = exp_side[j][3];
					q_pos[1] = exp_side[j][2];
					pt_add(exp_side[j][2], d_vec, q_pos[2]);
					pt_add(exp_side[j][3], d_vec, q_pos[3]);
					// ���ׂ�ʒu���l�ӌ`�Ɋ܂܂�邩���肷��.
					if (pt_in_poly(t_pos, 4, q_pos)) {
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
			D2D1_POINT_2F q_pos[4 + 1];	// �l�ӌ` (�}�C�^�[�����𒴂���Ȃ�Ό܊p�`)
			if (!stroke_find_intersection(exp_side[j][0], exp_side[j][3], exp_side[i][0], exp_side[i][3], s, t, q_pos[0])) {
				continue;
			}
			if (s < 1.0 || t > 0.0) {
				if (!stroke_find_intersection(exp_side[j][1], exp_side[j][2], exp_side[i][1], exp_side[i][2], s, t, q_pos[0])) {
					continue;
				}
				if (s < 1.0 || t > 0.0) {
					continue;
				}
				q_pos[1] = exp_side[j][2];
				q_pos[2] = exp_side[i][4];
				q_pos[3] = exp_side[i][1];
			}
			else {
				q_pos[1] = exp_side[i][0];
				q_pos[2] = exp_side[i][4];
				q_pos[3] = exp_side[j][3];
			}

			// ���_�ƌ�_�Ƃ̍����x�N�g���Ƃ��̒��������߂�.
			// �����x�N�g���̒�����, �}�C�^�[�����ȉ������肷��.
			D2D1_POINT_2F d_vec;	// �����x�N�g��
			pt_sub(q_pos[0], exp_side[i][4], d_vec);
			const double d_abs2 = pt_abs2(d_vec);	// �����x�N�g���̒���
			const double limit_len = exp_width * s_limit;
			if (d_abs2 <= limit_len * limit_len) {
				// �}�C�^�[�����ȉ��Ȃ��, ���ׂ�ʒu���l�ӌ` { q0, q1, q2, q3 } ���܂ނ����肷��.
				if (pt_in_poly(t_pos, 4, q_pos)) {
					// �ʒu���܂ނȂ� true ��Ԃ�.
					return true;
				}
				continue;
			}
			// �}�C�^�[�����𒴂���Ȃ��, ���̂Ȃ��肪�}�C�^�[�܂��͖ʎ�肩���肷��.
			if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				// ���̂Ȃ��肪�}�C�^�[�܂��͖ʎ��Ȃ��, ���ׂ�ʒu���O�p�` { q1, q2, q3 } ���܂ނ����肷��.
				const D2D1_POINT_2F* tri_pos = q_pos + 1;
				if (pt_in_poly(t_pos, 3, tri_pos)) {
					// �ʒu���܂ނȂ� true ��Ԃ�.
					return true;
				}
				continue;
			}
			// �����x�N�g����ɂ�����, ��_�Ƃ̋��������傤�ǃ}�C�^�[�����̒����ɂȂ�_ m ��,
			// �����x�N�g���ƒ��s����x�N�g����ɂ�����, �_ m ��ʂ钼����̓_ n �����߂�.
			// ���� q3 q0 �� m n �Ƃ̌�_������, q4 �Ɋi�[����.
			// ���� q0 q1 �� m n �Ƃ̌�_������, �����V���� q0 �Ƃ���.
			// ���ׂ�ʒu���܊p�` { q0, q1, q2, q3, q4 } ���܂ނ����肷��.
			D2D1_POINT_2F mit_pos;
			pt_mul(d_vec, limit_len / sqrt(d_abs2), exp_side[i][4], mit_pos);
			D2D1_POINT_2F nor_pos;
			pt_add(mit_pos, D2D1_POINT_2F{ d_vec.y, -d_vec.x }, nor_pos);
			stroke_find_intersection(q_pos[3], q_pos[0], mit_pos, nor_pos, s, t, q_pos[4]);
			stroke_find_intersection(q_pos[0], q_pos[1], mit_pos, nor_pos, s, t, q_pos[0]);
			const D2D1_POINT_2F* pen_pos = q_pos;
			if (pt_in_poly(t_pos, 5, pen_pos)) {
				// �ʒu���܂ނȂ� true ��Ԃ�.
				return true;
			}
		}
		return false;
	}

	// ���p�`�̊p���ʒu���܂ނ����肷�� (�ۂ܂����p)
	static bool stroke_test_join_round(const D2D1_POINT_2F& t_pos, const size_t v_cnt, const D2D1_POINT_2F v_pos[], const double exp_width)
	{
		for (size_t i = 0; i < v_cnt; i++) {
			if (pt_in_circle(t_pos, v_pos[i], exp_width)) {
				return true;
			}
			//D2D1_POINT_2F d_vec;
			//pt_sub(t_pos, v_pos[i], d_vec);
			//if (pt_abs2(d_vec) <= exp_width * exp_width) {
			//	return true;
			//}
		}
		return false;
	}

	// �ʒu��, �����Ɋ܂܂�邩���肷��.
	// t_pos	���肷��ʒu (�����̎n�_�����_�Ƃ���)
	// d_cnt	�Ӄx�N�g���̐�
	// diff	�Ӄx�N�g�� (���_�̊Ԃ̍���)
	// a_len	�}�`�̕��ʂ̑傫��
	// s_opa	�����s����������
	// s_width	���̑���
	// s_closed	�������Ă��邩����
	// s_join	���̂Ȃ���
	// s_limit	�}�C�^�[����
	// f_opa	�h��Ԃ����s����������
	static uint32_t stroke_hit_test(
		const D2D1_POINT_2F t_pos,
		const size_t d_cnt,
		const D2D1_POINT_2F diff[],
		const double a_len,
		const bool s_opa,
		const double s_width,
		const bool s_closed,
		const D2D1_CAP_STYLE s_cap,
		const D2D1_LINE_JOIN s_join,
		const double s_limit,
		const bool f_opa)
	{
		// �n�_�� { 0, 0 }
		D2D1_POINT_2F v_pos[N_GON_MAX]{ { 0.0f, 0.0f }, };	// ���_�̈ʒu
		// �e���_�̈ʒu�ƕӂ̒��������߂�.
		double s_len[N_GON_MAX];	// �ӂ̒���
		size_t nz_cnt = 0;	// �����̂���ӂ̐�
		size_t k = static_cast<size_t>(-1);	// �����������_
		for (size_t i = 0; i < d_cnt; i++) {
			// ���肷��ʒu��, ���_�̕��ʂɊ܂܂�邩���肷��.
			if (a_len > 0 && pt_in_anch(t_pos, v_pos[i], a_len)) {
				k = i;
			}
			// �ӂ̒��������߂�.
			s_len[i] = sqrt(pt_abs2(diff[i]));
			// �ӂ̒��������邩���肷��.
			if (s_len[i] > FLT_MIN) {
				nz_cnt++;
			}
			// ���_�ɕӃx�N�g��������, ���̒��_�����߂�.
			pt_add(v_pos[i], diff[i], v_pos[i + 1]);
		}
		// ���肷��ʒu��, �I�_�̕��ʂɊ܂܂�邩���肷��.
		if (pt_in_anch(t_pos, v_pos[d_cnt], a_len)) {
			k = d_cnt;
		}
		// ���_���������������肷��.
		if (k != -1) {
			return ANCH_TYPE::ANCH_P0 + static_cast<uint32_t>(k);
		}
		// �����s���������肷��.
		if (s_opa) {
			// �s�����Ȃ��, ���̑����̔����̕�������, �g�����镝�Ɋi�[����.
			const auto e_width = max(max(static_cast<double>(s_width), a_len) * 0.5, 0.5);	// �g�����镝
			// �S�Ă̕ӂ̒������[�������肷��.
			if (nz_cnt == 0) {
				// �[���Ȃ��, ���肷��ʒu��, �g�����镝�𔼌a�Ƃ���~�Ɋ܂܂�邩���肷��.
				if (pt_in_circle(t_pos, e_width)) {
				//if (pt_abs2(t_pos) <= e_width * e_width) {
					return ANCH_TYPE::ANCH_STROKE;
				}
				return ANCH_TYPE::ANCH_SHEET;
			}
			// �ӂ����Ă��邩���肷��.
			if (s_closed) {
				// ���Ă���Ȃ�, �n�_�� { 0, 0 } �Ȃ̂ŏI�_�ւ̃x�N�g����, ���̂܂܍Ō�̕ӂ̒����Ƃ���.
				s_len[d_cnt] = sqrt(pt_abs2(v_pos[d_cnt]));
			}
			// ���ĂȂ��Ȃ�, ���̒[�_���~�`�����肷��.
			else if (s_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
				if (stroke_test_cap_round(t_pos, v_pos[d_cnt], e_width)) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
			// ���ĂȂ��Ȃ�, ���̒[�_�������`�����肷��.
			else if (s_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
				if (stroke_test_cap_square(t_pos, v_pos[d_cnt], d_cnt, diff, s_len, e_width)) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
			// ���ĂȂ��Ȃ�, ���̒[�_���O�p�`�����肷��.
			else if (s_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
				if (stroke_test_cap_triangle(t_pos, v_pos[d_cnt], d_cnt, diff, s_len, e_width)) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
			D2D1_POINT_2F e_side[N_GON_MAX][4 + 1];	// �g�����ꂽ�� (+���_)
			size_t e_cnt = 0;
			for (size_t i = 0; i < d_cnt; i++) {
				// �� i �̒������Ȃ������肷��.
				if (s_len[i] <= FLT_MIN) {
					// �_ i ����~����, �����̂���� p ��������.
					size_t p = static_cast<size_t>(-1);
					for (size_t h = i; h > 0; h--) {
						if (s_len[h - 1] > FLT_MIN) {
							p = h - 1;
							break;
						}
					}
					// �� p ���������������肷��.
					D2D1_POINT_2F p_diff;	// ���O�̕Ӄx�N�g��
					if (p != static_cast<size_t>(-1)) {
						p_diff = diff[p];
					}
					// ������Ȃ������Ȃ��,
					// �ӂ����Ă���, ���Ō�̕ӂɒ��������邩���肷��.
					else if (s_closed && s_len[d_cnt] > FLT_MIN) {
						// �Ō�̒��_�̔��΃x�N�g��������, ���O�̕Ӄx�N�g���Ƃ���.
						p_diff = D2D1_POINT_2F{ -v_pos[d_cnt].x, -v_pos[d_cnt].y };
					}
					else {
						continue;
					}
					// �_ i ���珸����, �����̂���� n ��������.
					size_t n = static_cast<size_t>(-1);
					for (size_t j = i + 1; j < d_cnt; j++) {
						if (s_len[j] > FLT_MIN) {
							n = j;
							break;
						}
					}
					// �� n ���������������肷��.
					D2D1_POINT_2F n_diff;	// ����̕Ӄx�N�g��
					if (n != -1) {
						n_diff = diff[n];
					}
					// ������Ȃ������Ȃ��,
					// �ӂ����Ă���, ���Ō�̕ӂɒ��������邩���肷��.
					else if (s_closed && s_len[d_cnt] > FLT_MIN) {
						// �Ō�̒��_�̔��΃x�N�g��������, ����̕Ӄx�N�g���Ƃ���.
						n_diff = D2D1_POINT_2F{ -v_pos[d_cnt].x, -v_pos[d_cnt].y };
					}
					else {
						continue;
					}
					// ���O�ƒ���̕Ӄx�N�g��������, �����x�N�g�� c �����߂�.
					D2D1_POINT_2F c_vec;
					pt_add(p_diff, n_diff, c_vec);
					// �����x�N�g���̒������Ȃ������肷��.
					double c_abs = pt_abs2(c_vec);
					if (c_abs <= FLT_MIN) {
						// ���O�̕Ӄx�N�g���������x�N�g���Ƃ���.
						c_vec = p_diff;
						c_abs = pt_abs2(c_vec);
					}
					// �����x�N�g���ɒ�������x�N�g�������߂�.
					// ���x�N�g���Ƃ�������, �g�����镝�Ƃ���.
					pt_mul(c_vec, e_width / sqrt(c_abs), c_vec);
					const D2D1_POINT_2F o_vec{ c_vec.y, -c_vec.x };
					// ���_ i �𒼌��x�N�g���ɉ����Ďl���Ɋg����, �g�����ꂽ�� i �Ɋi�[����.
					const double cx = c_vec.x;
					const double cy = c_vec.y;
					const double ox = o_vec.x;
					const double oy = o_vec.y;
					pt_add(v_pos[i], -cx - ox, -cy - oy, e_side[e_cnt][0]);
					pt_add(v_pos[i], -cx + ox, -cy + oy, e_side[e_cnt][1]);
					pt_add(v_pos[i], cx + ox, cy + oy, e_side[e_cnt][2]);
					pt_add(v_pos[i], cx - ox, cy - oy, e_side[e_cnt][3]);
					e_side[e_cnt][4] = v_pos[i];
				}
				else {
					// �Ӄx�N�g���ɒ�������x�N�g�������߂�.
					// �����x�N�g���̒�����, �g�����镝�Ƃ���.
					D2D1_POINT_2F d_vec;
					pt_mul(diff[i], e_width / s_len[i], d_vec);
					const D2D1_POINT_2F o_vec{ d_vec.y, -d_vec.x };
					// ���_ i �� i+1 �𒼌��x�N�g���ɉ����Đ��t�Ɋg����, �g�����ꂽ�� i �Ɋi�[����.
					pt_sub(v_pos[i], o_vec, e_side[e_cnt][0]);
					pt_add(v_pos[i], o_vec, e_side[e_cnt][1]);
					pt_add(v_pos[i + 1], o_vec, e_side[e_cnt][2]);
					pt_sub(v_pos[i + 1], o_vec, e_side[e_cnt][3]);
					e_side[e_cnt][4] = v_pos[i];
				}
				// ���ׂ�ʒu��, �g�����ꂽ�� i �Ɋ܂܂�邩���肷��.
				if (pt_in_poly(t_pos, 4, e_side[e_cnt++])) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
			// �ӂ����Ă��邩���肷��.
			if (s_closed) {
				// �Ō�̕ӂ̒��������邩���肷��.
				if (s_len[d_cnt] > FLT_MIN) {
					// �Ō�̕ӂ̈ʒu�𔽓]����, �g�����镝�̒����ɍ��킹, �Ӄx�N�g�������߂�.
					// �Ӄx�N�g���ɒ�������x�N�g�������߂�.
					// �n�_�ƏI�_�𒼌��x�N�g���ɉ����Đ��t�Ɋg����, �g�����ꂽ�� i �Ɋi�[����.
					D2D1_POINT_2F d_vec;
					pt_mul(v_pos[d_cnt], -e_width / s_len[d_cnt], d_vec);
					const D2D1_POINT_2F o_vec{ d_vec.y, -d_vec.x };
					pt_sub(v_pos[d_cnt], o_vec, e_side[e_cnt][0]);
					pt_add(v_pos[d_cnt], o_vec, e_side[e_cnt][1]);
					e_side[e_cnt][2] = o_vec; // v0 + o_vec
					pt_neg(o_vec, e_side[e_cnt][3]); // v0 - o_vec
					e_side[e_cnt][4] = v_pos[d_cnt];
					if (pt_in_poly(t_pos, 4, e_side[e_cnt++])) {
						return ANCH_TYPE::ANCH_STROKE;
					}
				}
			}
			if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				if (stroke_test_join_bevel(t_pos, e_cnt, s_closed, e_side)) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
			else if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER
				|| s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				if (stroke_test_join_miter(t_pos, e_cnt, s_closed, e_width, e_side, s_limit, s_join)) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
			else if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				if (stroke_test_join_round(t_pos, d_cnt + 1, v_pos, e_width)) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
		}
		if (f_opa) {
			if (pt_in_poly(t_pos, d_cnt + 1, v_pos)) {
				return ANCH_TYPE::ANCH_FILL;
			}
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// ���s����x�N�g���𓾂�.
	//static D2D1_POINT_2F poly_pt_orth(const D2D1_POINT_2F vec) { return { -vec.y, vec.x }; }

	static bool poly_get_arrow_barbs(const size_t v_cnt, const D2D1_POINT_2F v_pos[], const ARROWHEAD_SIZE& a_size, D2D1_POINT_2F& h_tip, D2D1_POINT_2F h_barbs[]) noexcept
	{
		double b_offset = a_size.m_offset;	// ����[�̃I�t�Z�b�g
		for (size_t i = v_cnt - 1; i > 0; i--) {
			D2D1_POINT_2F a_vec;
			pt_sub(v_pos[i], v_pos[i - 1], a_vec);
			// ���������Ƃ��̒��������߂�.
			// ��̒������قڃ[�������肷��.
			//const auto a_vec = m_diff[i - 1];	// ��̃x�N�g��
			const auto a_len = sqrt(pt_abs2(a_vec));	// ��̒���
			if (a_len < FLT_MIN) {
				continue;
			}

			// ��̒���������[�̃I�t�Z�b�g���Z�������肷��.
			if (a_len < b_offset) {
				// ���̍��������邩���肷��.
				if (i > 1) {
					// �I�t�Z�b�g���̒��������Z������.
					b_offset -= a_len;
					continue;
				}
				b_offset = a_len;
			}

			// ���̕Ԃ��̈ʒu�����߂�.
			const auto a_end = v_pos[i - 1];		// ��̏I�[
			const auto b_len = a_size.m_length;	// ���̒���
			const auto b_width = a_size.m_width;	// ���̕�
			get_arrow_barbs(a_vec, a_len, b_width, b_len, h_barbs);
			pt_mul(a_vec, 1.0 - b_offset / a_len, a_end, h_tip);
			pt_add(h_barbs[0], h_tip, h_barbs[0]);
			pt_add(h_barbs[1], h_tip, h_barbs[1]);
			return true;
		}
		return false;
	}

	// ���p�`�̊e�ӂ̖@���x�N�g�������߂�.
	// v_cnt	���_�̐�
	// v_pos	���_�̔z�� [v_cnt]
	// n_vec	�e�ӂ̖@���x�N�g���̔z�� [v_cnt]
	// �߂�l	�@���x�N�g�������߂��Ȃ� true, ���ׂĂ̒��_���d�Ȃ��Ă����Ȃ� false
	/*
	static bool poly_get_nvec(const size_t v_cnt, const D2D1_POINT_2F v_pos[], D2D1_POINT_2F n_vec[]) noexcept
	{
		// ���p�`�̊e�ӂ̒����Ɩ@���x�N�g��, 
		// �d�����Ȃ����_�̐������߂�.
		//std::vector<double> side_len(v_cnt);	// �e�ӂ̒���
		//const auto s_len = reinterpret_cast<double*>(side_len.data());
		double s_len[N_GON_MAX];	// �e�ӂ̒���
		int q_cnt = 1;
		for (size_t i = 0; i < v_cnt; i++) {
			// ���̒��_�Ƃ̍��������߂�.
			D2D1_POINT_2F q_sub;
			pt_sub(v_pos[(i + 1) % v_cnt], v_pos[i], q_sub);
			// �����̒��������߂�.
			s_len[i] = sqrt(pt_abs2(q_sub));
			if (s_len[i] > FLT_MIN) {
				// �����̒����� 0 ���傫���Ȃ�, 
				// �d�����Ȃ����_�̐����C���N�������g����.
				q_cnt++;
			}
			// �����ƒ��s����x�N�g���𐳋K�����Ė@���x�N�g���Ɋi�[����.
			pt_mul(poly_pt_orth(q_sub), 1.0 / s_len[i], n_vec[i]);
		}
		if (q_cnt == 1) {
			// ���ׂĂ̒��_���d�Ȃ����Ȃ�, false ��Ԃ�.
			return false;
		}
		for (size_t i = 0; i < v_cnt; i++) {
			if (s_len[i] <= FLT_MIN) {
				// �ӂ̒������ق� 0 �Ȃ��, �אڂ���O��̕ӂ̒�����
				// ������ 0 �łȂ��ӂ�T��, �����̖@���x�N�g����������, 
				// ���� 0 �̕ӂ̖@���x�N�g���Ƃ���.
				size_t prev;
				for (size_t j = 1; s_len[prev = ((i - j) % v_cnt)] <= FLT_MIN; j++);
				size_t next;
				for (size_t j = 1; s_len[next = ((i + j) % v_cnt)] <= FLT_MIN; j++);
				pt_add(n_vec[prev], n_vec[next], n_vec[i]);
				auto len = sqrt(pt_abs2(n_vec[i]));
				if (len > FLT_MIN) {
					pt_mul(n_vec[i], 1.0 / len, n_vec[i]);
				}
				else {
					// �����x�N�g�����[���x�N�g���ɂȂ�Ȃ�,
					// �O���̗אڂ���ӂ̖@���x�N�g���ɒ�������x�N�g����@���x�N�g���Ƃ���.
					n_vec[i] = poly_pt_orth(n_vec[prev]);
				}
			}
		}
		return true;
	}
	*/

	// �̈�����Ƃɑ��p�`���쐬����.
	// b_pos	�̈�̎n�_
	// b_diff	�̈�̏I�_�ւ̍���
	// v_cnt	���p�`�̒��_�̐�
	// v_up	���_����ɍ쐬���邩����
	// v_reg	�����p�`���쐬���邩����
	// v_clock	���v����ō쐬���邩����
	// v_pos	���_�̔z�� [v_cnt]
	void ShapePoly::create_poly_by_bbox(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const size_t v_cnt, const bool v_up, const bool v_reg, const bool v_clock, D2D1_POINT_2F v_pos[]) noexcept
	{
		if (v_cnt == 0) {
			return;
		}

		// ���_�𒆐S�Ƃ��锼�a 1 �̉~�����Ƃɐ����p�`���쐬����.
		D2D1_POINT_2F v_min{ 0.0, 0.0 };	// ���p�`���͂ޗ̈�̍���_
		D2D1_POINT_2F v_max{ 0.0, 0.0 };	// ���p�`���͂ޗ̈�̉E���_
		const double s = v_up ? (M_PI / 2.0) : (M_PI / 2.0 + M_PI / v_cnt);	// �n�_�̊p�x
		const double pi2 = v_clock ? -2 * M_PI : 2 * M_PI;	// �񂷑S��
		for (uint32_t i = 0; i < v_cnt; i++) {
			const double t = s + pi2 * i / v_cnt;	// i �Ԗڂ̒��_�̊p�x
			v_pos[i].x = static_cast<FLOAT>(cos(t));
			v_pos[i].y = static_cast<FLOAT>(-sin(t));
			pt_inc(v_pos[i], v_min, v_max);
		}
		D2D1_POINT_2F v_diff;
		pt_sub(v_max, v_min, v_diff);

		// �����p�`��̈�̑傫���ɍ��킹��.
		const double rate_x = v_reg ? fmin(b_diff.x, b_diff.y) / fmax(v_diff.x, v_diff.y) : b_diff.x / v_diff.x;
		const double rate_y = v_reg ? rate_x : b_diff.y / v_diff.y;
		for (uint32_t i = 0; i < v_cnt; i++) {
			pt_sub(v_pos[i], v_min, v_pos[i]);
			v_pos[i].x = static_cast<FLOAT>(roundl(v_pos[i].x * rate_x));
			v_pos[i].y = static_cast<FLOAT>(roundl(v_pos[i].y * rate_y));
			pt_add(v_pos[i], b_pos, v_pos[i]);
		}
	}

	// �p�X�W�I���g�����쐬����.
	// d_factory DX �t�@�N�g��
	void ShapePoly::create_path_geometry(ID2D1Factory3* const d_factory)
	{
		if (m_d2d_path_geom != nullptr) {
			m_d2d_path_geom = nullptr;
		}
		if (m_d2d_arrow_geom != nullptr) {
			m_d2d_arrow_geom = nullptr;
		}

		const auto d_cnt = m_diff.size();	// �����̐�
		if (d_cnt < 1) {
			return;
		}

		// �J�n�ʒu��, �����̔z������Ƃ�, ���_�����߂�.
		const size_t v_cnt = d_cnt + 1;	// ���_�̐� (�����̐� + 1)
		//std::vector<D2D1_POINT_2F> vert_pos(v_cnt);	// ���_�̔z��
		//const auto v_pos = reinterpret_cast<D2D1_POINT_2F*>(vert_pos.data());
		D2D1_POINT_2F v_pos[N_GON_MAX];
		v_pos[0] = m_pos;
		for (size_t i = 1; i < v_cnt; i++) {
			pt_add(v_pos[i - 1], m_diff[i - 1], v_pos[i]);
		}

		// �܂���̃p�X�W�I���g�����쐬����.
		winrt::com_ptr<ID2D1GeometrySink> sink;
		winrt::check_hresult(d_factory->CreatePathGeometry(m_d2d_path_geom.put()));
		winrt::check_hresult(m_d2d_path_geom->Open(sink.put()));
		sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
		const auto figure_begin = is_opaque(m_fill_color) ? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW;
		sink->BeginFigure(v_pos[0], figure_begin);
		for (size_t i = 1; i < v_cnt; i++) {
			sink->AddLine(v_pos[i]);
		}
		sink->EndFigure(m_end_closed ? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
		winrt::check_hresult(sink->Close());
		sink = nullptr;

		// ���̌`�����Ȃ������肷��.
		const auto a_style = m_arrow_style;
		if (a_style == ARROWHEAD_STYLE::NONE) {
			return;
		}

		// ���̈ʒu�����߂�.
		D2D1_POINT_2F h_tip;
		D2D1_POINT_2F h_barbs[2];
		if (poly_get_arrow_barbs(v_cnt, v_pos, m_arrow_size, h_tip, h_barbs)) {
			// ���̃p�X�W�I���g�����쐬����.
			winrt::check_hresult(d_factory->CreatePathGeometry(m_d2d_arrow_geom.put()));
			winrt::check_hresult(m_d2d_arrow_geom->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(h_barbs[0], a_style == ARROWHEAD_STYLE::FILLED ? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
			sink->AddLine(h_tip);
			sink->AddLine(h_barbs[1]);
			sink->EndFigure(a_style == ARROWHEAD_STYLE::FILLED ? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
			winrt::check_hresult(sink->Close());
			sink = nullptr;
		}
		return;
	}

	// �}�`��\������.
	// dx	�}�`�̕`���
	void ShapePoly::draw(SHAPE_DX& dx)
	{
		if (is_opaque(m_fill_color)) {
			const auto p_geom = m_d2d_path_geom.get();
			if (p_geom != nullptr) {
				dx.m_shape_brush->SetColor(m_fill_color);
				dx.m_d2dContext->FillGeometry(p_geom, dx.m_shape_brush.get(), nullptr);
			}
		}
		if (is_opaque(m_stroke_color)) {
			const auto p_geom = m_d2d_path_geom.get();
			const auto s_width = m_stroke_width;
			const auto s_brush = dx.m_shape_brush.get();
			const auto s_style = m_d2d_stroke_style.get();
			s_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawGeometry(p_geom, s_brush, s_width, s_style);
			if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
				const auto a_geom = m_d2d_arrow_geom.get();
				if (a_geom != nullptr) {
					dx.m_d2dContext->FillGeometry(a_geom, s_brush, nullptr);
					if (m_arrow_style != ARROWHEAD_STYLE::FILLED) {
						dx.m_d2dContext->DrawGeometry(a_geom, s_brush, s_width, m_d2d_arrow_style.get());
					}
				}
			}
		}
		if (is_selected()) {
			D2D1_POINT_2F a_pos{ m_pos };	// �}�`�̕��ʂ̈ʒu
			anchor_draw_rect(a_pos, dx);
			const size_t d_cnt = m_diff.size();	// �����̐�
			for (size_t i = 0; i < d_cnt; i++) {
				pt_add(a_pos, m_diff[i], a_pos);
				anchor_draw_rect(a_pos, dx);
			}
		}
	}

	// �h��Ԃ��F�𓾂�.
	// value	����ꂽ�l
	// �߂�l	����ꂽ�Ȃ� true
	bool ShapePoly::get_fill_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_fill_color;
		return true;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// a_len	���ʂ̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapePoly::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		D2D1_POINT_2F t_vec;
		pt_sub(t_pos, m_pos, t_vec);
		return stroke_hit_test(
			t_vec,
			m_diff.size(), m_diff.data(),
			a_len,
			is_opaque(m_stroke_color),
			m_stroke_width,
			m_end_closed,
			m_stroke_cap_style,
			m_stroke_join_style,
			m_stroke_join_limit,
			is_opaque(m_fill_color)
		);
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapePoly::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		if (!pt_in_rect(m_pos, a_min, a_max)) {
			return false;
		}
		const size_t n = m_diff.size();	// �����̐�
		D2D1_POINT_2F e_pos = m_pos;
		for (size_t i = 0; i < n; i++) {
			pt_add(e_pos, m_diff[i], e_pos);	// ���̈ʒu
			if (!pt_in_rect(e_pos, a_min, a_max)) {
				return false;
			}
		}
		return true;
	}

	// �h��Ԃ��̐F�Ɋi�[����.
	void ShapePoly::set_fill_color(const D2D1_COLOR_F& value) noexcept
	{
		if (equal(m_fill_color, value)) {
			return;
		}
		m_fill_color = value;
		create_path_geometry(s_d2d_factory);
	}

	// �}�`���쐬����.
	// b_pos	�͂ޗ̈�̎n�_
	// b_diff	�͂ޗ̈�̏I�_�ւ̍���
	// s_attr	����
	// v_cnt	���_�̐�
	// v_reg	�����p�`�ɍ�}���邩����
	// v_up	���_����ɍ�}���邩����
	// v_end	�ӂ���č�}���邩����
	ShapePoly::ShapePoly(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr, const TOOL_POLY& t_poly) :
		ShapePath::ShapePath(t_poly.m_vertex_cnt - 1, s_attr),
		m_end_closed(t_poly.m_closed),
		m_fill_color(s_attr->m_fill_color)
	{
		std::vector<D2D1_POINT_2F> vert_pos(t_poly.m_vertex_cnt);	// ���_�̔z��
		const auto v_pos = reinterpret_cast<D2D1_POINT_2F*>(vert_pos.data());
		create_poly_by_bbox(b_pos, b_diff, t_poly.m_vertex_cnt, t_poly.m_vertex_up, t_poly.m_regular, t_poly.m_clockwise, v_pos);
		m_pos = v_pos[0];
		for (size_t i = 1; i < t_poly.m_vertex_cnt; i++) {
			pt_sub(v_pos[i], v_pos[i - 1], m_diff[i - 1]);
		}
		vert_pos.clear();
		create_path_geometry(s_d2d_factory);
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	// dt_reader	�f�[�^���[�_�[
	ShapePoly::ShapePoly(DataReader const& dt_reader) :
		ShapePath::ShapePath(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;
		m_end_closed = dt_reader.ReadBoolean();
		read(m_fill_color, dt_reader);
		create_path_geometry(s_d2d_factory);
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapePoly::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapePath::write(dt_writer);
		dt_writer.WriteBoolean(m_end_closed);
		write(m_fill_color, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapePoly::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<path d=\"", dt_writer);
		write_svg(m_pos, "M", dt_writer);
		const auto d_cnt = m_diff.size();	// �����̐�
		const auto v_cnt = d_cnt + 1;
		//std::vector<D2D1_POINT_2F> vert_pos(v_cnt);
		//const auto v_pos = reinterpret_cast<D2D1_POINT_2F*>(vert_pos.data());
		D2D1_POINT_2F v_pos[N_GON_MAX];

		v_pos[0] = m_pos;
		for (size_t i = 0; i < d_cnt; i++) {
			write_svg(m_diff[i], "l", dt_writer);
			pt_add(v_pos[i], m_diff[i], v_pos[i + 1]);
		}
		if (m_end_closed) {
			write_svg("Z", dt_writer);
		}
		write_svg("\" ", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg(m_fill_color, "fill", dt_writer);
		write_svg("/>" SVG_NEW_LINE, dt_writer);
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			D2D1_POINT_2F h_tip;
			D2D1_POINT_2F h_barbs[2];
			if (poly_get_arrow_barbs(v_cnt, v_pos, m_arrow_size, h_tip, h_barbs)) {
				write_svg("<path d=\"", dt_writer);
				write_svg(h_barbs[0], "M", dt_writer);
				write_svg(h_tip, "L", dt_writer);
				write_svg(h_barbs[1], "L", dt_writer);
				if (m_arrow_style == ARROWHEAD_STYLE::FILLED) {
					write_svg("Z", dt_writer);
				}
				write_svg("\" ", dt_writer);
				ShapeStroke::write_svg(dt_writer);
				if (m_arrow_style == ARROWHEAD_STYLE::FILLED) {
					write_svg(m_stroke_color, "fill", dt_writer);
				}
				else {
					write_svg("fill=\"transparent\" ", dt_writer);
				}
				write_svg("/>" SVG_NEW_LINE, dt_writer);
			}
		}
	}

}