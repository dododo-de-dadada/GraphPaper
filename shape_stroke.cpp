#include "pch.h"
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
		const double bx = b.x;
		const double by = b.y;
		const double cx = c.x;
		const double cy = c.y;
		const double dx = d.x;
		const double dy = d.y;
		const double ab_x = bx - ax;
		const double ab_y = by - ay;
		const double cd_x = dx - cx;
		const double cd_y = dy - cy;
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
		// ���ׂ�ʒu��, �ŏ��̒��_�𒆐S�Ƃ�, �g�����镝�𔼌a�Ƃ���, �~�Ɋ܂܂�邩���肷��.
		if (pt_abs2(t_pos) <= e_width * e_width) {
			return true;
		}
		// ���ׂ�ʒu��, �Ō�̒��_�𒆐S�Ƃ�, �g�����镝�𔼌a�Ƃ���, �~�Ɋ܂܂�邩���肷��.
		D2D1_POINT_2F u_pos;
		pt_sub(t_pos, v_end, u_pos);
		return pt_abs2(u_pos) <= e_width * e_width;
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
			D2D1_POINT_2F d_vec;
			pt_sub(t_pos, v_pos[i], d_vec);
			if (pt_abs2(d_vec) <= exp_width * exp_width) {
				return true;
			}
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
				if (pt_abs2(t_pos) <= e_width * e_width) {
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
					pt_add(v_pos[i], -c_vec.x - o_vec.x, -c_vec.y - o_vec.y, e_side[e_cnt][0]);
					pt_add(v_pos[i], -c_vec.x + o_vec.x, -c_vec.y + o_vec.y, e_side[e_cnt][1]);
					pt_add(v_pos[i], c_vec.x + o_vec.x, c_vec.y + o_vec.y, e_side[e_cnt][2]);
					pt_add(v_pos[i], c_vec.x - o_vec.x, c_vec.y - o_vec.y, e_side[e_cnt][3]);
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

	// D2D �X�g���[�N�������쐬����.
	static void create_stroke_dash_style(ID2D1Factory3* const d_factory, const D2D1_CAP_STYLE s_cap_line, const D2D1_CAP_STYLE s_cap_dash, const D2D1_DASH_STYLE s_dash, const STROKE_DASH_PATT& s_patt, const D2D1_LINE_JOIN s_join, const double s_limit, ID2D1StrokeStyle** s_style);

	// D2D �X�g���[�N�������쐬����.
	// s_cap_line	���̒[�_
	// s_cap_dash	�j���̒[�_
	// s_dash	�j���̎��
	// s_patt	�j���̔z�u�z��
	// s_join	���̂Ȃ���
	// s_limit	�}�C�^�[����
	// s_style	�쐬���ꂽ�X�g���[�N����
	static void create_stroke_dash_style(
		ID2D1Factory3* const d_factory,
		const D2D1_CAP_STYLE s_cap_line,
		const D2D1_CAP_STYLE s_cap_dash,
		const D2D1_DASH_STYLE s_dash,
		const STROKE_DASH_PATT& s_patt,
		const D2D1_LINE_JOIN s_join,
		const double s_limit, ID2D1StrokeStyle** s_style)
	{
		UINT32 d_cnt;	// �j���̔z�u�z��̗v�f��
		const FLOAT* d_arr;	// �j���̔z�u�z����w���|�C���^

		if (s_dash != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID) {
			const D2D1_STROKE_STYLE_PROPERTIES s_prop{
				s_cap_line,	// startCap
				s_cap_line,	// endCap
				s_cap_dash,	// dashCap
				s_join,	// lineJoin
				static_cast<FLOAT>(s_limit),	// miterLimit
				D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM,	// dashStyle
				0.0f
			};
			if (s_dash == D2D1_DASH_STYLE_DOT) {
				d_arr = s_patt.m_ + 2;
				d_cnt = 2;
			}
			else {
				d_arr = s_patt.m_;
				if (s_dash == D2D1_DASH_STYLE_DASH) {
					d_cnt = 2;
				}
				else if (s_dash == D2D1_DASH_STYLE_DASH_DOT) {
					d_cnt = 4;
				}
				else if (s_dash == D2D1_DASH_STYLE_DASH_DOT_DOT) {
					d_cnt = 6;
				}
				else {
					d_cnt = 0;
				}
			}
			winrt::check_hresult(
				d_factory->CreateStrokeStyle(s_prop, d_arr, d_cnt, s_style)
			);
		}
		else {
			const D2D1_STROKE_STYLE_PROPERTIES s_prop{
				s_cap_line,	// startCap
				s_cap_line,	// endCap
				s_cap_dash,	// dashCap
				s_join,	// lineJoin
				static_cast<FLOAT>(s_limit),	// miterLimit
				D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID,	// dashStyle
				0.0f	// dashOffset
			};
			winrt::check_hresult(
				d_factory->CreateStrokeStyle(s_prop, nullptr, 0, s_style)
			);
		}
	}

	// �}�`��j������.
	ShapeStroke::~ShapeStroke(void)
	{
		m_d2d_stroke_dash_style = nullptr;
	}

	// �}�`���͂ޗ̈�𓾂�.
	// a_min	���̗̈�̍���ʒu.
	// a_man	���̗̈�̉E���ʒu.
	// b_min	����ꂽ�̈�̍���ʒu.
	// b_max	����ꂽ�̈�̉E���ʒu.
	void ShapeStroke::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		const size_t d_cnt = m_diff.size();	// �����̐�
		D2D1_POINT_2F e_pos = m_pos;
		b_min = a_min;
		b_max = a_max;
		pt_inc(e_pos, b_min, b_max);
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(e_pos, m_diff[i], e_pos);
			pt_inc(e_pos, b_min, b_max);
		}
	}

	// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
	// value	�̈�̍���ʒu
	void ShapeStroke::get_min_pos(D2D1_POINT_2F& value) const noexcept
	{
		const size_t n = m_diff.size();	// �����̐�
		D2D1_POINT_2F v_pos = m_pos;	// ���_�̈ʒu
		value = m_pos;
		for (size_t i = 0; i < n; i++) {
			pt_add(v_pos, m_diff[i], v_pos);
			pt_min(value, v_pos, value);
		}

		//value.x = m_diff[0].x >= 0.0f ? m_pos.x : m_pos.x + m_diff[0].x;
		//value.y = m_diff[0].y >= 0.0f ? m_pos.y : m_pos.y + m_diff[0].y;
	}

	// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
	void ShapeStroke::get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept
	{
		if (anch == ANCH_TYPE::ANCH_SHEET || anch == ANCH_TYPE::ANCH_P0) {
			// �}�`�̕��ʂ��u�O���v�܂��́u�J�n�_�v�Ȃ��, �J�n�ʒu�𓾂�.
			value = m_pos;
		}
		else if (anch > ANCH_TYPE::ANCH_P0) {
			const size_t m = m_diff.size() + 1;		// ���_�̐� (�����̐� + 1)
			if (anch < ANCH_TYPE::ANCH_P0 + m) {
				value = m_pos;
				for (size_t i = 0; i < anch - ANCH_TYPE::ANCH_P0; i++) {
					pt_add(value, m_diff[i], value);
				}
			}
		}
		//value = m_pos;
	}

	// �J�n�ʒu�𓾂�
	// �߂�l	�˂� true
	bool ShapeStroke::get_start_pos(D2D1_POINT_2F& value) const noexcept
	{
		value = m_pos;
		return true;
	}

	// ���g�̐F�𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_stroke_color;
		return true;
	}

	// ���g�̃}�C�^�[�����̔䗦�𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_join_limit(float& value) const noexcept
	{
		value = m_stroke_join_limit;
		return true;
	}

	// ���̂Ȃ���𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_join_style(D2D1_LINE_JOIN& value) const noexcept
	{
		value = m_stroke_join_style;
		return true;
	}

	// ���̂Ȃ���𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_cap_dash(D2D1_CAP_STYLE& value) const noexcept
	{
		value = m_stroke_cap_dash;
		return true;
	}

	// ���̂Ȃ���𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_cap_line(D2D1_CAP_STYLE& value) const noexcept
	{
		value = m_stroke_cap_line;
		return true;
	}

	// �j���̔z�u�𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_dash_patt(STROKE_DASH_PATT& value) const noexcept
	{
		value = m_stroke_dash_patt;
		return true;
	}

	// ���g�̌`���𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_dash_style(D2D1_DASH_STYLE& value) const noexcept
	{
		value = m_stroke_dash_style;
		return true;
	}

	// ���g�̑����𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_width(float& value) const noexcept
	{
		value = m_stroke_width;
		return true;
	}

	uint32_t ShapeStroke::hit_test(const D2D1_POINT_2F t_pos, const double a_len, const size_t d_cnt, const D2D1_POINT_2F diff[], const bool s_close, const bool f_opa) const noexcept
	{
		return stroke_hit_test(t_pos, d_cnt, diff, a_len, is_opaque(m_stroke_color), m_stroke_width, 
			s_close,
			m_stroke_cap_line, m_stroke_join_style, m_stroke_join_limit, f_opa);
	}

	// �ʒu���܂ނ����肷��.
	// �߂�l	�˂� ANCH_SHEET
	uint32_t ShapeStroke::hit_test(const D2D1_POINT_2F /*t_pos*/, const double /*a_len*/) const noexcept
	{
		return ANCH_TYPE::ANCH_SHEET;
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// �߂�l	�˂� false
	bool ShapeStroke::in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept
	{
		return false;
	}

	// ���������ړ�����.
	void ShapeStroke::move(const D2D1_POINT_2F diff)
	{
		D2D1_POINT_2F s_pos;
		pt_add(m_pos, diff, s_pos);
		set_start_pos(s_pos);
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	void ShapeStroke::set_start_pos(const D2D1_POINT_2F value)
	{
		D2D1_POINT_2F s_pos;
		pt_round(value, PT_ROUND, s_pos);
		m_pos = s_pos;
	}

	// ���g�̐F�Ɋi�[����.
	void ShapeStroke::set_stroke_color(const D2D1_COLOR_F& value) noexcept
	{
		m_stroke_color = value;
	}

	// �����f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	// barbs	���̗��[�̈ʒu
	// tip_pos	���̐�[�̈ʒu
	// a_style	���̌`��
	// dt_writer	�f�[�^���C�^�[
	void ShapeStroke::write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, const ARROWHEAD_STYLE a_style, DataWriter const& dt_writer) const
	{
		using  winrt::GraphPaper::implementation::write_svg;

		write_svg("<path d=\"", dt_writer);
		write_svg("M", dt_writer);
		write_svg(barbs[0].x, dt_writer);
		write_svg(barbs[0].y, dt_writer);
		write_svg("L", dt_writer);
		write_svg(tip_pos.x, dt_writer);
		write_svg(tip_pos.y, dt_writer);
		write_svg("L", dt_writer);
		write_svg(barbs[1].x, dt_writer);
		write_svg(barbs[1].y, dt_writer);
		write_svg("\" ", dt_writer);
		if (a_style == ARROWHEAD_STYLE::FILLED) {
			write_svg(m_stroke_color, "fill", dt_writer);
		}
		else {
			write_svg("fill=\"none\" ", dt_writer);
		}
		write_svg(m_stroke_color, "stroke", dt_writer);
		write_svg(m_stroke_width, "stroke-width", dt_writer);
		write_svg(" />" SVG_NEW_LINE, dt_writer);
	}

	// �l��j���̒[�_�Ɋi�[����.
	void ShapeStroke::set_stroke_cap_dash(const D2D1_CAP_STYLE& value)
	{
		if (equal(m_stroke_cap_dash, value)) {
			return;
		}
		m_stroke_cap_dash = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// �l����̒[�_�Ɋi�[����.
	void ShapeStroke::set_stroke_cap_line(const D2D1_CAP_STYLE& value)
	{
		if (equal(m_stroke_cap_line, value)) {
			return;
		}
		m_stroke_cap_line = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// �l���}�C�^�[�����̔䗦�Ɋi�[����.
	void ShapeStroke::set_stroke_join_limit(const float& value)
	{
		if (equal(m_stroke_join_limit, value)) {
			return;
		}
		m_stroke_join_limit = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// �l����̂Ȃ���Ɋi�[����.
	void ShapeStroke::set_stroke_join_style(const D2D1_LINE_JOIN& value)
	{
		if (equal(m_stroke_join_style, value)) {
			return;
		}
		m_stroke_join_style = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// �l��j���̔z�u�Ɋi�[����.
	void ShapeStroke::set_stroke_dash_patt(const STROKE_DASH_PATT& value)
	{
		if (equal(m_stroke_dash_patt, value)) {
			return;
		}
		m_stroke_dash_patt = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// �l����g�̌`���Ɋi�[����.
	void ShapeStroke::set_stroke_dash_style(const D2D1_DASH_STYLE value)
	{
		if (m_stroke_dash_style == value) {
			return;
		}
		m_stroke_dash_style = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// �l����g�̑����Ɋi�[����.
	void ShapeStroke::set_stroke_width(const float value) noexcept
	{
		m_stroke_width = value;
	}

	// �}�`���쐬����.
	// d_cnt	�����̌� (�ő�l�� N_GON_MAX - 1)
	// s_attr	�����l
	ShapeStroke::ShapeStroke(const size_t d_cnt, const ShapeSheet* s_attr) :
		m_diff(d_cnt <= N_GON_MAX - 1 ? d_cnt : N_GON_MAX - 1),
		m_stroke_cap_dash(s_attr->m_stroke_cap_dash),
		m_stroke_cap_line(s_attr->m_stroke_cap_line),
		m_stroke_color(s_attr->m_stroke_color),
		m_stroke_dash_patt(s_attr->m_stroke_dash_patt),
		m_stroke_dash_style(s_attr->m_stroke_dash_style),
		m_stroke_join_limit(s_attr->m_stroke_join_limit),
		m_stroke_join_style(s_attr->m_stroke_join_style),
		m_stroke_width(s_attr->m_stroke_width),
		m_d2d_stroke_dash_style(nullptr)
	{
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapeStroke::ShapeStroke(DataReader const& dt_reader) :
		m_d2d_stroke_dash_style(nullptr)
	{
		using winrt::GraphPaper::implementation::read;

		set_delete(dt_reader.ReadBoolean());
		set_select(dt_reader.ReadBoolean());
		read(m_pos, dt_reader);
		read(m_diff, dt_reader);
		m_stroke_cap_dash = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
		m_stroke_cap_line = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
		read(m_stroke_color, dt_reader);
		m_stroke_join_limit = dt_reader.ReadSingle();
		m_stroke_join_style = static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32());
		read(m_stroke_dash_patt, dt_reader);
		m_stroke_dash_style = static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32());
		m_stroke_width = dt_reader.ReadSingle();
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapeStroke::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		dt_writer.WriteBoolean(is_deleted());
		dt_writer.WriteBoolean(is_selected());
		write(m_pos, dt_writer);
		write(m_diff, dt_writer);
		dt_writer.WriteUInt32(m_stroke_cap_dash);
		dt_writer.WriteUInt32(m_stroke_cap_line);
		write(m_stroke_color, dt_writer);
		dt_writer.WriteSingle(m_stroke_join_limit);
		dt_writer.WriteUInt32(m_stroke_join_style);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[0]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[1]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[2]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[3]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[4]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[5]);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_stroke_dash_style));
		dt_writer.WriteSingle(m_stroke_width);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeStroke::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg(m_stroke_color, "stroke", dt_writer);
		write_svg(m_stroke_dash_style, m_stroke_dash_patt, m_stroke_width, dt_writer);
		write_svg(m_stroke_width, "stroke-width", dt_writer);
		write_svg("stroke-linejoin=\"miter\" stroke-miterlimit=\"1\" ", dt_writer);
	}

}