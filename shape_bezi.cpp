//------------------------------
// Shape_bezi.cpp
// ベジェ曲線
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// セグメントを区切る助変数の値
	constexpr double T0 = 0.0;
	constexpr double T1 = 1.0 / 3.0;
	constexpr double T2 = 2.0 / 3.0;
	constexpr double T3 = 1.0;

	// シンプソン法の回数
	constexpr uint32_t SIMPSON_N = 30;

	// double 型の値をもつ位置
	// ShapeBase 内でのみ使用する.
	struct BZP {
		double x;
		double y;

		inline void inc(BZP& r_min, BZP& r_max) const noexcept
		{
			if (x < r_min.x) {
				r_min.x = x;
			}
			if (x > r_max.x) {
				r_max.x = x;
			}
			if (y < r_min.y) {
				r_min.y = y;
			}
			if (y > r_max.y) {
				r_max.y = y;
			}
		}
		inline BZP nextafter(const double d) const noexcept { return { std::nextafter(x, x + d), std::nextafter(y, y + d) }; }
		inline operator D2D1_POINT_2F(void) const noexcept { return { static_cast<FLOAT>(x), static_cast<FLOAT>(y) }; }
		inline BZP operator -(const BZP& q) const noexcept { return { x - q.x, y - q.y }; }
		inline BZP operator -(const D2D1_POINT_2F q) const noexcept { return { x - q.x, y - q.y }; }
		inline BZP operator -(void) const noexcept { return { -x, -y }; }
		inline BZP operator *(const double s) const noexcept { return { x * s, y * s }; }
		inline double operator *(const BZP& q) const noexcept { return x * q.x + y * q.y; }
		inline BZP operator +(const BZP& q) const noexcept { return { x + q.x, y + q.y }; }
		inline BZP operator +(const D2D1_POINT_2F p) const noexcept { return { x + p.x, y + p.y }; }
		inline bool operator <(const BZP& q) const noexcept { return x < q.x && y < q.y; }
		inline BZP operator =(const D2D1_POINT_2F p) noexcept { return { x = p.x, y = p.y }; }
		inline BZP operator =(const double s) noexcept { return { x = s, y = s }; }
		inline bool operator >(const BZP& q) const noexcept { return x > q.x && y > q.y; }
		inline bool operator ==(const BZP& q) const noexcept { return x == q.x && y == q.y; }
		inline bool operator !=(const BZP& q) const noexcept { return x != q.x || y != q.y; }
		inline double opro(const BZP& q) const noexcept { return x * q.y - y * q.x; }
	};

	// 曲線端の矢じりの両端点を求める.
	static bool bz_calc_arrow(const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROWHEAD_SIZE a_size, D2D1_POINT_2F barbs[3]) noexcept;

	// 曲線のジオメトリシンクに矢じりを追加する.
	static void bz_create_arrow_geometry(ID2D1Factory3* const d_factory, const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROWHEAD_STYLE a_style, const ARROWHEAD_SIZE a_size, ID2D1PathGeometry** a_geo);

	// 曲線上の助変数をもとに微分値を求める.
	static double bz_deriv_by_param(const BZP b_pos[4], const double t) noexcept;

	// 点の配列をもとにそれらをすべて含む凸包を求める.
	static void bz_get_convex(const uint32_t e_cnt, const BZP e_pos[], uint32_t& c_cnt, BZP c_pos[]);

	// 点と直線の間の最短距離を求める.
	//static double bz_shortest_dist(const BZP& p, const double a, const double b, const double c, const double d) noexcept;

	// 点が凸包に含まれるか判定する.
	static bool bz_in_convex(const double tx, const double ty, const size_t c_cnt, const BZP c_pos[]) noexcept;

	// 曲線上の長さをもとに助変数を求める.
	static double bz_param_by_len(const BZP b_pos[4], const double b_len) noexcept;

	// 曲線上の助変数をもとに位置を求める.
	static void bz_point_by_param(const BZP b_pos[4], const double t, BZP& p) noexcept;

	// 曲線上の助変数の区間をもとに長さを求める.
	static double bz_len_by_param(const BZP b_pos[4], const double t_min, const double t_max, const uint32_t sim_n) noexcept;

	// 2 つの助変数が区間 0-1 の間で正順か判定する.
	static bool bz_test_param(const double t_min, const double t_max) noexcept;

	// 曲線上の助変数をもとに接線ベクトルを求める.
	static void bz_tvec_by_param(const BZP b_pos[4], const double t, BZP& t_vec) noexcept;

	// 矢じりの両端点を計算する.
	// b_start	曲線の開始位置
	// b_seg	曲線の制御点
	// a_size	矢じりの寸法
	// barbs[3]	計算された両端点と先端点
	static bool bz_calc_arrow(const D2D1_POINT_2F b_start, const D2D1_BEZIER_SEGMENT& b_seg, const ARROWHEAD_SIZE a_size, D2D1_POINT_2F barbs[3]) noexcept
	{
		BZP seg[3]{};
		BZP b_pos[4]{};

		seg[0] = b_seg.point1;
		seg[1] = b_seg.point2;
		seg[2] = b_seg.point3;
		// 座標値による誤差を少なくできる, と思われるので,
		// ベジェ曲線を始点が原点となるように平行移動.
		b_pos[3] = 0.0;
		b_pos[2] = seg[0] - b_start;
		b_pos[1] = seg[1] - b_start;
		b_pos[0] = seg[2] - b_start;
		auto b_len = bz_len_by_param(b_pos, 0.0, 1.0, SIMPSON_N);
		if (b_len > FLT_MIN) {
			// 矢じりの先端のオフセット, または曲線の長さ, 
			// どちらか短い方で, 助変数を求める.
			const auto t = bz_param_by_len(b_pos, min(b_len, a_size.m_offset));
			// 助変数をもとに曲線の接線ベクトルを得る.
			BZP t_vec;
			bz_tvec_by_param(b_pos, t, t_vec);
			// 矢じりの返しの位置を計算する
			get_arrow_barbs(-t_vec, sqrt(t_vec * t_vec), a_size.m_width, a_size.m_length, barbs);
			// 助変数で曲線上の位置を得る.
			BZP t_pos;	// 終点を原点とする, 矢じりの先端の位置
			bz_point_by_param(b_pos, t, t_pos);
			// 曲線上の位置を矢じりの先端とし, 返しの位置も並行移動する.
			pt_add(barbs[0], t_pos.x, t_pos.y, barbs[0]);
			pt_add(barbs[1], t_pos.x, t_pos.y, barbs[1]);
			barbs[2] = t_pos;
			pt_add(barbs[0], b_start, barbs[0]);
			pt_add(barbs[1], b_start, barbs[1]);
			pt_add(barbs[2], b_start, barbs[2]);
			return true;
		}
		return false;
	}

	// 曲線のジオメトリシンクに矢じりを追加する
	// d_factory	D2D ファクトリ
	// b_pos	曲線の開始位置
	// b_seg	曲線の制御点
	// a_style	矢じりの種別
	// a_size	矢じりの寸法
	// a_geo	矢じりが追加されたジオメトリ
	static void bz_create_arrow_geometry(
		ID2D1Factory3* const d_factory,
		const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROWHEAD_STYLE a_style, const ARROWHEAD_SIZE a_size, 
		ID2D1PathGeometry** a_geo)
	{
		D2D1_POINT_2F barbs[3];	// 矢じりの返しの端点	
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (bz_calc_arrow(b_pos, b_seg, a_size, barbs)) {
			// ジオメトリシンクに追加する.
			winrt::check_hresult(d_factory->CreatePathGeometry(a_geo));
			winrt::check_hresult((*a_geo)->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(barbs[0], a_style == ARROWHEAD_STYLE::FILLED ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
			sink->AddLine(barbs[2]);
			sink->AddLine(barbs[1]);
			sink->EndFigure(a_style == ARROWHEAD_STYLE::FILLED ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
			winrt::check_hresult(sink->Close());
			sink = nullptr;
		}
	}

	// 曲線上の助変数をもとに微分値を求める.
	// b_pos	制御点
	// t	助変数
	static double bz_deriv_by_param(const BZP b_pos[4], const double t) noexcept
	{
		BZP t_vec;

		// 助変数をもとにベジェ曲線上の接線ベクトルを求め, その接線ベクトルの長さを返す.
		bz_tvec_by_param(b_pos, t, t_vec);
		return sqrt(t_vec * t_vec);
	}

	// 点の配列をもとにそれらをすべて含む凸包を求める.
	// ギフト包装法をもちいる.
	// e_cnt	点の数
	// e_pos	点の配列
	// c_cnt	凸包の頂点の数
	// c_pos	凸包の頂点の配列
	static void bz_get_convex(const uint32_t e_cnt, const BZP e_pos[], uint32_t& c_cnt, BZP c_pos[])
	{
		// e のうち, y 値が最も小さい点の集合から, x 値が最も小さい点の添え字 k を得る.
		uint32_t k = 0;
		auto ex = e_pos[0].x;
		auto ey = e_pos[0].y;
		for (uint32_t i = 1; i < e_cnt; i++) {
			if (e_pos[i].y < ey || (e_pos[i].y == ey && e_pos[i].x < ex)) {
				ex = e_pos[i].x;
				ey = e_pos[i].y;
				k = i;
			}
		}
		// e[k] を a に格納する.
		auto a = e_pos[k];
		c_cnt = 0;
		do {
			// c に a を追加する.
			c_pos[c_cnt++] = a;
			auto b = e_pos[0];
			for (uint32_t i = 1; i < e_cnt; i++) {
				const auto c = e_pos[i];
				if (b == a) {
					b = c;
				}
				else {
					const auto ab = b - a;
					const auto ac = c - a;
					const auto v = ab.opro(ac);
					if (v > 0.0 || (fabs(v) < FLT_MIN && ac * ac > ab * ab)) {
						b = c;
					}
				}
			}
			a = b;
		} while (c_cnt < e_cnt && a != c_pos[0]);
	}

	// 点と直線の間の最短距離を求める.
	/*
	static double bz_shortest_dist(const BZP& p, const double a, const double b, const double c, const double d) noexcept
	{
		return (a * p.x + b * p.y + c) / d;
	}
	*/

	// 二点を囲む方形を得る.
	/*
	static void bz_bound(const BZP& p, const BZP& q, BZP r[2])
	{
		if (p.x < q.x) {
			r[0].x = p.x;
			r[1].x = q.x;
		}
		else {
			r[0].x = q.x;
			r[1].x = p.x;
		}
		if (p.y < q.y) {
			r[0].y = p.y;
			r[1].y = q.y;
		}
		else {
			r[0].y = q.y;
			r[1].y = p.y;
		}
	}
	*/

	// 四つの点を囲む方形を得る.
	/*
	static void bz_bound(const BZP bz[4], BZP br[2])
	{
		bz_bound(bz[0], bz[1], br);
		br[0].x = min(br[0].x, bz[2].x);
		br[0].y = min(br[0].y, bz[2].y);
		br[0].x = min(br[0].x, bz[3].x);
		br[0].y = min(br[0].y, bz[3].y);
		br[1].x = max(br[1].x, bz[2].x);
		br[1].y = max(br[1].y, bz[2].y);
		br[1].x = max(br[1].x, bz[3].x);
		br[1].y = max(br[1].y, bz[3].y);
	}
	*/

	// 点 { 0, 0 } が凸包に含まれるか判定する.
	// 交差数判定を用いる.
	// c_cnt	凸包の頂点の数
	// c_pos	凸包の頂点の配列
	// 戻り値	含まれるなら true を, 含まれないなら false を返す.
	static bool bz_in_convex(const double tx, const double ty, const size_t c_cnt, const BZP c_pos[]) noexcept
	{
		int k = 0;	// 点をとおる水平線が凸包の辺と交差する回数.
		for (size_t i = c_cnt - 1, j = 0; j < c_cnt; i = j++) {
			// ルール 1. 上向きの辺. 点が垂直方向について, 辺の始点と終点の間にある. ただし、終点は含まない.
			// ルール 2. 下向きの辺. 点が垂直方向について, 辺の始点と終点の間にある. ただし、始点は含まない.
			// ルール 3. 点をとおる水平線と辺が水平でない.
			// ルール 1. ルール 2 を確認することで, ルール 3 も確認できている.
			if ((c_pos[i].y <= ty && c_pos[j].y > ty) || (c_pos[i].y > ty && c_pos[j].y <= ty)) {
				// ルール 4. 辺は点よりも右側にある. ただし, 重ならない.
				// 辺が点と同じ高さになる位置を特定し, その時の水平方向の値と点のその値とを比較する.
				if (tx < c_pos[i].x + (ty - c_pos[i].y) / (c_pos[j].y - c_pos[i].y) * (c_pos[j].x - c_pos[i].x)) {
					// ルール 1 またはルール 2, かつルール 4 を満たすなら, 点をとおる水平線は凸包と交差する.
					k++;
				}
			}
		}
		// 交差する回数が奇数か判定する.
		// 奇数ならば, 点は凸包に含まるので true を返す.
		return static_cast<bool>(k & 1);
	}

	// 曲線上の長さをもとに助変数を求める.
	// b_pos	制御点
	// b_len	長さ
	// 戻り値	得られた助変数の値
	static double bz_param_by_len(const BZP b_pos[4], const double b_len) noexcept
	{
		double t;	// 助変数
		double d;	// 助変数の変分
		double e;	// 誤差

		/* 区間の中間値 0.5 を助変数に格納する. */
		t = 0.5;
		/* 0.25 を助変数の変分に格納する. */
		/* 助変数の変分は 0.001953125 以上 ? */
		for (d = 0.25; d >= 0.001953125; d *= 0.5) {
			/* 0-助変数の範囲を合成シンプソン公式で積分し, 曲線の長さを求める. */
			/* 求めた長さと指定された長さの差分を誤差に格納する. */
			e = bz_len_by_param(b_pos, 0.0, t, SIMPSON_N) - b_len;
			/* 誤差の絶対は 0.125 より小 ? */
			if (fabs(e) < 0.125) {
				break;
			}
			/* 誤差は 0 より大 ? */
			else if (e > 0.0) {
				/* 変分を助変数から引く. */
				t -= d;
			}
			else {
				/* 変分を助変数に足す. */
				t += d;
			}
		}
		return t;
	}

	// 曲線上の助変数をもとに位置を求める.
	// b_pos	制御点
	// t	助変数
	// p	求まった位置
	static void bz_point_by_param(const BZP b_pos[4], const double t, BZP& p) noexcept
	{
		const double s = 1.0 - t;
		const double ss = s * s;
		const double tt = t * t;
		p = b_pos[0] * s * ss + b_pos[1] * 3.0 * ss * t + b_pos[2] * 3.0 * s * tt + b_pos[3] * t * tt;
	}

	// 方形が分割可能か判定し, その重心点を返す.
	/*
	static bool bz_dividable(const BZP r[2], BZP& mid)
	{
		mid = (r[0] + r[1]) * 0.5;
		return r[0] < mid.nextafter(-1.0) && r[1] > mid.nextafter(1.0);
	}
	*/

	// スタックから取り除く
	/*
	static void bz_pop(std::list<BZP>& stack, BZP bz[4])
	{
		bz[3] = stack.back();
		stack.pop_back();
		bz[2] = stack.back();
		stack.pop_back();
		bz[1] = stack.back();
		stack.pop_back();
		bz[0] = stack.back();
		stack.pop_back();
	}
	*/

	// スタックに積む.
	// i0 = 0	最初に積む位置の添え字
	// i1 = 1	2 番目に積む位置の添え字
	// i2 = 2	3 番目に積む位置の添え字
	// i3 = 3	4 番目に積む位置の添え字
	/*
	static void bz_push(std::list<BZP>& stack, const BZP bz[4], const uint32_t i0 = 0, const uint32_t i1 = 1, const uint32_t i2 = 2, const uint32_t i3 = 3)
	{
		stack.push_back(bz[i0]);
		stack.push_back(bz[i1]);
		stack.push_back(bz[i2]);
		stack.push_back(bz[i3]);
	}
	*/

	// 曲線上の助変数の区間をもとに長さを求める.
	// シンプソン法を用いる.
	// b_pos	制御点
	// t_min	区間の始端
	// t_max	区間の終端
	static double bz_len_by_param(const BZP b_pos[4], const double t_min, const double t_max, const uint32_t sim_n) noexcept
	{
		double t_diff;
		uint32_t n;
		double h;
		double a, b;
		double t;
		double b0, b2;
		double s;

		/* 範囲の上限下限は正順か判定する. */
		/* 正順 ? */
		if (bz_test_param(t_min, t_max)) {
			/* 範囲上限 t_max -範囲下限 t_min を差分 t_diff に格納する. */
			t_diff = t_max - t_min;
			/* 区間の分割数 sim_n と t_diff を乗算する. */
			/* その結果を切り上げて整数値する. */
			/* 整数値を区間の半数 n に格納する. */
			n = (int)std::ceil(t_diff * (double)sim_n);
			/* t_diff÷2n を階差 h に格納する. */
			h = t_diff / (2.0 * n);
			/* 0 を奇数番目の部分区間の合計値 a に格納する. */
			a = 0.0;
			/* 0 を偶数番目の部分区間の合計値 b に格納する. */
			b = 0.0;
			/* t_min+h を助変数 t に格納する. */
			t = t_min + h;
			/* 1 を添え字 i に格納する. */
			/* i は n より小 ? */
			for (uint32_t i = 1; i < n; i++) {
				/* 2i-1 番目の部分区間の微分値を求め, a に加える. */
				a += bz_deriv_by_param(b_pos, t);
				/* 階差 h を助変数 t に加える. */
				t += h;
				/* 2i 番目の部分区間の微分値を求め, b に加える. */
				b += bz_deriv_by_param(b_pos, t);
				/* 階差 h を助変数 t に加える. */
				t += h;
				/* i をインクリメントする. */
			}
			/* 2n-1 番目の部分区間の微分値を求め, a に加える. */
			a += bz_deriv_by_param(b_pos, t);
			/* 0 番目の部分区間での微分値を求め, b0 に格納する. */
			b0 = bz_deriv_by_param(b_pos, t_min);
			/* 2n 番目の部分区間での微分値を求め, b2 に格納する. */
			b2 = bz_deriv_by_param(b_pos, t_max);
			/* (b0+4×a+2×b+b2)×h/3 を求め, 積分値 s に格納する. */
			s = (b0 + 4.0 * a + 2.0 * b + b2) * h / 3.0f;
		}
		else {
			/* 0 を積分値 s に格納する. */
			s = 0.0;
		}
		/* s を返す. */
		return s;
	}

	// ベジェ曲線と方形が交わるか, 曲線を分割し, 判定する.
	/*
	static bool bz_splitting(const BZP BZ[4], const BZP pr[2])
	{
		BZP bz[10];
		BZP br[2];
		BZP br_mid;
		std::list<BZP> st;

		bz[0] = BZ[0];
		bz[1] = BZ[1];
		bz[2] = BZ[2];
		bz[3] = BZ[3];
		bz_push(st, bz);
		do {
			bz_pop(st, bz);
			bz_bound(bz, br);
			if (pr[0].x <= br[0].x && br[1].x <= pr[1].x
				&& pr[0].y <= br[0].y && br[1].y <= pr[1].y) {
				return true;
			}
			// 方形 br は方形 pr の少なくとも一部と重なる ?
			else if (br[1].x >= pr[0].x && br[1].y >= pr[0].y
				&& br[0].x <= pr[1].x && br[0].y <= pr[1].y) {
				if (bz_dividable(br, br_mid) != true) {
					return true;
				}
				bz[4] = (bz[0] + bz[1]) * 0.5;
				bz[5] = (bz[1] + bz[2]) * 0.5;
				bz[6] = (bz[2] + bz[3]) * 0.5;
				bz[7] = (bz[4] + bz[5]) * 0.5;
				bz[8] = (bz[5] + bz[6]) * 0.5;
				bz[9] = (bz[7] + bz[8]) * 0.5;
				// 分割されたベジェ制御点をスタックにプッシュする.
				bz_push(st, bz, 0, 4, 7, 9);
				bz_push(st, bz, 9, 8, 6, 3);
			}
		} while (st.empty() != true);
		return false;
	}
	*/

	// 2 つの助変数が区間 0-1 の間で正順か判定する.
	// t_min	小さい方の助変数
	// t_max	大きい方の助変数
	static bool bz_test_param(const double t_min, const double t_max) noexcept
	{
		// 範囲の上限 t_max は 1+DBL_EPSILON より小 ?
		// t_min より大きくて最も近い値は t_max より小 ?
		return -DBL_MIN < t_min && t_max < 1.0 + DBL_EPSILON && std::nextafter(t_min, t_min + 1.0) < t_max;
	}

	// 曲線上の助変数をもとに接線ベクトルを求める.
	// b_pos	曲線
	// t	助変数
	// t_vec	接線ベクトル
	static void bz_tvec_by_param(const BZP b_pos[4], const double t, BZP& t_vec) noexcept
	{
		const double a = -3.0 * (1.0 - t) * (1.0 - t);
		const double b = 3.0 * (1.0 - t) * (1.0 - 3.0 * t);
		const double c = 3.0 * t * (2.0 - 3.0 * t);
		const double d = (3.0 * t * t);
		t_vec = b_pos[0] * a + b_pos[1] * b + b_pos[2] * c + b_pos[3] * d;
	}

	// パスジオメトリを作成する.
	void ShapeBezi::create_path_geometry(ID2D1Factory3* const d_factory)
	{
		D2D1_BEZIER_SEGMENT b_seg;
		winrt::com_ptr<ID2D1GeometrySink> sink;

		m_d2d_path_geom = nullptr;
		m_d2d_arrow_geom = nullptr;
		pt_add(m_pos, m_diff[0], b_seg.point1);
		pt_add(b_seg.point1, m_diff[1], b_seg.point2);
		pt_add(b_seg.point2, m_diff[2], b_seg.point3);
		winrt::check_hresult(d_factory->CreatePathGeometry(m_d2d_path_geom.put()));
		m_d2d_path_geom->Open(sink.put());
		sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
		sink->BeginFigure(m_pos, D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
		sink->AddBezier(b_seg);
		sink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
		winrt::check_hresult(sink->Close());
		sink = nullptr;
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			bz_create_arrow_geometry(d_factory, m_pos, b_seg, m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
		}
	}

	// 図形を表示する.
	void ShapeBezi::draw(SHAPE_DX& dx)
	{
		D2D1_POINT_2F s_pos;
		D2D1_POINT_2F e_pos;

		if (is_opaque(m_stroke_color)) {
			const auto s_width = static_cast<FLOAT>(m_stroke_width);
			const auto s_brush = dx.m_shape_brush.get();
			const auto s_style = m_d2d_stroke_style.get();
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawGeometry(m_d2d_path_geom.get(), s_brush, s_width, s_style);
			if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
				const auto a_geom = m_d2d_arrow_geom.get();
				if (m_arrow_style == ARROWHEAD_STYLE::FILLED) {
					dx.m_d2dContext->FillGeometry(a_geom, s_brush, nullptr);
				}
				dx.m_d2dContext->DrawGeometry(a_geom, s_brush, s_width, s_style);
			}
			/*
			if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
				auto geo = m_d2d_arrow_geom.get();
				dx.m_d2dContext->DrawGeometry(geo, sb, sw, nullptr);
				if (m_arrow_style == ARROWHEAD_STYLE::FILLED) {
					dx.m_d2dContext->FillGeometry(geo, sb, nullptr);
				}
			}
			*/
		}
		if (is_selected() != true) {
			return;
		}
		D2D1_MATRIX_3X2_F tran;
		dx.m_d2dContext->GetTransform(&tran);
		const auto sw = static_cast<FLOAT>(1.0 / tran.m11);
		//auto sb = dx.m_anch_brush.get();
		//auto ss = dx.m_aux_style.get();

		anchor_draw_rect(m_pos, dx);
		s_pos = m_pos;
		pt_add(s_pos, m_diff[0], e_pos);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, dx.m_aux_style.get());
		anchor_draw_ellipse(e_pos, dx);

		s_pos = e_pos;
		pt_add(s_pos, m_diff[1], e_pos);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, dx.m_aux_style.get());
		anchor_draw_ellipse(e_pos, dx);

		s_pos = e_pos;
		pt_add(s_pos, m_diff[2], e_pos);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, dx.m_aux_style.get());
		anchor_draw_rect(e_pos, dx);
	}

	// 矢じりの寸法を得る.
	//bool ShapeBezi::get_arrow_size(ARROWHEAD_SIZE& value) const noexcept
	//{
	//	value = m_arrow_size;
	//	return true;
	//}

	// 矢じりの形式を得る.
	//bool ShapeBezi::get_arrow_style(ARROWHEAD_STYLE& value) const noexcept
	//{
	//	value = m_arrow_style;
	//	return true;
	//}

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位. 含まないときは「図形の外側」を返す.
	uint32_t ShapeBezi::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const auto e_width = max(max(m_stroke_width, a_len) * 0.5, 0.5);	// 線枠の太さの半分の値
		D2D1_POINT_2F tp;
		pt_sub(t_pos, m_pos, tp);
		// 判定する位置によって精度が落ちないよう, 開始位置が原点となるよう平行移動し, 制御点を得る.
		D2D1_POINT_2F c_pos[4];
		c_pos[0].x = c_pos[0].y = 0.0;
		//pt_sub(m_pos, t_pos, c_pos[0]);
		pt_add(c_pos[0], m_diff[0], c_pos[1]);
		pt_add(c_pos[1], m_diff[1], c_pos[2]);
		pt_add(c_pos[2], m_diff[2], c_pos[3]);
		if (pt_in_anch(tp, c_pos[3], a_len)) {
			return ANCH_TYPE::ANCH_P0 + 3;
		}
		if (pt_in_anch(tp, c_pos[2], a_len)) {
			return ANCH_TYPE::ANCH_P0 + 2;
		}
		if (pt_in_anch(tp, c_pos[1], a_len)) {
			return ANCH_TYPE::ANCH_P0 + 1;
		}
		if (pt_in_anch(tp, c_pos[0], a_len)) {
			return ANCH_TYPE::ANCH_P0 + 0;
		}
		// 最初の制御点の組をプッシュする.
		// ４つの点のうち端点は, 次につまれる組と共有するので, 1 + 3 * D_MAX 個の配列を確保する.
		constexpr auto D_MAX = 64;	// 分割する深さの最大値
		BZP s_arr[1 + D_MAX * 3] {};	// 制御点のスタック
		int32_t s_cnt = 0;	// スタックに積まれた点の数
		s_arr[0] = c_pos[0];
		s_arr[1] = c_pos[1];
		s_arr[2] = c_pos[2];
		s_arr[3] = c_pos[3];
		s_cnt += 4;
		// スタックに 一組以上の制御点が残っているか判定する.
		while (s_cnt >= 4) {
			// 制御点の組をポップする.
			BZP b_pos[10];
			b_pos[3] = s_arr[s_cnt - 1];
			b_pos[2] = s_arr[s_cnt - 2];
			b_pos[1] = s_arr[s_cnt - 3];
			b_pos[0] = s_arr[s_cnt - 4];
			// 端点は共有なのでピークする;
			s_cnt -= 4 - 1;
			// 制御点の組から凸包 c0 を得る (実際は方形で代用する).
			// 制御点の組から, 重複するものを除いた点の集合を得る.
			BZP c0_min = b_pos[0];	// 凸包 c0 (を含む方形の左上点)
			BZP c0_max = b_pos[0];	// 凸包 c0 (を含む方形の右下点)
			BZP d_pos[4];	// 重複しない点の集合.
			uint32_t d_cnt = 0;	// 重複しない点の集合の要素数
			d_pos[d_cnt++] = b_pos[0];
			for (uint32_t i = 1; i < 4; i++) {
				if (d_pos[d_cnt - 1] != b_pos[i]) {
					d_pos[d_cnt++] = b_pos[i];
					b_pos[i].inc(c0_min, c0_max);
				}
			}
			// 重複しない点の集合の要素数が 2 未満か判定する.
			if (d_cnt < 2) {
				// 制御点の組は 1 点に集まっているので, 中断する.
				continue;
			}

			// 拡張・延長された線分を得る.
			//   e[i][0]             e[i][1]
			//        + - - - - - - - - - +
			//        |          d_nor|         
			//   d[i] +---------------+---> d_vec
			//        |           d[i+1]
			//        + - - - - - - - - - +
			//   e[i][3]             e[i][2]
			BZP e_pos[3 * 4];	// 拡張・延長された線分の配列
			for (uint32_t i = 0, j = 0; i < d_cnt - 1; i++, j += 4) {
				auto d_vec = d_pos[i + 1] - d_pos[i];	// 線分のベクトル
				// 線分のベクトルの長さを, 太さの半分にする.
				d_vec = d_vec * (e_width / std::sqrt(d_vec * d_vec));
				const BZP d_nor{ d_vec.y, -d_vec.x };	// 線分の法線ベクトル

				// 法線ベクトルにそって正逆の方向に線分を拡張する.
				e_pos[j + 0] = d_pos[i] + d_nor;
				e_pos[j + 1] = d_pos[i + 1] + d_nor;
				e_pos[j + 2] = d_pos[i + 1] - d_nor;
				e_pos[j + 3] = d_pos[i] - d_nor;
				if (i > 0) {
					// 最初の制御点以外は, 線分ベクトルの方向に延長する.
					e_pos[j + 0] = e_pos[j + 0] - d_vec;
					e_pos[j + 3] = e_pos[j + 3] - d_vec;
				}
				if (i + 1 < d_cnt - 1) {
					// 最後の制御点以外は, 線分ベクトルの逆方向に延長する.
					e_pos[j + 1] = e_pos[j + 1] + d_vec;
					e_pos[j + 2] = e_pos[j + 2] + d_vec;
				}
			}
			// 拡張・延長された線分から, 凸包 c1 を得る.
			uint32_t c1_cnt;
			BZP c1_pos[3 * 4];
			bz_get_convex((d_cnt - 1) * 4, e_pos, c1_cnt, c1_pos);
			// 点が凸包 c1 に含まれないか判定する.
			if (!bz_in_convex(tp.x, tp.y, c1_cnt, c1_pos)) {
				// これ以上この制御点の組を分割する必要はない.
				// スタックに残った他の制御点の組を試す.
				continue;
			}

			// 凸包 c0 の大きさが 1 以下か判定する.
			BZP c0 = c0_max - c0_min;
			if (c0.x <= 1.0 && c0.y <= 1.0) {
				// 現在の制御点の組 (凸包 c0) をこれ以上分割する必要はない.
				// 凸包 c1 は判定する位置を含んでいるので, 図形の部位を返す.
				return ANCH_TYPE::ANCH_STROKE;
			}

			// スタックがオバーフローするか判定する.
			if (s_cnt + 6 > 1 + D_MAX * 3) {
				// 現在の制御点の組 (凸包 c0) をこれ以上分割することはできない.
				// 凸包 c1は判定する位置を含んでいるので, 図形の部位を返す.
				return ANCH_TYPE::ANCH_STROKE;
			}

			// 制御点の組を 2 分割する.
			// b[0,1,2,3] の中点を b[4,5,6] に, b[4,5,6] の中点を b[7,8] に, b[7,8] の中点を b[9] に格納する.
			// 分割した制御点の組はそれぞれ b[0,4,7,9] と b[9,8,6,3] になる.
			b_pos[4] = (b_pos[0] + b_pos[1]) * 0.5;
			b_pos[5] = (b_pos[1] + b_pos[2]) * 0.5;
			b_pos[6] = (b_pos[2] + b_pos[3]) * 0.5;
			b_pos[7] = (b_pos[4] + b_pos[5]) * 0.5;
			b_pos[8] = (b_pos[5] + b_pos[6]) * 0.5;
			b_pos[9] = (b_pos[7] + b_pos[8]) * 0.5;
			// 一方の組をプッシュする.
			// 始点 (0) はスタックに残っているので, 
			// 残りの 3 つの制御点をプッシュする.
			s_arr[s_cnt] = b_pos[4];
			s_arr[s_cnt + 1] = b_pos[7];
			s_arr[s_cnt + 2] = b_pos[9];
			// もう一方の組をプッシュする.
			// 始点 (9) はプッシュ済みなので,
			// 残りの 3 つの制御点をプッシュする.
			s_arr[s_cnt + 3] = b_pos[8];
			s_arr[s_cnt + 4] = b_pos[6];
			s_arr[s_cnt + 5] = b_pos[3];
			s_cnt += 6;
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// 範囲に含まれるか判定する.
	// 線の太さは考慮されない.
	// a_min	範囲の左上位置
	// a_max	範囲の右下位置
	// 戻り値	含まれるなら true
	bool ShapeBezi::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		// 計算精度がなるべく変わらないよう,
		// 範囲の左上が原点となるよう平行移動した制御点を得る.
		const auto w = static_cast<double>(a_max.x) - a_min.x;
		const auto h = static_cast<double>(a_max.y) - a_min.y;
		D2D1_POINT_2F c_pos[4];
		pt_sub(m_pos, a_min, c_pos[0]);
		pt_add(c_pos[0], m_diff[0], c_pos[1]);
		pt_add(c_pos[1], m_diff[1], c_pos[2]);
		pt_add(c_pos[2], m_diff[2], c_pos[3]);
		// 最初の制御点の組をプッシュする.
		constexpr auto D_MAX = 52;	// 分割する最大回数
		BZP s_arr[1 + D_MAX * 3] = {};
		int32_t s_cnt = 4;
		s_arr[0] = c_pos[0];
		s_arr[1] = c_pos[1];
		s_arr[2] = c_pos[2];
		s_arr[3] = c_pos[3];
		while (s_cnt >= 4) {
			// スタックが空でないなら, 制御点の組をポップする.
			// 端点は共有なのでピークする.
			BZP b_pos[10];
			b_pos[3] = s_arr[s_cnt - 1];
			b_pos[2] = s_arr[s_cnt - 2];
			b_pos[1] = s_arr[s_cnt - 3];
			b_pos[0] = s_arr[s_cnt - 4];
			s_cnt -= 3;
			// 始点が範囲の外にあるなら曲線は範囲に含まれない.
			if (b_pos[0].x < 0.0 || w < b_pos[0].x) {
				return false;
			}
			if (b_pos[0].y < 0.0 || h < b_pos[0].y) {
				return false;
			}
			// 終点が範囲の外にあるなら曲線は範囲に含まれない.
			if (b_pos[3].x < 0.0 || w < b_pos[3].x) {
				return false;
			}
			if (b_pos[3].y < 0.0 || h < b_pos[3].y) {
				return false;
			}
			// 他の 2 つの制御点が範囲内なら, 曲線のこの部分は範囲に含まれる.
			// さらに分割する必要はないので, スタックの残りの組について判定する.
			if (0.0 <= b_pos[1].x && b_pos[1].x <= w && 0.0 <= b_pos[1].y && b_pos[1].y <= h) {
				if (0.0 <= b_pos[2].x && b_pos[2].x <= w && 0.0 <= b_pos[2].y && b_pos[2].y <= h) {
					continue;
				}
			}
			// 制御点を含む領域を得る.
			BZP b_min = b_pos[0];
			BZP b_max = b_pos[0];
			b_pos[1].inc(b_min, b_max);
			b_pos[2].inc(b_min, b_max);
			b_pos[3].inc(b_min, b_max);
			BZP d = b_max - b_min;
			if (d.x <= 1.0 && d.y <= 1.0) {
				// 領域の各辺の大きさが 1 以下ならば, 
				// これ以上分割する必要はない.
				// 制御点の少なくとも 1 つが範囲に含まれてないのだから false を返す.
				return false;
			}
			if (s_cnt + 6 > 1 + D_MAX * 3) {
				// スタックオバーフローならこれ以上分割できない.
				// 制御点の少なくとも 1 つが範囲に含まれていないのなら false を返す.
				return false;
			}
			// 制御点の組を 2 分割する.
			b_pos[4] = (b_pos[0] + b_pos[1]) * 0.5;
			b_pos[5] = (b_pos[1] + b_pos[2]) * 0.5;
			b_pos[6] = (b_pos[2] + b_pos[3]) * 0.5;
			b_pos[7] = (b_pos[4] + b_pos[5]) * 0.5;
			b_pos[8] = (b_pos[5] + b_pos[6]) * 0.5;
			b_pos[9] = (b_pos[7] + b_pos[8]) * 0.5;
			// 一方の組をプッシュする.
			// 始点 (0) はスタックに残っているので, 
			// 残りの 3 つの制御点をプッシュする.
			s_arr[s_cnt] = b_pos[4];
			s_arr[s_cnt + 1] = b_pos[7];
			s_arr[s_cnt + 2] = b_pos[9];
			// もう一方の組をプッシュする.
			// 始点 (9) はプッシュ済みなので,
			// 残りの 3 つの制御点をプッシュする.
			s_arr[s_cnt + 3] = b_pos[8];
			s_arr[s_cnt + 4] = b_pos[6];
			s_arr[s_cnt + 5] = b_pos[3];
			s_cnt += 6;
		}
		return true;
	}

	// 矢じりの寸法に格納する.
	//void ShapeBezi::set_arrow_size(const ARROWHEAD_SIZE& value)
	//{
	//	if (equal(m_arrow_size, value)) {
	//		return;
	//	}
	//	m_arrow_size = value;
	//	create_path_geometry(s_d2d_factory);
	//}

	// 矢じりの形式に格納する.
	//void ShapeBezi::set_arrow_style(const ARROWHEAD_STYLE value)
	//{
	//	if (m_arrow_style == value) {
	//		return;
	//	}
	//	m_arrow_style = value;
	//	create_path_geometry(s_d2d_factory);
	//}

	// 値を始点に格納する. 他の部位の位置も動く.
	//void ShapeBezi::set_start_pos(const D2D1_POINT_2F value)
	//{
	//	ShapeStroke::set_start_pos(value);
	//	create_path_geometry(s_d2d_factory);
	//}

	// 図形を作成する.
	// b_pos	囲む領域の始点
	// b_diff	囲む領域の終点への差分
	// s_attr	属性
	ShapeBezi::ShapeBezi(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr) :
		ShapePath::ShapePath(3, s_attr)
	{
		m_pos = b_pos;
		m_diff[0].x = b_diff.x;
		m_diff[0].y = 0.0f;
		m_diff[1].x = -b_diff.x;
		m_diff[1].y = b_diff.y;
		m_diff[2].x = b_diff.x;
		m_diff[2].y = 0.0f;
		create_path_geometry(s_d2d_factory);
	}

	// 図形をデータリーダーから読み込む.
	ShapeBezi::ShapeBezi(DataReader const& dt_reader) :
		ShapePath::ShapePath(dt_reader)
	{
	//	m_arrow_style = static_cast<ARROWHEAD_STYLE>(dt_reader.ReadUInt32());
	//	m_arrow_size.m_width = dt_reader.ReadSingle();
	//	m_arrow_size.m_length = dt_reader.ReadSingle();
	//	m_arrow_size.m_offset = dt_reader.ReadSingle();
		create_path_geometry(s_d2d_factory);
	}

	// データライターに書き込む.
	//void ShapeBezi::write(DataWriter const& dt_writer) const
	//{
	//	ShapePath::write(dt_writer);
	//	dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));
	//	dt_writer.WriteSingle(m_arrow_size.m_width);
	//	dt_writer.WriteSingle(m_arrow_size.m_length);
	//	dt_writer.WriteSingle(m_arrow_size.m_offset);
	//}

	// データライターに SVG タグとして書き込む.
	void ShapeBezi::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;
		D2D1_BEZIER_SEGMENT b_seg;

		pt_add(m_pos, m_diff[0], b_seg.point1);
		pt_add(b_seg.point1, m_diff[1], b_seg.point2);
		pt_add(b_seg.point2, m_diff[2], b_seg.point3);
		write_svg("<path d=\"", dt_writer);
		write_svg(m_pos, "M", dt_writer);
		write_svg(b_seg.point1, "C", dt_writer);
		write_svg(b_seg.point2, ",", dt_writer);
		write_svg(b_seg.point3, ",", dt_writer);
		write_svg("\" ", dt_writer);
		write_svg("none", "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>" SVG_NEW_LINE, dt_writer);
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			D2D1_POINT_2F barbs[3];
			bz_calc_arrow(m_pos, b_seg, m_arrow_size, barbs);
			ShapeStroke::write_svg(barbs, barbs[2], m_arrow_style, dt_writer);
		}
	}

	/*
	static uint32_t clipping(const BZP& p, const BZP& q, double* t)
	{
		BZP bz[10];
		BZP br[2];
		BZP pr[2];

		uint32_t t_cnt;
		BZP br_mid;
		BZP pr_next;
		double dist;
		BZP pq;
		BZP pb;
		double a, b, c, d;
		double f0, f1, f2, f3;
		double s;
		double t_min, t_max;
		std::list<BZP> st;

		// 計算精度をなるべく一定にするため, ベジェ制御点 bz を, その始点が原点となるよう平行移動する.
		bz[0] = 0.0;
		bz[1].x = m_diff.x;
		bz[1].y = m_diff.y;
		bz[2].x = bz[1].x + m_diff_1.x;
		bz[2].y = bz[1].y + m_diff_1.y;
		bz[3].x = bz[2].x + m_diff_2.x;
		bz[3].y = bz[2].y + m_diff_2.y;
		// 線分 pq に外接する方形 pr を求める.
		pb.x = p.x - m_pos.x;
		pb.y = p.y - m_pos.y;
		bz_bound(p, q, pr);
		pr[0].x -= m_pos.x;
		pr[0].y -= m_pos.y;
		pr[1].x -= m_pos.x;
		pr[1].y -= m_pos.y;
		pr_next = pr[0].nextafter(1.0);
		pb = p - m_pos;
		pq = q - pb;
		a = pq.y / pq.x;
		b = -1.0;
		c = -a * pb.x + pb.y;
		d = std::sqrt(a * a + b * b);
		// 0 を助変数の個数に格納する.
		t_cnt = 0;
		// ベジェ制御点 b をスタックにプッシュする.
		bz_push(st, bz);
		do {
			bz_pop(st, bz);
			bz_bound(bz, br);
			// 方形 br は方形 pr の少なくとも一部と重なる ?
			if (pr[1].x >= br[0].x && pr[0].x <= br[1].x
				&& pr[1].y >= br[0].y && pr[0].y <= br[1].y) {
				if (bz_dividable(br, br_mid) != true) {
					// 線分 pq は y 軸にほぼ平行 ?
					if (pr[1].x < pr_next.x) {
						dist = br_mid.x - p.x;
					}
					// 線分 pq は x 軸にほぼ平行 ?
					else if (pr[1].y < pr_next.y) {
						// 中点と直線 pq の Y 成分の差分を長さに格納する.
						dist = br_mid.y - p.y;
					}
					else {
						// 中点と直線 pq の長さを求める.
						dist = bz_shortest_dist(br_mid, a, b, c, d);
					}
					if (dist < 1.0) {
						s = nearest(pb, pq, br_mid);
						if (s >= 0.0 && s <= 1.0) {
							if (t != nullptr) {
								t[t_cnt] = s;
							}
							t_cnt++;
						}
					}
				}
				else {
					// 線分 pq は y 軸にほぼ平行 ?
					if (pr[1].x < pr_next.x) {
						f0 = bz[0].x - pb.x;
						f1 = bz[1].x - pb.x;
						f2 = bz[2].x - pb.x;
						f3 = bz[3].x - pb.x;
					}
					// 線分 pq は x 軸にほぼ平行 ?
					else if (pr[1].y < pr_next.y) {
						// 中点と直線 pq の Y 成分の差分を長さに格納する.
						f0 = bz[0].y - pb.y;
						f1 = bz[1].y - pb.y;
						f2 = bz[2].y - pb.y;
						f3 = bz[3].y - pb.y;
					}
					else {
						f0 = bz_shortest_dist(bz[0], a, b, c, d);
						f1 = bz_shortest_dist(bz[1], a, b, c, d);
						f2 = bz_shortest_dist(bz[2], a, b, c, d);
						f3 = bz_shortest_dist(bz[3], a, b, c, d);
					}
					// 凸包 f の少なくとも 1 つは 0 以下 ?
					// (変換された凸包は t 軸と交わる ?)
					if ((f0 <= 0.0
						|| f1 <= 0.0
						|| f2 <= 0.0
						|| f3 <= 0.0)
						// 凸包 f の少なくとも 1 つは 0以上 ?
						&& (f0 >= 0.0
							|| f1 >= 0.0
							|| f2 >= 0.0
							|| f3 >= 0.0)) {
						// 1 を区間の最小値 tMin に, 0 を最大値 tMax に格納する.
						t_min = T3;
						t_max = T0;
						// 凸包と t 軸の交点を求め, 区間 tMin,tMax を更新.
						segment(T0, f0, T1, f1, t_min, t_max);
						segment(T0, f0, T2, f2, t_min, t_max);
						segment(T0, f0, T3, f3, t_min, t_max);
						segment(T1, f1, T2, f2, t_min, t_max);
						segment(T1, f1, T3, f3, t_min, t_max);
						segment(T2, f2, T3, f3, t_min, t_max);
						// 区間の差分は 0.5 以上 ?
						if (t_max - t_min >= 0.5f) {
							// 曲線を半分に分割し, 2 組の制御点を求める.
							bz[4] = (bz[0] + bz[1]) * 0.5;
							bz[5] = (bz[1] + bz[2]) * 0.5;
							bz[6] = (bz[2] + bz[3]) * 0.5;
							bz[7] = (bz[4] + bz[5]) * 0.5;
							bz[8] = (bz[5] + bz[6]) * 0.5;
							bz[9] = (bz[7] + bz[8]) * 0.5;
							// 一方の組の制御点をスタックにプッシュする.
							bz_push(st, bz, 0, 4, 7, 9);
							// もう一方の組の制御点をスタックにプッシュする.
							bz_push(st, bz, 9, 8, 6, 3);
						}
						// 区間の差分はほぼ 0 ?
						// tMax は tMin の浮動小数で表現可能な次の数より小 ?
						else if (t_max <= std::nextafter(t_min, t_min + 1.0)) {
							// 区間の収斂した値をもちいて曲線上の点 bzP を求める.
							BZP tp;
							bz_point_by_param(bz, t_min, tp);
							/* 直線 pq 上にあって, 点 bzP に最も近い点の助変数を得る.
							s = nearest(pb, pq, tp);
							// 助変数は 0 以上 1 以下 ?
							// (点は線分上にある ?)
							if (s >= 0.0 && s <= 1.0) {
								// 配列 t は NULL でない ?
								if (t != nullptr) {
									// 助変数を配列の tCnt 番目に格納する.
									t[t_cnt] = s;
								}
								// 助変数の個数 tCnt をインクリメントする.
								t_cnt++;
							}
						}
						/* 最大値 tMax はほぼ 0 より大 ?
						else if (t_max > DBL_MIN) {
							// 最大値 tMax 以下の制御点を求める.
							bz[4] = bz[0] + (bz[1] - bz[0]) * t_max;
							bz[5] = bz[1] + (bz[2] - bz[1]) * t_max;
							bz[6] = bz[2] + (bz[3] - bz[2]) * t_max;
							bz[7] = bz[4] + (bz[5] - bz[4]) * t_max;
							bz[8] = bz[5] + (bz[6] - bz[5]) * t_max;
							bz[9] = bz[7] + (bz[8] - bz[7]) * t_max;
							// 最小値 tMin を最大値 tMax で除して補正する.
							t_min /= t_max;
							// 補正した tMin 以上の制御点を求める.
							bz[1] = bz[0] + (bz[4] - bz[0]) * t_max;
							bz[2] = bz[4] + (bz[7] - bz[4]) * t_max;
							bz[3] = bz[7] + (bz[9] - bz[7]) * t_max;
							bz[5] = bz[1] + (bz[2] - bz[1]) * t_max;
							bz[6] = bz[2] + (bz[3] - bz[2]) * t_max;
							bz[8] = bz[5] + (bz[6] - bz[5]) * t_max;
							// 求めた制御点をスタックにプッシュする.
							bz_push(st, bz, 8, 6, 3, 9);
						}
					}
				}
			}
		} while (st.empty() != true);
		return t_cnt;
	}
	*/

}