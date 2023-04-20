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
	static void line_create_arrow_geom(ID2D1Factory3* const d_factory, const D2D1_POINT_2F start, const D2D1_POINT_2F pos, ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo) noexcept;
	// 矢じるしの D2D ストローク特性を作成する.
	//static void line_create_arrow_stroke(
	//	ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, 
	//	const D2D1_LINE_JOIN s_join_style, const double s_join_miter_limit,
	//	ID2D1StrokeStyle** s_arrow_style);

	// 矢じるしの D2D1 パスジオメトリを作成する
	static void line_create_arrow_geom(
		ID2D1Factory3* const factory,	// D2D ファクトリー
		const D2D1_POINT_2F start,	// 矢軸の始点
		const D2D1_POINT_2F pos,	// 矢軸の先端への位置ベクトル
		ARROW_STYLE style,	// 矢じるしの形式
		ARROW_SIZE& a_size,	// 矢じるしの大きさ
		ID2D1PathGeometry** geo	// 作成されたパスジオメトリ
	) noexcept
	{
		D2D1_POINT_2F barb[2];	// 矢じるしの返しの端点
		D2D1_POINT_2F tip;	// 矢じるしの先端点
		winrt::com_ptr<ID2D1GeometrySink> sink;
		HRESULT hr = S_OK;
		if (!ShapeLine::line_get_pos_arrow(start, pos, a_size, barb, tip)) {
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
	// a_end	矢軸の後端の位置
	// a_dir	矢軸の先端へのベクトル
	// a_size	矢じるしの寸法
	// barb	返しの位置
	// tip		先端の位置
	bool ShapeLine::line_get_pos_arrow(
		const D2D1_POINT_2F a_end, const D2D1_POINT_2F a_dir, const ARROW_SIZE& a_size,
		/*--->*/D2D1_POINT_2F barb[2], D2D1_POINT_2F& tip) noexcept
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

		D2D1_POINT_2F end;	// 終点
		pt_add(m_start, m_pos, end);
		target->DrawLine(m_start, end, brush, s_width, s_style);
		if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			if (m_d2d_arrow_stroke == nullptr) {
				create_arrow_stroke();
			}
			if (m_d2d_arrow_geom == nullptr) {
				line_create_arrow_geom(static_cast<ID2D1Factory3*>(factory), m_start, m_pos, m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
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
				target->DrawLine(m_start, end, brush, 2.0f * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawLine(m_start, end, brush, m_aux_width, m_aux_style.get());
			}
			// 図形の部位を描く.
			D2D1_POINT_2F mid;	// 中点
			pt_mul_add(m_pos, 0.5, m_start, mid);
			loc_draw_rhombus(mid, target, brush);
			loc_draw_square(m_start, target, brush);
			loc_draw_square(end, target, brush);
		}
	}

	// 指定した部位の点を得る.
	void ShapeLine::get_pos_loc(
		const uint32_t loc,	// 部位
		D2D1_POINT_2F& val	// 得られた点
	) const noexcept
	{
		// 図形の部位が「図形の外部」または「開始点」ならば, 始点を得る.
		if (loc == LOC_TYPE::LOC_START) {
			val = m_start;
		}
		else if (loc == LOC_TYPE::LOC_END) {
			val.x = m_start.x + m_pos.x;
			val.y = m_start.y + m_pos.y;
		}
	}

	// 境界矩形の左上点を得る.
	// val	左上点
	void ShapeLine::get_bbox_lt(D2D1_POINT_2F& val) const noexcept
	{
		val.x = m_pos.x < 0.0 ? m_start.x + m_pos.x : m_start.x;
		val.y = m_pos.y < 0.0 ? m_start.y + m_pos.y : m_start.y;
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
		if (m_start.x + m_pos.x < b_lt.x) {
			b_lt.x = m_start.x + m_pos.x;
		}
		if (m_start.x + m_pos.x > b_rb.x) {
			b_rb.x = m_start.x + m_pos.x;
		}
		if (m_start.y + m_pos.y < b_lt.y) {
			b_lt.y = m_start.y + m_pos.y;
		}
		if (m_start.y + m_pos.y > b_rb.y) {
			b_rb.y = m_start.y + m_pos.y;
		}
	}

	// 頂点を得る.
	size_t ShapeLine::get_verts(D2D1_POINT_2F p[]) const noexcept
	{
		p[0] = m_start;
		p[1].x = m_start.x + m_pos.x;
		p[1].y = m_start.y + m_pos.y;
		p[2].x = static_cast<FLOAT>(m_start.x + 0.5 * m_pos.x);
		p[2].y = static_cast<FLOAT>(m_start.y + 0.5 * m_pos.y);
		return 3;
	}

	// 図形が点を含むか判定する.
	// 戻り値	点を含む部位
	uint32_t ShapeLine::hit_test(
		const D2D1_POINT_2F t	// 判定される点
	) const noexcept
	{
		D2D1_POINT_2F end;	// 終点
		pt_add(m_start, m_pos, end);
		if (loc_hit_test(t, end, m_loc_width)) {
			return LOC_TYPE::LOC_END;
		}
		if (loc_hit_test(t, m_start, m_loc_width)) {
			return LOC_TYPE::LOC_START;
		}
		const double e_width = 0.5 * max(m_stroke_width, m_loc_width);
		if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE)) {
			D2D1_POINT_2F pos{ m_pos };
			const double abs2 = pt_abs2(pos);
			pt_mul(
				abs2 >= FLT_MIN ? pos : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 >= FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				pos);
			const double dx = pos.x;	// 辺の方向ベクトル X 軸
			const double dy = pos.y;	// 辺の方向ベクトル Y 軸
			const double ox = dy;	// 辺の直交ベクトル X 軸
			const double oy = -dx;	// 辺の直交ベクトル Y 軸
			D2D1_POINT_2F q[4]{};	// 太らせた辺の四辺形
			pt_add(m_start, -dx + ox, -dy + oy, q[0]);
			pt_add(m_start, -dx - ox, -dy - oy, q[1]);
			pt_add(end, dx - ox, dy - oy, q[2]);
			pt_add(end, dx + ox, dy + oy, q[3]);
			if (pt_in_poly(t, 4, q)) {
				return LOC_TYPE::LOC_STROKE;
			}
		}
		else if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE)) {
			D2D1_POINT_2F p{ m_pos };
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
			pt_add(end, -ox, -oy, h[3]);
			pt_add(end, dx, dy, h[4]);
			pt_add(end, ox, oy, h[5]);
			if (pt_in_poly(t, 6, h)) {
				return LOC_TYPE::LOC_STROKE;
			}
		}
		else {
			if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND)) {
				if (pt_in_circle(t, m_start, e_width) || pt_in_circle(t, end, e_width)) {
					return LOC_TYPE::LOC_STROKE;
				}
			}
			D2D1_POINT_2F p{ m_pos };
			const double abs2 = pt_abs2(p);
			if (abs2 >= FLT_MIN) {
				pt_mul(p, e_width / sqrt(abs2), p);
				const double ox = p.y;	// 辺の直交ベクトル X 座標
				const double oy = -p.x;	// 辺の直交ベクトル Y 座標
				D2D1_POINT_2F q[4];	// 太らせた辺の四辺形
				pt_add(m_start, ox, oy, q[0]);
				pt_add(m_start, -ox, -oy, q[1]);
				pt_add(end, -ox, -oy, q[2]);
				pt_add(end, ox, oy, q[3]);
				if (pt_in_poly(t, 4, q)) {
					return LOC_TYPE::LOC_STROKE;
				}
			}
		}
		return LOC_TYPE::LOC_PAGE;
	}

	// 矩形に含まれるか判定する.
	// lt	矩形の左上位置
	// rb	矩形の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeLine::is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		if (pt_in_rect(m_start, lt, rb)) {
			D2D1_POINT_2F p;
			pt_add(m_start, m_pos, p);
			return pt_in_rect(p, lt, rb);
		}
		return false;
	}

	// 値を線分の結合の尖り制限に格納する.
	bool ShapeLine::set_join_miter_limit(const float& val) noexcept
	{
		if (ShapeStroke::set_join_miter_limit(val)) {
			m_d2d_arrow_stroke = nullptr;
			return true;
		}
		return false;
	}

	// 値を線分の結合に格納する.
	bool ShapeLine::set_join_style(const D2D1_LINE_JOIN& val) noexcept
	{
		if (ShapeStroke::set_join_style(val)) {
			m_d2d_arrow_stroke = nullptr;
			return true;
		}
		return false;
	}

	// 値を, 指定した部位の点に格納する.
	bool ShapeLine::set_pos_loc(
		const D2D1_POINT_2F val,	// 値
		const uint32_t loc,	// 部位
		const float snap_point,	// 点を点にくっつけるしきい値
		const bool /*keep_aspect*/
	) noexcept
	{
		bool flag = false;
		if (loc == LOC_TYPE::LOC_START) {
			if (!equal(m_start, val)) {
				const D2D1_POINT_2F end{
					m_start.x + m_pos.x, m_start.y + m_pos.y
				};
				m_pos.x = end.x - val.x;
				m_pos.y = end.y - val.y;
				m_start = val;
				flag = true;
			}
		}
		else if (loc == LOC_TYPE::LOC_END) {
			const D2D1_POINT_2F end{
				m_start.x + m_pos.x, m_start.y + m_pos.y
			};
			if (!equal(end, val)) {
				m_pos.x = val.x - m_start.x;
				m_pos.y = val.y - m_start.y;
				flag = true;
			}
		}
		if (flag) {
			const double ss = static_cast<double>(snap_point) * static_cast<double>(snap_point);
			if (ss > FLT_MIN && pt_abs2(m_pos) <= ss) {
				if (loc == LOC_TYPE::LOC_START) {
					m_start.x = m_start.x + m_pos.x;
					m_start.y = m_start.y + m_pos.y;
				}
				m_pos.x = 0.0f;
				m_pos.y = 0.0f;
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
	// pos	位置ベクトル
	bool ShapeLine::move(const D2D1_POINT_2F pos) noexcept
	{
		D2D1_POINT_2F start;
		pt_add(m_start, pos, start);
		if (set_pos_start(start)) {
			m_d2d_arrow_geom = nullptr;
			return true;
		}
		return false;
	}

	// 図形を作成する.
	ShapeLine::ShapeLine(
		const D2D1_POINT_2F start,	// 始点
		const D2D1_POINT_2F pos,	// 終点への位置ベクトル
		const Shape* prop	// 属性
	) :
		ShapeArrow::ShapeArrow(prop)
	{
		m_start = start;
		m_pos = pos;
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
		m_pos.x = dt_reader.ReadSingle();
		m_pos.y = dt_reader.ReadSingle();
	}

	// 図形をデータライターに書き込む.
	void ShapeLine::write(DataWriter const& dt_writer) const
	{
		ShapeArrow::write(dt_writer);

		// 始点
		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);

		// 終点点への位置ベクトル
		dt_writer.WriteSingle(m_pos.x);
		dt_writer.WriteSingle(m_pos.y);
	}

	// 近傍の頂点を見つける.
	// pos	ある位置
	// dd	近傍とみなす距離 (の二乗値), これより離れた頂点は近傍とはみなさない.
	// val	ある位置の近傍にある頂点
	// 戻り値	見つかったら true
	bool ShapeLine::get_pos_nearest(const D2D1_POINT_2F p, double& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool done = false;
		D2D1_POINT_2F d;
		pt_sub(m_start, p, d);
		double d_abs = pt_abs2(d);
		if (d_abs < dd) {
			dd = d_abs;
			val = m_start;
			done = true;
		}
		D2D1_POINT_2F q{ m_start.x + m_pos.x, m_start.y + m_pos.y };
		pt_sub(q, p, d);
		d_abs = pt_abs2(d);
		if (d_abs < dd) {
			dd = d_abs;
			val = q;
			done = true;
		}
		D2D1_POINT_2F r{ 
			static_cast<FLOAT>(m_start.x + 0.5 * m_pos.x),
			static_cast<FLOAT>(m_start.y + 0.5 * m_pos.y)
		};
		pt_sub(r, p, d);
		d_abs = pt_abs2(d);
		if (d_abs < dd) {
			dd = d_abs;
			val = r;
			done = true;
		}
		return done;
	}

}