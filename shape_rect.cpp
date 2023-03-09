#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	// 図形を表示する.
	void ShapeRect::draw_anc(void)
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
		anc_draw_square(a_mid, target, brush);
		pt_avg(a_pos[0], a_pos[1], a_mid);
		anc_draw_square(a_mid, target, brush);
		pt_avg(a_pos[1], a_pos[2], a_mid);
		anc_draw_square(a_mid, target, brush);
		pt_avg(a_pos[2], a_pos[3], a_mid);
		anc_draw_square(a_mid, target, brush);
		anc_draw_square(a_pos[0], target, brush);
		anc_draw_square(a_pos[1], target, brush);
		anc_draw_square(a_pos[3], target, brush);
		anc_draw_square(a_pos[2], target, brush);
	}

	// 図形を表示する.
	void ShapeRect::draw(void)
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
		if (m_anc_show && is_selected()) {
			// 補助線を描く
			if (m_stroke_width >= Shape::m_anc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawRectangle(rect, brush, 2.0 * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawRectangle(rect, brush, m_aux_width, m_aux_style.get());
			}
			// 図形の部位を描く.
			draw_anc();
		}
	}

	// 方形の頂点とそれらの中間点のうち, どの部位に含まれるかを判定する.
	uint32_t rect_hit_test_anc(const D2D1_POINT_2F start, const D2D1_POINT_2F vec,
		const D2D1_POINT_2F test, const double a_len) noexcept
	{
		// 4----8----2
		// |         |
		// 7         6
		// |         |
		// 3----5----1
		const D2D1_POINT_2F anc_se{ start.x + vec.x, start.y + vec.y };
		if (pt_in_anc(test, anc_se, a_len)) {
			return ANC_TYPE::ANC_SE;
		}
		const D2D1_POINT_2F anc_ne{ anc_se.x, start.y };
		if (pt_in_anc(test, anc_ne, a_len)) {
			return ANC_TYPE::ANC_NE;
		}
		const D2D1_POINT_2F anc_sw{ start.x, anc_se.y };
		if (pt_in_anc(test, anc_sw, a_len)) {
			return ANC_TYPE::ANC_SW;
		}
		if (pt_in_anc(test, start, a_len)) {
			return ANC_TYPE::ANC_NW;
		}
		const D2D1_POINT_2F anc_s{ static_cast<FLOAT>(start.x + vec.x * 0.5), anc_se.y };
		if (pt_in_anc(test, anc_s, a_len)) {
			return ANC_TYPE::ANC_SOUTH;
		}
		const D2D1_POINT_2F anc_e{ anc_se.x, static_cast<FLOAT>(start.y + vec.y * 0.5f) };
		if (pt_in_anc(test, anc_e, a_len)) {
			return ANC_TYPE::ANC_EAST;
		}
		const D2D1_POINT_2F anc_w{ start.x, anc_e.y };
		if (pt_in_anc(test, anc_w, a_len)) {
			return ANC_TYPE::ANC_WEST;
		}
		const D2D1_POINT_2F anc_n{ anc_s.x, start.y };
		if (pt_in_anc(test, anc_n, a_len)) {
			return ANC_TYPE::ANC_NORTH;
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// 位置を含むか判定する.
	// t_pos	判定される位置
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeRect::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		// 各頂点の部位に含まれるか判定する.
		D2D1_POINT_2F p[4]{ m_start, };	// 頂点の配列

		p[2].x = m_start.x + m_pos.x;
		p[2].y = m_start.y + m_pos.y;
		if (pt_in_anc(test, p[2], m_anc_width)) {
			return ANC_TYPE::ANC_SE;
		}
		p[3].x = m_start.x;
		p[3].y = m_start.y + m_pos.y;
		if (pt_in_anc(test, p[3], m_anc_width)) {
			return ANC_TYPE::ANC_SW;
		}
		p[1].x = m_start.x + m_pos.x;
		p[1].y = m_start.y;
		if (pt_in_anc(test, p[1], m_anc_width)) {
			return ANC_TYPE::ANC_NE;
		}
		if (pt_in_anc(test, p[0], m_anc_width)) {
			return ANC_TYPE::ANC_NW;
		}

		// 各辺の中点の部位に含まれるか判定する.
		D2D1_POINT_2F bottom;
		pt_avg(p[2], p[3], bottom);
		if (pt_in_anc(test, bottom, m_anc_width)) {
			return ANC_TYPE::ANC_SOUTH;
		}
		D2D1_POINT_2F right;
		pt_avg(p[1], p[2], right);
		if (pt_in_anc(test, right, m_anc_width)) {
			return ANC_TYPE::ANC_EAST;
		}
		D2D1_POINT_2F left;
		pt_avg(p[0], p[3], left);
		if (pt_in_anc(test, left, m_anc_width)) {
			return ANC_TYPE::ANC_WEST;
		}
		D2D1_POINT_2F top;
		pt_avg(p[0], p[1], top);
		if (pt_in_anc(test, top, m_anc_width)) {
			return ANC_TYPE::ANC_NORTH;
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
			if (is_opaque(m_fill_color) && pt_in_rect2(test, r_lt, r_rb)) {
				return ANC_TYPE::ANC_FILL;
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
			// ただし太さがアンカーポイントの大きさ未満なら, 太さはアンカーポイントの大きさに調整する.
			D2D1_POINT_2F e_lb, e_rb;	// 拡大した方形
			const double s_thick = max(m_stroke_width, m_anc_width);
			const double e_thick = s_thick * 0.5;
			pt_add(r_lt, -e_thick, e_lb);
			pt_add(r_rb, e_thick, e_rb);
			// 拡大した方形に含まれるか判定する.
			if (pt_in_rect2(test, e_lb, e_rb)) {
				// 太さの大きさだけ内側に, 拡大した方形を縮小する.
				D2D1_POINT_2F s_lb, s_rb;	// 縮小した方形
				pt_add(e_lb, s_thick, s_lb);
				pt_add(e_rb, -s_thick, s_rb);
				// 縮小した方形に含まれる (辺に含まれない) か判定する.
				if (pt_in_rect2(test, s_lb, s_rb)) {
					if (is_opaque(m_fill_color)) {
						return ANC_TYPE::ANC_FILL;
					}
				}
				// 縮小した方形が反転する (枠が太すぎて図形を覆う),
				// または, 方形の角に含まれてない (辺に含まれる) か判定する.
				else if (s_rb.x <= s_lb.x || s_rb.y <= s_lb.y ||
					r_lt.x <= test.x && test.x <= r_rb.x || r_lt.y <= test.y && test.y <= r_rb.y) {
					return ANC_TYPE::ANC_STROKE;
				}
				// 線枠の結合が丸めか判定する.
				else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
					if (pt_in_circle(test, p[0], e_thick) ||
						pt_in_circle(test, p[1], e_thick) ||
						pt_in_circle(test, p[2], e_thick) ||
						pt_in_circle(test, p[3], e_thick)) {
						return ANC_TYPE::ANC_STROKE;
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
					if (pt_in_poly(D2D1_POINT_2F{ test.x - p[0].x, test.y - p[0].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ test.x - p[1].x, test.y - p[1].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ test.x - p[2].x, test.y - p[2].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ test.x - p[3].x, test.y - p[3].y }, 4, q)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				// 線枠の結合が尖り, または, 尖り/面取りでかつ尖り制限が√2 以上か判定する.
				else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
					(m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL &&
						m_join_miter_limit >= M_SQRT2)) {
					const auto limit = static_cast<FLOAT>(M_SQRT2 * 0.5 * m_stroke_width * 
						m_join_miter_limit);
					const D2D1_POINT_2F q[4]{
						D2D1_POINT_2F{ 0.0f, -limit }, D2D1_POINT_2F{ limit, 0.0f },
						D2D1_POINT_2F{ 0.0f, limit }, D2D1_POINT_2F{ -limit, 0.0f }
					};
					if (pt_in_poly(D2D1_POINT_2F{ test.x - p[0].x, test.y - p[0].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ test.x - p[1].x, test.y - p[1].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ test.x - p[2].x, test.y - p[2].y }, 4, q) ||
						pt_in_poly(D2D1_POINT_2F{ test.x - p[3].x, test.y - p[3].y }, 4, q)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// 図形を囲む領域を得る.
	// a_lt	元の領域の左上位置.
	// a_rb	元の領域の右下位置.
	// b_lt	囲む領域の左上位置.
	// b_rb	囲む領域の右下位置.
	void ShapeRect::get_bound(
		const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt,
		D2D1_POINT_2F& b_rb) const noexcept
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
	bool ShapeRect::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}

	// 塗りつぶし色に格納する.
	// val	格納する値.
	bool ShapeRect::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_fill_color, val)) {
			m_fill_color = val;
			return true;
		}
		return false;
	}

	//	部位の位置を得る.
	//	anc	図形の部位.
	//	val	得られた位置.
	//	戻り値	なし
	void ShapeRect::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		switch (anc) {
		case ANC_TYPE::ANC_NORTH:
			val.x = m_start.x + m_pos.x * 0.5f;
			val.y = m_start.y;
			break;
		case ANC_TYPE::ANC_NE:
			val.x = m_start.x + m_pos.x;
			val.y = m_start.y;
			break;
		case ANC_TYPE::ANC_WEST:
			val.x = m_start.x;
			val.y = m_start.y + m_pos.y * 0.5f;
			break;
		case ANC_TYPE::ANC_EAST:
			val.x = m_start.x + m_pos.x;
			val.y = m_start.y + m_pos.y * 0.5f;
			break;
		case ANC_TYPE::ANC_SW:
			val.x = m_start.x;
			val.y = m_start.y + m_pos.y;
			break;
		case ANC_TYPE::ANC_SOUTH:
			val.x = m_start.x + m_pos.x * 0.5f;
			val.y = m_start.y + m_pos.y;
			break;
		case ANC_TYPE::ANC_SE:
			val.x = m_start.x + m_pos.x;
			val.y = m_start.y + m_pos.y;
			break;
		default:
			val = m_start;
			break;
		}
	}

	// 図形を囲む領域の左上位置を得る.
	// val	領域の左上位置
	void ShapeRect::get_bound_lt(D2D1_POINT_2F& val) const noexcept
	{
		val.x = (0.0f <= m_pos.x ? m_start.x : m_start.x + m_pos.x);
		val.y = (0.0f <= m_pos.y ? m_start.y : m_start.y + m_pos.y);
	}

	// 開始位置を得る
	// 戻り値	つねに true
	bool ShapeRect::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		val = m_start;
		return true;
	}

	// 矩形範囲に含まれるか判定する.
	// lt	矩形の左上位置
	// rb	矩形の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeRect::in_area(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		D2D1_POINT_2F pos;
		pt_add(m_start, m_pos, pos);
		return pt_in_rect(m_start, lt, rb) && pt_in_rect(pos, lt, rb);
	}

	// 位置を移動する.
	// pos	位置ベクトル
	bool ShapeRect::move(const D2D1_POINT_2F pos) noexcept
	{
		D2D1_POINT_2F start;
		pt_add(m_start, pos, start);
		if (set_pos_start(start)) {
			return true;
		}
		return false;
	}

	// 値を, 部位の位置に格納する. 他の部位の位置も動く.
	// val	値
	// anc	図形の部位
	// limit	他の頂点との限界距離 (他の頂点との距離がこの値未満になるなら, その頂点に位置に合わせる)
	bool ShapeRect::set_pos_anc(
		const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool /*keep_aspect*/)
		noexcept
	{
		bool done = false;
		switch (anc) {
		case ANC_TYPE::ANC_PAGE:
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
		case ANC_TYPE::ANC_NW:
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
		case ANC_TYPE::ANC_SE:
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
		case ANC_TYPE::ANC_NE:
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
		case ANC_TYPE::ANC_SW:
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
		case ANC_TYPE::ANC_WEST:
		{
			const double vec_x = std::round(
				(static_cast<double>(val.x) - m_start.x) / PT_ROUND) * PT_ROUND;
			if (vec_x <= -FLT_MIN || vec_x >= FLT_MIN) {
				m_pos.x = static_cast<FLOAT>(m_pos.x - vec_x);
				m_start.x = static_cast<FLOAT>(m_start.x + vec_x);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_EAST:
		{
			const double vec_x = std::round(
				(static_cast<double>(val.x) - m_start.x - m_pos.x) / PT_ROUND) * PT_ROUND;
			if (vec_x <= -FLT_MIN || vec_x >= FLT_MIN) {
				m_pos.x += static_cast<FLOAT>(vec_x);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_NORTH:
		{
			const double vec_y = std::round(
				(static_cast<double>(val.y) - m_start.y) / PT_ROUND) * PT_ROUND;
			if (vec_y <= -FLT_MIN || vec_y >= FLT_MIN) {
				m_pos.y = static_cast<FLOAT>(m_pos.y - vec_y);
				m_start.y = static_cast<FLOAT>(m_start.y + vec_y);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_SOUTH:
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
		if (limit >= FLT_MIN) {
			// 終点への差分の x 値が, 限界距離未満か判定する.
			if (m_pos.x > -limit && m_pos.x < limit) {
				if (anc == ANC_TYPE::ANC_NE) {
					m_start.x += m_pos.x;
				}
				m_pos.x = 0.0f;
				done = true;
			}
			if (m_pos.y > -limit && m_pos.y < limit) {
				if (anc == ANC_TYPE::ANC_NE) {
					m_start.y += m_pos.y;
				}
				m_pos.y = 0.0f;
				done = true;
			}
		}
		return done;
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	bool ShapeRect::set_pos_start(const D2D1_POINT_2F val) noexcept
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
	ShapeRect::ShapeRect(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page) :
		ShapeStroke::ShapeStroke(page),
		m_start(start),
		m_pos(pos)
	{
		page->get_fill_color(m_fill_color);
	}

	// 図形をデータリーダーから読み込む.
	ShapeRect::ShapeRect(const Shape& page, DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(page, dt_reader)
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
	void ShapeRect::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);

		// 開始位置
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
	bool ShapeRect::get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& val) const noexcept
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
	size_t ShapeRect::get_verts(D2D1_POINT_2F p[]) const noexcept
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