//------------------------------
// shape.cpp
// 図形のひな型, その他
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
#if defined(_DEBUG)
	uint32_t debug_deleted_cnt = 0;
	uint32_t debug_leak_cnt = 0;
	uint32_t debug_shape_cnt = 0;
#endif
	float Shape::s_anp_len = 6.0f;	// アンカーポイントの大きさ
	D2D1_COLOR_F Shape::s_background_color = COLOR_WHITE;	// 前景色 (アンカーの背景色)
	D2D1_COLOR_F Shape::s_foreground_color = COLOR_BLACK;	// 背景色 (アンカーの前景色)
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style = nullptr;	// 補助線の形式

	// 図形の部位（円形）を表示する.
	// a_pos	部位の位置
	// dx		図形の描画環境
	void anp_draw_ellipse(const D2D1_POINT_2F a_pos, D2D_UI& dx)
	{
		const FLOAT rad = static_cast<FLOAT>(Shape::s_anp_len * 0.5 + 1.0);
		ID2D1SolidColorBrush* const brush = dx.m_solid_color_brush.get();
		brush->SetColor(Shape::s_background_color);
		dx.m_d2d_context->FillEllipse(D2D1_ELLIPSE{ a_pos, rad, rad }, brush);
		brush->SetColor(Shape::s_foreground_color);
		dx.m_d2d_context->FillEllipse(D2D1_ELLIPSE{ a_pos, rad - 1.0f, rad - 1.0f }, brush);
	}

	// 図形の部位 (方形) を表示する.
	// a_pos	部位の位置
	// dx		図形の描画環境
	void anp_draw_rect(const D2D1_POINT_2F a_pos, D2D_UI& dx)
	{
		D2D1_POINT_2F r_min;
		D2D1_POINT_2F r_max;
		pt_add(a_pos, -0.5 * Shape::s_anp_len, r_min);
		pt_add(r_min, Shape::s_anp_len, r_max);
		const D2D1_RECT_F rect{ r_min.x, r_min.y, r_max.x, r_max.y };
		ID2D1SolidColorBrush* const brush = dx.m_solid_color_brush.get();
		brush->SetColor(Shape::s_background_color);
		dx.m_d2d_context->DrawRectangle(rect, brush, 2.0, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		dx.m_d2d_context->FillRectangle(rect, brush);
	}

	// 矢じるしの返しの位置を求める.
	// a_vec	矢軸ベクトル.
	// a_len	矢軸ベクトルの長さ
	// h_width	矢じるしの幅 (返しの間の長さ)
	// h_len	矢じるしの長さ (先端から返しまでの軸ベクトル上での長さ)
	// barbs[2]	計算された矢じるしの返しの位置 (先端からのオフセット)
	void get_arrow_barbs(const D2D1_POINT_2F a_vec, const double a_len, const double h_width, const double h_len, D2D1_POINT_2F barbs[]) noexcept
	{
		if (a_len <= DBL_MIN) {
			constexpr D2D1_POINT_2F Z{ 0.0f, 0.0f };
			barbs[0] = Z;
			barbs[1] = Z;
		}
		else {
			const double hf = h_width * 0.5;	// 矢じるしの幅の半分の大きさ
			const double sx = a_vec.x * -h_len;	// 矢軸ベクトルを矢じるしの長さ分反転
			const double sy = a_vec.x * hf;
			const double tx = a_vec.y * -h_len;
			const double ty = a_vec.y * hf;
			const double ax = 1.0 / a_len;
			barbs[0].x = static_cast<FLOAT>((sx - ty) * ax);
			barbs[0].y = static_cast<FLOAT>((tx + sy) * ax);
			barbs[1].x = static_cast<FLOAT>((sx + ty) * ax);
			barbs[1].y = static_cast<FLOAT>((tx - sy) * ax);
		}
	}

	// 二点で囲まれた方形を得る.
	/*
	void pt_bound(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c_min, D2D1_POINT_2F& c_max) noexcept
	{
		if (a.x < b.x) {
			c_min.x = a.x;
			c_max.x = b.x;
		}
		else {
			c_min.x = b.x;
			c_max.x = a.x;
		}
		if (a.y < b.y) {
			c_min.y = a.y;
			c_max.y = b.y;
		}
		else {
			c_min.y = b.y;
			c_max.y = a.y;
		}
	}
	*/
	// 多角形が位置を含むか判定する.
	// t_pos	判定する位置
	// v_cnt	頂点の数
	// v_pos	頂点の配列 [v_cnt]
	// 戻り値	含む場合 true
	// 多角形の各辺と, 指定された点を開始点とする水平線が交差する数を求める.
	bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t v_cnt, const D2D1_POINT_2F v_pos[]) noexcept
	{
		const double tx = t_pos.x;
		const double ty = t_pos.y;
		int i_cnt;	// 交点の数
		int i;

		double px = v_pos[v_cnt - 1].x;
		double py = v_pos[v_cnt - 1].y;
		i_cnt = 0;
		for (i = 0; i < v_cnt; i++) {
			double qx = v_pos[i].x;
			double qy = v_pos[i].y;
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

	// 方形が位置を含むか判定する.
	// t_pos	判定する位置
	// r_min	方形のいずれかの頂点
	// r_max	方形のもう一方の頂点
	// 戻り値	含む場合 true
	bool pt_in_rect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept
	{
		double min_x;
		double max_x;
		double min_y;
		double max_y;

		if (r_min.x < r_max.x) {
			min_x = r_min.x;
			max_x = r_max.x;
		}
		else {
			min_x = r_max.x;
			max_x = r_min.x;
		}
		if (r_min.y < r_max.y) {
			min_y = r_min.y;
			max_y = r_max.y;
		}
		else {
			min_y = r_max.y;
			max_y = r_min.y;
		}
		return min_x <= t_pos.x && t_pos.x <= max_x && min_y <= t_pos.y && t_pos.y <= max_y;
	}

	// 指定した位置を含むよう, 方形を拡大する.
	// a	含まれる位置
	// r_min	元の方形の左上位置, 得られた左上位置
	// r_max	元の方形の右下位置, 得られた右下位置
	/*
	void pt_inc(const D2D1_POINT_2F a, D2D1_POINT_2F& r_min, D2D1_POINT_2F& r_max) noexcept
	{
		if (a.x < r_min.x) {
			r_min.x = a.x;
		}
		if (a.x > r_max.x) {
			r_max.x = a.x;
		}
		if (a.y < r_min.y) {
			r_min.y = a.y;
		}
		if (a.y > r_max.y) {
			r_max.y = a.y;
		}
	}
	*/
}
