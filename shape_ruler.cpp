//------------------------------
// shape_ruler.cpp
// ��K
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �}�`���_���܂ނ����肷��.
	// t_pos	���肳���_
	uint32_t ShapeRuler::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		const uint32_t anc = rect_hit_test_anc(m_start, m_pos, t_pos, m_anc_width);
		if (anc != ANC_TYPE::ANC_PAGE) {
			return anc;
		}
		if (is_opaque(m_stroke_color)) {
			const double g_len = m_grid_base + 1.0;
			const double f_size = m_dwrite_text_format->GetFontSize();
			const bool x_ge_y = fabs(m_pos.x) >= fabs(m_pos.y);
			const double vec_x = (x_ge_y ? m_pos.x : m_pos.y);
			const double vec_y = (x_ge_y ? m_pos.y : m_pos.x);
			const double grad_x = vec_x >= 0.0 ? g_len : -g_len;
			const double grad_y = min(f_size, g_len);
			const uint32_t k = static_cast<uint32_t>(floor(vec_x / grad_x));
			const double x0 = (x_ge_y ? m_start.x : m_start.y);
			const double y0 = static_cast<double>(x_ge_y ? m_start.y : m_start.x) + vec_y;
			const double y1 = y0 - (vec_y >= 0.0 ? grad_y : -grad_y);
			const double y1_5 = y0 - 0.625 * (vec_y >= 0.0 ? grad_y : -grad_y);
			const double y2 = y1 - (vec_y >= 0.0 ? f_size : -f_size);
			for (uint32_t i = 0; i <= k; i++) {
				// ����̑傫�����Ƃɖڐ����\������.
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
						static_cast<FLOAT>(p0.x - m_anc_width * 0.5), min(p0.y, p1.y)
					};
					const D2D1_POINT_2F p_max{
						static_cast<FLOAT>(p0.x + m_anc_width * 0.5f), max(p0.y, p1.y)
					};
					if (pt_in_rect(t_pos, p_min, p_max)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				else {
					const D2D1_POINT_2F p_min{
						min(p0.x, p1.x), static_cast<FLOAT>(p0.y - m_anc_width * 0.5)
					};
					const D2D1_POINT_2F p_max{
						max(p0.x, p1.x), static_cast<FLOAT>(p0.y + m_anc_width * 0.5)
					};
					if (pt_in_rect(t_pos, p_min, p_max)) {
						return ANC_TYPE::ANC_STROKE;
					}
				}
				// �ڐ���̒l��\������.
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
				if (pt_in_rect(t_pos, r_lt, r_rb)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
		}
		if (is_opaque(m_fill_color)) {
			D2D1_POINT_2F e_pos;
			pt_add(m_start, m_pos, e_pos);
			//D2D1_POINT_2F r_lt, r_rb;
			//pt_bound(m_start, e_pos, r_lt, r_rb);
			/*
			if (m_start.x < e_pos.x) {
				r_lt.x = m_start.x;
				r_rb.x = e_pos.x;
			}
			else {
				r_lt.x = e_pos.x;
				r_rb.x = m_start.x;
			}
			if (m_start.y < e_pos.y) {
				r_lt.y = m_start.y;
				r_rb.y = e_pos.y;
			}
			else {
				r_lt.y = e_pos.y;
				r_rb.y = m_start.y;
			}
			*/
			//if (pt_in_rect(t_pos, r_lt, r_rb)) {
			if (pt_in_rect(t_pos, m_start, e_pos)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	void ShapeRuler::create_text_format(void)
	{
		IDWriteFactory* const dwrite_factory = Shape::m_dwrite_factory.get();

		wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);
		//const float font_size = min(m_font_size, m_grid_base + 1.0f);
		const float font_size = m_font_size;
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
		m_dwrite_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	// �}�`��\������.
	void ShapeRuler::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		if (m_d2d_stroke_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			create_stroke_style(factory);
		}
		if (m_dwrite_text_format == nullptr) {
			create_text_format();
		}
		constexpr wchar_t* D[10] = { L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9" };

		const D2D1_RECT_F rect{
			m_start.x,
			m_start.y,
			m_start.x + m_pos.x,
			m_start.y + m_pos.y
		};
		if (is_opaque(m_fill_color)) {
			// �h��Ԃ��F���s�����ȏꍇ,
			// ���`��h��Ԃ�.
			brush->SetColor(m_fill_color);
			target->FillRectangle(&rect, brush);
		}
		if (is_opaque(m_stroke_color)) {
			// ���g�̐F���s�����ȏꍇ,
			const double g_len = m_grid_base + 1.0;	// ����̑傫��
			const double f_size = m_dwrite_text_format->GetFontSize();	// ���̂̑傫��
			const bool w_ge_h = fabs(m_pos.x) >= fabs(m_pos.y);	// ������蕝�̕����傫��
			const double vec_x = (w_ge_h ? m_pos.x : m_pos.y);	// �傫�����̒l�� x
			const double vec_y = (w_ge_h ? m_pos.y : m_pos.x);	// ���������̒l�� y
			const double intvl_x = vec_x >= 0.0 ? g_len : -g_len;	// �ڐ���̊Ԋu
			const double intvl_y = min(f_size, g_len);	// �ڐ���̊Ԋu
			const uint32_t k = static_cast<uint32_t>(floor(vec_x / intvl_x));	// �ڐ���̐�
			const double x0 = (w_ge_h ? m_start.x : m_start.y);
			const double y0 = static_cast<double>(w_ge_h ? m_start.y : m_start.x) + vec_y;
			const double y1 = y0 - (vec_y >= 0.0 ? intvl_y : -intvl_y);
			const double y1_5 = y0 - 0.625 * (vec_y >= 0.0 ? intvl_y : -intvl_y);
			const double y2 = y1 - (vec_y >= 0.0 ? f_size : -f_size);
			/*
			DWRITE_PARAGRAPH_ALIGNMENT p_align;
			if (w_ge_h) {
				// ���̂ق����傫���ꍇ,
				// ������ 0 �ȏ�̏ꍇ���悹�A�Ȃ��ꍇ��悹��i���̂��낦�Ɋi�[����.
				// �������z�u������`�������� (���̂̑傫���Ɠ���) ����,
				// DWRITE_PARAGRAPH_ALIGNMENT ��, �t�̌��ʂ������炷.
				p_align = (m_pos[0].y >= 0.0f ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			}
			else {
				// �c�̂ق����������ꍇ,
				// ���i��i���̂��낦�Ɋi�[����.
				p_align = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			}
			// �i���̂��낦���e�L�X�g�t�H�[�}�b�g�Ɋi�[����.
			m_dwrite_text_format->SetParagraphAlignment(p_align);
			*/
			brush->SetColor(m_stroke_color);
			for (uint32_t i = 0; i <= k; i++) {

				// �ڐ����\������.
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

				// �ڐ��胉�x����\������.
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
		if (m_anc_show && is_selected()) {
			draw_anc();
		}
	}

	// ���̖��𓾂�.
	bool ShapeRuler::get_font_family(wchar_t*& val) const noexcept
	{
		val = m_font_family;
		return true;
	}

	// ���̂̑傫���𓾂�.
	bool ShapeRuler::get_font_size(float& val) const noexcept
	{
		val = m_font_size;
		return true;
	}

	// �l�����̖��Ɋi�[����.
	bool ShapeRuler::set_font_family(wchar_t* const val) noexcept
	{
		// �l�����̖��Ɠ��������肷��.
		if (!equal(m_font_family, val)) {
			m_font_family = val;
			m_dwrite_text_format = nullptr;
			return true;
		}
		return false;
	}

	// �l�����̂̑傫���Ɋi�[����.
	bool ShapeRuler::set_font_size(const float val) noexcept
	{
		if (m_font_size != val) {
			m_font_size = val;
			m_dwrite_text_format = nullptr;
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// start	�n�_
	// pos	�Ίp�_�ւ̈ʒu�x�N�g��
	// page	����
	ShapeRuler::ShapeRuler(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page) :
		ShapeRect::ShapeRect(start, pos, page)
	{
		ShapeText::is_available_font(m_font_family);
		page->get_grid_base(m_grid_base);
		page->get_font_family(m_font_family);
		page->get_font_size(m_font_size);
	}

	static wchar_t* dt_read_name(DataReader const& dt_reader)
	{
		const size_t len = dt_reader.ReadUInt32();	// ������
		uint8_t* data = new uint8_t[2 * (len + 1)];
		dt_reader.ReadBytes(array_view(data, data + 2 * len));
		wchar_t* val = reinterpret_cast<wchar_t*>(data);
		val[len] = L'\0';
		return val;
	}

	// ���ʂ𓾂� (�g�p��� Release ����).
	bool ShapeRuler::get_font_face(IDWriteFontFace3*& face) const noexcept
	{
		return text_get_font_face<IDWriteTextFormat>(
			m_dwrite_text_format.get(), m_font_family, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, face);
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapeRuler::ShapeRuler(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader),
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

	// �}�`���f�[�^���C�^�[�ɏ�������.
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