#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	// 図形を表示する.
	void ShapeOblong::draw_loc(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		// 部位を表示する.
		// 0---1
		// |   |
		// 3---2
		D2D1_POINT_2F a_pos[4]{	// 方形の頂点
			{ m_start.x, m_start.y },
			{ m_start.x + m_pos.x, m_start.y },
			{ m_start.x + m_pos.x, m_start.y + m_pos.y },
			{ m_start.x, m_start.y + m_pos.y }
		};
		D2D1_POINT_2F a_mid;	// 方形の辺の中点
		pt_avg(a_pos[0], a_pos[3], a_mid);
		loc_draw_square(a_mid, target, brush);
		pt_avg(a_pos[0], a_pos[1], a_mid);
		loc_draw_square(a_mid, target, brush);
		pt_avg(a_pos[1], a_pos[2], a_mid);
		loc_draw_square(a_mid, target, brush);
		pt_avg(a_pos[2], a_pos[3], a_mid);
		loc_draw_square(a_mid, target, brush);
		loc_draw_square(a_pos[0], target, brush);
		loc_draw_square(a_pos[1], target, brush);
		loc_draw_square(a_pos[3], target, brush);
		loc_draw_square(a_pos[2], target, brush);
	}

	// 図形を表示する.
	void ShapeOblong::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		if (m_d2d_stroke_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			create_stroke_style(factory);
		}

		const D2D1_RECT_F rect{
			m_start.x,
			m_start.y,
			m_start.x + m_pos.x,
			m_start.y + m_pos.y
		};
		// 塗りつぶし色が不透明か判定する.
		if (is_opaque(m_fill_color)) {
			// 方形を塗りつぶす.
			brush->SetColor(m_fill_color);
			target->FillRectangle(rect, brush);
		}
		// 線枠の色が不透明か判定する.
		if (is_opaque(m_stroke_color)) {
			// 方形の枠を表示する.
			const auto w = m_stroke_width;
			brush->SetColor(m_stroke_color);
			target->DrawRectangle(rect, brush, w, m_d2d_stroke_style.get());
		}
		// この図形が選択されてるか判定する.
		if (m_loc_show && is_selected()) {
			// 補助線を描く
			if (m_stroke_width >= Shape::m_loc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawRectangle(rect, brush, 2.0f * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawRectangle(rect, brush, m_aux_width, m_aux_style.get());
			}
			// 図形の部位を描く.
			draw_loc();
		}
	}

	// 点が, 方形の頂点と中間点のうち, どの部位に含まれるかを判定する.
	uint32_t rect_loc_hit_test(
		const D2D1_POINT_2F start,	// 始点
		const D2D1_POINT_2F pos,	// 対角への位置ベクトル
		const D2D1_POINT_2F t,	// 判定される点
		const double a_len	// 図形の部位の大きさ
	) noexcept
	{
		// 4----8----2
		// |         |
		// 7         6
		// |         |
		// 3----5----1
		const D2D1_POINT_2F se{
			start.x + pos.x, start.y + pos.y
		};
		if (loc_hit_test(t, se, a_len)) {
			return LOC_TYPE::LOC_SE;
		}
		const D2D1_POINT_2F ne{
			se.x, start.y
		};
		if (loc_hit_test(t, ne, a_len)) {
			return LOC_TYPE::LOC_NE;
		}
		const D2D1_POINT_2F sw{
			start.x, se.y
		};
		if (loc_hit_test(t, sw, a_len)) {
			return LOC_TYPE::LOC_SW;
		}
		if (loc_hit_test(t, start, a_len)) {
			return LOC_TYPE::LOC_NW;
		}
		const D2D1_POINT_2F s{
			static_cast<FLOAT>(start.x + pos.x * 0.5), se.y
		};
		if (loc_hit_test(t, s, a_len)) {
			return LOC_TYPE::LOC_SOUTH;
		}
		const D2D1_POINT_2F e{
			se.x, static_cast<FLOAT>(start.y + pos.y * 0.5)
		};
		if (loc_hit_test(t, e, a_len)) {
			return LOC_TYPE::LOC_EAST;
		}
		const D2D1_POINT_2F w{
			start.x, e.y
		};
		if (loc_hit_test(t, w, a_len)) {
			return LOC_TYPE::LOC_WEST;
		}
		const D2D1_POINT_2F n{
			s.x, start.y
		};
		if (loc_hit_test(t, n, a_len)) {
			return LOC_TYPE::LOC_NORTH;
		}
		return LOC_TYPE::LOC_PAGE;
	}

	// 図形が点を含むか判定する.
	// 戻り値	点を含む部位
	uint32_t ShapeOblong::hit_test(
		const D2D1_POINT_2F t	// 判定される点
	) const noexcept
	{
		D2D1_POINT_2F p[4]{ m_start, };	// 頂点の配列

		p[2].x = m_start.x + m_pos.x;
		p[2].y = m_start.y + m_pos.y;
		if (loc_hit_test(t, p[2], m_loc_width)) {
			return LOC_TYPE::LOC_SE;
		}
		p[3].x = m_start.x;
		p[3].y = m_start.y + m_pos.y;
		if (loc_hit_test(t, p[3], m_loc_width)) {
			return LOC_TYPE::LOC_SW;
		}
		p[1].x = m_start.x + m_pos.x;
		p[1].y = m_start.y;
		if (loc_hit_test(t, p[1], m_loc_width)) {
			return LOC_TYPE::LOC_NE;
		}
		if (loc_hit_test(t, p[0], m_loc_width)) {
			return LOC_TYPE::LOC_NW;
		}

		// 各辺の中点の部位に含まれるか判定する.
		D2D1_POINT_2F bottom;
		pt_avg(p[2], p[3], bottom);
		if (loc_hit_test(t, bottom, m_loc_width)) {
			return LOC_TYPE::LOC_SOUTH;
		}
		D2D1_POINT_2F right;
		pt_avg(p[1], p[2], right);
		if (loc_hit_test(t, right, m_loc_width)) {
			return LOC_TYPE::LOC_EAST;
		}
		D2D1_POINT_2F left;
		pt_avg(p[0], p[3], left);
		if (loc_hit_test(t, left, m_loc_width)) {
			return LOC_TYPE::LOC_WEST;
		}
		D2D1_POINT_2F top;
		pt_avg(p[0], p[1], top);
		if (loc_hit_test(t, top, m_loc_width)) {
			return LOC_TYPE::LOC_NORTH;
		}

		// 対角にある頂点をもとに, 方形を得る.
		D2D1_POINT_2F r_lt, r_rb;
		if (p[0].x < p[2].x) {
			r_lt.x = p[0].x;
			r_rb.x = p[2].x;
		}
		else {
			r_lt.x = p[2].x;
			r_rb.x = p[0].x;
		}
		if (p[0].y < p[2].y) {
			r_lt.y = p[0].y;
			r_rb.y = p[2].y;
		}
		else {
			r_lt.y = p[2].y;
			r_rb.y = p[0].y;
		}

		if (!is_opaque(m_stroke_color) || m_stroke_width < FLT_MIN) {
			if (is_opaque(m_fill_color) && pt_in_rect2(t, r_lt, r_rb)) {
				return LOC_TYPE::LOC_FILL;
			}
		}
		else {
			//      |                |
			//    +-+----------------+-+
			// ---+-+----------------+-+---
			//    | |                | |
			//    | |                | |
			// ---+-+----------------+-+---
			//    +-+----------------+-+
			//      |                |
			// 線枠の太さの半分の大きさだけ外側に, 方形を拡大する.
			// ただし太さが部位の大きさ未満なら, 太さは部位の大きさに調整する.
			D2D1_POINT_2F e_lb, e_rb;	// 拡大した方形
			const double s_thick = max(m_stroke_width, m_loc_width);
			const double e_thick = s_thick * 0.5;
			pt_add(r_lt, -e_thick, e_lb);
			pt_add(r_rb, e_thick, e_rb);
			// 拡大した方形に含まれるか判定する.
			if (pt_in_rect2(t, e_lb, e_rb)) {
				// 太さの大きさだけ内側に, 拡大した方形を縮小する.
				D2D1_POINT_2F s_lb, s_rb;	// 縮小した方形
				pt_add(e_lb, s_thick, s_lb);
				pt_add(e_rb, -s_thick, s_rb);
				// 縮小した方形に含まれる (辺に含まれない) か判定する.
				if (pt_in_rect2(t, s_lb, s_rb)) {
					if (is_opaque(m_fill_color)) {
						return LOC_TYPE::LOC_FILL;
					}
				}
				// 縮小した方形が反転する (枠が太すぎて図形を覆う),
				// または, 方形の角に含まれてない (辺に含まれる) か判定する.
				else if (s_rb.x <= s_lb.x || s_rb.y <= s_lb.y ||
					r_lt.x <= t.x && t.x <= r_rb.x || r_lt.y <= t.y && t.y <= r_rb.y) {
					return LOC_TYPE::LOC_STROKE;
				}
				// 線枠の結合が丸めか判定する.
				else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
					if (pt_in_circle(t, p[0], e_thick) ||
						pt_in_circle(t, p[1], e_thick) ||
						pt_in_circle(t, p[2], e_thick) ||
						pt_in_circle(t, p[3], e_thick)) {
						return LOC_TYPE::LOC_STROKE;
					}
				}
				// 線枠の結合が面取り, または, 尖り・面取りでかつ尖り制限が√2 未満か判定する.
				else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL ||
					(m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL &&
						m_join_miter_limit < M_SQRT2)) {
					const auto limit = static_cast<FLOAT>(e_thick);
					const D2D1_POINT_2F q[4]{
						D2D1_POINT_2F{ 0.0f, -limit }, D2D1_POINT_2F{ limit, 0.0f }, 
						D2D1_POINT_2F{ 0.0f, limit }, D2D1_POINT_2F{ -limit, 0.0f }
					};
					if (pt_in_poly(D2D1_POINT_2F{ t.x - p[0].x, t.y - p[0].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ t.x - p[1].x, t.y - p[1].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ t.x - p[2].x, t.y - p[2].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ t.x - p[3].x, t.y - p[3].y }, 4, q)) {
						return LOC_TYPE::LOC_STROKE;
					}
				}
				// 線枠の結合が尖り, または, 尖り/面取りでかつ尖り制限が√2 以上か判定する.
				else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
					(m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL &&
						m_join_miter_limit >= M_SQRT2)) {
					const auto limit =
						static_cast<FLOAT>(M_SQRT2 * 0.5 * m_stroke_width * m_join_miter_limit);
					const D2D1_POINT_2F q[4]{
						D2D1_POINT_2F{ 0.0f, -limit }, D2D1_POINT_2F{ limit, 0.0f },
						D2D1_POINT_2F{ 0.0f, limit }, D2D1_POINT_2F{ -limit, 0.0f }
					};
					if (pt_in_poly(D2D1_POINT_2F{ t.x - p[0].x, t.y - p[0].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ t.x - p[1].x, t.y - p[1].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ t.x - p[2].x, t.y - p[2].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ t.x - p[3].x, t.y - p[3].y }, 4, q)) {
						return LOC_TYPE::LOC_STROKE;
					}
				}
			}
		}
		return LOC_TYPE::LOC_PAGE;
	}

	// 境界矩形を得る.
	// a_lt	元の領域の左上位置.
	// a_rb	元の領域の右下位置.
	// b_lt	囲む領域の左上位置.
	// b_rb	囲む領域の右下位置.
	void ShapeOblong::get_bbox(
		const D2D1_POINT_2F a_lt,
		const D2D1_POINT_2F a_rb,
		D2D1_POINT_2F& b_lt,
		D2D1_POINT_2F& b_rb
	) const noexcept
	{
		b_lt.x = m_start.x < a_lt.x ? m_start.x : a_lt.x;
		b_lt.y = m_start.y < a_lt.y ? m_start.y : a_lt.y;
		b_rb.x = m_start.x > a_rb.x ? m_start.x : a_rb.x;
		b_rb.y = m_start.y > a_rb.y ? m_start.y : a_rb.y;
		D2D1_POINT_2F end;
		pt_add(m_start, m_pos, end);
		if (end.x < b_lt.x) {
			b_lt.x = end.x;
		}
		if (end.x > b_rb.x) {
			b_rb.x = end.x;
		}
		if (end.y < b_lt.y) {
			b_lt.y = end.y;
		}
		if (end.y > b_rb.y) {
			b_rb.y = end.y;
		}
	}

	// 塗りつぶし色を得る.
	// val	得られた値.
	bool ShapeOblong::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}

	// 塗りつぶし色に格納する.
	// val	格納する値.
	bool ShapeOblong::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_fill_color, val)) {
			m_fill_color = val;
			return true;
		}
		return false;
	}

	// 指定した部位の点を得る.
	void ShapeOblong::get_pos_loc(
		const uint32_t loc,	// 部位
		D2D1_POINT_2F& val	// 得られた値
	) const noexcept
	{
		switch (loc) {
		case LOC_TYPE::LOC_NORTH:
			val.x = m_start.x + m_pos.x * 0.5f;
			val.y = m_start.y;
			break;
		case LOC_TYPE::LOC_NE:
			val.x = m_start.x + m_pos.x;
			val.y = m_start.y;
			break;
		case LOC_TYPE::LOC_WEST:
			val.x = m_start.x;
			val.y = m_start.y + m_pos.y * 0.5f;
			break;
		case LOC_TYPE::LOC_EAST:
			val.x = m_start.x + m_pos.x;
			val.y = m_start.y + m_pos.y * 0.5f;
			break;
		case LOC_TYPE::LOC_SW:
			val.x = m_start.x;
			val.y = m_start.y + m_pos.y;
			break;
		case LOC_TYPE::LOC_SOUTH:
			val.x = m_start.x + m_pos.x * 0.5f;
			val.y = m_start.y + m_pos.y;
			break;
		case LOC_TYPE::LOC_SE:
			val.x = m_start.x + m_pos.x;
			val.y = m_start.y + m_pos.y;
			break;
		default:
			val = m_start;
			break;
		}
	}

	// 境界矩形の左上点を得る.
	void ShapeOblong::get_bbox_lt(D2D1_POINT_2F& val) const noexcept
	{
		val.x = (0.0f <= m_pos.x ? m_start.x : m_start.x + m_pos.x);
		val.y = (0.0f <= m_pos.y ? m_start.y : m_start.y + m_pos.y);
	}

	// 始点を得る
	// val	始点
	// 戻り値	つねに true
	bool ShapeOblong::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		val = m_start;
		return true;
	}

	// 矩形に含まれるか判定する.
	// lt	矩形の左上位置
	// rb	矩形の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeOblong::is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		D2D1_POINT_2F pos;
		pt_add(m_start, m_pos, pos);
		return pt_in_rect(m_start, lt, rb) && pt_in_rect(pos, lt, rb);
	}

	// 位置を移動する.
	// pos	位置ベクトル
	bool ShapeOblong::move(const D2D1_POINT_2F pos) noexcept
	{
		D2D1_POINT_2F start;
		pt_add(m_start, pos, start);
		if (set_pos_start(start)) {
			return true;
		}
		return false;
	}

	// 値を, 指定した部位の点に格納する.
	bool ShapeOblong::set_pos_loc(
		const D2D1_POINT_2F val,	// 値
		const uint32_t loc,	// 部位
		const float snap_point,
		const bool /*keep_aspect*/) noexcept
	{
		bool done = false;
		switch (loc) {
		case LOC_TYPE::LOC_PAGE:
		{
			D2D1_POINT_2F pos;
			pt_round(val, PT_ROUND, pos);
			D2D1_POINT_2F vec;
			pt_sub(pos, m_start, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				pt_add(m_start, vec, m_start);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_NW:
		{
			D2D1_POINT_2F pos;
			pt_round(val, PT_ROUND, pos);
			D2D1_POINT_2F vec;
			pt_sub(pos, m_start, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				pt_add(m_start, vec, m_start);
				pt_sub(m_pos, vec, m_pos);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_SE:
		{
			D2D1_POINT_2F pos;
			pt_round(val, PT_ROUND, pos);
			D2D1_POINT_2F vec;
			pt_sub(pos, D2D1_POINT_2F{ m_start.x + m_pos.x, m_start.y + m_pos.y }, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				pt_add(m_pos, vec, m_pos);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_NE:
		{
			D2D1_POINT_2F pos;
			pt_round(val, PT_ROUND, pos);
			D2D1_POINT_2F vec;
			pt_sub(pos, D2D1_POINT_2F{ m_start.x + m_pos.x, m_start.y }, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				m_start.y += vec.y;
				pt_add(m_pos, vec.x, -vec.y, m_pos);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_SW:
		{
			D2D1_POINT_2F p;
			pt_round(val, PT_ROUND, p);
			D2D1_POINT_2F vec;
			pt_sub(p, D2D1_POINT_2F{ m_start.x, m_start.y + m_pos.y }, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				m_start.x += vec.x;
				pt_add(m_pos, -vec.x, vec.y, m_pos);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_WEST:
		{
			const double vec_x = std::round((static_cast<double>(val.x) - m_start.x) / PT_ROUND) * PT_ROUND;
			if (vec_x <= -FLT_MIN || vec_x >= FLT_MIN) {
				m_pos.x = static_cast<FLOAT>(m_pos.x - vec_x);
				m_start.x = static_cast<FLOAT>(m_start.x + vec_x);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_EAST:
		{
			const double vec_x = std::round((static_cast<double>(val.x) - m_start.x - m_pos.x) / PT_ROUND) * PT_ROUND;
			if (vec_x <= -FLT_MIN || vec_x >= FLT_MIN) {
				m_pos.x += static_cast<FLOAT>(vec_x);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_NORTH:
		{
			const double vec_y = std::round((static_cast<double>(val.y) - m_start.y) / PT_ROUND) * PT_ROUND;
			if (vec_y <= -FLT_MIN || vec_y >= FLT_MIN) {
				m_pos.y = static_cast<FLOAT>(m_pos.y - vec_y);
				m_start.y = static_cast<FLOAT>(m_start.y + vec_y);
				done = true;
			}
		}
		break;
		case LOC_TYPE::LOC_SOUTH:
		{
			const double vec_y = std::round(
				(static_cast<double>(val.y) - m_start.y - m_pos.y) / PT_ROUND) * PT_ROUND;
			if (vec_y <= -FLT_MIN || vec_y >= FLT_MIN) {
				m_pos.y += static_cast<FLOAT>(vec_y);
				done = true;
			}
		}
		break;
		default:
			return false;
		}
		if (snap_point >= FLT_MIN) {
			// 終点への差分の x 値が, 限界距離未満か判定する.
			if (m_pos.x > -snap_point && m_pos.x < snap_point) {
				if (loc == LOC_TYPE::LOC_NE) {
					m_start.x += m_pos.x;
				}
				m_pos.x = 0.0f;
				done = true;
			}
			if (m_pos.y > -snap_point && m_pos.y < snap_point) {
				if (loc == LOC_TYPE::LOC_NE) {
					m_start.y += m_pos.y;
				}
				m_pos.y = 0.0f;
				done = true;
			}
		}
		return done;
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	bool ShapeOblong::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F p;	// 丸めた値.
		pt_round(val, PT_ROUND, p);
		if (!equal(m_start, p)) {
			m_start = p;
			return true;
		}
		return false;
	}

	// 図形を作成する.
	// start	始点
	// pos	対角点への位置ベクトル
	// page	属性
	ShapeOblong::ShapeOblong(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop) :
		ShapeStroke(prop),
		m_start(start),
		m_pos(pos)
	{
		prop->get_fill_color(m_fill_color);
	}

	// 図形をデータリーダーから読み込む.
	ShapeOblong::ShapeOblong(DataReader const& dt_reader) :
		ShapeStroke(dt_reader)
	{
		m_start.x = dt_reader.ReadSingle();
		m_start.y = dt_reader.ReadSingle();
		m_pos.x = dt_reader.ReadSingle();
		m_pos.y = dt_reader.ReadSingle();
		const auto r = dt_reader.ReadSingle();
		const auto g = dt_reader.ReadSingle();
		const auto b = dt_reader.ReadSingle();
		const auto a = dt_reader.ReadSingle();
		m_fill_color.r = min(max(r, 0.0f), 1.0f);
		m_fill_color.g = min(max(g, 0.0f), 1.0f);
		m_fill_color.b = min(max(b, 0.0f), 1.0f);
		m_fill_color.a = min(max(a, 0.0f), 1.0f);
	}

	// 図形をデータライターに書き込む.
	void ShapeOblong::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);

		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);
		dt_writer.WriteSingle(m_pos.x);
		dt_writer.WriteSingle(m_pos.y);
		dt_writer.WriteSingle(m_fill_color.r);
		dt_writer.WriteSingle(m_fill_color.g);
		dt_writer.WriteSingle(m_fill_color.b);
		dt_writer.WriteSingle(m_fill_color.a);
	}

	// 近傍の頂点を見つける.
	// pos	ある位置
	// dd	近傍とみなす距離 (の二乗値), これより離れた頂点は近傍とはみなさない.
	// val	ある位置の近傍にある頂点
	// 戻り値	見つかったら true
	bool ShapeOblong::get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool flag = false;
		D2D1_POINT_2F q[4];
		const size_t q_cnt = get_verts(q);
		for (size_t i = 0; i < q_cnt; i++) {
			D2D1_POINT_2F vec;
			pt_sub(q[i], pos, vec);
			const float vv = static_cast<float>(pt_abs2(vec));
			if (vv < dd) {
				dd = vv;
				val = q[i];
				if (!flag) {
					flag = true;
				}
			}
		}
		return flag;
	}

	// 頂点を得る.
	// p	頂点を格納する配列
	// 戻り値	頂点の個数
	size_t ShapeOblong::get_verts(D2D1_POINT_2F p[]) const noexcept
	{
		// 左上
		p[0] = m_start;
		// 右上
		p[1].x = m_start.x + m_pos.x;
		p[1].y = m_start.y;
		// 右下
		p[2].x = p[1].x;
		p[2].y = m_start.y + m_pos.y;
		// 右下
		p[3].x = m_start.x;
		p[3].y = p[2].y;
		return 4;
	}

}