#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	// 図形を破棄する.
	ShapeStroke::~ShapeStroke(void)
	{
		if (m_d2d_stroke_style != nullptr) {
			//m_d2d_stroke_style->Release();
			m_d2d_stroke_style = nullptr;
		}
	}

	// 図形を囲む領域を得る.
	// a_min	元の領域の左上位置.
	// a_max	元の領域の右下位置.
	// b_min	囲む領域の左上位置.
	// b_max	囲む領域の右下位置.
	void ShapeStroke::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		b_min.x = m_pos.x < a_min.x ? m_pos.x : a_min.x;
		b_min.y = m_pos.y < a_min.y ? m_pos.y : a_min.y;
		b_max.x = m_pos.x > a_max.x ? m_pos.x : a_max.x;
		b_max.y = m_pos.y > a_max.y ? m_pos.y : a_max.y;
		const size_t d_cnt = m_vec.size();	// 差分の数
		D2D1_POINT_2F pos = m_pos;
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(pos, m_vec[i], pos);
			//pt_inc(e_pos, b_min, b_max);
			if (pos.x < b_min.x) {
				b_min.x = pos.x;
			}
			if (pos.x > b_max.x) {
				b_max.x = pos.x;
			}
			if (pos.y < b_min.y) {
				b_min.y = pos.y;
			}
			if (pos.y > b_max.y) {
				b_max.y = pos.y;
			}
		}
	}

	// 端の形式を得る.
	// val	得られた値
	// 戻り値	つねに true
	bool ShapeStroke::get_stroke_cap(CAP_STYLE& val) const noexcept
	{
		val = m_stroke_cap;
		return true;
	}

	// 線のつなぎを得る.
	// val	得られた値
	// 戻り値	つねに true
	bool ShapeStroke::get_dash_cap(D2D1_CAP_STYLE& val) const noexcept
	{
		val = m_dash_cap;
		return true;
	}

	// 破線の配置を得る.
	// val	得られた値
	// 戻り値	つねに true
	bool ShapeStroke::get_dash_patt(DASH_PATT& val) const noexcept
	{
		val = m_dash_patt;
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

	// 線分のつなぎのマイター制限を得る.
	// val	得られた値
	// 戻り値	つねに true
	bool ShapeStroke::get_join_limit(float& val) const noexcept
	{
		val = m_join_limit;
		return true;
	}

	// 線分のつなぎを得る.
	// val	得られた値
	// 戻り値	つねに true
	bool ShapeStroke::get_join_style(D2D1_LINE_JOIN& val) const noexcept
	{
		val = m_join_style;
		return true;
	}

	// 近傍の頂点を見つける.
	// pos	ある位置
	// dd	近傍とみなす距離 (の二乗値), これより離れた頂点は近傍とはみなさない.
	// val	ある位置の近傍にある頂点
	// 戻り値	見つかったら true
	bool ShapeStroke::get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool done = false;
		D2D1_POINT_2F vec;
		pt_sub(m_pos, pos, vec);
		float vv = static_cast<float>(pt_abs2(vec));
		if (vv < dd) {
			dd = vv;
			val = m_pos;
			if (!done) {
				done = true;
			}
		}
		D2D1_POINT_2F v_pos{ m_pos };
		for (const auto d_vec : m_vec) {
			pt_add(v_pos, d_vec, v_pos);
			pt_sub(v_pos, pos, vec);
			vv = static_cast<float>(pt_abs2(vec));
			if (vv < dd) {
				dd = vv;
				val = v_pos;
				if (!done) {
					done = true;
				}
			}
		}
		return done;
	}

	// 指定された部位の位置を得る.
	// anc	図形の部位
	// val	得られた位置
	void ShapeStroke::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		if (anc == ANC_TYPE::ANC_SHEET || anc == ANC_TYPE::ANC_P0) {
			// 図形の部位が「外部」または「開始点」ならば, 開始位置を得る.
			val = m_pos;
		}
		else if (anc > ANC_TYPE::ANC_P0) {
			const size_t a_cnt = anc - ANC_TYPE::ANC_P0;
			if (a_cnt < m_vec.size() + 1) {
				val = m_pos;
				for (size_t i = 0; i < a_cnt; i++) {
					pt_add(val, m_vec[i], val);
				}
			}
		}
	}

	// 図形を囲む領域の左上位置を得る.
	// val	領域の左上位置
	void ShapeStroke::get_pos_min(D2D1_POINT_2F& val) const noexcept
	{
		const size_t d_cnt = m_vec.size();	// 差分の数
		D2D1_POINT_2F v_pos = m_pos;	// 頂点の位置
		val = m_pos;
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(v_pos, m_vec[i], v_pos);
			//pt_min(val, v_pos, val);
			val.x = val.x < v_pos.x ? val.x : v_pos.x;
			val.y = val.y < v_pos.y ? val.y : v_pos.y;
		}
	}

	// 開始位置を得る
	// 戻り値	つねに true
	bool ShapeStroke::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		val = m_pos;
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

	// 頂点を得る.
	size_t ShapeStroke::get_verts(D2D1_POINT_2F v_pos[]) const noexcept
	{
		v_pos[0] = m_pos;
		const size_t d_cnt = m_vec.size();
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(v_pos[i], m_vec[i], v_pos[i + 1]);
		}
		return d_cnt + 1;
	}

	// 位置を含むか判定する.
	// 戻り値	つねに ANC_SHEET
	uint32_t ShapeStroke::hit_test(const D2D1_POINT_2F /*t_pos*/) const noexcept
	{
		return ANC_TYPE::ANC_SHEET;
	}

	// 範囲に含まれるか判定する.
	// 戻り値	つねに false
	bool ShapeStroke::in_area(const D2D1_POINT_2F /*area_min*/, const D2D1_POINT_2F /*area_max*/) const noexcept
	{
		return false;
	}

	// 差分だけ移動する.
	// d_vec	差分ベクトル
	bool ShapeStroke::move(const D2D1_POINT_2F d_vec) noexcept
	{
		D2D1_POINT_2F new_pos;
		pt_add(m_pos, d_vec, new_pos);
		return set_pos_start(new_pos);
	}

	// D2D ストロークスタイルを作成する.
	void ShapeStroke::create_stroke_style(D2D_UI const& d2d)
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
			d_arr[0] = m_dash_patt.m_[2] / m_stroke_width;
			d_arr[1] = m_dash_patt.m_[3] / m_stroke_width;
			d_ptr = d_arr;
			d_cnt = 2;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
		}
		else if (m_dash_style == D2D1_DASH_STYLE_DASH) {
			d_arr[0] = m_dash_patt.m_[0] / m_stroke_width;
			d_arr[1] = m_dash_patt.m_[1] / m_stroke_width;
			d_ptr = d_arr;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
			d_cnt = 2;
		}
		else if (m_dash_style == D2D1_DASH_STYLE_DASH_DOT) {
			d_arr[0] = m_dash_patt.m_[0] / m_stroke_width;
			d_arr[1] = m_dash_patt.m_[1] / m_stroke_width;
			d_arr[2] = m_dash_patt.m_[2] / m_stroke_width;
			d_arr[3] = m_dash_patt.m_[3] / m_stroke_width;
			d_ptr = d_arr;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
			d_cnt = 4;
		}
		else if (m_dash_style == D2D1_DASH_STYLE_DASH_DOT_DOT) {
			d_arr[0] = m_dash_patt.m_[0] / m_stroke_width;
			d_arr[1] = m_dash_patt.m_[1] / m_stroke_width;
			d_arr[2] = m_dash_patt.m_[2] / m_stroke_width;
			d_arr[3] = m_dash_patt.m_[3] / m_stroke_width;
			d_arr[4] = m_dash_patt.m_[4] / m_stroke_width;
			d_arr[5] = m_dash_patt.m_[5] / m_stroke_width;
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
			m_stroke_cap.m_start,	// startCap
			m_stroke_cap.m_end,	// endCap
			m_dash_cap,	// dashCap
			m_join_style,	// lineJoin
			m_join_limit,	// miterLimit
			d_style,	// dashStyle
			0.0f,
		};
		winrt::check_hresult(d2d.m_d2d_factory->CreateStrokeStyle(s_prop, d_ptr, d_cnt, m_d2d_stroke_style.put()));
	}

	// 値を端の形式に格納する.
	bool ShapeStroke::set_stroke_cap(const CAP_STYLE& val) noexcept
	{
		if (!equal(m_stroke_cap, val)) {
			m_stroke_cap = val;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// 値を破線の端の形式に格納する.
	bool ShapeStroke::set_dash_cap(const D2D1_CAP_STYLE& val) noexcept
	{
		if (!equal(m_dash_cap, val)) {
			m_dash_cap = val;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// 値を破線の配置に格納する.
	// val	格納する値
	bool ShapeStroke::set_dash_patt(const DASH_PATT& val) noexcept
	{
		if (!equal(m_dash_patt, val)) {
			m_dash_patt = val;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
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

	// 値を線分のつなぎのマイター制限に格納する.
	// val	格納する値
	bool ShapeStroke::set_join_limit(const float& val) noexcept
	{
		if (!equal(m_join_limit, val)) {
			m_join_limit = val;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// 値を線分のつなぎに格納する.
	// val	格納する値
	bool ShapeStroke::set_join_style(const D2D1_LINE_JOIN& val)  noexcept
	{
		if (m_join_style != val) {
			m_join_style = val;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// 値を, 部位の位置に格納する.
	// val	値
	// anc	図形の部位
	// limit	限界距離 (他の頂点との距離がこの値未満になるなら, その頂点に位置に合わせる)
	bool ShapeStroke::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool /*keep_aspect*/) noexcept
	{
		bool done = false;
		// 変更する頂点がどの頂点か判定する.
		const size_t d_cnt = m_vec.size();	// 差分の数
		if (anc >= ANC_TYPE::ANC_P0 && anc <= ANC_TYPE::ANC_P0 + d_cnt) {
			D2D1_POINT_2F v_pos[MAX_N_GON];	// 頂点の位置
			const size_t a_cnt = anc - ANC_TYPE::ANC_P0;	// 変更する頂点
			// 変更する頂点までの, 各頂点の位置を得る.
			v_pos[0] = m_pos;
			for (size_t i = 0; i < a_cnt; i++) {
				pt_add(v_pos[i], m_vec[i], v_pos[i + 1]);
			}
			// 値から変更前の位置を引き, 変更する差分を得る.
			D2D1_POINT_2F vec;
			pt_sub(val, v_pos[a_cnt], vec);
			pt_round(vec, PT_ROUND, vec);
			// 差分の長さがゼロより大きいか判定する.
			if (pt_abs2(vec) >= FLT_MIN) {
				// 変更する頂点が最初の頂点か判定する.
				if (a_cnt == 0) {
					// 最初の頂点の位置に変更分を加える.
					pt_add(m_pos, vec, m_pos);
				}
				else {
					// 頂点の直前の差分に変更分を加える.
					pt_add(m_vec[a_cnt - 1], vec, m_vec[a_cnt - 1]);
				}
				// 変更するのが最後の頂点以外か判定する.
				if (a_cnt < d_cnt) {
					// 次の頂点が動かないように,
					// 変更する頂点の次の頂点への差分から変更分を引く.
					pt_sub(m_vec[a_cnt], vec, m_vec[a_cnt]);
				}
				if (!done) {
					done = true;
				}
			}
			// 限界距離がゼロでないか判定する.
			if (limit >= FLT_MIN) {
				// 残りの頂点の位置を得る.
				for (size_t i = a_cnt; i < d_cnt; i++) {
					pt_add(v_pos[i], m_vec[i], v_pos[i + 1]);
				}
				for (size_t i = 0; i < d_cnt + 1; i++) {
					// 頂点が, 変更する頂点か判定する.
					if (i == a_cnt) {
						continue;
					}
					// 頂点と変更する頂点との距離が限界距離以上か判定する.
					D2D1_POINT_2F v_vec;
					pt_sub(v_pos[i], v_pos[a_cnt], v_vec);
					const double d = static_cast<double>(limit);
					if (pt_abs2(v_vec) >= d * d) {
						continue;
					}
					// 変更するのが最初の頂点か判定する.
					if (a_cnt == 0) {
						pt_add(m_pos, v_vec, m_pos);
					}
					else {
						pt_add(m_vec[a_cnt - 1], v_vec, m_vec[a_cnt - 1]);
					}
					// 変更するのが最後の頂点以外か判定する.
					if (a_cnt < d_cnt) {
						pt_sub(m_vec[a_cnt], v_vec, m_vec[a_cnt]);
					}
					if (!done) {
						done = true;
					}
					break;
				}
			}
		}
		return done;
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	bool ShapeStroke::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F new_pos;
		pt_round(val, PT_ROUND, new_pos);
		if (!equal(m_pos, new_pos)) {
			m_pos = new_pos;
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
	// s_attr	属性値
	ShapeStroke::ShapeStroke(const ShapeSheet* s_attr) :
		ShapeSelect(),
		m_dash_cap(s_attr->m_dash_cap),
		m_stroke_cap(s_attr->m_stroke_cap),
		m_stroke_color(s_attr->m_stroke_color),
		m_dash_patt(s_attr->m_dash_patt),
		m_dash_style(s_attr->m_dash_style),
		m_join_limit(s_attr->m_join_limit),
		m_join_style(s_attr->m_join_style),
		m_stroke_width(s_attr->m_stroke_width),
		m_d2d_stroke_style(nullptr)
	{}

	template <typename T>
	static std::vector<T> dt_read_vec(DataReader const& dt_reader)
	{
		std::vector<T> vec;
		dt_read(vec, dt_reader);
		return vec;
	}

	static CAP_STYLE dt_read_cap(DataReader const& dt_reader)
	{
		CAP_STYLE cap;
		dt_read(cap, dt_reader);
		return cap;
	}

	// 図形をデータリーダーから読み込む.
	ShapeStroke::ShapeStroke(DataReader const& dt_reader) :
		ShapeSelect(dt_reader),
		m_pos(D2D1_POINT_2F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_vec(dt_read_vec<D2D1_POINT_2F>(dt_reader)),
		m_stroke_cap(dt_read_cap(dt_reader)),
		m_stroke_color(D2D1_COLOR_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_dash_cap(static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32())),
		m_dash_patt(DASH_PATT{ dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_dash_style(static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32())),
		m_join_limit(dt_reader.ReadSingle()),
		m_join_style(static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32())),
		m_stroke_width(dt_reader.ReadSingle()),
		m_d2d_stroke_style(nullptr)
	{}

	// 図形をデータライターに書き込む.
	void ShapeStroke::write(DataWriter const& dt_writer) const
	{
		dt_writer.WriteBoolean(m_is_deleted);
		dt_writer.WriteBoolean(m_is_selected);
		dt_write(m_pos, dt_writer);
		dt_write(m_vec, dt_writer);
		dt_write(m_stroke_cap, dt_writer);
		dt_write(m_stroke_color, dt_writer);
		dt_writer.WriteUInt32(m_dash_cap);
		dt_writer.WriteSingle(m_dash_patt.m_[0]);
		dt_writer.WriteSingle(m_dash_patt.m_[1]);
		dt_writer.WriteSingle(m_dash_patt.m_[2]);
		dt_writer.WriteSingle(m_dash_patt.m_[3]);
		dt_writer.WriteSingle(m_dash_patt.m_[4]);
		dt_writer.WriteSingle(m_dash_patt.m_[5]);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_dash_style));
		dt_writer.WriteSingle(m_join_limit);
		dt_writer.WriteUInt32(m_join_style);
		dt_writer.WriteSingle(m_stroke_width);
	}

	// データライターに SVG タグとして書き込む.
	void ShapeStroke::write_svg(DataWriter const& dt_writer) const
	{
		dt_write_svg(m_stroke_color, "stroke", dt_writer);
		dt_write_svg(m_dash_style, m_dash_patt, m_stroke_width, dt_writer);
		dt_write_svg(m_stroke_width, "stroke-width", dt_writer);
		if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT })) {
			dt_write_svg("stroke-linecap=\"butt\" ", dt_writer);
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND })) {
			dt_write_svg("stroke-linecap=\"round\" ", dt_writer);
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE })) {
			dt_write_svg("stroke-linecap=\"square\" ", dt_writer);
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE })) {
			//dt_write_svg("stroke-linecap=\"???\" ", dt_writer);
		}
		if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
			dt_write_svg("stroke-linejoin=\"bevel\" ", dt_writer);
		}
		else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
			m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
			dt_write_svg("stroke-linejoin=\"miter\" ", dt_writer);
			dt_write_svg(m_join_limit, "stroke-miterlimit", dt_writer);
		}
		else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
			dt_write_svg("stroke-linejoin=\"round\" ", dt_writer);
		}
	}

}