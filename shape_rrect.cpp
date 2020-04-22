//------------------------------
// shape_rrect.cpp
// 角丸方形図形
//------------------------------
#include "pch.h"
#include "shape_rect.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 角丸方形の中点の配列
	constexpr ANCH_WHICH ANCH_ROUND[4]{
		ANCH_WHICH::ANCH_R_SE,	// 右下角
		ANCH_WHICH::ANCH_R_NE,	// 右上角
		ANCH_WHICH::ANCH_R_SW,	// 左下角
		ANCH_WHICH::ANCH_R_NW	// 左上角
	};

	// 角丸半径を計算する.
	static void calc_corner_radius(const D2D1_POINT_2F diff, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad);
	// 角丸半径の縦または横の成分を計算する.
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad);

	// 角丸半径を計算する.
	// diff	角丸方形の対角ベクトル
	// d_rad	既定の角丸半径
	// c_rad	計算された角丸半径
	static void calc_corner_radius(const D2D1_POINT_2F diff, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad)
	{
		calc_corner_radius(diff.x, d_rad.x, c_rad.x);
		calc_corner_radius(diff.y, d_rad.y, c_rad.y);
	}

	// 角丸半径の縦または横の成分を計算する.
	// r_len	角丸方形の一辺の大きさ
	// d_rad	もとの角丸半径
	// c_rad	得られた角丸半径
	static void calc_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad)
	{
		const double r = r_len * 0.5;
		if (fabs(d_rad) > fabs(r)) {
			// もとの角丸半径が方形の大きさの半分を超えるなら,
			// 大きさの半分を得られた角丸半径に格納する.
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
			// anchor_draw_rounded(c_pos, dx);
			// c_pos.x = r_rec.rect.right - rx;
			// anchor_draw_rounded(c_pos, dx);
			// c_pos.y = r_rec.rect.bottom - ry;
			// anchor_draw_rounded(c_pos, dx);
			// c_pos.x = r_min.x + rx;
			// anchor_draw_rounded(c_pos, dx);
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
				anchor_draw_rect(r_pos[i], dx);
				anchor_draw_rect(r_mid, dx);
			}
			//if (flag != true) {
				D2D1_POINT_2F c_pos;
				pt_add(r_min, rx, ry, c_pos);
				anchor_draw_rounded(c_pos, dx);
				c_pos.x = r_rec.rect.right - rx;
				anchor_draw_rounded(c_pos, dx);
				c_pos.y = r_rec.rect.bottom - ry;
				anchor_draw_rounded(c_pos, dx);
				c_pos.x = r_min.x + rx;
				anchor_draw_rounded(c_pos, dx);
			//}
		}
	}

	// 角丸半径を得る.
	bool ShapeRRect::get_corner_radius(D2D1_POINT_2F& value) const noexcept
	{
		value = m_corner_rad;
		return true;
	}

	// 指定された部位の位置を得る.
	void ShapeRRect::get_pos(const ANCH_WHICH a, D2D1_POINT_2F& value) const noexcept
	{
		const double vx = m_diff.x;
		const double vy = m_diff.y;
		const double hx = vx * 0.5;
		const double hy = vy * 0.5;
		const double rx = fabs(hx) < fabs(m_corner_rad.x) ? hx : m_corner_rad.x;
		const double ry = fabs(hy) < fabs(m_corner_rad.y) ? hy : m_corner_rad.y;
		switch (a) {
		case ANCH_WHICH::ANCH_R_NW:
			// 左上の角丸中心点を求める
			pt_add(m_pos, rx, ry, value);
			break;
		case ANCH_WHICH::ANCH_R_NE:
			// 右上の角丸中心点を求める
			pt_add(m_pos, vx - rx, ry, value);
			break;
		case ANCH_WHICH::ANCH_R_SE:
			// 右下の角丸中心点を求める
			pt_add(m_pos, vx - rx, vy - ry, value);
			break;
		case ANCH_WHICH::ANCH_R_SW:
			// 左下の角丸中心点を求める
			pt_add(m_pos, rx, vy - ry, value);
			break;
		default:
			ShapeRect::get_pos(a, value);
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

	// 位置を含むか調べる.
	// t_pos	調べる位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	ANCH_WHICH ShapeRRect::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const auto flag = (fabs(m_diff.x) > FLT_MIN&& fabs(m_diff.y) > FLT_MIN);
		if (flag) {
			for (uint32_t i = 0; i < 4; i++) {
				// 角丸の中心点を得る.
				D2D1_POINT_2F r_cen;
				get_pos(ANCH_ROUND[i], r_cen);
				// 位置が角丸の部位に含まれるか調べる.
				if (pt_in_anch(t_pos, r_cen, a_len)) {
					// 含まれるなら角丸の部位を返す.
					return ANCH_ROUND[i];
				}
			}
		}
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F r_pos;	// 方形の頂点
			get_pos(ANCH_CORNER[i], r_pos);
			if (pt_in_anch(t_pos, r_pos, a_len)) {
				return ANCH_CORNER[i];
			}
		}
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F r_pos;	// 方形の辺の中点
			get_pos(ANCH_MIDDLE[i], r_pos);
			if (pt_in_anch(t_pos, r_pos, a_len)) {
				return ANCH_MIDDLE[i];
			}
		}
		if (flag != true) {
			for (uint32_t i = 0; i < 4; i++) {
				D2D1_POINT_2F r_cen;	// 角丸部分の中心点
				get_pos(ANCH_ROUND[i], r_cen);
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
		// 外側の角丸方形の判定
		pt_add(r_min, -s_width * 0.5, r_min);
		pt_add(r_max, s_width * 0.5, r_max);
		pt_add(r_rad, s_width * 0.5, r_rad);
		if (pt_in_rrect(t_pos, r_min, r_max, r_rad) != true) {
			return ANCH_WHICH::ANCH_OUTSIDE;
		}
		// 内側の角丸方形の判定
		pt_add(r_min, s_width, r_min);
		pt_add(r_max, -s_width, r_max);
		pt_add(r_rad, -s_width, r_rad);
		if (pt_in_rrect(t_pos, r_min, r_max, r_rad) != true) {
			return ANCH_WHICH::ANCH_FRAME;
		}
		return is_opaque(m_fill_color) ? ANCH_WHICH::ANCH_INSIDE : ANCH_WHICH::ANCH_OUTSIDE;
	}

	// 値を指定した部位の位置に格納する. 他の部位の位置は動かない. 
	void ShapeRRect::set_pos(const D2D1_POINT_2F value, const ANCH_WHICH a)
	{
		D2D1_POINT_2F c_pos;
		D2D1_POINT_2F diff;
		D2D1_POINT_2F rad;

		switch (a) {
		case ANCH_WHICH::ANCH_R_NW:
			ShapeRRect::get_pos(a, c_pos);
			pt_sub(value, c_pos, diff);
			pt_add(m_corner_rad, diff, rad);
			calc_corner_radius(m_diff, rad, m_corner_rad);
			break;
		case ANCH_WHICH::ANCH_R_NE:
			ShapeRRect::get_pos(a, c_pos);
			pt_sub(value, c_pos, diff);
			rad.x = m_corner_rad.x - diff.x;
			rad.y = m_corner_rad.y + diff.y;
			calc_corner_radius(m_diff, rad, m_corner_rad);
			break;
		case ANCH_WHICH::ANCH_R_SE:
			ShapeRRect::get_pos(a, c_pos);
			pt_sub(value, c_pos, diff);
			rad.x = m_corner_rad.x - diff.x;
			rad.y = m_corner_rad.y - diff.y;
			calc_corner_radius(m_diff, rad, m_corner_rad);
			break;
		case ANCH_WHICH::ANCH_R_SW:
			ShapeRRect::get_pos(a, c_pos);
			pt_sub(value, c_pos, diff);
			rad.x = m_corner_rad.x + diff.x;
			rad.y = m_corner_rad.y - diff.y;
			calc_corner_radius(m_diff, rad, m_corner_rad);
			break;
		default:
			ShapeRect::set_pos(value, a);
			if (m_diff.x * m_corner_rad.x < 0.0f) {
				m_corner_rad.x = -m_corner_rad.x;
			}
			if (m_diff.y * m_corner_rad.y < 0.0f) {
				m_corner_rad.y = -m_corner_rad.y;
			}
			break;
		}
	}

	// 図形を作成する.
	// s_pos	開始位置
	// diff	開始位置からの差分
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, const ShapeSheet* attr) :
		ShapeRect::ShapeRect(s_pos, diff, attr)
	{
		calc_corner_radius(m_diff, attr->m_corner_rad, m_corner_rad);
	}

	// 図形をデータリーダーから読み込む.
	ShapeRRect::ShapeRRect(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_corner_rad, dt_reader);
	}

	// データライターに書き込む.
	void ShapeRRect::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeRect::write(dt_writer);
		write(m_corner_rad, dt_writer);
	}

	// データライターに SVG タグとして書き込む.
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