//------------------------------
// shape_dt.cpp
// 読み込み, 書き込み.
//------------------------------
#include "pch.h"
#include <shcore.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//------------------------------
	// 色を SVG の属性としてバッファに出力
	//------------------------------
	template <size_t N>
	static void svg_sprintf_color(wchar_t (&buf)[N], const D2D1_COLOR_F color, const wchar_t* name)
	{
		swprintf_s(buf,
			L"%s=\"#%02x%02x%02x\" %s-opacity=\"%f\" ",
			name,
			static_cast<uint32_t>(std::round(color.r) * 255.0),
			static_cast<uint32_t>(std::round(color.g) * 255.0),
			static_cast<uint32_t>(std::round(color.b) * 255.0),
			name, color.a
		);
	}

	//------------------------------
	// データライターに SVG として書き込む.
	// dt_reader	データリーダー
	//------------------------------
	void ShapeBezi::svg_write(DataWriter const& dt_writer) const
	{
		D2D1_BEZIER_SEGMENT b_seg;
		pt_add(m_pos, m_vec[0], b_seg.point1);
		pt_add(b_seg.point1, m_vec[1], b_seg.point2);
		pt_add(b_seg.point2, m_vec[2], b_seg.point3);

		wchar_t buf[1024];
		swprintf_s(buf,
			L"<path d=\"M%f %f C%f %f, %f %f, %f %f\" "
			L"fill=\"none\" ",
			m_pos.x, m_pos.y,
			b_seg.point1.x, b_seg.point1.y,
			b_seg.point2.x, b_seg.point2.y,
			b_seg.point3.x, b_seg.point3.y
		);
		dt_writer.WriteString(buf);
		svg_sprintf_stroke(buf);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");

		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[3];
			bezi_calc_arrow(m_pos, b_seg, m_arrow_size, barbs);
			svg_write_barbs(barbs, barbs[2], dt_writer);
		}
	}

	// データライターに SVG タグとして書き込む.
	void ShapeElli::svg_write(DataWriter const& dt_writer) const
	{
		D2D1_POINT_2F r;
		pt_mul(m_vec[0], 0.5, r);
		D2D1_POINT_2F c;
		pt_add(m_pos, r, c);

		wchar_t buf[1024];
		swprintf_s(buf,
			L"<ellipse cx=\"%f\" cy=\"%f\" rx=\"%f\" ry=\"%f\" ",
			c.x, c.y, r.x, r.y
		);
		dt_writer.WriteString(buf);
		svg_sprintf_color(buf, m_fill_color, L"fill");
		dt_writer.WriteString(buf);
		svg_sprintf_stroke(buf);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");
	}

	//------------------------------
	// データライターに SVG として書き込む.
	//------------------------------
	winrt::Windows::Foundation::IAsyncAction ShapeGroup::svg_write_async(DataWriter const& dt_writer) const
	{
		dt_writer.WriteString(L"<g>\n");
		for (const Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			if (typeid(*s) == typeid(ShapeImage)) {
				co_await static_cast<const ShapeImage*>(s)->svg_write_async(dt_writer);
			}
			else if (typeid(*s) == typeid(ShapeGroup)) {
				co_await static_cast<const ShapeGroup*>(s)->svg_write_async(dt_writer);
			}
			else {
				s->svg_write(dt_writer);
			}
		}
		dt_writer.WriteString(L"</g>\n");
	}

	// データライターに SVG として書き込む.
	// file_name	画像ファイル名
	// dt_write		データライター
	winrt::Windows::Foundation::IAsyncAction ShapeImage::svg_write_async(/*const wchar_t file_name[],*/ DataWriter const& dt_writer) const
	{
		using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;
		using winrt::Windows::Storage::Streams::IRandomAccessStream;
		using winrt::Windows::Graphics::Imaging::BitmapEncoder;
		using winrt::Windows::Security::Cryptography::CryptographicBuffer;
		using winrt::Windows::Storage::Streams::Buffer;
		using winrt::Windows::Storage::Streams::InputStreamOptions;

		// メモリのランダムアクセスストリーム
		InMemoryRandomAccessStream image_stream{};
		co_await copy_to(BitmapEncoder::PngEncoderId(), image_stream);
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
			m_pos.x, m_pos.y,
			m_view.width, m_view.height,
			m_opac
		);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(base64);
		dt_writer.WriteString(L"\" />\n");
	}

	// データライターに SVG タグとして書き込む.
	void ShapeLine::svg_write(DataWriter const& dt_writer) const
	{
		D2D1_POINT_2F e_pos;
		pt_add(m_pos, m_vec[0], e_pos);
		wchar_t buf[1024];
		swprintf_s(buf,
			L"<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" ",
			m_pos.x, m_pos.y, e_pos.x, e_pos.y
		);
		dt_writer.WriteString(buf);
		svg_sprintf_stroke(buf);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[2];
			D2D1_POINT_2F tip_pos;
			if (ShapeLine::line_get_arrow_pos(m_pos, m_vec[0], m_arrow_size, barbs, tip_pos)) {
				svg_write_barbs(barbs, tip_pos, dt_writer);
			}
		}
	}

	// 矢じりをデータライターに SVG タグとして書き込む.
	// barbs	矢じるしの両端の位置 [2]
	// tip_pos	矢じるしの先端の位置
	// a_style	矢じるしの形状
	// dt_writer	データライター
	void ShapeLine::svg_write_barbs(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, const DataWriter& dt_writer) const
	{
		wchar_t buf[1024];

		swprintf_s(
			buf,
			L"<path d=\"M%f %f L%f %f L%f %f\" ",
			barbs[0].x, barbs[0].y,
			tip_pos.x, tip_pos.y,
			barbs[1].x, barbs[1].y
		);
		dt_writer.WriteString(buf);
		if (m_arrow_style == ARROW_STYLE::FILLED) {
			svg_sprintf_color(buf, m_stroke_color, L"fill");
			dt_writer.WriteString(buf);
		}
		else {
			dt_writer.WriteString(L"fill=\"none\" ");
		}
		svg_sprintf_stroke<false>(buf);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");
	}

	// データライターに SVG タグとして書き込む.
	void ShapePoly::svg_write(DataWriter const& dt_writer) const
	{
		const auto d_cnt = m_vec.size();	// 差分の数
		const auto v_cnt = d_cnt + 1;
		D2D1_POINT_2F v_pos[MAX_N_GON];
		wchar_t buf[1024];
		swprintf_s(buf,
			L"<path d=\"M%f %f ",
			m_pos.x, m_pos.y
		);
		dt_writer.WriteString(buf);
		v_pos[0] = m_pos;
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
		svg_sprintf_stroke(buf);
		dt_writer.WriteString(buf);
		svg_sprintf_color(buf, m_fill_color, L"fill");
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F h_tip;
			D2D1_POINT_2F h_barbs[2];
			if (ShapePoly::poly_get_arrow_barbs(v_cnt, v_pos, m_arrow_size, h_tip, h_barbs)) {
				svg_write_barbs(h_barbs, h_tip, dt_writer);
			}
		}
	}

	// データライターに SVG タグとして書き込む.
	void ShapeRect::svg_write(DataWriter const& dt_writer) const
	{
		wchar_t buf[1024];

		swprintf_s(buf,
			L"<rect x=\"%f\" y=\"%f\" "
			L"width=\"%f\" height=\"%f\" ",
			m_pos.x, m_pos.y, m_vec[0].x, m_vec[0].y);
		dt_writer.WriteString(buf);
		svg_sprintf_color(buf, m_fill_color, L"fill");
		dt_writer.WriteString(buf);
		svg_sprintf_stroke(buf);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");
	}

	// データライターに SVG タグとして書き込む.
	void ShapeRRect::svg_write(DataWriter const& dt_writer) const
	{
		wchar_t buf[1024];

		swprintf_s(buf,
			L"<rect x=\"%f\" y=\"%f\" "
			L"width=\"%f\" height=\"%f\" "
			L"rx=\"%f\" ry=\"%f\" ",
			m_pos.x, m_pos.y, m_vec[0].x, m_vec[0].y,
			m_corner_rad.x, m_corner_rad.y
		);
		dt_writer.WriteString(buf);
		svg_sprintf_color(buf, m_fill_color, L"fill");
		dt_writer.WriteString(buf);
		svg_sprintf_stroke(buf);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");
	}

	// データライターに SVG タグとして書き込む.
	template<bool DASH, size_t N>
	void ShapeStroke::svg_sprintf_stroke(wchar_t (&buf)[N]) const
	{
		if (m_stroke_width >= FLT_MIN && m_stroke_color.a >= FLT_MIN) {
			wchar_t color[256]{};
			wchar_t array[256]{};
			wchar_t* linecap = L"";
			wchar_t join[256]{};

			svg_sprintf_color(color, m_stroke_color, L"stroke");
			if constexpr (DASH) {
				if (m_dash_style == D2D1_DASH_STYLE_DASH) {
					swprintf_s(array,
						L"stroke-dasharray=\"%.0f %.0f\" ",
						m_dash_patt.m_[0], m_dash_patt.m_[1]);
				}
				else if (m_dash_style == D2D1_DASH_STYLE_DOT) {
					swprintf_s(array,
						L"stroke-dasharray=\"%.0f %.0f\" ",
						m_dash_patt.m_[2], m_dash_patt.m_[3]);
				}
				else if (m_dash_style == D2D1_DASH_STYLE_DASH_DOT) {
					swprintf_s(array,
						L"stroke-dasharray=\"%.0f %.0f %.0f %.0f\" ",
						m_dash_patt.m_[0], m_dash_patt.m_[1], m_dash_patt.m_[2], m_dash_patt.m_[3]);
				}
				else if (m_dash_style == D2D1_DASH_STYLE_DASH_DOT_DOT) {
					swprintf_s(array,
						L"stroke-dasharray=\"%.0f %.0f %.0f %.0f %.0f %.0f\" ",
						m_dash_patt.m_[0], m_dash_patt.m_[1], m_dash_patt.m_[2], m_dash_patt.m_[3], m_dash_patt.m_[2], m_dash_patt.m_[3]);
				}
				else {
					wcscpy_s(array, L"stroke-dasharray=\"none\" ");
				}
			}
			else {
				wcscpy_s(array, L"stroke-dasharray=\"none\" ");
			}
			if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT })) {
				linecap = L"stroke-linecap=\"butt\" ";
			}
			else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND })) {
				linecap = L"stroke-linecap=\"round\" ";
			}
			else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE })) {
				linecap = L"stroke-linecap=\"square\" ";
			}
			else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE })) {
				// SVG に三角はない.
				linecap = L"stroke-linecap=\"butt\" ";
			}

			if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				wcscpy_s(join, L"stroke-linejoin=\"bevel\" ");
			}
			else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
				m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				swprintf_s(join,
					L"stroke-linejoin=\"miter\" "
					L"stroke-miterlimit=\"%f\" ",
					m_join_miter_limit
				);
			}
			else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				wcscpy_s(join, L"stroke-linejoin=\"round\" ");
			}
			swprintf_s(buf,
				L"stroke-width=\"%f\" %s%s%s%s",
				m_stroke_width,
				color, array, linecap, join
			);
		}

	}

	// データライターに SVG タグとして書き込む.
	void ShapeText::svg_write(DataWriter const& dt_writer) const
	{
		static constexpr char* SVG_STYLE[] = {
			"normal", "oblique", "italic"
		};
		static constexpr char* SVG_STRETCH[] = {
			"normal", "ultra-condensed", "extra-condensed",
			"condensed", "semi-condensed", "normal", "semi-expanded",
			"expanded", "extra-expanded", "ultra-expanded"
		};
		// 垂直方向のずらし量を求める.
		//
		// Chrome では, テキストタグに属性 alignment-baseline="text-before-edge" 
		// を指定するだけで, 左上位置を基準にして Dwrite と同じように表示される.
		// しかし, IE や Edge では, alignment-baseline 属性は期待した働きをしないので,
		// 上部からのベースラインまで値である, 垂直方向のずらし量 dy を
		// 行ごとに計算を必要がある.
		// このとき, 上部からのベースラインの高さ = アセントにはならないので
		// デセントを用いて計算する必要もある.
		// テキストレイアウトからフォントメトリックスを取得して, 以下のように求める.
		// ちなみに, designUnitsPerEm は, 配置 (Em) ボックスの単位あたりの大きさ.
		// デセントは, フォント文字の配置ボックスの下部からベースラインまでの長さ.
		// dy = その行のヒットテストメトリックスの高さ - フォントの大きさ × (デセント ÷ 単位大きさ) となる, はず.
		//IDWriteFontCollection* fonts;
		//m_dw_layout->GetFontCollection(&fonts);
		//IDWriteFontFamily* family;
		//fonts->GetFontFamily(0, &family);
		//IDWriteFont* font;
		//family->GetFont(0, &font);
		//DWRITE_FONT_METRICS metrics;
		//font->GetMetrics(&metrics);
		//const double descent = m_font_size * ((static_cast<double>(metrics.descent)) / metrics.designUnitsPerEm);

		if (is_opaque(m_fill_color) || is_opaque(m_stroke_color) || m_stroke_width >= FLT_MIN) {
			ShapeRect::svg_write(dt_writer);
		}
		wchar_t buf[1024];
		// %s はマルチバイトはそのままマルチバイト.
		// %hs はシングルバイトをマルチバイトにする.
		swprintf_s(buf,
			L"<g "
			L"font-size=\"%f\" "
			L"font-family=\"%s\" "
			L"font-style=\"%hs\" "
			L"font-stretch=\"%hs\" "
			L"stroke=\"none\" >\n",
			m_font_size,
			m_font_family,
			SVG_STYLE[static_cast<uint32_t>(m_font_style)],
			SVG_STRETCH[static_cast<uint32_t>(m_font_style)]
		);
		dt_writer.WriteString(buf);
		svg_sprintf_color(buf, m_font_color, L"fill");
		dt_writer.WriteString(buf);

		// 書体を表示する左上位置に余白を加える.
		D2D1_POINT_2F nw_pos;
		pt_add(m_pos, m_text_padding.width, m_text_padding.height, nw_pos);
		for (uint32_t i = 0; i < m_dw_test_cnt; i++) {
			const DWRITE_HIT_TEST_METRICS& tm = m_dw_test_metrics[i];
			const wchar_t* t = m_text + tm.textPosition;
			const uint32_t t_len = tm.length;
			const double px = static_cast<double>(nw_pos.x);
			const double qx = static_cast<double>(tm.left);
			const double py = static_cast<double>(nw_pos.y);
			const double qy = static_cast<double>(tm.top);
			// 文字列を表示する垂直なずらし位置を求める.
			const double dy = static_cast<double>(m_dw_line_metrics[i].baseline);
			// 文字列を書き込む.
			swprintf_s(buf,
				L"<text x=\"%f\" y=\"%f\" dy=\"%f\" >",
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

}