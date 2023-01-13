//------------------------------
// shape_pdf.cpp
// PDF �ւ̏�������.
//------------------------------

// PDF �t�H�[�}�b�g
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
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
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
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
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
	// ��邵���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
	//------------------------------
	static size_t export_pdf_barbs(const ShapeLine* s, const D2D1_SIZE_F page_size, const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer)
	{
		if (equal(s->m_stroke_width, 0.0f) ||
			!is_opaque(s->m_stroke_color)) {
			return 0;
		}
		wchar_t buf[1024];
		size_t n = 0;

		// �j���Ȃ��, �����ɖ߂�.
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
			n += dt_writer.WriteString(L"b\n");	// b �̓p�X����� (B �͕�����) �h��Ԃ�.
		}
		return n;
	}

	//------------------------------
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
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
				// s �̓p�X����ĕ`�悷��.
				n += dt_writer.WriteString(L"s\n");
			}
			else {
				// b �̓p�X����ēh��Ԃ�, �X�g���[�N���`�悷��.
				n += dt_writer.WriteString(L"b\n");
			}
		}
		else {
			if (equal(m_fill_color.a, 0.0f)) {
				// S �̓p�X������ɕ`�悷��.
				n += dt_writer.WriteString(L"S\n");
			}
			else {
				// B �̓p�X������ɓh��Ԃ�, �X�g���[�N���`�悷��.
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

	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
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
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
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
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
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

		// �h��Ԃ��F
		swprintf_s(buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		n += dt_writer.WriteString(buf);

		const double a = 4.0 * (sqrt(2.0) - 1.0) / 3.0;	// �x�W�F�ł��~���ߎ�����W��
		const float ty = page_size.height;	// D2D ���W�� PDF ���[�U�[��Ԃ֕ϊ����邽��
		const float rx = (m_vec[0].x >= 0.0f ? m_corner_rad.x : -m_corner_rad.x);	// ���~�� x �����̔��a
		const float ry = (m_vec[0].y >= 0.0f ? m_corner_rad.y : -m_corner_rad.y);	// ���~�� y �����̔��a

		// ��ӂ̊J�n�ʒu�Ɉړ�.
		swprintf_s(buf,
			L"%f %f m\n",
			m_start.x + rx, -(m_start.y) + ty
		);
		n += dt_writer.WriteString(buf);

		// ��ӂƉE��̊p��`��.
		float cx = m_start.x + m_vec[0].x - rx;	// �p�ۂ̒��S�_ x
		float cy = m_start.y + ry;	// �p�ۂ̒��S�_ y
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

		// �E�ӂƉE���̊p��`��.
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

		//�@���ӂƍ����̊p��`��.
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

		// ���ӂƍ���̊p��`��.
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
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
	//------------------------------
	size_t ShapeStroke::export_pdf_stroke(DataWriter const& dt_writer) const
	{
		size_t n = 0;
		wchar_t buf[1024];

		// ���g�̑���
		swprintf_s(buf, L"%f w\n", m_stroke_width);
		n += dt_writer.WriteString(buf);

		// ���g�̐F
		swprintf_s(buf, L"%f %f %f RG\n", m_stroke_color.r, m_stroke_color.g, m_stroke_color.b);	// RG �͐��g (rg �͓h��Ԃ�) �F
		n += dt_writer.WriteString(buf);

		// ���g�̒[�_
		if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE_ROUND })) {
			n += dt_writer.WriteString(L"2 J\n");
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND })) {
			n += dt_writer.WriteString(L"1 J\n");
		}
		else {
			n += dt_writer.WriteString(L"0 J\n");
		}

		// ���̌����̌`��
		// �ʎ��
		if (equal(m_join_style, D2D1_LINE_JOIN_BEVEL)) {
			n += dt_writer.WriteString(L"2 j\n");
		}
		// �ۂ�
		else if (equal(m_join_style, D2D1_LINE_JOIN_ROUND)) {
			n += dt_writer.WriteString(L"1 j\n");
		}
		// PDF �ɂ̓}�C�^�[���邢�͖ʎ�肵���Ȃ�.
		else {
			//if (equal(m_join_style, D2D1_LINE_JOIN_MITER) ||
			//equal(m_join_style, D2D1_LINE_JOIN_MITER_OR_BEVEL)) {
			n += dt_writer.WriteString(L"0 j\n");
			// �}�C�^�[����
			swprintf_s(buf, L"%f M\n", m_join_miter_limit);
			n += dt_writer.WriteString(buf);
		}

		// �j���̌`��
		if (m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH) {
			// �Ō�̐��l�͔z�u (�j���p�^�[��) ��K�p����I�t�Z�b�g.
			// [] 0		| ����
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
			// ����
			n += dt_writer.WriteString(L"[ ] 0 d\n");
		}
		return n;
	}

	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	size_t ShapeImage::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) const
	{
		// PDF �ł͕\���̑傫���̋K��l�� 1 x 1.
		// �����܂܂ł�, �摜�S�̂� 1 x 1 �Ƀ}�b�s���O�����.
		// �\������ɂ�, �ϊ��s��ɕ\������傫�����w�肵, �g�傷��.
		// �\������ʒu��, ����łȂ����������w�肷��.
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

		// ���������������.
		IDWriteFontCollection* coll = nullptr;
		if (m_dwrite_text_layout->GetFontCollection(&coll) == S_OK) {
			// �}�`�ƈ�v���鏑�̃t�@�~���𓾂�.
			IDWriteFontFamily* fam = nullptr;
			UINT32 index;
			BOOL exists;
			if (coll->FindFamilyName(family, &index, &exists) == S_OK &&
				exists &&
				coll->GetFontFamily(index, &fam) == S_OK) {
				// ���̃t�@�~������, �����ƕ�, ���̂���v���鏑�̂𓾂�.
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
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
	//------------------------------
	size_t ShapeText::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) const
	{
		size_t len = ShapeRect::export_pdf(page_size, dt_writer);
		len += dt_writer.WriteString(L"% Text\n");

		wchar_t buf[1024];
		// BT �e�L�X�g�I�u�W�F�N�g�̊J�n
		// �t�H���g�� �T�C�Y Tf
		// x���W y���W Td
		// TL�Ƃ����s�Ԃ�ݒ肷�鉉�Z�q
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
				if (encodingID == 3) {	// Unicode 2.0 (BMP �̂�)
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

		std::vector<uint8_t> mb_text{};	// �}���`�o�C�g������
		for (uint32_t i = 0; i < m_dwrite_test_cnt; i++) {
			const wchar_t* t = m_text + m_dwrite_test_metrics[i].textPosition;	// �s�̐擪�������w���|�C���^�[
			const uint32_t t_len = m_dwrite_test_metrics[i].length;	// �s�̕�����
			const float td_x = (i > 0 ? m_dwrite_test_metrics[i].left - m_dwrite_test_metrics[i - 1].left : m_dwrite_test_metrics[i].left);	// �s�� x �����̃I�t�Z�b�g
			const float td_y = (i > 0 ? m_dwrite_test_metrics[i].top - m_dwrite_test_metrics[i - 1].top : m_dwrite_test_metrics[i].top);	// �s�� y �����̃I�t�Z�b�g
			swprintf_s(buf,
				L"%f %f Td\n",
				td_x, -td_y);
			len += dt_writer.WriteString(buf);

			// ���������������.

			// wchar_t �� GID �ɕϊ����ď����o��.
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
			// wchar_t �� UTF-32 �ɕϊ����ď����o��.
			len += dt_writer.WriteString(L"% UTF-32\n<");
			std::vector<uint32_t> utf32{ conv_utf16_to_utf32(t, t_len) };
			for (int i = 0; i < utf32.size(); i++) {
				swprintf_s(buf, L"%06x", utf32[i]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"> Tj\n");
			*/

			/*
			// wchar_t �� CID �ɕϊ����ď����o��.
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
			// wchar_t �� UTF16 �Ƃ��Ă��̂܂܏����o��.
			len += dt_writer.WriteString(L"<");
			for (uint32_t j = 0; j < t_len; j++) {
				swprintf_s(buf, L"%04x", t[j]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"> Tj\n");
			*/

			/*
			const UINT CP = CP_ACP;	// �R�[�h�y�[�W
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
			// �h��Ԃ��F���s�����ȏꍇ,
			// ���`��h��Ԃ�.
			swprintf_s(
				buf,
				L"%f %f %f rg\n",
				m_fill_color.r, m_fill_color.g, m_fill_color.b
			);
			len += dt_writer.WriteString(buf);
		}
		if (is_opaque(m_stroke_color)) {

			// ���g�̐F���s�����ȏꍇ,
			const double g_len = m_grid_base + 1.0;	// ����̑傫��
			const double f_size = m_dwrite_text_format->GetFontSize();	// ���̂̑傫��
			const bool w_ge_h = fabs(m_vec[0].x) >= fabs(m_vec[0].y);	// ������蕝�̕����傫��
			const double vec_x = (w_ge_h ? m_vec[0].x : m_vec[0].y);	// �傫�����̒l�� x
			const double vec_y = (w_ge_h ? m_vec[0].y : m_vec[0].x);	// ���������̒l�� y
			const double intvl_x = vec_x >= 0.0 ? g_len : -g_len;	// �ڐ���̊Ԋu
			const double intvl_y = min(f_size, g_len);	// �ڐ���̊Ԋu
			const uint32_t k = static_cast<uint32_t>(floor(vec_x / intvl_x));	// �ڐ���̐�
			const double x0 = (w_ge_h ? m_start.x : m_start.y);
			const double y0 = static_cast<double>(w_ge_h ? m_start.y : m_start.x) + vec_y;
			const double y1 = y0 - (vec_y >= 0.0 ? intvl_y : -intvl_y);
			const double y1_5 = y0 - 0.625 * (vec_y >= 0.0 ? intvl_y : -intvl_y);
			const double y2 = y1 - (vec_y >= 0.0 ? f_size : -f_size);
			DWRITE_PARAGRAPH_ALIGNMENT p_align;
			if (w_ge_h) {
				// ���̂ق����傫���ꍇ,
				// ������ 0 �ȏ�̏ꍇ���悹�A�Ȃ��ꍇ��悹��i���̂��낦�Ɋi�[����.
				// �������z�u������`�������� (���̂̑傫���Ɠ���) ����,
				// DWRITE_PARAGRAPH_ALIGNMENT ��, �t�̌��ʂ������炷.
				p_align = (m_vec[0].y >= 0.0f ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			}
			else {
				// �c�̂ق����������ꍇ,
				// ���i��i���̂��낦�Ɋi�[����.
				p_align = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			}

			len += export_pdf_stroke(dt_writer);
			// �i���̂��낦���e�L�X�g�t�H�[�}�b�g�Ɋi�[����.
			for (uint32_t i = 0; i <= k; i++) {
				// ����̑傫�����Ƃɖڐ����\������.
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
				// ����̑傫�����Ƃɖڐ����\������.
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

				// �ڐ���̒l��\������.
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