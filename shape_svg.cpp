//------------------------------
// shape_svg.cpp
// SVG として書き込む.
//------------------------------
#include "pch.h"
#include <shcore.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;
	using winrt::Windows::Storage::Streams::IRandomAccessStream;
	using winrt::Windows::Graphics::Imaging::BitmapEncoder;
	using winrt::Windows::Security::Cryptography::CryptographicBuffer;
	using winrt::Windows::Storage::Streams::Buffer;
	using winrt::Windows::Storage::Streams::InputStreamOptions;

	static void export_svg_barbs(wchar_t* buf, const size_t len, const ARROW_STYLE arrow, const float width, const D2D1_COLOR_F& color, const CAP_STYLE& cap, const D2D1_LINE_JOIN join, const float miter_limit, const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos);
	static void export_svg_color(wchar_t* buf, const size_t len, const D2D1_COLOR_F color, const wchar_t* name);
	static void export_svg_stroke(wchar_t* buf, const size_t len, const float width, const D2D1_COLOR_F& color, const D2D1_DASH_STYLE dash, const DASH_PATT& patt, const CAP_STYLE cap, const D2D1_LINE_JOIN join, const float limit);

	//------------------------------
	// バッファに矢じりを SVG タグとして書き込む.
	// buf	出力先
	// len	出力先の長さ
	// arrow	矢じるしの形式
	// width	線・枠の太さ	
	// color	線・枠の色
	// cap	線分の端点の形式
	// join	線分の連結の形式
	// miter_limit	マイター制限
	// barbs[]	矢じりの返しの位置
	// tip_pos	矢じりの先端の位置
	//------------------------------
	static void export_svg_barbs(
		wchar_t* buf,	// 出力先
		const size_t len,	// 出力先の長さ
		const ARROW_STYLE arrow,	// 矢じるしの形式
		const float width,	// 線・枠の太さ	
		const D2D1_COLOR_F& color,	// 線・枠の色
		const CAP_STYLE& cap,	// 線分の端点の形式
		const D2D1_LINE_JOIN join,	// 線分の連結の形式
		const float miter_limit,	// マイター制限
		const D2D1_POINT_2F barbs[],	// 矢じりの両端の位置
		const D2D1_POINT_2F tip_pos)	// 矢じりの先端の位置
	{
		swprintf_s(buf, len,
			L"<path d=\"M%f %f L%f %f L%f %f\" ",
			barbs[0].x, barbs[0].y,
			tip_pos.x, tip_pos.y,
			barbs[1].x, barbs[1].y
		);

		const auto len1 = wcslen(buf);
		if (arrow == ARROW_STYLE::FILLED) {
			export_svg_color(buf + len1, len - len1, color, L"fill");
		}
		else {
			wcscpy_s(buf + len1, len - len1, L"fill=\"none\" ");
		}

		const auto len2 = wcslen(buf);
		export_svg_stroke(buf + len2, len - len2, width, color, D2D1_DASH_STYLE_SOLID, DASH_PATT{}, cap, join, miter_limit);

		const auto len3 = wcslen(buf);
		wcscpy_s(buf + len3, len - len3, L"/>\n");
	}

	//------------------------------
	// バッファに色を SVG の属性として出力
	// buf	出力先 
	// len	出力先の長さ
	// color	色
	// name	色の名前
	//------------------------------
	static void export_svg_color(
		wchar_t* buf,	// 出力先 
		const size_t len,	// 出力先の長さ
		const D2D1_COLOR_F color,	// 色
		const wchar_t* name	// 色の名前
	)
	{
		if (!is_opaque(color)) {
			swprintf_s(buf, len, L"%s=\"none\" ", name);
		}
		else {
			const int32_t r = static_cast<int32_t>(std::round(color.r * 255.0));
			const int32_t g = static_cast<int32_t>(std::round(color.g * 255.0));
			const int32_t b = static_cast<int32_t>(std::round(color.b * 255.0));
			swprintf_s(buf,
				len,
				L"%s=\"#%02x%02x%02x\" %s-opacity=\"%f\" ",
				name,
				min(max(r, 0), 255),
				min(max(g, 0), 255),
				min(max(b, 0), 255),
				name,
				color.a
			);
		}
	}

	//------------------------------
	// バッファに SVG タグとして書き込む.
	// buf	出力先
	// len	出力先の長さ
	// width	線・枠の太さ
	// color	線・枠の色
	// dash	破線の形式
	// patt	破線の配置
	// cap	線分の端の形式
	// join	線分の連結の形式
	// limit	マイター制限
	//------------------------------
	static void export_svg_stroke(
		wchar_t* buf, // 出力先
		const size_t len,	// 出力先の長さ
		const float width, 
		const D2D1_COLOR_F& color, 
		const D2D1_DASH_STYLE dash, 
		const DASH_PATT& patt, 
		const CAP_STYLE cap, 
		const D2D1_LINE_JOIN join, 
		const float limit
	)
	{
		if (equal(width, 0.0f) || !is_opaque(color)) {
			wcscpy_s(buf, len, L"stroke=\"none\" ");
		}
		else {
			swprintf_s(buf, len, L"stroke-width=\"%f\" ", width);

			const size_t len1 = wcslen(buf);
			export_svg_color(buf + len1, len - len1, color, L"stroke");

			const size_t len2 = wcslen(buf);
			if (dash == D2D1_DASH_STYLE_DASH) {
				swprintf_s(buf + len2, len - len2,
					L"stroke-dasharray=\"%.0f %.0f\" ", patt.m_[0], patt.m_[1]);
			}
			else if (dash == D2D1_DASH_STYLE_DOT) {
				swprintf_s(buf + len2, len - len2,
					L"stroke-dasharray=\"%.0f %.0f\" ", patt.m_[2], patt.m_[3]);
			}
			else if (dash == D2D1_DASH_STYLE_DASH_DOT) {
				swprintf_s(buf + len2, len - len2,
					L"stroke-dasharray=\"%.0f %.0f %.0f %.0f\" ",
					patt.m_[0], patt.m_[1], patt.m_[2], patt.m_[3]);
			}
			else if (dash == D2D1_DASH_STYLE_DASH_DOT_DOT) {
				swprintf_s(buf + len2, len - len2,
					L"stroke-dasharray=\"%.0f %.0f %.0f %.0f %.0f %.0f\" ",
					patt.m_[0], patt.m_[1], patt.m_[2], patt.m_[3], patt.m_[2], patt.m_[3]);
			}
			else {
				wcscpy_s(buf + len2, len - len2, L"stroke-dasharray=\"none\" ");
			}

			const auto len3 = wcslen(buf);
			if (equal(cap, CAP_FLAT)) {
				wcscpy_s(buf + len3, len - len3, L"stroke-linecap=\"butt\" ");
			}
			else if (equal(cap, CAP_ROUND)) {
				wcscpy_s(buf + len3, len - len3, L"stroke-linecap=\"round\" ");
			}
			else if (equal(cap, CAP_SQUARE)) {
				wcscpy_s(buf + len3, len - len3, L"stroke-linecap=\"square\" ");
			}
			else if (equal(cap, CAP_TRIANGLE)) {
				// SVG に三角はないので, かわりに stroke-linecap="butt"
				wcscpy_s(buf + len3, len - len3, L"stroke-linecap=\"butt\" ");
			}

			const auto len4 = wcslen(buf);
			if (join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				wcscpy_s(buf + len4, len - len4, L"stroke-linejoin=\"bevel\" ");
			}
			else if (join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
				join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				// D2D では, マイターを指定すると, マイター制限は常に無視され, すべてマイターになる.
				// D2D では, マイターまたはベベルを指定すると, マイター制限が有効になり, これを超える角がベベルになる.
				// SVG では, マイターを指定すると, マイター制限は常に有効で, これを超える角はベベルになる.
				// つまり, D2D のマイターまたはベベルは, SVG のマイターと同じ.
				// D2D のマイターは, SVG にはない.
				swprintf_s(buf + len4, len - len4,
					L"stroke-linejoin=\"miter\" stroke-miterlimit=\"%f\" ", limit);
			}
			else if (join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				wcscpy_s(buf + len4, len - len4, L"stroke-linejoin=\"round\" ");
			}
		}
	}

	//------------------------------
	// データライターに SVG として書き込む.
	// dt_reader	データリーダー
	//------------------------------
	void ShapeBezi::export_svg(DataWriter const& dt_writer)
	{
		D2D1_BEZIER_SEGMENT b_seg;
		pt_add(m_start, m_vec[0], b_seg.point1);
		pt_add(b_seg.point1, m_vec[1], b_seg.point2);
		pt_add(b_seg.point2, m_vec[2], b_seg.point3);

		// パスの始点と制御点
		wchar_t buf[1024];
		swprintf_s(buf,
			L"<path d=\"M%f %f C%f %f, %f %f, %f %f\" ",
			m_start.x, m_start.y,
			b_seg.point1.x, b_seg.point1.y,
			b_seg.point2.x, b_seg.point2.y,
			b_seg.point3.x, b_seg.point3.y
		);
		dt_writer.WriteString(buf);

		export_svg_color(buf, 1024, m_fill_color, L"fill");
		dt_writer.WriteString(buf);

		export_svg_stroke(buf, 1024, m_stroke_width, m_stroke_color, m_dash_style, m_dash_patt, m_stroke_cap, m_join_style, m_join_miter_limit);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");

		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[3];
			bezi_calc_arrow(m_start, b_seg, m_arrow_size, barbs);
			export_svg_barbs(buf, 1024, m_arrow_style, m_stroke_width, m_stroke_color, m_stroke_cap, m_join_style, m_join_miter_limit, barbs, barbs[2]);
			dt_writer.WriteString(buf);
		}
	}

	// データライターに SVG タグとして書き込む.
	void ShapeElli::export_svg(DataWriter const& dt_writer)
	{
		D2D1_POINT_2F r;
		pt_mul(m_vec[0], 0.5, r);
		D2D1_POINT_2F c;
		pt_add(m_start, r, c);

		// だ円を出力
		wchar_t buf[1024];
		swprintf_s(buf,
			L"<ellipse cx=\"%f\" cy=\"%f\" rx=\"%f\" ry=\"%f\" ",
			c.x, c.y, r.x, r.y
		);
		dt_writer.WriteString(buf);

		// 塗りつぶしを出力
		export_svg_color(buf, 1024, m_fill_color, L"fill");
		dt_writer.WriteString(buf);

		// 線・枠を出力
		export_svg_stroke(buf, 1024, m_stroke_width, m_stroke_color, m_dash_style, m_dash_patt, m_stroke_cap, m_join_style, m_join_miter_limit);
		dt_writer.WriteString(buf);

		// だ円を閉じる.
		dt_writer.WriteString(L"/>\n");
	}

	//------------------------------
	// データライターに SVG として書き込む.
	//------------------------------
	winrt::Windows::Foundation::IAsyncAction ShapeGroup::export_as_svg_async(const DataWriter& dt_writer)
	{
		dt_writer.WriteString(L"<!-- Group -->\n");
		dt_writer.WriteString(L"<g>\n");
		for (Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			if (typeid(*s) == typeid(ShapeImage)) {
				co_await static_cast<ShapeImage*>(s)->export_as_svg_async(dt_writer);
			}
			else if (typeid(*s) == typeid(ShapeGroup)) {
				co_await static_cast<ShapeGroup*>(s)->export_as_svg_async(dt_writer);
			}
			else {
				s->export_svg(dt_writer);
			}
		}
		dt_writer.WriteString(L"</g>\n");
	}

	// データライターに SVG として書き込む.
	// dt_write		データライター
	winrt::Windows::Foundation::IAsyncAction ShapeImage::export_as_svg_async(const DataWriter& dt_writer)
	{
		// メモリのランダムアクセスストリーム
		InMemoryRandomAccessStream image_stream{};
		co_await copy<true>(BitmapEncoder::PngEncoderId(), image_stream);
		const auto image_len = static_cast<uint32_t>(image_stream.Size());
		Buffer image_buf(image_len);
		co_await image_stream.ReadAsync(/*--->*/image_buf, image_len, InputStreamOptions::None);
		const auto base64{
			CryptographicBuffer::EncodeToBase64String(image_buf)
		};
		wchar_t buf[1024];
		swprintf_s(buf,
			L"<image x=\"%f\" y=\"%f\" "
			L"width=\"%f\" height=\"%f\" "
			L"opacity=\"%f\" "
			L"href=\"data:image/png;base64,",
			m_start.x, m_start.y,
			m_view.width, m_view.height,
			m_opac
		);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(base64);
		dt_writer.WriteString(L"\" />\n");
	}

	// データライターに SVG タグとして書き込む.
	void ShapeLine::export_svg(DataWriter const& dt_writer)
	{
		// 線・枠も無いなら,
		if (equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) {
			return;
		}

		D2D1_POINT_2F e_pos;
		pt_add(m_start, m_vec[0], e_pos);
		wchar_t buf[1024];
		swprintf_s(buf,
			L"<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" ",
			m_start.x, m_start.y, e_pos.x, e_pos.y
		);
		dt_writer.WriteString(buf);

		export_svg_stroke(buf, 1024, m_stroke_width, m_stroke_color, m_dash_style, m_dash_patt, m_stroke_cap, m_join_style, m_join_miter_limit);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[2];
			D2D1_POINT_2F tip_pos;
			if (ShapeLine::line_get_arrow_pos(m_start, m_vec[0], m_arrow_size, barbs, tip_pos)) {
				export_svg_barbs(buf, 1024, m_arrow_style, m_stroke_width, m_stroke_color, m_stroke_cap, m_join_style, m_join_miter_limit, barbs, tip_pos);
				dt_writer.WriteString(buf);
			}
		}
	}

	// データライターに SVG タグとして書き込む.
	void ShapePoly::export_svg(DataWriter const& dt_writer)
	{
		// 線・枠も塗りつぶしも無いなら,
		if ((equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) && !is_opaque(m_fill_color)) {
			return;
		}

		const auto d_cnt = m_vec.size();	// 始点を除くの頂点数
		std::vector<D2D1_POINT_2F> v_pos(d_cnt + 1);
		wchar_t buf[1024];
		swprintf_s(buf,
			L"<path d=\"M%f %f ",
			m_start.x, m_start.y
		);
		dt_writer.WriteString(buf);

		v_pos[0] = m_start;
		for (size_t i = 0; i < d_cnt; i++) {
			swprintf_s(buf,
				L"l%f %f ", m_vec[i].x, m_vec[i].y);
			dt_writer.WriteString(buf);
			pt_add(v_pos[i], m_vec[i], v_pos[i + 1]);
		}
		if (m_end_closed) {
			dt_writer.WriteString(L"Z");
		}
		dt_writer.WriteString(L"\" ");

		export_svg_stroke(buf, 1024, m_stroke_width, m_stroke_color, m_dash_style, m_dash_patt, m_stroke_cap, m_join_style, m_join_miter_limit);
		dt_writer.WriteString(buf);

		export_svg_color(buf, 1024, m_fill_color, L"fill");
		dt_writer.WriteString(buf);

		dt_writer.WriteString(L"/>\n");

		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F h_tip;
			D2D1_POINT_2F h_barbs[2];
			if (ShapePoly::poly_get_arrow_barbs(d_cnt + 1, std::data(v_pos), m_arrow_size, h_tip, h_barbs)) {
				export_svg_barbs(buf, 1024, m_arrow_style, m_stroke_width, m_stroke_color, m_stroke_cap, m_join_style, m_join_miter_limit, h_barbs, h_tip);
				dt_writer.WriteString(buf);
			}
		}
	}

	// データライターに SVG タグとして書き込む.
	void ShapeRect::export_svg(DataWriter const& dt_writer)
	{
		// 線・枠も塗りつぶしも無いなら,
		if ((equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) && 
			!is_opaque(m_fill_color)) {
			return;
		}
		wchar_t buf[1024];

		const auto x = min(m_start.x, m_start.x + m_vec[0].x);
		const auto y = min(m_start.y, m_start.y + m_vec[0].y);
		const auto w = fabsf(m_vec[0].x);
		const auto h = fabsf(m_vec[0].y);
		swprintf_s(buf,
			L"<rect x=\"%f\" y=\"%f\" width=\"%f\" height=\"%f\" ",
			x, y, w, h);
		dt_writer.WriteString(buf);

		export_svg_color(buf, 1024, m_fill_color, L"fill");
		dt_writer.WriteString(buf);

		export_svg_stroke(buf, 1024, m_stroke_width, m_stroke_color, m_dash_style, m_dash_patt, m_stroke_cap, m_join_style, m_join_miter_limit);
		dt_writer.WriteString(buf);

		dt_writer.WriteString(L"/>\n");
	}

	// データライターに SVG タグとして書き込む.
	void ShapeRRect::export_svg(DataWriter const& dt_writer)
	{
		// 線・枠も塗りつぶしも無いなら,
		if ((equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) && !is_opaque(m_fill_color)) {
			return;
		}

		wchar_t buf[1024];

		const auto x = min(m_start.x, m_start.x + m_vec[0].x);
		const auto y = min(m_start.y, m_start.y + m_vec[0].y);
		const auto w = fabsf(m_vec[0].x);
		const auto h = fabsf(m_vec[0].y);
		const auto rx = fabsf(m_corner_rad.x);
		const auto ry = fabsf(m_corner_rad.y);
		swprintf_s(buf,
			L"<rect x=\"%f\" y=\"%f\" width=\"%f\" height=\"%f\" "
			L"rx=\"%f\" ry=\"%f\" ",
			x, y, w, h,
			rx, ry
		);
		dt_writer.WriteString(buf);

		export_svg_color(buf, 1024, m_fill_color, L"fill");
		dt_writer.WriteString(buf);

		export_svg_stroke(buf, 1024, m_stroke_width, m_stroke_color, m_dash_style, m_dash_patt, m_stroke_cap, m_join_style, m_join_miter_limit);
		dt_writer.WriteString(buf);

		dt_writer.WriteString(L"/>\n");
	}

	void ShapeRuler::export_svg(const DataWriter& dt_writer)
	{
		// 線・枠も塗りつぶしも無いなら,
		if ((equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) && !is_opaque(m_fill_color)) {
			return;
		}

		if (m_dwrite_text_format == nullptr) {
			create_text_format();
		}

		constexpr wchar_t D[10] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9' };
		wchar_t buf[1024];
		//size_t len = 0;
		IDWriteFontFace3* face;	// フォントフェイス
		get_font_face(face);
		std::vector utf32{ conv_utf16_to_utf32(D, 10) };	// UTF-32 文字列
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

			// 塗りつぶし色が不透明なら, 方形を塗りつぶす.
			const auto x = min(m_start.x, m_start.x + m_vec[0].x);
			const auto y = min(m_start.y, m_start.y + m_vec[0].y);
			const auto w = fabsf(m_vec[0].x);
			const auto h = fabsf(m_vec[0].y);
			swprintf_s(buf,
				L"<rect x=\"%f\" y=\"%f\" width=\"%f\" height=\"%f\" " 
				L"stroke-width = \"0\" stroke=\"none\" ",
				x, y, w, h);
			dt_writer.WriteString(buf);

			export_svg_color(buf, 1024, m_fill_color, L"fill");
			dt_writer.WriteString(buf);

			dt_writer.WriteString(L"/>\n");
		}

		// 線・枠の色が不透明なら,
		if (is_opaque(m_stroke_color)) {

			// 目盛りとラベルを表示する.
			const double g_len = m_grid_base + 1.0;	// 方眼の大きさ
			const bool w_ge_h = fabs(m_vec[0].x) >= fabs(m_vec[0].y);	// 高さより幅の方が大きい
			const double vec_x = (w_ge_h ? m_vec[0].x : m_vec[0].y);	// 大きい方の値を x
			const double vec_y = (w_ge_h ? m_vec[0].y : m_vec[0].x);	// 小さい方の値を y
			const double intvl_x = vec_x >= 0.0 ? g_len : -g_len;	// 目盛りの間隔
			const double intvl_y = min(f_size, g_len);	// 目盛りの長さ.
			const double x0 = (w_ge_h ? m_start.x : m_start.y);
			const double y0 = static_cast<double>(w_ge_h ? m_start.y : m_start.x) + vec_y;
			const double y1 = y0 - (vec_y >= 0.0 ? intvl_y : -intvl_y);
			const double y1_5 = y0 - 0.625 * (vec_y >= 0.0 ? intvl_y : -intvl_y);

			dt_writer.WriteString(L"<g ");
			export_svg_stroke(buf, 1024,
				1.0f, m_stroke_color, D2D1_DASH_STYLE_SOLID, DASH_PATT{}, CAP_FLAT, D2D1_LINE_JOIN_BEVEL, MITER_LIMIT_DEFVAL);
			dt_writer.WriteString(buf);
			swprintf_s(buf,
				L"font-size=\"%f\" "
				L"font-family=\"%s\" "
				L"font-style=\"normal\" "
				L"font-stretch=\"normal\" "
				L"font-weight=\"normal\" ",
				m_font_size,
				m_font_family
			);
			dt_writer.WriteString(buf);

			dt_writer.WriteString(L">\n");

			const uint32_t k = static_cast<uint32_t>(floor(vec_x / intvl_x));	// 目盛りの数
			for (uint32_t i = 0; i <= k; i++) {
				const double x = x0 + i * intvl_x;
				const D2D1_POINT_2F p{	// 目盛りの始点
					w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y0),
					w_ge_h ? static_cast<FLOAT>(y0) : static_cast<FLOAT>(x)
				};
				const auto y = ((i % 5) == 0 ? y1 : y1_5);
				const D2D1_POINT_2F q{	// 目盛りの終点
					w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y),
					w_ge_h ? static_cast<FLOAT>(y) : static_cast<FLOAT>(x)
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
					static_cast<FLOAT>(m_vec[0].x >= 0.0f ? y1 - f_size / 2 - w / 2 : y1 + f_size / 2 - w / 2),
				w_ge_h ?
					// 目盛りの位置から, 書体大きさの半分だけずらし, さらに行の高さの半分だけずらし,
					// 文字の上位置を求めたあと, その位置からベースラインの距離だけずらし,
					// 文字の基点とする.
					static_cast<FLOAT>(m_vec[0].y >= 0.0f ? y1 - f_size / 2 - l_height / 2 + b_line : y1 + f_size / 2 - l_height / 2 + b_line) :
					// 目盛りの位置から, 行の高さの半分だけずらして, 文字の上位置を求め,
					// その位置からベースラインまでの距離を加え, 文字の基点とする.
					static_cast<FLOAT>(x - l_height / 2 + b_line)
				};
				swprintf_s(buf,
					L"<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\"/>\n",
					p.x, p.y, q.x, q.y
				);
				dt_writer.WriteString(buf);

				// stroke="none" を指定する.
				// stroke が指定されると線の太さ分, 文字が太るので.
				swprintf_s(buf,
					L"<text x=\"%f\" y=\"%f\" dx=\"%f\" dy=\"%f\" "
					L"stroke-width=\"0\" stroke=\"none\" >%c</text>\n",
					r.x, r.y, 0.0f, 0.0f,
					D[i % 10]);
				dt_writer.WriteString(buf);
			}
			dt_writer.WriteString(L"</g>\n");
		}
	}

	// データライターに SVG タグとして書き込む.
	void ShapeText::export_svg(DataWriter const& dt_writer)
	{
		static constexpr wchar_t* SVG_STYLE[] = {
			L"normal", L"oblique", L"italic"
		};
		static constexpr wchar_t* SVG_STRETCH[] = {
			L"normal", L"ultra-condensed", L"extra-condensed",
			L"condensed", L"semi-condensed", L"normal", L"semi-expanded",
			L"expanded", L"extra-expanded", L"ultra-expanded"
		};
		// 線・枠も塗りつぶしも, 書体の色も無いなら,
		if ((equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) && !is_opaque(m_fill_color) && !is_opaque(m_font_color)) {
			return;
		}
		if (m_dwrite_text_layout == nullptr) {
			create_text_layout();
		}
		ShapeRect::export_svg(dt_writer);

		wchar_t buf[1024];
		// %s はマルチバイトはそのままマルチバイト.
		// %hs はシングルバイトをマルチバイトにする.
		swprintf_s(buf,
			L"<g "
			L"font-size=\"%f\" "
			L"font-family=\"%s\" "
			L"font-style=\"%s\" "
			L"font-stretch=\"%s\" "
			L"font-weight=\"%d\" "
			L"stroke-width=\"0\" stroke=\"none\" >\n",
			m_font_size,
			m_font_family,
			SVG_STYLE[static_cast<uint32_t>(m_font_style)],
			SVG_STRETCH[static_cast<uint32_t>(m_font_style)],
			static_cast<uint32_t>(m_font_weight)
		);
		dt_writer.WriteString(buf);

		export_svg_color(buf, 1024, m_font_color, L"fill");
		dt_writer.WriteString(buf);

		// 書体を表示する左上位置に余白を加える.
		D2D1_POINT_2F lt_pos{};	// 左上位置
		pt_add(m_start, m_text_padding.width, m_text_padding.height, lt_pos);
		for (uint32_t i = 0; i < m_dwrite_test_cnt; i++) {
			const DWRITE_HIT_TEST_METRICS& tm = m_dwrite_test_metrics[i];
			const wchar_t* t = m_text + tm.textPosition;
			const uint32_t t_len = tm.length;
			const double px = static_cast<double>(lt_pos.x);
			const double qx = static_cast<double>(tm.left);
			const double py = static_cast<double>(lt_pos.y);
			const double qy = static_cast<double>(tm.top);
			// 文字列を表示する垂直なずらし位置を求める.
			const double dy = static_cast<double>(m_dwrite_line_metrics[i].baseline);
			// 文字列を書き込む.
			swprintf_s(buf, L"<text x=\"%f\" y=\"%f\" dy=\"%f\" >",
				px + qx, py + qy, dy);
			dt_writer.WriteString(buf);
			for (uint32_t j = 0; j < t_len; j++) {
				if (t[j] == L'<') {
					dt_writer.WriteString(L"&lt;");
				}
				else if (t[j] == L'>') {
					dt_writer.WriteString(L"&gt;");
				}
				else if (t[j] == L'&') {
					dt_writer.WriteString(L"&amp;");
				}
				else {
					const wchar_t s[2] = { t[j], L'\0' };
					dt_writer.WriteString(s);
				}
			}
			dt_writer.WriteString(L"</text>\n");
		}
		dt_writer.WriteString(L"</g>\n");
	}

	void ShapePage::export_svg(const DataWriter& dt_writer)
	{
		const float grid_base = m_grid_base;
		const D2D1_COLOR_F grid_color = m_grid_color;
		const GRID_EMPH grid_emph = m_grid_emph;
		const D2D1_POINT_2F grid_offset = m_grid_offset;
		const float page_scale = m_page_scale;
		const D2D1_SIZE_F page_size = m_page_size;
		// 拡大されても 1 ピクセルになるよう拡大率の逆数を線枠の太さに格納する.
		const FLOAT grid_width = static_cast<FLOAT>(1.0 / page_scale);	// 方眼の太さ
		D2D1_POINT_2F h_start, h_end;	// 横の方眼の開始・終了位置
		D2D1_POINT_2F v_start, v_end;	// 縦の方眼の開始・終了位置
		const auto page_h = page_size.height;
		const auto page_w = page_size.width;
		v_start.y = 0.0f;
		h_start.x = 0.0f;
		v_end.y = page_size.height;
		h_end.x = page_size.width;
		const double grid_len = max(grid_base + 1.0, 1.0);

		dt_writer.WriteString(L"<!-- Grid Lines -->\n");
		wchar_t buf[1024];
		dt_writer.WriteString(L"<g ");
		export_svg_stroke(buf, 1024,
			grid_width,
			grid_color,
			D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID, DASH_PATT{},
			CAP_FLAT,
			D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL, MITER_LIMIT_DEFVAL);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L">\n");

		// 垂直な方眼を表示する.
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
				L"<line "
				L"x1=\"%f\" y1=\"%f\" "
				L"x2 = \"%f\" y2=\"%f\" "
				L"stroke-width=\"%f\" />\n",
				v_start.x, v_start.y,
				v_end.x, v_end.y, w);
			dt_writer.WriteString(buf);
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
				L"<line "
				L"x1 = \"%f\" y1=\"%f\" "
				L"x2 = \"%f\" y2=\"%f\" "
				L"stroke-width=\"%f\" />\n",
				h_start.x, h_start.y,
				h_end.x, h_end.y, w);
			dt_writer.WriteString(buf);
		}
		dt_writer.WriteString(L"</g>\n");

	}
}