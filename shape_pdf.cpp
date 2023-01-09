//------------------------------
// shape_pdf.cpp
// PDF への書き込み.
//------------------------------

// PDF フォーマット
// https://aznote.jakou.com/prog/pdf/index.html

#include "pch.h"
#include "shape.h"
#include "CMap.h"

using namespace winrt;
using namespace winrt::CMap::implementation;

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
		D2D1_POINT_2F v_pos[MAX_N_GON];
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
		// PDF では表示の大きさの規定値は 1 × 1.
		// そもままでは, 画像全体が 1 × 1 にマッピングされる.
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

	static bool get_font_face(const ShapeText* s, IDWriteFontFace3** const face)
	{
		const auto family = s->m_font_family;
		const auto weight = s->m_font_weight;
		const auto stretch = s->m_font_stretch;
		const auto style = s->m_font_style;
		bool ret = false;

		// 文字列を書き込む.
		IDWriteFontCollection* coll = nullptr;
		if (s->m_dw_text_layout->GetFontCollection(&coll) == S_OK) {
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
						*face = nullptr;
						if (ref->CreateFontFace(face) == S_OK) {
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
			-(m_start.y + m_text_padding.height + m_dw_line_metrics[0].baseline) + page_size.height
		);
		len += dt_writer.WriteString(buf);

		IDWriteFontFace3* face;
		get_font_face(static_cast<const ShapeText*>(this), &face);
		std::vector<uint8_t> mb_text{};	// マルチバイト文字列
		for (uint32_t i = 0; i < m_dw_test_cnt; i++) {
			const wchar_t* t = m_text + m_dw_test_metrics[i].textPosition;	// 行の先頭文字を指すポインター
			const uint32_t t_len = m_dw_test_metrics[i].length;	// 行の文字数
			const float td_x = (i > 0 ? m_dw_test_metrics[i].left - m_dw_test_metrics[i - 1].left : m_dw_test_metrics[i].left);	// 行の x 方向のオフセット
			const float td_y = (i > 0 ? m_dw_test_metrics[i].top - m_dw_test_metrics[i - 1].top : m_dw_test_metrics[i].top);	// 行の y 方向のオフセット
			swprintf_s(buf,
				L"%f %f Td\n",
				td_x, -td_y);
			len += dt_writer.WriteString(buf);

			//
//https://github.com/wine-mirror/wine/blob/master/dlls/dwrite/tests/font.c
			const void* table_data;
			UINT32 table_size;
			void* table_context;
			BOOL exists;
			face->TryGetFontTable(DWRITE_MAKE_OPENTYPE_TAG('c', 'm', 'a', 'p'), &table_data, &table_size, &table_context, &exists);
			struct EncodingRecord {
				uint16_t platformID;
				uint16_t encodingID;
				uint32_t offset;
			};
			uint16_t version = get_uint16(table_data, 0);
			uint16_t numTables = get_uint16(table_data, 2);
			for (uint16_t i = 0; i < numTables; i++) {
				uint16_t platformID = get_uint16(table_data, 4ull + 8ull * i + 0);
				uint16_t encodingID = get_uint16(table_data, 4ull + 8ull * i + 2);
				uint32_t offset = get_uint32(table_data, 4ull + 8ull * i + 4);
			}
			face->ReleaseFontTable(table_context);

			// 文字列を書き込む.
			// GID
			const auto utf32 = cmap_utf16_to_utf32(t, t_len);
			const auto u_len = std::size(utf32);
			std::vector<uint16_t> gid(u_len);
			face->GetGlyphIndices(std::data(utf32), u_len, std::data(gid));
			len += dt_writer.WriteString(L"<");
			for (uint32_t j = 0; j < u_len; j++) {
				swprintf_s(buf, L"%04x", gid[j]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"> Tj\n");
			/*
			// wchar_t を UTF-32 に変換して書き出す.
			len += dt_writer.WriteString(L"% UTF-32\n<");
			std::vector<uint32_t> utf32{ cmap_utf16_to_utf32(t, t_len) };
			for (int i = 0; i < utf32.size(); i++) {
				swprintf_s(buf, L"%06x", utf32[i]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"> Tj\n");
			*/
			/*
			// wchar_t を CID に変換して書き出す.
			len += dt_writer.WriteString(L"% CID\n<");
			std::vector<uint32_t> utf32{ cmap_utf16_to_utf32(t, t_len) };
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

}