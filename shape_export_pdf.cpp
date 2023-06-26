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
	// ��邵���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	static size_t export_pdf_arrow(const float width, const D2D1_COLOR_F& color, const ARROW_STYLE style, const D2D1_CAP_STYLE cap, const D2D1_LINE_JOIN join, const float join_limit, const D2D1_SIZE_F sheet_size, const D2D1_POINT_2F barb[], const D2D1_POINT_2F tip, DataWriter const& dt_writer);
	// �X�g���[�N���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	static size_t export_pdf_stroke(const float width, const D2D1_COLOR_F& color, const D2D1_CAP_STYLE& cap, const D2D1_DASH_STYLE dash, const DASH_PAT& patt, const D2D1_LINE_JOIN join, const float miter_limit, const DataWriter& dt_writer);
	// PDF �̃p�X�`�施�߂𓾂�.
	template <bool C> static bool export_pdf_path_cmd(const float width, const D2D1_COLOR_F& stroke,	const D2D1_COLOR_F& fill, wchar_t cmd[4]);

	// ��邵���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// width	���E�g�̑���
	// stroke	���E�g�̐F
	// style	��邵�̌`��
	// cap	��邵�̕Ԃ��̌`��
	// join	��邵�̐�[�̌`��
	// join_limit	��萧���l
	// sheet_size	�p���̑傫��
	// barb[]	���̕Ԃ��̓_
	// tip	���̐�[�̓_
	// dt_writer	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
	static size_t export_pdf_arrow(const float width, const D2D1_COLOR_F& stroke, const ARROW_STYLE style, const D2D1_CAP_STYLE cap, const D2D1_LINE_JOIN join, const float join_limit, const D2D1_SIZE_F sheet_size, const D2D1_POINT_2F barb[], const D2D1_POINT_2F tip, DataWriter const& dt_writer)
	{
		if (equal(width, 0.0f) || !is_opaque(stroke)) {
			return 0;
		}
		wchar_t buf[1024];
		size_t len = 0;

		len += export_pdf_stroke(width, stroke, cap, D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID, DASH_PAT{}, join, join_limit, dt_writer);
		// �����ɖ߂�.
		len += dt_writer.WriteString(L"[ ] 0 d\n");
		if (style == ARROW_STYLE::ARROW_FILLED) {
			swprintf_s(buf,
				L"%f %f %f rg\n",
				stroke.r, stroke.g, stroke.b);
			len += dt_writer.WriteString(buf);
		}
		const double b0x = barb[0].x;
		const double b0y = barb[0].y;
		const double tx = tip.x;
		const double ty = tip.y;
		const double b1x = barb[1].x;
		const double b1y = barb[1].y;
		const double sh = sheet_size.height;
		swprintf_s(buf,
			L"%f %f m %f %f l %f %f l\n",
			b0x, -b0y + sh,
			tx, -ty + sh,
			b1x, -b1y + sh
		);
		len += dt_writer.WriteString(buf);
		if (style == ARROW_STYLE::ARROW_OPENED) {
			len += dt_writer.WriteString(L"S\n");
		}
		else if (style == ARROW_STYLE::ARROW_FILLED) {
			len += dt_writer.WriteString(L"b\n");	// b �̓p�X����� (B �͕�����) �h��Ԃ�.
		}
		return len;
	}

	// PDF �̃p�X�`�施�߂𓾂�.
	// C	�p�X�����Ȃ� true, �J�����܂܂Ȃ� false
	// width	���g�̑���
	// stroke	���g�̐F
	// fill	�h��Ԃ��F
	// cmd[4]	�p�X�`�施��
	// �߂�l	���߂�����ꂽ�Ȃ� true, �Ȃ���� false
	template <bool C>	// �p�X�����Ȃ� true, �J�����܂܂Ȃ� false
	static bool export_pdf_path_cmd(const float width, const D2D1_COLOR_F& stroke, const D2D1_COLOR_F& fill, wchar_t cmd[4])
	{
		// �p�X�`�施��
		// B* = �p�X��h��Ԃ��āA�X�g���[�N���`�悷�� (���K��)
		// b* = B* �Ɠ��������A�`��O�Ƀp�X�����
		// S = �p�X���X�g���[�N�ŕ`��
		// s = ���݂̃p�X������� (�J�n�_�܂ł𒼐��łȂ�)�A�X�g���[�N�ŕ`��
		// f* = ���K�����g�p���ăp�X��h��Ԃ�, �p�X�͎����I�ɕ�����.

		// ���g���\�������Ȃ�,
		if (!equal(width, 0.0f) && is_opaque(stroke)) {
			if (is_opaque(fill)) {
				// B* = �p�X��h��Ԃ��āA�X�g���[�N���`�悷�� (���K��)
				// b* = B* �Ɠ��������A�`��O�Ƀp�X�����
				if constexpr (C) {
					memcpy(cmd, L"b*\n", sizeof(L"b*\n"));
				}
				else {
					memcpy(cmd, L"B*\n", sizeof(L"B*\n"));
				}
			}
			else {
				// S = �p�X���X�g���[�N�ŕ`��
				// s = ���݂̃p�X������� (�J�n�_�܂ł𒼐��łȂ�)�A�X�g���[�N�ŕ`��
				if constexpr (C) {
					memcpy(cmd, L"s\n", sizeof(L"s\n"));
				}
				else {
					memcpy(cmd, L"S\n", sizeof(L"S\n"));
				}
			}
		}
		else {
			if (is_opaque(fill)) {
				// f* = ���K�����g�p���ăp�X��h��Ԃ�, �p�X�͎����I�ɕ�����.
				memcpy(cmd, L"f*\n", sizeof(L"f*\n"));
			}
			else {
				return false;
			}
		}
		return true;
	}

	// �X�g���[�N���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// width	���E�g�̑���
	// color	���E�g�̐F
	// cap	�[�_�̌`��
	// dash	�j���̌`��
	// patt	�j���̔z�u
	// join	�����̌`��
	// miter_limit	��萧���l
	// dt_writer	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
	static size_t export_pdf_stroke(const float width, const D2D1_COLOR_F& color, const D2D1_CAP_STYLE& cap, const D2D1_DASH_STYLE dash, const DASH_PAT& patt, const D2D1_LINE_JOIN join, const float miter_limit, const DataWriter& dt_writer)
	{
		size_t len = 0;
		wchar_t buf[1024];

		// ���g�̑���
		swprintf_s(buf, L"%f w\n", width);
		len += dt_writer.WriteString(buf);

		// ���g�̐F
		swprintf_s(buf, 
			L"%f %f %f RG\n",	// RG �̓X�g���[�N�F�w��
			color.r, color.g, color.b);
		len += dt_writer.WriteString(buf);

		// ���g�̒[�̌`��
		if (cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
			len += dt_writer.WriteString(L"2 J\n");
		}
		else if (cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
			len += dt_writer.WriteString(L"1 J\n");
		}
		else {
			len += dt_writer.WriteString(L"0 J\n");
		}

		// ���̌����̌`��
		// �ʎ��
		if (equal(join, D2D1_LINE_JOIN_BEVEL)) {
			len += dt_writer.WriteString(L"2 j\n");
		}
		// �ۂ܂�
		else if (equal(join, D2D1_LINE_JOIN_ROUND)) {
			len += dt_writer.WriteString(L"1 j\n");
		}
		// PDF �ɂ͐�� (�}�C�^�[) �܂��͖ʎ�� (�x�x��) �����Ȃ�.
		else {
			//if (equal(m_stroke_join, D2D1_LINE_JOIN_MITER) ||
			//equal(m_stroke_join, D2D1_LINE_JOIN_MITER_OR_BEVEL)) {
			// D2D �̐�萧���Ƃ͈قȂ�̂Œ���.
			// ��萧���𒴂��镔��������Ƃ�, D2D �ł͒����������������f���؂��邪, 
			// PDF �� SVG �ł͂����Ȃ� Bevel join�@�ɂȂ�.
			swprintf_s(buf, 
				L"0 j\n"
				L"%f M\n",
				miter_limit);
			len += dt_writer.WriteString(buf);
		}

		// �j���̌`��
		// �Ō�̐��l�͔z�u (�j���p�^�[��) ��K�p����I�t�Z�b�g.
		// [] 0		| ����
		// [3] 0	| ***___ ***___
		// [2] 1	| *__**__**
		// [2 1] 0	| **_**_ **_
		// [3 5] 6	| __ ***_____***_____
		// [2 3] 11	| *___ **___ **___
		if (dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH) {
			// �j��
			swprintf_s(buf, 
				L"[ %f %f ] 0 d\n",
				patt.m_[0], patt.m_[1]);
			len += dt_writer.WriteString(buf);
		}
		else if (dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT) {
			// �_��
			swprintf_s(buf,
				L"[ %f %f ] 0 d\n",
				patt.m_[2], patt.m_[3]);
			len += dt_writer.WriteString(buf);
		}
		else if (dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT) {
			// ��_�j��
			swprintf_s(buf,
				L"[ %f %f %f %f ] 0 d\n",
				patt.m_[0], patt.m_[1], patt.m_[2], patt.m_[3]);
			len += dt_writer.WriteString(buf);
		}
		else if (dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
			// ��_�j��
			swprintf_s(buf,
				L"[ %f %f %f %f %f %f ] 0 d\n",
				patt.m_[0], patt.m_[1], patt.m_[2], patt.m_[3], patt.m_[4], patt.m_[5]);
			len += dt_writer.WriteString(buf);
		}
		else {
			// ����
			len += dt_writer.WriteString(
				L"[ ] 0 d\n");
		}
		return len;
	}

	//------------------------------
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
	//------------------------------
	size_t ShapeBezier::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		wchar_t cmd[4];
		if (!export_pdf_path_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
			return 0;
		}

		D2D1_BEZIER_SEGMENT b_seg;
		pt_add(m_start, m_lineto[0], b_seg.point1);
		pt_add(b_seg.point1, m_lineto[1], b_seg.point2);
		pt_add(b_seg.point2, m_lineto[2], b_seg.point3);

		wchar_t buf[1024];
		size_t len = 0;
		swprintf_s(buf,
			L"%% Bezier curve\n"
			L"%f %f %f rg\n",	// rg �̓X�g���[�N�ȊO�p
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);
		const double sh = sheet_size.height;
		const double sx = m_start.x;
		const double sy = m_start.y;
		const double b1x = b_seg.point1.x;
		const double b1y = b_seg.point1.y;
		const double b2x = b_seg.point2.x;
		const double b2y = b_seg.point2.y;
		const double b3x = b_seg.point3.x;
		const double b3y = b_seg.point3.y;
		swprintf_s(buf, 
			L"%f %f m %f %f %f %f %f %f c %s",
			sx, -sy + sh,
			b1x, -b1y + sh,
			b2x, -b2y + sh,
			b3x, -b3y + sh, cmd
		);
		len += dt_writer.WriteString(buf);
		if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			D2D1_POINT_2F barbs[3];
			bezi_get_pos_arrow(m_start, b_seg, m_arrow_size, barbs);
			len += export_pdf_arrow(m_stroke_width, m_stroke_color, m_arrow_style, m_arrow_cap, m_arrow_join, m_arrow_join_limit, sheet_size, barbs, barbs[2], dt_writer);
		}
		return len;
	}

	//------------------------------
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
	//------------------------------
	size_t ShapeLine::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		if (equal(m_stroke_width, 0.0f) || !is_opaque(m_stroke_color)) {
			return 0;
		}

		size_t len = 0;	// �������񂾃o�C�g��
		len += dt_writer.WriteString(
			L"% Line\n");

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);

		const double sx = m_start.x;
		const double sy = -static_cast<double>(m_start.y) + static_cast<double>(sheet_size.height);
		const double ex = static_cast<double>(m_start.x) + static_cast<double>(m_lineto.x);
		const double ey = -(static_cast<double>(m_start.y) + static_cast<double>(m_lineto.y)) + static_cast<double>(sheet_size.height);
		wchar_t buf[1024];
		swprintf_s(buf,
			L"%f %f m %f %f l S\n",
			sx, sy, ex, ey
		);
		len += dt_writer.WriteString(buf);

		if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			D2D1_POINT_2F barbs[3];
			if (line_get_pos_arrow(m_start, m_lineto, m_arrow_size, barbs, barbs[2])) {
				len += export_pdf_arrow(m_stroke_width, m_stroke_color, m_arrow_style, m_arrow_cap, m_arrow_join, m_arrow_join_limit, sheet_size, barbs, barbs[2], dt_writer);
			}
		}
		return len;
	}

	//------------------------------
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
	//------------------------------
	size_t ShapePoly::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		wchar_t cmd[4];
		if (m_end == D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED) {
			if (!export_pdf_path_cmd<true>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
				return 0;
			}
		}
		else {
			if (!export_pdf_path_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
				return 0;
			}
		}
		size_t len = 0;
		len += dt_writer.WriteString(
			L"% Polyline\n");
		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);

		wchar_t buf[1024];
		swprintf_s(buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);

		// ��邵���Ȃ�Ό�ŕK�v�ɂȂ�̂�, �܂���̊e�_�����߂Ȃ���o�͂���.
		const size_t p_cnt = m_lineto.size() + 1;	// �܂���e�_�̐�
		D2D1_POINT_2F p[N_GON_MAX];	// �܂���e�_�̔z��
		p[0] = m_start;
		const double sx = p[0].x;
		const double sy = -static_cast<double>(p[0].y) + static_cast<double>(sheet_size.height);
		swprintf_s(buf, L"%f %f m\n", sx, sy);
		len += dt_writer.WriteString(buf);
		for (size_t i = 1; i < p_cnt; i++) {
			pt_add(p[i - 1], m_lineto[i - 1], p[i]);
			const double px = p[i].x;
			const double py = -static_cast<double>(p[i].y) + static_cast<double>(sheet_size.height);
			swprintf_s(buf, L"%f %f l\n", px, py);
			len += dt_writer.WriteString(buf);
		}
		len += dt_writer.WriteString(cmd);

		if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			D2D1_POINT_2F tip;
			D2D1_POINT_2F barb[2];
			if (poly_get_pos_arrow(p_cnt, p, m_arrow_size, barb, tip)) {
				len += export_pdf_arrow(m_stroke_width, m_stroke_color, m_arrow_style, m_arrow_cap, m_arrow_join, m_arrow_join_limit, sheet_size, barb, tip, dt_writer);
			}
		}
		return len;
	}

	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	size_t ShapeEllipse::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		wchar_t cmd[4];	// �p�X�`�施��
		if (!export_pdf_path_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
			return 0;
		}

		const double sh = sheet_size.height;
		constexpr double a = 4.0 * (M_SQRT2 - 1.0) / 3.0;
		const double rx = 0.5 * m_lineto.x;
		const double ry = 0.5 * m_lineto.y;
		const double cx = m_start.x + rx;
		const double cy = m_start.y + ry;

		size_t len = 0;
		len += dt_writer.WriteString(L"% Ellipse\n");

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);

		wchar_t buf[1024];
		swprintf_s(buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f m\n"
			L"%f %f %f %f %f %f c\n",
			cx + rx, -(cy) + sh,
			cx + rx, -(cy + a * ry) + sh,
			cx + a * rx, -(cy + ry) + sh,
			cx, -(cy + ry) + sh
		);
		len += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f %f %f %f %f c\n",
			cx - a * rx, -(cy + ry) + sh,
			cx - rx, -(cy + a * ry) + sh,
			cx - rx, -(cy) + sh
		);
		len += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f %f %f %f %f c\n",
			cx - rx, -(cy - a * ry) + sh,
			cx - a * rx, -(cy - ry) + sh,
			cx, -(cy - ry) + sh
		);
		len += dt_writer.WriteString(buf);

		swprintf_s(buf,
			L"%f %f %f %f %f %f c %s",
			cx + a * rx, -(cy - ry) + sh,
			cx + rx, -(cy - a * ry) + sh,
			cx + rx, -(cy) + sh,
			cmd
		);
		len += dt_writer.WriteString(buf);
		return len;
	}

	//------------------------------
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
	//------------------------------
	size_t ShapeOblong::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		wchar_t cmd[4];	// �p�X�`�施��
		if (!export_pdf_path_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
			return 0;
		}
		size_t len = 0;
		len += dt_writer.WriteString(
			L"% Rectangle\n");

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);

		wchar_t buf[1024];
		swprintf_s(buf,
			L"%f %f %f rg\n",	// rg = �h��Ԃ��F
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);
		const double sx = m_start.x;
		const double sy = -static_cast<double>(m_start.y) + static_cast<double>(sheet_size.height);
		const double px = m_lineto.x;
		const double py = -static_cast<double>(m_lineto.y);
		swprintf_s(buf,
			L"%f %f %f %f re %s",
			sx, sy, px, py, cmd
		);
		len += dt_writer.WriteString(buf);
		return len;
	}

	//------------------------------
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
	//------------------------------
	size_t ShapeRRect::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		wchar_t cmd[4];	// �p�X�`�施��
		if (!export_pdf_path_cmd<false>(m_stroke_width, m_stroke_color, m_fill_color, cmd)) {
			return 0;
		}

		wchar_t buf[1024];
		size_t len = 0;
		len += dt_writer.WriteString(
			L"% Rounded Rectangle\n");
		// �h��Ԃ��F
		swprintf_s(buf,
			L"%f %f %f rg\n",
			m_fill_color.r, m_fill_color.g, m_fill_color.b
		);
		len += dt_writer.WriteString(buf);

		len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);

		constexpr double a = 4.0 * (M_SQRT2 - 1.0) / 3.0;	// �x�W�F�ł��~���ߎ�����W��
		const double sh = sheet_size.height;	// D2D ���W�� PDF ���[�U�[��Ԃ֕ϊ����邽��
		const double rx = (m_lineto.x >= 0.0f ? m_corner_radius.x : -m_corner_radius.x);	// ���~�� x �����̔��a
		const double ry = (m_lineto.y >= 0.0f ? m_corner_radius.y : -m_corner_radius.y);	// ���~�� y �����̔��a

		// ��ӂ̎n�_.
		const double sx = m_start.x;
		const double sy = m_start.y;
		swprintf_s(buf,
			L"%f %f m\n",
			sx + rx, -(sy) + sh
		);
		len += dt_writer.WriteString(buf);

		// ��ӂƉE��̊p��`��.
		const double px = static_cast<double>(m_lineto.x);
		const double py = static_cast<double>(m_lineto.y);
		double cx = sx + px - rx;	// �p�ۂ̒��S�_ x
		double cy = sy + ry;	// �p�ۂ̒��S�_ y
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f %f %f %f %f c\n",
			cx, -sy + sh,
			cx + a * rx, -(cy - ry) + sh,
			cx + rx, -(cy - a * ry) + sh,
			cx + rx, -(cy) + sh
		);
		len += dt_writer.WriteString(buf);

		// �E�ӂƉE���̊p��`��.
		cx = sx + px - rx;
		cy = sy + py - ry;
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f %f %f %f %f c\n",
			sx + px, -(cy) + sh,
			cx + rx, -(cy + a * ry) + sh,
			cx + a * rx, -(cy + ry) + sh,
			cx, -(cy + ry) + sh
		);
		len += dt_writer.WriteString(buf);

		//�@���ӂƍ����̊p��`��.
		cx = sx + rx;
		cy = sy + py - ry;
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f %f %f %f %f c\n",
			cx, -(sy + py) + sh,
			cx - a * rx, -(cy + ry) + sh,
			cx - rx, -(cy + a * ry) + sh,
			cx - rx, -cy + sh
		);
		len += dt_writer.WriteString(buf);

		// ���ӂƍ���̊p��`��.
		cx = sx + rx;
		cy = sy + ry;
		swprintf_s(buf,
			L"%f %f l\n"
			L"%f %f %f %f %f %f c %s",
			sx, -cy + sh,
			cx - rx, -(cy - a * ry) + sh,
			cx - a * rx, -(cy - ry) + sh,
			cx, -(cy - ry) + sh,
			cmd
		);
		len += dt_writer.WriteString(buf);
		return len;
	}

	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	size_t ShapeImage::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		// PDF �ł͕\���̑傫���̋K��l�� 1 x 1.
		// �����܂܂ł�, �摜�S�̂� 1 x 1 �Ƀ}�b�s���O�����.
		// ������ƕ\������ɂ�, �ϊ��s��ɑ傫�����w�肵, �g�傷��.
		// �\������ʒu��, ����łȂ����������w�肷��.
		wchar_t buf[1024];
		const double vw = m_view.width;
		const double vh = m_view.height;
		const double sx = m_start.x;
		const double sy = m_start.y;
		swprintf_s(buf,
			L"%% Image\n"
			L"q\n"
			L"%f 0 0 %f %f %f cm\n"
			L"/Image%d Do\n"
			L"Q\n",
			vw, vh, sx, -(sy + vh) + static_cast<double>(sheet_size.height),
			m_pdf_image_cnt
		);
		return dt_writer.WriteString(buf);
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

	static void export_cmap_subtable(const void* table_data, const size_t offset)
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

	static void export_cmap_table(const void* table_data, const UINT32 table_size, const void* table_context, const size_t offset)
	{
		//https://learn.microsoft.com/en-us/typography/opentype/spec/cmap
		//https://github.com/wine-mirror/wine/blob/master/dlls/dwrite/tests/font.c
		uint16_t version = get_uint16(table_data, 0);
		uint16_t numTables = get_uint16(table_data, 2);
		for (uint16_t i = 0; i < numTables; i++) {
			uint16_t platformID = get_uint16(table_data, 4ull + 8ull * i + 0);
			uint16_t encodingID = get_uint16(table_data, 4ull + 8ull * i + 2);
			size_t offset = get_uint32(table_data, 4ull + 8ull * i + 4);
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
	}

	//------------------------------
	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// dt_weiter	�f�[�^���C�^�[
	// �߂�l	�������񂾃o�C�g��
	//------------------------------
	size_t ShapeText::export_pdf(const D2D1_SIZE_F sheet_size, DataWriter const& dt_writer)
	{
		size_t len = ShapeRect::export_pdf(sheet_size, dt_writer);

		// ���̂̐F�������Ȃ牽�����Ȃ�.
		if (!is_opaque(m_font_color)) {
			return len;
		}

		// �e�L�X�g�t�H�[�}�b�g����Ȃ�쐬����.
		if (m_dwrite_text_layout == nullptr) {
			create_text_layout();
		}

		len += dt_writer.WriteString(
			L"% Text\n");
		double oblique = (m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE ? tan(0.349066) : 0.0);

		wchar_t buf[1024];	// �o�͗p�̃o�b�t�@

		// ������̐F�ƃt�H���g�����̖��O
		// rg = �F��Ԃ� DeviceRGB �ɐݒ肵�A�����ɐF��ݒ肷�� (�X�g���[�N�ȊO�p)
		// RG = �F��Ԃ� DeviceRGB �ɐݒ肵�A�����ɐF��ݒ肷�� (�X�g���[�N�p)
		// BT = BT ���߂��� ET ���߂̊ԂɋL�q���ꂽ�e�L�X�g�́A��̃e�L�X�g�I�u�W�F�N�g
		// Tr = �e�L�X�g�����_�����O���[�h��ݒ�. (0: �ʏ� (�h��Ԃ��̂�) [default])
		swprintf_s(buf,
			L"%f %f %f rg\n"
			L"%f %f %f RG\n"
			L"BT\n"
			L"/Font%d %f Tf\n"
			L"0 Tr\n",
			m_font_color.r, m_font_color.g, m_font_color.b,
			m_font_color.r, m_font_color.g, m_font_color.b,
			m_pdf_text_cnt, m_font_size
		);
		len += dt_writer.WriteString(buf);


		IDWriteFontFace3* face;	// ����
		get_font_face(face);
		/*
		const void* table_data;
		UINT32 table_size;
		void* table_context;
		BOOL exists;
		face->TryGetFontTable(DWRITE_MAKE_OPENTYPE_TAG('c', 'm', 'a', 'p'), &table_data, &table_size, &table_context, &exists);
		face->ReleaseFontTable(table_context);
		*/

		for (uint32_t i = 0; i < m_dwrite_test_cnt; i++) {
			const wchar_t* t = m_text + m_dwrite_test_metrics[i].textPosition;	// �s�̐擪�������w���|�C���^�[
			const uint32_t t_len = m_dwrite_test_metrics[i].length;	// �s�̕�����

			// Tm = �e�L�X�g��Ԃ��烆�[�U�[��Ԃւ̕ϊ��s���ݒ�
			const double sx = m_start.x;	// �n�_
			const double sy = m_start.y;	// �n�_
			const double pw = m_text_padding.width;	// ���]���̕�
			const double ph = m_text_padding.height;	// ���]���̍���
			const double left = m_dwrite_test_metrics[i].left;
			const double top = m_dwrite_test_metrics[i].top;
			const double bl = m_dwrite_line_metrics[0].baseline;
			swprintf_s(buf,
				L"1 0 %f 1 %f %f Tm\n",
				oblique, sx + pw + left, -(sy + ph + top + bl) + static_cast<double>(sheet_size.height));
			len += dt_writer.WriteString(buf);

			// ���������������.

			// wchar_t �� GID �ɕϊ����ď����o��.
			const auto utf32{ text_utf16_to_utf32(t, t_len) };	// UTF-32 ������
			const auto u_len = std::size(utf32);	// UTF-32 ������̒���
			std::vector<uint16_t> gid(u_len);	// �O���t���ʎq
			face->GetGlyphIndices(std::data(utf32), static_cast<UINT32>(u_len), std::data(gid));
			len += dt_writer.WriteString(L"<");
			for (uint32_t j = 0; j < u_len; j++) {
				if (gid[j] == 0) {
					continue;
				}
				swprintf_s(buf, L"%04x", gid[j]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"> Tj\n");

			/*
			// wchar_t �� UTF-32 �ɕϊ����ď����o��.
			len += dt_writer.WriteString(L"% UTF-32\n<");
			std::vector<uint32_t> utf32{ text_utf16_to_utf32(t, t_len) };
			for (int i = 0; i < utf32.size(); i++) {
				swprintf_s(buf, L"%06x", utf32[i]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"> Tj\n");
			*/

			/*
			// wchar_t �� CID �ɕϊ����ď����o��.
			len += dt_writer.WriteString(L"% CID\n<");
			std::vector<uint32_t> utf32{ text_utf16_to_utf32(t, t_len) };
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
			std::vector<uint8_t> mb_text(mb_len);	// �}���`�o�C�g������
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

		// ET = BT ���߂��� ET ���߂̊ԂɋL�q���ꂽ�e�L�X�g�́A��̃e�L�X�g�I�u�W�F�N�g
		len += dt_writer.WriteString(L"ET\n");
		return len;
	}

	size_t ShapeRuler::export_pdf(const D2D1_SIZE_F sheet_size, const DataWriter& dt_writer)
	{
		const bool exist_stroke = (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color));
		// ���E�g�̑������F���Ȃ�, ���h��Ԃ��F���Ȃ��Ȃ璆�f����.
		if (!exist_stroke && !is_opaque(m_fill_color)) {
			return 0;
		}

		if (m_dwrite_text_format == nullptr) {
			create_text_format();
		}
		constexpr wchar_t D[10] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9' };
		wchar_t buf[1024];
		size_t len = 0;
		IDWriteFontFace3* face;	// ����
		get_font_face(face);
		std::vector utf32{ text_utf16_to_utf32(D, 10) };	// UINT-32 ������
		uint16_t gid[10];	// �O���t���ʎq
		face->GetGlyphIndices(std::data(utf32), 10, gid);
		DWRITE_FONT_METRICS f_met;	// ���̂̌v��
		face->GetMetrics(&f_met);
		int32_t g_adv[10];	// �O���t���Ƃ̕�
		face->GetDesignGlyphAdvances(10, gid, g_adv);
		face->Release();

		const double f_size = m_font_size;	// ���̂̑傫��
		const double f_asc = f_met.ascent;
		const double f_des = f_met.descent;
		const double f_gap = f_met.lineGap;
		const double f_upe = f_met.designUnitsPerEm;
		const double l_height = f_size * (f_asc + f_des + f_gap) / f_upe;	// �s�̍���
		const double b_line = f_size * (f_asc) / f_upe;	// (�����̏�[�����) �x�[�X���C���܂ł̋���

		if (is_opaque(m_fill_color)) {

			// �h��Ԃ��F���s�����ȏꍇ,
			// ���`��h��Ԃ�.
			const double sx = m_start.x;
			const double sy = m_start.y;
			const double px = m_lineto.x;
			const double py = m_lineto.y;
			swprintf_s(buf,
				L"%% Ruler\n"
				L"%f %f %f rg\n"
				L"%f %f %f %f re\n"
				L"f*\n",
				m_fill_color.r, m_fill_color.g, m_fill_color.b,
				sx, -(sy) + static_cast<double>(sheet_size.height), px, -py
			);
			len += dt_writer.WriteString(buf);
		}
		if (exist_stroke) {

			// ���g�̐F���s�����ȏꍇ,
			const double g_len = m_grid_base + 1.0;	// ����̑傫��
			const bool w_ge_h = fabs(m_lineto.x) >= fabs(m_lineto.y);	// ������蕝�̕����傫��
			const double to_x = (w_ge_h ? m_lineto.x : m_lineto.y);	// �傫�����̒l�� x
			const double to_y = (w_ge_h ? m_lineto.y : m_lineto.x);	// ���������̒l�� y
			const double intvl_x = to_x >= 0.0 ? g_len : -g_len;	// �ڐ���̊Ԋu
			const double intvl_y = min(f_size, g_len);	// �ڐ���̊Ԋu
			const double x0 = (w_ge_h ? m_start.x : m_start.y);
			const double y0 = static_cast<double>(w_ge_h ? m_start.y : m_start.x) + to_y;
			const double y1 = y0 - (to_y >= 0.0 ? intvl_y : -intvl_y);
			const double y1_5 = y0 - 0.625 * (to_y >= 0.0 ? intvl_y : -intvl_y);
			const double y2 = y1 - (to_y >= 0.0 ? f_size : -f_size);
			/*
			DWRITE_PARAGRAPH_ALIGNMENT p_align;
			if (w_ge_h) {
				// ���̂ق����傫���ꍇ,
				// ������ 0 �ȏ�̏ꍇ���悹�A�Ȃ��ꍇ��悹��i���̂��낦�Ɋi�[����.
				// �������z�u������`�������� (���̂̑傫���Ɠ���) ����,
				// DWRITE_PARAGRAPH_ALIGNMENT ��, �t�̌��ʂ������炷.
				p_align = (m_pos[0].y >= 0.0f ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			}
			else {
				// �c�̂ق����������ꍇ,
				// ���i��i���̂��낦�Ɋi�[����.
				p_align = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			}
			*/
			len += export_pdf_stroke(1.0f, m_stroke_color, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_DASH_STYLE_SOLID, DASH_PAT{}, D2D1_LINE_JOIN_MITER_OR_BEVEL, JOIN_MITER_LIMIT_DEFVAL, dt_writer);

			const double sh = sheet_size.height;
			const uint32_t k = static_cast<uint32_t>(floor(to_x / intvl_x));	// �ڐ���̐�
			for (uint32_t i = 0; i <= k; i++) {

				// ����̑傫�����Ƃɖڐ����\������.
				const double x = x0 + i * intvl_x;
				const double px = (w_ge_h ? x : y0);
				const double py = (w_ge_h ? y0 : x);
				//const D2D1_POINT_2F p{
				//	w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y0),
				//	w_ge_h ? static_cast<FLOAT>(y0) : static_cast<FLOAT>(x)
				//};
				const auto y = ((i % 5) == 0 ? y1 : y1_5);
				const double qx = (w_ge_h ? x : y);
				const double qy = (w_ge_h ? y : x);
				//const D2D1_POINT_2F q{
				//	w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y),
				//	w_ge_h ? static_cast<FLOAT>(y) : static_cast<FLOAT>(x)
				//};
				swprintf_s(buf,
					L"%f %f m %f %f l\n"
					L"S\n",
					px, -py + sh,
					qx, -qy + sh
				);
				len += dt_writer.WriteString(buf);
			}
			// ������\������̂ɗ����K�v.
			// rg = �h��Ԃ��F, RG = ���E�g�̐F
			swprintf_s(buf,
				L"%f %f %f rg\n"
				L"%f %f %f RG\n"
				L"BT\n"
				L"/Font%d %f Tf\n"
				L"0 Tr\n",
				m_stroke_color.r, m_stroke_color.g, m_stroke_color.b,
				m_stroke_color.r, m_stroke_color.g, m_stroke_color.b,
				m_pdf_text_cnt, m_font_size
			);
			len += dt_writer.WriteString(buf);
			//float before = 0;
			for (uint32_t i = 0; i <= k; i++) {
				// ����̑傫�����Ƃɖڐ����\������.
				const double x = x0 + i * intvl_x;
				//const D2D1_POINT_2F q{
				//	w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y1),
				//	w_ge_h ? static_cast<FLOAT>(y1) : static_cast<FLOAT>(x)
				//};
				const double qx = (w_ge_h ? x : y1);
				const double qy = (w_ge_h ? y1 : x);
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
				const double f_upe = f_met.designUnitsPerEm;
				const double w = f_size * g_adv[i % 10] / f_upe;
				const double rx = (
					w_ge_h ?
					// �ڐ���̈ʒu���玚�̂̕��̔����������炵, �����̊�_�Ƃ���.
					x - w / 2 :
					// �ڐ���̈ʒu����, ���̂̔����̑傫���������炵, �����̒����ʒu������,
					// ���̈ʒu���玚�̂̕��̔����������炵��, �����̊�_�Ƃ���.
					(m_lineto.x >= 0.0f ? qx - f_size / 2 - w / 2 : qx + f_size / 2 - w / 2)
				);
				const double ry = (
					w_ge_h ?
					// �ڐ���̈ʒu����, ���̑傫���̔����������炵, ����ɍs�̍����̔����������炵,
					// �����̏�ʒu�����߂�����, ���̈ʒu����x�[�X���C���̋����������炵,
					// �����̊�_�Ƃ���.
					(m_lineto.y >= 0.0f ? qy - f_size / 2 - l_height / 2 + b_line : qy + f_size / 2 - l_height / 2 + b_line) :
					// �ڐ���̈ʒu����, �s�̍����̔����������炵��, �����̏�ʒu������,
					// ���̈ʒu����x�[�X���C���܂ł̋���������, �����̊�_�Ƃ���.
					qy - l_height / 2 + b_line
				);
				swprintf_s(buf,
					L"1 0 0 1 %f %f Tm <%04x> Tj\n",
					rx, -ry + sh, gid[i % 10]);
				len += dt_writer.WriteString(buf);
			}
			len += dt_writer.WriteString(L"ET\n");
		}
		return len;
	}

	size_t ShapeSheet::export_pdf(const D2D1_COLOR_F& background, DataWriter const& dt_writer)
	{
		wchar_t buf[1024];	// PDF
		size_t len = 0;

		// PDF �̓A���t�@�ɑΉ����ĂȂ��̂�, �w�i�F�ƍ�����, �p����h��Ԃ�.
		const double page_a = m_sheet_color.a;
		const double page_r = page_a * m_sheet_color.r + (1.0 - page_a) * background.r;
		const double page_g = page_a * m_sheet_color.g + (1.0 - page_a) * background.g;
		const double page_b = page_a * m_sheet_color.b + (1.0 - page_a) * background.b;
		// re = ���`, f = ������h��Ԃ�.
		// cm = �ϊ��s�� (�p���̒��ł͓��]���̕����s�ړ�)
		swprintf_s(buf,
			L"%f %f %f rg\n"
			L"0 0 %f %f re\n"
			L"f\n"
			L"1 0 0 1 %f %f cm\n",
			min(max(page_r, 0.0), 1.0),
			min(max(page_g, 0.0), 1.0),
			min(max(page_b, 0.0), 1.0),
			m_sheet_size.width,
			m_sheet_size.height,
			m_sheet_padding.left,
			-m_sheet_padding.top
		);
		len += dt_writer.WriteString(buf);


		if (m_grid_show == GRID_SHOW::FRONT || m_grid_show == GRID_SHOW::HIDE) {
			// �}�`���o��
			const D2D1_SIZE_F sheet_size = m_sheet_size;
			for (const auto s : m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				len += s->export_pdf(sheet_size, dt_writer);
			}
		}

		if (m_grid_show == GRID_SHOW::FRONT || m_grid_show == GRID_SHOW::BACK) {
			const float grid_base = m_grid_base;
			// PDF �̓A���t�@�ɑΉ����ĂȂ��̂�, �w�i�F, �p���F�ƍ�����.
			const double page_a = m_sheet_color.a;
			const double page_r = page_a * m_sheet_color.r + (1.0 - page_a) * background.r;
			const double page_g = page_a * m_sheet_color.g + (1.0 - page_a) * background.g;
			const double page_b = page_a * m_sheet_color.b + (1.0 - page_a) * background.b;
			const double grid_a = m_grid_color.a;
			const double grid_r = grid_a * m_grid_color.r + (1.0f - grid_a) * page_r;
			const double grid_g = grid_a * m_grid_color.g + (1.0f - grid_a) * page_g;
			const double grid_b = grid_a * m_grid_color.b + (1.0f - grid_a) * page_b;
			const GRID_EMPH grid_emph = m_grid_emph;
			const D2D1_POINT_2F grid_offset = m_grid_offset;
			// �p���̑傫��������]���̑傫��������.
			const auto grid_w = m_sheet_size.width - (m_sheet_padding.left + m_sheet_padding.right);	// �����`���̈�̑傫��
			const auto grid_h = m_sheet_size.height - (m_sheet_padding.top + m_sheet_padding.bottom);	// �����`���̈�̑傫��

			const FLOAT stroke_w = 1.0;	// ����̑���
			D2D1_POINT_2F h_start, h_end;	// ���̕���̊J�n�E�I���ʒu
			D2D1_POINT_2F v_start, v_end;	// �c�̕���̊J�n�E�I���ʒu
			v_start.y = 0.0f;
			h_start.x = 0.0f;
			v_end.y = grid_h;
			h_end.x = grid_w;
			const double grid_len = max(grid_base + 1.0, 1.0);
			len += dt_writer.WriteString(
				L"% Grid Lines\n");
			len += export_pdf_stroke(
				0.0f,
				D2D1_COLOR_F{ static_cast<FLOAT>(grid_r), static_cast<FLOAT>(grid_g), static_cast<FLOAT>(grid_b), 1.0f },
				D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID, DASH_PAT{},
				D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL, JOIN_MITER_LIMIT_DEFVAL, dt_writer);

			// �����ȕ����\������.
			float sw;
			double x;
			for (uint32_t i = 0;
				(x = round((grid_len * i + grid_offset.x) / PT_ROUND) * PT_ROUND) <= grid_w; i++) {
				if (grid_emph.m_gauge_2 != 0 && (i % grid_emph.m_gauge_2) == 0) {
					sw = 2.0F * stroke_w;
				}
				else if (grid_emph.m_gauge_1 != 0 && (i % grid_emph.m_gauge_1) == 0) {
					sw = stroke_w;
				}
				else {
					sw = 0.5F * stroke_w;
				}
				v_start.x = v_end.x = static_cast<FLOAT>(x);
				const double sx = v_start.x;
				const double sy = v_start.y;
				const double ex = v_end.x;
				const double ey = v_end.y;
				const double ph = m_sheet_size.height;
				swprintf_s(buf,
					L"%f w %f %f m %f %f l S\n",
					sw,
					sx, -sy + ph,
					ex, -ey + ph
				);
				len += dt_writer.WriteString(buf);
			}
			// �����ȕ����\������.
			double y;
			for (uint32_t i = 0;
				(y = round((grid_len * i + grid_offset.y) / PT_ROUND) * PT_ROUND) <= grid_h; i++) {
				if (grid_emph.m_gauge_2 != 0 && (i % grid_emph.m_gauge_2) == 0) {
					sw = 2.0F * stroke_w;
				}
				else if (grid_emph.m_gauge_1 != 0 && (i % grid_emph.m_gauge_1) == 0) {
					sw = stroke_w;
				}
				else {
					sw = 0.5F * stroke_w;
				}
				h_start.y = h_end.y = static_cast<FLOAT>(y);
				const double sx = h_start.x;
				const double sy = h_start.y;
				const double ex = h_end.x;
				const double ey = h_end.y;
				const double ph = m_sheet_size.height;
				swprintf_s(buf,
					L"%f w %f %f m %f %f l S\n",
					sw,
					sx, -sy + ph,
					ex, -ey + ph
				);
				len += dt_writer.WriteString(buf);
			}

		}

		if (m_grid_show == GRID_SHOW::BACK) {
			// �}�`���o��
			for (const auto s : m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				len += s->export_pdf(m_sheet_size, dt_writer);
			}
		}
		return len;
	}

	size_t ShapeGroup::export_pdf(const D2D1_SIZE_F sheet_size, const DataWriter& dt_writer)
	{
		size_t len = 0;
		for (Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			len += s->export_pdf(sheet_size, dt_writer);
		}
		return len;
	}

	size_t ShapeArc::export_pdf(const D2D1_SIZE_F sheet_size, const DataWriter& dt_writer)
	{
		if (!is_opaque(m_fill_color) && (equal(m_stroke_width, 0.0f) ||
			!is_opaque(m_stroke_color))) {
			return 0;
		}

		D2D1_POINT_2F start1{};
		D2D1_BEZIER_SEGMENT b_seg1{};
		alter_bezier(start1, b_seg1);

		D2D1_POINT_2F ctr{};
		if (is_opaque(m_fill_color) || m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			get_pos_loc(LOCUS_TYPE::LOCUS_A_CENTER, ctr);
		}

		size_t len = 0;
		if (is_opaque(m_fill_color)) {
			// rg = �h��Ԃ��F
			// f* = ���K�����g�p���ăp�X��h��Ԃ��B
			// �p�X�͎����I�ɕ�����
			wchar_t buf[1024];
			const double sx = start1.x;
			const double sy = start1.y;
			const double b1x = b_seg1.point1.x;
			const double b1y = b_seg1.point1.y;
			const double b2x = b_seg1.point2.x;
			const double b2y = b_seg1.point2.y;
			const double b3x = b_seg1.point3.x;
			const double b3y = b_seg1.point3.y;
			const double sh = sheet_size.height;
			swprintf_s(buf,
				L"%f %f %f rg\n"
				L"%f %f m %f %f %f %f %f %f c %f %f l f*\n",
				m_fill_color.r, m_fill_color.g, m_fill_color.b,
				sx, -sy + sh,
				b1x, -b1y + sh,
				b2x, -b2y + sh,
				b3x, -b3y + sh,
				ctr.x, -ctr.y + sh
			);
			len += dt_writer.WriteString(buf);
		}
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			len += export_pdf_stroke(m_stroke_width, m_stroke_color, m_stroke_cap, m_stroke_dash, m_dash_pat, m_stroke_join, m_stroke_join_limit, dt_writer);
			// S = �p�X���X�g���[�N�ŕ`��
			// �p�X�͊J�����܂�.
			wchar_t buf[1024];
			const double sx = start1.x;
			const double sy = start1.y;
			const double b1x = b_seg1.point1.x;
			const double b1y = b_seg1.point1.y;
			const double b2x = b_seg1.point2.x;
			const double b2y = b_seg1.point2.y;
			const double b3x = b_seg1.point3.x;
			const double b3y = b_seg1.point3.y;
			const double sh = sheet_size.height;
			swprintf_s(buf,
				L"%f %f m %f %f %f %f %f %f c S\n",
				sx, -sy + sh,
				b1x, -b1y + sh,
				b2x, -b2y + sh,
				b3x, -b3y + sh
			);
			len += dt_writer.WriteString(buf);
			if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
				D2D1_POINT_2F arrow[3];
				arc_get_pos_arrow(m_lineto[0], ctr, m_radius, m_angle_start, m_angle_end, m_angle_rot, m_arrow_size, m_sweep_dir, arrow);
				len += export_pdf_arrow(m_stroke_width, m_stroke_color, m_arrow_style, m_arrow_cap, m_arrow_join, m_arrow_join_limit, sheet_size, arrow, arrow[2], dt_writer);
			}
		}
		return len;
	}
}