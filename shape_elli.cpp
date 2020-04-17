//------------------------------
// Shape_elli.cpp
// Çæâ~
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ê}å`Çï\é¶Ç∑ÇÈ.
	void ShapeElli::draw(SHAPE_DX& dx)
	{
		// îºåaÇãÅÇﬂÇÈ.
		D2D1_POINT_2F rad;
		pt_scale(m_diff, 0.5, rad);
		// íÜêSì_ÇãÅÇﬂÇÈ.
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		// Çæâ~ç\ë¢ëÃÇ…äiî[Ç∑ÇÈ.
		D2D1_ELLIPSE elli{ c_pos, rad.x, rad.y };
		if (is_opaque(m_fill_color)) {
			// ìhÇËÇ¬Ç‘ÇµêFÇ™ïsìßñæÇ»èÍçá,
			dx.m_shape_brush->SetColor(m_fill_color);
			dx.m_d2dContext->FillEllipse(elli, dx.m_shape_brush.get());
		}
		if (is_opaque(m_stroke_color)) {
			// ògê¸ÇÃêFÇ™ïsìßñæÇ»èÍçá,
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawEllipse(elli, dx.m_shape_brush.get(), static_cast<FLOAT>(m_stroke_width), m_d2d_stroke_style.get());
		}
		if (is_selected() != true) {
			return;
		}
		D2D1_POINT_2F a_pos[4];
		// ìÏ
		a_pos[0].x = m_pos.x + m_diff.x * 0.5f;
		a_pos[0].y = m_pos.y + m_diff.y;
		// ìå
		a_pos[1].x = m_pos.x + m_diff.x;
		a_pos[1].y = m_pos.y + m_diff.y * 0.5f;
		// êº
		a_pos[2].x = m_pos.x;
		a_pos[2].y = a_pos[1].y;
		// ñk
		a_pos[3].x = a_pos[0].x;
		a_pos[3].y = m_pos.y;
		for (uint32_t i = 0; i < 4; i++) {
			anchor_draw_rect(a_pos[i], dx);
		}
		a_pos[0] = m_pos;
		pt_add(m_pos, m_diff, a_pos[3]);
		a_pos[1].x = a_pos[0].x;
		a_pos[1].y = a_pos[3].y;
		a_pos[2].x = a_pos[3].x;
		a_pos[2].y = a_pos[0].y;
		for (uint32_t i = 0; i < 4; i++) {
			anchor_draw_rounded(a_pos[i], dx);
		}
	}

	// à íuÇä‹ÇﬁÇ©í≤Ç◊ÇÈ.
	// t_pos	í≤Ç◊ÇÈà íu
	// a_len	ïîà ÇÃëÂÇ´Ç≥
	// ñﬂÇËíl	à íuÇä‹Çﬁê}å`ÇÃïîà 
	ANCH_WHICH ShapeElli::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const auto anchor = hit_test_anchor(t_pos, a_len);
		if (anchor != ANCH_WHICH::ANCH_OUTSIDE) {
			return anchor;
		}

		// îºåaÇìæÇÈ.
		D2D1_POINT_2F rad;
		pt_scale(m_diff, 0.5, rad);
		// íÜêSì_ÇìæÇÈ.
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		// â°ÇÃåaÇ™ïâêîÇ»ÇÁÇŒê≥êîÇ…Ç∑ÇÈ.
		//if (rad.x < 0.0F) {
			rad.x = fabsf(rad.x);
		//}
		// ècÇÃåaÇ™ïâêîÇ»ÇÁÇŒê≥êîÇ…Ç∑ÇÈ.
		//if (rad.y < 0.0F) {
			rad.y = fabsf(rad.y);
		//}
		if (is_opaque(m_stroke_color)) {
			// à íuÇ™Çæâ~ÇÃäOë§Ç…Ç†ÇÈÇ©í≤Ç◊ÇÈ.
			// ògÇÃëæÇ≥Ç™ïîà ÇÃëÂÇ´Ç≥ñ¢ñûÇ»ÇÁÇŒ,
			// ïîà ÇÃëÂÇ´Ç≥ÇògÇÃëæÇ≥Ç…äiî[Ç∑ÇÈ.
			const double s_width = max(m_stroke_width, a_len);
			// îºåaÇ…ògÇÃëæÇ≥ÇÃîºï™Çâ¡Ç¶ÇΩílÇäOåaÇ…äiî[Ç∑ÇÈ.
			D2D1_POINT_2F r_outer;
			pt_add(rad, s_width * 0.5, r_outer);
			if (pt_in_elli(t_pos, c_pos, r_outer.x, r_outer.y) != true) {
				// äOåaÇÃÇæâ~Ç…ä‹Ç‹ÇÍÇ»Ç¢Ç»ÇÁ, 
				// ANCH_OUTSIDE Çï‘Ç∑.
				return ANCH_WHICH::ANCH_OUTSIDE;
			}
			// à íuÇ™Çæâ~ÇÃògè„Ç…Ç†ÇÈÇ©í≤Ç◊ÇÈ.
			D2D1_POINT_2F r_inner;
			// äOåaÇ©ÇÁògÇÃëæÇ≥Çà¯Ç¢ÇΩílÇì‡åaÇ…äiî[Ç∑ÇÈ.
			pt_add(r_outer, -s_width, r_inner);
			// ì‡åaÇ™ïâêîÇ»ÇÁ,
			// ANCH_FRAME Çï‘Ç∑.
			if (r_inner.x <= 0.0f) {
				return ANCH_WHICH::ANCH_FRAME;
			}
			if (r_inner.y <= 0.0f) {
				return ANCH_WHICH::ANCH_FRAME;
			}
			if (pt_in_elli(t_pos, c_pos, r_inner.x, r_inner.y) != true) {
				// ì‡åaÇÃÇæâ~Ç…ä‹Ç‹ÇÍÇ»Ç¢Ç»ÇÁ ANCH_FRAME Çï‘Ç∑.
				return ANCH_WHICH::ANCH_FRAME;
			}
		}
		if (is_opaque(m_fill_color)) {
			// à íuÇ™Çæâ~ÇÃì‡ë§Ç…Ç†ÇÈÇ©í≤Ç◊ÇÈ.
			if (pt_in_elli(t_pos, c_pos, rad.x, rad.y)) {
				return ANCH_WHICH::ANCH_INSIDE;
			}
		}
		return ANCH_WHICH::ANCH_OUTSIDE;
	}

	// ÉfÅ[É^ÉâÉCÉ^Å[Ç… SVG É^ÉOÇ∆ÇµÇƒèëÇ´çûÇﬁ.
	void ShapeElli::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		D2D1_POINT_2F rad;
		pt_scale(m_diff, 0.5, rad);
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