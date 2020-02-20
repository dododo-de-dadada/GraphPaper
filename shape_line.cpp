//------------------------------
// Shape_line.cpp
// 直線
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr ANCH_WHICH ANCH_BEGIN = ANCH_R_NW;
	constexpr ANCH_WHICH ANCH_END = ANCH_R_SE;

	static bool ln_calc_arrowhead(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, const ARROW_SIZE& a_size, D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip_pos) noexcept
	{
		const auto len = std::sqrt(pt_abs2(vec));	// 軸の長さ
		if (len > FLT_MIN) {
			// 矢じりの先端と返しの位置を計算する.
			get_arrow_barbs(
				vec, len,
				a_size.m_width, a_size.m_length, barbs);
			if (a_size.m_offset >= len) {
				// 矢じりの先端
				tip_pos = pos;
			}
			else {
				pt_scale(vec, 1.0 - a_size.m_offset / len, pos, tip_pos);
			}
			pt_add(barbs[0], tip_pos, barbs[0]);
			pt_add(barbs[1], tip_pos, barbs[1]);
			return true;
		}
		return false;
	}

	// 矢じりの D2D1 パスジオメトリを作成する
	// fa	D2D ファクトリー
	// pos	軸の始点
	// vec	軸の終点ベクトル
	// style	矢じりの形式
	// size	矢じりの寸法
	// geo	作成されたパスジオメトリ
	static void ln_create_arrow_geometry(ID2D1Factory3* fa, const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo)
	{
		D2D1_POINT_2F barbs[2];	// 矢じりの返しの端点
		D2D1_POINT_2F tip_pos;	// 矢じりの先端点
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (ln_calc_arrowhead(pos, vec, a_size, barbs, tip_pos)) {
			// ジオメトリパスを作成する.
			winrt::check_hresult(
				fa->CreatePathGeometry(geo)
			);
			winrt::check_hresult(
				(*geo)->Open(sink.put())
			);
			sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(
				barbs[0],
				style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_BEGIN_FILLED
				: D2D1_FIGURE_BEGIN_HOLLOW
			);
			sink->AddLine(tip_pos);
			sink->AddLine(barbs[1]);
			sink->EndFigure(
				style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_END_CLOSED
				: D2D1_FIGURE_END_OPEN
			);
			winrt::check_hresult(sink->Close());
			//sink.attach(nullptr);
			sink = nullptr;
		}
	}

	// 図形を破棄する
	ShapeLine::~ShapeLine(void)
	{
		m_d2d_arrow_geometry = nullptr;
	}

	// 図形を表示する.
	void ShapeLine::draw(SHAPE_DX& dx)
	{
		D2D1_POINT_2F e_pos;

		dx.m_shape_brush->SetColor(m_stroke_color);
		pt_add(m_pos, m_vec, e_pos);
		dx.m_d2dContext->DrawLine(
			m_pos,
			e_pos,
			dx.m_shape_brush.get(),
			static_cast<FLOAT>(m_stroke_width),
			m_d2d_stroke_style.get());
		if (m_arrow_style != ARROW_STYLE::NONE) {
			/*
			if (m_d2d_arrow_geometry.get() == nullptr) {
				//ID2D1Factory3 *factory = dev->2DFactory();
				ln_create_arrow_geometry(
					s_d2d_factory, m_pos, m_vec, m_arrow_style,
					m_arrow_size, m_d2d_arrow_geometry.put());
			}
			*/
			if (m_d2d_arrow_geometry.get() != nullptr) {
				dx.m_d2dContext->DrawGeometry(
					m_d2d_arrow_geometry.get(),
					dx.m_shape_brush.get(),
					static_cast<FLOAT>(m_stroke_width),
					nullptr);
				if (m_arrow_style == ARROW_STYLE::FILLED) {
					dx.m_d2dContext->FillGeometry(
						m_d2d_arrow_geometry.get(),
						dx.m_shape_brush.get(), nullptr);
				}
			}
		}
		if (is_selected()) {
			D2D1_POINT_2F mid;
			pt_scale(m_vec, 0.5, m_pos, mid);
			draw_anchor(m_pos, dx);
			draw_anchor(mid, dx);
			draw_anchor(e_pos, dx);
		}
	}

	// 矢じりの寸法を得る.
	bool ShapeLine::get_arrow_size(ARROW_SIZE& val) const noexcept
	{
		val = m_arrow_size;
		return true;
	}

	// 矢じりの形式を得る.
	bool ShapeLine::get_arrow_style(ARROW_STYLE& val) const noexcept
	{
		val = m_arrow_style;
		return true;
	}

	// 指定された部位の位置を得る.
	void ShapeLine::get_pos(const ANCH_WHICH a, D2D1_POINT_2F& pos) const noexcept
	{
		if (a == ANCH_END) {
			pt_add(m_pos, m_vec, pos);
		}
		else {
			pos = m_pos;
		}
	}

	// 位置を含むか調べる.
	// t_pos	調べる位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	ANCH_WHICH ShapeLine::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		D2D1_POINT_2F e_pos;
		pt_add(m_pos, m_vec, e_pos);
		D2D1_POINT_2F line[2]{ m_pos, e_pos };

		if (pt_in_anch(t_pos, e_pos, a_len)) {
			return ANCH_END;
		}
		if (pt_in_anch(t_pos, m_pos, a_len)) {
			return ANCH_BEGIN;
		}
		if (pt_in_line(t_pos, line, max(m_stroke_width, a_len))) {
			return ANCH_FRAME;
		}
		return ANCH_OUTSIDE;
	}

	// 範囲に含まれるか調べる.
	// a_min	範囲の左上位置
	// a_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeLine::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		D2D1_POINT_2F pos;

		if (pt_in_rect(m_pos, a_min, a_max)) {
			pt_add(m_pos, m_vec, pos);
			return pt_in_rect(pos, a_min, a_max);
		}
		return false;
	}

	// 差分だけ移動する.
	void ShapeLine::move(const D2D1_POINT_2F d)
	{
		ShapeStroke::move(d);
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_vec, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// データライターから読み込む.
	void ShapeLine::read(DataReader const& dt_reader)
	{
		m_arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadInt32());
		m_arrow_size.m_width = dt_reader.ReadSingle();
		m_arrow_size.m_length = dt_reader.ReadSingle();
		m_arrow_size.m_offset = dt_reader.ReadSingle();
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_vec, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// 値を矢じりの寸法に格納する.
	void ShapeLine::set_arrow_size(const ARROW_SIZE& val)
	{
		if (equal(m_arrow_size, val)) {
			return;
		}
		m_arrow_size = val;
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_vec, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// 値を矢じりの形式に格納する.
	void ShapeLine::set_arrow_style(const ARROW_STYLE val)
	{
		if (m_arrow_style != val) {
			m_arrow_style = val;
			m_d2d_arrow_geometry = nullptr;
			if (val != ARROW_STYLE::NONE) {
				ln_create_arrow_geometry(
					s_d2d_factory, m_pos, m_vec, m_arrow_style,
					m_arrow_size, m_d2d_arrow_geometry.put());
			}
		}
	}

	// 値を指定した部位の位置に格納する. 他の部位の位置は動かない. 
	void ShapeLine::set_pos(const D2D1_POINT_2F pos, const ANCH_WHICH a)
	{
		D2D1_POINT_2F d;

		if (a == ANCH_END) {
			pt_sub(pos, m_pos, m_vec);
		}
		else if (a == ANCH_BEGIN) {
			pt_sub(pos, m_pos, d);
			pt_sub(m_vec, d, m_vec);
			m_pos = pos;
		}
		else {
			m_pos = pos;
		}
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_vec, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// 値を始点に格納する. 他の部位の位置も動く.
	void ShapeLine::set_start_pos(const D2D1_POINT_2F pos)
	{
		ShapeStroke::set_start_pos(pos);
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_vec, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// 図形を作成する.
	// pos	開始位置
	// vec	終了ベクトル
	// attr	既定の属性値
	ShapeLine::ShapeLine(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, const ShapePanel* attr) :
		ShapeStroke::ShapeStroke(attr),
		m_arrow_style(attr->m_arrow_style),
		m_arrow_size(attr->m_arrow_size)
	{
		m_pos = pos;
		m_vec = vec;
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_vec, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// 図形をデータリーダーから読み込む.
	// dt_reader	読み込むデータリーダー
	ShapeLine::ShapeLine(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader)
	{
		m_arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadInt32());
		m_arrow_size.m_width = dt_reader.ReadSingle();
		m_arrow_size.m_length = dt_reader.ReadSingle();
		m_arrow_size.m_offset = dt_reader.ReadSingle();
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_vec, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// データライターに書き込む.
	void ShapeLine::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);
		dt_writer.WriteInt32(static_cast<int32_t>(m_arrow_style));
		dt_writer.WriteSingle(m_arrow_size.m_width);
		dt_writer.WriteSingle(m_arrow_size.m_length);
		dt_writer.WriteSingle(m_arrow_size.m_offset);
	}

	// データライターに SVG タグとして書き込む.
	void ShapeLine::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;
		D2D1_POINT_2F pos;

		pt_add(m_pos, m_vec, pos);
		write_svg("<line ", dt_writer);
		write_svg(m_pos, "x1", "y1", dt_writer);
		write_svg(pos, "x2", "y2", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>" SVG_NL, dt_writer);
		if (m_arrow_style == ARROW_STYLE::NONE) {
			return;
		}
		D2D1_POINT_2F barbs[2];
		D2D1_POINT_2F tip_pos;
		if (ln_calc_arrowhead(m_pos, m_vec, m_arrow_size, barbs, tip_pos)) {
			ShapeStroke::write_svg(barbs, tip_pos, m_arrow_style, dt_writer);
		}
	}
}