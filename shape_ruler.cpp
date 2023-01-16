//------------------------------
// shape_ruler.cpp
// 定規
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 位置を含むか判定する.
	// t_pos	判定する位置
	uint32_t ShapeRuler::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		const uint32_t anc = rect_hit_test_anc(m_start, m_vec[0], t_pos);
		if (anc != ANC_TYPE::ANC_PAGE) {
			return anc;
		}
		if (is_opaque(m_stroke_color)) {
			const double g_len = m_grid_base + 1.0;
			const double f_size = m_dwrite_text_format->GetFontSize();
			const bool x_ge_y = fabs(m_vec[0].x) >= fabs(m_vec[0].y);
			const double vec_x = (x_ge_y ? m_vec[0].x : m_vec[0].y);
			const double vec_y = (x_ge_y ? m_vec[0].y : m_vec[0].x);
			const double grad_x = vec_x >= 0.0 ? g_len : -g_len;
			const double grad_y = min(f_size, g_len);
			const uint32_t k = static_cast<uint32_t>(floor(vec_x / grad_x));
			const double x0 = (x_ge_y ? m_start.x : m_start.y);
			const double y0 = static_cast<double>(x_ge_y ? m_start.y : m_start.x) + vec_y;
			const double y1 = y0 - (vec_y >= 0.0 ? grad_y : -grad_y);
			const double y1_5 = y0 - 0.625 * (vec_y >= 0.0 ? grad_y : -grad_y);
			const double y2 = y1 - (vec_y >= 0.0 ? f_size : -f_size);
			for (uint32_t i = 0; i <= k; i++) {
				// 方眼の大きさごとに目盛りを表示する.
				const double x = x0 + i * grad_x;
				D2D1_POINT_2F p0{
					x_ge_y ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y0),
					x_ge_y ? static_cast<FLOAT>(y0) : static_cast<FLOAT>(x)
				};
				const auto y = ((i % 5) == 0 ? y1 : y1_5);
				D2D1_POINT_2F p1{
					x_ge_y ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y),
					x_ge_y ? static_cast<FLOAT>(y) : static_cast<FLOAT>(x)
				};
				if (x_ge_y) {
					const float a_len = s_anc_len * 0.5f;
					const D2D1_POINT_2F p_min{ p0.x - a_len, min(p0.y, p1.y) };
					const D2D1_POINT_2F p_max{ p0.x + a_len, max(p0.y, p1.y) };
					if (pt_in_rect(t_pos, p_min, p_max)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				else {
					const float a_len = s_anc_len * 0.5f;
					const D2D1_POINT_2F p_min{ min(p0.x, p1.x), p0.y - a_len };
					const D2D1_POINT_2F p_max{ max(p0.x, p1.x), p0.y + a_len };
					if (pt_in_rect(t_pos, p_min, p_max)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				// 目盛りの値を表示する.
				const double x1 = x + f_size * 0.5;
				const double x2 = x1 - f_size;
				D2D1_POINT_2F r_nw = {
					x_ge_y ? static_cast<FLOAT>(x2) : static_cast<FLOAT>(y2),
					x_ge_y ? static_cast<FLOAT>(y2) : static_cast<FLOAT>(x2)
				};
				D2D1_POINT_2F r_sw = {
					x_ge_y ? static_cast<FLOAT>(x1) : static_cast<FLOAT>(y1),
					x_ge_y ? static_cast<FLOAT>(y1) : static_cast<FLOAT>(x1)
				};
				//pt_bound(r_nw, r_sw, r_nw, r_sw);
				/*
				if (r_nw.x > r_sw.x) {
					const auto less_x = r_sw.x;
					r_sw.x = r_nw.x;
					r_nw.x = less_x;
				}
				if (r_nw.y > r_sw.y) {
					const auto less_y = r_sw.y;
					r_sw.y = r_nw.y;
					r_nw.y = less_y;
				}
				*/
				if (pt_in_rect(t_pos, r_nw, r_sw)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
		}
		if (is_opaque(m_fill_color)) {
			D2D1_POINT_2F e_pos;
			pt_add(m_start, m_vec[0], e_pos);
			//D2D1_POINT_2F r_nw, r_sw;
			//pt_bound(m_start, e_pos, r_nw, r_sw);
			/*
			if (m_start.x < e_pos.x) {
				r_nw.x = m_start.x;
				r_sw.x = e_pos.x;
			}
			else {
				r_nw.x = e_pos.x;
				r_sw.x = m_start.x;
			}
			if (m_start.y < e_pos.y) {
				r_nw.y = m_start.y;
				r_sw.y = e_pos.y;
			}
			else {
				r_nw.y = e_pos.y;
				r_sw.y = m_start.y;
			}
			*/
			//if (pt_in_rect(t_pos, r_nw, r_sw)) {
			if (pt_in_rect(t_pos, m_start, e_pos)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	void ShapeRuler::create_text_format(void)
	{
		IDWriteFactory* const dwrite_factory = Shape::s_dwrite_factory;
		wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);
		const float font_size = min(m_font_size, m_grid_base + 1.0f);
		winrt::check_hresult(
			dwrite_factory->CreateTextFormat(
				m_font_family,
				static_cast<IDWriteFontCollection*>(nullptr),
				DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL,
				DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL,
				font_size,
				locale_name,
				m_dwrite_text_format.put()
			)
		);
		m_dwrite_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	// 図形を表示する.
	void ShapeRuler::draw(void)
	{
		ID2D1Factory* const factory = Shape::s_d2d_factory;
		ID2D1RenderTarget* const target = Shape::s_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::s_d2d_color_brush;

		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}
		if (m_dwrite_text_format == nullptr) {
			create_text_format();
		}
		constexpr wchar_t* D[10] = { L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9" };

		const D2D1_RECT_F rect{
			m_start.x,
			m_start.y,
			m_start.x + m_vec[0].x,
			m_start.y + m_vec[0].y
		};
		if (is_opaque(m_fill_color)) {
			// 塗りつぶし色が不透明な場合,
			// 方形を塗りつぶす.
			brush->SetColor(m_fill_color);
			target->FillRectangle(&rect, brush);
		}
		if (is_opaque(m_stroke_color)) {
			// 線枠の色が不透明な場合,
			const double g_len = m_grid_base + 1.0;	// 方眼の大きさ
			const double f_size = m_dwrite_text_format->GetFontSize();	// 書体の大きさ
			const bool w_ge_h = fabs(m_vec[0].x) >= fabs(m_vec[0].y);	// 高さより幅の方が大きい
			const double vec_x = (w_ge_h ? m_vec[0].x : m_vec[0].y);	// 大きい方の値を x
			const double vec_y = (w_ge_h ? m_vec[0].y : m_vec[0].x);	// 小さい方の値を y
			const double intvl_x = vec_x >= 0.0 ? g_len : -g_len;	// 目盛りの間隔
			const double intvl_y = min(f_size, g_len);	// 目盛りの間隔
			const uint32_t k = static_cast<uint32_t>(floor(vec_x / intvl_x));	// 目盛りの数
			const double x0 = (w_ge_h ? m_start.x : m_start.y);
			const double y0 = static_cast<double>(w_ge_h ? m_start.y : m_start.x) + vec_y;
			const double y1 = y0 - (vec_y >= 0.0 ? intvl_y : -intvl_y);
			const double y1_5 = y0 - 0.625 * (vec_y >= 0.0 ? intvl_y : -intvl_y);
			const double y2 = y1 - (vec_y >= 0.0 ? f_size : -f_size);
			DWRITE_PARAGRAPH_ALIGNMENT p_align;
			if (w_ge_h) {
				// 横のほうが大きい場合,
				// 高さが 0 以上の場合下よせ、ない場合上よせを段落のそろえに格納する.
				// 文字列を配置する方形が小さい (書体の大きさと同じ) ため,
				// DWRITE_PARAGRAPH_ALIGNMENT は, 逆の効果をもたらす.
				p_align = (m_vec[0].y >= 0.0f ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			}
			else {
				// 縦のほうが小さい場合,
				// 中段を段落のそろえに格納する.
				p_align = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			}
			// 段落のそろえをテキストフォーマットに格納する.
			m_dwrite_text_format->SetParagraphAlignment(p_align);
			brush->SetColor(m_stroke_color);
			for (uint32_t i = 0; i <= k; i++) {
				// 方眼の大きさごとに目盛りを表示する.
				const double x = x0 + i * intvl_x;
				D2D1_POINT_2F p0{
					w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y0),
					w_ge_h ? static_cast<FLOAT>(y0) : static_cast<FLOAT>(x)
				};
				const auto y = ((i % 5) == 0 ? y1 : y1_5);
				D2D1_POINT_2F p1{
					w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y),
					w_ge_h ? static_cast<FLOAT>(y) : static_cast<FLOAT>(x)
				};
				target->DrawLine(p0, p1, brush);
				// 目盛りの値を表示する.
				const double x1 = x + f_size * 0.5;
				const double x2 = x1 - f_size;
				D2D1_RECT_F t_rect{
					w_ge_h ? static_cast<FLOAT>(x2) : static_cast<FLOAT>(y2),
					w_ge_h ? static_cast<FLOAT>(y2) : static_cast<FLOAT>(x2),
					w_ge_h ? static_cast<FLOAT>(x1) : static_cast<FLOAT>(y1),
					w_ge_h ? static_cast<FLOAT>(y1) : static_cast<FLOAT>(x1)
				};
				target->DrawRectangle(t_rect, brush);
				target->DrawText(D[i % 10], 1u, m_dwrite_text_format.get(), t_rect, brush);
			}
		}
		if (is_selected()) {
			// 選択フラグが立っている場合,
			// 選択されているなら基準部位を表示する.
			D2D1_POINT_2F r_pos[4];	// 方形の頂点
			r_pos[0] = m_start;
			r_pos[1].y = rect.top;
			r_pos[1].x = rect.right;
			r_pos[2].x = rect.right;
			r_pos[2].y = rect.bottom;
			r_pos[3].y = rect.bottom;
			r_pos[3].x = rect.left;
			for (uint32_t i = 0, j = 3; i < 4; j = i++) {
				anc_draw_rect(r_pos[i], target, brush);
				D2D1_POINT_2F r_mid;	// 方形の辺の中点
				pt_avg(r_pos[j], r_pos[i], r_mid);
				anc_draw_rect(r_mid, target, brush);
			}
		}
	}

	// 書体名を得る.
	bool ShapeRuler::get_font_family(wchar_t*& val) const noexcept
	{
		val = m_font_family;
		return true;
	}

	// 書体の大きさを得る.
	bool ShapeRuler::get_font_size(float& val) const noexcept
	{
		val = m_font_size;
		return true;
	}

	// 値を書体名に格納する.
	bool ShapeRuler::set_font_family(wchar_t* const val) noexcept
	{
		// 値が書体名と同じか判定する.
		if (!equal(m_font_family, val)) {
			m_font_family = val;
			m_dwrite_text_format = nullptr;
			return true;
		}
		return false;
	}

	// 値を書体の大きさに格納する.
	bool ShapeRuler::set_font_size(const float val) noexcept
	{
		if (m_font_size != val) {
			m_font_size = val;
			m_dwrite_text_format = nullptr;
			return true;
		}
		return false;
	}

	// 図形を作成する.
	// b_pos	囲む領域の始点
	// b_vec	囲む領域の終点への差分
	// page	属性
	ShapeRuler::ShapeRuler(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapePage* page) :
		ShapeRect::ShapeRect(b_pos, b_vec, page),
		m_grid_base(page->m_grid_base),
		m_font_family(page->m_font_family),
		//m_font_size(min(page->m_font_size, page->m_grid_base + 1.0f))
		m_font_size(page->m_font_size)
	{
		ShapeText::is_available_font(m_font_family);
	}

	static wchar_t* dt_read_name(DataReader const& dt_reader)
	{
		const size_t len = dt_reader.ReadUInt32();	// 文字数
		uint8_t* data = new uint8_t[2 * (len + 1)];
		dt_reader.ReadBytes(array_view(data, data + 2 * len));
		wchar_t* val = reinterpret_cast<wchar_t*>(data);
		val[len] = L'\0';
		return val;
	}

	// 図形をデータリーダーから読み込む.
	ShapeRuler::ShapeRuler(const ShapePage& page, DataReader const& dt_reader) :
		ShapeRect::ShapeRect(page, dt_reader),
		m_grid_base(dt_reader.ReadSingle()),
		m_font_family(dt_read_name(dt_reader)),
		m_font_size(dt_reader.ReadSingle())
	{
		ShapeText::is_available_font(m_font_family);
	}

	// 図形をデータライターに書き込む.
	void ShapeRuler::write(DataWriter const& dt_writer) const
	{
		ShapeRect::write(dt_writer);
		dt_writer.WriteSingle(m_grid_base);
		const uint32_t font_family_len = wchar_len(m_font_family);
		dt_writer.WriteUInt32(font_family_len);
		const auto font_family_data = reinterpret_cast<const uint8_t*>(m_font_family);
		dt_writer.WriteBytes(array_view(font_family_data, font_family_data + 2 * font_family_len));
		dt_writer.WriteSingle(m_dwrite_text_format->GetFontSize());
	}

}