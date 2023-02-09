//------------------------------
// Shape_elli.cpp
// ���~
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataWriter;

	// �}�`��\������.
	void ShapeEllipse::draw(void)
	{
		ID2D1Factory* const factory = Shape::s_d2d_factory;
		ID2D1RenderTarget* const target = Shape::s_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::s_d2d_color_brush;

		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}

		// ���a�����߂�.
		D2D1_POINT_2F rad;
		pt_mul(m_vec[0], 0.5, rad);
		// ���S�_�����߂�.
		D2D1_POINT_2F c_pos;
		pt_add(m_start, rad, c_pos);
		// ���~�\���̂Ɋi�[����.
		D2D1_ELLIPSE elli{ c_pos, rad.x, rad.y };
		// �h��Ԃ��F���s���������肷��.
		if (is_opaque(m_fill_color)) {
			brush->SetColor(m_fill_color);
			target->FillEllipse(elli, brush);
		}
		// �g���̐F���s���������肷��.
		if (is_opaque(m_stroke_color)) {
			brush->SetColor(m_stroke_color);
			target->DrawEllipse(elli, brush, m_stroke_width, m_d2d_stroke_style.get());
		}
		if (!is_selected()) {
			return;
		}
		D2D1_MATRIX_3X2_F t32;
		target->GetTransform(&t32);
		draw_anc(Shape::s_anc_len / t32._11);
		/*
		D2D1_POINT_2F a_pos[4];
		// ��
		a_pos[0].x = m_start.x + m_vec[0].x * 0.5f;
		a_pos[0].y = m_start.y + m_vec[0].y;
		// ��
		a_pos[1].x = m_start.x + m_vec[0].x;
		a_pos[1].y = m_start.y + m_vec[0].y * 0.5f;
		// ��
		a_pos[2].x = m_start.x;
		a_pos[2].y = a_pos[1].y;
		// �k
		a_pos[3].x = a_pos[0].x;
		a_pos[3].y = m_start.y;
		for (uint32_t i = 0; i < 4; i++) {
			anc_draw_rect(a_pos[i], target, brush);
		}
		a_pos[0] = m_start;
		pt_add(m_start, m_vec[0], a_pos[3]);
		a_pos[1].x = a_pos[0].x;
		a_pos[1].y = a_pos[3].y;
		a_pos[2].x = a_pos[3].x;
		a_pos[2].y = a_pos[0].y;
		for (uint32_t i = 0; i < 4; i++) {
			anc_draw_ellipse(a_pos[i], target, brush);
		}
		*/
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeEllipse::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const auto anc = rect_hit_test_anc(m_start, m_vec[0], t_pos, a_len);
		if (anc != ANC_TYPE::ANC_PAGE) {
			return anc;
		}

		// ���a�𓾂�.
		D2D1_POINT_2F rad;
		pt_mul(m_vec[0], 0.5, rad);
		// ���S�_�𓾂�.
		D2D1_POINT_2F c_pos;
		pt_add(m_start, rad, c_pos);
		rad.x = fabsf(rad.x);
		rad.y = fabsf(rad.y);
		if (is_opaque(m_stroke_color)) {
			// �ʒu�����~�̊O���ɂ��邩���肷��.
			// �g�̑��������ʂ̑傫�������Ȃ��,
			// ���ʂ̑傫����g�̑����Ɋi�[����.
			const double s_width = max(static_cast<double>(m_stroke_width), a_len);
			// ���a�ɘg�̑����̔������������l���O�a�Ɋi�[����.
			D2D1_POINT_2F r_outer;
			pt_add(rad, s_width * 0.5, r_outer);
			if (!pt_in_ellipse(t_pos, c_pos, r_outer.x, r_outer.y)) {
				// �O�a�̂��~�Ɋ܂܂�Ȃ��Ȃ�, 
				// ANC_PAGE ��Ԃ�.
				return ANC_TYPE::ANC_PAGE;
			}
			// �ʒu�����~�̘g��ɂ��邩���肷��.
			D2D1_POINT_2F r_inner;
			// �O�a����g�̑������������l����a�Ɋi�[����.
			pt_add(r_outer, -s_width, r_inner);
			// ���a�������Ȃ�,
			// ANC_STROKE ��Ԃ�.
			if (r_inner.x <= 0.0f) {
				return ANC_TYPE::ANC_STROKE;
			}
			if (r_inner.y <= 0.0f) {
				return ANC_TYPE::ANC_STROKE;
			}
			// ���a�̂��~�Ɋ܂܂�Ȃ������肷��.
			if (!pt_in_ellipse(t_pos, c_pos, r_inner.x, r_inner.y)) {
				return ANC_TYPE::ANC_STROKE;
			}
		}
		if (is_opaque(m_fill_color)) {
			// ���~�Ɉʒu���܂܂�邩���肷��.
			if (pt_in_ellipse(t_pos, c_pos, rad.x, rad.y)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

}