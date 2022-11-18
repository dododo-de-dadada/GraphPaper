//------------------------------
// shape_rrect.cpp
// 角丸方形図形
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;

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
	// sh	表示する用紙
	void ShapeRRect::draw(ShapeSheet const& sh)
	{
		const D2D_UI& d2d = sh.m_d2d;
		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(d2d);
		}

		auto s_style = m_d2d_stroke_style.get();
		auto s_width = m_stroke_width;
		auto dc = d2d.m_d2d_context;

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
			sh.m_color_brush->SetColor(m_fill_color);
			dc->FillRoundedRectangle(r_rec, sh.m_color_brush.get());
		}
		sh.m_color_brush->SetColor(m_stroke_color);
		dc->DrawRoundedRectangle(r_rec, sh.m_color_brush.get(), s_width, s_style);
		if (is_selected()) {
			//const auto zero = (std::abs(m_vec[0].x) >= FLT_MIN && std::abs(m_vec[0].y) >= FLT_MIN);
			//if (zero) {
			// D2D1_POINT_2F c_pos;
			// pt_add(r_min, rx, ry, c_pos);
			// anc_draw_ellipse(c_pos, d2d);
			// c_pos.x = r_rec.rect.right - rx;
			// anc_draw_ellipse(c_pos, d2d);
			// c_pos.y = r_rec.rect.bottom - ry;
			// anc_draw_ellipse(c_pos, d2d);
			// c_pos.x = r_min.x + rx;
			// anc_draw_ellipse(c_pos, d2d);
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
				anc_draw_rect(r_pos[i], sh);
				anc_draw_rect(r_mid, sh);
			}
			//if (!zero) {
				D2D1_POINT_2F c_pos;
				pt_add(r_min, rx, ry, c_pos);
				anc_draw_ellipse(c_pos, sh);
				c_pos.x = r_rec.rect.right - rx;
				anc_draw_ellipse(c_pos, sh);
				c_pos.y = r_rec.rect.bottom - ry;
				anc_draw_ellipse(c_pos, sh);
				c_pos.x = r_min.x + rx;
				anc_draw_ellipse(c_pos, sh);
			//}
		}
	}

	// 角丸半径を得る.
	bool ShapeRRect::get_corner_radius(D2D1_POINT_2F& val) const noexcept
	{
		val = m_corner_rad;
		return true;
	}

	//	部位の位置を得る.
	//	anc	図形の部位.
	//	val	得られた位置.
	//	戻り値	なし
	void ShapeRRect::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		const double dx = m_vec[0].x;
		const double dy = m_vec[0].y;
		const double mx = dx * 0.5;	// 中点
		const double my = dy * 0.5;	// 中点
		const double rx = fabs(mx) < fabs(m_corner_rad.x) ? mx : m_corner_rad.x;	// 角丸
		const double ry = fabs(my) < fabs(m_corner_rad.y) ? my : m_corner_rad.y;	// 角丸
		switch (anc) {
		case ANC_TYPE::ANC_R_NW:
			// 左上の角丸中心点を求める
			pt_add(m_pos, rx, ry, val);
			break;
		case ANC_TYPE::ANC_R_NE:
			// 右上の角丸中心点を求める
			pt_add(m_pos, dx - rx, ry, val);
			break;
		case ANC_TYPE::ANC_R_SE:
			// 右下の角丸中心点を求める
			pt_add(m_pos, dx - rx, dy - ry, val);
			break;
		case ANC_TYPE::ANC_R_SW:
			// 左下の角丸中心点を求める
			pt_add(m_pos, rx, dy - ry, val);
			break;
		default:
			ShapeRect::get_pos_anc(anc, val);
			break;
		}
	}

	// 位置が角丸方形に含まれるか判定する.
	// t_pos	判定する位置
	// r_min	角丸方形の左上位置
	// r_max	角丸方形の右下位置
	// r_rad	角丸の半径
	// 戻り値	含まれるなら true を返す.
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

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeRRect::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		// 角丸の円弧の中心点に含まれるか判定する.
		// +---------+
		// | 1     3 |
		// |         |
		// | 4     2 |
		// +---------+
		uint32_t anc_r;
		const double mx = m_vec[0].x * 0.5;	// 中点
		const double my = m_vec[0].y * 0.5;	// 中点
		const double rx = fabs(mx) < fabs(m_corner_rad.x) ? mx : m_corner_rad.x;	// 角丸
		const double ry = fabs(my) < fabs(m_corner_rad.y) ? my : m_corner_rad.y;	// 角丸
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
						anc_r = ANC_TYPE::ANC_SHEET;
					}
				}
			}
		}
		// 角丸の円弧の中心点に含まれる,
		if (anc_r != ANC_TYPE::ANC_SHEET &&
			// かつ, 方形の大きさが図形の部位の大きさより大きいか判定する.
			fabs(m_vec[0].x) > Shape::s_anc_len && fabs(m_vec[0].y) > Shape::s_anc_len) {
			return anc_r;
		}
		// 方形の各頂点に含まれるか判定する.
		const uint32_t anc_v = hit_test_anc(t_pos);
		if (anc_v != ANC_TYPE::ANC_SHEET) {
			return anc_v;
		}
		// 頂点に含まれず, 角丸の円弧の中心点に含まれるか判定する.
		else if (anc_r != ANC_TYPE::ANC_SHEET) {
			return anc_r;
		}

		// 角丸方形を正規化する.
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

		// 線枠が透明または太さ 0 か判定する.
		if (!is_opaque(m_stroke_color) || m_stroke_width < FLT_MIN) {
			// 塗りつぶし色が不透明, かつ角丸方形そのものに含まれるか判定する.
			if (is_opaque(m_fill_color) && pt_in_rrect(t_pos, r_min, r_max, r_rad)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		// 線枠の色が不透明, かつ太さが 0 より大きい.
		else {
			// 以下の手順は, 縮小した角丸方形の外側に, 角丸交点があるケースでうまくいかない.
			// 拡大した角丸方形に含まれるか判定
			const double s_thick = max(m_stroke_width, Shape::s_anc_len);
			D2D1_POINT_2F s_min, s_max, s_rad;
			pt_add(r_min, -s_thick * 0.5, s_min);
			pt_add(r_max, s_thick * 0.5, s_max);
			pt_add(r_rad, s_thick * 0.5, s_rad);
			if (pt_in_rrect(t_pos, s_min, s_max, s_rad)) {
				// 縮小した角丸方形が逆転してない, かつ位置が縮小した角丸方形に含まれるか判定する.
				D2D1_POINT_2F u_min, u_max, u_rad;
				pt_add(s_min, s_thick, u_min);
				pt_add(s_max, -s_thick, u_max);
				pt_add(s_rad, -s_thick, u_rad);
				if (u_min.x < u_max.x && u_min.y < u_max.y && pt_in_rrect(t_pos, u_min, u_max, u_rad)) {
					// 塗りつぶし色が不透明なら, ANC_FILL を返す.
					if (is_opaque(m_fill_color)) {
						return ANC_TYPE::ANC_FILL;
					}
				}
				else {
					// 拡大した角丸方形に含まれ, 縮小した角丸方形に含まれないなら ANC_STROKE を返す.
					return ANC_TYPE::ANC_STROKE;
				}
			}
		}
		return ANC_TYPE::ANC_SHEET;
	}

	// 値を, 部位の位置に格納する. 他の部位の位置も動く.
	// val	値
	// anc	図形の部位
	// limit	限界距離 (他の頂点との距離がこの値未満になるなら, その頂点に位置に合わせる)
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

	// 図形を作成する.
	// b_pos	囲む領域の始点
	// b_vec	囲む領域の終点への差分
	// s_attr	属性
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr) :
		ShapeRect::ShapeRect(b_pos, b_vec, s_attr)
	{
		calc_corner_radius(m_vec[0], s_attr->m_corner_rad, m_corner_rad);
	}

	// 図形をデータリーダーから読み込む.
	ShapeRRect::ShapeRRect(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		dt_read(m_corner_rad, dt_reader);
	}

	// 図形をデータライターに書き込む.
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
		dt_write_svg(m_vec[0], "width", "height", dt_writer);
		if (std::round(m_corner_rad.x) != 0.0f && std::round(m_corner_rad.y) != 0.0f) {
			dt_write_svg(m_corner_rad, "rx", "ry", dt_writer);
		}
		dt_write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		dt_write_svg("/>", dt_writer);
	}
}