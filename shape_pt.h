#pragma once
#include <d2d1.h>

namespace winrt::GraphPaper::implementation
{
	// ベクトルの長さ (の自乗値) を得る
	// a	ベクトル
	// 戻り値	長さ (の自乗値) 
	inline double pt_abs2(const D2D1_POINT_2F a) noexcept
	{
		const double ax = a.x;
		const double ay = a.y;
		return ax * ax + ay * ay;
	}

	// 点に点を足す
	inline void pt_add(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x + b.x;
		c.y = a.y + b.y;
	}

	// 点にスカラー値を足す
	inline void pt_add(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(a.x + b);
		c.y = static_cast<FLOAT>(a.y + b);
	}

	// 点に X と Y の値を足す
	inline void pt_add(const D2D1_POINT_2F a, const double x, const double y, D2D1_POINT_2F& b)
		noexcept
	{
		b.x = static_cast<FLOAT>(a.x + x);
		b.y = static_cast<FLOAT>(a.y + y);
	}

	// 二点の中点を求める.
	inline void pt_avg(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>((a.x + b.x) * 0.5);
		c.y = static_cast<FLOAT>((a.y + b.y) * 0.5);
	}

	// 円が点を含むか判定する.
	// ctr	円の中心点
	// rad	円の半径
	inline bool pt_in_circle(
		const D2D1_POINT_2F t, const D2D1_POINT_2F ctr, const double rad) noexcept
	{
		const double tx = static_cast<double>(t.x) - static_cast<double>(ctr.x);
		const double ty = static_cast<double>(t.y) - static_cast<double>(ctr.y);
		return tx * tx + ty * ty <= rad * rad;
	}

	// 円が点を含むか判定する.
	// tx, ty	判定される位置 (円の中心点を原点とする)
	// rad	円の半径
	inline bool pt_in_circle(const double tx, const double ty, const double rad) noexcept
	{
		return tx * tx + ty * ty <= rad * rad;
	}

	// だ円が点を含むか判定する.
	// t	判定される点
	// ctr	だ円の中心
	// rad_x	だ円の X 軸方向の径
	// rad_y	だ円の Y 軸方向の径
	// rot	だ円の傾き (ラジアン)
	// 戻り値	含む場合 true
	inline bool pt_in_ellipse(
		const D2D1_POINT_2F t, const D2D1_POINT_2F ctr, const double rad_x,
		const double rad_y, const double rot = 0.0) noexcept
	{
		// だ円の傾きに合わせて判定される点を回転.
		const double tx = static_cast<double>(t.x) - static_cast<double>(ctr.x);
		const double ty = static_cast<double>(t.y) - static_cast<double>(ctr.y);
		const double c = cos(rot);
		const double s = sin(rot);
		const double x = c * tx + s * ty;
		const double y = -s * tx + c * ty;
		const double aa = rad_x * rad_x;
		const double bb = rad_y * rad_y;
		return x * x / aa + y * y / bb <= 1.0;
	}

	// だ円が点を含むか判定する.
	// tx, ty	判定される点 (だ円の中心点が原点)
	// rad_x	だ円の X 軸方向の径
	// rad_y	だ円の Y 軸方向の径
	// rot	だ円の傾き (ラジアン)
	// 戻り値	含む場合 true
	inline bool pt_in_ellipse(
		const double tx, const double ty, const double rad_x, const double rad_y, const double rot)
		noexcept
	{
		// だ円の傾きに合わせて判定される点を回転.
		const double c = cos(rot);
		const double s = sin(rot);
		const double x = c * tx + s * ty;
		const double y = -s * tx + c * ty;
		const double aa = rad_x * rad_x;
		const double bb = rad_y * rad_y;
		return x * x / aa + y * y / bb <= 1.0;
	}

