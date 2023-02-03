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
	static bool poly_find_intersection(const D2D1_POINT_2F a, const D2D1_POINT_2F b, const D2D1_POINT_2F c, const D2D1_POINT_2F d, double& s, double& t, D2D1_POINT_2F& e)
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

	// �ʒu��, �����̒[ (�~�`) �Ɋ܂܂�邩���肷��.
	// t_vec	�����̎n�_�����_�Ƃ���, ���肷��ʒu.
	// v_end	�����̏I�_
	// �߂�l	�܂܂��Ȃ� true
	//static bool stroke_test_cap_round(const D2D1_POINT_2F t_vec, const D2D1_POINT_2F v_end, const double e_width)
	//{
	//	return pt_in_circle(t_vec, e_width) || pt_in_circle(t_vec, v_end, e_width);
	//}

	static bool poly_test_cap_square(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F v_end, const size_t d_cnt, const D2D1_POINT_2F d_vec[], const double s_len[], const double e_width)
	{
		for (size_t i = 0; i < d_cnt; i++) {
			if (s_len[i] >= FLT_MIN) {
				D2D1_POINT_2F s_vec;	// �Ӄx�N�g��
				pt_mul(d_vec[i], -e_width / s_len[i], s_vec);
				const D2D1_POINT_2F o_vec{ s_vec.y, -s_vec.x };
				D2D1_POINT_2F q_pos[4];
				pt_add(s_vec, o_vec, q_pos[0]);
				pt_sub(s_vec, o_vec, q_pos[1]);
				//pt_neg(o_vec, q_pos[2]);
				q_pos[2].x = -o_vec.x;
				q_pos[2].y = -o_vec.y;
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
			if (s_len[i - 1] >= FLT_MIN) {
				D2D1_POINT_2F s_vec;	// �Ӄx�N�g��
				pt_mul(d_vec[i - 1], e_width / s_len[i - 1], s_vec);
				const D2D1_POINT_2F o_vec{ s_vec.y, -s_vec.x };
				D2D1_POINT_2F q_pos[4];
				pt_add(s_vec, o_vec, q_pos[0]);
				pt_sub(s_vec, o_vec, q_pos[1]);
				//pt_neg(o_vec, q_pos[2]);
				q_pos[2].x = -o_vec.x;
				q_pos[2].y = -o_vec.y;
				q_pos[3] = o_vec;
				if (pt_in_poly(u_pos, 4, q_pos)) {
					return true;
				}
				break;
			}
		}
		return false;
	}

	static bool poly_test_cap_triangle(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F v_end, const size_t d_cnt, const D2D1_POINT_2F d_vec[], const double s_len[], const double e_width)
	{
		for (size_t i = 0; i < d_cnt; i++) {
			if (s_len[i] >= FLT_MIN) {
				D2D1_POINT_2F s_vec;
				pt_mul(d_vec[i], -e_width / s_len[i], s_vec);
				const D2D1_POINT_2F o_vec{ s_vec.y, -s_vec.x };
				D2D1_POINT_2F tri_pos[3];
				tri_pos[0] = s_vec;
				//pt_neg(o_vec, tri_pos[1]);
				tri_pos[1].x = -o_vec.x;
				tri_pos[1].y = -o_vec.y;
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
			if (s_len[i - 1] >= FLT_MIN) {
				D2D1_POINT_2F s_vec;
				pt_mul(d_vec[i - 1], e_width / s_len[i - 1], s_vec);
				const D2D1_POINT_2F o_vec{ s_vec.y, -s_vec.x };
				D2D1_POINT_2F tri_pos[3];
				tri_pos[0] = s_vec;
				//pt_neg(o_vec, tri_pos[1]);
				tri_pos[1].x = -o_vec.x;
				tri_pos[1].y = -o_vec.y;
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
	static bool poly_test_join_bevel(
		const D2D1_POINT_2F t_pos,
		const size_t v_cnt,
		const bool v_close,
		const D2D1_POINT_2F e_side[][4 + 1]) noexcept
	{
		// { 0, 1 }, { 1, 2 }, { v_cnt-2, v_cnt-1 }
		for (size_t i = 1; i < v_cnt; i++) {
			const D2D1_POINT_2F bev_pos[4]{ e_side[i - 1][3], e_side[i][0], e_side[i][1], e_side[i - 1][2] };
			if (pt_in_poly(t_pos, 4, bev_pos)) {
				return true;
			}
		}
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
	// s_join	���̌������@
	static bool poly_test_join_miter(
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
				// ���̌������}�C�^�[�����肷��.
				if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER) {
					// �}�C�^�[�Ȃ��, �� j ���}�C�^�[�����̒����������������l�ӌ`�����߂�.
					D2D1_POINT_2F e_vec;	// �Ӄx�N�g��
					pt_sub(exp_side[j][3], exp_side[j][0], e_vec);
					pt_mul(e_vec, exp_width * s_limit / sqrt(pt_abs2(e_vec)), e_vec);
					D2D1_POINT_2F q_pos[4];
					q_pos[0] = exp_side[j][3];
					q_pos[1] = exp_side[j][2];
					pt_add(exp_side[j][2], e_vec, q_pos[2]);
					pt_add(exp_side[j][3], e_vec, q_pos[3]);
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
			if (!poly_find_intersection(exp_side[j][0], exp_side[j][3], exp_side[i][0], exp_side[i][3], s, t, q_pos[0])) {
				continue;
			}
			if (s < 1.0 || t > 0.0) {
				if (!poly_find_intersection(exp_side[j][1], exp_side[j][2], exp_side[i][1], exp_side[i][2], s, t, q_pos[0])) {
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
			// �}�C�^�[�����𒴂���Ȃ��, ���̌������}�C�^�[�܂��͖ʎ�肩���肷��.
			if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				// ���̌������}�C�^�[�܂��͖ʎ��Ȃ��, ���ׂ�ʒu���O�p�` { q1, q2, q3 } ���܂ނ����肷��.
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
			pt_mul_add(d_vec, limit_len / sqrt(d_abs2), exp_side[i][4], mit_pos);
			D2D1_POINT_2F nor_pos;
			pt_add(mit_pos, D2D1_POINT_2F{ d_vec.y, -d_vec.x }, nor_pos);
			poly_find_intersection(q_pos[3], q_pos[0], mit_pos, nor_pos, s, t, q_pos[4]);
			poly_find_intersection(q_pos[0], q_pos[1], mit_pos, nor_pos, s, t, q_pos[0]);
			const D2D1_POINT_2F* pen_pos = q_pos;
			if (pt_in_poly(t_pos, 5, pen_pos)) {
				// �ʒu���܂ނȂ� true ��Ԃ�.
				return true;
			}
		}
		return false;
	}

	// ���p�`�̊p���ʒu���܂ނ����肷�� (�ۂ܂����p)
	static bool poly_test_join_round(const D2D1_POINT_2F& t_pos, const size_t v_cnt, const D2D1_POINT_2F v_pos[], const double exp_width)
	{
		for (size_t i = 0; i < v_cnt; i++) {
			if (pt_in_circle(t_pos, v_pos[i], exp_width)) {
				return true;
			}
		}
		return false;
	}

	// �ʒu��, �����Ɋ܂܂�邩���肷��.
	// t_vec	���肷��ʒu (�����̎n�_�����_�Ƃ���)
	// d_cnt	�����x�N�g���̐�
	// d_vec	�����x�N�g�� (���_�̊Ԃ̍���)
	// t_anc
	// s_opa	�����s����������
	// s_width	���̑���
	// s_closed	�������Ă��邩����
	// s_join	���̌���
	// s_limit	�}�C�^�[����
	// f_opa	�h��Ԃ����s����������
	static uint32_t poly_hit_test(
		const D2D1_POINT_2F t_vec,
		const size_t d_cnt,
		const D2D1_POINT_2F d_vec[],
		//const bool t_anc,
		const bool s_opaque,
		const double s_width,
		const bool s_closed,
		const CAP_STYLE& s_cap,
		const D2D1_LINE_JOIN s_join,
		const double s_limit,
		const bool f_opa,
		const double a_len
	)
	{
		D2D1_POINT_2F v_pos[N_GON_MAX]{ { 0.0f, 0.0f }, };	// ���_�̈ʒu
		double s_len[N_GON_MAX];	// �ӂ̒���
		size_t nz_cnt = 0;	// �����̂���ӂ̐�
		size_t k = static_cast<size_t>(-1);	// �����������_
		for (size_t i = 0; i < d_cnt; i++) {
			// ���肷��ʒu��, ���_�̕��ʂɊ܂܂�邩���肷��.
			if (/*t_anc &&*/pt_in_anc(t_vec, v_pos[i], a_len)) {
				k = i;
			}
			// �ӂ̒��������߂�.
			s_len[i] = sqrt(pt_abs2(d_vec[i]));
			// �ӂ̒��������邩���肷��.
			if (s_len[i] >= FLT_MIN) {
				nz_cnt++;
			}
			// ���_�ɕӃx�N�g��������, ���̒��_�����߂�.
			pt_add(v_pos[i], d_vec[i], v_pos[i + 1]);
		}
		// ���肷��ʒu��, �I�_�̕��ʂɊ܂܂�邩���肷��.
		if (pt_in_anc(t_vec, v_pos[d_cnt], a_len)) {
			k = d_cnt;
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
				if (pt_in_circle(t_vec, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
				return ANC_TYPE::ANC_PAGE;
			}
			// �ӂ����Ă��邩���肷��.
			if (s_closed) {
				// ���Ă���Ȃ�, �n�_�� { 0, 0 } �Ȃ̂ŏI�_�ւ̃x�N�g����, ���̂܂܍Ō�̕ӂ̒����Ƃ���.
				s_len[d_cnt] = sqrt(pt_abs2(v_pos[d_cnt]));
			}
			// ���ĂȂ��Ȃ�, �[�̌`�����~�`�����肷��.
			else if (equal(s_cap, CAP_ROUND)) {
				if (pt_in_circle(t_vec, e_width) || pt_in_circle(t_vec, v_pos[d_cnt], e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// ���ĂȂ��Ȃ�, �[�̌`���������`�����肷��.
			else if (equal(s_cap, CAP_SQUARE)) {
				if (poly_test_cap_square(t_vec, v_pos[d_cnt], d_cnt, d_vec, s_len, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// ���ĂȂ��Ȃ�, �[�̌`�����O�p�`�����肷��.
			else if (equal(s_cap, CAP_TRIANGLE)) {
				if (poly_test_cap_triangle(t_vec, v_pos[d_cnt], d_cnt, d_vec, s_len, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			D2D1_POINT_2F e_side[N_GON_MAX][4 + 1];	// �g�����ꂽ�� (+���_)
			size_t e_cnt = 0;
			for (size_t i = 0; i < d_cnt; i++) {
				// �� i �̒������Ȃ������肷��.
				if (s_len[i] < FLT_MIN) {
					// �_ i ����~����, �����̂���ӂ�������.
					size_t p = static_cast<size_t>(-1);
					for (size_t h = i; h > 0; h--) {
						if (s_len[h - 1] >= FLT_MIN) {
							p = h - 1;
							break;
						}
					}
					// �~���̕ӂ��������������肷��.
					D2D1_POINT_2F p_vec;	// ���O�̕Ӄx�N�g��
					if (p != static_cast<size_t>(-1)) {
						// ���������ӂ𒼑O�̕Ӄx�N�g���Ɋi�[����.
						p_vec = d_vec[p];
					}
					// ������Ȃ������Ȃ��,
					// �ӂ����Ă���, ���Ō�̕ӂ̒�������[�������肷��.
					else if (s_closed && s_len[d_cnt] >= FLT_MIN) {
						// �Ō�̒��_�̔��΃x�N�g��������, ���O�̕Ӄx�N�g���Ƃ���.
						p_vec = D2D1_POINT_2F{ -v_pos[d_cnt].x, -v_pos[d_cnt].y };
					}
					else {
						continue;
					}
					// �_ i ���珸����, �����̂���ӂ�������.
					size_t n = static_cast<size_t>(-1);
					for (size_t j = i + 1; j < d_cnt; j++) {
						if (s_len[j] >= FLT_MIN) {
							n = j;
							break;
						}
					}
					// �����̕ӂ��������������肷��.
					D2D1_POINT_2F n_vec;	// ����̕Ӄx�N�g��
					if (n != -1) {
						// ���������ӂ𒼌�̕Ӄx�N�g���Ɋi�[����.
						n_vec = d_vec[n];
					}
					// ������Ȃ������Ȃ��,
					// �ӂ����Ă���, ���Ō�̕ӂɒ��������邩���肷��.
					else if (s_closed && s_len[d_cnt] >= FLT_MIN) {
						// �Ō�̒��_�̔��΃x�N�g��������, ����̕Ӄx�N�g���Ƃ���.
						n_vec = D2D1_POINT_2F{ -v_pos[d_cnt].x, -v_pos[d_cnt].y };
					}
					else {
						continue;
					}
					// ���O�ƒ���̕Ӄx�N�g��������, �����x�N�g�������߂�.
					D2D1_POINT_2F c_vec;
					pt_add(p_vec, n_vec, c_vec);
					// �����x�N�g���̒������Ȃ������肷��.
					double c_abs = pt_abs2(c_vec);
					if (c_abs < FLT_MIN) {
						// ���O�̕Ӄx�N�g���������x�N�g���Ƃ���.
						c_vec = p_vec;
						c_abs = pt_abs2(c_vec);
					}
					// �����x�N�g���̒����x�N�g�������߂�.
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
					D2D1_POINT_2F s_vec;
					pt_mul(d_vec[i], e_width / s_len[i], s_vec);
					const D2D1_POINT_2F o_vec{ s_vec.y, -s_vec.x };
					// ���_ i �� i+1 �𒼌��x�N�g���ɉ����Đ��t�Ɋg����, �g�����ꂽ�� i �Ɋi�[����.
					pt_sub(v_pos[i], o_vec, e_side[e_cnt][0]);
					pt_add(v_pos[i], o_vec, e_side[e_cnt][1]);
					pt_add(v_pos[i + 1], o_vec, e_side[e_cnt][2]);
					pt_sub(v_pos[i + 1], o_vec, e_side[e_cnt][3]);
					e_side[e_cnt][4] = v_pos[i];
				}
				// ���ׂ�ʒu��, �g�����ꂽ�ӂɊ܂܂�邩���肷��.
				if (pt_in_poly(t_vec, 4, e_side[e_cnt++])) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// �ӂ����Ă��邩, �����ӂɒ��������邩���肷��.
			if (s_closed && s_len[d_cnt] >= FLT_MIN) {
				// �Ō�̕ӂ̈ʒu�𔽓]����, �g�����镝�̒����ɍ��킹, �Ӄx�N�g�������߂�.
				// �Ӄx�N�g���ɒ�������x�N�g�������߂�.
				// �n�_�ƏI�_�𒼌��x�N�g���ɉ����Đ��t�Ɋg����, �g�����ꂽ�ӂɊi�[����.
				D2D1_POINT_2F s_vec;
				pt_mul(v_pos[d_cnt], -e_width / s_len[d_cnt], s_vec);
				const D2D1_POINT_2F o_vec{ s_vec.y, -s_vec.x };
				pt_sub(v_pos[d_cnt], o_vec, e_side[e_cnt][0]);
				pt_add(v_pos[d_cnt], o_vec, e_side[e_cnt][1]);
				e_side[e_cnt][2] = o_vec; // v0 + o_vec
				//pt_neg(o_vec, e_side[e_cnt][3]); // v0 - o_vec
				e_side[e_cnt][3].x = -o_vec.x;
				e_side[e_cnt][3].y = -o_vec.y;
				e_side[e_cnt][4] = v_pos[d_cnt];
				// ���肷��ʒu���g�����ꂽ�ӂɊ܂܂�邩���肷��.
				if (pt_in_poly(t_vec, 4, e_side[e_cnt++])) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				if (poly_test_join_bevel(t_vec, e_cnt, s_closed, e_side)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			else if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER
				|| s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				if (poly_test_join_miter(t_vec, e_cnt, s_closed, e_width, e_side, s_limit, s_join)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			else if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				if (poly_test_join_round(t_vec, d_cnt + 1, v_pos, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
		}
		if (f_opa) {
			if (pt_in_poly(t_vec, d_cnt + 1, v_pos)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// ���̕Ԃ��Ɛ�[�̈ʒu�𓾂�.
	bool ShapePolygon::poly_get_arrow_barbs(const size_t v_cnt, const D2D1_POINT_2F v_pos[], const ARROW_SIZE& a_size, D2D1_POINT_2F& h_tip, D2D1_POINT_2F h_barbs[]) noexcept
	{
		double a_offset = a_size.m_offset;	// ���̐�[�̃I�t�Z�b�g
		for (size_t i = v_cnt - 1; i > 0; i--) {

			// ���_�Ԃ̍��������Ƃ��̒��������߂�.
			// ��̒������قڃ[�������肷��.
			// �����[���Ȃ炱�̒��_�͖�������.
			D2D1_POINT_2F a_vec;
			pt_sub(v_pos[i], v_pos[i - 1], a_vec);	// ���_�Ԃ̍���
			const auto a_len = sqrt(pt_abs2(a_vec));	// ��̒���
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
			const auto a_end = v_pos[i - 1];		// ��̏I�[
			const auto b_len = a_size.m_length;	// ���̒���
			const auto b_width = a_size.m_width;	// ���̕Ԃ��̕�
			get_arrow_barbs(a_vec, a_len, b_width, b_len, h_barbs);
			pt_mul_add(a_vec, 1.0 - a_offset / a_len, a_end, h_tip);
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
			if (s_len[i] >= FLT_MIN) {
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
			if (s_len[i] < FLT_MIN) {
				// �ӂ̒������ق� 0 �Ȃ��, �אڂ���O��̕ӂ̒�����
				// ������ 0 �łȂ��ӂ�T��, �����̖@���x�N�g����������, 
				// ���� 0 �̕ӂ̖@���x�N�g���Ƃ���.
				size_t prev;
				for (size_t j = 1; s_len[prev = ((i - j) % v_cnt)] < FLT_MIN; j++);
				size_t next;
				for (size_t j = 1; s_len[next = ((i + j) % v_cnt)] < FLT_MIN; j++);
				pt_add(n_vec[prev], n_vec[next], n_vec[i]);
				auto len = sqrt(pt_abs2(n_vec[i]));
				if (len >= FLT_MIN) {
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
	// b_vec	�̈�̏I�_�ւ̍���
	// p_opt	���p�`�̍쐬���@
	// v_pos	���_�̔z�� [v_cnt]
	void ShapePolygon::poly_by_bbox(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const POLY_OPTION& p_opt, D2D1_POINT_2F v_pos[]) noexcept//, D2D1_POINT_2F& v_vec) noexcept
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
			pt_add(v_pos[i], b_pos, v_pos[i]);
		}
	}

	// �}�`��\������.
	void ShapePolygon::draw(void)
	{
		ID2D1Factory3* const factory = Shape::s_d2d_factory;
		ID2D1RenderTarget* const target = Shape::s_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::s_d2d_color_brush;

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
			//const auto d_cnt = m_vec.size();	// �����̐�
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
			const auto f_begin = is_opaque(m_fill_color) ? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW;
			winrt::com_ptr<ID2D1GeometrySink> sink;
			winrt::check_hresult(factory->CreatePathGeometry(m_d2d_path_geom.put()));
			winrt::check_hresult(m_d2d_path_geom->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(v_pos[0], f_begin);
			for (size_t i = 1; i < v_cnt; i++) {
				sink->AddLine(v_pos[i]);
			}
			sink->EndFigure(m_end_closed ? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
			winrt::check_hresult(sink->Close());
			sink = nullptr;

			// ��邵�̌`�����Ȃ������肷��.
			const auto a_style = m_arrow_style;
			if (a_style != ARROW_STYLE::NONE) {

				// ��邵�̈ʒu�����߂�.
				D2D1_POINT_2F h_tip;
				D2D1_POINT_2F h_barbs[2];
				if (poly_get_arrow_barbs(v_cnt, v_pos, m_arrow_size, h_tip, h_barbs)) {

					// ��邵�̃p�X�W�I���g�����쐬����.
					const auto a_begin = (a_style == ARROW_STYLE::FILLED ? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
					winrt::check_hresult(factory->CreatePathGeometry(m_d2d_arrow_geom.put()));
					winrt::check_hresult(m_d2d_arrow_geom->Open(sink.put()));
					sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
					sink->BeginFigure(h_barbs[0], a_begin);
					sink->AddLine(h_tip);
					sink->AddLine(h_barbs[1]);
					sink->EndFigure(a_style == ARROW_STYLE::FILLED ? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
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
		if (is_selected()) {
			D2D1_MATRIX_3X2_F t32;
			target->GetTransform(&t32);
			D2D1_POINT_2F a_pos{ m_start };	// �}�`�̕��ʂ̈ʒu
			anc_draw_rect(a_pos, Shape::s_anc_len / t32._11, target, brush);
			const size_t d_cnt = m_vec.size();	// �����̐�
			for (size_t i = 0; i < d_cnt; i++) {
				pt_add(a_pos, m_vec[i], a_pos);
				anc_draw_rect(a_pos, Shape::s_anc_len / t32._11, target, brush);
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
	uint32_t ShapePolygon::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		D2D1_POINT_2F t_vec;
		pt_sub(t_pos, m_start, t_vec);
		return poly_hit_test(
			t_vec,
			m_vec.size(),
			m_vec.data(),
			//true,
			is_opaque(m_stroke_color),
			m_stroke_width,
			m_end_closed,
			m_stroke_cap,
			m_join_style,
			m_join_miter_limit,
			is_opaque(m_fill_color),
			a_len
		);
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// a_lt	�͈͂̍���ʒu
	// a_rb	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapePolygon::in_area(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb) const noexcept
	{
		if (!pt_in_rect(m_start, a_lt, a_rb)) {
			return false;
		}
		const size_t d_cnt = m_vec.size();	// �����̐�
		D2D1_POINT_2F e_pos = m_start;
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(e_pos, m_vec[i], e_pos);	// ���̈ʒu
			if (!pt_in_rect(e_pos, a_lt, a_rb)) {
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
	// b_pos	�͂ޗ̈�̎n�_
	// b_vec	�͂ޗ̈�̏I�_�ւ̍���
	// page	�y�[�W
	// p_opt	���p�`�̑I����
	ShapePolygon::ShapePolygon(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapePage* page, const POLY_OPTION& p_opt) :
		ShapePath::ShapePath(page, p_opt.m_end_closed),
		m_end_closed(p_opt.m_end_closed)
	{
		D2D1_POINT_2F v_pos[N_GON_MAX];

		poly_by_bbox(b_pos, b_vec, p_opt, v_pos);
		m_start = v_pos[0];
		m_vec.resize(p_opt.m_vertex_cnt - 1);
		m_vec.shrink_to_fit();
		for (size_t i = 1; i < p_opt.m_vertex_cnt; i++) {
			pt_sub(v_pos[i], v_pos[i - 1], m_vec[i - 1]);
		}
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	// dt_reader	�f�[�^���[�_�[
	ShapePolygon::ShapePolygon(const ShapePage& page, DataReader const& dt_reader) :
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