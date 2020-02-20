//------------------------------
// Shape_elli.cpp
// ‚¾‰~
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// •ûŒ`‚Ì’†“_‚Ì”z—ñ
	constexpr ANCH_WHICH ANCH_ELLI[4]{
		ANCH_SOUTH,
		ANCH_EAST,
		ANCH_WEST,
		ANCH_NORTH
	};

	// ‚¾‰~‚Ì•”ˆÊ‚ð“¾‚é.
	static void ep_get_anchor(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_vec, D2D1_POINT_2F a_pos[4]) noexcept;

	// ‚¾‰~‚Ì•”ˆÊ‚ð“¾‚é.
	static void ep_get_anchor(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_vec, D2D1_POINT_2F a_pos[4]) noexcept
	{
		// “ì
		a_pos[0].x = s_pos.x + d_vec.x * 0.5f;
		a_pos[0].y = s_pos.y + d_vec.y;
		// “Œ
		a_pos[1].x = s_pos.x + d_vec.x;
		a_pos[1].y = s_pos.y + d_vec.y * 0.5f;
		// ¼
		a_pos[2].x = s_pos.x;
		a_pos[2].y = a_pos[1].y;
		// –k
		a_pos[3].x = a_pos[0].x;
		a_pos[3].y = s_pos.y;
	}

	// }Œ`‚ð•\Ž¦‚·‚é.
	void ShapeElli::draw(SHAPE_DX& dx)
	{
		auto br = dx.m_shape_brush.get();

		// ”¼Œa‚ð‹‚ß‚é.
		D2D1_POINT_2F rad;
		pt_scale(m_vec, 0.5, rad);
		// ’†S“_‚ð‹‚ß‚é.
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		// ‚¾‰~\‘¢‘Ì‚ÉŠi”[‚·‚é.
		D2D1_ELLIPSE elli{ c_pos, rad.x, rad.y };
		if (is_opaque(m_fill_color)) {
			// “h‚è‚Â‚Ô‚µF‚ª•s“§–¾‚È‚ç‚¾‰~‚ð“h‚è‚Â‚Ô‚·.
			dx.m_shape_brush->SetColor(m_fill_color);
			dx.m_d2dContext->FillEllipse(elli, br);
		}
		if (is_opaque(m_stroke_color)) {
			// ˜g/ü‚ÌF‚ª•s“§–¾‚È‚ç‚¾‰~‚ð“h‚è‚Â‚Ô‚·.
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawEllipse(elli, br,
				static_cast<FLOAT>(m_stroke_width),
				m_d2d_stroke_style.get());
		}
		if (is_selected()) {
			D2D1_POINT_2F a_pos[4];
			ep_get_anchor(m_pos, m_vec, a_pos);
			for (uint32_t i = 0; i < 4; i++) {
				draw_anchor(a_pos[i], dx);
			}
		}
	}

	// ˆÊ’u‚ðŠÜ‚Þ‚©’²‚×‚é.
	// t_pos	’²‚×‚éˆÊ’u
	// a_len	•”ˆÊ‚Ì‘å‚«‚³
	// –ß‚è’l	ˆÊ’u‚ðŠÜ‚Þ}Œ`‚Ì•”ˆÊ
	ANCH_WHICH ShapeElli::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		D2D1_POINT_2F a_pos[4];
		ep_get_anchor(m_pos, m_vec, a_pos);
		for (uint32_t i = 0; i < 4; i++) {
			// ‚¾‰~‚ÌŠe•”ˆÊ‚ªˆÊ’u‚ðŠÜ‚Þ‚©’²‚×‚é.
			if (pt_in_anch(t_pos, a_pos[i], a_len)) {
				// ŠÜ‚Þ‚È‚ç‚»‚Ì•”ˆÊ‚ð•Ô‚·.
				return ANCH_ELLI[i];
			}
		}
		// ”¼Œa‚ð“¾‚é.
		D2D1_POINT_2F rad;
		pt_scale(m_vec, 0.5, rad);
		// ’†S“_‚ð“¾‚é.
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		// ‰¡‚ÌŒa‚ª•‰”‚È‚ç‚Î³”‚É‚·‚é.
		if (rad.x < 0.0) {
			rad.x = -rad.x;
		}
		// c‚ÌŒa‚ª•‰”‚È‚ç‚Î³”‚É‚·‚é.
		if (rad.y < 0.0) {
			rad.y = -rad.y;
		}
		if (is_opaque(m_stroke_color)) {
			// ˆÊ’u‚ª‚¾‰~‚ÌŠO‘¤‚É‚ ‚é‚©’²‚×‚é.
			// ˜g‚Ì‘¾‚³‚ª•”ˆÊ‚Ì‘å‚«‚³–¢–ž‚È‚ç‚Î,
			// •”ˆÊ‚Ì‘å‚«‚³‚ð˜g‚Ì‘¾‚³‚ÉŠi”[‚·‚é.
			const double s_width = max(m_stroke_width, a_len);
			// ”¼Œa‚É˜g‚Ì‘¾‚³‚Ì”¼•ª‚ð‰Á‚¦‚½’l‚ðŠOŒa‚ÉŠi”[‚·‚é.
			D2D1_POINT_2F r_outer;
			pt_add(rad, s_width * 0.5, r_outer);
			if (pt_in_elli(t_pos, c_pos, r_outer.x, r_outer.y) == false) {
				// ŠOŒa‚Ì‚¾‰~‚ÉŠÜ‚Ü‚ê‚È‚¢‚È‚ç, 
				// ANCH_OUTSIDE ‚ð•Ô‚·.
				return ANCH_OUTSIDE;
			}
			// ˆÊ’u‚ª‚¾‰~‚Ì˜gã‚É‚ ‚é‚©’²‚×‚é.
			D2D1_POINT_2F r_inner;
			// ŠOŒa‚©‚ç˜g‚Ì‘¾‚³‚ðˆø‚¢‚½’l‚ð“àŒa‚ÉŠi”[‚·‚é.
			pt_add(r_outer, -s_width, r_inner);
			// “àŒa‚ª•‰”‚È‚ç,
			// ANCH_FRAME ‚ð•Ô‚·.
			if (r_inner.x <= 0.0f) {
				return ANCH_FRAME;
			}
			if (r_inner.y <= 0.0f) {
				return ANCH_FRAME;
			}
			if (pt_in_elli(t_pos, c_pos, r_inner.x, r_inner.y) == false) {
				// “àŒa‚Ì‚¾‰~‚ÉŠÜ‚Ü‚ê‚È‚¢‚È‚ç ANCH_FRAME ‚ð•Ô‚·.
				return ANCH_FRAME;
			}
		}
		if (is_opaque(m_fill_color)) {
			// ˆÊ’u‚ª‚¾‰~‚Ì“à‘¤‚É‚ ‚é‚©’²‚×‚é.
			if (pt_in_elli(t_pos, c_pos, rad.x, rad.y)) {
				return ANCH_INSIDE;
			}
		}
		return ANCH_OUTSIDE;
	}

	// ƒf[ƒ^ƒ‰ƒCƒ^[‚É SVG ƒ^ƒO‚Æ‚µ‚Ä‘‚«ž‚Þ.
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