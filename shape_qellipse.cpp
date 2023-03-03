//------------------------------
// 四分だ円 (円弧)
// 
// 円弧を描画する 3 つの方法 - 楕円の円弧
// https://learn.microsoft.com/ja-jp/xamarin/xamarin-forms/user-interface/graphics/skiasharp/curves/arcs
// パス - 円弧
// https://developer.mozilla.org/ja/docs/Web/SVG/Tutorial/Paths
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 円弧の各点
	constexpr int AXIS1 = 0;	// 最初の軸
	constexpr int AXIS2 = 1;	// 次の軸
	constexpr int CENTER = 2;	// 中心点
	constexpr int START = 3;	// 始点
	constexpr int END = 4;	// 終点

	// 円弧が含まれる象限を得る.
	static int qellipse_quadrant_number(const double px, const double py, const double c, const double s);

	// 四分だ円をベジェ曲線で近似する.
	// px, py	終点への位置ベクトル
	// rx, ry	X 軸方向の半径と, Y 軸方向の半径.
	// start	四分だ円を囲む領域の始点.
	// b_vec	四分だ円を囲む領域の終点ベクトル.
	// 得られたベジェ曲線の開始点と制御点は, だ円の中心点を原点とする座標で得られる.
	// 3次ベジェ曲線を用いた楕円の近似
	// https://clown.cube-soft.jp/entry/20090606/p1
	static void qellipse_alternate(
		const double px, const double py, const double rx, const double ry, const double c, const double s,
		const double t_min, const double t_max, const bool dir, D2D1_POINT_2F& start, 
		D2D1_BEZIER_SEGMENT& b_seg)
	{
		constexpr double a = 4.0 * (M_SQRT2 - 1.0) / 3.0;
		double start_x = 0.0f;
		double start_y = 0.0f;
		double b_seg1x = 0.0f;
		double b_seg1y = 0.0f;
		double b_seg2x = 0.0f;
		double b_seg2y = 0.0f;
		double b_seg3x = 0.0f;
		double b_seg3y = 0.0f;

		// 終点ベクトルの傾きを戻す.
		//const double vx = c * px + s * py;
		//const double vy = -s * px + c * py;
		//const int qn = qellipse_quadrant_number(vx, vy);	// 象限の番号
		const int qn = qellipse_quadrant_number(px, py, c, s);	// 象限の番号
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
			start_y = 0.0f;
			b_seg1x = -rx;
			b_seg1y = -a * ry;
			b_seg2x = -a * rx;
			b_seg2y = -ry;
			b_seg3x = 0.0f;
			b_seg3y = -ry;
		}
		else {
			start_x = 0.0f;
			start_y = 0.0f;
			b_seg1x = 0.0f;
			b_seg1y = 0.0f;
			b_seg2x = 0.0f;
			b_seg2y = 0.0f;
			b_seg3x = 0.0f;
			b_seg3y = 0.0f;
			return;
		}
		const double x0 = c * start_x - s * start_y;
		const double y0 = s * start_x + c * start_y;
		const double x1 = c * b_seg1x - s * b_seg1y;
		const double y1 = s * b_seg1x + c * b_seg1y;
		const double x2 = c * b_seg2x - s * b_seg2y;
		const double y2 = s * b_seg2x + c * b_seg2y;
		const double x3 = c * b_seg3x - s * b_seg3y;
		const double y3 = s * b_seg3x + c * b_seg3y;

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
		start.x = (1.0 - t0) * (1.0 - t0) * (1.0 - t0) * x0 + 3.0 * t0 * (1.0 - t0) * (1.0 - t0) * x1 + 3.0 * t0 * t0 * (1.0 - t0) * x2 + t0 * t0 * t0 * x3;
		start.y = (1.0 - t0) * (1.0 - t0) * (1.0 - t0) * y0 + 3.0 * t0 * (1.0 - t0) * (1.0 - t0) * y1 + 3.0 * t0 * t0 * (1.0 - t0) * y2 + t0 * t0 * t0 * y3;
		b_seg.point1.x = start.x + (1.0 - t0) * (1.0 - t0) * (x1 - x0) + 2.0 * t0 * (1.0 - t0) * (x2 - x1) + t0 * t0 * (x3 - x2);
		b_seg.point1.y = start.y + (1.0 - t0) * (1.0 - t0) * (y1 - y0) + 2.0 * t0 * (1.0 - t0) * (y2 - y1) + t0 * t0 * (y3 - y2);
		b_seg.point3.x = (1.0 - t3) * (1.0 - t3) * (1.0 - t3) * x0 + 3.0 * t3 * (1.0 - t3) * (1.0 - t3) * x1 + 3.0 * t3 * t3 * (1.0 - t3) * x2 + t3 * t3 * t3 * x3;
		b_seg.point3.y = (1.0 - t3) * (1.0 - t3) * (1.0 - t3) * y0 + 3.0 * t3 * (1.0 - t3) * (1.0 - t3) * y1 + 3.0 * t3 * t3 * (1.0 - t3) * y2 + t3 * t3 * t3 * y3;
		b_seg.point2.x = b_seg.point3.x - ((1.0 - t3) * (1.0 - t3) * (x1 - x0) + 2.0 * t3 * (1.0 - t3) * (x2 - x1) + t3 * t3 * (x3 - x2));
		b_seg.point2.y = b_seg.point3.y - ((1.0 - t3) * (1.0 - t3) * (y1 - y0) + 2.0 * t3 * (1.0 - t3) * (y2 - y1) + t3 * t3 * (y3 - y2));
		*/
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
			start.x = static_cast<FLOAT>(new_x0);
			start.y = static_cast<FLOAT>(new_y0);
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
			start.x = static_cast<FLOAT>(new_x3);
			start.y = static_cast<FLOAT>(new_y3);
			b_seg.point1.x = static_cast<FLOAT>(new_x3 - (ss3 * x1_0 + 2.0 * t3 * s3 * x2_1 + tt3 * x3_2));
			b_seg.point1.y = static_cast<FLOAT>(new_y3 - (ss3 * y1_0 + 2.0 * t3 * s3 * y2_1 + tt3 * y3_2));
		}
	}

	// 円弧が含まれる象限を得る.
	// px, py	円弧の終点ベクトル
	// c, s	円弧の傾きのコサインとサイン (傾きは, 時計周りが正)
	// 戻り値	象限の番号 (1,2,3,4). 終点ベクトルがゼロベクトルなら 0.
	// Y 軸は下向きだが, 向かって右上が第 1 象限.
	static int qellipse_quadrant_number(const double px, const double py, const double c, const double s)
	{
		// 円弧の終点ベクトルを, 円弧の傾きが 0 になるよう回転して戻す.
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
	// start	円弧の開始点
	// vec	円弧の終点へのベクトル
	// rad	だ円の半径 (標準形における X 軸方向と Y 軸方向)
	// c	cos(だ円の傾き)
	// s	sin(だ円の傾き)
	// dir	だ円の方向 (時計回り=true)
	// val	得られた中心点.
	static bool qellipse_center(
		const D2D1_POINT_2F start, const D2D1_POINT_2F end, const D2D1_SIZE_F rad, const double C,
		const double S, D2D1_POINT_2F& val) noexcept
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
		const double A = 1.0 / (rad.width * rad.width);
		const double B = 1.0 / (rad.height * rad.height);
		//const double C = cos(rot);
		//const double S = sin(rot);
		const double d = -2 * A * C * (C * px + S * py) - 2 * B * S * (S * px - C * py);
		const double e = -2 * A * S * (C * px + S * py) + 2 * B * C * (S * px - C * py);
		const double f = A * (C * px + S * py) * (C * px + S * py) + B * (S * px - C * py) *
			(S * px - C * py);
		const double g = -2 * A * C * (C * qx + S * qy) - 2 * B * S * (S * qx - C * qy);
		const double h = -2 * A * S * (C * qx + S * qy) + 2 * B * C * (S * qx - C * qy);
		const double i = A * (C * qx + S * qy) * (C * qx + S * qy) + B * (S * qx - C * qy) *
			(S * qx - C * qy);
		const double j = (g - d) / (e - h);
		const double k = (i - f) / (e - h);
		const double a = A * (C + S * j) * (C + S * j) + B * (S - C * j) * (S - C * j);
		const double b = -2 * A * (C + S * j) * (C * px + S * py - S * k) - 2 * B * (S - C * j) *
			(S * px - C * py + C * k);
		const double c = A * (C * px + S * py - S * k) * (C * px + S * py - S * k) + B *
			(S * px - C * py + C * k) * (S * px - C * py + C * k) - 1;
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

	void ShapeQEllipse::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		if (anc == ANC_TYPE::ANC_P0) {
			if (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				val = m_start;
			}
			else {
				pt_add(m_start, m_pos[0], val);
			}
		}
		else if (anc == ANC_TYPE::ANC_P0 + 1) {
			if (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				pt_add(m_start, m_pos[0], val);
			}
			else {
				val = m_start;
			}
		}
		else if (anc == ANC_TYPE::ANC_A_CENTER) {
			const double r = M_PI * m_deg_rot / 180.0;
			D2D1_POINT_2F end{
				m_start.x + m_pos[0].x,
				m_start.y + m_pos[0].y
			};
			qellipse_center(m_start, end, m_radius, cos(r), sin(r), val);
		}
		else if (anc == ANC_TYPE::ANC_A_START) {
			constexpr double R[]{ 0.0, 90.0, 180.0, 270.0 };
			const D2D1_POINT_2F end{
				m_start.x + m_pos[0].x, m_start.y + m_pos[0].y
			};
			const double r = M_PI * m_deg_rot / 180.0;
			const double c = cos(r);
			const double s = sin(r);
			D2D1_POINT_2F center;
			if (qellipse_center(m_start, end, m_radius, c, s, center)) {
				const int qn = qellipse_quadrant_number(m_pos[0].x, m_pos[0].y, c, s);
				//if (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
					const double sr = M_PI * (R[(qn + 2) % 4] + m_deg_start) / 180.0;
					const double sc = cos(sr);
					const double ss = sin(sr);
					const double sx = c * m_radius.width * sc - s * m_radius.height * ss;
					const double sy = s * m_radius.width * sc + c * m_radius.height * ss;
					val.x = static_cast<FLOAT>(sx + center.x);
					val.y = static_cast<FLOAT>(sy + center.y);
				//}
				//else {
				//	const double er = M_PI * (R[qn - 1] + m_deg_end) / 180.0;
				//	const double ec = cos(er);
				//	const double es = sin(er);
				//	const double ex = c * m_radius.width * ec - s * m_radius.height * es;
				//	const double ey = s * m_radius.width * ec + c * m_radius.height * es;
				//	val.x = static_cast<FLOAT>(ex + center.x);
				//	val.y = static_cast<FLOAT>(ey + center.y);
				//}
			}
		}
		else if (anc == ANC_TYPE::ANC_A_END) {
			constexpr double R[]{ 0.0, 90.0, 180.0, 270.0 };
			const D2D1_POINT_2F end{
				m_start.x + m_pos[0].x, m_start.y + m_pos[0].y
			};
			const double r = M_PI * m_deg_rot / 180.0;
			const double c = cos(r);
			const double s = sin(r);
			D2D1_POINT_2F center;
			if (qellipse_center(m_start, end, m_radius, c, s, center)) {
				const int qn = qellipse_quadrant_number(m_pos[0].x, m_pos[0].y, c, s);
				//if (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
					const double er = M_PI * (R[qn - 1] + m_deg_end) / 180.0;
					const double ec = cos(er);
					const double es = sin(er);
					const double ex = c * m_radius.width * ec - s * m_radius.height * es;
					const double ey = s * m_radius.width * ec + c * m_radius.height * es;
					val.x = static_cast<FLOAT>(ex + center.x);
					val.y = static_cast<FLOAT>(ey + center.y);
				//}
				//else {
				//	const double sr = M_PI * (R[(qn + 2) % 4] + m_deg_start) / 180.0;
				//	const double sc = cos(sr);
				//	const double ss = sin(sr);
				//	const double sx = c * m_radius.width * sc - s * m_radius.height * ss;
				//	const double sy = s * m_radius.width * sc + c * m_radius.height * ss;
				//	val.x = static_cast<FLOAT>(sx + center.x);
				//	val.y = static_cast<FLOAT>(sy + center.y);
				//}
			}
		}
	}

	// 頂点を得る.
	size_t ShapeQEllipse::get_verts(D2D1_POINT_2F p[]) const noexcept
	{
		constexpr double R[]{ 0.0, 90.0, 180.0, 270.0 };

		p[AXIS1] = m_start;
		p[AXIS2].x = m_start.x + m_pos[0].x;
		p[AXIS2].y = m_start.y + m_pos[0].y;
		const double r = M_PI * m_deg_rot / 180.0;
		const double c = cos(r);
		const double s = sin(r);
		qellipse_center(p[AXIS1], p[AXIS2], m_radius, c, s, p[CENTER]);
		const int qn = qellipse_quadrant_number(m_pos[0].x, m_pos[0].y, c, s);
		const double er = M_PI * (R[qn - 1] + m_deg_end) / 180.0;
		const double ec = cos(er);
		const double es = sin(er);
		const double ex = c * m_radius.width * ec - s * m_radius.height * es;
		const double ey = s * m_radius.width * ec + c * m_radius.height * es;
		p[END].x = static_cast<FLOAT>(ex + p[CENTER].x);
		p[END].y = static_cast<FLOAT>(ey + p[CENTER].y);
		const double sr = M_PI * (R[(qn + 2) % 4] + m_deg_start) / 180.0;
		const double sc = cos(sr);
		const double ss = sin(sr);
		const double sx = c * m_radius.width * sc - s * m_radius.height * ss;
		const double sy = s * m_radius.width * sc + c * m_radius.height * ss;
		p[START].x = static_cast<FLOAT>(sx + p[CENTER].x);
		p[START].y = static_cast<FLOAT>(sy + p[CENTER].y);
		return 5;
	}

	// 値を傾き角度に格納する.
	bool ShapeQEllipse::set_arc_rot(const float val) noexcept
	{
		if (equal(m_deg_rot, val)) {
			return false;
		}
		// 終点ベクトルの傾きを戻す.
		const double old_r = M_PI * m_deg_rot / 180.0;
		const auto old_c = cos(old_r);
		const auto old_s = sin(old_r);
		const int qn = qellipse_quadrant_number(m_pos[0].x, m_pos[0].y, old_c, old_s);	// 象限番号
		D2D1_POINT_2F start, end;
		start = m_start;
		end.x = m_start.x + m_pos[0].x;
		end.y = m_start.y + m_pos[0].y;

		// だ円での中心点を得る.
		D2D1_POINT_2F center;
		qellipse_center(start, end, m_radius, old_c, old_s, /*d,*/ center);
		// 新しいだ円の軸を得る.
		const double new_r = M_PI * val / 180.0;
		const auto new_c = cos(-new_r);
		const auto new_s = sin(-new_r);
		double px, py, qx, qy;
		if (qn == 1) {
			px = 0.0;
			py = -m_radius.height;
			qx = m_radius.width;
			qy = 0.0;
			m_start.x = static_cast<FLOAT>(new_c * px + new_s * py + center.x);
			m_start.y = static_cast<FLOAT>(-new_s * px + new_c * py + center.y);
			m_pos.resize(1);
			m_pos[0].x = static_cast<FLOAT>(new_c * qx + new_s * qy + center.x - m_start.x);
			m_pos[0].y = static_cast<FLOAT>(-new_s * qx + new_c * qy + center.y - m_start.y);
		}
		else if (qn == 2) {
			px = m_radius.width;
			py = 0.0f;
			qx = 0.0f;
			qy = m_radius.height;
			m_start.x = static_cast<FLOAT>(new_c * px + new_s * py + center.x);
			m_start.y = static_cast<FLOAT>(-new_s * px + new_c * py + center.y);
			m_pos.resize(1);
			m_pos[0].x = static_cast<FLOAT>(new_c * qx + new_s * qy + center.x - m_start.x);
			m_pos[0].y = static_cast<FLOAT>(-new_s * qx + new_c * qy + center.y - m_start.y);
		}
		else if (qn == 3) {
			px = 0.0;
			py = m_radius.height;
			qx = -m_radius.width;
			qy = 0.0;
			m_start.x = static_cast<FLOAT>(new_c * px + new_s * py + center.x);
			m_start.y = static_cast<FLOAT>(-new_s * px + new_c * py + center.y);
			m_pos.resize(1);
			m_pos[0].x = static_cast<FLOAT>(new_c * qx + new_s * qy + center.x - m_start.x);
			m_pos[0].y = static_cast<FLOAT>(-new_s * qx + new_c * qy + center.y - m_start.y);
		}
		else if (qn == 4) {
			px = -m_radius.width;
			py = 0.0f;
			qx = 0.0f;
			qy = -m_radius.height;
			m_start.x = static_cast<FLOAT>(new_c * px + new_s * py + center.x);
			m_start.y = static_cast<FLOAT>(-new_s * px + new_c * py + center.y);
			m_pos.resize(1);
			m_pos[0].x = static_cast<FLOAT>(new_c * qx + new_s * qy + center.x - m_start.x);
			m_pos[0].y = static_cast<FLOAT>(-new_s * qx + new_c * qy + center.y - m_start.y);
		}
		else {
			m_start = center;
			m_pos[0] = center;
		}
		m_deg_rot = val;
		if (m_d2d_fill_geom != nullptr) {
			m_d2d_fill_geom = nullptr;
		}
		if (m_d2d_path_geom != nullptr) {
			m_d2d_path_geom = nullptr;
		}
		if (m_d2d_arrow_geom != nullptr) {
			m_d2d_arrow_geom = nullptr;
		}
		return true;
	}

	// 値を円弧の始点の角度に格納する.
	bool ShapeQEllipse::set_arc_start(const float val) noexcept
	{
		//if (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
			if (val >= 0.0 && val <= 45.0 && !equal(m_deg_start, val)) {
				m_deg_start = val;
				m_d2d_fill_geom = nullptr;
				m_d2d_path_geom = nullptr;
				m_d2d_arrow_geom = nullptr;
				return true;
			}
			/*
		}
		else {
			if (val >= 0.0 && val <= 45.0 && !equal(m_deg_end, -val)) {
				m_deg_end = -val;
				m_d2d_fill_geom = nullptr;
				m_d2d_path_geom = nullptr;
				m_d2d_arrow_geom = nullptr;
				return true;
			}
		}
		*/
		return false;
	}

	// 値を円弧の終点の角度に格納する.
	bool ShapeQEllipse::set_arc_end(const float val) noexcept
	{
		//if (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
			if (val >= -45.0 && val <= 0.0 && !equal(m_deg_end, val)) {
				m_deg_end = val;
				m_d2d_fill_geom = nullptr;
				m_d2d_path_geom = nullptr;
				m_d2d_arrow_geom = nullptr;
				return true;
			}
			/*
		}
		else {
			if (val >= -45.0 && val <= 0.0 && !equal(m_deg_start, -val)) {
				m_deg_start = -val;
				m_d2d_fill_geom = nullptr;
				m_d2d_path_geom = nullptr;
				m_d2d_arrow_geom = nullptr;
				return true;
			}
		}
		*/
		return false;
	}

	// 値を円弧を描く方向に格納する.
	bool ShapeQEllipse::set_arc_dir(const D2D1_SWEEP_DIRECTION val) noexcept
	{
		if (m_sweep_dir != val) {
			// 終点を始点に格納し, 位置ベクトルを逆転.
			m_start.x += m_pos[0].x;
			m_start.y += m_pos[0].y;
			m_pos[0].x = -m_pos[0].x;
			m_pos[0].y = -m_pos[0].y;
			m_sweep_dir = val;
			m_d2d_fill_geom = nullptr;
			m_d2d_path_geom = nullptr;
			m_d2d_arrow_geom = nullptr;
			return true;
		}
		return false;
	}

	// 値を, 部位の位置に格納する.
	bool ShapeQEllipse::set_pos_anc(
		const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect)
		noexcept
	{
		if (anc == ANC_TYPE::ANC_A_START) {
			const double rot = M_PI * m_deg_rot / 180.0;
			const double c = cos(rot);
			const double s = sin(rot);
			D2D1_POINT_2F center;
			D2D1_POINT_2F start = m_start;
			D2D1_POINT_2F end{
				m_start.x + m_pos[0].x, m_start.y + m_pos[0].y
			};
			if (qellipse_center(start, end, m_radius, c, s, center)) {
				const int qn = qellipse_quadrant_number(m_pos[0].x, m_pos[0].y, c, s);	// 象限番号
				if (qn == 1 || qn == 3) {
					const double ty = -s * (val.x - center.x) + c * (val.y - center.y);
					const double a = ty / -m_radius.height;
					if (abs(a) > FLT_MIN) {
						const double tx = c * (val.x - center.x) + s * (val.y - center.y);
						const double b = tx / m_radius.width;
						const double r = atan(b / a);
						const double d = 180.0 * r / M_PI;
						/*
						if (set_arc_start(static_cast<float>(d))) {
							return true;
						}
						*/
						if (d >= 0.0 && d <= 45.0) {
							m_deg_start = static_cast<float>(d);
							if (m_d2d_arrow_geom != nullptr) {
								m_d2d_arrow_geom = nullptr;
							}
							if (m_d2d_path_geom != nullptr) {
								m_d2d_path_geom = nullptr;
							}
							if (m_d2d_fill_geom != nullptr) {
								m_d2d_fill_geom = nullptr;
							}
							return true;
						}
					}
				}
				else {
					const double tx = c * (val.x - center.x) + s * (val.y - center.y);
					const double a = tx / m_radius.height;
					if (abs(a) > FLT_MIN) {
						const double ty = -s * (val.x - center.x) + c * (val.y - center.y);
						const double b = ty / m_radius.width;
						const double r = atan(b / a);
						const double d = 180.0 * r / M_PI;
						/*
						if (set_arc_start(static_cast<float>(d))) {
							return true;
						}
						*/
						if (d >= 0.0 && d <= 45.0) {
							m_deg_start = static_cast<float>(d);
							if (m_d2d_arrow_geom != nullptr) {
								m_d2d_arrow_geom = nullptr;
							}
							if (m_d2d_path_geom != nullptr) {
								m_d2d_path_geom = nullptr;
							}
							if (m_d2d_fill_geom != nullptr) {
								m_d2d_fill_geom = nullptr;
							}
							return true;
						}
					}
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_A_END) {
			const double rot = M_PI * m_deg_rot / 180.0;
			const double c = cos(rot);
			const double s = sin(rot);
			D2D1_POINT_2F start = m_start;
			D2D1_POINT_2F end{
				m_start.x + m_pos[0].x, m_start.y + m_pos[0].y
			};
			D2D1_POINT_2F center;
			if (qellipse_center(start, end, m_radius, c, s, /*d,*/ center)) {
				int qn = qellipse_quadrant_number(m_pos[0].x, m_pos[0].y, c, s);	// 象限番号
				if (qn == 1 || qn == 3) {
					const double tx = c * (val.x - center.x) + s * (val.y - center.y);
					const double a = tx / m_radius.width;
					if (abs(a) > FLT_MIN) {
						const double ty = -s * (val.x - center.x) + c * (val.y - center.y);
						const double b = ty / m_radius.height;
						const double r = atan(b / a);
						const double d = 180.0 * r / M_PI;
						//if (set_arc_end(static_cast<float>(d))) {
						//	return true;
						//}
						if (d >= -45.0 && d <= 0.0) {
							m_deg_end = static_cast<float>(d);
							if (m_d2d_arrow_geom != nullptr) {
								m_d2d_arrow_geom = nullptr;
							}
							if (m_d2d_path_geom != nullptr) {
								m_d2d_path_geom = nullptr;
							}
							if (m_d2d_fill_geom != nullptr) {
								m_d2d_fill_geom = nullptr;
							}
							return true;
						}
					}
				}
				else {
					const double ty = -s * (val.x - center.x) + c * (val.y - center.y);
					const double a = ty / -m_radius.height;
					if (abs(a) > FLT_MIN) {
						const double tx = c * (val.x - center.x) + s * (val.y - center.y);
						const double b = tx / m_radius.width;
						const double r = atan(b / a);
						const double d = 180.0 * r / M_PI;
						/*
						if (set_arc_end(static_cast<float>(d))) {
							return true;
						}
						*/
						if (d >= -45.0 && d <= 0.0) {
							m_deg_end = static_cast<float>(d);
							if (m_d2d_arrow_geom != nullptr) {
								m_d2d_arrow_geom = nullptr;
							}
							if (m_d2d_path_geom != nullptr) {
								m_d2d_path_geom = nullptr;
							}
							if (m_d2d_fill_geom != nullptr) {
								m_d2d_fill_geom = nullptr;
							}
							return true;
						}
					}
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_A_CENTER) {
			const double rot = M_PI * m_deg_rot / 180.0;
			const double c = cos(rot);
			const double s = sin(rot);
			D2D1_POINT_2F start = m_start;
			D2D1_POINT_2F end{
				m_start.x + m_pos[0].x, m_start.y + m_pos[0].y
			};
			D2D1_POINT_2F center;
			if (qellipse_center(start, end, m_radius, c, s, /*d,*/ center)) {
				const D2D1_POINT_2F start{
					m_start.x + val.x - center.x,
					m_start.y + val.y - center.y
				};
				if (ShapePath::set_pos_start(start)) {
					if (m_d2d_fill_geom != nullptr) {
						m_d2d_fill_geom = nullptr;
					}
					return true;
				}
			}
		}
		else {
			if (ShapePath::set_pos_anc(val, anc, limit, keep_aspect)) {
				const double rot = M_PI * m_deg_rot / 180.0;
				const double c = cos(rot);
				const double s = sin(rot);
				const double x = m_pos[0].x;
				const double y = m_pos[0].y;
				const double w = c * x + s * y;
				const double h = -s * x + c * y;
				m_radius.width = static_cast<FLOAT>(fabs(w));
				m_radius.height = static_cast<FLOAT>(fabs(h));
				if (m_d2d_fill_geom != nullptr) {
					m_d2d_fill_geom = nullptr;
				}
				return true;
			}
		}
		return false;
	}

	// 値を始点に格納する. 他の部位の位置も動く.
	bool ShapeQEllipse::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		if (ShapePath::set_pos_start(val)) {
			m_d2d_fill_geom = nullptr;
			return true;
		}
		return false;
	}

	uint32_t ShapeQEllipse::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		D2D1_POINT_2F p[5];
		get_verts(p);
		if (pt_in_anc(test, p[AXIS2], m_anc_width)) {
			return ANC_TYPE::ANC_P0 + 1;
		}
		else if (pt_in_anc(test, p[AXIS1], m_anc_width)) {
			return ANC_TYPE::ANC_P0;
		}
		else if (pt_in_anc(test, p[END], m_anc_width)) {
			//if (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				return ANC_TYPE::ANC_A_END;
			//}
			//else {
			//	return ANC_TYPE::ANC_A_START;
			//}
		}
		else if (pt_in_anc(test, p[START], m_anc_width)) {
			//if (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				return ANC_TYPE::ANC_A_START;
			//}
			//else {
			//	return ANC_TYPE::ANC_A_END;
			//}
		}
		// アンカーポイントに含まれるか判定する.
		/*
		if (pt_in_anc(test, m_start, m_anc_width)) {
			return ANC_TYPE::ANC_P0;
		}
		else if (pt_in_anc(
			test, D2D1_POINT_2F{ m_start.x + m_pos[0].x, m_start.y + m_pos[0].y }, m_anc_width)) {
			return ANC_TYPE::ANC_P0 + 1;
		}

		// だ円の中心点に含まれるか判定する.
		const double rot = M_PI * m_deg_rot / 180.0;
		const double c = cos(rot);
		const double s = sin(rot);
		//D2D1_POINT_2F start = m_start;
		D2D1_POINT_2F end{
			m_start.x + m_pos[0].x, m_start.y + m_pos[0].y
		};
		D2D1_POINT_2F center;
		if (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
			qellipse_center(m_start, end, m_radius, c, s, center);
		}
		else {
			qellipse_center(end, m_start, m_radius, c, s, center);
		}
		/*
		if (pt_in_anc(test, center, m_anc_width)) {
			return  ANC_TYPE::ANC_A_CENTER;
		}

		// 対角点の位置ベクトルを回転移動する.
		const double vx = c * m_pos[0].x + s * m_pos[0].y;
		const double vy = -s * m_pos[0].x + c * m_pos[0].y;
		constexpr double R[]{ 0.0, 90.0, 180.0, 270.0 };
		const int q = qellipse_quadrant_number(vx, vy);

		D2D1_POINT_2F end{};
		const double er = M_PI * (R[q - 1] + m_deg_end) / 180.0;
		const double ec = cos(er);
		const double es = sin(er);
		const double ex = c * m_radius.width * ec - s * m_radius.height * es;
		const double ey = s * m_radius.width * ec + c * m_radius.height * es;
		end.x = static_cast<FLOAT>(ex + center.x);
		end.y = static_cast<FLOAT>(ey + center.y);
		if (pt_in_anc(test, end, m_anc_width)) {
			return ANC_TYPE::ANC_A_END;
		}

		D2D1_POINT_2F start{};
		const double sr = M_PI * (R[(q + 2) % 4] + m_deg_start) / 180.0;
		const double sc = cos(sr);
		const double ss = sin(sr);
		const double sx = c * m_radius.width * sc - s * m_radius.height * ss;
		const double sy = s * m_radius.width * sc + c * m_radius.height * ss;
		start.x = static_cast<FLOAT>(sx + center.x);
		start.y = static_cast<FLOAT>(sy + center.y);
		if (pt_in_anc(test, start, m_anc_width)) {
			return ANC_TYPE::ANC_A_START;
		}
		*/
		// 位置 t が, 扇形の内側にあるか判定する.
		// 円弧の端点を s, e とする.
		// 時計周りの場合, s と t の外積が 0 以上で,
		// e と t の外積が 0 以下なら, 内側.
		const double rot = M_PI * m_deg_rot / 180.0;
		const double c = cos(rot);
		const double s = sin(rot);
		const double sx = p[START].x - p[CENTER].x;
		const double sy = p[START].y - p[CENTER].y;
		const double ex = p[END].x - p[CENTER].x;
		const double ey = p[END].y - p[CENTER].y;
		const double tx = test.x - p[CENTER].x;
		const double ty = test.y - p[CENTER].y;
		const double st = sx * ty - sy * tx;
		const double et = ex * ty - ey * tx;
		const double rx = abs(m_radius.width);
		const double ry = abs(m_radius.height);
		if (st >= 0.0 && et <= 0.0) {
			// 線枠が可視で,
			if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
				// 判定する位置が, 内側と外側のだ円の中にあるなら,
				const double e_width = max(m_stroke_width, m_anc_width) * 0.5;
				if (!pt_in_ellipse(test, p[CENTER], rx - e_width, ry - e_width, rot)) {
					if (pt_in_ellipse(test, p[CENTER], rx + e_width, ry + e_width, rot)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				// 判定する位置が, 内側のだ円の中にあるなら
				else if (is_opaque(m_fill_color)) {
					return ANC_TYPE::ANC_FILL;
				}
			}
			// 塗りつぶし色が不透明で,
			else if (is_opaque(m_fill_color)) {
				// 判定する位置が, だ円の内にあるなら,
				if (pt_in_ellipse(test, p[CENTER], rx, ry, rot)) {
					return ANC_TYPE::ANC_FILL;
				}
			}
		}
		// 判定する位置が, 扇形の外側にあり, かつ線枠が可視なら,
		else if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			// 端点に含まれるか判定する.
			// 時計回りに対してのみ判定しているので要注意.
			auto c_style = m_stroke_cap.m_start;
			const double e_width = m_stroke_width * 0.5;	// 辺の半分の幅.
			if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
				// 判定する位置を, 端点を原点とする座標に平行移動する.
				const D2D1_POINT_2F t{
					static_cast<FLOAT>(test.x - sx), static_cast<FLOAT>(test.y - sy)
				};
				if (pt_in_circle(t, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			else {
				const D2D1_POINT_2F t{
					static_cast<FLOAT>(tx), static_cast<FLOAT>(ty)
				};
				// だ円 A・x^2 + B・y^2 = 1 における点 p { x0, y0 } の接線は
				// (A・x0)・x + (B・y0)・y = 1
				const double x0 = sx;
				const double y0 = sy;
				// p を回転移動し, だ円の標準形での接線の係数を得る.
				const double Ax0 = (c * x0 + s * y0) / (rx * rx);
				const double By0 = (-s * x0 + c * y0) / (ry * ry);
				// 得られた接線を逆に回転移動し, 傾いただ円における接線を得る.
				const double a = c * Ax0 - s * By0;
				const double b = s * Ax0 + c * By0;
				// 得られた接線から, 接線に垂直なベクトル o (長さは辺の半分の幅) を得る.
				const double ab = sqrt(a * a + b * b);
				const double ox = e_width * a / ab;
				const double oy = e_width * b / ab;
				if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT) {
					const D2D1_POINT_2F quad[4]{
						{ static_cast<FLOAT>(x0 + ox), static_cast<FLOAT>(y0 + oy) },
						{ static_cast<FLOAT>(x0 - ox), static_cast<FLOAT>(y0 - oy) },
						{ static_cast<FLOAT>(x0 - ox - oy), static_cast<FLOAT>(y0 + ox - oy) },
						{ static_cast<FLOAT>(x0 + ox - oy), static_cast<FLOAT>(y0 + ox + oy) }
					};
					if (pt_in_poly(t, 4, quad)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				else if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
					const D2D1_POINT_2F quad[4]{
						{ static_cast<FLOAT>(x0 + ox + oy), static_cast<FLOAT>(y0 - ox + oy) },
						{ static_cast<FLOAT>(x0 - ox + oy), static_cast<FLOAT>(y0 - ox - oy) },
						{ static_cast<FLOAT>(x0 - ox), static_cast<FLOAT>(y0 - oy) },
						{ static_cast<FLOAT>(x0 + ox), static_cast<FLOAT>(y0 + oy) },
					};
					if (pt_in_poly(t, 4, quad)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				else if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
					const D2D1_POINT_2F tri[3/*4*/]{
						{ static_cast<FLOAT>(x0 + ox), static_cast<FLOAT>(y0 + oy) },
						{ static_cast<FLOAT>(x0 + oy), static_cast<FLOAT>(y0 - ox) },
						{ static_cast<FLOAT>(x0 - ox), static_cast<FLOAT>(y0 - oy) },
						//{ static_cast<FLOAT>(x0 - ey), static_cast<FLOAT>(y0 + ex) }
					};
					if (pt_in_poly(t, 3/*4*/, tri)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
			}
			if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
				// 判定する位置を, 端点を原点とする座標に平行移動する.
				const D2D1_POINT_2F t{
					static_cast<FLOAT>(test.x - ex), static_cast<FLOAT>(test.y - ey)
				};
				if (pt_in_circle(t, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			else {
				const D2D1_POINT_2F t{
					static_cast<FLOAT>(tx), static_cast<FLOAT>(ty)
				};
				// だ円 A・x^2 + B・y^2 = 1 における点 { x0, y0 } の接線は
				// (A・x0)・x + (B・y0)・y = 1
				const double x0 = ex;
				const double y0 = ey;
				// 点 { x0, y0 } を回転移動し, だ円の標準形での接線の係数を得る.
				const double Ax0 = (c * x0 + s * y0) / (rx * rx);
				const double By0 = (-s * x0 + c * y0) / (ry * ry);
				// 得られた接線を逆に回転移動し, 傾いただ円における接線を得る.
				const double a = c * Ax0 - s * By0;
				const double b = s * Ax0 + c * By0;
				// 得られた接線から, 接線に垂直なベクトル o (長さは辺の半分の幅) を得る.
				// e は, だ円の外側向き.
				const double ab = sqrt(a * a + b * b);
				const double ox = e_width * a / ab;
				const double oy = e_width * b / ab;
				if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT) {
					const D2D1_POINT_2F quad[4]{
						{ static_cast<FLOAT>(x0 + ox + oy), static_cast<FLOAT>(y0 - ox + oy) },
						{ static_cast<FLOAT>(x0 - ox + oy), static_cast<FLOAT>(y0 - ox - oy) },
						{ static_cast<FLOAT>(x0 - ox), static_cast<FLOAT>(y0 - oy) },
						{ static_cast<FLOAT>(x0 + ox), static_cast<FLOAT>(y0 + oy) },
					};
					if (pt_in_poly(t, 4, quad)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				else if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
					const D2D1_POINT_2F quad[4]{
						{ static_cast<FLOAT>(x0 + ox), static_cast<FLOAT>(y0 + oy) },
						{ static_cast<FLOAT>(x0 - ox), static_cast<FLOAT>(y0 - oy) },
						{ static_cast<FLOAT>(x0 - ex - oy), static_cast<FLOAT>(y0 + ox - oy) },
						{ static_cast<FLOAT>(x0 + ex - oy), static_cast<FLOAT>(y0 + ox + oy) }
					};
					if (pt_in_poly(t, 4, quad)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				else if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
					const D2D1_POINT_2F tri[3/*4*/]{
						{ static_cast<FLOAT>(x0 + ox), static_cast<FLOAT>(y0 + oy) },
						{ static_cast<FLOAT>(x0 - ox), static_cast<FLOAT>(y0 - oy) },
						{ static_cast<FLOAT>(x0 - oy), static_cast<FLOAT>(y0 + ox) }
					};
					if (pt_in_poly(t, 3/*4*/, tri)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// 円弧をベジェ曲線で近似する.
	// ただし, 始点の終点の角度が 180 度に近くなるとズレる.
	void ShapeQEllipse::alternate_bezier(D2D1_POINT_2F& start, D2D1_BEZIER_SEGMENT& b_seg) const noexcept
	{
		D2D1_POINT_2F center;
		const double rot = M_PI * m_deg_rot / 180.0;
		const double c = cos(rot);
		const double s = sin(rot);
		D2D1_POINT_2F end{
			m_start.x + m_pos[0].x,
			m_start.y + m_pos[0].y
		};
		qellipse_center(m_start, end, m_radius, c, s, /*d,*/ center);
		const double t_min = 0.0 + m_deg_start / 90.0;
		const double t_max = 1.0 + m_deg_end / 90.0;
		const double d = (m_sweep_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE);
		D2D1_POINT_2F pos = m_pos[0];
		qellipse_alternate(pos.x, pos.y, m_radius.width, m_radius.height, c, s, t_min, t_max, d,
			start, b_seg);
		start.x += center.x;
		start.y += center.y;
		b_seg.point1.x += center.x;
		b_seg.point1.y += center.y;
		b_seg.point2.x += center.x;
		b_seg.point2.y += center.y;
		b_seg.point3.x += center.x;
		b_seg.point3.y += center.y;
	}

	// 矢じりの返しと先端の位置を得る
	// pos	始点からの終点ベクトル.
	// center	四分だ円の中心点
	// rad	X 軸方向の半径と, Y 軸方向の半径.
	// rot	だ円の傾き (時計回りのラジアン)
	// a_size	矢じりの大きさ
	// arrow	矢じりの返しと先端の位置
	bool ShapeQEllipse::qellipse_calc_arrow(
		const D2D1_POINT_2F pos, const D2D1_POINT_2F center, const D2D1_SIZE_F rad,
		const double deg_start, const double deg_end, const double deg_rot,
		const D2D1_SWEEP_DIRECTION dir, const ARROW_SIZE a_size, D2D1_POINT_2F arrow[])
	{
		D2D1_POINT_2F start{};	// ベジェ曲線の始点
		D2D1_BEZIER_SEGMENT b_seg{};	// ベジェ曲線の制御点
		const double rot = M_PI * deg_rot / 180.0;
		const double t_min = 0.0 + deg_start / 90.0;
		const double t_max = 1.0 + deg_end / 90.0;
		const double d = (dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE);
		qellipse_alternate(pos.x, pos.y, rad.width, rad.height, cos(rot), sin(rot), t_min, t_max,
			d, start, b_seg);
		if (ShapeBezier::bezi_calc_arrow(start, b_seg, a_size, arrow)) {
			// 得られた各位置は, だ円中心点を原点とする座標なので, もとの座標へ戻す.
			arrow[0].x += center.x;
			arrow[0].y += center.y;
			arrow[1].x += center.x;
			arrow[1].y += center.y;
			arrow[2].x += center.x;
			arrow[2].y += center.y;
			return true;
		}
		return false;
	}

	void ShapeQEllipse::draw(void)
	{
		ID2D1RenderTarget* target = Shape::m_d2d_target;
		ID2D1Factory* factory;
		target->GetFactory(&factory);
		ID2D1SolidColorBrush* brush = Shape::m_d2d_color_brush.get();

		D2D1_POINT_2F p[5]{};
		if ((!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) ||
			(is_opaque(m_fill_color) && m_d2d_fill_geom == nullptr || is_selected())) {
			get_verts(p);
		}
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			if (m_d2d_stroke_style == nullptr) {
				create_stroke_style(factory);
			}
			if (m_d2d_path_geom == nullptr) {
				D2D1_ARC_SEGMENT arc{
					p[END],
					D2D1_SIZE_F{ fabsf(m_radius.width), fabsf(m_radius.height) },
					m_deg_rot,
					//m_sweep_dir,
					D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE,
					m_larg_flag
				};
				winrt::com_ptr<ID2D1GeometrySink> sink;
				winrt::check_hresult(
					factory->CreatePathGeometry(m_d2d_path_geom.put())
				);
				winrt::check_hresult(
					m_d2d_path_geom->Open(sink.put())
				);
				sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
				const auto f_begin = (is_opaque(m_fill_color) ?
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
				sink->BeginFigure(p[START], f_begin);
				sink->AddArc(arc);
				sink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
				winrt::check_hresult(
					sink->Close()
				);
				sink = nullptr;
			}
			if (m_arrow_style != ARROW_STYLE::NONE) {
				if (m_d2d_arrow_geom == nullptr) {
					// だ円の弧長を求めるのはしんどいので, ベジェで近似
					D2D1_POINT_2F arrow[3];
					qellipse_calc_arrow(
						m_pos[0], p[CENTER], m_radius, m_deg_start, m_deg_end, m_deg_rot, m_sweep_dir,
						m_arrow_size, arrow);
					winrt::com_ptr<ID2D1GeometrySink> sink;
					const ARROW_STYLE a_style{ m_arrow_style };
					// ジオメトリパスを作成する.
					winrt::check_hresult(
						factory->CreatePathGeometry(m_d2d_arrow_geom.put())
					);
					winrt::check_hresult(
						m_d2d_arrow_geom->Open(sink.put())
					);
					sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
					const auto f_begin = (a_style == ARROW_STYLE::FILLED
						? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
						: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
					const auto f_end = (a_style == ARROW_STYLE::FILLED
						? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED
						: D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
					sink->BeginFigure(arrow[0], f_begin);
					sink->AddLine(arrow[2]);
					sink->AddLine(arrow[1]);
					sink->EndFigure(f_end);
					winrt::check_hresult(
						sink->Close()
					);
					sink = nullptr;
				}
				if (m_d2d_arrow_style == nullptr) {
					const CAP_STYLE c_style{ m_stroke_cap };
					const D2D1_LINE_JOIN j_style{ m_join_style };
					const double j_miter_limit = m_join_miter_limit;
					// 矢じるしの破線の形式はかならず実線.
					const D2D1_STROKE_STYLE_PROPERTIES s_prop{
						c_style.m_start,	// startCap
						c_style.m_end,	// endCap
						D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// dashCap
						j_style,	// lineJoin
						static_cast<FLOAT>(j_miter_limit),	// miterLimit
						D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID,	// dashStyle
						0.0f	// dashOffset
					};
					winrt::check_hresult(
						factory->CreateStrokeStyle(s_prop, nullptr, 0, m_d2d_arrow_style.put())
					);
				}
			}
		}
		if (is_opaque(m_fill_color) && m_d2d_fill_geom == nullptr) {
			D2D1_ARC_SEGMENT arc{
				p[END],
				D2D1_SIZE_F{ fabsf(m_radius.width), fabsf(m_radius.height) },
				m_deg_rot,
				//m_sweep_dir,
				D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE,
				m_larg_flag
			};
			winrt::com_ptr<ID2D1GeometrySink> sink;
			winrt::check_hresult(factory->CreatePathGeometry(m_d2d_fill_geom.put()));
			winrt::check_hresult(m_d2d_fill_geom->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			const auto f_begin = (is_opaque(m_fill_color) ?
				D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
				D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
			sink->BeginFigure(p[START], f_begin);
			sink->AddArc(arc);
			sink->AddLine(p[CENTER]);
			sink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED);
			winrt::check_hresult(sink->Close());
			sink = nullptr;
		}
		if (m_d2d_fill_geom != nullptr) {
			brush->SetColor(m_fill_color);
			target->FillGeometry(m_d2d_fill_geom.get(), brush);
		}
		if (m_d2d_path_geom != nullptr) {
			brush->SetColor(m_stroke_color);
			target->DrawGeometry(
				m_d2d_path_geom.get(), brush, m_stroke_width, m_d2d_stroke_style.get());
			if (m_d2d_arrow_geom != nullptr) {
				if (m_arrow_style == ARROW_STYLE::FILLED) {
					target->FillGeometry(m_d2d_arrow_geom.get(), brush);
					target->DrawGeometry(m_d2d_arrow_geom.get(), brush, m_stroke_width, 
						m_d2d_arrow_style.get());
				}
				if (m_arrow_style == ARROW_STYLE::OPENED) {
					target->DrawGeometry(m_d2d_arrow_geom.get(), brush, m_stroke_width, 
						m_d2d_arrow_style.get());
				}
			}
		}
		if (m_anc_show && is_selected()) {
			//const D2D1_POINT_2F p{ m_start };
			//const D2D1_POINT_2F q{ m_start.x + m_pos[0].x, m_start.y + m_pos[0].y };
			brush->SetColor(COLOR_WHITE);
			target->DrawLine(p[CENTER], p[AXIS1], brush, m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawLine(p[CENTER], p[AXIS1], brush, m_aux_width, m_aux_style.get());
			brush->SetColor(COLOR_WHITE);
			target->DrawLine(p[CENTER], p[AXIS2], brush, m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawLine(p[CENTER], p[AXIS2], brush, m_aux_width, m_aux_style.get());
			anc_draw_circle(p[START], target, brush);
			anc_draw_circle(p[END], target, brush);
			anc_draw_square(p[AXIS1], target, brush);
			anc_draw_square(p[AXIS2], target, brush);
		}
	}

	ShapeQEllipse::ShapeQEllipse(
		const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page) : 
		ShapePath(page, false),
		m_radius(D2D1_SIZE_F{ fabs(pos.x), fabs(pos.y) }),
		m_deg_rot(0.0f),
		m_deg_start(0.0f),
		m_deg_end(0.0f),
		m_sweep_dir(D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE),
		m_larg_flag(D2D1_ARC_SIZE_SMALL)
	{
		m_start = start;	// 始点
		m_pos.push_back(pos);	// 終点
		if (typeid(*page) == typeid(ShapeQEllipse)) {
			static_cast<const ShapeQEllipse*>(page)->get_arc_start(m_deg_start);
			static_cast<const ShapeQEllipse*>(page)->get_arc_end(m_deg_end);
			static_cast<const ShapeQEllipse*>(page)->get_arc_rot(m_deg_rot);
		}
	}

	ShapeQEllipse::ShapeQEllipse(const Shape& page, const DataReader& dt_reader) :
		ShapePath(page, dt_reader),
		m_radius({ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_deg_rot(dt_reader.ReadSingle()),
		m_deg_start(dt_reader.ReadSingle()),
		m_deg_end(dt_reader.ReadSingle()),
		m_sweep_dir(static_cast<D2D1_SWEEP_DIRECTION>(dt_reader.ReadUInt32())),
		m_larg_flag(static_cast<D2D1_ARC_SIZE>(dt_reader.ReadUInt32()))
	{}

	void ShapeQEllipse::write(const DataWriter& dt_writer) const
	{
		ShapePath::write(dt_writer);
		dt_writer.WriteSingle(m_radius.width);
		dt_writer.WriteSingle(m_radius.height);
		dt_writer.WriteSingle(m_deg_rot);
		dt_writer.WriteSingle(m_deg_start);
		dt_writer.WriteSingle(m_deg_end);
		dt_writer.WriteUInt32(m_sweep_dir);
		dt_writer.WriteUInt32(m_larg_flag);
	}
}