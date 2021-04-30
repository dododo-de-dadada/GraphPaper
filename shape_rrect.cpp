//------------------------------
// shape_rrect.cpp
// ŠpŠÛ•ûŒ`}Œ`
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ŠpŠÛ•ûŒ`‚Ì’†“_‚Ì”z—ñ
	constexpr ANCH_WHICH ANCH_ROUND[4]{
		ANCH_WHICH::ANCH_R_SE,	// ‰E‰ºŠp
		ANCH_WHICH::ANCH_R_NE,	// ‰EãŠp
		ANCH_WHICH::ANCH_R_SW,	// ¶‰ºŠp
		ANCH_WHICH::ANCH_R_NW	// ¶ãŠp
	};

	// ŠpŠÛ”¼Œa‚ğŒvZ‚·‚é.
	static void calc_corner_radius(const D2D1_POINT_2F diff, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad);
	// ŠpŠÛ”¼Œa‚Ìc‚Ü‚½‚Í‰¡‚Ì¬•ª‚ğŒvZ‚·‚é.
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad);

	// ŠpŠÛ”¼Œa‚ğŒvZ‚·‚é.
	// diff	ŠpŠÛ•ûŒ`‚Ì‘ÎŠpƒxƒNƒgƒ‹
	// d_rad	Šù’è‚ÌŠpŠÛ”¼Œa
	// c_rad	ŒvZ‚³‚ê‚½ŠpŠÛ”¼Œa
	static void calc_corner_radius(const D2D1_POINT_2F diff, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad)
	{
		calc_corner_radius(diff.x, d_rad.x, c_rad.x);
		calc_corner_radius(diff.y, d_rad.y, c_rad.y);
	}

	// ŠpŠÛ”¼Œa‚Ìc‚Ü‚½‚Í‰¡‚Ì¬•ª‚ğŒvZ‚·‚é.
	// r_len	ŠpŠÛ•ûŒ`‚Ìˆê•Ó‚Ì‘å‚«‚³
	// d_rad	‚à‚Æ‚ÌŠpŠÛ”¼Œa
	// c_rad	“¾‚ç‚ê‚½ŠpŠÛ”¼Œa
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad)
	{
		const double r = r_len * 0.5;
		if (fabs(d_rad) > fabs(r)) {
			// ‚à‚Æ‚ÌŠpŠÛ”¼Œa‚ª•ûŒ`‚Ì‘å‚«‚³‚Ì”¼•ª‚ğ’´‚¦‚é‚È‚ç,
			// ‘å‚«‚³‚Ì”¼•ª‚ğ“¾‚ç‚ê‚½ŠpŠÛ”¼Œa‚ÉŠi”[‚·‚é.
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
	void ShapeRRect::draw(SHAPE_DX& dx)
	{
		auto sb = dx.m_shape_brush.get();
		auto ss = m_d2d_stroke_style.get();
		auto sw = static_cast<FLOAT>(m_stroke_width);
		auto dc = dx.m_d2dContext;

		D2D1_POINT_2F r_min;
		pt_add(m_pos, min(m_diff.x, 0.0), min(m_diff.y, 0.0), r_min);
		float rx = std::fabsf(m_corner_rad.x);
		float ry = std::fabsf(m_corner_rad.y);
		float vx = std::fabsf(m_diff.x);
		float vy = std::fabsf(m_diff.y);
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
			sb->SetColor(m_fill_color);
			dc->FillRoundedRectangle(r_rec, sb);
		}
		sb->SetColor(m_stroke_color);
		dc->DrawRoundedRectangle(r_rec, sb, sw, ss);
		if (is_selected()) {
			const auto flag = (std::abs(m_diff.x) > FLT_MIN && std::abs(m_diff.y) > FLT_MIN);
			//if (flag) {
			// D2D1_POINT_2F c_pos;
			// pt_add(r_min, rx, ry, c_pos);
			// anchor_draw_ellipse(c_pos, dx);
			// c_pos.x = r_rec.rect.right - rx;
			// anchor_draw_ellipse(c_pos, dx);
			// c_pos.y = r_rec.rect.bottom - ry;
			// anchor_draw_ellipse(c_pos, dx);
			// c_pos.x = r_min.x + rx;
			// anchor_draw_ellipse(c_pos, dx);
			//}
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
				anchor_draw_rect(r_pos[i], dx);
				anchor_draw_rect(r_mid, dx);
			}
			//if (flag != true) {
				D2D1_POINT_2F c_pos;
				pt_add(r_min, rx, ry, c_pos);
				anchor_draw_ellipse(c_pos, dx);
				c_pos.x = r_rec.rect.right - rx;
				anchor_draw_ellipse(c_pos, dx);
				c_pos.y = r_rec.rect.bottom - ry;
				anchor_draw_ellipse(c_pos, dx);
				c_pos.x = r_min.x + rx;
				anchor_draw_ellipse(c_pos, dx);
			//}
		}
	}

	// ŠpŠÛ”¼Œa‚ğ“¾‚é.
	bool ShapeRRect::get_corner_radius(D2D1_POINT_2F& value) const noexcept
	{
		value = m_corner_rad;
		return true;
	}

	//	•”ˆÊ‚ÌˆÊ’u‚ğ“¾‚é.
	//	anch	}Œ`‚Ì•”ˆÊ.
	//	value	“¾‚ç‚ê‚½ˆÊ’u.
	//	–ß‚è’l	‚È‚µ
	void ShapeRRect::get_anch_pos(const ANCH_WHICH anch, D2D1_POINT_2F& value) const noexcept
	{
		const double dx = m_diff.x;
		const double dy = m_diff.y;
		const double mx = dx * 0.5;	// ’†“_
		const double my = dy * 0.5;	// ’†“_
		const double rx = fabs(mx) < fabs(m_corner_rad.x) ? mx : m_corner_rad.x;	// ŠpŠÛ
		const double ry = fabs(my) < fabs(m_corner_rad.y) ? my : m_corner_rad.y;	// ŠpŠÛ
		switch (anch) {
		case ANCH_WHICH::ANCH_R_NW:
			// ¶ã‚ÌŠpŠÛ’†S“_‚ğ‹‚ß‚é
			pt_add(m_pos, rx, ry, value);
			break;
		case ANCH_WHICH::ANCH_R_NE:
			// ‰Eã‚ÌŠpŠÛ’†S“_‚ğ‹‚ß‚é
			pt_add(m_pos, dx - rx, ry, value);
			break;
		case ANCH_WHICH::ANCH_R_SE:
			// ‰E‰º‚ÌŠpŠÛ’†S“_‚ğ‹‚ß‚é
			pt_add(m_pos, dx - rx, dy - ry, value);
			break;
		case ANCH_WHICH::ANCH_R_SW:
			// ¶‰º‚ÌŠpŠÛ’†S“_‚ğ‹‚ß‚é
			pt_add(m_pos, rx, dy - ry, value);
			break;
		default:
			ShapeRect::get_anch_pos(anch, value);
			break;
		}
	}

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
				return pt_in_elli(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_max.x - r_rad.x;
		c_pos.y = r_min.y + r_rad.y;
		if (t_pos.x > c_pos.x) {
			if (t_pos.y < c_pos.y) {
				return pt_in_elli(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_max.x - r_rad.x;
		c_pos.y = r_max.y - r_rad.y;
		if (t_pos.x > c_pos.x) {
			if (t_pos.y > c_pos.y) {
				return pt_in_elli(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_min.x + r_rad.x;
		c_pos.y = r_max.y - r_rad.y;
		if (t_pos.x < c_pos.x) {
			if (t_pos.y > c_pos.y) {
				return pt_in_elli(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		return true;
	}

	// ˆÊ’u‚ğŠÜ‚Ş‚©’²‚×‚é.
	// t_pos	’²‚×‚éˆÊ’u
	// a_len	•”ˆÊ‚Ì‘å‚«‚³
	// –ß‚è’l	ˆÊ’u‚ğŠÜ‚Ş}Œ`‚Ì•”ˆÊ
	ANCH_WHICH ShapeRRect::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const auto flag = (fabs(m_diff.x) > FLT_MIN&& fabs(m_diff.y) > FLT_MIN);
		if (flag) {
			for (uint32_t i = 0; i < 4; i++) {
				// ŠpŠÛ‚Ì’†S“_‚ğ“¾‚é.
				D2D1_POINT_2F r_cen;
				get_anch_pos(ANCH_ROUND[i], r_cen);
				// ˆÊ’u‚ªŠpŠÛ‚Ì•”ˆÊ‚ÉŠÜ‚Ü‚ê‚é‚©’²‚×‚é.
				if (pt_in_anch(t_pos, r_cen, a_len)) {
					// ŠÜ‚Ü‚ê‚é‚È‚çŠpŠÛ‚Ì•”ˆÊ‚ğ•Ô‚·.
					return ANCH_ROUND[i];
				}
			}
		}
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F r_pos;	// •ûŒ`‚Ì’¸“_
			get_anch_pos(ANCH_CORNER[i], r_pos);
			if (pt_in_anch(t_pos, r_pos, a_len)) {
				return ANCH_CORNER[i];
			}
		}
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F r_pos;	// •ûŒ`‚Ì•Ó‚Ì’†“_
			get_anch_pos(ANCH_MIDDLE[i], r_pos);
			if (pt_in_anch(t_pos, r_pos, a_len)) {
				return ANCH_MIDDLE[i];
			}
		}
		if (flag != true) {
			for (uint32_t i = 0; i < 4; i++) {
				D2D1_POINT_2F r_cen;	// ŠpŠÛ•”•ª‚Ì’†S“_
				get_anch_pos(ANCH_ROUND[i], r_cen);
				if (pt_in_anch(t_pos, r_cen, a_len)) {
					return ANCH_ROUND[i];
				}
			}
		}
		D2D1_POINT_2F r_min;
		D2D1_POINT_2F r_max;
		D2D1_POINT_2F r_rad;
		if (m_diff.x > 0.0f) {
			r_min.x = m_pos.x;
			r_max.x = m_pos.x + m_diff.x;
		}
		else {
			r_min.x = m_pos.x + m_diff.x;
			r_max.x = m_pos.x;
		}
		if (m_diff.y > 0.0f) {
			r_min.y = m_pos.y;
			r_max.y = m_pos.y + m_diff.y;
		}
		else {
			r_min.y = m_pos.y + m_diff.y;
			r_max.y = m_pos.y;
		}
		r_rad.x = std::abs(m_corner_rad.x);
		r_rad.y = std::abs(m_corner_rad.y);
		if (is_opaque(m_stroke_color) != true) {
			return is_opaque(m_fill_color) && pt_in_rrect(t_pos, r_min, r_max, r_rad) ? ANCH_WHICH::ANCH_INSIDE : ANCH_WHICH::ANCH_OUTSIDE;
		}
		const double s_width = max(m_stroke_width, a_len);
		// ŠO‘¤‚ÌŠpŠÛ•ûŒ`‚Ì”»’è
		pt_add(r_min, -s_width * 0.5, r_min);
		pt_add(r_max, s_width * 0.5, r_max);
		pt_add(r_rad, s_width * 0.5, r_rad);
		if (pt_in_rrect(t_pos, r_min, r_max, r_rad) != true) {
			return ANCH_WHICH::ANCH_OUTSIDE;
		}
		// “à‘¤‚ÌŠpŠÛ•ûŒ`‚Ì”»’è
		pt_add(r_min, s_width, r_min);
		pt_add(r_max, -s_width, r_max);
		pt_add(r_rad, -s_width, r_rad);
		if (pt_in_rrect(t_pos, r_min, r_max, r_rad) != true) {
			return ANCH_WHICH::ANCH_FRAME;
		}
		return is_opaque(m_fill_color) ? ANCH_WHICH::ANCH_INSIDE : ANCH_WHICH::ANCH_OUTSIDE;
	}

	//	’l‚ğ, •”ˆÊ‚ÌˆÊ’u‚ÉŠi”[‚·‚é. ‘¼‚Ì•”ˆÊ‚ÌˆÊ’u‚Í“®‚©‚È‚¢. 
	//	value	Ši”[‚·‚é’l
	//	abch	}Œ`‚Ì•”ˆÊ
	void ShapeRRect::set_anch_pos(const D2D1_POINT_2F value, const ANCH_WHICH anch)
	{
		D2D1_POINT_2F c_pos;
		D2D1_POINT_2F diff;
		D2D1_POINT_2F rad;

		switch (anch) {
		case ANCH_WHICH::ANCH_R_NW:
			ShapeRRect::get_anch_pos(anch, c_pos);
			pt_sub(value, c_pos, diff);
			pt_add(m_corner_rad, diff, rad);
			calc_corner_radius(m_diff, rad, m_corner_rad);
			break;
		case ANCH_WHICH::ANCH_R_NE:
			ShapeRRect::get_anch_pos(anch, c_pos);
			pt_sub(value, c_pos, diff);
			rad.x = m_corner_rad.x - diff.x;
			rad.y = m_corner_rad.y + diff.y;
			calc_corner_radius(m_diff, rad, m_corner_rad);
			break;
		case ANCH_WHICH::ANCH_R_SE:
			ShapeRRect::get_anch_pos(anch, c_pos);
			pt_sub(value, c_pos, diff);
			rad.x = m_corner_rad.x - diff.x;
			rad.y = m_corner_rad.y - diff.y;
			calc_corner_radius(m_diff, rad, m_corner_rad);
			break;
		case ANCH_WHICH::ANCH_R_SW:
			ShapeRRect::get_anch_pos(anch, c_pos);
			pt_sub(value, c_pos, diff);
			rad.x = m_corner_rad.x + diff.x;
			rad.y = m_corner_rad.y - diff.y;
			calc_corner_radius(m_diff, rad, m_corner_rad);
			break;
		default:
			ShapeRect::set_anch_pos(value, anch);
			if (m_diff.x * m_corner_rad.x < 0.0f) {
				m_corner_rad.x = -m_corner_rad.x;
			}
			if (m_diff.y * m_corner_rad.y < 0.0f) {
				m_corner_rad.y = -m_corner_rad.y;
			}
			break;
		}
	}

	// }Œ`‚ğì¬‚·‚é.
	// s_pos	ŠJnˆÊ’u
	// diff	ŠJnˆÊ’u‚©‚ç‚Ì·•ª
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, const ShapeSheet* attr) :
		ShapeRect::ShapeRect(s_pos, diff, attr)
	{
		calc_corner_radius(m_diff, attr->m_corner_rad, m_corner_rad);
	}

	// }Œ`‚ğƒf[ƒ^ƒŠ[ƒ_[‚©‚ç“Ç‚İ‚Ş.
	ShapeRRect::ShapeRRect(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_corner_rad, dt_reader);
	}

	// ƒf[ƒ^ƒ‰ƒCƒ^[‚É‘‚«‚Ş.
	void ShapeRRect::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeRect::write(dt_writer);
		write(m_corner_rad, dt_writer);
	}

	// ƒf[ƒ^ƒ‰ƒCƒ^[‚É SVG ƒ^ƒO‚Æ‚µ‚Ä‘‚«‚Ş.
	void ShapeRRect::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<rect ", dt_writer);
		write_svg(m_pos, "x", "y", dt_writer);
		write_svg(m_diff, "width", "height", dt_writer);
		if (std::round(m_corner_rad.x) != 0.0f && std::round(m_corner_rad.y) != 0.0f) {
			write_svg(m_corner_rad, "rx", "ry", dt_writer);
		}
		write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>", dt_writer);
	}
}