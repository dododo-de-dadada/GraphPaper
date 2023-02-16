//------------------------------
// Shape_elli.cpp
// だ円
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataWriter;

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
		D2D1_POINT_2F rad;
		pt_mul(m_vec[0], 0.5, rad);
		// 中心点を求める.
		D2D1_POINT_2F c_pos;
		pt_add(m_start, rad, c_pos);
		// だ円構造体に格納する.
		D2D1_ELLIPSE elli{ c_pos, rad.x, rad.y };
		// 塗りつぶし色が不透明か判定する.
		if (is_opaque(m_fill_color)) {
			brush->SetColor(m_fill_color);
			target->FillEllipse(elli, brush);
		}
		// 枠線の色が不透明か判定する.
		if (is_opaque(m_stroke_color)) {
			brush->SetColor(m_stroke_color);
			target->DrawEllipse(elli, brush, m_stroke_width, m_d2d_stroke_style.get());
		}
		if (!is_selected()) {
			return;
		}
		draw_anc();
	}

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeEllipse::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		const auto anc = rect_hit_test_anc(m_start, m_vec[0], t_pos, m_anc_width);
		if (anc != ANC_TYPE::ANC_PAGE) {
			return anc;
		}

		// 半径を得る.
		D2D1_POINT_2F rad;
		pt_mul(m_vec[0], 0.5, rad);
		// 中心点を得る.
		D2D1_POINT_2F c_pos;
		pt_add(m_start, rad, c_pos);
		rad.x = fabsf(rad.x);
		rad.y = fabsf(rad.y);
		if (is_opaque(m_stroke_color)) {
			// 位置がだ円の外側にあるか判定する.
			// 枠の太さが部位の大きさ未満ならば,
			// 部位の大きさを枠の太さに格納する.
			const double s_width = max(static_cast<double>(m_stroke_width), m_anc_width);
			// 半径に枠の太さの半分を加えた値を外径に格納する.
			D2D1_POINT_2F r_outer;
			pt_add(rad, s_width * 0.5, r_outer);
			if (!pt_in_ellipse(t_pos, c_pos, r_outer.x, r_outer.y)) {
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
			if (!pt_in_ellipse(t_pos, c_pos, r_inner.x, r_inner.y)) {
				return ANC_TYPE::ANC_STROKE;
			}
		}
		if (is_opaque(m_fill_color)) {
			// だ円に位置が含まれるか判定する.
			if (pt_in_ellipse(t_pos, c_pos, rad.x, rad.y)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

}