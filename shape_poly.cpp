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
	// ���p�`�̕ӂ̖@���x�N�g�������߂�.
	static bool poly_get_nvec(const size_t v_cnt, const D2D1_POINT_2F v_pos[], D2D1_POINT_2F n_vec[]) noexcept;

	// �܂���̖��̈ʒu�𓾂�


	// ���s����x�N�g���𓾂�.
	static D2D1_POINT_2F poly_pt_orth(const D2D1_POINT_2F vec) { return { -vec.y, vec.x }; }

	// ���p�`�̕ӂ��ʒu���܂ނ����肷��.
	static bool poly_test_side(const D2D1_POINT_2F t_pos, const size_t v_cnt, const bool v_end, const D2D1_POINT_2F v_pos[], const D2D1_POINT_2F n_vec[], const double s_width, D2D1_POINT_2F exp_side[][4]) noexcept;

	// ���p�`�̊p���ʒu���܂ނ����肷��.
	//static bool poly_test_join_miter(const D2D1_POINT_2F t_pos, const size_t exp_cnt, const bool exp_end, const D2D1_POINT_2F exp_side[], const D2D1_POINT_2F n_vec[], const double ext_len) noexcept;

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
	static bool poly_get_nvec(const size_t v_cnt, const D2D1_POINT_2F v_pos[], D2D1_POINT_2F n_vec[]) noexcept
	{
		// ���p�`�̊e�ӂ̒����Ɩ@���x�N�g��, 
		// �d�����Ȃ����_�̐������߂�.
		//std::vector<double> side_len(v_cnt);	// �e�ӂ̒���
		//const auto s_len = reinterpret_cast<double*>(side_len.data());
		double s_len[MAX_GON];	// �e�ӂ̒���
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

	// ���p�`�̊p���ʒu���܂ނ����肷�� (�ۂ܂������_)
	static bool poly_test_join_round(const D2D1_POINT_2F& t_pos, const size_t v_cnt, const D2D1_POINT_2F v_pos[], const double exp_width)
	{
		for (size_t i = 0; i < v_cnt; i++) {
			D2D1_POINT_2F diff;
			pt_sub(t_pos, v_pos[i], diff);
			if (pt_abs2(diff) <= exp_width * exp_width) {
				return true;
			}
		}
		return false;
	}

	static bool poly_test_join_bevel(const D2D1_POINT_2F& t_pos, const size_t v_cnt, const bool v_end, const D2D1_POINT_2F v_pos[], const double e_width, const D2D1_POINT_2F exp_side[][4]) noexcept
	{
		for (size_t i = (v_end ? v_cnt - 1 : 0), j = (v_end ? 0 : 1); j < v_cnt; i = j++) {
			if (equal(v_pos[i], v_pos[j])) {
				D2D1_POINT_2F bev_pos[4];
				const D2D1_POINT_2F v_min{ static_cast<FLOAT>(v_pos[i].x - e_width), static_cast<FLOAT>(v_pos[i].y - e_width) };
				const D2D1_POINT_2F v_max{ static_cast<FLOAT>(v_pos[i].x + e_width), static_cast<FLOAT>(v_pos[i].y + e_width) };
				//pt_add(v_pos[i], -e_width, -e_width, v_min);
				//pt_add(v_pos[i], e_width, e_width, v_max);
				if (!pt_in_rect(t_pos, v_min, v_max)) {
					continue;
				}
				// �g�������ӂ�, ���̕ӂɕ��s�ȕ����ɉ�������.
				const D2D1_POINT_2F exp_para{ (exp_side[i][1].y - exp_side[i][0].y) * 0.5f, -(exp_side[i][1].x - exp_side[i][0].x) * 0.5f };
				//pt_sub(exp_side[i][1], exp_side[i][0], exp_diff);
				//pt_orth(exp_diff. exp_orth);
				//pt_mul(exp_orth, 0.5, exp_para);
				pt_sub(exp_side[i][0], exp_para, bev_pos[0]);
				pt_sub(exp_side[i][1], exp_para, bev_pos[1]);
				pt_add(exp_side[i][2], exp_para, bev_pos[2]);
				pt_add(exp_side[i][3], exp_para, bev_pos[3]);
				if (pt_in_poly(t_pos, 4, bev_pos)) {
					return true;
				}
			}
			else {
				const D2D1_POINT_2F bev_pos[4]{ exp_side[i][3], exp_side[j][0], exp_side[j][1], exp_side[i][2] };
				if (pt_in_poly(t_pos, 4, bev_pos)) {
					return true;
				}
			}
		}
		return false;
	}

	// ���p�`�̕ӂ��ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// v_cnt	���_�̐�
	// v_end	�ӂ����Ă��邩����
	// v_pos	���_�̔z�� [v_cnt]
	// n_vec	�e�ӂ̖@���x�N�g���̔z�� [v_cnt]
	// exp_width	�ӂ̑����̔����̒l
	// exp_side	�g�������ӂ̔z�� [v_cnt][4]
	static bool poly_test_side(const D2D1_POINT_2F t_pos, const size_t v_cnt, const bool v_end, const D2D1_POINT_2F v_pos[], const D2D1_POINT_2F n_vec[], const double exp_width, D2D1_POINT_2F exp_side[][4]) noexcept
	{
		const auto cnt = (v_end ? v_cnt : v_cnt - 1);
		for (size_t i = 0; i < cnt; i++) {
			D2D1_POINT_2F nor;
			pt_mul(n_vec[i], exp_width, nor);
			// ���Ƃ̕ӂ̕Е��̒[�_��, �@���x�N�g���ɂ����Đ��t�̗������Ɉړ���, ����ꂽ�_��, �g�������ӂɊi�[����.
			pt_sub(v_pos[i], nor, exp_side[i][0]);
			pt_add(v_pos[i], nor, exp_side[i][1]);
			// ��������̒[�_��, �����悤�ɂ���, �g�������ӂ�����������.
			const auto j = (i + 1) % v_cnt;
			pt_add(v_pos[j], nor, exp_side[i][2]);
			pt_sub(v_pos[j], nor, exp_side[i][3]);
			// �ʒu���g�������ӂɊ܂܂�邩���肷��.
			if (pt_in_poly(t_pos, 4, exp_side[i])) {
				// �܂܂��Ȃ� true ��Ԃ�.
				return true;
			}
		}
		/*
		D2D1_POINT_2F bev_pos[4];
		for (size_t i = (v_end ? v_cnt - 1 : 0), j = (v_end ? 0 : 1); j < v_cnt; i = j++) {
			if (equal(v_pos[i], v_pos[j])) {
				// �g�������ӂ�, ���̕ӂɕ��s�ȕ����ɉ�������.
				const D2D1_POINT_2F exp_para{ (exp_side[i][1].y - exp_side[i][0].y) * 0.5f, -(exp_side[i][1].x - exp_side[i][0].x) * 0.5f };
				//pt_sub(exp_side[i][1], exp_side[i][0], exp_diff);
				//pt_orth(exp_diff. exp_orth);
				//pt_mul(exp_orth, 0.5, exp_para);
				pt_sub(exp_side[i][0], exp_para, bev_pos[0]);
				pt_sub(exp_side[i][1], exp_para, bev_pos[1]);
				pt_add(exp_side[i][2], exp_para, bev_pos[2]);
				pt_add(exp_side[i][3], exp_para, bev_pos[3]);
				if (pt_in_poly(t_pos, 4, bev_pos)) {
					return true;
				}
				continue;
			}
			bev_pos[0] = exp_side[i][3];
			bev_pos[1] = exp_side[j][0];
			bev_pos[2] = exp_side[j][1];
			bev_pos[3] = exp_side[i][2];
			if (pt_in_poly(t_pos, 4, bev_pos)) {
				return true;
			}
		}
		*/
		return false;
	}

	// �����̌�_��, ���̏��ϐ������߂�.
	static bool poly_intersection(const D2D1_POINT_2F a, const D2D1_POINT_2F b, const D2D1_POINT_2F c, const D2D1_POINT_2F d, double& s, double& t, D2D1_POINT_2F& e)
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
		if (fabs(r) < FLT_MIN) {
			return false;
		}
		s = (cd_y * ac_x - cd_x * ac_y) / r;
		t = (ab_y * ac_x - ab_x * ac_y) / r;
		e.x = static_cast<FLOAT>(ax + ab_x * s);
		e.y = static_cast<FLOAT>(ay + ab_y * s);
		return true;
	}

	// ���p�`�̊p���ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// exp_cnt	�g�������ӂ̐�
	// exp_end	�g�������ӂ����Ă��邩����
	// v_pos	���_�̔z�� [exp_cnt]
	// exp_width	�ӂ̑����̔���.
	// exp_side	�g�������ӂ̔z�� [exp_cnt][4]
	// mit_limit	�}�C�^�[������
	// line_join	�����̘A�����@
	static bool poly_test_join_miter(const D2D1_POINT_2F t_pos, const size_t exp_cnt, const bool exp_end, const D2D1_POINT_2F v_pos[], const double exp_width, const D2D1_POINT_2F exp_side[][4], const double mit_limit, const D2D1_LINE_JOIN line_join) noexcept
	{
		D2D1_POINT_2F c_pos[4 + 1];
		for (size_t i = (exp_end ? 0 : 1), j = (exp_end ? exp_cnt - 1 : 0); i < exp_cnt; j = i++) {
			// �g�����ꂽ�ӂɂ��Ċp�̕��������߂�.
			//
			// �_ vi �ŘA������, �g�����ꂽ�� j �� i �̃C���[�W
			// j0                     j3      i0                     i3
			//  +---expanded side[j]---+  vi   +---expanded side[i]---+ 
			// j1                     j2      i1                     i2
			//
			// �� j �� i �����s�����肷��.
			if (equal(exp_side[j][3], exp_side[i][0])) {
				// ���s�Ȃ�Ίp�͂Ȃ�.
				continue;
			}
			// �� j �� i ���d�Ȃ邩���肷��.
			if (equal(exp_side[j][3], exp_side[i][1])) {
				// �d�Ȃ�Ȃ��, �p�͖������ɂȂ邪, �}�C�^�[�����ł����f���؂�.
				// �x�N�g�� vi j3 �����߂�.
				D2D1_POINT_2F vij3;
				pt_sub(exp_side[j][3], v_pos[i], vij3);
				// ���߂��x�N�g���Ƀ}�C�^�[�����̔䗦���|��, �����x�N�g�������߂�.
				D2D1_POINT_2F ext_vec;
				pt_mul(poly_pt_orth(vij3), mit_limit, ext_vec);
				// �_ j3 �� j2 �������x�N�g��������, ���ꂼ��_ e3 �� e2 �����߂�.
				D2D1_POINT_2F e3;
				D2D1_POINT_2F e2;
				pt_add(exp_side[j][3], ext_vec, e3);
				pt_add(exp_side[j][2], ext_vec, e2);
				// �l�ӌ` { j3, j2, e2, e3 } �Ɉʒu���܂܂�邩���肷��.
				D2D1_POINT_2F ext_side[4]{ exp_side[j][3], exp_side[j][2], e2, e3};
				if (pt_in_poly(t_pos, 4, ext_side)) {
					return true;
				}
				continue;
			}
			// j0 �� j3 ���������W (�܂蒸�_ v[i-1] �� v[i] ������) �ɂȂ�ꍇ������̂�,
			// j2 j3 �ɒ��s����x�N�g��������, ��������Ƃ� j0 �� j1 �Ƃ���.
			// i0 �� i3 ���������W (�܂蒸�_ v[i] �� v[i+1] ������) �ꍇ��, 
			// ���l�ɋ���, ������ i3 �� i2 �Ƃ���.
			D2D1_POINT_2F j0;
			pt_sub(exp_side[j][2], exp_side[j][3], j0);
			pt_add(exp_side[j][3], poly_pt_orth(j0), j0);
			D2D1_POINT_2F i3;
			pt_sub(exp_side[i][1], exp_side[i][0], i3);
			pt_sub(exp_side[i][0], poly_pt_orth(i3), i3);
			// ���� j0 j3 �� i0 i3 (��������� 0..3 ���ƌĂ�) �Ƃ̌�_�����߂�.
			// 0..3 ���Ɍ�_���Ȃ��Ȃ��, ���̕ӂ̑g������.
			// 0..3 ���Ɍ�_�������Đ��� j0 j3 �� i0 i3 �̊O���Ȃ��, c[4] { ��_, j3, v[i], i0 } �������ׂ��p�̕���. 
			// ���� j1 j2 �� i1 i2 (1..2 ���ƌĂ�) �Ƃ̌�_�����߂�.
			// 1..2 ���Ɍ�_���Ȃ�, �܂���, ��_�������Ă����� j1..2 �� i1..2 �̊O���łȂ��Ȃ��, ���̕ӂ̑g������.
			// 1..2 ���Ɍ�_�������Đ��� j1 j2 �� i1 i2 �̊O���Ȃ��, c[4] { ��_, j2, v[i], i1 } �������ׂ��p�̕���.
			double s, t;	// ��_�̏��ϐ�
			if (!poly_intersection(j0, exp_side[j][3], exp_side[i][0], i3, s, t, c_pos[0])) {
			//if (!poly_intersection(exp_side[j][0], exp_side[j][3], exp_side[i][0], exp_side[i][3], s, t, c_pos[0])) {
				continue;
			}
			// �����̓����Ō������邩���肷��.
			if (s < 1.0 || t > 0.0) {
				D2D1_POINT_2F j1;
				pt_sub(exp_side[j][3], exp_side[j][2], j1);
				pt_sub(exp_side[j][2], poly_pt_orth(j1), j1);
				D2D1_POINT_2F i2;
				pt_sub(exp_side[i][0], exp_side[i][1], i2);
				pt_add(exp_side[i][1], poly_pt_orth(i2), i2);
				if (!poly_intersection(j1, exp_side[j][2], exp_side[i][1], i2, s, t, c_pos[0])) {
				//if (!poly_intersection(exp_side[j][1], exp_side[j][2], exp_side[i][1], exp_side[i][2], s, t, c_pos[0])) {
					continue;
				}
				if (s < 1.0 || t > 0.0) {
					continue;
				}
				c_pos[1] = exp_side[j][2];
				c_pos[2] = v_pos[i];
				c_pos[3] = exp_side[i][1];
			}
			else {
				c_pos[1] = exp_side[i][0];
				c_pos[2] = v_pos[i];
				c_pos[3] = exp_side[j][3];
			}

			// ���_��, �����̌�_�Ƃ̍���������, 
			// �����̒�����, �}�C�^�[�����ȉ���, ���肷��.
			D2D1_POINT_2F diff;
			pt_sub(c_pos[0], v_pos[i], diff);	// ���_��, �����̌�_�̍���
			const double d_abs2 = pt_abs2(diff);
			const double limit_len = exp_width * mit_limit;
			if (d_abs2 <= limit_len * limit_len) {
				// �l�ӌ` { c0, c1, c2, c3 } ���ʒu t ���܂ނ����肷��.
				if (pt_in_poly(t_pos, 4, c_pos)) {
					return true;
				}
				continue;
			}

			// �����̒������}�C�^�[�����𒴂�, �������̘A�����@�� miter or bevel �����肷��.
			if (line_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				// �O�p�` { c1, c2, c3 } ���ʒu t ���܂ނ����肷��.
				if (pt_in_poly(t_pos, 3, c_pos + 1)) {
					return true;
				}
				continue;
			}

			// �}�C�^�[�����̓_ m �����߂�.
			D2D1_POINT_2F mit_pos;
			pt_mul(diff, limit_len / sqrt(d_abs2), v_pos[i], mit_pos);
			// �_ m ��ʂ�, �����ɒ�������_ n �����߂�.
			// ���� c3 c0 �� m n �Ƃ̌�_ c4 �����߂�.
			// ���� c0 c1 �� m n �Ƃ̌�_������, �����V���� c0 �Ƃ���.
			// �܊p�` { c0, c1, c2, c3, c4 } ���ʒu t ���܂ނ����肷��.
			D2D1_POINT_2F nor_pos;
			pt_add(mit_pos, D2D1_POINT_2F{ diff.y, -diff.x }, nor_pos);
			poly_intersection(c_pos[3], c_pos[0], mit_pos, nor_pos, s, t, c_pos[4]);
			poly_intersection(c_pos[0], c_pos[1], mit_pos, nor_pos, s, t, c_pos[0]);
			if (pt_in_poly(t_pos, 5, c_pos)) {
				return true;
			}
		}
		return false;
	}

	// ���p�`�̊p���ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// exp_cnt	�g�������ӂ̐�
	// exp_end	�g�������ӂ����Ă��邩����
	// exp_side	�g�������ӂ̔z�� [exp_cnt][4]
	// n_vec	�e�ӂ̖@���x�N�g���̔z�� [exp_cnt]
	// ext_len	�p�𒴂��ĉ������钷��
	static bool poly_test_join_miter2(const D2D1_POINT_2F t_pos, const size_t exp_cnt, const bool exp_end, const D2D1_POINT_2F exp_side[][4], const D2D1_POINT_2F n_vec[], const double ext_len) noexcept
	{
		D2D1_POINT_2F ext_side[4];	// �g�������ӂ�����ɉ���������
		D2D1_POINT_2F ext_vec;	// �g�������ӂɕ��s�ȃx�N�g��

		for (size_t i = (exp_end ? 0 : 1), j = (exp_end ? exp_cnt - 1 : 0); i < exp_cnt; j = i++) {
			// ���钸�_�ɗאڂ���ӂɂ���.
			// �g�������ӂ̈���̒[��, ���������ӂɊi�[����.
			ext_side[0] = exp_side[j][3];
			ext_side[1] = exp_side[j][2];
			// �@���x�N�g���ƒ��s����x�N�g����,
			// �������钷���̕������{��,
			// ���s�ȃx�N�g���Ɋi�[����.
			pt_mul(poly_pt_orth(n_vec[j]), ext_len, ext_vec);
			// �i�[�����ʒu�𕽍s�ȃx�N�g���ɉ����ĉ�����,
			// ���������ӂɊi�[����.
			pt_sub(ext_side[1], ext_vec, ext_side[2]);
			pt_sub(ext_side[0], ext_vec, ext_side[3]);
			// �ʒu�����������ӂɊ܂܂�邩���肷��.
			if (pt_in_poly(t_pos, 4, ext_side) != true) {
				// �܂܂�Ȃ��Ȃ�p������.
				continue;
			}
			// �אڂ�������Е��̕ӂɂ���.
			// �g�������ӂ̒[��, ���������ӂ̒[�Ɋi�[����.
			ext_side[2] = exp_side[i][1];
			ext_side[3] = exp_side[i][0];
			// �@���x�N�g���ƒ��s����x�N�g�� (��قǂƂ͋t����) �𓾂�,
			// �p���������钷���̕������{��,
			// ���s�ȃx�N�g���Ɋi�[����.
			pt_mul(poly_pt_orth(n_vec[i]), ext_len, ext_vec);
			// �i�[�����ʒu�𕽍s�ȃx�N�g���ɉ����ĉ�����,
			// ���������ӂ̂�������̒[�Ɋi�[����.
			pt_add(ext_side[3], ext_vec, ext_side[0]);
			pt_add(ext_side[2], ext_vec, ext_side[1]);
			// �ʒu�����������ӂɊ܂܂�邩���肷��.
			if (pt_in_poly(t_pos, 4, ext_side)) {
				// �܂܂��Ȃ� true ��Ԃ�.
				return true;
			}
		}
		return false;
	}

	// �̈�����Ƃɑ��p�`���쐬����.
	// b_pos	�̈�̊J�n�ʒu
	// b_diff	�̈�̏I���ʒu�ւ̍���
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
		D2D1_POINT_2F v_pos[MAX_GON];
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
		// Shape ��Ŏn�_�ƏI�_���d�˂��Ƃ�,
		// �p�X�Ɏn�_�������Ȃ���, LINE_JOINT ���ւ�Ȃ��ƂɂȂ�.
		//if (m_end_closed) {
		//	sink->AddLine(v_pos[0]);
		//}
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
		/*
		double b_offset = m_arrow_size.m_offset;	// ����[�̃I�t�Z�b�g
		for (size_t i = d_cnt; i > 0; i--) {

			// ���������Ƃ��̒��������߂�.
			// ��̒������قڃ[�������肷��.
			const auto a_vec = m_diff[i - 1];	// ��̃x�N�g��	
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
			const auto b_len = m_arrow_size.m_length;	// ���̒���
			const auto b_width = m_arrow_size.m_width;	// ���̕�
			D2D1_POINT_2F barbs[2];	// ���̕Ԃ��̈ʒu
			get_arrow_barbs(a_vec, a_len, b_width, b_len, barbs);
			D2D1_POINT_2F a_tip;	// ���̐�[
			pt_mul(a_vec, 1.0 - b_offset / a_len, a_end, a_tip);
			pt_add(barbs[0], a_tip, barbs[0]);
			pt_add(barbs[1], a_tip, barbs[1]);

			// ���̃p�X�W�I���g�����쐬����.
			winrt::check_hresult(d_factory->CreatePathGeometry(m_d2d_arrow_geom.put()));
			winrt::check_hresult(m_d2d_arrow_geom->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(barbs[0], a_style == ARROWHEAD_STYLE::FILLED ? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
			sink->AddLine(a_tip);
			sink->AddLine(barbs[1]);
			sink->EndFigure(a_style == ARROWHEAD_STYLE::FILLED ? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
			winrt::check_hresult(sink->Close());
			sink = nullptr;
			break;
		}
		*/
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
			const auto s_style = m_d2d_stroke_dash_style.get();
			s_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawGeometry(p_geom, s_brush, s_width, s_style);
			if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
				const auto a_geom = m_d2d_arrow_geom.get();
				if (a_geom != nullptr) {
					dx.m_d2dContext->FillGeometry(a_geom, s_brush, nullptr);
					if (m_arrow_style != ARROWHEAD_STYLE::FILLED) {
						dx.m_d2dContext->DrawGeometry(a_geom, s_brush, s_width, s_style);
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

	static bool hit_test_stroke(const D2D1_POINT_2F t_pos, const double e_width, const size_t v_cnt, const D2D1_POINT_2F v_pos[], const bool v_closed, const D2D1_LINE_JOIN v_join, const double v_limit)
	{
		//std::vector<D2D1_POINT_2F> nor_vec(v_cnt);	// �@���x�N�g��
		//const auto n_vec = reinterpret_cast<D2D1_POINT_2F*>(nor_vec.data());
		D2D1_POINT_2F n_vec[MAX_GON];
		//if (!poly_get_nvec(v_cnt, v_pos, n_vec.data())) {
		if (!poly_get_nvec(v_cnt, v_pos, n_vec)) {
			D2D1_POINT_2F diff;
			pt_sub(t_pos, v_pos[0], diff);
			return pt_abs2(diff) <= e_width * e_width;
		}

		//std::vector<D2D1_POINT_2F> exp_side(v_cnt * 4);	// ��������
		//const auto e_side = reinterpret_cast<D2D1_POINT_2F(*)[4]>(exp_side.data());
		D2D1_POINT_2F e_side[MAX_GON][4];
		if (poly_test_side(t_pos, v_cnt, v_closed, v_pos, n_vec, e_width, e_side)) {
			return true;
		}
		else if (v_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
			if (poly_test_join_round(t_pos, v_cnt, v_pos, e_width)) {
				return true;
			}
		}
		else if (v_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
			if (poly_test_join_bevel(t_pos, v_cnt, v_closed, v_pos, e_width, e_side)) {
				return true;
			}
		}
		else if (v_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER
			|| v_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
			//if (poly_test_join_miter2(tp, v_cnt, m_end_closed, q_exp.data(), n_vec.data(), e_width * m_stroke_join_limit)) {
			if (poly_test_join_miter(t_pos, v_cnt, v_closed, v_pos, e_width, e_side, v_limit, v_join)) {
				return true;
			}
		}
		return false;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// a_len	���ʂ̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapePoly::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		constexpr D2D1_POINT_2F PZ{ 0.0f, 0.0f };	// ��_
		const size_t v_cnt = m_diff.size() + 1;	// ���_�̐� (�����̐� + 1)
		//std::vector<D2D1_POINT_2F> vert_pos(v_cnt);	// ���_�̔z��
		//const auto v_pos = reinterpret_cast<D2D1_POINT_2F*>(vert_pos.data());
		D2D1_POINT_2F v_pos[MAX_GON];	// ���_�̔z��
		size_t j = static_cast<size_t>(-1);	// �_���܂ޒ��_�̓Y����
		// 
		D2D1_POINT_2F tp;
		pt_sub(t_pos, m_pos, tp);
		v_pos[0].x = 0.0;
		v_pos[0].y = 0.0;
		if (pt_in_anch(tp, v_pos[0], a_len)) {
			j = 0;
		}
		for (size_t i = 1; i < v_cnt; i++) {
			pt_add(v_pos[i - 1], m_diff[i - 1], v_pos[i]);
			if (pt_in_anch(tp, v_pos[i], a_len)) {
				j = i;
			}
		}
		if (j != -1) {
			const auto anch = ANCH_TYPE::ANCH_P0 + j;
			return static_cast<uint32_t>(anch);
		}

		if (is_opaque(m_stroke_color)) {
			const auto e_width = max(max(static_cast<double>(m_stroke_width), a_len) * 0.5, 0.5);	// �g�����镝
			const bool v_closed = m_end_closed;
			const auto v_style = m_stroke_join_style;
			const auto v_limit = m_stroke_join_limit;
			if (hit_test_stroke(tp, e_width, v_cnt, v_pos, v_closed, v_style, v_limit)) {
				return ANCH_TYPE::ANCH_STROKE;
			}
			/*
			// �e�ӂ̖@���x�N�g���𓾂�.
			std::vector<D2D1_POINT_2F> n_vec(v_cnt);	// �@���x�N�g��
			poly_get_nvec(v_cnt, v_pos.data(), n_vec.data());

			// ���p�`�̊e�ӂ��ʒu���܂ނ����肷��.
			std::vector<D2D1_POINT_2F> q_exp(v_cnt * 4);	// ��������
			const auto e_side = reinterpret_cast<D2D1_POINT_2F(*)[4]>(q_exp.data());
			if (poly_test_side(tp, v_cnt, m_end_closed, v_pos.data(), n_vec.data(), e_width, e_side)) {
				return ANCH_TYPE::ANCH_STROKE;
			}
			else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				if (poly_test_join_round(tp, v_cnt, v_pos.data(), e_width)) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
			else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				if (poly_test_join_bevel(tp, v_cnt, m_end_closed, v_pos.data(), e_side)) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
			else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER
			|| m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				//if (poly_test_join_miter2(tp, v_cnt, m_end_closed, q_exp.data(), n_vec.data(), e_width * m_stroke_join_limit)) {
				if (poly_test_join_miter(tp, v_cnt, m_end_closed, e_side, v_pos.data(), e_width, m_stroke_join_limit, m_stroke_join_style)) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
			*/
		}

		// �ӂ��s����, �܂��͈ʒu���ӂɊ܂܂�Ă��Ȃ��Ȃ�,
		// �h��Ԃ��F���s���������肷��.
		if (is_opaque(m_fill_color)) {
			if (pt_in_poly(tp, v_cnt, v_pos)) {
				// �܂܂��Ȃ� ANCH_FILL ��Ԃ�.
				return ANCH_TYPE::ANCH_FILL;
			}
		}
		return ANCH_TYPE::ANCH_SHEET;
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
		D2D1_POINT_2F v_pos[MAX_GON];

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