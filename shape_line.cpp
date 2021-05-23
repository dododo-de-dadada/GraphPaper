//------------------------------
// Shape_line.cpp
// ����
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr uint32_t ANCH_BEGIN = ANCH_TYPE::ANCH_P0;
	constexpr uint32_t ANCH_END = ANCH_TYPE::ANCH_P0 + 1;
	// s_pos	���̊J�n�ʒu
	// diff	���̏I�[�ւ̍���
	// h_size	���̐��@
	// barbs_pos	�Ԃ��̈ʒu
	// tip_pos		��[�̈ʒu
	static bool ln_calc_arrow(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, const ARROWHEAD_SIZE& h_size, D2D1_POINT_2F barbs_pos[2], D2D1_POINT_2F& tip_pos) noexcept
	{
		const auto d_len = std::sqrt(pt_abs2(diff));	// ��̒���
		if (d_len > FLT_MIN) {
			get_arrow_barbs(diff, d_len, h_size.m_width, h_size.m_length, barbs_pos);
			if (h_size.m_offset >= d_len) {
				// ���̐�[
				tip_pos = s_pos;
			}
			else {
				pt_mul(diff, 1.0 - h_size.m_offset / d_len, s_pos, tip_pos);
			}
			pt_add(barbs_pos[0], tip_pos, barbs_pos[0]);
			pt_add(barbs_pos[1], tip_pos, barbs_pos[1]);
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
	static void ln_create_arrow_geom(ID2D1Factory3* const d_factory, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, ARROWHEAD_STYLE style, ARROWHEAD_SIZE& a_size, ID2D1PathGeometry** geo)
	{
		D2D1_POINT_2F barbs[2];	// ���̕Ԃ��̒[�_
		D2D1_POINT_2F tip_pos;	// ���̐�[�_
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (ln_calc_arrow(s_pos, diff, a_size, barbs, tip_pos)) {
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
			//sink.attach(nullptr);
			sink = nullptr;
		}
	}

	// �}�`��j������
	ShapeLine::~ShapeLine(void)
	{
		m_d2d_arrow_geom = nullptr;
	}

	// �}�`��\������.
	void ShapeLine::draw(SHAPE_DX& dx)
	{
		D2D1_POINT_2F e_pos;
		pt_add(m_pos, m_diff[0], e_pos);

		dx.m_shape_brush->SetColor(m_stroke_color);
		const auto s_brush = dx.m_shape_brush.get();
		const auto s_style = m_d2d_stroke_style.get();
		const auto s_width = static_cast<FLOAT>(m_stroke_width);
		dx.m_d2dContext->DrawLine(m_pos, e_pos, s_brush, s_width, s_style);
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			const auto a_geom = m_d2d_arrow_geom.get();
			if (a_geom != nullptr) {
				dx.m_d2dContext->DrawGeometry(a_geom, s_brush, s_width, nullptr);
				if (m_arrow_style == ARROWHEAD_STYLE::FILLED) {
					dx.m_d2dContext->FillGeometry(a_geom, s_brush, nullptr);
				}
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
	bool ShapeLine::get_arrow_size(ARROWHEAD_SIZE& value) const noexcept
	{
		value = m_arrow_size;
		return true;
	}

	// ���̌`���𓾂�.
	bool ShapeLine::get_arrow_style(ARROWHEAD_STYLE& value) const noexcept
	{
		value = m_arrow_style;
		return true;
	}

	//	���ʂ̈ʒu�𓾂�.
	//	anch	�}�`�̕���.
	//	value	����ꂽ�ʒu.
	//	�߂�l	�Ȃ�
	void ShapeLine::get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept
	{
		if (anch == ANCH_END) {
			pt_add(m_pos, m_diff[0], value);
		}
		else {
			value = m_pos;
		}
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// a_len	���ʂ̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeLine::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		D2D1_POINT_2F e_pos;
		pt_add(m_pos, m_diff[0], e_pos);
		if (pt_in_anch(t_pos, e_pos, a_len)) {
			return ANCH_END;
		}
		if (pt_in_anch(t_pos, m_pos, a_len)) {
			return ANCH_BEGIN;
		}
		if (pt_in_line(t_pos, m_pos, e_pos, max(m_stroke_width, a_len))) {
			return ANCH_TYPE::ANCH_STROKE;
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeLine::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		D2D1_POINT_2F e_pos;

		if (pt_in_rect(m_pos, a_min, a_max)) {
			pt_add(m_pos, m_diff[0], e_pos);
			return pt_in_rect(e_pos, a_min, a_max);
		}
		return false;
	}

	// ���������ړ�����.
	void ShapeLine::move(const D2D1_POINT_2F diff)
	{
		ShapeStroke::move(diff);
		m_d2d_arrow_geom = nullptr;
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			ln_create_arrow_geom(s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
		}
	}

	// �f�[�^���C�^�[����ǂݍ���.
	void ShapeLine::read(DataReader const& dt_reader)
	{
		m_arrow_style = static_cast<ARROWHEAD_STYLE>(dt_reader.ReadInt32());
		m_arrow_size.m_width = dt_reader.ReadSingle();
		m_arrow_size.m_length = dt_reader.ReadSingle();
		m_arrow_size.m_offset = dt_reader.ReadSingle();
		m_d2d_arrow_geom = nullptr;
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			ln_create_arrow_geom(s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
		}
	}

	// �l����̐��@�Ɋi�[����.
	void ShapeLine::set_arrow_size(const ARROWHEAD_SIZE& value)
	{
		if (equal(m_arrow_size, value)) {
			return;
		}
		m_arrow_size = value;
		m_d2d_arrow_geom = nullptr;
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			ln_create_arrow_geom(s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
		}
	}

	// �l����̌`���Ɋi�[����.
	void ShapeLine::set_arrow_style(const ARROWHEAD_STYLE value)
	{
		if (m_arrow_style == value) {
			return;
		}
		m_arrow_style = value;
		m_d2d_arrow_geom = nullptr;
		if (value != ARROWHEAD_STYLE::NONE) {
			ln_create_arrow_geom(s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
		}
	}

	//	�l��, ���ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
	//	value	�i�[����l
	//	abch	�}�`�̕���
	void ShapeLine::set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anchor)
	{
		D2D1_POINT_2F diff;

		if (anchor == ANCH_END) {
			pt_sub(value, m_pos, m_diff[0]);
		}
		else if (anchor == ANCH_BEGIN || anchor == ANCH_STROKE) {
			if (anchor == ANCH_BEGIN) {
				pt_sub(value, m_pos, diff);
				pt_sub(m_diff[0], diff, m_diff[0]);
			}
			m_pos = value;
		}
		else {
			throw hresult_not_implemented();
		}
		m_d2d_arrow_geom = nullptr;
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			ln_create_arrow_geom(s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
		}
	}

	// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
	void ShapeLine::set_start_pos(const D2D1_POINT_2F value)
	{
		ShapeStroke::set_start_pos(value);
		m_d2d_arrow_geom = nullptr;
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			ln_create_arrow_geom(s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
		}
	}

	// �}�`���쐬����.
	// b_pos	�͂ޗ̈�̎n�_
	// b_diff	�͂ޗ̈�̏I�_�ւ̍���
	// s_attr	����̑����l
	ShapeLine::ShapeLine(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr) :
		ShapeStroke::ShapeStroke(1, s_attr),
		m_arrow_style(s_attr->m_arrow_style),
		m_arrow_size(s_attr->m_arrow_size)
	{
		m_pos = b_pos;
		m_diff[0] = b_diff;
		m_d2d_arrow_geom = nullptr;
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			ln_create_arrow_geom(s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
		}
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	// dt_reader	�ǂݍ��ރf�[�^���[�_�[
	ShapeLine::ShapeLine(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader)
	{
		m_arrow_style = static_cast<ARROWHEAD_STYLE>(dt_reader.ReadInt32());
		m_arrow_size.m_width = dt_reader.ReadSingle();
		m_arrow_size.m_length = dt_reader.ReadSingle();
		m_arrow_size.m_offset = dt_reader.ReadSingle();
		m_d2d_arrow_geom = nullptr;
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			ln_create_arrow_geom(s_d2d_factory, m_pos, m_diff[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
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

		pt_add(m_pos, m_diff[0], e_pos);
		write_svg("<line ", dt_writer);
		write_svg(m_pos, "x1", "y1", dt_writer);
		write_svg(e_pos, "x2", "y2", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>" SVG_NEW_LINE, dt_writer);
		if (m_arrow_style == ARROWHEAD_STYLE::NONE) {
			return;
		}
		D2D1_POINT_2F barbs[2];
		D2D1_POINT_2F tip_pos;
		if (ln_calc_arrow(m_pos, m_diff[0], m_arrow_size, barbs, tip_pos)) {
			ShapeStroke::write_svg(barbs, tip_pos, m_arrow_style, dt_writer);
		}
	}
}