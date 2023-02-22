//------------------------------
// shape_rrect.cpp
// ŠpŠÛ•ûŒ`}Œ`
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	// ŠpŠÛ”¼Œa‚ğŒvZ‚·‚é.
	static void calc_corner_radius(const D2D1_POINT_2F d_vec, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad);

	// ŠpŠÛ”¼Œa‚Ìc‚Ü‚½‚Í‰¡‚Ì¬•ª‚ğŒvZ‚·‚é.
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad);

	// ŠpŠÛ”¼Œa‚ğŒvZ‚·‚é.
	// d_vec	ŠpŠÛ•ûŒ`‚Ì‘ÎŠpƒxƒNƒgƒ‹
	// d_rad	Šù’è‚ÌŠpŠÛ”¼Œa
	// c_rad	ŒvZ‚³‚ê‚½ŠpŠÛ”¼Œa
	static void calc_corner_radius(const D2D1_POINT_2F d_vec, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad)
	{
		calc_corner_radius(d_vec.x, d_rad.x, c_rad.x);
		calc_corner_radius(d_vec.y, d_rad.y, c_rad.y);
	}

	// ŠpŠÛ”¼Œa‚Ìc‚Ü‚½‚Í‰¡‚Ì¬•ª‚ğŒvZ‚·‚é.
	// r_len	ŠpŠÛ•ûŒ`‚Ìˆê•Ó‚Ì‘å‚«‚³
	// d_rad	‚à‚Æ‚ÌŠpŠÛ”¼Œa
	// c_rad	“¾‚ç‚ê‚½ŠpŠÛ”¼Œa
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad)
	{
		const double r = r_len * 0.5;
		// ‚à‚Æ‚ÌŠpŠÛ”¼Œa‚ª•ûŒ`‚Ì‘å‚«‚³‚Ì”¼•ª‚ğ’´‚¦‚È‚¢‚æ‚¤‚É‚·‚é.
		if (fabs(d_rad) > fabs(r)) {
			c_rad = static_cast<FLOAT>(r);
		}
		else if (r_len * d_rad < 0.0f) {
			// ŠpŠÛ•ûŒ`‚Ì‘å‚«‚³‚Æ‚à‚Æ‚ÌŠpŠÛ”¼Œa‚Ì•„†‚ª‹t‚È‚ç,
			// ‚à‚Æ‚ÌŠpŠÛ”¼Œa‚Ì•„†‚ğ‹t‚É‚µ‚½’l‚ğ
			// “¾‚ç‚ê‚½ŠpŠÛ”¼Œa‚ÉŠi”[‚·‚é.
			c_rad = -d_rad;
		}
		else {
			c_rad = d_rad;
		}
	}

	// }Œ`‚ğ•\¦‚·‚é.
	void ShapeRRect::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		if (m_d2d_stroke_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			create_stroke_style(factory);
		}

		D2D1_POINT_2F r_lt;
		pt_add(m_start, min(m_vec[0].x, 0.0), min(m_vec[0].y, 0.0), r_lt);
		float rx = std::fabsf(m_corner_radius.x);
		float ry = std::fabsf(m_corner_radius.y);
		float vx = std::fabsf(m_vec[0].x);
		float vy = std::fabsf(m_vec[0].y);
		if (rx > vx * 0.5f) {
			rx = vx * 0.5f;
		}
		if (ry > vy * 0.5f) {
			ry = vy * 0.5f;
		}
		D2D1_ROUNDED_RECT r_rec;
		r_rec.rect.left = r_lt.x;
		r_rec.rect.top = r_lt.y;
		r_rec.rect.right = r_lt.x + vx;
		r_rec.rect.bottom = r_lt.y + vy;
		r_rec.radiusX = rx;
		r_rec.radiusY = ry;
		if (is_opaque(m_fill_color)) {
			brush->SetColor(m_fill_color);
			target->FillRoundedRectangle(r_rec, brush);
		}
		brush->SetColor(m_stroke_color);
		target->DrawRoundedRectangle(r_rec, brush, m_stroke_width, m_d2d_stroke_style.get());
		if (m_anc_show && is_selected()) {
			D2D1_POINT_2F circle[4]{	// ‰~ã‚Ì“_
				{ r_lt.x + rx, r_lt.y + ry },
				{ r_lt.x + vx - rx, r_lt.y + ry },
				{ r_lt.x + vx - rx, r_lt.y + vy - ry },
				{ r_lt.x + rx, r_lt.y + vy - ry }
			};
			anc_draw_circle(circle[2], target, brush);
			anc_draw_circle(circle[3], target, brush);
			anc_draw_circle(circle[1], target, brush);
			anc_draw_circle(circle[0], target, brush);
			draw_anc();
		}
	}

	// ŠpŠÛ”¼Œa‚ğ“¾‚é.
	bool ShapeRRect::get_corner_radius(D2D1_POINT_2F& val) const noexcept
	{
		val = m_corner_radius;
		return true;
	}

	//	•”ˆÊ‚ÌˆÊ’u‚ğ“¾‚é.
	//	anc	}Œ`‚Ì•”ˆÊ.
	//	val	“¾‚ç‚ê‚½ˆÊ’u.
	//	–ß‚è’l	‚È‚µ
	void ShapeRRect::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		const double dx = m_vec[0].x;	// ·•ª x
		const double dy = m_vec[0].y;	// ·•ª y
		const double mx = dx * 0.5;	// ’†“_ x
		const double my = dy * 0.5;	// ’†“_ y
		const double rx = fabs(mx) < fabs(m_corner_radius.x) ? mx : m_corner_radius.x;	// ŠpŠÛ x
		const double ry = fabs(my) < fabs(m_corner_radius.y) ? my : m_corner_radius.y;	// ŠpŠÛ y

		switch (anc) {
		case ANC_TYPE::ANC_R_NW:
			// ¶ã‚ÌŠpŠÛ’†S“_‚ğ‹‚ß‚é
			pt_add(m_start, rx, ry, val);
			break;
		case ANC_TYPE::ANC_R_NE:
			// ‰Eã‚ÌŠpŠÛ’†S“_‚ğ‹‚ß‚é
			pt_add(m_start, dx - rx, ry, val);
			break;
		case ANC_TYPE::ANC_R_SE:
			// ‰E‰º‚ÌŠpŠÛ’†S“_‚ğ‹‚ß‚é
			pt_add(m_start, dx - rx, dy - ry, val);
			break;
		case ANC_TYPE::ANC_R_SW:
			// ¶‰º‚ÌŠpŠÛ’†S“_‚ğ‹‚ß‚é
			pt_add(m_start, rx, dy - ry, val);
			break;
		default:
			ShapeRect::get_pos_anc(anc, val);
			break;
		}
	}

	// ˆÊ’u‚ªŠpŠÛ•ûŒ`‚ÉŠÜ‚Ü‚ê‚é‚©”»’è‚·‚é.
	// test	”»’è‚·‚éˆÊ’u
	// r_lt	ŠpŠÛ•ûŒ`‚Ì¶ãˆÊ’u
	// r_rb	ŠpŠÛ•ûŒ`‚Ì‰E‰ºˆÊ’u
	// r_rad	ŠpŠÛ‚Ì”¼Œa
	// –ß‚è’l	ŠÜ‚Ü‚ê‚é‚È‚ç true ‚ğ•Ô‚·.
	static bool pt_in_rrect(const D2D1_POINT_2F test, const D2D1_POINT_2F r_lt, const D2D1_POINT_2F r_rb, const D2D1_POINT_2F r_rad)
	{
		if (test.x < r_lt.x) {
			return false;
		}
		if (test.x > r_rb.x) {
			return false;
		}
		if (test.y < r_lt.y) {
			return false;
		}
		if (test.y > r_rb.y) {
			return false;
		}
		D2D1_POINT_2F center;	// ŠpŠÛ‚Ì’†S“_
		pt_add(r_lt, r_rad, center);
		if (test.x < center.x) {
			if (test.y < center.y) {
				return pt_in_ellipse(test, center, r_rad.x, r_rad.y);
			}
		}
		center.x = r_rb.x - r_rad.x;
		center.y = r_lt.y + r_rad.y;
		if (test.x > center.x) {
			if (test.y < center.y) {
				return pt_in_ellipse(test, center, r_rad.x, r_rad.y);
			}
		}
		center.x = r_rb.x - r_rad.x;
		center.y = r_rb.y - r_rad.y;
		if (test.x > center.x) {
			if (test.y > center.y) {
				return pt_in_ellipse(test, center, r_rad.x, r_rad.y);
			}
		}
		center.x = r_lt.x + r_rad.x;
		center.y = r_rb.y - r_rad.y;
		if (test.x < center.x) {
			if (test.y > center.y) {
				return pt_in_ellipse(test, center, r_rad.x, r_rad.y);
			}
		}
		return true;
	}

	// ˆÊ’u‚ğŠÜ‚Ş‚©”»’è‚·‚é.
	// test	”»’è‚·‚éˆÊ’u
	// –ß‚è’l	ˆÊ’u‚ğŠÜ‚Ş}Œ`‚Ì•”ˆÊ
	uint32_t ShapeRRect::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		// ŠpŠÛ‚Ì‰~ŒÊ‚Ì’†S“_‚ÉŠÜ‚Ü‚ê‚é‚©”»’è‚·‚é.
		// +---------+
		// | 1     3 |
		// |         |
		// | 4     2 |
		// +---------+
		uint32_t anc_r;
		const double mx = m_vec[0].x * 0.5;	// ’†“_
		const double my = m_vec[0].y * 0.5;	// ’†“_
		const double rx = fabs(mx) < fabs(m_corner_radius.x) ? mx : m_corner_radius.x;	// ŠpŠÛ
		const double ry = fabs(my) < fabs(m_corner_radius.y) ? my : m_corner_radius.y;	// ŠpŠÛ
		const D2D1_POINT_2F anc_r_nw{
			static_cast<FLOAT>(m_start.x + rx), 
			static_cast<FLOAT>(m_start.y + ry)
		};
		if (pt_in_anc(test, anc_r_nw, m_anc_width)) {
			anc_r = ANC_TYPE::ANC_R_NW;
		}
		else {
			const D2D1_POINT_2F anc_r_se{
				static_cast<FLOAT>(m_start.x + m_vec[0].x - rx),
				static_cast<FLOAT>(m_start.y + m_vec[0].y - ry)
			};
			if (pt_in_anc(test, anc_r_se, m_anc_width)) {
				anc_r = ANC_TYPE::ANC_R_SE;
			}
			else {
				const D2D1_POINT_2F anc_r_ne{ anc_r_se.x, anc_r_nw.y };
				if (pt_in_anc(test, anc_r_ne, m_anc_width)) {
					anc_r = ANC_TYPE::ANC_R_NE;
				}
				else {
					const D2D1_POINT_2F anc_r_sw{ anc_r_nw.x, anc_r_se.y };
					if (pt_in_anc(test, anc_r_sw, m_anc_width)) {
						anc_r = ANC_TYPE::ANC_R_SW;
					}
					else {
						anc_r = ANC_TYPE::ANC_PAGE;
					}
				}
			}
		}
		// ŠpŠÛ‚Ì‰~ŒÊ‚Ì’†S“_‚ÉŠÜ‚Ü‚ê‚é,
		if (anc_r != ANC_TYPE::ANC_PAGE &&
			// ‚©‚Â, •ûŒ`‚Ì‘å‚«‚³‚ª}Œ`‚Ì•”ˆÊ‚Ì‘å‚«‚³‚æ‚è‘å‚«‚¢‚©”»’è‚·‚é.
			fabs(m_vec[0].x) > m_anc_width && fabs(m_vec[0].y) > m_anc_width) {
			return anc_r;
		}
		// •ûŒ`‚ÌŠe’¸“_‚ÉŠÜ‚Ü‚ê‚é‚©”»’è‚·‚é.
		const uint32_t anc_v = rect_hit_test_anc(m_start, m_vec[0], test, m_anc_width);
		if (anc_v != ANC_TYPE::ANC_PAGE) {
			return anc_v;
		}
		// ’¸“_‚ÉŠÜ‚Ü‚ê‚¸, ŠpŠÛ‚Ì‰~ŒÊ‚Ì’†S“_‚ÉŠÜ‚Ü‚ê‚é‚©”»’è‚·‚é.
		else if (anc_r != ANC_TYPE::ANC_PAGE) {
			return anc_r;
		}

		// ŠpŠÛ•ûŒ`‚ğ³‹K‰»‚·‚é.
		D2D1_POINT_2F r_lt;
		D2D1_POINT_2F r_rb;
		D2D1_POINT_2F r_rad;
		if (m_vec[0].x > 0.0f) {
			r_lt.x = m_start.x;
			r_rb.x = m_start.x + m_vec[0].x;
		}
		else {
			r_lt.x = m_start.x + m_vec[0].x;
			r_rb.x = m_start.x;
		}
		if (m_vec[0].y > 0.0f) {
			r_lt.y = m_start.y;
			r_rb.y = m_start.y + m_vec[0].y;
		}
		else {
			r_lt.y = m_start.y + m_vec[0].y;
			r_rb.y = m_start.y;
		}
		r_rad.x = std::abs(m_corner_radius.x);
		r_rad.y = std::abs(m_corner_radius.y);

		// ü˜g‚ª“§–¾‚Ü‚½‚Í‘¾‚³ 0 ‚©”»’è‚·‚é.
		if (!is_opaque(m_stroke_color) || m_stroke_width < FLT_MIN) {
			// “h‚è‚Â‚Ô‚µF‚ª•s“§–¾, ‚©‚ÂŠpŠÛ•ûŒ`‚»‚Ì‚à‚Ì‚ÉŠÜ‚Ü‚ê‚é‚©”»’è‚·‚é.
			if (is_opaque(m_fill_color) && pt_in_rrect(test, r_lt, r_rb, r_rad)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		// ü˜g‚ÌF‚ª•s“§–¾, ‚©‚Â‘¾‚³‚ª 0 ‚æ‚è‘å‚«‚¢.
		else {
			// ˆÈ‰º‚Ìè‡‚Í, k¬‚µ‚½ŠpŠÛ•ûŒ`‚ÌŠO‘¤‚É, ŠpŠÛŒğ“_‚ª‚ ‚éƒP[ƒX‚Å‚¤‚Ü‚­‚¢‚©‚È‚¢.
			// Šg‘å‚µ‚½ŠpŠÛ•ûŒ`‚ÉŠÜ‚Ü‚ê‚é‚©”»’è
			const double s_thick = max(m_stroke_width, m_anc_width);
			D2D1_POINT_2F e_lt, e_rb, e_rad;	// Šg‘å‚µ‚½ŠpŠÛ•ûŒ`
			pt_add(r_lt, -s_thick * 0.5, e_lt);
			pt_add(r_rb, s_thick * 0.5, e_rb);
			pt_add(r_rad, s_thick * 0.5, e_rad);
			if (pt_in_rrect(test, e_lt, e_rb, e_rad)) {
				// k¬‚µ‚½ŠpŠÛ•ûŒ`‚ª‹t“]‚µ‚Ä‚È‚¢, ‚©‚ÂˆÊ’u‚ªk¬‚µ‚½ŠpŠÛ•ûŒ`‚ÉŠÜ‚Ü‚ê‚é‚©”»’è‚·‚é.
				D2D1_POINT_2F s_lt, s_rb, s_rad;	// k¬‚µ‚½ŠpŠÛ•ûŒ`
				pt_add(e_lt, s_thick, s_lt);
				pt_add(e_rb, -s_thick, s_rb);
				pt_add(e_rad, -s_thick, s_rad);
				if (s_lt.x < s_rb.x && s_lt.y < s_rb.y && pt_in_rrect(test, s_lt, s_rb, s_rad)) {
					// “h‚è‚Â‚Ô‚µF‚ª•s“§–¾‚È‚ç, ANC_FILL ‚ğ•Ô‚·.
					if (is_opaque(m_fill_color)) {
						return ANC_TYPE::ANC_FILL;
					}
				}
				else {
					// Šg‘å‚µ‚½ŠpŠÛ•ûŒ`‚ÉŠÜ‚Ü‚ê, k¬‚µ‚½ŠpŠÛ•ûŒ`‚ÉŠÜ‚Ü‚ê‚È‚¢‚È‚ç ANC_STROKE ‚ğ•Ô‚·.
					return ANC_TYPE::ANC_STROKE;
				}
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// ’l‚ğ, •”ˆÊ‚ÌˆÊ’u‚ÉŠi”[‚·‚é. ‘¼‚Ì•”ˆÊ‚ÌˆÊ’u‚à“®‚­.
	// val	’l
	// anc	}Œ`‚Ì•”ˆÊ
	// limit	ŒÀŠE‹——£ (‘¼‚Ì’¸“_‚Æ‚Ì‹——£‚ª‚±‚Ì’l–¢–‚É‚È‚é‚È‚ç, ‚»‚Ì’¸“_‚ÉˆÊ’u‚É‡‚í‚¹‚é)
	bool ShapeRRect::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool /*keep_aspect*/) noexcept
	{
		D2D1_POINT_2F a_point;	// }Œ`‚Ì•”ˆÊ‚ÌˆÊ’u
		D2D1_POINT_2F posotion;	// ˆÊ’uƒxƒNƒgƒ‹
		D2D1_POINT_2F new_radius;
		D2D1_POINT_2F new_point;

		switch (anc) {
		case ANC_TYPE::ANC_R_NW:
			ShapeRRect::get_pos_anc(anc, a_point);
			pt_round(val, PT_ROUND, new_point);
			pt_sub(new_point, a_point, posotion);
			if (pt_abs2(posotion) < FLT_MIN) {
				return false;
			}
			pt_add(m_corner_radius, posotion, new_radius);
			calc_corner_radius(m_vec[0], new_radius, m_corner_radius);
			break;
		case ANC_TYPE::ANC_R_NE:
			ShapeRRect::get_pos_anc(anc, a_point);
			pt_round(val, PT_ROUND, new_point);
			pt_sub(new_point, a_point, posotion);
			if (pt_abs2(posotion) < FLT_MIN) {
				return false;
			}
			new_radius.x = m_corner_radius.x - posotion.x;
			new_radius.y = m_corner_radius.y + posotion.y;
			calc_corner_radius(m_vec[0], new_radius, m_corner_radius);
			break;
		case ANC_TYPE::ANC_R_SE:
			ShapeRRect::get_pos_anc(anc, a_point);
			pt_round(val, PT_ROUND, new_point);
			pt_sub(new_point, a_point, posotion);
			if (pt_abs2(posotion) < FLT_MIN) {
				return false;
			}
			new_radius.x = m_corner_radius.x - posotion.x;
			new_radius.y = m_corner_radius.y - posotion.y;
			calc_corner_radius(m_vec[0], new_radius, m_corner_radius);
			break;
		case ANC_TYPE::ANC_R_SW:
			ShapeRRect::get_pos_anc(anc, a_point);
			pt_round(val, PT_ROUND, new_point);
			pt_sub(new_point, a_point, posotion);
			if (pt_abs2(posotion) < FLT_MIN) {
				return false;
			}
			new_radius.x = m_corner_radius.x + posotion.x;
			new_radius.y = m_corner_radius.y - posotion.y;
			calc_corner_radius(m_vec[0], new_radius, m_corner_radius);
			break;
		default:
			if (!ShapeRect::set_pos_anc(val, anc, limit, false)) {
				return false;
			}
			if (m_vec[0].x * m_corner_radius.x < 0.0f) {
				m_corner_radius.x = -m_corner_radius.x;
			}
			if (m_vec[0].y * m_corner_radius.y < 0.0f) {
				m_corner_radius.y = -m_corner_radius.y;
			}
			break;
		}
		const double d = static_cast<double>(limit);
		if (pt_abs2(m_corner_radius) < d * d) {
			m_corner_radius.x = m_corner_radius.y = 0.0f;
		}
		return true;
	}

	// }Œ`‚ğì¬‚·‚é.
	// start	ˆÍ‚Ş—Ìˆæ‚Ìn“_
	// b_vec	ˆÍ‚Ş—Ìˆæ‚ÌI“_‚Ö‚Ì·•ª
	// page	‘®«
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F start, const D2D1_POINT_2F b_vec, const Shape* page) :
		ShapeRect::ShapeRect(start, b_vec, page)
	{
		float g_base;
		page->get_grid_base(g_base);
		calc_corner_radius(m_vec[0], D2D1_POINT_2F{ g_base + 1.0f, g_base + 1.0f }, m_corner_radius);
	}

	// }Œ`‚ğƒf[ƒ^ƒŠ[ƒ_[‚©‚ç“Ç‚İ‚Ş.
	ShapeRRect::ShapeRRect(const Shape& page, DataReader const& dt_reader) :
		ShapeRect::ShapeRect(page, dt_reader)
	{
		m_corner_radius = D2D1_POINT_2F{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
	}

	// }Œ`‚ğƒf[ƒ^ƒ‰ƒCƒ^[‚É‘‚«‚Ş.
	void ShapeRRect::write(DataWriter const& dt_writer) const
	{
		ShapeRect::write(dt_writer);
		dt_writer.WriteSingle(m_corner_radius.x);
		dt_writer.WriteSingle(m_corner_radius.y);
	}

}