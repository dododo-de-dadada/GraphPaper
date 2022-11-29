//------------------------------
// Shape_line.cpp
// �����Ɩ�邵
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ��邵�� D2D1 �p�X�W�I���g�����쐬����.
	static void line_create_arrow_geom(ID2D1Factory3* const d_factory, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_vec, ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo);
	// ��邵�� D2D �X�g���[�N�������쐬����.
	static void line_create_arrow_style(ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, const D2D1_LINE_JOIN s_join_style, const double s_join_limit, ID2D1StrokeStyle** s_arrow_style);
	// ��邵�̐�[�ƕԂ��̈ʒu�����߂�.
	static bool line_get_arrow_pos(const D2D1_POINT_2F a_pos, const D2D1_POINT_2F a_vec, const ARROW_SIZE& a_size, D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept;
	// �������ʒu���܂ނ�, �������l�����Ĕ��肷��.
	static bool line_hit_test(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width, const CAP_STYLE& s_cap) noexcept;

	// ��邵�� D2D1 �p�X�W�I���g�����쐬����
	// d_factory	D2D �t�@�N�g���[
	// s_pos	���̊J�n�ʒu
	// d_vec	���̏I���ʒu�ւ̍���
	// style	��邵�̌`��
	// size	��邵�̐��@
	// geo	�쐬���ꂽ�p�X�W�I���g��
	static void line_create_arrow_geom(ID2D1Factory3* const d_factory, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_vec, ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo)
	{
		D2D1_POINT_2F barbs[2];	// ��邵�̕Ԃ��̒[�_
		D2D1_POINT_2F tip_pos;	// ��邵�̐�[�_
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (line_get_arrow_pos(s_pos, d_vec, a_size, barbs, tip_pos)) {
			// �W�I���g���p�X���쐬����.
			winrt::check_hresult(d_factory->CreatePathGeometry(geo));
			winrt::check_hresult((*geo)->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(
				barbs[0],
				style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
				: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW
			);
			sink->AddLine(tip_pos);
			sink->AddLine(barbs[1]);
			sink->EndFigure(
				style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED
				: D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN
			);
			winrt::check_hresult(sink->Close());
			sink = nullptr;
		}
	}

	// ��邵�� D2D �X�g���[�N�������쐬����.
	static void line_create_arrow_style(ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, const D2D1_LINE_JOIN s_join_style, const double s_join_limit, ID2D1StrokeStyle** s_arrow_style)
	{
		// ��邵�̔j���̌`���͂��Ȃ炸�\���b�h�Ƃ���.
		const D2D1_STROKE_STYLE_PROPERTIES s_prop{
			s_cap_style.m_start,	// startCap
			s_cap_style.m_end,	// endCap
			D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// dashCap
			s_join_style,	// lineJoin
			static_cast<FLOAT>(s_join_limit),	// miterLimit
			D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID,	// dashStyle
			0.0f	// dashOffset
		};
		winrt::check_hresult(
			d_factory->CreateStrokeStyle(s_prop, nullptr, 0, s_arrow_style)
		);
	}

	// ��邵�̐�[�ƕԂ��̈ʒu�����߂�.
	// a_pos	��̌�[�̈ʒu
	// a_vec	��̐�[�ւ̃x�N�g��
	// a_size	��邵�̐��@
	// barbs	�Ԃ��̈ʒu
	// tip		��[�̈ʒu
	static bool line_get_arrow_pos(const D2D1_POINT_2F a_pos, const D2D1_POINT_2F a_vec, const ARROW_SIZE& a_size, D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept
	{
		const auto a_len = std::sqrt(pt_abs2(a_vec));	// ��̒���
		if (a_len >= FLT_MIN) {
			get_arrow_barbs(a_vec, a_len, a_size.m_width, a_size.m_length, barbs);
			if (a_size.m_offset >= a_len) {
				// ��邵�̐�[
				tip = a_pos;
			}
			else {
				pt_mul_add(a_vec, 1.0 - a_size.m_offset / a_len, a_pos, tip);
			}
			pt_add(barbs[0], tip, barbs[0]);
			pt_add(barbs[1], tip, barbs[1]);
			return true;
		}
		return false;
	}

	// �������ʒu���܂ނ�, �������l�����Ĕ��肷��.
	// t_pos	���肷��ʒu
	// s_pos	�����̎n�[
	// e_pos	�����̏I�[
	// s_width	�����̑���
	// s_cap	�����̒[�̌`��
	// �߂�l	�܂ޏꍇ true
	static bool line_hit_test(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width, const CAP_STYLE& s_cap) noexcept
	{
		const double e_width = max(s_width * 0.5, 0.5);
		if (equal(s_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE })) {
			D2D1_POINT_2F d_vec;	// ���������̃x�N�g��
			pt_sub(e_pos, s_pos, d_vec);
			const double abs2 = pt_abs2(d_vec);
			pt_mul(
				abs2 >= FLT_MIN ? d_vec : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 >= FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				d_vec);
			const double dx = d_vec.x;
			const double dy = d_vec.y;
			const double ox = dy;
			const double oy = -dx;
			D2D1_POINT_2F e_side[4];
			pt_add(s_pos, -dx + ox, -dy + oy, e_side[0]);
			pt_add(s_pos, -dx - ox, -dy - oy, e_side[1]);
			pt_add(e_pos, dx - ox, dy - oy, e_side[2]);
			pt_add(e_pos, dx + ox, dy + oy, e_side[3]);
			return pt_in_poly(t_pos, 4, e_side);
		}
		else if (equal(s_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE })) {
			D2D1_POINT_2F d_vec;	// ���������̃x�N�g��
			pt_sub(e_pos, s_pos, d_vec);
			const double abs2 = pt_abs2(d_vec);
			pt_mul(
				abs2 >= FLT_MIN ? d_vec : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 >= FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				d_vec);
			const double dx = d_vec.x;
			const double dy = d_vec.y;
			const double ox = dy;
			const double oy = -dx;
			D2D1_POINT_2F e_side[6];
			pt_add(s_pos, ox, oy, e_side[0]);
			pt_add(s_pos, -dx, -dy, e_side[1]);
			pt_add(s_pos, -ox, -oy, e_side[2]);
			pt_add(e_pos, -ox, -oy, e_side[3]);
			pt_add(e_pos, dx, dy, e_side[4]);
			pt_add(e_pos, ox, oy, e_side[5]);
			return pt_in_poly(t_pos, 6, e_side);
		}
		else {
			if (equal(s_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND })) {
				if (pt_in_circle(t_pos, s_pos, e_width) || pt_in_circle(t_pos, e_pos, e_width)) {
					return true;
				}
			}
			D2D1_POINT_2F d_vec;	// �����x�N�g��
			pt_sub(e_pos, s_pos, d_vec);
			const double abs2 = pt_abs2(d_vec);
			if (abs2 >= FLT_MIN) {
				pt_mul(d_vec, e_width / sqrt(abs2), d_vec);
				const double ox = d_vec.y;
				const double oy = -d_vec.x;
				D2D1_POINT_2F e_side[4];
				pt_add(s_pos, ox, oy, e_side[0]);
				pt_add(s_pos, -ox, -oy, e_side[1]);
				pt_add(e_pos, -ox, -oy, e_side[2]);
				pt_add(e_pos, ox, oy, e_side[3]);
				return pt_in_poly(t_pos, 4, e_side);
			}
		}
		return false;
	}

	// �}�`��\������.
	// sh	�\������p��
	void ShapeLine::draw(ShapeSheet const& sh)
	{
		const D2D_UI& dx = sh.m_d2d;
		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(dx);
		}

		sh.m_color_brush->SetColor(m_stroke_color);
		const auto s_style = m_d2d_stroke_style.get();
		const auto s_width = m_stroke_width;

		D2D1_POINT_2F e_pos;
		pt_add(m_pos, m_vec[0], e_pos);
		dx.m_d2d_context->DrawLine(m_pos, e_pos, sh.m_color_brush.get(), s_width, s_style);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			if (m_d2d_arrow_style == nullptr) {
				line_create_arrow_style(dx.m_d2d_factory.get(), m_stroke_cap, m_join_style, m_join_limit, m_d2d_arrow_style.put());
			}
			if (m_d2d_arrow_geom == nullptr) {
				line_create_arrow_geom(dx.m_d2d_factory.get(), m_pos, m_vec[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
			}
			const auto a_geom = m_d2d_arrow_geom.get();
			if (m_d2d_arrow_geom != nullptr) {
				if (m_arrow_style == ARROW_STYLE::FILLED) {
					dx.m_d2d_context->FillGeometry(a_geom, sh.m_color_brush.get(), nullptr);
				}
				dx.m_d2d_context->DrawGeometry(a_geom, sh.m_color_brush.get(), s_width, m_d2d_arrow_style.get());
			}
		}
		if (is_selected()) {
			D2D1_POINT_2F mid;
			pt_mul_add(m_vec[0], 0.5, m_pos, mid);
			anc_draw_rect(m_pos, sh);
			anc_draw_rect(mid, sh);
			anc_draw_rect(e_pos, sh);
		}
	}

	// ��邵�̐��@�𓾂�.
	bool ShapeLine::get_arrow_size(ARROW_SIZE& val) const noexcept
	{
		val = m_arrow_size;
		return true;
	}

	// ��邵�̌`���𓾂�.
	bool ShapeLine::get_arrow_style(ARROW_STYLE& val) const noexcept
	{
		val = m_arrow_style;
		return true;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeLine::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		D2D1_POINT_2F e_pos;
		pt_add(m_pos, m_vec[0], e_pos);
		if (pt_in_anc(t_pos, e_pos)) {
			return ANC_TYPE::ANC_P0 + 1;
		}
		if (pt_in_anc(t_pos, m_pos)) {
			return ANC_TYPE::ANC_P0;
		}
		const float s_width = static_cast<float>(max(static_cast<double>(m_stroke_width), Shape::s_anc_len));
		if (line_hit_test(t_pos, m_pos, e_pos, s_width, m_stroke_cap)) {
			return ANC_TYPE::ANC_STROKE;
		}
		return ANC_TYPE::ANC_SHEET;
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// area_min	�͈͂̍���ʒu
	// area_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeLine::in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept
	{
		if (pt_in_rect(m_pos, area_min, area_max)) {
			D2D1_POINT_2F pos;
			pt_add(m_pos, m_vec[0], pos);
			return pt_in_rect(pos, area_min, area_max);
		}
		return false;
	}

	// ���������ړ�����.
	bool ShapeLine::move(const D2D1_POINT_2F d_vec) noexcept
	{
		if (ShapeStroke::move(d_vec)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l���邵�̐��@�Ɋi�[����.
	bool ShapeLine::set_arrow_size(const ARROW_SIZE& val) noexcept
	{
		if (!equal(m_arrow_size, val)) {
			m_arrow_size = val;
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l���邵�̌`���Ɋi�[����.
	bool ShapeLine::set_arrow_style(const ARROW_STYLE val) noexcept
	{
		if (m_arrow_style != val) {
			m_arrow_style = val;
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l������̂Ȃ��̃}�C�^�[�����Ɋi�[����.
	bool ShapeLine::set_join_limit(const float& val) noexcept
	{
		if (ShapeStroke::set_join_limit(val)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l������̂Ȃ��Ɋi�[����.
	bool ShapeLine::set_join_style(const D2D1_LINE_JOIN& val) noexcept
	{
		if (ShapeStroke::set_join_style(val)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����. 
	// val	�l
	// anc	�}�`�̕���
	// limit	���E���� (���̒��_�Ƃ̋��������̒l�����ɂȂ�Ȃ�, ���̒��_�Ɉʒu�ɍ��킹��)
	bool ShapeLine::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept
	{
		if (ShapeStroke::set_pos_anc(val, anc, limit, keep_aspect)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
	bool ShapeLine::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		if (ShapeStroke::set_pos_start(val)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l��[�̌`���Ɋi�[����.
	bool ShapeLine::set_stroke_cap(const CAP_STYLE& val) noexcept
	{
		if (ShapeStroke::set_stroke_cap(val)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// b_pos	�͂ޗ̈�̎n�_
	// b_vec	�͂ޗ̈�̏I�_�ւ̍���
	// s_sheet	����̑����l
	ShapeLine::ShapeLine(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet) :
		ShapeStroke::ShapeStroke(s_sheet),
		m_arrow_style(s_sheet->m_arrow_style),
		m_arrow_size(s_sheet->m_arrow_size),
		m_d2d_arrow_geom(nullptr),
		m_d2d_arrow_style(nullptr)
	{
		m_pos = b_pos;
		m_vec.resize(1, b_vec);
		m_vec.shrink_to_fit();
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	// dt_reader	�ǂݍ��ރf�[�^���[�_�[
	ShapeLine::ShapeLine(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader),
		m_d2d_arrow_style(nullptr),
		m_d2d_arrow_geom(nullptr)
	{
		m_arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadInt32());
		dt_read(m_arrow_size, dt_reader);
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapeLine::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);
		dt_writer.WriteInt32(static_cast<int32_t>(m_arrow_style));
		dt_write(m_arrow_size, dt_writer);
	}

	// �f�[�^���C�^�[�� PDF �X�g���[���̈ꕔ�Ƃ��ď�������.
	size_t ShapeLine::write_pdf(DataWriter const& dt_writer) const
	{
		size_t n = dt_write("%Line\n", dt_writer);
		n += write_pdf_stroke(dt_writer);

		char buf[1024];
		sprintf_s(buf, "%f %f m\n", m_pos.x, m_pos.y);
		n += dt_write(buf, dt_writer);
		sprintf_s(buf, "%f %f l\n", m_pos.x + m_vec[0].x, m_pos.y + m_vec[0].y);
		n += dt_write(buf, dt_writer);
		n += dt_write("S\n", dt_writer);
		if (m_arrow_style == ARROW_STYLE::OPENED || m_arrow_style == ARROW_STYLE::FILLED) {
			D2D1_POINT_2F barbs[3];
			if (line_get_arrow_pos(m_pos, m_vec[0], m_arrow_size, barbs, barbs[2])) {
				n += write_pdf_barbs(barbs, barbs[2], dt_writer);
			}
		}
		return n;
	}

	size_t ShapeLine::write_pdf_barbs(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const
	{
		char buf[1024];
		size_t n = 0;

		// �j���Ȃ��, �����ɖ߂�.
		if (m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH ||
			m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT ||
			m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
			n += dt_write("[ ] 0 d\n", dt_writer);
		}
		if (m_arrow_style == ARROW_STYLE::FILLED) {
			sprintf_s(buf, "%f %f %f rg\n", m_stroke_color.r, m_stroke_color.g, m_stroke_color.b);
			n += dt_write(buf, dt_writer);
		}
		sprintf_s(buf, "%f %f m\n", barbs[0].x, barbs[0].y);
		n += dt_write(buf, dt_writer);
		sprintf_s(buf, "%f %f l\n", tip_pos.x, tip_pos.y);
		n += dt_write(buf, dt_writer);
		sprintf_s(buf, "%f %f l\n", barbs[1].x, barbs[1].y);
		n += dt_write(buf, dt_writer);
		if (m_arrow_style == ARROW_STYLE::OPENED) {
			n += dt_write("S\n", dt_writer);
		}
		else if (m_arrow_style == ARROW_STYLE::FILLED) {
			n += dt_write("b\n", dt_writer);	// b �̓p�X����� (B �͕�����) �h��Ԃ�.
		}
		return n;
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeLine::write_svg(DataWriter const& dt_writer) const
	{
		D2D1_POINT_2F e_pos;

		pt_add(m_pos, m_vec[0], e_pos);
		dt_write_svg("<line ", dt_writer);
		dt_write_svg(m_pos, "x1", "y1", dt_writer);
		dt_write_svg(e_pos, "x2", "y2", dt_writer);
		write_svg_stroke(dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[2];
			D2D1_POINT_2F tip_pos;
			if (line_get_arrow_pos(m_pos, m_vec[0], m_arrow_size, barbs, tip_pos)) {
				write_svg_barbs(barbs, tip_pos, dt_writer);
			}
		}
	}

	// �����f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	// barbs	��邵�̗��[�̈ʒu [2]
	// tip_pos	��邵�̐�[�̈ʒu
	// a_style	��邵�̌`��
	// dt_writer	�f�[�^���C�^�[
	void ShapeLine::write_svg_barbs(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const
	{
		dt_write_svg("<path d=\"", dt_writer);
		dt_write_svg("M", dt_writer);
		dt_write_svg(barbs[0].x, dt_writer);
		dt_write_svg(barbs[0].y, dt_writer);
		dt_write_svg("L", dt_writer);
		dt_write_svg(tip_pos.x, dt_writer);
		dt_write_svg(tip_pos.y, dt_writer);
		dt_write_svg("L", dt_writer);
		dt_write_svg(barbs[1].x, dt_writer);
		dt_write_svg(barbs[1].y, dt_writer);
		dt_write_svg("\" ", dt_writer);
		if (m_arrow_style == ARROW_STYLE::FILLED) {
			dt_write_svg(m_stroke_color, "fill", dt_writer);
		}
		else {
			dt_write_svg("fill=\"none\" ", dt_writer);
		}
		dt_write_svg(m_stroke_color, "stroke", dt_writer);
		dt_write_svg(m_stroke_width, "stroke-width", dt_writer);
		if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT })) {
			dt_write_svg("stroke-linecap=\"butt\" ", dt_writer);
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND })) {
			dt_write_svg("stroke-linecap=\"round\" ", dt_writer);
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE })) {
			dt_write_svg("stroke-linecap=\"square\" ", dt_writer);
		}
		else if (equal(m_stroke_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE })) {
			//dt_write_svg("stroke-linecap=\"square\" ", dt_writer);
		}
		if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
			dt_write_svg("stroke-linejoin=\"bevel\" ", dt_writer);
		}
		else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
			m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
			dt_write_svg("stroke-linejoin=\"miter\" ", dt_writer);
			dt_write_svg(m_join_limit, "stroke-miterlimit", dt_writer);
		}
		else if (m_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
			dt_write_svg("stroke-linejoin=\"round\" ", dt_writer);
		}
		dt_write_svg(" />" SVG_NEW_LINE, dt_writer);
	}

}