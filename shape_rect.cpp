#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	// 図形を表示する.
	void ShapeRect::draw(void)
	{
		ID2D1Factory* const factory = Shape::s_d2d_factory;
		ID2D1RenderTarget* const target = Shape::s_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::s_d2d_color_brush;

		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}

		const D2D1_RECT_F rect{
			m_start.x,
			m_start.y,
			m_start.x + m_vec[0].x,
			m_start.y + m_vec[0].y
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
		if (is_selected()) {
			// 部位を表示する.
			D2D1_POINT_2F a_pos[4];	// 方形の頂点
			a_pos[0] = m_start;
			a_pos[1].y = rect.top;
			a_pos[1].x = rect.right;
			a_pos[2].x = rect.right;
			a_pos[2].y = rect.bottom;
			a_pos[3].y = rect.bottom;
			a_pos[3].x = rect.left;
			for (uint32_t i = 0, j = 3; i < 4; j = i++) {
				anc_draw_rect(a_pos[i], target, brush);
				D2D1_POINT_2F a_mid;	// 方形の辺の中点
				pt_avg(a_pos[j], a_pos[i], a_mid);
				anc_draw_rect(a_mid, target, brush);
			}
		}
	}

	uint32_t rect_hit_test_anc(const D2D1_POINT_2F start, const D2D1_POINT_2F vec, const D2D1_POINT_2F t_pos) noexcept
	{
		// 4----8----2
		// |         |
		// 7         6
		// |         |
		// 3----5----1
		const D2D1_POINT_2F anc_se{ start.x + vec.x, start.y + vec.y };
		if (pt_in_anc(t_pos, anc_se)) {
			return ANC_TYPE::ANC_SE;
		}
		const D2D1_POINT_2F anc_ne{ anc_se.x, start.y };
		if (pt_in_anc(t_pos, anc_ne)) {
			return ANC_TYPE::ANC_NE;
		}
		const D2D1_POINT_2F anc_sw{ start.x, anc_se.y };
		if (pt_in_anc(t_pos, anc_sw)) {
			return ANC_TYPE::ANC_SW;
		}
		if (pt_in_anc(t_pos, start)) {
			return ANC_TYPE::ANC_NW;
		}
		const D2D1_POINT_2F anc_s{ static_cast<FLOAT>(start.x + vec.x * 0.5), anc_se.y };
		if (pt_in_anc(t_pos, anc_s)) {
			return ANC_TYPE::ANC_SOUTH;
		}
		const D2D1_POINT_2F anc_e{ anc_se.x, static_cast<FLOAT>(start.y + vec.y * 0.5f) };
		if (pt_in_anc(t_pos, anc_e)) {
			return ANC_TYPE::ANC_EAST;
		}
		const D2D1_POINT_2F anc_w{ start.x, anc_e.y };
		if (pt_in_anc(t_pos, anc_w)) {
			return ANC_TYPE::ANC_WEST;
		}
		const D2D1_POINT_2F anc_n{ anc_s.x, start.y };
		if (pt_in_anc(t_pos, anc_n)) {
			return ANC_TYPE::ANC_NORTH;
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// 図形の部位が位置を含むか判定する.
	//uint32_t ShapeRect::hit_test_anc(const D2D1_POINT_2F t_pos) const noexcept
	//{
	//	const auto anc = rect_hit_test_anc(m_start, m_vec[0], t_pos);
	//}

	// 位置を含むか判定する.
	// t_pos	判定される位置
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeRect::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		// 各頂点の部位に含まれるか判定する.
		D2D1_POINT_2F v_pos[4]{ m_start, };
		v_pos[2].x = m_start.x + m_vec[0].x;
		v_pos[2].y = m_start.y + m_vec[0].y;
		if (pt_in_anc(t_pos, v_pos[2])) {
			return ANC_TYPE::ANC_SE;
		}
		v_pos[3].x = m_start.x;
		v_pos[3].y = m_start.y + m_vec[0].y;
		if (pt_in_anc(t_pos, v_pos[3])) {
			return ANC_TYPE::ANC_SW;
		}
		v_pos[1].x = m_start.x + m_vec[0].x;
		v_pos[1].y = m_start.y;
		if (pt_in_anc(t_pos, v_pos[1])) {
			return ANC_TYPE::ANC_NE;
		}
		if (pt_in_anc(t_pos, v_pos[0])) {
			return ANC_TYPE::ANC_NW;
		}

		// 各辺の中点の部位に含まれるか判定する.
		D2D1_POINT_2F s_pos;
		pt_avg(v_pos[2], v_pos[3], s_pos);
		if (pt_in_anc(t_pos, s_pos)) {
			return ANC_TYPE::ANC_SOUTH;
		}
		D2D1_POINT_2F e_pos;
		pt_avg(v_pos[1], v_pos[2], e_pos);
		if (pt_in_anc(t_pos, e_pos)) {
			return ANC_TYPE::ANC_EAST;
		}
		D2D1_POINT_2F w_pos;
		pt_avg(v_pos[0], v_pos[3], w_pos);
		if (pt_in_anc(t_pos, w_pos)) {
			return ANC_TYPE::ANC_WEST;
		}
		D2D1_POINT_2F n_pos;
		pt_avg(v_pos[0], v_pos[1], n_pos);
		if (pt_in_anc(t_pos, n_pos)) {
			return ANC_TYPE::ANC_NORTH;
		}

		// 対角にある頂点をもとに, 方形を得る.
		D2D1_POINT_2F r_lt, r_rb;
		//pt_bound(v_pos[0], v_pos[2], r_nw, r_sw);
		if (v_pos[0].x < v_pos[2].x) {
			r_lt.x = v_pos[0].x;
			r_rb.x = v_pos[2].x;
		}
		else {
			r_lt.x = v_pos[2].x;
			r_rb.x = v_pos[0].x;
		}
		if (v_pos[0].y < v_pos[2].y) {
			r_lt.y = v_pos[0].y;
			r_rb.y = v_pos[2].y;
		}
		else {
			r_lt.y = v_pos[2].y;
			r_rb.y = v_pos[0].y;
		}

		if (!is_opaque(m_stroke_color) || m_stroke_width < FLT_MIN) {
			if (is_opaque(m_fill_color) && pt_in_rect2(t_pos, r_lt, r_rb)) {
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
			const double s_thick = max(m_stroke_width, Shape::s_anc_len);
			const double e_thick = s_thick * 0.5;
			pt_add(r_lt, -e_thick, e_lb);
			pt_add(r_rb, e_thick, e_rb);
			// 拡大した方形に含まれるか判定する.
			if (pt_in_rect2(t_pos, e_lb, e_rb)) {
				// 太さの大きさだけ内側に, 拡大した方形を縮小する.
				D2D1_POINT_2F s_lb, s_rb;	// 縮小した方形
				pt_add(e_lb, s_thick, s_lb);
				pt_add(e_rb, -s_thick, s_rb);
				// 縮小した方形に含まれる (辺に含まれない) か判定する.
				if (pt_in_rect2(t_pos, s_lb, s_rb)) {
					if (is_opaque(m_fill_color)) {
						return ANC_TYPE::ANC_FILL;
					}
				}
				// 縮小した方形が反転する (枠が太すぎて図形を覆う),
				// または, 方形の角に含まれてない (辺に含まれる) か判定する.
				else if (s_rb.x <= s_lb.x || s_rb.y <= s_lb.y ||
					r_lt.x <= t_pos.x && t_pos.x <= r_rb.x || r_lt.y <= t_pos.y && t_pos.y <= r_rb.y) {
					return ANC_TYPE::ANC_STROKE;
				}
				// 線枠の結合が丸めか判定する.
				else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
					if (pt_in_circle(t_pos, v_pos[0], e_thick) ||
						pt_in_circle(t_pos, v_pos[1], e_thick) ||
						pt_in_circle(t_pos, v_pos[2], e_thick) ||
						pt_in_circle(t_pos, v_pos[3], e_thick)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				// 線枠の結合が面取り, または, マイター・面取りでかつマイター制限が√2 未満か判定する.
				else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL ||
					(m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL && m_join_miter_limit < M_SQRT2)) {
					const auto limit = static_cast<FLOAT>(e_thick);
					const D2D1_POINT_2F q_pos[4]{
						D2D1_POINT_2F{ 0.0f, -limit }, D2D1_POINT_2F{ limit, 0.0f }, D2D1_POINT_2F{ 0.0f, limit }, D2D1_POINT_2F{ -limit, 0.0f }
					};
					if (pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[0].x, t_pos.y - v_pos[0].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[1].x, t_pos.y - v_pos[1].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[2].x, t_pos.y - v_pos[2].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[3].x, t_pos.y - v_pos[3].y }, 4, q_pos)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				// 線枠の結合がマイター, または, マイター/面取りでかつマイター制限が√2 以上か判定する.
				else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
					(m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL && m_join_miter_limit >= M_SQRT2)) {
					const auto limit = static_cast<FLOAT>(m_stroke_width * M_SQRT2 * 0.5 * m_join_miter_limit);
					const D2D1_POINT_2F q_pos[4]{
						D2D1_POINT_2F{ 0.0f, -limit }, D2D1_POINT_2F{ limit, 0.0f }, D2D1_POINT_2F{ 0.0f, limit }, D2D1_POINT_2F{ -limit, 0.0f }
					};
					if (pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[0].x, t_pos.y - v_pos[0].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[1].x, t_pos.y - v_pos[1].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[2].x, t_pos.y - v_pos[2].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[3].x, t_pos.y - v_pos[3].y }, 4, q_pos)) {
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
	void ShapeRect::get_bound(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept
	{
		b_lt.x = m_start.x < a_lt.x ? m_start.x : a_lt.x;
		b_lt.y = m_start.y < a_lt.y ? m_start.y : a_lt.y;
		b_rb.x = m_start.x > a_rb.x ? m_start.x : a_rb.x;
		b_rb.y = m_start.y > a_rb.y ? m_start.y : a_rb.y;
		const size_t d_cnt = m_vec.size();	// 差分の数
		D2D1_POINT_2F pos = m_start;
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(pos, m_vec[i], pos);
			if (pos.x < b_lt.x) {
				b_lt.x = pos.x;
			}
			if (pos.x > b_rb.x) {
				b_rb.x = pos.x;
			}
			if (pos.y < b_lt.y) {
				b_lt.y = pos.y;
			}
			if (pos.y > b_rb.y) {
				b_rb.y = pos.y;
			}
		}
	}

	// 塗りつぶしの色を得る.
	// val	得られた値.
	bool ShapeRect::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}

	// 塗りつぶしの色に格納する.
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
			val.x = m_start.x + m_vec[0].x * 0.5f;
			val.y = m_start.y;
			break;
		case ANC_TYPE::ANC_NE:
			val.x = m_start.x + m_vec[0].x;
			val.y = m_start.y;
			break;
		case ANC_TYPE::ANC_WEST:
			val.x = m_start.x;
			val.y = m_start.y + m_vec[0].y * 0.5f;
			break;
		case ANC_TYPE::ANC_EAST:
			val.x = m_start.x + m_vec[0].x;
			val.y = m_start.y + m_vec[0].y * 0.5f;
			break;
		case ANC_TYPE::ANC_SW:
			val.x = m_start.x;
			val.y = m_start.y + m_vec[0].y;
			break;
		case ANC_TYPE::ANC_SOUTH:
			val.x = m_start.x + m_vec[0].x * 0.5f;
			val.y = m_start.y + m_vec[0].y;
			break;
		case ANC_TYPE::ANC_SE:
			val.x = m_start.x + m_vec[0].x;
			val.y = m_start.y + m_vec[0].y;
			break;
		default:
			val = m_start;
			break;
		}
	}

	// 図形を囲む領域の左上位置を得る.
	// val	領域の左上位置
	void ShapeRect::get_pos_lt(D2D1_POINT_2F& val) const noexcept
	{
		const size_t d_cnt = m_vec.size();	// 差分の数
		D2D1_POINT_2F v_pos = m_start;	// 頂点の位置
		val = m_start;
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(v_pos, m_vec[i], v_pos);
			val.x = val.x < v_pos.x ? val.x : v_pos.x;
			val.y = val.y < v_pos.y ? val.y : v_pos.y;
		}
	}

	// 開始位置を得る
	// 戻り値	つねに true
	bool ShapeRect::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		val = m_start;
		return true;
	}

	// 範囲に含まれるか判定する.
	// area_lt	範囲の左上位置
	// area_rb	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeRect::in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept
	{
		D2D1_POINT_2F pos;
		pt_add(m_start, m_vec[0], pos);
		return pt_in_rect(m_start, area_lt, area_rb) && pt_in_rect(pos, area_lt, area_rb);
	}

	// 差分だけ移動する.
	// val	差分ベクトル
	bool ShapeRect::move(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F new_pos;
		pt_add(m_start, val, new_pos);
		if (set_pos_start(new_pos)) {
			return true;
		}
		return false;
	}

	// 値を, 部位の位置に格納する. 他の部位の位置も動く.
	// val	値
	// anc	図形の部位
	// limit	他の頂点との限界距離 (他の頂点との距離がこの値未満になるなら, その頂点に位置に合わせる)
	bool ShapeRect::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool /*keep_aspect*/) noexcept
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
				pt_sub(m_vec[0], vec, m_vec[0]);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_SE:
		{
			D2D1_POINT_2F pos;
			pt_round(val, PT_ROUND, pos);
			D2D1_POINT_2F vec;
			pt_sub(pos, D2D1_POINT_2F{ m_start.x + m_vec[0].x, m_start.y + m_vec[0].y }, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				pt_add(m_vec[0], vec, m_vec[0]);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_NE:
		{
			D2D1_POINT_2F pos;
			pt_round(val, PT_ROUND, pos);
			D2D1_POINT_2F vec;
			pt_sub(pos, D2D1_POINT_2F{ m_start.x + m_vec[0].x, m_start.y }, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				m_start.y += vec.y;
				pt_add(m_vec[0], vec.x, -vec.y, m_vec[0]);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_SW:
		{
			D2D1_POINT_2F pos;
			pt_round(val, PT_ROUND, pos);
			D2D1_POINT_2F vec;
			pt_sub(pos, D2D1_POINT_2F{ m_start.x, m_start.y + m_vec[0].y }, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				m_start.x += vec.x;
				pt_add(m_vec[0], -vec.x, vec.y, m_vec[0]);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_WEST:
		{
			const double vec_x = std::round((static_cast<double>(val.x) - m_start.x) / PT_ROUND) * PT_ROUND;
			if (vec_x <= -FLT_MIN || vec_x >= FLT_MIN) {
				m_vec[0].x = static_cast<FLOAT>(m_vec[0].x - vec_x);
				m_start.x = static_cast<FLOAT>(m_start.x + vec_x);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_EAST:
		{
			const double vec_x = std::round((static_cast<double>(val.x) - m_start.x - m_vec[0].x) / PT_ROUND) * PT_ROUND;
			if (vec_x <= -FLT_MIN || vec_x >= FLT_MIN) {
				m_vec[0].x += static_cast<FLOAT>(vec_x);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_NORTH:
		{
			const double vec_y = std::round((static_cast<double>(val.y) - m_start.y) / PT_ROUND) * PT_ROUND;
			if (vec_y <= -FLT_MIN || vec_y >= FLT_MIN) {
				m_vec[0].y = static_cast<FLOAT>(m_vec[0].y - vec_y);
				m_start.y = static_cast<FLOAT>(m_start.y + vec_y);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_SOUTH:
		{
			const double vec_y = std::round((static_cast<double>(val.y) - m_start.y - m_vec[0].y) / PT_ROUND) * PT_ROUND;
			if (vec_y <= -FLT_MIN || vec_y >= FLT_MIN) {
				m_vec[0].y += static_cast<FLOAT>(vec_y);
				done = true;
			}
		}
		break;
		default:
			return false;
		}
		if (limit >= FLT_MIN) {
			// 終点への差分の x 値が, 限界距離未満か判定する.
			if (m_vec[0].x > -limit && m_vec[0].x < limit) {
				if (anc == ANC_TYPE::ANC_NE) {
					m_start.x += m_vec[0].x;
				}
				m_vec[0].x = 0.0f;
				done = true;
			}
			if (m_vec[0].y > -limit && m_vec[0].y < limit) {
				if (anc == ANC_TYPE::ANC_NE) {
					m_start.y += m_vec[0].y;
				}
				m_vec[0].y = 0.0f;
				done = true;
			}
		}
		return done;
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	bool ShapeRect::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F new_pos;
		pt_round(val, PT_ROUND, new_pos);
		if (!equal(m_start, new_pos)) {
			m_start = new_pos;
			return true;
		}
		return false;
	}

	// 図形を作成する.
	// b_pos	囲む領域の始点
	// b_vec	囲む領域の終点への差分
	// page	属性
	ShapeRect::ShapeRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapePage* page) :
		ShapeStroke::ShapeStroke(page),
		m_fill_color(page->m_fill_color)
	{
		m_start = b_pos;
		m_vec.resize(1, b_vec);
		m_vec.shrink_to_fit();
	}

	// 図形をデータリーダーから読み込む.
	ShapeRect::ShapeRect(const ShapePage& page, DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(page, dt_reader)
	{
		m_start.x = dt_reader.ReadSingle();
		m_start.y = dt_reader.ReadSingle();
		const size_t vec_cnt = dt_reader.ReadUInt32();	// 要素数
		m_vec.resize(vec_cnt);
		for (size_t i = 0; i < vec_cnt; i++) {
			m_vec[i].x = dt_reader.ReadSingle();
			m_vec[i].y = dt_reader.ReadSingle();
		}

		const D2D1_COLOR_F fill_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		m_fill_color.r = min(max(fill_color.r, 0.0f), 1.0f);
		m_fill_color.g = min(max(fill_color.g, 0.0f), 1.0f);
		m_fill_color.b = min(max(fill_color.b, 0.0f), 1.0f);
		m_fill_color.a = min(max(fill_color.a, 0.0f), 1.0f);
	}

	// 図形をデータライターに書き込む.
	void ShapeRect::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);

		// 開始位置
		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);

		// 次の位置への差分
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_vec.size()));
		for (const D2D1_POINT_2F vec : m_vec) {
			dt_writer.WriteSingle(vec.x);
			dt_writer.WriteSingle(vec.y);
		}

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
		bool found = false;
		D2D1_POINT_2F v_pos[4];
		const size_t v_cnt = get_verts(v_pos);
		for (size_t i = 0; i < v_cnt; i++) {
			D2D1_POINT_2F vec;
			pt_sub(v_pos[i], pos, vec);
			const float vv = static_cast<float>(pt_abs2(vec));
			if (vv < dd) {
				dd = vv;
				val = v_pos[i];
				if (!found) {
					found = true;
				}
			}
		}
		return found;
	}

	// 頂点を得る.
	// v_pos	頂点を格納する配列
	// 戻り値	頂点の個数
	size_t ShapeRect::get_verts(D2D1_POINT_2F v_pos[]) const noexcept
	{
		// 左上
		v_pos[0] = m_start;
		// 右上
		v_pos[1].x = m_start.x + m_vec[0].x;
		v_pos[1].y = m_start.y;
		// 右下
		v_pos[2].x = v_pos[1].x;
		v_pos[2].y = m_start.y + m_vec[0].y;
		// 右下
		v_pos[3].x = m_start.x;
		v_pos[3].y = v_pos[2].y;
		return 4;
	}

}