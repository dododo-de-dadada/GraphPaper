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
	static void line_create_arrow_geom(ID2D1Factory3* const d_factory, const D2D1_POINT_2F start, const D2D1_POINT_2F end_to, ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo) noexcept;

	// 矢じるしの D2D1 パスジオメトリを作成する
	//ID2D1Factory3* const factory,	// D2D ファクトリー
	// start,	// 矢軸の始点
	// end_to,	// 矢軸の終点への位置ベクトル
	// style,	// 矢じるしの形式
	// a_size,	// 矢じるしの大きさ
	// geo	// 作成されたパスジオメトリ
	static void line_create_arrow_geom(
		ID2D1Factory3* const factory,	// D2D ファクトリー
		const D2D1_POINT_2F start,	// 矢軸の始点
		const D2D1_POINT_2F end_to,	// 矢軸の先端への位置ベクトル
		ARROW_STYLE style,	// 矢じるしの形式
		ARROW_SIZE& a_size,	// 矢じるしの大きさ
		ID2D1PathGeometry** geo	// 作成されたパスジオメトリ
	) noexcept
	{
		D2D1_POINT_2F barb[2];	// 矢じるしの返しの端点
		D2D1_POINT_2F tip;	// 矢じるしの先端点
		winrt::com_ptr<ID2D1GeometrySink> sink;
		HRESULT hr = S_OK;
		if (!ShapeLine::line_get_pos_arrow(start, end_to, a_size, barb, tip)) {
			hr = E_FAIL;
		}
		if (hr == S_OK) {
			// ジオメトリパスを作成する.
			hr = factory->CreatePathGeometry(geo);
		}
		if (hr == S_OK) {
			hr = (*geo)->Open(sink.put());
		}
		if (hr == S_OK) {
			const auto f_begin = (
				style == ARROW_STYLE::ARROW_FILLED
				? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
				: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW
				);
			const auto f_end = (
				style == ARROW_STYLE::ARROW_FILLED
				? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED
				: D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN
				);
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(barb[0], f_begin);
			sink->AddLine(tip);
			sink->AddLine(barb[1]);
			sink->EndFigure(f_end);
			hr = sink->Close();
		}
		sink = nullptr;
	}

	// 矢じるしの先端と返しの位置を求める.
	bool ShapeLine::line_get_pos_arrow(
		const D2D1_POINT_2F a_end,	// 矢軸の後端の位置
		const D2D1_POINT_2F a_dir,	// 矢軸の先端へのベクトル
		const ARROW_SIZE& a_size,	// 矢じるしの寸法
		D2D1_POINT_2F barb[2],	// 返しの位置
		D2D1_POINT_2F& tip	// 先端の位置
	) noexcept
	{
		const auto a_len = std::sqrt(pt_abs2(a_dir));	// 矢軸の長さ
		if (a_len >= FLT_MIN) {
			get_pos_barbs(a_dir, a_len, a_size.m_width, a_size.m_length, barb);
			if (a_size.m_offset >= a_len) {
				// 矢じるしの先端
				tip = a_end;
			}
			else {
				pt_mul_add(a_dir, 1.0 - a_size.m_offset / a_len, a_end, tip);
			}
			pt_add(barb[0], tip, barb[0]);
			pt_add(barb[1], tip, barb[1]);
			return true;
		}
		return false;
	}

	// 図形を表示する.
	void ShapeLine::draw(void) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
		ID2D1Factory* factory;
		target->GetFactory(&factory);

		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}

		brush->SetColor(m_stroke_color);
		const auto s_style = m_d2d_stroke_style.get();
		const auto s_width = m_stroke_width;

		D2D1_POINT_2F end_pt;	// 終点
		pt_add(m_start, m_lineto, end_pt);
		target->DrawLine(m_start, end_pt, brush, s_width, s_style);
		if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			if (m_d2d_arrow_stroke == nullptr) {
				create_arrow_stroke();
			}
			if (m_d2d_arrow_geom == nullptr) {
				line_create_arrow_geom(static_cast<ID2D1Factory3*>(factory), m_start, m_lineto, m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
			}
			const auto a_geom = m_d2d_arrow_geom.get();
			if (m_d2d_arrow_geom != nullptr) {
				if (m_arrow_style == ARROW_STYLE::ARROW_FILLED) {
					target->FillGeometry(a_geom, brush);
				}
				target->DrawGeometry(a_geom, brush, s_width, m_d2d_arrow_stroke.get());
			}
		}
		if (m_loc_show && is_selected()) {
			// 補助線を描く
			if (m_stroke_width >= Shape::m_loc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawLine(m_start, end_pt, brush, 2.0f * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawLine(m_start, end_pt, brush, m_aux_width, m_aux_style.get());
			}
			// 図形の部位を描く.
			D2D1_POINT_2F mid_pt;	// 中点
			pt_mul_add(m_lineto, 0.5, m_start, mid_pt);
			loc_draw_rhombus(mid_pt, target, brush);
			loc_draw_square(m_start, target, brush);
			loc_draw_square(end_pt, target, brush);
		}
	}

	// 指定した部位の点を得る.
	void ShapeLine::get_pos_loc(
		const uint32_t loc,	// 部位
		D2D1_POINT_2F& val	// 得られた点
	) const noexcept
	{
		// 図形の部位が「図形の外部」または「開始点」ならば, 始点を得る.
		if (loc == LOCUS_TYPE::LOCUS_START) {
			val = m_start;
		}
		else if (loc == LOCUS_TYPE::LOCUS_END) {
			val.x = m_start.x + m_lineto.x;
			val.y = m_start.y + m_lineto.y;
		}
	}

	// 境界矩形の左上点を得る.
	// val	左上点
	void ShapeLine::get_bbox_lt(D2D1_POINT_2F& val) const noexcept
	{
		val.x = m_lineto.x < 0.0 ? m_start.x + m_lineto.x : m_start.x;
		val.y = m_lineto.y < 0.0 ? m_start.y + m_lineto.y : m_start.y;
	}

	// 始点を得る
	// val	始点
	// 戻り値	つねに true
	bool ShapeLine::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		val = m_start;
		return true;
	}

	// 境界矩形を得る.
	// a_lt	元の矩形の左上点.
	// a_rb	元の矩形の右下点.
	// b_lt	矩形の左上点.
	// b_rb	矩形の右下点.
	void ShapeLine::get_bbox(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept
	{
		b_lt.x = m_start.x < a_lt.x ? m_start.x : a_lt.x;
		b_lt.y = m_start.y < a_lt.y ? m_start.y : a_lt.y;
		b_rb.x = m_start.x > a_rb.x ? m_start.x : a_rb.x;
		b_rb.y = m_start.y > a_rb.y ? m_start.y : a_rb.y;
		if (m_start.x + m_lineto.x < b_lt.x) {
			b_lt.x = m_start.x + m_lineto.x;
		}
		if (m_start.x + m_lineto.x > b_rb.x) {
			b_rb.x = m_start.x + m_lineto.x;
		}
		if (m_start.y + m_lineto.y < b_lt.y) {
			b_lt.y = m_start.y + m_lineto.y;
		}
		if (m_start.y + m_lineto.y > b_rb.y) {
			b_rb.y = m_start.y + m_lineto.y;
		}
	}

	// 頂点を得る.
	size_t ShapeLine::get_verts(D2D1_POINT_2F p[]) const noexcept
	{
		p[0] = m_start;
		p[1].x = m_start.x + m_lineto.x;
		p[1].y = m_start.y + m_lineto.y;
		p[2].x = static_cast<FLOAT>(m_start.x + 0.5 * m_lineto.x);
		p[2].y = static_cast<FLOAT>(m_start.y + 0.5 * m_lineto.y);
		return 3;
	}

	// 図形が点を含むか判定する.
	// test_pt	判定される点
	// 戻り値	点を含む部位
	uint32_t ShapeLine::hit_test(const D2D1_POINT_2F test_pt, const bool/*ctrl_key*/) const noexcept
	{
		const D2D1_POINT_2F end_pt{	// 終点
			m_start.x + m_lineto.x,
			m_start.y + m_lineto.y
		};
		if (loc_hit_test(test_pt, end_pt, m_loc_width)) {
			return LOCUS_TYPE::LOCUS_END;
		}
		if (loc_hit_test(test_pt, m_start, m_loc_width)) {
			return LOCUS_TYPE::LOCUS_START;
		}
		const double e_width = 0.5 * max(m_stroke_width, m_loc_width);
		if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE)) {
			D2D1_POINT_2F to{ m_lineto };
			const double abs2 = pt_abs2(to);
			pt_mul(
				abs2 >= FLT_MIN ? to : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 >= FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				to);
			const double dx = to.x;	// 辺の方向ベクトル X 軸
			const double dy = to.y;	// 辺の方向ベクトル Y 軸
			const double ox = dy;	// 辺の直交ベクトル X 軸
			const double oy = -dx;	// 辺の直交ベクトル Y 軸
			D2D1_POINT_2F q[4]{};	// 太らせた辺の四辺形
			pt_add(m_start, -dx + ox, -dy + oy, q[0]);
			pt_add(m_start, -dx - ox, -dy - oy, q[1]);
			pt_add(end_pt, dx - ox, dy - oy, q[2]);
			pt_add(end_pt, dx + ox, dy + oy, q[3]);
			if (pt_in_poly(test_pt, 4, q)) {
				return LOCUS_TYPE::LOCUS_STROKE;
			}
		}
		else if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE)) {
			D2D1_POINT_2F p{ m_lineto };
			const double abs2 = pt_abs2(p);
			pt_mul(
				abs2 >= FLT_MIN ? p : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 >= FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				p);
			const double dx = p.x;	// 辺の方向ベクトル X 座標
			const double dy = p.y;	// 辺の方向ベクトル Y 座標
			const double ox = dy;	// 辺の直交ベクトル X 座標
			const double oy = -dx;	// 辺の直交ベクトル Y 座標
			D2D1_POINT_2F h[6]{};	// 太らせた辺の六角形
			pt_add(m_start, ox, oy, h[0]);
			pt_add(m_start, -dx, -dy, h[1]);
			pt_add(m_start, -ox, -oy, h[2]);
			pt_add(end_pt, -ox, -oy, h[3]);
			pt_add(end_pt, dx, dy, h[4]);
			pt_add(end_pt, ox, oy, h[5]);
			if (pt_in_poly(test_pt, 6, h)) {
				return LOCUS_TYPE::LOCUS_STROKE;
			}
		}
		else {
			if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND)) {
				if (pt_in_circle(test_pt, m_start, e_width) || pt_in_circle(test_pt, end_pt, e_width)) {
					return LOCUS_TYPE::LOCUS_STROKE;
				}
			}
			D2D1_POINT_2F p{ m_lineto };
			const double abs2 = pt_abs2(p);
			if (abs2 >= FLT_MIN) {
				pt_mul(p, e_width / sqrt(abs2), p);
				const double ox = p.y;	// 辺の直交ベクトル X 座標
				const double oy = -p.x;	// 辺の直交ベクトル Y 座標
				D2D1_POINT_2F q[4];	// 太らせた辺の四辺形
				pt_add(m_start, ox, oy, q[0]);
				pt_add(m_start, -ox, -oy, q[1]);
				pt_add(end_pt, -ox, -oy, q[2]);
				pt_add(end_pt, ox, oy, q[3]);
				if (pt_in_poly(test_pt, 4, q)) {
					return LOCUS_TYPE::LOCUS_STROKE;
				}
			}
		}
		return LOCUS_TYPE::LOCUS_SHEET;
	}

	// 矩形に含まれるか判定する.
	// lt	矩形の左上位置
	// rb	矩形の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeLine::is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		if (pt_in_rect(m_start, lt, rb)) {
			D2D1_POINT_2F end_pt;
			pt_add(m_start, m_lineto , end_pt);
			return pt_in_rect(end_pt, lt, rb);
		}
		return false;
	}

	// 値を線分の結合の尖り制限に格納する.
	bool ShapeLine::set_stroke_join_limit(const float& val) noexcept
	{
		if (ShapeStroke::set_stroke_join_limit(val)) {
			m_d2d_arrow_stroke = nullptr;
			return true;
		}
		return false;
	}

	// 値を線分の結合に格納する.
	bool ShapeLine::set_stroke_join(const D2D1_LINE_JOIN& val) noexcept
	{
		if (ShapeStroke::set_stroke_join(val)) {
			m_d2d_arrow_stroke = nullptr;
			return true;
		}
		return false;
	}

	// 値を, 指定した部位の点に格納する.
	// val	値
	// loc	部位
	// snap_point	点を点にくっつけるしきい値
	// /*keep_aspect*/
	bool ShapeLine::set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool /*keep_aspect*/) noexcept
	{
		bool flag = false;
		if (loc == LOCUS_TYPE::LOCUS_START) {
			if (!equal(m_start, val)) {
				const D2D1_POINT_2F end{
					m_start.x + m_lineto.x, m_start.y + m_lineto.y
				};
				m_lineto.x = end.x - val.x;
				m_lineto.y = end.y - val.y;
				m_start = val;
				flag = true;
			}
		}
		else if (loc == LOCUS_TYPE::LOCUS_END) {
			const D2D1_POINT_2F end_pt{
				m_start.x + m_lineto.x, m_start.y + m_lineto.y
			};
			if (!equal(end_pt, val)) {
				m_lineto.x = val.x - m_start.x;
				m_lineto.y = val.y - m_start.y;
				flag = true;
			}
		}
		if (flag) {
			const double ss = static_cast<double>(snap_point) * static_cast<double>(snap_point);
			if (ss > FLT_MIN && pt_abs2(m_lineto) <= ss) {
				if (loc == LOCUS_TYPE::LOCUS_START) {
					m_start.x = m_start.x + m_lineto.x;
					m_start.y = m_start.y + m_lineto.y;
				}
				m_lineto.x = 0.0f;
				m_lineto.y = 0.0f;
			}
			m_d2d_arrow_geom = nullptr;
		}
		return flag;
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	bool ShapeLine::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F p;
		pt_round(val, PT_ROUND, p);
		if (!equal(m_start, p)) {
			m_start = p;
			m_d2d_arrow_geom = nullptr;
			return true;
		}
		return false;
	}

	// 値を端の形式に格納する.
	bool ShapeLine::set_stroke_cap(const D2D1_CAP_STYLE& val) noexcept
	//bool ShapeLine::set_stroke_cap(const CAP_STYLE& val) noexcept
	{
		if (ShapeStroke::set_stroke_cap(val)) {
			m_d2d_arrow_stroke = nullptr;
			return true;
		}
		return false;
	}

	// 位置を移動する.
	// to	移動先へのベクトル
	bool ShapeLine::move(const D2D1_POINT_2F to) noexcept
	{
		const D2D1_POINT_2F pt{
			m_start.x + to.x, m_start.y + to.y
		};
		if (set_pos_start(pt)) {
			m_d2d_arrow_geom = nullptr;
			return true;
		}
		return false;
	}

	// 図形を作成する.
	// start	始点
	// lineto	終点への位置ベクトル
	// prop	属性
	ShapeLine::ShapeLine(const D2D1_POINT_2F start, const D2D1_POINT_2F lineto, const Shape* prop) :
		ShapeArrow::ShapeArrow(prop)
	{
		m_start = start;
		m_lineto = lineto;
		prop->get_arrow_style(m_arrow_style);
		prop->get_arrow_size(m_arrow_size);
		m_d2d_arrow_geom = nullptr;
		m_d2d_arrow_stroke = nullptr;
	}

	// 図形をデータリーダーから読み込む.
	// dt_reader	読み込むデータリーダー
	ShapeLine::ShapeLine(DataReader const& dt_reader) :
		ShapeArrow::ShapeArrow(dt_reader)
	{
		m_start.x = dt_reader.ReadSingle();
		m_start.y = dt_reader.ReadSingle();
		m_lineto.x = dt_reader.ReadSingle();
		m_lineto.y = dt_reader.ReadSingle();
	}

	// 図形をデータライターに書き込む.
	void ShapeLine::write(DataWriter const& dt_writer) const
	{
		ShapeArrow::write(dt_writer);

		// 始点
		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);

		// 終点点への位置ベクトル
		dt_writer.WriteSingle(m_lineto.x);
		dt_writer.WriteSingle(m_lineto.y);
	}

	// 指定された点の近傍の頂点を見つける.
	// 戻り値	見つかったら true
	bool ShapeLine::get_pos_nearest(
		const D2D1_POINT_2F pt,	// 指定された点
		double& dd,	// 近傍とみなす距離 (の二乗値), これより離れた頂点は近傍とはみなさない.
		D2D1_POINT_2F& val	// 見つかった点
	) const noexcept
	{
		bool done = false;
		D2D1_POINT_2F to;
		pt_sub(m_start, pt, to);
		double d_abs = pt_abs2(to);
		if (d_abs < dd) {
			dd = d_abs;
			val = m_start;
			done = true;
		}
		D2D1_POINT_2F end_pt{ m_start.x + m_lineto.x, m_start.y + m_lineto.y };
		pt_sub(end_pt, pt, to);
		d_abs = pt_abs2(to);
		if (d_abs < dd) {
			dd = d_abs;
			val = end_pt;
			done = true;
		}
		D2D1_POINT_2F mid_pt{ 
			static_cast<FLOAT>(m_start.x + 0.5 * m_lineto.x),
			static_cast<FLOAT>(m_start.y + 0.5 * m_lineto.y)
		};
		pt_sub(mid_pt, pt, to);
		d_abs = pt_abs2(to);
		if (d_abs < dd) {
			dd = d_abs;
			val = mid_pt;
			done = true;
		}
		return done;
	}

}