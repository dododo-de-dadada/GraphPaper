#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 線枠の形式を作成する.
	static void create_stroke_style(const D2D1_DASH_STYLE style, const STROKE_PATTERN& array, ID2D1StrokeStyle** d2d_stroke_style);

	// D2D ストローク特性を作成する.
	// ds	破線の種類
	// da	破線の配置配列
	// ss	作成されたストローク特性
	static void create_stroke_style(const D2D1_DASH_STYLE ds, const STROKE_PATTERN& da, ID2D1StrokeStyle** ss)
	{
		D2D1_STROKE_STYLE_PROPERTIES prop{
			D2D1_CAP_STYLE_SQUARE,	// startCap
			D2D1_CAP_STYLE_SQUARE,	// endCap
			D2D1_CAP_STYLE_FLAT,	// dashCap
			D2D1_LINE_JOIN_MITER,	// lineJoin
			1.0f,					// miterLimit
			D2D1_DASH_STYLE_CUSTOM,	// dashStyle
			0.0f
		};
		UINT32 d_cnt;
		const FLOAT* d_arr;

		if (ds != D2D1_DASH_STYLE_SOLID) {
			if (ds == D2D1_DASH_STYLE_DOT) {
				d_arr = da.m_ + 2;
				d_cnt = 2;
			}
			else {
				d_arr = da.m_;
				if (ds == D2D1_DASH_STYLE_DASH) {
					d_cnt = 2;
				}
				else if (ds == D2D1_DASH_STYLE_DASH_DOT) {
					d_cnt = 4;
				}
				else if (ds == D2D1_DASH_STYLE_DASH_DOT_DOT) {
					d_cnt = 6;
				}
				else {
					d_cnt = 0;
				}
			}
			winrt::check_hresult(
				Shape::s_d2d_factory->CreateStrokeStyle(prop, d_arr, d_cnt, ss)
			);
		}
		else {
			*ss = nullptr;
		}
	}

	// 図形を破棄する.
	ShapePoly::~ShapePoly(void)
	{
		m_poly_geom = nullptr;
	}

	// 図形を囲む方形を得る.
	void ShapePoly::get_bound(D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		D2D1_POINT_2F pos;
		pt_inc(m_pos, b_min, b_max);
		pt_add(m_pos, m_vec, pos);
		pt_inc(pos, b_min, b_max);
		pt_add(pos, m_vec_1, pos);
		pt_inc(pos, b_min, b_max);
		pt_add(pos, m_vec_2, pos);
		pt_inc(pos, b_min, b_max);
	}

	// 図形を囲む方形の左上点を得る.
	void ShapePoly::get_min_pos(D2D1_POINT_2F& val) const noexcept
	{
		D2D1_POINT_2F pos;
		pt_add(m_pos, m_vec, pos);
		pt_min(m_pos, pos, val);
		pt_add(pos, m_vec_1, pos);
		pt_min(val, pos, val);
		pt_add(pos, m_vec_2, pos);
		pt_min(val, pos, val);
	}

	// 指定された部位の位置を得る.
	void ShapePoly::get_pos(const ANCH_WHICH a, D2D1_POINT_2F& pos) const noexcept
	{
		switch (a) {
		case ANCH_OUTSIDE:
			pos = m_pos;
			break;
		case ANCH_R_NW:
			pos = m_pos;
			break;
		case ANCH_R_NE:
			pt_add(m_pos, m_vec, pos);
			break;
		case ANCH_R_SW:
			pt_add(m_pos, m_vec, pos);
			pt_add(pos, m_vec_1, pos);
			break;
		case ANCH_R_SE:
			pt_add(m_pos, m_vec, pos);
			pt_add(pos, m_vec_1, pos);
			pt_add(pos, m_vec_2, pos);
			break;
		default:
			return;
		}
	}

	// 差分だけ移動する.
	void ShapePoly::move(const D2D1_POINT_2F d)
	{
		ShapeStroke::move(d);
		create_path_geometry();
	}

	// 値を指定した部位の位置に格納する. 他の部位の位置は動かない. 
	void ShapePoly::set_pos(const D2D1_POINT_2F pos, const ANCH_WHICH a)
	{
		D2D1_POINT_2F a_pos;
		D2D1_POINT_2F d;

		switch (a) {
		case ANCH_OUTSIDE:
			m_pos = pos;
			break;
		case ANCH_R_NW:
			pt_sub(pos, m_pos, d);
			m_pos = pos;
			pt_sub(m_vec, d, m_vec);
			break;
		case ANCH_R_NE:
			get_pos(ANCH_R_NE, a_pos);
			pt_sub(pos, a_pos, d);
			pt_add(m_vec, d, m_vec);
			pt_sub(m_vec_1, d, m_vec_1);
			break;
		case ANCH_R_SW:
			get_pos(ANCH_R_SW, a_pos);
			pt_sub(pos, a_pos, d);
			pt_add(m_vec_1, d, m_vec_1);
			pt_sub(m_vec_2, d, m_vec_2);
			break;
		case ANCH_R_SE:
			get_pos(ANCH_R_SE, a_pos);
			pt_sub(pos, a_pos, d);
			pt_add(m_vec_2, d, m_vec_2);
			break;
		default:
			return;
		}
		create_path_geometry();
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	void ShapePoly::set_start_pos(const D2D1_POINT_2F pos)
	{
		ShapeStroke::set_start_pos(pos);
		create_path_geometry();
	}

	// 図形を作成する.
	ShapePoly::ShapePoly(const ShapePanel* attr) :
		ShapeStroke::ShapeStroke(attr)
	{}

	// 図形をデータリーダーから読み込む.
	ShapePoly::ShapePoly(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_vec_1, dt_reader);
		read(m_vec_2, dt_reader);
	}

	// データライターに書き込む.
	void ShapePoly::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeStroke::write(dt_writer);
		write(m_vec_1, dt_writer);
		write(m_vec_2, dt_writer);
	}

	// 図形を破棄する.
	ShapeStroke::~ShapeStroke(void)
	{
		m_d2d_stroke_style = nullptr;
	}

	// 図形を囲む領域を得る.
	// b_min	領域の左上点
	// b_max	領域の右下点
	void ShapeStroke::get_bound(D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		D2D1_POINT_2F pos;

		pt_inc(m_pos, b_min, b_max);
		pt_add(m_pos, m_vec, pos);
		pt_inc(pos, b_min, b_max);
	}

	// 図形を囲む方形の左上点を得る.
	// D2D1_POINT_2F& pos	// 方形の左上点
	void ShapeStroke::get_min_pos(D2D1_POINT_2F& val) const noexcept
	{
		val.x = m_vec.x >= 0.0f ? m_pos.x : m_pos.x + m_vec.x;
		val.y = m_vec.y >= 0.0f ? m_pos.y : m_pos.y + m_vec.y;
	}

	// 指定された部位の位置を得る.
	void ShapeStroke::get_pos(const ANCH_WHICH /*a*/, D2D1_POINT_2F& pos) const noexcept
	{
		pos = m_pos;
	}

	// 始点を得る
	bool ShapeStroke::get_start_pos(D2D1_POINT_2F& val) const noexcept
	{
		val = m_pos;
		return true;
	}

	// 線枠の色を得る.
	bool ShapeStroke::get_stroke_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_stroke_color;
		return true;
	}

	// 破線の配置を得る.
	bool ShapeStroke::get_stroke_pattern(STROKE_PATTERN& val) const noexcept
	{
		val = m_stroke_pattern;
		return true;
	}

	// 線枠の形式を得る.
	bool ShapeStroke::get_stroke_style(D2D1_DASH_STYLE& val) const noexcept
	{
		val = m_stroke_style;
		return true;
	}

	// 線枠の太さを得る.
	bool ShapeStroke::get_stroke_width(double& val) const noexcept
	{
		val = m_stroke_width;
		return true;
	}

	// 位置を含むか調べる.
	// 戻り値	つねに ANCH_OUTSIDE
	ANCH_WHICH ShapeStroke::hit_test(const D2D1_POINT_2F /*t_pos*/, const double /*a_len*/) const noexcept
	{
		return ANCH_OUTSIDE;
	}

	// 範囲に含まれるか調べる.
	// 戻り値	つねに false
	bool ShapeStroke::in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept
	{
		return false;
	}

	// 差分だけ移動する.
	void ShapeStroke::move(const D2D1_POINT_2F d)
	{
		pt_add(m_pos, d, m_pos);
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	void ShapeStroke::set_start_pos(const D2D1_POINT_2F pos)
	{
		m_pos = pos;
	}

	// 線枠の色に格納する.
	void ShapeStroke::set_stroke_color(const D2D1_COLOR_F& val) noexcept
	{
		m_stroke_color = val;
	}

	// 矢じりをデータライターに SVG タグとして書き込む.
	void ShapeStroke::write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, const ARROW_STYLE a_style, DataWriter const& dt_writer) const
	{
		using  winrt::GraphPaper::implementation::write_svg;

		write_svg("<path d=\"", dt_writer);
		write_svg("M", dt_writer);
		write_svg(barbs[0].x, dt_writer);
		write_svg(barbs[0].y, dt_writer);
		write_svg("L", dt_writer);
		write_svg(tip_pos.x, dt_writer);
		write_svg(tip_pos.y, dt_writer);
		write_svg("L", dt_writer);
		write_svg(barbs[1].x, dt_writer);
		write_svg(barbs[1].y, dt_writer);
		write_svg("\" ", dt_writer);
		if (a_style == ARROW_STYLE::FILLED) {
			write_svg(m_stroke_color, "fill", dt_writer);
		}
		else {
			write_svg("fill=\"none\" ", dt_writer);
		}
		write_svg(m_stroke_color, "stroke", dt_writer);
		write_svg(m_stroke_width, "stroke-width", dt_writer);
		write_svg(" />" SVG_NL, dt_writer);
	}

	// 値を破線の配置に格納する.
	void ShapeStroke::set_stroke_pattern(const STROKE_PATTERN& val)
	{
		if (equal(m_stroke_pattern, val)) {
			return;
		}
		m_stroke_pattern = val;
		m_d2d_stroke_style = nullptr;
		create_stroke_style(m_stroke_style, m_stroke_pattern, m_d2d_stroke_style.put());
	}

	// 値を線枠の形式に格納する.
	void ShapeStroke::set_stroke_style(const D2D1_DASH_STYLE val)
	{
		if (equal(m_stroke_style, val)) {
			return;
		}
		m_stroke_style = val;
		m_d2d_stroke_style = nullptr;
		create_stroke_style(m_stroke_style, m_stroke_pattern, m_d2d_stroke_style.put());
	}

	// 値を線枠の太さに格納する.
	void ShapeStroke::set_stroke_width(const double val) noexcept
	{
		m_stroke_width = val;
	}

	// 図形を作成する.
	ShapeStroke::ShapeStroke(const ShapePanel* attr) :
		m_stroke_color(attr->m_stroke_color),
		m_stroke_pattern(attr->m_stroke_pattern),
		m_stroke_style(attr->m_stroke_style),
		m_stroke_width(attr->m_stroke_width),
		m_d2d_stroke_style(nullptr)
	{
		create_stroke_style(m_stroke_style, m_stroke_pattern, m_d2d_stroke_style.put());
	}

	// 図形をデータリーダーから読み込む.
	ShapeStroke::ShapeStroke(DataReader const& dt_reader) :
		m_d2d_stroke_style(nullptr)
	{
		using winrt::GraphPaper::implementation::read;

		set_delete(dt_reader.ReadBoolean());
		set_select(dt_reader.ReadBoolean());
		read(m_pos, dt_reader);
		read(m_vec, dt_reader);
		read(m_stroke_color, dt_reader);
		read(m_stroke_pattern, dt_reader);
		read(m_stroke_style, dt_reader);
		m_stroke_width = dt_reader.ReadDouble();
		create_stroke_style(m_stroke_style, m_stroke_pattern, m_d2d_stroke_style.put());
	}

	// データライターに書き込む.
	void ShapeStroke::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		dt_writer.WriteBoolean(is_deleted());
		dt_writer.WriteBoolean(is_selected());
		write(m_pos, dt_writer);
		write(m_vec, dt_writer);
		write(m_stroke_color, dt_writer);
		dt_writer.WriteSingle(m_stroke_pattern.m_[0]);
		dt_writer.WriteSingle(m_stroke_pattern.m_[1]);
		dt_writer.WriteSingle(m_stroke_pattern.m_[2]);
		dt_writer.WriteSingle(m_stroke_pattern.m_[3]);
		dt_writer.WriteSingle(m_stroke_pattern.m_[4]);
		dt_writer.WriteSingle(m_stroke_pattern.m_[5]);
		dt_writer.WriteInt32(static_cast<int32_t>(m_stroke_style));
		dt_writer.WriteDouble(m_stroke_width);
	}

	// データライターに SVG タグとして書き込む.
	void ShapeStroke::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg(m_stroke_color, "stroke", dt_writer);
		write_svg(m_stroke_style, m_stroke_pattern, m_stroke_width, dt_writer);
		write_svg(m_stroke_width, "stroke-width", dt_writer);
	}

}