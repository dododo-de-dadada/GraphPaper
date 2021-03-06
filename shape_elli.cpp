//------------------------------
// Shape_elli.cpp
// だ円
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 図形を表示する.
	void ShapeElli::draw(SHAPE_DX& dx)
	{
		// 半径を求める.
		D2D1_POINT_2F rad;
		pt_mul(m_diff[0], 0.5, rad);
		// 中心点を求める.
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		// だ円構造体に格納する.
		D2D1_ELLIPSE elli{ c_pos, rad.x, rad.y };
		if (is_opaque(m_fill_color)) {
			// 塗りつぶし色が不透明な場合,
			dx.m_shape_brush->SetColor(m_fill_color);
			dx.m_d2dContext->FillEllipse(elli, dx.m_shape_brush.get());
		}
		if (is_opaque(m_stroke_color)) {
			// 枠線の色が不透明な場合,
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawEllipse(elli, dx.m_shape_brush.get(), m_stroke_width, m_d2d_stroke_style.get());
		}
		if (is_selected() != true) {
			return;
		}
		D2D1_POINT_2F a_pos[4];
		// 南
		a_pos[0].x = m_pos.x + m_diff[0].x * 0.5f;
		a_pos[0].y = m_pos.y + m_diff[0].y;
		// 東
		a_pos[1].x = m_pos.x + m_diff[0].x;
		a_pos[1].y = m_pos.y + m_diff[0].y * 0.5f;
		// 西
		a_pos[2].x = m_pos.x;
		a_pos[2].y = a_pos[1].y;
		// 北
		a_pos[3].x = a_pos[0].x;
		a_pos[3].y = m_pos.y;
		for (uint32_t i = 0; i < 4; i++) {
			anch_draw_rect(a_pos[i], dx);
		}
		a_pos[0] = m_pos;
		pt_add(m_pos, m_diff[0], a_pos[3]);
		a_pos[1].x = a_pos[0].x;
		a_pos[1].y = a_pos[3].y;
		a_pos[2].x = a_pos[3].x;
		a_pos[2].y = a_pos[0].y;
		for (uint32_t i = 0; i < 4; i++) {
			anch_draw_ellipse(a_pos[i], dx);
		}
	}

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeElli::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		const auto anch = hit_test_anch(t_pos);
		if (anch != ANCH_TYPE::ANCH_SHEET) {
			return anch;
		}

		// 半径を得る.
		D2D1_POINT_2F rad;
		pt_mul(m_diff[0], 0.5, rad);
		// 中心点を得る.
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		rad.x = fabsf(rad.x);
		rad.y = fabsf(rad.y);
		if (is_opaque(m_stroke_color)) {
			// 位置がだ円の外側にあるか判定する.
			// 枠の太さが部位の大きさ未満ならば,
			// 部位の大きさを枠の太さに格納する.
			const double s_width = max(static_cast<double>(m_stroke_width), Shape::s_anch_len);
			// 半径に枠の太さの半分を加えた値を外径に格納する.
			D2D1_POINT_2F r_outer;
			pt_add(rad, s_width * 0.5, r_outer);
			if (pt_in_elli(t_pos, c_pos, r_outer.x, r_outer.y) != true) {
				// 外径のだ円に含まれないなら, 
				// ANCH_SHEET を返す.
				return ANCH_TYPE::ANCH_SHEET;
			}
			// 位置がだ円の枠上にあるか判定する.
			D2D1_POINT_2F r_inner;
			// 外径から枠の太さを引いた値を内径に格納する.
			pt_add(r_outer, -s_width, r_inner);
			// 内径が負数なら,
			// ANCH_STROKE を返す.
			if (r_inner.x <= 0.0f) {
				return ANCH_TYPE::ANCH_STROKE;
			}
			if (r_inner.y <= 0.0f) {
				return ANCH_TYPE::ANCH_STROKE;
			}
			// 内径のだ円に含まれないか判定する.
			if (!pt_in_elli(t_pos, c_pos, r_inner.x, r_inner.y)) {
				return ANCH_TYPE::ANCH_STROKE;
			}
		}
		if (is_opaque(m_fill_color)) {
			// だ円に位置が含まれるか判定する.
			if (pt_in_elli(t_pos, c_pos, rad.x, rad.y)) {
				return ANCH_TYPE::ANCH_FILL;
			}
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// データライターに SVG タグとして書き込む.
	void ShapeElli::write_svg(DataWriter const& dt_writer) const
	{
		D2D1_POINT_2F rad;
		pt_mul(m_diff[0], 0.5, rad);
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		dt_write_svg("<ellipse ", dt_writer);
		dt_write_svg(c_pos, "cx", "cy", dt_writer);
		dt_write_svg(static_cast<double>(rad.x), "rx", dt_writer);
		dt_write_svg(static_cast<double>(rad.y), "ry", dt_writer);
		dt_write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
	}
}