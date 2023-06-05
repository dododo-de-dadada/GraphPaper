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
	wchar_t** ShapeText::s_available_fonts = nullptr;	//�L���ȏ��̖�
	D2D1_COLOR_F ShapeText::s_text_selected_background{ COLOR_ACCENT };	// �����͈͂̔w�i�F
	D2D1_COLOR_F ShapeText::s_text_selected_foreground{ COLOR_TEXT_RANGE };	// �����͈͂̕����F

	// �q�b�g�e�X�g�̌v�ʂ��쐬����.
	static HRESULT text_create_test_metrics(IDWriteTextLayout* text_lay, const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt) noexcept;
	// �q�b�g�e�X�g�̌v��, �s�̌v��, ������I���̌v�ʂ��쐬����.
	//static HRESULT text_create_text_metrics(IDWriteTextLayout* text_lay, const uint32_t text_len, UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& line_cnt, DWRITE_LINE_METRICS*& line_met, UINT32& sele_cnt, DWRITE_HIT_TEST_METRICS*& sele_met, const DWRITE_TEXT_RANGE& sele_rng) noexcept;
	// ���̂̌v�ʂ𓾂�.
	static HRESULT text_get_font_metrics(IDWriteTextLayout* text_lay, DWRITE_FONT_METRICS* font_met) noexcept;

	//------------------------------
	// �q�b�g�e�X�g�̌v�ʂ��쐬����.
	// text_lay	�����񃌃C�A�E�g
	// text_rng	�����͈�
	// test_met	�q�b�g�e�X�g�̌v��
	// test_cnt	�v�ʂ̗v�f��
	//------------------------------
	static HRESULT text_create_test_metrics(IDWriteTextLayout* text_lay, const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt) noexcept
	{
		const uint32_t pos = text_rng.startPosition;
		const uint32_t len = text_rng.length;
		DWRITE_HIT_TEST_METRICS test[1];

		// �܂��v�ʂ̗v�f���𓾂�.
		// �ŏ��� HitTestTextRange �֐��Ăяo����, ���s���邱�Ƃ��O��Ȃ̂�, check_hresult ���Ȃ�.
		// �z����m�ۂ���, ���炽�߂Ċ֐����Ăяo��, �v�ʂ𓾂�.
		text_lay->HitTestTextRange(pos, len, 0, 0, test, 1, &test_cnt);
		test_met = new DWRITE_HIT_TEST_METRICS[test_cnt];
		return text_lay->HitTestTextRange(pos, len, 0, 0, test_met, test_cnt, &test_cnt);
	}

	//------------------------------
	// ���̂̌v�ʂ𓾂�
	// text_lay	�e�L�X�g���C�A�E�g
	// font_met ���̂̌v��
	//------------------------------
	static HRESULT text_get_font_metrics(IDWriteTextLayout* text_lay, DWRITE_FONT_METRICS* font_met) noexcept
	{
		HRESULT hr = S_OK;
		// �����񃌃C�A�E�g ---> ���̃��X�g ---> ���̃t�@�~���[ ---> ���̂𓾂�.
		winrt::com_ptr<IDWriteFontCollection> fonts;
		hr = text_lay->GetFontCollection(fonts.put());
		winrt::com_ptr<IDWriteFontFamily> fam;
		if (hr == S_OK) {
			hr = fonts->GetFontFamily(0, fam.put());
		}
		fonts = nullptr;
		winrt::com_ptr<IDWriteFont> font;
		if (hr == S_OK) {
			fam->GetFont(0, font.put());
		}
		fam = nullptr;
		// ���̂̌v�ʂ𓾂�.
		font->GetMetrics(font_met);
		font = nullptr;
		return hr;
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
	//------------------------------
	/*
	static HRESULT text_create_text_metrics(
		IDWriteTextLayout* text_lay,	// �����񃌃C�A�E�g
		const uint32_t text_len,	// ������̒���
		UINT32& test_cnt,	// �q�b�g�e�X�g�̌v�ʂ̗v�f��
		DWRITE_HIT_TEST_METRICS*& test_met,	// �q�b�g�e�X�g�̌v��
		UINT32& line_cnt,	// �q�b�g�e�X�g�̌v�ʂ̗v�f��
		DWRITE_LINE_METRICS*& line_met,	// �s�̌v��
		UINT32& sele_cnt,	// �I�����ꂽ�����͈͂̌v�ʂ̗v�f��
		DWRITE_HIT_TEST_METRICS*& sele_met,	// �I�����ꂽ�����͈͂̌v��
		const DWRITE_TEXT_RANGE& sele_rng	// �I�����ꂽ�����͈�
	) noexcept
	{
		if (text_lay == nullptr) {
			return E_FAIL;
		}
		const DWRITE_TEXT_ALIGNMENT alig_horz = text_lay->GetTextAlignment();
		const DWRITE_PARAGRAPH_ALIGNMENT alig_vert = text_lay->GetParagraphAlignment();
		HRESULT hr = S_OK;
		// �s�̌v�ʂ��쐬����.
		text_lay->GetLineMetrics(nullptr, 0, &line_cnt);
		line_met = new DWRITE_LINE_METRICS[line_cnt];
		hr = text_lay->GetLineMetrics(line_met, line_cnt, &line_cnt);
		// �q�b�g�e�X�g�̌v�ʂ��쐬����.
		if (hr == S_OK) {
			test_met = new DWRITE_HIT_TEST_METRICS[line_cnt];
			hr = text_lay->HitTestTextRange(0, text_len, 0.0f, 0.0f, test_met, line_cnt, &test_cnt);
		}
		// �I�����ꂽ�����͈͂̌v�ʂ��쐬����.
		if (sele_rng.length > 0) {
			if (hr == S_OK) {
				hr = text_create_test_metrics(text_lay, sele_rng, sele_met, sele_cnt);
			}
		}
		return hr;
	}
	*/

	//------------------------------
	// �g�𕶎���ɍ��킹��.
	// �߂�l	�傫�����������ꂽ�Ȃ�ΐ^.
	//------------------------------
	bool ShapeText::fit_frame_to_text(
		const float g_len	// ����̑傫�� (���̒l�� 1 �ȏ�Ȃ�΂��̒l�ɍ��킹��)
	) noexcept
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
		if (!equal(t_box, m_lineto)) {
			D2D1_POINT_2F se;
			pt_add(m_start, t_box, se);
			set_pos_loc(se, LOC_TYPE::LOC_SE, 0.0f, false);
			return true;
		}
		return false;
	}

	//------------------------------
	// �����񃌃C�A�E�g���쐬����.
	//------------------------------
	void ShapeText::create_text_layout(void) noexcept
	{
		IDWriteFactory* const dwrite_factory = Shape::m_dwrite_factory.get();
		HRESULT hr = S_OK;
		bool updated = false;	// �����񃌃C�A�E�g���쐬�܂��͕ύX

		// �����񂪕ύX���ꂽ�Ƃ�, �����񃌃C�A�E�g�͋�ɐݒ肳���̂�, �����񃌃C�A�E�g���쐬����.
		if (m_dwrite_text_layout == nullptr) {

			// ����̃��P�[�����𓾂�.
			wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
			GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);
			const UINT32 text_len = get_text_len();

			// ������t�H�[�}�b�g���쐬����.
			// �����l���Ȃ�ł���, DWRITE_FONT_STRETCH_NORMAL �Ńe�L�X�g�t�H�[�}�b�g�͍쐬����.
			winrt::com_ptr<IDWriteTextFormat> t_format;
			hr = dwrite_factory->CreateTextFormat(m_font_family, static_cast<IDWriteFontCollection*>(nullptr), m_font_weight, m_font_style, m_font_stretch, m_font_size, locale_name,
				t_format.put());

			// ������t�H�[�}�b�g���當���񃌃C�A�E�g���쐬����.
			if (hr == S_OK) {
				const double text_w = std::fabs(m_lineto.x) - 2.0 * m_text_pad.width;
				const double text_h = std::fabs(m_lineto.y) - 2.0 * m_text_pad.height;
				const FLOAT max_w = static_cast<FLOAT>(max(text_w, 0.0));
				const FLOAT max_h = static_cast<FLOAT>(max(text_h, 0.0));
				if (m_text == nullptr) {
					hr = dwrite_factory->CreateTextLayout(L"", 0, t_format.get(), max_w, max_h, m_dwrite_text_layout.put());
				}
				else {
					hr = dwrite_factory->CreateTextLayout(m_text, text_len, t_format.get(), max_w, max_h, m_dwrite_text_layout.put());
				}
			}

			// ������t�H�[�}�b�g��j������.
			t_format = nullptr;

			// �^�C�|�O���t�B��ݒ肷���, fi �Ȃǌ����������������炵��
			winrt::com_ptr<IDWriteTypography> typo;
			if (hr == S_OK) {
				hr = dwrite_factory->CreateTypography(typo.put());
			}
			if (hr == S_OK) {
				DWRITE_FONT_FEATURE feat{};
				feat.nameTag = DWRITE_FONT_FEATURE_TAG::DWRITE_FONT_FEATURE_TAG_STANDARD_LIGATURES;
				feat.parameter = 0;
				hr = typo->AddFontFeature(feat);
			}
			if (hr == S_OK) {
				hr = m_dwrite_text_layout->SetTypography(typo.get(), DWRITE_TEXT_RANGE{0, text_len});
			}
			typo = nullptr;

			winrt::com_ptr<IDWriteTextLayout3> t3;
			if (hr == S_OK && !m_dwrite_text_layout.try_as(t3)) {
				hr = E_FAIL;
			}

			// �����̕�, �����̂��낦, �i���̂��낦�𕶎��񃌃C�A�E�g�Ɋi�[����.
			if (hr == S_OK) {
				hr = t3->SetFontStretch(m_font_stretch, DWRITE_TEXT_RANGE{ 0, text_len });
			}
			if (hr == S_OK) {
				hr = t3->SetTextAlignment(m_text_align_horz);
			}
			if (hr == S_OK) {
				hr = t3->SetParagraphAlignment(m_text_align_vert);
			}
			if (hr == S_OK) {
				hr = t3->SetWordWrapping(m_text_word_wrap);
			}

			// �s�Ԃ𕶎��񃌃C�A�E�g�Ɋi�[����.
			DWRITE_LINE_SPACING new_sp;
			new_sp.leadingBefore = 0.0f;
			new_sp.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
			if (hr == S_OK && m_dwrite_font_metrics.designUnitsPerEm == 0) {
				hr = text_get_font_metrics(t3.get(), &m_dwrite_font_metrics);
			}
			// �s�Ԃ��[�����傫���Ȃ�, ���̒l��ݒ肷��.
			if (m_text_line_sp >= FLT_MIN) {
				new_sp.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
				new_sp.height = m_font_size + m_text_line_sp;
				if (m_dwrite_font_metrics.designUnitsPerEm == 0) {
					new_sp.baseline = m_font_size + m_text_line_sp;
				}
				else {
					const float descent = m_font_size * m_dwrite_font_metrics.descent / m_dwrite_font_metrics.designUnitsPerEm;
					new_sp.baseline = m_font_size + m_text_line_sp - descent;
				}
			}
			// �s�Ԃ��[���Ȃ�, ����l��ݒ肷��.
			else {
				new_sp.method = DWRITE_LINE_SPACING_METHOD_DEFAULT;
				new_sp.height = 0.0f;
				new_sp.baseline = 0.0f;
			}
			if (hr == S_OK) {
				hr = t3->SetLineSpacing(&new_sp);
			}
			t3 = nullptr;

			if (hr == S_OK) {
				updated = true;
			}
		}

		// �ύX.
		else {

			winrt::com_ptr<IDWriteTextLayout3> t3;
			if (!m_dwrite_text_layout.try_as(t3)) {
				hr = E_FAIL;
			}
			const DWRITE_TEXT_RANGE range{ 0, get_text_len() };
			// ���̖����ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
			const UINT32 name_len = t3->GetFontFamilyNameLength();
			std::vector<WCHAR> f_family(name_len + 1);
			if (hr == S_OK) {
				hr = t3->GetFontFamilyName(f_family.data(), name_len + 1);
			}
			if (hr == S_OK && wcscmp(f_family.data(), m_font_family) != 0) {
				hr = t3->SetFontFamilyName(m_font_family, range);
				if (hr == S_OK) {
					hr = text_get_font_metrics(t3.get(), &m_dwrite_font_metrics);
				}
				if (hr == S_OK) {
					updated = true;
				}
			}
			f_family.resize(0);
			f_family.shrink_to_fit();

			// ���̂̑傫�����ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
			FLOAT f_size = 0.0f;
			if (hr == S_OK) {
				hr = t3->GetFontSize(0, &f_size);
			}
			if (hr == S_OK && !equal(f_size, m_font_size)) {
				hr = t3->SetFontSize(m_font_size, range);
				if (hr == S_OK) {
					updated = true;
				}
			}

			// ���̂̕����ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
			DWRITE_FONT_STRETCH f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_UNDEFINED;
			if (hr == S_OK) {
				hr = t3->GetFontStretch(0, &f_stretch);
			}
			if (hr == S_OK && !equal(f_stretch, m_font_stretch)) {
				hr = t3->SetFontStretch(m_font_stretch, range);
				if (hr == S_OK) {
					updated = true;
				}
			}

			// ���̂̎��̂��ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
			DWRITE_FONT_STYLE f_style = static_cast<DWRITE_FONT_STYLE>(-1);
			if (hr == S_OK) {
				hr = t3->GetFontStyle(0, &f_style);
			}
			if (hr == S_OK && !equal(f_style, m_font_style)) {
				hr = t3->SetFontStyle(m_font_style, range);
				if (hr == S_OK) {
					updated = true;
				}
			}

			// ���̂̑������ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
			DWRITE_FONT_WEIGHT f_weight = static_cast<DWRITE_FONT_WEIGHT>(-1);
			if (hr == S_OK) {
				hr = t3->GetFontWeight(0, &f_weight);
			}
			if (hr == S_OK && !equal(f_weight, m_font_weight)) {
				hr = t3->SetFontWeight(m_font_weight, range);
				if (hr == S_OK) {
					updated = true;
				}
			}

			// ������̃p�f�B���O���ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
			FLOAT text_w = static_cast<FLOAT>(std::fabs(m_lineto.x) - m_text_pad.width * 2.0);
			if (text_w < 0.0f) {
				text_w = 0.0;
			}
			if (hr == S_OK && !equal(text_w, t3->GetMaxWidth())) {
				hr =  t3->SetMaxWidth(text_w);
				if (hr == S_OK) {
					updated = true;
				}
			}
			FLOAT text_h = static_cast<FLOAT>(std::fabs(m_lineto.y) - m_text_pad.height * 2.0);
			if (text_h < 0.0f) {
				text_h = 0.0f;
			}
			if (hr == S_OK && !equal(text_h, t3->GetMaxHeight())) {
				hr = t3->SetMaxHeight(text_h);
				if (hr == S_OK) {
					updated = true;
				}
			}

			// �i���̂��낦���ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
			const DWRITE_PARAGRAPH_ALIGNMENT p_align = t3->GetParagraphAlignment();
			if (hr == S_OK && !equal(p_align, m_text_align_vert)) {
				hr = t3->SetParagraphAlignment(m_text_align_vert);
				if (hr == S_OK) {
					updated = true;
				}
			}

			// �����̂��낦���ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
			const DWRITE_TEXT_ALIGNMENT t_align = t3->GetTextAlignment();
			if (hr == S_OK && !equal(t_align, m_text_align_horz)) {
				hr = t3->SetTextAlignment(m_text_align_horz);
				if (hr == S_OK) {
					updated = true;
				}
			}

			// ������̐܂�Ԃ����ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
			const DWRITE_WORD_WRAPPING w_wrap = t3->GetWordWrapping();
			if (hr == S_OK && !equal(w_wrap, m_text_word_wrap)) {
				hr = t3->SetWordWrapping(m_text_word_wrap);
				if (hr == S_OK) {
					updated = true;
				}
			}

			// �s�Ԃ��ύX���ꂽ�Ȃ當���񃌃C�A�E�g�Ɋi�[����.
			DWRITE_LINE_SPACING new_sp;
			new_sp.leadingBefore = 0.0f;
			new_sp.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
			// �s�Ԃ��[�����傫���Ȃ�,�@���̒l��ݒ肷��.
			if (m_text_line_sp >= FLT_MIN) {
				if (hr == S_OK && m_dwrite_font_metrics.designUnitsPerEm == 0) {
					hr = text_get_font_metrics(t3.get(), &m_dwrite_font_metrics);
				}
				new_sp.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
				new_sp.height = m_font_size + m_text_line_sp;
				if (m_dwrite_font_metrics.designUnitsPerEm == 0) {
					new_sp.baseline = m_font_size + m_text_line_sp;
				}
				else {
					const float descent = m_font_size * m_dwrite_font_metrics.descent / m_dwrite_font_metrics.designUnitsPerEm;
					new_sp.baseline = m_font_size + m_text_line_sp - descent;
				}
			}
			// �s�Ԃ��[���Ȃ�, ����l��ݒ肷��.
			else {
				new_sp.method = DWRITE_LINE_SPACING_METHOD_DEFAULT;
				new_sp.height = 0.0f;
				new_sp.baseline = 0.0f;
			}
			DWRITE_LINE_SPACING old_sp{};
			if (hr == S_OK) {
				hr = t3->GetLineSpacing(&old_sp);
			}
			if (hr == S_OK && (!equal(old_sp.baseline, new_sp.baseline) ||
				old_sp.fontLineGapUsage != new_sp.fontLineGapUsage ||
				!equal(old_sp.height, new_sp.height) ||
				!equal(old_sp.leadingBefore, new_sp.leadingBefore) ||
				old_sp.method != new_sp.method)) {
				hr = t3->SetLineSpacing(&new_sp);
			}

			if (hr == S_OK) {
				updated = true;
			}
		}
		if (hr == S_OK && updated) {
			relese_metrics();

			// �s�̌v�ʂ��쐬����.
			if (hr == S_OK) {
				// �s�̌v�ʂ̌��𓾂�, ���������m�ۂ�, ���߂Čv�ʂ𓾂�.
				DWRITE_LINE_METRICS dummy;	// nullptr ���ƃn���O�A�b�v�����������Ƃ�����̂�, �_�~�[���g��.
				m_dwrite_text_layout->GetLineMetrics(&dummy, 1, &m_dwrite_line_cnt);
				m_dwrite_line_metrics = new DWRITE_LINE_METRICS[m_dwrite_line_cnt];
				hr = m_dwrite_text_layout->GetLineMetrics(m_dwrite_line_metrics, m_dwrite_line_cnt, &m_dwrite_line_cnt);
			}

			// �q�b�g�e�X�g�̌v�ʂ��쐬����.
			if (hr == S_OK) {
				// �q�b�g�e�X�g�̌v�ʂ̌���, �s�̌v�ʂ̌��𒴂��邱�Ƃ͂Ȃ� (�͂�).
				// �������� (���s) �����̂Ƃ�, �t�͂��肤��.
				m_dwrite_test_metrics = new DWRITE_HIT_TEST_METRICS[m_dwrite_line_cnt];
				hr = m_dwrite_text_layout->HitTestTextRange(0, get_text_len(), 0.0f, 0.0f, m_dwrite_test_metrics, m_dwrite_line_cnt, &m_dwrite_test_cnt);
			}

			// �q�b�g�e�X�g�̌v�ʂ͕����񖖔��̉��s�̓g���~���O���Ă��܂��̂�,
			// �K�v�Ȃ�, �s�̌v�ʂ����Ƃ�, �g���~���O���ꂽ�v�ʂ�₤.
			if (hr == S_OK && m_dwrite_test_cnt < m_dwrite_line_cnt) {
				float last_top;	// �ŏI�s�̏�[
				float last_left;	// �ŏI�s�̍��[
				uint32_t last_pos;	// �ŏI�s�̕����ʒu
				// �q�b�g�e�X�g�̌v�ʂ�����Ȃ�, ���̍ŏI�s����v�Z����.
				if (m_dwrite_test_cnt > 0) {
					last_top = m_dwrite_test_metrics[m_dwrite_test_cnt - 1].top + m_dwrite_test_metrics[m_dwrite_test_cnt - 1].height;
					last_pos = m_dwrite_test_metrics[m_dwrite_test_cnt - 1].textPosition + m_dwrite_line_metrics[m_dwrite_test_cnt - 1].length;
				}
				// �q�b�g�e�X�g�̌v�ʂ��Ȃ����, �s�̌v�ʂ���v�Z����.
				else {
					if (m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR) {
						last_top = 0.0f;
						last_pos = 0;
						for (uint32_t i = 0; i + 1 < m_dwrite_line_cnt; i++) {
							last_top = last_top + m_dwrite_line_metrics[i - 1].height;
							last_pos = last_pos + m_dwrite_line_metrics[i - 1].length;
						}
						last_top = last_top + m_font_size;	// �ŏI�s�̍����͏��̂̑傫��
						last_pos = last_pos + m_dwrite_line_metrics[m_dwrite_line_cnt - 1].length;
						last_top = fabs(m_lineto.y) - min(fabs(m_lineto.y), 2.0f * m_text_pad.height) - last_top;
					}
					else if (m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER) {
						last_top = 0.0f;
						last_pos = 0;
						for (uint32_t i = 0; i + 1 < m_dwrite_line_cnt; i++) {
							last_top = last_top + m_dwrite_line_metrics[i].height;
							last_pos = last_pos + m_dwrite_line_metrics[i].length;
						}
						last_top = 0.5 * (last_top + m_font_size);	// �ŏI�s�̍����͏��̂̑傫��
						last_pos = last_pos + m_dwrite_line_metrics[m_dwrite_line_cnt - 1].length;
					}
					else {
						last_top = 0.0f;
						last_pos = 0;
						for (uint32_t i = 0; i < m_dwrite_line_cnt; i++) {
							last_pos = last_pos + m_dwrite_line_metrics[i - 1].length;
						}
					}
				}
				// �i���̂��낦���E�悹�Ȃ�, �ŏI�s�̍��[�ɂ͕����񃌃C�A�E�g�̕����i�[����.
				if (m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING) {
					//last_left = fabs(m_pos.x) - min(fabs(m_pos.x), 2.0f * m_text_pad.width);
					last_left = m_dwrite_text_layout->GetMaxWidth();
				}
				// �i���̂��낦�������܂��͋ϓ��Ȃ�, �ŏI�s�̍��[�ɂ͕����񃌃C�A�E�g�̕��̔������i�[����.
				else if (m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER ||
					m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED) {
					//last_left = 0.5f * fabs(m_pos.x);
					last_left = m_dwrite_text_layout->GetMaxWidth() * 0.5f;
				}
				// �i���̂��낦������ȊO�Ȃ�, �ŏI�s�̍��[�ɂ� 0 ���i�[����.
				else {
					last_left = 0.0;
				}
				for (uint32_t i = m_dwrite_test_cnt; i < m_dwrite_line_cnt; i++) {
					m_dwrite_test_metrics[i].top = last_top;
					m_dwrite_test_metrics[i].width = 0.0f;
					m_dwrite_test_metrics[i].height = m_dwrite_line_metrics[i].height;
					m_dwrite_test_metrics[i].left = last_left;
					m_dwrite_test_metrics[i].length = m_dwrite_line_metrics[i].length;
					m_dwrite_test_metrics[i].textPosition = last_pos;
					last_top = last_top + m_dwrite_line_metrics[i].height;
					last_pos = last_pos + m_dwrite_line_metrics[i].length;
				}
				m_dwrite_test_cnt = m_dwrite_line_cnt;
			}
		}
		else if (hr == S_OK) {
			// �����l�̕ύX���Ȃ��Ă�, �I�����ꂽ�����͈͂��ύX���ꂽ�Ȃ�,
			//const auto end = m_select_trail ? m_select_end + 1 : m_select_end;
			//const auto s = min(m_select_start, end);
			//const auto e = max(m_select_start, end);
			//if (m_dwrite_selected_cnt == 0) {
			//	if (m_select_start != end) {
			//		updated = true;
			//	}
			//}
			//else {
			//	uint32_t s_len = 0;	// �I��͈͂̒���
			//	for (uint32_t i = 0; i < m_dwrite_selected_cnt; i++) {
			//		s_len += m_dwrite_selected_metrics[i].length;
			//	}
			//	if (m_dwrite_selected_metrics[0].textPosition != s || s_len != e - s) {
			//		updated = true;
			//	}
			//}
			//if (updated) {
			//	m_dwrite_text_layout->HitTestTextRange(s, e - s, 0.0f, 0.0f, nullptr, 0, &m_dwrite_selected_cnt);
			//	m_dwrite_selected_metrics = new DWRITE_HIT_TEST_METRICS[m_dwrite_selected_cnt];
			//	hr = m_dwrite_text_layout->HitTestTextRange(s, e - s, 0.0f, 0.0f, m_dwrite_selected_metrics, m_dwrite_selected_cnt, &m_dwrite_selected_cnt);
			//}
		}
	}

	// �I��͈͂��ꂽ�������\������.
	void ShapeText::draw_selection(const uint32_t sele_start, const uint32_t sele_end, const bool sele_trailing) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const color_brush = Shape::m_d2d_color_brush.get();
		ID2D1SolidColorBrush* const range_brush = Shape::m_d2d_range_brush.get();

		// �]���������킦��, ������̍���ʒu���v�Z����.
		const double h = min(m_text_pad.width, fabs(m_lineto.x) * 0.5f);
		const double v = min(m_text_pad.height, fabs(m_lineto.y) * 0.5f);
		D2D1_POINT_2F t_lt{
			m_lineto.x < 0.0f ? h + m_start.x + m_lineto.x : h + m_start.x,
			m_lineto.y < 0.0f ? v + m_start.y + m_lineto.y : v + m_start.y
		};

		// �I��͈͂�����ΑI��͈͂̌v�ʂ𓾂�.
		const auto len = get_text_len();
		const auto end = sele_trailing ? sele_end + 1 : sele_end;
		const auto s = min(min(sele_start, end), len);
		const auto e = min(max(sele_start, end), len);
		UINT32 sele_cnt = 0;
		DWRITE_HIT_TEST_METRICS* sele_met = nullptr;
		HRESULT hr = S_OK;
		if (hr == S_OK && s < e) {
			m_dwrite_text_layout->HitTestTextRange(s, e - s, 0.0f, 0.0f, nullptr, 0, &sele_cnt);
			sele_met = new DWRITE_HIT_TEST_METRICS[sele_cnt];
			hr = m_dwrite_text_layout->HitTestTextRange(s, e - s, 0.0f, 0.0f, sele_met, sele_cnt, &sele_cnt);
		}

		// �I��͈͂̌v�ʂ����Ƃɔ͈͂�h��Ԃ�.
		if (hr == S_OK && s < e) {
			if (m_dwrite_font_metrics.designUnitsPerEm == 0) {
				text_get_font_metrics(m_dwrite_text_layout.get(), &m_dwrite_font_metrics);
			}
			const float descent = (m_dwrite_font_metrics.designUnitsPerEm == 0 ? 0.0f : m_font_size * m_dwrite_font_metrics.descent / m_dwrite_font_metrics.designUnitsPerEm);
			const uint32_t rc = sele_cnt;
			const uint32_t tc = m_dwrite_test_cnt;
			for (uint32_t i = 0; i < rc; i++) {
				for (uint32_t j = 0; j < tc; j++) {
					const DWRITE_HIT_TEST_METRICS& tm = m_dwrite_test_metrics[j];
					const DWRITE_LINE_METRICS& lm = m_dwrite_line_metrics[j];
					if (tm.textPosition <= sele_met[i].textPosition &&
						sele_met[i].textPosition + sele_met[i].length <= tm.textPosition + tm.length) {
						D2D1_RECT_F sele_rect;
						sele_rect.left = t_lt.x + sele_met[i].left;
						sele_rect.top = static_cast<FLOAT>(t_lt.y + tm.top + lm.baseline + descent - m_font_size);
						if (sele_met[i].width < FLT_MIN) {
							const float sp_len = max(lm.trailingWhitespaceLength * m_font_size * 0.25f, 1.0f);
							sele_rect.right = sele_rect.left + sp_len;
						}
						else {
							sele_rect.right = sele_rect.left + sele_met[i].width;
						}
						sele_rect.bottom = sele_rect.top + m_font_size;
						color_brush->SetColor(ShapeText::s_text_selected_foreground);
						target->DrawRectangle(sele_rect, color_brush, 2.0, nullptr);
						color_brush->SetColor(ShapeText::s_text_selected_background);
						target->FillRectangle(sele_rect, color_brush);
						break;
					}
				}
			}
			delete[] sele_met;
		}

		// ������S�̂𓧖���
		if (hr == S_OK && s < e) {
			constexpr D2D1_COLOR_F TRANSPALENT{ 0.0f, 0.0f, 0.0f, 0.0f };
			color_brush->SetColor(TRANSPALENT);
			hr = m_dwrite_text_layout->SetDrawingEffect(color_brush, DWRITE_TEXT_RANGE{ 0, len });
		}
		// �I��͈͂����O�i�F��ݒ肷��.
		if (hr == S_OK && s < e) {
			DWRITE_TEXT_RANGE ran{
				static_cast<uint32_t>(s), static_cast<uint32_t>(e - s)
			};
			range_brush->SetColor(ShapeText::s_text_selected_foreground);
			hr = m_dwrite_text_layout->SetDrawingEffect(range_brush, ran);
		}
		// �������\����������, ��������߂�
		if (hr == S_OK && s < e) {
			target->DrawTextLayout(t_lt, m_dwrite_text_layout.get(), color_brush);
			color_brush->SetColor(m_font_color);
			hr = m_dwrite_text_layout->SetDrawingEffect(color_brush, DWRITE_TEXT_RANGE{ 0, len });
		}
	}

	// �}�`��\������.
	void ShapeText::draw(void) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const color_brush = Shape::m_d2d_color_brush.get();
		ID2D1SolidColorBrush* const range_brush = Shape::m_d2d_range_brush.get();
		ID2D1Factory* factory;
		target->GetFactory(&factory);

		// ���`��`��.
		ShapeRect::draw();

		create_text_layout();

		// �]���������킦��, ������̍���ʒu���v�Z����.
		const double h = min(m_text_pad.width, fabs(m_lineto.x) * 0.5f);
		const double v = min(m_text_pad.height, fabs(m_lineto.y) * 0.5f);
		D2D1_POINT_2F t_lt{
			m_lineto.x < 0.0f ? h + m_start.x + m_lineto.x : h + m_start.x,
			m_lineto.y < 0.0f ? v + m_start.y + m_lineto.y : v + m_start.y
		};

		HRESULT hr = S_OK;

		// �������\������
		color_brush->SetColor(m_font_color);
		target->DrawTextLayout(t_lt, m_dwrite_text_layout.get(), color_brush);

		// �}�`���I������Ă���Ȃ�, ������̕⏕����\������
		if (m_loc_show && is_selected()) {
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

			D2D1_POINT_2F p[4];
			D2D1_STROKE_STYLE_PROPERTIES1 s_prop{ AUXILIARY_SEG_STYLE };
			const float descent = (m_dwrite_font_metrics.designUnitsPerEm == 0 ? 0.0f : m_font_size * m_dwrite_font_metrics.descent / m_dwrite_font_metrics.designUnitsPerEm);
			for (uint32_t i = 0; i < m_dwrite_test_cnt; i++) {
				DWRITE_HIT_TEST_METRICS const& tm = m_dwrite_test_metrics[i];
				DWRITE_LINE_METRICS const& lm = m_dwrite_line_metrics[i];
				// �j��������ďd�Ȃ��ĕ\������Ȃ��悤��, �j���̃I�t�Z�b�g���v�Z��,
				// ������̘g��ӂ��Ƃɕ\������.
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
				static_cast<ID2D1Factory1*>(factory)->CreateStrokeStyle(&s_prop, d_arr, d_cnt,
					selected_style.put());
				target->DrawLine(p[0], p[1], color_brush, Shape::m_aux_width, selected_style.get());
				target->DrawLine(p[3], p[2], color_brush, Shape::m_aux_width, selected_style.get());
				selected_style = nullptr;
				s_prop.dashOffset = static_cast<FLOAT>(std::fmod(p[0].y, mod));
				static_cast<ID2D1Factory1*>(factory)->CreateStrokeStyle(&s_prop, d_arr, d_cnt,
					selected_style.put());
				target->DrawLine(p[1], p[2], color_brush, Shape::m_aux_width, selected_style.get());
				target->DrawLine(p[0], p[3], color_brush, Shape::m_aux_width, selected_style.get());
				selected_style = nullptr;
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
	//bool ShapeText::get_text_selected(DWRITE_TEXT_RANGE& val) const noexcept
	//{
	//	val = m_text_selected_range;
	//	return true;
	//}

	// �}�`���_���܂ނ����肷��.
	// test_pt	���肳���_
	// �߂�l	�_���܂ޕ���
	uint32_t ShapeText::hit_test(const D2D1_POINT_2F test_pt, const bool/*ctrl_key*/) const noexcept
	{
		const uint32_t loc = rect_loc_hit_test(m_start, m_lineto, test_pt, m_loc_width);
		if (loc != LOC_TYPE::LOC_SHEET) {
			return loc;
		}
		const float descent = m_dwrite_font_metrics.designUnitsPerEm == 0 ?
			0.0f : 
			(m_font_size * m_dwrite_font_metrics.descent / m_dwrite_font_metrics.designUnitsPerEm);

		// ������̋�`�̍���_�ƉE���� Y �l�𓾂�.
		const float h = fabs(m_lineto.x) * 0.5f;
		const float v = fabs(m_lineto.y) * 0.5f;
		const float f = m_font_size * 0.5f;
		float left = (m_lineto.x < 0.0f ? m_start.x + m_lineto.x : m_start.x) + min(m_text_pad.width, h);
		float right = (m_lineto.x < 0.0f ? m_start.x : m_start.x + m_lineto.x) - min(m_text_pad.width, h);
		const float top = (m_lineto.y < 0.0f ? m_start.y + m_lineto.y : m_start.y) + min(m_text_pad.height, v);

		float tt = 0.0f;
		for (uint32_t i = 0; i < m_dwrite_test_cnt; i++) {
			const auto tl = m_dwrite_test_metrics[i].left;
			const auto tw = m_dwrite_test_metrics[i].width;
			tt = m_dwrite_test_metrics[i].top;
			const auto bl = m_dwrite_line_metrics[i].baseline;
			//const D2D1_POINT_2F v{	// �s�̍���_
			//	max(left, left + tl - f), top + tt + bl + descent - m_font_size
			//};
			//const D2D1_POINT_2F w{	// �s�̉E���_
			//	min(right, left + tl + tw + f), top + tt + bl + descent
			//};
			const D2D1_POINT_2F v{	// �s�̍���_
				min(left, left + tl - f), top + tt + bl + descent - m_font_size
			};
			const D2D1_POINT_2F w{	// �s�̉E���_
				max(right, left + tl + tw + f), top + tt + bl + descent
			};
			if (pt_in_rect(test_pt, v, w)) {
				return LOC_TYPE::LOC_TEXT;
			}
		}
		/*
		float line_x;
		if (m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER ||
			m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED) {
			left = max(left, (right - left) * 0.5f - f);
			right = left + m_font_size;
		}
		else if (m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING) {
			left = max(left, right - f);
			right = left + m_font_size;
		}
		else {
			left = max(left, left - f);
			right = left + m_font_size;
		}
		for (uint32_t i = m_dwrite_test_cnt; i < m_dwrite_line_cnt; i++) {
			tt += m_dwrite_line_metrics[i].height;
			const auto bl = m_dwrite_line_metrics[i].baseline;
			const D2D1_POINT_2F v{	// �s�̍���_
				left, top + tt + bl + descent - m_font_size
			};
			const D2D1_POINT_2F w{	// �s�̉E���_
				right, top + tt + bl + descent
			};
			if (pt_in_rect(pt, v, w)) {
				return LOC_TYPE::LOC_TEXT;
			}
		}
		*/
		return ShapeRect::hit_test(test_pt);
	}

	// ��`�Ɋ܂܂�邩���肷��.
	// lt	��`�̍���ʒu
	// rb	��`�̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeText::is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		D2D1_POINT_2F p_lt;	// ����ʒu

		if (m_dwrite_test_cnt > 0 && m_dwrite_test_cnt < UINT32_MAX) {
			const float descent = 
				m_dwrite_font_metrics.designUnitsPerEm == 0 ? 0.0f :
				(m_font_size * m_dwrite_font_metrics.descent / m_dwrite_font_metrics.designUnitsPerEm);

			// ������̊e�s����`�Ɋ܂܂�锻�肷��.
			ShapeRect::get_bbox_lt(p_lt);
			for (uint32_t i = 0; i < m_dwrite_test_cnt; i++) {
				const auto tl = m_dwrite_test_metrics[i].left;
				const auto tt = m_dwrite_test_metrics[i].top;
				const auto tw = m_dwrite_test_metrics[i].width;
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
		return ShapeRect::is_inside(lt, rb);
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
	// ����̒n��E���ꖼ�ɑΉ��������̖��𓾂�, ������z��Ɋi�[����.
	// 'en-us' �� GetUserDefaultLocaleName �œ�����, 2 �̃��P�[�����ꂼ��̏��̖��𓾂�.
	// ���̂悤�Ɋi�[�����.
	// "�l�r �S�V�b�N\0MS Gothic\0"
	void ShapeText::set_available_fonts(void) noexcept
	{
		HRESULT hr = S_OK;

		//s_d2d_factory = d2d.m_d2d_factory.get();
		// ����̒n��E���ꖼ�𓾂�.
		wchar_t lang[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(lang, LOCALE_NAME_MAX_LENGTH);

		// �V�X�e�������t�H���g�W���� DWriteFactory ���瓾��.
		winrt::com_ptr<IDWriteFontCollection> collection;

		hr = Shape::m_dwrite_factory->GetSystemFontCollection(collection.put());

		// �t�H���g�W���̗v�f���𓾂�.
		const uint32_t f_cnt = collection->GetFontFamilyCount();

		// ����ꂽ�v�f�� + 1 �̔z����m�ۂ���.
		s_available_fonts = new wchar_t* [static_cast<size_t>(f_cnt) + 1];

		// �t�H���g�W���̊e�v�f�ɂ���.
		for (uint32_t i = 0; hr == S_OK && i < f_cnt; i++) {

			// �v�f���珑�̂𓾂�.
			winrt::com_ptr<IDWriteFontFamily> font_family;
			if (hr == S_OK) {
				hr = collection->GetFontFamily(i, font_family.put());
			}

			// ���̂��烍�[�J���C�Y���ꂽ���̖��𓾂�.
			winrt::com_ptr<IDWriteLocalizedStrings> loc_name;
			if (hr == S_OK) {
				hr = font_family->GetFamilyNames(loc_name.put());
			}

			// ���[�J���C�Y���ꂽ���̖��̈ʒu�𓾂�.
			UINT32 index_en_us = 0;
			UINT32 index_local = 0;
			BOOL exists = false;
			if (hr == S_OK) {
				hr = loc_name->FindLocaleName(L"en-us", &index_en_us, &exists);
			}
			if (exists != TRUE) {
				// en-us ���Ȃ��ꍇ,
				// 0 ���ʒu�Ɋi�[����.
				index_en_us = 0;
			}
			if (hr == S_OK) {
				hr = loc_name->FindLocaleName(lang, &index_local, &exists);
			}
			if (exists != TRUE) {
				// �n�於���Ȃ��ꍇ,
				// en_us ���J�n�ʒu�Ɋi�[����.
				index_local = index_en_us;
			}

			// �J�n�ʒu�����̕������𓾂� (�k�������͊܂܂�Ȃ�).
			UINT32 length_en_us = 0;
			UINT32 length_local = 0;
			if (hr == S_OK) {
				hr = loc_name->GetStringLength(index_en_us, &length_en_us);
			}
			if (hr == S_OK) {
				hr = loc_name->GetStringLength(index_local, &length_local);
			}

			// ������ + 1 �̕����z����m�ۂ�, ���̖��̔z��Ɋi�[����.
			s_available_fonts[i] = new wchar_t[static_cast<size_t>(length_en_us) + 1 + static_cast<size_t>(length_local) + 1];
			if (hr == S_OK) {
				hr = loc_name->GetString(index_en_us, s_available_fonts[i], length_en_us + 1);
			}
			if (hr == S_OK) {
				hr = loc_name->GetString(index_local, s_available_fonts[i] + static_cast<size_t>(length_en_us) + 1, length_local + 1);
			}

			// ���[�J���C�Y���ꂽ���̖��Ə��̂�j������.
			loc_name = nullptr;
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
		if (!equal(static_cast<const wchar_t*>(m_text), val)) {
			m_text = val;
			m_text_len = wchar_len(val);
			m_dwrite_text_layout = nullptr;
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
	//bool ShapeText::set_text_selected(const DWRITE_TEXT_RANGE val) noexcept
	//{
	//	if (!equal(m_text_selected_range, val)) {
	//		m_text_selected_range = val;
	//		return true;
	//	}
	//	return false;
	//}

	// �}�`���쐬����.
	// start	�n�_
	// pos	�I�_�ւ̈ʒu�x�N�g��
	// text	������
	// prop	����
	ShapeText::ShapeText(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, wchar_t* const text, const Shape* prop) :
		ShapeRect::ShapeRect(start, pos, prop)
	{
		prop->get_font_color(m_font_color);
		prop->get_font_family(m_font_family);
		prop->get_font_size(m_font_size);
		prop->get_font_stretch(m_font_stretch);
		prop->get_font_style(m_font_style);
		prop->get_font_weight(m_font_weight);
		prop->get_text_line_sp(m_text_line_sp);
		prop->get_text_pad(m_text_pad),
		prop->get_text_align_horz(m_text_align_horz);
		prop->get_text_align_vert(m_text_align_vert);
		m_text_len = wchar_len(text);
		m_text = text;

		ShapeText::is_available_font(m_font_family);
	}

	bool ShapeText::get_font_face(IDWriteFontFace3*& face) const noexcept
	{
		return text_get_font_face(m_dwrite_text_layout.get(), m_font_family, m_font_weight, m_font_stretch, m_font_style, face);
	}

	static wchar_t* text_read_text(DataReader const& dt_reader)
	{
		const auto text_len{ dt_reader.ReadUInt32() };
		wchar_t* text = new wchar_t[text_len + 1];
		dt_reader.ReadBytes(winrt::array_view(reinterpret_cast<uint8_t*>(text), 2 * text_len));
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
		m_font_family([](DataReader const& dt_reader, const uint32_t text_len)->wchar_t* {
			wchar_t* text = new wchar_t[text_len + 1];
			if (text != nullptr) {
				dt_reader.ReadBytes(winrt::array_view(reinterpret_cast<uint8_t*>(text), 2 * text_len));
				text[text_len] = L'\0';
			}
			return text;
		}(dt_reader, dt_reader.ReadUInt32())),
		m_font_size(dt_reader.ReadSingle()),
		m_font_stretch(static_cast<DWRITE_FONT_STRETCH>(dt_reader.ReadUInt32())),
		m_font_style(static_cast<DWRITE_FONT_STYLE>(dt_reader.ReadUInt32())),
		m_font_weight(static_cast<DWRITE_FONT_WEIGHT>(dt_reader.ReadUInt32())),
		m_text_len(dt_reader.ReadUInt32()),
		m_text([](DataReader const& dt_reader, const uint32_t text_len)->wchar_t* {
			wchar_t* text = new wchar_t[text_len + 1];
			if (text != nullptr) {
				dt_reader.ReadBytes(winrt::array_view(reinterpret_cast<uint8_t*>(text), 2 * text_len));
				text[text_len] = L'\0';
			}
			return text;
		}(dt_reader, m_text_len)),
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
			m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_SEMI_BOLD ||
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

		const uint32_t f_len = wchar_len(m_font_family);
		dt_writer.WriteUInt32(f_len);
		const auto f_data = reinterpret_cast<const uint8_t*>(m_font_family);
		dt_writer.WriteBytes(array_view(f_data, f_data + 2 * static_cast<size_t>(f_len)));

		dt_writer.WriteSingle(m_font_size);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_stretch));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_style));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_weight));

		const uint32_t t_len = get_text_len();
		dt_writer.WriteUInt32(t_len);
		const auto t_data = reinterpret_cast<const uint8_t*>(m_text);
		dt_writer.WriteBytes(array_view(t_data, t_data + 2 * static_cast<size_t>(t_len)));

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
	bool text_get_font_face(T* src, const wchar_t* family, const DWRITE_FONT_WEIGHT weight, const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_STYLE style, IDWriteFontFace3*& face) noexcept
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
		IDWriteTextFormat* src, const wchar_t* family, const DWRITE_FONT_WEIGHT weight,
		const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_STYLE style, IDWriteFontFace3*& face)
		noexcept;
	template bool text_get_font_face<IDWriteTextLayout>(
		IDWriteTextLayout* src, const wchar_t* family, const DWRITE_FONT_WEIGHT weight,
		const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_STYLE style, IDWriteFontFace3*& face)
		noexcept;
}