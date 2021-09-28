//------------------------------
// Shape_elli.cpp
// ‚¾‰~
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// }Œ`‚ğ•\¦‚·‚é.
	void ShapeElli::draw(D2D_UI& dx)
	{
		// ”¼Œa‚ğ‹‚ß‚é.
		D2D1_POINT_2F rad;
		pt_mul(m_vec[0], 0.5, rad);
		// ’†S“_‚ğ‹‚ß‚é.
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		// ‚¾‰~\‘¢‘Ì‚ÉŠi”[‚·‚é.
		D2D1_ELLIPSE elli{ c_pos, rad.x, rad.y };
		// “h‚è‚Â‚Ô‚µF‚ª•s“§–¾‚©”»’è‚·‚é.
		if (is_opaque(m_fill_color)) {
			dx.m_solid_color_brush->SetColor(m_fill_color);
			dx.m_d2d_context->FillEllipse(elli, dx.m_solid_color_brush.get());
		}
		// ˜gü‚ÌF‚ª•s“§–¾‚©”»’è‚·‚é.
		if (is_opaque(m_stroke_color)) {
			dx.m_solid_color_brush->SetColor(m_stroke_color);
			dx.m_d2d_context->DrawEllipse(elli, dx.m_solid_color_brush.get(), m_stroke_width, m_d2d_stroke_style.get());
		}
		if (is_selected() != true) {
			return;
		}
		D2D1_POINT_2F a_pos[4];
		// “ì
		a_pos[0].x = m_pos.x + m_vec[0].x * 0.5f;
		a_pos[0].y = m_pos.y + m_vec[0].y;
		// “Œ
		a_pos[1].x = m_pos.x + m_vec[0].x;
		a_pos[1].y = m_pos.y + m_vec[0].y * 0.5f;
		// ¼
		a_pos[2].x = m_pos.x;
		a_pos[2].y = a_pos[1].y;
		// –k
		a_pos[3].x = a_pos[0].x;
		a_pos[3].y = m_pos.y;
		for (uint32_t i = 0; i < 4; i++) {
			anch_draw_rect(a_pos[i], dx);
		}
		a_pos[0] = m_pos;
		pt_add(m_pos, m_vec[0], a_pos[3]);
		a_pos[1].x = a_pos[0].x;
		a_pos[1].y = a_pos[3].y;
		a_pos[2].x = a_pos[3].x;
		a_pos[2].y = a_pos[0].y;
		for (uint32_t i = 0; i < 4; i++) {
			anch_draw_ellipse(a_pos[i], dx);
		}
	}

	// ˆÊ’u‚ğŠÜ‚Ş‚©”»’è‚·‚é.
	// t_pos	”»’è‚·‚éˆÊ’u
	// –ß‚è’l	ˆÊ’u‚ğŠÜ‚Ş}Œ`‚Ì•”ˆÊ
	uint32_t ShapeElli::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		const auto anch = hit_test_anch(t_pos);
		if (anch != ANCH_TYPE::ANCH_SHEET) {
			return anch;
		}

		// ”¼Œa‚ğ“¾‚é.
		D2D1_POINT_2F rad;
		pt_mul(m_vec[0], 0.5, rad);
		// ’†S“_‚ğ“¾‚é.
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		rad.x = fabsf(rad.x);
		rad.y = fabsf(rad.y);
		if (is_opaque(m_stroke_color)) {
			// ˆÊ’u‚ª‚¾‰~‚ÌŠO‘¤‚É‚ ‚é‚©”»’è‚·‚é.
			// ˜g‚Ì‘¾‚³‚ª•”ˆÊ‚Ì‘å‚«‚³–¢–‚È‚ç‚Î,
			// •”ˆÊ‚Ì‘å‚«‚³‚ğ˜g‚Ì‘¾‚³‚ÉŠi”[‚·‚é.
			const double s_width = max(static_cast<double>(m_stroke_width), Shape::s_anch_len);
			// ”¼Œa‚É˜g‚Ì‘¾‚³‚Ì”¼•ª‚ğ‰Á‚¦‚½’l‚ğŠOŒa‚ÉŠi”[‚·‚é.
			D2D1_POINT_2F r_outer;
			pt_add(rad, s_width * 0.5, r_outer);
			if (pt_in_elli(t_pos, c_pos, r_outer.x, r_outer.y) != true) {
				// ŠOŒa‚Ì‚¾‰~‚ÉŠÜ‚Ü‚ê‚È‚¢‚È‚ç, 
				// ANCH_SHEET ‚ğ•Ô‚·.
				return ANCH_TYPE::ANCH_SHEET;
			}
			// ˆÊ’u‚ª‚¾‰~‚Ì˜gã‚É‚ ‚é‚©”»’è‚·‚é.
			D2D1_POINT_2F r_inner;
			// ŠOŒa‚©‚ç˜g‚Ì‘¾‚³‚ğˆø‚¢‚½’l‚ğ“àŒa‚ÉŠi”[‚·‚é.
			pt_add(r_outer, -s_width, r_inner);
			// “àŒa‚ª•‰”‚È‚ç,
			// ANCH_STROKE ‚ğ•Ô‚·.
			if (r_inner.x <= 0.0f) {
				return ANCH_TYPE::ANCH_STROKE;
			}
			if (r_inner.y <= 0.0f) {
				return ANCH_TYPE::ANCH_STROKE;
			}
			// “àŒa‚Ì‚¾‰~‚ÉŠÜ‚Ü‚ê‚È‚¢‚©”»’è‚·‚é.
			if (!pt_in_elli(t_pos, c_pos, r_inner.x, r_inner.y)) {
				return ANCH_TYPE::ANCH_STROKE;
			}
		}
		if (is_opaque(m_fill_color)) {
			// ‚¾‰~‚ÉˆÊ’u‚ªŠÜ‚Ü‚ê‚é‚©”»’è‚·‚é.
			if (pt_in_elli(t_pos, c_pos, rad.x, rad.y)) {
				return ANCH_TYPE::ANCH_FILL;
			}
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// ƒf[ƒ^ƒ‰ƒCƒ^[‚É SVG ƒ^ƒO‚Æ‚µ‚Ä‘‚«‚Ş.
	void ShapeElli::write_svg(DataWriter const& dt_writer) const
	{
		D2D1_POINT_2F rad;
		pt_mul(m_vec[0], 0.5, rad);
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