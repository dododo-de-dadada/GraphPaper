#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 端の形式を得る.
	// val	得られた値
	// 戻り値	つねに true
	//bool ShapeStroke::get_stroke_cap(CAP_STYLE& val) const noexcept
	bool ShapeStroke::get_stroke_cap(D2D1_CAP_STYLE& val) const noexcept
	{
		val = m_stroke_cap;
		return true;
	}

	// 線の結合を得る.
	// val	得られた値
	// 戻り値	つねに true
	//bool ShapeStroke::get_dash_cap(D2D1_CAP_STYLE& val) const noexcept
	//{
	//	val = m_dash_cap;
	//	return true;
	//}

	// 破線の配置を得る.
	// val	得られた値
	// 戻り値	つねに true
	bool ShapeStroke::get_dash_pat(DASH_PAT& val) const noexcept
	{
		val = m_dash_pat;
		return true;
	}

	// 線枠の形式を得る.
	// val	得られた値
	// 戻り値	つねに true
	bool ShapeStroke::get_dash_style(D2D1_DASH_STYLE& val) const noexcept
	{
		val = m_dash_style;
		return true;
	}

	// 線分の結合の尖り制限を得る.
	// val	得られた値
	// 戻り値	つねに true
	bool ShapeStroke::get_join_miter_limit(float& val) const noexcept
	{
		val = m_join_miter_limit;
		return true;
	}

	// 線分の結合を得る.
	// val	得られた値
	// 戻り値	つねに true
	bool ShapeStroke::get_join_style(D2D1_LINE_JOIN& val) const noexcept
	{
		val = m_join_style;
		return true;
	}

	// 線枠の色を得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_stroke_color;
		return true;
	}

	// 線枠の太さを得る.
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_width(float& val) const noexcept
	{
		val = m_stroke_width;
		return true;
	}

	// 図形が点を含むか判定する.
	// 戻り値	つねに LOC_PAGE
	uint32_t ShapeStroke::hit_test(const D2D1_POINT_2F /*t_pos*/) const noexcept
	{
		return LOC_TYPE::LOC_PAGE;
	}

	// D2D ストロークスタイルを作成する.
	void ShapeStroke::create_stroke_style(ID2D1Factory* const factory)
	{
		UINT32 d_cnt;	// 破線の配置の要素数
		FLOAT d_arr[6];	// 破線の配置
		FLOAT* d_ptr;
		D2D1_DASH_STYLE d_style;

		// 太さがゼロか判定する.
		if (m_stroke_width < FLT_MIN) {
			return;
		}
		if (m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT) {
			d_arr[0] = m_dash_pat.m_[2] / m_stroke_width;
			d_arr[1] = m_dash_pat.m_[3] / m_stroke_width;
			d_ptr = d_arr;
			d_cnt = 2;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
		}
		else if (m_dash_style == D2D1_DASH_STYLE_DASH) {
			d_arr[0] = m_dash_pat.m_[0] / m_stroke_width;
			d_arr[1] = m_dash_pat.m_[1] / m_stroke_width;
			d_ptr = d_arr;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
			d_cnt = 2;
		}
		else if (m_dash_style == D2D1_DASH_STYLE_DASH_DOT) {
			d_arr[0] = m_dash_pat.m_[0] / m_stroke_width;
			d_arr[1] = m_dash_pat.m_[1] / m_stroke_width;
			d_arr[2] = m_dash_pat.m_[2] / m_stroke_width;
			d_arr[3] = m_dash_pat.m_[3] / m_stroke_width;
			d_ptr = d_arr;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
			d_cnt = 4;
		}
		else if (m_dash_style == D2D1_DASH_STYLE_DASH_DOT_DOT) {
			d_arr[0] = m_dash_pat.m_[0] / m_stroke_width;
			d_arr[1] = m_dash_pat.m_[1] / m_stroke_width;
			d_arr[2] = m_dash_pat.m_[2] / m_stroke_width;
			d_arr[3] = m_dash_pat.m_[3] / m_stroke_width;
			d_arr[4] = m_dash_pat.m_[4] / m_stroke_width;
			d_arr[5] = m_dash_pat.m_[5] / m_stroke_width;
			d_ptr = d_arr;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
			d_cnt = 6;
		}
		else {
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;
			d_ptr = nullptr;
			d_cnt = 0;
		}
		const D2D1_STROKE_STYLE_PROPERTIES s_prop{
			m_stroke_cap,	// startCap
			m_stroke_cap,	// endCap
			//m_dash_cap,	// dashCap
			m_stroke_cap,	// dashCap
			m_join_style,	// lineJoin
			m_join_miter_limit,	// miterLimit
			d_style,	// dashStyle
			0.0f,
		};
		winrt::check_hresult(
			factory->CreateStrokeStyle(s_prop, d_ptr, d_cnt, m_d2d_stroke_style.put())
		);
	}

	// 値を端の形式に格納する.
	//bool ShapeStroke::set_stroke_cap(const CAP_STYLE& val) noexcept
	bool ShapeStroke::set_stroke_cap(const D2D1_CAP_STYLE& val) noexcept
	{
		if (!equal(m_stroke_cap, val)) {
			m_stroke_cap = val;
			m_d2d_stroke_style = nullptr;
			return true;
		}
		return false;
	}

	// 値を破線の端の形式に格納する.
	//bool ShapeStroke::set_dash_cap(const D2D1_CAP_STYLE& val) noexcept
	//{
	//	if (!equal(m_dash_cap, val)) {
	//		m_dash_cap = val;
	//		m_d2d_stroke_style = nullptr;
	//		return true;
	//	}
	//	return false;
	//}

	// 値を破線の配置に格納する.
	// val	格納する値
	bool ShapeStroke::set_dash_pat(const DASH_PAT& val) noexcept
	{
		if (!equal(m_dash_pat, val)) {
			m_dash_pat = val;
			m_d2d_stroke_style = nullptr;
			return true;
		}
		return false;
	}

	// 値を線枠の形式に格納する.
	// val	格納する値
	bool ShapeStroke::set_dash_style(const D2D1_DASH_STYLE val) noexcept
	{
		if (m_dash_style != val) {
			m_dash_style = val;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// 値を線分の結合の尖り制限に格納する.
	// val	格納する値
	bool ShapeStroke::set_join_miter_limit(const float& val) noexcept
	{
		if (!equal(m_join_miter_limit, val)) {
			m_join_miter_limit = val;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// 値を線分の結合に格納する.
	// val	格納する値
	bool ShapeStroke::set_join_style(const D2D1_LINE_JOIN& val)  noexcept
	{
		if (m_join_style != val) {
			m_join_style = val;
			m_d2d_stroke_style = nullptr;
			return true;
		}
		return false;
	}

	// 線枠の色に格納する.
	bool ShapeStroke::set_stroke_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_stroke_color, val)) {
			m_stroke_color = val;
			return true;
		}
		return false;
	}

	// 値を線枠の太さに格納する.
	// val	格納する値
	bool ShapeStroke::set_stroke_width(const float val) noexcept
	{
		if (!equal(m_stroke_width, val)) {
			m_stroke_width = val;
			if (m_dash_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID) {
				if (m_d2d_stroke_style != nullptr) {
					m_d2d_stroke_style = nullptr;
				}
			}
			return true;
		}
		return false;
	}

	// 図形を作成する.
	// prop	設定
	ShapeStroke::ShapeStroke(const Shape* prop)
	{
		//prop->get_dash_cap(m_dash_cap);
		prop->get_stroke_cap(m_stroke_cap);
		prop->get_stroke_color(m_stroke_color);
		prop->get_dash_pat(m_dash_pat);
		prop->get_dash_style(m_dash_style);
		prop->get_join_miter_limit(m_join_miter_limit);
		prop->get_join_style(m_join_style);
		prop->get_stroke_width(m_stroke_width);
		m_d2d_stroke_style = nullptr;
	}

	// 図形をデータリーダーから読み込む.
	ShapeStroke::ShapeStroke(DataReader const& dt_reader) :
		// 読み込む順番は定義された順
		ShapeSelect(dt_reader),
		//m_stroke_cap(CAP_STYLE{
		//	static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32()),
		//	static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32())
		//}),
		m_stroke_cap(static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32())),
		m_stroke_color(D2D1_COLOR_F{
			dt_reader.ReadSingle(), 
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		}),
		m_stroke_width(dt_reader.ReadSingle()),
		//m_dash_cap(static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32())),
		m_dash_pat(DASH_PAT{
			{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
			}
		}),
		m_dash_style(static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32())),
		m_join_miter_limit(dt_reader.ReadSingle()),
		m_join_style(static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32())),
		m_d2d_stroke_style(nullptr)
	{
		// 値が無効なら, ページの属性の値を図形に格納する.
		//if ((m_stroke_cap.m_start != D2D1_CAP_STYLE_FLAT &&
		//	m_stroke_cap.m_start != D2D1_CAP_STYLE_ROUND &&
		//	m_stroke_cap.m_start != D2D1_CAP_STYLE_SQUARE &&
		//	m_stroke_cap.m_start != D2D1_CAP_STYLE_TRIANGLE) ||
		//	m_stroke_cap.m_start != m_stroke_cap.m_end) {
		//	m_stroke_cap = CAP_STYLE_FLAT;
		//}
		if (m_stroke_cap != D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT &&
			m_stroke_cap != D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND &&
			m_stroke_cap != D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE &&
			m_stroke_cap != D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
			m_stroke_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;
		}
		if (m_stroke_color.r < 0.0f || m_stroke_color.r > 1.0f ||
			m_stroke_color.g < 0.0f || m_stroke_color.g > 1.0f ||
			m_stroke_color.b < 0.0f || m_stroke_color.b > 1.0f ||
			m_stroke_color.a < 0.0f || m_stroke_color.a > 1.0f) {
			m_stroke_color = COLOR_BLACK;
		}
		if (m_stroke_width < 0.0f) {
			m_stroke_width = 1.0f;
		}
		//if (m_dash_cap != D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT &&
		//	m_dash_cap != D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND &&
		//	m_dash_cap != D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE &&
		//	m_dash_cap != D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
		//	m_dash_cap = D2D1_CAP_STYLE_FLAT;
		//}
		if (m_dash_pat.m_[0] < 0.0f ||
			m_dash_pat.m_[1] < 0.0f ||
			m_dash_pat.m_[2] < 0.0f ||
			m_dash_pat.m_[3] < 0.0f ||
			m_dash_pat.m_[4] < 0.0f ||
			m_dash_pat.m_[5] < 0.0f) {
			m_dash_pat = DASH_PAT_DEFVAL;
		}
		if (m_dash_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID &&
			m_dash_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH &&
			m_dash_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT &&
			m_dash_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT &&
			m_dash_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT) {
			m_dash_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;
		}
		if (m_join_miter_limit < 0.0f) {
			m_join_miter_limit = JOIN_MITER_LIMIT_DEFVAL;
		}
		if (m_join_style != D2D1_LINE_JOIN_BEVEL &&
			m_join_style != D2D1_LINE_JOIN_ROUND &&
			m_join_style != D2D1_LINE_JOIN_MITER &&
			m_join_style != D2D1_LINE_JOIN_MITER_OR_BEVEL) {
			m_join_style = JOIN_STYLE_DEFVAL;
		}
	}

	// 図形をデータライターに書き込む.
	void ShapeStroke::write(DataWriter const& dt_writer) const
	{
		ShapeSelect::write(dt_writer);
		// 線の端の形式
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_stroke_cap));
		// 線・枠の色
		dt_writer.WriteSingle(m_stroke_color.r);
		dt_writer.WriteSingle(m_stroke_color.g);
		dt_writer.WriteSingle(m_stroke_color.b);
		dt_writer.WriteSingle(m_stroke_color.a);
		// 線・枠の太さ
		dt_writer.WriteSingle(m_stroke_width);
		// 破線の端の形式
		//dt_writer.WriteUInt32(static_cast<uint32_t>(m_dash_cap));
		// 破線の配置
		dt_writer.WriteSingle(m_dash_pat.m_[0]);
		dt_writer.WriteSingle(m_dash_pat.m_[1]);
		dt_writer.WriteSingle(m_dash_pat.m_[2]);
		dt_writer.WriteSingle(m_dash_pat.m_[3]);
		dt_writer.WriteSingle(m_dash_pat.m_[4]);
		dt_writer.WriteSingle(m_dash_pat.m_[5]);
		// 破線の形式
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_dash_style));
		// 線の結合の尖り制限
		dt_writer.WriteSingle(m_join_miter_limit);
		// 線の結合の形式
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_join_style));
	}

}