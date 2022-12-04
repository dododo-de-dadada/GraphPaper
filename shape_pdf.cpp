//------------------------------
// shape_pdf.cpp
// PDF への書き込み.
//------------------------------

// PDF フォーマット
// https://aznote.jakou.com/prog/pdf/index.html

#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//------------------------------
	// データライターに PDF ストリームの一部として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeBezi::pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const
	{
		if (equal(m_stroke_color.a, 0.0f)) {
			return 0;
		}

		size_t n = dt_write("% Bezi\n", dt_writer);
		n += pdf_write_stroke(dt_writer);

		D2D1_BEZIER_SEGMENT b_seg;
		pt_add(m_pos, m_vec[0], b_seg.point1);
		pt_add(b_seg.point1, m_vec[1], b_seg.point2);
		pt_add(b_seg.point2, m_vec[2], b_seg.point3);

		char buf[1024];
		sprintf_s(buf, "%f %f m\n", m_pos.x, -m_pos.y + sheet.m_sheet_size.height);
		n += dt_write(buf, dt_writer);
		sprintf_s(buf, "%f %f ", b_seg.point1.x, -b_seg.point1.y + sheet.m_sheet_size.height);
		n += dt_write(buf, dt_writer);
		sprintf_s(buf, "%f %f ", b_seg.point2.x, -b_seg.point2.y + sheet.m_sheet_size.height);
		n += dt_write(buf, dt_writer);
		sprintf_s(buf, "%f %f c\n", b_seg.point3.x, -b_seg.point3.y + sheet.m_sheet_size.height);
		n += dt_write(buf, dt_writer);
		n += dt_write("S\n", dt_writer);
		if (m_arrow_style == ARROW_STYLE::OPENED || m_arrow_style == ARROW_STYLE::FILLED) {
			D2D1_POINT_2F barbs[3];
			bezi_calc_arrow(m_pos, b_seg, m_arrow_size, barbs);
			n += pdf_write_barbs(sheet, barbs, barbs[2], dt_writer);
		}
		return n;
	}

	//------------------------------
	// データライターに PDF ストリームの一部として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeLine::pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const
	{
		if (equal(m_stroke_color.a, 0.0f)) {
			return 0;
		}
		size_t n = dt_write("% Line\n", dt_writer);
		n += pdf_write_stroke(dt_writer);

		char buf[1024];
		sprintf_s(buf, "%f %f m\n", m_pos.x, -m_pos.y + sheet.m_sheet_size.height);
		n += dt_write(buf, dt_writer);
		sprintf_s(buf, "%f %f l\n", m_pos.x + m_vec[0].x, -(m_pos.y + m_vec[0].y) + sheet.m_sheet_size.height);
		n += dt_write(buf, dt_writer);
		n += dt_write("S\n", dt_writer);
		if (m_arrow_style == ARROW_STYLE::OPENED || m_arrow_style == ARROW_STYLE::FILLED) {
			D2D1_POINT_2F barbs[3];
			if (line_get_arrow_pos(m_pos, m_vec[0], m_arrow_size, barbs, barbs[2])) {
				n += pdf_write_barbs(sheet, barbs, barbs[2], dt_writer);
			}
		}
		return n;
	}

	//------------------------------
	// 矢じるしをデータライターに PDF ストリームの一部として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeLine::pdf_write_barbs(const ShapeSheet& sheet, const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const
	{
		char buf[1024];
		size_t n = 0;

		// 破線ならば, 実線に戻す.
		if (m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH ||
			m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT ||
			m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
			n += dt_write("[ ] 0 d\n", dt_writer);
		}
		if (m_arrow_style == ARROW_STYLE::FILLED) {
			sprintf_s(buf, 
				"%f %f %f rg\n", 
				m_stroke_color.r, m_stroke_color.g, m_stroke_color.b);
			n += dt_write(buf, dt_writer);
		}
		sprintf_s(buf, "%f %f m\n", barbs[0].x, -barbs[0].y + sheet.m_sheet_size.height);
		n += dt_write(buf, dt_writer);
		sprintf_s(buf, "%f %f l\n", tip_pos.x, -tip_pos.y + sheet.m_sheet_size.height);
		n += dt_write(buf, dt_writer);
		sprintf_s(buf, "%f %f l\n", barbs[1].x, -barbs[1].y + sheet.m_sheet_size.height);
		n += dt_write(buf, dt_writer);
		if (m_arrow_style == ARROW_STYLE::OPENED) {
			n += dt_write("S\n", dt_writer);
		}
		else if (m_arrow_style == ARROW_STYLE::FILLED) {
			n += dt_write("b\n", dt_writer);	// b はパスを閉じて (B は閉じずに) 塗りつぶす.
		}
		return n;
	}

	//------------------------------
	// データライターに PDF ストリームの一部として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapePoly::pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const
	{
		size_t n = dt_write("% Poly\n", dt_writer);
		n += pdf_write_stroke(dt_writer);

		char buf[1024];
		sprintf_s(
			buf,
			"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		n += dt_write(buf, dt_writer);

		const size_t v_cnt = m_vec.size() + 1;
		D2D1_POINT_2F v_pos[MAX_N_GON];
		v_pos[0] = m_pos;
		sprintf_s(buf, "%f %f m\n", v_pos[0].x, -v_pos[0].y + sheet.m_sheet_size.height);
		n += dt_write(buf, dt_writer);
		for (size_t i = 1; i < v_cnt; i++) {
			pt_add(v_pos[i - 1], m_vec[i - 1], v_pos[i]);
			sprintf_s(buf, "%f %f l\n", v_pos[i].x, -v_pos[i].y + sheet.m_sheet_size.height);
			n += dt_write(buf, dt_writer);
		}
		if (m_end_closed) {
			if (equal(m_fill_color.a, 0.0f)) {
				// s はパスを閉じて描画する.
				n += dt_write("s\n", dt_writer);
			}
			else {
				// b はパスを閉じて塗りつぶし, ストロークも描画する.
				n += dt_write("b\n", dt_writer);
			}
		}
		else {
			if (equal(m_fill_color.a, 0.0f)) {
				// S はパスを閉じずに描画する.
				n += dt_write("S\n", dt_writer);
			}
			else {
				// B はパスを閉じずに塗りつぶし, ストロークも描画する.
				n += dt_write("B\n", dt_writer);
			}
		}
		if (m_arrow_style == ARROW_STYLE::OPENED ||
			m_arrow_style == ARROW_STYLE::FILLED) {
			D2D1_POINT_2F h_tip;
			D2D1_POINT_2F h_barbs[2];
			if (poly_get_arrow_barbs(v_cnt, v_pos, m_arrow_size, h_tip, h_barbs)) {
				n += pdf_write_barbs(sheet, h_barbs, h_tip, dt_writer);
			}
		}
		return n;
	}

	//------------------------------
	// データライターに PDF ストリームの一部として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeRect::pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const
	{
		size_t n = dt_write("% Rect\n", dt_writer);
		n += pdf_write_stroke(dt_writer);

		char buf[1024];
		sprintf_s(
			buf,
			"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		n += dt_write(buf, dt_writer);

		if (typeid(*this) == typeid(ShapeElli)) {
			const float ty = sheet.m_sheet_size.height;
			const double a = 4.0 * (sqrt(2.0) - 1.0) / 3.0;
			D2D1_POINT_2F r;
			pt_mul(m_vec[0], 0.5, r);
			D2D1_POINT_2F c;
			pt_add(m_pos, r, c);
			sprintf_s(buf,
				"%f %f m\n"
				"%f %f "
				"%f %f "
				"%f %f c\n",
				c.x + r.x, -(c.y) + ty,
				c.x + r.x, -(c.y + a * r.y) + ty,
				c.x + a * r.x, -(c.y + r.y) + ty,
				c.x, -(c.y + r.y) + ty
			);
			n += dt_write(buf, dt_writer);
			sprintf_s(buf,
				"%f %f "
				"%f %f "
				"%f %f c\n",
				c.x - a * r.x, -(c.y + r.y) + ty,
				c.x - r.x, -(c.y + a * r.y) + ty,
				c.x - r.x, -(c.y) + ty
			);
			n += dt_write(buf, dt_writer);
			sprintf_s(buf,
				"%f %f "
				"%f %f "
				"%f %f c\n",
				c.x - r.x, -(c.y - a * r.y) + ty,
				c.x - a * r.x, -(c.y - r.y) + ty,
				c.x, -(c.y - r.y) + ty
			);
			n += dt_write(buf, dt_writer);
			sprintf_s(buf,
				"%f %f "
				"%f %f "
				"%f %f c\n",
				c.x + a * r.x, -(c.y - r.y) + ty,
				c.x + r.x, -(c.y - a * r.y) + ty,
				c.x + r.x, -(c.y) + ty
			);
			n += dt_write(buf, dt_writer);
		}
		else if (typeid(*this) == typeid(ShapeRRect)) {

		}
		else {
			sprintf_s(buf,
				"%f %f %f %f re\n",
				m_pos.x, -m_pos.y + sheet.m_sheet_size.height, m_vec[0].x, -m_vec[0].y
			);
			n += dt_write(buf, dt_writer);
		}
		if (equal(m_fill_color.a, 0.0f)) {
			n += dt_write("S\n", dt_writer);
		}
		else {
			n += dt_write("B\n", dt_writer);
		}
		return 0;
	}

	//------------------------------
	// データライターに PDF ストリームの一部として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeStroke::pdf_write_stroke(DataWriter const& dt_writer) const
	{
		size_t n = 0;
		char buf[1024];

		// 線枠の太さ
		sprintf_s(buf, "%f w\n", m_stroke_width);
		n += dt_write(buf, dt_writer);

		// 線枠の色
		sprintf_s(buf, "%f %f %f RG\n", m_stroke_color.r, m_stroke_color.g, m_stroke_color.b);	// RG は線枠 (rg は塗りつぶし) 色
		n += dt_write(buf, dt_writer);

		// 線枠の端点
		if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE_ROUND })) {
			n += dt_write("2 J\n", dt_writer);
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND })) {
			n += dt_write("1 J\n", dt_writer);
		}
		else {
			n += dt_write("0 J\n", dt_writer);
		}

		// 線枠の結合
		if (equal(m_join_style, D2D1_LINE_JOIN_BEVEL)) {
			n += dt_write("2 j\n", dt_writer);
		}
		else if (equal(m_join_style, D2D1_LINE_JOIN_ROUND)) {
			n += dt_write("1 j\n", dt_writer);
		}
		else {
			//if (equal(m_join_style, D2D1_LINE_JOIN_MITER) ||
			//equal(m_join_style, D2D1_LINE_JOIN_MITER_OR_BEVEL)) {
			n += dt_write("0 j\n", dt_writer);
		}

		// マイター制限
		sprintf_s(buf,
			"%f M\n",
			m_join_miter_limit);
		n += dt_write(buf, dt_writer);

		// 破線の種類
		if (m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH) {
			// 最後の数値は配置 (破線パターン) を適用するオフセット.
			// [] 0		| 実線
			// [3] 0	| ***___ ***___
			// [2] 1	| *__**__**
			// [2 1] 0	| **_**_ **_
			// [3 5] 6	| __ ***_____***_____
			// [2 3] 11	| *___ **___ **___
			sprintf_s(buf, "[ %f %f ] 0 d\n", m_dash_patt.m_[0], m_dash_patt.m_[1]);
			n += dt_write(buf, dt_writer);
		}
		else if (m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT) {
			sprintf_s(buf, "[ %f %f %f %f ] 0 d\n", m_dash_patt.m_[0], m_dash_patt.m_[1], m_dash_patt.m_[2], m_dash_patt.m_[3]);
			n += dt_write(buf, dt_writer);
		}
		else if (m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
			sprintf_s(buf, "[ %f %f %f %f %f %f ] 0 d\n", m_dash_patt.m_[0], m_dash_patt.m_[1], m_dash_patt.m_[2], m_dash_patt.m_[3], m_dash_patt.m_[4], m_dash_patt.m_[5]);
			n += dt_write(buf, dt_writer);
		}
		else {
			n += dt_write("[ ] 0 d\n", dt_writer);
		}
		return n;
	}

	//------------------------------
	// データライターに PDF ストリームの一部として書き込む.
	// dt_weiter	データライター
	// 戻り値	書き込んだバイト数
	//------------------------------
	size_t ShapeText::pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const
	{
		size_t len = dt_write("% Text\n", dt_writer);
		len += ShapeRect::pdf_write(sheet, dt_writer);
		/*
		winrt::com_ptr<IWICImagingFactory2> wic_factory;
		winrt::check_hresult(
			CoCreateInstance(
				CLSID_WICImagingFactory,
				nullptr,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&wic_factory)
			)
		);

		UINT w = 0;
		UINT h = 0;
		for (int i = 0; i < m_dw_test_cnt; i++) {
			w = max(w, m_dw_test_metrics[i].width);
			h += m_dw_test_metrics[i].height;
		}

		std::vector<uint8_t> vec(4 * w * h);
		winrt::com_ptr<IWICBitmap> wic_bitmap;
		wic_factory->CreateBitmapFromMemory(w, h, GUID_WICPixelFormat32bppBGRA, 4 * w, 4 * w * h, vec.data(), wic_bitmap.put());
		D2D1_RENDER_TARGET_PROPERTIES prop{
			D2D1_RENDER_TARGET_TYPE::D2D1_RENDER_TARGET_TYPE_SOFTWARE,
			D2D1_PIXEL_FORMAT{
				DXGI_FORMAT_B8G8R8A8_UNORM,
				D2D1_ALPHA_MODE_STRAIGHT
				},
			96.0f,
			96.0f,
			D2D1_RENDER_TARGET_USAGE_FORCE_BITMAP_REMOTING,
			D2D1_FEATURE_LEVEL_DEFAULT
		};
		winrt::com_ptr<ID2D1RenderTarget> target;
		s_d2d_factory->CreateWicBitmapRenderTarget(wic_bitmap.get(), prop, target.put());
		winrt::com_ptr<ID2D1SolidColorBrush> brush;
		target->CreateSolidColorBrush(m_fill_color, brush.put());
		target->BeginDraw();
		target->DrawTextLayout(D2D1_POINT_2F{ m_text_padding.width, m_text_padding.height },
			m_dw_text_layout.get(), brush.get());
		target->EndDraw();
		target->Flush();
		char buf[1024];
		sprintf_s(
			buf,
			"<<\n"
			"/Length /%zu\n"
			">>\n"
			"",
			4 * w * h
		);
		dt_write(buf, dt_writer);
		dt_writer.WriteBytes(vec);
		vec.clear();
		*/
		//ShapeText::pdf_write(sheet, dt_writer);
		char buf[1024];
		// BT テキストオブジェクトの開始
		// フォント名 サイズ Tf
		// x座標 y座標 Td
		// TLという行間を設定する演算子
		D2D1_POINT_2F nw_pos;	// 左上の位置
		pt_add(m_pos, m_text_padding.width, m_text_padding.height, nw_pos);
		sprintf_s(buf,
			"%f %f %f rg\n"
			"%f %f %f RG\n"
			"BT\n"
			"/F%d %f Tf\n"
			"0 Tr\n"
			"%f %f Td\n",
			m_font_color.r, m_font_color.g, m_font_color.b,
			m_font_color.r, m_font_color.g, m_font_color.b,
			m_pdf_font, m_font_size,
			nw_pos.x, -nw_pos.y + sheet.m_sheet_size.height
		);
		len += dt_write(buf, dt_writer);

		std::vector<uint8_t> sjis{};
		for (uint32_t i = 0; i < m_dw_test_cnt; i++) {

			const DWRITE_HIT_TEST_METRICS& tm = m_dw_test_metrics[i];
			const wchar_t* t = m_text + tm.textPosition;
			const uint32_t t_len = tm.length;

			sprintf_s(buf, "%f %f Td\n", tm.left, -tm.top);
			len += dt_write(buf, dt_writer);

			// 文字列を表示する垂直なずらし位置を求める.
			//const double dy = static_cast<double>(m_dw_line_metrics[i].baseline);


			// 文字列を書き込む.
			//dt_writer.WriteByte(L'<'); len++;
			//for (int i = 0; i < t_len; i++) {
			//	sprintf_s(buf, "%04x", t[i]);
			//	len += dt_write(buf, dt_writer);
			//}
			//len += dt_write("> Tj\n", dt_writer);

			size_t sjis_len = WideCharToMultiByte(CP_ACP, 0, t, t_len, NULL, 0, NULL, NULL);
			if (sjis.size() < sjis_len) {
				sjis.resize(sjis_len);
			}
			WideCharToMultiByte(CP_ACP, 0, t, t_len, (LPSTR)sjis.data(), static_cast<int>(sjis_len), NULL, NULL);

			len += dt_write(
				"<",
				dt_writer);
			for (int j = 0; j < sjis_len; j++) {
				constexpr char* X = "0123456789abcdef";
				dt_writer.WriteByte(X[sjis[j] >> 4]);
				dt_writer.WriteByte(X[sjis[j] & 15]);
			}
			len += 2 * sjis_len;
			len += dt_write(
				"> Tj\n",
				dt_writer);
		}
		len += dt_write("ET\n", dt_writer);
		return len;
	}

}