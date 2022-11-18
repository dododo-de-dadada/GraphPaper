//------------------------------
// shape_ruler.cpp
// ��K
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;

	// �}�`��j������.
	ShapeRuler::~ShapeRuler(void)
	{
		if (m_dw_text_format != nullptr) {
			//m_dw_text_format->Release();
			m_dw_text_format = nullptr;
		}
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	uint32_t ShapeRuler::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		const uint32_t anc = ShapeRect::hit_test_anc(t_pos);
		if (anc != ANC_TYPE::ANC_SHEET) {
			return anc;
		}
		if (is_opaque(m_stroke_color)) {
			const double g_len = m_grid_base + 1.0;
			const double f_size = m_dw_text_format->GetFontSize();
			const bool x_ge_y = fabs(m_vec[0].x) >= fabs(m_vec[0].y);
			const double vec_x = (x_ge_y ? m_vec[0].x : m_vec[0].y);
			const double vec_y = (x_ge_y ? m_vec[0].y : m_vec[0].x);
			const double grad_x = vec_x >= 0.0 ? g_len : -g_len;
			const double grad_y = min(f_size, g_len);
			const uint32_t k = static_cast<uint32_t>(floor(vec_x / grad_x));
			const double x0 = (x_ge_y ? m_pos.x : m_pos.y);
			const double y0 = static_cast<double>(x_ge_y ? m_pos.y : m_pos.x) + vec_y;
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
				// �ڐ���̒l��\������.
				const double x1 = x + f_size * 0.5;
				const double x2 = x1 - f_size;
				D2D1_POINT_2F r_min = {
					x_ge_y ? static_cast<FLOAT>(x2) : static_cast<FLOAT>(y2),
					x_ge_y ? static_cast<FLOAT>(y2) : static_cast<FLOAT>(x2)
				};
				D2D1_POINT_2F r_max = {
					x_ge_y ? static_cast<FLOAT>(x1) : static_cast<FLOAT>(y1),
					x_ge_y ? static_cast<FLOAT>(y1) : static_cast<FLOAT>(x1)
				};
				//pt_bound(r_min, r_max, r_min, r_max);
				/*
				if (r_min.x > r_max.x) {
					const auto less_x = r_max.x;
					r_max.x = r_min.x;
					r_min.x = less_x;
				}
				if (r_min.y > r_max.y) {
					const auto less_y = r_max.y;
					r_max.y = r_min.y;
					r_min.y = less_y;
				}
				*/
				if (pt_in_rect(t_pos, r_min, r_max)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
		}
		if (is_opaque(m_fill_color)) {
			D2D1_POINT_2F e_pos;
			pt_add(m_pos, m_vec[0], e_pos);
			//D2D1_POINT_2F r_min, r_max;
			//pt_bound(m_pos, e_pos, r_min, r_max);
			/*
			if (m_pos.x < e_pos.x) {
				r_min.x = m_pos.x;
				r_max.x = e_pos.x;
			}
			else {
				r_min.x = e_pos.x;
				r_max.x = m_pos.x;
			}
			if (m_pos.y < e_pos.y) {
				r_min.y = m_pos.y;
				r_max.y = e_pos.y;
			}
			else {
				r_min.y = e_pos.y;
				r_max.y = m_pos.y;
			}
			*/
			//if (pt_in_rect(t_pos, r_min, r_max)) {
			if (pt_in_rect(t_pos, m_pos, e_pos)) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_SHEET;
	}

	// �}�`��\������.
	// sh	�\������p��
	void ShapeRuler::draw(ShapeSheet const& sh)
	{
		const D2D_UI& d2d = sh.m_d2d;
		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(d2d);
		}
		if (m_dw_text_format == nullptr) {
			wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
			GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);
			const float font_size = min(m_font_size, m_grid_base + 1.0f);
			winrt::check_hresult(
				d2d.m_dwrite_factory->CreateTextFormat(
					m_font_family,
					static_cast<IDWriteFontCollection*>(nullptr),
					DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL,
					DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL,
					DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL,
					font_size,
					locale_name,
					m_dw_text_format.put()
				)
			);
			m_dw_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
		}
		constexpr wchar_t* D[10] = { L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9" };

		const D2D1_RECT_F rect{
			m_pos.x,
			m_pos.y,
			m_pos.x + m_vec[0].x,
			m_pos.y + m_vec[0].y
		};
		if (is_opaque(m_fill_color)) {
			// �h��Ԃ��F���s�����ȏꍇ,
			// ���`��h��Ԃ�.
			sh.m_color_brush->SetColor(m_fill_color);
			d2d.m_d2d_context->FillRectangle(&rect, sh.m_color_brush.get());
		}
		if (is_opaque(m_stroke_color)) {
			// ���g�̐F���s�����ȏꍇ,
			const double g_len = m_grid_base + 1.0;
			const double f_size = m_dw_text_format->GetFontSize();
			const bool xy = fabs(m_vec[0].x) >= fabs(m_vec[0].y);
			const double vec_x = (xy ? m_vec[0].x : m_vec[0].y);
			const double vec_y = (xy ? m_vec[0].y : m_vec[0].x);
			const double grad_x = vec_x >= 0.0 ? g_len : -g_len;
			const double grad_y = min(f_size, g_len);
			const uint32_t k = static_cast<uint32_t>(floor(vec_x / grad_x));
			const double x0 = (xy ? m_pos.x : m_pos.y);
			const double y0 = static_cast<double>(xy ? m_pos.y : m_pos.x) + vec_y;
			const double y1 = y0 - (vec_y >= 0.0 ? grad_y : -grad_y);
			const double y1_5 = y0 - 0.625 * (vec_y >= 0.0 ? grad_y : -grad_y);
			const double y2 = y1 - (vec_y >= 0.0 ? f_size : -f_size);
			DWRITE_PARAGRAPH_ALIGNMENT p_align;
			if (xy) {
				// ���̂ق����傫���ꍇ,
				// ������ 0 �ȏ�̏ꍇ���悹�A�Ȃ��ꍇ��悹��i���̂��낦�Ɋi�[����.
				// �������z�u������`�������� (���̂̑傫���Ɠ���) ����,
				// DWRITE_PARAGRAPH_ALIGNMENT ��, �t�̌��ʂ������炷.
				p_align = (m_vec[0].y >= 0.0f ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			}
			else {
				// �c�̂ق����������ꍇ,
				// ���i��i���̂��낦�Ɋi�[����.
				p_align = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			}
			// �i���̂��낦���e�L�X�g�t�H�[�}�b�g�Ɋi�[����.
			m_dw_text_format->SetParagraphAlignment(p_align);
			sh.m_color_brush->SetColor(m_stroke_color);
			for (uint32_t i = 0; i <= k; i++) {
				// ����̑傫�����Ƃɖڐ����\������.
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
				d2d.m_d2d_context->DrawLine(p0, p1, sh.m_color_brush.get());
				// �ڐ���̒l��\������.
				const double x1 = x + f_size * 0.5;
				const double x2 = x1 - f_size;
				D2D1_RECT_F t_rect{
					xy ? static_cast<FLOAT>(x2) : static_cast<FLOAT>(y2),
					xy ? static_cast<FLOAT>(y2) : static_cast<FLOAT>(x2),
					xy ? static_cast<FLOAT>(x1) : static_cast<FLOAT>(y1),
					xy ? static_cast<FLOAT>(y1) : static_cast<FLOAT>(x1)
				};
				d2d.m_d2d_context->DrawText(D[i % 10], 1u, m_dw_text_format.get(), t_rect, sh.m_color_brush.get());
			}
		}
		if (is_selected()) {
			// �I���t���O�������Ă���ꍇ,
			// �I������Ă���Ȃ����ʂ�\������.
			D2D1_POINT_2F r_pos[4];	// ���`�̒��_
			r_pos[0] = m_pos;
			r_pos[1].y = rect.top;
			r_pos[1].x = rect.right;
			r_pos[2].x = rect.right;
			r_pos[2].y = rect.bottom;
			r_pos[3].y = rect.bottom;
			r_pos[3].x = rect.left;
			for (uint32_t i = 0, j = 3; i < 4; j = i++) {
				anc_draw_rect(r_pos[i], sh);
				D2D1_POINT_2F r_mid;	// ���`�̕ӂ̒��_
				pt_avg(r_pos[j], r_pos[i], r_mid);
				anc_draw_rect(r_mid, sh);
			}
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
			m_dw_text_format = nullptr;
			return true;
		}
		return false;
	}

	// �l�����̂̑傫���Ɋi�[����.
	bool ShapeRuler::set_font_size(const float val) noexcept
	{
		if (m_font_size != val) {
			m_font_size = val;
			m_dw_text_format = nullptr;
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// b_pos	�͂ޗ̈�̎n�_
	// b_vec	�͂ޗ̈�̏I�_�ւ̍���
	// s_attr	����
	ShapeRuler::ShapeRuler(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr) :
		ShapeRect::ShapeRect(b_pos, b_vec, s_attr),
		m_grid_base(s_attr->m_grid_base),
		m_font_family(s_attr->m_font_family),
		m_font_size(min(s_attr->m_font_size, s_attr->m_grid_base + 1.0f))
	{}

	static wchar_t* dt_read_name(DataReader const& dt_reader)
	{
		wchar_t* name = nullptr;
		dt_read(name, dt_reader);
		return name;
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapeRuler::ShapeRuler(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader),
		m_grid_base(dt_reader.ReadSingle()),
		m_font_family(dt_read_name(dt_reader)),
		m_font_size(dt_reader.ReadSingle())
	{
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapeRuler::write(DataWriter const& dt_writer) const
	{
		ShapeRect::write(dt_writer);
		dt_writer.WriteSingle(m_grid_base);
		dt_write(m_font_family, dt_writer);
		dt_writer.WriteSingle(m_dw_text_format->GetFontSize());
	}

}