//------------------------------
// shape_pdf.cpp
// PDF �ւ̏�������.
//------------------------------

// PDF �t�H�[�}�b�g
// https://aznote.jakou.com/prog/pdf/index.html

#include "pch.h"
#include "shape.h"

using namespace winrt;

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
		if (equal(m_stroke_color.a, 0.0f)) {
			return 0;
		}

		size_t n = dt_writer.WriteString(L"% Bezier curve\n");
		//size_t n = dt_write("% Bezier curve\n", dt_writer);
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
		if (equal(m_stroke_color.a, 0.0f)) {
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

		// ���g�̌���
		if (equal(m_join_style, D2D1_LINE_JOIN_BEVEL)) {
			n += dt_writer.WriteString(L"2 j\n");
		}
		else if (equal(m_join_style, D2D1_LINE_JOIN_ROUND)) {
			n += dt_writer.WriteString(L"1 j\n");
		}
		else {
			//if (equal(m_join_style, D2D1_LINE_JOIN_MITER) ||
			//equal(m_join_style, D2D1_LINE_JOIN_MITER_OR_BEVEL)) {
			n += dt_writer.WriteString(L"0 j\n");
		}

		// �}�C�^�[����
		swprintf_s(buf, L"%f M\n", m_join_miter_limit);
		n += dt_writer.WriteString(buf);

		// �j���̎��
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
		std::vector<uint8_t> vec(4 * m_orig.width * m_orig.height);
		winrt::com_ptr<IWICBitmap> wic_bitmap;
		wic_factory->CreateBitmapFromMemory(m_orig.width, m_orig.height, GUID_WICPixelFormat32bppBGRA, 4 * m_orig.width, 4 * m_orig.width * m_orig.height, vec.data(), wic_bitmap.put());
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
		sheet.m_d2d.m_d2d_factory->CreateWicBitmapRenderTarget(wic_bitmap.get(), prop, target.put());
		*/
		wchar_t buf[1024];

		// �\���̑傫���̋K��l�� 1 �~ 1.
		// �����܂܂ł�, �摜�S�̂� 1 �~ 1 �Ƀ}�b�s���O�����.
		// �\������ɂ�, �ϊ��s��ɕ\������傫�����w�肵, �g�傷��.
		// �\������ʒu��, ����łȂ����������w�肷��.
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

	//------------------------------
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
	//------------------------------
	size_t ShapeText::export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) const
	{
		size_t len = dt_writer.WriteString(L"% Text\n");
		len += ShapeRect::export_pdf(page_size, dt_writer);
		/*
		*/
		//ShapeText::export_pdf(sheet, dt_writer);
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
			-(m_start.y + m_text_padding.height + m_dw_line_metrics[0].baseline) + page_size.height
		);
		len += dt_writer.WriteString(buf);

		std::vector<uint8_t> sjis{};
		for (uint32_t i = 0; i < m_dw_test_cnt; i++) {
			const wchar_t* t = m_text + m_dw_test_metrics[i].textPosition;	// �s�̐擪�������w���|�C���^�[
			const uint32_t t_len = m_dw_test_metrics[i].length;	// �s�̕�����
			const float td_x = (i > 0 ? m_dw_test_metrics[i].left - m_dw_test_metrics[i - 1].left : m_dw_test_metrics[i].left);	// �s�� x �����̃I�t�Z�b�g
			const float td_y = (i > 0 ? m_dw_test_metrics[i].top - m_dw_test_metrics[i - 1].top : m_dw_test_metrics[i].top);	// �s�� y �����̃I�t�Z�b�g
			swprintf_s(buf,
				L"%f %f Td\n",
				td_x, -td_y);
			len += dt_writer.WriteString(buf);

			// ���������������.
			dt_writer.WriteByte(L'<'); len++;
			for (uint32_t j = 0; j < t_len; j++) {
				swprintf_s(buf, L"%04x", t[j]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"> Tj\n");
			/*
			const size_t sjis_len = WideCharToMultiByte(CP_ACP, 0, t, t_len, NULL, 0, NULL, NULL);
			if (sjis.size() < sjis_len) {
				sjis.resize(sjis_len);
			}
			WideCharToMultiByte(CP_ACP, 0, t, t_len, (LPSTR)sjis.data(), static_cast<int>(sjis_len), NULL, NULL);
			len += dt_write(
				"<",
				dt_writer);
			for (int j = 0; j < sjis_len; j++) {
				constexpr char* HEX = "0123456789abcdef";
				dt_writer.WriteByte(HEX[sjis[j] >> 4]);
				dt_writer.WriteByte(HEX[sjis[j] & 15]);
			}
			len += 2 * sjis_len;
			len += dt_write(
				"> Tj\n",
				dt_writer);
			*/
		}
		len += dt_writer.WriteString(L"ET\n");
		return len;
	}

}