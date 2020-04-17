#include "pch.h"
#include "shape_rect.h"

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
			m_pos.x + m_diff.x,
			m_pos.y + m_diff.y
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
			const auto w = static_cast<FLOAT>(m_stroke_width);
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawRectangle(
				rect, dx.m_shape_brush.get(), w, m_d2d_stroke_style.get());
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

	ANCH_WHICH ShapeRect::hit_test_anchor(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		// どの頂点が位置を含むか調べる.
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F a_pos;
			get_pos(ANCH_CORNER[i], a_pos);
			if (pt_in_anch(t_pos, a_pos, a_len)) {
				return ANCH_CORNER[i];
			}
		}
		// どの中点が位置を含むか調べる.
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F a_pos;
			get_pos(ANCH_MIDDLE[i], a_pos);
			if (pt_in_anch(t_pos, a_pos, a_len)) {
				return ANCH_MIDDLE[i];
			}
		}
		return ANCH_WHICH::ANCH_OUTSIDE;
	}

	// 位置を含むか調べる.
	// t_pos	調べる位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	ANCH_WHICH ShapeRect::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const auto anchor = hit_test_anchor(t_pos, a_len);
		if (anchor != ANCH_WHICH::ANCH_OUTSIDE) {
			return anchor;
		}
		// 方形の右上点と左下点を求める.
		D2D1_POINT_2F r_pos;
		pt_add(m_pos, m_diff, r_pos);
		D2D1_POINT_2F r_min;	// 方形の左上点
		D2D1_POINT_2F r_max;	// 方形の右下点
		pt_bound(m_pos, r_pos, r_min, r_max);
		if (is_opaque(m_stroke_color) != true) {
			// 線枠の色が透明な場合,
			if (is_opaque(m_fill_color)) {
				// 塗りつぶし色が不透明な場合,
				if (pt_in_rect(t_pos, r_min, r_max)) {
					// 位置が方形にふくまれる場合,
					return ANCH_WHICH::ANCH_INSIDE;
				}
			}
			return ANCH_WHICH::ANCH_OUTSIDE;
		}
		const double sw = max(m_stroke_width, a_len);	// 線枠の太さ
		pt_add(r_min, sw * 0.5, r_min);
		pt_add(r_max, sw * -0.5, r_max);
		if (pt_in_rect(t_pos, r_min, r_max)) {
			return is_opaque(m_fill_color) ? ANCH_WHICH::ANCH_INSIDE : ANCH_WHICH::ANCH_OUTSIDE;
		}
		pt_add(r_min, -sw, r_min);
		pt_add(r_max, sw, r_max);
		if (pt_in_rect(t_pos, r_min, r_max)) {
			return ANCH_WHICH::ANCH_FRAME;
		}
		return ANCH_WHICH::ANCH_OUTSIDE;
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

	// 指定された部位の位置を得る.
	void ShapeRect::get_pos(const ANCH_WHICH a, D2D1_POINT_2F& value) const noexcept
	{
		switch (a) {
		case ANCH_WHICH::ANCH_NORTH:
			value.x = m_pos.x + m_diff.x * 0.5f;
			value.y = m_pos.y;
			break;
		case ANCH_WHICH::ANCH_NE:
			value.x = m_pos.x + m_diff.x;
			value.y = m_pos.y;
			break;
		case ANCH_WHICH::ANCH_WEST:
			value.x = m_pos.x;
			value.y = m_pos.y + m_diff.y * 0.5f;
			break;
		case ANCH_WHICH::ANCH_EAST:
			value.x = m_pos.x + m_diff.x;
			value.y = m_pos.y + m_diff.y * 0.5f;
			break;
		case ANCH_WHICH::ANCH_SW:
			value.x = m_pos.x;
			value.y = m_pos.y + m_diff.y;
			break;
		case ANCH_WHICH::ANCH_SOUTH:
			value.x = m_pos.x + m_diff.x * 0.5f;
			value.y = m_pos.y + m_diff.y;
			break;
		case ANCH_WHICH::ANCH_SE:
			value.x = m_pos.x + m_diff.x;
			value.y = m_pos.y + m_diff.y;
			break;
		default:
			value = m_pos;
			break;
		}
	}

	// 範囲に含まれるか調べる.
	// a_min	範囲の左上位置
	// a_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeRect::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		D2D1_POINT_2F e_pos;

		pt_add(m_pos, m_diff, e_pos);
		return pt_in_rect(m_pos, a_min, a_max) && pt_in_rect(e_pos, a_min, a_max);
	}

	// 値を指定した部位の位置に格納する. 他の部位の位置は動かない. 
	void ShapeRect::set_pos(const D2D1_POINT_2F value, const ANCH_WHICH a)
	{
		D2D1_POINT_2F a_pos;
		D2D1_POINT_2F diff;

		switch (a) {
		case ANCH_WHICH::ANCH_OUTSIDE:
			m_pos = value;
			break;
		case ANCH_WHICH::ANCH_NW:
			pt_sub(value, m_pos, diff);
			pt_add(m_pos, diff, m_pos);
			pt_sub(m_diff, diff, m_diff);
			break;
		case ANCH_WHICH::ANCH_NORTH:
			m_diff.y -= value.y - m_pos.y;
			m_pos.y = value.y;
			break;
		case ANCH_WHICH::ANCH_NE:
			a_pos.x = m_pos.x + m_diff.x;
			a_pos.y = m_pos.y;
			m_pos.y = value.y;
			pt_sub(value, a_pos, diff);
			pt_add(m_diff, diff.x, -diff.y, m_diff);
			break;
		case ANCH_WHICH::ANCH_WEST:
			m_diff.x -= value.x - m_pos.x;
			m_pos.x = value.x;
			break;
		case ANCH_WHICH::ANCH_EAST:
			m_diff.x = value.x - m_pos.x;
			break;
		case ANCH_WHICH::ANCH_SW:
			a_pos.x = m_pos.x;
			a_pos.y = m_pos.y + m_diff.y;
			m_pos.x = value.x;
			pt_sub(value, a_pos, diff);
			pt_add(m_diff, -diff.x, diff.y, m_diff);
			break;
		case ANCH_WHICH::ANCH_SOUTH:
			m_diff.y = value.y - m_pos.y;
			break;
		case ANCH_WHICH::ANCH_SE:
			pt_sub(value, m_pos, m_diff);
			break;
		}
	}

	// 図形を作成する.
	ShapeRect::ShapeRect(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, const ShapeLayout* attr) :
		ShapeStroke::ShapeStroke(attr),
		m_fill_color(attr->m_fill_color)
	{
		m_pos = s_pos;
		m_diff = diff;
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
		write_svg(m_diff, "width", "height", dt_writer);
		write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>" SVG_NL, dt_writer);
	}
}