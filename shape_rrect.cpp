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
	static void rrect_corner_radius(const D2D1_POINT_2F pos, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad);

	// 角丸半径の縦または横の成分を計算する.
	static void rrect_corner_radius(const FLOAT r_len, const FLOAT d_rad, FLOAT& c_rad);

	// 角丸半径を計算する.
	// pos	対角点への位置ベクトル
	// d_rad	既定の角丸半径
	// c_rad	計算された角丸半径
	static void rrect_corner_radius(const D2D1_POINT_2F pos, const D2D1_POINT_2F d_rad, D2D1_POINT_2F& c_rad)
	{
		rrect_corner_radius(pos.x, d_rad.x, c_rad.x);
		rrect_corner_radius(pos.y, d_rad.y, c_rad.y);
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
		pt_add(m_start, min(m_pos.x, 0.0), min(m_pos.y, 0.0), r_lt);
		float rx = std::fabsf(m_corner_radius.x);
		float ry = std::fabsf(m_corner_radius.y);
		float vx = std::fabsf(m_pos.x);
		float vy = std::fabsf(m_pos.y);
		if (rx > vx * 0.5f) {
			rx = vx * 0.5f;
		}
		if (ry > vy * 0.5f) {
			ry = vy * 0.5f;
		}
		const D2D1_ROUNDED_RECT r_rec{
			{ r_lt.x, r_lt.y, r_lt.x + vx,  r_lt.y + vy },
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

		if (m_anc_show && is_selected()) {
			// 補助線を描く
			if (m_stroke_width >= Shape::m_anc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawRoundedRectangle(r_rec, brush, 2.0f * Shape::m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawRoundedRectangle(r_rec, brush, Shape::m_aux_width, m_aux_style.get());
			}
			// 角丸の中心点を描く.
			D2D1_POINT_2F circle[4]{	// 円上の点
				{ r_lt.x + rx, r_lt.y + ry },
				{ r_lt.x + vx - rx, r_lt.y + ry },
				{ r_lt.x + vx - rx, r_lt.y + vy - ry },
				{ r_lt.x + rx, r_lt.y + vy - ry }
			};
			anc_draw_circle(circle[2], target, brush);
			anc_draw_circle(circle[3], target, brush);
			anc_draw_circle(circle[1], target, brush);
			anc_draw_circle(circle[0], target, brush);
			// 図形の部位を描く.
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
		const double dx = m_pos.x;	// 差分 x
		const double dy = m_pos.y;	// 差分 y
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
	// test	判定される点
	// r_lt	角丸方形の左上位置
	// r_rb	角丸方形の右下位置
	// r_rad	角丸の半径
	// 戻り値	含まれるなら true を返す.
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
		D2D1_POINT_2F ctr;	// 角丸の中心点
		pt_add(r_lt, r_rad, ctr);
		if (test.x < ctr.x) {
			if (test.y < ctr.y) {
				return pt_in_ellipse(test, ctr, r_rad.x, r_rad.y);
			}
		}
		ctr.x = r_rb.x - r_rad.x;
		ctr.y = r_lt.y + r_rad.y;
		if (test.x > ctr.x) {
			if (test.y < ctr.y) {
				return pt_in_ellipse(test, ctr, r_rad.x, r_rad.y);
			}
		}
		ctr.x = r_rb.x - r_rad.x;
		ctr.y = r_rb.y - r_rad.y;
		if (test.x > ctr.x) {
			if (test.y > ctr.y) {
				return pt_in_ellipse(test, ctr, r_rad.x, r_rad.y);
			}
		}
		ctr.x = r_lt.x + r_rad.x;
		ctr.y = r_rb.y - r_rad.y;
		if (test.x < ctr.x) {
			if (test.y > ctr.y) {
				return pt_in_ellipse(test, ctr, r_rad.x, r_rad.y);
			}
		}
		return true;
	}

	// 図形が点を含むか判定する.
	// test	判定される点
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeRRect::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		// 角丸の円弧の中心点に含まれるか判定する.
		// +---------+
		// | 2     4 |
		// |         |
		// | 3     1 |
		// +---------+
		uint32_t anc_r;
		const double mx = m_pos.x * 0.5;	// 中間点
		const double my = m_pos.y * 0.5;	// 中間点
		const double rx = fabs(mx) < fabs(m_corner_radius.x) ? mx : m_corner_radius.x;	// 角丸
		const double ry = fabs(my) < fabs(m_corner_radius.y) ? my : m_corner_radius.y;	// 角丸
		const D2D1_POINT_2F anc_r_nw{
			static_cast<FLOAT>(m_start.x + rx), 
			static_cast<FLOAT>(m_start.y + ry)
		};
		const D2D1_POINT_2F anc_r_se{
			static_cast<FLOAT>(m_start.x + m_pos.x - rx),
			static_cast<FLOAT>(m_start.y + m_pos.y - ry)
		};
		const D2D1_POINT_2F anc_r_ne{ anc_r_se.x, anc_r_nw.y };
		const D2D1_POINT_2F anc_r_sw{ anc_r_nw.x, anc_r_se.y };
		if (anc_hit_test(test, anc_r_se, m_anc_width)) {
			anc_r = ANC_TYPE::ANC_R_SE;
		}
		else if (anc_hit_test(test, anc_r_nw, m_anc_width)) {
			anc_r = ANC_TYPE::ANC_R_NW;
		}
		else if (anc_hit_test(test, anc_r_sw, m_anc_width)) {
			anc_r = ANC_TYPE::ANC_R_SW;
		}
		else if (anc_hit_test(test, anc_r_ne, m_anc_width)) {
			anc_r = ANC_TYPE::ANC_R_NE;
		}
		else {
			anc_r = ANC_TYPE::ANC_PAGE;
		}

		// 角丸のいずれかの中心点に含まれる,
		if (anc_r != ANC_TYPE::ANC_PAGE &&
			// かつ, 方形の大きさが図形の部位の倍の大きさより大きいか判定する.
			fabs(m_pos.x) > m_anc_width && fabs(m_pos.y) > 2.0f * m_anc_width) {
			return anc_r;
		}
		const uint32_t anc_v = rect_hit_test_anc(m_start, m_pos, test, m_anc_width);
		if (anc_v != ANC_TYPE::ANC_PAGE) {
			return anc_v;
		}
		// 頂点に含まれず, 角丸の円弧の中心点に含まれるか判定する.
		else if (anc_r != ANC_TYPE::ANC_PAGE) {
			return anc_r;
		}

		D2D1_POINT_2F r_lt;	// 左上位置
		D2D1_POINT_2F r_rb;	// 右下位置
		D2D1_POINT_2F r_rad;	// 角丸の半径
		if (m_pos.x > 0.0f) {
			r_lt.x = m_start.x;
			r_rb.x = m_start.x + m_pos.x;
		}
		else {
			r_lt.x = m_start.x + m_pos.x;
			r_rb.x = m_start.x;
		}
		if (m_pos.y > 0.0f) {
			r_lt.y = m_start.y;
			r_rb.y = m_start.y + m_pos.y;
		}
		else {
			r_lt.y = m_start.y + m_pos.y;
			r_rb.y = m_start.y;
		}
		r_rad.x = std::abs(m_corner_radius.x);
		r_rad.y = std::abs(m_corner_radius.y);

		// 線枠が透明または太さ 0 か判定する.
		if (!is_opaque(m_stroke_color) || m_stroke_width < FLT_MIN) {
			// 塗りつぶし色が不透明, かつ角丸方形そのものに含まれるか判定する.
			if (is_opaque(m_fill_color) && pt_in_rrect(test, r_lt, r_rb, r_rad)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		// 線枠の色が不透明, かつ太さが 0 より大きい.
		else {
			// 拡大した角丸方形に含まれるか判定
			const double ew = max(m_stroke_width, m_anc_width);
			D2D1_POINT_2F e_lt, e_rb, e_rad;	// 拡大した角丸方形
			pt_add(r_lt, -ew * 0.5, e_lt);
			pt_add(r_rb, ew * 0.5, e_rb);
			pt_add(r_rad, ew * 0.5, e_rad);
			if (pt_in_rrect(test, e_lt, e_rb, e_rad)) {
				// 縮小した角丸方形が逆転してない, かつ位置が縮小した角丸方形に含まれるか判定する.
				D2D1_POINT_2F s_lt, s_rb, s_rad;	// 縮小した角丸方形
				pt_add(e_lt, ew, s_lt);
				pt_add(e_rb, -ew, s_rb);
				pt_add(e_rad, -ew, s_rad);
				if (s_lt.x < s_rb.x && s_lt.y < s_rb.y && pt_in_rrect(test, s_lt, s_rb, s_rad)) {
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
	// snap_point	他の点との間隔 (この値より離れた点は無視する)
	bool ShapeRRect::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float snap_point, const bool /*keep_aspect*/) noexcept
	{
		D2D1_POINT_2F a;	// 図形の部位の位置
		D2D1_POINT_2F p;	// 位置ベクトル
		D2D1_POINT_2F r;	// 角丸半径
		D2D1_POINT_2F q;	// 新しい点

		switch (anc) {
		case ANC_TYPE::ANC_R_NW:
			ShapeRRect::get_pos_anc(anc, a);
			pt_round(val, PT_ROUND, q);
			pt_sub(q, a, p);
			if (pt_abs2(p) < FLT_MIN) {
				return false;
			}
			pt_add(m_corner_radius, p, r);
			rrect_corner_radius(m_pos, r, m_corner_radius);
			break;
		case ANC_TYPE::ANC_R_NE:
			ShapeRRect::get_pos_anc(anc, a);
			pt_round(val, PT_ROUND, q);
			pt_sub(q, a, p);
			if (pt_abs2(p) < FLT_MIN) {
				return false;
			}
			r.x = m_corner_radius.x - p.x;
			r.y = m_corner_radius.y + p.y;
			rrect_corner_radius(m_pos, r, m_corner_radius);
			break;
		case ANC_TYPE::ANC_R_SE:
			ShapeRRect::get_pos_anc(anc, a);
			pt_round(val, PT_ROUND, q);
			pt_sub(q, a, p);
			if (pt_abs2(p) < FLT_MIN) {
				return false;
			}
			r.x = m_corner_radius.x - p.x;
			r.y = m_corner_radius.y - p.y;
			rrect_corner_radius(m_pos, r, m_corner_radius);
			break;
		case ANC_TYPE::ANC_R_SW:
			ShapeRRect::get_pos_anc(anc, a);
			pt_round(val, PT_ROUND, q);
			pt_sub(q, a, p);
			if (pt_abs2(p) < FLT_MIN) {
				return false;
			}
			r.x = m_corner_radius.x + p.x;
			r.y = m_corner_radius.y - p.y;
			rrect_corner_radius(m_pos, r, m_corner_radius);
			break;
		default:
			if (!ShapeRect::set_pos_anc(val, anc, snap_point, false)) {
				return false;
			}
			if (m_pos.x * m_corner_radius.x < 0.0f) {
				m_corner_radius.x = -m_corner_radius.x;
			}
			if (m_pos.y * m_corner_radius.y < 0.0f) {
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
	// pos	対角点への位置ベクトル
	// page	属性
	ShapeRRect::ShapeRRect(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page) :
		ShapeRect::ShapeRect(start, pos, page)
	{
		float g_base;
		page->get_grid_base(g_base);
		rrect_corner_radius(pos, D2D1_POINT_2F{ g_base + 1.0f, g_base + 1.0f }, m_corner_radius);
	}

	// 図形をデータリーダーから読み込む.
	ShapeRRect::ShapeRRect(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		m_corner_radius = D2D1_POINT_2F{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (m_corner_radius.x < 0.0f || m_corner_radius.x > 0.5f * fabs(m_pos.x)) {
			m_corner_radius.x = 0.0f;
		}
		if (m_corner_radius.y < 0.0f || m_corner_radius.y > 0.5f * fabs(m_pos.y)) {
			m_corner_radius.y = 0.0f;
		}
	}

	// 図形をデータライターに書き込む.
	void ShapeRRect::write(DataWriter const& dt_writer) const
	{
		ShapeRect::write(dt_writer);
		dt_writer.WriteSingle(m_corner_radius.x);
		dt_writer.WriteSingle(m_corner_radius.y);
	}

}