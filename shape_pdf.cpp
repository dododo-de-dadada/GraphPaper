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
	static size_t export_pdf_barbs(const ShapeLine* s, const D2D1_SIZE_F page_size, const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer);

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeBezi::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) const
	{
		if (equal(m_stroke_width, 0.0f) ||
			!is_opaque(m_stroke_color)) {
			return 0;
		}

		size_t n = dt_writer.WriteString(L"% Bezier curve\n");
		n += export_pdf_stroke(dt_writer);

		D2D1_BEZIER_SEGMENT b_seg;
		pt_add(m_start, m_vec[0], b_seg.point1);
		pt_add(b_seg.point1, m_vec[1], b_seg.point2);
		pt_add(b_seg.point2, m_vec[2], b_seg.point3);

		wchar_t buf[1024];
		swprintf_s(buf, L"%f %f m\n", m_start.x, -m_start.y + page_size.height);
		n += dt_writer.WriteString(buf);
		swprintf_s(buf, L"%f %f ", b_seg.point1.x, -b_seg.point1.y + page_size.height);
		n += dt_writer.WriteString(buf);
		swprintf_s(buf, L"%f %f ", b_seg.point2.x, -b_seg.point2.y + page_size.height);
		n += dt_writer.WriteString(buf);
		swprintf_s(buf, L"%f %f c\n", b_seg.point3.x, -b_seg.point3.y + page_size.height);
		n += dt_writer.WriteString(buf);
		n += dt_writer.WriteString(L"S\n");
		if (m_arrow_style == ARROW_STYLE::OPENED ||
			m_arrow_style == ARROW_STYLE::FILLED) {
			D2D1_POINT_2F barbs[3];
			bezi_calc_arrow(m_start, b_seg, m_arrow_size, barbs);
			n += export_pdf_barbs(this, page_size, barbs, barbs[2], dt_writer);
		}
		return n;
	}

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeLine::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) const
	{
		if (equal(m_stroke_width, 0.0f) ||
			!is_opaque(m_stroke_color)) {
			return 0;
		}
		size_t n = dt_writer.WriteString(L"% Line\n");
		n += export_pdf_stroke(dt_writer);

		wchar_t buf[1024];
		swprintf_s(buf, L"%f %f m\n", m_start.x, -m_start.y + page_size.height);
		n += dt_writer.WriteString(buf);
		swprintf_s(buf, L"%f %f l\n", m_start.x + m_vec[0].x, -(m_start.y + m_vec[0].y) + page_size.height);
		n += dt_writer.WriteString(buf);
		n += dt_writer.WriteString(L"S\n");
		if (m_arrow_style == ARROW_STYLE::OPENED || m_arrow_style == ARROW_STYLE::FILLED) {
			D2D1_POINT_2F barbs[3];
			if (line_get_arrow_pos(m_start, m_vec[0], m_arrow_size, barbs, barbs[2])) {
				n += export_pdf_barbs(this, page_size, barbs, barbs[2], dt_writer);
			}
		}
		return n;
	}

	//------------------------------
	// 矢じるしをデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	static size_t export_pdf_barbs(const ShapeLine* s, const D2D1_SIZE_F page_size, const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer)
	{
		if (equal(s->m_stroke_width, 0.0f) ||
			!is_opaque(s->m_stroke_color)) {
			return 0;
		}
		wchar_t buf[1024];
		size_t n = 0;

		// 破線ならば, 実線に戻す.
		if (s->m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH ||
			s->m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT ||
			s->m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
			n += dt_writer.WriteString(L"[ ] 0 d\n");
		}
		if (s->m_arrow_style == ARROW_STYLE::FILLED) {
			swprintf_s(buf, 
				L"%f %f %f rg\n", 
				s->m_stroke_color.r, s->m_stroke_color.g, s->m_stroke_color.b);
			n += dt_writer.WriteString(buf);
		}
		swprintf_s(buf, L"%f %f m\n", barbs[0].x, -barbs[0].y + page_size.height);
		n += dt_writer.WriteString(buf);
		swprintf_s(buf, L"%f %f l\n", tip_pos.x, -tip_pos.y + page_size.height);
		n += dt_writer.WriteString(buf);
		swprintf_s(buf, L"%f %f l\n", barbs[1].x, -barbs[1].y + page_size.height);
		n += dt_writer.WriteString(buf);
		if (s->m_arrow_style == ARROW_STYLE::OPENED) {
			n += dt_writer.WriteString(L"S\n");
		}
		else if (s->m_arrow_style == ARROW_STYLE::FILLED) {
			n += dt_writer.WriteString(L"b\n");	// b はパスを閉じて (B は閉じずに) 塗りつぶす.
		}
		return n;
	}

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapePoly::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) const
	{
		if ((equal(m_stroke_width, 0.0f) ||
			!is_opaque(m_stroke_color)) &&
			!is_opaque(m_fill_color)) {
			return 0;
		}

		size_t n = dt_writer.WriteString(L"% Polyline\n");
		n += export_pdf_stroke(dt_writer);

		wchar_t buf[1024];
		swprintf_s(
			buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		n += dt_writer.WriteString(buf);

		const size_t v_cnt = m_vec.size() + 1;
		D2D1_POINT_2F v_pos[N_GON_MAX];
		v_pos[0] = m_start;
		swprintf_s(buf, L"%f %f m\n", v_pos[0].x, -v_pos[0].y + page_size.height);
		n += dt_writer.WriteString(buf);
		for (size_t i = 1; i < v_cnt; i++) {
			pt_add(v_pos[i - 1], m_vec[i - 1], v_pos[i]);
			swprintf_s(buf, L"%f %f l\n", v_pos[i].x, -v_pos[i].y + page_size.height);
			n += dt_writer.WriteString(buf);
		}
		if (m_end_closed) {
			if (equal(m_fill_color.a, 0.0f)) {
				// s はパスを閉じて描画する.
				n += dt_writer.WriteString(L"s\n");
			}
			else {
				// b はパスを閉じて塗りつぶし, ストロークも描画する.
				n += dt_writer.WriteString(L"b\n");
			}
		}
		else {
			if (equal(m_fill_color.a, 0.0f)) {
				// S はパスを閉じずに描画する.
				n += dt_writer.WriteString(L"S\n");
			}
			else {
				// B はパスを閉じずに塗りつぶし, ストロークも描画する.
				n += dt_writer.WriteString(L"B\n");
			}
		}
		if (m_arrow_style == ARROW_STYLE::OPENED ||
			m_arrow_style == ARROW_STYLE::FILLED) {
			D2D1_POINT_2F h_tip;
			D2D1_POINT_2F h_barbs[2];
			if (poly_get_arrow_barbs(v_cnt, v_pos, m_arrow_size, h_tip, h_barbs)) {
				n += export_pdf_barbs(this, page_size, h_barbs, h_tip, dt_writer);
			}
		}
		return n;
	}

	// 図形をデータライターに PDF として書き込む.
	size_t ShapeElli::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) const
	{
		if ((equal(m_stroke_width, 0.0f) ||
			!is_opaque(m_stroke_color)) &&
			!is_opaque(m_fill_color)) {
			return 0;
		}

		size_t n = dt_writer.WriteString(L"% Ellipse\n");
		n += export_pdf_stroke(dt_writer);

		wchar_t buf[1024];
		swprintf_s(
			buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		n += dt_writer.WriteString(buf);

		const float ty = page_size.height;
		const double a = 4.0 * (sqrt(2.0) - 1.0) / 3.0;
		const float rx = 0.5f * m_vec[0].x;
		const float ry = 0.5f * m_vec[0].y;
		const float cx = m_start.x + rx;
		const float cy = m_start.y + ry;

		swprintf_s(buf,
			L"%f %f m\n",
			cx + rx, -(cy)+ty
		);
		n += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f "
			L"%f %f "
			L"%f %f c\n",
			cx + rx, -(cy + a * ry) + ty,
			cx + a * rx, -(cy + ry) + ty,
			cx, -(cy + ry) + ty
		);
		n += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f "
			L"%f %f "
			L"%f %f c\n",
			cx - a * rx, -(cy + ry) + ty,
			cx - rx, -(cy + a * ry) + ty,
			cx - rx, -(cy)+ty
		);
		n += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f "
			L"%f %f "
			L"%f %f c\n",
			cx - rx, -(cy - a * ry) + ty,
			cx - a * rx, -(cy - ry) + ty,
			cx, -(cy - ry) + ty
		);
		n += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f "
			L"%f %f "
			L"%f %f c\n",
			cx + a * rx, -(cy - ry) + ty,
			cx + rx, -(cy - a * ry) + ty,
			cx + rx, -(cy)+ty
		);
		n += dt_writer.WriteString(buf);

		if (equal(m_fill_color.a, 0.0f)) {
			n += dt_writer.WriteString(L"S\n");
		}
		else {
			n += dt_writer.WriteString(L"B\n");
		}
		return n;
	}


	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeRect::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) const
	{
		if ((equal(m_stroke_width, 0.0f) ||
			!is_opaque(m_stroke_color)) &&
			!is_opaque(m_fill_color)) {
			return 0;
		}

		size_t n = dt_writer.WriteString(L"% Rectangle\n");
		n += export_pdf_stroke(dt_writer);

		wchar_t buf[1024];
		swprintf_s(
			buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		n += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f %f %f re\n",
			m_start.x, -(m_start.y) + page_size.height, m_vec[0].x, -m_vec[0].y
		);
		n += dt_writer.WriteString(buf);

		if (equal(m_fill_color.a, 0.0f)) {
			n += dt_writer.WriteString(L"S\n");
		}
		else {
			n += dt_writer.WriteString(L"B\n");
		}
		return n;
	}

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeRRect::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) const
	{
		if ((equal(m_stroke_width, 0.0f) ||
			!is_opaque(m_stroke_color)) &&
			!is_opaque(m_fill_color)) {
			return 0;
		}

		wchar_t buf[1024];
		size_t n = dt_writer.WriteString(L"% Rounded Rectangle\n");
		n += export_pdf_stroke(dt_writer);

		// 塗りつぶし色
		swprintf_s(buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		n += dt_writer.WriteString(buf);

		const double a = 4.0 * (sqrt(2.0) - 1.0) / 3.0;	// ベジェでだ円を近似する係数
		const float ty = page_size.height;	// D2D 座標を PDF ユーザー空間へ変換するため
		const float rx = (m_vec[0].x >= 0.0f ? m_corner_rad.x : -m_corner_rad.x);	// だ円の x 方向の半径
		const float ry = (m_vec[0].y >= 0.0f ? m_corner_rad.y : -m_corner_rad.y);	// だ円の y 方向の半径

		// 上辺の開始位置に移動.
		swprintf_s(buf,
			L"%f %f m\n",
			m_start.x + rx, -(m_start.y) + ty
		);
		n += dt_writer.WriteString(buf);

		// 上辺と右上の角を描く.
		float cx = m_start.x + m_vec[0].x - rx;	// 角丸の中心点 x
		float cy = m_start.y + ry;	// 角丸の中心点 y
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f "
			L"%f %f "
			L"%f %f c\n",
			cx, -(m_start.y) + ty,
			cx + a * rx, -(cy - ry) + ty,
			cx + rx, -(cy - a * ry) + ty,
			cx + rx, -(cy)+ty
		);
		n += dt_writer.WriteString(buf);

		// 右辺と右下の角を描く.
		cx = m_start.x + m_vec[0].x - rx;
		cy = m_start.y + m_vec[0].y - ry;
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f "
			L"%f %f "
			L"%f %f c\n",
			m_start.x + m_vec[0].x, -(cy)+ty,
			cx + rx, -(cy + a * ry) + ty,
			cx + a * rx, -(cy + ry) + ty,
			cx, -(cy + ry) + ty
		);
		n += dt_writer.WriteString(buf);

		//　下辺と左下の角を描く.
		cx = m_start.x + rx;
		cy = m_start.y + m_vec[0].y - ry;
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f "
			L"%f %f "
			L"%f %f c\n",
			cx, -(m_start.y + m_vec[0].y) + ty,
			cx - a * rx, -(cy + ry) + ty,
			cx - rx, -(cy + a * ry) + ty,
			cx - rx, -(cy)+ty
		);
		n += dt_writer.WriteString(buf);

		// 左辺と左上の角を描く.
		cx = m_start.x + rx;
		cy = m_start.y + ry;
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f "
			L"%f %f "
			L"%f %f c\n",
			m_start.x, -(cy)+ty,
			cx - rx, -(cy - a * ry) + ty,
			cx - a * rx, -(cy - ry) + ty,
			cx, -(cy - ry) + ty
		);
		n += dt_writer.WriteString(buf);

		if (equal(m_fill_color.a, 0.0f)) {
			n += dt_writer.WriteString(L"S\n");
		}
		else {
			n += dt_writer.WriteString(L"B\n");
		}
		return n;
	}

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeStroke::export_pdf_stroke(DataWriter const& dt_writer) const
	{
		size_t n = 0;
		wchar_t buf[1024];

		// 線枠の太さ
		swprintf_s(buf, L"%f w\n", m_stroke_width);
		n += dt_writer.WriteString(buf);

		// 線枠の色
		swprintf_s(buf, L"%f %f %f RG\n", m_stroke_color.r, m_stroke_color.g, m_stroke_color.b);	// RG は線枠 (rg は塗りつぶし) 色
		n += dt_writer.WriteString(buf);

		// 線枠の端点
		if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE_ROUND })) {
			n += dt_writer.WriteString(L"2 J\n");
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND })) {
			n += dt_writer.WriteString(L"1 J\n");
		}
		else {
			n += dt_writer.WriteString(L"0 J\n");
		}

		// 線の結合の形式
		// 面取り
		if (equal(m_join_style, D2D1_LINE_JOIN_BEVEL)) {
			n += dt_writer.WriteString(L"2 j\n");
		}
		// 丸い
		else if (equal(m_join_style, D2D1_LINE_JOIN_ROUND)) {
			n += dt_writer.WriteString(L"1 j\n");
		}
		// PDF にはマイターあるいは面取りしかない.
		else {
			//if (equal(m_join_style, D2D1_LINE_JOIN_MITER) ||
			//equal(m_join_style, D2D1_LINE_JOIN_MITER_OR_BEVEL)) {
			n += dt_writer.WriteString(L"0 j\n");
			// マイター制限
			swprintf_s(buf, L"%f M\n", m_join_miter_limit);
			n += dt_writer.WriteString(buf);
		}

		// 破線の形式
		if (m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH) {
			// 最後の数値は配置 (破線パターン) を適用するオフセット.
			// [] 0		| 実線
			// [3] 0	| ***___ ***___
			// [2] 1	| *__**__**
			// [2 1] 0	| **_**_ **_
			// [3 5] 6	| __ ***_____***_____
			// [2 3] 11	| *___ **___ **___
			swprintf_s(buf, L"[ %f %f ] 0 d\n", m_dash_patt.m_[0], m_dash_patt.m_[1]);
			n += dt_writer.WriteString(buf);
		}
		else if (m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT) {
			swprintf_s(buf, L"[ %f %f %f %f ] 0 d\n", m_dash_patt.m_[0], m_dash_patt.m_[1], m_dash_patt.m_[2], m_dash_patt.m_[3]);
			n += dt_writer.WriteString(buf);
		}
		else if (m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
			swprintf_s(buf, L"[ %f %f %f %f %f %f ] 0 d\n", m_dash_patt.m_[0], m_dash_patt.m_[1], m_dash_patt.m_[2], m_dash_patt.m_[3], m_dash_patt.m_[4], m_dash_patt.m_[5]);
			n += dt_writer.WriteString(buf);
		}
		else {
			// 実線
			n += dt_writer.WriteString(L"[ ] 0 d\n");
		}
		return n;
	}

	// 図形をデータライターに PDF として書き込む.
	size_t ShapeImage::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) const
	{
		// PDF では表示の大きさの規定値は 1 x 1.
		// そもままでは, 画像全体が 1 x 1 にマッピングされる.
		// 表示するには, 変換行列に表示する大きさを指定し, 拡大する.
		// 表示する位置は, 左上でなく左下隅を指定する.
		wchar_t buf[1024];
		swprintf_s(buf,
			L"%% Image\n"
			L"q\n"
			L"%f 0 0 %f %f %f cm\n"
			L"/I%d Do\n"
			L"Q\n",
			m_view.width, m_view.height,
			m_start.x, -(m_start.y + m_view.height) + page_size.height,
			m_pdf_obj
		);
		return dt_writer.WriteString(buf);
	}

	/*
	bool ShapeText::get_font_face(IDWriteFontFace3*& face) const
	{
		const auto family = m_font_family;
		const auto weight = m_font_weight;
		const auto stretch = m_font_stretch;
		const auto style = m_font_style;
		bool ret = false;

		// 文字列を書き込む.
		IDWriteFontCollection* coll = nullptr;
		if (m_dwrite_text_layout->GetFontCollection(&coll) == S_OK) {
			// 図形と一致する書体ファミリを得る.
			IDWriteFontFamily* fam = nullptr;
			UINT32 index;
			BOOL exists;
			if (coll->FindFamilyName(family, &index, &exists) == S_OK &&
				exists &&
				coll->GetFontFamily(index, &fam) == S_OK) {
				// 書体ファミリから, 太さと幅, 字体が一致する書体を得る.
				IDWriteFont* font = nullptr;
				if (fam->GetFirstMatchingFont(weight, stretch, style, &font) == S_OK) {
					IDWriteFontFaceReference* ref = nullptr;
					if (static_cast<IDWriteFont3*>(font)->GetFontFaceReference(&ref) == S_OK) {
						face = nullptr;
						if (ref->CreateFontFace(&face) == S_OK) {
							ret = true;
						}
						ref->Release();
					}
					font->Release();
				}
				fam->Release();
			}
			coll->Release();
		}
		return true;
	}
	*/

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

	static void export_subtable(const void* table_data, const size_t offset)
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

	//------------------------------
	// 図形をデータライターに PDF として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeText::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) const
	{
		size_t len = ShapeRect::export_pdf(page_size, dt_writer);
		len += dt_writer.WriteString(L"% Text\n");

		wchar_t buf[1024];
		// BT テキストオブジェクトの開始
		// フォント名 サイズ Tf
		// x座標 y座標 Td
		// TLという行間を設定する演算子
		swprintf_s(buf,
			L"%f %f %f rg\n"
			L"%f %f %f RG\n"
			L"BT\n"
			L"/F%d %f Tf\n"
			L"0 Tr\n"
			L"%f %f Td\n",
			m_font_color.r, m_font_color.g, m_font_color.b,
			m_font_color.r, m_font_color.g, m_font_color.b,
			m_pdf_font_num, m_font_size,
			m_start.x + m_text_padding.width,
			-(m_start.y + m_text_padding.height + m_dwrite_line_metrics[0].baseline) + page_size.height
		);
		len += dt_writer.WriteString(buf);

		IDWriteFontFace3* face;
		get_font_face(face);

		//https://learn.microsoft.com/en-us/typography/opentype/spec/cmap
//https://github.com/wine-mirror/wine/blob/master/dlls/dwrite/tests/font.c
		struct CMAP {
			uint32_t startCharCode;
			uint32_t endCharCode;
			uint32_t startGlyphID;
		};

		const void* table_data;
		UINT32 table_size;
		void* table_context;
		BOOL exists;
		face->TryGetFontTable(DWRITE_MAKE_OPENTYPE_TAG('c', 'm', 'a', 'p'), &table_data, &table_size, &table_context, &exists);
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
		face->ReleaseFontTable(table_context);

		std::vector<uint8_t> mb_text{};	// マルチバイト文字列
		for (uint32_t i = 0; i < m_dwrite_test_cnt; i++) {
			const wchar_t* t = m_text + m_dwrite_test_metrics[i].textPosition;	// 行の先頭文字を指すポインター
			const uint32_t t_len = m_dwrite_test_metrics[i].length;	// 行の文字数
			const float td_x = (i > 0 ? m_dwrite_test_metrics[i].left - m_dwrite_test_metrics[i - 1].left : m_dwrite_test_metrics[i].left);	// 行の x 方向のオフセット
			const float td_y = (i > 0 ? m_dwrite_test_metrics[i].top - m_dwrite_test_metrics[i - 1].top : m_dwrite_test_metrics[i].top);	// 行の y 方向のオフセット
			swprintf_s(buf,
				L"%f %f Td\n",
				td_x, -td_y);
			len += dt_writer.WriteString(buf);

			// 文字列を書き込む.

			// wchar_t を GID に変換して書き出す.
			const auto utf32{ conv_utf16_to_utf32(t, t_len) };
			const auto u_len = std::size(utf32);
			std::vector<uint16_t> gid(u_len);
			face->GetGlyphIndices(std::data(utf32), static_cast<UINT32>(u_len), std::data(gid));
			len += dt_writer.WriteString(L"<");
			for (uint32_t j = 0; j < u_len; j++) {
				if (gid[j] != 0) {
					swprintf_s(buf, L"%04x", gid[j]);
					len += dt_writer.WriteString(buf);
				}
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
			if (mb_text.size() < mb_len) {
				mb_text.resize(mb_len);
			}
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
		len += dt_writer.WriteString(L"ET\n");
		return len;
	}

	size_t ShapeRuler::export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer) const
	{
		wchar_t buf[1024];
		size_t len = 0;
		/*
		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}
		if (m_dwrite_text_format == nullptr) {
			wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
			GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);
			const float font_size = min(m_font_size, m_grid_base + 1.0f);
			winrt::check_hresult(
				dwrite_factory->CreateTextFormat(
					m_font_family,
					static_cast<IDWriteFontCollection*>(nullptr),
					DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL,
					DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL,
					DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL,
					font_size,
					locale_name,
					m_dwrite_text_format.put()
				)
			);
			m_dwrite_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
		}
		*/
		constexpr wchar_t D[10] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9' };
		IDWriteFontFace3* face;
		get_font_face(face);
		std::vector utf32{ conv_utf16_to_utf32(D, 10) };
		uint16_t gid[10];
		face->GetGlyphIndices(std::data(utf32), 10, gid);
		DWRITE_FONT_METRICS f_met;
		face->GetMetrics(&f_met);
		face->Release();

		const D2D1_RECT_F rect{
			m_start.x,
			m_start.y,
			m_start.x + m_vec[0].x,
			m_start.y + m_vec[0].y
		};
		if (is_opaque(m_fill_color)) {
			// 塗りつぶし色が不透明な場合,
			// 方形を塗りつぶす.
			swprintf_s(
				buf,
				L"%f %f %f rg\n",
				m_fill_color.r, m_fill_color.g, m_fill_color.b
			);
			len += dt_writer.WriteString(buf);
		}
		if (is_opaque(m_stroke_color)) {

			// 線枠の色が不透明な場合,
			const double g_len = m_grid_base + 1.0;	// 方眼の大きさ
			const double f_size = m_dwrite_text_format->GetFontSize();	// 書体の大きさ
			const bool w_ge_h = fabs(m_vec[0].x) >= fabs(m_vec[0].y);	// 高さより幅の方が大きい
			const double vec_x = (w_ge_h ? m_vec[0].x : m_vec[0].y);	// 大きい方の値を x
			const double vec_y = (w_ge_h ? m_vec[0].y : m_vec[0].x);	// 小さい方の値を y
			const double intvl_x = vec_x >= 0.0 ? g_len : -g_len;	// 目盛りの間隔
			const double intvl_y = min(f_size, g_len);	// 目盛りの間隔
			const uint32_t k = static_cast<uint32_t>(floor(vec_x / intvl_x));	// 目盛りの数
			const double x0 = (w_ge_h ? m_start.x : m_start.y);
			const double y0 = static_cast<double>(w_ge_h ? m_start.y : m_start.x) + vec_y;
			const double y1 = y0 - (vec_y >= 0.0 ? intvl_y : -intvl_y);
			const double y1_5 = y0 - 0.625 * (vec_y >= 0.0 ? intvl_y : -intvl_y);
			const double y2 = y1 - (vec_y >= 0.0 ? f_size : -f_size);
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

			len += export_pdf_stroke(dt_writer);
			// 段落のそろえをテキストフォーマットに格納する.
			for (uint32_t i = 0; i <= k; i++) {
				// 方眼の大きさごとに目盛りを表示する.
				const double x = x0 + i * intvl_x;
				D2D1_POINT_2F p0{
					w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y0),
					w_ge_h ? static_cast<FLOAT>(y0) : static_cast<FLOAT>(x)
				};
				const auto y = ((i % 5) == 0 ? y1 : y1_5);
				D2D1_POINT_2F p1{
					w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y),
					w_ge_h ? static_cast<FLOAT>(y) : static_cast<FLOAT>(x)
				};

				wchar_t buf[1024];
				swprintf_s(buf, L"%f %f m\n", p0.x, -p0.y + page_size.height);
				len += dt_writer.WriteString(buf);
				swprintf_s(buf, L"%f %f l\n", p1.x, -p1.y + page_size.height);
				len += dt_writer.WriteString(buf);
				len += dt_writer.WriteString(L"S\n");
			}
			swprintf_s(buf,
				L"%f %f %f rg\n"
				L"%f %f %f RG\n"
				L"BT\n"
				L"/F%d %f Tf\n"
				L"0 Tr\n"
				L"%f %f Td\n",
				m_stroke_color.r, m_stroke_color.g, m_stroke_color.b,
				m_stroke_color.r, m_stroke_color.g, m_stroke_color.b,
				m_pdf_font_num, m_font_size,
				m_start.x,
				-(m_start.y + f_met.ascent) + page_size.height
			);
			len += dt_writer.WriteString(buf);
			for (uint32_t i = 0; i <= k; i++) {
				// 方眼の大きさごとに目盛りを表示する.
				const double x = x0 + i * intvl_x;
				/*
				D2D1_POINT_2F p0{
					w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y0),
					w_ge_h ? static_cast<FLOAT>(y0) : static_cast<FLOAT>(x)
				};
				const auto y = ((i % 5) == 0 ? y1 : y1_5);
				D2D1_POINT_2F p1{
					w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y),
					w_ge_h ? static_cast<FLOAT>(y) : static_cast<FLOAT>(x)
				};
				*/

				// 目盛りの値を表示する.
				const double x1 = x + f_size * 0.5;
				const double x2 = x1 - f_size;
				D2D1_RECT_F t_rect{
					w_ge_h ? static_cast<FLOAT>(x2) : static_cast<FLOAT>(y2),
					w_ge_h ? static_cast<FLOAT>(y2) : static_cast<FLOAT>(x2),
					w_ge_h ? static_cast<FLOAT>(x1) : static_cast<FLOAT>(y1),
					w_ge_h ? static_cast<FLOAT>(y1) : static_cast<FLOAT>(x1)
				};

				swprintf_s(buf,
					L"%f %f Td <%04x> Tj\n",
					t_rect.left, -t_rect.top + page_size.height,
					gid[i % 10]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"ET\n");
		}
		return len;
	}

}