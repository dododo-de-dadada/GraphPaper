//------------------------------
// shape_pdf.cpp
// PDF への書き込み.
//------------------------------

// PDF フォーマット
// https://aznote.jakou.com/prog/pdf/index.html

#include "pch.h"
#include "shape.h"
//#include "CMap.h"

using namespace winrt;
//using namespace winrt::CMap::implementation;

namespace winrt::GraphPaper::implementation
{
	// 矢じるしをデータライターに PDF として書き込む.
	static size_t export_pdf_arrow(const float width, const D2D1_COLOR_F& color, const ARROW_STYLE style, const D2D1_CAP_STYLE cap, const D2D1_LINE_JOIN join, const float join_limit, const D2D1_SIZE_F sheet_size, const D2D1_POINT_2F barb[], const D2D1_POINT_2F tip, DataWriter const& dt_writer);
	// ストロークをデータライターに PDF として書き込む.
	static size_t export_pdf_stroke(const float width, const D2D1_COLOR_F& color, const D2D1_CAP_STYLE& cap, const D2D1_DASH_STYLE dash, const DASH_PAT& patt, const D2D1_LINE_JOIN join, const float miter_limit, const DataWriter& dt_writer);
	// PDF のパス描画命令を得る.
	template <bool C> static bool export_pdf_path_cmd(const float width, const D2D1_COLOR_F& stroke,	const D2D1_COLOR_F& fill, wchar_t cmd[4]);

	// 矢じるしをデータライターに PDF として書き込む.
	// width	線・枠の太さ
	// stroke	線・枠の色
	// style	矢じるしの形式
	// cap	矢じるしの返しの形式
	// join	矢じるしの先端の形式
	// join_limit	尖り制限値
	// sheet_size	用紙の大きさ
	// barb[]	矢じりの返しの点
	// tip	矢じりの先端の点
	// dt_writer	データライター
	// 戻り値	書き込んだバイト数
	static size_t export_pdf_arrow(const float width, const D2D1_COLOR_F& stroke, const ARROW_STYLE style, const D2D1_CAP_STYLE cap, const D2D1_LINE_JOIN join, const float join_limit, const D2D1_SIZE_F sheet_size, const D2D1_POINT_2F barb[], const D2D1_POINT_2F tip, DataWriter const& dt_writer)
	{
		if (equal(width, 0.0f) || !is_opaque(stroke)) {
			return 0;
		}
		wchar_t buf[1024];
		size_t len = 0;

		len += export_pdf_stroke(width, stroke, cap, D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID, DASH_PAT{}, join, join_limit, dt_writer);
		// 実線に戻す.
		len += dt_writer.WriteString(L"[ ] 0 d\n");
		if (style == ARROW_STYLE::ARROW_FILLED) {
			swprintf_s(buf,
				L"%f %f %f rg\n",
				stroke.r, stroke.g, stroke.b);
			len += dt_writer.WriteString(buf);
		}
		const double b0x = barb[0].x;
		const double b0y = barb[0].y;
		const double tx = tip.x;
		const double ty = tip.y;
		const double b1x = barb[1].x;
		const double b1y = barb[1].y;
		const double sh = sheet_size.height;
		swprintf_s(buf,
			L"%f %f m %f %f l %f %f l\n",
			b0x, -b0y + sh,
			tx, -ty + sh,
			b1x, -b1y + sh
		);
		len += dt_writer.WriteString(buf);
		if (style == ARROW_STYLE::ARROW_OPENED) {
			len += dt_writer.WriteString(L"S\n");
		}
		else if (style == ARROW_STYLE::ARROW_FILLED) {
			len += dt_writer.WriteString(L"b\n");	// b はパスを閉じて (B は閉じずに) 塗りつぶす.
		}
		return len;
	}

	// PDF のパス描画命令を得る.
	// C	パスを閉じるなら true, 開いたままなら false
	// width	線枠の太さ
	// stroke	線枠の色
	// fill	塗りつぶし色
	// cmd[4]	パス描画命令
	// 戻り値	命令が得られたなら true, なければ false
	template <bool C>	// パスを閉じるなら true, 開いたままなら false
	static bool export_pdf_path_cmd(const float width, const D2D1_COLOR_F& stroke, const D2D1_COLOR_F& fill, wchar_t cmd[4])
	{
		// パス描画命令
		// B* = パスを塗りつぶして、ストロークも描画する (偶奇規則)
		// b* = B* と同じだが、描画前にパスを閉じる
		// S = パスをストロークで描画
		// s = 現在のパスを閉じた後 (開始点までを直線でつなぐ)、ストロークで描画
		// f* = 偶奇規則を使用してパスを塗りつぶし, パスは自動的に閉じられる.

		// 線枠が表示されるなら,
		if (!equal(width, 0.0f) && is_opaque(stroke)) {
			if (is_opaque(fill)) {
				// B* = パスを塗りつぶして、ストロークも描画する (偶奇規則)
				// b* = B* と同じだが、描画前にパスを閉じる
				if constexpr (C) {
					memcpy(cmd, L"b*\n", sizeof(L"b*\n"));
				}
				else {
					memcpy(cmd, L"B*\n", sizeof(L"B*\n"));
				}
			}
			else {
				// S = パスをストロークで描画
				// s = 現在のパスを閉じた後 (開始点までを直線でつなぐ)、ストロークで描画
				if constexpr (C) {
					memcpy(cmd, L"s\n", sizeof(L"s\n"));
				}
				else {
					memcpy(cmd, L"S\n", sizeof(L"S\n"));
				}
			}
		}
		else {
			if (is_opaque(fill)) {
				// f* = 偶奇規則を使用してパスを塗りつぶし, パスは自動的に閉じられる.
				memcpy(cmd, L"f*\n", sizeof(L"f*\n"));
			}
			else {
				return false;
			}
		}
		return true;
	}

