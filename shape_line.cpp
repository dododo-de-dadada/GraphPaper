//------------------------------
// Shape_line.cpp
// �����Ɩ��
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ���̐�[�ƕԂ��̈ʒu�����߂�.
	static bool get_arrow_pos(const D2D1_POINT_2F a_pos, const D2D1_POINT_2F a_vec, const ARROWHEAD_SIZE& a_size, D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept;
	// ���� D2D �X�g���[�N�������쐬����.
	static void create_arrow_style(ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, const D2D1_LINE_JOIN s_join_style, const double s_join_limit, ID2D1StrokeStyle** s_arrow_style);

	// ���� D2D �X�g���[�N�������쐬����.
	static void create_arrow_style(ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, const D2D1_LINE_JOIN s_join_style, const double s_join_limit, ID2D1StrokeStyle** s_arrow_style)
	{
		// ���̔j���̌`���͂��Ȃ炸�\���b�h�Ƃ���.
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

	// ���̐�[�ƕԂ��̈ʒu�����߂�.
	// a_pos	��̌�[�̈ʒu
	// a_vec	��̐�[�ւ̃x�N�g��
	// a_size	���̐��@
	// barbs	�Ԃ��̈ʒu
	// tip		��[�̈ʒu
	static bool get_arrow_pos(const D2D1_POINT_2F a_pos, const D2D1_POINT_2F a_vec, const ARROWHEAD_SIZE& a_size, D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept
	{
		const auto a_len = std::sqrt(pt_abs2(a_vec));	// ��̒���
		if (a_len > FLT_MIN) {
			get_arrow_barbs(a_vec, a_len, a_size.m_width, a_size.m_length, barbs);
			if (a_size.m_offset >= a_len) {
				// ���̐�[
				tip = a_pos;
			}
			else {
				pt_mul(a_vec, 1.0 - a_size.m_offset / a_len, a_pos, tip);
			}
			pt_add(barbs[0], tip, barbs[0]);
			pt_add(barbs[1], tip, barbs[1]);
			return true;
		}
		return false;
	}

	// ���� D2D1 �p�X�W�I���g�����쐬����
	// d_factory	D2D �t�@�N�g���[
	// s_pos	���̊J�n�ʒu
	// diff	���̏I���ʒu�ւ̍���
	// style	���̌`��
	// size	���̐��@
	// geo	�쐬���ꂽ�p�X�W�I���g��
	static void create_arrow_geom(ID2D1Factory3* const d_factory, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, ARROWHEAD_STYLE style, ARROWHEAD_SIZE& a_size, ID2D1PathGeometry** geo)
	{
		D2D1_POINT_2F barbs[2];	// ���̕Ԃ��̒[�_
		D2D1_POINT_2F tip_pos;	// ���̐�[�_
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (get_arrow_pos(s_pos, diff, a_size, barbs, tip_pos)) {
			// �W�I���g���p�X���쐬����.
			winrt::check_hresult(d_factory->CreatePathGeometry(geo));
			winrt::check_hresult((*geo)->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(
				barbs[0], 
				style == ARROWHEAD_STYLE::FILLED
				? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
				: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW
			);
			sink->AddLine(tip_pos);
			sink->AddLine(barbs[1]);
			sink->EndFigure(
				style == ARROWHEAD_STYLE::FILLED
				? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED
				: D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN
			);
			winrt::check_hresult(sink->Close());
			sink = nullptr;
		}
	}

	// �}�`��j������
	ShapeLineA::~ShapeLineA(void)
	{
		if (m_d2d_arrow_geom != nullptr) {
			m_d2d_arrow_geom = nullptr;
		}
		if (m_d2d_arrow_style != nullptr) {
			m_d2d_arrow_style = nullptr;
		}
	}

	// �����f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	// barbs	���̗��[�̈ʒu [2]
	// tip_pos	���̐�[�̈ʒu
	// a_style	���̌`��
	// dt_writer	�f�[�^���C�^�[
	void ShapeLineA::write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<path d=\"", dt_writer);
		write_svg("M", dt_writer);
		write_svg(barbs[0].x, dt_writer);
		write_svg(barbs[0].y, dt_writer);
		write_svg("L", dt_writer);
		write_svg(tip_pos.x, dt_writer);
		write_svg(tip_pos.y, dt_writer);
		write_svg("L", dt_writer);
		write_svg(barbs[1].x, dt_writer);
		write_svg(barbs[1].y, dt_writer);
		write_svg("\" ", dt_writer);
		if (m_arrow_style == ARROWHEAD_STYLE::FILLED) {
			write_svg(m_stroke_color, "fill", dt_writer);
		}
		else {
			write_svg("fill=\"none\" ", dt_writer);
		}
		write_svg(m_stroke_color, "stroke", dt_writer);
		write_svg(m_stroke_width, "stroke-width", dt_writer);
		if (equal(m_stroke_cap_style, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT })) {
			write_svg("stroke-linecap=\"butt\" ", dt_writer);
		}
		else if (equal(m_stroke_cap_style, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND })) {
			write_svg("stroke-linecap=\"round\" ", dt_writer);
		}
		else if (equal(m_stroke_cap_style, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE })) {
			write_svg("stroke-linecap=\"square\" ", dt_writer);
		}
		else if (equal(m_stroke_cap_style, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE })) {
			//write_svg("stroke-linecap=\"square\" ", dt_writer);
		}
		if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
			write_svg("stroke-linejoin=\"bevel\" ", dt_writer);
		}
		else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
			m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
			write_svg("stroke-linejoin=\"miter\" ", dt_writer);
			write_svg(m_stroke_join_limit, "stroke-miterlimit", dt_writer);
		}
		else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
			write_svg("stroke-linejoin=\"round\" ", dt_writer);
		}
		write_svg(" />" SVG_NEW_LINE, dt_writer);
	}

	// �}�`��\������.
	void ShapeLineA::draw(SHAPE_DX& dx)
	{
		D2D1_POINT_2F e_pos;
		pt_add(m_pos, m_diff[0], e_pos);

		dx.m_shape_brush->SetColor(m_stroke_color);
		const auto s_brush = dx.m_shape_brush.get();
		const auto s_style = m_d2d_stroke_style.get();
		const auto s_width = m_stroke_width;
		dx.m_d2dContext->DrawLine(m_pos, e_pos, s_brush, s_width, s_style);
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			const auto a_geom = m_d2d_arrow_geom.get();
			if (a_geom != nullptr) {
				if (m_arrow_style == ARROWHEAD_STYLE::FILLED) {
					dx.m_d2dContext->FillGeometry(a_geom, s_brush, nullptr);
				}
				dx.m_d2dContext->DrawGeometry(a_geom, s_brush, s_width, m_d2d_arrow_style.get());
			}
		}
		if (is_selected()) {
			D2D1_POINT_2F mid;
			pt_mul(m_diff[0], 0.5, m_pos, mid);
			anchor_draw_rect(m_pos, dx);
			anchor_draw_rect(mid, dx);
			anchor_draw_rect(e_pos, dx);
		}
	}

	// ���̐��@�𓾂�.
	bool ShapeLineA::get_arrow_size(ARROWHEAD_SIZE& value) const noexcept
	{
		value = m_arrow_size;
		return true;
	}

	// ���̌`���𓾂�.
	bool ShapeLineA::get_arrow_style(ARROWHEAD_STYLE& value) const noexcept
	{
		value = m_arrow_style;
		return true;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeLineA::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		D2D1_POINT_2F e_pos;
		pt_add(m_pos, m_diff[0], e_pos);
		if (pt_in_anch(t_pos, e_pos)) {
			return ANCH_TYPE::ANCH_P0 + 1;
		}
		if (pt_in_anch(t_pos, m_pos)) {
			return ANCH_TYPE::ANCH_P0;
		}
		const float s_width = static_cast<float>(max(static_cast<double>(m_stroke_width), Shape::s_anch_len));
		if (pt_in_line(t_pos, m_pos, e_pos, s_width, m_stroke_cap_style)) {
			return ANCH_TYPE::ANCH_STROKE;
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeLineA::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		D2D1_POINT_2F e_pos;

		if (pt_in_rect(m_pos, a_min, a_max)) {
			pt_add(m_pos, m_diff[0], e_pos);
			return pt_in_rect(e_pos, a_min, a_max);
		}
		return false;
	}

	// ���������ړ�����.
	bool ShapeLineA::move(const D2D1_POINT_2F diff)
	{
		if (ShapeStroke::move(diff)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
				create_arrow_geom(Shape::s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
			}
			return true;
		}
		return false;
	}

	// �l������̒[�_�Ɋi�[����.
	bool ShapeLineA::set_stroke_cap_style(const CAP_STYLE& value)
	{
		if (ShapeStroke::set_stroke_cap_style(value)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
				create_arrow_style(Shape::s_d2d_factory, m_stroke_cap_style, m_stroke_join_style, m_stroke_join_limit, m_d2d_arrow_style.put());
			}
			return true;
		}
		return false;
	}

	// �l���}�C�^�[�����̔䗦�Ɋi�[����.
	bool ShapeLineA::set_stroke_join_limit(const float& value)
	{
		if (ShapeStroke::set_stroke_join_limit(value)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
				create_arrow_style(Shape::s_d2d_factory, m_stroke_cap_style, m_stroke_join_style, m_stroke_join_limit, m_d2d_arrow_style.put());
			}
			return true;
		}
		return false;
	}

	// �l����̂Ȃ���Ɋi�[����.
	bool ShapeLineA::set_stroke_join_style(const D2D1_LINE_JOIN& value)
	{
		if (ShapeStroke::set_stroke_join_style(value)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
				create_arrow_style(Shape::s_d2d_factory, m_stroke_cap_style, m_stroke_join_style, m_stroke_join_limit, m_d2d_arrow_style.put());
			}
			return true;
		}
		return false;
	}

	// �l����̐��@�Ɋi�[����.
	bool ShapeLineA::set_arrow_size(const ARROWHEAD_SIZE& value)
	{
		if (!equal(m_arrow_size, value)) {
			m_arrow_size = value;
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
				create_arrow_geom(Shape::s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
			}
			return true;
		}
		return false;
	}

	// �l����̌`���Ɋi�[����.
	bool ShapeLineA::set_arrow_style(const ARROWHEAD_STYLE value)
	{
		if (m_arrow_style != value) {
			m_arrow_style = value;
			if (value == ARROWHEAD_STYLE::NONE) {
				if (m_d2d_arrow_geom != nullptr) {
					m_d2d_arrow_geom = nullptr;
				}
				if (m_d2d_arrow_style != nullptr) {
					m_d2d_arrow_style = nullptr;
				}
			}
			else {
				if (m_d2d_arrow_geom != nullptr) {
					m_d2d_arrow_geom = nullptr;
				}
				create_arrow_geom(Shape::s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
				if (m_d2d_arrow_style == nullptr) {
					create_arrow_style(Shape::s_d2d_factory, m_stroke_cap_style, m_stroke_join_style, m_stroke_join_limit, m_d2d_arrow_style.put());
				}
			}
			return true;
		}
		return false;
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
	// value	�i�[����l
	// anchor	�}�`�̕���
	bool ShapeLineA::set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anchor)
	{
		if (ShapeStroke::set_anchor_pos(value, anchor)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
				create_arrow_geom(Shape::s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
			}
			return true;
		}
		return false;
	}

	// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
	bool ShapeLineA::set_start_pos(const D2D1_POINT_2F value)
	{
		if (ShapeStroke::set_start_pos(value)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
				create_arrow_geom(Shape::s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
			}
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// b_pos	�͂ޗ̈�̎n�_
	// b_diff	�͂ޗ̈�̏I�_�ւ̍���
	// s_attr	����̑����l
	ShapeLineA::ShapeLineA(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr) :
		ShapeStroke::ShapeStroke(1, s_attr),
		m_arrow_style(s_attr->m_arrow_style),
		m_arrow_size(s_attr->m_arrow_size),
		m_d2d_arrow_geom(nullptr),
		m_d2d_arrow_style(nullptr)
	{
		m_pos = b_pos;
		m_diff[0] = b_diff;
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			create_arrow_style(Shape::s_d2d_factory, m_stroke_cap_style, m_stroke_join_style, m_stroke_join_limit, m_d2d_arrow_style.put());
			create_arrow_geom(Shape::s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
		}
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	// dt_reader	�ǂݍ��ރf�[�^���[�_�[
	ShapeLineA::ShapeLineA(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader),
		m_d2d_arrow_style(nullptr),
		m_d2d_arrow_geom(nullptr)
	{
		m_arrow_style = static_cast<ARROWHEAD_STYLE>(dt_reader.ReadInt32());
		dt_read(m_arrow_size, dt_reader);
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			create_arrow_geom(Shape::s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
			create_arrow_style(Shape::s_d2d_factory, m_stroke_cap_style, m_stroke_join_style, m_stroke_join_limit, m_d2d_arrow_style.put());
		}
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapeLineA::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);
		dt_writer.WriteInt32(static_cast<int32_t>(m_arrow_style));
		dt_write(m_arrow_size, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeLineA::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;
		D2D1_POINT_2F e_pos;

		pt_add(m_pos, m_diff[0], e_pos);
		write_svg("<line ", dt_writer);
		write_svg(m_pos, "x1", "y1", dt_writer);
		write_svg(e_pos, "x2", "y2", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>" SVG_NEW_LINE, dt_writer);
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			D2D1_POINT_2F barbs[2];
			D2D1_POINT_2F tip_pos;
			if (get_arrow_pos(m_pos, m_diff[0], m_arrow_size, barbs, tip_pos)) {
				ShapeLineA::write_svg(barbs, tip_pos, dt_writer);
			}
		}
	}
}