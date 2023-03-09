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
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		if (m_d2d_stroke_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			create_stroke_style(factory);
		}

		// ���a�����߂�.
		D2D1_POINT_2F rad;
		pt_mul(m_pos, 0.5, rad);
		// ���S�_�����߂�.
		D2D1_POINT_2F c;
		pt_add(m_start, rad, c);
		// ���~�\���̂Ɋi�[����.
		D2D1_ELLIPSE elli{ c, rad.x, rad.y };
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
		if (is_selected()) {
			// �⏕����`��
			if (m_stroke_width >= Shape::m_anc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawEllipse(elli, brush, 2.0 * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawEllipse(elli, brush, m_aux_width, m_aux_style.get());
			}
			draw_anc();
		}
	}

	// �ʒu���܂ނ����肷��.
	// test	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeEllipse::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		const auto anc = rect_hit_test_anc(m_start, m_pos, test, m_anc_width);
		if (anc != ANC_TYPE::ANC_PAGE) {
			return anc;
		}

		// ���a�𓾂�.
		D2D1_POINT_2F r;
		pt_mul(m_pos, 0.5, r);
		// ���S�_�𓾂�.
		D2D1_POINT_2F c;
		pt_add(m_start, r, c);
		r.x = fabsf(r.x);
		r.y = fabsf(r.y);
		if (is_opaque(m_stroke_color)) {
			// �ʒu�����~�̊O���ɂ��邩���肷��.
			// �g�̑��������ʂ̑傫�������Ȃ��,
			// ���ʂ̑傫����g�̑����Ɋi�[����.
			const double s_width = max(static_cast<double>(m_stroke_width), m_anc_width);
			// ���a�ɘg�̑����̔������������l���O�a�Ɋi�[����.
			D2D1_POINT_2F r_outer;
			pt_add(r, s_width * 0.5, r_outer);
			if (!pt_in_ellipse(test, c, r_outer.x, r_outer.y)) {
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
			if (!pt_in_ellipse(test, c, r_inner.x, r_inner.y)) {
				return ANC_TYPE::ANC_STROKE;
			}
		}
		if (is_opaque(m_fill_color)) {
			// ���~�Ɉʒu���܂܂�邩���肷��.
			if (pt_in_ellipse(test, c, r.x, r.y)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

}