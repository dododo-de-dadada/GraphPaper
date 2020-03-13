//------------------------------
// Shape_line.cpp
// ����
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr ANCH_WHICH ANCH_BEGIN = ANCH_WHICH::ANCH_R_NW;
	constexpr ANCH_WHICH ANCH_END = ANCH_WHICH::ANCH_R_SE;
	// s_pos	���̊J�n�ʒu
	// d_pos	���̏I���ʒu�ւ̍���
	// a_size	���̐��@
	// barbs_pos	�Ԃ��̈ʒu
	// tip_pos		��[�̈ʒu
	static bool ln_calc_arrowhead(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_pos, const ARROW_SIZE& a_size, D2D1_POINT_2F barbs_pos[2], D2D1_POINT_2F& tip_pos) noexcept
	{
		const auto d_len = std::sqrt(pt_abs2(d_pos));	// ���̒���
		if (d_len > FLT_MIN) {
			// ���̐�[�ƕԂ��̈ʒu���v�Z����.
			get_arrow_barbs(d_pos, d_len, a_size.m_width, a_size.m_length, barbs_pos);
			if (a_size.m_offset >= d_len) {
				// ���̐�[
				tip_pos = s_pos;
			}
			else {
				pt_scale(d_pos, 1.0 - a_size.m_offset / d_len, s_pos, tip_pos);
			}
			pt_add(barbs_pos[0], tip_pos, barbs_pos[0]);
			pt_add(barbs_pos[1], tip_pos, barbs_pos[1]);
			return true;
		}
		return false;
	}

	// ���� D2D1 �p�X�W�I���g�����쐬����
	// fa	D2D �t�@�N�g���[
	// s_pos	���̊J�n�ʒu
	// d_pos	���̏I���ʒu�ւ̍���
	// style	���̌`��
	// size	���̐��@
	// geo	�쐬���ꂽ�p�X�W�I���g��
	static void ln_create_arrow_geometry(ID2D1Factory3* fa, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_pos, ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo)
	{
		D2D1_POINT_2F barbs[2];	// ���̕Ԃ��̒[�_
		D2D1_POINT_2F tip_pos;	// ���̐�[�_
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (ln_calc_arrowhead(s_pos, d_pos, a_size, barbs, tip_pos)) {
			// �W�I���g���p�X���쐬����.
			winrt::check_hresult(
				fa->CreatePathGeometry(geo)
			);
			winrt::check_hresult(
				(*geo)->Open(sink.put())
			);
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
			//sink.attach(nullptr);
			sink = nullptr;
		}
	}

	// �}�`��j������
	ShapeLine::~ShapeLine(void)
	{
		m_d2d_arrow_geometry = nullptr;
	}

	// �}�`��\������.
	void ShapeLine::draw(SHAPE_DX& dx)
	{
		D2D1_POINT_2F e_pos;

		dx.m_shape_brush->SetColor(m_stroke_color);
		pt_add(m_pos, m_diff, e_pos);
		dx.m_d2dContext->DrawLine(
			m_pos,
			e_pos,
			dx.m_shape_brush.get(),
			static_cast<FLOAT>(m_stroke_width),
			m_d2d_stroke_style.get());
		if (m_arrow_style != ARROW_STYLE::NONE) {
			/*
			if (m_d2d_arrow_geometry.get() == nullptr) {
				//ID2D1Factory3 *factory = dev->2DFactory();
				ln_create_arrow_geometry(
					s_d2d_factory, m_pos, m_diff, m_arrow_style,
					m_arrow_size, m_d2d_arrow_geometry.put());
			}
			*/
			if (m_d2d_arrow_geometry.get() != nullptr) {
				dx.m_d2dContext->DrawGeometry(
					m_d2d_arrow_geometry.get(),
					dx.m_shape_brush.get(),
					static_cast<FLOAT>(m_stroke_width),
					nullptr);
				if (m_arrow_style == ARROW_STYLE::FILLED) {
					dx.m_d2dContext->FillGeometry(
						m_d2d_arrow_geometry.get(),
						dx.m_shape_brush.get(), nullptr);
				}
			}
		}
		if (is_selected()) {
			D2D1_POINT_2F mid;
			pt_scale(m_diff, 0.5, m_pos, mid);
			anchor_draw_rect(m_pos, dx);
			anchor_draw_rect(mid, dx);
			anchor_draw_rect(e_pos, dx);
		}
	}

	// ���̐��@�𓾂�.
	bool ShapeLine::get_arrow_size(ARROW_SIZE& value) const noexcept
	{
		value = m_arrow_size;
		return true;
	}

	// ���̌`���𓾂�.
	bool ShapeLine::get_arrow_style(ARROW_STYLE& value) const noexcept
	{
		value = m_arrow_style;
		return true;
	}

	// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
	void ShapeLine::get_pos(const ANCH_WHICH a, D2D1_POINT_2F& value) const noexcept
	{
		if (a == ANCH_END) {
			pt_add(m_pos, m_diff, value);
		}
		else {
			value = m_pos;
		}
	}

	// �ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// a_len	���ʂ̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	ANCH_WHICH ShapeLine::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		D2D1_POINT_2F e_pos;
		pt_add(m_pos, m_diff, e_pos);
		if (pt_in_anch(t_pos, e_pos, a_len)) {
			return ANCH_END;
		}
		if (pt_in_anch(t_pos, m_pos, a_len)) {
			return ANCH_BEGIN;
		}
		if (pt_in_line(t_pos, m_pos, e_pos, max(m_stroke_width, a_len))) {
			return ANCH_WHICH::ANCH_FRAME;
		}
		return ANCH_WHICH::ANCH_OUTSIDE;
	}

	// �͈͂Ɋ܂܂�邩���ׂ�.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeLine::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		D2D1_POINT_2F e_pos;

		if (pt_in_rect(m_pos, a_min, a_max)) {
			pt_add(m_pos, m_diff, e_pos);
			return pt_in_rect(e_pos, a_min, a_max);
		}
		return false;
	}

	// ���������ړ�����.
	void ShapeLine::move(const D2D1_POINT_2F d_pos)
	{
		ShapeStroke::move(d_pos);
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_diff, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// �f�[�^���C�^�[����ǂݍ���.
	void ShapeLine::read(DataReader const& dt_reader)
	{
		m_arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadInt32());
		m_arrow_size.m_width = dt_reader.ReadSingle();
		m_arrow_size.m_length = dt_reader.ReadSingle();
		m_arrow_size.m_offset = dt_reader.ReadSingle();
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_diff, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// �l����̐��@�Ɋi�[����.
	void ShapeLine::set_arrow_size(const ARROW_SIZE& value)
	{
		if (equal(m_arrow_size, value)) {
			return;
		}
		m_arrow_size = value;
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_diff, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// �l����̌`���Ɋi�[����.
	void ShapeLine::set_arrow_style(const ARROW_STYLE value)
	{
		if (m_arrow_style == value) {
			return;
		}
		m_arrow_style = value;
		m_d2d_arrow_geometry = nullptr;
		if (value != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_diff, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// �l���w�肵�����ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
	void ShapeLine::set_pos(const D2D1_POINT_2F value, const ANCH_WHICH a)
	{
		D2D1_POINT_2F d_pos;

		if (a == ANCH_END) {
			pt_sub(value, m_pos, m_diff);
		}
		else {
			if (a == ANCH_BEGIN) {
				pt_sub(value, m_pos, d_pos);
				pt_sub(m_diff, d_pos, m_diff);
			}
			m_pos = value;
		}
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_diff, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
	void ShapeLine::set_start_pos(const D2D1_POINT_2F value)
	{
		ShapeStroke::set_start_pos(value);
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_diff, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// �}�`���쐬����.
	// pos	�J�n�ʒu
	// d_pos	�I���ʒu�ւ̍���
	// attr	����̑����l
	ShapeLine::ShapeLine(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_pos, const ShapePanel* attr) :
		ShapeStroke::ShapeStroke(attr),
		m_arrow_style(attr->m_arrow_style),
		m_arrow_size(attr->m_arrow_size)
	{
		m_pos = s_pos;
		m_diff = d_pos;
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(s_d2d_factory, m_pos, m_diff, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	// dt_reader	�ǂݍ��ރf�[�^���[�_�[
	ShapeLine::ShapeLine(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader)
	{
		m_arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadInt32());
		m_arrow_size.m_width = dt_reader.ReadSingle();
		m_arrow_size.m_length = dt_reader.ReadSingle();
		m_arrow_size.m_offset = dt_reader.ReadSingle();
		m_d2d_arrow_geometry = nullptr;
		if (m_arrow_style != ARROW_STYLE::NONE) {
			ln_create_arrow_geometry(
				s_d2d_factory, m_pos, m_diff, m_arrow_style,
				m_arrow_size, m_d2d_arrow_geometry.put());
		}
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapeLine::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);
		dt_writer.WriteInt32(static_cast<int32_t>(m_arrow_style));
		dt_writer.WriteSingle(m_arrow_size.m_width);
		dt_writer.WriteSingle(m_arrow_size.m_length);
		dt_writer.WriteSingle(m_arrow_size.m_offset);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeLine::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;
		D2D1_POINT_2F e_pos;

		pt_add(m_pos, m_diff, e_pos);
		write_svg("<line ", dt_writer);
		write_svg(m_pos, "x1", "y1", dt_writer);
		write_svg(e_pos, "x2", "y2", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>" SVG_NL, dt_writer);
		if (m_arrow_style == ARROW_STYLE::NONE) {
			return;
		}
		D2D1_POINT_2F barbs[2];
		D2D1_POINT_2F tip_pos;
		if (ln_calc_arrowhead(m_pos, m_diff, m_arrow_size, barbs, tip_pos)) {
			ShapeStroke::write_svg(barbs, tip_pos, m_arrow_style, dt_writer);
		}
	}
}