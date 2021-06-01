#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 図形を表示する.
	// dx	描画環境
	void ShapeRect::draw(SHAPE_DX& dx)
	{
		const D2D1_RECT_F rect{
			m_pos.x,
			m_pos.y,
			m_pos.x + m_diff[0].x,
			m_pos.y + m_diff[0].y
		};
		if (is_opaque(m_fill_color)) {
			// 塗りつぶし色が不透明な場合,
			// 方形を塗りつぶす.
			dx.m_shape_brush->SetColor(m_fill_color);
			dx.m_d2dContext->FillRectangle(&rect, dx.m_shape_brush.get());
		}
		if (is_opaque(m_stroke_color)) {
			// 線枠の色が不透明な場合,
			// 方形の枠を表示する.
			const auto w = m_stroke_width;
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawRectangle(
				rect, dx.m_shape_brush.get(), w, m_d2d_stroke_dash_style.get());
		}
		if (is_selected() != true) {
			return;
		}
		// 選択フラグが立っている場合,
		// 部位を表示する.
		D2D1_POINT_2F r_pos[4];	// 方形の頂点
		r_pos[0] = m_pos;
		r_pos[1].y = rect.top;
		r_pos[1].x = rect.right;
		r_pos[2].x = rect.right;
		r_pos[2].y = rect.bottom;
		r_pos[3].y = rect.bottom;
		r_pos[3].x = rect.left;
		for (uint32_t i = 0, j = 3; i < 4; j = i++) {
			anchor_draw_rect(r_pos[i], dx);
			D2D1_POINT_2F r_mid;	// 方形の辺の中点
			pt_avg(r_pos[j], r_pos[i], r_mid);
			anchor_draw_rect(r_mid, dx);
		}
	}

	// 折れ線の図形の部位が位置を含むか判定する.
	uint32_t ShapeRect::hit_test_anchor(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		// どの頂点が位置を含むか判定する.
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F a_pos;
			get_anch_pos(ANCH_CORNER[i], a_pos);
			if (pt_in_anch(t_pos, a_pos, a_len)) {
				return ANCH_CORNER[i];
			}
		}
		// どの中点が位置を含むか判定する.
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F a_pos;
			get_anch_pos(ANCH_MIDDLE[i], a_pos);
			if (pt_in_anch(t_pos, a_pos, a_len)) {
				return ANCH_MIDDLE[i];
			}
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeRect::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		// 方形の各頂点を求め, 判定する位置がそれぞれの部位に含まれるか判定する.
		D2D1_POINT_2F v_pos[4]{ m_pos, };
		v_pos[2].x = m_pos.x + m_diff[0].x;
		v_pos[2].y = m_pos.y + m_diff[0].y;
		if (pt_in_anch(t_pos, v_pos[2], a_len)) {
			return ANCH_TYPE::ANCH_SE;
		}
		v_pos[3].x = m_pos.x;
		v_pos[3].y = m_pos.y + m_diff[0].y;
		if (pt_in_anch(t_pos, v_pos[3], a_len)) {
			return ANCH_TYPE::ANCH_SW;
		}
		v_pos[1].x = m_pos.x + m_diff[0].x;
		v_pos[1].y = m_pos.y;
		if (pt_in_anch(t_pos, v_pos[1], a_len)) {
			return ANCH_TYPE::ANCH_NE;
		}
		if (pt_in_anch(t_pos, v_pos[0], a_len)) {
			return ANCH_TYPE::ANCH_NW;
		}

		// 各辺の中点を求め, 判定する位置がそれぞれの部位に含まれるか判定する.
		D2D1_POINT_2F s_pos;
		pt_avg(v_pos[2], v_pos[3], s_pos);
		if (pt_in_anch(t_pos, s_pos, a_len)) {
			return ANCH_TYPE::ANCH_SOUTH;
		}
		D2D1_POINT_2F e_pos;
		pt_avg(v_pos[1], v_pos[2], e_pos);
		if (pt_in_anch(t_pos, e_pos, a_len)) {
			return ANCH_TYPE::ANCH_EAST;
		}
		D2D1_POINT_2F w_pos;
		pt_avg(v_pos[0], v_pos[3], w_pos);
		if (pt_in_anch(t_pos, w_pos, a_len)) {
			return ANCH_TYPE::ANCH_WEST;
		}
		D2D1_POINT_2F n_pos;
		pt_avg(v_pos[0], v_pos[1], n_pos);
		if (pt_in_anch(t_pos, n_pos, a_len)) {
			return ANCH_TYPE::ANCH_NORTH;
		}

		D2D1_POINT_2F r_min;
		D2D1_POINT_2F r_max;
		pt_bound(v_pos[0], v_pos[2], r_min, r_max);
		D2D1_POINT_2F o_min;
		D2D1_POINT_2F o_max;
		const double e_width = m_stroke_width * 0.5;
		pt_add(r_min, -e_width, -e_width, o_min);
		pt_add(r_max, e_width, e_width, o_max);
		if (!pt_in_rect(t_pos, o_min, o_max)) {
			return ANCH_TYPE::ANCH_SHEET;
		}
		if (is_opaque(m_stroke_color)) {
			D2D1_POINT_2F i_min;
			D2D1_POINT_2F i_max;
			pt_add(o_min, m_stroke_width, m_stroke_width, i_min);
			pt_add(o_max, -m_stroke_width, -m_stroke_width, i_max);
			if (i_max.x <= i_min.x || i_max.y <= i_min.y ||
				!pt_in_rect(t_pos, i_min, i_max)) {
				if (r_min.x <= t_pos.x && t_pos.x <= r_max.x ||
					r_min.y <= t_pos.y && t_pos.y <= r_max.y) {
					return ANCH_TYPE::ANCH_STROKE;
				}
				else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
					if (pt_in_elli(t_pos, v_pos[0], e_width, e_width) ||
						pt_in_elli(t_pos, v_pos[1], e_width, e_width) ||
						pt_in_elli(t_pos, v_pos[2], e_width, e_width) ||
						pt_in_elli(t_pos, v_pos[3], e_width, e_width)) {
						return ANCH_TYPE::ANCH_STROKE;
					}
				}
				else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL ||
					(m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL &&
						m_stroke_join_limit < M_SQRT2)) {
					//const auto e_width = static_cast<FLOAT>(m_stroke_width * 0.5);
					const D2D1_POINT_2F q_pos[4]{
						D2D1_POINT_2F{ 0.0f, -static_cast<FLOAT>(e_width) },
						D2D1_POINT_2F{ static_cast<FLOAT>(e_width), 0.0f },
						D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
						D2D1_POINT_2F{ -static_cast<FLOAT>(e_width), 0.0f }
					};
					if (pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[0].x, t_pos.y - v_pos[0].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[1].x, t_pos.y - v_pos[1].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[2].x, t_pos.y - v_pos[2].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[3].x, t_pos.y - v_pos[3].y }, 4, q_pos)) {
						return ANCH_TYPE::ANCH_STROKE;
					}
				}
				else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
					(m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL &&
						m_stroke_join_limit >= M_SQRT2)) {
					const auto limit = static_cast<FLOAT>(m_stroke_width * M_SQRT2 * 0.5 * m_stroke_join_limit);
					const D2D1_POINT_2F q_pos[4]{
						D2D1_POINT_2F{ 0.0f, -limit }, D2D1_POINT_2F{ limit, 0.0f }, D2D1_POINT_2F{ 0.0f, limit }, D2D1_POINT_2F{ -limit, 0.0f }
					};
					if (pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[0].x, t_pos.y - v_pos[0].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[1].x, t_pos.y - v_pos[1].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[2].x, t_pos.y - v_pos[2].y }, 4, q_pos) ||
						pt_in_poly(D2D1_POINT_2F{ t_pos.x - v_pos[3].x, t_pos.y - v_pos[3].y }, 4, q_pos)) {
						return ANCH_TYPE::ANCH_STROKE;
					}
				}
			}
		}
		if (is_opaque(m_fill_color) && pt_in_rect(t_pos, r_min, r_max)) {
			return ANCH_TYPE::ANCH_FILL;
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// 塗りつぶしの色を得る.
	// val	得られた値.
	bool ShapeRect::get_fill_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_fill_color;
		return true;
	}

	// 塗りつぶしの色に格納する.
	// val	格納する値.
	void ShapeRect::set_fill_color(const D2D1_COLOR_F& value) noexcept
	{
		m_fill_color = value;
	}

	//	部位の位置を得る.
	//	anch	図形の部位.
	//	value	得られた位置.
	//	戻り値	なし
	void ShapeRect::get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept
	{
		switch (anch) {
		case ANCH_TYPE::ANCH_NORTH:
			value.x = m_pos.x + m_diff[0].x * 0.5f;
			value.y = m_pos.y;
			break;
		case ANCH_TYPE::ANCH_NE:
			value.x = m_pos.x + m_diff[0].x;
			value.y = m_pos.y;
			break;
		case ANCH_TYPE::ANCH_WEST:
			value.x = m_pos.x;
			value.y = m_pos.y + m_diff[0].y * 0.5f;
			break;
		case ANCH_TYPE::ANCH_EAST:
			value.x = m_pos.x + m_diff[0].x;
			value.y = m_pos.y + m_diff[0].y * 0.5f;
			break;
		case ANCH_TYPE::ANCH_SW:
			value.x = m_pos.x;
			value.y = m_pos.y + m_diff[0].y;
			break;
		case ANCH_TYPE::ANCH_SOUTH:
			value.x = m_pos.x + m_diff[0].x * 0.5f;
			value.y = m_pos.y + m_diff[0].y;
			break;
		case ANCH_TYPE::ANCH_SE:
			value.x = m_pos.x + m_diff[0].x;
			value.y = m_pos.y + m_diff[0].y;
			break;
		default:
			value = m_pos;
			break;
		}
	}

	// 範囲に含まれるか判定する.
	// a_min	範囲の左上位置
	// a_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeRect::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		D2D1_POINT_2F e_pos;

		pt_add(m_pos, m_diff[0], e_pos);
		return pt_in_rect(m_pos, a_min, a_max) && pt_in_rect(e_pos, a_min, a_max);
	}

	//	値を, 部位の位置に格納する. 他の部位の位置は動かない. 
	//	value	格納する値
	//	abch	図形の部位
	void ShapeRect::set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch)
	{
		//D2D1_POINT_2F a_pos;

		switch (anch) {
		case ANCH_TYPE::ANCH_SHEET:
			{
			D2D1_POINT_2F diff;
			pt_sub(value, m_pos, diff);
			pt_round(diff, PT_ROUND, diff);
			pt_add(m_pos, diff, m_pos);
			}
			break;
		case ANCH_TYPE::ANCH_NW:
			{
			D2D1_POINT_2F diff;
			pt_sub(value, m_pos, diff);
			pt_round(diff, PT_ROUND, diff);
			pt_add(m_pos, diff, m_pos);
			pt_sub(m_diff[0], diff, m_diff[0]);
			}
			break;
		case ANCH_TYPE::ANCH_NORTH:
			{
			const double diff_y = std::round((static_cast<double>(value.y) - m_pos.y) / PT_ROUND) * PT_ROUND;
			m_diff[0].y = static_cast<FLOAT>(m_diff[0].y - diff_y);
			m_pos.y = static_cast<FLOAT>(m_pos.y + diff_y);
			}
			break;
		case ANCH_TYPE::ANCH_NE:
			{
			D2D1_POINT_2F a_pos;
			get_anch_pos(ANCH_TYPE::ANCH_NE, a_pos);
			D2D1_POINT_2F diff;
			pt_sub(value, a_pos, diff);
			pt_round(diff, PT_ROUND, diff);
			m_pos.y += diff.y;
			pt_add(m_diff[0], diff.x, -diff.y, m_diff[0]);
			}
			break;
		case ANCH_TYPE::ANCH_WEST:
			{
			const double diff_x = std::round((static_cast<double>(value.x) - m_pos.x) / PT_ROUND) * PT_ROUND;
			m_diff[0].x = static_cast<FLOAT>(m_diff[0].x - diff_x);
			m_pos.x = static_cast<FLOAT>(m_pos.x + diff_x);
			}
			break;
		case ANCH_TYPE::ANCH_EAST:
			{
			const double diff_x = std::round((static_cast<double>(value.x) - m_pos.x) / PT_ROUND) * PT_ROUND;
			m_diff[0].x = static_cast<FLOAT>(diff_x);

			}
			break;
		case ANCH_TYPE::ANCH_SW:
			{
			D2D1_POINT_2F a_pos;
			get_anch_pos(ANCH_TYPE::ANCH_SW, a_pos);
			D2D1_POINT_2F diff;
			pt_sub(value, a_pos, diff);
			m_pos.x += diff.x;
			pt_add(m_diff[0], -diff.x, diff.y, m_diff[0]);
			}
			break;
		case ANCH_TYPE::ANCH_SOUTH:
			{
			const double diff_y = std::round((static_cast<double>(value.y) - m_pos.y) / PT_ROUND) * PT_ROUND;
			m_diff[0].y = static_cast<FLOAT>(diff_y);
			}
			break;
		case ANCH_TYPE::ANCH_SE:
			{
			D2D1_POINT_2F diff;
			pt_sub(value, m_pos, diff);
			pt_round(diff, PT_ROUND, diff);
			m_diff[0] = diff;
			}
			break;
		}
	}

	// 図形を作成する.
	// b_pos	囲む領域の始点
	// b_diff	囲む領域の終点への差分
	// s_sttr	属性
	ShapeRect::ShapeRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr) :
		ShapeStroke::ShapeStroke(1, s_attr),
		m_fill_color(s_attr->m_fill_color)
	{
		m_pos = b_pos;
		m_diff[0] = b_diff;
	}

	// 図形をデータリーダーから読み込む.
	ShapeRect::ShapeRect(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_fill_color, dt_reader);
	}

	// データライターに書き込む.
	void ShapeRect::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeStroke::write(dt_writer);
		write(m_fill_color, dt_writer);
	}

	// データライターに SVG タグとして書き込む.
	void ShapeRect::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<rect ", dt_writer);
		write_svg(m_pos, "x", "y", dt_writer);
		write_svg(m_diff[0], "width", "height", dt_writer);
		write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>" SVG_NEW_LINE, dt_writer);
	}
}