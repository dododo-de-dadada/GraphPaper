#include "pch.h"
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

	// 位置が, 線分の端点 (円形) に含まれるか判定する.
	// t_pos	線分の始点を原点とした, 判定する位置.
	// v_end	線分の終点
	// 戻り値	含まれるなら true
	static bool stroke_test_cap_round(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F v_end, const double e_width)
	{
		// 調べる位置が, 最初の頂点を中心とし, 拡張する幅を半径とする, 円に含まれるか判定する.
		if (pt_abs2(t_pos) <= e_width * e_width) {
			return true;
		}
		// 調べる位置が, 最後の頂点を中心とし, 拡張する幅を半径とする, 円に含まれるか判定する.
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
			D2D1_POINT_2F d_vec;
			pt_sub(t_pos, v_pos[i], d_vec);
			if (pt_abs2(d_vec) <= exp_width * exp_width) {
				return true;
			}
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
				if (pt_abs2(t_pos) <= e_width * e_width) {
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
					pt_add(v_pos[i], -c_vec.x - o_vec.x, -c_vec.y - o_vec.y, e_side[e_cnt][0]);
					pt_add(v_pos[i], -c_vec.x + o_vec.x, -c_vec.y + o_vec.y, e_side[e_cnt][1]);
					pt_add(v_pos[i], c_vec.x + o_vec.x, c_vec.y + o_vec.y, e_side[e_cnt][2]);
					pt_add(v_pos[i], c_vec.x - o_vec.x, c_vec.y - o_vec.y, e_side[e_cnt][3]);
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

	// D2D ストローク特性を作成する.
	static void create_stroke_dash_style(ID2D1Factory3* const d_factory, const D2D1_CAP_STYLE s_cap_line, const D2D1_CAP_STYLE s_cap_dash, const D2D1_DASH_STYLE s_dash, const STROKE_DASH_PATT& s_patt, const D2D1_LINE_JOIN s_join, const double s_limit, ID2D1StrokeStyle** s_style);

	// D2D ストローク特性を作成する.
	// s_cap_line	線の端点
	// s_cap_dash	破線の端点
	// s_dash	破線の種類
	// s_patt	破線の配置配列
	// s_join	線のつながり
	// s_limit	マイター制限
	// s_style	作成されたストローク特性
	static void create_stroke_dash_style(
		ID2D1Factory3* const d_factory,
		const D2D1_CAP_STYLE s_cap_line,
		const D2D1_CAP_STYLE s_cap_dash,
		const D2D1_DASH_STYLE s_dash,
		const STROKE_DASH_PATT& s_patt,
		const D2D1_LINE_JOIN s_join,
		const double s_limit, ID2D1StrokeStyle** s_style)
	{
		UINT32 d_cnt;	// 破線の配置配列の要素数
		const FLOAT* d_arr;	// 破線の配置配列を指すポインタ

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

	// 図形を破棄する.
	ShapeStroke::~ShapeStroke(void)
	{
		m_d2d_stroke_dash_style = nullptr;
	}

	// 図形を囲む領域を得る.
	// a_min	元の領域の左上位置.
	// a_man	元の領域の右下位置.
	// b_min	得られた領域の左上位置.
	// b_max	得られた領域の右下位置.
	void ShapeStroke::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		const size_t d_cnt = m_diff.size();	// 差分の数
		D2D1_POINT_2F e_pos = m_pos;
		b_min = a_min;
		b_max = a_max;
		pt_inc(e_pos, b_min, b_max);
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(e_pos, m_diff[i], e_pos);
			pt_inc(e_pos, b_min, b_max);
		}
	}

	// 図形を囲む領域の左上位置を得る.
	// value	領域の左上位置
	void ShapeStroke::get_min_pos(D2D1_POINT_2F& value) const noexcept
	{
		const size_t n = m_diff.size();	// 差分の数
		D2D1_POINT_2F v_pos = m_pos;	// 頂点の位置
		value = m_pos;
		for (size_t i = 0; i < n; i++) {
			pt_add(v_pos, m_diff[i], v_pos);
			pt_min(value, v_pos, value);
		}

		//value.x = m_diff[0].x >= 0.0f ? m_pos.x : m_pos.x + m_diff[0].x;
		//value.y = m_diff[0].y >= 0.0f ? m_pos.y : m_pos.y + m_diff[0].y;
	}

	// 指定された部位の位置を得る.
	void ShapeStroke::get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept
	{
		if (anch == ANCH_TYPE::ANCH_SHEET || anch == ANCH_TYPE::ANCH_P0) {
			// 図形の部位が「外部」または「開始点」ならば, 開始位置を得る.
			value = m_pos;
		}
		else if (anch > ANCH_TYPE::ANCH_P0) {
			const size_t m = m_diff.size() + 1;		// 頂点の数 (差分の数 + 1)
			if (anch < ANCH_TYPE::ANCH_P0 + m) {
				value = m_pos;
				for (size_t i = 0; i < anch - ANCH_TYPE::ANCH_P0; i++) {
					pt_add(value, m_diff[i], value);
				}
			}
		}
		//value = m_pos;
	}

	// 開始位置を得る
	// 戻り値	つねに true
	bool ShapeStroke::get_start_pos(D2D1_POINT_2F& value) const noexcept
	{
		value = m_pos;
		return true;
	}

	// 線枠の色を得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_stroke_color;
		return true;
	}

	// 線枠のマイター制限の比率を得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_join_limit(float& value) const noexcept
	{
		value = m_stroke_join_limit;
		return true;
	}

	// 線のつながりを得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_join_style(D2D1_LINE_JOIN& value) const noexcept
	{
		value = m_stroke_join_style;
		return true;
	}

	// 線のつながりを得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_cap_dash(D2D1_CAP_STYLE& value) const noexcept
	{
		value = m_stroke_cap_dash;
		return true;
	}

	// 線のつながりを得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_cap_line(D2D1_CAP_STYLE& value) const noexcept
	{
		value = m_stroke_cap_line;
		return true;
	}

	// 破線の配置を得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_dash_patt(STROKE_DASH_PATT& value) const noexcept
	{
		value = m_stroke_dash_patt;
		return true;
	}

	// 線枠の形式を得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_dash_style(D2D1_DASH_STYLE& value) const noexcept
	{
		value = m_stroke_dash_style;
		return true;
	}

	// 線枠の太さを得る.
	// 戻り値	つねに true
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

	// 位置を含むか判定する.
	// 戻り値	つねに ANCH_SHEET
	uint32_t ShapeStroke::hit_test(const D2D1_POINT_2F /*t_pos*/, const double /*a_len*/) const noexcept
	{
		return ANCH_TYPE::ANCH_SHEET;
	}

	// 範囲に含まれるか判定する.
	// 戻り値	つねに false
	bool ShapeStroke::in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept
	{
		return false;
	}

	// 差分だけ移動する.
	void ShapeStroke::move(const D2D1_POINT_2F diff)
	{
		D2D1_POINT_2F s_pos;
		pt_add(m_pos, diff, s_pos);
		set_start_pos(s_pos);
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	void ShapeStroke::set_start_pos(const D2D1_POINT_2F value)
	{
		D2D1_POINT_2F s_pos;
		pt_round(value, PT_ROUND, s_pos);
		m_pos = s_pos;
	}

	// 線枠の色に格納する.
	void ShapeStroke::set_stroke_color(const D2D1_COLOR_F& value) noexcept
	{
		m_stroke_color = value;
	}

	// 矢じりをデータライターに SVG タグとして書き込む.
	// barbs	矢じりの両端の位置
	// tip_pos	矢じりの先端の位置
	// a_style	矢じりの形状
	// dt_writer	データライター
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

	// 値を破線の端点に格納する.
	void ShapeStroke::set_stroke_cap_dash(const D2D1_CAP_STYLE& value)
	{
		if (equal(m_stroke_cap_dash, value)) {
			return;
		}
		m_stroke_cap_dash = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// 値を線の端点に格納する.
	void ShapeStroke::set_stroke_cap_line(const D2D1_CAP_STYLE& value)
	{
		if (equal(m_stroke_cap_line, value)) {
			return;
		}
		m_stroke_cap_line = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// 値をマイター制限の比率に格納する.
	void ShapeStroke::set_stroke_join_limit(const float& value)
	{
		if (equal(m_stroke_join_limit, value)) {
			return;
		}
		m_stroke_join_limit = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// 値を線のつながりに格納する.
	void ShapeStroke::set_stroke_join_style(const D2D1_LINE_JOIN& value)
	{
		if (equal(m_stroke_join_style, value)) {
			return;
		}
		m_stroke_join_style = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// 値を破線の配置に格納する.
	void ShapeStroke::set_stroke_dash_patt(const STROKE_DASH_PATT& value)
	{
		if (equal(m_stroke_dash_patt, value)) {
			return;
		}
		m_stroke_dash_patt = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// 値を線枠の形式に格納する.
	void ShapeStroke::set_stroke_dash_style(const D2D1_DASH_STYLE value)
	{
		if (m_stroke_dash_style == value) {
			return;
		}
		m_stroke_dash_style = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// 値を線枠の太さに格納する.
	void ShapeStroke::set_stroke_width(const float value) noexcept
	{
		m_stroke_width = value;
	}

	// 図形を作成する.
	// d_cnt	差分の個数 (最大値は N_GON_MAX - 1)
	// s_attr	属性値
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

	// 図形をデータリーダーから読み込む.
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

	// データライターに書き込む.
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

	// データライターに SVG タグとして書き込む.
	void ShapeStroke::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg(m_stroke_color, "stroke", dt_writer);
		write_svg(m_stroke_dash_style, m_stroke_dash_patt, m_stroke_width, dt_writer);
		write_svg(m_stroke_width, "stroke-width", dt_writer);
		write_svg("stroke-linejoin=\"miter\" stroke-miterlimit=\"1\" ", dt_writer);
	}

}