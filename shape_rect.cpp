#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 図形を表示する.
	void ShapeRect::draw(SHAPE_DX& dx)
	{
		const D2D1_RECT_F rect{
			m_pos.x,
			m_pos.y,
			m_pos.x + m_vec.x,
			m_pos.y + m_vec.y
		};
		if (is_opaque(m_fill_color)) {
			// 塗りつぶし色が透明でなければ方形を塗りつぶす.
			dx.m_shape_brush->SetColor(m_fill_color);
			dx.m_d2dContext->FillRectangle(&rect, dx.m_shape_brush.get());
		}
		if (is_opaque(m_stroke_color)) {
			// 線枠の色が透明でなければ方形の枠を表示する.
			const auto w = static_cast<FLOAT>(m_stroke_width);
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawRectangle(
				rect, dx.m_shape_brush.get(), w, m_d2d_stroke_style.get());
		}
		if (is_selected() == false) {
			return;
		}
		// 選択されているなら基準部位を表示する.
		D2D1_POINT_2F r_pos[4];	// 方形の頂点
		r_pos[0] = m_pos;
		r_pos[1].y = rect.top;
		r_pos[1].x = rect.right;
		r_pos[2].x = rect.right;
		r_pos[2].y = rect.bottom;
		r_pos[3].y = rect.bottom;
		r_pos[3].x = rect.left;
		for (uint32_t i = 0, j = 3; i < 4; j = i++) {
			draw_anchor(r_pos[i], dx);
			D2D1_POINT_2F r_mid;	// 方形の辺の中点
			pt_avg(r_pos[j], r_pos[i], r_mid);
			draw_anchor(r_mid, dx);
		}
	}

	// 位置を含むか調べる.
	// t_pos	調べる位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	ANCH_WHICH ShapeRect::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		// どの頂点が位置を含むか調べる.
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F pos;
			get_pos(ANCH_CORNER[i], pos);
			if (pt_in_anch(t_pos, pos, a_len)) {
				return ANCH_CORNER[i];
			}
		}
		// どの中点が位置を含むか調べる.
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F pos;
			get_pos(ANCH_MIDDLE[i], pos);
			if (pt_in_anch(t_pos, pos, a_len)) {
				return ANCH_MIDDLE[i];
			}
		}
		// 方形の右上点と左下点を求める.
		D2D1_POINT_2F r_pos;
		pt_add(m_pos, m_vec, r_pos);
		D2D1_POINT_2F r_min;	// 方形の左上点
		D2D1_POINT_2F r_max;	// 方形の右下点
		pt_bound(m_pos, r_pos, r_min, r_max);
		if (is_opaque(m_stroke_color) == false) {
			//	線枠の色が透明な場合,
			if (is_opaque(m_fill_color)) {
				//	塗りつぶし色が不透明な場合,
				if (pt_in_rect(t_pos, r_min, r_max)) {
				//if (r_min.x <= t_pos.x && t_pos.x <= r_max.x) {
				//	if (r_min.y <= t_pos.y && t_pos.y <= r_max.y) {
						//	位置が方形にふくまれる場合,
						return ANCH_INSIDE;
				//	}
				}
			}
			return ANCH_OUTSIDE;
		}
		const double sw = max(m_stroke_width, a_len);	// 線枠の太さ
		pt_add(r_min, sw * 0.5, r_min);
		pt_add(r_max, sw * -0.5, r_max);
		if (pt_in_rect(t_pos, r_min, r_max)) {
		//if (r_min.x <= t_pos.x && t_pos.x <= r_max.x) {
		//	if (r_min.y <= t_pos.y && t_pos.y <= r_max.y) {
				return is_opaque(m_fill_color) ? ANCH_INSIDE : ANCH_OUTSIDE;
		//	}
		}
		pt_add(r_min, -sw, r_min);
		pt_add(r_max, sw, r_max);
		if (pt_in_rect(t_pos, r_min, r_max)) {
		//if (r_min.x <= t_pos.x && t_pos.x <= r_max.x) {
		//	if (r_min.y <= t_pos.y && t_pos.y <= r_max.y) {
				return ANCH_FRAME;
		//	}
		}
		return ANCH_OUTSIDE;
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
	void ShapeRect::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		m_fill_color = val;
	}

	// 指定された部位の位置を得る.
	void ShapeRect::get_pos(const ANCH_WHICH a, D2D1_POINT_2F& a_pos) const noexcept
	{
		switch (a) {
		case ANCH_NORTH:
			a_pos.x = m_pos.x + m_vec.x * 0.5f;
			a_pos.y = m_pos.y;
			break;
		case ANCH_NE:
			a_pos.x = m_pos.x + m_vec.x;
			a_pos.y = m_pos.y;
			break;
		case ANCH_WEST:
			a_pos.x = m_pos.x;
			a_pos.y = m_pos.y + m_vec.y * 0.5f;
			break;
		case ANCH_EAST:
			a_pos.x = m_pos.x + m_vec.x;
			a_pos.y = m_pos.y + m_vec.y * 0.5f;
			break;
		case ANCH_SW:
			a_pos.x = m_pos.x;
			a_pos.y = m_pos.y + m_vec.y;
			break;
		case ANCH_SOUTH:
			a_pos.x = m_pos.x + m_vec.x * 0.5f;
			a_pos.y = m_pos.y + m_vec.y;
			break;
		case ANCH_SE:
			a_pos.x = m_pos.x + m_vec.x;
			a_pos.y = m_pos.y + m_vec.y;
			break;
		default:
			a_pos = m_pos;
			break;
		}
	}

	//	範囲に含まれるか調べる.
	//	a_min	範囲の左上位置
	//	a_max	範囲の右下位置
	//	戻り値	含まれるなら true
	//	線の太さは考慮されない.
	bool ShapeRect::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		D2D1_POINT_2F pos;

		pt_add(m_pos, m_vec, pos);
		return pt_in_rect(m_pos, a_min, a_max) && pt_in_rect(pos, a_min, a_max);
	}

	//	値を指定した部位の位置に格納する. 他の部位の位置は動かない. 
	void ShapeRect::set_pos(const D2D1_POINT_2F pos, const ANCH_WHICH a)
	{
		D2D1_POINT_2F a_pos;
		D2D1_POINT_2F d;

		switch (a) {
		case ANCH_OUTSIDE:
			m_pos = pos;
			break;
		case ANCH_NW:
			pt_sub(pos, m_pos, d);
			pt_add(m_pos, d, m_pos);
			pt_sub(m_vec, d, m_vec);
			break;
		case ANCH_NORTH:
			m_vec.y -= pos.y - m_pos.y;
			m_pos.y = pos.y;
			break;
		case ANCH_NE:
			a_pos.x = m_pos.x + m_vec.x;
			a_pos.y = m_pos.y;
			m_pos.y = pos.y;
			pt_sub(pos, a_pos, d);
			pt_add(m_vec, d.x, -d.y, m_vec);
			break;
		case ANCH_WEST:
			m_vec.x -= pos.x - m_pos.x;
			m_pos.x = pos.x;
			break;
		case ANCH_EAST:
			m_vec.x = pos.x - m_pos.x;
			break;
		case ANCH_SW:
			a_pos.x = m_pos.x;
			a_pos.y = m_pos.y + m_vec.y;
			m_pos.x = pos.x;
			pt_sub(pos, a_pos, d);
			pt_add(m_vec, -d.x, d.y, m_vec);
			break;
		case ANCH_SOUTH:
			m_vec.y = pos.y - m_pos.y;
			break;
		case ANCH_SE:
			pt_sub(pos, m_pos, m_vec);
			break;
		}
	}

	//	図形を作成する.
	ShapeRect::ShapeRect(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, const ShapePanel* attr) :
		ShapeStroke::ShapeStroke(attr),
		m_fill_color(attr->m_fill_color)
	{
		m_pos = pos;
		m_vec = vec;
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
		write_svg(m_vec, "width", "height", dt_writer);
		write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>" SVG_NL, dt_writer);
	}
}