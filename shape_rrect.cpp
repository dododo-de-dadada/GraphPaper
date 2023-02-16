//------------------------------
// shape_rrect.cpp
// 角丸方形図形
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

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
			D2D1_POINT_2F c_pos[4]{
				{ r_lt.x + rx, r_lt.y + ry },
				{ r_lt.x + vx - rx, r_lt.y + ry },
				{ r_lt.x + vx - rx, r_lt.y + vy - ry },
				{ r_lt.x + rx, r_lt.y + vy - ry }
			};
			anc_draw_circle(c_pos[2], target, brush);
			anc_draw_circle(c_pos[3], target, brush);
			anc_draw_circle(c_pos[1], target, brush);
			anc_draw_circle(c_pos[0], target, brush);
			draw_anc();
		}
	}

	// 角丸半径を得る.
	bool ShapeRRect::get_corner_radius(D2D1_POINT_2F& val) const noexcept
	{
		val = m_corner_radius;
		return true;
	}

	//	部位の位置を得る.
	//	anc	図形の部位.
	//	val	得られた位置.
	//	戻り値	なし
	void ShapeRRect::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		const double dx = m_vec[0].x;	// 差分 x
		const double dy = m_vec[0].y;	// 差分 y
		const double mx = dx * 0.5;	// 中点 x
		const double my = dy * 0.5;	// 中点 y
		const double rx = fabs(mx) < fabs(m_corner_radius.x) ? mx : m_corner_radius.x;	// 角丸 x
		const double ry = fabs(my) < fabs(m_corner_radius.y) ? my : m_corner_radius.y;	// 角丸 y

		switch (anc) {
		case ANC_TYPE::ANC_R_NW:
			// 左上の角丸中心点を求める
			pt_add(m_start, rx, ry, val);
			break;
		case ANC_TYPE::ANC_R_NE:
			// 右上の角丸中心点を求める
			pt_add(m_start, dx - rx, ry, val);
			break;
		case ANC_TYPE::ANC_R_SE:
			// 右下の角丸中心点を求める
			pt_add(m_start, dx - rx, dy - ry, val);
			break;
		case ANC_TYPE::ANC_R_SW:
			// 左下の角丸中心点を求める
			pt_add(m_start, rx, dy - ry, val);
			break;
		default:
			ShapeRect::get_pos_anc(anc, val);
			break;
		}
	}

	// 位置が角丸方形に含まれるか判定する.
	// t_pos	判定する位置
	// r_lt	角丸方形の左上位置
	// r_rb	角丸方形の右下位置
	// r_rad	角丸の半径
	// 戻り値	含まれるなら true を返す.
	static bool pt_in_rrect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_lt, const D2D1_POINT_2F r_rb, const D2D1_POINT_2F r_rad)
	{
		if (t_pos.x < r_lt.x) {
			return false;
		}
		if (t_pos.x > r_rb.x) {
			return false;
		}
		if (t_pos.y < r_lt.y) {
			return false;
		}
		if (t_pos.y > r_rb.y) {
			return false;
		}
		D2D1_POINT_2F c_pos;
		pt_add(r_lt, r_rad, c_pos);
		if (t_pos.x < c_pos.x) {
			if (t_pos.y < c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_rb.x - r_rad.x;
		c_pos.y = r_lt.y + r_rad.y;
		if (t_pos.x > c_pos.x) {
			if (t_pos.y < c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_rb.x - r_rad.x;
		c_pos.y = r_rb.y - r_rad.y;
		if (t_pos.x > c_pos.x) {
			if (t_pos.y > c_pos.y) {
				return pt_in_ellipse(t_pos, c_pos, r_rad.x, r_rad.y);
			}
		}
		c_pos.x = r_lt.x + r_rad.x;
		c_pos.y = r_rb.y - r_rad.y;
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
		const double rx = fabs(mx) < fabs(m_corner_radius.x) ? mx : m_corner_radius.x;	// 角丸
		const double ry = fabs(my) < fabs(m_corner_radius.y) ? my : m_corner_radius.y;	// 角丸
		const D2D1_POINT_2F anc_r_nw{
			static_cast<FLOAT>(m_start.x + rx), 
			static_cast<FLOAT>(m_start.y + ry)
		};
		if (pt_in_anc(t_pos, anc_r_nw, m_anc_width)) {
			anc_r = ANC_TYPE::ANC_R_NW;
		}
		else {
			const D2D1_POINT_2F anc_r_se{
				static_cast<FLOAT>(m_start.x + m_vec[0].x - rx),
				static_cast<FLOAT>(m_start.y + m_vec[0].y - ry)
			};
			if (pt_in_anc(t_pos, anc_r_se, m_anc_width)) {
				anc_r = ANC_TYPE::ANC_R_SE;
			}
			else {
				const D2D1_POINT_2F anc_r_ne{ anc_r_se.x, anc_r_nw.y };
				if (pt_in_anc(t_pos, anc_r_ne, m_anc_width)) {
					anc_r = ANC_TYPE::ANC_R_NE;
				}
				else {
					const D2D1_POINT_2F anc_r_sw{ anc_r_nw.x, anc_r_se.y };
					if (pt_in_anc(t_pos, anc_r_sw, m_anc_width)) {
						anc_r = ANC_TYPE::ANC_R_SW;
					}
					else {
						anc_r = ANC_TYPE::ANC_PAGE;
					}
				}
			}
		}
		// 角丸の円弧の中心点に含まれる,
		if (anc_r != ANC_TYPE::ANC_PAGE &&
			// かつ, 方形の大きさが図形の部位の大きさより大きいか判定する.
			fabs(m_vec[0].x) > m_anc_width && fabs(m_vec[0].y) > m_anc_width) {
			return anc_r;
		}
		// 方形の各頂点に含まれるか判定する.
		const uint32_t anc_v = rect_hit_test_anc(m_start, m_vec[0], t_pos, m_anc_width);
		if (anc_v != ANC_TYPE::ANC_PAGE) {
			return anc_v;
		}
		// 頂点に含まれず, 角丸の円弧の中心点に含まれるか判定する.
		else if (anc_r != ANC_TYPE::ANC_PAGE) {
			return anc_r;
		}

		// 角丸方形を正規化する.
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

		// 線枠が透明または太さ 0 か判定する.
		if (!is_opaque(m_stroke_color) || m_stroke_width < FLT_MIN) {
			// 塗りつぶし色が不透明, かつ角丸方形そのものに含まれるか判定する.
			if (is_opaque(m_fill_color) && pt_in_rrect(t_pos, r_lt, r_rb, r_rad)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		// 線枠の色が不透明, かつ太さが 0 より大きい.
		else {
			// 以下の手順は, 縮小した角丸方形の外側に, 角丸交点があるケースでうまくいかない.
			// 拡大した角丸方形に含まれるか判定
			const double s_thick = max(m_stroke_width, m_anc_width);
			D2D1_POINT_2F e_lt, e_rb, e_rad;	// 拡大した角丸方形
			pt_add(r_lt, -s_thick * 0.5, e_lt);
			pt_add(r_rb, s_thick * 0.5, e_rb);
			pt_add(r_rad, s_thick * 0.5, e_rad);
			if (pt_in_rrect(t_pos, e_lt, e_rb, e_rad)) {
				// 縮小した角丸方形が逆転してない, かつ位置が縮小した角丸方形に含まれるか判定する.
				D2D1_POINT_2F s_lt, s_rb, s_rad;	// 縮小した角丸方形
				pt_add(e_lt, s_thick, s_lt);
				pt_add(e_rb, -s_thick, s_rb);
				pt_add(e_rad, -s_thick, s_rad);
				if (s_lt.x < s_rb.x && s_lt.y < s_rb.y && pt_in_rrect(t_pos, s_lt, s_rb, s_rad)) {
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
		return ANC_TYPE::ANC_PAGE;
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
			pt_add(m_corner_radius, vec, rad);
			calc_corner_radius(m_vec[0], rad, m_corner_radius);
			break;
		case ANC_TYPE::ANC_R_NE:
			ShapeRRect::get_pos_anc(anc, c_pos);
			pt_round(val, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_radius.x - vec.x;
			rad.y = m_corner_radius.y + vec.y;
			calc_corner_radius(m_vec[0], rad, m_corner_radius);
			break;
		case ANC_TYPE::ANC_R_SE:
			ShapeRRect::get_pos_anc(anc, c_pos);
			pt_round(val, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_radius.x - vec.x;
			rad.y = m_corner_radius.y - vec.y;
			calc_corner_radius(m_vec[0], rad, m_corner_radius);
			break;
		case ANC_TYPE::ANC_R_SW:
			ShapeRRect::get_pos_anc(anc, c_pos);
			pt_round(val, PT_ROUND, new_pos);
			pt_sub(new_pos, c_pos, vec);
			if (pt_abs2(vec) < FLT_MIN) {
				return false;
			}
			rad.x = m_corner_radius.x + vec.x;
			rad.y = m_corner_radius.y - vec.y;
			calc_corner_radius(m_vec[0], rad, m_corner_radius);
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

	// 図形を作成する.
	// b_pos	囲む領域の始点
	// b_vec	囲む領域の終点への差分
	// page	属性
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const Shape* page) :
		ShapeRect::ShapeRect(b_pos, b_vec, page)
	{
		float g_base;
		page->get_grid_base(g_base);
		calc_corner_radius(m_vec[0], D2D1_POINT_2F{ g_base + 1.0f, g_base + 1.0f }, m_corner_radius);
	}

	// 図形をデータリーダーから読み込む.
	ShapeRRect::ShapeRRect(const Shape& page, DataReader const& dt_reader) :
		ShapeRect::ShapeRect(page, dt_reader)
	{
		m_corner_radius = D2D1_POINT_2F{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
	}

	// 図形をデータライターに書き込む.
	void ShapeRRect::write(DataWriter const& dt_writer) const
	{
		ShapeRect::write(dt_writer);
		dt_writer.WriteSingle(m_corner_radius.x);
		dt_writer.WriteSingle(m_corner_radius.y);
	}

}