	// 多角形が点を含むか判定する.
	// tx, ty	判定される位置
	// p_cnt	頂点の数
	// p	頂点の配列 [v_cnt]
	// 戻り値	含む場合 true
	// 多角形の各辺と, 指定された点を開始点とする水平線が交差する数を求める.
	inline bool pt_in_poly(
		const double tx, const double ty, const size_t p_cnt, const D2D1_POINT_2F p[]) noexcept
	{
		int i_cnt;	// 交点の数

		double px = p[p_cnt - 1].x;
		double py = p[p_cnt - 1].y;
		i_cnt = 0;
		for (size_t i = 0; i < p_cnt; i++) {
			const double qx = p[i].x;
			const double qy = p[i].y;
			// ルール 1. 上向きの辺. 点が y 軸方向について、始点と終点の間にある (ただし、終点は含まない).
			// ルール 2. 下向きの辺. 点が y 軸方向について、始点と終点の間にある (ただし、始点は含まない).
			if ((py <= ty && qy > ty) || (py > ty && qy <= ty)) {
				// ルール 3. 点を通る水平線が辺と重なる (ルール 1, ルール 2 を確認することで, ルール 3 も確認できている).
				// ルール 4. 辺は点よりも右側にある. ただし, 重ならない.
				// 辺が点と同じ高さになる位置を特定し, その時のxの値と点のxの値を比較する.
				if (tx < px + (ty - py) / (qy - py) * (qx - px)) {
					i_cnt++;
				}
			}
			px = qx;
			py = qy;
		}
		return static_cast<bool>(i_cnt & 1);
	}

	// 多角形が点を含むか判定する.
	inline bool pt_in_poly(
		const D2D1_POINT_2F t, const size_t p_cnt, const D2D1_POINT_2F p[]) noexcept
	{
		return pt_in_poly(t.x, t.y, p_cnt, p);
	}

	// 方形が点を含むか判定する.
	// t	判定される位置
	// r_lt	方形の左上位置
	// r_rb	方形の右下位置
	// 戻り値	含む場合 true
	inline bool pt_in_rect2(
		const D2D1_POINT_2F t, const D2D1_POINT_2F r_lt, const D2D1_POINT_2F r_rb) noexcept
	{
		return r_lt.x <= t.x && t.x <= r_rb.x && r_lt.y <= t.y && t.y <= r_rb.y;
	}

	// 方形が点を含むか判定する.
	// t	判定される点
	// r_lt	方形のいずれかの頂点
	// r_rb	r_lt に対して対角にある頂点
	// 戻り値	含む場合 true
	inline bool pt_in_rect(
		const D2D1_POINT_2F t, const D2D1_POINT_2F r_lt, const D2D1_POINT_2F r_rb) noexcept
	{
		const double lt_x = r_lt.x < r_rb.x ? r_lt.x : r_rb.x;	// 左上の x
		const double lt_y = r_lt.y < r_rb.y ? r_lt.y : r_rb.y;	// 左上の y
		const double rb_x = r_lt.x < r_rb.x ? r_rb.x : r_lt.x;	// 右下の x
		const double rb_y = r_lt.y < r_rb.y ? r_rb.y : r_lt.y;	// 右下の y
		return lt_x <= t.x && t.x <= rb_x && lt_y <= t.y && t.y <= rb_y;
	}

	// 点にスカラーを掛けて, 別の点を足す
	// a	点
	// b	スカラー値
	// c	別の点
	// d	結果
	inline void pt_mul_add(
		const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.x * b + c.x);
		d.y = static_cast<FLOAT>(a.y * b + c.y);
	}

	// 点にスカラーを掛ける.
	// a	位置
	// b	スカラー値
	// c	結果
	inline void pt_mul(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(a.x * b);
		c.y = static_cast<FLOAT>(a.y * b);
	}

	// 寸法にスカラー値を掛ける.
	// a	寸法
	// b	スカラー値
	// c	結果
	inline void pt_mul(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& c) noexcept
	{
		c.width = static_cast<FLOAT>(a.width * b);
		c.height = static_cast<FLOAT>(a.height * b);
	}

	// 点をスカラー倍に丸める.
	// a	位置
	// b	スカラー値
	// c	結果
	inline void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(std::round(a.x / b) * b);
		c.y = static_cast<FLOAT>(std::round(a.y / b) * b);
	}

	// 位置から位置を引く.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.x;
		c.y = a.y - b.y;
	}

	// 位置から大きさを引く.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.width;
		c.y = a.y - b.height;
	}

}