//------------------------------
// Shape_elli.cpp
// だ円
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 方形の中点の配列
	//constexpr ANCH_WHICH ANCH_ELLI[4]{
	//	ANCH_SOUTH,
	//	ANCH_EAST,
	//	ANCH_WEST,
	//	ANCH_NORTH
	//};

	// 図形を表示する.
	void ShapeElli::draw(SHAPE_DX& dx)
	{
		auto br = dx.m_shape_brush.get();

		// 半径を求める.
		D2D1_POINT_2F rad;
		pt_scale(m_vec, 0.5, rad);
		// 中心点を求める.
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		// だ円構造体に格納する.
		D2D1_ELLIPSE elli{ c_pos, rad.x, rad.y };
		if (is_opaque(m_fill_color)) {
			// 塗りつぶし色が不透明ならだ円を塗りつぶす.
			dx.m_shape_brush->SetColor(m_fill_color);
			dx.m_d2dContext->FillEllipse(elli, br);
		}
		if (is_opaque(m_stroke_color)) {
			// 枠/線の色が不透明ならだ円を塗りつぶす.
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawEllipse(elli, br,
				static_cast<FLOAT>(m_stroke_width),
				m_d2d_stroke_style.get());
		}
		if (is_selected() == false) {
			return;
		}
		D2D1_POINT_2F a_pos[4];
		// 南
		a_pos[0].x = m_pos.x + m_vec.x * 0.5f;
		a_pos[0].y = m_pos.y + m_vec.y;
		// 東
		a_pos[1].x = m_pos.x + m_vec.x;
		a_pos[1].y = m_pos.y + m_vec.y * 0.5f;
		// 西
		a_pos[2].x = m_pos.x;
		a_pos[2].y = a_pos[1].y;
		// 北
		a_pos[3].x = a_pos[0].x;
		a_pos[3].y = m_pos.y;
		for (uint32_t i = 0; i < 4; i++) {
			TOOL_anchor(a_pos[i], dx);
		}
		a_pos[0] = m_pos;
		pt_add(m_pos, m_vec, a_pos[3]);
		a_pos[1].x = a_pos[0].x;
		a_pos[1].y = a_pos[3].y;
		a_pos[2].x = a_pos[3].x;
		a_pos[2].y = a_pos[0].y;
		for (uint32_t i = 0; i < 4; i++) {
			TOOL_anchor_rounded(a_pos[i], dx);
		}
	}

	// 位置を含むか調べる.
	// t_pos	調べる位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	ANCH_WHICH ShapeElli::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const auto anchor = hit_test_anchor(t_pos, a_len);
		if (anchor != ANCH_OUTSIDE) {
			return anchor;
		}

		// 半径を得る.
		D2D1_POINT_2F rad;
		pt_scale(m_vec, 0.5, rad);
		// 中心点を得る.
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		// 横の径が負数ならば正数にする.
		if (rad.x < 0.0) {
			rad.x = -rad.x;
		}
		// 縦の径が負数ならば正数にする.
		if (rad.y < 0.0) {
			rad.y = -rad.y;
		}
		if (is_opaque(m_stroke_color)) {
			// 位置がだ円の外側にあるか調べる.
			// 枠の太さが部位の大きさ未満ならば,
			// 部位の大きさを枠の太さに格納する.
			const double s_width = max(m_stroke_width, a_len);
			// 半径に枠の太さの半分を加えた値を外径に格納する.
			D2D1_POINT_2F r_outer;
			pt_add(rad, s_width * 0.5, r_outer);
			if (pt_in_elli(t_pos, c_pos, r_outer.x, r_outer.y) == false) {
				// 外径のだ円に含まれないなら, 
				// ANCH_OUTSIDE を返す.
				return ANCH_OUTSIDE;
			}
			// 位置がだ円の枠上にあるか調べる.
			D2D1_POINT_2F r_inner;
			// 外径から枠の太さを引いた値を内径に格納する.
			pt_add(r_outer, -s_width, r_inner);
			// 内径が負数なら,
			// ANCH_FRAME を返す.
			if (r_inner.x <= 0.0f) {
				return ANCH_FRAME;
			}
			if (r_inner.y <= 0.0f) {
				return ANCH_FRAME;
			}
			if (pt_in_elli(t_pos, c_pos, r_inner.x, r_inner.y) == false) {
				// 内径のだ円に含まれないなら ANCH_FRAME を返す.
				return ANCH_FRAME;
			}
		}
		if (is_opaque(m_fill_color)) {
			// 位置がだ円の内側にあるか調べる.
			if (pt_in_elli(t_pos, c_pos, rad.x, rad.y)) {
				return ANCH_INSIDE;
			}
		}
		return ANCH_OUTSIDE;
	}

	// データライターに SVG タグとして書き込む.
	void ShapeElli::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		D2D1_POINT_2F rad;
		pt_scale(m_vec, 0.5, rad);
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		write_svg("<ellipse ", dt_writer);
		write_svg(c_pos, "cx", "cy", dt_writer);
		write_svg(static_cast<double>(rad.x), "rx", dt_writer);
		write_svg(static_cast<double>(rad.y), "ry", dt_writer);
		write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>" SVG_NL, dt_writer);
	}
}