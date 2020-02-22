//------------------------------
// Shape_ruler.cpp
// 定規
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	ShapeRuler::~ShapeRuler(void)
	{
		m_text_fmt = nullptr;
	}

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
		const bool xy = fabs(m_vec.x) >= fabs(m_vec.y);
		const double diff_x = (xy ? m_vec.x : m_vec.y);
		const double diff_y = (xy ? m_vec.y : m_vec.x);
		const double grad_x = diff_x >= 0.0 ? g_len : -g_len;
		const double grad_y = min(f_size, g_len);
		const uint32_t k = static_cast<uint32_t>(floor(diff_x / grad_x));
		const double x0 = (xy ? m_pos.x : m_pos.y);
		const double y0 = static_cast<double>(xy ? m_pos.y : m_pos.x) + diff_y;
		const double y1 = y0 - (diff_y >= 0.0 ? grad_y : -grad_y);
		const double y2 = y1 - (diff_y >= 0.0 ? f_size : -f_size);
		DWRITE_PARAGRAPH_ALIGNMENT p_align;
		if (xy) {
			// 文字列を配置する方形が小さい (書体の大きさと同じ) ため,
			// DWRITE_PARAGRAPH_ALIGNMENT は, 逆の効果をもたらす.
			p_align = (m_vec.y >= 0.0f ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		}
		else {
			p_align = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		}
		m_text_fmt->SetParagraphAlignment(p_align);
		for (uint32_t i = 0; i <= k; i++) {
			const double x = x0 + i * grad_x;
			D2D1_POINT_2F p0{
				xy ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y0),
				xy ? static_cast<FLOAT>(y0) : static_cast<FLOAT>(x)
			};
			D2D1_POINT_2F p1{
				xy ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y1),
				xy ? static_cast<FLOAT>(y1) : static_cast<FLOAT>(x)
			};
			dx.m_d2dContext->DrawLine(p0, p1, br.get());
			const double x1 = x + f_size * 0.5;
			const double x2 = x1 - f_size;
			D2D1_RECT_F rect{
				xy ? static_cast<FLOAT>(x2) : static_cast<FLOAT>(y2),
				xy ? static_cast<FLOAT>(y2) : static_cast<FLOAT>(x2),
				xy ? static_cast<FLOAT>(x1) : static_cast<FLOAT>(y1),
				xy ? static_cast<FLOAT>(y1) : static_cast<FLOAT>(x1)
			};
			dx.m_d2dContext->DrawText(D[i % 10], 1u, m_text_fmt.get(), rect, br.get());
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

	//	図形を作成する.
	ShapeRuler::ShapeRuler(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, const ShapePanel* attr) :
		ShapeRect::ShapeRect(pos, vec, attr),
		m_grid_len(attr->m_grid_len)
	{
		wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);
		auto f_size = min(attr->m_font_size, attr->m_grid_len);
		winrt::check_hresult(
			Shape::s_dwrite_factory->CreateTextFormat(
				attr->m_font_family,
				static_cast<IDWriteFontCollection*>(nullptr),
				attr->m_font_weight,
				attr->m_font_style,
				DWRITE_FONT_STRETCH_NORMAL,
				static_cast<FLOAT>(f_size),
				locale_name,
				m_text_fmt.put()
			)
		);
		m_text_fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	// 図形をデータリーダーから読み込む.
	ShapeRuler::ShapeRuler(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader),
		m_grid_len(dt_reader.ReadDouble())
	{
		//	書体名
		wchar_t* f_family;
		read(f_family, dt_reader);
		//	書体の大きさ
		auto f_size = dt_reader.ReadDouble();
		//	字体
		DWRITE_FONT_STYLE f_style;
		read(f_style, dt_reader);
		//	書体の太さ
		DWRITE_FONT_WEIGHT f_weight;
		read(f_weight, dt_reader);

		wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);
		auto size = min(f_size, m_grid_len);
		winrt::check_hresult(
			Shape::s_dwrite_factory->CreateTextFormat(
				f_family,
				static_cast<IDWriteFontCollection*>(nullptr),
				f_weight,
				f_style,
				DWRITE_FONT_STRETCH_NORMAL,
				static_cast<FLOAT>(size),
				locale_name,
				m_text_fmt.put()
			)
		);
		delete[] f_family;
		m_text_fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	// データライターに書き込む.
	void ShapeRuler::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeRect::write(dt_writer);
		dt_writer.WriteDouble(m_grid_len);
		//	書体名
		auto n_size = m_text_fmt->GetFontFamilyNameLength() + 1;
		wchar_t* f_name = new wchar_t[n_size];
		m_text_fmt->GetFontFamilyName(f_name, n_size);
		write(f_name, dt_writer);
		delete[] f_name;
		//	書体の大きさ
		dt_writer.WriteDouble(m_text_fmt->GetFontSize());
		//	字体
		write(m_text_fmt->GetFontStyle(), dt_writer);
		//	書体の太さ
		write(m_text_fmt->GetFontWeight(), dt_writer);
	}

}