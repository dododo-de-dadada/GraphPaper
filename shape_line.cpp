//------------------------------
// Shape_line.cpp
// 直線と矢じるし
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 矢じるしの D2D1 パスジオメトリを作成する.
	static void line_create_arrow_geom(ID2D1Factory3* const d_factory, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_vec, ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo);
	// 矢じるしの D2D ストローク特性を作成する.
	static void line_create_arrow_style(ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, const D2D1_LINE_JOIN s_join_style, const double s_join_miter_limit, ID2D1StrokeStyle** s_arrow_style);
	// 矢じるしの先端と返しの位置を求める.
	//static bool line_get_arrow_pos(const D2D1_POINT_2F a_pos, const D2D1_POINT_2F a_vec, const ARROW_SIZE& a_size, D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept;
	// 線分が位置を含むか, 太さも考慮して判定する.
	static bool line_hit_test(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width, const CAP_STYLE& s_cap) noexcept;

	// 矢じるしの D2D1 パスジオメトリを作成する
	// d_factory	D2D ファクトリー
	// s_pos	軸の開始位置
	// d_vec	軸の終了位置への差分
	// style	矢じるしの形式
	// size	矢じるしの寸法
	// geo	作成されたパスジオメトリ
	static void line_create_arrow_geom(ID2D1Factory3* const d_factory, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_vec, ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo)
	{
		D2D1_POINT_2F barbs[2];	// 矢じるしの返しの端点
		D2D1_POINT_2F tip_pos;	// 矢じるしの先端点
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (ShapeLine::line_get_arrow_pos(s_pos, d_vec, a_size, barbs, tip_pos)) {
			// ジオメトリパスを作成する.
			winrt::check_hresult(d_factory->CreatePathGeometry(geo));
			winrt::check_hresult((*geo)->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(
				barbs[0],
				style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
				: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW
			);
			sink->AddLine(tip_pos);
			sink->AddLine(barbs[1]);
			sink->EndFigure(
				style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED
				: D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN
			);
			winrt::check_hresult(sink->Close());
			sink = nullptr;
		}
	}

	// 矢じるしの D2D ストローク特性を作成する.
	static void line_create_arrow_style(ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, const D2D1_LINE_JOIN s_join_style, const double s_join_miter_limit, ID2D1StrokeStyle** s_arrow_style)
	{
		// 矢じるしの破線の形式はかならずソリッドとする.
		const D2D1_STROKE_STYLE_PROPERTIES s_prop{
			s_cap_style.m_start,	// startCap
			s_cap_style.m_end,	// endCap
			D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// dashCap
			s_join_style,	// lineJoin
			static_cast<FLOAT>(s_join_miter_limit),	// miterLimit
			D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID,	// dashStyle
			0.0f	// dashOffset
		};
		winrt::check_hresult(
			d_factory->CreateStrokeStyle(s_prop, nullptr, 0, s_arrow_style)
		);
	}

	// 矢じるしの先端と返しの位置を求める.
	// a_pos	矢軸の後端の位置
	// a_vec	矢軸の先端へのベクトル
	// a_size	矢じるしの寸法
	// barbs	返しの位置
	// tip		先端の位置
	bool ShapeLine::line_get_arrow_pos(const D2D1_POINT_2F a_pos, const D2D1_POINT_2F a_vec, const ARROW_SIZE& a_size, D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept
	{
		const auto a_len = std::sqrt(pt_abs2(a_vec));	// 矢軸の長さ
		if (a_len >= FLT_MIN) {
			get_arrow_barbs(a_vec, a_len, a_size.m_width, a_size.m_length, barbs);
			if (a_size.m_offset >= a_len) {
				// 矢じるしの先端
				tip = a_pos;
			}
			else {
				pt_mul_add(a_vec, 1.0 - a_size.m_offset / a_len, a_pos, tip);
			}
			pt_add(barbs[0], tip, barbs[0]);
			pt_add(barbs[1], tip, barbs[1]);
			return true;
		}
		return false;
	}

	// 線分が位置を含むか, 太さも考慮して判定する.
	// t_pos	判定する位置
	// s_pos	線分の始端
	// e_pos	線分の終端
	// s_width	線分の太さ
	// s_cap	線分の端の形式
	// 戻り値	含む場合 true
	static bool line_hit_test(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width, const CAP_STYLE& s_cap) noexcept
	{
		const double e_width = max(s_width * 0.5, 0.5);
		if (equal(s_cap, CAP_SQUARE)) {
			D2D1_POINT_2F d_vec;	// 差分線分のベクトル
			pt_sub(e_pos, s_pos, d_vec);
			const double abs2 = pt_abs2(d_vec);
			pt_mul(
				abs2 >= FLT_MIN ? d_vec : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 >= FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				d_vec);
			const double dx = d_vec.x;
			const double dy = d_vec.y;
			const double ox = dy;
			const double oy = -dx;
			D2D1_POINT_2F e_side[4];
			pt_add(s_pos, -dx + ox, -dy + oy, e_side[0]);
			pt_add(s_pos, -dx - ox, -dy - oy, e_side[1]);
			pt_add(e_pos, dx - ox, dy - oy, e_side[2]);
			pt_add(e_pos, dx + ox, dy + oy, e_side[3]);
			return pt_in_poly(t_pos, 4, e_side);
		}
		else if (equal(s_cap, CAP_TRIANGLE)) {
			D2D1_POINT_2F d_vec;	// 差分線分のベクトル
			pt_sub(e_pos, s_pos, d_vec);
			const double abs2 = pt_abs2(d_vec);
			pt_mul(
				abs2 >= FLT_MIN ? d_vec : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 >= FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				d_vec);
			const double dx = d_vec.x;
			const double dy = d_vec.y;
			const double ox = dy;
			const double oy = -dx;
			D2D1_POINT_2F e_side[6];
			pt_add(s_pos, ox, oy, e_side[0]);
			pt_add(s_pos, -dx, -dy, e_side[1]);
			pt_add(s_pos, -ox, -oy, e_side[2]);
			pt_add(e_pos, -ox, -oy, e_side[3]);
			pt_add(e_pos, dx, dy, e_side[4]);
			pt_add(e_pos, ox, oy, e_side[5]);
			return pt_in_poly(t_pos, 6, e_side);
		}
		else {
			if (equal(s_cap, CAP_ROUND)) {
				if (pt_in_circle(t_pos, s_pos, e_width) || pt_in_circle(t_pos, e_pos, e_width)) {
					return true;
				}
			}
			D2D1_POINT_2F d_vec;	// 差分ベクトル
			pt_sub(e_pos, s_pos, d_vec);
			const double abs2 = pt_abs2(d_vec);
			if (abs2 >= FLT_MIN) {
				pt_mul(d_vec, e_width / sqrt(abs2), d_vec);
				const double ox = d_vec.y;
				const double oy = -d_vec.x;
				D2D1_POINT_2F e_side[4];
				pt_add(s_pos, ox, oy, e_side[0]);
				pt_add(s_pos, -ox, -oy, e_side[1]);
				pt_add(e_pos, -ox, -oy, e_side[2]);
				pt_add(e_pos, ox, oy, e_side[3]);
				return pt_in_poly(t_pos, 4, e_side);
			}
		}
		return false;
	}

	// 図形を表示する.
	void ShapeLine::draw(void)
	{
		ID2D1Factory3* const factory = Shape::s_d2d_factory;
		ID2D1RenderTarget* const context = Shape::s_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::s_d2d_color_brush;

		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}

		brush->SetColor(m_stroke_color);
		const auto s_style = m_d2d_stroke_style.get();
		const auto s_width = m_stroke_width;

		D2D1_POINT_2F e_pos;
		pt_add(m_start, m_vec[0], e_pos);
		context->DrawLine(m_start, e_pos, brush, s_width, s_style);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			if (m_d2d_arrow_style == nullptr) {
				line_create_arrow_style(factory, m_stroke_cap, m_join_style, m_join_miter_limit, m_d2d_arrow_style.put());
			}
			if (m_d2d_arrow_geom == nullptr) {
				line_create_arrow_geom(factory, m_start, m_vec[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
			}
			const auto a_geom = m_d2d_arrow_geom.get();
			if (m_d2d_arrow_geom != nullptr) {
				if (m_arrow_style == ARROW_STYLE::FILLED) {
					context->FillGeometry(a_geom, brush, nullptr);
				}
				context->DrawGeometry(a_geom, brush, s_width, m_d2d_arrow_style.get());
			}
		}
		if (is_selected()) {
			D2D1_POINT_2F mid;
			pt_mul_add(m_vec[0], 0.5, m_start, mid);
			anc_draw_rect(m_start, context, brush);
			anc_draw_rect(mid, context, brush);
			anc_draw_rect(e_pos, context, brush);
		}
	}

	// 指定された部位の位置を得る.
	// anc	図形の部位
	// val	得られた位置
	void ShapeLine::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		if (anc == ANC_TYPE::ANC_PAGE || anc == ANC_TYPE::ANC_P0) {
			// 図形の部位が「外部」または「開始点」ならば, 開始位置を得る.
			val = m_start;
		}
		else if (anc > ANC_TYPE::ANC_P0) {
			const size_t a_cnt = anc - ANC_TYPE::ANC_P0;
			if (a_cnt < m_vec.size() + 1) {
				val = m_start;
				for (size_t i = 0; i < a_cnt; i++) {
					pt_add(val, m_vec[i], val);
				}
			}
		}
	}

	// 図形を囲む領域の左上位置を得る.
	// val	領域の左上位置
	void ShapeLine::get_pos_lt(D2D1_POINT_2F& val) const noexcept
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
	bool ShapeLine::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		val = m_start;
		return true;
	}

	// 矢じるしの寸法を得る.
	bool ShapeLine::get_arrow_size(ARROW_SIZE& val) const noexcept
	{
		val = m_arrow_size;
		return true;
	}

	// 矢じるしの形式を得る.
	bool ShapeLine::get_arrow_style(ARROW_STYLE& val) const noexcept
	{
		val = m_arrow_style;
		return true;
	}

	// 図形を囲む領域を得る.
