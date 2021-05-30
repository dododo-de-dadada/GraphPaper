#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// D2D ストローク特性を作成する.
	static void create_stroke_dash_style(ID2D1Factory3* const d_factory, const D2D1_CAP_STYLE s_cap_line, const D2D1_CAP_STYLE s_cap_dash, const D2D1_DASH_STYLE s_dash, const STROKE_DASH_PATT& s_patt, const D2D1_LINE_JOIN s_join, const double s_limit, ID2D1StrokeStyle** s_style);

	// D2D ストローク特性を作成する.
	// s_cap_line	線の端点
	// s_cap_dash	破線の端点
	// s_dash	破線の種類
	// s_patt	破線の配置配列
	// s_join	線のつながり
	// s_limit	マイター制限
	// s_style	作成されたストローク特性
	static void create_stroke_dash_style(
		ID2D1Factory3* const d_factory,
		const D2D1_CAP_STYLE s_cap_line,
		const D2D1_CAP_STYLE s_cap_dash,
		const D2D1_DASH_STYLE s_dash,
		const STROKE_DASH_PATT& s_patt,
		const D2D1_LINE_JOIN s_join,
		const double s_limit, ID2D1StrokeStyle** s_style)
	{
		UINT32 d_cnt;	// 破線の配置配列の要素数
		const FLOAT* d_arr;	// 破線の配置配列を指すポインタ

		if (s_dash != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID) {
			const D2D1_STROKE_STYLE_PROPERTIES s_prop{
				s_cap_line,	// startCap
				s_cap_line,	// endCap
				s_cap_dash,	// dashCap
				s_join,	// lineJoin
				static_cast<FLOAT>(s_limit),	// miterLimit
				D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM,	// dashStyle
				0.0f
			};
			if (s_dash == D2D1_DASH_STYLE_DOT) {
				d_arr = s_patt.m_ + 2;
				d_cnt = 2;
			}
			else {
				d_arr = s_patt.m_;
				if (s_dash == D2D1_DASH_STYLE_DASH) {
					d_cnt = 2;
				}
				else if (s_dash == D2D1_DASH_STYLE_DASH_DOT) {
					d_cnt = 4;
				}
				else if (s_dash == D2D1_DASH_STYLE_DASH_DOT_DOT) {
					d_cnt = 6;
				}
				else {
					d_cnt = 0;
				}
			}
			winrt::check_hresult(
				d_factory->CreateStrokeStyle(s_prop, d_arr, d_cnt, s_style)
			);
		}
		else {
			const D2D1_STROKE_STYLE_PROPERTIES s_prop{
				s_cap_line,	// startCap
				s_cap_line,	// endCap
				s_cap_dash,	// dashCap
				s_join,	// lineJoin
				static_cast<FLOAT>(s_limit),	// miterLimit
				D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID,	// dashStyle
				0.0f	// dashOffset
			};
			winrt::check_hresult(
				d_factory->CreateStrokeStyle(s_prop, nullptr, 0, s_style)
			);
		}
	}

	// 図形を破棄する.
	ShapeStroke::~ShapeStroke(void)
	{
		m_d2d_stroke_dash_style = nullptr;
	}

	// 図形を囲む領域を得る.
	// a_min	元の領域の左上位置.
	// a_man	元の領域の右下位置.
	// b_min	得られた領域の左上位置.
	// b_max	得られた領域の右下位置.
	void ShapeStroke::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		const size_t d_cnt = m_diff.size();	// 差分の数
		D2D1_POINT_2F e_pos = m_pos;
		b_min = a_min;
		b_max = a_max;
		pt_inc(e_pos, b_min, b_max);
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(e_pos, m_diff[i], e_pos);
			pt_inc(e_pos, b_min, b_max);
		}
	}

	// 図形を囲む領域の左上位置を得る.
	// value	領域の左上位置
	void ShapeStroke::get_min_pos(D2D1_POINT_2F& value) const noexcept
	{
		const size_t n = m_diff.size();	// 差分の数
		D2D1_POINT_2F v_pos = m_pos;	// 頂点の位置
		value = m_pos;
		for (size_t i = 0; i < n; i++) {
			pt_add(v_pos, m_diff[i], v_pos);
			pt_min(value, v_pos, value);
		}

		//value.x = m_diff[0].x >= 0.0f ? m_pos.x : m_pos.x + m_diff[0].x;
		//value.y = m_diff[0].y >= 0.0f ? m_pos.y : m_pos.y + m_diff[0].y;
	}

	// 指定された部位の位置を得る.
	void ShapeStroke::get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept
	{
		if (anch == ANCH_TYPE::ANCH_SHEET || anch == ANCH_TYPE::ANCH_P0) {
			// 図形の部位が「外部」または「開始点」ならば, 開始位置を得る.
			value = m_pos;
		}
		else if (anch > ANCH_TYPE::ANCH_P0) {
			const size_t m = m_diff.size() + 1;		// 頂点の数 (差分の数 + 1)
			if (anch < ANCH_TYPE::ANCH_P0 + m) {
				value = m_pos;
				for (size_t i = 0; i < anch - ANCH_TYPE::ANCH_P0; i++) {
					pt_add(value, m_diff[i], value);
				}
			}
		}
		//value = m_pos;
	}

	// 開始位置を得る
	// 戻り値	つねに true
	bool ShapeStroke::get_start_pos(D2D1_POINT_2F& value) const noexcept
	{
		value = m_pos;
		return true;
	}

	// 線枠の色を得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_stroke_color;
		return true;
	}

	// 線枠のマイター制限の比率を得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_join_limit(float& value) const noexcept
	{
		value = m_stroke_join_limit;
		return true;
	}

	// 線のつながりを得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_join_style(D2D1_LINE_JOIN& value) const noexcept
	{
		value = m_stroke_join_style;
		return true;
	}

	// 線のつながりを得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_cap_dash(D2D1_CAP_STYLE& value) const noexcept
	{
		value = m_stroke_cap_dash;
		return true;
	}

	// 線のつながりを得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_cap_line(D2D1_CAP_STYLE& value) const noexcept
	{
		value = m_stroke_cap_line;
		return true;
	}

	// 破線の配置を得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_dash_patt(STROKE_DASH_PATT& value) const noexcept
	{
		value = m_stroke_dash_patt;
		return true;
	}

	// 線枠の形式を得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_dash_style(D2D1_DASH_STYLE& value) const noexcept
	{
		value = m_stroke_dash_style;
		return true;
	}

	// 線枠の太さを得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_width(float& value) const noexcept
	{
		value = m_stroke_width;
		return true;
	}

	// 位置を含むか判定する.
	// 戻り値	つねに ANCH_SHEET
	uint32_t ShapeStroke::hit_test(const D2D1_POINT_2F /*t_pos*/, const double /*a_len*/) const noexcept
	{
		return ANCH_TYPE::ANCH_SHEET;
	}

	// 範囲に含まれるか判定する.
	// 戻り値	つねに false
	bool ShapeStroke::in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept
	{
		return false;
	}

	// 差分だけ移動する.
	void ShapeStroke::move(const D2D1_POINT_2F diff)
	{
		D2D1_POINT_2F s_pos;
		pt_add(m_pos, diff, s_pos);
		set_start_pos(s_pos);
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	void ShapeStroke::set_start_pos(const D2D1_POINT_2F value)
	{
		D2D1_POINT_2F s_pos;
		pt_round(value, PT_ROUND, s_pos);
		m_pos = s_pos;
	}

	// 線枠の色に格納する.
	void ShapeStroke::set_stroke_color(const D2D1_COLOR_F& value) noexcept
	{
		m_stroke_color = value;
	}

	// 矢じりをデータライターに SVG タグとして書き込む.
	// barbs	矢じりの両端の位置
	// tip_pos	矢じりの先端の位置
	// a_style	矢じりの形状
	// dt_writer	データライター
	void ShapeStroke::write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, const ARROWHEAD_STYLE a_style, DataWriter const& dt_writer) const
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
		if (a_style == ARROWHEAD_STYLE::FILLED) {
			write_svg(m_stroke_color, "fill", dt_writer);
		}
		else {
			write_svg("fill=\"none\" ", dt_writer);
		}
		write_svg(m_stroke_color, "stroke", dt_writer);
		write_svg(m_stroke_width, "stroke-width", dt_writer);
		write_svg(" />" SVG_NEW_LINE, dt_writer);
	}

	// 値を破線の端点に格納する.
	void ShapeStroke::set_stroke_cap_dash(const D2D1_CAP_STYLE& value)
	{
		if (equal(m_stroke_cap_dash, value)) {
			return;
		}
		m_stroke_cap_dash = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// 値を線の端点に格納する.
	void ShapeStroke::set_stroke_cap_line(const D2D1_CAP_STYLE& value)
	{
		if (equal(m_stroke_cap_line, value)) {
			return;
		}
		m_stroke_cap_line = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// 値をマイター制限の比率に格納する.
	void ShapeStroke::set_stroke_join_limit(const float& value)
	{
		if (equal(m_stroke_join_limit, value)) {
			return;
		}
		m_stroke_join_limit = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// 値を線のつながりに格納する.
	void ShapeStroke::set_stroke_join_style(const D2D1_LINE_JOIN& value)
	{
		if (equal(m_stroke_join_style, value)) {
			return;
		}
		m_stroke_join_style = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// 値を破線の配置に格納する.
	void ShapeStroke::set_stroke_dash_patt(const STROKE_DASH_PATT& value)
	{
		if (equal(m_stroke_dash_patt, value)) {
			return;
		}
		m_stroke_dash_patt = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// 値を線枠の形式に格納する.
	void ShapeStroke::set_stroke_dash_style(const D2D1_DASH_STYLE value)
	{
		if (m_stroke_dash_style == value) {
			return;
		}
		m_stroke_dash_style = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// 値を線枠の太さに格納する.
	void ShapeStroke::set_stroke_width(const float value) noexcept
	{
		m_stroke_width = value;
	}

	// 図形を作成する.
	// d_cnt	差分の個数 (最大値は N_GON_MAX - 1)
	// s_attr	属性値
	ShapeStroke::ShapeStroke(const size_t d_cnt, const ShapeSheet* s_attr) :
		m_diff(d_cnt <= N_GON_MAX - 1 ? d_cnt : N_GON_MAX - 1),
		m_stroke_cap_dash(s_attr->m_stroke_cap_dash),
		m_stroke_cap_line(s_attr->m_stroke_cap_line),
		m_stroke_color(s_attr->m_stroke_color),
		m_stroke_dash_patt(s_attr->m_stroke_dash_patt),
		m_stroke_dash_style(s_attr->m_stroke_dash_style),
		m_stroke_join_limit(s_attr->m_stroke_join_limit),
		m_stroke_join_style(s_attr->m_stroke_join_style),
		m_stroke_width(s_attr->m_stroke_width),
		m_d2d_stroke_dash_style(nullptr)
	{
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// 図形をデータリーダーから読み込む.
	ShapeStroke::ShapeStroke(DataReader const& dt_reader) :
		m_d2d_stroke_dash_style(nullptr)
	{
		using winrt::GraphPaper::implementation::read;

		set_delete(dt_reader.ReadBoolean());
		set_select(dt_reader.ReadBoolean());
		read(m_pos, dt_reader);
		read(m_diff, dt_reader);
		m_stroke_cap_dash = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
		m_stroke_cap_line = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
		read(m_stroke_color, dt_reader);
		m_stroke_join_limit = dt_reader.ReadSingle();
		m_stroke_join_style = static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32());
		read(m_stroke_dash_patt, dt_reader);
		m_stroke_dash_style = static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32());
		m_stroke_width = dt_reader.ReadSingle();
		create_stroke_dash_style(s_d2d_factory, m_stroke_cap_line, m_stroke_cap_dash, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// データライターに書き込む.
	void ShapeStroke::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		dt_writer.WriteBoolean(is_deleted());
		dt_writer.WriteBoolean(is_selected());
		write(m_pos, dt_writer);
		write(m_diff, dt_writer);
		dt_writer.WriteUInt32(m_stroke_cap_dash);
		dt_writer.WriteUInt32(m_stroke_cap_line);
		write(m_stroke_color, dt_writer);
		dt_writer.WriteSingle(m_stroke_join_limit);
		dt_writer.WriteUInt32(m_stroke_join_style);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[0]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[1]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[2]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[3]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[4]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[5]);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_stroke_dash_style));
		dt_writer.WriteSingle(m_stroke_width);
	}

	// データライターに SVG タグとして書き込む.
	void ShapeStroke::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg(m_stroke_color, "stroke", dt_writer);
		write_svg(m_stroke_dash_style, m_stroke_dash_patt, m_stroke_width, dt_writer);
		write_svg(m_stroke_width, "stroke-width", dt_writer);
		write_svg("stroke-linejoin=\"miter\" stroke-miterlimit=\"1\" ", dt_writer);
	}

}