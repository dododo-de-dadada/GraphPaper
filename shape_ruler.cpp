//------------------------------
// Shape_ruler.cpp
// 定規
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 図形を表示する.
	void ShapeRuler::draw(SHAPE_DX& dx)
	{
		wchar_t* D[10] = { L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9" };
		auto br = dx.m_shape_brush;

		if (is_opaque(m_fill_color)) {
			D2D1_RECT_F rect{
				m_pos.x,
				m_pos.y,
				m_pos.x + m_vec.x,
				m_pos.y + m_vec.y
			};
			br->SetColor(m_fill_color);
			dx.m_d2dContext->FillRectangle(rect, br.get());
		}
		br->SetColor(m_stroke_color);
		const double g_len = static_cast<double>(m_grid_len) + 1.0;
		const double f_size = m_text_fmt->GetFontSize();
		if (fabs(m_vec.x) >= fabs(m_vec.y)) {
			const double vec_x = m_vec.x;
			const double pos_x = m_pos.x;
			const double delta = vec_x >= 0.0 ? g_len : -g_len;
			const double end_x = pos_x + vec_x + (vec_x >= 0.0 ? FLT_MIN : -FLT_MIN);
			const double bot_y = static_cast<double>(m_pos.y) + static_cast<double>(m_vec.y);
			const double top_y = bot_y - (m_vec.y >= 0.0f ? min(f_size, g_len) : -min(f_size, g_len));
			double x;
			for (size_t i = 0; (x = pos_x + i * delta) <= end_x; i++) {
				D2D1_POINT_2F grad0{ x, bot_y };
				D2D1_POINT_2F grad1{ x, top_y };
				dx.m_d2dContext->DrawLine(grad0, grad1, br.get());
				D2D1_RECT_F rect{ x - f_size * 0.5, top_y - (m_vec.y >= 0.0f ? f_size : -f_size), x + f_size * 0.5,  top_y };
				dx.m_d2dContext->DrawText(D[i % 10], 1u, m_text_fmt.get(), rect, br.get());
			}
		}
		else {
			const double vec_y = m_vec.y;
			const double pos_y = m_pos.y;
			const double delta = vec_y >= 0.0 ? g_len : -g_len;
			const double end_y = pos_y + vec_y + (vec_y >= 0.0 ? FLT_MIN : -FLT_MIN);
			const double right_x = static_cast<double>(m_pos.x) + static_cast<double>(m_vec.x);
			const double left_x = right_x - min(f_size, g_len);
			double y;
			for (size_t i = 0; (y = pos_y + i * delta) <= end_y; y = pos_y + i * delta, i++) {
				D2D1_POINT_2F grad0{ left_x, y };
				D2D1_POINT_2F grad1{ right_x, y };
				dx.m_d2dContext->DrawLine(grad0, grad1, br.get());
				D2D1_RECT_F rect{ left_x - f_size, y - f_size * 0.5, left_x, y + f_size * 0.5 };
				dx.m_d2dContext->DrawText(D[i % 10], 1u, m_text_fmt.get(), rect, br.get());
			}

		}

		if (is_selected()) {
			D2D1_POINT_2F r_pos[4];
			// 選択されているなら基準部位を表示する.
			r_pos[0] = m_pos;
			r_pos[1].x = m_pos.x + m_vec.x;
			r_pos[1].y = m_pos.y;
			r_pos[2].x = r_pos[1].x;
			r_pos[2].y = m_pos.y + m_vec.y;
			r_pos[3].x = m_pos.x;
			r_pos[3].y = r_pos[2].y;
			for (uint32_t i = 0, j = 3; i < 4; j = i++) {
				draw_anchor(r_pos[i], dx);
				D2D1_POINT_2F r_mid;
				pt_avg(r_pos[j], r_pos[i], r_mid);
				draw_anchor(r_mid, dx);
			}
		}
	}

}