//------------------------------
// 円弧
// 
// 円弧を描画する 3 つの方法 - 楕円の円弧
// https://learn.microsoft.com/ja-jp/xamarin/xamarin-forms/user-interface/graphics/skiasharp/curves/arcs
// パス - 円弧
// https://developer.mozilla.org/ja/docs/Web/SVG/Tutorial/Paths
// 3次ベジェ曲線を用いた楕円の近似
// https://clown.cube-soft.jp/entry/20090606/p1
//------------------------------
#include "pch.h"
#include "shape.h"
#include "shape_bezier.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 円弧の各点
	constexpr int AXIS_Y = 0;	// 標準形での垂直軸
	constexpr int AXIS_X = 1;	// 標準形での水平軸
	constexpr int CENTER = 2;	// 中心点
	constexpr int START = 3;	// 始点
	constexpr int END = 4;	// 終点

	// 円弧をベジェ曲線で近似する.
	static void arc_alter_bezier(const double px, const double py, const double rx, const double ry, const double c, const double s, const double t_min, const double t_max, const bool clockwise, D2D1_POINT_2F& b_start, D2D1_BEZIER_SEGMENT& b_seg) noexcept;
	// 円弧が含まれる象限を, 時計周りで得る.
	inline static int arc_quadrant_number(const double px, const double py, const double c, const double s) noexcept;
	// だ円の中心点を得る.
	static bool arc_center(const D2D1_POINT_2F start, const D2D1_POINT_2F end, const D2D1_SIZE_F rad, const double C, const double S, D2D1_POINT_2F& val) noexcept;

	// 円弧をベジェ曲線で近似する.
	// 得られたベジェ曲線の始点と制御点は, だ円の中心点を原点とする座標で得られる.
	// 助変数にちょうど 0-1 を与えると, 1/4 円弧が得られる.
	// 助変数を外挿できるが, 近似精度は悪くなる.
	// px, py	終点への位置ベクトル
	// rx, ry	X 軸方向の半径と, Y 軸方向の半径.
	// c, s	円弧の傾きのコサインとサイン.
	// t_min, t_max	近似されたベジェ曲線の助変数の下限と上限
	// clockwise	時計回りなら true, 反時計周りなら false
	// b_start, b_seg	近似されたベジェ曲線の始点と制御点.
	static void arc_alter_bezier(const double px, const double py, const double rx, const double ry, const double c, const double s, const double t_min, const double t_max, const bool clockwise,
		D2D1_POINT_2F& b_start, D2D1_BEZIER_SEGMENT& b_seg) noexcept
	{
		const int qn = arc_quadrant_number(px, py, c, s);	// 象限の番号
		if (qn != 1 && qn != 2 && qn != 3 && qn != 4) {
			b_start.x = 0.0f;
			b_start.y = 0.0f;
			b_seg.point1.x = 0.0f;
			b_seg.point1.y = 0.0f;
			b_seg.point2.x = 0.0f;
			b_seg.point2.y = 0.0f;
			b_seg.point3.x = 0.0f;
			b_seg.point3.y = 0.0f;
			return;
		}
		constexpr double a = 4.0 * (M_SQRT2 - 1.0) / 3.0;
		double start_x = 0.0;
		double start_y = 0.0;
		double b_seg1x = 0.0;
		double b_seg1y = 0.0;
		double b_seg2x = 0.0;
		double b_seg2y = 0.0;
		double b_seg3x = 0.0;
		double b_seg3y = 0.0;
		if (clockwise) {
			if (qn == 1) {
				start_x = 0.0f;
				start_y = -ry;
				b_seg1x = a * rx;
				b_seg1y = -ry;
				b_seg2x = rx;
				b_seg2y = -a * ry;
				b_seg3x = rx;
				b_seg3y = 0.0f;
			}
			else if (qn == 2) {
				start_x = rx;
				start_y = 0.0f;
				b_seg1x = rx;
				b_seg1y = a * ry;
				b_seg2x = a * rx;
				b_seg2y = ry;
				b_seg3x = 0.0f;
				b_seg3y = ry;
			}
			else if (qn == 3) {
				start_x = 0.0f;
				start_y = ry;
				b_seg1x = -a * rx;
				b_seg1y = ry;
				b_seg2x = -rx;
				b_seg2y = a * ry;
				b_seg3x = -rx;
				b_seg3y = 0.0f;
			}
			else if (qn == 4) {
				start_x = -rx;
				start_y = 0.0;
				b_seg1x = -rx;
				b_seg1y = -a * ry;
				b_seg2x = -a * rx;
				b_seg2y = -ry;
				b_seg3x = 0.0;
				b_seg3y = -ry;
			}
		}
		else {
			if (qn == 1) {
				start_x = 0.0;
				start_y = -ry;
				b_seg1x = 0.0;
				b_seg1y = a * -ry;
				b_seg2x = (1.0 - a) * rx;
				b_seg2y = 0.0;
				b_seg3x = rx;
				b_seg3y = 0.0f;
			}
			else if (qn == 2) {
				start_x = rx;
				start_y = 0.0f;
				b_seg1x = (1 - a) * rx;
				b_seg1y = 0.0;
				b_seg2x = 0.0;
				b_seg2y = (1 - a) * ry;
				b_seg3x = 0.0;
				b_seg3y = ry;
			}
			else if (qn == 3) {
				start_x = 0.0;
				start_y = ry;
				b_seg1x = 0.0;
				b_seg1y = (1 - a) * ry;
				b_seg2x = (1 - a) * -rx;
				b_seg2y = 0.0;
				b_seg3x = -rx;
				b_seg3y = 0.0;
			}
			else if (qn == 4) {
				start_x = -rx;
				start_y = 0.0;
				b_seg1x = (1 - a) * -rx;
				b_seg1y = 0.0;
				b_seg2x = 0.0;
				b_seg2y = (1 - a) * -ry;
				b_seg3x = 0.0;
				b_seg3y = -ry;
			}
		}
		double x0 = c * start_x - s * start_y;
		double y0 = s * start_x + c * start_y;
		double x1 = c * b_seg1x - s * b_seg1y;
		double y1 = s * b_seg1x + c * b_seg1y;
		double x2 = c * b_seg2x - s * b_seg2y;
		double y2 = s * b_seg2x + c * b_seg2y;
		double x3 = c * b_seg3x - s * b_seg3y;
		double y3 = s * b_seg3x + c * b_seg3y;
/*
		constexpr double a = 0.5522847498307933984;
		double x0;
		double y0;
		double x1;
		double y1;
		double x2;
		double y2;
		double x3;
		double y3;
		if (qn == 1) {
			x0 = -s * -ry;
			y0 = c * -ry;
			x1 = c * a * rx - s * -ry;
			y1 = s * a * rx + c * -ry;
			x2 = c * rx - s * -a * ry;
			y2 = s * rx + c * -a * ry;
			x3 = c * rx;
			y3 = s * rx;
		}
		else if (qn == 2) {
			x0 = c * rx;
			y0 = s * rx;
			x1 = c * rx - s * a * ry;
			y1 = s * rx + c * a * ry;
			x2 = c * a * rx - s * ry;
			y2 = s * a * rx + c * ry;
			x3 = -s * ry;
			y3 = c * ry;
		}
		else if (qn == 3) {
			x0 = -s * ry;
			y0 = c * ry;
			x1 = c * -a * rx - s * ry;
			y1 = s * -a * rx + c * ry;
			x2 = c * -rx - s * a * ry;
			y2 = s * -rx + c * a * ry;
			x3 = c * -rx;
			y3 = s * -rx;
		}
		else if (qn == 4) {
			x0 = c * -rx;
			y0 = s * -rx;
			x1 = c * -rx - s * -a * ry;
			y1 = s * -rx + c * -a * ry;
			x2 = c * -a * rx - s * -ry;
			y2 = s * -a * rx + c * -ry;
			x3 = -s * -ry;
			y3 = c * -ry;
		}
		else {
			b_start.x = 0.0f;
			b_start.y = 0.0f;
			b_seg.point1.x = 0.0f;
			b_seg.point1.y = 0.0f;
			b_seg.point2.x = 0.0f;
			b_seg.point2.y = 0.0f;
			b_seg.point3.x = 0.0f;
			b_seg.point3.y = 0.0f;
			return;
		}
		*/
		// 助変数の下限と上限であらわされる範囲の, 曲線の長さを得る.
		POINT_2D b[4]{
			{ x0, y0 },
			{ x1, y1 },
			{ x2, y2 },
			{ x3, y3 }
		};
		const double t_len = bezi_len_by_param(b, t_min, t_max, SIMPSON_CNT);
		// 助変数の下限で, 曲線を分割し, 後半の曲線を得る.
		double x4, y4;
		double x5, y5;
		double x6, y6;
		double x7, y7;
		double x8, y8;
		double x9, y9;
		if (t_min > FLT_MIN) {
			x4 = x0 + (x1 - x0) * t_min;
			y4 = y0 + (y1 - y0) * t_min;
			x5 = x1 + (x2 - x1) * t_min;
			y5 = y1 + (y2 - y1) * t_min;
			x6 = x2 + (x3 - x2) * t_min;
			y6 = y2 + (y3 - y2) * t_min;
			x7 = x4 + (x5 - x4) * t_min;
			y7 = y4 + (y5 - y4) * t_min;
			x8 = x5 + (x6 - x5) * t_min;
			y8 = y5 + (y6 - y5) * t_min;
			x9 = x7 + (x8 - x7) * t_min;
			y9 = y7 + (y8 - y7) * t_min;
			b[0].x = x9;
			b[0].y = y9;
			b[1].x = x8;
			b[1].y = y8;
			b[2].x = x6;
			b[2].y = y6;
		}
		// 得られた曲線を, 最初に求めた曲線の長さで分割し, 前半の曲線を得る.
		const double t_new = bezi_param_by_len(b, t_len);
		if (t_new > FLT_MIN) {
			x0 = b[0].x;
			y0 = b[0].y;
			x1 = b[1].x;
			y1 = b[1].y;
			x2 = b[2].x;
			y2 = b[2].y;
			x3 = b[3].x;
			y3 = b[3].y;
			x4 = x0 + (x1 - x0) * t_new;
			y4 = y0 + (y1 - y0) * t_new;
			x5 = x1 + (x2 - x1) * t_new;
			y5 = y1 + (y2 - y1) * t_new;
			x6 = x2 + (x3 - x2) * t_new;
			y6 = y2 + (y3 - y2) * t_new;
			x7 = x4 + (x5 - x4) * t_new;
			y7 = y4 + (y5 - y4) * t_new;
			x8 = x5 + (x6 - x5) * t_new;
			y8 = y5 + (y6 - y5) * t_new;
			x9 = x7 + (x8 - x7) * t_new;
			y9 = y7 + (y8 - y7) * t_new;
			b[1].x = x4;
			b[1].y = y4;
			b[2].x = x7;
			b[2].y = y7;
			b[3].x = x9;
			b[3].y = y9;
		}
		b_start.x = static_cast<FLOAT>(b[0].x);
		b_start.y = static_cast<FLOAT>(b[0].y);
		b_seg.point1.x = static_cast<FLOAT>(b[1].x);
		b_seg.point1.y = static_cast<FLOAT>(b[1].y);
		b_seg.point2.x = static_cast<FLOAT>(b[2].x);
		b_seg.point2.y = static_cast<FLOAT>(b[2].y);
		b_seg.point3.x = static_cast<FLOAT>(b[3].x);
		b_seg.point3.y = static_cast<FLOAT>(b[3].y);
		return;

		// ベジェ曲線を用いて正確に描ける曲線
		// https://qiita.com/HMMNRST/items/e20bd56ea875436d1709
		// ベジェ曲線とその接線の式は,
		// r(s) = (1-s)^3・r0 + 3s(1-s)^2・r1 + 3(s^2)(1-s)r2 + (s^3)r3
		// (1/3)(dr/dt) = (1-s)^2 (r1-r0) + 2s(1-s) (r2-r1) + (s^2)(r3-r2)
		// ただし, 0<=s<=1 とする.
		// これらの式に範囲 s を超える値 t (超えない値も) をいれると, あたらしい制御点が求まる.
		// その範囲を t0...t3 とすると制御点は,
		// q0 = r(t0)
		// q1 = r(t0) + (1/3)(dr/dt0)
		// q2 = r(t3) - (1/3)(dr/dt3)
		// q3 = r(t3)
		/*
		const double t0 = t_min;
		const double t3 = t_max;
		b_start.x = (1.0 - t0) * (1.0 - t0) * (1.0 - t0) * x0 + 3.0 * t0 * (1.0 - t0) * (1.0 - t0) * x1 + 3.0 * t0 * t0 * (1.0 - t0) * x2 + t0 * t0 * t0 * x3;
		b_start.y = (1.0 - t0) * (1.0 - t0) * (1.0 - t0) * y0 + 3.0 * t0 * (1.0 - t0) * (1.0 - t0) * y1 + 3.0 * t0 * t0 * (1.0 - t0) * y2 + t0 * t0 * t0 * y3;
		b_seg.point1.x = b_start.x + (1.0 - t0) * (1.0 - t0) * (x1 - x0) + 2.0 * t0 * (1.0 - t0) * (x2 - x1) + t0 * t0 * (x3 - x2);
		b_seg.point1.y = b_start.y + (1.0 - t0) * (1.0 - t0) * (y1 - y0) + 2.0 * t0 * (1.0 - t0) * (y2 - y1) + t0 * t0 * (y3 - y2);
		b_seg.point3.x = (1.0 - t3) * (1.0 - t3) * (1.0 - t3) * x0 + 3.0 * t3 * (1.0 - t3) * (1.0 - t3) * x1 + 3.0 * t3 * t3 * (1.0 - t3) * x2 + t3 * t3 * t3 * x3;
		b_seg.point3.y = (1.0 - t3) * (1.0 - t3) * (1.0 - t3) * y0 + 3.0 * t3 * (1.0 - t3) * (1.0 - t3) * y1 + 3.0 * t3 * t3 * (1.0 - t3) * y2 + t3 * t3 * t3 * y3;
		b_seg.point2.x = b_seg.point3.x - ((1.0 - t3) * (1.0 - t3) * (x1 - x0) + 2.0 * t3 * (1.0 - t3) * (x2 - x1) + t3 * t3 * (x3 - x2));
		b_seg.point2.y = b_seg.point3.y - ((1.0 - t3) * (1.0 - t3) * (y1 - y0) + 2.0 * t3 * (1.0 - t3) * (y2 - y1) + t3 * t3 * (y3 - y2));
		*/
		/*
		const double s0 = 1.0 - t_min;
		const double ss0 = s0 * s0;
		const double sss0 = ss0 * s0;
		const double t0 = t_min;
		const double tt0 = t0 * t0;
		const double ttt0 = tt0 * t0;
		const double s3 = 1.0 - t_max;
		const double ss3 = s3 * s3;
		const double sss3 = ss3 * s3;
		const double t3 = t_max;
		const double tt3 = t3 * t3;
		const double ttt3 = tt3 * t3;
		const double x1_0 = x1 - x0;
		const double y1_0 = y1 - y0;
		const double x2_1 = x2 - x1;
		const double y2_1 = y2 - y1;
		const double x3_2 = x3 - x2;
		const double y3_2 = y3 - y2;
		const double new_x0 = sss0 * x0 + 3.0 * t0 * ss0 * x1 + 3.0 * tt0 * s0 * x2 + ttt0 * x3;
		const double new_y0 = sss0 * y0 + 3.0 * t0 * ss0 * y1 + 3.0 * tt0 * s0 * y2 + ttt0 * y3;
		const double new_x3 = sss3 * x0 + 3.0 * t3 * ss3 * x1 + 3.0 * tt3 * s3 * x2 + ttt3 * x3;
		const double new_y3 = sss3 * y0 + 3.0 * t3 * ss3 * y1 + 3.0 * tt3 * s3 * y2 + ttt3 * y3;
		if (dir) {
			b_start.x = static_cast<FLOAT>(new_x0);
			b_start.y = static_cast<FLOAT>(new_y0);
			b_seg.point1.x = static_cast<FLOAT>(new_x0 + ss0 * x1_0 + 2.0 * t0 * s0 * x2_1 + tt0 * x3_2);
			b_seg.point1.y = static_cast<FLOAT>(new_y0 + ss0 * y1_0 + 2.0 * t0 * s0 * y2_1 + tt0 * y3_2);
			b_seg.point3.x = static_cast<FLOAT>(new_x3);
			b_seg.point3.y = static_cast<FLOAT>(new_y3);
			b_seg.point2.x = static_cast<FLOAT>(new_x3 - (ss3 * x1_0 + 2.0 * t3 * s3 * x2_1 + tt3 * x3_2));
			b_seg.point2.y = static_cast<FLOAT>(new_y3 - (ss3 * y1_0 + 2.0 * t3 * s3 * y2_1 + tt3 * y3_2));
		}
		else {
			b_seg.point3.x = static_cast<FLOAT>(new_x0);
			b_seg.point3.y = static_cast<FLOAT>(new_y0);
			b_seg.point2.x = static_cast<FLOAT>(new_x0 + (ss0 * x1_0 + 2.0 * t0 * s0 * x2_1 + tt0 * x3_2));
			b_seg.point2.y = static_cast<FLOAT>(new_y0 + (ss0 * y1_0 + 2.0 * t0 * s0 * y2_1 + tt0 * y3_2));
			b_start.x = static_cast<FLOAT>(new_x3);
			b_start.y = static_cast<FLOAT>(new_y3);
			b_seg.point1.x = static_cast<FLOAT>(new_x3 - (ss3 * x1_0 + 2.0 * t3 * s3 * x2_1 + tt3 * x3_2));
			b_seg.point1.y = static_cast<FLOAT>(new_y3 - (ss3 * y1_0 + 2.0 * t3 * s3 * y2_1 + tt3 * y3_2));
		}
		*/
	}

	// 円弧が含まれる象限を, 時計周りで得る.
	// 戻り値	象限の番号 (1,2,3,4). 終点ベクトルがゼロベクトルなら 0.
	// 用紙の Y 軸は下向きだが, 向かって右上を第 1 象限とする.
	// px, py	円弧の始点から終点への位置ベクトル
	// c, s	円弧の傾きのコサインとサイン (傾きは, 時計周りが正)
	inline static int arc_quadrant_number(const double px, const double py, const double c, const double s) noexcept
	{
		// 円弧の, 始点から終点への位置ベクトルを, 円弧の傾きが 0 になるよう回転して戻す.
		const double qx = c * px + s * py;
		const double qy = -s * px + c * py;
		// 第 1 象限 (正方向の Y軸を含み, X 軸は含まない)
		//       ‖-   \
		// -     ‖  1  \
		// ------+------
		//       |     +
		//      +|
		if (qx >= 0.0 && qy > 0.0) {
			return 1;
		}
		// 第 2 象限 (正方向の X軸を含み, Y 軸は含まない)
		//      -|
		//       |     +
		// ------+======
		// -     |  2  /
		//       |+   /
		else if (qx < 0.0 && qy >= 0.0) {
			return 2;
		}
		// 第 3 象限 (負方向の Y軸を含み, X 軸は含まない)
		//       |-
		// -     |
		// ------+------
		// \  3  ‖     +
		//  \   +‖
		else if (qx <= 0.0 && qy < 0.0) {
			return 3;
		}
		// 第 4 象限 (負方向の X軸を含み, Y 軸は含まない)
		//  /   -|
		// /  4  |
		// ======+------
		// -     |     +
		//      +|
		else if (qx > 0.0 && qy <= 0.0) {
			return 4;
		}
		return 0;
	}

	// だ円の中心点を得る.
	static bool arc_center(
		const D2D1_POINT_2F start, const D2D1_POINT_2F end,	// 円弧の開始点と終点
		const D2D1_SIZE_F rad,	// だ円の半径 (標準形における X 軸方向と Y 軸方向)
		const double C, const double S,	// 円弧の傾きのコサインとサイン.
		D2D1_POINT_2F& val	// 得られた中心点.
	) noexcept
	{
		// だ円の中心点を求める.
		// A = 1 / (rx^2)
		// B = 1 / (ry^2)
		// C = cos(θ)
		// S = sin(θ)
		// 点 p (px, py) を円の中心 (ox, oy) を原点とする座標に平行移動して, 回転する.
		// x = C・(px - ox) + S・(py - oy)
		// y =-S・(px - ox) + C・(py - oy)
		// 点 q についても同様.
		// これらを, だ円の標準形 A・x^2 + B・y^2 = 1 に代入する.
		// A・{ C・(px - ox) + S・(py - oy) }^2 + B・{ -S・(px - ox) + C・(py - oy) }^2 = 1 ...[1]
		// A・{ C・(qx - ox) + S・(qy - oy) }^2 + B・{ -S・(qx - ox) + C・(qy - oy) }^2 = 1 ...[2]
		// [1] 式の第 1 項を ox, oy について展開する.
		// A・{ C・(px - ox) + S・(py - oy) }^2 =
		// A・{ C・px - C・ox + S・py - S・oy }^2 = 
		// A・{ -C・ox - S・oy + (C・px + S・py) }^2 =
		// A・C^2・ox^2 + 2A・C・S・ox・oy + A・S^2・oy^2 - 2A・C・(C・px + S・py)・ox - 2A・S・(C・px + S・py)・oy + A・(C・px + S・py)^2
		// [1] 式の第 2 項も同様.
		// B・{ -S・(px - ox) + C・(py - oy) }^2 =
		// B・{ -S・px + S・ox + C・py - C・oy }^2 = 
		// B・{  S・ox - C・oy - (S・px - C・py) }^2 =
		// B・S^2・ox^2 - 2B・S・C・ox・oy + B・C^2・oy^2 - 2B・S・(S・px - C・py)・ox + 2B・C・(S・px - C・py)・oy + B・(S・px - C・py)^2
		// したがって [1] 式は,
		// (A・C^2 + B・S^2)・ox^2 + (2A・C・S - 2B・S・C)・ox・oy + (A・S^2 + B・C^2)・oy^2 - (2A・C・(C・px + S・py) + 2B・S・(S・px - C・py))・ox - (2A・S・(C・px + S・py) - 2B・C・(S・px - C・py))・oy + (A・(C・px + S・py)^2 + B・(S・px - C・py)^2)
		//  (A・(C・px + S・py)^2 + B・(S・px - C・py)^2)
		// [2] 式は, [1] 式に含まれる px, py を, qx, qy に置き換えるだけ.
		// ox^2, ox・oy, oy^2 の係数は　px, py を含まない.
		// したがってこれらの係数は [1]-[2] で消え, 1 次の項である ox と oy, 定数項が残る.
		// d = -(2A・C・(C・px + S・py) + 2B・S・(S・px - C・py)) ... [1] 式の ox の項
		// e = -(2A・S・(C・px + S・py) - 2B・C・(S・px - C・py)) ... [1] 式の oy の項
		// f = A・(C・px + S・py)^2 + B・(S・px - C・py)^2 ... [1] 定数項
		// d・ox + e・oy + f = g・ox + h・oy + i
		// oy = (g - d)/(e - h)・ox + (i - f)/(e - h)
		// oy = j・ox + k
		// これを [1] 式に代入して,
		// A・{ C・(px - ox) + S・(py - oy) }^2 = 1
		// A・{ C・(px - ox) + S・(py - j・ox - k) }^2 + B・{ S・(px - ox) - C・(py - j・ox - k) }^2 - 1 = 0 ...[3]
		// [3] 式を ox について展開する
		// [3] 式の第 1 項は,
		// A・{ C・(px - ox) + S・(py - j・ox - k) }^2 =
		// A・{ C・px - C・ox + S・py - S・j・ox - S・k }^2 =
		// A・{-(C + S・j)・ox + (C・px + S・py - S・k) }^2 =
		// A・(C + S・j)^2・ox^2 - 2A・(C + S・j)(C・px + S・py - S・k)・ox + A・(C・px + S・py - S・k)^2
		// [3] 式の第 2 項は,
		// B・{ S・(px - ox) - C・(py - j・ox - k) }^2 =
		// B・{ S・px - S・ox - C・py + C・j・ox + C・k) }^2 =
		// B・{-(S - C・j)・ox + (S・px - C・py + C・k) }^2 =
		// B・(S - C・j)^2・ox^2 - 2B・(S - C・j)(S・px - C・py + C・k)・ox + B・(S・px - C・py + C・k)^2
		// [3] 式を a・ox^2 + b・ox + c = 0 とすると,
		// a = A・(C + S・j)^2 + B・(S - C・j)^2
		// b = -2A・(C + S・j)(C・px + S・py - S・k) - 2B・(S - C・j)(S・px - C・py + C・k)
		// c = A・(C・px + S・py - S・k)^2 + B・(S・px - C・py + C・k)^2 - 1
		// 2 次方程式の解公式に代入すれば, ox が求まる.
		const double px = start.x;
		const double py = start.y;
		const double qx = end.x;
		const double qy = end.y;
		const double A = 1.0 / (static_cast<double>(rad.width) * static_cast<double>(rad.width));
		const double B = 1.0 / (static_cast<double>(rad.height) * static_cast<double>(rad.height));
		const double d = -2 * A * C * (C * px + S * py) - 2 * B * S * (S * px - C * py);
		const double e = -2 * A * S * (C * px + S * py) + 2 * B * C * (S * px - C * py);
		const double f = A * (C * px + S * py) * (C * px + S * py) + B * (S * px - C * py) * (S * px - C * py);
		const double g = -2 * A * C * (C * qx + S * qy) - 2 * B * S * (S * qx - C * qy);
		const double h = -2 * A * S * (C * qx + S * qy) + 2 * B * C * (S * qx - C * qy);
		const double i = A * (C * qx + S * qy) * (C * qx + S * qy) + B * (S * qx - C * qy) * (S * qx - C * qy);
		const double j = (g - d) / (e - h);
		const double k = (i - f) / (e - h);
		const double a = A * (C + S * j) * (C + S * j) + B * (S - C * j) * (S - C * j);
		const double b = -2 * A * (C + S * j) * (C * px + S * py - S * k) - 2 * B * (S - C * j) * (S * px - C * py + C * k);
		const double c = A * (C * px + S * py - S * k) * (C * px + S * py - S * k) + B * (S * px - C * py + C * k) * (S * px - C * py + C * k) - 1;
		const double bb_4ac = b * b - 4 * a * c;
		if (bb_4ac <= FLT_MIN) {
			return false;
			//__debugbreak();
		}
		const double s = bb_4ac <= FLT_MIN ? 0.0f : sqrt(bb_4ac);
		const double ox = (-b + s) / (a + a);
		const double oy = j * ox + k;
		const double vx = px - ox;
		const double vy = py - oy;
		const double wx = qx - ox;
		const double wy = qy - oy;
		if (vx * wy - vy * wx >= 0.0) {
			val.x = static_cast<FLOAT>(ox);
			val.y = static_cast<FLOAT>(oy);
		}
		else {
			const double x = (-b - s) / (a + a);
			val.x = static_cast<FLOAT>(x);
			val.y = static_cast<FLOAT>(j * x + k);
		}
		return true;
	}

	// 指定した部位の点を得る.
	// loc	部位
	// val	得られた値
	void ShapeArc::get_pos_loc(const uint32_t loc, D2D1_POINT_2F& val) const noexcept
	{
		if (loc == LOCUS_TYPE::LOCUS_P0) {
			D2D1_POINT_2F p[5];
			get_verts(p);
			val = p[AXIS_Y];
		}
		else if (loc == LOCUS_TYPE::LOCUS_P0 + 1) {
			D2D1_POINT_2F p[5];
			get_verts(p);
			val = p[AXIS_X];
		}
		else if (loc == LOCUS_TYPE::LOCUS_A_START) {
			D2D1_POINT_2F p[5];
			get_verts(p);
			val = p[START];
		}
		else if (loc == LOCUS_TYPE::LOCUS_A_END) {
			D2D1_POINT_2F p[5];
			get_verts(p);
			val = p[END];
		}
		else if (loc == LOCUS_TYPE::LOCUS_A_CENTER) {
			D2D1_POINT_2F p[5];
			get_verts(p);
			val = p[CENTER];
		}
		else if (loc == LOCUS_TYPE::LOCUS_A_AXIS_X) {
			D2D1_POINT_2F p[5];
			get_verts(p);
			val = p[AXIS_X];
		}
		else if (loc == LOCUS_TYPE::LOCUS_A_AXIS_Y) {
			D2D1_POINT_2F p[5];
			get_verts(p);
			val = p[AXIS_Y];
		}
		else {
			__debugbreak();
		}
	}

	// 点を得る.
	size_t ShapeArc::get_verts(D2D1_POINT_2F p[]) const noexcept
	{
		constexpr double R[]{ 0.0, 90.0, 180.0, 270.0 };

		p[AXIS_Y] = m_start;
		p[AXIS_X].x = m_start.x + m_lineto[0].x;
		p[AXIS_X].y = m_start.y + m_lineto[0].y;
		const double r = M_PI * m_angle_rot / 180.0;
		const double c = cos(r);
		const double s = sin(r);
		arc_center(p[AXIS_Y], p[AXIS_X], m_radius, c, s, p[CENTER]);

		const double t_min = 0.0 + m_angle_start / 90.0;
		const double t_max = 1.0 + m_angle_end / 90.0;
		const bool clockwise = (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE);
		//const double rot = M_PI * m_angle_rot / 180.0;
		//const double c = cos(rot);
		//const double s = sin(rot);
		D2D1_POINT_2F b_start;
		D2D1_BEZIER_SEGMENT b_seg;
		arc_alter_bezier(m_lineto[0].x, m_lineto[0].y, m_radius.width, m_radius.height, c, s, t_min, t_max, clockwise, b_start, b_seg);
		p[START].x = b_start.x + p[CENTER].x;
		p[START].y = b_start.y + p[CENTER].y;
		p[END].x = b_seg.point3.x + p[CENTER].x;
		p[END].y = b_seg.point3.y + p[CENTER].y;
		return 5;
	}

	// 値を傾き角度に格納する.
	// val	格納する値
	bool ShapeArc::set_arc_rot(const float val) noexcept
	{
		if (equal(m_angle_rot, val)) {
			return false;
		}
		const double old_r = M_PI * m_angle_rot / 180.0;	// 元の傾き
		const auto old_c = cos(old_r);
		const auto old_s = sin(old_r);
		const int qn = arc_quadrant_number(m_lineto[0].x, m_lineto[0].y, old_c, old_s);	// 象限番号

		// だ円での中心点を得る.
		D2D1_POINT_2F ctr;
		D2D1_POINT_2F end{
			m_start.x + m_lineto[0].x, m_start.y + m_lineto[0].y
		};
		arc_center(m_start, end, m_radius, old_c, old_s, ctr);

		// 新しいだ円の傾き.
		const double new_r = M_PI * val / 180.0;
		const auto new_c = cos(-new_r);
		const auto new_s = sin(-new_r);
		if (qn == 1) {
			constexpr double px = 0.0;
			const double py = -m_radius.height;
			const double qx = m_radius.width;
			constexpr double qy = 0.0;
			m_start.x = static_cast<FLOAT>(new_c * px + new_s * py + ctr.x);
			m_start.y = static_cast<FLOAT>(-new_s * px + new_c * py + ctr.y);
			m_lineto.resize(1);
			m_lineto[0].x = static_cast<FLOAT>(new_c * qx + new_s * qy + ctr.x - m_start.x);
			m_lineto[0].y = static_cast<FLOAT>(-new_s * qx + new_c * qy + ctr.y - m_start.y);
		}
		else if (qn == 2) {
			const double px = m_radius.width;
			constexpr double py = 0.0f;
			constexpr double qx = 0.0f;
			const double qy = m_radius.height;
			m_start.x = static_cast<FLOAT>(new_c * px + new_s * py + ctr.x);
			m_start.y = static_cast<FLOAT>(-new_s * px + new_c * py + ctr.y);
			m_lineto.resize(1);
			m_lineto[0].x = static_cast<FLOAT>(new_c * qx + new_s * qy + ctr.x - m_start.x);
			m_lineto[0].y = static_cast<FLOAT>(-new_s * qx + new_c * qy + ctr.y - m_start.y);
		}
		else if (qn == 3) {
			constexpr double px = 0.0;
			const double py = m_radius.height;
			const double qx = -m_radius.width;
			constexpr double qy = 0.0;
			m_start.x = static_cast<FLOAT>(new_c * px + new_s * py + ctr.x);
			m_start.y = static_cast<FLOAT>(-new_s * px + new_c * py + ctr.y);
			m_lineto.resize(1);
			m_lineto[0].x = static_cast<FLOAT>(new_c * qx + new_s * qy + ctr.x - m_start.x);
			m_lineto[0].y = static_cast<FLOAT>(-new_s * qx + new_c * qy + ctr.y - m_start.y);
		}
		else if (qn == 4) {
			const double px = -m_radius.width;
			constexpr double py = 0.0f;
			constexpr double qx = 0.0f;
			const double qy = -m_radius.height;
			m_start.x = static_cast<FLOAT>(new_c * px + new_s * py + ctr.x);
			m_start.y = static_cast<FLOAT>(-new_s * px + new_c * py + ctr.y);
			m_lineto.resize(1);
			m_lineto[0].x = static_cast<FLOAT>(new_c * qx + new_s * qy + ctr.x - m_start.x);
			m_lineto[0].y = static_cast<FLOAT>(-new_s * qx + new_c * qy + ctr.y - m_start.y);
		}
		else {
			m_start = ctr;
			m_lineto[0] = ctr;
		}
		m_angle_rot = val;
		m_d2d_fill_geom = nullptr;
		m_d2d_path_geom = nullptr;
		m_d2d_arrow_geom = nullptr;
		return true;
	}

	// 値を円弧の始点の角度に格納する.
	bool ShapeArc::set_arc_start(const float val) noexcept
	{
		if (val >= 0.0 && val <= 45.0 && !equal(m_angle_start, val)) {
			m_angle_start = val;
			m_d2d_fill_geom = nullptr;
			m_d2d_path_geom = nullptr;
			m_d2d_arrow_geom = nullptr;
			return true;
		}
		return false;
	}

	// 値を円弧の終点の角度に格納する.
	bool ShapeArc::set_arc_end(const float val) noexcept
	{
		if (val >= -45.0 && val <= 0.0 && !equal(m_angle_end, val)) {
			m_angle_end = val;
			m_d2d_fill_geom = nullptr;
			m_d2d_path_geom = nullptr;
			m_d2d_arrow_geom = nullptr;
			return true;
		}
		return false;
	}

	// 値を円弧を描く方向に格納する.
	bool ShapeArc::set_arc_dir(const D2D1_SWEEP_DIRECTION val) noexcept
	{
		if (m_sweep_dir != val) {
			// 始点に終点を格納し, 位置ベクトルを反転.
			//m_start.x += m_pos[0].x;
			//m_start.y += m_pos[0].y;
			//m_pos[0].x = -m_pos[0].x;
			//m_pos[0].y = -m_pos[0].y;
			m_sweep_dir = val;
			m_d2d_fill_geom = nullptr;
			m_d2d_path_geom = nullptr;
			m_d2d_arrow_geom = nullptr;
			return true;
		}
		return false;
	}

	// 値を, 指定した部位の点に格納する.
	// val	値
	// loc	部位
	// snap_point	点と点をくっつける間隔
	// keep_aspect	画像の縦横比の維持/可変
	bool ShapeArc::set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool keep_aspect) noexcept
	{
		if (loc == LOCUS_TYPE::LOCUS_A_AXIS_Y) {
			const double a = M_PI * m_angle_rot / 180.0;
			const double c = cos(a);
			const double s = sin(a);
			const D2D1_POINT_2F end{
				m_start.x + m_lineto[0].x, m_start.y + m_lineto[0].y
			};
			D2D1_POINT_2F ctr2;
			if (arc_center(m_start, end, m_radius, c, s, ctr2)) {
				const double ax = m_start.x + m_lineto[0].x - ctr2.x;	// 水平軸の X 座標
				const double ay = m_start.y + m_lineto[0].y - ctr2.y;	// 水平軸の Y 座標
				const double ad = sqrt(ax * ax + ay * ay);	// a の長さ
				if (ad > FLT_MIN) {
					const double px = val.x - ctr2.x;
					const double py = val.y - ctr2.y;
					const double pd = sqrt(px * px + py * py);	// v の長さ
					if (pd > FLT_MIN) {
						float old_rot;
						get_arc_rot(old_rot);
						const double new_rot = old_rot + 90.0 - acos((px * ax + py * ay) / (ad * pd)) * 180.0 / M_PI;
						if (fabs(new_rot) < 45.0) {
							set_arc_rot(static_cast<float>(new_rot));
						}
					}
				}
			}
		}
		else if (loc == LOCUS_TYPE::LOCUS_A_AXIS_X) {
			const double a = M_PI * m_angle_rot / 180.0;
			const double c = cos(a);
			const double s = sin(a);
			const D2D1_POINT_2F end{
				m_start.x + m_lineto[0] .x, m_start.y + m_lineto[0].y
			};
			D2D1_POINT_2F ctr2;
			if (arc_center(m_start, end, m_radius, c, s, ctr2)) {
				const double ax = m_start.x - ctr2.x;	// 垂直軸の X 座標
				const double ay = m_start.y - ctr2.y;	// 垂直軸の Y 座標
				const double ad = sqrt(ax * ax + ay * ay);	// a の長さ
				if (ad > FLT_MIN) {
					const double px = val.x - ctr2.x;
					const double py = val.y - ctr2.y;
					const double pd = sqrt(px * px + py * py);	// v の長さ
					if (pd > FLT_MIN) {
						float old_rot;
						get_arc_rot(old_rot);
						const double new_rot = old_rot - 90.0 + acos((px * ax + py * ay) / (ad * pd)) * 180.0 / M_PI;
						if (fabs(new_rot) < 45.0) {
							set_arc_rot(static_cast<float>(new_rot));
						}
					}
				}
			}
		}
		else if (loc == LOCUS_TYPE::LOCUS_A_START) {
			const double a = M_PI * m_angle_rot / 180.0;
			const double c = cos(a);
			const double s = sin(a);
			D2D1_POINT_2F start = m_start;
			const D2D1_POINT_2F end{
				m_start.x + m_lineto[0].x, m_start.y + m_lineto[0].y
			};
			D2D1_POINT_2F ctr2;
			if (arc_center(start, end, m_radius, c, s, ctr2)) {
				if (m_sweep_dir != D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
					ctr2.x = ctr2.x + (m_start.x - ctr2.x) + (m_start.x + m_lineto[0].x - ctr2.x);
					ctr2.y = ctr2.y + (m_start.y - ctr2.y) + (m_start.y + m_lineto[0].y - ctr2.y);
				}
				int qn = arc_quadrant_number(m_lineto[0].x, m_lineto[0].y, c, s);	// 象限番号
				if (m_sweep_dir != D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
					if (qn == 1) {
						qn = 2;
					}
					else if (qn == 2) {
						qn = 3;
					}
					else if (qn == 3) {
						qn = 4;
					}
					else if (qn == 4) {
						qn = 1;
					}
				}
				if (qn == 1 || qn == 3) {
					const double vx = static_cast<double>(val.x) - ctr2.x;
					const double vy = static_cast<double>(val.y) - ctr2.y;
					const double ty = -s * vx + c * vy;
					const bool clockwise = (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE);
					const double a = ty / (clockwise ? -m_radius.height : m_radius.height);
					if (abs(a) > FLT_MIN) {
						const double tx = c * vx + s * vy;
						const double b = tx / m_radius.width;
						const double r = atan(b / a);
						const double d = 180.0 * r / M_PI;
						if (d >= 0.0 && d <= 45.0) {
							m_angle_start = static_cast<float>(d);
							m_d2d_arrow_geom = nullptr;
							m_d2d_path_geom = nullptr;
							m_d2d_fill_geom = nullptr;
							return true;
						}
					}
				}
				else {
					const double vx = static_cast<double>(val.x) - ctr2.x;
					const double vy = static_cast<double>(val.y) - ctr2.y;
					const double tx = c * vx + s * vy;
					const bool clockwise = (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE);
					const double a = tx / (clockwise ? m_radius.width : -m_radius.width);
					if (abs(a) > FLT_MIN) {
						const double ty = -s * vx + c * vy;
						const double b = ty / m_radius.height;
						const double r = atan(b / a);
						const double d = 180.0 * r / M_PI;
						if (d >= 0.0 && d <= 45.0) {
							m_angle_start = static_cast<float>(d);
							m_d2d_arrow_geom = nullptr;
							m_d2d_path_geom = nullptr;
							m_d2d_fill_geom = nullptr;
							return true;
						}
					}
				}
			}
		}
		else if (loc == LOCUS_TYPE::LOCUS_A_END) {
			const double rot = M_PI * m_angle_rot / 180.0;
			const double c = cos(rot);
			const double s = sin(rot);
			const D2D1_POINT_2F end{
				m_start.x + m_lineto[0].x, m_start.y + m_lineto[0].y
			};
			D2D1_POINT_2F ctr2;
			if (arc_center(m_start, end, m_radius, c, s, ctr2)) {
				if (m_sweep_dir != D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
					ctr2.x = ctr2.x + (m_start.x - ctr2.x) + (m_start.x + m_lineto[0].x - ctr2.x);
					ctr2.y = ctr2.y + (m_start.y - ctr2.y) + (m_start.y + m_lineto[0].y - ctr2.y);
				}
				int qn = arc_quadrant_number(m_lineto[0].x, m_lineto[0].y, c, s);
				if (m_sweep_dir != D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
					if (qn == 1) {
						qn = 2;
					}
					else if (qn == 2) {
						qn = 3;
					}
					else if (qn == 3) {
						qn = 4;
					}
					else if (qn == 4) {
						qn = 1;
					}
				}

				if (qn == 1 || qn == 3) {
					const double vx = static_cast<double>(val.x) - ctr2.x;
					const double vy = static_cast<double>(val.y) - ctr2.y;
					const double tx = c * vx + s * vy;
					const bool clockwise = (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE);
					const double a = tx / (clockwise ? m_radius.width : -m_radius.width);
					if (abs(a) > FLT_MIN) {
						const double ty = -s * vx + c * vy;
						const double b = ty / m_radius.height;
						const double r = atan(b / a);
						const double d = 180.0 * r / M_PI;
						if (d >= -45.0 && d <= 0.0) {
							m_angle_end = static_cast<float>(d);
							m_d2d_arrow_geom = nullptr;
							m_d2d_path_geom = nullptr;
							m_d2d_fill_geom = nullptr;
							return true;
						}
					}
				}
				else {
					const double vx = static_cast<double>(val.x) - ctr2.x;
					const double vy = static_cast<double>(val.y) - ctr2.y;
					const double ty = -s * vx + c * vy;
					const bool clockwise = (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE);
					const double a = ty / (clockwise ? -m_radius.height : m_radius.height);
					if (abs(a) > FLT_MIN) {
						const double tx = c * vx + s * vy;
						const double b = tx / m_radius.width;
						const double r = atan(b / a);
						const double d = 180.0 * r / M_PI;
						if (d >= -45.0 && d <= 0.0) {
							m_angle_end = static_cast<float>(d);
							m_d2d_arrow_geom = nullptr;
							m_d2d_path_geom = nullptr;
							m_d2d_fill_geom = nullptr;
							return true;
						}
					}
				}
			}
		}
		else if (loc == LOCUS_TYPE::LOCUS_A_CENTER) {
			const double rot = M_PI * m_angle_rot / 180.0;
			const double c = cos(rot);
			const double s = sin(rot);
			const D2D1_POINT_2F end{
				m_start.x + m_lineto[0].x, m_start.y + m_lineto[0].y
			};
			D2D1_POINT_2F ctr2;
			if (arc_center(m_start, end, m_radius, c, s, ctr2)) {
				if (m_sweep_dir != D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
					ctr2.x = ctr2.x + (m_start.x - ctr2.x) + (m_start.x + m_lineto[0].x - ctr2.x);
					ctr2.y = ctr2.y + (m_start.y - ctr2.y) + (m_start.y + m_lineto[0].y - ctr2.y);
				}
				const D2D1_POINT_2F start{
					static_cast<FLOAT>(m_start.x + val.x - ctr2.x),
					static_cast<FLOAT>(m_start.y + val.y - ctr2.y)
				};
				if (ShapePath::set_pos_start(start)) {
					m_d2d_fill_geom = nullptr;
					return true;
				}
			}
		}
		else {
			if (ShapePath::set_pos_loc(val, loc, snap_point, keep_aspect)) {
				const double rot = M_PI * m_angle_rot / 180.0;
				const double c = cos(rot);
				const double s = sin(rot);
				const double x = m_lineto[0].x;
				const double y = m_lineto[0].y;
				const double w = c * x + s * y;
				const double h = -s * x + c * y;
				m_radius.width = static_cast<FLOAT>(fabs(w));
				m_radius.height = static_cast<FLOAT>(fabs(h));
				m_d2d_fill_geom = nullptr;
				return true;
			}
		}
		return false;
	}

	// 値を始点に格納する. 他の部位の位置も動く.
	bool ShapeArc::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		if (ShapePath::set_pos_start(val)) {
			m_d2d_fill_geom = nullptr;
			return true;
		}
		return false;
	}

	/*
	static bool arc_get_tangent(const double x0, const double y0, const double rot, const double rw, const double rh, double& ox, double& oy)
	{
		// だ円 A・x^2 + B・y^2 = 1 における点 p { x0, y0 } の接線は
		// (A・x0)・x + (B・y0)・y = 1
		//const double rot = M_PI * m_angle_rot / 180.0;
		const double c = cos(rot);
		const double s = sin(rot);
		//const double ew = m_stroke_width * 0.5;	// 辺の半分の幅.
		//const double x0 = sx;
		//const double y0 = sy;
		// p を回転移動し, だ円の標準形での接線の係数を得る.
		//const double rw = abs(m_radius.width);
		//const double rh = abs(m_radius.height);
		const double Ax0 = (c * x0 + s * y0) / (rw * rw);
		const double By0 = (-s * x0 + c * y0) / (rh * rh);
		// 得られた接線を逆に回転移動し, 傾いただ円における接線を得る.
		const double a = c * Ax0 - s * By0;
		const double b = s * Ax0 + c * By0;
		// 得られた接線から, 接線に垂直なベクトル o (長さは辺の半分の幅) を得る.
		const double ab = sqrt(a * a + b * b);
		ox = a / ab;
		oy = b / ab;
		return ab > FLT_MIN;
	}
	*/
	// 図形が点を含むか判定する.
	// test_pt	判定する点
	// 戻り値	点を含む部位
	uint32_t ShapeArc::hit_test(const D2D1_POINT_2F test_pt, const bool ctrl_key) const noexcept
	{
		D2D1_POINT_2F p[5];
		get_verts(p);
		if (ctrl_key && loc_hit_test(test_pt, p[END], m_loc_width)) {
			return LOCUS_TYPE::LOCUS_A_END;
		}
		else if (ctrl_key && loc_hit_test(test_pt, p[START], m_loc_width)) {
			return LOCUS_TYPE::LOCUS_A_START;
		}
		else if (loc_hit_test(test_pt, p[AXIS_X], m_loc_width)) {
			return ctrl_key ? LOCUS_TYPE::LOCUS_A_AXIS_X : LOCUS_TYPE::LOCUS_P0 + 1;
		}
		else if (loc_hit_test(test_pt, p[AXIS_Y], m_loc_width)) {
			return ctrl_key ? LOCUS_TYPE::LOCUS_A_AXIS_Y : LOCUS_TYPE::LOCUS_P0;
		}
		else if (!ctrl_key && loc_hit_test(test_pt, p[END], m_loc_width)) {
			return LOCUS_TYPE::LOCUS_A_END;
		}
		else if (!ctrl_key && loc_hit_test(test_pt, p[START], m_loc_width)) {
			return LOCUS_TYPE::LOCUS_A_START;
		}
		else if (loc_hit_test(test_pt, p[CENTER], m_loc_width)) {
			return LOCUS_TYPE::LOCUS_FILL;
		}
		D2D1_POINT_2F b_start;
		D2D1_BEZIER_SEGMENT b_seg;
		alter_bezier(b_start, b_seg);
		D2D1_POINT_2F b_pt[4]{
			{ 0.0f, 0.0f },
			{ b_seg.point1.x - b_start.x, b_seg.point1.y - b_start.y },
			{ b_seg.point2.x - b_start.x, b_seg.point2.y - b_start.y },
			{ b_seg.point3.x - b_start.x, b_seg.point3.y - b_start.y }
		};
		D2D1_POINT_2F b_to[3]{
			{ b_seg.point1.x - b_start.x, b_seg.point1.y - b_start.y },
			{ b_seg.point2.x - b_seg.point1.x, b_seg.point2.y - b_seg.point1.y },
			{ b_seg.point3.x - b_seg.point2.x, b_seg.point3.y - b_seg.point2.y }
		};
		const auto ew = max(max(static_cast<double>(m_stroke_width), m_loc_width) * 0.5, 0.5);
		const auto loc = ShapeBezier::hit_test({ test_pt.x - b_start.x, test_pt.y - b_start.y }, b_pt, b_to, m_stroke_cap, true, ew);
		if (loc == LOCUS_TYPE::LOCUS_STROKE) {
			return LOCUS_TYPE::LOCUS_STROKE;
		}
		else if (is_opaque(m_fill_color)) {
			if (loc == LOCUS_TYPE::LOCUS_FILL) {
				if (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
					return LOCUS_TYPE::LOCUS_FILL;
				}
			}
			else {
				const D2D1_POINT_2F tri[3]{
					p[CENTER], p[START], p[END]
				};
				if (pt_in_poly(test_pt, 3, tri)) {
					return LOCUS_TYPE::LOCUS_FILL;
				}
			}
		}
		return LOCUS_TYPE::LOCUS_SHEET;
	}

	// 円弧をベジェ曲線で近似する.
	// ただし, だ円の中心点と始点, 終点がなす角度が 90 度以上, 開けば開くほどズレる.
	// bezi_start	曲線の始点
	// bezi_seg	曲線の制御点
	void ShapeArc::alter_bezier(D2D1_POINT_2F& bezi_start, D2D1_BEZIER_SEGMENT& bezi_seg) const noexcept
	{
		const double rot = M_PI * m_angle_rot / 180.0;
		const double c = cos(rot);
		const double s = sin(rot);
		const double t_min = 0.0 + m_angle_start / 90.0;
		const double t_max = 1.0 + m_angle_end / 90.0;
		const bool clockwise = (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE);
		arc_alter_bezier(m_lineto[0].x, m_lineto[0].y, m_radius.width, m_radius.height, c, s, t_min, t_max, clockwise, bezi_start, bezi_seg);
		D2D1_POINT_2F p[5];
		get_verts(p);
		// 精度がさがらないよう, 始点と終点には, 近似したベジェ曲線からでなく, 元の始点と終点を格納する.
		bezi_start = p[START];
		bezi_seg.point1.x += p[CENTER].x;
		bezi_seg.point1.y += p[CENTER].y;
		bezi_seg.point2.x += p[CENTER].x;
		bezi_seg.point2.y += p[CENTER].y;
		bezi_seg.point3 = p[END];
	}

	// 矢じりの返しと先端の位置を得る
	bool ShapeArc::arc_get_pos_arrow(
		const D2D1_POINT_2F line_to,	// 始点からの終点ベクトル.
		const D2D1_POINT_2F ctr_pt,	// 円弧がなすだ円の中心点
		const D2D1_SIZE_F rad,	//	標準形における X 軸方向の半径と, Y 軸方向の半径.
		const double deg_start,	// 始点の角度
		const double deg_end,	// 終点の角度
		const double deg_rot,	// 円弧がなすだ円の傾き
		const ARROW_SIZE a_size,	// 矢じりの大きさ
		const D2D1_SWEEP_DIRECTION sweep_dir,	// 円弧を掃く方向
		D2D1_POINT_2F arrow_pt[]	// 矢じりの返しと先端の位置
	)
	{
		D2D1_POINT_2F b_start{};	// ベジェ曲線の始点
		D2D1_BEZIER_SEGMENT b_seg{};	// ベジェ曲線の制御点
		const double rot = M_PI * deg_rot / 180.0;	// 円弧の傾き
		const double t_min = 0.0 + deg_start / 90.0;	// 助変数の下限
		const double t_max = 1.0 + deg_end / 90.0;	// 助変数の上限
		const double c = cos(rot);
		const double s = sin(rot);
		const bool clockwise = (sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE);
		arc_alter_bezier(line_to.x, line_to.y, rad.width, rad.height, c, s, t_min, t_max, clockwise, b_start, b_seg);
		if (ShapeBezier::bezi_get_pos_arrow(b_start, b_seg, a_size, arrow_pt)) {
			// 得られた各点は, だ円中心点を原点とする座標なので, もとの座標へ戻す.
			arrow_pt[0].x += ctr_pt.x;
			arrow_pt[0].y += ctr_pt.y;
			arrow_pt[1].x += ctr_pt.x;
			arrow_pt[1].y += ctr_pt.y;
			arrow_pt[2].x += ctr_pt.x;
			arrow_pt[2].y += ctr_pt.y;
			return true;
		}
		return false;
	}

	void ShapeArc::draw(void) noexcept
	{
		ID2D1RenderTarget* target = Shape::m_d2d_target;
		ID2D1Factory* factory;
		target->GetFactory(&factory);
		ID2D1SolidColorBrush* brush = Shape::m_d2d_color_brush.get();
		HRESULT hr = S_OK;
		const bool exist_stroke = (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color));

		D2D1_POINT_2F p[5]{};
		if (exist_stroke || (is_opaque(m_fill_color) && m_d2d_fill_geom == nullptr || is_selected())) {
			get_verts(p);
		}
		if (exist_stroke && m_d2d_stroke_style == nullptr) {
			hr = create_stroke_style(factory);
		}
		if (exist_stroke && m_d2d_path_geom == nullptr) {
			D2D1_ARC_SEGMENT arc{
				p[END],
				D2D1_SIZE_F{ fabsf(m_radius.width), fabsf(m_radius.height) },
				m_angle_rot,
				m_sweep_dir,
				//D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE,
				m_larg_flag
			};
			winrt::com_ptr<ID2D1GeometrySink> sink;
			if (hr == S_OK) {
				hr = factory->CreatePathGeometry(m_d2d_path_geom.put());
			}
			if (hr == S_OK) {
				hr = m_d2d_path_geom->Open(sink.put());
			}
			if (hr == S_OK) {
				sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
				const auto f_begin = (is_opaque(m_fill_color) ?
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
				sink->BeginFigure(p[START], f_begin);
				sink->AddArc(arc);
				sink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
				hr = sink->Close();
			}
			sink = nullptr;
		}
		if (exist_stroke && m_arrow_style != ARROW_STYLE::ARROW_NONE && m_d2d_arrow_geom == nullptr) {
			// だ円の弧長を求めるのはしんどいので, ベジェで近似
			D2D1_POINT_2F arrow[3];
			arc_get_pos_arrow(m_lineto[0], p[CENTER], m_radius, m_angle_start, m_angle_end, m_angle_rot, m_arrow_size, m_sweep_dir,
				arrow);
			winrt::com_ptr<ID2D1GeometrySink> sink;
			const ARROW_STYLE a_style{
				m_arrow_style
			};
			// ジオメトリパスを作成する.
			if (hr == S_OK) {
				hr = factory->CreatePathGeometry(m_d2d_arrow_geom.put());
			}
			if (hr == S_OK) {
				hr = m_d2d_arrow_geom->Open(sink.put());
			}
			if (hr == S_OK) {
				sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
				const auto f_begin = (a_style == ARROW_STYLE::ARROW_FILLED
					? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
					: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
				const auto f_end = (a_style == ARROW_STYLE::ARROW_FILLED
					? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED
					: D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
				sink->BeginFigure(arrow[0], f_begin);
				sink->AddLine(arrow[2]);
				sink->AddLine(arrow[1]);
				sink->EndFigure(f_end);
				hr = sink->Close();
			}
			sink = nullptr;
		}
		if (exist_stroke && m_arrow_style != ARROW_STYLE::ARROW_NONE && m_d2d_arrow_stroke == nullptr) {
			hr = create_arrow_stroke();
		}
		if (is_opaque(m_fill_color) && m_d2d_fill_geom == nullptr) {
			D2D1_ARC_SEGMENT arc{
				p[END],
				D2D1_SIZE_F{ fabsf(m_radius.width), fabsf(m_radius.height) },
				m_angle_rot,
				m_sweep_dir,
				//D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE,
				m_larg_flag
			};
			winrt::com_ptr<ID2D1GeometrySink> sink;
			if (hr == S_OK) {
				hr = factory->CreatePathGeometry(m_d2d_fill_geom.put());
			}
			if (hr == S_OK) {
				hr = m_d2d_fill_geom->Open(sink.put());
			}
			if (hr == S_OK) {
				sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
				const auto f_begin = (is_opaque(m_fill_color) ?
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
				sink->BeginFigure(p[START], f_begin);
				sink->AddArc(arc);
				sink->AddLine(p[CENTER]);
				sink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED);
			}
			hr = sink->Close();
			sink = nullptr;
		}
		if (hr == S_OK) {
			if (m_d2d_fill_geom != nullptr) {
				brush->SetColor(m_fill_color);
				target->FillGeometry(m_d2d_fill_geom.get(), brush);
			}
			if (m_d2d_path_geom != nullptr) {
				brush->SetColor(m_stroke_color);
				target->DrawGeometry(m_d2d_path_geom.get(), brush, m_stroke_width, m_d2d_stroke_style.get());
				if (m_d2d_arrow_geom != nullptr) {
					if (m_arrow_style == ARROW_STYLE::ARROW_FILLED) {
						target->FillGeometry(m_d2d_arrow_geom.get(), brush);
						target->DrawGeometry(m_d2d_arrow_geom.get(), brush, m_stroke_width, m_d2d_arrow_stroke.get());
					}
					if (m_arrow_style == ARROW_STYLE::ARROW_OPENED) {
						target->DrawGeometry(m_d2d_arrow_geom.get(), brush, m_stroke_width,
							m_d2d_arrow_stroke.get());
					}
				}
			}
			if (m_loc_show && is_selected()) {
				// 補助線を描く
				if (m_stroke_width >= Shape::m_loc_square_inner) {
					brush->SetColor(COLOR_WHITE);
					target->DrawGeometry(m_d2d_path_geom.get(), brush, 2.0f * m_aux_width, nullptr);
					brush->SetColor(COLOR_BLACK);
					target->DrawGeometry(m_d2d_path_geom.get(), brush, m_aux_width, m_aux_style.get());
				}
				// 軸の補助線を描く.
				brush->SetColor(COLOR_WHITE);
				target->DrawLine(p[CENTER], p[AXIS_Y], brush, m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawLine(p[CENTER], p[AXIS_Y], brush, m_aux_width, m_aux_style.get());
				brush->SetColor(COLOR_WHITE);
				target->DrawLine(p[CENTER], p[AXIS_X], brush, m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawLine(p[CENTER], p[AXIS_X], brush, m_aux_width, m_aux_style.get());
				// 図形の部位を描く.
				loc_draw_rhombus(p[CENTER], target, brush);
				loc_draw_circle(p[START], target, brush);
				loc_draw_circle(p[END], target, brush);
				loc_draw_square(p[AXIS_Y], target, brush);
				loc_draw_square(p[AXIS_X], target, brush);
			}
		}
	}

	ShapeArc::ShapeArc(
		const D2D1_POINT_2F start,
		const D2D1_POINT_2F end_to,
		const Shape* prop
	) : 
		ShapePath(prop, false),
		m_radius(D2D1_SIZE_F{ fabs(end_to.x), fabs(end_to.y) }),
		m_angle_rot(0.0f),
		m_angle_start(0.0f),
		m_angle_end(0.0f),
		m_sweep_dir(D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE),
		m_larg_flag(D2D1_ARC_SIZE_SMALL)
	{
		m_start = start;	// 始点
		m_lineto.push_back(end_to);	// 終点
		if (typeid(*prop) == typeid(ShapeArc)) {
			const float angle_start = static_cast<const ShapeArc*>(prop)->m_angle_start;
			const float angle_end = static_cast<const ShapeArc*>(prop)->m_angle_end;
			const float angle_rot = static_cast<const ShapeArc*>(prop)->m_angle_rot;
			const D2D1_SWEEP_DIRECTION dir = static_cast<const ShapeArc*>(prop)->m_sweep_dir;
			set_arc_start(angle_start);
			set_arc_end(angle_end);
			set_arc_rot(angle_rot);
			set_arc_dir(dir);
		}
	}

	ShapeArc::ShapeArc(const DataReader& dt_reader) :
		ShapePath(dt_reader),
		m_radius({ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_angle_rot(dt_reader.ReadSingle()),
		m_angle_start(dt_reader.ReadSingle()),
		m_angle_end(dt_reader.ReadSingle()),
		m_sweep_dir(static_cast<D2D1_SWEEP_DIRECTION>(dt_reader.ReadUInt32())),
		m_larg_flag(static_cast<D2D1_ARC_SIZE>(dt_reader.ReadUInt32()))
	{
		if (m_radius.width < 0.0f) {
			m_radius.width = GRID_LEN_DEFVAL;
		}
		if (m_radius.height < 0.0f) {
			m_radius.height = GRID_LEN_DEFVAL;
		}
		if (fabs(m_angle_rot) > 45.0) {
			m_angle_rot = 0.0f;
		}
		if (fabs(m_angle_start) > 45.0) {
			m_angle_start = 0.0f;
		}
		if (fabs(m_angle_end) > 45.0) {
			m_angle_end = 0.0f;
		}
		if (m_sweep_dir != D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE &&
			m_sweep_dir != D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE) {
			m_sweep_dir = D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE;
		}
		//if (m_larg_flag != D2D1_ARC_SIZE::D2D1_ARC_SIZE_SMALL &&
		//	m_larg_flag != D2D1_ARC_SIZE::D2D1_ARC_SIZE_LARGE) {
		//	m_larg_flag = D2D1_ARC_SIZE::D2D1_ARC_SIZE_SMALL;
		//}
		if (m_larg_flag != D2D1_ARC_SIZE::D2D1_ARC_SIZE_SMALL) {
			m_larg_flag = D2D1_ARC_SIZE::D2D1_ARC_SIZE_SMALL;
		}
	}

	void ShapeArc::write(const DataWriter& dt_writer) const
	{
		ShapePath::write(dt_writer);
		dt_writer.WriteSingle(m_radius.width);
		dt_writer.WriteSingle(m_radius.height);
		dt_writer.WriteSingle(m_angle_rot);
		dt_writer.WriteSingle(m_angle_start);
		dt_writer.WriteSingle(m_angle_end);
		dt_writer.WriteUInt32(m_sweep_dir);
		dt_writer.WriteUInt32(m_larg_flag);
	}

}