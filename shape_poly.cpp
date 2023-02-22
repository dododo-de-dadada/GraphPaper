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
				D2D1_POINT_2F direction;	// 辺ベクトル
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
				D2D1_POINT_2F direction;	// 辺ベクトル
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
				D2D1_POINT_2F triangle[3];	// 三角形
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

	// 多角形の角が位置を含むか判定する (面取り)
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

	// 多角形の角が位置を含むか判定する.
	// t_pos	判定する位置
	// e_cnt	拡張した辺の数
	// e_close	拡張した辺が閉じているか判定
	// e_width	辺の太さの半分.
	// e	拡張した辺の配列 [exp_cnt][4+1]
	// miter_limit	線の尖り制限
	// j_style	線の結合方法
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
			// 拡張された辺について角の部分を求める.
			//
			// 点 vi でつながる, 拡張された辺 j と i のイメージ
			// j0                     j3      i0                     i3
			//  +---expanded side[j]-->+  vi   +---expanded side[i]-->+ 
			// j1                     j2      i1                     i2
			//
			// 拡張された辺 j と i が平行か判定する.
			if (equal(e[j][3], e[i][0])) {
				// 平行ならば重なる部分はないので, 次の辺を試す.
				continue;
			}
			// 拡張された辺 j と i が重なるか判定する.
			if (equal(e[j][3], e[i][1])) {
				// 線の結合が尖りか判定する.
				if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER) {
					//尖りならば, 辺 j を尖り制限の長さだけ延長した四辺形を求める.
					D2D1_POINT_2F direction;	// 辺ベクトル
					pt_sub(e[j][3], e[j][0], direction);
					pt_mul(direction, e_width * miter_limit / sqrt(pt_abs2(direction)), direction);
					D2D1_POINT_2F quadrilateral[4];
					quadrilateral[0] = e[j][3];
					quadrilateral[1] = e[j][2];
					pt_add(e[j][2], direction, quadrilateral[2]);
					pt_add(e[j][3], direction, quadrilateral[3]);
					// 調べる位置が四辺形に含まれるか判定する.
					if (pt_in_poly(test, 4, quadrilateral)) {
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
			D2D1_POINT_2F q[4 + 1];	// 四辺形 (尖り制限を超えるならば五角形)
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

			// 交点における方向ベクトルとその長さを求める.
			// 差分ベクトルの長さが, 尖り制限以下か判定する.
			D2D1_POINT_2F d;	// 方向ベクトル
			pt_sub(q[0], e[i][4], d);
			const double d_abs2 = pt_abs2(d);	// 方向ベクトルの長さの二乗
			const double limit_len = e_width * miter_limit;
			if (d_abs2 <= limit_len * limit_len) {
				// 尖り制限以下ならば, 調べる位置を四辺形 { q0, q1, q2, q3 } が含むか判定する.
				if (pt_in_poly(test, 4, q)) {
					// 位置を含むなら true を返す.
					return true;
				}
				continue;
			}
			// 尖り制限を超えるならば, 線の結合が尖りまたは面取りか判定する.
			if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				// 線の結合が尖りまたは面取りならば, 調べる位置を三角形 { q1, q2, q3 } が含むか判定する.
				const D2D1_POINT_2F* triangle = q + 1;
				if (pt_in_poly(test, 3, triangle)) {
					// 位置を含むなら true を返す.
					return true;
				}
				continue;
			}
			// 差分ベクトル上にあって, 交点との距離がちょうど尖り制限の長さになる点 m と,
			// 差分ベクトルと直行するベクトル上にあって, 点 m を通る直線上の点 n を求める.
			// 線分 q3 q0 と m n との交点を求め, q4 に格納する.
			// 線分 q0 q1 と m n との交点を求め, これを新たな q0 とする.
			// 調べる位置を五角形 { q0, q1, q2, q3, q4 } が含むか判定する.
			D2D1_POINT_2F mitered;
			pt_mul_add(d, limit_len / sqrt(d_abs2), e[i][4], mitered);
			D2D1_POINT_2F orthogonal;
			pt_add(mitered, D2D1_POINT_2F{ d.y, -d.x }, orthogonal);
			poly_find_intersection(q[3], q[0], mitered, orthogonal, s, t, q[4]);
			poly_find_intersection(q[0], q[1], mitered, orthogonal, s, t, q[0]);
			const D2D1_POINT_2F* pentagon = q;
			if (pt_in_poly(test, 5, pentagon)) {
				// 位置を含むなら true を返す.
				return true;
			}
		}
		return false;
	}

	// 多角形の角が位置を含むか判定する (丸まった角)
	// e_width	辺の半分の太さ
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

	// 位置が, 線分に含まれるか判定する.
	// test	判定する位置 (線分の始点を原点とする)
	// p_cnt	始点を除く位置ベクトルの数
	// p	始点を除く位置ベクトルの配列
	// s_opaque	線が不透明か判定
	// s_width	線の太さ
	// e_closed	線が閉じているか判定
	// s_join	線の結合
	// s_limit	尖り制限
	// f_opa	塗りつぶしが不透明か判定
	static uint32_t poly_hit_test(
		const D2D1_POINT_2F test, const size_t p_cnt, const D2D1_POINT_2F p[], const bool s_opaque,
		const double s_width, const bool e_closed, const CAP_STYLE& s_cap,
		const D2D1_LINE_JOIN s_join, const double s_limit, const bool f_opaque, const double a_len)
	{
		D2D1_POINT_2F q[N_GON_MAX]{ { 0.0f, 0.0f }, };	// 頂点 (始点 { 0,0 } を含めた)
		double e_len[N_GON_MAX];	// 辺の長さ
		size_t nz_cnt = 0;	// 長さのある辺の数
		size_t k = static_cast<size_t>(-1);	// 見つかった頂点
		for (size_t i = 0; i < p_cnt; i++) {
			// 判定する位置が, 頂点の部位に含まれるか判定する.
			if (pt_in_anc(test, q[i], a_len)) {
				k = i;
			}
			// 辺の長さを求める.
			e_len[i] = sqrt(pt_abs2(p[i]));
			// 辺の長さがあるか判定する.
			if (e_len[i] >= FLT_MIN) {
				nz_cnt++;
			}
			// 頂点に辺ベクトルを加え, 次の頂点を求める.
			pt_add(q[i], p[i], q[i + 1]);
		}
		// 判定する位置が, 終点の部位に含まれるか判定する.
		if (pt_in_anc(test, q[p_cnt], a_len)) {
			k = p_cnt;
		}
		// 頂点が見つかったか判定する.
		if (k != -1) {
			return ANC_TYPE::ANC_P0 + static_cast<uint32_t>(k);
		}
		// 線が不透明か判定する.
		if (s_opaque) {
			// 不透明ならば, 線の太さの半分の幅を求め, 拡張する幅に格納する.
			const auto e_width = max(max(static_cast<double>(s_width), a_len) * 0.5, 0.5);	// 拡張する幅
			// 全ての辺の長さがゼロか判定する.
			if (nz_cnt == 0) {
				// ゼロならば, 判定する位置が, 拡張する幅を半径とする円に含まれるか判定する.
				if (pt_in_circle(test, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
				return ANC_TYPE::ANC_PAGE;
			}
			// 辺が閉じているか判定する.
			if (e_closed) {
				// 閉じているなら, 始点は { 0, 0 } なので終点へのベクトルを, そのまま最後の辺の長さとする.
				e_len[p_cnt] = sqrt(pt_abs2(q[p_cnt]));
			}
			// 閉じてないなら, 端の形式が円形か判定する.
			else if (equal(s_cap, CAP_ROUND)) {
				if (pt_in_circle(test, e_width) || pt_in_circle(test, q[p_cnt], e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// 閉じてないなら, 端の形式が正方形か判定する.
			else if (equal(s_cap, CAP_SQUARE)) {
				if (poly_test_cap_square(test, q[p_cnt], p_cnt, p, e_len, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// 閉じてないなら, 端の形式が三角形か判定する.
			else if (equal(s_cap, CAP_TRIANGLE)) {
				if (poly_test_cap_triangle(test, q[p_cnt], p_cnt, p, e_len, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			D2D1_POINT_2F e[N_GON_MAX][4 + 1];	// 太さ分拡張された辺 (+頂点)
			size_t e_cnt = 0;
			for (size_t i = 0; i < p_cnt; i++) {
				// 辺 i の長さがないか判定する.
				if (e_len[i] < FLT_MIN) {
					// 点 i から降順に, 長さのある辺 m を見つける.
					size_t m = static_cast<size_t>(-1);
					for (size_t h = i; h > 0; h--) {
						if (e_len[h - 1] >= FLT_MIN) {
							m = h - 1;
							break;
						}
					}
					// 降順の辺が見つかったか判定する.
					D2D1_POINT_2F prev;	// 直前の辺ベクトル
					if (m != static_cast<size_t>(-1)) {
						// 見つかった辺を直前の辺ベクトルに格納する.
						prev = p[m];
					}
					// 見つからなかったならば,
					// 辺が閉じている, かつ最後の辺の長さが非ゼロか判定する.
					else if (e_closed && e_len[p_cnt] >= FLT_MIN) {
						// 最後の頂点の反対ベクトルを求め, 直前の辺ベクトルとする.
						prev = D2D1_POINT_2F{ -q[p_cnt].x, -q[p_cnt].y };
					}
					else {
						continue;
					}
					// 点 i から昇順に, 長さのある辺を見つける.
					size_t n = static_cast<size_t>(-1);
					for (size_t j = i + 1; j < p_cnt; j++) {
						if (e_len[j] >= FLT_MIN) {
							n = j;
							break;
						}
					}
					// 昇順の辺が見つかったか判定する.
					D2D1_POINT_2F next;	// 直後の辺ベクトル
					if (n != -1) {
						// 見つかった辺を直後の辺ベクトルに格納する.
						next = p[n];
					}
					// 見つからなかったならば,
					// 辺が閉じている, かつ最後の辺に長さがあるか判定する.
					else if (e_closed && e_len[p_cnt] >= FLT_MIN) {
						// 最後の頂点の反対ベクトルを求め, 直後の辺ベクトルとする.
						next = D2D1_POINT_2F{ -q[p_cnt].x, -q[p_cnt].y };
					}
					else {
						continue;
					}
					// 直前と直後の辺ベクトルを加え, 結果ベクトルを求める.
					D2D1_POINT_2F resultant;
					pt_add(prev, next, resultant);
					// 合成ベクトルの長さがないか判定する.
					double r_abs = pt_abs2(resultant);
					if (r_abs < FLT_MIN) {
						// 直前の辺ベクトルを合成ベクトルとする.
						resultant = prev;
						r_abs = pt_abs2(resultant);
					}
					// 合成ベクトルの直交ベクトルを求める.
					// 両ベクトルとも長さは, 拡張する幅とする.
					pt_mul(resultant, e_width / sqrt(r_abs), resultant);
					const D2D1_POINT_2F orthogonal{ resultant.y, -resultant.x };
					// 頂点 i を直交ベクトルに沿って四方に拡張し, 拡張された辺 i に格納する.
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
					// 辺ベクトルに直交するベクトルを求める.
					// 直交ベクトルの長さは, 拡張する幅とする.
					D2D1_POINT_2F direction;	// 方向ベクトル
					pt_mul(p[i], e_width / e_len[i], direction);
					const D2D1_POINT_2F orthogonal{ direction.y, -direction.x };	// 直交するベクトル
					// 頂点 i と i+1 を直交ベクトルに沿って正逆に拡張し, 拡張された辺 i に格納する.
					pt_sub(q[i], orthogonal, e[e_cnt][0]);
					pt_add(q[i], orthogonal, e[e_cnt][1]);
					pt_add(q[i + 1], orthogonal, e[e_cnt][2]);
					pt_sub(q[i + 1], orthogonal, e[e_cnt][3]);
					e[e_cnt][4] = q[i];
				}
				// 調べる位置が, 拡張された辺に含まれるか判定する.
				if (pt_in_poly(test, 4, e[e_cnt++])) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// 辺が閉じているか, 閉じた辺に長さがあるか判定する.
			if (e_closed && e_len[p_cnt] >= FLT_MIN) {
				// 最後の辺の位置を反転させ, 拡張する幅の長さに合わせ, 辺ベクトルを求める.
				// 辺ベクトルに直交するベクトルを求める.
				// 始点と終点を直交ベクトルに沿って正逆に拡張し, 拡張された辺に格納する.
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
				// 判定する位置が拡張された辺に含まれるか判定する.
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

	// 矢じりの返しと先端の位置を得る.
	// p_cnt	折れ線の頂点 (端点を含む) の数
	// p	頂点の配列
	// a_size	矢じりの大きさ
	// tip	矢じりの先端の位置
	// barb	矢じりの返しの位置
	bool ShapePolygon::poly_get_pos_arrow(
		const size_t p_cnt, const D2D1_POINT_2F p[], const ARROW_SIZE& a_size, D2D1_POINT_2F& tip,
		D2D1_POINT_2F barb[]) noexcept
	{
		double a_offset = a_size.m_offset;	// 矢じりの先端のオフセット
		for (size_t i = p_cnt - 1; i > 0; i--) {

			// 頂点間の差分から矢軸とその長さを求める.
			// 矢軸の長さがほぼゼロか判定する.
			// 長さゼロならこの頂点は無視する.
			D2D1_POINT_2F q;
			pt_sub(p[i], p[i - 1], q);	// 頂点間の差分
			const auto a_len = sqrt(pt_abs2(q));	// 矢軸の長さ
			if (a_len < FLT_MIN) {
				continue;
			}

			// 矢軸の長さが矢じるし先端のオフセットより短いか判定する.
			if (a_len < a_offset) {
				// 次の差分があるか判定する.
				if (i > 1) {
					// オフセットを矢軸の長さだけ短くする.
					a_offset -= a_len;
					continue;
				}
				a_offset = a_len;
			}

			// 矢じりの返しの位置を求める.
			const auto a_end = p[i - 1];		// 矢軸の終端
			const auto b_len = a_size.m_length;	// 矢じりの長さ
			const auto b_width = a_size.m_width;	// 矢じりの返しの幅
			get_pos_barbs(q, a_len, b_width, b_len, barb);
			pt_mul_add(q, 1.0 - a_offset / a_len, a_end, tip);
			pt_add(barb[0], tip, barb[0]);
			pt_add(barb[1], tip, barb[1]);
			return true;
		}
		return false;
	}

	// 領域をもとに多角形を作成する.
	// start	領域の始点
	// b_vec	領域の終点への差分
	// p_opt	多角形の作成方法
	// v_pos	頂点の配列 [v_cnt]
	void ShapePolygon::poly_by_bbox(
		const D2D1_POINT_2F start, const D2D1_POINT_2F b_vec, const POLY_OPTION& p_opt,
		D2D1_POINT_2F v_pos[]) noexcept
	{
		// v_cnt	多角形の頂点の数
		// v_up	頂点を上に作成するか判定
		// v_reg	正多角形を作成するか判定
		// v_clock	時計周りで作成するか判定
		const auto v_cnt = p_opt.m_vertex_cnt;
		if (v_cnt == 0) {
			return;
		}
		const auto v_up = p_opt.m_vertex_up;
		const auto v_reg = p_opt.m_regular;
		const auto v_clock = p_opt.m_clockwise;

		// 原点を中心とする半径 1 の円をもとに正多角形を作成する.
		D2D1_POINT_2F v_lt{ 0.0, 0.0 };	// 多角形を囲む領域の左上点
		D2D1_POINT_2F v_rb{ 0.0, 0.0 };	// 多角形を囲む領域の右下点
		const double s = v_up ? (M_PI / 2.0) : (M_PI / 2.0 + M_PI / v_cnt);	// 始点の角度
		const double pi2 = v_clock ? -2 * M_PI : 2 * M_PI;	// 回す全周
		for (uint32_t i = 0; i < v_cnt; i++) {
			const double t = s + pi2 * i / v_cnt;	// i 番目の頂点の角度
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

		// 正多角形を領域の大きさに合わせる.
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

	// 図形を表示する.
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

			// 開始位置と, 差分の配列をもとに, 頂点を求める.
			const size_t v_cnt = m_vec.size() + 1;	// 頂点の数 (差分の数 + 1)
			D2D1_POINT_2F v_pos[N_GON_MAX];
			v_pos[0] = m_start;
			for (size_t i = 1; i < v_cnt; i++) {
				pt_add(v_pos[i - 1], m_vec[i - 1], v_pos[i]);
			}

			// 折れ線のパスジオメトリを作成する.
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

			// 矢じるしの形式がなしか判定する.
			const auto a_style = m_arrow_style;
			if (a_style != ARROW_STYLE::NONE) {

				// 矢じるしの位置を求める.
				D2D1_POINT_2F tip;
				D2D1_POINT_2F barb[2];
				if (poly_get_pos_arrow(v_cnt, v_pos, m_arrow_size, tip, barb)) {

					// 矢じるしのパスジオメトリを作成する.
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
			const auto p_geom = m_d2d_path_geom.get();	// パスのジオメトリ
			const auto s_width = m_stroke_width;	// 折れ線の太さ
			const auto s_style = m_d2d_stroke_style.get();	// 折れ線の形式
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
			D2D1_POINT_2F a_pos{ m_start };	// 図形の部位の位置
			anc_draw_square(a_pos, target, brush);
			const size_t d_cnt = m_vec.size();	// 差分の数
			for (size_t i = 0; i < d_cnt; i++) {
				pt_add(a_pos, m_vec[i], a_pos);
				anc_draw_square(a_pos, target, brush);
			}
		}
	}

	/*
	// 塗りつぶし色を得る.
	// val	得られた値
	// 戻り値	得られたなら true
	bool ShapePolygon::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}
	*/

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// a_len	アンカーの大きさ
	// 戻り値	位置を含む図形の部位
	uint32_t ShapePolygon::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		D2D1_POINT_2F t;
		pt_sub(test, m_start, t);
		return poly_hit_test(
			t, m_vec.size(), m_vec.data(), is_opaque(m_stroke_color), m_stroke_width, m_end_closed,
			m_stroke_cap, m_join_style, m_join_miter_limit, is_opaque(m_fill_color), m_anc_width);
	}

	// 範囲に含まれるか判定する.
	// a_lt	範囲の左上位置
	// a_rb	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapePolygon::in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept
	{
		if (!pt_in_rect(m_start, area_lt, area_rb)) {
			return false;
		}
		const size_t d_cnt = m_vec.size();	// 差分の数
		D2D1_POINT_2F e_pos = m_start;
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(e_pos, m_vec[i], e_pos);	// 次の位置
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
	// 塗りつぶし色に格納する.
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

	// 図形を作成する.
	// start	囲む領域の始点
	// b_vec	囲む領域の終点への差分
	// page	ページ
	// p_opt	多角形の選択肢
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

	// 図形をデータリーダーから読み込む.
	// dt_reader	データリーダー
	ShapePolygon::ShapePolygon(const Shape& page, DataReader const& dt_reader) :
		ShapePath::ShapePath(page, dt_reader)
	{
		m_end_closed = dt_reader.ReadBoolean();
	}

	// 図形をデータライターに書き込む.
	void ShapePolygon::write(DataWriter const& dt_writer) const
	{
		ShapePath::write(dt_writer);
		dt_writer.WriteBoolean(m_end_closed);
	}

}