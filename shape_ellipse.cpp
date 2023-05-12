//------------------------------
// Shape_elli.cpp
// だ円
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 図形を表示する.
	void ShapeEllipse::draw(void) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color) && m_d2d_stroke_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			create_stroke_style(factory);
		}

		// 半径を求める.
		const double rx = 0.5 * m_pos.x;
		const double ry = 0.5 * m_pos.y;
		// だ円構造体に格納する.
		const D2D1_ELLIPSE elli{ 
			D2D1_POINT_2F {
				static_cast<FLOAT>(m_start.x + rx), static_cast<FLOAT>(m_start.y + ry)
			},
			static_cast<FLOAT>(rx), static_cast<FLOAT>(ry)
		};
		// 塗りつぶし色が不透明か判定する.
		if (is_opaque(m_fill_color)) {
			brush->SetColor(m_fill_color);
			target->FillEllipse(elli, brush);
		}
		// 枠線の色が不透明か判定する.
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			brush->SetColor(m_stroke_color);
			target->DrawEllipse(elli, brush, m_stroke_width, m_d2d_stroke_style.get());
		}
		if (m_loc_show && is_selected()) {
			// 補助線を描く
			if (m_stroke_width >= Shape::m_loc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawEllipse(elli, brush, 2.0f * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawEllipse(elli, brush, m_aux_width, m_aux_style.get());
			}
			draw_loc();
		}
	}

	// 図形が点を含むか判定する.
	// 戻り値	点を含む部位
	uint32_t ShapeEllipse::hit_test(const D2D1_POINT_2F pt, const bool/*ctrl_key*/) const noexcept
	{
		const auto loc = rect_loc_hit_test(m_start, m_pos, pt, m_loc_width);
		if (loc != LOC_TYPE::LOC_PAGE) {
			return loc;
		}

		// 半径を得る.
		double rx = 0.5 * m_pos.x;
		double ry = 0.5 * m_pos.y;
		// 中心点を得る.
		const D2D1_POINT_2F c{
			static_cast<FLOAT>(m_start.x + rx),
			static_cast<FLOAT>(m_start.y + ry)
		};
		rx = abs(rx);
		ry = abs(ry);
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			// だ円の外径に対し, 点の内外を判定する.
			const double s_width = static_cast<double>(max(m_stroke_width, m_loc_width));
			const double ox = rx + s_width * 0.5;
			const double oy = ry + s_width * 0.5;
			if (!pt_in_ellipse(pt, c, ox, oy)) {
				// 外側なら LOC_PAGE を返す.
				return LOC_TYPE::LOC_PAGE;
			}
			// だ円の内径に対し, 点の内外を判定する.
			const double ix = ox - s_width;
			const double iy = oy - s_width;
			if (ix <= 0.0 || iy <= 0.0 || !pt_in_ellipse(pt, c, ix, iy)) {
				// 内径が負数, 、または点が外側なら LOC_STROKE を返す.
				return LOC_TYPE::LOC_STROKE;
			}
		}
		if (is_opaque(m_fill_color)) {
			// だ円に点が含まれるか判定する.
			if (pt_in_ellipse(pt, c, rx, ry)) {
				return LOC_TYPE::LOC_FILL;
			}
		}
		return LOC_TYPE::LOC_PAGE;
	}

}