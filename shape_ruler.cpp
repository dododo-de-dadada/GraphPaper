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
			const double diff_x = m_vec.x;
			const double diff_y = m_vec.y;
			const double grad_x = diff_x >= 0.0 ? g_len : -g_len;
			const double grad_y = min(f_size, g_len);
			const uint32_t k = diff_x / grad_x;
			const double x0 = m_pos.x;
			const double y0 = static_cast<double>(m_pos.y) + diff_y;
			const double y1 = y0 - (diff_y >= 0.0 ? grad_y : -grad_y);
			const double y2 = y1 - (diff_y >= 0.0 ? f_size : -f_size);
			double x;
			for (uint32_t i = 0; i <= k; i++) {
				const auto x = x0 + i * grad_x;
				D2D1_POINT_2F p0{ x, y0 };
				D2D1_POINT_2F p1{ x, y1 };
				dx.m_d2dContext->DrawLine(p0, p1, br.get());
				D2D1_RECT_F rect{ x - f_size * 0.5, y2, x + f_size * 0.5, y1 };
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