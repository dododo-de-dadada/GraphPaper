//------------------------------
// shape_rrect.cpp
// 角丸方形図形
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 角丸方形の中点の配列
	constexpr uint32_t ANCH_ROUND[4]{
		ANCH_TYPE::ANCH_R_SE,	// 右下角
		ANCH_TYPE::ANCH_R_NE,	// 右上角
		ANCH_TYPE::ANCH_R_SW,	// 左下角
		ANCH_TYPE::ANCH_R_NW	// 左上角
	};

	// 角丸半径を計算する.
	static void calc_corner_radius(const D2D1_POINT_2F d_vec, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad);

	// 角丸半径の縦または横の成分を計算する.
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad);

	// 角丸半径を計算する.
	// d_vec	角丸方形の対角ベクトル
	// d_rad	既定の角丸半径
	// c_rad	計算された角丸半径
	static void calc_corner_radius(const D2D1_POINT_2F d_vec, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad)
	{
		calc_corner_radius(d_vec.x, d_rad.x, c_rad.x);
		calc_corner_radius(d_vec.y, d_rad.y, c_rad.y);
	}

	// 角丸半径の縦または横の成分を計算する.
	// r_len	角丸方形の一辺の大きさ
	// d_rad	もとの角丸半径
	// c_rad	得られた角丸半径
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad)
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
	void ShapeRRect::draw(SHAPE_DX& dx)
	{
		auto s_brush = dx.m_shape_brush.get();
		auto s_style = m_d2d_stroke_style.get();
		auto s_width = m_stroke_width;
		auto dc = dx.m_d2d_context;

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
			s_brush->SetColor(m_fill_color);
			dc->FillRoundedRectangle(r_rec, s_brush);
		}
		s_brush->SetColor(m_stroke_color);
		dc->DrawRoundedRectangle(r_rec, s_brush, s_width, s_style);
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
				// 方形の頂点のアンカーを表示する.
				// 辺の中点を求め, そのアンカーを表示する.
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

	// 角丸半径を得る.
	bool ShapeRRect::get_corner_radius(D2D1_POINT_2F& value) const noexcept
	{
		value = m_corner_rad;
		return true;
	}

	//	部位の位置を得る.
	//	anch	図形の部位.
	//	value	得られた位置.
	//	戻り値	なし
	void ShapeRRect::get_pos_anch(const uint32_t anch, D2D1_POINT_2F& value) const noexcept
	{
		const double dx = m_diff[0].x;
		const double dy = m_diff[0].y;
		const double mx = dx * 0.5;	// 中点
		const double my = dy * 0.5;	// 中点
		const double rx = fabs(mx) < fabs(m_corner_rad.x) ? mx : m_corner_rad.x;	// 角丸
		const double ry = fabs(my) < fabs(m_corner_rad.y) ? my : m_corner_rad.y;	// 角丸
		switch (anch) {
		case ANCH_TYPE::ANCH_R_NW:
			// 左上の角丸中心点を求める
			pt_add(m_pos, rx, ry, value);
			break;
		case ANCH_TYPE::ANCH_R_NE:
			// 右上の角丸中心点を求める
			pt_add(m_pos, dx - rx, ry, value);
			break;
		case ANCH_TYPE::ANCH_R_SE:
			// 右下の角丸中心点を求める
			pt_add(m_pos, dx - rx, dy - ry, value);
			break;
		case ANCH_TYPE::ANCH_R_SW:
			// 左下の角丸中心点を求める
			pt_add(m_pos, rx, dy - ry, value);
			break;
		default:
			ShapeRect::get_pos_anch(anch, value);
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

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeRRect::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		const auto flag = (fabs(m_diff[0].x) >= FLT_MIN && fabs(m_diff[0].y) >= FLT_MIN);
		if (flag) {
			for (uint32_t i = 0; i < 4; i++) {
				// 角丸の中心点を得る.
				D2D1_POINT_2F r_cen;
				get_pos_anch(ANCH_ROUND[i], r_cen);
				// 角丸の部位に位置が含まれるか判定する.
				if (pt_in_anch(t_pos, r_cen)) {
					// 含まれるなら角丸の部位を返す.
					return ANCH_ROUND[i];
				}
			}
		}
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F r_pos;	// 方形の頂点
			get_pos_anch(ANCH_CORNER[i], r_pos);
			if (pt_in_anch(t_pos, r_pos)) {
				return ANCH_CORNER[i];
			}
		}
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F r_pos;	// 方形の辺の中点
			get_pos_anch(ANCH_MIDDLE[i], r_pos);
			if (pt_in_anch(t_pos, r_pos)) {
				return ANCH_MIDDLE[i];
			}
		}
		if (flag != true) {
			for (uint32_t i = 0; i < 4; i++) {
				D2D1_POINT_2F r_cen;	// 角丸部分の中心点
				get_pos_anch(ANCH_ROUND[i], r_cen);
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
		// 外側の角丸方形の判定
		pt_add(r_min, -s_width * 0.5, r_min);
		pt_add(r_max, s_width * 0.5, r_max);
		pt_add(r_rad, s_width * 0.5, r_rad);
		if (pt_in_rrect(t_pos, r_min, r_max, r_rad) != true) {
			return ANCH_TYPE::ANCH_SHEET;
		}
		// 内側の角丸方形の判定
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

	// 値を, 部位の位置に格納する. 他の部位の位置も動く.
	// value	値
	// anch	図形の部位
	// limit	限界距離 (他の頂点との距離がこの値未満になるなら, その頂点に位置に合わせる)
	bool ShapeRRect::set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float limit, const bool /*keep_aspect*/)
	{
		D2D1_POINT_2F c_pos;
		D2D1_POINT_2F vec;
		D2D1_POINT_2F rad;

		switch (anch) {
		case ANCH_TYPE::ANCH_R_NW:
			ShapeRRect::get_pos_anch(anch, c_pos);
			pt_sub(value, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			pt_add(m_corner_rad, vec, rad);
			calc_corner_radius(m_diff[0], rad, m_corner_rad);
			break;
		case ANCH_TYPE::ANCH_R_NE:
			ShapeRRect::get_pos_anch(anch, c_pos);
			pt_sub(value, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x - vec.x;
			rad.y = m_corner_rad.y + vec.y;
			calc_corner_radius(m_diff[0], rad, m_corner_rad);
			break;
		case ANCH_TYPE::ANCH_R_SE:
			ShapeRRect::get_pos_anch(anch, c_pos);
			pt_sub(value, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x - vec.x;
			rad.y = m_corner_rad.y - vec.y;
			calc_corner_radius(m_diff[0], rad, m_corner_rad);
			break;
		case ANCH_TYPE::ANCH_R_SW:
			ShapeRRect::get_pos_anch(anch, c_pos);
			pt_sub(value, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_rad.x + vec.x;
			rad.y = m_corner_rad.y - vec.y;
			calc_corner_radius(m_diff[0], rad, m_corner_rad);
			break;
		default:
			ShapeRect::set_pos_anch(value, anch, limit, false);
			if (m_diff[0].x * m_corner_rad.x < 0.0f) {
				m_corner_rad.x = -m_corner_rad.x;
			}
			if (m_diff[0].y * m_corner_rad.y < 0.0f) {
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

	// 図形を作成する.
	// b_pos	囲む領域の始点
	// b_vec	囲む領域の終点への差分
	// s_attr	属性
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr) :
		ShapeRect::ShapeRect(b_pos, b_vec, s_attr)
	{
		calc_corner_radius(m_diff[0], s_attr->m_corner_rad, m_corner_rad);
	}

	// データリーダーから図形を読み込む.
	ShapeRRect::ShapeRRect(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		dt_read(m_corner_rad, dt_reader);
	}

	// データライターに書き込む.
	void ShapeRRect::write(DataWriter const& dt_writer) const
	{
		ShapeRect::write(dt_writer);
		dt_write(m_corner_rad, dt_writer);
	}

	// データライターに SVG タグとして書き込む.
	void ShapeRRect::write_svg(DataWriter const& dt_writer) const
	{
		dt_write_svg("<rect ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_diff[0], "width", "height", dt_writer);
		if (std::round(m_corner_rad.x) != 0.0f && std::round(m_corner_rad.y) != 0.0f) {
			dt_write_svg(m_corner_rad, "rx", "ry", dt_writer);
		}
		dt_write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		dt_write_svg("/>", dt_writer);
	}
}