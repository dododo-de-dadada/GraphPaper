#include "pch.h"
#include "shape_rect.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �}�`��\������.
	// dx	�`���
	void ShapeRect::draw(SHAPE_DX& dx)
	{
		const D2D1_RECT_F rect{
			m_pos.x,
			m_pos.y,
			m_pos.x + m_diff.x,
			m_pos.y + m_diff.y
		};
		if (is_opaque(m_fill_color)) {
			// �h��Ԃ��F���s�����ȏꍇ,
			// ���`��h��Ԃ�.
			dx.m_shape_brush->SetColor(m_fill_color);
			dx.m_d2dContext->FillRectangle(&rect, dx.m_shape_brush.get());
		}
		if (is_opaque(m_stroke_color)) {
			// ���g�̐F���s�����ȏꍇ,
			// ���`�̘g��\������.
			const auto w = static_cast<FLOAT>(m_stroke_width);
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawRectangle(
				rect, dx.m_shape_brush.get(), w, m_d2d_stroke_style.get());
		}
		if (is_selected() != true) {
			return;
		}
		// �I���t���O�������Ă���ꍇ,
		// ���ʂ�\������.
		D2D1_POINT_2F r_pos[4];	// ���`�̒��_
		r_pos[0] = m_pos;
		r_pos[1].y = rect.top;
		r_pos[1].x = rect.right;
		r_pos[2].x = rect.right;
		r_pos[2].y = rect.bottom;
		r_pos[3].y = rect.bottom;
		r_pos[3].x = rect.left;
		for (uint32_t i = 0, j = 3; i < 4; j = i++) {
			anchor_draw_rect(r_pos[i], dx);
			D2D1_POINT_2F r_mid;	// ���`�̕ӂ̒��_
			pt_avg(r_pos[j], r_pos[i], r_mid);
			anchor_draw_rect(r_mid, dx);
		}
	}

	ANCH_WHICH ShapeRect::hit_test_anchor(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		// �ǂ̒��_���ʒu���܂ނ����ׂ�.
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F a_pos;
			get_pos(ANCH_CORNER[i], a_pos);
			if (pt_in_anch(t_pos, a_pos, a_len)) {
				return ANCH_CORNER[i];
			}
		}
		// �ǂ̒��_���ʒu���܂ނ����ׂ�.
		for (uint32_t i = 0; i < 4; i++) {
			D2D1_POINT_2F a_pos;
			get_pos(ANCH_MIDDLE[i], a_pos);
			if (pt_in_anch(t_pos, a_pos, a_len)) {
				return ANCH_MIDDLE[i];
			}
		}
		return ANCH_WHICH::ANCH_OUTSIDE;
	}

	// �ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// a_len	���ʂ̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	ANCH_WHICH ShapeRect::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const auto anchor = hit_test_anchor(t_pos, a_len);
		if (anchor != ANCH_WHICH::ANCH_OUTSIDE) {
			return anchor;
		}
		// ���`�̉E��_�ƍ����_�����߂�.
		D2D1_POINT_2F r_pos;
		pt_add(m_pos, m_diff, r_pos);
		D2D1_POINT_2F r_min;	// ���`�̍���_
		D2D1_POINT_2F r_max;	// ���`�̉E���_
		pt_bound(m_pos, r_pos, r_min, r_max);
		if (is_opaque(m_stroke_color) != true) {
			// ���g�̐F�������ȏꍇ,
			if (is_opaque(m_fill_color)) {
				// �h��Ԃ��F���s�����ȏꍇ,
				if (pt_in_rect(t_pos, r_min, r_max)) {
					// �ʒu�����`�ɂӂ��܂��ꍇ,
					return ANCH_WHICH::ANCH_INSIDE;
				}
			}
			return ANCH_WHICH::ANCH_OUTSIDE;
		}
		const double sw = max(m_stroke_width, a_len);	// ���g�̑���
		pt_add(r_min, sw * 0.5, r_min);
		pt_add(r_max, sw * -0.5, r_max);
		if (pt_in_rect(t_pos, r_min, r_max)) {
			return is_opaque(m_fill_color) ? ANCH_WHICH::ANCH_INSIDE : ANCH_WHICH::ANCH_OUTSIDE;
		}
		pt_add(r_min, -sw, r_min);
		pt_add(r_max, sw, r_max);
		if (pt_in_rect(t_pos, r_min, r_max)) {
			return ANCH_WHICH::ANCH_FRAME;
		}
		return ANCH_WHICH::ANCH_OUTSIDE;
	}

	// �h��Ԃ��̐F�𓾂�.
	// val	����ꂽ�l.
	bool ShapeRect::get_fill_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_fill_color;
		return true;
	}

	// �h��Ԃ��̐F�Ɋi�[����.
	// val	�i�[����l.
	void ShapeRect::set_fill_color(const D2D1_COLOR_F& value) noexcept
	{
		m_fill_color = value;
	}

	// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
	void ShapeRect::get_pos(const ANCH_WHICH a, D2D1_POINT_2F& value) const noexcept
	{
		switch (a) {
		case ANCH_WHICH::ANCH_NORTH:
			value.x = m_pos.x + m_diff.x * 0.5f;
			value.y = m_pos.y;
			break;
		case ANCH_WHICH::ANCH_NE:
			value.x = m_pos.x + m_diff.x;
			value.y = m_pos.y;
			break;
		case ANCH_WHICH::ANCH_WEST:
			value.x = m_pos.x;
			value.y = m_pos.y + m_diff.y * 0.5f;
			break;
		case ANCH_WHICH::ANCH_EAST:
			value.x = m_pos.x + m_diff.x;
			value.y = m_pos.y + m_diff.y * 0.5f;
			break;
		case ANCH_WHICH::ANCH_SW:
			value.x = m_pos.x;
			value.y = m_pos.y + m_diff.y;
			break;
		case ANCH_WHICH::ANCH_SOUTH:
			value.x = m_pos.x + m_diff.x * 0.5f;
			value.y = m_pos.y + m_diff.y;
			break;
		case ANCH_WHICH::ANCH_SE:
			value.x = m_pos.x + m_diff.x;
			value.y = m_pos.y + m_diff.y;
			break;
		default:
			value = m_pos;
			break;
		}
	}

	// �͈͂Ɋ܂܂�邩���ׂ�.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeRect::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		D2D1_POINT_2F e_pos;

		pt_add(m_pos, m_diff, e_pos);
		return pt_in_rect(m_pos, a_min, a_max) && pt_in_rect(e_pos, a_min, a_max);
	}

	// �l���w�肵�����ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
	void ShapeRect::set_pos(const D2D1_POINT_2F value, const ANCH_WHICH a)
	{
		D2D1_POINT_2F a_pos;
		D2D1_POINT_2F diff;

		switch (a) {
		case ANCH_WHICH::ANCH_OUTSIDE:
			m_pos = value;
			break;
		case ANCH_WHICH::ANCH_NW:
			pt_sub(value, m_pos, diff);
			pt_add(m_pos, diff, m_pos);
			pt_sub(m_diff, diff, m_diff);
			break;
		case ANCH_WHICH::ANCH_NORTH:
			m_diff.y -= value.y - m_pos.y;
			m_pos.y = value.y;
			break;
		case ANCH_WHICH::ANCH_NE:
			a_pos.x = m_pos.x + m_diff.x;
			a_pos.y = m_pos.y;
			m_pos.y = value.y;
			pt_sub(value, a_pos, diff);
			pt_add(m_diff, diff.x, -diff.y, m_diff);
			break;
		case ANCH_WHICH::ANCH_WEST:
			m_diff.x -= value.x - m_pos.x;
			m_pos.x = value.x;
			break;
		case ANCH_WHICH::ANCH_EAST:
			m_diff.x = value.x - m_pos.x;
			break;
		case ANCH_WHICH::ANCH_SW:
			a_pos.x = m_pos.x;
			a_pos.y = m_pos.y + m_diff.y;
			m_pos.x = value.x;
			pt_sub(value, a_pos, diff);
			pt_add(m_diff, -diff.x, diff.y, m_diff);
			break;
		case ANCH_WHICH::ANCH_SOUTH:
			m_diff.y = value.y - m_pos.y;
			break;
		case ANCH_WHICH::ANCH_SE:
			pt_sub(value, m_pos, m_diff);
			break;
		}
	}

	// �}�`���쐬����.
	ShapeRect::ShapeRect(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, const ShapeLayout* attr) :
		ShapeStroke::ShapeStroke(attr),
		m_fill_color(attr->m_fill_color)
	{
		m_pos = s_pos;
		m_diff = diff;
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapeRect::ShapeRect(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_fill_color, dt_reader);
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapeRect::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeStroke::write(dt_writer);
		write(m_fill_color, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeRect::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<rect ", dt_writer);
		write_svg(m_pos, "x", "y", dt_writer);
		write_svg(m_diff, "width", "height", dt_writer);
		write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>" SVG_NL, dt_writer);
	}
}