	// ストロークをデータライターに PDF として書き込む.
	// width	線・枠の太さ
	// color	線・枠の色
	// cap	端点の形式
	// dash	破線の形式
	// patt	破線の配置
	// join	結合の形式
	// miter_limit	尖り制限値
	// dt_writer	データライター
	// 戻り値	書き込んだバイト数
	static size_t export_pdf_stroke(const float width, const D2D1_COLOR_F& color, const D2D1_CAP_STYLE& cap, const D2D1_DASH_STYLE dash, const DASH_PAT& patt, const D2D1_LINE_JOIN join, const float miter_limit, const DataWriter& dt_writer)
	{
		size_t len = 0;
		wchar_t buf[1024];

		// 線枠の太さ
		swprintf_s(buf, L"%f w\n", width);
		len += dt_writer.WriteString(buf);

		// 線枠の色
		swprintf_s(buf, 
			L"%f %f %f RG\n",	// RG はストローク色指定
			color.r, color.g, color.b);
		len += dt_writer.WriteString(buf);

		// 線枠の端の形式
		if (cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
			len += dt_writer.WriteString(L"2 J\n");
		}
		else if (cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
			len += dt_writer.WriteString(L"1 J\n");
		}
		else {
			len += dt_writer.WriteString(L"0 J\n");
		}

		// 線の結合の形式
		// 面取り
		if (equal(join, D2D1_LINE_JOIN_BEVEL)) {
			len += dt_writer.WriteString(L"2 j\n");
		}
		// 丸まり
		else if (equal(join, D2D1_LINE_JOIN_ROUND)) {
			len += dt_writer.WriteString(L"1 j\n");
		}
		// PDF には尖り (マイター) または面取り (ベベル) しかない.
		else {
			//if (equal(m_stroke_join, D2D1_LINE_JOIN_MITER) ||
			//equal(m_stroke_join, D2D1_LINE_JOIN_MITER_OR_BEVEL)) {
			// D2D の尖り制限とは異なるので注意.
			// 尖り制限を超える部分があるとき, D2D では超えた部分だけが断ち切られるが, 
			// PDF と SVG ではいきなり Bevel join　になる.
			swprintf_s(buf, 
				L"0 j\n"
				L"%f M\n",
				miter_limit);
			len += dt_writer.WriteString(buf);
		}

		// 破線の形式
		// 最後の数値は配置 (破線パターン) を適用するオフセット.
		// [] 0		| 実線
		// [3] 0	| ***___ ***___
		// [2] 1	| *__**__**
		// [2 1] 0	| **_**_ **_
		// [3 5] 6	| __ ***_____***_____
		// [2 3] 11	| *___ **___ **___
		if (dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH) {
			// 破線
			swprintf_s(buf, 
				L"[ %f %f ] 0 d\n",
				patt.m_[0], patt.m_[1]);
			len += dt_writer.WriteString(buf);
		}
		else if (dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT) {
			// 点線
			swprintf_s(buf,
				L"[ %f %f ] 0 d\n",
				patt.m_[2], patt.m_[3]);
			len += dt_writer.WriteString(buf);
		}
		else if (dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT) {
			// 一点破線
			swprintf_s(buf,
				L"[ %f %f %f %f ] 0 d\n",
				patt.m_[0], patt.m_[1], patt.m_[2], patt.m_[3]);
			len += dt_writer.WriteString(buf);
		}
		else if (dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
			// 二点破線
			swprintf_s(buf,
				L"[ %f %f %f %f %f %f ] 0 d\n",
				patt.m_[0], patt.m_[1], patt.m_[2], patt.m_[3], patt.m_[4], patt.m_[5]);
			len += dt_writer.WriteString(buf);
		}
		else {
			// 実線
			len += dt_writer.WriteString(
				L"[ ] 0 d\n");
		}
		return len;
	}

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeBezier::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		wchar_t cmd[4];
		if (!export_pdf_path_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
			return 0;
		}

		D2D1_BEZIER_SEGMENT b_seg;
		pt_add(m_start, m_lineto[0], b_seg.point1);
		pt_add(b_seg.point1, m_lineto[1], b_seg.point2);
		pt_add(b_seg.point2, m_lineto[2], b_seg.point3);

