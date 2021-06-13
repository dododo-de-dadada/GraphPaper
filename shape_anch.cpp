//#include <numbers>
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 図形の部位 (方形) を表示する.
	// a_pos	部位の位置
	// dx		図形の描画環境
	void anch_draw_rect(const D2D1_POINT_2F a_pos, SHAPE_DX& dx)
	{
		D2D1_POINT_2F r_min;
		pt_add(a_pos, -0.5 * Shape::s_anch_len, r_min);
		D2D1_POINT_2F r_max;
		pt_add(r_min, Shape::s_anch_len, r_max);
		const D2D1_RECT_F r{ r_min.x, r_min.y, r_max.x, r_max.y };

		dx.m_shape_brush->SetColor(Shape::m_default_background);
		dx.m_d2dContext->DrawRectangle(r, dx.m_shape_brush.get(), 2.0, nullptr);
		dx.m_shape_brush->SetColor(Shape::m_default_foreground);
		dx.m_d2dContext->FillRectangle(r, dx.m_shape_brush.get());
	}

	// 図形の部位（円形）を表示する.
	// a_pos	部位の位置
	// dx		図形の描画環境
	void anch_draw_ellipse(const D2D1_POINT_2F a_pos, SHAPE_DX& dx)
	{
		const FLOAT rad = static_cast<FLOAT>(Shape::s_anch_len * 0.5 + 1.0);
		dx.m_shape_brush->SetColor(Shape::m_default_background);
		dx.m_d2dContext->FillEllipse({ a_pos, rad, rad }, dx.m_shape_brush.get());
		dx.m_shape_brush->SetColor(Shape::m_default_foreground);
		dx.m_d2dContext->FillEllipse({ a_pos, rad - 1.0F, rad - 1.0F }, dx.m_shape_brush.get());
	}

}