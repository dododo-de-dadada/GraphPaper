#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	// 図形を表示する.
	// sh	表示する用紙
	void ShapeRect::draw(ShapeSheet const& sheet)
	{
		ID2D1Factory* const factory = Shape::s_factory;
		ID2D1RenderTarget* const target = Shape::s_target;
		ID2D1SolidColorBrush* const brush = Shape::s_color_brush;

		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}

		const D2D1_RECT_F rect{
			m_pos.x,
			m_pos.y,
			m_pos.x + m_vec[0].x,
			m_pos.y + m_vec[0].y
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
			a_pos[0] = m_pos;
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

	// 図形の部位が位置を含むか判定する.
	uint32_t ShapeRect::hit_test_anc(const D2D1_POINT_2F t_pos) const noexcept
	{
		// 4----8----2
		// |         |
		// 7         6
		// |         |
		// 3----5----1
		const D2D1_POINT_2F anc_se{ m_pos.x + m_vec[0].x, m_pos.y + m_vec[0].y };
		if (pt_in_anc(t_pos, anc_se)) {
			return ANC_TYPE::ANC_SE;
		}
		const D2D1_POINT_2F anc_ne{ anc_se.x, m_pos.y };
		if (pt_in_anc(t_pos, anc_ne)) {
			return ANC_TYPE::ANC_NE;
		}
		const D2D1_POINT_2F anc_sw{ m_pos.x, anc_se.y };
		if (pt_in_anc(t_pos, anc_sw)) {
			return ANC_TYPE::ANC_SW;
		}
		if (pt_in_anc(t_pos, m_pos)) {
			return ANC_TYPE::ANC_NW;
		}
		const D2D1_POINT_2F anc_s{ static_cast<FLOAT>(m_pos.x + m_vec[0].x * 0.5), anc_se.y };
		if (pt_in_anc(t_pos, anc_s)) {
			return ANC_TYPE::ANC_SOUTH;
		}
		const D2D1_POINT_2F anc_e{ anc_se.x, static_cast<FLOAT>(m_pos.y + m_vec[0].y * 0.5f) };
		if (pt_in_anc(t_pos, anc_e)) {
			return ANC_TYPE::ANC_EAST;
		}
		const D2D1_POINT_2F anc_w{ m_pos.x, anc_e.y };
		if (pt_in_anc(t_pos, anc_w)) {
			return ANC_TYPE::ANC_WEST;
		}
		const D2D1_POINT_2F anc_n{ anc_s.x, m_pos.y };
		if (pt_in_anc(t_pos, anc_n)) {
			return ANC_TYPE::ANC_NORTH;
		}
		return ANC_TYPE::ANC_SHEET;
	}

	// 位置を含むか判定する.
	// t_pos	判定される位置
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeRect::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		// 各頂点の部位に含まれるか判定する.
		D2D1_POINT_2F v_pos[4]{ m_pos, };
		v_pos[2].x = m_pos.x + m_vec[0].x;
		v_pos[2].y = m_pos.y + m_vec[0].y;
		if (pt_in_anc(t_pos, v_pos[2])) {
			return ANC_TYPE::ANC_SE;
		}
		v_pos[3].x = m_pos.x;
		v_pos[3].y = m_pos.y + m_vec[0].y;
		if (pt_in_anc(t_pos, v_pos[3])) {
			return ANC_TYPE::ANC_SW;
		}
		v_pos[1].x = m_pos.x + m_vec[0].x;
		v_pos[1].y = m_pos.y;
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
		D2D1_POINT_2F t_min, t_max;
		//pt_bound(v_pos[0], v_pos[2], r_min, r_max);
		if (v_pos[0].x < v_pos[2].x) {
			t_min.x = v_pos[0].x;
			t_max.x = v_pos[2].x;
		}
		else {
			t_min.x = v_pos[2].x;
			t_max.x = v_pos[0].x;
		}
		if (v_pos[0].y < v_pos[2].y) {
			t_min.y = v_pos[0].y;
			t_max.y = v_pos[2].y;
		}
		else {
			t_min.y = v_pos[2].y;
			t_max.y = v_pos[0].y;
		}

		if (!is_opaque(m_stroke_color) || m_stroke_width < FLT_MIN) {
			if (is_opaque(m_fill_color) && pt_in_rect2(t_pos, t_min, t_max)) {
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
			D2D1_POINT_2F s_min, s_max;	// 拡大した方形
			const double s_thick = max(m_stroke_width, Shape::s_anc_len);
			const double e_thick = s_thick * 0.5;
			pt_add(t_min, -e_thick, s_min);
			pt_add(t_max, e_thick, s_max);
			// 拡大した方形に含まれるか判定する.
			if (pt_in_rect2(t_pos, s_min, s_max)) {
				// 太さの大きさだけ内側に, 拡大した方形を縮小する.
				D2D1_POINT_2F u_min, u_max;	// 縮小した方形
				pt_add(s_min, s_thick, u_min);
				pt_add(s_max, -s_thick, u_max);
				// 縮小した方形に含まれる (辺に含まれない) か判定する.
				if (pt_in_rect2(t_pos, u_min, u_max)) {
					if (is_opaque(m_fill_color)) {
						return ANC_TYPE::ANC_FILL;
					}
				}
				// 縮小した方形が反転する (枠が太すぎて図形を覆う),
				// または, 方形の角に含まれてない (辺に含まれる) か判定する.
				else if (u_max.x <= u_min.x || u_max.y <= u_min.y ||
					t_min.x <= t_pos.x && t_pos.x <= t_max.x || t_min.y <= t_pos.y && t_pos.y <= t_max.y) {
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
		return ANC_TYPE::ANC_SHEET;
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
			val.x = m_pos.x + m_vec[0].x * 0.5f;
			val.y = m_pos.y;
			break;
		case ANC_TYPE::ANC_NE:
			val.x = m_pos.x + m_vec[0].x;
			val.y = m_pos.y;
			break;
		case ANC_TYPE::ANC_WEST:
			val.x = m_pos.x;
			val.y = m_pos.y + m_vec[0].y * 0.5f;
			break;
		case ANC_TYPE::ANC_EAST:
			val.x = m_pos.x + m_vec[0].x;
			val.y = m_pos.y + m_vec[0].y * 0.5f;
			break;
		case ANC_TYPE::ANC_SW:
			val.x = m_pos.x;
			val.y = m_pos.y + m_vec[0].y;
			break;
		case ANC_TYPE::ANC_SOUTH:
			val.x = m_pos.x + m_vec[0].x * 0.5f;
			val.y = m_pos.y + m_vec[0].y;
			break;
		case ANC_TYPE::ANC_SE:
			val.x = m_pos.x + m_vec[0].x;
			val.y = m_pos.y + m_vec[0].y;
			break;
		default:
			val = m_pos;
			break;
		}
	}

	// 範囲に含まれるか判定する.
	// area_min	範囲の左上位置
	// area_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeRect::in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept
	{
		D2D1_POINT_2F pos;
		pt_add(m_pos, m_vec[0], pos);
		return pt_in_rect(m_pos, area_min, area_max) && pt_in_rect(pos, area_min, area_max);
	}

	// 値を, 部位の位置に格納する. 他の部位の位置も動く.
	// val	値
	// anc	図形の部位
	// limit	他の頂点との限界距離 (他の頂点との距離がこの値未満になるなら, その頂点に位置に合わせる)
	bool ShapeRect::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool /*keep_aspect*/) noexcept
	{
		bool done = false;
		switch (anc) {
		case ANC_TYPE::ANC_SHEET:
		{
			D2D1_POINT_2F pos;
			pt_round(val, PT_ROUND, pos);
			D2D1_POINT_2F vec;
			pt_sub(pos, m_pos, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				pt_add(m_pos, vec, m_pos);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_NW:
		{
			D2D1_POINT_2F pos;
			pt_round(val, PT_ROUND, pos);
			D2D1_POINT_2F vec;
			pt_sub(pos, m_pos, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				pt_add(m_pos, vec, m_pos);
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
			pt_sub(pos, D2D1_POINT_2F{ m_pos.x + m_vec[0].x, m_pos.y + m_vec[0].y }, vec);
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
			pt_sub(pos, D2D1_POINT_2F{ m_pos.x + m_vec[0].x, m_pos.y }, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				m_pos.y += vec.y;
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
			pt_sub(pos, D2D1_POINT_2F{ m_pos.x, m_pos.y + m_vec[0].y }, vec);
			if (pt_abs2(vec) >= FLT_MIN) {
				m_pos.x += vec.x;
				pt_add(m_vec[0], -vec.x, vec.y, m_vec[0]);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_WEST:
		{
			const double vec_x = std::round((static_cast<double>(val.x) - m_pos.x) / PT_ROUND) * PT_ROUND;
			if (vec_x <= -FLT_MIN || vec_x >= FLT_MIN) {
				m_vec[0].x = static_cast<FLOAT>(m_vec[0].x - vec_x);
				m_pos.x = static_cast<FLOAT>(m_pos.x + vec_x);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_EAST:
		{
			const double vec_x = std::round((static_cast<double>(val.x) - m_pos.x - m_vec[0].x) / PT_ROUND) * PT_ROUND;
			if (vec_x <= -FLT_MIN || vec_x >= FLT_MIN) {
				m_vec[0].x += static_cast<FLOAT>(vec_x);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_NORTH:
		{
			const double vec_y = std::round((static_cast<double>(val.y) - m_pos.y) / PT_ROUND) * PT_ROUND;
			if (vec_y <= -FLT_MIN || vec_y >= FLT_MIN) {
				m_vec[0].y = static_cast<FLOAT>(m_vec[0].y - vec_y);
				m_pos.y = static_cast<FLOAT>(m_pos.y + vec_y);
				done = true;
			}
		}
		break;
		case ANC_TYPE::ANC_SOUTH:
		{
			const double vec_y = std::round((static_cast<double>(val.y) - m_pos.y - m_vec[0].y) / PT_ROUND) * PT_ROUND;
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
					m_pos.x += m_vec[0].x;
				}
				m_vec[0].x = 0.0f;
				done = true;
			}
			if (m_vec[0].y > -limit && m_vec[0].y < limit) {
				if (anc == ANC_TYPE::ANC_NE) {
					m_pos.y += m_vec[0].y;
				}
				m_vec[0].y = 0.0f;
				done = true;
			}
		}
		return done;
	}

	// 図形を作成する.
	// b_pos	囲む領域の始点
	// b_vec	囲む領域の終点への差分
	// s_sttr	属性
	ShapeRect::ShapeRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet) :
		ShapeStroke::ShapeStroke(s_sheet),
		m_fill_color(s_sheet->m_fill_color)
	{
		m_pos = b_pos;
		m_vec.resize(1, b_vec);
		m_vec.shrink_to_fit();
	}

	// 図形をデータリーダーから読み込む.
	ShapeRect::ShapeRect(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader)
	{
		dt_read(m_fill_color, dt_reader);
	}

	// 図形をデータライターに書き込む.
	void ShapeRect::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);
		dt_write(m_fill_color, dt_writer);
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
		v_pos[0] = m_pos;
		// 右上
		v_pos[1].x = m_pos.x + m_vec[0].x;
		v_pos[1].y = m_pos.y;
		// 右下
		v_pos[2].x = v_pos[1].x;
		v_pos[2].y = m_pos.y + m_vec[0].y;
		// 右下
		v_pos[3].x = m_pos.x;
		v_pos[3].y = v_pos[2].y;
		return 4;
	}

}