		wchar_t buf[1024];
		size_t len = 0;
		swprintf_s(buf,
			L"%% Bezier curve\n"
			L"%f %f %f rg\n",	// rg はストローク以外用
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);
		const double sh = sheet_size.height;
		const double sx = m_start.x;
		const double sy = m_start.y;
		const double b1x = b_seg.point1.x;
		const double b1y = b_seg.point1.y;
		const double b2x = b_seg.point2.x;
		const double b2y = b_seg.point2.y;
		const double b3x = b_seg.point3.x;
		const double b3y = b_seg.point3.y;
		swprintf_s(buf, 
			L"%f %f m %f %f %f %f %f %f c %s",
			sx, -sy + sh,
			b1x, -b1y + sh,
			b2x, -b2y + sh,
			b3x, -b3y + sh, cmd
		);
		len += dt_writer.WriteString(buf);
		if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			D2D1_POINT_2F barbs[3];
			bezi_get_pos_arrow(m_start, b_seg, m_arrow_size, barbs);
			len += export_pdf_arrow(m_stroke_width, m_stroke_color, m_arrow_style, m_arrow_cap, m_arrow_join, m_arrow_join_limit, sheet_size, barbs, barbs[2], dt_writer);
		}
		return len;
	}

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeLine::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		if (equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) {
			return 0;
		}

		size_t len = 0;	// 書き込んだバイト数
		len += dt_writer.WriteString(
			L"% Line\n");

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);

		const double sx = m_start.x;
		const double sy = -static_cast<double>(m_start.y) + static_cast<double>(sheet_size.height);
		const double ex = static_cast<double>(m_start.x) + static_cast<double>(m_lineto.x);
		const double ey = -(static_cast<double>(m_start.y) + static_cast<double>(m_lineto.y)) + static_cast<double>(sheet_size.height);
		wchar_t buf[1024];
		swprintf_s(buf,
			L"%f %f m %f %f l S\n",
			sx, sy, ex, ey
		);
		len += dt_writer.WriteString(buf);

		if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			D2D1_POINT_2F barbs[3];
			if (line_get_pos_arrow(m_start, m_lineto, m_arrow_size, barbs, barbs[2])) {
				len += export_pdf_arrow(m_stroke_width, m_stroke_color, m_arrow_style, m_arrow_cap, m_arrow_join, m_arrow_join_limit, sheet_size, barbs, barbs[2], dt_writer);
			}
		}
		return len;
	}

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapePoly::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		wchar_t cmd[4];
		if (m_end == D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED) {
			if (!export_pdf_path_cmd<true>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
				return 0;
			}
		}
		else {
			if (!export_pdf_path_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
				return 0;
			}
		}
		size_t len = 0;
		len += dt_writer.WriteString(
			L"% Polyline\n");
		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);

		wchar_t buf[1024];
		swprintf_s(buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);

		// 矢じるしつきならば後で必要になるので, 折れ線の各点を求めながら出力する.
		const size_t p_cnt = m_lineto.size() + 1;	// 折れ線各点の数
		D2D1_POINT_2F p[N_GON_MAX];	// 折れ線各点の配列
		p[0] = m_start;
		const double sx = p[0].x;
		const double sy = -static_cast<double>(p[0].y) + static_cast<double>(sheet_size.height);
		swprintf_s(buf, L"%f %f m\n", sx, sy);
		len += dt_writer.WriteString(buf);
		for (size_t i = 1; i < p_cnt; i++) {
			pt_add(p[i - 1], m_lineto[i - 1], p[i]);
			const double px = p[i].x;
			const double py = -static_cast<double>(p[i].y) + static_cast<double>(sheet_size.height);
			swprintf_s(buf, L"%f %f l\n", px, py);
			len += dt_writer.WriteString(buf);
		}
		len += dt_writer.WriteString(cmd);

		if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			D2D1_POINT_2F tip;
			D2D1_POINT_2F barb[2];
			if (poly_get_pos_arrow(p_cnt, p, m_arrow_size, barb, tip)) {
				len += export_pdf_arrow(m_stroke_width, m_stroke_color, m_arrow_style, m_arrow_cap, m_arrow_join, m_arrow_join_limit, sheet_size, barb, tip, dt_writer);
			}
		}
		return len;
	}

	// 図形をデータライターに PDF として書き込む.
	size_t ShapeEllipse::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		wchar_t cmd[4];	// パス描画命令
		if (!export_pdf_path_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
			return 0;
		}

		const double sh = sheet_size.height;
		constexpr double a = 4.0 * (M_SQRT2 - 1.0) / 3.0;
		const double rx = 0.5 * m_lineto.x;
		const double ry = 0.5 * m_lineto.y;
		const double cx = m_start.x + rx;
		const double cy = m_start.y + ry;

		size_t len = 0;
		len += dt_writer.WriteString(L"% Ellipse\n");

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);

		wchar_t buf[1024];
		swprintf_s(buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f m\n"
			L"%f %f %f %f %f %f c\n",
			cx + rx, -(cy) + sh,
			cx + rx, -(cy + a * ry) + sh,
			cx + a * rx, -(cy + ry) + sh,
			cx, -(cy + ry) + sh
		);
		len += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f %f %f %f %f c\n",
			cx - a * rx, -(cy + ry) + sh,
			cx - rx, -(cy + a * ry) + sh,
			cx - rx, -(cy) + sh
		);
		len += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f %f %f %f %f c\n",
			cx - rx, -(cy - a * ry) + sh,
			cx - a * rx, -(cy - ry) + sh,
			cx, -(cy - ry) + sh
		);
		len += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f %f %f %f %f c %s",
			cx + a * rx, -(cy - ry) + sh,
			cx + rx, -(cy - a * ry) + sh,
			cx + rx, -(cy) + sh,
			cmd
		);
		len += dt_writer.WriteString(buf);
		return len;
	}

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeOblong::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		wchar_t cmd[4];	// パス描画命令
		if (!export_pdf_path_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
			return 0;
		}
		size_t len = 0;
		len += dt_writer.WriteString(
			L"% Rectangle\n");

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);

		wchar_t buf[1024];
		swprintf_s(buf,
			L"%f %f %f rg\n",	// rg = 塗りつぶし色
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);
		const double sx = m_start.x;
		const double sy = -static_cast<double>(m_start.y) + static_cast<double>(sheet_size.height);
		const double px = m_lineto.x;
		const double py = -static_cast<double>(m_lineto.y);
		swprintf_s(buf,
			L"%f %f %f %f re %s",
			sx, sy, px, py, cmd
		);
		len += dt_writer.WriteString(buf);
		return len;
	}

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeRRect::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		wchar_t cmd[4];	// パス描画命令
		if (!export_pdf_path_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
			return 0;
		}

		wchar_t buf[1024];
		size_t len = 0;
		len += dt_writer.WriteString(
			L"% Rounded Rectangle\n");
		// 塗りつぶし色
		swprintf_s(buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);

		constexpr double a = 4.0 * (M_SQRT2 - 1.0) / 3.0;	// ベジェでだ円を近似する係数
		const double sh = sheet_size.height;	// D2D 座標を PDF ユーザー空間へ変換するため
		const double rx = (m_lineto.x >= 0.0f ? m_corner_radius.x : -m_corner_radius.x);	// だ円の x 方向の半径
		const double ry = (m_lineto.y >= 0.0f ? m_corner_radius.y : -m_corner_radius.y);	// だ円の y 方向の半径

		// 上辺の始点.
		const double sx = m_start.x;
		const double sy = m_start.y;
		swprintf_s(buf,
			L"%f %f m\n",
			sx + rx, -(sy) + sh
		);
		len += dt_writer.WriteString(buf);

		// 上辺と右上の角を描く.
		const double px = static_cast<double>(m_lineto.x);
		const double py = static_cast<double>(m_lineto.y);
		double cx = sx + px - rx;	// 角丸の中心点 x
		double cy = sy + ry;	// 角丸の中心点 y
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f %f %f %f %f c\n",
			cx, -sy + sh,
			cx + a * rx, -(cy - ry) + sh,
			cx + rx, -(cy - a * ry) + sh,
			cx + rx, -(cy) + sh
		);
		len += dt_writer.WriteString(buf);

		// 右辺と右下の角を描く.
		cx = sx + px - rx;
		cy = sy + py - ry;
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f %f %f %f %f c\n",
			sx + px, -(cy) + sh,
			cx + rx, -(cy + a * ry) + sh,
			cx + a * rx, -(cy + ry) + sh,
			cx, -(cy + ry) + sh
		);
		len += dt_writer.WriteString(buf);

		//　下辺と左下の角を描く.
		cx = sx + rx;
		cy = sy + py - ry;
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f %f %f %f %f c\n",
			cx, -(sy + py) + sh,
			cx - a * rx, -(cy + ry) + sh,
			cx - rx, -(cy + a * ry) + sh,
			cx - rx, -cy + sh
		);
		len += dt_writer.WriteString(buf);

		// 左辺と左上の角を描く.
		cx = sx + rx;
		cy = sy + ry;
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f %f %f %f %f c %s",
			sx, -cy + sh,
			cx - rx, -(cy - a * ry) + sh,
			cx - a * rx, -(cy - ry) + sh,
			cx, -(cy - ry) + sh,
			cmd
		);
		len += dt_writer.WriteString(buf);
		return len;
	}

	// 図形をデータライターに PDF として書き込む.
	size_t ShapeImage::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		// PDF では表示の大きさの規定値は 1 x 1.
		// そもままでは, 画像全体が 1 x 1 にマッピングされる.
		// きちんと表示するには, 変換行列に大きさを指定し, 拡大する.
		// 表示する位置は, 左上でなく左下隅を指定する.
		wchar_t buf[1024];
		const double vw = m_view.width;
		const double vh = m_view.height;
		const double sx = m_start.x;
		const double sy = m_start.y;
		swprintf_s(buf,
			L"%% Image\n"
			L"q\n"
			L"%f 0 0 %f %f %f cm\n"
			L"/Image%d Do\n"
			L"Q\n",
			vw, vh, sx, -(sy + vh) + static_cast<double>(sheet_size.height),
			m_pdf_image_cnt
		);
		return dt_writer.WriteString(buf);
	}

	static uint16_t get_uint16(const void* addr, size_t offs)
	{
		const uint8_t* a = static_cast<const uint8_t*>(addr) + offs;
		return
			(static_cast<uint16_t>(a[0]) << 8) |
			(static_cast<uint16_t>(a[1]));
	}

	static uint32_t get_uint32(const void* addr, size_t offs)
	{
		const uint8_t* a = static_cast<const uint8_t*>(addr) + offs;
		return 
			(static_cast<uint32_t>(a[0]) << 24) | 
			(static_cast<uint32_t>(a[1]) << 16) |
			(static_cast<uint32_t>(a[2]) << 8) | 
			(static_cast<uint32_t>(a[3]));
	}

	static void export_cmap_subtable(const void* table_data, const size_t offset)
	{
		uint16_t format = get_uint16(table_data, offset);
		//Format 0: Byte encoding table
		if (format == 0) {

		}
		// Format 2: High-byte mapping through table
		else if (format == 2) {

		}
		// Format 4: Segment mapping to delta values
		else if (format == 4) {

		}
		// Format 6: Trimmed table mapping
		else if (format == 6) {

		}
		// Format 8: mixed 16-bit and 32-bit coverage
		else if (format == 8) {

		}
		// Format 10: Trimmed array
		else if (format == 10) {

		}
		// Format 12: Segmented coverage
		else if (format == 12) {

		}
	}

	static void export_cmap_table(const void* table_data, const UINT32 table_size, const void* table_context, const size_t offset)
	{
		//https://learn.microsoft.com/en-us/typography/opentype/spec/cmap
		//https://github.com/wine-mirror/wine/blob/master/dlls/dwrite/tests/font.c
		uint16_t version = get_uint16(table_data, 0);
		uint16_t numTables = get_uint16(table_data, 2);
		for (uint16_t i = 0; i < numTables; i++) {
			uint16_t platformID = get_uint16(table_data, 4ull + 8ull * i + 0);
			uint16_t encodingID = get_uint16(table_data, 4ull + 8ull * i + 2);
			size_t offset = get_uint32(table_data, 4ull + 8ull * i + 4);
			if (platformID == 0) {	// Unicode
				if (encodingID == 3) {	// Unicode 2.0 (BMP のみ)
					uint16_t format = get_uint16(table_data, offset);
					if (format == 0) {

					}
					else if (format == 4) {

					}
					else if (format == 6) {

					}
				}
				else if (encodingID == 4) {	// Unicode 2.0 (full repertoire)
					uint16_t format = get_uint16(table_data, offset);
					if (format == 0) {

					}
					else if (format == 4) {

					}
					else if (format == 6) {

					}
					else if (format == 10) {

					}
					else if (format == 12) {

					}
				}
				else if (encodingID == 6) {	// Unicode full repertoire
					uint16_t format = get_uint16(table_data, offset);
					if (format == 0) {

					}
					else if (format == 4) {

					}
					else if (format == 6) {

					}
					else if (format == 10) {

					}
					else if (format == 12) {

					}
				}
			}
			else if (platformID == 3) {
				uint16_t format = get_uint16(table_data, offset);
				if (encodingID == 0) {	// Symbol
				}
				else if (encodingID == 1) {	// Unicode BMP
					// format-4
				}
				else if (encodingID == 2) {	// ShiftJIS

				}
				else if (encodingID == 3) {	// PRC

				}
				else if (encodingID == 4) {	// Big5
				}
				else if (encodingID == 5) {	// Wansung
				}
				else if (encodingID == 6) {	// Johab
				}
				else if (encodingID == 10) {	// Unicode full repertoire
					if (format == 0) {

					}
					else if (format == 12) {
						uint16_t reserved = get_uint16(table_data, offset + 2);
						uint32_t len = get_uint32(table_data, offset + 4);
						uint32_t language = get_uint32(table_data, offset + 8);
						uint32_t numGroups = get_uint32(table_data, offset + 12);

						//uint32 startCharCode
						//uint32 endCharCode
						//uint32 startGlyphID
					}
				}
			}
		}
	}

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeText::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		size_t len = ShapeRect::export_pdf(sheet_size, dt_writer);

		// 書体の色が透明なら何もしない.
		if (!is_opaque(m_font_color)) {
			return len;
		}

		// テキストフォーマットが空なら作成する.
		if (m_dwrite_text_layout == nullptr) {
			create_text_layout();
		}

		len += dt_writer.WriteString(
			L"% Text\n");
		double oblique = (m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE ? tan(0.349066) : 0.0);

		wchar_t buf[1024];	// 出力用のバッファ

		// 文字列の色とフォント辞書の名前
		// rg = 色空間を DeviceRGB に設定し、同時に色を設定する (ストローク以外用)
		// RG = 色空間を DeviceRGB に設定し、同時に色を設定する (ストローク用)
		// BT = BT 命令から ET 命令の間に記述されたテキストは、一つのテキストオブジェクト
		// Tr = テキストレンダリングモードを設定. (0: 通常 (塗りつぶしのみ) [default])
		swprintf_s(buf,
			L"%f %f %f rg\n"
			L"%f %f %f RG\n"
			L"BT\n"
			L"/Font%d %f Tf\n"
			L"0 Tr\n",
			m_font_color.r, m_font_color.g, m_font_color.b,
			m_font_color.r, m_font_color.g, m_font_color.b,
			m_pdf_text_cnt, m_font_size
		);
		len += dt_writer.WriteString(buf);


		IDWriteFontFace3* face;	// 字面
		get_font_face(face);
		/*
		const void* table_data;
		UINT32 table_size;
		void* table_context;
		BOOL exists;
		face->TryGetFontTable(DWRITE_MAKE_OPENTYPE_TAG('c', 'm', 'a', 'p'), &table_data, &table_size, &table_context, &exists);
		face->ReleaseFontTable(table_context);
		*/

		for (uint32_t i = 0; i < m_dwrite_test_cnt; i++) {
			const wchar_t* t = m_text + m_dwrite_test_metrics[i].textPosition;	// 行の先頭文字を指すポインター
			const uint32_t t_len = m_dwrite_test_metrics[i].length;	// 行の文字数

			// Tm = テキスト空間からユーザー空間への変換行列を設定
			const double sx = m_start.x;	// 始点
			const double sy = m_start.y;	// 始点
			const double pw = m_text_padding.width;	// 内余白の幅
			const double ph = m_text_padding.height;	// 内余白の高さ
			const double left = m_dwrite_test_metrics[i].left;
			const double top = m_dwrite_test_metrics[i].top;
			const double bl = m_dwrite_line_metrics[0].baseline;
			swprintf_s(buf,
				L"1 0 %f 1 %f %f Tm\n",
				oblique, sx + pw + left, -(sy + ph + top + bl) + static_cast<double>(sheet_size.height));
			len += dt_writer.WriteString(buf);

			// 文字列を書き込む.

			// wchar_t を GID に変換して書き出す.
			const auto utf32{ text_utf16_to_utf32(t, t_len) };	// UTF-32 文字列
			const auto u_len = std::size(utf32);	// UTF-32 文字列の長さ
			std::vector<uint16_t> gid(u_len);	// グリフ識別子
			face->GetGlyphIndices(std::data(utf32), static_cast<UINT32>(u_len), std::data(gid));
			len += dt_writer.WriteString(L"<");
			for (uint32_t j = 0; j < u_len; j++) {
				if (gid[j] == 0) {
					continue;
				}
				swprintf_s(buf, L"%04x", gid[j]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"> Tj\n");

			/*
			// wchar_t を UTF-32 に変換して書き出す.
			len += dt_writer.WriteString(L"% UTF-32\n<");
			std::vector<uint32_t> utf32{ text_utf16_to_utf32(t, t_len) };
			for (int i = 0; i < utf32.size(); i++) {
				swprintf_s(buf, L"%06x", utf32[i]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"> Tj\n");
			*/

			/*
			// wchar_t を CID に変換して書き出す.
			len += dt_writer.WriteString(L"% CID\n<");
			std::vector<uint32_t> utf32{ text_utf16_to_utf32(t, t_len) };
			for (int i = 0; i < utf32.size(); i++) {
				const auto cid = cmap_getcid(utf32[i]);
				if (cid != 0) {
					swprintf_s(buf, L"%04x", cid);
					len += dt_writer.WriteString(buf);
				}
			}
			len += dt_writer.WriteString(L"> Tj\n");
			*/

			/*
			// wchar_t を UTF16 としてそのまま書き出す.
			len += dt_writer.WriteString(L"<");
			for (uint32_t j = 0; j < t_len; j++) {
				swprintf_s(buf, L"%04x", t[j]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"> Tj\n");
			*/

			/*
			const UINT CP = CP_ACP;	// コードページ
			const size_t mb_len = WideCharToMultiByte(CP, 0, t, t_len, NULL, 0, NULL, NULL);
			std::vector<uint8_t> mb_text(mb_len);	// マルチバイト文字列
			WideCharToMultiByte(CP, 0, t, t_len, (LPSTR)std::data(mb_text), static_cast<int>(mb_len), NULL, NULL);
			len += dt_writer.WriteString(L"<");
			for (int j = 0; j < mb_len; j++) {
				constexpr char* HEX = "0123456789abcdef";
				dt_writer.WriteByte(HEX[mb_text[j] >> 4]);
				dt_writer.WriteByte(HEX[mb_text[j] & 15]);
			}
			len += 2 * mb_len;
			len += dt_writer.WriteString(L"> Tj\n");
			*/
		}
		face->Release();

		// ET = BT 命令から ET 命令の間に記述されたテキストは、一つのテキストオブジェクト
		len += dt_writer.WriteString(L"ET\n");
		return len;
	}

	size_t ShapeRuler::export_pdf(const D2D1_SIZE_F sheet_size, const DataWriter& dt_writer)
	{
		const bool exist_stroke = (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color));
		// 線・枠の太さか色がなし, かつ塗りつぶし色もなしなら中断する.
		if (!exist_stroke && !is_opaque(m_fill_color)) {
			return 0;
		}

		if (m_dwrite_text_format == nullptr) {
			create_text_format();
		}
		constexpr wchar_t D[10] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9' };
		wchar_t buf[1024];
		size_t len = 0;
		IDWriteFontFace3* face;	// 字面
		get_font_face(face);
		std::vector utf32{ text_utf16_to_utf32(D, 10) };	// UINT-32 文字列
		uint16_t gid[10];	// グリフ識別子
		face->GetGlyphIndices(std::data(utf32), 10, gid);
		DWRITE_FONT_METRICS f_met;	// 書体の計量
		face->GetMetrics(&f_met);
		int32_t g_adv[10];	// グリフごとの幅
		face->GetDesignGlyphAdvances(10, gid, g_adv);
		face->Release();

		const double f_size = m_font_size;	// 書体の大きさ
		const double f_asc = f_met.ascent;
		const double f_des = f_met.descent;
		const double f_gap = f_met.lineGap;
		const double f_upe = f_met.designUnitsPerEm;
		const double l_height = f_size * (f_asc + f_des + f_gap) / f_upe;	// 行の高さ
		const double b_line = f_size * (f_asc) / f_upe;	// (文字の上端からの) ベースラインまでの距離

		if (is_opaque(m_fill_color)) {

			// 塗りつぶし色が不透明な場合,
			// 方形を塗りつぶす.
			const double sx = m_start.x;
			const double sy = m_start.y;
			const double px = m_lineto.x;
			const double py = m_lineto.y;
			swprintf_s(buf,
				L"%% Ruler\n"
				L"%f %f %f rg\n"
				L"%f %f %f %f re\n"
				L"f*\n",
				m_fill_color.r, m_fill_color.g, m_fill_color.b,
				sx, -(sy) + static_cast<double>(sheet_size.height), px, -py
			);
			len += dt_writer.WriteString(buf);
		}
		if (exist_stroke) {

			// 線枠の色が不透明な場合,
			const double g_len = m_grid_base + 1.0;	// 方眼の大きさ
			const bool w_ge_h = fabs(m_lineto.x) >= fabs(m_lineto.y);	// 高さより幅の方が大きい
			const double to_x = (w_ge_h ? m_lineto.x : m_lineto.y);	// 大きい方の値を x
			const double to_y = (w_ge_h ? m_lineto.y : m_lineto.x);	// 小さい方の値を y
			const double intvl_x = to_x >= 0.0 ? g_len : -g_len;	// 目盛りの間隔
			const double intvl_y = min(f_size, g_len);	// 目盛りの間隔
			const double x0 = (w_ge_h ? m_start.x : m_start.y);
			const double y0 = static_cast<double>(w_ge_h ? m_start.y : m_start.x) + to_y;
			const double y1 = y0 - (to_y >= 0.0 ? intvl_y : -intvl_y);
			const double y1_5 = y0 - 0.625 * (to_y >= 0.0 ? intvl_y : -intvl_y);
			const double y2 = y1 - (to_y >= 0.0 ? f_size : -f_size);
			/*
			DWRITE_PARAGRAPH_ALIGNMENT p_align;
			if (w_ge_h) {
				// 横のほうが大きい場合,
				// 高さが 0 以上の場合下よせ、ない場合上よせを段落のそろえに格納する.
				// 文字列を配置する方形が小さい (書体の大きさと同じ) ため,
				// DWRITE_PARAGRAPH_ALIGNMENT は, 逆の効果をもたらす.
				p_align = (m_pos[0].y >= 0.0f ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			}
			else {
				// 縦のほうが小さい場合,
				// 中段を段落のそろえに格納する.
				p_align = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			}
			*/
			len += export_pdf_stroke(1.0f, m_stroke_color, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_DASH_STYLE_SOLID, DASH_PAT{}, D2D1_LINE_JOIN_MITER_OR_BEVEL, JOIN_MITER_LIMIT_DEFVAL, dt_writer);

			const double sh = sheet_size.height;
			const uint32_t k = static_cast<uint32_t>(floor(to_x / intvl_x));	// 目盛りの数
			for (uint32_t i = 0; i <= k; i++) {

				// 方眼の大きさごとに目盛りを表示する.
				const double x = x0 + i * intvl_x;
				const double px = (w_ge_h ? x : y0);
				const double py = (w_ge_h ? y0 : x);
				//const D2D1_POINT_2F p{
				//	w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y0),
				//	w_ge_h ? static_cast<FLOAT>(y0) : static_cast<FLOAT>(x)
				//};
				const auto y = ((i % 5) == 0 ? y1 : y1_5);
				const double qx = (w_ge_h ? x : y);
				const double qy = (w_ge_h ? y : x);
				//const D2D1_POINT_2F q{
				//	w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y),
				//	w_ge_h ? static_cast<FLOAT>(y) : static_cast<FLOAT>(x)
				//};
				swprintf_s(buf,
					L"%f %f m %f %f l\n"
					L"S\n",
					px, -py + sh,
					qx, -qy + sh
				);
				len += dt_writer.WriteString(buf);
			}
			// 文字を表示するのに両方必要.
			// rg = 塗りつぶし色, RG = 線・枠の色
			swprintf_s(buf,
				L"%f %f %f rg\n"
				L"%f %f %f RG\n"
				L"BT\n"
				L"/Font%d %f Tf\n"
				L"0 Tr\n",
				m_stroke_color.r, m_stroke_color.g, m_stroke_color.b,
				m_stroke_color.r, m_stroke_color.g, m_stroke_color.b,
				m_pdf_text_cnt, m_font_size
			);
			len += dt_writer.WriteString(buf);
			//float before = 0;
			for (uint32_t i = 0; i <= k; i++) {
				// 方眼の大きさごとに目盛りを表示する.
				const double x = x0 + i * intvl_x;
				//const D2D1_POINT_2F q{
				//	w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y1),
				//	w_ge_h ? static_cast<FLOAT>(y1) : static_cast<FLOAT>(x)
				//};
				const double qx = (w_ge_h ? x : y1);
				const double qy = (w_ge_h ? y1 : x);
				// .left top
				// |                |      |
				// |       /\       |      |
				// |      /  \      |      |
				//b_line /    \     f_size |
				// |    /      \    |     l_height
				// |   /--------\   |      |
				// |  /          \  |      |
				//                  |      |
				//   |--g_adv[A]--|        |
				// |-----f_size-----|
				const double f_upe = f_met.designUnitsPerEm;
				const double w = f_size * g_adv[i % 10] / f_upe;
				const double rx = (
					w_ge_h ?
					// 目盛りの位置から字体の幅の半分だけずらし, 文字の基点とする.
					x - w / 2 :
					// 目盛りの位置から, 書体の半分の大きさだけずらし, 文字の中央位置を求め,
					// その位置から字体の幅の半分だけずらして, 文字の基点とする.
					(m_lineto.x >= 0.0f ? qx - f_size / 2 - w / 2 : qx + f_size / 2 - w / 2)
				);
				const double ry = (
					w_ge_h ?
					// 目盛りの位置から, 書体大きさの半分だけずらし, さらに行の高さの半分だけずらし,
					// 文字の上位置を求めたあと, その位置からベースラインの距離だけずらし,
					// 文字の基点とする.
					(m_lineto.y >= 0.0f ? qy - f_size / 2 - l_height / 2 + b_line : qy + f_size / 2 - l_height / 2 + b_line) :
					// 目盛りの位置から, 行の高さの半分だけずらして, 文字の上位置を求め,
					// その位置からベースラインまでの距離を加え, 文字の基点とする.
					qy - l_height / 2 + b_line
				);
				swprintf_s(buf,
					L"1 0 0 1 %f %f Tm <%04x> Tj\n",
					rx, -ry + sh, gid[i % 10]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"ET\n");
		}
		return len;
	}

	size_t ShapeSheet::export_pdf(const D2D1_COLOR_F& background, DataWriter const& dt_writer)
	{
		wchar_t buf[1024];	// PDF
		size_t len = 0;

		// PDF はアルファに対応してないので, 背景色と混ぜて, 用紙を塗りつぶす.
		const double page_a = m_sheet_color.a;
		const double page_r = page_a * m_sheet_color.r + (1.0 - page_a) * background.r;
		const double page_g = page_a * m_sheet_color.g + (1.0 - page_a) * background.g;
		const double page_b = page_a * m_sheet_color.b + (1.0 - page_a) * background.b;
		// re = 方形, f = 内部を塗りつぶす.
		// cm = 変換行列 (用紙の中では内余白の分平行移動)
		swprintf_s(buf,
			L"%f %f %f rg\n"
			L"0 0 %f %f re\n"
			L"f\n"
			L"1 0 0 1 %f %f cm\n",
			min(max(page_r, 0.0), 1.0),
			min(max(page_g, 0.0), 1.0),
			min(max(page_b, 0.0), 1.0),
			m_sheet_size.width,
			m_sheet_size.height,
			m_sheet_padding.left,
			-m_sheet_padding.top
		);
		len += dt_writer.WriteString(buf);


		if (m_grid_show == GRID_SHOW::FRONT || m_grid_show == GRID_SHOW::HIDE) {
			// 図形を出力
			const D2D1_SIZE_F sheet_size = m_sheet_size;
			for (const auto s : m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				len += s->export_pdf(sheet_size, dt_writer);
			}
		}

		if (m_grid_show == GRID_SHOW::FRONT || m_grid_show == GRID_SHOW::BACK) {
			const float grid_base = m_grid_base;
			// PDF はアルファに対応してないので, 背景色, 用紙色と混ぜる.
			const double page_a = m_sheet_color.a;
			const double page_r = page_a * m_sheet_color.r + (1.0 - page_a) * background.r;
			const double page_g = page_a * m_sheet_color.g + (1.0 - page_a) * background.g;
			const double page_b = page_a * m_sheet_color.b + (1.0 - page_a) * background.b;
			const double grid_a = m_grid_color.a;
			const double grid_r = grid_a * m_grid_color.r + (1.0f - grid_a) * page_r;
			const double grid_g = grid_a * m_grid_color.g + (1.0f - grid_a) * page_g;
			const double grid_b = grid_a * m_grid_color.b + (1.0f - grid_a) * page_b;
			const GRID_EMPH grid_emph = m_grid_emph;
			const D2D1_POINT_2F grid_offset = m_grid_offset;
			// 用紙の大きさから内余白の大きさを除く.
			const auto grid_w = m_sheet_size.width - (m_sheet_padding.left + m_sheet_padding.right);	// 方眼を描く領域の大きさ
			const auto grid_h = m_sheet_size.height - (m_sheet_padding.top + m_sheet_padding.bottom);	// 方眼を描く領域の大きさ

			const FLOAT stroke_w = 1.0;	// 方眼の太さ
			D2D1_POINT_2F h_start, h_end;	// 横の方眼の開始・終了位置
			D2D1_POINT_2F v_start, v_end;	// 縦の方眼の開始・終了位置
			v_start.y = 0.0f;
			h_start.x = 0.0f;
			v_end.y = grid_h;
			h_end.x = grid_w;
			const double grid_len = max(grid_base + 1.0, 1.0);
			len += dt_writer.WriteString(
				L"% Grid Lines\n");
			len += export_pdf_stroke(
				0.0f,
				D2D1_COLOR_F{ static_cast<FLOAT>(grid_r), static_cast<FLOAT>(grid_g), static_cast<FLOAT>(grid_b), 1.0f },
				D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID, DASH_PAT{},
				D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL, JOIN_MITER_LIMIT_DEFVAL, dt_writer);

			// 垂直な方眼を表示する.
			float sw;
			double x;
			for (uint32_t i = 0;
				(x = round((grid_len * i + grid_offset.x) / PT_ROUND) * PT_ROUND) <= grid_w; i++) {
				if (grid_emph.m_gauge_2 != 0 && (i % grid_emph.m_gauge_2) == 0) {
					sw = 2.0F * stroke_w;
				}
				else if (grid_emph.m_gauge_1 != 0 && (i % grid_emph.m_gauge_1) == 0) {
					sw = stroke_w;
				}
				else {
					sw = 0.5F * stroke_w;
				}
				v_start.x = v_end.x = static_cast<FLOAT>(x);
				const double sx = v_start.x;
				const double sy = v_start.y;
				const double ex = v_end.x;
				const double ey = v_end.y;
				const double ph = m_sheet_size.height;
				swprintf_s(buf,
					L"%f w %f %f m %f %f l S\n",
					sw,
					sx, -sy + ph,
					ex, -ey + ph
				);
				len += dt_writer.WriteString(buf);
			}
			// 水平な方眼を表示する.
			double y;
			for (uint32_t i = 0;
				(y = round((grid_len * i + grid_offset.y) / PT_ROUND) * PT_ROUND) <= grid_h; i++) {
				if (grid_emph.m_gauge_2 != 0 && (i % grid_emph.m_gauge_2) == 0) {
					sw = 2.0F * stroke_w;
				}
				else if (grid_emph.m_gauge_1 != 0 && (i % grid_emph.m_gauge_1) == 0) {
					sw = stroke_w;
				}
				else {
					sw = 0.5F * stroke_w;
				}
				h_start.y = h_end.y = static_cast<FLOAT>(y);
				const double sx = h_start.x;
				const double sy = h_start.y;
				const double ex = h_end.x;
				const double ey = h_end.y;
				const double ph = m_sheet_size.height;
				swprintf_s(buf,
					L"%f w %f %f m %f %f l S\n",
					sw,
					sx, -sy + ph,
					ex, -ey + ph
				);
				len += dt_writer.WriteString(buf);
			}

		}

		if (m_grid_show == GRID_SHOW::BACK) {
			// 図形を出力
			for (const auto s : m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				len += s->export_pdf(m_sheet_size, dt_writer);
			}
		}
		return len;
	}

	size_t ShapeGroup::export_pdf(const D2D1_SIZE_F sheet_size, const DataWriter& dt_writer)
	{
		size_t len = 0;
		for (Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			len += s->export_pdf(sheet_size, dt_writer);
		}
		return len;
	}

	size_t ShapeArc::export_pdf(const D2D1_SIZE_F sheet_size, const DataWriter& dt_writer)
	{
		if (!is_opaque(m_fill_color) && (equal(m_stroke_width, 0.0f) ||
			!is_opaque(m_stroke_color))) {
			return 0;
		}

		D2D1_POINT_2F start1{};
		D2D1_BEZIER_SEGMENT b_seg1{};
		alter_bezier(start1, b_seg1);

		D2D1_POINT_2F ctr{};
		if (is_opaque(m_fill_color) || m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			get_pos_loc(LOCUS_TYPE::LOCUS_A_CENTER, ctr);
		}

		size_t len = 0;
		if (is_opaque(m_fill_color)) {
			// rg = 塗りつぶし色
			// f* = 偶奇規則を使用してパスを塗りつぶす。
			// パスは自動的に閉じられる
			wchar_t buf[1024];
			const double sx = start1.x;
			const double sy = start1.y;
			const double b1x = b_seg1.point1.x;
			const double b1y = b_seg1.point1.y;
			const double b2x = b_seg1.point2.x;
			const double b2y = b_seg1.point2.y;
			const double b3x = b_seg1.point3.x;
			const double b3y = b_seg1.point3.y;
			const double sh = sheet_size.height;
			swprintf_s(buf,
				L"%f %f %f rg\n"
				L"%f %f m %f %f %f %f %f %f c %f %f l f*\n",
				m_fill_color.r, m_fill_color.g, m_fill_color.b,
				sx, -sy + sh,
				b1x, -b1y + sh,
				b2x, -b2y + sh,
				b3x, -b3y + sh,
				ctr.x, -ctr.y + sh
			);
			len += dt_writer.WriteString(buf);
		}
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);
			// S = パスをストロークで描画
			// パスは開いたまま.
			wchar_t buf[1024];
			const double sx = start1.x;
			const double sy = start1.y;
			const double b1x = b_seg1.point1.x;
			const double b1y = b_seg1.point1.y;
			const double b2x = b_seg1.point2.x;
			const double b2y = b_seg1.point2.y;
			const double b3x = b_seg1.point3.x;
			const double b3y = b_seg1.point3.y;
			const double sh = sheet_size.height;
			swprintf_s(buf,
				L"%f %f m %f %f %f %f %f %f c S\n",
				sx, -sy + sh,
				b1x, -b1y + sh,
				b2x, -b2y + sh,
				b3x, -b3y + sh
			);
			len += dt_writer.WriteString(buf);
			if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
				D2D1_POINT_2F arrow[3];
				arc_get_pos_arrow(m_lineto[0], ctr, m_radius, m_angle_start, m_angle_end, m_angle_rot, m_arrow_size, m_sweep_dir, arrow);
				len += export_pdf_arrow(m_stroke_width, m_stroke_color, m_arrow_style, m_arrow_cap, m_arrow_join, m_arrow_join_limit, sheet_size, arrow, arrow[2], dt_writer);
			}
		}
		return len;
	}
}