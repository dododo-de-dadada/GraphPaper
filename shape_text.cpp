//------------------------------
// shape_text.cpp
// ������}�`
//------------------------------
#include <cwctype> 
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;

	wchar_t** ShapeText::s_available_fonts = nullptr;	//�L���ȏ��̖�
	D2D1_COLOR_F ShapeText::s_text_selected_background{ ACCENT_COLOR };	// �����͈͂̔w�i�F
	D2D1_COLOR_F ShapeText::s_text_selected_foreground{ COLOR_TEXT_SELECTED };	// �����͈͂̕����F

	// �q�b�g�e�X�g�̌v�ʂ𓾂�.
	static void tx_create_test_metrics(IDWriteTextLayout* text_lay, const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt);
	// �q�b�g�e�X�g�̌v��, �s�̌v��, ������I���̌v�ʂ𓾂�.
	static void tx_create_text_metrics(IDWriteTextLayout* text_lay, const uint32_t text_len, UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_met, DWRITE_LINE_METRICS*& line_met, UINT32& sele_cnt, DWRITE_HIT_TEST_METRICS*& sele_met, const DWRITE_TEXT_RANGE& sele_rng);
	// ��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	static void tx_dt_write_svg(const wchar_t* t, const uint32_t t_len, const double x, const double y, const double dy, DataWriter const& dt_writer);
	// ���̂̌v�ʂ𓾂�
	static void tx_get_font_metrics(IDWriteTextLayout* text_lay, DWRITE_FONT_METRICS* font_met);
	// �q�b�g�e�X�g�̌v��, �s�̌v��, ������I���̌v�ʂ�j������.
	static void tx_relese_metrics(UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_metrics, DWRITE_LINE_METRICS*& line_metrics, UINT32& sele_cnt, DWRITE_HIT_TEST_METRICS*& sele_metrics) noexcept;

	// �q�b�g�e�X�g�̌v�ʂ𓾂�.
	// text_lay	�����񃌃C�A�E�g
	// text_rng	�����͈�
	// test_met	�q�b�g�e�X�g�̌v��
	// test_cnt	�v�ʂ̗v�f��
	static void tx_create_test_metrics(IDWriteTextLayout* text_lay, const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt)
	{
		const uint32_t pos = text_rng.startPosition;
		const uint32_t len = text_rng.length;
		DWRITE_HIT_TEST_METRICS test[1];

		// �܂��v�ʂ̗v�f���𓾂�.
		// �ŏ��� HitTestTextRange �֐��Ăяo����, ���s���邱�Ƃ��O��Ȃ̂�, check_hresult ���Ȃ�.
		// �z����m�ۂ���, ���炽�߂Ċ֐����Ăяo��, �v�ʂ𓾂�.
		text_lay->HitTestTextRange(pos, len, 0, 0, test, 1, &test_cnt);
		test_met = new DWRITE_HIT_TEST_METRICS[test_cnt];
		winrt::check_hresult(text_lay->HitTestTextRange(pos, len, 0, 0, test_met, test_cnt, &test_cnt));
	}

	// ���̂̌v�ʂ𓾂�
	static void tx_get_font_metrics(IDWriteTextLayout* text_lay, DWRITE_FONT_METRICS* font_met)
	{
		// �����񃌃C�A�E�g ---> ���̃��X�g ---> ���̃t�@�~���[ ---> ���̂𓾂�.
		// �����񃌃C�A�E�g�ɂЂƂ̏��̃t�@�~���[��, ���̃t�@�~���[�ɂЂƂ̏��̂��ݒ肳��Ă��邱�Ƃ��O��.
		winrt::com_ptr<IDWriteFontCollection> fonts;
		text_lay->GetFontCollection(fonts.put());
		winrt::com_ptr<IDWriteFontFamily> fam;
		fonts->GetFontFamily(0, fam.put());
		fonts = nullptr;
		winrt::com_ptr<IDWriteFont> font;
		fam->GetFont(0, font.put());
		fam = nullptr;
		// ���̂̌v�ʂ𓾂�.
		font->GetMetrics(font_met);
		font = nullptr;
	}

	// �q�b�g�e�X�g�̌v��, �s�̌v��, �I�����ꂽ�����͈͂̌v�ʂ�j������.
	// test_cnt	�q�b�g�e�X�g�ƍs�̌v�ʂ̊e�v�f��
	// test_met	�q�b�g�e�X�g�̌v��
	// line_met �s�̌v��
	// sele_cnt	�I�����ꂽ�����͈͂̌v�ʂ̗v�f��
	// sele_met �I�����ꂽ�����͈͂̌v��
	static void tx_relese_metrics(UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_met, DWRITE_LINE_METRICS*& line_met, UINT32& sele_cnt, DWRITE_HIT_TEST_METRICS*& sele_met) noexcept
	{
		test_cnt = 0;
		if (test_met != nullptr) {
			delete[] test_met;
			test_met = nullptr;
		}
		if (line_met != nullptr) {
			delete[] line_met;
			line_met = nullptr;
		}
		sele_cnt = 0;
		if (sele_met != nullptr) {
			delete[] sele_met;
			sele_met = nullptr;
		}
	}

	// �q�b�g�e�X�g�̌v��, �s�̌v��, ������I���̌v�ʂ𓾂�.
	// text_lay	�����񃌃C�A�E�g
	// text_len	������̒���
	// test_cnt	�q�b�g�e�X�g�̌v�ʂ̗v�f��
	// test_met	�q�b�g�e�X�g�̌v��
	// line_met �s�̌v��
	// sele_cnt	�I�����ꂽ�����͈͂̌v�ʂ̗v�f��
	// sele_met �I�����ꂽ�����͈͂̌v��
	// sele_rng	�I�����ꂽ�����͈�
	static void tx_create_text_metrics(
		IDWriteTextLayout* text_lay, const uint32_t text_len,
		UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_met, DWRITE_LINE_METRICS*& line_met,
		UINT32& sele_cnt, DWRITE_HIT_TEST_METRICS*& sele_met,
		const DWRITE_TEXT_RANGE& sele_rng)
	{
		tx_relese_metrics(test_cnt, test_met, line_met, sele_cnt, sele_met);
		if (text_lay != nullptr) {
			// �q�b�g�e�X�g�̌v��
			tx_create_test_metrics(text_lay, { 0, text_len }, test_met, test_cnt);

			// �s�̌v��
			UINT32 line_cnt;
			text_lay->GetLineMetrics(nullptr, 0, &line_cnt);
			line_met = new DWRITE_LINE_METRICS[line_cnt];
			text_lay->GetLineMetrics(line_met, line_cnt, &line_cnt);

			// �I�����ꂽ�����͈͂̌v��
			if (sele_rng.length > 0) {
				tx_create_test_metrics(text_lay, sele_rng, sele_met, sele_cnt);
			}
		}
	}

	// ��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	// t	������
	// t_len	������
	// x, y	�ʒu
	// dy	�����Ȃ��炵��
	// dt_writer	�f�[�^���C�^�[
	static void tx_dt_write_svg(const wchar_t* t, const uint32_t t_len, const double x, const double y, const double dy, DataWriter const& dt_writer)
	{
		dt_write_svg("<text ", dt_writer);
		dt_write_svg(x, "x", dt_writer);
		dt_write_svg(y, "y", dt_writer);
		dt_write_svg(dy, "dy", dt_writer);
		//dt_write_svg("text-before-edge", "alignment-baseline", dt_writer);
		dt_write_svg(">", dt_writer);
		uint32_t k = 0;
		for (uint32_t i = k; i < t_len; i++) {
			const wchar_t c = t[i];
			char* ent;
			if (c == L'<') {
				ent = "&lt;";
			}
			else if (c == L'>') {
				ent = "&gt;";
			}
			else if (c == L'&') {
				ent = "&amp;";
			}
			//else if (c == L'"') {
			// ent = "&quot;";
			//}
			//else if (c == L'\'') {
			// ent = "&apos;";
			//}
			else {
				continue;
			}
			if (i > k) {
				dt_write_svg(t + k, i - k, dt_writer);
			}
			dt_write_svg(ent, dt_writer);
			k = i + 1;
		}
		if (t_len > k) {
			dt_write_svg(t + k, t_len - k, dt_writer);
		}
		dt_write_svg("</text>" SVG_NEW_LINE, dt_writer);
	}

	// �}�`��j������.
	ShapeText::~ShapeText(void)
	{
		if (m_dw_text_layout != nullptr) {
			//m_dw_text_layout->Release();
			m_dw_text_layout = nullptr;
		}
		tx_relese_metrics(m_dw_test_cnt, m_dw_test_metrics, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics);

		// ���̖���j������.
		if (m_font_family != nullptr) {
			// �����̃I�u�W�F�N�g����Q�Ƃ���Ă��Ȃ��ꍇ�̂�, ���̖���j������.
			if (!is_available_font(m_font_family)) {
				delete[] m_font_family;
			}
			m_font_family = nullptr;
		}
		// �������j������.
		if (m_text != nullptr) {
			delete[] m_text;
			m_text = nullptr;
		}
	}

	// �g�̑傫���𕶎���ɍ��킹��.
	// g_len	����̑傫�� (1 �ȏ�Ȃ�Ε���̑傫���ɍ��킹��)
	// �߂�l	�傫�����������ꂽ�Ȃ�ΐ^.
	bool ShapeText::adjust_bbox(const float g_len) noexcept
	{
		// ������̑傫�����v�Z��, �g�Ɋi�[����.
		D2D1_POINT_2F t_box{ 0.0f, 0.0f };	// �g
		for (size_t i = 0; i < m_dw_test_cnt; i++) {
			t_box.x = fmax(t_box.x, m_dw_test_metrics[i].width);
			t_box.y += m_dw_test_metrics[i].height;
		}
		// �g�ɍ��E�̃p�f�B���O��������.
		const float sp = m_text_padding.width * 2.0f;	// ���E�̃p�f�B���O
		pt_add(t_box, sp, sp, t_box);
		if (g_len >= 1.0f) {
			// �g�����̑傫���ɐ؂�グ��.
			t_box.x = floor((t_box.x + g_len - 1.0f) / g_len) * g_len;
			t_box.y = floor((t_box.y + g_len - 1.0f) / g_len) * g_len;
		}
		// �}�`�̑傫����ύX����.
		if (!equal(t_box, m_vec[0])) {
			D2D1_POINT_2F se;
			pt_add(m_pos, t_box, se);
			set_pos_anc(se, ANC_TYPE::ANC_SE, 0.0f, false);
			return true;
		}
		return false;
	}

	// �}�`��\������.
	// sh	�\������p��
	void ShapeText::draw(ShapeSheet const& sh)
	{
		const D2D_UI& d2d = sh.m_d2d;
		// ���`��`��.
		ShapeRect::draw(sh);

		// �����񂪋󂩔��肷��.
		if (m_text == nullptr || m_text[0] == L'\0') {
			// �ʒu�̌v��, �s�̌v��, ������I���̌v�ʂ�j������.
			tx_relese_metrics(m_dw_test_cnt, m_dw_test_metrics, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics);
			// �����񃌃C�A�E�g����łȂ��Ȃ�j������.
			if (m_dw_text_layout != nullptr) {
				//m_dw_text_layout->Release();
				m_dw_text_layout = nullptr;
			}
		}
		else {
			// �V�K�쐬
			// �����񃌃C�A�E�g���󂩔��肷��.
			if (m_dw_text_layout == nullptr) {
				// ����̃��P�[�����𓾂�.
				wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
				GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);

				// ������t�H�[�}�b�g���쐬����.
				// CreateTextFormat �� DWRITE_FONT_STRETCH_UNDEFINED ���w�肳�ꂽ�ꍇ�G���[�ɂȂ�.
				// �����l���Ȃ�ł���, DWRITE_FONT_STRETCH_NORMAL �Ńe�L�X�g�t�H�[�}�b�g�͍쐬����.
				winrt::com_ptr<IDWriteTextFormat> t_format;
				winrt::check_hresult(
					d2d.m_dwrite_factory->CreateTextFormat(
						m_font_family, static_cast<IDWriteFontCollection*>(nullptr),
						m_font_weight, m_font_style, DWRITE_FONT_STRETCH_NORMAL,
						m_font_size, locale_name, t_format.put())
				);

				// ������t�H�[�}�b�g���當���񃌃C�A�E�g���쐬����.
				const double text_w = std::fabs(m_vec[0].x) - 2.0 * m_text_padding.width;
				const double text_h = std::fabs(m_vec[0].y) - 2.0 * m_text_padding.height;
				const UINT32 text_len = wchar_len(m_text);
				winrt::check_hresult(d2d.m_dwrite_factory->CreateTextLayout(m_text, text_len, t_format.get(), static_cast<FLOAT>(max(text_w, 0.0)), static_cast<FLOAT>(max(text_h, 0.0)), m_dw_text_layout.put()));

				// ������t�H�[�}�b�g��j������.
				t_format = nullptr;

				// �����̐L�k, �����̂��낦, �i���̂��낦�𕶎��񃌃C�A�E�g�Ɋi�[����.
				winrt::check_hresult(m_dw_text_layout->SetFontStretch(m_font_stretch, DWRITE_TEXT_RANGE{ 0, text_len }));
				winrt::check_hresult(m_dw_text_layout->SetTextAlignment(m_text_align_t));
				winrt::check_hresult(m_dw_text_layout->SetParagraphAlignment(m_text_align_p));

				// �s�Ԃ𕶎��񃌃C�A�E�g�Ɋi�[����.
				winrt::com_ptr<IDWriteTextLayout3> t3;
				if (m_dw_text_layout.try_as(t3)) {
					DWRITE_LINE_SPACING new_sp;
					new_sp.leadingBefore = 0.0f;
					new_sp.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
					// �s�Ԃ��[�����傫���Ȃ�, �s�Ԃ�ݒ肷��.
					if (m_text_line_sp >= FLT_MIN) {
						if (m_dw_font_metrics.designUnitsPerEm == 0) {
							tx_get_font_metrics(t3.get(), &m_dw_font_metrics);
						}
						const float descent = (m_dw_font_metrics.designUnitsPerEm == 0 ? 0.0f : m_font_size * m_dw_font_metrics.descent / m_dw_font_metrics.designUnitsPerEm);

						new_sp.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
						new_sp.height = m_font_size + m_text_line_sp;
						new_sp.baseline = m_font_size + m_text_line_sp - descent;
					}
					// ����̍s�Ԃ�ݒ肷��.
					else {
						new_sp.method = DWRITE_LINE_SPACING_METHOD_DEFAULT;
						new_sp.height = 0.0f;
						new_sp.baseline = 0.0f;
					}
					winrt::check_hresult(t3->SetLineSpacing(&new_sp));
					t3 = nullptr;
				}
				tx_create_text_metrics(m_dw_text_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_text_selected_range);
			}

			// �ύX.
			else {

				const uint32_t text_len = wchar_len(m_text);
				bool updated = false;

				// ���̖����ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				WCHAR font_family[256];
				winrt::check_hresult(m_dw_text_layout->GetFontFamilyName(0, font_family, 256));
				if (wcscmp(font_family, m_font_family) != 0) {
					winrt::check_hresult(m_dw_text_layout->SetFontFamilyName(m_font_family, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// ���̂̑傫�����ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				FLOAT font_size;
				winrt::check_hresult(m_dw_text_layout->GetFontSize(0, &font_size));
				if (!equal(font_size, m_font_size)) {
					winrt::check_hresult(m_dw_text_layout->SetFontSize(m_font_size, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// ���̂̐L�k���ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				DWRITE_FONT_STRETCH font_stretch;
				winrt::check_hresult(m_dw_text_layout->GetFontStretch(0, &font_stretch));
				if (!equal(font_stretch, m_font_stretch)) {
					winrt::check_hresult(m_dw_text_layout->SetFontStretch(m_font_stretch, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// ���̂̎��̂��ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				DWRITE_FONT_STYLE font_style;
				winrt::check_hresult(m_dw_text_layout->GetFontStyle(0, &font_style));
				if (!equal(font_style, m_font_style)) {
					winrt::check_hresult(m_dw_text_layout->SetFontStyle(m_font_style, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// ���̂̑������ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				DWRITE_FONT_WEIGHT font_weight;
				winrt::check_hresult(m_dw_text_layout->GetFontWeight(0, &font_weight));
				if (!equal(font_weight, m_font_weight)) {
					winrt::check_hresult(m_dw_text_layout->SetFontWeight(m_font_weight, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// ������̃p�f�B���O���ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				const FLOAT text_w = static_cast<FLOAT>(max(std::fabs(m_vec[0].x) - m_text_padding.width * 2.0, 0.0));
				const FLOAT text_h = static_cast<FLOAT>(max(std::fabs(m_vec[0].y) - m_text_padding.height * 2.0, 0.0));
				if (!equal(text_w, m_dw_text_layout->GetMaxWidth())) {
					winrt::check_hresult(m_dw_text_layout->SetMaxWidth(text_w));
					if (!updated) {
						updated = true;
					}
				}
				if (!equal(text_h, m_dw_text_layout->GetMaxHeight())) {
					winrt::check_hresult(m_dw_text_layout->SetMaxHeight(text_h));
					if (!updated) {
						updated = true;
					}
				}

				// �i���̂��낦���ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				DWRITE_PARAGRAPH_ALIGNMENT para_align = m_dw_text_layout->GetParagraphAlignment();
				if (!equal(para_align, m_text_align_p)) {
					winrt::check_hresult(m_dw_text_layout->SetParagraphAlignment(m_text_align_p));
					if (!updated) {
						updated = true;
					}
				}

				// �����̂��낦���ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				DWRITE_TEXT_ALIGNMENT text_align = m_dw_text_layout->GetTextAlignment();
				if (!equal(text_align, m_text_align_t)) {
					winrt::check_hresult(m_dw_text_layout->SetTextAlignment(m_text_align_t));
					if (!updated) {
						updated = true;
					}
				}

				// �s�Ԃ��ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				winrt::com_ptr<IDWriteTextLayout3> t3;
				if (m_dw_text_layout.try_as(t3)) {
					DWRITE_LINE_SPACING old_sp;
					winrt::check_hresult(t3->GetLineSpacing(&old_sp));

					DWRITE_LINE_SPACING new_sp;
					new_sp.leadingBefore = 0.0f;
					new_sp.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
					// �s�Ԃ��[�����傫���Ȃ�, �s�Ԃ�ݒ肷��.
					if (m_text_line_sp >= FLT_MIN) {
						if (m_dw_font_metrics.designUnitsPerEm == 0) {
							tx_get_font_metrics(t3.get(), &m_dw_font_metrics);
						}
						const float descent = (m_dw_font_metrics.designUnitsPerEm == 0 ? 0.0f : m_font_size * m_dw_font_metrics.descent / m_dw_font_metrics.designUnitsPerEm);

						new_sp.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
						new_sp.height = m_font_size + m_text_line_sp;
						new_sp.baseline = m_font_size + m_text_line_sp - descent;
					}
					// ����̍s�Ԃ�ݒ肷��.
					else {
						new_sp.method = DWRITE_LINE_SPACING_METHOD_DEFAULT;
						new_sp.height = 0.0f;
						new_sp.baseline = 0.0f;
					}
					if (memcmp(&old_sp, &new_sp, sizeof(old_sp)) != 0) {
						winrt::check_hresult(t3->SetLineSpacing(&new_sp));
						if (!updated) {
							updated = true;
						}
					}
					t3 = nullptr;
				}

				// �v�ʂ̗v�f���� 0 �Ȃ̂ɑI�����ꂽ�����͈͂� 0 �łȂ�,
				// �܂���, �I�����ꂽ�����͈͂�, ����܂ł̑I�����ꂽ�����͈͂̌v�ʂƈقȂ�.
				if (m_dw_selected_cnt == 0) {
					if (m_text_selected_range.length > 0) {
						if (!updated) {
							updated = true;
						}
					}
				}
				else {
					const uint32_t start_pos = m_dw_selected_metrics[0].textPosition;
					uint32_t length = 0;
					for (uint32_t i = 0; i < m_dw_selected_cnt; i++) {
						length += m_dw_selected_metrics[i].length;
					}
					if (start_pos != m_text_selected_range.startPosition || length != m_text_selected_range.length) {
						if (!updated) {
							updated = true;
						}
					}
				}

				// �ύX���ꂽ�����肷��.
				if (updated) {
					// �ʒu�̌v��, �s�̌v��, ������I���̌v�ʂ��č쐬����.
					tx_relese_metrics(m_dw_test_cnt, m_dw_test_metrics, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics);
					tx_create_text_metrics(m_dw_text_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_text_selected_range);
				}
			}

			// �]���������킦��, ������̍���ʒu���v�Z����.
			D2D1_POINT_2F t_min;
			pt_add(m_pos, m_vec[0], t_min);
			//pt_min(m_pos, t_min, t_min);
			t_min.x = m_pos.x < t_min.x ? m_pos.x : t_min.x;
			t_min.y = m_pos.y < t_min.y ? m_pos.y : t_min.y;

			const FLOAT pw = m_text_padding.width;
			const FLOAT ph = m_text_padding.height;
			const double hm = min(pw, fabs(m_vec[0].x) * 0.5);
			const double vm = min(ph, fabs(m_vec[0].y) * 0.5);
			pt_add(t_min, hm, vm, t_min);

			// �I�����ꂽ�����͈͂�����Ȃ�, �w�i��h��Ԃ�.
			if (m_text_selected_range.length > 0) {
				if (m_dw_font_metrics.designUnitsPerEm == 0) {
					tx_get_font_metrics(m_dw_text_layout.get(), &m_dw_font_metrics);
				}
				const float descent = (m_dw_font_metrics.designUnitsPerEm == 0 ? 0.0f : m_font_size * m_dw_font_metrics.descent / m_dw_font_metrics.designUnitsPerEm);
				const uint32_t rc = m_dw_selected_cnt;
				const uint32_t tc = m_dw_test_cnt;
				for (uint32_t i = 0; i < rc; i++) {
					const DWRITE_HIT_TEST_METRICS& rm = m_dw_selected_metrics[i];
					for (uint32_t j = 0; j < tc; j++) {
						const DWRITE_HIT_TEST_METRICS& tm = m_dw_test_metrics[j];
						const DWRITE_LINE_METRICS& lm = m_dw_line_metrics[j];
						if (tm.textPosition <= rm.textPosition && rm.textPosition + rm.length <= tm.textPosition + tm.length) {
							D2D1_RECT_F rect;
							rect.left = t_min.x + rm.left;
							rect.top = static_cast<FLOAT>(t_min.y + tm.top + lm.baseline + descent - m_font_size);
							if (rm.width < FLT_MIN) {
								const float sp_len = max(lm.trailingWhitespaceLength * m_font_size * 0.25f, 1.0f);
								rect.right = rect.left + sp_len;
							}
							else {
								rect.right = rect.left + rm.width;
							}
							rect.bottom = rect.top + m_font_size;
							sh.m_color_brush->SetColor(ShapeText::s_text_selected_foreground);
							d2d.m_d2d_context->DrawRectangle(rect, sh.m_color_brush.get(), 2.0, nullptr);
							sh.m_color_brush->SetColor(ShapeText::s_text_selected_background);
							d2d.m_d2d_context->FillRectangle(rect, sh.m_color_brush.get());
							break;
						}
					}
				}
				sh.m_range_brush->SetColor(ShapeText::s_text_selected_foreground);
				winrt::check_hresult(m_dw_text_layout->SetDrawingEffect(sh.m_range_brush.get(), m_text_selected_range));
			}

			// �������\������
			sh.m_color_brush->SetColor(m_font_color);
			d2d.m_d2d_context->DrawTextLayout(t_min, m_dw_text_layout.get(), sh.m_color_brush.get());
			if (m_text_selected_range.length > 0) {
				winrt::check_hresult(m_dw_text_layout->SetDrawingEffect(nullptr, { 0, wchar_len(m_text) }));
			}

			// �}�`���I������Ă���Ȃ�, ������̕⏕����\������
			if (is_selected()) {
				const uint32_t d_cnt = Shape::m_aux_style->GetDashesCount();
				if (d_cnt <= 0 || d_cnt > 6) {
					return;
				}

				D2D1_MATRIX_3X2_F tran;
				d2d.m_d2d_context->GetTransform(&tran);
				FLOAT s_width = static_cast<FLOAT>(1.0 / tran.m11);
				D2D1_STROKE_STYLE_PROPERTIES1 s_prop{ AUXILIARY_SEG_STYLE };

				FLOAT d_arr[6];
				Shape::m_aux_style->GetDashes(d_arr, d_cnt);
				double mod = d_arr[0];
				for (uint32_t i = 1; i < d_cnt; i++) {
					mod += d_arr[i];
				}
				if (m_dw_font_metrics.designUnitsPerEm == 0) {
					tx_get_font_metrics(m_dw_text_layout.get(), &m_dw_font_metrics);
				}
				const float descent = (m_dw_font_metrics.designUnitsPerEm == 0 ? 0.0f : m_font_size * m_dw_font_metrics.descent / m_dw_font_metrics.designUnitsPerEm);
				for (uint32_t i = 0; i < m_dw_test_cnt; i++) {
					DWRITE_HIT_TEST_METRICS const& tm = m_dw_test_metrics[i];
					DWRITE_LINE_METRICS const& lm = m_dw_line_metrics[i];
					// �j��������ďd�Ȃ��ĕ\������Ȃ��悤��, �j���̃I�t�Z�b�g���v�Z��,
					// ������̘g��ӂ��Ƃɕ\������.
					D2D1_POINT_2F p[4];
					p[0].x = t_min.x + tm.left;
					p[0].y = static_cast<FLOAT>(t_min.y + tm.top + lm.baseline + descent - m_font_size);
					p[2].x = p[0].x + tm.width;
					p[2].y = p[0].y + m_font_size;
					p[1].x = p[2].x;
					p[1].y = p[0].y;
					p[3].x = p[0].x;
					p[3].y = p[2].y;
					sh.m_color_brush->SetColor(ShapeText::s_text_selected_foreground);
					d2d.m_d2d_context->DrawRectangle({ p[0].x, p[0].y, p[2].x, p[2].y }, sh.m_color_brush.get(), s_width, nullptr);
					sh.m_color_brush->SetColor(ShapeText::s_text_selected_background);
					s_prop.dashOffset = static_cast<FLOAT>(std::fmod(p[0].x, mod));
					winrt::com_ptr<ID2D1StrokeStyle1> s_style;
					d2d.m_d2d_factory->CreateStrokeStyle(&s_prop, d_arr, d_cnt, s_style.put());
					d2d.m_d2d_context->DrawLine(p[0], p[1], sh.m_color_brush.get(), s_width, s_style.get());
					d2d.m_d2d_context->DrawLine(p[3], p[2], sh.m_color_brush.get(), s_width, s_style.get());
					s_style = nullptr;
					s_prop.dashOffset = static_cast<FLOAT>(std::fmod(p[0].y, mod));
					d2d.m_d2d_factory->CreateStrokeStyle(&s_prop, d_arr, d_cnt, s_style.put());
					d2d.m_d2d_context->DrawLine(p[1], p[2], sh.m_color_brush.get(), s_width, s_style.get());
					d2d.m_d2d_context->DrawLine(p[0], p[3], sh.m_color_brush.get(), s_width, s_style.get());
					s_style = nullptr;
				}
			}
		}
	}

	// ���̂̐F�𓾂�.
	bool ShapeText::get_font_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_font_color;
		return true;
	}

	// ���̖��𓾂�.
	bool ShapeText::get_font_family(wchar_t*& val) const noexcept
	{
		val = m_font_family;
		return true;
	}

	// ���̂̑傫���𓾂�.
	bool ShapeText::get_font_size(float& val) const noexcept
	{
		val = m_font_size;
		return true;
	}

	// ���̂̐L�k�𓾂�.
	bool ShapeText::get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept
	{
		val = m_font_stretch;
		return true;
	}

	// ���̂̎��̂𓾂�.
	bool ShapeText::get_font_style(DWRITE_FONT_STYLE& val) const noexcept
	{
		val = m_font_style;
		return true;
	}

	// ���̂̑����𓾂�.
	bool ShapeText::get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept
	{
		val = m_font_weight;
		return true;
	}

	// �i���̂��낦�𓾂�.
	bool ShapeText::get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_p;
		return true;
	}

	// ������̂��낦�𓾂�.
	bool ShapeText::get_text_align_t(DWRITE_TEXT_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_t;
		return true;
	}

	// ������𓾂�.
	bool ShapeText::get_text_content(wchar_t*& val) const noexcept
	{
		val = m_text;
		return true;
	}

	// �s�Ԃ𓾂�.
	bool ShapeText::get_text_line_sp(float& val) const noexcept
	{
		val = m_text_line_sp;
		return true;
	}

	// ������̗]���𓾂�.
	bool ShapeText::get_text_padding(D2D1_SIZE_F& val) const noexcept
	{
		val = m_text_padding;
		return true;
	}

	// �����͈͂𓾂�
	bool ShapeText::get_text_selected(DWRITE_TEXT_RANGE& val) const noexcept
	{
		val = m_text_selected_range;
		return true;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeText::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		const uint32_t anc = ShapeRect::hit_test_anc(t_pos);
		if (anc != ANC_TYPE::ANC_SHEET) {
			return anc;
		}
		const float descent = m_dw_font_metrics.designUnitsPerEm == 0 ? 0.0f : (m_font_size * m_dw_font_metrics.descent / m_dw_font_metrics.designUnitsPerEm);
		// ������͈̔͂̍��オ���_�ɂȂ�悤, ���肷��ʒu���ړ�����.
		D2D1_POINT_2F p_min;
		ShapeStroke::get_pos_min(p_min);
		pt_sub(t_pos, p_min, p_min);
		pt_sub(p_min, m_text_padding, p_min);
		for (uint32_t i = 0; i < m_dw_test_cnt; i++) {
			DWRITE_HIT_TEST_METRICS const& tm = m_dw_test_metrics[i];
			DWRITE_LINE_METRICS const& lm = m_dw_line_metrics[i];
			const D2D1_POINT_2F r_max{ tm.left + tm.width, tm.top + lm.baseline + descent };
			const D2D1_POINT_2F r_min{ tm.left, r_max.y - m_font_size };
			if (pt_in_rect(p_min, r_min, r_max)) {
				return ANC_TYPE::ANC_TEXT;
			}
		}
		return ShapeRect::hit_test(t_pos);
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// area_min	�͈͂̍���ʒu
	// area_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeText::in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept
	{
		D2D1_POINT_2F p_min;
		D2D1_POINT_2F h_min;
		D2D1_POINT_2F h_max;

		if (m_dw_test_cnt > 0 && m_dw_test_cnt < UINT32_MAX) {
			const float descent = m_dw_font_metrics.designUnitsPerEm == 0 ? 0.0f : (m_font_size * m_dw_font_metrics.descent / m_dw_font_metrics.designUnitsPerEm);

			ShapeStroke::get_pos_min(p_min);
			for (uint32_t i = 0; i < m_dw_test_cnt; i++) {
				DWRITE_HIT_TEST_METRICS const& tm = m_dw_test_metrics[i];
				DWRITE_LINE_METRICS const& lm = m_dw_line_metrics[i];
				const double top = static_cast<double>(tm.top) + lm.baseline + descent - m_font_size;
				pt_add(p_min, tm.left, top, h_min);
				if (!pt_in_rect(h_min, area_min, area_max)) {
					return false;
				}
				pt_add(h_min, tm.width, m_font_size, h_max);
				if (!pt_in_rect(h_max, area_min, area_max)) {
					return false;
				}
			}
		}
		return ShapeRect::in_area(area_min, area_max);
	}

	// ���̖����L�������肵, �L���Ȃ�, �����̏��̖��͔j����, �L���ȏ��̖��̔z��̗v�f�ƒu��������.
	// font	���̖�
	// �߂�l	�L���Ȃ� true
	bool ShapeText::is_available_font(wchar_t*& font) noexcept
	{
		if (font) {
			for (uint32_t i = 0; s_available_fonts[i]; i++) {
				if (font == s_available_fonts[i]) {
					return true;
				}
				if (wcscmp(font, s_available_fonts[i]) == 0) {
					delete[] font;
					font = s_available_fonts[i];
					return true;
				}
			}
		}
		return false;
	}

	// �L���ȏ��̖��̔z���j������.
	void ShapeText::release_available_fonts(void) noexcept
	{
		if (s_available_fonts != nullptr) {
			for (uint32_t i = 0; s_available_fonts[i]; i++) {
				delete[] s_available_fonts[i];
			}
			delete[] s_available_fonts;
			s_available_fonts = nullptr;
		}
	}

	// �L���ȏ��̖��̔z���ݒ肷��.
	//
	// DWriteFactory �̃V�X�e���t�H���g�R���N�V��������,
	// ����̒n��E���ꖼ�ɑΉ��������̂𓾂�,
	// ������z��Ɋi�[����.
	void ShapeText::set_available_fonts(const D2D_UI& d2d)
	{
		// ����̒n��E���ꖼ�𓾂�.
		wchar_t lang[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(lang, LOCALE_NAME_MAX_LENGTH);
		// �V�X�e���t�H���g�R���N�V������ DWriteFactory ���瓾��.
		winrt::com_ptr<IDWriteFontCollection> collection;
		winrt::check_hresult(d2d.m_dwrite_factory->GetSystemFontCollection(collection.put()));
		//winrt::check_hresult(Shape::s_dwrite_factory->GetSystemFontCollection(collection.put()));
		// �t�H���g�R���N�V�����̗v�f���𓾂�.
		const uint32_t f_cnt = collection->GetFontFamilyCount();
		// ����ꂽ�v�f�� + 1 �̔z����m�ۂ���.
		s_available_fonts = new wchar_t* [static_cast<size_t>(f_cnt) + 1];
		// �t�H���g�R���N�V�����̊e�v�f�ɂ���.
		for (uint32_t i = 0; i < f_cnt; i++) {
			// �v�f���珑�̂𓾂�.
			winrt::com_ptr<IDWriteFontFamily> font_family;
			winrt::check_hresult(collection->GetFontFamily(i, font_family.put()));
			// ���̂��烍�[�J���C�Y���ꂽ���̖��𓾂�.
			winrt::com_ptr<IDWriteLocalizedStrings> localized_name;
			winrt::check_hresult(font_family->GetFamilyNames(localized_name.put()));
			// ���[�J���C�Y���ꂽ���̖�����, �n�於���̂��������̖��̊J�n�ʒu�𓾂�.
			UINT32 index = 0;
			BOOL exists = false;
			winrt::check_hresult(localized_name->FindLocaleName(lang, &index, &exists));
			if (exists != TRUE) {
				// �n�於���Ȃ��ꍇ,
				// 0 ���J�n�ʒu�Ɋi�[����.
				index = 0;
			}
			// �J�n�ʒu�����̕������𓾂� (�k�������͊܂܂�Ȃ�).
			UINT32 length;
			winrt::check_hresult(localized_name->GetStringLength(index, &length));
			// ������ + 1 �̕����z����m�ۂ�, ���̖��̔z��Ɋi�[����.
			s_available_fonts[i] = new wchar_t[static_cast<size_t>(length) + 1];
			winrt::check_hresult(localized_name->GetString(index, s_available_fonts[i], length + 1));
			// ���[�J���C�Y���ꂽ���̖���j������.
			localized_name = nullptr;
			// ���̂���j������.
			font_family = nullptr;
		}
		// �L���ȏ��̖��̔z��̖����ɏI�[�Ƃ��ăk�����i�[����.
		s_available_fonts[f_cnt] = nullptr;
	}

	// �l�����̂̐F�Ɋi�[����.
	bool ShapeText::set_font_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_font_color, val)) {
			m_font_color = val;
			return true;
		}
		return false;
	}

	// �l�����̖��Ɋi�[����.
	bool ShapeText::set_font_family(wchar_t* const val) noexcept
	{
		// �l�����̖��Ɠ��������肷��.
		if (!equal(m_font_family, val)) {
			m_font_family = val;
			return true;
		}
		return false;
	}

	// �l�����̂̑傫���Ɋi�[����.
	bool ShapeText::set_font_size(const float val) noexcept
	{
		if (m_font_size != val) {
			m_font_size = val;
			return true;
		}
		return false;
	}

	// �l�����̂̉����Ɋi�[����.
	bool ShapeText::set_font_stretch(const DWRITE_FONT_STRETCH val) noexcept
	{
		if (m_font_stretch != val) {
			m_font_stretch = val;
			return true;
		}
		return false;
	}

	// �l�����̂̎��̂Ɋi�[����.
	bool ShapeText::set_font_style(const DWRITE_FONT_STYLE val) noexcept
	{
		if (m_font_style != val) {
			m_font_style = val;
			return true;
		}
		return false;
	}

	// �l�����̂̑����Ɋi�[����.
	bool ShapeText::set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept
	{
		if (m_font_weight != val) {
			m_font_weight = val;
			return true;
		}
		return false;
	}

	// �l�𕶎���Ɋi�[����.
	bool ShapeText::set_text_content(wchar_t* const val) noexcept
	{
		if (!equal(m_text, val)) {
			m_text = val;
			m_text_selected_range.startPosition = 0;
			m_text_selected_range.length = 0;
			if (m_dw_text_layout != nullptr) {
				//m_dw_text_layout->Release();
				m_dw_text_layout = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l��i���̂��낦�Ɋi�[����.
	bool ShapeText::set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept
	{
		if (m_text_align_p != val) {
			m_text_align_p = val;
			return true;
		}
		return false;
	}

	// �l�𕶎���̂��낦�Ɋi�[����.
	bool ShapeText::set_text_align_t(const DWRITE_TEXT_ALIGNMENT val) noexcept
	{
		if (m_text_align_t != val) {
			m_text_align_t = val;
			return true;
		}
		return false;
	}

	// �l���s�ԂɊi�[����.
	bool ShapeText::set_text_line_sp(const float val) noexcept
	{
		if (!equal(m_text_line_sp, val)) {
			m_text_line_sp = val;
			return true;
		}
		return false;
	}

	// �l�𕶎���̗]���Ɋi�[����.
	bool ShapeText::set_text_padding(const D2D1_SIZE_F val) noexcept
	{
		if (!equal(m_text_padding, val)) {
			m_text_padding = val;
			return true;
		}
		return false;
	}

	// �l��I�����ꂽ�����͈͂Ɋi�[����.
	bool ShapeText::set_text_selected(const DWRITE_TEXT_RANGE val) noexcept
	{
		if (!equal(m_text_selected_range, val)) {
			m_text_selected_range = val;
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// b_pos	�͂ޗ̈�̎n�_
	// b_vec	�͂ޗ̈�̏I�_�ւ̍���
	// text	������
	// s_attr	����
	ShapeText::ShapeText(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, wchar_t* const text, const ShapeSheet* s_attr) :
		ShapeRect::ShapeRect(b_pos, b_vec, s_attr),
		m_font_color(s_attr->m_font_color),
		m_font_family(s_attr->m_font_family),
		m_font_size(s_attr->m_font_size),
		m_font_stretch(s_attr->m_font_stretch),
		m_font_style(s_attr->m_font_style),
		m_font_weight(s_attr->m_font_weight),
		m_text_line_sp(s_attr->m_text_line_sp),
		m_text_padding(s_attr->m_text_padding),
		m_text(text),
		m_text_align_t(s_attr->m_text_align_t),
		m_text_align_p(s_attr->m_text_align_p),
		m_text_selected_range(DWRITE_TEXT_RANGE{ 0, 0 })
	{
	}

	// �}�`���f�[�^���C�^�[����ǂݍ���.
	ShapeText::ShapeText(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		dt_read(m_font_color, dt_reader);
		dt_read(m_font_family, dt_reader);
		is_available_font(m_font_family);
		m_font_size = dt_reader.ReadSingle();
		m_font_stretch = static_cast<DWRITE_FONT_STRETCH>(dt_reader.ReadUInt32());
		m_font_style = static_cast<DWRITE_FONT_STYLE>(dt_reader.ReadUInt32());
		m_font_weight = static_cast<DWRITE_FONT_WEIGHT>(dt_reader.ReadUInt32());
		dt_read(m_text, dt_reader);
		m_text_align_p = static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(dt_reader.ReadUInt32());
		m_text_align_t = static_cast<DWRITE_TEXT_ALIGNMENT>(dt_reader.ReadUInt32());
		m_text_line_sp = dt_reader.ReadSingle();
		dt_read(m_text_padding, dt_reader);
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapeText::write(DataWriter const& dt_writer) const
	{
		ShapeRect::write(dt_writer);
		dt_write(m_font_color, dt_writer);
		dt_write(m_font_family, dt_writer);
		dt_writer.WriteSingle(m_font_size);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_stretch));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_style));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_weight));
		dt_write(m_text, dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_p));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_t));
		dt_writer.WriteSingle(m_text_line_sp);
		dt_write(m_text_padding, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeText::write_svg(DataWriter const& dt_writer) const
	{
		static constexpr char* SVG_STYLE[] = {
			"normal", "oblique", "italic"
		};
		static constexpr char* SVG_STRETCH[] = {
			"normal", "ultra-condensed", "extra-condensed",
			"condensed", "semi-condensed", "normal", "semi-expanded",
			"expanded", "extra-expanded", "ultra-expanded"
		};
		// ���������̂��炵�ʂ����߂�.
		//
		// Chrome �ł�, �e�L�X�g�^�O�ɑ��� alignment-baseline="text-before-edge" 
		// ���w�肷�邾����, ����ʒu����ɂ��� Dwrite �Ɠ����悤�ɕ\�������.
		// ������, IE �� Edge �ł�, alignment-baseline �����͊��҂������������Ȃ��̂�,
		// �㕔����̃x�[�X���C���܂Œl�ł���, ���������̂��炵�� dy ��
		// �s���ƂɌv�Z��K�v������.
		// ���̂Ƃ�, �㕔����̃x�[�X���C���̍��� = �A�Z���g�ɂ͂Ȃ�Ȃ��̂�
		// �f�Z���g��p���Čv�Z����K�v������.
		// �e�L�X�g���C�A�E�g����t�H���g���g���b�N�X���擾����, �ȉ��̂悤�ɋ��߂�.
		// ���Ȃ݂�, designUnitsPerEm ��, �z�u (Em) �{�b�N�X�̒P�ʂ�����̑傫��.
		// �f�Z���g��, �t�H���g�����̔z�u�{�b�N�X�̉�������x�[�X���C���܂ł̒���.
		// dy = ���̍s�̃q�b�g�e�X�g���g���b�N�X�̍��� - �t�H���g�̑傫�� �~ (�f�Z���g �� �P�ʑ傫��) �ƂȂ�, �͂�.
		//IDWriteFontCollection* fonts;
		//m_dw_layout->GetFontCollection(&fonts);
		//IDWriteFontFamily* family;
		//fonts->GetFontFamily(0, &family);
		//IDWriteFont* font;
		//family->GetFont(0, &font);
		//DWRITE_FONT_METRICS metrics;
		//font->GetMetrics(&metrics);
		//const double descent = m_font_size * ((static_cast<double>(metrics.descent)) / metrics.designUnitsPerEm);

		if (is_opaque(m_fill_color) || is_opaque(m_stroke_color)) {
			ShapeRect::write_svg(dt_writer);
		}
		// ������S�̂̑������w�肷�邽�߂� g �^�O���J�n����.
		dt_write_svg("<g ", dt_writer);
		// ���̂̐F����������.
		dt_write_svg(m_font_color, "fill", dt_writer);
		// ���̖�����������.
		dt_write_svg("font-family=\"", dt_writer);
		dt_write_svg(m_font_family, wchar_len(m_font_family), dt_writer);
		dt_write_svg("\" ", dt_writer);
		// ���̂̑傫������������.
		dt_write_svg(m_font_size, "font-size", dt_writer);
		// ���̂̐L�k����������.
		const int32_t stretch = static_cast<int32_t>(m_font_stretch);
		dt_write_svg(SVG_STRETCH[stretch], "font-stretch", dt_writer);
		// ���̂̌`������������.
		const int32_t style = static_cast<int32_t>(m_font_style);
		dt_write_svg(SVG_STYLE[style], "font-style", dt_writer);
		// ���̂̑�������������.
		const uint32_t weight = static_cast<uint32_t>(m_font_weight);
		dt_write_svg(weight, "font-weight", dt_writer);
		dt_write_svg("none", "stroke", dt_writer);
		dt_write_svg(">" SVG_NEW_LINE, dt_writer);
		// ���̂�\�����鍶��ʒu�ɗ]����������.
		D2D1_POINT_2F nw_pos;
		pt_add(m_pos, m_text_padding.width, m_text_padding.height, nw_pos);
		for (uint32_t i = 0; i < m_dw_test_cnt; i++) {
			const DWRITE_HIT_TEST_METRICS& tm = m_dw_test_metrics[i];
			const wchar_t* t = m_text + tm.textPosition;
			const uint32_t t_len = tm.length;
			const double px = static_cast<double>(nw_pos.x);
			const double qx = static_cast<double>(tm.left);
			const double py = static_cast<double>(nw_pos.y);
			const double qy = static_cast<double>(tm.top);
			// �������\�����鐂���Ȃ��炵�ʒu�����߂�.
			const double dy = static_cast<double>(m_dw_line_metrics[i].baseline);
			// ���������������.
			tx_dt_write_svg(t, t_len, px + qx, py + qy, dy, dt_writer);
		}
		dt_write_svg("</g>" SVG_NEW_LINE, dt_writer);
	}

}
