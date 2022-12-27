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
		ID2D1Factory* const factory = Shape::s_factory;
		ID2D1RenderTarget* const target = Shape::s_target;
		ID2D1SolidColorBrush* const brush = Shape::s_color_brush;

		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}

		D2D1_POINT_2F r_min;
		pt_add(m_pos, min(m_vec[0].x, 0.0), min(m_vec[0].y, 0.0), r_min);
		float rx = std::fabsf(m_corner_rad.x);
		float ry = std::fabsf(m_corner_rad.y);
		float vx = std::fabsf(m_vec[0].x);
		float vy = std::fabsf(m_vec[0].y);
		if (rx > vx * 0.5f) {
			rx = vx * 0.5f;
		}
		if (ry > vy * 0.5f) {
			ry = vy * 0.5f;
		}
		D2D1_ROUNDED_RECT r_rec;
		r_rec.rect.left = r_min.x;
		r_rec.rect.top = r_min.y;
		r_rec.rect.right = r_min.x + vx;
		r_rec.rect.bottom = r_min.y + vy;
		r_rec.radiusX = rx;
		r_rec.radiusY = ry;
		if (is_opaque(m_fill_color)) {
			brush->SetColor(m_fill_color);
			target->FillRoundedRectangle(r_rec, brush);
		}
		brush->SetColor(m_stroke_color);
		target->DrawRoundedRectangle(r_rec, brush, m_stroke_width, m_d2d_stroke_style.get());
		if (is_selected()) {
			D2D1_POINT_2F r_pos[4];
			r_pos[0] = r_min;
			r_pos[1].x = r_rec.rect.right;
			r_pos[1].y = r_rec.rect.top;
			r_pos[2].x = r_rec.rect.right;
			r_pos[2].y = r_rec.rect.bottom;
			r_pos[3].x = r_rec.rect.left;
			r_pos[3].y = r_rec.rect.bottom;
			for (uint32_t i = 0, j = 3; i < 4; j = i++) {
				D2D1_POINT_2F r_mid;
				// •ûŒ`‚Ì’¸“_‚ÌƒAƒ“ƒJ[‚ğ•\¦‚·‚é.
				// •Ó‚Ì’†“_‚ğ‹‚ß, ‚»‚ÌƒAƒ“ƒJ[‚ğ•\¦‚·‚é.
				pt_avg(r_pos[j], r_pos[i], r_mid);
				anc_draw_rect(r_pos[i], target, brush);
				anc_draw_rect(r_mid, target, brush);
			}
			//if (!zero) {
				D2D1_POINT_2F c_pos;
				pt_add(r_min, rx, ry, c_pos);
				anc_draw_ellipse(c_pos, target, brush);
				c_pos.x = r_rec.rect.right - rx;
				anc_draw_ellipse(c_pos, target, brush);
				c_pos.y = r_rec.rect.bottom - ry;
				anc_draw_ellipse(c_pos, target, brush);
				c_pos.x = r_min.x + rx;
				anc_draw_ellipse(c_pos, target, brush);
			//}
		}
	}

	// ŠpŠÛ”¼Œa‚ğ“¾‚é.
	bool ShapeRRect::get_corner_radius(D2D1_POINT_2F& val) const noexcept
	{
		val = m_corner_rad;
		return true;
	}

	//	•”ˆÊ‚ÌˆÊ’u‚ğ“¾‚é.
	//	anc	}Œ`‚Ì•”ˆÊ.
	//	val	“¾‚ç‚ê‚½ˆÊ’u.
	//	–ß‚è’l	‚È‚µ
	void ShapeRRect::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		const double dx = m_vec[0].x;
		const double dy = m_vec[0].y;
		const double mx = dx * 0.5;	// ’†“_
		const double my = dy * 0.5;	// ’†“_
		const double rx = fabs(mx) < fabs(m_corner_rad.x) ? mx : m_corner_rad.x;	// ŠpŠÛ
		const double ry = fabs(my) < fabs(m_corner_rad.y) ? my : m_corner_rad.y;	// ŠpŠÛ
		switch (anc) {
		case ANC_TYPE::ANC_R_NW:
			// ¶ã‚ÌŠpŠÛ’†S“_‚ğ‹‚ß‚é
			pt_add(m_pos, rx, ry, val);
			break;
		case ANC_TYPE::ANC_R_NE:
			// ‰Eã‚ÌŠpŠÛ’†S“_‚ğ‹‚ß‚é
			pt_add(m_pos, dx - rx, ry, val);
			break;
		case ANC_TYPE::ANC_R_SE:
			// ‰E‰º‚ÌŠpŠÛ’†S“_‚ğ‹‚ß‚é
			pt_add(m_pos, dx - rx, dy - ry, val);
			break;
		case ANC_TYPE::ANC_R_SW:
			// ¶‰º‚ÌŠpŠÛ’†S“_‚ğ‹‚ß‚é
			pt_add(m_pos, rx, dy - ry, val);
			break;
		default:
			ShapeRect::get_pos_anc(anc, val);
			break;
		}
	}

	// ˆÊ’u‚ªŠpŠÛ•ûŒ`‚ÉŠÜ‚Ü‚ê‚é‚©”»’è‚·‚é.
	// t_pos	”»’è‚·‚éˆÊ’u
	// r_min	ŠpŠÛ•ûŒ`‚Ì¶ãˆÊ’u
	// r_max	ŠpŠÛ•ûŒ`‚Ì‰E‰ºˆÊ’u
	// r_rad	ŠpŠÛ‚Ì”¼Œa
	// –ß‚è’l	ŠÜ‚Ü‚ê‚é‚È‚ç true ‚ğ•Ô‚·.
	static bool pt_in_rrect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max, const D2D1_POINT_2F r_rad)
	{
		if (t_pos.x < r_min.x) {
			return false;
		}
		if (t_pos.x > r_max.x) {
			return false;
		}
		if (t_pos.y < r_min.y) {
			return false;
		}
		if (t_pos.y > r_max.y) {
			return false;
		}
		D2D1_POINT_2F c_pos;
		pt_add(r_min, r_rad, c_pos);
		if (t_pos.x < c_pos.x) {
			if (t_pos.y < c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_max.x - r_rad.x;
		c_pos.y = r_min.y + r_rad.y;
		if (t_pos.x > c_pos.x) {
			if (t_pos.y < c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_max.x - r_rad.x;
		c_pos.y = r_max.y - r_rad.y;
		if (t_pos.x > c_pos.x) {
			if (t_pos.y > c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_min.x + r_rad.x;
		c_pos.y = r_max.y - r_rad.y;
		if (t_pos.x < c_pos.x) {
			if (t_pos.y > c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		return true;
	}

	// ˆÊ’u‚ğŠÜ‚Ş‚©”»’è‚·‚é.
	// t_pos	”»’è‚·‚éˆÊ’u
	// –ß‚è’l	ˆÊ’u‚ğŠÜ‚Ş}Œ`‚Ì•”ˆÊ
	uint32_t ShapeRRect::hit_test(const D2D1_POINT_2F t_pos) const noexcept
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
		const double rx = fabs(mx) < fabs(m_corner_rad.x) ? mx : m_corner_rad.x;	// ŠpŠÛ
		const double ry = fabs(my) < fabs(m_corner_rad.y) ? my : m_corner_rad.y;	// ŠpŠÛ
		const D2D1_POINT_2F anc_r_nw{ static_cast<FLOAT>(m_pos.x + rx), static_cast<FLOAT>(m_pos.y + ry) };
		if (pt_in_anc(t_pos, anc_r_nw)) {
			anc_r = ANC_TYPE::ANC_R_NW;
		}
		else {
			const D2D1_POINT_2F anc_r_se{ static_cast<FLOAT>(m_pos.x + m_vec[0].x - rx), static_cast<FLOAT>(m_pos.y + m_vec[0].y - ry) };
			if (pt_in_anc(t_pos, anc_r_se)) {
				anc_r = ANC_TYPE::ANC_R_SE;
			}
			else {
				const D2D1_POINT_2F anc_r_ne{ anc_r_se.x, anc_r_nw.y };
				if (pt_in_anc(t_pos, anc_r_ne)) {
					anc_r = ANC_TYPE::ANC_R_NE;
				}
				else {
					const D2D1_POINT_2F anc_r_sw{ anc_r_nw.x, anc_r_se.y };
					if (pt_in_anc(t_pos, anc_r_sw)) {
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
			fabs(m_vec[0].x) > Shape::s_anc_len && fabs(m_vec[0].y) > Shape::s_anc_len) {
			return anc_r;
		}
		// •ûŒ`‚ÌŠe’¸“_‚ÉŠÜ‚Ü‚ê‚é‚©”»’è‚·‚é.
		const uint32_t anc_v = hit_test_anc(t_pos);
		if (anc_v != ANC_TYPE::ANC_PAGE) {
			return anc_v;
		}
		// ’¸“_‚ÉŠÜ‚Ü‚ê‚¸, ŠpŠÛ‚Ì‰~ŒÊ‚Ì’†S“_‚ÉŠÜ‚Ü‚ê‚é‚©”»’è‚·‚é.
		else if (anc_r != ANC_TYPE::ANC_PAGE) {
			return anc_r;
		}

		// ŠpŠÛ•ûŒ`‚ğ³‹K‰»‚·‚é.
		D2D1_POINT_2F r_min;
		D2D1_POINT_2F r_max;
		D2D1_POINT_2F r_rad;
		if (m_vec[0].x > 0.0f) {
			r_min.x = m_pos.x;
			r_max.x = m_pos.x + m_vec[0].x;
		}
		else {
			r_min.x = m_pos.x + m_vec[0].x;
			r_max.x = m_pos.x;
		}
		if (m_vec[0].y > 0.0f) {
			r_min.y = m_pos.y;
			r_max.y = m_pos.y + m_vec[0].y;
		}
		else {
			r_min.y = m_pos.y + m_vec[0].y;
			r_max.y = m_pos.y;
		}
		r_rad.x = std::abs(m_corner_rad.x);
		r_rad.y = std::abs(m_corner_rad.y);

		// ü˜g‚ª“§–¾‚Ü‚½‚Í‘¾‚³ 0 ‚©”»’è‚·‚é.
		if (!is_opaque(m_stroke_color) || m_stroke_width < FLT_MIN) {
			// “h‚è‚Â‚Ô‚µF‚ª•s“§–¾, ‚©‚ÂŠpŠÛ•ûŒ`‚»‚Ì‚à‚Ì‚ÉŠÜ‚Ü‚ê‚é‚©”»’è‚·‚é.
			if (is_opaque(m_fill_color) && pt_in_rrect(t_pos, r_min, r_max, r_rad)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		// ü˜g‚ÌF‚ª•s“§–¾, ‚©‚Â‘¾‚³‚ª 0 ‚æ‚è‘å‚«‚¢.
		else {
			// ˆÈ‰º‚Ìè‡‚Í, k¬‚µ‚½ŠpŠÛ•ûŒ`‚ÌŠO‘¤‚É, ŠpŠÛŒğ“_‚ª‚ ‚éƒP[ƒX‚Å‚¤‚Ü‚­‚¢‚©‚È‚¢.
			// Šg‘å‚µ‚½ŠpŠÛ•ûŒ`‚ÉŠÜ‚Ü‚ê‚é‚©”»’è
			const double s_thick = max(m_stroke_width, Shape::s_anc_len);
			D2D1_POINT_2F s_min, s_max, s_rad;
			pt_add(r_min, -s_thick * 0.5, s_min);
			pt_add(r_max, s_thick * 0.5, s_max);
			pt_add(r_rad, s_thick * 0.5, s_rad);
			if (pt_in_rrect(t_pos, s_min, s_max, s_rad)) {
				// k¬‚µ‚½ŠpŠÛ•ûŒ`‚ª‹t“]‚µ‚Ä‚È‚¢, ‚©‚ÂˆÊ’u‚ªk¬‚µ‚½ŠpŠÛ•ûŒ`‚ÉŠÜ‚Ü‚ê‚é‚©”»’è‚·‚é.
				D2D1_POINT_2F u_min, u_max, u_rad;
				pt_add(s_min, s_thick, u_min);
				pt_add(s_max, -s_thick, u_max);
				pt_add(s_rad, -s_thick, u_rad);
				if (u_min.x < u_max.x && u_min.y < u_max.y && pt_in_rrect(t_pos, u_min, u_max, u_rad)) {
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
		D2D1_POINT_2F c_pos;
		D2D1_POINT_2F vec;
		D2D1_POINT_2F rad;
		D2D1_POINT_2F new_pos;

		switch (anc) {
		case ANC_TYPE::ANC_R_NW:
			ShapeRRect::get_pos_anc(anc, c_pos);
			pt_round(val, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			pt_add(m_corner_rad, vec, rad);
			calc_corner_radius(m_vec[0], rad, m_corner_rad);
			break;
		case ANC_TYPE::ANC_R_NE:
			ShapeRRect::get_pos_anc(anc, c_pos);
			pt_round(val, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x - vec.x;
			rad.y = m_corner_rad.y + vec.y;
			calc_corner_radius(m_vec[0], rad, m_corner_rad);
			break;
		case ANC_TYPE::ANC_R_SE:
			ShapeRRect::get_pos_anc(anc, c_pos);
			pt_round(val, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x - vec.x;
			rad.y = m_corner_rad.y - vec.y;
			calc_corner_radius(m_vec[0], rad, m_corner_rad);
			break;
		case ANC_TYPE::ANC_R_SW:
			ShapeRRect::get_pos_anc(anc, c_pos);
			pt_round(val, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x + vec.x;
			rad.y = m_corner_rad.y - vec.y;
			calc_corner_radius(m_vec[0], rad, m_corner_rad);
			break;
		default:
			if (!ShapeRect::set_pos_anc(val, anc, limit, false)) {
				return false;
			}
			if (m_vec[0].x * m_corner_rad.x < 0.0f) {
				m_corner_rad.x = -m_corner_rad.x;
			}
			if (m_vec[0].y * m_corner_rad.y < 0.0f) {
				m_corner_rad.y = -m_corner_rad.y;
			}
			break;
		}
		const double d = static_cast<double>(limit);
		if (pt_abs2(m_corner_rad) < d * d) {
			m_corner_rad.x = m_corner_rad.y = 0.0f;
		}
		return true;
	}

	// }Œ`‚ğì¬‚·‚é.
	// b_pos	ˆÍ‚Ş—Ìˆæ‚Ìn“_
	// b_vec	ˆÍ‚Ş—Ìˆæ‚ÌI“_‚Ö‚Ì·•ª
	// page	‘®«
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapePage* page) :
		ShapeRect::ShapeRect(b_pos, b_vec, page)
	{
		calc_corner_radius(m_vec[0], page->m_corner_rad, m_corner_rad);
	}

	// }Œ`‚ğƒf[ƒ^ƒŠ[ƒ_[‚©‚ç“Ç‚İ‚Ş.
	ShapeRRect::ShapeRRect(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		dt_read(m_corner_rad, dt_reader);
	}

	// }Œ`‚ğƒf[ƒ^ƒ‰ƒCƒ^[‚É‘‚«‚Ş.
	void ShapeRRect::write(DataWriter const& dt_writer) const
	{
		ShapeRect::write(dt_writer);
		dt_write(m_corner_rad, dt_writer);
	}

}