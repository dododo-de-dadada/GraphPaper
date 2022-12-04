//------------------------------
// shape_dt.cpp
// �ǂݍ���, ��������.
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	static void tx_svg_dt_write(const wchar_t* t, const uint32_t t_len, const double x, const double y, const double dy, DataWriter const& dt_writer);

	// ��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
// t	������
// t_len	������
// x, y	�ʒu
// dy	�����Ȃ��炵��
// dt_writer	�f�[�^���C�^�[
	static void tx_svg_dt_write(const wchar_t* t, const uint32_t t_len, const double x, const double y, const double dy, DataWriter const& dt_writer)
	{
		svg_dt_write("<text ", dt_writer);
		svg_dt_write(x, "x", dt_writer);
		svg_dt_write(y, "y", dt_writer);
		svg_dt_write(dy, "dy", dt_writer);
		//svg_dt_write("text-before-edge", "alignment-baseline", dt_writer);
		svg_dt_write(">", dt_writer);
		uint32_t k = 0;
		for (uint32_t i = k; i < t_len; i++) {
			const wchar_t c = t[i];
			char* ent;
			if (c == L'<') {
				ent = "&lt;";
			}
			else if (c == L'>') {
				ent = "&gt;";
			}
			else if (c == L'&') {
				ent = "&amp;";
			}
			//else if (c == L'"') {
			// ent = "&quot;";
			//}
			//else if (c == L'\'') {
			// ent = "&apos;";
			//}
			else {
				continue;
			}
			if (i > k) {
				svg_dt_write(t + k, i - k, dt_writer);
			}
			svg_dt_write(ent, dt_writer);
			k = i + 1;
		}
		if (t_len > k) {
			svg_dt_write(t + k, t_len - k, dt_writer);
		}
		svg_dt_write("</text>" SVG_NEW_LINE, dt_writer);
	}

	//------------------------------
	// �f�[�^���C�^�[�� SVG �Ƃ��đ������ƃV���O���o�C�g���������������.
	// val	�V���O���o�C�g������
	// name	������
	//------------------------------
	void svg_dt_write(const char* val, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		std::snprintf(buf, sizeof(buf), "%s=\"%s\" ", name, val);
		svg_dt_write(buf, dt_writer);
	}

	//------------------------------
	// �f�[�^���C�^�[�� SVG �Ƃ��ăV���O���o�C�g���������������.
	// val	�V���O���o�C�g������
	// dt_writer	�f�[�^���C�^�[
	//------------------------------
	void svg_dt_write(const char* val, DataWriter const& dt_writer)
	{
		for (uint32_t i = 0; val[i] != '\0'; i++) {
			dt_writer.WriteByte(val[i]);
		}
	}

	//------------------------------
	// �f�[�^���C�^�[�� SVG �Ƃ��đ������ƐF����������.
	// val	�F
	// name	������
	//------------------------------
	void svg_dt_write(const D2D1_COLOR_F val, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		const uint32_t vr = static_cast<uint32_t>(std::round(val.r * 255.0)) & 0xff;
		const uint32_t vb = static_cast<uint32_t>(std::round(val.b * 255.0)) & 0xff;
		const uint32_t vg = static_cast<uint32_t>(std::round(val.g * 255.0)) & 0xff;
		sprintf_s(buf, "%s=\"#%02x%02x%02x\" ", name, vr, vg, vb);
		svg_dt_write(buf, dt_writer);
		if (!is_opaque(val)) {
			std::snprintf(buf, sizeof(buf), "%s-opacity=\"%.3f\" ", name, val.a);
			svg_dt_write(buf, dt_writer);
		}
	}

	//------------------------------
	// �f�[�^���C�^�[�� SVG �Ƃ��ĐF����������.
	// val	�F
	//------------------------------
	void svg_dt_write(const D2D1_COLOR_F val, DataWriter const& dt_writer)
	{
		char buf[8];
		const uint32_t vr = static_cast<uint32_t>(std::round(val.r * 255.0)) & 0xFF;
		const uint32_t vb = static_cast<uint32_t>(std::round(val.b * 255.0)) & 0xFF;
		const uint32_t vg = static_cast<uint32_t>(std::round(val.g * 255.0)) & 0xFF;
		sprintf_s(buf, "#%02x%02x%02x", vr, vg, vb);
		svg_dt_write(buf, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��Ĕj���̌`���Ɣz�u����������.
	// d_style	���g�̌`��
	// d_patt	�j���̔z�u
	// s_width	���g�̑���
	void svg_dt_write(const D2D1_DASH_STYLE d_style, const DASH_PATT& d_patt, const double s_width, DataWriter const& dt_writer)
	{
		if (s_width < FLT_MIN) {
			return;
		}
		const double a[]{
			d_patt.m_[0],
			d_patt.m_[1],
			d_patt.m_[2],
			d_patt.m_[3]
		};
		char buf[256];
		if (d_style == D2D1_DASH_STYLE_DASH) {
			sprintf_s(buf, "stroke-dasharray=\"%.0f %.0f\" ", a[0], a[1]);
		}
		else if (d_style == D2D1_DASH_STYLE_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f\" ", a[2], a[3]);
		}
		else if (d_style == D2D1_DASH_STYLE_DASH_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f %.0f %.0f\" ", a[0], a[1], a[2], a[3]);
		}
		else if (d_style == D2D1_DASH_STYLE_DASH_DOT_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f %.0f %.0f %.0f %.0f\" ", a[0], a[1], a[2], a[3], a[2], a[3]);
		}
		else {
			return;
		}
		svg_dt_write(buf, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��Ė��߂ƈʒu����������.
	// val	�ʒu
	// cmd	����
	void svg_dt_write(const D2D1_POINT_2F val, const char* cmd, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s%f %f ", cmd, val.x, val.y);
		svg_dt_write(buf, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��đ������ƈʒu����������.
	// val	�ʒu
	// name_x	x ���̖��O
	// name_y	y ���̖��O
	void svg_dt_write(const D2D1_POINT_2F val, const char* x_name, const char* y_name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%f\" %s=\"%f\" ", x_name, val.x, y_name, val.y);
		svg_dt_write(buf, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��đ������ƕ��������l����������
	// val	���������l
	// a_name	������
	void svg_dt_write(const double val, const char* a_name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%f\" ", a_name, val);
		svg_dt_write(buf, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��ĕ��������l����������
	// val	���������l
	void svg_dt_write(const float val, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%f ", val);
		svg_dt_write(buf, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��đ������� 32 �r�b�g����������������.
	// val	32 �r�b�g������
	// a_name	������
	void svg_dt_write(const uint32_t val, const char* a_name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%u\" ", a_name, val);
		svg_dt_write(buf, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��ă}���`�o�C�g���������������.
	// val	�}���`�o�C�g������
	// v_len	������̕�����
	// dt_writer	�f�[�^���C�^�[
	void svg_dt_write(const wchar_t val[], const uint32_t v_len, DataWriter const& dt_writer)
	{
		if (v_len > 0) {
			const auto s_len = WideCharToMultiByte(CP_UTF8, 0, val, v_len, (char*)NULL, 0, NULL, NULL);
			auto s = new char[static_cast<size_t>(s_len) + 1];
			WideCharToMultiByte(CP_UTF8, 0, val, v_len, static_cast<LPSTR>(s), s_len, NULL, NULL);
			s[s_len] = '\0';
			svg_dt_write(s, dt_writer);
			delete[] s;
		}
	}

	//------------------------------
	// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	// dt_reader	�f�[�^���[�_�[
	//------------------------------
	void ShapeBezi::svg_write(DataWriter const& dt_writer) const
	{
		D2D1_BEZIER_SEGMENT b_seg;

		pt_add(m_pos, m_vec[0], b_seg.point1);
		pt_add(b_seg.point1, m_vec[1], b_seg.point2);
		pt_add(b_seg.point2, m_vec[2], b_seg.point3);
		svg_dt_write("<path d=\"", dt_writer);
		svg_dt_write(m_pos, "M", dt_writer);
		svg_dt_write(b_seg.point1, "C", dt_writer);
		svg_dt_write(b_seg.point2, ",", dt_writer);
		svg_dt_write(b_seg.point3, ",", dt_writer);
		svg_dt_write("\" ", dt_writer);
		svg_dt_write("none", "fill", dt_writer);
		svg_write_stroke(dt_writer);
		svg_dt_write("/>" SVG_NEW_LINE, dt_writer);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[3];
			bezi_calc_arrow(m_pos, b_seg, m_arrow_size, barbs);
			svg_write_barbs(barbs, barbs[2], dt_writer);
		}
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeElli::svg_write(DataWriter const& dt_writer) const
	{
		D2D1_POINT_2F r;
		pt_mul(m_vec[0], 0.5, r);
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, r, c_pos);
		svg_dt_write("<ellipse ", dt_writer);
		svg_dt_write(c_pos, "cx", "cy", dt_writer);
		svg_dt_write(static_cast<double>(r.x), "rx", dt_writer);
		svg_dt_write(static_cast<double>(r.y), "ry", dt_writer);
		svg_dt_write(m_fill_color, "fill", dt_writer);
		ShapeStroke::svg_write(dt_writer);
		svg_dt_write("/>" SVG_NEW_LINE, dt_writer);
	}

	//------------------------------
	// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	//------------------------------
	void ShapeGroup::svg_write(DataWriter const& dt_writer) const
	{
		svg_dt_write("<g>" SVG_NEW_LINE, dt_writer);
		for (const Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			s->svg_write(dt_writer);
		}
		svg_dt_write("</g>" SVG_NEW_LINE, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	// file_name	�摜�t�@�C����
	// dt_write		�f�[�^���C�^�[
	void ShapeImage::svg_write(const wchar_t file_name[], DataWriter const& dt_writer) const
	{
		svg_dt_write("<image href=\"", dt_writer);
		svg_dt_write(file_name, wchar_len(file_name), dt_writer);
		svg_dt_write("\" ", dt_writer);
		svg_dt_write(m_pos, "x", "y", dt_writer);
		svg_dt_write(m_view.width, "width", dt_writer);
		svg_dt_write(m_view.height, "height", dt_writer);
		svg_dt_write(m_opac, "opacity", dt_writer);
		svg_dt_write("/>" SVG_NEW_LINE, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	// �摜�Ȃ��̏ꍇ.
	// dt_write		�f�[�^���C�^�[
	void ShapeImage::svg_write(DataWriter const& dt_writer) const
	{
		constexpr char RECT[] =
			"<rect x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" "
			"stroke=\"black\" fill=\"white\" />" SVG_NEW_LINE;
		constexpr char TEXT[] =
			"<text x=\"50%\" y=\"50%\" text-anchor=\"middle\" dominant-baseline=\"central\">"
			"NO IMAGE</text>" SVG_NEW_LINE;
		svg_dt_write("<!--" SVG_NEW_LINE, dt_writer);
		svg_write(nullptr, dt_writer);
		svg_dt_write("-->" SVG_NEW_LINE, dt_writer);
		svg_dt_write("<svg ", dt_writer);
		svg_dt_write(m_pos, "x", "y", dt_writer);
		svg_dt_write(m_view.width, "width", dt_writer);
		svg_dt_write(m_view.height, "height", dt_writer);
		svg_dt_write("viewBox=\"0 0 ", dt_writer);
		svg_dt_write(m_view.width, dt_writer);
		svg_dt_write(m_view.height, dt_writer);
		svg_dt_write("\">" SVG_NEW_LINE, dt_writer);
		svg_dt_write(RECT, dt_writer);
		svg_dt_write(TEXT, dt_writer);
		svg_dt_write("</svg>" SVG_NEW_LINE, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeLine::svg_write(DataWriter const& dt_writer) const
	{
		D2D1_POINT_2F e_pos;

		pt_add(m_pos, m_vec[0], e_pos);
		svg_dt_write("<line ", dt_writer);
		svg_dt_write(m_pos, "x1", "y1", dt_writer);
		svg_dt_write(e_pos, "x2", "y2", dt_writer);
		svg_write_stroke(dt_writer);
		svg_dt_write("/>" SVG_NEW_LINE, dt_writer);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[2];
			D2D1_POINT_2F tip_pos;
			if (ShapeLine::line_get_arrow_pos(m_pos, m_vec[0], m_arrow_size, barbs, tip_pos)) {
				svg_write_barbs(barbs, tip_pos, dt_writer);
			}
		}
	}

	// �����f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	// barbs	��邵�̗��[�̈ʒu [2]
	// tip_pos	��邵�̐�[�̈ʒu
	// a_style	��邵�̌`��
	// dt_writer	�f�[�^���C�^�[
	void ShapeLine::svg_write_barbs(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const
	{
		svg_dt_write("<path d=\"", dt_writer);
		svg_dt_write("M", dt_writer);
		svg_dt_write(barbs[0].x, dt_writer);
		svg_dt_write(barbs[0].y, dt_writer);
		svg_dt_write("L", dt_writer);
		svg_dt_write(tip_pos.x, dt_writer);
		svg_dt_write(tip_pos.y, dt_writer);
		svg_dt_write("L", dt_writer);
		svg_dt_write(barbs[1].x, dt_writer);
		svg_dt_write(barbs[1].y, dt_writer);
		svg_dt_write("\" ", dt_writer);
		if (m_arrow_style == ARROW_STYLE::FILLED) {
			svg_dt_write(m_stroke_color, "fill", dt_writer);
		}
		else {
			svg_dt_write("fill=\"none\" ", dt_writer);
		}
		svg_dt_write(m_stroke_color, "stroke", dt_writer);
		svg_dt_write(m_stroke_width, "stroke-width", dt_writer);
		if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT })) {
			svg_dt_write("stroke-linecap=\"butt\" ", dt_writer);
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND })) {
			svg_dt_write("stroke-linecap=\"round\" ", dt_writer);
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE })) {
			svg_dt_write("stroke-linecap=\"square\" ", dt_writer);
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE })) {
			//svg_dt_write("stroke-linecap=\"square\" ", dt_writer);
		}
		if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
			svg_dt_write("stroke-linejoin=\"bevel\" ", dt_writer);
		}
		else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
			m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
			svg_dt_write("stroke-linejoin=\"miter\" ", dt_writer);
			svg_dt_write(m_join_miter_limit, "stroke-miterlimit", dt_writer);
		}
		else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
			svg_dt_write("stroke-linejoin=\"round\" ", dt_writer);
		}
		svg_dt_write(" />" SVG_NEW_LINE, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapePoly::svg_write(DataWriter const& dt_writer) const
	{
		svg_dt_write("<path d=\"", dt_writer);
		svg_dt_write(m_pos, "M", dt_writer);
		const auto d_cnt = m_vec.size();	// �����̐�
		const auto v_cnt = d_cnt + 1;
		D2D1_POINT_2F v_pos[MAX_N_GON];

		v_pos[0] = m_pos;
		for (size_t i = 0; i < d_cnt; i++) {
			svg_dt_write(m_vec[i], "l", dt_writer);
			pt_add(v_pos[i], m_vec[i], v_pos[i + 1]);
		}
		if (m_end_closed) {
			svg_dt_write("Z", dt_writer);
		}
		svg_dt_write("\" ", dt_writer);
		svg_write_stroke(dt_writer);
		svg_dt_write(m_fill_color, "fill", dt_writer);
		svg_dt_write("/>" SVG_NEW_LINE, dt_writer);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F h_tip;
			D2D1_POINT_2F h_barbs[2];
			if (ShapePoly::poly_get_arrow_barbs(v_cnt, v_pos, m_arrow_size, h_tip, h_barbs)) {
				svg_write_barbs(h_barbs, h_tip, dt_writer);
			}
		}
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeRect::svg_write(DataWriter const& dt_writer) const
	{
		svg_dt_write("<rect ", dt_writer);
		svg_dt_write(m_pos, "x", "y", dt_writer);
		svg_dt_write(m_vec[0], "width", "height", dt_writer);
		svg_dt_write(m_fill_color, "fill", dt_writer);
		ShapeStroke::svg_write(dt_writer);
		svg_dt_write("/>" SVG_NEW_LINE, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeRRect::svg_write(DataWriter const& dt_writer) const
	{
		svg_dt_write("<rect ", dt_writer);
		svg_dt_write(m_pos, "x", "y", dt_writer);
		svg_dt_write(m_vec[0], "width", "height", dt_writer);
		if (std::round(m_corner_rad.x) != 0.0f && std::round(m_corner_rad.y) != 0.0f) {
			svg_dt_write(m_corner_rad, "rx", "ry", dt_writer);
		}
		svg_dt_write(m_fill_color, "fill", dt_writer);
		ShapeStroke::svg_write(dt_writer);
		svg_dt_write("/>", dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeStroke::svg_write_stroke(DataWriter const& dt_writer) const
	{
		svg_dt_write(m_stroke_color, "stroke", dt_writer);
		svg_dt_write(m_dash_style, m_dash_patt, m_stroke_width, dt_writer);
		svg_dt_write(m_stroke_width, "stroke-width", dt_writer);
		if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT })) {
			svg_dt_write("stroke-linecap=\"butt\" ", dt_writer);
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND })) {
			svg_dt_write("stroke-linecap=\"round\" ", dt_writer);
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE })) {
			svg_dt_write("stroke-linecap=\"square\" ", dt_writer);
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE })) {
			//svg_dt_write("stroke-linecap=\"???\" ", dt_writer);
		}
		if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
			svg_dt_write("stroke-linejoin=\"bevel\" ", dt_writer);
		}
		else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
			m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
			svg_dt_write("stroke-linejoin=\"miter\" ", dt_writer);
			svg_dt_write(m_join_miter_limit, "stroke-miterlimit", dt_writer);
		}
		else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
			svg_dt_write("stroke-linejoin=\"round\" ", dt_writer);
		}
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
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
		// dy = ���̍s�̃q�b�g�e�X�g���g���b�N�X�̍��� - �t�H���g�̑傫�� �~ (�f�Z���g �� �P�ʑ傫��) �ƂȂ�, �͂�.
		//IDWriteFontCollection* fonts;
		//m_dw_layout->GetFontCollection(&fonts);
		//IDWriteFontFamily* family;
		//fonts->GetFontFamily(0, &family);
		//IDWriteFont* font;
		//family->GetFont(0, &font);
		//DWRITE_FONT_METRICS metrics;
		//font->GetMetrics(&metrics);
		//const double descent = m_font_size * ((static_cast<double>(metrics.descent)) / metrics.designUnitsPerEm);

		if (is_opaque(m_fill_color) || is_opaque(m_stroke_color)) {
			ShapeRect::svg_write(dt_writer);
		}
		// ������S�̂̑������w�肷�邽�߂� g �^�O���J�n����.
		svg_dt_write("<g ", dt_writer);
		// ���̂̐F����������.
		svg_dt_write(m_font_color, "fill", dt_writer);
		// ���̖�����������.
		svg_dt_write("font-family=\"", dt_writer);
		svg_dt_write(m_font_family, wchar_len(m_font_family), dt_writer);
		svg_dt_write("\" ", dt_writer);
		// ���̂̑傫������������.
		svg_dt_write(m_font_size, "font-size", dt_writer);
		// ���̂̕��̐L�k����������.
		const int32_t stretch = static_cast<int32_t>(m_font_stretch);
		svg_dt_write(SVG_STRETCH[stretch], "font-stretch", dt_writer);
		// ���̂̌`������������.
		const int32_t style = static_cast<int32_t>(m_font_style);
		svg_dt_write(SVG_STYLE[style], "font-style", dt_writer);
		// ���̂̑�������������.
		const uint32_t weight = static_cast<uint32_t>(m_font_weight);
		svg_dt_write(weight, "font-weight", dt_writer);
		svg_dt_write("none", "stroke", dt_writer);
		svg_dt_write(">" SVG_NEW_LINE, dt_writer);
		// ���̂�\�����鍶��ʒu�ɗ]����������.
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
			// �������\�����鐂���Ȃ��炵�ʒu�����߂�.
			const double dy = static_cast<double>(m_dw_line_metrics[i].baseline);
			// ���������������.
			tx_svg_dt_write(t, t_len, px + qx, py + qy, dy, dt_writer);
		}
		svg_dt_write("</g>" SVG_NEW_LINE, dt_writer);
	}

}