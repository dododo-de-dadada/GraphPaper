//------------------------------
// shape_svg.cpp
// SVG �Ƃ��ď�������.
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

	template<bool ARROW = false, size_t N>
	static void export_svg_stroke(wchar_t(&buf)[N], /*<---*/const ShapeStroke* s);
	template <size_t N>
	static void export_svg_barbs(wchar_t(&buf)[N], const ShapeLine* s, const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos);

	//------------------------------
	// �F�� SVG �̑����Ƃ��ăo�b�t�@�ɏo��
	//------------------------------
	template <size_t N>
	static void export_svg_color(wchar_t (&buf)[N], const D2D1_COLOR_F color, const wchar_t* name)
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
	// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	// dt_reader	�f�[�^���[�_�[
	//------------------------------
	void ShapeBezi::export_svg(DataWriter const& dt_writer) const
	{
		D2D1_BEZIER_SEGMENT b_seg;
		pt_add(m_start, m_vec[0], b_seg.point1);
		pt_add(b_seg.point1, m_vec[1], b_seg.point2);
		pt_add(b_seg.point2, m_vec[2], b_seg.point3);

		wchar_t buf[1024];
		swprintf_s(buf,
			L"<path d=\"M%f %f C%f %f, %f %f, %f %f\" "
			L"fill=\"none\" ",
			m_start.x, m_start.y,
			b_seg.point1.x, b_seg.point1.y,
			b_seg.point2.x, b_seg.point2.y,
			b_seg.point3.x, b_seg.point3.y
		);
		dt_writer.WriteString(buf);
		export_svg_stroke(buf, this);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");

		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[3];
			bezi_calc_arrow(m_start, b_seg, m_arrow_size, barbs);
			export_svg_barbs(buf, this, barbs, barbs[2]);
			dt_writer.WriteString(buf);
		}
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeElli::export_svg(DataWriter const& dt_writer) const
	{
		D2D1_POINT_2F r;
		pt_mul(m_vec[0], 0.5, r);
		D2D1_POINT_2F c;
		pt_add(m_start, r, c);

		wchar_t buf[1024];
		swprintf_s(buf,
			L"<ellipse cx=\"%f\" cy=\"%f\" rx=\"%f\" ry=\"%f\" ",
			c.x, c.y, r.x, r.y
		);
		dt_writer.WriteString(buf);
		export_svg_color(buf, m_fill_color, L"fill");
		dt_writer.WriteString(buf);
		export_svg_stroke(buf, this);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");
	}

	//------------------------------
	// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	//------------------------------
	winrt::Windows::Foundation::IAsyncAction ShapeGroup::export_as_svg_async(const DataWriter& dt_writer) const
	{
		dt_writer.WriteString(L"<g>\n");
		for (const Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			if (typeid(*s) == typeid(ShapeImage)) {
				co_await static_cast<const ShapeImage*>(s)->export_as_svg_async(dt_writer);
			}
			else if (typeid(*s) == typeid(ShapeGroup)) {
				co_await static_cast<const ShapeGroup*>(s)->export_as_svg_async(dt_writer);
			}
			else {
				s->export_svg(dt_writer);
			}
		}
		dt_writer.WriteString(L"</g>\n");
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	// dt_write		�f�[�^���C�^�[
	winrt::Windows::Foundation::IAsyncAction ShapeImage::export_as_svg_async(const DataWriter& dt_writer) const
	{
		// �������̃����_���A�N�Z�X�X�g���[��
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

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeLine::export_svg(DataWriter const& dt_writer) const
	{
		D2D1_POINT_2F e_pos;
		pt_add(m_start, m_vec[0], e_pos);
		wchar_t buf[1024];
		swprintf_s(buf,
			L"<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" ",
			m_start.x, m_start.y, e_pos.x, e_pos.y
		);
		dt_writer.WriteString(buf);
		export_svg_stroke(buf, this);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[2];
			D2D1_POINT_2F tip_pos;
			if (ShapeLine::line_get_arrow_pos(m_start, m_vec[0], m_arrow_size, barbs, tip_pos)) {
				export_svg_barbs(buf, this, barbs, tip_pos);
				dt_writer.WriteString(buf);
			}
		}
	}

	// �����f�[�^���C�^�[�� SVG �^�O�Ƃ��ăo�b�t�@�ɏ�������.
	// buf	�o�b�t�@
	// s	�����}�`
	// barbs	���̗��[�̈ʒu [2]
	// tip_pos	���̐�[�̈ʒu
	template <size_t N>
	static void export_svg_barbs(wchar_t (&buf)[N], const ShapeLine* s, const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos)
	{
		wchar_t fill[256];
		if (s->m_arrow_style == ARROW_STYLE::FILLED) {
			export_svg_color(fill, s->m_stroke_color, L"fill");
		}
		else {
			wcscpy_s(fill, L"fill=\"none\" ");
		}
		wchar_t stroke[256];
		export_svg_stroke<true>(stroke, s);
		swprintf_s(
			buf,
			L"<path d=\"M%f %f L%f %f L%f %f\" %s%s/>\n",
			barbs[0].x, barbs[0].y,
			tip_pos.x, tip_pos.y,
			barbs[1].x, barbs[1].y,
			fill, stroke
		);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapePoly::export_svg(DataWriter const& dt_writer) const
	{
		const auto d_cnt = m_vec.size();	// �����̐�
		const auto v_cnt = d_cnt + 1;
		D2D1_POINT_2F v_pos[MAX_N_GON];
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
		export_svg_stroke(buf, this);
		dt_writer.WriteString(buf);
		export_svg_color(buf, m_fill_color, L"fill");
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F h_tip;
			D2D1_POINT_2F h_barbs[2];
			if (ShapePoly::poly_get_arrow_barbs(v_cnt, v_pos, m_arrow_size, h_tip, h_barbs)) {
				export_svg_barbs(buf, this, h_barbs, h_tip);
				dt_writer.WriteString(buf);
			}
		}
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeRect::export_svg(DataWriter const& dt_writer) const
	{
		wchar_t buf[1024];

		swprintf_s(buf,
			L"<rect x=\"%f\" y=\"%f\" "
			L"width=\"%f\" height=\"%f\" ",
			m_start.x, m_start.y, m_vec[0].x, m_vec[0].y);
		dt_writer.WriteString(buf);
		export_svg_color(buf, m_fill_color, L"fill");
		dt_writer.WriteString(buf);
		export_svg_stroke(buf, this);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeRRect::export_svg(DataWriter const& dt_writer) const
	{
		wchar_t buf[1024];

		swprintf_s(buf,
			L"<rect x=\"%f\" y=\"%f\" "
			L"width=\"%f\" height=\"%f\" "
			L"rx=\"%f\" ry=\"%f\" ",
			m_start.x, m_start.y, m_vec[0].x, m_vec[0].y,
			m_corner_rad.x, m_corner_rad.y
		);
		dt_writer.WriteString(buf);
		export_svg_color(buf, m_fill_color, L"fill");
		dt_writer.WriteString(buf);
		export_svg_stroke(buf, this);
		dt_writer.WriteString(buf);
		dt_writer.WriteString(L"/>\n");
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	template<bool ARROW, size_t N>
	static void export_svg_stroke(wchar_t (&buf)[N], /*<---*/const ShapeStroke* s)
	{
		const float width = s->m_stroke_width;
		const D2D1_COLOR_F& color = s->m_stroke_color;
		if (width >= FLT_MIN && color.a >= FLT_MIN) {
			const D2D1_DASH_STYLE dash = s->m_dash_style;	// �j���̌`��
			const DASH_PATT& patt = s->m_dash_patt;	// �j���̔z�u
			const CAP_STYLE cap = s->m_stroke_cap;	// �[�̌`��
			const D2D1_LINE_JOIN join = s->m_join_style;	// ���̌���
			const auto limit = s->m_join_miter_limit;	// ���̌����̃}�C�^�[����
			wchar_t buf_color[256]{};
			wchar_t buf_patt[256]{};
			wchar_t* buf_cap = L"";
			wchar_t buf_join[256]{};

			export_svg_color(buf_color, color, L"stroke");
			if constexpr (ARROW) {
				// ���Ȃ�j���p�^�[���͖�������.
				wcscpy_s(buf_patt, L"stroke-dasharray=\"none\" ");
			}
			else {
				if (dash == D2D1_DASH_STYLE_DASH) {
					swprintf_s(buf_patt,
						L"stroke-dasharray=\"%.0f %.0f\" ", patt.m_[0], patt.m_[1]);
				}
				else if (dash == D2D1_DASH_STYLE_DOT) {
					swprintf_s(buf_patt,
						L"stroke-dasharray=\"%.0f %.0f\" ", patt.m_[2], patt.m_[3]);
				}
				else if (dash == D2D1_DASH_STYLE_DASH_DOT) {
					swprintf_s(buf_patt,
						L"stroke-dasharray=\"%.0f %.0f %.0f %.0f\" ",
						patt.m_[0], patt.m_[1], patt.m_[2], patt.m_[3]);
				}
				else if (dash == D2D1_DASH_STYLE_DASH_DOT_DOT) {
					swprintf_s(buf_patt,
						L"stroke-dasharray=\"%.0f %.0f %.0f %.0f %.0f %.0f\" ",
						patt.m_[0], patt.m_[1], patt.m_[2], patt.m_[3], patt.m_[2], patt.m_[3]);
				}
				else {
					wcscpy_s(buf_patt, L"stroke-dasharray=\"none\" ");
				}
			}
			if (equal(cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT })) {
				buf_cap = L"stroke-linecap=\"butt\" ";
			}
			else if (equal(cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND })) {
				buf_cap = L"stroke-linecap=\"round\" ";
			}
			else if (equal(cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE })) {
				buf_cap = L"stroke-linecap=\"square\" ";
			}
			else if (equal(cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE })) {
				// SVG �ɎO�p�͂Ȃ�.
				buf_cap = L"stroke-linecap=\"butt\" ";
			}

			if (join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				wcscpy_s(buf_join, L"stroke-linejoin=\"bevel\" ");
			}
			else if (join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
				join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				swprintf_s(buf_join,
					L"stroke-linejoin=\"miter\" stroke-miterlimit=\"%f\" ", limit);
			}
			else if (join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				wcscpy_s(buf_join, L"stroke-linejoin=\"round\" ");
			}
			swprintf_s(buf,
				L"stroke-width=\"%f\" %s%s%s%s",
				width,
				buf_color, buf_patt, buf_cap, buf_join
			);
		}

	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeText::export_svg(DataWriter const& dt_writer) const
	{
		static constexpr wchar_t* SVG_STYLE[] = {
			L"normal", L"oblique", L"italic"
		};
		static constexpr wchar_t* SVG_STRETCH[] = {
			L"normal", L"ultra-condensed", L"extra-condensed",
			L"condensed", L"semi-condensed", L"normal", L"semi-expanded",
			L"expanded", L"extra-expanded", L"ultra-expanded"
		};
		// ���������̂��炵�ʂ����߂�.
		//
		// Chrome �ł�, �e�L�X�g�^�O�ɑ��� alignment-baseline="text-before-edge" 
		// ���w�肷�邾����, ����ʒu����ɂ��� Dwrite �Ɠ����悤�ɕ\�������.
		// ������, IE �� Edge �ł�, alignment-baseline �����͊��҂������������Ȃ��̂�,
		// �㕔����̃x�[�X���C���܂Œl�ł���, ���������̂��炵�� dy ��
		// �s���ƂɌv�Z��K�v������.
		// ���̂Ƃ�, �㕔����̃x�[�X���C���̍��� = �A�Z���g�ɂ͂Ȃ�Ȃ��̂�
		// �f�Z���g��p���Čv�Z����K�v������.
		// �e�L�X�g���C�A�E�g����t�H���g���g���b�N�X���擾����, �ȉ��̂悤�ɋ��߂�.
		// ���Ȃ݂�, designUnitsPerEm ��, �z�u (Em) �{�b�N�X�̒P�ʂ�����̑傫��.
		// �f�Z���g��, �t�H���g�����̔z�u�{�b�N�X�̉�������x�[�X���C���܂ł̒���.
		// dy = ���̍s�̃q�b�g�e�X�g���g���b�N�X�̍��� - �t�H���g�̑傫�� * (�f�Z���g / �P�ʑ傫��) �ƂȂ�, �͂�.
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
			ShapeRect::export_svg(dt_writer);
		}
		wchar_t buf[1024];
		// %s �̓}���`�o�C�g�͂��̂܂܃}���`�o�C�g.
		// %hs �̓V���O���o�C�g���}���`�o�C�g�ɂ���.
		swprintf_s(buf,
			L"<g "
			L"font-size=\"%f\" "
			L"font-family=\"%s\" "
			L"font-style=\"%s\" "
			L"font-stretch=\"%s\" "
			L"stroke=\"none\" >\n",
			m_font_size,
			m_font_family,
			SVG_STYLE[static_cast<uint32_t>(m_font_style)],
			SVG_STRETCH[static_cast<uint32_t>(m_font_style)]
		);
		dt_writer.WriteString(buf);
		export_svg_color(buf, m_font_color, L"fill");
		dt_writer.WriteString(buf);

		// ���̂�\�����鍶��ʒu�ɗ]����������.
		D2D1_POINT_2F nw_pos;
		pt_add(m_start, m_text_padding.width, m_text_padding.height, nw_pos);
		for (uint32_t i = 0; i < m_dw_test_cnt; i++) {
			const DWRITE_HIT_TEST_METRICS& tm = m_dw_test_metrics[i];
			const wchar_t* t = m_text + tm.textPosition;
			const uint32_t t_len = tm.length;
			const double px = static_cast<double>(nw_pos.x);
			const double qx = static_cast<double>(tm.left);
			const double py = static_cast<double>(nw_pos.y);
			const double qy = static_cast<double>(tm.top);
			// �������\�����鐂���Ȃ��炵�ʒu�����߂�.
			const double dy = static_cast<double>(m_dw_line_metrics[i].baseline);
			// ���������������.
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