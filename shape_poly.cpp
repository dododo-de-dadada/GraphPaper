//------------------------------
// Shape_poly.cpp
// 多角形
//------------------------------
#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 多角形の辺の法線ベクトルを求める.
	static bool poly_get_nvec(const size_t v_cnt, const D2D1_POINT_2F v_pos[], D2D1_POINT_2F n_vec[]) noexcept;

	// 折れ線の矢じりの位置を得る


	// 直行するベクトルを得る.
	static D2D1_POINT_2F poly_pt_orth(const D2D1_POINT_2F vec) { return { -vec.y, vec.x }; }

	// 多角形の辺が位置を含むか判定する.
	static bool poly_test_side(const D2D1_POINT_2F t_pos, const size_t v_cnt, const bool v_end, const D2D1_POINT_2F v_pos[], const D2D1_POINT_2F n_vec[], const double s_width, D2D1_POINT_2F exp_side[][4]) noexcept;

	// 多角形の角が位置を含むか判定する.
	//static bool poly_test_join_miter(const D2D1_POINT_2F t_pos, const size_t exp_cnt, const bool exp_end, const D2D1_POINT_2F exp_side[], const D2D1_POINT_2F n_vec[], const double ext_len) noexcept;

	static bool poly_get_arrow_barbs(const size_t v_cnt, const D2D1_POINT_2F v_pos[], const ARROWHEAD_SIZE& a_size, D2D1_POINT_2F& h_tip, D2D1_POINT_2F h_barbs[]) noexcept
	{
		double b_offset = a_size.m_offset;	// 矢じり先端のオフセット
		for (size_t i = v_cnt - 1; i > 0; i--) {
			D2D1_POINT_2F a_vec;
			pt_sub(v_pos[i], v_pos[i - 1], a_vec);
			// 差分から矢軸とその長さを求める.
			// 矢軸の長さがほぼゼロか判定する.
			//const auto a_vec = m_diff[i - 1];	// 矢軸のベクトル
			const auto a_len = sqrt(pt_abs2(a_vec));	// 矢軸の長さ
			if (a_len < FLT_MIN) {
				continue;
			}

			// 矢軸の長さが矢じり先端のオフセットより短いか判定する.
			if (a_len < b_offset) {
				// 次の差分があるか判定する.
				if (i > 1) {
					// オフセットを矢軸の長さだけ短くする.
					b_offset -= a_len;
					continue;
				}
				b_offset = a_len;
			}

			// 矢じりの返しの位置を求める.
			const auto a_end = v_pos[i - 1];		// 矢軸の終端
			const auto b_len = a_size.m_length;	// 矢じりの長さ
			const auto b_width = a_size.m_width;	// 矢じりの幅
			get_arrow_barbs(a_vec, a_len, b_width, b_len, h_barbs);
			pt_mul(a_vec, 1.0 - b_offset / a_len, a_end, h_tip);
			pt_add(h_barbs[0], h_tip, h_barbs[0]);
			pt_add(h_barbs[1], h_tip, h_barbs[1]);
			return true;
		}
		return false;
	}

	// 多角形の各辺の法線ベクトルを求める.
	// v_cnt	頂点の数
	// v_pos	頂点の配列 [v_cnt]
	// n_vec	各辺の法線ベクトルの配列 [v_cnt]
	// 戻り値	法線ベクトルを求めたなら true, すべての頂点が重なっていたなら false
	static bool poly_get_nvec(const size_t v_cnt, const D2D1_POINT_2F v_pos[], D2D1_POINT_2F n_vec[]) noexcept
	{
		// 多角形の各辺の長さと法線ベクトル, 
		// 重複しない頂点の数を求める.
		//std::vector<double> side_len(v_cnt);	// 各辺の長さ
		//const auto s_len = reinterpret_cast<double*>(side_len.data());
		double s_len[MAX_GON];	// 各辺の長さ
		int q_cnt = 1;
		for (size_t i = 0; i < v_cnt; i++) {
			// 次の頂点との差分を求める.
			D2D1_POINT_2F q_sub;
			pt_sub(v_pos[(i + 1) % v_cnt], v_pos[i], q_sub);
			// 差分の長さを求める.
			s_len[i] = sqrt(pt_abs2(q_sub));
			if (s_len[i] > FLT_MIN) {
				// 差分の長さが 0 より大きいなら, 
				// 重複しない頂点の数をインクリメントする.
				q_cnt++;
			}
			// 差分と直行するベクトルを正規化して法線ベクトルに格納する.
			pt_mul(poly_pt_orth(q_sub), 1.0 / s_len[i], n_vec[i]);
		}
		if (q_cnt == 1) {
			// すべての頂点が重なったなら, false を返す.
			return false;
		}
		for (size_t i = 0; i < v_cnt; i++) {
			if (s_len[i] <= FLT_MIN) {
				// 辺の長さがほぼ 0 ならば, 隣接する前後の辺の中から
				// 長さが 0 でない辺を探し, それらの法線ベクトルを合成し, 
				// 長さ 0 の辺の法線ベクトルとする.
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
					// 合成ベクトルがゼロベクトルになるなら,
					// 前方の隣接する辺の法線ベクトルに直交するベクトルを法線ベクトルとする.
					n_vec[i] = poly_pt_orth(n_vec[prev]);
				}
			}
		}
		return true;
	}

	// 多角形の角が位置を含むか判定する (丸まった頂点)
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
				// 拡張した辺を, その辺に平行な方向に延長する.
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

	// 多角形の辺が位置を含むか判定する.
	// t_pos	判定する位置
	// v_cnt	頂点の数
	// v_end	辺が閉じているか判定
	// v_pos	頂点の配列 [v_cnt]
	// n_vec	各辺の法線ベクトルの配列 [v_cnt]
	// exp_width	辺の太さの半分の値
	// exp_side	拡張した辺の配列 [v_cnt][4]
	static bool poly_test_side(const D2D1_POINT_2F t_pos, const size_t v_cnt, const bool v_end, const D2D1_POINT_2F v_pos[], const D2D1_POINT_2F n_vec[], const double exp_width, D2D1_POINT_2F exp_side[][4]) noexcept
	{
		const auto cnt = (v_end ? v_cnt : v_cnt - 1);
		for (size_t i = 0; i < cnt; i++) {
			D2D1_POINT_2F nor;
			pt_mul(n_vec[i], exp_width, nor);
			// もとの辺の片方の端点を, 法線ベクトルにそって正逆の両方向に移動し, 得られた点を, 拡張した辺に格納する.
			pt_sub(v_pos[i], nor, exp_side[i][0]);
			pt_add(v_pos[i], nor, exp_side[i][1]);
			// もう一方の端点も, 同じようにして, 拡張した辺を完成させる.
			const auto j = (i + 1) % v_cnt;
			pt_add(v_pos[j], nor, exp_side[i][2]);
			pt_sub(v_pos[j], nor, exp_side[i][3]);
			// 位置が拡張した辺に含まれるか判定する.
			if (pt_in_poly(t_pos, 4, exp_side[i])) {
				// 含まれるなら true を返す.
				return true;
			}
		}
		/*
		D2D1_POINT_2F bev_pos[4];
		for (size_t i = (v_end ? v_cnt - 1 : 0), j = (v_end ? 0 : 1); j < v_cnt; i = j++) {
			if (equal(v_pos[i], v_pos[j])) {
				// 拡張した辺を, その辺に平行な方向に延長する.
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

	// 線分の交点と, その助変数を求める.
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

	// 多角形の角が位置を含むか判定する.
	// t_pos	判定する位置
	// exp_cnt	拡張した辺の数
	// exp_end	拡張した辺が閉じているか判定
	// v_pos	頂点の配列 [exp_cnt]
	// exp_width	辺の太さの半分.
	// exp_side	拡張した辺の配列 [exp_cnt][4]
	// mit_limit	マイター制限比
	// line_join	線分の連結方法
	static bool poly_test_join_miter(const D2D1_POINT_2F t_pos, const size_t exp_cnt, const bool exp_end, const D2D1_POINT_2F v_pos[], const double exp_width, const D2D1_POINT_2F exp_side[][4], const double mit_limit, const D2D1_LINE_JOIN line_join) noexcept
	{
		D2D1_POINT_2F c_pos[4 + 1];
		for (size_t i = (exp_end ? 0 : 1), j = (exp_end ? exp_cnt - 1 : 0); i < exp_cnt; j = i++) {
			// 拡張された辺について角の部分を求める.
			//
			// 点 vi で連結する, 拡張された辺 j と i のイメージ
			// j0                     j3      i0                     i3
			//  +---expanded side[j]---+  vi   +---expanded side[i]---+ 
			// j1                     j2      i1                     i2
			//
			// 辺 j と i が平行か判定する.
			if (equal(exp_side[j][3], exp_side[i][0])) {
				// 平行ならば角はない.
				continue;
			}
			// 辺 j と i が重なるか判定する.
			if (equal(exp_side[j][3], exp_side[i][1])) {
				// 重なるならば, 角は無限遠になるが, マイター制限でこれを断ち切る.
				// ベクトル vi j3 を求める.
				D2D1_POINT_2F vij3;
				pt_sub(exp_side[j][3], v_pos[i], vij3);
				// 求めたベクトルにマイター制限の比率を掛け, 延長ベクトルを求める.
				D2D1_POINT_2F ext_vec;
				pt_mul(poly_pt_orth(vij3), mit_limit, ext_vec);
				// 点 j3 と j2 を延長ベクトルを加え, それぞれ点 e3 と e2 を求める.
				D2D1_POINT_2F e3;
				D2D1_POINT_2F e2;
				pt_add(exp_side[j][3], ext_vec, e3);
				pt_add(exp_side[j][2], ext_vec, e2);
				// 四辺形 { j3, j2, e2, e3 } に位置が含まれるか判定する.
				D2D1_POINT_2F ext_side[4]{ exp_side[j][3], exp_side[j][2], e2, e3};
				if (pt_in_poly(t_pos, 4, ext_side)) {
					return true;
				}
				continue;
			}
			// j0 と j3 が同じ座標 (つまり頂点 v[i-1] と v[i] が同じ) になる場合もあるので,
			// j2 j3 に直行するベクトルを求め, これをもとに j0 と j1 とする.
			// i0 と i3 が同じ座標 (つまり頂点 v[i] と v[i+1] が同じ) 場合も, 
			// 同様に求め, これらを i3 と i2 とする.
			D2D1_POINT_2F j0;
			pt_sub(exp_side[j][2], exp_side[j][3], j0);
			pt_add(exp_side[j][3], poly_pt_orth(j0), j0);
			D2D1_POINT_2F i3;
			pt_sub(exp_side[i][1], exp_side[i][0], i3);
			pt_sub(exp_side[i][0], poly_pt_orth(i3), i3);
			// 線分 j0 j3 と i0 i3 (これを仮に 0..3 側と呼ぶ) との交点を求める.
			// 0..3 側に交点がないならば, 次の辺の組を試す.
			// 0..3 側に交点があって線分 j0 j3 と i0 i3 の外側ならば, c[4] { 交点, j3, v[i], i0 } が試すべき角の部分. 
			// 線分 j1 j2 と i1 i2 (1..2 側と呼ぶ) との交点を求める.
			// 1..2 側に交点がない, または, 交点があっても線分 j1..2 と i1..2 の外側でないならば, 次の辺の組を試す.
			// 1..2 側に交点があって線分 j1 j2 と i1 i2 の外側ならば, c[4] { 交点, j2, v[i], i1 } が試すべき角の部分.
			double s, t;	// 交点の助変数
			if (!poly_intersection(j0, exp_side[j][3], exp_side[i][0], i3, s, t, c_pos[0])) {
			//if (!poly_intersection(exp_side[j][0], exp_side[j][3], exp_side[i][0], exp_side[i][3], s, t, c_pos[0])) {
				continue;
			}
			// 線分の内側で交差するか判定する.
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

			// 頂点と, 線分の交点との差分を求め, 
			// 差分の長さが, マイター制限以下か, 判定する.
			D2D1_POINT_2F diff;
			pt_sub(c_pos[0], v_pos[i], diff);	// 頂点と, 線分の交点の差分
			const double d_abs2 = pt_abs2(diff);
			const double limit_len = exp_width * mit_limit;
			if (d_abs2 <= limit_len * limit_len) {
				// 四辺形 { c0, c1, c2, c3 } が位置 t を含むか判定する.
				if (pt_in_poly(t_pos, 4, c_pos)) {
					return true;
				}
				continue;
			}

			// 差分の長さがマイター制限を超え, かつ線分の連結方法が miter or bevel か判定する.
			if (line_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				// 三角形 { c1, c2, c3 } が位置 t を含むか判定する.
				if (pt_in_poly(t_pos, 3, c_pos + 1)) {
					return true;
				}
				continue;
			}

			// マイター制限の点 m を求める.
			D2D1_POINT_2F mit_pos;
			pt_mul(diff, limit_len / sqrt(d_abs2), v_pos[i], mit_pos);
			// 点 m を通り, 差分に直交する点 n を求める.
			// 線分 c3 c0 と m n との交点 c4 を求める.
			// 線分 c0 c1 と m n との交点を求め, これを新たな c0 とする.
			// 五角形 { c0, c1, c2, c3, c4 } が位置 t を含むか判定する.
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

	// 多角形の角が位置を含むか判定する.
	// t_pos	判定する位置
	// exp_cnt	拡張した辺の数
	// exp_end	拡張した辺が閉じているか判定
	// exp_side	拡張した辺の配列 [exp_cnt][4]
	// n_vec	各辺の法線ベクトルの配列 [exp_cnt]
	// ext_len	角を超えて延長する長さ
	static bool poly_test_join_miter2(const D2D1_POINT_2F t_pos, const size_t exp_cnt, const bool exp_end, const D2D1_POINT_2F exp_side[][4], const D2D1_POINT_2F n_vec[], const double ext_len) noexcept
	{
		D2D1_POINT_2F ext_side[4];	// 拡張した辺をさらに延長した辺
		D2D1_POINT_2F ext_vec;	// 拡張した辺に平行なベクトル

		for (size_t i = (exp_end ? 0 : 1), j = (exp_end ? exp_cnt - 1 : 0); i < exp_cnt; j = i++) {
			// ある頂点に隣接する辺について.
			// 拡張した辺の一方の端を, 延長した辺に格納する.
			ext_side[0] = exp_side[j][3];
			ext_side[1] = exp_side[j][2];
			// 法線ベクトルと直行するベクトルを,
			// 延長する長さの分だけ倍し,
			// 平行なベクトルに格納する.
			pt_mul(poly_pt_orth(n_vec[j]), ext_len, ext_vec);
			// 格納した位置を平行なベクトルに沿って延長し,
			// 延長した辺に格納する.
			pt_sub(ext_side[1], ext_vec, ext_side[2]);
			pt_sub(ext_side[0], ext_vec, ext_side[3]);
			// 位置が延長した辺に含まれるか判定する.
			if (pt_in_poly(t_pos, 4, ext_side) != true) {
				// 含まれないなら継続する.
				continue;
			}
			// 隣接するもう片方の辺について.
			// 拡張した辺の端を, 延長した辺の端に格納する.
			ext_side[2] = exp_side[i][1];
			ext_side[3] = exp_side[i][0];
			// 法線ベクトルと直行するベクトル (先ほどとは逆方向) を得て,
			// 角を延長する長さの分だけ倍し,
			// 平行なベクトルに格納する.
			pt_mul(poly_pt_orth(n_vec[i]), ext_len, ext_vec);
			// 格納した位置を平行なベクトルに沿って延長し,
			// 延長した辺のもう一方の端に格納する.
			pt_add(ext_side[3], ext_vec, ext_side[0]);
			pt_add(ext_side[2], ext_vec, ext_side[1]);
			// 位置が延長した辺に含まれるか判定する.
			if (pt_in_poly(t_pos, 4, ext_side)) {
				// 含まれるなら true を返す.
				return true;
			}
		}
		return false;
	}

	// 領域をもとに多角形を作成する.
	// b_pos	領域の開始位置
	// b_diff	領域の終了位置への差分
	// v_cnt	多角形の頂点の数
	// v_up	頂点を上に作成するか判定
	// v_reg	正多角形を作成するか判定
	// v_clock	時計周りで作成するか判定
	// v_pos	頂点の配列 [v_cnt]
	void ShapePoly::create_poly_by_bbox(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const size_t v_cnt, const bool v_up, const bool v_reg, const bool v_clock, D2D1_POINT_2F v_pos[]) noexcept
	{
		if (v_cnt == 0) {
			return;
		}

		// 原点を中心とする半径 1 の円をもとに正多角形を作成する.
		D2D1_POINT_2F v_min{ 0.0, 0.0 };	// 多角形を囲む領域の左上点
		D2D1_POINT_2F v_max{ 0.0, 0.0 };	// 多角形を囲む領域の右下点
		const double s = v_up ? (M_PI / 2.0) : (M_PI / 2.0 + M_PI / v_cnt);	// 始点の角度
		const double pi2 = v_clock ? -2 * M_PI : 2 * M_PI;	// 回す全周
		for (uint32_t i = 0; i < v_cnt; i++) {
			const double t = s + pi2 * i / v_cnt;	// i 番目の頂点の角度
			v_pos[i].x = static_cast<FLOAT>(cos(t));
			v_pos[i].y = static_cast<FLOAT>(-sin(t));
			pt_inc(v_pos[i], v_min, v_max);
		}
		D2D1_POINT_2F v_diff;
		pt_sub(v_max, v_min, v_diff);

		// 正多角形を領域の大きさに合わせる.
		const double rate_x = v_reg ? fmin(b_diff.x, b_diff.y) / fmax(v_diff.x, v_diff.y) : b_diff.x / v_diff.x;
		const double rate_y = v_reg ? rate_x : b_diff.y / v_diff.y;
		for (uint32_t i = 0; i < v_cnt; i++) {
			pt_sub(v_pos[i], v_min, v_pos[i]);
			v_pos[i].x = static_cast<FLOAT>(roundl(v_pos[i].x * rate_x));
			v_pos[i].y = static_cast<FLOAT>(roundl(v_pos[i].y * rate_y));
			pt_add(v_pos[i], b_pos, v_pos[i]);
		}
	}

	// パスジオメトリを作成する.
	// d_factory DX ファクトリ
	void ShapePoly::create_path_geometry(ID2D1Factory3* const d_factory)
	{
		if (m_d2d_path_geom != nullptr) {
			m_d2d_path_geom = nullptr;
		}
		if (m_d2d_arrow_geom != nullptr) {
			m_d2d_arrow_geom = nullptr;
		}

		const auto d_cnt = m_diff.size();	// 差分の数
		if (d_cnt < 1) {
			return;
		}

		// 開始位置と, 差分の配列をもとに, 頂点を求める.
		const size_t v_cnt = d_cnt + 1;	// 頂点の数 (差分の数 + 1)
		//std::vector<D2D1_POINT_2F> vert_pos(v_cnt);	// 頂点の配列
		//const auto v_pos = reinterpret_cast<D2D1_POINT_2F*>(vert_pos.data());
		D2D1_POINT_2F v_pos[MAX_GON];
		v_pos[0] = m_pos;
		for (size_t i = 1; i < v_cnt; i++) {
			pt_add(v_pos[i - 1], m_diff[i - 1], v_pos[i]);
		}

		// 折れ線のパスジオメトリを作成する.
		winrt::com_ptr<ID2D1GeometrySink> sink;
		winrt::check_hresult(d_factory->CreatePathGeometry(m_d2d_path_geom.put()));
		winrt::check_hresult(m_d2d_path_geom->Open(sink.put()));
		sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
		const auto figure_begin = is_opaque(m_fill_color) ? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW;
		sink->BeginFigure(v_pos[0], figure_begin);
		for (size_t i = 1; i < v_cnt; i++) {
			sink->AddLine(v_pos[i]);
		}
		// Shape 上で始点と終点を重ねたとき,
		// パスに始点を加えないと, LINE_JOINT がへんなことになる.
		//if (m_end_closed) {
		//	sink->AddLine(v_pos[0]);
		//}
		sink->EndFigure(m_end_closed ? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
		winrt::check_hresult(sink->Close());
		sink = nullptr;

		// 矢じりの形式がなしか判定する.
		const auto a_style = m_arrow_style;
		if (a_style == ARROWHEAD_STYLE::NONE) {
			return;
		}

		// 矢じりの位置を求める.
		D2D1_POINT_2F h_tip;
		D2D1_POINT_2F h_barbs[2];
		if (poly_get_arrow_barbs(v_cnt, v_pos, m_arrow_size, h_tip, h_barbs)) {
			// 矢じりのパスジオメトリを作成する.
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
		double b_offset = m_arrow_size.m_offset;	// 矢じり先端のオフセット
		for (size_t i = d_cnt; i > 0; i--) {

			// 差分から矢軸とその長さを求める.
			// 矢軸の長さがほぼゼロか判定する.
			const auto a_vec = m_diff[i - 1];	// 矢軸のベクトル	
			const auto a_len = sqrt(pt_abs2(a_vec));	// 矢軸の長さ
			if (a_len < FLT_MIN) {
				continue;
			}

			// 矢軸の長さが矢じり先端のオフセットより短いか判定する.
			if (a_len < b_offset) {
				// 次の差分があるか判定する.
				if (i > 1) {
					// オフセットを矢軸の長さだけ短くする.
					b_offset -= a_len;
					continue;
				}
				b_offset = a_len;
			}

			// 矢じりの返しの位置を求める.
			const auto a_end = v_pos[i - 1];		// 矢軸の終端
			const auto b_len = m_arrow_size.m_length;	// 矢じりの長さ
			const auto b_width = m_arrow_size.m_width;	// 矢じりの幅
			D2D1_POINT_2F barbs[2];	// 矢じりの返しの位置
			get_arrow_barbs(a_vec, a_len, b_width, b_len, barbs);
			D2D1_POINT_2F a_tip;	// 矢じりの先端
			pt_mul(a_vec, 1.0 - b_offset / a_len, a_end, a_tip);
			pt_add(barbs[0], a_tip, barbs[0]);
			pt_add(barbs[1], a_tip, barbs[1]);

			// 矢じりのパスジオメトリを作成する.
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

	// 図形を表示する.
	// dx	図形の描画環境
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
			D2D1_POINT_2F a_pos{ m_pos };	// 図形の部位の位置
			anchor_draw_rect(a_pos, dx);
			const size_t d_cnt = m_diff.size();	// 差分の数
			for (size_t i = 0; i < d_cnt; i++) {
				pt_add(a_pos, m_diff[i], a_pos);
				anchor_draw_rect(a_pos, dx);
			}
		}
	}

	// 塗りつぶし色を得る.
	// value	得られた値
	// 戻り値	得られたなら true
	bool ShapePoly::get_fill_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_fill_color;
		return true;
	}

	static bool hit_test_stroke(const D2D1_POINT_2F t_pos, const double e_width, const size_t v_cnt, const D2D1_POINT_2F v_pos[], const bool v_closed, const D2D1_LINE_JOIN v_join, const double v_limit)
	{
		//std::vector<D2D1_POINT_2F> nor_vec(v_cnt);	// 法線ベクトル
		//const auto n_vec = reinterpret_cast<D2D1_POINT_2F*>(nor_vec.data());
		D2D1_POINT_2F n_vec[MAX_GON];
		//if (!poly_get_nvec(v_cnt, v_pos, n_vec.data())) {
		if (!poly_get_nvec(v_cnt, v_pos, n_vec)) {
			D2D1_POINT_2F diff;
			pt_sub(t_pos, v_pos[0], diff);
			return pt_abs2(diff) <= e_width * e_width;
		}

		//std::vector<D2D1_POINT_2F> exp_side(v_cnt * 4);	// 幅をもつ辺
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

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	uint32_t ShapePoly::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		constexpr D2D1_POINT_2F PZ{ 0.0f, 0.0f };	// 零点
		const size_t v_cnt = m_diff.size() + 1;	// 頂点の数 (差分の数 + 1)
		//std::vector<D2D1_POINT_2F> vert_pos(v_cnt);	// 頂点の配列
		//const auto v_pos = reinterpret_cast<D2D1_POINT_2F*>(vert_pos.data());
		D2D1_POINT_2F v_pos[MAX_GON];	// 頂点の配列
		size_t j = static_cast<size_t>(-1);	// 点を含む頂点の添え字
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
			const auto e_width = max(max(static_cast<double>(m_stroke_width), a_len) * 0.5, 0.5);	// 拡張する幅
			const bool v_closed = m_end_closed;
			const auto v_style = m_stroke_join_style;
			const auto v_limit = m_stroke_join_limit;
			if (hit_test_stroke(tp, e_width, v_cnt, v_pos, v_closed, v_style, v_limit)) {
				return ANCH_TYPE::ANCH_STROKE;
			}
			/*
			// 各辺の法線ベクトルを得る.
			std::vector<D2D1_POINT_2F> n_vec(v_cnt);	// 法線ベクトル
			poly_get_nvec(v_cnt, v_pos.data(), n_vec.data());

			// 多角形の各辺が位置を含むか判定する.
			std::vector<D2D1_POINT_2F> q_exp(v_cnt * 4);	// 幅をもつ辺
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

		// 辺が不透明, または位置が辺に含まれていないなら,
		// 塗りつぶし色が不透明か判定する.
		if (is_opaque(m_fill_color)) {
			if (pt_in_poly(tp, v_cnt, v_pos)) {
				// 含まれるなら ANCH_FILL を返す.
				return ANCH_TYPE::ANCH_FILL;
			}
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// 範囲に含まれるか判定する.
	// a_min	範囲の左上位置
	// a_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapePoly::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		if (!pt_in_rect(m_pos, a_min, a_max)) {
			return false;
		}
		const size_t n = m_diff.size();	// 差分の数
		D2D1_POINT_2F e_pos = m_pos;
		for (size_t i = 0; i < n; i++) {
			pt_add(e_pos, m_diff[i], e_pos);	// 次の位置
			if (!pt_in_rect(e_pos, a_min, a_max)) {
				return false;
			}
		}
		return true;
	}

	// 塗りつぶしの色に格納する.
	void ShapePoly::set_fill_color(const D2D1_COLOR_F& value) noexcept
	{
		if (equal(m_fill_color, value)) {
			return;
		}
		m_fill_color = value;
		create_path_geometry(s_d2d_factory);
	}

	// 図形を作成する.
	// b_pos	囲む領域の始点
	// b_diff	囲む領域の終点への差分
	// s_attr	属性
	// v_cnt	頂点の数
	// v_reg	正多角形に作図するか判定
	// v_up	頂点を上に作図するか判定
	// v_end	辺を閉じて作図するか判定
	ShapePoly::ShapePoly(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr, const TOOL_POLY& t_poly) :
		ShapePath::ShapePath(t_poly.m_vertex_cnt - 1, s_attr),
		m_end_closed(t_poly.m_closed),
		m_fill_color(s_attr->m_fill_color)
	{
		std::vector<D2D1_POINT_2F> vert_pos(t_poly.m_vertex_cnt);	// 頂点の配列
		const auto v_pos = reinterpret_cast<D2D1_POINT_2F*>(vert_pos.data());
		create_poly_by_bbox(b_pos, b_diff, t_poly.m_vertex_cnt, t_poly.m_vertex_up, t_poly.m_regular, t_poly.m_clockwise, v_pos);
		m_pos = v_pos[0];
		for (size_t i = 1; i < t_poly.m_vertex_cnt; i++) {
			pt_sub(v_pos[i], v_pos[i - 1], m_diff[i - 1]);
		}
		vert_pos.clear();
		create_path_geometry(s_d2d_factory);
	}

	// 図形をデータリーダーから読み込む.
	// dt_reader	データリーダー
	ShapePoly::ShapePoly(DataReader const& dt_reader) :
		ShapePath::ShapePath(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;
		m_end_closed = dt_reader.ReadBoolean();
		read(m_fill_color, dt_reader);
		create_path_geometry(s_d2d_factory);
	}

	// データライターに書き込む.
	void ShapePoly::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapePath::write(dt_writer);
		dt_writer.WriteBoolean(m_end_closed);
		write(m_fill_color, dt_writer);
	}

	// データライターに SVG タグとして書き込む.
	void ShapePoly::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<path d=\"", dt_writer);
		write_svg(m_pos, "M", dt_writer);
		const auto d_cnt = m_diff.size();	// 差分の数
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