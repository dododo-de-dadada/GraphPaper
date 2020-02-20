//------------------------------
// Shape_elli.cpp
// ���~
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ���`�̒��_�̔z��
	constexpr ANCH_WHICH ANCH_ELLI[4]{
		ANCH_SOUTH,
		ANCH_EAST,
		ANCH_WEST,
		ANCH_NORTH
	};

	// ���~�̕��ʂ𓾂�.
	static void ep_get_anchor(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_vec, D2D1_POINT_2F a_pos[4]) noexcept;

	// ���~�̕��ʂ𓾂�.
	static void ep_get_anchor(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_vec, D2D1_POINT_2F a_pos[4]) noexcept
	{
		// ��
		a_pos[0].x = s_pos.x + d_vec.x * 0.5f;
		a_pos[0].y = s_pos.y + d_vec.y;
		// ��
		a_pos[1].x = s_pos.x + d_vec.x;
		a_pos[1].y = s_pos.y + d_vec.y * 0.5f;
		// ��
		a_pos[2].x = s_pos.x;
		a_pos[2].y = a_pos[1].y;
		// �k
		a_pos[3].x = a_pos[0].x;
		a_pos[3].y = s_pos.y;
	}

	// �}�`��\������.
	void ShapeElli::draw(SHAPE_DX& dx)
	{
		auto br = dx.m_shape_brush.get();

		// ���a�����߂�.
		D2D1_POINT_2F rad;
		pt_scale(m_vec, 0.5, rad);
		// ���S�_�����߂�.
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		// ���~�\���̂Ɋi�[����.
		D2D1_ELLIPSE elli{ c_pos, rad.x, rad.y };
		if (is_opaque(m_fill_color)) {
			// �h��Ԃ��F���s�����Ȃ炾�~��h��Ԃ�.
			dx.m_shape_brush->SetColor(m_fill_color);
			dx.m_d2dContext->FillEllipse(elli, br);
		}
		if (is_opaque(m_stroke_color)) {
			// �g/���̐F���s�����Ȃ炾�~��h��Ԃ�.
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawEllipse(elli, br,
				static_cast<FLOAT>(m_stroke_width),
				m_d2d_stroke_style.get());
		}
		if (is_selected()) {
			D2D1_POINT_2F a_pos[4];
			ep_get_anchor(m_pos, m_vec, a_pos);
			for (uint32_t i = 0; i < 4; i++) {
				draw_anchor(a_pos[i], dx);
			}
		}
	}

	// �ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// a_len	���ʂ̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	ANCH_WHICH ShapeElli::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		D2D1_POINT_2F a_pos[4];
		ep_get_anchor(m_pos, m_vec, a_pos);
		for (uint32_t i = 0; i < 4; i++) {
			// ���~�̊e���ʂ��ʒu���܂ނ����ׂ�.
			if (pt_in_anch(t_pos, a_pos[i], a_len)) {
				// �܂ނȂ炻�̕��ʂ�Ԃ�.
				return ANCH_ELLI[i];
			}
		}
		// ���a�𓾂�.
		D2D1_POINT_2F rad;
		pt_scale(m_vec, 0.5, rad);
		// ���S�_�𓾂�.
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		// ���̌a�������Ȃ�ΐ����ɂ���.
		if (rad.x < 0.0) {
			rad.x = -rad.x;
		}
		// �c�̌a�������Ȃ�ΐ����ɂ���.
		if (rad.y < 0.0) {
			rad.y = -rad.y;
		}
		if (is_opaque(m_stroke_color)) {
			// �ʒu�����~�̊O���ɂ��邩���ׂ�.
			// �g�̑��������ʂ̑傫�������Ȃ��,
			// ���ʂ̑傫����g�̑����Ɋi�[����.
			const double s_width = max(m_stroke_width, a_len);
			// ���a�ɘg�̑����̔������������l���O�a�Ɋi�[����.
			D2D1_POINT_2F r_outer;
			pt_add(rad, s_width * 0.5, r_outer);
			if (pt_in_elli(t_pos, c_pos, r_outer.x, r_outer.y) == false) {
				// �O�a�̂��~�Ɋ܂܂�Ȃ��Ȃ�, 
				// ANCH_OUTSIDE ��Ԃ�.
				return ANCH_OUTSIDE;
			}
			// �ʒu�����~�̘g��ɂ��邩���ׂ�.
			D2D1_POINT_2F r_inner;
			// �O�a����g�̑������������l����a�Ɋi�[����.
			pt_add(r_outer, -s_width, r_inner);
			// ���a�������Ȃ�,
			// ANCH_FRAME ��Ԃ�.
			if (r_inner.x <= 0.0f) {
				return ANCH_FRAME;
			}
			if (r_inner.y <= 0.0f) {
				return ANCH_FRAME;
			}
			if (pt_in_elli(t_pos, c_pos, r_inner.x, r_inner.y) == false) {
				// ���a�̂��~�Ɋ܂܂�Ȃ��Ȃ� ANCH_FRAME ��Ԃ�.
				return ANCH_FRAME;
			}
		}
		if (is_opaque(m_fill_color)) {
			// �ʒu�����~�̓����ɂ��邩���ׂ�.
			if (pt_in_elli(t_pos, c_pos, rad.x, rad.y)) {
				return ANCH_INSIDE;
			}
		}
		return ANCH_OUTSIDE;
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeElli::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		D2D1_POINT_2F rad;
		pt_scale(m_vec, 0.5, rad);
		D2D1_POINT_2F c_pos;
		pt_add(m_pos, rad, c_pos);
		write_svg("<ellipse ", dt_writer);
		write_svg(c_pos, "cx", "cy", dt_writer);
		write_svg(static_cast<double>(rad.x), "rx", dt_writer);
		write_svg(static_cast<double>(rad.y), "ry", dt_writer);
		write_svg(m_fill_color, "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>" SVG_NL, dt_writer);
	}
}