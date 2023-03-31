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
	// i	交点
	// 戻り値	交点が求まれば true, そうでなければ false.
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

	// test	判定する点
	// p_end	最後の頂点
	// s_cnt	辺の数
	// s[s_cnt]	辺のベクトル (次の頂点への位置ベクトル)
	// s_len[s_cnt]	辺の長さ
	// e_width	辺の半分の太さ
	static bool poly_test_cap_square(
		const D2D1_POINT_2F test, const D2D1_POINT_2F p_end, const size_t s_cnt, 
		const D2D1_POINT_2F s[], const double s_len[], const double e_width)
	{
		for (size_t i = 0; i < s_cnt; i++) {
			if (s_len[i] >= FLT_MIN) {
				D2D1_POINT_2F d;	// 辺の太さに合わせた辺の方向ベクトル
				pt_mul(s[i], -e_width / s_len[i], d);
				const D2D1_POINT_2F o{ d.y, -d.x };	// 辺の直交ベクトル
				D2D1_POINT_2F q[4];	// 太らせた辺の端点の四辺形
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
				D2D1_POINT_2F d;	// 辺の太さに合わせた辺の方向ベクトル
				pt_mul(s[i - 1], e_width / s_len[i - 1], d);
				const D2D1_POINT_2F o{ d.y, -d.x };	// 辺の直交ベクトル
				D2D1_POINT_2F q[4];	// 太らせた辺の端点の四辺形
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

	// test	判定する点
	// p_end	最後の頂点
	// s_cnt	辺の数
	// s[s_cnt]	辺のベクトル (次の頂点への位置ベクトル)
	// s_len[s_cnt]	辺の長さ
	// e_width	辺の半分の太さ
	static bool poly_test_cap_triangle(
		const D2D1_POINT_2F test, const D2D1_POINT_2F p_end, const size_t s_cnt,
		const D2D1_POINT_2F s[], const double s_len[], const double e_width)
	{
		for (size_t i = 0; i < s_cnt; i++) {
			if (s_len[i] >= FLT_MIN) {
				D2D1_POINT_2F d;
				pt_mul(s[i], -e_width / s_len[i], d);
				const D2D1_POINT_2F o{ d.y, -d.x };
				D2D1_POINT_2F t[3];	//  太らせた辺の端点の三角形
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
				D2D1_POINT_2F t[3];	// 三角形
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

	// 多角形の角が図形が点を含むか判定する (面取り)
	// e_side	太さ分拡張された辺の配列
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

	// 多角形の角が図形が点を含むか判定する.
	// t_pos	判定される点
	// s_cnt	辺の数
	// e_close	辺が閉じているか判定
	// e_width	辺の太さの半分.
	// s	辺の配列 [exp_cnt][4+1]
	// miter_limit	線の尖り制限
	// j_style	線の結合方法
	static bool poly_test_join_miter(
		const D2D1_POINT_2F test,
		const size_t s_cnt,
		const bool e_close,
		const double e_width,
		const D2D1_POINT_2F s[][4 + 1],
		const double miter_limit,
		const D2D1_LINE_JOIN j_style) noexcept
	{
		for (size_t i = (e_close ? 0 : 1), j = (e_close ? s_cnt - 1 : 0); i < s_cnt; j = i++) {
			// 拡張された辺について角の部分を求める.
			//
			// 点 vi でつながる, 拡張された辺 j と i のイメージ
			// j0                     j3      i0                     i3
			//  +---expanded side[j]-->+  vi   +---expanded side[i]-->+ 
			// j1                     j2      i1                     i2
			//
			// 拡張された辺 j と i が平行か判定する.
			if (equal(s[j][3], s[i][0])) {
				// 平行ならば重なる部分はないので, 次の辺を試す.
				continue;
			}
			// 拡張された辺 j と i が重なるか判定する.
			if (equal(s[j][3], s[i][1])) {
				// 線の結合が尖りか判定する.
				if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER) {
					//尖りならば, 辺 j を尖り制限の長さだけ延長した四辺形を求める.
					D2D1_POINT_2F direction;	// 辺ベクトル
					pt_sub(s[j][3], s[j][0], direction);
					pt_mul(direction, e_width * miter_limit / sqrt(pt_abs2(direction)), direction);
					D2D1_POINT_2F quadrilateral[4];
					quadrilateral[0] = s[j][3];
					quadrilateral[1] = s[j][2];
					pt_add(s[j][2], direction, quadrilateral[2]);
					pt_add(s[j][3], direction, quadrilateral[3]);
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
			double t, u;	// 交点の助変数
			D2D1_POINT_2F q[4 + 1];	// 四辺形 (尖り制限を超えるならば五角形)
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

			// 交点における方向ベクトルとその長さを求める.
			// 差分ベクトルの長さが, 尖り制限以下か判定する.
			D2D1_POINT_2F d;	// 方向ベクトル
			pt_sub(q[0], s[i][4], d);
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
			pt_mul_add(d, limit_len / sqrt(d_abs2), s[i][4], mitered);
			D2D1_POINT_2F orthogonal;
			pt_add(mitered, D2D1_POINT_2F{ d.y, -d.x }, orthogonal);
			poly_find_intersection(q[3], q[0], mitered, orthogonal, t, u, q[4]);
			poly_find_intersection(q[0], q[1], mitered, orthogonal, t, u, q[0]);
			const D2D1_POINT_2F* pentagon = q;
			if (pt_in_poly(test, 5, pentagon)) {
				// 位置を含むなら true を返す.
				return true;
			}
		}
		return false;
	}

	// 多角形の角が図形が点を含むか判定する (丸まった角)
	// e_width	辺の半分の太さ
	static bool poly_test_join_round(
		const D2D1_POINT_2F& t, const size_t s_cnt, const D2D1_POINT_2F s[], const double e_width)
	{
		for (size_t i = 0; i < s_cnt; i++) {
			if (pt_in_circle(t, s[i], e_width)) {
				return true;
			}
		}
		return false;
	}

	// 位置が, 線分に含まれるか判定する.
	// t	判定される点 (線分の始点を原点とする)
	// p_cnt	始点を除く位置ベクトルの数
	// p	始点を除く位置ベクトルの配列
	// s_opaque	線が不透明か判定
	// s_width	線の太さ
	// e_closed	線が閉じているか判定
	// s_join	線の結合
	// s_limit	尖り制限
	// f_opa	塗りつぶしが不透明か判定
	static uint32_t poly_hit_test(
		const D2D1_POINT_2F t, const size_t p_cnt, const D2D1_POINT_2F p[], const bool s_opaque,
		const double s_width, const bool e_closed, const CAP_STYLE& s_cap,
		const D2D1_LINE_JOIN s_join, const double s_limit, const bool f_opaque, const double a_len)
	{
		D2D1_POINT_2F q[N_GON_MAX]{ { 0.0f, 0.0f }, };	// 頂点 (始点 { 0,0 } を含めた)
		double s_len[N_GON_MAX];	// 辺の長さ
		size_t n_cnt = 0;	// 長さのある辺の数
		size_t k = static_cast<size_t>(-1);	// 見つかった頂点
		for (size_t i = 0; i < p_cnt; i++) {
			// 判定される点が, 頂点の部位に含まれるか判定する.
			if (anc_hit_test(t, q[i], a_len)) {
				k = i;
			}
			// 辺の長さを求める.
			s_len[i] = sqrt(pt_abs2(p[i]));
			// 辺の長さがあるか判定する.
			if (s_len[i] >= FLT_MIN) {
				n_cnt++;
			}
			// 頂点に辺ベクトルを加え, 次の頂点を求める.
			pt_add(q[i], p[i], q[i + 1]);
		}
		// 判定される点が, 終点の部位に含まれるか判定する.
		if (anc_hit_test(t, q[p_cnt], a_len)) {
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
			if (n_cnt == 0) {
				// ゼロならば, 判定される点が, 拡張する幅を半径とする円に含まれるか判定する.
				if (pt_in_circle(t.x, t.y, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
				return ANC_TYPE::ANC_PAGE;
			}
			// 辺が閉じているか判定する.
			if (e_closed) {
				// 閉じているなら, 始点は { 0, 0 } なので終点へのベクトルを, そのまま最後の辺の長さとする.
				s_len[p_cnt] = sqrt(pt_abs2(q[p_cnt]));
			}
			// 閉じてないなら, 端の形式が円形か判定する.
			else if (equal(s_cap, CAP_STYLE_ROUND)) {
				if (pt_in_circle(t.x, t.y, e_width) ||
					pt_in_circle(t, q[p_cnt], e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// 閉じてないなら, 端の形式が正方形か判定する.
			else if (equal(s_cap, CAP_STYLE_SQUARE)) {
				if (poly_test_cap_square(t, q[p_cnt], p_cnt, p, s_len, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// 閉じてないなら, 端の形式が三角形か判定する.
			else if (equal(s_cap, CAP_STYLE_TRIANGLE)) {
				if (poly_test_cap_triangle(t, q[p_cnt], p_cnt, p, s_len, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			D2D1_POINT_2F s[N_GON_MAX][4 + 1];	// 太さ分拡張された辺 (+頂点)
			size_t s_cnt = 0;
			for (size_t i = 0; i < p_cnt; i++) {
				// 辺 i の長さがないか判定する.
				if (s_len[i] < FLT_MIN) {
					// 点 i から降順に, 長さのある辺 m を見つける.
					size_t m = static_cast<size_t>(-1);
					for (size_t h = i; h > 0; h--) {
						if (s_len[h - 1] >= FLT_MIN) {
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
					else if (e_closed && s_len[p_cnt] >= FLT_MIN) {
						// 最後の頂点の反対ベクトルを求め, 直前の辺ベクトルとする.
						prev = D2D1_POINT_2F{ -q[p_cnt].x, -q[p_cnt].y };
					}
					else {
						continue;
					}
					// 点 i から昇順に, 長さのある辺を見つける.
					size_t n = static_cast<size_t>(-1);
					for (size_t j = i + 1; j < p_cnt; j++) {
						if (s_len[j] >= FLT_MIN) {
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
					else if (e_closed && s_len[p_cnt] >= FLT_MIN) {
						// 最後の頂点の反対ベクトルを求め, 直後の辺ベクトルとする.
						next = D2D1_POINT_2F{ -q[p_cnt].x, -q[p_cnt].y };
					}
					else {
						continue;
					}
					// 直前と直後の辺ベクトルを加えたベクトル (resultant) を求める.
					D2D1_POINT_2F r;
					pt_add(prev, next, r);
					// 合成ベクトルの長さがないか判定する.
					double r_abs = pt_abs2(r);
					if (r_abs < FLT_MIN) {
						// 直前の辺ベクトルを合成ベクトルとする.
						r = prev;
						r_abs = pt_abs2(r);
					}
					// 合成ベクトルの直交ベクトルを求める.
					// 両ベクトルとも長さは, 拡張する幅とする.
					pt_mul(r, e_width / sqrt(r_abs), r);
					const D2D1_POINT_2F orthogonal{ r.y, -r.x };
					// 頂点 i を直交ベクトルに沿って四方に拡張し, 拡張された辺 i に格納する.
					const double cx = r.x;
					const double cy = r.y;
					const double ox = orthogonal.x;
					const double oy = orthogonal.y;
					pt_add(q[i], -cx - ox, -cy - oy, s[s_cnt][0]);
					pt_add(q[i], -cx + ox, -cy + oy, s[s_cnt][1]);
					pt_add(q[i], cx + ox, cy + oy, s[s_cnt][2]);
					pt_add(q[i], cx - ox, cy - oy, s[s_cnt][3]);
					s[s_cnt][4] = q[i];
				}
				else {
					// 辺ベクトルに直交するベクトルを求める.
					// 直交ベクトルの長さは, 拡張する幅とする.
					D2D1_POINT_2F direction;	// 方向ベクトル
					pt_mul(p[i], e_width / s_len[i], direction);
					const D2D1_POINT_2F orthogonal{ direction.y, -direction.x };	// 直交するベクトル
					// 頂点 i と i+1 を直交ベクトルに沿って正逆に拡張し, 拡張された辺 i に格納する.
					pt_sub(q[i], orthogonal, s[s_cnt][0]);
					pt_add(q[i], orthogonal, s[s_cnt][1]);
					pt_add(q[i + 1], orthogonal, s[s_cnt][2]);
					pt_sub(q[i + 1], orthogonal, s[s_cnt][3]);
					s[s_cnt][4] = q[i];
				}
				// 調べる位置が, 拡張された辺に含まれるか判定する.
				if (pt_in_poly(t, 4, s[s_cnt++])) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			// 辺が閉じているか, 閉じた辺に長さがあるか判定する.
			if (e_closed && s_len[p_cnt] >= FLT_MIN) {
				// 最後の辺の位置を反転させ, 拡張する幅の長さに合わせ, 辺ベクトルを求める.
				// 辺ベクトルに直交するベクトルを求める.
				// 始点と終点を直交ベクトルに沿って正逆に拡張し, 拡張された辺に格納する.
				D2D1_POINT_2F direction;
				pt_mul(q[p_cnt], -e_width / s_len[p_cnt], direction);
				const D2D1_POINT_2F orthogonal{ direction.y, -direction.x };
				pt_sub(q[p_cnt], orthogonal, s[s_cnt][0]);
				pt_add(q[p_cnt], orthogonal, s[s_cnt][1]);
				s[s_cnt][2] = orthogonal; // v0 + o_vec
				//pt_neg(o_vec, e_side[e_cnt][3]); // v0 - o_vec
				s[s_cnt][3].x = -orthogonal.x;
				s[s_cnt][3].y = -orthogonal.y;
				s[s_cnt][4] = q[p_cnt];
				// 判定される点が拡張された辺に含まれるか判定する.
				if (pt_in_poly(t, 4, s[s_cnt++])) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				if (poly_test_join_bevel(t, s_cnt, e_closed, s)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			else if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER
				|| s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				if (poly_test_join_miter(t, s_cnt, e_closed, e_width, s, s_limit, s_join)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			else if (s_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				if (poly_test_join_round(t, p_cnt + 1, q, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
		}
		if (f_opaque) {
			if (pt_in_poly(t, p_cnt + 1, q)) {
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
	bool ShapePoly::poly_get_pos_arrow(
		const size_t p_cnt, const D2D1_POINT_2F p[], const ARROW_SIZE& a_size,
		D2D1_POINT_2F barb[], D2D1_POINT_2F& tip) noexcept
	{
		double a_offset = a_size.m_offset;	// 矢じりの先端の位置
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

			// 矢軸の長さが矢じるし先端の位置より短いか判定する.
			if (a_len < a_offset) {
				// 次の差分があるか判定する.
				if (i > 1) {
					// 先端の位置を矢軸の長さだけ短くする.
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

	// 矩形をもとに多角形を作成する.
	// start	始点
	// pos	終点の位置ベクトル
	// p_opt	多角形の作成方法
	// p	頂点の配列
	void ShapePoly::poly_create_by_box(
		const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const POLY_OPTION& p_opt,
		D2D1_POINT_2F p[]) noexcept
	{
		// v_cnt	多角形の頂点の数
		// v_up	頂点を上に作成するか判定
		// v_reg	正多角形を作成するか判定
		// v_clock	時計周りで作成するか判定
		const auto p_cnt = p_opt.m_vertex_cnt;
		if (p_cnt == 0) {
			return;
		}
		const auto p_up = p_opt.m_vertex_up;
		const auto p_reg = p_opt.m_regular;
		const auto p_clock = p_opt.m_clockwise;

		// { 0, 0 } を中心とする半径 1 の円をもとに正多角形を作成し,
		// ついでに, その頂点をちょうど含む, 境界矩形を得る.
		D2D1_POINT_2F box_lt{ 0.0f, 0.0f };	// 境界矩形の左上点
		D2D1_POINT_2F box_rb{ 0.0f, 0.0f };	// 境界矩形の右下点
		const double r = (p_clock ? -2.0 * M_PI : 2.0 * M_PI);	// 全周
		const double s = p_up ? (M_PI_2) : (M_PI_2 + M_PI / p_cnt);	// 始点の角度
		for (uint32_t i = 0; i < p_cnt; i++) {
			const double t = s + r * i / p_cnt;	// i 番目の頂点の角度
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

		// 境界矩形を位置ベクトルで表される方形に合致させるための, 拡大率を得る.
		// 正多角形の場合, X 方向と Y 方向の, どちらか小さい方の拡大率に一致させる.
		const double px = fabs(pos.x);
		const double py = fabs(pos.y);
		double sx;	// X 方向の拡大率
		double sy;	// Y 方向の拡大率
		if (px <= py) {
			sx = px / (box_rb.x - box_lt.x);
			if (p_reg) {
				sy = sx;
			}
			else {
				sy = py / (box_rb.y - box_lt.y);
			}
		}
		else{
			sy = py / (box_rb.y - box_lt.y);
			if (p_reg) {
				sx = sy;
			}
			else {
				sx = px / (box_rb.x - box_lt.x);
			}
		}

		// 位置ベクトルの正負によって, 境界矩形のどの頂点を基点にするか決める.
		double bx;	// 基点 X 座標
		double by;	// 基点 Y 座標
		if (pos.x >= 0.0f && pos.y >= 0.0f) {
			bx = box_lt.x;
			by = box_lt.y;
		}
		else if (pos.x < 0.0f && pos.y >= 0.0f) {
			bx = box_rb.x;
			by = box_lt.y;
		}
		else if (pos.x < 0.0f && pos.y <= 0.0f) {
			bx = box_rb.x;
			by = box_rb.y;
		}
		else {
			bx = box_lt.x;
			by = box_rb.y;
		}

		// 正多角形を, 基点が原点となるよう平行移動したあと拡大し,
		// 始点に平行移動する.
		for (uint32_t i = 0; i < p_cnt; i++) {
			p[i].x = static_cast<FLOAT>(start.x + sx * (p[i].x - bx));
			p[i].y = static_cast<FLOAT>(start.y + sy * (p[i].y - by));
			pt_round(p[i], PT_ROUND, p[i]);
		}

	}

	// 図形を表示する.
	void ShapePoly::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
		ID2D1Factory* factory;
		target->GetFactory(&factory);
		D2D1_POINT_2F p[N_GON_MAX];
		size_t p_cnt = static_cast<size_t>(-1);

		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color) &&
			m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color) &&
			m_arrow_style != ARROW_STYLE::NONE && m_d2d_arrow_stroke == nullptr) {
			create_arrow_stroke();
		}
		if (((!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) ||
			is_opaque(m_fill_color)) && m_d2d_path_geom == nullptr) {
			if (p_cnt == static_cast<size_t>(-1)) {
				p_cnt = get_verts(p);
			}
			if (p_cnt != static_cast<size_t>(-1)) {
				// 折れ線のパスジオメトリを作成する.
				const auto f_begin = is_opaque(m_fill_color) ?
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW;
				const auto f_end = (m_end_closed ?
					D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED :
					D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
				winrt::com_ptr<ID2D1GeometrySink> sink;
				winrt::check_hresult(
					factory->CreatePathGeometry(m_d2d_path_geom.put()));
				winrt::check_hresult(
					m_d2d_path_geom->Open(sink.put()));
				sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
				sink->BeginFigure(p[0], f_begin);
				for (size_t i = 1; i < p_cnt; i++) {
					sink->AddLine(p[i]);
				}
				sink->EndFigure(f_end);
				winrt::check_hresult(
					sink->Close());
				sink = nullptr;
			}
		}
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color) && 
			m_arrow_style != ARROW_STYLE::NONE && m_d2d_arrow_geom == nullptr) {
			if (p_cnt == static_cast<size_t>(-1)) {
				p_cnt = get_verts(p);
			}
			if (p_cnt != static_cast<size_t>(-1)) {
				// 矢じるしの位置を求める.
				D2D1_POINT_2F tip;
				D2D1_POINT_2F barb[2];
				if (poly_get_pos_arrow(p_cnt, p, m_arrow_size, barb, tip)) {
					winrt::com_ptr<ID2D1GeometrySink> sink;
					// 矢じるしのパスジオメトリを作成する.
					const auto a_begin = (m_arrow_style == ARROW_STYLE::FILLED ?
						D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
						D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
					const auto a_end = (m_arrow_style == ARROW_STYLE::FILLED ?
						D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED :
						D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
					winrt::check_hresult(
						factory->CreatePathGeometry(m_d2d_arrow_geom.put()));
					winrt::check_hresult(
						m_d2d_arrow_geom->Open(sink.put()));
					sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
					sink->BeginFigure(barb[0], a_begin);
					sink->AddLine(tip);
					sink->AddLine(barb[1]);
					sink->EndFigure(a_end);
					winrt::check_hresult(
						sink->Close());
					sink = nullptr;
				}
			}
		}
		if (is_opaque(m_fill_color)) {
			const auto p_geom = m_d2d_path_geom.get();	// パスのジオメトリ
			if (p_geom != nullptr) {
				brush->SetColor(m_fill_color);
				target->FillGeometry(p_geom, brush, nullptr);
			}
		}
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			const auto p_geom = m_d2d_path_geom.get();	// パスのジオメトリ
			if (p_geom != nullptr) {
				brush->SetColor(m_stroke_color);
				target->DrawGeometry(p_geom, brush, m_stroke_width, m_d2d_stroke_style.get());
			}
			const auto a_geom = m_d2d_arrow_geom.get();	// 矢じるしのジオメトリ
			if (a_geom != nullptr && m_arrow_style == ARROW_STYLE::OPENED) {
				target->DrawGeometry(a_geom, brush, m_stroke_width, m_d2d_arrow_stroke.get());
			}
			else if (a_geom != nullptr && m_arrow_style == ARROW_STYLE::FILLED) {
				target->FillGeometry(a_geom, brush, nullptr);
				target->DrawGeometry(a_geom, brush, m_stroke_width, m_d2d_arrow_stroke.get());
			}
		}
		if (m_anc_show && is_selected()) {
			if (p_cnt == static_cast<size_t>(-1)) {
				p_cnt = get_verts(p);
			}
			if (p_cnt != static_cast<size_t>(-1)) {
				// 補助線を描く
				if (m_stroke_width >= Shape::m_anc_square_inner) {
					const auto p_geom = m_d2d_path_geom.get();	// パスのジオメトリ
					brush->SetColor(COLOR_WHITE);
					target->DrawGeometry(p_geom, brush, 2.0f * m_aux_width, nullptr);
					brush->SetColor(COLOR_BLACK);
					target->DrawGeometry(p_geom, brush, m_aux_width, m_aux_style.get());
				}
				// 図形の部位を描く.
				for (size_t i = 0; i < p_cnt; i++) {
					anc_draw_square(p[i], target, brush);
				}
			}
		}
	}

	// 図形が点を含むか判定する.
	// t_pos	判定される点
	// a_len	アンカーの大きさ
	// 戻り値	位置を含む図形の部位
	uint32_t ShapePoly::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		D2D1_POINT_2F t;
		pt_sub(test, m_start, t);
		return poly_hit_test(
			t, m_pos.size(), m_pos.data(), is_opaque(m_stroke_color), m_stroke_width, m_end_closed,
			m_stroke_cap, m_join_style, m_join_miter_limit, is_opaque(m_fill_color), m_anc_width);
	}

	// 矩形に含まれるか判定する.
	// lt	矩形の左上位置
	// rb	矩形の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapePoly::is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		if (!pt_in_rect(m_start, lt, rb)) {
			return false;
		}
		const size_t p_cnt = m_pos.size();	// 差分の数
		D2D1_POINT_2F p{ m_start };
		for (size_t i = 0; i < p_cnt; i++) {
			p.x += m_pos[i].x;
			p.y += m_pos[i].y;
			if (!pt_in_rect(p, lt, rb)) {
				return false;
			}
		}
		return true;
	}

	bool ShapePoly::set_arrow_style(const ARROW_STYLE val) noexcept
	{
		if (!m_end_closed) {
			return ShapePath::set_arrow_style(val);
		}
		return false;
	}

	// 図形を作成する.
	// start	囲む領域の始点
	// pos	囲む領域の終点への位置ベクトル
	// page	ページ
	// p_opt	多角形の選択肢
	ShapePoly::ShapePoly(
		const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page, const POLY_OPTION& p_opt) :
		ShapePath::ShapePath(page, p_opt.m_end_closed),
		m_end_closed(p_opt.m_end_closed)
	{
		D2D1_POINT_2F p[N_GON_MAX];
		poly_create_by_box(start, pos, p_opt, p);

		m_start = p[0];
		m_pos.resize(p_opt.m_vertex_cnt - 1);
		m_pos.shrink_to_fit();
		for (size_t i = 1; i < p_opt.m_vertex_cnt; i++) {
			m_pos[i - 1].x = p[i].x - p[i - 1].x;
			m_pos[i - 1].y = p[i].y - p[i - 1].y;
		}
	}

	// 図形をデータリーダーから読み込む.
	// dt_reader	データリーダー
	ShapePoly::ShapePoly(DataReader const& dt_reader) :
		ShapePath::ShapePath(dt_reader)
	{
		m_end_closed = dt_reader.ReadBoolean();
	}

	// 図形をデータライターに書き込む.
	void ShapePoly::write(DataWriter const& dt_writer) const
	{
		ShapePath::write(dt_writer);
		dt_writer.WriteBoolean(m_end_closed);
	}

}