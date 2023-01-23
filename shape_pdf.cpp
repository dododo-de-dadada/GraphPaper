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
	static size_t export_pdf_barbs(const float width, const D2D1_COLOR_F& color, const ARROW_STYLE style, const D2D1_SIZE_F page_size, const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer);
	static size_t export_pdf_stroke(const float width, const D2D1_COLOR_F& color, const CAP_STYLE& cap, const D2D1_DASH_STYLE dash, const DASH_PATT& patt, const D2D1_LINE_JOIN join, const float miter_limit, const DataWriter& dt_writer);

	//------------------------------
	// 矢じるしをデータライターに PDF として書き込む.
	// width	線・枠の太さ
	// stroke	線・枠の色
	// dt_weiter	データライター
	// styke	矢じるしの形式
	// page_size	ページの大きさ
	// barbs	矢じりの返しの位置
	// tip_pos	矢じりの先端の位置
	// 戻り値	書き込んだバイト数
	//------------------------------
	static size_t export_pdf_barbs(const float width, const D2D1_COLOR_F& stroke, const ARROW_STYLE style, const D2D1_SIZE_F page_size, const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer)
	{
		if (equal(width, 0.0f) || !is_opaque(stroke)) {
			return 0;
		}
		wchar_t buf[1024];
		size_t len = 0;

		// 実線に戻す.
		len += dt_writer.WriteString(L"[ ] 0 d\n");
		if (style == ARROW_STYLE::FILLED) {
			swprintf_s(buf,
				L"%f %f %f rg\n",
				stroke.r, stroke.g, stroke.b);
			len += dt_writer.WriteString(buf);
		}
		swprintf_s(buf,
			L"%f %f m %f %f l %f %f l\n",
			barbs[0].x, -barbs[0].y + page_size.height,
			tip_pos.x, -tip_pos.y + page_size.height,
			barbs[1].x, -barbs[1].y + page_size.height
		);
		len += dt_writer.WriteString(buf);
		if (style == ARROW_STYLE::OPENED) {
			len += dt_writer.WriteString(L"S\n");
		}
		else if (style == ARROW_STYLE::FILLED) {
			len += dt_writer.WriteString(L"b\n");	// b はパスを閉じて (B は閉じずに) 塗りつぶす.
		}
		return len;
	}

	//------------------------------
	// PDF のパス描画命令を得る.
	// C	パスを閉じるなら true, 開いたままなら false
	// width	線・枠の太さ
	// stroke	線・枠の色
	// fill	塗りつぶし色
	// cmd	得られたパス描画命令
	// 戻り値	命令が得られたなら true, なければ false
	//------------------------------
	template <bool C>
	static bool export_pdf_cmd(const float width, const D2D1_COLOR_F& stroke, const D2D1_COLOR_F& fill, wchar_t*& cmd)
	{
		if (!equal(width, 0.0f) && is_opaque(stroke)) {
			if (is_opaque(fill)) {
				// B* = パスを塗りつぶして、ストロークも描画する (偶奇規則)
				// b* = B* と同じだが、描画前にパスを閉じる
				if constexpr (C) {
					cmd = L"b*\n";
				}
				else {
					cmd = L"B*\n";
				}
			}
			else {
				// S = パスをストロークで描画
				// s = 現在のパスを閉じた後 (開始点までを直線でつなぐ)、ストロークで描画
				if constexpr (C) {
					cmd = L"s\n";
				}
				else {
					cmd = L"S\n";
				}
			}
		}
		else {
			if (is_opaque(fill)) {
				// f* = 偶奇規則を使用してパスを塗りつぶす。
				// パスは自動的に閉じられる。
				cmd = L"f*\n";
			}
			else {
				return false;
			}
		}
		return true;
	}

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	static size_t export_pdf_stroke(const float width, const D2D1_COLOR_F& color, const CAP_STYLE& cap, const D2D1_DASH_STYLE dash, const DASH_PATT& patt, const D2D1_LINE_JOIN join, const float miter_limit, const DataWriter& dt_writer)
	{
		size_t len = 0;
		wchar_t buf[1024];

		// 線枠の太さ
		swprintf_s(buf, L"%f w\n", width);
		len += dt_writer.WriteString(buf);

		// 線枠の色
		swprintf_s(buf, 
			L"%f %f %f RG\n",	// RG はストローク用の色指定
			color.r, color.g, color.b);
		len += dt_writer.WriteString(buf);

		// 線枠の端の形式
		if (equal(cap, CAP_SQUARE)) {
			len += dt_writer.WriteString(L"2 J\n");
		}
		else if (equal(cap, CAP_ROUND)) {
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
		// 丸い
		else if (equal(join, D2D1_LINE_JOIN_ROUND)) {
			len += dt_writer.WriteString(L"1 j\n");
		}
		// PDF には留め継ぎ (マイター) あるいは面取り (ベベル) しかない.
		else {
			//if (equal(m_join_style, D2D1_LINE_JOIN_MITER) ||
			//equal(m_join_style, D2D1_LINE_JOIN_MITER_OR_BEVEL)) {
			// マイター制限
			swprintf_s(buf, 
				L"0 j\n"
				L"%f M\n", 
				miter_limit);
			len += dt_writer.WriteString(buf);
		}

		// 破線の形式
		if (dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH) {
			// 最後の数値は配置 (破線パターン) を適用するオフセット.
			// [] 0		| 実線
			// [3] 0	| ***___ ***___
			// [2] 1	| *__**__**
			// [2 1] 0	| **_**_ **_
			// [3 5] 6	| __ ***_____***_____
			// [2 3] 11	| *___ **___ **___
			swprintf_s(buf, 
				L"[ %f %f ] 0 d\n",
				patt.m_[0], patt.m_[1]);
			len += dt_writer.WriteString(buf);
		}
		else if (dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT) {
			swprintf_s(buf, 
				L"[ %f %f %f %f ] 0 d\n",
				patt.m_[0], patt.m_[1], patt.m_[2], patt.m_[3]);
			len += dt_writer.WriteString(buf);
		}
		else if (dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
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
	size_t ShapeBezi::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer)
	{
		wchar_t* cmd;
		if (!export_pdf_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
			return 0;
		}

		D2D1_BEZIER_SEGMENT b_seg;
		pt_add(m_start, m_vec[0], b_seg.point1);
		pt_add(b_seg.point1, m_vec[1], b_seg.point2);
		pt_add(b_seg.point2, m_vec[2], b_seg.point3);

		wchar_t buf[1024];
		size_t len = dt_writer.WriteString(
			L"% Bezier curve\n");

		swprintf_s(buf,
			L"%f %f %f rg\n",	// rg はストローク以外用
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_dash_style, m_dash_patt, m_join_style, m_join_miter_limit, dt_writer);
	
		swprintf_s(buf, 
			L"%f %f m %f %f %f %f %f %f c %s",
			m_start.x, -m_start.y + page_size.height,
			b_seg.point1.x, -b_seg.point1.y + page_size.height,
			b_seg.point2.x, -b_seg.point2.y + page_size.height,
			b_seg.point3.x, -b_seg.point3.y + page_size.height,
			cmd
		);
		len += dt_writer.WriteString(buf);
		if (m_arrow_style == ARROW_STYLE::OPENED ||
			m_arrow_style == ARROW_STYLE::FILLED) {
			D2D1_POINT_2F barbs[3];
			bezi_calc_arrow(m_start, b_seg, m_arrow_size, barbs);
			len += export_pdf_barbs(m_stroke_width, m_stroke_color, m_arrow_style, page_size, barbs, barbs[2], dt_writer);
		}
		return len;
	}

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeLine::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer)
	{
		if (equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) {
			return 0;
		}

		size_t len = dt_writer.WriteString(
			L"% Line\n");	// 書き込んだバイト数

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_dash_style, m_dash_patt, m_join_style, m_join_miter_limit, dt_writer);

		wchar_t buf[1024];
		swprintf_s(buf, 
			L"%f %f m %f %f l S\n",
			m_start.x, -m_start.y + page_size.height,
			m_start.x + m_vec[0].x, -(m_start.y + m_vec[0].y) + page_size.height
		);
		len += dt_writer.WriteString(buf);

		if (m_arrow_style == ARROW_STYLE::OPENED ||
			m_arrow_style == ARROW_STYLE::FILLED) {
			D2D1_POINT_2F barbs[3];
			if (line_get_arrow_pos(m_start, m_vec[0], m_arrow_size, barbs, barbs[2])) {
				len += export_pdf_barbs(m_stroke_width, m_stroke_color, m_arrow_style, page_size, barbs, barbs[2], dt_writer);
			}
		}
		return len;
	}

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapePoly::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer)
	{
		wchar_t* cmd;	// パス描画命令
		if (m_end_closed) {
			if (!export_pdf_cmd<true>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
				return 0;
			}
		}
		else {
			if (!export_pdf_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
				return 0;
			}
		}

		size_t len = dt_writer.WriteString(
			L"% Polyline\n");
		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_dash_style, m_dash_patt, m_join_style, m_join_miter_limit, dt_writer);

		wchar_t buf[1024];
		swprintf_s(buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);

		const size_t v_cnt = m_vec.size() + 1;
		D2D1_POINT_2F v_pos[N_GON_MAX];
		v_pos[0] = m_start;
		swprintf_s(buf, L"%f %f m\n", v_pos[0].x, -v_pos[0].y + page_size.height);
		len += dt_writer.WriteString(buf);

		for (size_t i = 1; i < v_cnt; i++) {
			pt_add(v_pos[i - 1], m_vec[i - 1], v_pos[i]);
			swprintf_s(buf, L"%f %f l\n", v_pos[i].x, -v_pos[i].y + page_size.height);
			len += dt_writer.WriteString(buf);
		}
		len += dt_writer.WriteString(cmd);

		if (m_arrow_style == ARROW_STYLE::OPENED ||
			m_arrow_style == ARROW_STYLE::FILLED) {
			D2D1_POINT_2F h_tip;
			D2D1_POINT_2F h_barbs[2];
			if (poly_get_arrow_barbs(v_cnt, v_pos, m_arrow_size, h_tip, h_barbs)) {
				len += export_pdf_barbs(m_stroke_width, m_stroke_color, m_arrow_style, page_size, h_barbs, h_tip, dt_writer);
			}
		}
		return len;
	}

	// 図形をデータライターに PDF として書き込む.
	size_t ShapeElli::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer)
	{
		wchar_t* cmd;	// パス描画命令
		if (!export_pdf_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
			return 0;
		}

		const float ty = page_size.height;
		const double a = 4.0 * (sqrt(2.0) - 1.0) / 3.0;
		const float rx = 0.5f * m_vec[0].x;
		const float ry = 0.5f * m_vec[0].y;
		const float cx = m_start.x + rx;
		const float cy = m_start.y + ry;

		size_t len = dt_writer.WriteString(
			L"% Ellipse\n");

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_dash_style, m_dash_patt, m_join_style, m_join_miter_limit, dt_writer);

		wchar_t buf[1024];
		swprintf_s(buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f m\n"
			L"%f %f %f %f %f %f c\n",
			cx + rx, -(cy) + ty,
			cx + rx, -(cy + a * ry) + ty,
			cx + a * rx, -(cy + ry) + ty,
			cx, -(cy + ry) + ty
		);
		len += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f %f %f %f %f c\n",
			cx - a * rx, -(cy + ry) + ty,
			cx - rx, -(cy + a * ry) + ty,
			cx - rx, -(cy)+ty
		);
		len += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f %f %f %f %f c\n",
			cx - rx, -(cy - a * ry) + ty,
			cx - a * rx, -(cy - ry) + ty,
			cx, -(cy - ry) + ty
		);
		len += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f %f %f %f %f c %s",
			cx + a * rx, -(cy - ry) + ty,
			cx + rx, -(cy - a * ry) + ty,
			cx + rx, -(cy)+ty,
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
	size_t ShapeRect::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer)
	{
		wchar_t* cmd;
		if (!export_pdf_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
			return 0;
		}
		size_t len = dt_writer.WriteString(
			L"% Rectangle\n");

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_dash_style, m_dash_patt, m_join_style, m_join_miter_limit, dt_writer);

		wchar_t buf[1024];
		swprintf_s(buf,
			L"%f %f %f rg\n",	// rg = 塗りつぶし色
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f %f %f re %s",
			m_start.x, -(m_start.y) + page_size.height, m_vec[0].x, -m_vec[0].y,
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
	size_t ShapeRRect::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer)
	{
		wchar_t* cmd;
		if (!export_pdf_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
			return 0;
		}

		wchar_t buf[1024];
		size_t len = dt_writer.WriteString(L"% Rounded Rectangle\n");

		// 塗りつぶし色
		swprintf_s(buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_dash_style, m_dash_patt, m_join_style, m_join_miter_limit, dt_writer);

		const double a = 4.0 * (sqrt(2.0) - 1.0) / 3.0;	// ベジェでだ円を近似する係数
		const float ty = page_size.height;	// D2D 座標を PDF ユーザー空間へ変換するため
		const float rx = (m_vec[0].x >= 0.0f ? m_corner_rad.x : -m_corner_rad.x);	// だ円の x 方向の半径
		const float ry = (m_vec[0].y >= 0.0f ? m_corner_rad.y : -m_corner_rad.y);	// だ円の y 方向の半径

		// 上辺の開始位置に移動.
		swprintf_s(buf,
			L"%f %f m\n",
			m_start.x + rx, -(m_start.y) + ty
		);
		len += dt_writer.WriteString(buf);

		// 上辺と右上の角を描く.
		float cx = m_start.x + m_vec[0].x - rx;	// 角丸の中心点 x
		float cy = m_start.y + ry;	// 角丸の中心点 y
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f %f %f %f %f c\n",
			cx, -(m_start.y) + ty,
			cx + a * rx, -(cy - ry) + ty,
			cx + rx, -(cy - a * ry) + ty,
			cx + rx, -(cy)+ty
		);
		len += dt_writer.WriteString(buf);

		// 右辺と右下の角を描く.
		cx = m_start.x + m_vec[0].x - rx;
		cy = m_start.y + m_vec[0].y - ry;
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f %f %f %f %f c\n",
			m_start.x + m_vec[0].x, -(cy)+ty,
			cx + rx, -(cy + a * ry) + ty,
			cx + a * rx, -(cy + ry) + ty,
			cx, -(cy + ry) + ty
		);
		len += dt_writer.WriteString(buf);

		//　下辺と左下の角を描く.
		cx = m_start.x + rx;
		cy = m_start.y + m_vec[0].y - ry;
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f %f %f %f %f c\n",
			cx, -(m_start.y + m_vec[0].y) + ty,
			cx - a * rx, -(cy + ry) + ty,
			cx - rx, -(cy + a * ry) + ty,
			cx - rx, -(cy)+ty
		);
		len += dt_writer.WriteString(buf);

		// 左辺と左上の角を描く.
		cx = m_start.x + rx;
		cy = m_start.y + ry;
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f %f %f %f %f c %s",
			m_start.x, -(cy)+ty,
			cx - rx, -(cy - a * ry) + ty,
			cx - a * rx, -(cy - ry) + ty,
			cx, -(cy - ry) + ty,
			cmd
		);
		len += dt_writer.WriteString(buf);
		return len;
	}

	// 図形をデータライターに PDF として書き込む.
	size_t ShapeImage::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer)
	{
		// PDF では表示の大きさの規定値は 1 x 1.
		// そもままでは, 画像全体が 1 x 1 にマッピングされる.
		// きちんと表示するには, 変換行列に大きさを指定し, 拡大する.
		// 表示する位置は, 左上でなく左下隅を指定する.
		wchar_t buf[1024];
		swprintf_s(buf,
			L"%% Image\n"
			L"q\n"
			L"%f 0 0 %f %f %f cm\n"
			L"/Image%d Do\n"
			L"Q\n",
			m_view.width, m_view.height,
			m_start.x, -(m_start.y + m_view.height) + page_size.height,
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
			uint32_t offset = get_uint32(table_data, 4ull + 8ull * i + 4);
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
	size_t ShapeText::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer)
	{
		size_t len = ShapeRect::export_pdf(page_size, dt_writer);

		// 書体の色が透明なら何もしない.
		if (!is_opaque(m_font_color)) {
			return len;
		}

		// テキストフォーマットが空なら作成する.
		if (m_dwrite_text_layout == nullptr) {
			create_text_layout();
		}

		len += dt_writer.WriteString(L"% Text\n");
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


		IDWriteFontFace3* face;	// フォントフェイス
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
			swprintf_s(buf,
				L"1 0 %f 1 %f %f Tm\n",
				oblique,
				m_start.x + m_text_padding.width + m_dwrite_test_metrics[i].left,
				-(m_start.y + m_text_padding.height + m_dwrite_test_metrics[i].top + m_dwrite_line_metrics[0].baseline) + page_size.height);
			len += dt_writer.WriteString(buf);

			// 文字列を書き込む.

			// wchar_t を GID に変換して書き出す.
			const auto utf32{ conv_utf16_to_utf32(t, t_len) };	// UTF-32 文字列
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
			std::vector<uint32_t> utf32{ conv_utf16_to_utf32(t, t_len) };
			for (int i = 0; i < utf32.size(); i++) {
				swprintf_s(buf, L"%06x", utf32[i]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"> Tj\n");
			*/

			/*
			// wchar_t を CID に変換して書き出す.
			len += dt_writer.WriteString(L"% CID\n<");
			std::vector<uint32_t> utf32{ conv_utf16_to_utf32(t, t_len) };
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

	size_t ShapeRuler::export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer)
	{
		// 線・枠の太さか色がなし, かつ塗りつぶし色もなしなら中断する.
		if ((equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) &&
			!is_opaque(m_fill_color)) {
			return 0;
		}

		if (m_dwrite_text_format == nullptr) {
			create_text_format();
		}
		constexpr wchar_t D[10] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9' };
		wchar_t buf[1024];
		size_t len = 0;
		IDWriteFontFace3* face;	// フォントフェイス
		get_font_face(face);
		std::vector utf32{ conv_utf16_to_utf32(D, 10) };	// UINT-32 文字列
		uint16_t gid[10];	// グリフ識別子
		face->GetGlyphIndices(std::data(utf32), 10, gid);
		DWRITE_FONT_METRICS f_met;	// 書体の計量
		face->GetMetrics(&f_met);
		int32_t g_adv[10];	// グリフごとの幅
		face->GetDesignGlyphAdvances(10, gid, g_adv);
		face->Release();

		const double f_size = m_font_size;	// 書体の大きさ
		const double l_height = f_size * (f_met.ascent + f_met.descent + f_met.lineGap) / f_met.designUnitsPerEm;	// 行の高さ
		const double b_line = f_size * (f_met.ascent) / f_met.designUnitsPerEm;	// (文字の上端からの) ベースラインまでの距離

		if (is_opaque(m_fill_color)) {

			// 塗りつぶし色が不透明な場合,
			// 方形を塗りつぶす.
			swprintf_s(buf,
				L"%% Ruler\n"
				L"%f %f %f rg\n"
				L"%f %f %f %f re\n"
				L"f*\n",
				m_fill_color.r, m_fill_color.g, m_fill_color.b,
				m_start.x, -(m_start.y) + page_size.height, m_vec[0].x, -m_vec[0].y
			);
			len += dt_writer.WriteString(buf);
		}
		if (is_opaque(m_stroke_color)) {

			// 線枠の色が不透明な場合,
			const double g_len = m_grid_base + 1.0;	// 方眼の大きさ
			const bool w_ge_h = fabs(m_vec[0].x) >= fabs(m_vec[0].y);	// 高さより幅の方が大きい
			const double vec_x = (w_ge_h ? m_vec[0].x : m_vec[0].y);	// 大きい方の値を x
			const double vec_y = (w_ge_h ? m_vec[0].y : m_vec[0].x);	// 小さい方の値を y
			const double intvl_x = vec_x >= 0.0 ? g_len : -g_len;	// 目盛りの間隔
			const double intvl_y = min(f_size, g_len);	// 目盛りの間隔
			const double x0 = (w_ge_h ? m_start.x : m_start.y);
			const double y0 = static_cast<double>(w_ge_h ? m_start.y : m_start.x) + vec_y;
			const double y1 = y0 - (vec_y >= 0.0 ? intvl_y : -intvl_y);
			const double y1_5 = y0 - 0.625 * (vec_y >= 0.0 ? intvl_y : -intvl_y);
			const double y2 = y1 - (vec_y >= 0.0 ? f_size : -f_size);
			/*
			DWRITE_PARAGRAPH_ALIGNMENT p_align;
			if (w_ge_h) {
				// 横のほうが大きい場合,
				// 高さが 0 以上の場合下よせ、ない場合上よせを段落のそろえに格納する.
				// 文字列を配置する方形が小さい (書体の大きさと同じ) ため,
				// DWRITE_PARAGRAPH_ALIGNMENT は, 逆の効果をもたらす.
				p_align = (m_vec[0].y >= 0.0f ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			}
			else {
				// 縦のほうが小さい場合,
				// 中段を段落のそろえに格納する.
				p_align = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			}
			*/
			len += export_pdf_stroke(1.0f, m_stroke_color, CAP_FLAT, D2D1_DASH_STYLE_SOLID, DASH_PATT{}, D2D1_LINE_JOIN_BEVEL, MITER_LIMIT_DEFVAL, dt_writer);

			const uint32_t k = static_cast<uint32_t>(floor(vec_x / intvl_x));	// 目盛りの数
			for (uint32_t i = 0; i <= k; i++) {

				// 方眼の大きさごとに目盛りを表示する.
				const double x = x0 + i * intvl_x;
				const D2D1_POINT_2F p{
					w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y0),
					w_ge_h ? static_cast<FLOAT>(y0) : static_cast<FLOAT>(x)
				};
				const auto y = ((i % 5) == 0 ? y1 : y1_5);
				const D2D1_POINT_2F q{
					w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y),
					w_ge_h ? static_cast<FLOAT>(y) : static_cast<FLOAT>(x)
				};
				swprintf_s(buf,
					L"%f %f m %f %f l\n"
					L"S\n",
					p.x, -p.y + page_size.height,
					q.x, -q.y + page_size.height
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
			float before = 0;
			for (uint32_t i = 0; i <= k; i++) {
				// 方眼の大きさごとに目盛りを表示する.
				const double x = x0 + i * intvl_x;
				//const D2D1_POINT_2F p{
				//	w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y0),
				//	w_ge_h ? static_cast<FLOAT>(y0) : static_cast<FLOAT>(x)
				//};
				const D2D1_POINT_2F q{
					w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y1),
					w_ge_h ? static_cast<FLOAT>(y1) : static_cast<FLOAT>(x)
				};
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
				const double w = f_size * g_adv[i % 10] / f_met.designUnitsPerEm;
				const D2D1_POINT_2F r{
					w_ge_h ?
						// 目盛りの位置から字体の幅の半分だけずらし, 文字の基点とする.
						static_cast<FLOAT>(x - w / 2) :
						// 目盛りの位置から, 書体の半分の大きさだけずらし, 文字の中央位置を求め,
						// その位置から字体の幅の半分だけずらして, 文字の基点とする.
						static_cast<FLOAT>(m_vec[0].x >= 0.0f ? q.x - f_size / 2 - w / 2 : q.x + f_size / 2 - w / 2),
					w_ge_h ? 
						// 目盛りの位置から, 書体大きさの半分だけずらし, さらに行の高さの半分だけずらし,
						// 文字の上位置を求めたあと, その位置からベースラインの距離だけずらし,
						// 文字の基点とする.
						static_cast<FLOAT>(m_vec[0].y >= 0.0f ? q.y - f_size / 2 - l_height / 2 + b_line : q.y + f_size / 2 - l_height / 2 + b_line) :
						// 目盛りの位置から, 行の高さの半分だけずらして, 文字の上位置を求め,
						// その位置からベースラインまでの距離を加え, 文字の基点とする.
						static_cast<FLOAT>(q.y - l_height / 2 + b_line)
				};
				swprintf_s(buf,
					L"1 0 0 1 %f %f Tm <%04x> Tj\n",
					r.x, -r.y + page_size.height, gid[i % 10]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"ET\n");
		}
		return len;
	}

	// 図形をデータライターに PDF として書き込む.
	size_t ShapePage::export_pdf(const D2D1_SIZE_F /*page_size*/, DataWriter const& dt_writer)
	{
		const float grid_base = m_grid_base;
		const float grid_a = m_grid_color.a;
		const D2D1_COLOR_F grid_color{
			grid_a * m_grid_color.r + (1.0f - grid_a) * m_page_color.r,
			grid_a * m_grid_color.g + (1.0f - grid_a) * m_page_color.g,
			grid_a * m_grid_color.b + (1.0f - grid_a) * m_page_color.b,
			1.0f
		};
		const GRID_EMPH grid_emph = m_grid_emph;
		const D2D1_POINT_2F grid_offset = m_grid_offset;
		const float page_scale = m_page_scale;
		const D2D1_SIZE_F page_size = m_page_size;
		// 拡大されても 1 ピクセルになるよう拡大率の逆数を線枠の太さに格納する.
		const FLOAT grid_width = static_cast<FLOAT>(1.0 / page_scale);	// 方眼の太さ
		D2D1_POINT_2F h_start, h_end;	// 横の方眼の開始・終了位置
		D2D1_POINT_2F v_start, v_end;	// 縦の方眼の開始・終了位置
		v_start.y = 0.0f;
		h_start.x = 0.0f;
		const auto page_h = page_size.height;
		const auto page_w = page_size.width;
		v_end.y = page_size.height;
		h_end.x = page_size.width;
		const double grid_len = max(grid_base + 1.0, 1.0);

		size_t len = dt_writer.WriteString(L"% Grid Lines\n");	// 書き込んだバイト数
		len += export_pdf_stroke(
			0.0f,
			grid_color,
			CAP_FLAT,
			D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID, DASH_PATT{},
			D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL, MITER_LIMIT_DEFVAL, dt_writer);

		// 垂直な方眼を表示する.
		wchar_t buf[1024];
		float w;
		double x;
		for (uint32_t i = 0; (x = round((grid_len * i + grid_offset.x) / PT_ROUND) * PT_ROUND) < page_w; i++) {
			if (grid_emph.m_gauge_2 != 0 && (i % grid_emph.m_gauge_2) == 0) {
				w = 2.0F * grid_width;
			}
			else if (grid_emph.m_gauge_1 != 0 && (i % grid_emph.m_gauge_1) == 0) {
				w = grid_width;
			}
			else {
				w = 0.5F * grid_width;
			}
			v_start.x = v_end.x = static_cast<FLOAT>(x);

			swprintf_s(buf, 
				L"%f w %f %f m %f %f l S\n",
				w,
				v_start.x, -(v_start.y) + page_size.height,
				v_end.x, -(v_end.y) + page_size.height
			);
			len += dt_writer.WriteString(buf);
		}
		// 水平な方眼を表示する.
		double y;
		for (uint32_t i = 0; (y = round((grid_len * i + grid_offset.y) / PT_ROUND) * PT_ROUND) < page_h; i++) {
			if (grid_emph.m_gauge_2 != 0 && (i % grid_emph.m_gauge_2) == 0) {
				w = 2.0F * grid_width;
			}
			else if (grid_emph.m_gauge_1 != 0 && (i % grid_emph.m_gauge_1) == 0) {
				w = grid_width;
			}
			else {
				w = 0.5F * grid_width;
			}
			h_start.y = h_end.y = static_cast<FLOAT>(y);

			swprintf_s(buf,
				L"%f w %f %f m %f %f l S\n",
				w,
				h_start.x, -(h_start.y) + page_size.height,
				h_end.x, -(h_end.y) + page_size.height
			);
			len += dt_writer.WriteString(buf);
		}

		return len;
	}

	size_t ShapeGroup::export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer)
	{
		size_t len = 0;
		for (Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			len += s->export_pdf(page_size, dt_writer);
		}
		return len;
	}

}