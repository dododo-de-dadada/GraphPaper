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
	void ShapeEllipse::draw(void) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color) && m_d2d_stroke_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			create_stroke_style(factory);
		}

		// ���a�����߂�.
		const double rx = 0.5 * m_pos.x;
		const double ry = 0.5 * m_pos.y;
		// ���~�\���̂Ɋi�[����.
		const D2D1_ELLIPSE elli{ 
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
		if (m_loc_show && is_selected()) {
			// �⏕����`��
			if (m_stroke_width >= Shape::m_loc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawEllipse(elli, brush, 2.0f * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawEllipse(elli, brush, m_aux_width, m_aux_style.get());
			}
			draw_loc();
		}
	}

	// �}�`���_���܂ނ����肷��.
	// �߂�l	�_���܂ޕ���
	uint32_t ShapeEllipse::hit_test(const D2D1_POINT_2F pt, const bool/*ctrl_key*/) const noexcept
	{
		const auto loc = rect_loc_hit_test(m_start, m_pos, pt, m_loc_width);
		if (loc != LOC_TYPE::LOC_PAGE) {
			return loc;
		}

		// ���a�𓾂�.
		double rx = 0.5 * m_pos.x;
		double ry = 0.5 * m_pos.y;
		// ���S�_�𓾂�.
		const D2D1_POINT_2F c{
			static_cast<FLOAT>(m_start.x + rx),
			static_cast<FLOAT>(m_start.y + ry)
		};
		rx = abs(rx);
		ry = abs(ry);
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			// ���~�̊O�a�ɑ΂�, �_�̓��O�𔻒肷��.
			const double s_width = static_cast<double>(max(m_stroke_width, m_loc_width));
			const double ox = rx + s_width * 0.5;
			const double oy = ry + s_width * 0.5;
			if (!pt_in_ellipse(pt, c, ox, oy)) {
				// �O���Ȃ� LOC_PAGE ��Ԃ�.
				return LOC_TYPE::LOC_PAGE;
			}
			// ���~�̓��a�ɑ΂�, �_�̓��O�𔻒肷��.
			const double ix = ox - s_width;
			const double iy = oy - s_width;
			if (ix <= 0.0 || iy <= 0.0 || !pt_in_ellipse(pt, c, ix, iy)) {
				// ���a������, �A�܂��͓_���O���Ȃ� LOC_STROKE ��Ԃ�.
				return LOC_TYPE::LOC_STROKE;
			}
		}
		if (is_opaque(m_fill_color)) {
			// ���~�ɓ_���܂܂�邩���肷��.
			if (pt_in_ellipse(pt, c, rx, ry)) {
				return LOC_TYPE::LOC_FILL;
			}
		}
		return LOC_TYPE::LOC_PAGE;
	}

}