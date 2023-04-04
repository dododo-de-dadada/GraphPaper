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
	void ShapeEllipse::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		if (m_d2d_stroke_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			create_stroke_style(factory);
		}

		// 半径を求める.
		const double rx = 0.5 * m_pos.x;
		const double ry = 0.5 * m_pos.y;
		// だ円構造体に格納する.
		D2D1_ELLIPSE elli{ 
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
		if (m_anc_show && is_selected()) {
			// 補助線を描く
			if (m_stroke_width >= Shape::m_anc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawEllipse(elli, brush, 2.0f * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawEllipse(elli, brush, m_aux_width, m_aux_style.get());
			}
			draw_anc();
		}
	}

	// 図形が点を含むか判定する.
	// test	判定される点
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeEllipse::hit_test(const D2D1_POINT_2F t) const noexcept
	{
		const auto anc = rect_hit_test_anc(m_start, m_pos, t, m_anc_width);
		if (anc != ANC_TYPE::ANC_PAGE) {
			return anc;
		}

		// 半径を得る.
		//D2D1_POINT_2F r;
		//pt_mul(m_pos, 0.5, r);
		double rx = 0.5 * m_pos.x;
		double ry = 0.5 * m_pos.y;
		// 中心点を得る.
		D2D1_POINT_2F ctr;
		pt_add(m_start, rx, ry, ctr);
		rx = fabsf(rx);
		ry = fabsf(ry);
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			// 位置がだ円の外側にあるか判定する.
			// 枠の太さが部位の大きさ未満ならば,
			// 部位の大きさを枠の太さに格納する.
			const double s_width = max(static_cast<double>(m_stroke_width), m_anc_width);
			// 半径に枠の太さの半分を加えた値を外径に格納する.
			D2D1_POINT_2F r_outer{
				rx + s_width * 0.5,
				ry + s_width * 0.5
			};
			//pt_add(r, s_width * 0.5, r_outer);
			if (!pt_in_ellipse(t, ctr, r_outer.x, r_outer.y)) {
				// 外径のだ円に含まれないなら, 
				// ANC_PAGE を返す.
				return ANC_TYPE::ANC_PAGE;
			}
			// 位置がだ円の枠上にあるか判定する.
			D2D1_POINT_2F r_inner;
			// 外径から枠の太さを引いた値を内径に格納する.
			pt_add(r_outer, -s_width, r_inner);
			// 内径が負数なら,
			// ANC_STROKE を返す.
			if (r_inner.x <= 0.0f) {
				return ANC_TYPE::ANC_STROKE;
			}
			if (r_inner.y <= 0.0f) {
				return ANC_TYPE::ANC_STROKE;
			}
			// 内径のだ円に含まれないか判定する.
			if (!pt_in_ellipse(t, ctr, r_inner.x, r_inner.y)) {
				return ANC_TYPE::ANC_STROKE;
			}
		}
		if (is_opaque(m_fill_color)) {
			// だ円に位置が含まれるか判定する.
			if (pt_in_ellipse(t, ctr, rx, ry)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

}