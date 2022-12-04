//------------------------------
// shape_dt.cpp
// 読み込み, 書き込み.
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// データライターに SVG として属性名とシングルバイト文字列を書き込む.
	// val	シングルバイト文字列
	// name	属性名
	void dt_write_svg(const char* val, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		std::snprintf(buf, sizeof(buf), "%s=\"%s\" ", name, val);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG としてシングルバイト文字列を書き込む.
	void dt_write_svg(const char* val, DataWriter const& dt_writer)
	{
		for (uint32_t i = 0; val[i] != '\0'; i++) {
			dt_writer.WriteByte(val[i]);
		}
	}

	// データライターに SVG として属性名と色を書き込む.
	void dt_write_svg(const D2D1_COLOR_F val, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		const uint32_t vr = static_cast<uint32_t>(std::round(val.r * 255.0)) & 0xff;
		const uint32_t vb = static_cast<uint32_t>(std::round(val.b * 255.0)) & 0xff;
		const uint32_t vg = static_cast<uint32_t>(std::round(val.g * 255.0)) & 0xff;
		sprintf_s(buf, "%s=\"#%02x%02x%02x\" ", name, vr, vg, vb);
		dt_write_svg(buf, dt_writer);
		if (!is_opaque(val)) {
			std::snprintf(buf, sizeof(buf), "%s-opacity=\"%.3f\" ", name, val.a);
			dt_write_svg(buf, dt_writer);
		}
	}

	// データライターに SVG として色を書き込む.
	void dt_write_svg(const D2D1_COLOR_F val, DataWriter const& dt_writer)
	{
		char buf[8];
		const uint32_t vr = static_cast<uint32_t>(std::round(val.r * 255.0)) & 0xFF;
		const uint32_t vb = static_cast<uint32_t>(std::round(val.b * 255.0)) & 0xFF;
		const uint32_t vg = static_cast<uint32_t>(std::round(val.g * 255.0)) & 0xFF;
		sprintf_s(buf, "#%02x%02x%02x", vr, vg, vb);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG として破線の形式と配置を書き込む.
	// d_style	線枠の形式
	// d_patt	破線の配置
	// s_width	線枠の太さ
	void dt_write_svg(const D2D1_DASH_STYLE d_style, const DASH_PATT& d_patt, const double s_width, DataWriter const& dt_writer)
	{
		if (s_width < FLT_MIN) {
			return;
		}
		const double a[]{
			d_patt.m_[0],
			d_patt.m_[1],
			d_patt.m_[2],
			d_patt.m_[3]
		};
		char buf[256];
		if (d_style == D2D1_DASH_STYLE_DASH) {
			sprintf_s(buf, "stroke-dasharray=\"%.0f %.0f\" ", a[0], a[1]);
		}
		else if (d_style == D2D1_DASH_STYLE_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f\" ", a[2], a[3]);
		}
		else if (d_style == D2D1_DASH_STYLE_DASH_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f %.0f %.0f\" ", a[0], a[1], a[2], a[3]);
		}
		else if (d_style == D2D1_DASH_STYLE_DASH_DOT_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f %.0f %.0f %.0f %.0f\" ", a[0], a[1], a[2], a[3], a[2], a[3]);
		}
		else {
			return;
		}
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG として命令と位置を書き込む.
	// val	位置
	// cmd	命令
	void dt_write_svg(const D2D1_POINT_2F val, const char* cmd, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s%f %f ", cmd, val.x, val.y);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG として属性名と位置を書き込む.
	// val	位置
	// name_x	x 軸の名前
	// name_y	y 軸の名前
	void dt_write_svg(const D2D1_POINT_2F val, const char* x_name, const char* y_name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%f\" %s=\"%f\" ", x_name, val.x, y_name, val.y);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG として属性名と浮動小数値を書き込む
	// val	浮動小数値
	// a_name	属性名
	void dt_write_svg(const double val, const char* a_name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%f\" ", a_name, val);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG として浮動小数値を書き込む
	// val	浮動小数値
	void dt_write_svg(const float val, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%f ", val);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG として属性名と 32 ビット正整数を書き込む.
	// val	32 ビット正整数
	// a_name	属性名
	void dt_write_svg(const uint32_t val, const char* a_name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%u\" ", a_name, val);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG としてマルチバイト文字列を書き込む.
	// val	マルチバイト文字列
	// v_len	文字列の文字数
	// dt_writer	データライター
	void dt_write_svg(const wchar_t val[], const uint32_t v_len, DataWriter const& dt_writer)
	{
		if (v_len > 0) {
			const auto s_len = WideCharToMultiByte(CP_UTF8, 0, val, v_len, (char*)NULL, 0, NULL, NULL);
			auto s = new char[static_cast<size_t>(s_len) + 1];
			WideCharToMultiByte(CP_UTF8, 0, val, v_len, static_cast<LPSTR>(s), s_len, NULL, NULL);
			s[s_len] = '\0';
			dt_write_svg(s, dt_writer);
			delete[] s;
		}
	}

	//------------------------------
	// データライターに SVG として書き込む.
	// dt_reader	データリーダー
	//------------------------------
	void ShapeBezi::write_svg(DataWriter const& dt_writer) const
	{
		D2D1_BEZIER_SEGMENT b_seg;

		pt_add(m_pos, m_vec[0], b_seg.point1);
		pt_add(b_seg.point1, m_vec[1], b_seg.point2);
		pt_add(b_seg.point2, m_vec[2], b_seg.point3);
		dt_write_svg("<path d=\"", dt_writer);
		dt_write_svg(m_pos, "M", dt_writer);
		dt_write_svg(b_seg.point1, "C", dt_writer);
		dt_write_svg(b_seg.point2, ",", dt_writer);
		dt_write_svg(b_seg.point3, ",", dt_writer);
		dt_write_svg("\" ", dt_writer);
		dt_write_svg("none", "fill", dt_writer);
		write_svg_stroke(dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[3];
			bezi_calc_arrow(m_pos, b_seg, m_arrow_size, barbs);
			write_svg_barbs(barbs, barbs[2], dt_writer);
		}
	}

	// データライターに SVG タグとして書き込む.
	void ShapeElli::write_svg(DataWriter const& dt_writer) const
	{
		D2D1_POINT_2F r;
		pt_mul(m_vec[0], 0.5, r);
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, r, c_pos);
		dt_write_svg("<ellipse ", dt_writer);
		dt_write_svg(c_pos, "cx", "cy", dt_writer);
		dt_write_svg(static_cast<double>(r.x), "rx", dt_writer);
		dt_write_svg(static_cast<double>(r.y), "ry", dt_writer);
		dt_write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
	}

	//------------------------------
	// データライターに SVG として書き込む.
	//------------------------------
	void ShapeGroup::write_svg(DataWriter const& dt_writer) const
	{
		dt_write_svg("<g>" SVG_NEW_LINE, dt_writer);
		for (const Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			s->write_svg(dt_writer);
		}
		dt_write_svg("</g>" SVG_NEW_LINE, dt_writer);
	}

	// データライターに SVG として書き込む.
	// file_name	画像ファイル名
	// dt_write		データライター
	void ShapeImage::write_svg(const wchar_t file_name[], DataWriter const& dt_writer) const
	{
		dt_write_svg("<image href=\"", dt_writer);
		dt_write_svg(file_name, wchar_len(file_name), dt_writer);
		dt_write_svg("\" ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_view.width, "width", dt_writer);
		dt_write_svg(m_view.height, "height", dt_writer);
		dt_write_svg(m_opac, "opacity", dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
	}

	// データライターに SVG として書き込む.
	// 画像なしの場合.
	// dt_write		データライター
	void ShapeImage::write_svg(DataWriter const& dt_writer) const
	{
		constexpr char RECT[] =
			"<rect x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" "
			"stroke=\"black\" fill=\"white\" />" SVG_NEW_LINE;
		constexpr char TEXT[] =
			"<text x=\"50%\" y=\"50%\" text-anchor=\"middle\" dominant-baseline=\"central\">"
			"NO IMAGE</text>" SVG_NEW_LINE;
		dt_write_svg("<!--" SVG_NEW_LINE, dt_writer);
		write_svg(nullptr, dt_writer);
		dt_write_svg("-->" SVG_NEW_LINE, dt_writer);
		dt_write_svg("<svg ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_view.width, "width", dt_writer);
		dt_write_svg(m_view.height, "height", dt_writer);
		dt_write_svg("viewBox=\"0 0 ", dt_writer);
		dt_write_svg(m_view.width, dt_writer);
		dt_write_svg(m_view.height, dt_writer);
		dt_write_svg("\">" SVG_NEW_LINE, dt_writer);
		dt_write_svg(RECT, dt_writer);
		dt_write_svg(TEXT, dt_writer);
		dt_write_svg("</svg>" SVG_NEW_LINE, dt_writer);
	}

	// データライターに SVG タグとして書き込む.
	void ShapeLine::write_svg(DataWriter const& dt_writer) const
	{
		D2D1_POINT_2F e_pos;

		pt_add(m_pos, m_vec[0], e_pos);
		dt_write_svg("<line ", dt_writer);
		dt_write_svg(m_pos, "x1", "y1", dt_writer);
		dt_write_svg(e_pos, "x2", "y2", dt_writer);
		write_svg_stroke(dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[2];
			D2D1_POINT_2F tip_pos;
			if (ShapeLine::line_get_arrow_pos(m_pos, m_vec[0], m_arrow_size, barbs, tip_pos)) {
				write_svg_barbs(barbs, tip_pos, dt_writer);
			}
		}
	}

	// 矢じりをデータライターに SVG タグとして書き込む.
	// barbs	矢じるしの両端の位置 [2]
	// tip_pos	矢じるしの先端の位置
	// a_style	矢じるしの形状
	// dt_writer	データライター
	void ShapeLine::write_svg_barbs(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const
	{
		dt_write_svg("<path d=\"", dt_writer);
		dt_write_svg("M", dt_writer);
		dt_write_svg(barbs[0].x, dt_writer);
		dt_write_svg(barbs[0].y, dt_writer);
		dt_write_svg("L", dt_writer);
		dt_write_svg(tip_pos.x, dt_writer);
		dt_write_svg(tip_pos.y, dt_writer);
		dt_write_svg("L", dt_writer);
		dt_write_svg(barbs[1].x, dt_writer);
		dt_write_svg(barbs[1].y, dt_writer);
		dt_write_svg("\" ", dt_writer);
		if (m_arrow_style == ARROW_STYLE::FILLED) {
			dt_write_svg(m_stroke_color, "fill", dt_writer);
		}
		else {
			dt_write_svg("fill=\"none\" ", dt_writer);
		}
		dt_write_svg(m_stroke_color, "stroke", dt_writer);
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
			//dt_write_svg("stroke-linecap=\"square\" ", dt_writer);
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
		dt_write_svg(" />" SVG_NEW_LINE, dt_writer);
	}

	// データライターに SVG タグとして書き込む.
	void ShapePoly::write_svg(DataWriter const& dt_writer) const
	{
		dt_write_svg("<path d=\"", dt_writer);
		dt_write_svg(m_pos, "M", dt_writer);
		const auto d_cnt = m_vec.size();	// 差分の数
		const auto v_cnt = d_cnt + 1;
		D2D1_POINT_2F v_pos[MAX_N_GON];

		v_pos[0] = m_pos;
		for (size_t i = 0; i < d_cnt; i++) {
			dt_write_svg(m_vec[i], "l", dt_writer);
			pt_add(v_pos[i], m_vec[i], v_pos[i + 1]);
		}
		if (m_end_closed) {
			dt_write_svg("Z", dt_writer);
		}
		dt_write_svg("\" ", dt_writer);
		write_svg_stroke(dt_writer);
		dt_write_svg(m_fill_color, "fill", dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F h_tip;
			D2D1_POINT_2F h_barbs[2];
			if (ShapePoly::poly_get_arrow_barbs(v_cnt, v_pos, m_arrow_size, h_tip, h_barbs)) {
				write_svg_barbs(h_barbs, h_tip, dt_writer);
			}
		}
	}

	// データライターに SVG タグとして書き込む.
	void ShapeRect::write_svg(DataWriter const& dt_writer) const
	{
		dt_write_svg("<rect ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_vec[0], "width", "height", dt_writer);
		dt_write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
	}

	// データライターに SVG タグとして書き込む.
	void ShapeRRect::write_svg(DataWriter const& dt_writer) const
	{
		dt_write_svg("<rect ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_vec[0], "width", "height", dt_writer);
		if (std::round(m_corner_rad.x) != 0.0f && std::round(m_corner_rad.y) != 0.0f) {
			dt_write_svg(m_corner_rad, "rx", "ry", dt_writer);
		}
		dt_write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		dt_write_svg("/>", dt_writer);
	}

	// データライターに SVG タグとして書き込む.
	void ShapeStroke::write_svg_stroke(DataWriter const& dt_writer) const
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