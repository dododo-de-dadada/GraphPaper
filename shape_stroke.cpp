#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{

	// D2D ストローク特性を作成する.
	static void create_stroke_style(ID2D1Factory3* const d_factory, const CAP_STYLE& s_cap_style, const D2D1_CAP_STYLE s_dash_cap, const D2D1_DASH_STYLE s_dash_style, const STROKE_DASH_PATT& s_dash_patt, const D2D1_LINE_JOIN s_join_style, const double s_join_limit, ID2D1StrokeStyle** s_stroke_style);

	// D2D ストローク特性を作成する.
	// s_cap_style	線の端点
	// s_dash_cap	破線の端点
	// s_dash_style	破線の種類
	// s_dash_patt	破線の配置配列
	// s_join_style	線のつながり
	// s_join_limit	マイター制限
	// s_style	作成されたストローク特性
	static void create_stroke_style(
		ID2D1Factory3* const d_factory,
		const CAP_STYLE& s_cap_style,
		const D2D1_CAP_STYLE s_dash_cap,
		const D2D1_DASH_STYLE s_dash_style,
		const STROKE_DASH_PATT& s_dash_patt,
		const D2D1_LINE_JOIN s_join_style,
		const double s_join_limit,
		ID2D1StrokeStyle** s_stroke_style
	)
	{
		UINT32 d_cnt;	// 破線の配置配列の要素数
		const FLOAT* d_arr;	// 破線の配置配列を指すポインタ
		D2D1_DASH_STYLE d_style;

		if (s_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT) {
			d_arr = s_dash_patt.m_ + 2;
			d_cnt = 2;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
		}
		else if (s_dash_style == D2D1_DASH_STYLE_DASH) {
			d_arr = s_dash_patt.m_;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
			d_cnt = 2;
		}
		else if (s_dash_style == D2D1_DASH_STYLE_DASH_DOT) {
			d_arr = s_dash_patt.m_;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
			d_cnt = 4;
		}
		else if (s_dash_style == D2D1_DASH_STYLE_DASH_DOT_DOT) {
			d_arr = s_dash_patt.m_;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
			d_cnt = 6;
		}
		else {
			d_arr = nullptr;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;
			d_cnt = 0;
		}
		const D2D1_STROKE_STYLE_PROPERTIES s_prop{
			s_cap_style.m_start,	// startCap
			s_cap_style.m_end,	// endCap
			s_dash_cap,	// dashCap
			s_join_style,	// lineJoin
			static_cast<FLOAT>(s_join_limit),	// miterLimit
			d_style,	// dashStyle
			0.0f,
		};
		winrt::check_hresult(d_factory->CreateStrokeStyle(s_prop, d_arr, d_cnt, s_stroke_style));
	}

	// 図形を破棄する.
	ShapeStroke::~ShapeStroke(void)
	{
		if (m_d2d_stroke_style != nullptr) {
			m_d2d_stroke_style = nullptr;
		}
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
	bool ShapeStroke::get_stroke_dash_cap(D2D1_CAP_STYLE& value) const noexcept
	{
		value = m_stroke_dash_cap;
		return true;
	}

	// 線分の端点を得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_cap_style(CAP_STYLE& value) const noexcept
	{
		value = m_stroke_cap_style;
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
	bool ShapeStroke::move(const D2D1_POINT_2F diff)
	{
		D2D1_POINT_2F new_pos;
		pt_add(m_pos, diff, new_pos);
		return set_start_pos(new_pos);
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	bool ShapeStroke::set_start_pos(const D2D1_POINT_2F value)
	{
		D2D1_POINT_2F new_pos;
		pt_round(value, PT_ROUND, new_pos);
		if (!equal(m_pos, new_pos)) {
			m_pos = new_pos;
			return true;
		}
		return false;
	}

	// 線枠の色に格納する.
	bool ShapeStroke::set_stroke_color(const D2D1_COLOR_F& value) noexcept
	{
		if (!equal(m_stroke_color, value)) {
			m_stroke_color = value;
			return true;
		}
		return false;
	}

	// 値を, 部位の位置に格納する. 他の部位の位置は動かない. 
	bool ShapeStroke::set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch)
	{
		// 図形の部位が頂点以外か判定する.
		const size_t d_cnt = m_diff.size();	// 差分の数
		if (anch >= ANCH_TYPE::ANCH_P0 && anch <= ANCH_TYPE::ANCH_P0 + d_cnt) {
			// 図形の部位が始点か判定する.
			if (anch == ANCH_TYPE::ANCH_P0) {
				D2D1_POINT_2F diff;
				pt_sub(value, m_pos, diff);
				pt_round(diff, PT_ROUND, diff);
				if (pt_abs2(diff) >= FLT_MIN) {
					pt_add(m_pos, diff, m_pos);
					pt_sub(m_diff[0], diff, m_diff[0]);
					return true;
				}
			}
			else {
				D2D1_POINT_2F a_pos;
				get_anch_pos(anch, a_pos);
				D2D1_POINT_2F diff;
				pt_sub(value, a_pos, diff);
				pt_round(diff, PT_ROUND, diff);
				if (pt_abs2(diff) >= FLT_MIN) {
					const size_t i = anch - ANCH_TYPE::ANCH_P0;
					pt_add(m_diff[i - 1], diff, m_diff[i - 1]);
					if (anch != ANCH_TYPE::ANCH_P0 + d_cnt) {
						pt_sub(m_diff[i], diff, m_diff[i]);
					}
					return true;
				}
			}
		}
		return false;
	}

	// 値を線分の端点に格納する.
	bool ShapeStroke::set_stroke_cap_style(const CAP_STYLE& value)
	{
		if (!equal(m_stroke_cap_style, value)) {
			m_stroke_cap_style = value;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			create_stroke_style(s_d2d_factory, m_stroke_cap_style, m_stroke_dash_cap, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_style.put());
			return true;
		}
		return false;
	}

	// 値を破線の端点に格納する.
	bool ShapeStroke::set_stroke_dash_cap(const D2D1_CAP_STYLE& value)
	{
		if (!equal(m_stroke_dash_cap, value)) {
			m_stroke_dash_cap = value;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			create_stroke_style(s_d2d_factory, m_stroke_cap_style, m_stroke_dash_cap, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_style.put());
			return true;
		}
		return false;
	}

	// 値を破線の配置に格納する.
	bool ShapeStroke::set_stroke_dash_patt(const STROKE_DASH_PATT& value)
	{
		if (!equal(m_stroke_dash_patt, value)) {
			m_stroke_dash_patt = value;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			create_stroke_style(s_d2d_factory, m_stroke_cap_style, m_stroke_dash_cap, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_style.put());
			return true;
		}
		return false;
	}

	// 値を線枠の形式に格納する.
	bool ShapeStroke::set_stroke_dash_style(const D2D1_DASH_STYLE value)
	{
		if (m_stroke_dash_style != value) {
			m_stroke_dash_style = value;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			create_stroke_style(s_d2d_factory, m_stroke_cap_style, m_stroke_dash_cap, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_style.put());
			return true;
		}
		return false;
	}

	// 値をマイター制限の比率に格納する.
	bool ShapeStroke::set_stroke_join_limit(const float& value)
	{
		if (!equal(m_stroke_join_limit, value)) {
			m_stroke_join_limit = value;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			create_stroke_style(s_d2d_factory, m_stroke_cap_style, m_stroke_dash_cap, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_style.put());
			return true;
		}
		return false;
	}

	// 値を線のつながりに格納する.
	bool ShapeStroke::set_stroke_join_style(const D2D1_LINE_JOIN& value)
	{
		if (m_stroke_join_style != value) {
			m_stroke_join_style = value;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			create_stroke_style(s_d2d_factory, m_stroke_cap_style, m_stroke_dash_cap, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_style.put());
			return true;
		}
		return false;
	}

	// 値を線枠の太さに格納する.
	bool ShapeStroke::set_stroke_width(const float value) noexcept
	{
		if (!equal(m_stroke_width, value)) {
			m_stroke_width = value;
			return true;
		}
		return false;
	}

	// 図形を作成する.
	// d_cnt	差分の個数 (最大値は N_GON_MAX - 1)
	// s_attr	属性値
	ShapeStroke::ShapeStroke(const size_t d_cnt, const ShapeSheet* s_attr) :
		m_diff(d_cnt <= N_GON_MAX - 1 ? d_cnt : N_GON_MAX - 1),
		m_stroke_dash_cap(s_attr->m_stroke_dash_cap),
		m_stroke_cap_style(s_attr->m_stroke_cap_style),
		m_stroke_color(s_attr->m_stroke_color),
		m_stroke_dash_patt(s_attr->m_stroke_dash_patt),
		m_stroke_dash_style(s_attr->m_stroke_dash_style),
		m_stroke_join_limit(s_attr->m_stroke_join_limit),
		m_stroke_join_style(s_attr->m_stroke_join_style),
		m_stroke_width(s_attr->m_stroke_width),
		m_d2d_stroke_style(nullptr)
	{
		create_stroke_style(s_d2d_factory, m_stroke_cap_style, m_stroke_dash_cap, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_style.put());
	}

	// 図形をデータリーダーから読み込む.
	ShapeStroke::ShapeStroke(DataReader const& dt_reader) :
		m_d2d_stroke_style(nullptr)
	{
		using winrt::GraphPaper::implementation::read;

		set_delete(dt_reader.ReadBoolean());
		set_select(dt_reader.ReadBoolean());
		read(m_pos, dt_reader);
		read(m_diff, dt_reader);
		read(m_stroke_cap_style, dt_reader);
		read(m_stroke_color, dt_reader);
		m_stroke_dash_cap = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
		read(m_stroke_dash_patt, dt_reader);
		m_stroke_dash_style = static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32());
		m_stroke_join_limit = dt_reader.ReadSingle();
		m_stroke_join_style = static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32());
		m_stroke_width = dt_reader.ReadSingle();
		create_stroke_style(s_d2d_factory, m_stroke_cap_style, m_stroke_dash_cap, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_style.put());
	}

	// データライターに書き込む.
	void ShapeStroke::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		dt_writer.WriteBoolean(is_deleted());
		dt_writer.WriteBoolean(is_selected());
		write(m_pos, dt_writer);
		write(m_diff, dt_writer);
		write(m_stroke_cap_style, dt_writer);
		write(m_stroke_color, dt_writer);
		dt_writer.WriteUInt32(m_stroke_dash_cap);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[0]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[1]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[2]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[3]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[4]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[5]);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_stroke_dash_style));
		dt_writer.WriteSingle(m_stroke_join_limit);
		dt_writer.WriteUInt32(m_stroke_join_style);
		dt_writer.WriteSingle(m_stroke_width);
	}

	// データライターに SVG タグとして書き込む.
	void ShapeStroke::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg(m_stroke_color, "stroke", dt_writer);
		write_svg(m_stroke_dash_style, m_stroke_dash_patt, m_stroke_width, dt_writer);
		write_svg(m_stroke_width, "stroke-width", dt_writer);
		if (equal(m_stroke_cap_style, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT })) {
			write_svg("stroke-linecap=\"butt\" ", dt_writer);
		}
		else if (equal(m_stroke_cap_style, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND })) {
			write_svg("stroke-linecap=\"round\" ", dt_writer);
		}
		else if (equal(m_stroke_cap_style, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE })) {
			write_svg("stroke-linecap=\"square\" ", dt_writer);
		}
		else if (equal(m_stroke_cap_style, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE })) {
			//write_svg("stroke-linecap=\"???\" ", dt_writer);
		}
		if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
			write_svg("stroke-linejoin=\"bevel\" ", dt_writer);
		}
		else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
			m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
			write_svg("stroke-linejoin=\"miter\" ", dt_writer);
			write_svg(m_stroke_join_limit, "stroke-miterlimit", dt_writer);
		}
		else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
			write_svg("stroke-linejoin=\"round\" ", dt_writer);
		}
	}

}