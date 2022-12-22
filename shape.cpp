//------------------------------
// shape.cpp
// �}�`�̂ЂȌ^, ���̑�
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
	float Shape::s_anc_len = 6.0f;	// �A���J�[�|�C���g�̑傫��
	D2D1_COLOR_F Shape::s_background_color = COLOR_WHITE;	// �O�i�F (�A���J�[�̔w�i�F)
	D2D1_COLOR_F Shape::s_foreground_color = COLOR_BLACK;	// �w�i�F (�A���J�[�̑O�i�F)
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style = nullptr;	// �⏕���̌`��
	ID2D1Factory3* Shape::s_factory = nullptr;
	ID2D1RenderTarget* Shape::s_target = nullptr;
	ID2D1SolidColorBrush* Shape::s_color_brush = nullptr;
	IDWriteFactory* Shape::s_dw_factory = nullptr;
	ID2D1SolidColorBrush* Shape::s_range_brush = nullptr;

	// ��_�ň͂܂ꂽ���`�𓾂�.
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
