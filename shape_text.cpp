#include <cwctype> 
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	wchar_t** ShapeText::s_available_fonts = nullptr;	//���p�\�ȏ��̖�

	//	�e�L�X�g���C�A�E�g����q�b�g�e�X�g�̂��߂̌v�ʂ̔z��𓾂�.
	static void create_test_metrics(IDWriteTextLayout* t_lay, const DWRITE_TEXT_RANGE t_rng, DWRITE_HIT_TEST_METRICS*& t_met, UINT32& m_cnt);
	//	��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	static void write_svg_text(const wchar_t* t, const uint32_t t_len, const double x, const double y, const double dy, DataWriter const& dt_writer);

	//	�e�L�X�g���C�A�E�g����q�b�g�e�X�g�̂��߂̌v�ʂ̔z��𓾂�.
	//	t_lay	���ƂɂȂ�e�L�X�g���C�A�E�g
	//	t_rng	������͈̔�
	//	t_met	����ꂽ�v�ʂ̔z��
	//	m_cnt	�����z��̗v�f��
	static void create_test_metrics(IDWriteTextLayout* t_lay, const DWRITE_TEXT_RANGE t_rng, DWRITE_HIT_TEST_METRICS*& t_met, UINT32& m_cnt)
	{
		const uint32_t pos = t_rng.startPosition;
		const uint32_t len = t_rng.length;
		DWRITE_HIT_TEST_METRICS test[1];

		//	���s���邱�Ƃ��O��Ȃ̂�, �ŏ��� HitTestTextRange �֐�
		//	�Ăяo����, check_hresult ���Ȃ�.
		t_lay->HitTestTextRange(pos, len, 0, 0, test, 1, &m_cnt);
		//	�z����m�ۂ���, �֐������炽�߂ČĂяo��.
		t_met = new DWRITE_HIT_TEST_METRICS[m_cnt];
		winrt::check_hresult(
			t_lay->HitTestTextRange(pos, len, 0, 0, t_met, m_cnt, &m_cnt)
		);
	}

	//	�}�`��j������.
	ShapeText::~ShapeText(void)
	{
		m_dw_text_layout = nullptr;
		m_dw_hit_linecnt = 0;
		if (m_dw_hit_metrics != nullptr) {
			delete[] m_dw_hit_metrics;
			m_dw_hit_metrics = nullptr;
		}
		m_dw_rng_linecnt = 0;
		if (m_dw_rng_metrics != nullptr) {
			delete[] m_dw_rng_metrics;
			m_dw_rng_metrics = nullptr;
		}
		if (m_font_family) {
			if (is_available_font(m_font_family) == false) {
				delete[] m_font_family;
			}
			m_font_family = nullptr;
		}
		if (m_text != nullptr) {
			delete[] m_text;
			m_text = nullptr;
		}
	}

	// �v�ʂ̔z����e�L�X�g���C�A�E�g���瓾��.
	void ShapeText::create_test_metrics(void)
	{
		using winrt::GraphPaper::implementation::create_test_metrics;

		m_dw_hit_linecnt = 0;
		if (m_dw_hit_metrics != nullptr) {
			delete[] m_dw_hit_metrics;
			m_dw_hit_metrics = nullptr;
		}
		m_dw_rng_linecnt = 0;
		if (m_dw_rng_metrics != nullptr) {
			delete[] m_dw_rng_metrics;
			m_dw_rng_metrics = nullptr;
		}
		if (m_dw_text_layout.get() != nullptr) {
			create_test_metrics(m_dw_text_layout.get(),
				{ 0, wchar_len(m_text) }, m_dw_hit_metrics, m_dw_hit_linecnt);
			if (m_sel_range.length > 0) {
				create_test_metrics(m_dw_text_layout.get(),
					m_sel_range, m_dw_rng_metrics, m_dw_rng_linecnt);
			}
		}
	}

	// �e�L�X�g���C�A�E�g��j�����č쐬����.
	void ShapeText::create_text_layout(void)
	{
		using winrt::GraphPaper::implementation::create_test_metrics;

		m_dw_text_layout = nullptr;;
		m_dw_hit_linecnt = 0;
		if (m_dw_hit_metrics != nullptr) {
			delete[] m_dw_hit_metrics;
			m_dw_hit_metrics = nullptr;
		}
		m_dw_rng_linecnt = 0;
		if (m_dw_rng_metrics != nullptr) {
			delete[] m_dw_rng_metrics;
			m_dw_rng_metrics = nullptr;
		}
		const uint32_t len = wchar_len(m_text);
		if (len == 0) {
			return;
		}
		wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);
		winrt::com_ptr<IDWriteTextFormat> format;
		// CreateTextFormat ��,
		// DWRITE_FONT_STRETCH_UNDEFINED ���w�肳�ꂽ�ꍇ, �G���[�ɂȂ邱�Ƃ�����.
		// �����l���Ȃ�ł���, DWRITE_FONT_STRETCH_NORMAL �Ńe�L�X�g�t�H�[�}�b�g�͍쐬����.
		winrt::check_hresult(
			s_dwrite_factory->CreateTextFormat(
				m_font_family,
				static_cast<IDWriteFontCollection*>(nullptr),
				m_font_weight,
				m_font_style,
				DWRITE_FONT_STRETCH_NORMAL,
				static_cast<FLOAT>(m_font_size),
				locale_name,
				format.put()
			)
		);
		const auto w = static_cast<FLOAT>(max(std::fabsf(m_vec.x) - 2.0 * m_text_mar.width, 0.0));
		const auto h = static_cast<FLOAT>(max(std::fabsf(m_vec.y) - 2.0 * m_text_mar.height, 0.0));
		winrt::check_hresult(
			s_dwrite_factory->CreateTextLayout(
				m_text, len, format.get(),
				w, h, m_dw_text_layout.put())
		);
		format = nullptr;
		winrt::com_ptr<IDWriteTextLayout3> t3;
		if (m_dw_text_layout.try_as(t3)) {
			DWRITE_LINE_SPACING ls;
			if (m_text_line > FLT_MIN) {
				ls.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
				ls.height = static_cast<FLOAT>(m_text_line);
				ls.baseline = ls.height * 0.8f;
				ls.leadingBefore = 0.0f;
				ls.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
			}
			else {
				ls.method = DWRITE_LINE_SPACING_METHOD_DEFAULT;
				ls.height = 0.0f;
				ls.baseline = 0.0f;
				ls.leadingBefore = 0.0f;
				ls.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
			}
			t3->SetLineSpacing(&ls);
		}
		m_dw_text_layout->SetTextAlignment(m_text_align_t);
		m_dw_text_layout->SetParagraphAlignment(m_text_align_p);
		m_dw_text_layout->SetFontStretch(m_font_stretch, { 0, len });
		create_test_metrics(m_dw_text_layout.get(), { 0, len }, m_dw_hit_metrics, m_dw_hit_linecnt);
		create_test_metrics(m_dw_text_layout.get(), m_sel_range, m_dw_rng_metrics, m_dw_rng_linecnt);
	}

	// �v�ʂ�j�����č쐬����.
	void ShapeText::create_text_metrics(void)
	{
		if (m_text != nullptr && m_text[0] != '\0') {
			if (m_dw_text_layout.get() == nullptr) {
				create_text_layout();
			}
			else {
				const FLOAT w = static_cast<FLOAT>(max(std::fabs(m_vec.x) - m_text_mar.width * 2.0, 0.0));
				const FLOAT h = static_cast<FLOAT>(max(std::fabs(m_vec.y) - m_text_mar.height * 2.0, 0.0));
				bool flag = false;
				if (equal(w, m_dw_text_layout->GetMaxWidth()) == false) {
					m_dw_text_layout->SetMaxWidth(w);
					flag = true;
				}
				if (equal(h, m_dw_text_layout->GetMaxHeight()) == false) {
					m_dw_text_layout->SetMaxHeight(h);
					flag = true;
				}
				if (flag) {
					create_test_metrics();
				}
			}
		}
	}

	// �����̋󔒂���菜��.
	void ShapeText::delete_bottom_blank(void) noexcept
	{
		if (m_text == nullptr) {
			return;
		}
		auto i = wcslen(m_text);
		while (i-- > 0) {
			if (iswspace(m_text[i]) == false) {
				break;
			}
			m_text[i] = L'\0';
		}

	}

	// �}�`��\������.
	void ShapeText::draw(SHAPE_DX& dx)
	{
		ShapeRect::draw(dx);
		if (m_dw_text_layout == nullptr) {
			return;
		}
		D2D1_POINT_2F t_min;
		pt_add(m_pos, m_vec, t_min);
		pt_min(m_pos, t_min, t_min);
		auto hm = min(m_text_mar.width, fabs(m_vec.x) * 0.5);
		auto vm = min(m_text_mar.height, fabs(m_vec.y) * 0.5);
		pt_add(t_min, hm, vm, t_min);
		if (m_sel_range.length > 0) {
			// �����͈͂̌v�ʂ�h��Ԃ��ŕ\������.
			const auto br = dx.m_shape_brush.get();
			br->SetColor(dx.m_rng_bcolor);
			const auto cnt = m_dw_rng_linecnt;
			const auto dc = dx.m_d2dContext.get();
			for (uint32_t i = 0; i < cnt; i++) {
				const auto m = m_dw_rng_metrics[i];
				D2D1_RECT_F r;
				r.left = t_min.x + m.left;
				r.top = static_cast<FLOAT>(t_min.y + m.top + m.height - m_font_size);
				r.right = r.left + m.width;
				r.bottom = static_cast<FLOAT>(r.top + m_font_size);
				dc->FillRectangle(r, br);
			}
			dx.m_rng_brush->SetColor(dx.m_rng_tcolor);
			m_dw_text_layout->SetDrawingEffect(dx.m_rng_brush.get(), m_sel_range);
		}
		dx.m_shape_brush->SetColor(m_font_color);
		dx.m_d2dContext->DrawTextLayout(t_min, m_dw_text_layout.get(), dx.m_shape_brush.get());
		if (m_sel_range.length > 0 && m_text != nullptr) {
			m_dw_text_layout->SetDrawingEffect(nullptr, { 0, wchar_len(m_text) });
		}
		if (is_selected() == false) {
			return;
		}
		// ������̌v�ʂ�g�ŕ\������.
		const auto ss = dx.m_aux_style.get();
		auto d_cnt = ss->GetDashesCount();
		if (d_cnt <= 0 || d_cnt > 6) {
			return;
		}
		const auto dc = dx.m_d2dContext.get();
		const auto cnt = m_dw_hit_linecnt;
		const auto br = dx.m_shape_brush.get();
		br->SetColor(dx.m_rng_bcolor);
		D2D1_MATRIX_3X2_F tran;
		dc->GetTransform(&tran);
		auto sw = static_cast<FLOAT>(1.0 / tran.m11);
		D2D1_STROKE_STYLE_PROPERTIES sp;
		sp.dashCap = ss->GetDashCap();
		sp.dashOffset = ss->GetDashOffset();
		sp.dashStyle = ss->GetDashStyle();
		sp.endCap = ss->GetEndCap();
		sp.lineJoin = ss->GetLineJoin();
		sp.miterLimit = ss->GetMiterLimit();
		sp.startCap = ss->GetStartCap();
		FLOAT d_arr[6];
		ss->GetDashes(d_arr, d_cnt);
		double mod = d_arr[0];
		for (uint32_t i = 1; i < d_cnt; i++) {
			mod += d_arr[i];
		}
		ID2D1Factory* fa;
		ss->GetFactory(&fa);
		for (uint32_t i = 0; i < cnt; i++) {
			auto const& m = m_dw_hit_metrics[i];
			winrt::com_ptr<ID2D1StrokeStyle> style;
			// �j��������ďd�Ȃ��ĕ\������Ȃ��悤��, �j���̃I�t�Z�b�g��ݒ肵,
			// ������̘g��ӂ��Ƃɕ\������.
			D2D1_POINT_2F p[4];
			p[0].x = t_min.x + m.left;
			p[0].y = static_cast<FLOAT>(t_min.y + m.top + m.height - m_font_size);
			p[2].x = p[0].x + m.width;
			p[2].y = static_cast<FLOAT>(p[0].y + m_font_size);
			p[1].x = p[2].x;
			p[1].y = p[0].y;
			p[3].x = p[0].x;
			p[3].y = p[2].y;
			sp.dashOffset = static_cast<FLOAT>(std::fmod(p[0].x, mod));
			fa->CreateStrokeStyle(&sp, d_arr, d_cnt, style.put());
			dc->DrawLine(p[0], p[1], br, sw, style.get());
			dc->DrawLine(p[3], p[2], br, sw, style.get());
			style = nullptr;
			sp.dashOffset = static_cast<FLOAT>(std::fmod(p[0].y, mod));
			fa->CreateStrokeStyle(&sp, d_arr, d_cnt, style.put());
			dc->DrawLine(p[1], p[2], br, sw, style.get());
			dc->DrawLine(p[0], p[3], br, sw, style.get());
			style = nullptr;
		}
	}

	// �v�f�𗘗p�\�ȏ��̖����瓾��.
	wchar_t* ShapeText::get_available_font(const uint32_t i)
	{
		return s_available_fonts[i];
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
	bool ShapeText::get_font_size(double& val) const noexcept
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

	// ������𓾂�.
	bool ShapeText::get_text(wchar_t*& val) const noexcept
	{
		val = m_text;
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

	// �s�Ԃ𓾂�.
	bool ShapeText::get_text_line(double& val) const noexcept
	{
		val = m_text_line;
		return true;
	}

	// ������̗]���𓾂�.
	bool ShapeText::get_text_margin(D2D1_SIZE_F& val) const noexcept
	{
		val = m_text_mar;
		return true;
	}

	// �����͈͂𓾂�
	bool ShapeText::get_text_range(DWRITE_TEXT_RANGE& val) const noexcept
	{
		val = m_sel_range;
		return true;
	}

	// �ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// a_len	���ʂ̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	ANCH_WHICH ShapeText::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		// ������}�`�̍��オ���_�ɂȂ�悤, ���ׂ�ʒu���ړ�����.
		D2D1_POINT_2F pos;
		ShapeRect::get_min_pos(pos);
		const auto px = static_cast<double>(t_pos.x) - pos.x - m_text_mar.width;
		const auto py = static_cast<double>(t_pos.y) - pos.y - m_text_mar.height;
		const auto cnt = m_dw_hit_linecnt;
		for (uint32_t i = 0; i < cnt; i++) {
			auto const& m = m_dw_hit_metrics[i];
			const auto min_x = static_cast<double>(m.left);
			const auto min_y = static_cast<double>(m.top) + static_cast<double>(m.height) - m_font_size;
			const auto max_x = min_x + m.width;
			const auto max_y = min_y + m_font_size;
			if (min_x <= px && px <= max_x && min_y <= py && py <= max_y) {
				return ANCH_TEXT;
			}
		}
		return ShapeRect::hit_test(t_pos, a_len);
	}

	// �͈͂Ɋ܂܂�邩���ׂ�.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeText::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		const uint32_t cnt = m_dw_hit_linecnt;
		D2D1_POINT_2F pos;
		D2D1_POINT_2F h_min;
		D2D1_POINT_2F h_max;

		if (cnt > 0) {
			ShapeRect::get_min_pos(pos);
			for (uint32_t i = 0; i < cnt; i++) {
				auto const& m = m_dw_hit_metrics[i];
				pt_add(pos, m.left, m.top + m.height - m_font_size, h_min);
				if (pt_in_rect(h_min, a_min, a_max) == false) {
					return false;
				}
				pt_add(h_min, m_dw_hit_metrics[i].width, m_font_size, h_max);
				if (pt_in_rect(h_max, a_min, a_max) == false) {
					return false;
				}
			}
		}
		return ShapeRect::in_area(a_min, a_max);
	}

	// ���̖������p�\�����ׂ�.
	// font	���̖�
	// �߂�l	���p�\�Ȃ� true
	// ���p�\�Ȃ�, �����̏��̖���j����, ���p�\�ȏ��̖��z��̗v�f�ƒu��������.
	bool ShapeText::is_available_font(wchar_t*& font)
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

	// ���p�\�ȏ��̖���j������.
	void ShapeText::release_available_fonts(void)
	{
		for (uint32_t i = 0; s_available_fonts[i]; i++) {
			delete[] s_available_fonts[i];
		}
		delete[] s_available_fonts;
		s_available_fonts = nullptr;
	}

	// ���p�\�ȏ��̖��Ɋi�[����.
	// coll		DWRITE �t�H���g�R���N�V����
	// lang		�n��E���ꖼ
	void ShapeText::set_available_fonts(IDWriteFontCollection* coll, wchar_t lang[])
	{
		UINT32 index = 0;
		BOOL exists = false;
		// �t�H���g�R���N�V�����̗v�f���𓾂�.
		// ����ꂽ�v�f���̗��p�\�ȏ��̖����m�ۂ���.
		const auto f_cnt = coll->GetFontFamilyCount();
		if (s_available_fonts != nullptr) {
			// ���p�\�ȏ��̖�����łȂ����, ������j������.
			for (auto i = 0; s_available_fonts[i] != nullptr; i++) {
				delete[] s_available_fonts[i];
			}
			delete[] s_available_fonts;
			s_available_fonts = nullptr;
		}
		s_available_fonts = new wchar_t* [static_cast<size_t>(f_cnt) + 1];
		// �t�H���g�R���N�V�����̊e�v�f�ɂ���.
		for (uint32_t i = 0; i < f_cnt; i++) {
			// �v�f���珑�̖��𓾂�.
			// ���̖����烍�[�J���C�Y���ꂽ���̖��𓾂�.
			// ���[�J���C�Y���ꂽ���̖�����, �n�於���̂��������̖��̊J�n�ʒu�𓾂�.
			// �n�於���܂܂�ĂȂ���� 0 ���J�n�ʒu�Ɋi�[����.
			// �J�n�ʒu�����̕������𓾂� (�k�������͊܂܂�Ȃ�).
			// ������ + 1 �̕����z����m�ۂ�, ���̖��̔z��Ɋi�[����.
			// ���[�J���C�Y���ꂽ���̖���j������.
			// �t�H���g�t�@�~���[����j������.
			winrt::com_ptr<IDWriteFontFamily> font_family;
			winrt::check_hresult(
				coll->GetFontFamily(i, font_family.put())
			);
			winrt::com_ptr<IDWriteLocalizedStrings> localized_name;
			winrt::check_hresult(
				font_family->GetFamilyNames(localized_name.put())
			);
			winrt::check_hresult(
				localized_name->FindLocaleName(lang, &index, &exists)
			);
			if (exists == false) {
				index = 0;
			}
			UINT32 length;
			winrt::check_hresult(
				localized_name->GetStringLength(index, &length)
			);
			s_available_fonts[i] = new wchar_t[static_cast<size_t>(length) + 1];
			winrt::check_hresult(
				localized_name->GetString(index, s_available_fonts[i], length + 1)
			);
			localized_name = nullptr;
			font_family = nullptr;
		}
		// ���p�\�ȏ��̖��̖����ɏI�[�Ƃ��ăk�����i�[����.
		s_available_fonts[f_cnt] = nullptr;
	}

	// �l�����̂̐F�Ɋi�[����.
	void ShapeText::set_font_color(const D2D1_COLOR_F& val) noexcept
	{
		m_font_color = val;
	}

	// �l�����̖��Ɋi�[����.
	void ShapeText::set_font_family(wchar_t* const val)
	{
		// �l�����̖��Ɠ��������ׂ�.
		if (equal(m_font_family, val)) {
			// �����Ȃ�I������.
			return;
		}
		m_font_family = val;
		if (m_dw_text_layout != nullptr) {
			const DWRITE_TEXT_RANGE t_range{ 0, wchar_len(m_text) };
			m_dw_text_layout->SetFontFamilyName(m_font_family, t_range);
			create_test_metrics();
		}
		else {
			create_text_layout();
		}

	}

	// �l�����̂̑傫���Ɋi�[����.
	void ShapeText::set_font_size(const double val)
	{
		if (equal(m_font_size, val)) {
			return;
		}
		m_font_size = val;
		if (m_dw_text_layout.get() != nullptr) {
			const FLOAT z = static_cast<FLOAT>(val);
			m_dw_text_layout->SetFontSize(z, { 0, wchar_len(m_text) });
			create_test_metrics();
		}
		else {
			create_text_layout();
		}
	}

	// �l�����̂̉����Ɋi�[����.
	void ShapeText::set_font_stretch(const DWRITE_FONT_STRETCH val)
	{
		if (m_font_stretch != val) {
			m_font_stretch = val;
			if (m_dw_text_layout.get() != nullptr) {
				m_dw_text_layout->SetFontStretch(val, { 0, wchar_len(m_text) });
				create_test_metrics();
			}
			else {
				create_text_layout();
			}
		}
	}

	// �l�����̂̎��̂Ɋi�[����.
	void ShapeText::set_font_style(const DWRITE_FONT_STYLE val)
	{
		if (m_font_style != val) {
			m_font_style = val;
			if (m_dw_text_layout.get() != nullptr) {
				m_dw_text_layout->SetFontStyle(val, { 0, wchar_len(m_text) });
				create_test_metrics();
			}
			else {
				create_text_layout();
			}
		}
	}

	// �l�����̂̑����Ɋi�[����.
	void ShapeText::set_font_weight(const DWRITE_FONT_WEIGHT val)
	{
		if (m_font_weight != val) {
			m_font_weight = val;
			if (m_dw_text_layout.get() != nullptr) {
				m_dw_text_layout->SetFontWeight(val, { 0, wchar_len(m_text) });
				create_test_metrics();
			}
			else {
				create_text_layout();
			}
		}
	}

	// �l���w�肵�����ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
	void ShapeText::set_pos(const D2D1_POINT_2F val, const ANCH_WHICH a)
	{
		ShapeRect::set_pos(val, a);
		create_text_metrics();
	}

	// �l�𕶎���Ɋi�[����.
	void ShapeText::set_text(wchar_t* const val)
	{
		if (equal(m_text, val)) {
			return;
		}
		m_text = val;
		m_sel_range.startPosition = 0;
		m_sel_range.length = 0;
		create_text_layout();
	}

	// �l��i���̂��낦�Ɋi�[����.
	void ShapeText::set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT val)
	{
		if (m_text_align_p != val) {
			m_text_align_p = val;
			if (m_dw_text_layout.get() != nullptr) {
				m_dw_text_layout->SetParagraphAlignment(val);
				create_test_metrics();
			}
			else {
				create_text_layout();
			}
		}
	}

	// �l�𕶎���̂��낦�Ɋi�[����.
	void ShapeText::set_text_align_t(const DWRITE_TEXT_ALIGNMENT align)
	{
		if (m_text_align_t != align) {
			m_text_align_t = align;
			if (m_text != nullptr && m_text[0] != L'\0') {
				if (m_dw_text_layout.get() == nullptr) {
					create_text_layout();
				}
				else {
					m_dw_text_layout->SetTextAlignment(align);
					create_test_metrics();
				}
			}
		}
	}

	// �l���s�ԂɊi�[����.
	void ShapeText::set_text_line(const double val)
	{
		if (m_text_line != val) {
			m_text_line = val;
			if (m_dw_text_layout.get() != nullptr) {
				winrt::com_ptr<IDWriteTextLayout3> t3;
				if (m_dw_text_layout.try_as(t3)) {
					DWRITE_LINE_SPACING ls;
					if (m_text_line > 0.0) {
						ls.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
						ls.height = static_cast<FLOAT>(m_text_line);
						ls.baseline = static_cast<FLOAT>(ls.height * 0.8);
					}
					else {
						ls.method = DWRITE_LINE_SPACING_METHOD_DEFAULT;
						ls.height = 0.0f;
						ls.baseline = 0.0f;
					}
					ls.leadingBefore = 0.0f;
					ls.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
					t3->SetLineSpacing(&ls);
				}
				create_test_metrics();
			}
			else {
				create_text_layout();
			}
		}
	}

	// �l�𕶎���̗]���Ɋi�[����.
	void ShapeText::set_text_margin(const D2D1_SIZE_F val)
	{
		if (equal(m_text_mar, val)) {
			return;
		}
		m_text_mar = val;
		create_text_metrics();
	}

	// �l�𕶎��͈͂Ɋi�[����.
	void ShapeText::set_text_range(const DWRITE_TEXT_RANGE val)
	{
		if (equal(m_sel_range, val)) {
			return;
		}
		m_sel_range = val;
		create_test_metrics();
	}

	// �}�`���쐬����.
	// pos	�J�n�ʒu
	// vec	�I���x�N�g��
	// text	������
	// attr	����̑����l
	ShapeText::ShapeText(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, wchar_t* const text, const ShapePanel* attr) :
		ShapeRect::ShapeRect(pos, vec, attr),
		m_font_color(attr->m_font_color),
		m_font_family(attr->m_font_family),
		m_font_size(attr->m_font_size),
		m_font_stretch(attr->m_font_stretch),
		m_font_style(attr->m_font_style),
		m_font_weight(attr->m_font_weight),
		m_text_line(attr->m_text_line),
		m_text_mar(attr->m_text_mar),
		m_text(text),
		m_text_align_t(attr->m_text_align_t),
		m_text_align_p(attr->m_text_align_p),
		m_sel_range()
	{
		create_text_layout();
	}

	// �}�`���f�[�^���C�^�[����ǂݍ���.
	ShapeText::ShapeText(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_font_color, dt_reader);
		read(m_font_family, dt_reader);
		is_available_font(m_font_family);
		m_font_size = dt_reader.ReadDouble();
		m_font_stretch = static_cast<DWRITE_FONT_STRETCH>(dt_reader.ReadUInt32());
		m_font_style = static_cast<DWRITE_FONT_STYLE>(dt_reader.ReadUInt32());
		m_font_weight = static_cast<DWRITE_FONT_WEIGHT>(dt_reader.ReadUInt32());
		read(m_text, dt_reader);
		m_text_align_p = static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(dt_reader.ReadUInt32());
		m_text_align_t = static_cast<DWRITE_TEXT_ALIGNMENT>(dt_reader.ReadUInt32());
		m_text_line = dt_reader.ReadDouble();
		read(m_text_mar, dt_reader);
		create_text_layout();
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapeText::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeRect::write(dt_writer);

		write(m_font_color, dt_writer);
		write(m_font_family, dt_writer);
		dt_writer.WriteDouble(m_font_size);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_stretch));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_style));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_weight));
		write(m_text, dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_p));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_t));
		dt_writer.WriteDouble(m_text_line);
		write(m_text_mar, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeText::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		static constexpr char* SVG_STYLE[] = {
			"normal", "oblique", "italic"
		};
		static constexpr char* SVG_STRETCH[] = {
			"normal", "ultra-condensed", "extra-condensed",
			"condensed", "semi-condensed", "normal", "semi-expanded",
			"expanded", "extra-expanded", "ultra-expanded"
		};
		//	���������̂��炵�ʂ����߂�.
		//
		//	Chrome �ł�, �e�L�X�g�^�O�ɑ��� alignment-baseline="text-before-edge" 
		//	���w�肷�邾����, ����ʒu����ɂ��� Dwrite �Ɠ����悤�ɕ\�������.
		//	������, IE �� Edge �ł�, alignment-baseline �����͊��҂������������Ȃ��̂�,
		//	�㕔����̃x�[�X���C���܂Œl�ł���, ���������̂��炵�� dy ��
		//	�s���ƂɌv�Z��K�v������.
		//	���̂Ƃ�, �㕔����̃x�[�X���C���̍��� = �A�Z���g�ɂ͂Ȃ�Ȃ��̂�
		//	�f�Z���g��p���Čv�Z����K�v������.
		//	�e�L�X�g���C�A�E�g����t�H���g���g���b�N�X���擾����, �ȉ��̂悤�ɋ��߂�.
		//	���Ȃ݂�, designUnitsPerEm ��, �z�u (Em) �{�b�N�X�̒P�ʂ�����̑傫��.
		//	�f�Z���g��, �t�H���g�����̔z�u�{�b�N�X�̉�������x�[�X���C���܂ł̋���.
		//	dy = ���̍s�̃q�b�g�e�X�g���g���b�N�X�̍��� - �t�H���g�̑傫�� �~ (�f�Z���g �� �P�ʑ傫��) �ƂȂ�, �͂�.
		IDWriteFontCollection* fonts;
		m_dw_text_layout->GetFontCollection(&fonts);
		IDWriteFontFamily* family;
		fonts->GetFontFamily(0, &family);
		IDWriteFont* font;
		family->GetFont(0, &font);
		DWRITE_FONT_METRICS metrics;
		font->GetMetrics(&metrics);
		const double descent = m_font_size * ((static_cast<double>(metrics.descent)) / metrics.designUnitsPerEm);

		if (is_opaque(m_fill_color) || is_opaque(m_stroke_color)) {
			ShapeRect::write_svg(dt_writer);
		}
		//	������S�̂̑������w�肷�邽�߂� g �^�O���J�n����.
		write_svg("<g ", dt_writer);
		//	���̂̐F����������.
		write_svg(m_font_color, "fill", dt_writer);
		//	���̖�����������.
		write_svg("font-family=\"", dt_writer);
		write_svg(m_font_family, wchar_len(m_font_family), dt_writer);
		write_svg("\" ", dt_writer);
		//	���̂̑傫������������.
		write_svg(m_font_size, "font-size", dt_writer);
		//	���̂̐L�k����������.
		const auto stretch = static_cast<int32_t>(m_font_stretch);
		write_svg(SVG_STRETCH[stretch], "font-stretch", dt_writer);
		//	���̂̌`������������.
		const auto style = static_cast<int32_t>(m_font_style);
		write_svg(SVG_STYLE[style], "font-style", dt_writer);
		//	���̂̑�������������.
		const auto weight = static_cast<uint32_t>(m_font_weight);
		write_svg(weight, "font-weight", dt_writer);
		write_svg("none", "stroke", dt_writer);
		write_svg(">" SVG_NL, dt_writer);
		//	���̂�\�����鍶��ʒu�ɗ]����������.
		D2D1_POINT_2F pos;
		pt_add(m_pos, m_text_mar.width, m_text_mar.height, pos);
		for (uint32_t i = 0; i < m_dw_hit_linecnt; i++) {
			const wchar_t* t = m_text + m_dw_hit_metrics[i].textPosition;
			const uint32_t t_len = m_dw_hit_metrics[i].length;
			const double px = static_cast<double>(pos.x);
			const double qx = static_cast<double>(m_dw_hit_metrics[i].left);
			const double py = static_cast<double>(pos.y);
			const double qy = static_cast<double>(m_dw_hit_metrics[i].top);
			//	�������\�����鐂���Ȃ��炵�ʒu�����߂�.
			const double dy = m_dw_hit_metrics[i].height - descent;
			//	���������������.
			write_svg_text(t, t_len, px + qx, py + qy, dy, dt_writer);
		}
		write_svg("</g>" SVG_NL, dt_writer);
	}

	//	��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	//	t	������
	//	t_len	������
	//	x, y	�ʒu
	//	dy	�����Ȃ��炵��
	//	dt_writer	�f�[�^���C�^�[
	//	�߂�l	�Ȃ�
	static void write_svg_text(const wchar_t* t, const uint32_t t_len, const double x, const double y, const double dy, DataWriter const& dt_writer)
	{
		write_svg("<text ", dt_writer);
		write_svg(x, "x", dt_writer);
		write_svg(y, "y", dt_writer);
		write_svg(dy, "dy", dt_writer);
		//write_svg("text-before-edge", "alignment-baseline", dt_writer);
		write_svg(">", dt_writer);
		uint32_t k = 0;
		for (uint32_t i = k; i < t_len; i++) {
			const auto c = t[i];
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
			//	ent = "&quot;";
			//}
			//else if (c == L'\'') {
			//	ent = "&apos;";
			//}
			else {
				continue;
			}
			if (i > k) {
				write_svg(t + k, i - k, dt_writer);
			}
			write_svg(ent, dt_writer);
			k = i + 1;
		}
		if (t_len > k) {
			write_svg(t + k, t_len - k, dt_writer);
		}
		write_svg("</text>" SVG_NL, dt_writer);
	}

}
