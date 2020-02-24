//------------------------------
// Shape_ruler.cpp
// 定規
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 図形を破棄する.
	ShapeRuler::~ShapeRuler(void)
	{
		m_dw_text_fmt = nullptr;
	}

	//	図形を表示する.
	//	dx	描画環境
	void ShapeRuler::draw(SHAPE_DX& dx)
	{
		wchar_t* D[10] = { L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9" };
		auto br = dx.m_shape_brush;

		const auto a = m_stroke_color.a;
		m_stroke_color.a = 0.0f;
		ShapeRect::draw(dx);
		m_stroke_color.a = a;

		const double g_len = static_cast<double>(m_grid_len) + 1.0;
		const double f_size = m_dw_text_fmt->GetFontSize();
		const bool xy = fabs(m_vec.x) >= fabs(m_vec.y);
		const double diff_x = (xy ? m_vec.x : m_vec.y);
		const double diff_y = (xy ? m_vec.y : m_vec.x);
		const double grad_x = diff_x >= 0.0 ? g_len : -g_len;
		const double grad_y = min(f_size, g_len);
		const uint32_t k = static_cast<uint32_t>(floor(diff_x / grad_x));
		const double x0 = (xy ? m_pos.x : m_pos.y);
		const double y0 = static_cast<double>(xy ? m_pos.y : m_pos.x) + diff_y;
		const double y1 = y0 - (diff_y >= 0.0 ? grad_y : -grad_y);
		const double y1_5 = y0 - 0.625 * (diff_y >= 0.0 ? grad_y : -grad_y);
		const double y2 = y1 - (diff_y >= 0.0 ? f_size : -f_size);
		DWRITE_PARAGRAPH_ALIGNMENT p_align;
		if (xy) {
			//	横のほうが大きい場合,
			//	高さが 0 以上の場合下よせ、ない場合上よせを段落のそろえに格納する.
			//	文字列を配置する方形が小さい (書体の大きさと同じ) ため,
			//	DWRITE_PARAGRAPH_ALIGNMENT は, 逆の効果をもたらす.
			p_align = (m_vec.y >= 0.0f ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		}
		else {
			//	縦のほうが小さい場合,
			//	中段を段落のそろえに格納する.
			p_align = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		}
		//	段落のそろえをテキストフォーマットに格納する.
		m_dw_text_fmt->SetParagraphAlignment(p_align);
		br->SetColor(m_stroke_color);
		for (uint32_t i = 0; i <= k; i++) {
			//	方眼の大きさごとに目盛りを表示する.
			const double x = x0 + i * grad_x;
			D2D1_POINT_2F p0{
				xy ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y0),
				xy ? static_cast<FLOAT>(y0) : static_cast<FLOAT>(x)
			};
			const auto y = ((i % 5) == 0 ? y1 : y1_5);
			D2D1_POINT_2F p1{
				xy ? static_cast<FLOAT>(x) : static_cast<FLOAT>(y),
				xy ? static_cast<FLOAT>(y) : static_cast<FLOAT>(x)
			};
			dx.m_d2dContext->DrawLine(p0, p1, br.get());
			//	目盛りの値を表示する.
			const double x1 = x + f_size * 0.5;
			const double x2 = x1 - f_size;
			D2D1_RECT_F rect{
				xy ? static_cast<FLOAT>(x2) : static_cast<FLOAT>(y2),
				xy ? static_cast<FLOAT>(y2) : static_cast<FLOAT>(x2),
				xy ? static_cast<FLOAT>(x1) : static_cast<FLOAT>(y1),
				xy ? static_cast<FLOAT>(y1) : static_cast<FLOAT>(x1)
			};
			dx.m_d2dContext->DrawText(D[i % 10], 1u, m_dw_text_fmt.get(), rect, br.get());
		}
	}

	void ShapeRuler::get_font_family()

	// 値を書体名に格納する.
	void ShapeRuler::set_font_family(wchar_t* const val)
	{
		const auto n_len = m_dw_text_fmt->GetFontFamilyNameLength();
		wchar_t* f_name = new wchar_t[n_len];
		m_dw_text_fmt->GetFontFamilyName(f_name, n_len);
		const auto f_weight = m_dw_text_fmt->GetFontWeight();
		const auto f_style = m_dw_text_fmt->GetFontStyle();
		const auto f_stretch = m_dw_text_fmt->GetFontStretch();
		const auto f_size = m_dw_text_fmt->GetFontSize();
		m_dw_text_fmt = nullptr;
		winrt::check_hresult(
			Shape::s_dwrite_factory->CreateTextFormat(
				attr->m_font_family,
				static_cast<IDWriteFontCollection*>(nullptr),
				attr->m_font_weight,
				attr->m_font_style,
				DWRITE_FONT_STRETCH_NORMAL,
				static_cast<FLOAT>(f_size),
				locale_name,
				m_dw_text_fmt.put()
			)
		);
	}
	// 値を書体の大きさに格納する.
	void set_font_size(const double /*val*/);
	// 値を書体の横幅に格納する.
	void set_font_stretch(const DWRITE_FONT_STRETCH /*val*/);
	// 値を書体の字体に格納する.
	void set_font_style(const DWRITE_FONT_STYLE /*val*/);
	// 値を書体の太さに格納する.
	void set_font_weight(const DWRITE_FONT_WEIGHT /*val*/);
	//	図形を作成する.
	//	pos	位置
	//	vec	ベクトル
	//	attr	属性値
	ShapeRuler::ShapeRuler(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, const ShapePanel* attr) :
		ShapeRect::ShapeRect(pos, vec, attr),
		m_grid_len(attr->m_grid_len)
	{
		wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);
		auto f_size = min(attr->m_font_size, attr->m_grid_len + 1.0);
		winrt::check_hresult(
			Shape::s_dwrite_factory->CreateTextFormat(
				attr->m_font_family,
				static_cast<IDWriteFontCollection*>(nullptr),
				attr->m_font_weight,
				attr->m_font_style,
				DWRITE_FONT_STRETCH_NORMAL,
				static_cast<FLOAT>(f_size),
				locale_name,
				m_dw_text_fmt.put()
			)
		);
		m_dw_text_fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	//	図形をデータリーダーから読み込む.
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
		auto size = min(f_size, m_grid_len + 1.0);
		winrt::check_hresult(
			Shape::s_dwrite_factory->CreateTextFormat(
				f_family,
				static_cast<IDWriteFontCollection*>(nullptr),
				f_weight,
				f_style,
				DWRITE_FONT_STRETCH_NORMAL,
				static_cast<FLOAT>(size),
				locale_name,
				m_dw_text_fmt.put()
			)
		);
		delete[] f_family;
		m_dw_text_fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	// データライターに書き込む.
	void ShapeRuler::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeRect::write(dt_writer);
		dt_writer.WriteDouble(m_grid_len);
		//	書体名
		auto n_size = m_dw_text_fmt->GetFontFamilyNameLength() + 1;
		wchar_t* f_name = new wchar_t[n_size];
		m_dw_text_fmt->GetFontFamilyName(f_name, n_size);
		write(f_name, dt_writer);
		delete[] f_name;
		//	書体の大きさ
		dt_writer.WriteDouble(m_dw_text_fmt->GetFontSize());
		//	字体
		write(m_dw_text_fmt->GetFontStyle(), dt_writer);
		//	書体の太さ
		write(m_dw_text_fmt->GetFontWeight(), dt_writer);
	}

}