//------------------------------
// shape_ruler.cpp
// 定規
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 図形が点を含むか判定する.
	// test_pt	判定される点
	uint32_t ShapeRuler::hit_test(const D2D1_POINT_2F test_pt, const bool/*ctrl_key*/) const noexcept
	{
		const uint32_t hit = rect_hit_test(m_start, m_lineto, test_pt, m_hit_width);
		if (hit != HIT_TYPE::HIT_SHEET) {
			return hit;
		}
		if (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color)) {
			const double g_len = m_grid_base + 1.0;
			const double f_size = m_dwrite_text_format->GetFontSize();
			const bool x_ge_y = fabs(m_lineto.x) >= fabs(m_lineto.y);
			const double to_x = (x_ge_y ? m_lineto.x : m_lineto.y);
			const double to_y = (x_ge_y ? m_lineto.y : m_lineto.x);
			const double grad_x = to_x >= 0.0 ? g_len : -g_len;
			const double grad_y = min(f_size, g_len);
			const uint32_t k = static_cast<uint32_t>(floor(to_x / grad_x));
			const double x0 = (x_ge_y ? m_start.x : m_start.y);
			const double y0 = static_cast<double>(x_ge_y ? m_start.y : m_start.x) + to_y;
			const double y1 = y0 - (to_y >= 0.0 ? grad_y : -grad_y);
			const double y1_5 = y0 - 0.625 * (to_y >= 0.0 ? grad_y : -grad_y);
			const double y2 = y1 - (to_y >= 0.0 ? f_size : -f_size);
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
					const D2D1_POINT_2F p_min{
						static_cast<FLOAT>(p0.x - m_hit_width * 0.5), min(p0.y, p1.y)
					};
					const D2D1_POINT_2F p_max{
						static_cast<FLOAT>(p0.x + m_hit_width * 0.5f), max(p0.y, p1.y)
					};
					if (pt_in_rect(test_pt, p_min, p_max)) {
						return HIT_TYPE::HIT_STROKE;
					}
				}
				else {
					const D2D1_POINT_2F p_min{
						min(p0.x, p1.x), static_cast<FLOAT>(p0.y - m_hit_width * 0.5)
					};
					const D2D1_POINT_2F p_max{
						max(p0.x, p1.x), static_cast<FLOAT>(p0.y + m_hit_width * 0.5)
					};
					if (pt_in_rect(test_pt, p_min, p_max)) {
						return HIT_TYPE::HIT_STROKE;
					}
				}
				// 目盛りの値を表示する.
				const double x1 = x + f_size * 0.5;
				const double x2 = x1 - f_size;
				D2D1_POINT_2F r_lt = {
					x_ge_y ? static_cast<FLOAT>(x2) : static_cast<FLOAT>(y2),
					x_ge_y ? static_cast<FLOAT>(y2) : static_cast<FLOAT>(x2)
				};
				D2D1_POINT_2F r_rb = {
					x_ge_y ? static_cast<FLOAT>(x1) : static_cast<FLOAT>(y1),
					x_ge_y ? static_cast<FLOAT>(y1) : static_cast<FLOAT>(x1)
				};
				//pt_bound(r_lt, r_rb, r_lt, r_rb);
				/*
				if (r_lt.x > r_rb.x) {
					const auto less_x = r_rb.x;
					r_rb.x = r_lt.x;
					r_lt.x = less_x;
				}
				if (r_lt.y > r_rb.y) {
					const auto less_y = r_rb.y;
					r_rb.y = r_lt.y;
					r_lt.y = less_y;
				}
				*/
				if (pt_in_rect(test_pt, r_lt, r_rb)) {
					return HIT_TYPE::HIT_STROKE;
				}
			}
		}
		if (is_opaque(m_fill_color)) {
			D2D1_POINT_2F end_pt;
			pt_add(m_start, m_lineto, end_pt);
			if (pt_in_rect(test_pt, m_start, end_pt)) {
				return HIT_TYPE::HIT_FILL;
			}
		}
		return HIT_TYPE::HIT_SHEET;
	}

	HRESULT ShapeRuler::create_text_format(void) noexcept
	{
		IDWriteFactory* const dwrite_factory = SHAPE::m_dwrite_factory.get();
		wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);

		const float font_size = m_font_size;
		HRESULT hr = dwrite_factory->CreateTextFormat(
			m_font_family,
			static_cast<IDWriteFontCollection*>(nullptr),
			DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL,
			font_size,
			locale_name,
			m_dwrite_text_format.put()
		);
		if (hr == S_OK) {
			hr = m_dwrite_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
		}
		if (hr == S_OK) {
			hr = m_dwrite_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		}
		return hr;
	}

	// 図形を表示する.
	void ShapeRuler::draw(void) noexcept
	{
		HRESULT hr = S_OK;
		ID2D1RenderTarget* const target = SHAPE::m_d2d_target;
		ID2D1SolidColorBrush* const brush = SHAPE::m_d2d_color_brush.get();
		const bool exist_stroke = (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color));

		if (exist_stroke && m_d2d_stroke_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			hr = create_stroke_style(factory);
		}
		if (is_opaque(m_stroke_color) && m_dwrite_text_format == nullptr) {
			hr = create_text_format();
		}
		constexpr wchar_t* D[10] = { L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9" };

		const D2D1_RECT_F rect{
			m_start.x,
			m_start.y,
			m_start.x + m_lineto.x,
			m_start.y + m_lineto.y
		};
		if (is_opaque(m_fill_color)) {
			// 塗りつぶし色が不透明な場合,
			// 方形を塗りつぶす.
			brush->SetColor(m_fill_color);
			target->FillRectangle(&rect, brush);
		}
		if (exist_stroke) {
			// 線枠の色が不透明な場合,
			const double g_len = m_grid_base + 1.0;	// 方眼の大きさ
			const double f_size = m_dwrite_text_format->GetFontSize();	// 書体の大きさ
			const bool w_ge_h = fabs(m_lineto.x) >= fabs(m_lineto.y);	// 高さより幅の方が大きい
			const double to_x = (w_ge_h ? m_lineto.x : m_lineto.y);	// 大きい方の値を x
			const double to_y = (w_ge_h ? m_lineto.y : m_lineto.x);	// 小さい方の値を y
			const double intvl_x = to_x >= 0.0 ? g_len : -g_len;	// 目盛りの間隔
			const double intvl_y = min(f_size, g_len);	// 目盛りの間隔
			const uint32_t k = static_cast<uint32_t>(floor(to_x / intvl_x));	// 目盛りの数
			const double x0 = (w_ge_h ? m_start.x : m_start.y);
			const double y0 = static_cast<double>(w_ge_h ? m_start.y : m_start.x) + to_y;
			const double y1 = y0 - (to_y >= 0.0 ? intvl_y : -intvl_y);
			const double y1_5 = y0 - 0.625 * (to_y >= 0.0 ? intvl_y : -intvl_y);
			const double y2 = y1 - (to_y >= 0.0 ? f_size : -f_size);
			/*
			DWRITE_PARAGRAPH_ALIGNMENT p_align;
			if (w_ge_h) {
				// 横のほうが大きい場合,
				// 高さが 0 以上の場合下よせ、ない場合上よせを段落のそろえに格納する.
				// 文字列を配置する方形が小さい (書体の大きさと同じ) ため,
				// DWRITE_PARAGRAPH_ALIGNMENT は, 逆の効果をもたらす.
				p_align = (m_pos[0].y >= 0.0f ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			}
			else {
				// 縦のほうが小さい場合,
				// 中段を段落のそろえに格納する.
				p_align = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			}
			// 段落のそろえをテキストフォーマットに格納する.
			m_dwrite_text_format->SetParagraphAlignment(p_align);
			*/
			brush->SetColor(m_stroke_color);
			for (uint32_t i = 0; i <= k; i++) {

				// 目盛りを表示する.
				const double x = x0 + i * intvl_x;
				D2D1_POINT_2F p{
					w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y0),
					w_ge_h ? static_cast<FLOAT>(y0) : static_cast<FLOAT>(x)
				};
				const auto y = ((i % 5) == 0 ? y1 : y1_5);
				D2D1_POINT_2F q{
					w_ge_h ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y),
					w_ge_h ? static_cast<FLOAT>(y) : static_cast<FLOAT>(x)
				};
				target->DrawLine(p, q, brush);

				// 目盛りラベルを表示する.
				const double x1 = x + f_size * 0.5;
				const double x2 = x1 - f_size;
				D2D1_RECT_F r{
					w_ge_h ? static_cast<FLOAT>(x2) : static_cast<FLOAT>(y2),
					w_ge_h ? static_cast<FLOAT>(y2) : static_cast<FLOAT>(x2),
					w_ge_h ? static_cast<FLOAT>(x1) : static_cast<FLOAT>(y1),
					w_ge_h ? static_cast<FLOAT>(y1) : static_cast<FLOAT>(x1)
				};
				target->DrawText(D[i % 10], 1u, m_dwrite_text_format.get(), r, brush);
			}
		}
		if (m_hit_show && is_selected()) {
			draw_hit();
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
	// start	始点
	// end_to	終点への位置ベクトル
	// prop	属性
	ShapeRuler::ShapeRuler(const D2D1_POINT_2F start, const D2D1_POINT_2F end_to, const SHAPE* prop) :
		SHAPE_CLOSED::SHAPE_CLOSED(start, end_to, prop)
	{
		ShapeText::is_available_font(m_font_family);
		prop->get_grid_base(m_grid_base);
		prop->get_font_family(m_font_family);
		prop->get_font_size(m_font_size);
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

	// 字面を得る (使用後は Release する).
	bool ShapeRuler::get_font_face(IDWriteFontFace3*& face) const noexcept
	{
		return text_get_font_face(
		//return text_get_font_face<IDWriteTextFormat>(
			m_dwrite_text_format.get(), m_font_family, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, face);
	}

	// 図形をデータリーダーから読み込む.
	ShapeRuler::ShapeRuler(DataReader const& dt_reader) :
		SHAPE_CLOSED::SHAPE_CLOSED(dt_reader),
		m_grid_base(dt_reader.ReadSingle()),
		m_font_family(dt_read_name(dt_reader)),
		m_font_size(dt_reader.ReadSingle())
	{
		if (m_grid_base < 0.0f) {
			m_grid_base = GRID_LEN_DEFVAL - 1.0f;
		}
		if (m_font_size < 0.0f) {
			m_font_size = FONT_SIZE_DEFVAL;
		}
		ShapeText::is_available_font(m_font_family);
	}

	// 図形をデータライターに書き込む.
	void ShapeRuler::write(DataWriter const& dt_writer) const
	{
		SHAPE_CLOSED::write(dt_writer);
		dt_writer.WriteSingle(m_grid_base);
		const uint32_t f_len = wchar_len(m_font_family);
		dt_writer.WriteUInt32(f_len);
		const auto font_family_data = reinterpret_cast<const uint8_t*>(m_font_family);
		dt_writer.WriteBytes(array_view(font_family_data, font_family_data + 2 * static_cast<size_t>(f_len)));
		dt_writer.WriteSingle(m_dwrite_text_format->GetFontSize());
	}

}