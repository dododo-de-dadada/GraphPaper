//------------------------------
// Shape_line.cpp
// �����Ɩ�邵
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ��邵�̐�[�ƕԂ��̈ʒu�����߂�.
	static bool get_arrow_pos(const D2D1_POINT_2F a_pos, const D2D1_POINT_2F a_vec, const ARROW_SIZE& a_size, D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept;
	// ��邵�� D2D �X�g���[�N�������쐬����.
	static void create_arrow_style(ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, const D2D1_LINE_JOIN s_join_style, const double s_join_limit, ID2D1StrokeStyle** s_arrow_style);

	// ��邵�� D2D �X�g���[�N�������쐬����.
	static void create_arrow_style(ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, const D2D1_LINE_JOIN s_join_style, const double s_join_limit, ID2D1StrokeStyle** s_arrow_style)
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
	static bool get_arrow_pos(const D2D1_POINT_2F a_pos, const D2D1_POINT_2F a_vec, const ARROW_SIZE& a_size, D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept
	{
		const auto a_len = std::sqrt(pt_abs2(a_vec));	// ��̒���
		if (a_len >= FLT_MIN) {
			get_arrow_barbs(a_vec, a_len, a_size.m_width, a_size.m_length, barbs);
			if (a_size.m_offset >= a_len) {
				// ��邵�̐�[
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

	// ��邵�� D2D1 �p�X�W�I���g�����쐬����
	// d_factory	D2D �t�@�N�g���[
	// s_pos	���̊J�n�ʒu
	// d_vec	���̏I���ʒu�ւ̍���
	// style	��邵�̌`��
	// size	��邵�̐��@
	// geo	�쐬���ꂽ�p�X�W�I���g��
	static void create_arrow_geom(ID2D1Factory3* const d_factory, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_vec, ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo)
	{
		D2D1_POINT_2F barbs[2];	// ��邵�̕Ԃ��̒[�_
		D2D1_POINT_2F tip_pos;	// ��邵�̐�[�_
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (get_arrow_pos(s_pos, d_vec, a_size, barbs, tip_pos)) {
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

	// �������ʒu���܂ނ�, �������l�����Ĕ��肷��.
	// t_pos	���肷��ʒu
	// s_pos	�����̎n�[
	// e_pos	�����̏I�[
	// s_width	�����̑���
	// s_cap	�����̒[�_
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

	// ��邵���f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	// barbs	��邵�̗��[�̈ʒu [2]
	// tip_pos	��邵�̐�[�̈ʒu
	// a_style	��邵�̌`��
	// dt_writer	�f�[�^���C�^�[
	void ShapeLineA::dt_write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::dt_write_svg;

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
		if (equal(m_stroke_cap_style, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT })) {
			dt_write_svg("stroke-linecap=\"butt\" ", dt_writer);
		}
		else if (equal(m_stroke_cap_style, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND })) {
			dt_write_svg("stroke-linecap=\"round\" ", dt_writer);
		}
		else if (equal(m_stroke_cap_style, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE })) {
			dt_write_svg("stroke-linecap=\"square\" ", dt_writer);
		}
		else if (equal(m_stroke_cap_style, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE })) {
			//dt_write_svg("stroke-linecap=\"square\" ", dt_writer);
		}
		if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
			dt_write_svg("stroke-linejoin=\"bevel\" ", dt_writer);
		}
		else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
			m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
			dt_write_svg("stroke-linejoin=\"miter\" ", dt_writer);
			dt_write_svg(m_stroke_join_limit, "stroke-miterlimit", dt_writer);
		}
		else if (m_stroke_join_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
			dt_write_svg("stroke-linejoin=\"round\" ", dt_writer);
		}
		dt_write_svg(" />" SVG_NEW_LINE, dt_writer);
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
		if (m_arrow_style != ARROW_STYLE::NONE) {
			const auto a_geom = m_d2d_arrow_geom.get();
			if (a_geom != nullptr) {
				if (m_arrow_style == ARROW_STYLE::FILLED) {
					dx.m_d2dContext->FillGeometry(a_geom, s_brush, nullptr);
				}
				dx.m_d2dContext->DrawGeometry(a_geom, s_brush, s_width, m_d2d_arrow_style.get());
			}
		}
		if (is_selected()) {
			D2D1_POINT_2F mid;
			pt_mul(m_diff[0], 0.5, m_pos, mid);
			anch_draw_rect(m_pos, dx);
			anch_draw_rect(mid, dx);
			anch_draw_rect(e_pos, dx);
		}
	}

	// ��邵�̐��@�𓾂�.
	bool ShapeLineA::get_arrow_size(ARROW_SIZE& value) const noexcept
	{
		value = m_arrow_size;
		return true;
	}

	// ��邵�̌`���𓾂�.
	bool ShapeLineA::get_arrow_style(ARROW_STYLE& value) const noexcept
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
		if (line_hit_test(t_pos, m_pos, e_pos, s_width, m_stroke_cap_style)) {
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
	bool ShapeLineA::move(const D2D1_POINT_2F d_vec)
	{
		if (ShapeStroke::move(d_vec)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			if (m_arrow_style != ARROW_STYLE::NONE) {
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
			if (m_arrow_style != ARROW_STYLE::NONE) {
				create_arrow_style(Shape::s_d2d_factory, m_stroke_cap_style, m_stroke_join_style, m_stroke_join_limit, m_d2d_arrow_style.put());
			}
			return true;
		}
		return false;
	}

	// �l������̂Ȃ��̃}�C�^�[�����Ɋi�[����.
	bool ShapeLineA::set_stroke_join_limit(const float& value)
	{
		if (ShapeStroke::set_stroke_join_limit(value)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			if (m_arrow_style != ARROW_STYLE::NONE) {
				create_arrow_style(Shape::s_d2d_factory, m_stroke_cap_style, m_stroke_join_style, m_stroke_join_limit, m_d2d_arrow_style.put());
			}
			return true;
		}
		return false;
	}

	// �l������̂Ȃ��Ɋi�[����.
	bool ShapeLineA::set_stroke_join_style(const D2D1_LINE_JOIN& value)
	{
		if (ShapeStroke::set_stroke_join_style(value)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			if (m_arrow_style != ARROW_STYLE::NONE) {
				create_arrow_style(Shape::s_d2d_factory, m_stroke_cap_style, m_stroke_join_style, m_stroke_join_limit, m_d2d_arrow_style.put());
			}
			return true;
		}
		return false;
	}

	// �l���邵�̐��@�Ɋi�[����.
	bool ShapeLineA::set_arrow_size(const ARROW_SIZE& value)
	{
		if (!equal(m_arrow_size, value)) {
			m_arrow_size = value;
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			if (m_arrow_style != ARROW_STYLE::NONE) {
				create_arrow_geom(Shape::s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
			}
			return true;
		}
		return false;
	}

	// �l���邵�̌`���Ɋi�[����.
	bool ShapeLineA::set_arrow_style(const ARROW_STYLE value)
	{
		if (m_arrow_style != value) {
			m_arrow_style = value;
			if (value == ARROW_STYLE::NONE) {
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
	// anch	�}�`�̕���
	// limit	
	bool ShapeLineA::set_anch_pos(const D2D1_POINT_2F value, const uint32_t anch, const float dist)
	{
		if (ShapeStroke::set_anch_pos(value, anch, dist)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			if (m_arrow_style != ARROW_STYLE::NONE) {
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
			if (m_arrow_style != ARROW_STYLE::NONE) {
				create_arrow_geom(Shape::s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
			}
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// b_pos	�͂ޗ̈�̎n�_
	// b_vec	�͂ޗ̈�̏I�_�ւ̍���
	// s_attr	����̑����l
	ShapeLineA::ShapeLineA(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr) :
		ShapeStroke::ShapeStroke(1, s_attr),
		m_arrow_style(s_attr->m_arrow_style),
		m_arrow_size(s_attr->m_arrow_size),
		m_d2d_arrow_geom(nullptr),
		m_d2d_arrow_style(nullptr)
	{
		m_pos = b_pos;
		m_diff[0] = b_vec;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			create_arrow_style(Shape::s_d2d_factory, m_stroke_cap_style, m_stroke_join_style, m_stroke_join_limit, m_d2d_arrow_style.put());
			create_arrow_geom(Shape::s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
		}
	}

	// �f�[�^���[�_�[����}�`��ǂݍ���.
	// dt_reader	�ǂݍ��ރf�[�^���[�_�[
	ShapeLineA::ShapeLineA(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader),
		m_d2d_arrow_style(nullptr),
		m_d2d_arrow_geom(nullptr)
	{
		m_arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadInt32());
		dt_read(m_arrow_size, dt_reader);
		if (m_arrow_style != ARROW_STYLE::NONE) {
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
	void ShapeLineA::dt_write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::dt_write_svg;
		D2D1_POINT_2F e_pos;

		pt_add(m_pos, m_diff[0], e_pos);
		dt_write_svg("<line ", dt_writer);
		dt_write_svg(m_pos, "x1", "y1", dt_writer);
		dt_write_svg(e_pos, "x2", "y2", dt_writer);
		ShapeStroke::dt_write_svg(dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[2];
			D2D1_POINT_2F tip_pos;
			if (get_arrow_pos(m_pos, m_diff[0], m_arrow_size, barbs, tip_pos)) {
				ShapeLineA::dt_write_svg(barbs, tip_pos, dt_writer);
			}
		}
	}
}