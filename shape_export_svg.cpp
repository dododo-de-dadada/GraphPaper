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

	static void export_svg_arrow(wchar_t* buf, const size_t len, const ARROW_STYLE arrow, const float width, const D2D1_COLOR_F& color, const D2D1_CAP_STYLE& cap, const D2D1_LINE_JOIN join, const float miter_limit, const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pt);
	static void export_svg_color(wchar_t* buf, const size_t len, const D2D1_COLOR_F color, const wchar_t* name);
	static void export_svg_stroke(wchar_t* buf, const size_t len, const float width, const D2D1_COLOR_F& color, const D2D1_DASH_STYLE dash, const DASH_PAT& patt, const D2D1_CAP_STYLE cap, const D2D1_LINE_JOIN join, const float limit);

	//------------------------------
	// バッファに矢じりを SVG タグとして書き込む.
	// buf	出力先バッファ
	// len	バッファの長さ
	// arrow	矢じるしの形式
	// width	線・枠の太さ	
	// color	線・枠の色
	// cap	線分の端点の形式
	// join	線分の連結の形式
	// miter_limit	尖り制限
	// barbs[]	矢じりの両端の位置
	// tip_pt	矢じりの先端の位置
	//------------------------------
	static void export_svg_arrow(wchar_t* buf, const size_t len, const ARROW_STYLE arrow, const float width, const D2D1_COLOR_F& color, const D2D1_CAP_STYLE& cap, const D2D1_LINE_JOIN join, const float miter_limit, const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pt)	// 矢じりの先端の位置
	{
		if (arrow == ARROW_STYLE::ARROW_FILLED) {
			swprintf_s(buf, len,
				L"<path d=\"M%f %f L%f %f L%f %f z\" ",
				barbs[0].x, barbs[0].y,
				tip_pt.x, tip_pt.y,
				barbs[1].x, barbs[1].y
			);
			const auto len1 = wcslen(buf);
			export_svg_color(buf + len1, len - len1, color, L"fill");
		}
		else {
			swprintf_s(buf, len,
				L"<path d=\"M%f %f L%f %f L%f %f\" fill=\"none\" ",
				barbs[0].x, barbs[0].y,
				tip_pt.x, tip_pt.y,
				barbs[1].x, barbs[1].y
			);
		}
		const auto len2 = wcslen(buf);

		export_svg_stroke(buf + len2, len - len2, width, color, D2D1_DASH_STYLE_SOLID, DASH_PAT{}, cap, join, miter_limit);
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
			const int32_t r = static_cast<int32_t>(min(floor(color.r * 256.0), 255.0));	// Red
			const int32_t g = static_cast<int32_t>(min(floor(color.g * 256.0), 255.0));	// Green
			const int32_t b = static_cast<int32_t>(min(floor(color.b * 256.0), 255.0));	// Blue
			swprintf_s(buf,
				len,
				L"%s=\"#%02x%02x%02x\" %s-opacity=\"%f\" ",
				name,
				r, g, b,
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
	// limit	尖り制限
	//------------------------------
	static void export_svg_stroke(
		wchar_t* buf, // 出力先
		const size_t len,	// 出力先の長さ
		const float width, 
		const D2D1_COLOR_F& color, 
		const D2D1_DASH_STYLE dash, 
		const DASH_PAT& patt, 
		const D2D1_CAP_STYLE cap, 
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
			if (cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT) {
				wcscpy_s(buf + len3, len - len3, L"stroke-linecap=\"butt\" ");
			}
			else if (cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
				wcscpy_s(buf + len3, len - len3, L"stroke-linecap=\"round\" ");
			}
			else if (cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
				wcscpy_s(buf + len3, len - len3, L"stroke-linecap=\"square\" ");
			}
			else if (cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
				// SVG に三角はないので, かわりに stroke-linecap="butt"
				wcscpy_s(buf + len3, len - len3, L"stroke-linecap=\"butt\" ");
			}

			const auto len4 = wcslen(buf);
			if (join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				wcscpy_s(buf + len4, len - len4, L"stroke-linejoin=\"bevel\" ");
			}
			else if (join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
				join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				// D2D の尖り制限とは異なるので注意.
				// 尖り制限を超える部分があるとき, D2D では超えた部分だけが断ち切られるが, 
				// PDF と SVG ではいきなり Bevel join　になる.
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
	void ShapeBezier::export_svg(DataWriter const& dt_writer) noexcept
	{
		D2D1_BEZIER_SEGMENT b_seg;
		pt_add(m_start, m_lineto[0], b_seg.point1);
		pt_add(b_seg.point1, m_lineto[1], b_seg.point2);
		pt_add(b_seg.point2, m_lineto[2], b_seg.point3);

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

		export_svg_stroke(buf, 1024, m_stroke_width, m_stroke_color, m_stroke_dash, m_dash_pat, m_stroke_cap, m_stroke_join, m_stroke_join_limit);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");

		if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			D2D1_POINT_2F barbs[3];
			bezi_get_pos_arrow(m_start, b_seg, m_arrow_size, barbs);
			export_svg_arrow(buf, 1024, m_arrow_style, m_stroke_width, m_stroke_color, m_arrow_cap,
				m_arrow_join, m_arrow_join_limit, barbs, barbs[2]);
			dt_writer.WriteString(buf);
		}
	}

	// データライターに SVG タグとして書き込む.
	void ShapeEllipse::export_svg(DataWriter const& dt_writer) noexcept
	{
		D2D1_POINT_2F r;
		pt_mul(m_lineto, 0.5, r);
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
		export_svg_stroke(buf, 1024, m_stroke_width, m_stroke_color, m_stroke_dash, m_dash_pat, m_stroke_cap, m_stroke_join, m_stroke_join_limit);
		dt_writer.WriteString(buf);

		// だ円を閉じる.
		dt_writer.WriteString(L"/>\n");
	}

	//------------------------------
	// データライターに SVG として書き込む.
	//------------------------------
	winrt::Windows::Foundation::IAsyncAction SHAPE_GROUP::export_as_svg_async(const DataWriter& dt_writer)
	{
		dt_writer.WriteString(L"<!-- Group -->\n");
		dt_writer.WriteString(L"<g>\n");
		for (SHAPE* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			if (typeid(*s) == typeid(SHAPE_IMAGE)) {
				co_await static_cast<SHAPE_IMAGE*>(s)->export_as_svg_async(dt_writer);
			}
			else if (typeid(*s) == typeid(SHAPE_GROUP)) {
				co_await static_cast<SHAPE_GROUP*>(s)->export_as_svg_async(dt_writer);
			}
			else {
				s->export_svg(dt_writer);
			}
		}
		dt_writer.WriteString(L"</g>\n");
	}

	// データライターに SVG として書き込む.
	// dt_write		データライター
	winrt::Windows::Foundation::IAsyncAction SHAPE_IMAGE::export_as_svg_async(const DataWriter& dt_writer)
	{
		InMemoryRandomAccessStream image_stream{};	// メモリのランダムアクセスストリーム
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
	void ShapeLine::export_svg(DataWriter const& dt_writer) noexcept
	{
		// 線・枠も無いなら,
		if (equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) {
			return;
		}
		const D2D1_POINT_2F end_pt{
			m_start.x + m_lineto.x,
			m_start.y + m_lineto.y
		};
		wchar_t buf[1024];
		swprintf_s(buf,
			L"<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" ",
			m_start.x, m_start.y, end_pt.x, end_pt.y
		);
		dt_writer.WriteString(buf);

		export_svg_stroke(buf, 1024, m_stroke_width, m_stroke_color, m_stroke_dash, m_dash_pat, m_stroke_cap, m_stroke_join, m_stroke_join_limit);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");
		if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			D2D1_POINT_2F barb[2];
			D2D1_POINT_2F tip_pt;
			if (ShapeLine::line_get_pos_arrow(m_start, m_lineto, m_arrow_size, barb, tip_pt)) {
				export_svg_arrow(buf, 1024, m_arrow_style, m_stroke_width, m_stroke_color, m_arrow_cap, m_arrow_join, m_arrow_join_limit, barb, tip_pt);
				dt_writer.WriteString(buf);
			}
		}
	}

	// データライターに SVG タグとして書き込む.
	void ShapePoly::export_svg(DataWriter const& dt_writer) noexcept
	{
		// 線・枠も塗りつぶしも無いなら,
		if ((equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) && 
			!is_opaque(m_fill_color)) {
			return;
		}

		const auto d_cnt = m_lineto.size();	// 始点を除くの頂点数
		std::vector<D2D1_POINT_2F> p(d_cnt + 1);
		wchar_t buf[1024];
		swprintf_s(buf,
			L"<path d=\"M%f %f ",
			m_start.x, m_start.y
		);
		dt_writer.WriteString(buf);

		p[0] = m_start;
		for (size_t i = 0; i < d_cnt; i++) {
			swprintf_s(buf,
				L"l%f %f ", m_lineto[i].x, m_lineto[i].y);
			dt_writer.WriteString(buf);
			pt_add(p[i], m_lineto[i], p[i + 1]);
		}
		if (m_end == D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED) {
			dt_writer.WriteString(L"Z");
		}
		dt_writer.WriteString(L"\" ");

		export_svg_stroke(buf, 1024, m_stroke_width, m_stroke_color, m_stroke_dash, m_dash_pat, m_stroke_cap, m_stroke_join, m_stroke_join_limit);
		dt_writer.WriteString(buf);

		export_svg_color(buf, 1024, m_fill_color, L"fill");
		dt_writer.WriteString(buf);

		dt_writer.WriteString(L"/>\n");

		if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			D2D1_POINT_2F tip;
			D2D1_POINT_2F barb[2];
			if (ShapePoly::poly_get_pos_arrow(d_cnt + 1, std::data(p), m_arrow_size, barb, tip)) {
				export_svg_arrow(buf, 1024, m_arrow_style, m_stroke_width, m_stroke_color, m_arrow_cap, m_arrow_join, m_arrow_join_limit, barb, tip);
				dt_writer.WriteString(buf);
			}
		}
	}

	// データライターに SVG タグとして書き込む.
	void SHAPE_CLOSED::export_svg(DataWriter const& dt_writer) noexcept
	{
		// 線・枠も塗りつぶしも無いなら,
		if ((equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) && 
			!is_opaque(m_fill_color)) {
			return;
		}
		wchar_t buf[1024];

		const auto x = min(m_start.x, m_start.x + m_lineto.x);
		const auto y = min(m_start.y, m_start.y + m_lineto.y);
		const auto w = fabsf(m_lineto.x);
		const auto h = fabsf(m_lineto.y);
		swprintf_s(buf,
			L"<rect x=\"%f\" y=\"%f\" width=\"%f\" height=\"%f\" ",
			x, y, w, h);
		dt_writer.WriteString(buf);

		export_svg_color(buf, 1024, m_fill_color, L"fill");
		dt_writer.WriteString(buf);

		export_svg_stroke(buf, 1024, m_stroke_width, m_stroke_color, m_stroke_dash, m_dash_pat, m_stroke_cap, m_stroke_join, m_stroke_join_limit);
		dt_writer.WriteString(buf);

		dt_writer.WriteString(L"/>\n");
	}

	// データライターに SVG タグとして書き込む.
	void ShapeRRect::export_svg(DataWriter const& dt_writer) noexcept
	{
		// 線・枠も塗りつぶしも無いなら,
		if ((equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) && !is_opaque(m_fill_color)) {
			return;
		}

		wchar_t buf[1024];

		const auto x = min(m_start.x, m_start.x + m_lineto.x);
		const auto y = min(m_start.y, m_start.y + m_lineto.y);
		const auto w = fabsf(m_lineto.x);
		const auto h = fabsf(m_lineto.y);
		const auto rx = fabsf(m_corner_radius.x);
		const auto ry = fabsf(m_corner_radius.y);
		swprintf_s(buf,
			L"<rect x=\"%f\" y=\"%f\" width=\"%f\" height=\"%f\" "
			L"rx=\"%f\" ry=\"%f\" ",
			x, y, w, h,
			rx, ry
		);
		dt_writer.WriteString(buf);

		export_svg_color(buf, 1024, m_fill_color, L"fill");
		dt_writer.WriteString(buf);

		export_svg_stroke(buf, 1024, m_stroke_width, m_stroke_color, m_stroke_dash, m_dash_pat, m_stroke_cap, m_stroke_join, m_stroke_join_limit);
		dt_writer.WriteString(buf);

		dt_writer.WriteString(L"/>\n");
	}

	void ShapeRuler::export_svg(const DataWriter& dt_writer) noexcept
	{
		HRESULT hr = S_OK;

		// 線・枠の色も塗りつぶしの色も透明なら
		if ((equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) &&
			!is_opaque(m_fill_color)) {
			return;
		}

		if (m_dwrite_text_format == nullptr) {
			hr = create_text_format();
		}

		constexpr wchar_t D[10] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9' };
		wchar_t buf[1024];
		IDWriteFontFace3* f_face;	// 字面
		get_font_face(f_face);
		std::vector utf32{ text_utf16_to_utf32(D, 10) };	// UTF-32 文字列
		uint16_t gid[10];	// グリフ識別子
		f_face->GetGlyphIndices(std::data(utf32), 10, gid);
		DWRITE_FONT_METRICS f_met;	// 書体の計量
		f_face->GetMetrics(&f_met);
		int32_t g_adv[10];	// グリフごとの幅
		f_face->GetDesignGlyphAdvances(10, gid, g_adv);
		f_face->Release();
		const double f_size = m_font_size;	// 書体の大きさ
		const double f_asc = f_met.ascent;
		const double f_des = f_met.descent;
		const double f_gap = f_met.lineGap;
		const double f_upe = f_met.designUnitsPerEm;
		const double l_height = f_size * (f_asc + f_des + f_gap) / f_upe;	// 行の高さ
		const double b_line = f_size * f_asc / f_upe;	// (文字の上端からの) ベースラインまでの距離

		if (is_opaque(m_fill_color)) {

			// 塗りつぶし色が不透明なら, 方形を塗りつぶす.
			const double sx = m_start.x;
			const double sy = m_start.y;
			const double px = m_lineto.x;
			const double py = m_lineto.y;
			const double x = min(sx, sx + px);
			const double y = min(sy, sy + py);
			const double w = abs(px);
			const double h = abs(py);
			swprintf_s(buf,
				L"<rect x=\"%f\" y=\"%f\" width=\"%f\" height=\"%f\" stroke-width = \"0\" stroke=\"none\" ",
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
			const bool w_ge_h = fabs(m_lineto.x) >= fabs(m_lineto.y);	// 高さより幅の方が大きい
			const double to_x = (w_ge_h ? m_lineto.x : m_lineto.y);	// 大きい方の値を x
			const double to_y = (w_ge_h ? m_lineto.y : m_lineto.x);	// 小さい方の値を y
			const double intvl_x = to_x >= 0.0 ? g_len : -g_len;	// 目盛りの間隔
			const double intvl_y = min(f_size, g_len);	// 目盛りの長さ.
			const double x0 = (w_ge_h ? m_start.x : m_start.y);
			const double y0 = static_cast<double>(w_ge_h ? m_start.y : m_start.x) + to_y;
			const double y1 = y0 - (to_y >= 0.0 ? intvl_y : -intvl_y);
			const double y1_5 = y0 - 0.625 * (to_y >= 0.0 ? intvl_y : -intvl_y);

			dt_writer.WriteString(L"<g ");
			export_svg_stroke(buf, 1024, 1.0f, m_stroke_color, D2D1_DASH_STYLE_SOLID, DASH_PAT{}, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_MITER_OR_BEVEL, JOIN_MITER_LIMIT_DEFVAL);
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

			const uint32_t k = static_cast<uint32_t>(floor(to_x / intvl_x));	// 目盛りの数
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
					static_cast<FLOAT>(m_lineto.x >= 0.0f ? y1 - f_size / 2.0 - w / 2.0 : y1 + f_size / 2.0 - w / 2.0),
				w_ge_h ?
					// 目盛りの位置から, 書体大きさの半分だけずらし, さらに行の高さの半分だけずらし,
					// 文字の上位置を求めたあと, その位置からベースラインの距離だけずらし,
					// 文字の基点とする.
					static_cast<FLOAT>(m_lineto.y >= 0.0f ? y1 - f_size / 2.0 - l_height / 2.0 + b_line : y1 + f_size / 2.0 - l_height / 2.0 + b_line) :
					// 目盛りの位置から, 行の高さの半分だけずらして, 文字の上位置を求め,
					// その位置からベースラインまでの距離を加え, 文字の基点とする.
					static_cast<FLOAT>(x - l_height / 2.0 + b_line)
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
	void ShapeText::export_svg(DataWriter const& dt_writer) noexcept
	{
		// 線・枠の色も塗りつぶしの色も, 書体の色も無いなら,
		if ((equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) &&
			!is_opaque(m_fill_color) && !is_opaque(m_font_color)) {
			return;
		}

		// 文字列の枠を書き込む.
		ShapeRect::export_svg(dt_writer);

		if (m_dwrite_text_layout == nullptr) {
			create_text_layout();
		}

		wchar_t* f_style;
		if (m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE) {
			f_style = L"oblique";
		}
		else if (m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC) {
			f_style = L"italic";
		}
		else {
			f_style = L"normal";
		}
		wchar_t* f_stretch;
		if (m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_CONDENSED) {
			f_stretch = L"extra-condensed";
		}
		else if (m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_CONDENSED) {
			f_stretch = L"condensed";
		}
		else if (m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_CONDENSED) {
			f_stretch = L"semi-condensed";
		}
		else if (m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL) {
			f_stretch = L"normal";
		}
		else if (m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_EXPANDED) {
			f_stretch = L"semi-expanded";
		}
		else if (m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXPANDED) {
			f_stretch = L"expanded";
		}
		else if (m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_EXPANDED) {
			f_stretch = L"extra-expanded";
		}
		else if (m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_EXPANDED) {
			f_stretch = L"ultra-expanded";
		}
		else {
			f_stretch = L"normal";
		}

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
			f_style,
			f_stretch,
			static_cast<uint32_t>(m_font_weight)
		);
		dt_writer.WriteString(buf);

		export_svg_color(buf, 1024, m_font_color, L"fill");
		dt_writer.WriteString(buf);

		// 文字列を表示する左上点に余白を加える.
		D2D1_POINT_2F lt_pt{};	// 左上点
		pt_add(m_start, m_text_padding.width, m_text_padding.height, lt_pt);
		for (uint32_t i = 0; i < m_dwrite_test_cnt; i++) {
			const DWRITE_HIT_TEST_METRICS& tm = m_dwrite_test_metrics[i];
			const wchar_t* t = m_text + tm.textPosition;
			const uint32_t t_len = tm.length;
			const double px = static_cast<double>(lt_pt.x);
			const double qx = static_cast<double>(tm.left);
			const double py = static_cast<double>(lt_pt.y);
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

	// 方眼を表示する.
	void SHAPE_SHEET::export_svg(const DataWriter& dt_writer) noexcept
	{
		//const D2D1_SIZE_F g_size{	// グリッドを表示する大きさ (用紙から内余白を除いた分)
		//	m_sheet_size.width - (m_sheet_padding.left + m_sheet_padding.right),
		//	m_sheet_size.height - (m_sheet_padding.top + m_sheet_padding.bottom)
		//};
		const auto draw_w = m_sheet_size.width - (m_sheet_padding.left + m_sheet_padding.right);
		const auto draw_h = m_sheet_size.height - (m_sheet_padding.top + m_sheet_padding.bottom);
		const FLOAT g_width = 1.0f;	// 方眼の太さ
		D2D1_POINT_2F h_start, h_end;	// 横の方眼の開始・終了位置
		D2D1_POINT_2F v_start, v_end;	// 縦の方眼の開始・終了位置
		const auto sh = draw_h;
		const auto sw = draw_w;
		v_start.y = 0.0f;
		h_start.x = 0.0f;
		v_end.y = draw_h;
		h_end.x = draw_w;
		const double g_len = max(m_grid_base + 1.0, 1.0);

		wchar_t buf[1024];
		dt_writer.WriteString(L"<!-- Grids -->\n");
		dt_writer.WriteString(L"<g ");
		export_svg_stroke(buf, 1024, g_width, m_grid_color, D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID, DASH_PAT{}, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL, JOIN_MITER_LIMIT_DEFVAL);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L">\n");

		// 垂直な方眼を表示する.
		float gw;
		double x;
		for (uint32_t i = 0;
			(x = round((g_len * i + m_grid_offset.x) / PT_ROUND) * PT_ROUND) <= sw; i++) {
			if (m_grid_emph.m_gauge_2 != 0 && (i % m_grid_emph.m_gauge_2) == 0) {
				gw = 2.0F * g_width;
			}
			else if (m_grid_emph.m_gauge_1 != 0 && (i % m_grid_emph.m_gauge_1) == 0) {
				gw = g_width;
			}
			else {
				gw = 0.5F * g_width;
			}
			v_start.x = v_end.x = static_cast<FLOAT>(x);

			swprintf_s(buf,
				L"<line x1=\"%f\" y1=\"%f\" "
				L"x2 = \"%f\" y2=\"%f\" "
				L"stroke-width=\"%f\" />\n",
				v_start.x, v_start.y,
				v_end.x, v_end.y, gw);
			dt_writer.WriteString(buf);
		}
		// 水平な方眼を表示する.
		double y;
		for (uint32_t i = 0;
			(y = round((g_len * i + m_grid_offset.y) / PT_ROUND) * PT_ROUND) <= sh; i++) {
			if (m_grid_emph.m_gauge_2 != 0 && (i % m_grid_emph.m_gauge_2) == 0) {
				gw = 2.0F * g_width;
			}
			else if (m_grid_emph.m_gauge_1 != 0 && (i % m_grid_emph.m_gauge_1) == 0) {
				gw = g_width;
			}
			else {
				gw = 0.5F * g_width;
			}
			h_start.y = h_end.y = static_cast<FLOAT>(y);

			swprintf_s(buf,
				L"<line x1 = \"%f\" y1=\"%f\" "
				L"x2 = \"%f\" y2=\"%f\" "
				L"stroke-width=\"%f\" />\n",
				h_start.x, h_start.y,
				h_end.x, h_end.y, gw);
			dt_writer.WriteString(buf);
		}
		dt_writer.WriteString(L"</g>\n");
	}

	// 図形をデータライターに SVG として書き込む.
	void SHAPE_ARC::export_svg(const DataWriter& dt_writer) noexcept
	{
		wchar_t buf[1024];
		D2D1_POINT_2F p[5]{};
		const bool exists_fill = is_opaque(m_fill_color);
		const bool exists_stroke = (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color));

		if (exists_fill || exists_stroke) {
			get_verts(p);
		}
		if (exists_fill) {
			// A rx ry x-axis-rotation large-arc-flag sweep-flag x y
			// rx	X 軸方向の半径
			// ry	Y 軸方向の半径
			// x-axis-rotation	傾き (角度)
			// large-arc-flag	円弧が 180 度より大きいなら 1, そうでないなら 0.
			// sweep-flag	円弧が時計回りなら 1, そうでないなら 0.
			swprintf_s(buf,
				L"<path d=\"M %f %f "
				L"A %f %f %f %d %d %f %f "
				L"L %f %f"
				L"\" stroke=\"none\" ",
				p[3].x, p[3].y,
				fabs(m_radius.width), fabs(m_radius.height),
				m_angle_rot,
				m_larg_flag != 0 ? 1 : 0,
				1,//m_sweep_dir != D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE ? 0 : 1,
				p[4].x, p[4].y,
				p[2].x, p[2].y
			);
			dt_writer.WriteString(buf);
			export_svg_color(buf, 1024, m_fill_color, L"fill");
			dt_writer.WriteString(buf);
			dt_writer.WriteString(L"/>\n");
		}
		if (exists_stroke) {
			swprintf_s(buf,
				L"<path d=\"M %f %f "
				L"A %f %f %f %d %d %f %f "
				L"\" fill=\"none\" ",
				p[3].x, p[3].y,
				fabs(m_radius.width), fabs(m_radius.height),
				m_angle_rot,
				m_larg_flag != 0 ? 1 : 0,
				1,//m_sweep_dir != D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE ? 0 : 1,
				p[4].x, p[4].y
			);
			dt_writer.WriteString(buf);
			export_svg_stroke(buf, 1024, m_stroke_width, m_stroke_color, m_stroke_dash, m_dash_pat, m_stroke_cap, m_stroke_join, m_stroke_join_limit);
			dt_writer.WriteString(buf);
			dt_writer.WriteString(L"/>\n");
			if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
				D2D1_POINT_2F arrow[3];
				arc_get_pos_arrow(m_lineto[0], p[2], m_radius, m_angle_start, m_angle_end, m_angle_rot, m_arrow_size, m_sweep_dir,
					arrow);
				export_svg_arrow(buf, 1024, m_arrow_style, m_stroke_width, m_stroke_color, m_arrow_cap, m_arrow_join, m_arrow_join_limit, arrow, arrow[2]);
				dt_writer.WriteString(buf);
			}
		}
	}
}