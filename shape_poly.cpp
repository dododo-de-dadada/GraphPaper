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
	// 線分の交点と, その助変数を求める.
	// a	線分 ab の始点
	// b	線分 ab の終点
	// c	線分 cd の始点
	// d	線分 cd の終点
	// s	線分 ab に対する交点の助変数
	// t	線分 cd に対する交点の助変数
	// e	交点
	// 戻り値	交点が求まれば true, そうでなければ false.
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

	// 位置が, 線分の端点 (円形) に含まれるか判定する.
	// t_pos	線分の始点を原点とする, 判定する位置.
	// v_end	線分の終点
	// 戻り値	含まれるなら true
	static bool stroke_test_cap_round(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F v_end, const double e_width)
	{
		return pt_in_circle(t_pos, e_width) || pt_in_circle(t_pos, v_end, e_width);
		// 調べる位置が, 最初の頂点を中心とし, 拡張する幅を半径とする, 円に含まれるか判定する.
		//if (pt_abs2(t_pos) <= e_width * e_width) {
		//	return true;
		//}
		// 調べる位置が, 最後の頂点を中心とし, 拡張する幅を半径とする, 円に含まれるか判定する.
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

	// 多角形の角が位置を含むか判定する (面取り)
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

	// 多角形の角が位置を含むか判定する.
	// t_pos	判定する位置
	// exp_cnt	拡張した辺の数
	// exp_close	拡張した辺が閉じているか判定
	// exp_width	辺の太さの半分.
	// exp_side	拡張した辺の配列 [exp_cnt][4+1]
	// mit_limit	線のマイター制限
	// s_join	線のつながり方法
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
			// 拡張された辺について角の部分を求める.
			//
			// 点 vi でつながる, 拡張された辺 j と i のイメージ
			// j0                     j3      i0                     i3
			//  +---expanded side[j]-->+  vi   +---expanded side[i]-->+ 
			// j1                     j2      i1                     i2
			//
			// 拡張された辺 j と i が平行か判定する.
			if (equal(exp_side[j][3], exp_side[i][0])) {
				// 平行ならば重なる部分はないので, 次の辺を試す.
				continue;
			}
			// 拡張された辺 j と i が重なるか判定する.
			if (equal(exp_side[j][3], exp_side[i][1])) {
				// 線のつながりがマイターか判定する.
				if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER) {
					// マイターならば, 辺 j をマイター制限の長さだけ延長した四辺形を求める.
					D2D1_POINT_2F d_vec;
					pt_sub(exp_side[j][3], exp_side[j][0], d_vec);
					pt_mul(d_vec, exp_width * s_limit / sqrt(pt_abs2(d_vec)), d_vec);
					//D2D1_POINT_2F c_pos[4];
					D2D1_POINT_2F q_pos[4];
					q_pos[0] = exp_side[j][3];
					q_pos[1] = exp_side[j][2];
					pt_add(exp_side[j][2], d_vec, q_pos[2]);
					pt_add(exp_side[j][3], d_vec, q_pos[3]);
					// 調べる位置が四辺形に含まれるか判定する.
					if (pt_in_poly(t_pos, 4, q_pos)) {
						return true;
					}
				}
				continue;
			}
			// 拡張された辺 i と j が重なる部分を求め, 四辺形に格納する.
			// 線分 j0 j3 と i0 i3 (これを仮に 0..3 側と呼ぶ) との交点を求める.
			// 0..3 側に交点がないならば, 次の拡張された辺を試す.
			// 0..3 側に交点があって, 拡張された辺の外側ならば, 四辺形 { 交点, j3, v[i], i0 } を得る.
			// 0..3 側に交点があって, 拡張された辺の内側ならば, 線分 j1 j2 と i1 i2 (1..2 側と呼ぶ) との交点を求める.
			// 1..2 側に交点がない, または, 交点があっても拡張された辺の内側ならば, 次の拡張された辺を試す.
			// 1..2 側に交点があって, 拡張された辺の外側ならば, 四辺形 { 交点, j2, v[i], i1 } を得る.
			double s, t;	// 交点の助変数
			D2D1_POINT_2F q_pos[4 + 1];	// 四辺形 (マイター制限を超えるならば五角形)
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

			// 頂点と交点との差分ベクトルとその長さを求める.
			// 差分ベクトルの長さが, マイター制限以下か判定する.
			D2D1_POINT_2F d_vec;	// 差分ベクトル
			pt_sub(q_pos[0], exp_side[i][4], d_vec);
			const double d_abs2 = pt_abs2(d_vec);	// 差分ベクトルの長さ
			const double limit_len = exp_width * s_limit;
			if (d_abs2 <= limit_len * limit_len) {
				// マイター制限以下ならば, 調べる位置を四辺形 { q0, q1, q2, q3 } が含むか判定する.
				if (pt_in_poly(t_pos, 4, q_pos)) {
					// 位置を含むなら true を返す.
					return true;
				}
				continue;
			}
			// マイター制限を超えるならば, 線のつながりがマイターまたは面取りか判定する.
			if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				// 線のつながりがマイターまたは面取りならば, 調べる位置を三角形 { q1, q2, q3 } が含むか判定する.
				const D2D1_POINT_2F* tri_pos = q_pos + 1;
				if (pt_in_poly(t_pos, 3, tri_pos)) {
					// 位置を含むなら true を返す.
					return true;
				}
				continue;
			}
			// 差分ベクトル上にあって, 交点との距離がちょうどマイター制限の長さになる点 m と,
			// 差分ベクトルと直行するベクトル上にあって, 点 m を通る直線上の点 n を求める.
			// 線分 q3 q0 と m n との交点を求め, q4 に格納する.
			// 線分 q0 q1 と m n との交点を求め, これを新たな q0 とする.
			// 調べる位置を五角形 { q0, q1, q2, q3, q4 } が含むか判定する.
			D2D1_POINT_2F mit_pos;
			pt_mul(d_vec, limit_len / sqrt(d_abs2), exp_side[i][4], mit_pos);
			D2D1_POINT_2F nor_pos;
			pt_add(mit_pos, D2D1_POINT_2F{ d_vec.y, -d_vec.x }, nor_pos);
			stroke_find_intersection(q_pos[3], q_pos[0], mit_pos, nor_pos, s, t, q_pos[4]);
			stroke_find_intersection(q_pos[0], q_pos[1], mit_pos, nor_pos, s, t, q_pos[0]);
			const D2D1_POINT_2F* pen_pos = q_pos;
			if (pt_in_poly(t_pos, 5, pen_pos)) {
				// 位置を含むなら true を返す.
				return true;
			}
		}
		return false;
	}

	// 多角形の角が位置を含むか判定する (丸まった角)
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

	// 位置が, 線分に含まれるか判定する.
	// t_pos	判定する位置 (線分の始点を原点とする)
	// d_cnt	辺ベクトルの数
	// diff	辺ベクトル (頂点の間の差分)
	// a_len	図形の部位の大きさ
	// s_opa	線が不透明か判定
	// s_width	線の太さ
	// s_closed	線が閉じているか判定
	// s_join	線のつながり
	// s_limit	マイター制限
	// f_opa	塗りつぶしが不透明か判定
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
		// 始点は { 0, 0 }
		D2D1_POINT_2F v_pos[N_GON_MAX]{ { 0.0f, 0.0f }, };	// 頂点の位置
		// 各頂点の位置と辺の長さを求める.
		double s_len[N_GON_MAX];	// 辺の長さ
		size_t nz_cnt = 0;	// 長さのある辺の数
		size_t k = static_cast<size_t>(-1);	// 見つかった頂点
		for (size_t i = 0; i < d_cnt; i++) {
			// 判定する位置が, 頂点の部位に含まれるか判定する.
			if (a_len > 0 && pt_in_anch(t_pos, v_pos[i], a_len)) {
				k = i;
			}
			// 辺の長さを求める.
			s_len[i] = sqrt(pt_abs2(diff[i]));
			// 辺の長さがあるか判定する.
			if (s_len[i] > FLT_MIN) {
				nz_cnt++;
			}
			// 頂点に辺ベクトルを加え, 次の頂点を求める.
			pt_add(v_pos[i], diff[i], v_pos[i + 1]);
		}
		// 判定する位置が, 終点の部位に含まれるか判定する.
		if (pt_in_anch(t_pos, v_pos[d_cnt], a_len)) {
			k = d_cnt;
		}
		// 頂点が見つかったか判定する.
		if (k != -1) {
			return ANCH_TYPE::ANCH_P0 + static_cast<uint32_t>(k);
		}
		// 線が不透明か判定する.
		if (s_opa) {
			// 不透明ならば, 線の太さの半分の幅を求め, 拡張する幅に格納する.
			const auto e_width = max(max(static_cast<double>(s_width), a_len) * 0.5, 0.5);	// 拡張する幅
			// 全ての辺の長さがゼロか判定する.
			if (nz_cnt == 0) {
				// ゼロならば, 判定する位置が, 拡張する幅を半径とする円に含まれるか判定する.
				if (pt_in_circle(t_pos, e_width)) {
				//if (pt_abs2(t_pos) <= e_width * e_width) {
					return ANCH_TYPE::ANCH_STROKE;
				}
				return ANCH_TYPE::ANCH_SHEET;
			}
			// 辺が閉じているか判定する.
			if (s_closed) {
				// 閉じているなら, 始点は { 0, 0 } なので終点へのベクトルを, そのまま最後の辺の長さとする.
				s_len[d_cnt] = sqrt(pt_abs2(v_pos[d_cnt]));
			}
			// 閉じてないなら, 線の端点が円形か判定する.
			else if (s_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
				if (stroke_test_cap_round(t_pos, v_pos[d_cnt], e_width)) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
			// 閉じてないなら, 線の端点が正方形か判定する.
			else if (s_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
				if (stroke_test_cap_square(t_pos, v_pos[d_cnt], d_cnt, diff, s_len, e_width)) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
			// 閉じてないなら, 線の端点が三角形か判定する.
			else if (s_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
				if (stroke_test_cap_triangle(t_pos, v_pos[d_cnt], d_cnt, diff, s_len, e_width)) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
			D2D1_POINT_2F e_side[N_GON_MAX][4 + 1];	// 拡張された辺 (+頂点)
			size_t e_cnt = 0;
			for (size_t i = 0; i < d_cnt; i++) {
				// 辺 i の長さがないか判定する.
				if (s_len[i] <= FLT_MIN) {
					// 点 i から降順に, 長さのある辺 p を見つける.
					size_t p = static_cast<size_t>(-1);
					for (size_t h = i; h > 0; h--) {
						if (s_len[h - 1] > FLT_MIN) {
							p = h - 1;
							break;
						}
					}
					// 辺 p が見つかったか判定する.
					D2D1_POINT_2F p_diff;	// 直前の辺ベクトル
					if (p != static_cast<size_t>(-1)) {
						p_diff = diff[p];
					}
					// 見つからなかったならば,
					// 辺が閉じている, かつ最後の辺に長さがあるか判定する.
					else if (s_closed && s_len[d_cnt] > FLT_MIN) {
						// 最後の頂点の反対ベクトルを求め, 直前の辺ベクトルとする.
						p_diff = D2D1_POINT_2F{ -v_pos[d_cnt].x, -v_pos[d_cnt].y };
					}
					else {
						continue;
					}
					// 点 i から昇順に, 長さのある辺 n を見つける.
					size_t n = static_cast<size_t>(-1);
					for (size_t j = i + 1; j < d_cnt; j++) {
						if (s_len[j] > FLT_MIN) {
							n = j;
							break;
						}
					}
					// 辺 n が見つかったか判定する.
					D2D1_POINT_2F n_diff;	// 直後の辺ベクトル
					if (n != -1) {
						n_diff = diff[n];
					}
					// 見つからなかったならば,
					// 辺が閉じている, かつ最後の辺に長さがあるか判定する.
					else if (s_closed && s_len[d_cnt] > FLT_MIN) {
						// 最後の頂点の反対ベクトルを求め, 直後の辺ベクトルとする.
						n_diff = D2D1_POINT_2F{ -v_pos[d_cnt].x, -v_pos[d_cnt].y };
					}
					else {
						continue;
					}
					// 直前と直後の辺ベクトルを加え, 合成ベクトル c を求める.
					D2D1_POINT_2F c_vec;
					pt_add(p_diff, n_diff, c_vec);
					// 合成ベクトルの長さがないか判定する.
					double c_abs = pt_abs2(c_vec);
					if (c_abs <= FLT_MIN) {
						// 直前の辺ベクトルを合成ベクトルとする.
						c_vec = p_diff;
						c_abs = pt_abs2(c_vec);
					}
					// 合成ベクトルに直交するベクトルを求める.
					// 両ベクトルとも長さは, 拡張する幅とする.
					pt_mul(c_vec, e_width / sqrt(c_abs), c_vec);
					const D2D1_POINT_2F o_vec{ c_vec.y, -c_vec.x };
					// 頂点 i を直交ベクトルに沿って四方に拡張し, 拡張された辺 i に格納する.
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
					// 辺ベクトルに直交するベクトルを求める.
					// 直交ベクトルの長さは, 拡張する幅とする.
					D2D1_POINT_2F d_vec;
					pt_mul(diff[i], e_width / s_len[i], d_vec);
					const D2D1_POINT_2F o_vec{ d_vec.y, -d_vec.x };
					// 頂点 i と i+1 を直交ベクトルに沿って正逆に拡張し, 拡張された辺 i に格納する.
					pt_sub(v_pos[i], o_vec, e_side[e_cnt][0]);
					pt_add(v_pos[i], o_vec, e_side[e_cnt][1]);
					pt_add(v_pos[i + 1], o_vec, e_side[e_cnt][2]);
					pt_sub(v_pos[i + 1], o_vec, e_side[e_cnt][3]);
					e_side[e_cnt][4] = v_pos[i];
				}
				// 調べる位置が, 拡張された辺 i に含まれるか判定する.
				if (pt_in_poly(t_pos, 4, e_side[e_cnt++])) {
					return ANCH_TYPE::ANCH_STROKE;
				}
			}
			// 辺が閉じているか判定する.
			if (s_closed) {
				// 最後の辺の長さがあるか判定する.
				if (s_len[d_cnt] > FLT_MIN) {
					// 最後の辺の位置を反転させ, 拡張する幅の長さに合わせ, 辺ベクトルを求める.
					// 辺ベクトルに直交するベクトルを求める.
					// 始点と終点を直交ベクトルに沿って正逆に拡張し, 拡張された辺 i に格納する.
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

	// 直行するベクトルを得る.
	//static D2D1_POINT_2F poly_pt_orth(const D2D1_POINT_2F vec) { return { -vec.y, vec.x }; }

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
	/*
	static bool poly_get_nvec(const size_t v_cnt, const D2D1_POINT_2F v_pos[], D2D1_POINT_2F n_vec[]) noexcept
	{
		// 多角形の各辺の長さと法線ベクトル, 
		// 重複しない頂点の数を求める.
		//std::vector<double> side_len(v_cnt);	// 各辺の長さ
		//const auto s_len = reinterpret_cast<double*>(side_len.data());
		double s_len[N_GON_MAX];	// 各辺の長さ
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
	*/

	// 領域をもとに多角形を作成する.
	// b_pos	領域の始点
	// b_diff	領域の終点への差分
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
		D2D1_POINT_2F v_pos[N_GON_MAX];
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

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
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