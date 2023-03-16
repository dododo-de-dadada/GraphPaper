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
	//using winrt::GraphPaper::implementation::get_font_face;

	wchar_t** ShapeText::s_available_fonts = nullptr;	//�L���ȏ��̖�
	D2D1_COLOR_F ShapeText::s_text_selected_background{ COLOR_ACCENT };	// �����͈͂̔w�i�F
	D2D1_COLOR_F ShapeText::s_text_selected_foreground{ COLOR_TEXT_RANGE };	// �����͈͂̕����F

	// �q�b�g�e�X�g�̌v�ʂ��쐬����.
	static void text_create_test_metrics(IDWriteTextLayout* text_lay, 
		const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt);
	// �q�b�g�e�X�g�̌v��, �s�̌v��, ������I���̌v�ʂ��쐬����.
	static void text_create_text_metrics(IDWriteTextLayout* text_lay, const uint32_t text_len,
		UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_met, DWRITE_LINE_METRICS*& line_met, 
		UINT32& sele_cnt, DWRITE_HIT_TEST_METRICS*& sele_met, const DWRITE_TEXT_RANGE& sele_rng);
	// ���̂̌v�ʂ𓾂�.
	static void text_get_font_metrics(IDWriteTextLayout* text_lay, DWRITE_FONT_METRICS* font_met);

	//------------------------------
	// �q�b�g�e�X�g�̌v�ʂ��쐬����.
	// text_lay	�����񃌃C�A�E�g
	// text_rng	�����͈�
	// test_met	�q�b�g�e�X�g�̌v��
	// test_cnt	�v�ʂ̗v�f��
	//------------------------------
	static void text_create_test_metrics(IDWriteTextLayout* text_lay, 
		const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt)
	{
		const uint32_t pos = text_rng.startPosition;
		const uint32_t len = text_rng.length;
		DWRITE_HIT_TEST_METRICS test[1];

		// �܂��v�ʂ̗v�f���𓾂�.
		// �ŏ��� HitTestTextRange �֐��Ăяo����, ���s���邱�Ƃ��O��Ȃ̂�, check_hresult ���Ȃ�.
		// �z����m�ۂ���, ���炽�߂Ċ֐����Ăяo��, �v�ʂ𓾂�.
		text_lay->HitTestTextRange(pos, len, 0, 0, test, 1, &test_cnt);
		test_met = new DWRITE_HIT_TEST_METRICS[test_cnt];
		winrt::check_hresult(
			text_lay->HitTestTextRange(pos, len, 0, 0, test_met, test_cnt, &test_cnt));
	}

	//------------------------------
	// ���̂̌v�ʂ𓾂�
	// text_lay	�e�L�X�g���C�A�E�g
	// font_met ���̂̌v��
	//------------------------------
	static void text_get_font_metrics(IDWriteTextLayout* text_lay, DWRITE_FONT_METRICS* font_met)
	{
		// �����񃌃C�A�E�g ---> ���̃��X�g ---> ���̃t�@�~���[ ---> ���̂𓾂�.
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

	//------------------------------
	// �q�b�g�e�X�g�̌v��, �s�̌v��, �I�����ꂽ�����͈͂̌v�ʂ�j������.
	// test_cnt	�q�b�g�e�X�g�ƍs�̌v�ʂ̊e�v�f��
	// test_met	�q�b�g�e�X�g�̌v��
	// line_met �s�̌v��
	// sele_cnt	�I�����ꂽ�����͈͂̌v�ʂ̗v�f��
	// sele_met �I�����ꂽ�����͈͂̌v��
	//------------------------------
	void ShapeText::relese_metrics(void) noexcept
	{
		m_dwrite_test_cnt = 0;
		if (m_dwrite_test_metrics != nullptr) {
			delete[] m_dwrite_test_metrics;
			m_dwrite_test_metrics = nullptr;
		}
		if (m_dwrite_line_metrics != nullptr) {
			delete[] m_dwrite_line_metrics;
			m_dwrite_line_metrics = nullptr;
		}
		m_dwrite_selected_cnt = 0;
		if (m_dwrite_selected_metrics != nullptr) {
			delete[] m_dwrite_selected_metrics;
			m_dwrite_selected_metrics = nullptr;
		}
	}

	//------------------------------
	// �q�b�g�e�X�g�̌v��, �s�̌v��, ������I���̌v�ʂ𓾂�.
	// text_lay	�����񃌃C�A�E�g
	// text_len	������̒���
	// test_cnt	�q�b�g�e�X�g�̌v�ʂ̗v�f��
	// test_met	�q�b�g�e�X�g�̌v��
	// line_met �s�̌v��
	// sele_cnt	�I�����ꂽ�����͈͂̌v�ʂ̗v�f��
	// sele_met �I�����ꂽ�����͈͂̌v��
	// sele_rng	�I�����ꂽ�����͈�
	//------------------------------
	static void text_create_text_metrics(
		IDWriteTextLayout* text_lay,
		const uint32_t text_len,
		UINT32& test_cnt,
		DWRITE_HIT_TEST_METRICS*& test_met,
		DWRITE_LINE_METRICS*& line_met,
		UINT32& sele_cnt,
		DWRITE_HIT_TEST_METRICS*& sele_met,
		const DWRITE_TEXT_RANGE& sele_rng)
	{
		if (text_lay != nullptr) {
			// �q�b�g�e�X�g�̌v�ʂ��쐬����.
			text_create_test_metrics(text_lay, 
				{ 0, text_len }, test_met, test_cnt);

			// �s�̌v�ʂ��쐬����.
			UINT32 line_cnt;
			text_lay->GetLineMetrics(nullptr, 0, &line_cnt);
			line_met = new DWRITE_LINE_METRICS[line_cnt];
			text_lay->GetLineMetrics(line_met, line_cnt, &line_cnt);

			// �I�����ꂽ�����͈͂̌v�ʂ��쐬����.
			if (sele_rng.length > 0) {
				text_create_test_metrics(text_lay,
					sele_rng, sele_met, sele_cnt);
			}
		}
	}

	//------------------------------
	// �g�𕶎���ɍ��킹��.
	// g_len	����̑傫�� (1 �ȏ�Ȃ�Ε���̑傫���ɍ��킹��)
	// �߂�l	�傫�����������ꂽ�Ȃ�ΐ^.
	//------------------------------
	bool ShapeText::fit_frame_to_text(const float g_len) noexcept
	{
		// ������̑傫�����v�Z��, �g�Ɋi�[����.
		D2D1_POINT_2F t_box{ 0.0f, 0.0f };	// �g
		for (size_t i = 0; i < m_dwrite_test_cnt; i++) {
			t_box.x = fmax(t_box.x, m_dwrite_test_metrics[i].width);
			t_box.y += m_dwrite_test_metrics[i].height;
		}
		// �g�ɍ��E�̃p�f�B���O��������.
		const float sp = m_text_pad.width * 2.0f;	// ���E�̃p�f�B���O
		pt_add(t_box, sp, sp, t_box);
		if (g_len >= 1.0f) {
			// �g�����̑傫���ɐ؂�グ��.
			t_box.x = floor((t_box.x + g_len - 1.0f) / g_len) * g_len;
			t_box.y = floor((t_box.y + g_len - 1.0f) / g_len) * g_len;
		}
		// �}�`�̑傫����ύX����.
		if (!equal(t_box, m_pos)) {
			D2D1_POINT_2F se;
			pt_add(m_start, t_box, se);
			set_pos_anc(se, ANC_TYPE::ANC_SE, 0.0f, false);
			return true;
		}
		return false;
	}

	//------------------------------
	// �����񃌃C�A�E�g���쐬����.
	//------------------------------
	void ShapeText::create_text_layout(void)
	{
		IDWriteFactory* const dwrite_factory = Shape::m_dwrite_factory.get();

		// �V�K�쐬
		// �����񃌃C�A�E�g���󂩔��肷��.
		if (m_dwrite_text_layout == nullptr) {
			// ����̃��P�[�����𓾂�.
			wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
			GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);

			// ������t�H�[�}�b�g���쐬����.
			// CreateTextFormat �� DWRITE_FONT_STRETCH_UNDEFINED ���w�肳�ꂽ�ꍇ�G���[�ɂȂ�.
			// �����l���Ȃ�ł���, DWRITE_FONT_STRETCH_NORMAL �Ńe�L�X�g�t�H�[�}�b�g�͍쐬����.
			winrt::com_ptr<IDWriteTextFormat> t_format;
			winrt::check_hresult(
				dwrite_factory->CreateTextFormat(
					m_font_family, static_cast<IDWriteFontCollection*>(nullptr),
					m_font_weight, m_font_style, m_font_stretch,
					m_font_size, locale_name, t_format.put())
			);

			// ������t�H�[�}�b�g���當���񃌃C�A�E�g���쐬����.
			const double text_w = std::fabs(m_pos.x) - 2.0 * m_text_pad.width;
			const double text_h = std::fabs(m_pos.y) - 2.0 * m_text_pad.height;
			const UINT32 text_len = wchar_len(m_text);
			winrt::check_hresult(
				dwrite_factory->CreateTextLayout(
					m_text, text_len, t_format.get(), static_cast<FLOAT>(max(text_w, 0.0)),
					static_cast<FLOAT>(max(text_h, 0.0)), m_dwrite_text_layout.put()));

			// ������t�H�[�}�b�g��j������.
			t_format = nullptr;

			winrt::com_ptr<IDWriteTextLayout3> t3;
			if (m_dwrite_text_layout.try_as(t3)) {
				// �����̕�, �����̂��낦, �i���̂��낦�𕶎��񃌃C�A�E�g�Ɋi�[����.
				winrt::check_hresult(
					t3->SetFontStretch(m_font_stretch, DWRITE_TEXT_RANGE{ 0, text_len }));
				winrt::check_hresult(
					t3->SetTextAlignment(m_text_align_horz));
				winrt::check_hresult(
					t3->SetParagraphAlignment(m_text_align_vert));

				// �s�Ԃ𕶎��񃌃C�A�E�g�Ɋi�[����.
				DWRITE_LINE_SPACING new_sp;
				new_sp.leadingBefore = 0.0f;
				new_sp.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
				// �s�Ԃ��[�����傫���Ȃ�, �s�Ԃ�ݒ肷��.
				if (m_text_line_sp >= FLT_MIN) {
					if (m_dwrite_font_metrics.designUnitsPerEm == 0) {
						text_get_font_metrics(t3.get(), &m_dwrite_font_metrics);
					}
					new_sp.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
					new_sp.height = m_font_size + m_text_line_sp;
					if (m_dwrite_font_metrics.designUnitsPerEm == 0) {
						new_sp.baseline = m_font_size + m_text_line_sp;
					}
					else {
						const float descent = m_font_size * m_dwrite_font_metrics.descent /
							m_dwrite_font_metrics.designUnitsPerEm;
						new_sp.baseline = m_font_size + m_text_line_sp - descent;
					}
				}
				// ����̍s�Ԃ�ݒ肷��.
				else {
					new_sp.method = DWRITE_LINE_SPACING_METHOD_DEFAULT;
					new_sp.height = 0.0f;
					new_sp.baseline = 0.0f;
				}
				winrt::check_hresult(
					t3->SetLineSpacing(&new_sp));
				t3 = nullptr;
			}
			// �ʒu�̌v��, �s�̌v��, ������I���̌v�ʂ�j������.
			relese_metrics();
			text_create_text_metrics(
				m_dwrite_text_layout.get(), wchar_len(m_text), m_dwrite_test_cnt,
				m_dwrite_test_metrics, m_dwrite_line_metrics, m_dwrite_selected_cnt,
				m_dwrite_selected_metrics, m_text_selected_range);
		}

		// �ύX.
		else {

			winrt::com_ptr<IDWriteTextLayout3> t3;
			if (m_dwrite_text_layout.try_as(t3)) {
				const uint32_t text_len = wchar_len(m_text);
				bool updated = false;

				// ���̖����ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				const UINT32 n_size = t3->GetFontFamilyNameLength() + 1;
				std::vector<WCHAR> font_family(n_size);
				winrt::check_hresult(
					t3->GetFontFamilyName(font_family.data(), n_size));
				if (wcscmp(font_family.data(), m_font_family) != 0) {
					winrt::check_hresult(
						t3->SetFontFamilyName(m_font_family, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}
				font_family.resize(0);
				font_family.shrink_to_fit();

				// ���̂̑傫�����ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				FLOAT font_size;
				winrt::check_hresult(
					t3->GetFontSize(0, &font_size));
				if (!equal(font_size, m_font_size)) {
					winrt::check_hresult(
						t3->SetFontSize(m_font_size, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// ���̂̕����ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				DWRITE_FONT_STRETCH font_stretch;
				winrt::check_hresult(
					t3->GetFontStretch(0, &font_stretch));
				if (!equal(font_stretch, m_font_stretch)) {
					winrt::check_hresult(
						t3->SetFontStretch(m_font_stretch, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// ���̂̎��̂��ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				DWRITE_FONT_STYLE font_style;
				winrt::check_hresult(
					t3->GetFontStyle(0, &font_style));
				if (!equal(font_style, m_font_style)) {
					winrt::check_hresult(
						t3->SetFontStyle(m_font_style, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// ���̂̑������ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				DWRITE_FONT_WEIGHT font_weight;
				winrt::check_hresult(
					t3->GetFontWeight(0, &font_weight));
				if (!equal(font_weight, m_font_weight)) {
					winrt::check_hresult(
						t3->SetFontWeight(m_font_weight, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// ������̃p�f�B���O���ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				FLOAT text_w = static_cast<FLOAT>(std::fabs(m_pos.x) - m_text_pad.width * 2.0);
				FLOAT text_h = static_cast<FLOAT>(std::fabs(m_pos.y) - m_text_pad.height * 2.0);
				if (text_w < 0.0f) {
					text_w = 0.0;
				}
				if (text_h < 0.0f) {
					text_h = 0.0f;
				}
				if (!equal(text_w, t3->GetMaxWidth())) {
					winrt::check_hresult(
						t3->SetMaxWidth(text_w));
					if (!updated) {
						updated = true;
					}
				}
				if (!equal(text_h, t3->GetMaxHeight())) {
					winrt::check_hresult(
						t3->SetMaxHeight(text_h));
					if (!updated) {
						updated = true;
					}
				}

				// �i���̂��낦���ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				DWRITE_PARAGRAPH_ALIGNMENT para_align = t3->GetParagraphAlignment();
				if (!equal(para_align, m_text_align_vert)) {
					winrt::check_hresult(
						t3->SetParagraphAlignment(m_text_align_vert));
					if (!updated) {
						updated = true;
					}
				}

				// �����̂��낦���ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				DWRITE_TEXT_ALIGNMENT text_align = t3->GetTextAlignment();
				if (!equal(text_align, m_text_align_horz)) {
					winrt::check_hresult(
						t3->SetTextAlignment(m_text_align_horz));
					if (!updated) {
						updated = true;
					}
				}

				// �s�Ԃ��ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
				DWRITE_LINE_SPACING old_sp;
				winrt::check_hresult(
					t3->GetLineSpacing(&old_sp));

				DWRITE_LINE_SPACING new_sp;
				new_sp.leadingBefore = 0.0f;
				new_sp.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;

				// �s�Ԃ��[�����傫���Ȃ�, �s�Ԃ�ݒ肷��.
				if (m_text_line_sp >= FLT_MIN) {
					if (m_dwrite_font_metrics.designUnitsPerEm == 0) {
						text_get_font_metrics(t3.get(), &m_dwrite_font_metrics);
					}
					new_sp.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
					new_sp.height = m_font_size + m_text_line_sp;
					if (m_dwrite_font_metrics.designUnitsPerEm == 0) {
						new_sp.baseline = m_font_size + m_text_line_sp;
					}
					else {
						const float descent = m_font_size * m_dwrite_font_metrics.descent /
							m_dwrite_font_metrics.designUnitsPerEm;
						new_sp.baseline = m_font_size + m_text_line_sp - descent;
					}
				}

				// ����̍s�Ԃ�ݒ肷��.
				else {
					new_sp.method = DWRITE_LINE_SPACING_METHOD_DEFAULT;
					new_sp.height = 0.0f;
					new_sp.baseline = 0.0f;
				}
				if (memcmp(&old_sp, &new_sp, sizeof(old_sp)) != 0) {
					winrt::check_hresult(
						t3->SetLineSpacing(&new_sp));
					if (!updated) {
						updated = true;
					}
				}

				// �v�ʂ̗v�f���� 0 �Ȃ̂ɑI�����ꂽ�����͈͂� 0 �łȂ�
				// �܂���, �I��͈͂̒�����, ����܂ł̑I�����ꂽ�����͈͂̌v�ʂƈقȂ�Ȃ�,
				// �X�V�t���O�����Ă�.
				if (m_dwrite_selected_cnt == 0) {
					if (m_text_selected_range.length > 0) {
						if (!updated) {
							updated = true;
						}
					}
				}
				else {
					uint32_t s_len = 0;	// �I��͈͂̒���
					for (uint32_t i = 0; i < m_dwrite_selected_cnt; i++) {
						s_len += m_dwrite_selected_metrics[i].length;
					}
					if (m_dwrite_selected_metrics[0].textPosition != m_text_selected_range.startPosition ||
						s_len != m_text_selected_range.length) {
						if (!updated) {
							updated = true;
						}
					}
				}

				if (updated) {
					relese_metrics();
					text_create_text_metrics(m_dwrite_text_layout.get(), wchar_len(m_text),
						m_dwrite_test_cnt, m_dwrite_test_metrics, m_dwrite_line_metrics,
						m_dwrite_selected_cnt, m_dwrite_selected_metrics, m_text_selected_range);
				}
			}
		}
	}

	// �}�`��\������.
	void ShapeText::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const color_brush = Shape::m_d2d_color_brush.get();
		ID2D1SolidColorBrush* const range_brush = Shape::m_d2d_range_brush.get();
		ID2D1Factory* factory;
		target->GetFactory(&factory);

		// ���`��`��.
		ShapeRect::draw();

		// �����񂪋󂩔��肷��.
		if (m_text == nullptr || m_text[0] == L'\0') {
			relese_metrics();
			// �����񃌃C�A�E�g����łȂ��Ȃ�j������.
			if (m_dwrite_text_layout != nullptr) {
				m_dwrite_text_layout = nullptr;
			}
		}
		else {
			create_text_layout();

			// �]���������킦��, ������̍���ʒu���v�Z����.
			D2D1_POINT_2F t_lt;
			pt_add(m_start, m_pos, t_lt);
			t_lt.x = m_start.x < t_lt.x ? m_start.x : t_lt.x;
			t_lt.y = m_start.y < t_lt.y ? m_start.y : t_lt.y;
			const FLOAT pw = m_text_pad.width;
			const FLOAT ph = m_text_pad.height;
			const double hm = min(pw, fabs(m_pos.x) * 0.5);
			const double vm = min(ph, fabs(m_pos.y) * 0.5);
			pt_add(t_lt, hm, vm, t_lt);

			// �I�����ꂽ�����͈͂�����Ȃ�, �w�i��h��Ԃ�.
			if (m_text_selected_range.length > 0) {
				if (m_dwrite_font_metrics.designUnitsPerEm == 0) {
					text_get_font_metrics(m_dwrite_text_layout.get(), &m_dwrite_font_metrics);
				}
				const float descent = (m_dwrite_font_metrics.designUnitsPerEm == 0 ? 0.0f : m_font_size * m_dwrite_font_metrics.descent / m_dwrite_font_metrics.designUnitsPerEm);
				const uint32_t rc = m_dwrite_selected_cnt;
				const uint32_t tc = m_dwrite_test_cnt;
				for (uint32_t i = 0; i < rc; i++) {
					const DWRITE_HIT_TEST_METRICS& rm = m_dwrite_selected_metrics[i];
					for (uint32_t j = 0; j < tc; j++) {
						const DWRITE_HIT_TEST_METRICS& tm = m_dwrite_test_metrics[j];
						const DWRITE_LINE_METRICS& lm = m_dwrite_line_metrics[j];
						if (tm.textPosition <= rm.textPosition && rm.textPosition + rm.length <= tm.textPosition + tm.length) {
							D2D1_RECT_F rect;
							rect.left = t_lt.x + rm.left;
							rect.top = static_cast<FLOAT>(t_lt.y + tm.top + lm.baseline + descent - m_font_size);
							if (rm.width < FLT_MIN) {
								const float sp_len = max(lm.trailingWhitespaceLength * m_font_size * 0.25f, 1.0f);
								rect.right = rect.left + sp_len;
							}
							else {
								rect.right = rect.left + rm.width;
							}
							rect.bottom = rect.top + m_font_size;
							color_brush->SetColor(ShapeText::s_text_selected_foreground);
							target->DrawRectangle(rect, color_brush, 2.0, nullptr);
							color_brush->SetColor(ShapeText::s_text_selected_background);
							target->FillRectangle(rect, color_brush);
							break;
						}
					}
				}
				range_brush->SetColor(ShapeText::s_text_selected_foreground);
				winrt::check_hresult(
					m_dwrite_text_layout->SetDrawingEffect(range_brush, m_text_selected_range));
			}

			// �������\������
			color_brush->SetColor(m_font_color);
			target->DrawTextLayout(t_lt, m_dwrite_text_layout.get(), color_brush);
			if (m_text_selected_range.length > 0) {
				winrt::check_hresult(
					m_dwrite_text_layout->SetDrawingEffect(nullptr, { 0, wchar_len(m_text) }));
			}

			// �}�`���I������Ă���Ȃ�, ������̕⏕����\������
			if (m_anc_show && is_selected()) {
				const uint32_t d_cnt = Shape::m_aux_style->GetDashesCount();
				if (d_cnt <= 0 || d_cnt > 6) {
					return;
				}

				FLOAT d_arr[6];
				Shape::m_aux_style->GetDashes(d_arr, d_cnt);
				double mod = d_arr[0];
				for (uint32_t i = 1; i < d_cnt; i++) {
					mod += d_arr[i];
				}
				if (m_dwrite_font_metrics.designUnitsPerEm == 0) {
					text_get_font_metrics(m_dwrite_text_layout.get(), &m_dwrite_font_metrics);
				}

				//D2D1_MATRIX_3X2_F tran;
				//target->GetTransform(&tran);
				//const auto s_width = 1.0 / tran.m11;
				D2D1_STROKE_STYLE_PROPERTIES1 s_prop{ AUXILIARY_SEG_STYLE };

				const float descent = (m_dwrite_font_metrics.designUnitsPerEm == 0 ? 0.0f : m_font_size * m_dwrite_font_metrics.descent / m_dwrite_font_metrics.designUnitsPerEm);
				for (uint32_t i = 0; i < m_dwrite_test_cnt; i++) {
					DWRITE_HIT_TEST_METRICS const& tm = m_dwrite_test_metrics[i];
					DWRITE_LINE_METRICS const& lm = m_dwrite_line_metrics[i];
					// �j��������ďd�Ȃ��ĕ\������Ȃ��悤��, �j���̃I�t�Z�b�g���v�Z��,
					// ������̘g��ӂ��Ƃɕ\������.
					D2D1_POINT_2F p[4];
					p[0].x = t_lt.x + tm.left;
					p[0].y = static_cast<FLOAT>(t_lt.y + tm.top + lm.baseline + descent - m_font_size);
					p[2].x = p[0].x + tm.width;
					p[2].y = p[0].y + m_font_size;
					p[1].x = p[2].x;
					p[1].y = p[0].y;
					p[3].x = p[0].x;
					p[3].y = p[2].y;
					const D2D1_RECT_F r{
						p[0].x, p[0].y, p[2].x, p[2].y
					};

					color_brush->SetColor(ShapeText::s_text_selected_foreground);
					target->DrawRectangle(r, color_brush, Shape::m_aux_width, nullptr);
					color_brush->SetColor(ShapeText::s_text_selected_background);
					s_prop.dashOffset = static_cast<FLOAT>(std::fmod(p[0].x, mod));
					winrt::com_ptr<ID2D1StrokeStyle1> selected_style;
					static_cast<ID2D1Factory1*>(factory)->CreateStrokeStyle(&s_prop, d_arr, d_cnt, selected_style.put());
					target->DrawLine(p[0], p[1], color_brush, Shape::m_aux_width, selected_style.get());
					target->DrawLine(p[3], p[2], color_brush, Shape::m_aux_width, selected_style.get());
					selected_style = nullptr;
					s_prop.dashOffset = static_cast<FLOAT>(std::fmod(p[0].y, mod));
					static_cast<ID2D1Factory1*>(factory)->CreateStrokeStyle(&s_prop, d_arr, d_cnt, selected_style.put());
					target->DrawLine(p[1], p[2], color_brush, Shape::m_aux_width, selected_style.get());
					target->DrawLine(p[0], p[3], color_brush, Shape::m_aux_width, selected_style.get());
					selected_style = nullptr;
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

	// ���̂̕��𓾂�.
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
	bool ShapeText::get_text_align_vert(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_vert;
		return true;
	}

	// ������̂��낦�𓾂�.
	bool ShapeText::get_text_align_horz(DWRITE_TEXT_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_horz;
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
	bool ShapeText::get_text_pad(D2D1_SIZE_F& val) const noexcept
	{
		val = m_text_pad;
		return true;
	}

	// �����͈͂𓾂�
	bool ShapeText::get_text_selected(DWRITE_TEXT_RANGE& val) const noexcept
	{
		val = m_text_selected_range;
		return true;
	}

	// �ʒu���܂ނ����肷��.
	// test	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeText::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		const uint32_t anc = rect_hit_test_anc(m_start, m_pos, test, m_anc_width);
		if (anc != ANC_TYPE::ANC_PAGE) {
			return anc;
		}
		const float descent = m_dwrite_font_metrics.designUnitsPerEm == 0 ? 0.0f : 
			(m_font_size * m_dwrite_font_metrics.descent / m_dwrite_font_metrics.designUnitsPerEm);

		// ������͈̔͂̍��オ���_�ɂȂ�悤, ���肷��ʒu���ړ�����.
		D2D1_POINT_2F lt;
		ShapeRect::get_bound_lt(lt);
		pt_sub(test, lt, lt);
		pt_sub(lt, m_text_pad, lt);
		for (uint32_t i = 0; i < m_dwrite_test_cnt; i++) {
			const auto tl = m_dwrite_test_metrics[i].left;
			const auto tw = m_dwrite_test_metrics[i].width;
			const auto tt = m_dwrite_test_metrics[i].top;
			const auto bl = m_dwrite_line_metrics[i].baseline;
			if (pt_in_rect(
				lt, D2D1_POINT_2F{ tl, tt + bl + descent - m_font_size },
				D2D1_POINT_2F{ tl + tw, tt + bl + descent })) {
				return ANC_TYPE::ANC_TEXT;
			}
		}
		return ShapeRect::hit_test(test);
	}

	// ��`�͈͂Ɋ܂܂�邩���肷��.
	// lt	��`�̍���ʒu
	// rb	��`�̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeText::in_area(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		D2D1_POINT_2F p_lt;	// ����ʒu

		if (m_dwrite_test_cnt > 0 && m_dwrite_test_cnt < UINT32_MAX) {
			const float descent = 
				m_dwrite_font_metrics.designUnitsPerEm == 0 ? 0.0f :
				(m_font_size * m_dwrite_font_metrics.descent / m_dwrite_font_metrics.designUnitsPerEm);

			ShapeRect::get_bound_lt(p_lt);
			for (uint32_t i = 0; i < m_dwrite_test_cnt; i++) {
				//const DWRITE_HIT_TEST_METRICS& t_met = m_dwrite_test_metrics[i];
				const auto tl = m_dwrite_test_metrics[i].left;
				const auto tt = m_dwrite_test_metrics[i].top;
				const auto tw = m_dwrite_test_metrics[i].width;
				//const DWRITE_LINE_METRICS& l_met = m_dwrite_line_metrics[i];
				const auto bl = m_dwrite_line_metrics[i].baseline;
				const double top = static_cast<double>(tt) + bl + descent - m_font_size;
				D2D1_POINT_2F t_lt;	// ������̍���ʒu
				pt_add(p_lt, tl, top, t_lt);
				if (!pt_in_rect(t_lt, lt, rb)) {
					return false;
				}
				D2D1_POINT_2F t_rb;	// ������̉E���ʒu
				pt_add(t_lt, tw, m_font_size, t_rb);
				if (!pt_in_rect(t_rb, lt, rb)) {
					return false;
				}
			}
		}
		return ShapeRect::in_area(lt, rb);
	}

	// ���̖����L�������肷��.
	// font	���̖�
	// �߂�l	�L���Ȃ� true
	bool ShapeText::is_available_font(wchar_t*& font) noexcept
	{
		if (font) {
			for (uint32_t i = 0; s_available_fonts[i]; i++) {
				// �L���ȏ��̖��ƃA�h���X����v�����Ȃ� true ��Ԃ�.
				if (font == s_available_fonts[i]) {
					return true;
				}
				// �A�h���X�͈Ⴄ���L���ȏ��̖��ƈ�v�����Ȃ�,
				if (wcscmp(font, s_available_fonts[i]) == 0) {
					// �����̏��̖��͔j����, �����ɗL���ȏ��̖��������Ɋi�[��, true ��Ԃ�.
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
			for (uint32_t i = 0; s_available_fonts[i] != nullptr; i++) {
				delete[] s_available_fonts[i];
				s_available_fonts[i] = nullptr;
			}
			delete[] s_available_fonts;
			s_available_fonts = nullptr;
		}
	}

	// �L���ȏ��̖��̔z���ݒ肷��.
	// DWriteFactory �̃V�X�e���t�H���g�R���N�V��������,
	// ����̒n��E���ꖼ�ɑΉ��������̖��𓾂�,
	// ������z��Ɋi�[����.
	// 'en-us' �� GetUserDefaultLocaleName �œ���ꂽ���P�[����, 2 �̏��̖��𓾂�.
	// ���̂悤�Ɋi�[�����.
	// "�l�r �S�V�b�N\0MS Gothic\0"
	void ShapeText::set_available_fonts(void)
	{
		//s_d2d_factory = d2d.m_d2d_factory.get();
		// ����̒n��E���ꖼ�𓾂�.
		wchar_t lang[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(lang, LOCALE_NAME_MAX_LENGTH);

		// �V�X�e�������t�H���g�W���� DWriteFactory ���瓾��.
		winrt::com_ptr<IDWriteFontCollection> collection;
		winrt::check_hresult(
			Shape::m_dwrite_factory->GetSystemFontCollection(collection.put()));

		// �t�H���g�W���̗v�f���𓾂�.
		const uint32_t f_cnt = collection->GetFontFamilyCount();

		// ����ꂽ�v�f�� + 1 �̔z����m�ۂ���.
		s_available_fonts = new wchar_t* [static_cast<size_t>(f_cnt) + 1];

		// �t�H���g�W���̊e�v�f�ɂ���.
		for (uint32_t i = 0; i < f_cnt; i++) {

			// �v�f���珑�̂𓾂�.
			winrt::com_ptr<IDWriteFontFamily> font_family;
			winrt::check_hresult(
				collection->GetFontFamily(i, font_family.put()));

			// ���̂��烍�[�J���C�Y���ꂽ���̖��𓾂�.
			winrt::com_ptr<IDWriteLocalizedStrings> localized_name;
			winrt::check_hresult(
				font_family->GetFamilyNames(localized_name.put()));

			// ���[�J���C�Y���ꂽ���̖��̈ʒu�𓾂�.
			UINT32 index_en_us = 0;
			UINT32 index_local = 0;
			BOOL exists = false;
			winrt::check_hresult(
				localized_name->FindLocaleName(L"en-us", &index_en_us, &exists));
			if (exists != TRUE) {
				// en-us ���Ȃ��ꍇ,
				// 0 ���ʒu�Ɋi�[����.
				index_en_us = 0;
			}
			winrt::check_hresult(
				localized_name->FindLocaleName(lang, &index_local, &exists));
			if (exists != TRUE) {
				// �n�於���Ȃ��ꍇ,
				// en_us ���J�n�ʒu�Ɋi�[����.
				index_local = index_en_us;
			}

			// �J�n�ʒu�����̕������𓾂� (�k�������͊܂܂�Ȃ�).
			UINT32 length_en_us = 0;
			UINT32 length_local = 0;
			winrt::check_hresult(
				localized_name->GetStringLength(index_en_us, &length_en_us));
			winrt::check_hresult(
				localized_name->GetStringLength(index_local, &length_local));

			// ������ + 1 �̕����z����m�ۂ�, ���̖��̔z��Ɋi�[����.
			s_available_fonts[i] = new wchar_t[length_en_us + 1 + length_local + 1];
			winrt::check_hresult(
				localized_name->GetString(index_en_us, s_available_fonts[i], length_en_us + 1));
			winrt::check_hresult(
				localized_name->GetString(index_local, s_available_fonts[i] + length_en_us + 1,
					length_local + 1));

			// ���[�J���C�Y���ꂽ���̖��Ə��̂�j������.
			localized_name = nullptr;
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
			//if (m_dwrite_text_layout != nullptr) {
			//	m_dwrite_text_layout = nullptr;
			//}
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
			//if (m_dwrite_text_layout != nullptr) {
			//	m_dwrite_text_layout = nullptr;
			//}
			return true;
		}
		return false;
	}

	// �l�����̂̎��̂Ɋi�[����.
	bool ShapeText::set_font_style(const DWRITE_FONT_STYLE val) noexcept
	{
		if (m_font_style != val) {
			m_font_style = val;
			//if (m_dwrite_text_layout != nullptr) {
			//	m_dwrite_text_layout = nullptr;
			//}
			return true;
		}
		return false;
	}

	// �l�����̂̑����Ɋi�[����.
	bool ShapeText::set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept
	{
		if (m_font_weight != val) {
			m_font_weight = val;
			//if (m_dwrite_text_layout != nullptr) {
			//	m_dwrite_text_layout = nullptr;
			//}
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
			if (m_dwrite_text_layout != nullptr) {
				m_dwrite_text_layout = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l��i���̂��낦�Ɋi�[����.
	bool ShapeText::set_text_align_vert(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept
	{
		if (m_text_align_vert != val) {
			m_text_align_vert = val;
			return true;
		}
		return false;
	}

	// �l�𕶎���̂��낦�Ɋi�[����.
	bool ShapeText::set_text_align_horz(const DWRITE_TEXT_ALIGNMENT val) noexcept
	{
		if (m_text_align_horz != val) {
			m_text_align_horz = val;
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
	bool ShapeText::set_text_pad(const D2D1_SIZE_F val) noexcept
	{
		if (!equal(m_text_pad, val)) {
			m_text_pad = val;
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
	// start	�n�_
	// pos	�Ίp�_�ւ̈ʒu�x�N�g��
	// text	������
	// page	����
	ShapeText::ShapeText(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, wchar_t* const text, const Shape* page) :
		ShapeRect::ShapeRect(start, pos, page)
	{
		page->get_font_color(m_font_color),
		page->get_font_family(m_font_family),
		page->get_font_size(m_font_size),
		page->get_font_stretch(m_font_stretch),
		page->get_font_style(m_font_style),
		page->get_font_weight(m_font_weight),
		page->get_text_line_sp(m_text_line_sp),
		page->get_text_pad(m_text_pad),
		m_text = text;
		page->get_text_align_horz(m_text_align_horz);
		page->get_text_align_vert(m_text_align_vert);
		m_text_selected_range = DWRITE_TEXT_RANGE{ 0, 0 };
		ShapeText::is_available_font(m_font_family);
	}

	bool ShapeText::get_font_face(IDWriteFontFace3*& face) const noexcept
	{
		return text_get_font_face<IDWriteTextLayout>(
			m_dwrite_text_layout.get(), m_font_family, m_font_weight, m_font_stretch, m_font_style,
			face);
	}

	static wchar_t* text_read_text(DataReader const& dt_reader)
	{
		const int text_len = dt_reader.ReadUInt32();
		wchar_t* text = new wchar_t[text_len + 1];
		dt_reader.ReadBytes(
			winrt::array_view(reinterpret_cast<uint8_t*>(text), 2 * text_len));
		text[text_len] = L'\0';
		return reinterpret_cast<wchar_t*>(text);
	}

	// �}�`���f�[�^���C�^�[����ǂݍ���.
	ShapeText::ShapeText(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader),
		m_font_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		},
		m_font_family(text_read_text(dt_reader)),
		m_font_size(dt_reader.ReadSingle()),
		m_font_stretch(static_cast<DWRITE_FONT_STRETCH>(dt_reader.ReadUInt32())),
		m_font_style(static_cast<DWRITE_FONT_STYLE>(dt_reader.ReadUInt32())),
		m_font_weight(static_cast<DWRITE_FONT_WEIGHT>(dt_reader.ReadUInt32())),
		m_text(text_read_text(dt_reader)),
		m_text_align_vert(static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(dt_reader.ReadUInt32())),
		m_text_align_horz(static_cast<DWRITE_TEXT_ALIGNMENT>(dt_reader.ReadUInt32())),
		m_text_line_sp(dt_reader.ReadSingle()),
		m_text_pad(D2D1_SIZE_F{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
			})
	{
		// ���̂̐F
		if (m_font_color.r < 0.0f || m_font_color.r > 1.0f ||
			m_font_color.g < 0.0f || m_font_color.g > 1.0f ||
			m_font_color.b < 0.0f || m_font_color.b > 1.0f ||
			m_font_color.a < 0.0f || m_font_color.a > 1.0f) {
			m_font_color = COLOR_BLACK;
		}
		// ���̖�
		is_available_font(m_font_family);
		// ���̂̑傫��
		if (m_font_size < 1.0f || m_font_size > FONT_SIZE_MAX) {
			m_font_size = FONT_SIZE_DEFVAL;
		}
		// ���̂̕�
		if (!(m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_CONDENSED ||
			m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXPANDED ||
			m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_CONDENSED ||
			m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_EXPANDED ||
			m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL ||
			m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_CONDENSED ||
			m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_EXPANDED ||
			m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_CONDENSED ||
			m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_EXPANDED)) {
			m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;
		}
		// ����
		if (!(m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC ||
			m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL ||
			m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE)) {
			m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;
		}
		// ���̂̑���
		if (!(m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_THIN ||
			m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_LIGHT ||
			m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_LIGHT ||
			m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_SEMI_LIGHT ||
			m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL ||
			m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_MEDIUM ||
			m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_DEMI_BOLD ||
			m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BOLD ||
			m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BOLD ||
			m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BLACK ||
			m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BLACK)) {
			m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;
		}
		// �i���̂��낦
		if (!(m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER ||
			m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR ||
			m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR)) {
			m_text_align_vert = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		}
		// ������̂��낦
		if (!(m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER ||
			m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED ||
			m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING ||
			m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING)) {
			m_text_align_horz = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
		}
		// �s��
		if (m_text_line_sp < 0.0f || m_text_line_sp > 127.5f) {
			m_text_line_sp = 0.0f;
		}
		// ������̗]��
		if (m_text_pad.width < 0.0f || m_text_pad.width > 127.5 ||
			m_text_pad.height < 0.0f || m_text_pad.height > 127.5) {
			m_text_pad.width = 0.0f;
			m_text_pad.height = 0.0f;
		}
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapeText::write(DataWriter const& dt_writer) const
	{
		ShapeRect::write(dt_writer);

		dt_writer.WriteSingle(m_font_color.r);
		dt_writer.WriteSingle(m_font_color.g);
		dt_writer.WriteSingle(m_font_color.b);
		dt_writer.WriteSingle(m_font_color.a);

		const uint32_t font_family_len = wchar_len(m_font_family);
		dt_writer.WriteUInt32(font_family_len);
		const auto font_family_data = reinterpret_cast<const uint8_t*>(m_font_family);
		dt_writer.WriteBytes(array_view(font_family_data, font_family_data + 2 * font_family_len));

		dt_writer.WriteSingle(m_font_size);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_stretch));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_style));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_weight));

		const uint32_t text_len = wchar_len(m_text);
		dt_writer.WriteUInt32(text_len);
		const auto text_data = reinterpret_cast<const uint8_t*>(m_text);
		dt_writer.WriteBytes(array_view(text_data, text_data + 2 * text_len));

		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_vert));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_horz));
		dt_writer.WriteSingle(m_text_line_sp);
		dt_writer.WriteSingle(m_text_pad.width);
		dt_writer.WriteSingle(m_text_pad.height);
	}

	// wchar_t �^�̕����� (UTF-16) �� uint32_t �^�̔z��ɕϊ�����.
	std::vector<uint32_t> text_utf16_to_utf32(const wchar_t* w, const size_t w_len) noexcept
	{
		const auto utf16 = winrt::array_view<const wchar_t>(w, w + w_len);
		std::vector<uint32_t> utf32{};
		for (uint32_t i = 0; i < std::size(utf16); i++) {
			// �T���Q�[�g�y�A�̏���
			// ��ʂ��͈͓���
			if (utf16[i] >= 0xD800 && utf16[i] <= 0xDBFF) {
				// ���ʂ��͈͓��Ȃ�
				if (i + 1 < std::size(utf16) && utf16[i + 1] >= 0xDC00 && utf16[i + 1] <= 0xDFFF) {
					// 32 �r�b�g�l�����o���i�[����.
					utf32.push_back(0x10000 + (utf16[i] - 0xD800) * 0x400 + (utf16[i + 1] - 0xDC00));
					i++;
				}
			}
			else if (utf16[i] < 0xDC00 || utf16[i] > 0xDFFF) {
				utf32.push_back(utf16[i]);
			}
		}
		return utf32;
	}

	// ���ʂ𓾂�.
	// T	������t�H�[�}�b�g�܂��͕����񃌃C�A�E�g�̂����ꂩ.
	template <typename T>
	bool text_get_font_face(
		T* src, const wchar_t* family, const DWRITE_FONT_WEIGHT weight,
		const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_STYLE style, IDWriteFontFace3*& face)
		noexcept
	{
		bool ret = false;

		// ���������������.
		IDWriteFontCollection* coll = nullptr;
		if (src->GetFontCollection(&coll) == S_OK) {
			// �}�`�ƈ�v���鏑�̃t�@�~���𓾂�.
			IDWriteFontFamily* fam = nullptr;
			UINT32 index;
			BOOL exists;
			if (coll->FindFamilyName(family, &index, &exists) == S_OK &&
				exists &&
				coll->GetFontFamily(index, &fam) == S_OK) {
				// ���̃t�@�~������, �����ƕ�, ���̂���v���鏑�̂𓾂�.
				IDWriteFont* font = nullptr;
				if (fam->GetFirstMatchingFont(weight, stretch, style, &font) == S_OK) {
					IDWriteFontFaceReference* ref = nullptr;
					if (static_cast<IDWriteFont3*>(font)->GetFontFaceReference(&ref) == S_OK) {
						face = nullptr;
						if (ref->CreateFontFace(&face) == S_OK) {
							ret = true;
						}
						ref->Release();
					}
					font->Release();
				}
				fam->Release();
			}
			coll->Release();
		}
		return ret;
	}
	template bool text_get_font_face<IDWriteTextFormat>(
		IDWriteTextFormat* t, const wchar_t* family, const DWRITE_FONT_WEIGHT weight,
		const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_STYLE style, IDWriteFontFace3*& face)
		noexcept;
	template bool text_get_font_face<IDWriteTextLayout>(
		IDWriteTextLayout* t, const wchar_t* family, const DWRITE_FONT_WEIGHT weight,
		const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_STYLE style, IDWriteFontFace3*& face)
		noexcept;
}