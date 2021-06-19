//------------------------------
// shape_rrect.cpp
// äpä€ï˚å`ê}å`
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// äpä€ï˚å`ÇÃíÜì_ÇÃîzóÒ
	constexpr uint32_t ANCH_ROUND[4]{
		ANCH_TYPE::ANCH_R_SE,	// âEâ∫äp
		ANCH_TYPE::ANCH_R_NE,	// âEè„äp
		ANCH_TYPE::ANCH_R_SW,	// ç∂â∫äp
		ANCH_TYPE::ANCH_R_NW	// ç∂è„äp
	};

	// äpä€îºåaÇåvéZÇ∑ÇÈ.
	static void calc_corner_radius(const D2D1_POINT_2F d_vec, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad);

	// äpä€îºåaÇÃècÇ‹ÇΩÇÕâ°ÇÃê¨ï™ÇåvéZÇ∑ÇÈ.
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad);

	// äpä€îºåaÇåvéZÇ∑ÇÈ.
	// d_vec	äpä€ï˚å`ÇÃëŒäpÉxÉNÉgÉã
	// d_rad	ä˘íËÇÃäpä€îºåa
	// c_rad	åvéZÇ≥ÇÍÇΩäpä€îºåa
	static void calc_corner_radius(const D2D1_POINT_2F d_vec, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad)
	{
		calc_corner_radius(d_vec.x, d_rad.x, c_rad.x);
		calc_corner_radius(d_vec.y, d_rad.y, c_rad.y);
	}

	// äpä€îºåaÇÃècÇ‹ÇΩÇÕâ°ÇÃê¨ï™ÇåvéZÇ∑ÇÈ.
	// r_len	äpä€ï˚å`ÇÃàÍï”ÇÃëÂÇ´Ç≥
	// d_rad	Ç‡Ç∆ÇÃäpä€îºåa
	// c_rad	ìæÇÁÇÍÇΩäpä€îºåa
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad)
	{
		const double r = r_len * 0.5;
		// Ç‡Ç∆ÇÃäpä€îºåaÇ™ï˚å`ÇÃëÂÇ´Ç≥ÇÃîºï™Çí¥Ç¶Ç»Ç¢ÇÊÇ§Ç…Ç∑ÇÈ.
		if (fabs(d_rad) > fabs(r)) {
			c_rad = static_cast<FLOAT>(r);
		}
		else if (r_len * d_rad < 0.0f) {
			// äpä€ï˚å`ÇÃëÂÇ´Ç≥Ç∆Ç‡Ç∆ÇÃäpä€îºåaÇÃïÑçÜÇ™ãtÇ»ÇÁ,
			// Ç‡Ç∆ÇÃäpä€îºåaÇÃïÑçÜÇãtÇ…ÇµÇΩílÇ
			// ìæÇÁÇÍÇΩäpä€îºåaÇ…äiî[Ç∑ÇÈ.
			c_rad = -d_rad;
		}
		else {
			c_rad = d_rad;
		}
	}

	// ê}å`Çï\é¶Ç∑ÇÈ.
	void ShapeRRect::draw(SHAPE_DX& dx)
	{
		auto sb = dx.m_shape_brush.get();
		auto ss = m_d2d_stroke_style.get();
		auto sw = m_stroke_width;
		auto dc = dx.m_d2dContext;

		D2D1_POINT_2F r_min;
		pt_add(m_pos, min(m_diff[0].x, 0.0), min(m_diff[0].y, 0.0), r_min);
		float rx = std::fabsf(m_corner_rad.x);
		float ry = std::fabsf(m_corner_rad.y);
		float vx = std::fabsf(m_diff[0].x);
		float vy = std::fabsf(m_diff[0].y);
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
			const auto flag = (std::abs(m_diff[0].x) >= FLT_MIN && std::abs(m_diff[0].y) >= FLT_MIN);
			//if (flag) {
			// D2D1_POINT_2F c_pos;
			// pt_add(r_min, rx, ry, c_pos);
			// anch_draw_ellipse(c_pos, dx);
			// c_pos.x = r_rec.rect.right - rx;
			// anch_draw_ellipse(c_pos, dx);
			// c_pos.y = r_rec.rect.bottom - ry;
			// anch_draw_ellipse(c_pos, dx);
			// c_pos.x = r_min.x + rx;
			// anch_draw_ellipse(c_pos, dx);
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
				// ï˚å`ÇÃí∏ì_ÇÃÉAÉìÉJÅ[Çï\é¶Ç∑ÇÈ.
				// ï”ÇÃíÜì_ÇãÅÇﬂ, ÇªÇÃÉAÉìÉJÅ[Çï\é¶Ç∑ÇÈ.
				pt_avg(r_pos[j], r_pos[i], r_mid);
				anch_draw_rect(r_pos[i], dx);
				anch_draw_rect(r_mid, dx);
			}
			//if (flag != true) {
				D2D1_POINT_2F c_pos;
				pt_add(r_min, rx, ry, c_pos);
				anch_draw_ellipse(c_pos, dx);
				c_pos.x = r_rec.rect.right - rx;
				anch_draw_ellipse(c_pos, dx);
				c_pos.y = r_rec.rect.bottom - ry;
				anch_draw_ellipse(c_pos, dx);
				c_pos.x = r_min.x + rx;
				anch_draw_ellipse(c_pos, dx);
			//}
		}
	}

	// äpä€îºåaÇìæÇÈ.
	bool ShapeRRect::get_corner_radius(D2D1_POINT_2F& value) const noexcept
	{
		value = m_corner_rad;
		return true;
	}

	//	ïîà ÇÃà íuÇìæÇÈ.
	//	anch	ê}å`ÇÃïîà .
	//	value	ìæÇÁÇÍÇΩà íu.
	//	ñﬂÇËíl	Ç»Çµ
	void ShapeRRect::get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept
	{
		const double dx = m_diff[0].x;
		const double dy = m_diff[0].y;
		const double mx = dx * 0.5;	// íÜì_
		const double my = dy * 0.5;	// íÜì_
		const double rx = fabs(mx) < fabs(m_corner_rad.x) ? mx : m_corner_rad.x;	// äpä€
		const double ry = fabs(my) < fabs(m_corner_rad.y) ? my : m_corner_rad.y;	// äpä€
		switch (anch) {
		case ANCH_TYPE::ANCH_R_NW:
			// ç∂è„ÇÃäpä€íÜêSì_ÇãÅÇﬂÇÈ
			pt_add(m_pos, rx, ry, value);
			break;
		case ANCH_TYPE::ANCH_R_NE:
			// âEè„ÇÃäpä€íÜêSì_ÇãÅÇﬂÇÈ
			pt_add(m_pos, dx - rx, ry, value);
			break;
		case ANCH_TYPE::ANCH_R_SE:
			// âEâ∫ÇÃäpä€íÜêSì_ÇãÅÇﬂÇÈ
			pt_add(m_pos, dx - rx, dy - ry, value);
			break;
		case ANCH_TYPE::ANCH_R_SW:
			// ç∂â∫ÇÃäpä€íÜêSì_ÇãÅÇﬂÇÈ
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

	// à íuÇä‹ÇﬁÇ©îªíËÇ∑ÇÈ.
	// t_pos	îªíËÇ∑ÇÈà íu
	// ñﬂÇËíl	à íuÇä‹Çﬁê}å`ÇÃïîà 
	uint32_t ShapeRRect::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		const auto flag = (fabs(m_diff[0].x) >= FLT_MIN && fabs(m_diff[0].y) >= FLT_MIN);
		if (flag) {
			for (uint32_t i = 0; i < 4; i++) {
				// äpä€ÇÃíÜêSì_ÇìæÇÈ.
				D2D1_POINT_2F r_cen;
				get_anch_pos(ANCH_ROUND[i], r_cen);
				// äpä€ÇÃïîà Ç…à íuÇ™ä‹Ç‹ÇÍÇÈÇ©îªíËÇ∑ÇÈ.
				if (pt_in_anch(t_pos, r_cen)) {
					// ä‹Ç‹ÇÍÇÈÇ»ÇÁäpä€ÇÃïîà Çï‘Ç∑.
					return ANCH_ROUND[i];
				}
			}
		}
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F r_pos;	// ï˚å`ÇÃí∏ì_
			get_anch_pos(ANCH_CORNER[i], r_pos);
			if (pt_in_anch(t_pos, r_pos)) {
				return ANCH_CORNER[i];
			}
		}
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F r_pos;	// ï˚å`ÇÃï”ÇÃíÜì_
			get_anch_pos(ANCH_MIDDLE[i], r_pos);
			if (pt_in_anch(t_pos, r_pos)) {
				return ANCH_MIDDLE[i];
			}
		}
		if (flag != true) {
			for (uint32_t i = 0; i < 4; i++) {
				D2D1_POINT_2F r_cen;	// äpä€ïîï™ÇÃíÜêSì_
				get_anch_pos(ANCH_ROUND[i], r_cen);
				if (pt_in_anch(t_pos, r_cen)) {
					return ANCH_ROUND[i];
				}
			}
		}
		D2D1_POINT_2F r_min;
		D2D1_POINT_2F r_max;
		D2D1_POINT_2F r_rad;
		if (m_diff[0].x > 0.0f) {
			r_min.x = m_pos.x;
			r_max.x = m_pos.x + m_diff[0].x;
		}
		else {
			r_min.x = m_pos.x + m_diff[0].x;
			r_max.x = m_pos.x;
		}
		if (m_diff[0].y > 0.0f) {
			r_min.y = m_pos.y;
			r_max.y = m_pos.y + m_diff[0].y;
		}
		else {
			r_min.y = m_pos.y + m_diff[0].y;
			r_max.y = m_pos.y;
		}
		r_rad.x = std::abs(m_corner_rad.x);
		r_rad.y = std::abs(m_corner_rad.y);
		if (is_opaque(m_stroke_color) != true) {
			if (is_opaque(m_fill_color) && pt_in_rrect(t_pos, r_min, r_max, r_rad)) {
				return ANCH_TYPE::ANCH_FILL;
			}
			return ANCH_TYPE::ANCH_SHEET;
		}
		const double s_width = max(static_cast<double>(m_stroke_width), Shape::s_anch_len);
		// äOë§ÇÃäpä€ï˚å`ÇÃîªíË
		pt_add(r_min, -s_width * 0.5, r_min);
		pt_add(r_max, s_width * 0.5, r_max);
		pt_add(r_rad, s_width * 0.5, r_rad);
		if (pt_in_rrect(t_pos, r_min, r_max, r_rad) != true) {
			return ANCH_TYPE::ANCH_SHEET;
		}
		// ì‡ë§ÇÃäpä€ï˚å`ÇÃîªíË
		pt_add(r_min, s_width, r_min);
		pt_add(r_max, -s_width, r_max);
		pt_add(r_rad, -s_width, r_rad);
		if (pt_in_rrect(t_pos, r_min, r_max, r_rad) != true) {
			return ANCH_TYPE::ANCH_STROKE;
		}
		if (is_opaque(m_fill_color)) {
			return ANCH_TYPE::ANCH_FILL;
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	//	ílÇ, ïîà ÇÃà íuÇ…äiî[Ç∑ÇÈ. ëºÇÃïîà ÇÃà íuÇ‡ìÆÇ≠.
	//	value	äiî[Ç∑ÇÈíl
	//	abch	ê}å`ÇÃïîà 
	bool ShapeRRect::set_anch_pos(const D2D1_POINT_2F value, const uint32_t anch, const float dist)
	{
		D2D1_POINT_2F c_pos;
		D2D1_POINT_2F vec;
		D2D1_POINT_2F rad;

		switch (anch) {
		case ANCH_TYPE::ANCH_R_NW:
			ShapeRRect::get_anch_pos(anch, c_pos);
			pt_sub(value, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			pt_add(m_corner_rad, vec, rad);
			calc_corner_radius(m_diff[0], rad, m_corner_rad);
			break;
		case ANCH_TYPE::ANCH_R_NE:
			ShapeRRect::get_anch_pos(anch, c_pos);
			pt_sub(value, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x - vec.x;
			rad.y = m_corner_rad.y + vec.y;
			calc_corner_radius(m_diff[0], rad, m_corner_rad);
			break;
		case ANCH_TYPE::ANCH_R_SE:
			ShapeRRect::get_anch_pos(anch, c_pos);
			pt_sub(value, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x - vec.x;
			rad.y = m_corner_rad.y - vec.y;
			calc_corner_radius(m_diff[0], rad, m_corner_rad);
			break;
		case ANCH_TYPE::ANCH_R_SW:
			ShapeRRect::get_anch_pos(anch, c_pos);
			pt_sub(value, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x + vec.x;
			rad.y = m_corner_rad.y - vec.y;
			calc_corner_radius(m_diff[0], rad, m_corner_rad);
			break;
		default:
			ShapeRect::set_anch_pos(value, anch, dist);
			if (m_diff[0].x * m_corner_rad.x < 0.0f) {
				m_corner_rad.x = -m_corner_rad.x;
			}
			if (m_diff[0].y * m_corner_rad.y < 0.0f) {
				m_corner_rad.y = -m_corner_rad.y;
			}
			break;
		}
		if (pt_abs2(m_corner_rad) < dist * dist) {
			m_corner_rad.x = m_corner_rad.y = 0.0f;
		}
		return true;
	}

	// ê}å`ÇçÏê¨Ç∑ÇÈ.
	// b_pos	àÕÇﬁóÃàÊÇÃénì_
	// b_vec	àÕÇﬁóÃàÊÇÃèIì_Ç÷ÇÃç∑ï™
	// s_attr	ëÆê´
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr) :
		ShapeRect::ShapeRect(b_pos, b_vec, s_attr)
	{
		calc_corner_radius(m_diff[0], s_attr->m_corner_rad, m_corner_rad);
	}

	// ÉfÅ[É^ÉäÅ[É_Å[Ç©ÇÁê}å`Çì«Ç›çûÇﬁ.
	ShapeRRect::ShapeRRect(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		dt_read(m_corner_rad, dt_reader);
	}

	// ÉfÅ[É^ÉâÉCÉ^Å[Ç…èëÇ´çûÇﬁ.
	void ShapeRRect::write(DataWriter const& dt_writer) const
	{
		ShapeRect::write(dt_writer);
		dt_write(m_corner_rad, dt_writer);
	}

	// ÉfÅ[É^ÉâÉCÉ^Å[Ç… SVG É^ÉOÇ∆ÇµÇƒèëÇ´çûÇﬁ.
	void ShapeRRect::dt_write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::dt_write_svg;

		dt_write_svg("<rect ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_diff[0], "width", "height", dt_writer);
		if (std::round(m_corner_rad.x) != 0.0f && std::round(m_corner_rad.y) != 0.0f) {
			dt_write_svg(m_corner_rad, "rx", "ry", dt_writer);
		}
		dt_write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::dt_write_svg(dt_writer);
		dt_write_svg("/>", dt_writer);
	}
}