// a_lt	元の領域の左上位置.
// a_rb	元の領域の右下位置.
// b_lt	囲む領域の左上位置.
// b_rb	囲む領域の右下位置.
	void ShapeLine::get_bound(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept
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

	// 頂点を得る.
	size_t ShapeLine::get_verts(D2D1_POINT_2F v_pos[]) const noexcept
	{
		v_pos[0] = m_start;
		const size_t d_cnt = m_vec.size();
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(v_pos[i], m_vec[i], v_pos[i + 1]);
		}
		return d_cnt + 1;
	}

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeLine::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		D2D1_POINT_2F e_pos;
		pt_add(m_start, m_vec[0], e_pos);
		if (pt_in_anc(t_pos, e_pos)) {
			return ANC_TYPE::ANC_P0 + 1;
		}
		if (pt_in_anc(t_pos, m_start)) {
			return ANC_TYPE::ANC_P0;
		}
		const float s_width = static_cast<float>(max(static_cast<double>(m_stroke_width), Shape::s_anc_len));
		if (line_hit_test(t_pos, m_start, e_pos, s_width, m_stroke_cap)) {
			return ANC_TYPE::ANC_STROKE;
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// 範囲に含まれるか判定する.
	// area_lt	範囲の左上位置
	// area_rb	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeLine::in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept
	{
		if (pt_in_rect(m_start, area_lt, area_rb)) {
			D2D1_POINT_2F pos;
			pt_add(m_start, m_vec[0], pos);
			return pt_in_rect(pos, area_lt, area_rb);
		}
		return false;
	}

	// 差分だけ移動する.
	/*
	bool ShapeLine::move(const D2D1_POINT_2F d_vec) noexcept
	{
		if (ShapeStroke::move(d_vec)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}
	*/

	// 値を矢じるしの寸法に格納する.
	bool ShapeLine::set_arrow_size(const ARROW_SIZE& val) noexcept
	{
		if (!equal(m_arrow_size, val)) {
			m_arrow_size = val;
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	// 値を矢じるしの形式に格納する.
	bool ShapeLine::set_arrow_style(const ARROW_STYLE val) noexcept
	{
		if (m_arrow_style != val) {
			m_arrow_style = val;
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// 値を線分の結合のマイター制限に格納する.
	bool ShapeLine::set_join_miter_limit(const float& val) noexcept
	{
		if (ShapeStroke::set_join_miter_limit(val)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// 値を線分の結合に格納する.
	bool ShapeLine::set_join_style(const D2D1_LINE_JOIN& val) noexcept
	{
		if (ShapeStroke::set_join_style(val)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// 値を, 部位の位置に格納する.
	// val	値
	// anc	図形の部位
	// limit	限界距離 (他の頂点との距離がこの値未満になるなら, その頂点に位置に合わせる)
	bool ShapeLine::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool /*keep_aspect*/) noexcept
	{
		bool done = false;
		// 変更する頂点がどの頂点か判定する.
		const size_t d_cnt = m_vec.size();	// 差分の数
		if (anc >= ANC_TYPE::ANC_P0 && anc <= ANC_TYPE::ANC_P0 + d_cnt) {
			D2D1_POINT_2F v_pos[N_GON_MAX];	// 頂点の位置
			const size_t a_cnt = anc - ANC_TYPE::ANC_P0;	// 変更する頂点
			// 変更する頂点までの, 各頂点の位置を得る.
			v_pos[0] = m_start;
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
					pt_add(m_start, vec, m_start);
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
						pt_add(m_start, v_vec, m_start);
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
		if (done && m_d2d_arrow_geom != nullptr) {
			m_d2d_arrow_geom = nullptr;
		}
		return done;
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	bool ShapeLine::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F new_pos;
		pt_round(val, PT_ROUND, new_pos);
		if (!equal(m_start, new_pos)) {
			m_start = new_pos;
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	/*
	// 値を, 部位の位置に格納する. 
	// val	値
	// anc	図形の部位
	// limit	限界距離 (他の頂点との距離がこの値未満になるなら, その頂点に位置に合わせる)
	bool ShapeLine::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept
	{
		if (ShapeStroke::set_pos_anc(val, anc, limit, keep_aspect)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}
	*/

	// 値を始点に格納する. 他の部位の位置も動く.
	/*
	bool ShapeLine::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		if (ShapeStroke::set_pos_start(val)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}
	*/

	// 値を端の形式に格納する.
	bool ShapeLine::set_stroke_cap(const CAP_STYLE& val) noexcept
	{
		if (ShapeStroke::set_stroke_cap(val)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// 差分だけ移動する.
	// d_vec	差分ベクトル
	bool ShapeLine::move(const D2D1_POINT_2F d_vec) noexcept
	{
		D2D1_POINT_2F new_pos;
		pt_add(m_start, d_vec, new_pos);
		if (set_pos_start(new_pos)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	// 図形を作成する.
	// b_pos	囲む領域の始点
	// b_vec	囲む領域の終点への差分
	// page	既定の属性値
	ShapeLine::ShapeLine(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapePage* page) :
		ShapeStroke::ShapeStroke(page),
		m_arrow_style(page->m_arrow_style),
		m_arrow_size(page->m_arrow_size),
		m_d2d_arrow_geom(nullptr),
		m_d2d_arrow_style(nullptr)
	{
		m_start = b_pos;
		m_vec.resize(1, b_vec);
		m_vec.shrink_to_fit();
	}

	// 図形をデータリーダーから読み込む.
	// dt_reader	読み込むデータリーダー
	ShapeLine::ShapeLine(const ShapePage& page, DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(page, dt_reader),
		m_d2d_arrow_style(nullptr),
		m_d2d_arrow_geom(nullptr)
	{
		m_start.x = dt_reader.ReadSingle();
		m_start.y = dt_reader.ReadSingle();
		const size_t vec_cnt = dt_reader.ReadUInt32();	// 要素数
		m_vec.resize(vec_cnt);
		for (size_t i = 0; i < vec_cnt; i++) {
			m_vec[i].x = dt_reader.ReadSingle();
			m_vec[i].y = dt_reader.ReadSingle();
		}

		m_arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadInt32());
		m_arrow_size.m_width = dt_reader.ReadSingle();
		m_arrow_size.m_length = dt_reader.ReadSingle();
		m_arrow_size.m_offset = dt_reader.ReadSingle();
	}

	// 図形をデータライターに書き込む.
	void ShapeLine::write(DataWriter const& dt_writer) const
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

		dt_writer.WriteInt32(static_cast<int32_t>(m_arrow_style));

		dt_writer.WriteSingle(m_arrow_size.m_width);
		dt_writer.WriteSingle(m_arrow_size.m_length);
		dt_writer.WriteSingle(m_arrow_size.m_offset);
	}

	// 近傍の頂点を見つける.
// pos	ある位置
// dd	近傍とみなす距離 (の二乗値), これより離れた頂点は近傍とはみなさない.
// val	ある位置の近傍にある頂点
// 戻り値	見つかったら true
	bool ShapeLine::get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool done = false;
		D2D1_POINT_2F vec;
		pt_sub(m_start, pos, vec);
		float vv = static_cast<float>(pt_abs2(vec));
		if (vv < dd) {
			dd = vv;
			val = m_start;
			if (!done) {
				done = true;
			}
		}
		D2D1_POINT_2F v_pos{ m_start };
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

}