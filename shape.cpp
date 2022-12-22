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
	float Shape::s_anc_len = 6.0f;	// アンカーポイントの大きさ
	D2D1_COLOR_F Shape::s_background_color = COLOR_WHITE;	// 前景色 (アンカーの背景色)
	D2D1_COLOR_F Shape::s_foreground_color = COLOR_BLACK;	// 背景色 (アンカーの前景色)
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style = nullptr;	// 補助線の形式
	ID2D1Factory3* Shape::s_factory = nullptr;
	ID2D1RenderTarget* Shape::s_target = nullptr;
	ID2D1SolidColorBrush* Shape::s_color_brush = nullptr;
	IDWriteFactory* Shape::s_dw_factory = nullptr;
	ID2D1SolidColorBrush* Shape::s_range_brush = nullptr;

	// 二点で囲まれた方形を得る.
	/*
	void pt_bound(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c_min, D2D1_POINT_2F& c_max) noexcept
	{
		if (a.x < b.x) {
			c_min.x = a.x;
			c_max.x = b.x;
		}
		else {
			c_min.x = b.x;
			c_max.x = a.x;
		}
		if (a.y < b.y) {
			c_min.y = a.y;
			c_max.y = b.y;
		}
		else {
			c_min.y = b.y;
			c_max.y = a.y;
		}
	}
	*/
}
