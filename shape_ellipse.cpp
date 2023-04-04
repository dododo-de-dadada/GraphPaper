//------------------------------
// Shape_elli.cpp
// ���~
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �}�`��\������.
	void ShapeEllipse::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		if (m_d2d_stroke_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			create_stroke_style(factory);
		}

		// ���a�����߂�.
		const double rx = 0.5 * m_pos.x;
		const double ry = 0.5 * m_pos.y;
		// ���~�\���̂Ɋi�[����.
		D2D1_ELLIPSE elli{ 
			D2D1_POINT_2F {
				static_cast<FLOAT>(m_start.x + rx), static_cast<FLOAT>(m_start.y + ry)
		},
			static_cast<FLOAT>(rx), static_cast<FLOAT>(ry)
		};
		// �h��Ԃ��F���s���������肷��.
		if (is_opaque(m_fill_color)) {
			brush->SetColor(m_fill_color);
			target->FillEllipse(elli, brush);
		}
		// �g���̐F���s���������肷��.
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			brush->SetColor(m_stroke_color);
			target->DrawEllipse(elli, brush, m_stroke_width, m_d2d_stroke_style.get());
		}
		if (m_anc_show && is_selected()) {
			// �⏕����`��
			if (m_stroke_width >= Shape::m_anc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawEllipse(elli, brush, 2.0f * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawEllipse(elli, brush, m_aux_width, m_aux_style.get());
			}
			draw_anc();
		}
	}

	// �}�`���_���܂ނ����肷��.
	// test	���肳���_
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeEllipse::hit_test(const D2D1_POINT_2F t) const noexcept
	{
		const auto anc = rect_hit_test_anc(m_start, m_pos, t, m_anc_width);
		if (anc != ANC_TYPE::ANC_PAGE) {
			return anc;
		}

		// ���a�𓾂�.
		//D2D1_POINT_2F r;
		//pt_mul(m_pos, 0.5, r);
		double rx = 0.5 * m_pos.x;
		double ry = 0.5 * m_pos.y;
		// ���S�_�𓾂�.
		D2D1_POINT_2F ctr;
		pt_add(m_start, rx, ry, ctr);
		rx = fabsf(rx);
		ry = fabsf(ry);
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			// �ʒu�����~�̊O���ɂ��邩���肷��.
			// �g�̑��������ʂ̑傫�������Ȃ��,
			// ���ʂ̑傫����g�̑����Ɋi�[����.
			const double s_width = max(static_cast<double>(m_stroke_width), m_anc_width);
			// ���a�ɘg�̑����̔������������l���O�a�Ɋi�[����.
			D2D1_POINT_2F r_outer{
				rx + s_width * 0.5,
				ry + s_width * 0.5
			};
			//pt_add(r, s_width * 0.5, r_outer);
			if (!pt_in_ellipse(t, ctr, r_outer.x, r_outer.y)) {
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
			if (!pt_in_ellipse(t, ctr, r_inner.x, r_inner.y)) {
				return ANC_TYPE::ANC_STROKE;
			}
		}
		if (is_opaque(m_fill_color)) {
			// ���~�Ɉʒu���܂܂�邩���肷��.
			if (pt_in_ellipse(t, ctr, rx, ry)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

}