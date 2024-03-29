//------------------------------
// shape_rrect.cpp
// 角丸方形図形
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 角丸半径を計算する.
	static void rrect_corner_radius(const D2D1_POINT_2F end_to, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad);

	// 角丸半径の縦または横の成分を計算する.
	static void rrect_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad);

	// 角丸半径を計算する.
	// end_to	対角点への位置ベクトル
	// d_rad	既定の角丸半径
	// c_rad	計算された角丸半径
	static void rrect_corner_radius(const D2D1_POINT_2F end_to, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad)
	{
		rrect_corner_radius(end_to.x, d_rad.x, c_rad.x);
		rrect_corner_radius(end_to.y, d_rad.y, c_rad.y);
	}

	// 角丸半径を
	// r_len	角丸方形の一辺の大きさ
	// d_rad	もとの角丸半径
	// c_rad	得られた角丸半径
	static void rrect_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad)
	{
		const double r = r_len * 0.5;
		// もとの角丸半径が方形の大きさの半分を超えないようにする.
		if (fabs(d_rad) > fabs(r)) {
			c_rad = static_cast<FLOAT>(r);
		}
		else if (r_len * d_rad < 0.0f) {
			// 角丸方形の大きさともとの角丸半径の符号が逆なら,
			// もとの角丸半径の符号を逆にした値を
			// 得られた角丸半径に格納する.
			c_rad = -d_rad;
		}
		else {
			c_rad = d_rad;
		}
	}

	// 図形を表示する.
	void ShapeRRect::draw(void) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		if (m_d2d_stroke_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			create_stroke_style(factory);
		}

		D2D1_POINT_2F r_lt;
		pt_add(m_start, min(m_lineto.x, 0.0), min(m_lineto.y, 0.0), r_lt);
		float rx = std::fabsf(m_corner_radius.x);
		float ry = std::fabsf(m_corner_radius.y);
		float tx = std::fabsf(m_lineto.x);
		float ty = std::fabsf(m_lineto.y);
		if (rx > tx * 0.5f) {
			rx = tx * 0.5f;
		}
		if (ry > ty * 0.5f) {
			ry = ty * 0.5f;
		}
		const D2D1_ROUNDED_RECT r_rec{
			{ r_lt.x, r_lt.y, r_lt.x + tx,  r_lt.y + ty },
			rx, ry
		};
		/*
		r_rec.rect.left = r_lt.x;
		r_rec.rect.top = r_lt.y;
		r_rec.rect.right = r_lt.x + vx;
		r_rec.rect.bottom = r_lt.y + vy;
		r_rec.radiusX = rx;
		r_rec.radiusY = ry;
		*/
		if (is_opaque(m_fill_color)) {
			brush->SetColor(m_fill_color);
			target->FillRoundedRectangle(r_rec, brush);
		}
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			brush->SetColor(m_stroke_color);
			target->DrawRoundedRectangle(r_rec, brush, m_stroke_width, m_d2d_stroke_style.get());
		}

		if (m_loc_show && is_selected()) {
			// 補助線を描く
			if (m_stroke_width >= Shape::m_loc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawRoundedRectangle(r_rec, brush, 2.0f * Shape::m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawRoundedRectangle(r_rec, brush, Shape::m_aux_width, m_aux_style.get());
			}
			// 角丸の中心点を描く.
			D2D1_POINT_2F circle[4]{	// 円上の点
				{ r_lt.x + rx, r_lt.y + ry },
				{ r_lt.x + tx - rx, r_lt.y + ry },
				{ r_lt.x + tx - rx, r_lt.y + ty - ry },
				{ r_lt.x + rx, r_lt.y + ty - ry }
			};
			loc_draw_circle(circle[2], target, brush);
			loc_draw_circle(circle[3], target, brush);
			loc_draw_circle(circle[1], target, brush);
			loc_draw_circle(circle[0], target, brush);
			// 図形の部位を描く.
			draw_loc();
		}
	}

	// 角丸半径を得る.
	bool ShapeRRect::get_corner_radius(D2D1_POINT_2F& val) const noexcept
	{
		val = m_corner_radius;
		return true;
	}

	// 指定した部位の点を得る.
	// loc	部位
	// val	得られた値
	void ShapeRRect::get_pos_loc(const uint32_t loc, D2D1_POINT_2F& val) const noexcept
	{
		const double dx = m_lineto.x;	// 差分 x
		const double dy = m_lineto.y;	// 差分 y
		const double mx = dx * 0.5;	// 中点 x
		const double my = dy * 0.5;	// 中点 y
		const double rx = fabs(mx) < fabs(m_corner_radius.x) ? mx : m_corner_radius.x;	// 角丸 x
		const double ry = fabs(my) < fabs(m_corner_radius.y) ? my : m_corner_radius.y;	// 角丸 y

		switch (loc) {
		case LOCUS_TYPE::LOCUS_R_NW:
			// 左上の角丸中心点を求める
			pt_add(m_start, rx, ry, val);
			break;
		case LOCUS_TYPE::LOCUS_R_NE:
			// 右上の角丸中心点を求める
			pt_add(m_start, dx - rx, ry, val);
			break;
		case LOCUS_TYPE::LOCUS_R_SE:
			// 右下の角丸中心点を求める
			pt_add(m_start, dx - rx, dy - ry, val);
			break;
		case LOCUS_TYPE::LOCUS_R_SW:
			// 左下の角丸中心点を求める
			pt_add(m_start, rx, dy - ry, val);
			break;
		default:
			ShapeOblong::get_pos_loc(loc, val);
			break;
		}
	}

	// 位置が角丸方形に含まれるか判定する.
	// test_pt	判定される点
	// rect_lt	角丸方形の左上位置
	// rect_rb	角丸方形の右下位置
	// corner_rad	角丸の半径
	// 戻り値	含まれるなら true を返す.
	static bool pt_in_rrect(const D2D1_POINT_2F test_pt, const D2D1_POINT_2F rect_lt, const D2D1_POINT_2F rect_rb, const D2D1_POINT_2F corner_rad)
	{
		if (test_pt.x < rect_lt.x) {
			return false;
		}
		if (test_pt.x > rect_rb.x) {
			return false;
		}
		if (test_pt.y < rect_lt.y) {
			return false;
		}
		if (test_pt.y > rect_rb.y) {
			return false;
		}
		D2D1_POINT_2F c;	// 角丸の中心点
		pt_add(rect_lt, corner_rad, c);
		if (test_pt.x < c.x) {
			if (test_pt.y < c.y) {
				return pt_in_ellipse(test_pt, c, corner_rad.x, corner_rad.y);
			}
		}
		c.x = rect_rb.x - corner_rad.x;
		c.y = rect_lt.y + corner_rad.y;
		if (test_pt.x > c.x) {
			if (test_pt.y < c.y) {
				return pt_in_ellipse(test_pt, c, corner_rad.x, corner_rad.y);
			}
		}
		c.x = rect_rb.x - corner_rad.x;
		c.y = rect_rb.y - corner_rad.y;
		if (test_pt.x > c.x) {
			if (test_pt.y > c.y) {
				return pt_in_ellipse(test_pt, c, corner_rad.x, corner_rad.y);
			}
		}
		c.x = rect_lt.x + corner_rad.x;
		c.y = rect_rb.y - corner_rad.y;
		if (test_pt.x < c.x) {
			if (test_pt.y > c.y) {
				return pt_in_ellipse(test_pt, c, corner_rad.x, corner_rad.y);
			}
		}
		return true;
	}

	// 図形が点を含むか判定する.
	// test_pt	判定される点
	// 戻り値	点を含む部位
	uint32_t ShapeRRect::hit_test(const D2D1_POINT_2F test_pt, const bool/*ctrl_key*/) const noexcept
	{
		// 角丸の円弧の中心点に含まれるか判定する.
		// +---------+
		// | 2     4 |
		// |         |
		// | 3     1 |
		// +---------+
		uint32_t loc_r;
		const double mx = m_lineto.x * 0.5;	// 中間点
		const double my = m_lineto.y * 0.5;	// 中間点
		const double rx = fabs(mx) < fabs(m_corner_radius.x) ? mx : m_corner_radius.x;	// 角丸
		const double ry = fabs(my) < fabs(m_corner_radius.y) ? my : m_corner_radius.y;	// 角丸
		const D2D1_POINT_2F loc_r_nw{
			static_cast<FLOAT>(m_start.x + rx), 
			static_cast<FLOAT>(m_start.y + ry)
		};
		const D2D1_POINT_2F loc_r_se{
			static_cast<FLOAT>(m_start.x + m_lineto.x - rx),
			static_cast<FLOAT>(m_start.y + m_lineto.y - ry)
		};
		const D2D1_POINT_2F loc_r_ne{ loc_r_se.x, loc_r_nw.y };
		const D2D1_POINT_2F loc_r_sw{ loc_r_nw.x, loc_r_se.y };
		if (loc_hit_test(test_pt, loc_r_se, m_loc_width)) {
			loc_r = LOCUS_TYPE::LOCUS_R_SE;
		}
		else if (loc_hit_test(test_pt, loc_r_nw, m_loc_width)) {
			loc_r = LOCUS_TYPE::LOCUS_R_NW;
		}
		else if (loc_hit_test(test_pt, loc_r_sw, m_loc_width)) {
			loc_r = LOCUS_TYPE::LOCUS_R_SW;
		}
		else if (loc_hit_test(test_pt, loc_r_ne, m_loc_width)) {
			loc_r = LOCUS_TYPE::LOCUS_R_NE;
		}
		else {
			loc_r = LOCUS_TYPE::LOCUS_SHEET;
		}

		// 角丸のいずれかの中心点に含まれる,
		if (loc_r != LOCUS_TYPE::LOCUS_SHEET &&
			// かつ, 方形の大きさが図形の部位の倍の大きさより大きいか判定する.
			fabs(m_lineto.x) > m_loc_width && fabs(m_lineto.y) > 2.0f * m_loc_width) {
			return loc_r;
		}
		const uint32_t loc_v = rect_loc_hit_test(m_start, m_lineto, test_pt, m_loc_width);
		if (loc_v != LOCUS_TYPE::LOCUS_SHEET) {
			return loc_v;
		}
		// 頂点に含まれず, 角丸の円弧の中心点に含まれるか判定する.
		else if (loc_r != LOCUS_TYPE::LOCUS_SHEET) {
			return loc_r;
		}

		D2D1_POINT_2F r_lt;	// 左上位置
		D2D1_POINT_2F r_rb;	// 右下位置
		D2D1_POINT_2F r_rad;	// 角丸の半径
		if (m_lineto.x > 0.0f) {
			r_lt.x = m_start.x;
			r_rb.x = m_start.x + m_lineto.x;
		}
		else {
			r_lt.x = m_start.x + m_lineto.x;
			r_rb.x = m_start.x;
		}
		if (m_lineto.y > 0.0f) {
			r_lt.y = m_start.y;
			r_rb.y = m_start.y + m_lineto.y;
		}
		else {
			r_lt.y = m_start.y + m_lineto.y;
			r_rb.y = m_start.y;
		}
		r_rad.x = std::abs(m_corner_radius.x);
		r_rad.y = std::abs(m_corner_radius.y);

		// 線枠が透明または太さ 0 か判定する.
		if (!is_opaque(m_stroke_color) || m_stroke_width < FLT_MIN) {
			// 塗りつぶし色が不透明, かつ角丸方形そのものに含まれるか判定する.
			if (is_opaque(m_fill_color) && pt_in_rrect(test_pt, r_lt, r_rb, r_rad)) {
				return LOCUS_TYPE::LOCUS_FILL;
			}
		}
		// 線枠の色が不透明, かつ太さが 0 より大きい.
		else {
			// 拡大した角丸方形に含まれるか判定
			const double ew = max(m_stroke_width, m_loc_width);
			D2D1_POINT_2F e_lt, e_rb, e_rad;	// 拡大した角丸方形
			pt_add(r_lt, -ew * 0.5, e_lt);
			pt_add(r_rb, ew * 0.5, e_rb);
			pt_add(r_rad, ew * 0.5, e_rad);
			if (pt_in_rrect(test_pt, e_lt, e_rb, e_rad)) {
				// 縮小した角丸方形が逆転してない, かつ位置が縮小した角丸方形に含まれるか判定する.
				D2D1_POINT_2F s_lt, s_rb, s_rad;	// 縮小した角丸方形
				pt_add(e_lt, ew, s_lt);
				pt_add(e_rb, -ew, s_rb);
				pt_add(e_rad, -ew, s_rad);
				if (s_lt.x < s_rb.x && s_lt.y < s_rb.y && pt_in_rrect(test_pt, s_lt, s_rb, s_rad)) {
					// 塗りつぶし色が不透明なら, LOCUS_FILL を返す.
					if (is_opaque(m_fill_color)) {
						return LOCUS_TYPE::LOCUS_FILL;
					}
				}
				else {
					// 拡大した角丸方形に含まれ, 縮小した角丸方形に含まれないなら LOCUS_STROKE を返す.
					return LOCUS_TYPE::LOCUS_STROKE;
				}
			}
		}
		return LOCUS_TYPE::LOCUS_SHEET;
	}

	// 値を, 指定した部位の点に格納する.
	// val	値
	// loc	部位
	// snap_point	他の点との間隔 (この値より離れた点は無視する)
	bool ShapeRRect::set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool /*keep_aspect*/) noexcept
	{
		D2D1_POINT_2F a;	// 図形の部位の位置
		D2D1_POINT_2F p;	// 位置ベクトル
		D2D1_POINT_2F r;	// 角丸半径
		D2D1_POINT_2F q;	// 新しい点

		switch (loc) {
		case LOCUS_TYPE::LOCUS_R_NW:
			ShapeRRect::get_pos_loc(loc, a);
			pt_round(val, PT_ROUND, q);
			pt_sub(q, a, p);
			if (pt_abs2(p) < FLT_MIN) {
				return false;
			}
			pt_add(m_corner_radius, p, r);
			rrect_corner_radius(m_lineto, r, m_corner_radius);
			break;
		case LOCUS_TYPE::LOCUS_R_NE:
			ShapeRRect::get_pos_loc(loc, a);
			pt_round(val, PT_ROUND, q);
			pt_sub(q, a, p);
			if (pt_abs2(p) < FLT_MIN) {
				return false;
			}
			r.x = m_corner_radius.x - p.x;
			r.y = m_corner_radius.y + p.y;
			rrect_corner_radius(m_lineto, r, m_corner_radius);
			break;
		case LOCUS_TYPE::LOCUS_R_SE:
			ShapeRRect::get_pos_loc(loc, a);
			pt_round(val, PT_ROUND, q);
			pt_sub(q, a, p);
			if (pt_abs2(p) < FLT_MIN) {
				return false;
			}
			r.x = m_corner_radius.x - p.x;
			r.y = m_corner_radius.y - p.y;
			rrect_corner_radius(m_lineto, r, m_corner_radius);
			break;
		case LOCUS_TYPE::LOCUS_R_SW:
			ShapeRRect::get_pos_loc(loc, a);
			pt_round(val, PT_ROUND, q);
			pt_sub(q, a, p);
			if (pt_abs2(p) < FLT_MIN) {
				return false;
			}
			r.x = m_corner_radius.x + p.x;
			r.y = m_corner_radius.y - p.y;
			rrect_corner_radius(m_lineto, r, m_corner_radius);
			break;
		default:
			if (!ShapeOblong::set_pos_loc(val, loc, snap_point, false)) {
				return false;
			}
			if (m_lineto.x * m_corner_radius.x < 0.0f) {
				m_corner_radius.x = -m_corner_radius.x;
			}
			if (m_lineto.y * m_corner_radius.y < 0.0f) {
				m_corner_radius.y = -m_corner_radius.y;
			}
			break;
		}
		const double dd = static_cast<double>(snap_point) * static_cast<double>(snap_point);
		if (pt_abs2(m_corner_radius) < dd) {
			m_corner_radius.x = m_corner_radius.y = 0.0f;
		}
		return true;
	}

	// 図形を作成する.
	// start	始点
	// end_to	対角点への位置ベクトル
	// page	属性
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F start, const D2D1_POINT_2F end_to, const Shape* prop) :
		ShapeOblong::ShapeOblong(start, end_to, prop)
	{
		float g_base;
		prop->get_grid_base(g_base);
		rrect_corner_radius(end_to, D2D1_POINT_2F{ g_base + 1.0f, g_base + 1.0f }, m_corner_radius);
	}

	// 図形をデータリーダーから読み込む.
	ShapeRRect::ShapeRRect(DataReader const& dt_reader) :
		ShapeOblong::ShapeOblong(dt_reader)
	{
		m_corner_radius = D2D1_POINT_2F{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (m_corner_radius.x < 0.0f || m_corner_radius.x > 0.5f * fabs(m_lineto.x)) {
			m_corner_radius.x = 0.0f;
		}
		if (m_corner_radius.y < 0.0f || m_corner_radius.y > 0.5f * fabs(m_lineto.y)) {
			m_corner_radius.y = 0.0f;
		}
	}

	// 図形をデータライターに書き込む.
	void ShapeRRect::write(DataWriter const& dt_writer) const
	{
		ShapeOblong::write(dt_writer);
		dt_writer.WriteSingle(m_corner_radius.x);
		dt_writer.WriteSingle(m_corner_radius.y);
	}

}