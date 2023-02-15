//------------------------------
// Shape_elli.cpp
// ‚¾‰~
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataWriter;

	// }Œ`‚ğ•\¦‚·‚é.
	void ShapeEllipse::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::s_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::s_d2d_color_brush;
		ID2D1Factory* factory;
		target->GetFactory(&factory);

		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}

		// ”¼Œa‚ğ‹‚ß‚é.
		D2D1_POINT_2F rad;
		pt_mul(m_vec[0], 0.5, rad);
		// ’†S“_‚ğ‹‚ß‚é.
		D2D1_POINT_2F c_pos;
		pt_add(m_start, rad, c_pos);
		// ‚¾‰~\‘¢‘Ì‚ÉŠi”[‚·‚é.
		D2D1_ELLIPSE elli{ c_pos, rad.x, rad.y };
		// “h‚è‚Â‚Ô‚µF‚ª•s“§–¾‚©”»’è‚·‚é.
		if (is_opaque(m_fill_color)) {
			brush->SetColor(m_fill_color);
			target->FillEllipse(elli, brush);
		}
		// ˜gü‚ÌF‚ª•s“§–¾‚©”»’è‚·‚é.
		if (is_opaque(m_stroke_color)) {
			brush->SetColor(m_stroke_color);
			target->DrawEllipse(elli, brush, m_stroke_width, m_d2d_stroke_style.get());
		}
		if (!is_selected()) {
			return;
		}
		draw_anc();
	}

	// ˆÊ’u‚ğŠÜ‚Ş‚©”»’è‚·‚é.
	// t_pos	”»’è‚·‚éˆÊ’u
	// –ß‚è’l	ˆÊ’u‚ğŠÜ‚Ş}Œ`‚Ì•”ˆÊ
	uint32_t ShapeEllipse::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const auto anc = rect_hit_test_anc(m_start, m_vec[0], t_pos, a_len);
		if (anc != ANC_TYPE::ANC_PAGE) {
			return anc;
		}

		// ”¼Œa‚ğ“¾‚é.
		D2D1_POINT_2F rad;
		pt_mul(m_vec[0], 0.5, rad);
		// ’†S“_‚ğ“¾‚é.
		D2D1_POINT_2F c_pos;
		pt_add(m_start, rad, c_pos);
		rad.x = fabsf(rad.x);
		rad.y = fabsf(rad.y);
		if (is_opaque(m_stroke_color)) {
			// ˆÊ’u‚ª‚¾‰~‚ÌŠO‘¤‚É‚ ‚é‚©”»’è‚·‚é.
			// ˜g‚Ì‘¾‚³‚ª•”ˆÊ‚Ì‘å‚«‚³–¢–‚È‚ç‚Î,
			// •”ˆÊ‚Ì‘å‚«‚³‚ğ˜g‚Ì‘¾‚³‚ÉŠi”[‚·‚é.
			const double s_width = max(static_cast<double>(m_stroke_width), a_len);
			// ”¼Œa‚É˜g‚Ì‘¾‚³‚Ì”¼•ª‚ğ‰Á‚¦‚½’l‚ğŠOŒa‚ÉŠi”[‚·‚é.
			D2D1_POINT_2F r_outer;
			pt_add(rad, s_width * 0.5, r_outer);
			if (!pt_in_ellipse(t_pos, c_pos, r_outer.x, r_outer.y)) {
				// ŠOŒa‚Ì‚¾‰~‚ÉŠÜ‚Ü‚ê‚È‚¢‚È‚ç, 
				// ANC_PAGE ‚ğ•Ô‚·.
				return ANC_TYPE::ANC_PAGE;
			}
			// ˆÊ’u‚ª‚¾‰~‚Ì˜gã‚É‚ ‚é‚©”»’è‚·‚é.
			D2D1_POINT_2F r_inner;
			// ŠOŒa‚©‚ç˜g‚Ì‘¾‚³‚ğˆø‚¢‚½’l‚ğ“àŒa‚ÉŠi”[‚·‚é.
			pt_add(r_outer, -s_width, r_inner);
			// “àŒa‚ª•‰”‚È‚ç,
			// ANC_STROKE ‚ğ•Ô‚·.
			if (r_inner.x <= 0.0f) {
				return ANC_TYPE::ANC_STROKE;
			}
			if (r_inner.y <= 0.0f) {
				return ANC_TYPE::ANC_STROKE;
			}
			// “àŒa‚Ì‚¾‰~‚ÉŠÜ‚Ü‚ê‚È‚¢‚©”»’è‚·‚é.
			if (!pt_in_ellipse(t_pos, c_pos, r_inner.x, r_inner.y)) {
				return ANC_TYPE::ANC_STROKE;
			}
		}
		if (is_opaque(m_fill_color)) {
			// ‚¾‰~‚ÉˆÊ’u‚ªŠÜ‚Ü‚ê‚é‚©”»’è‚·‚é.
			if (pt_in_ellipse(t_pos, c_pos, rad.x, rad.y)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

}