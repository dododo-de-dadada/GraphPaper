//
// ベジェ曲線の補助関数
//
#pragma once
#include <stdint.h>
#include <cmath>
#include <cfloat>
#include <d2d1.h>

namespace winrt::GraphPaper::implementation
{
	constexpr uint32_t SIMPSON_CNT = 30;	// シンプソン法の回数

	//------------------------------
// double 型の値をもつ位置
// ShapeBase, ShapeArc で使用する.
//------------------------------
	struct POINT_2D {
		double x;
		double y;
		// この位置を含むよう方形を拡張する.
		inline void exp(POINT_2D& r_lt, POINT_2D& r_rb) const noexcept
		{
			if (x < r_lt.x) {
				r_lt.x = x;
			}
			if (x > r_rb.x) {
				r_rb.x = x;
			}
			if (y < r_lt.y) {
				r_lt.y = y;
			}
			if (y > r_rb.y) {
				r_rb.y = y;
			}
		}
		inline POINT_2D nextafter(const double d) const noexcept
		{
			return POINT_2D{ std::nextafter(x, x + d), std::nextafter(y, y + d) };
		}
		inline operator D2D1_POINT_2F(void) const noexcept
		{
			return D2D1_POINT_2F{ static_cast<FLOAT>(x), static_cast<FLOAT>(y) };
		}
		inline POINT_2D operator -(const POINT_2D& q) const noexcept
		{
			return POINT_2D{ x - q.x, y - q.y };
		}
		inline POINT_2D operator -(const D2D1_POINT_2F q) const noexcept
		{
			return POINT_2D{ x - q.x, y - q.y };
		}
		inline POINT_2D operator -(void) const noexcept
		{
			return POINT_2D{ -x, -y };
		}
		inline POINT_2D operator *(const double s) const noexcept
		{
			return POINT_2D{ x * s, y * s };
		}
		inline double operator *(const POINT_2D& q) const noexcept
		{
			return x * q.x + y * q.y;
		}
		inline POINT_2D operator +(const POINT_2D& q) const noexcept
		{
			return POINT_2D{ x + q.x, y + q.y };
		}
		inline POINT_2D operator +(const D2D1_POINT_2F p) const noexcept
		{
			return POINT_2D{ x + p.x, y + p.y };
		}
		inline bool operator <(const POINT_2D& q) const noexcept
		{
			return x < q.x && y < q.y;
		}
		inline POINT_2D operator =(const D2D1_POINT_2F p) noexcept
		{
			return POINT_2D{ x = p.x, y = p.y };
		}
		inline POINT_2D operator =(const double s) noexcept
		{
			return POINT_2D{ x = s, y = s };
		}
		inline bool operator >(const POINT_2D& q) const noexcept
		{
			return x > q.x && y > q.y;
		}
		inline bool operator ==(const POINT_2D& q) const noexcept
		{
			return x == q.x && y == q.y;
		}
		inline bool operator !=(const POINT_2D& q) const noexcept
		{
			return x != q.x || y != q.y;
		}
		inline double opro(const POINT_2D& q) const noexcept
		{
			return x * q.y - y * q.x;
		}
	};

	//------------------------------
	// 曲線上の助変数をもとに接線ベクトルを求める.
	// c	制御点
	// t	助変数
	// v	t における接線ベクトル
	//------------------------------
	static inline void bezi_tvec_by_param(const POINT_2D c[4], const double t, POINT_2D& v) noexcept
	{
		const double a = -3.0 * (1.0 - t) * (1.0 - t);
		const double b = 3.0 * (1.0 - t) * (1.0 - 3.0 * t);
		const double d = 3.0 * t * (2.0 - 3.0 * t);
		const double e = (3.0 * t * t);
		v = c[0] * a + c[1] * b + c[2] * d + c[3] * e;
	}

	//------------------------------
	// 曲線上の助変数をもとに微分値を求める.
	// c	制御点 (コントロールポイント)
	// t	助変数
	// 戻り値	求まった微分値
	//------------------------------
	static inline double bezi_deriv_by_param(const POINT_2D c[4], const double t) noexcept
	{
		// 助変数をもとにベジェ曲線上の接線ベクトルを求め, その接線ベクトルの長さを返す.
		POINT_2D v;	// t における接線ベクトル
		bezi_tvec_by_param(c, t, v);
		return sqrt(v * v);
	}

	// 2 つの助変数が区間 0-1 の間で正順か判定する.
	// t_min	小さい方の助変数
	// t_max	大きい方の助変数
	static inline bool bezi_test_param(const double t_min, const double t_max) noexcept
	{
		// 範囲の上限 t_max は 1+DBL_EPSILON より小 ?
		// t_min より大きくて最も近い値は t_max より小 ?
		return -DBL_MIN < t_min && t_max < 1.0 + DBL_EPSILON &&
			std::nextafter(t_min, t_min + 1.0) < t_max;
	}

	//------------------------------
	// 曲線上の助変数の区間をもとに長さを求める.
	// シンプソン法を用いる.
	// c	制御点
	// t_min	区間の始端
	// t_max	区間の終端
	// s_cnt	シンプソン法の回数
	// 戻り値	求まった長さ
	//------------------------------
	static double bezi_len_by_param(
		const POINT_2D c[4], const double t_min, const double t_max, const uint32_t s_cnt) noexcept
	{
		double t_len;
		uint32_t n;
		double h;
		double a, b;
		double t;
		double b0, b2;
		double s;

		/* 範囲の上限下限は正順か判定する. */
		/* 正順 ? */
		if (bezi_test_param(t_min, t_max)) {
			/* 範囲上限 t_max -範囲下限 t_min を差分 t_len に格納する. */
			t_len = t_max - t_min;
			/* 区間の分割数 s_cnt と t_len を乗算する. */
			/* その結果を切り上げて整数値する. */
			/* 整数値を区間の半数 n に格納する. */
			n = (int)std::ceil(t_len * (double)s_cnt);
			/* t_len / 2n を階差 h に格納する. */
			h = t_len / (2.0 * n);
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
				a += bezi_deriv_by_param(c, t);
				/* 階差 h を助変数 t に加える. */
				t += h;
				/* 2i 番目の部分区間の微分値を求め, b に加える. */
				b += bezi_deriv_by_param(c, t);
				/* 階差 h を助変数 t に加える. */
				t += h;
				/* i をインクリメントする. */
			}
			/* 2n-1 番目の部分区間の微分値を求め, a に加える. */
			a += bezi_deriv_by_param(c, t);
			/* 0 番目の部分区間での微分値を求め, b0 に格納する. */
			b0 = bezi_deriv_by_param(c, t_min);
			/* 2n 番目の部分区間での微分値を求め, b2 に格納する. */
			b2 = bezi_deriv_by_param(c, t_max);
			/* (b0+4a+2b+b2)h/3 を求め, 積分値 s に格納する. */
			s = (b0 + 4.0 * a + 2.0 * b + b2) * h / 3.0f;
		}
		else {
			/* 0 を積分値 s に格納する. */
			s = 0.0;
		}
		/* s を返す. */
		return s;
	}

	//------------------------------
	// 曲線上の長さをもとに助変数を求める.
	// c	制御点
	// len	長さ
	// 戻り値	得られた助変数の値
	//------------------------------
	static double bezi_param_by_len(const POINT_2D c[4], const double len) noexcept
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
			e = bezi_len_by_param(c, 0.0, t, SIMPSON_CNT) - len;
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

}