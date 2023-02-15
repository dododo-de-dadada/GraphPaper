//------------------------------
// shape.cpp
// 図形のひな型, その他
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
#if defined(_DEBUG)
	uint32_t debug_deleted_cnt = 0;
	uint32_t debug_leak_cnt = 0;
	uint32_t debug_shape_cnt = 0;
#endif
	/*
	float Shape::s_anc_len = 6.0f;	// アンカーポイントの大きさ
	D2D1_COLOR_F Shape::s_background_color = COLOR_WHITE;	// 前景色 (アンカーの背景色)
	D2D1_COLOR_F Shape::s_foreground_color = COLOR_BLACK;	// 背景色 (アンカーの前景色)
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style = nullptr;	// 補助線の形式
	ID2D1Factory3* Shape::s_d2d_factory = nullptr;
	ID2D1RenderTarget* Shape::s_d2d_target = nullptr;
	ID2D1SolidColorBrush* Shape::s_d2d_color_brush = nullptr;
	ID2D1SolidColorBrush* Shape::s_d2d_range_brush = nullptr;
	IDWriteFactory* Shape::s_dwrite_factory = nullptr;
	*/
}
