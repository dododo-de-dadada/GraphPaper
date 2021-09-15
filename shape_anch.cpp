//#include <numbers>
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 図形の部位（円形）を表示する.
	// a_pos	部位の位置
	// dx		図形の描画環境
	void anch_draw_ellipse(const D2D1_POINT_2F a_pos, SHAPE_DX& dx)
	{
		const FLOAT rad = static_cast<FLOAT>(Shape::s_anch_len * 0.5 + 1.0);
		ID2D1SolidColorBrush* const brush = dx.m_shape_brush.get();
		brush->SetColor(Shape::m_default_background);
		dx.m_d2d_context->FillEllipse(D2D1_ELLIPSE{ a_pos, rad, rad }, brush);
		brush->SetColor(Shape::m_default_foreground);
		dx.m_d2d_context->FillEllipse(D2D1_ELLIPSE{ a_pos, rad - 1.0f, rad - 1.0f }, brush);
	}

	// 図形の部位 (方形) を表示する.
	// a_pos	部位の位置
	// dx		図形の描画環境
	void anch_draw_rect(const D2D1_POINT_2F a_pos, SHAPE_DX& dx)
	{
		D2D1_POINT_2F r_min;
		pt_add(a_pos, -0.5 * Shape::s_anch_len, r_min);
		D2D1_POINT_2F r_max;
		pt_add(r_min, Shape::s_anch_len, r_max);
		const D2D1_RECT_F rect{ r_min.x, r_min.y, r_max.x, r_max.y };
		ID2D1SolidColorBrush* brush = dx.m_shape_brush.get();
		brush->SetColor(Shape::m_default_background);
		dx.m_d2d_context->DrawRectangle(rect, brush, 2.0, nullptr);
		brush->SetColor(Shape::m_default_foreground);
		dx.m_d2d_context->FillRectangle(rect, brush);
	}

}