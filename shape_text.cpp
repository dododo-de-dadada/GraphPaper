//------------------------------
// shape_text.cpp
// 文字列図形
//------------------------------
#include <cwctype>
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	wchar_t** ShapeText::s_available_fonts = nullptr;	//有効な書体名
	D2D1_COLOR_F ShapeText::s_text_selected_background{ COLOR_ACCENT };	// 文字範囲の背景色
	D2D1_COLOR_F ShapeText::s_text_selected_foreground{ COLOR_TEXT_RANGE };	// 文字範囲の文字色

	// ヒットテストの計量を作成する.
	static HRESULT text_create_test_metrics(IDWriteTextLayout* text_lay, const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt) noexcept;
	// ヒットテストの計量, 行の計量, 文字列選択の計量を作成する.
	//static HRESULT text_create_text_metrics(IDWriteTextLayout* text_lay, const uint32_t text_len, UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& line_cnt, DWRITE_LINE_METRICS*& line_met, UINT32& sele_cnt, DWRITE_HIT_TEST_METRICS*& sele_met, const DWRITE_TEXT_RANGE& sele_rng) noexcept;
	// 書体の計量を得る.
	static HRESULT text_get_font_metrics(IDWriteTextLayout* text_lay, DWRITE_FONT_METRICS* font_met) noexcept;

	//------------------------------
	// ヒットテストの計量を作成する.
	// text_lay	文字列レイアウト
	// text_rng	文字範囲
	// test_met	ヒットテストの計量
	// test_cnt	計量の要素数
	//------------------------------
	static HRESULT text_create_test_metrics(IDWriteTextLayout* text_lay, const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt) noexcept
	{
		const uint32_t pos = text_rng.startPosition;
		const uint32_t len = text_rng.length;
		DWRITE_HIT_TEST_METRICS test[1];

		// まず計量の要素数を得る.
		// 最初の HitTestTextRange 関数呼び出しは, 失敗することが前提なので, check_hresult しない.
		// 配列を確保して, あらためて関数を呼び出し, 計量を得る.
		text_lay->HitTestTextRange(pos, len, 0, 0, test, 1, &test_cnt);
		test_met = new DWRITE_HIT_TEST_METRICS[test_cnt];
		return text_lay->HitTestTextRange(pos, len, 0, 0, test_met, test_cnt, &test_cnt);
	}

	//------------------------------
	// 書体の計量を得る
	// text_lay	テキストレイアウト
	// font_met 書体の計量
	//------------------------------
	static HRESULT text_get_font_metrics(IDWriteTextLayout* text_lay, DWRITE_FONT_METRICS* font_met) noexcept
	{
		HRESULT hr = S_OK;
		// 文字列レイアウト ---> 書体リスト ---> 書体ファミリー ---> 書体を得る.
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
		// 書体の計量を得る.
		font->GetMetrics(font_met);
		font = nullptr;
		return hr;
	}

	//------------------------------
	// ヒットテストの計量, 行の計量, 選択された文字範囲の計量を破棄する.
	// test_cnt	ヒットテストと行の計量の各要素数
	// test_met	ヒットテストの計量
	// line_met 行の計量
	// sele_cnt	選択された文字範囲の計量の要素数
	// sele_met 選択された文字範囲の計量
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
	// ヒットテストの計量, 行の計量, 文字列選択の計量を得る.
	//------------------------------
	/*
	static HRESULT text_create_text_metrics(
		IDWriteTextLayout* text_lay,	// 文字列レイアウト
		const uint32_t text_len,	// 文字列の長さ
		UINT32& test_cnt,	// ヒットテストの計量の要素数
		DWRITE_HIT_TEST_METRICS*& test_met,	// ヒットテストの計量
		UINT32& line_cnt,	// ヒットテストの計量の要素数
		DWRITE_LINE_METRICS*& line_met,	// 行の計量
		UINT32& sele_cnt,	// 選択された文字範囲の計量の要素数
		DWRITE_HIT_TEST_METRICS*& sele_met,	// 選択された文字範囲の計量
		const DWRITE_TEXT_RANGE& sele_rng	// 選択された文字範囲
	) noexcept
	{
		if (text_lay == nullptr) {
			return E_FAIL;
		}
		const DWRITE_TEXT_ALIGNMENT alig_horz = text_lay->GetTextAlignment();
		const DWRITE_PARAGRAPH_ALIGNMENT alig_vert = text_lay->GetParagraphAlignment();
		HRESULT hr = S_OK;
		// 行の計量を作成する.
		text_lay->GetLineMetrics(nullptr, 0, &line_cnt);
		line_met = new DWRITE_LINE_METRICS[line_cnt];
		hr = text_lay->GetLineMetrics(line_met, line_cnt, &line_cnt);
		// ヒットテストの計量を作成する.
		if (hr == S_OK) {
			test_met = new DWRITE_HIT_TEST_METRICS[line_cnt];
			hr = text_lay->HitTestTextRange(0, text_len, 0.0f, 0.0f, test_met, line_cnt, &test_cnt);
		}
		// 選択された文字範囲の計量を作成する.
		if (sele_rng.length > 0) {
			if (hr == S_OK) {
				hr = text_create_test_metrics(text_lay, sele_rng, sele_met, sele_cnt);
			}
		}
		return hr;
	}
	*/

	//------------------------------
	// 枠を文字列に合わせる.
	// 戻り値	大きさが調整されたならば真.
	//------------------------------
	bool ShapeText::fit_frame_to_text(
		const float g_len	// 方眼の大きさ (この値が 1 以上ならばこの値に合わせる)
	) noexcept
	{
		// 文字列の大きさを計算し, 枠に格納する.
		D2D1_POINT_2F t_box{ 0.0f, 0.0f };	// 枠
		for (size_t i = 0; i < m_dwrite_test_cnt; i++) {
			t_box.x = fmax(t_box.x, m_dwrite_test_metrics[i].width);
			t_box.y += m_dwrite_test_metrics[i].height;
		}
		// 枠に左右のパディングを加える.
		const float sp = m_text_pad.width * 2.0f;	// 左右のパディング
		pt_add(t_box, sp, sp, t_box);
		if (g_len >= 1.0f) {
			// 枠を方眼の大きさに切り上げる.
			t_box.x = floor((t_box.x + g_len - 1.0f) / g_len) * g_len;
			t_box.y = floor((t_box.y + g_len - 1.0f) / g_len) * g_len;
		}
		// 図形の大きさを変更する.
		if (!equal(t_box, m_lineto)) {
			D2D1_POINT_2F se;
			pt_add(m_start, t_box, se);
			set_pos_loc(se, LOC_TYPE::LOC_SE, 0.0f, false);
			return true;
		}
		return false;
	}

	//------------------------------
	// 文字列レイアウトを作成する.
	//------------------------------
	void ShapeText::create_text_layout(void) noexcept
	{
		IDWriteFactory* const dwrite_factory = Shape::m_dwrite_factory.get();
		HRESULT hr = S_OK;
		bool updated = false;	// 文字列レイアウトが作成または変更

		// 文字列が変更されたとき, 文字列レイアウトは空に設定されるので, 文字列レイアウトを作成する.
		if (m_dwrite_text_layout == nullptr) {

			// 既定のロケール名を得る.
			wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
			GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);
			const UINT32 text_len = get_text_len();

			// 文字列フォーマットを作成する.
			// 属性値がなんであれ, DWRITE_FONT_STRETCH_NORMAL でテキストフォーマットは作成する.
			winrt::com_ptr<IDWriteTextFormat> t_format;
			hr = dwrite_factory->CreateTextFormat(m_font_family, static_cast<IDWriteFontCollection*>(nullptr), m_font_weight, m_font_style, m_font_stretch, m_font_size, locale_name,
				t_format.put());

			// 文字列フォーマットから文字列レイアウトを作成する.
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

			// 文字列フォーマットを破棄する.
			t_format = nullptr;

			// タイポグラフィを設定すると, fi など結束文字を避けられるらしい
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

			// 文字の幅, 文字のそろえ, 段落のそろえを文字列レイアウトに格納する.
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

			// 行間を文字列レイアウトに格納する.
			DWRITE_LINE_SPACING new_sp;
			new_sp.leadingBefore = 0.0f;
			new_sp.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
			if (hr == S_OK && m_dwrite_font_metrics.designUnitsPerEm == 0) {
				hr = text_get_font_metrics(t3.get(), &m_dwrite_font_metrics);
			}
			// 行間がゼロより大きいなら, その値を設定する.
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
			// 行間がゼロなら, 既定値を設定する.
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

		// 変更.
		else {

			winrt::com_ptr<IDWriteTextLayout3> t3;
			if (!m_dwrite_text_layout.try_as(t3)) {
				hr = E_FAIL;
			}
			const DWRITE_TEXT_RANGE range{ 0, get_text_len() };
			// 書体名が変更されたなら文字列レイアウトに格納する.
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

			// 書体の大きさが変更されたなら文字列レイアウトに格納する.
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

			// 書体の幅が変更されたなら文字列レイアウトに格納する.
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

			// 書体の字体が変更されたなら文字列レイアウトに格納する.
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

			// 書体の太さが変更されたなら文字列レイアウトに格納する.
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

			// 文字列のパディングが変更されたなら文字列レイアウトに格納する.
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

			// 段落のそろえが変更されたなら文字列レイアウトに格納する.
			const DWRITE_PARAGRAPH_ALIGNMENT p_align = t3->GetParagraphAlignment();
			if (hr == S_OK && !equal(p_align, m_text_align_vert)) {
				hr = t3->SetParagraphAlignment(m_text_align_vert);
				if (hr == S_OK) {
					updated = true;
				}
			}

			// 文字のそろえが変更されたなら文字列レイアウトに格納する.
			const DWRITE_TEXT_ALIGNMENT t_align = t3->GetTextAlignment();
			if (hr == S_OK && !equal(t_align, m_text_align_horz)) {
				hr = t3->SetTextAlignment(m_text_align_horz);
				if (hr == S_OK) {
					updated = true;
				}
			}

			// 文字列の折り返しが変更されたなら文字列レイアウトに格納する.
			const DWRITE_WORD_WRAPPING w_wrap = t3->GetWordWrapping();
			if (hr == S_OK && !equal(w_wrap, m_text_word_wrap)) {
				hr = t3->SetWordWrapping(m_text_word_wrap);
				if (hr == S_OK) {
					updated = true;
				}
			}

			// 行間が変更されたなら文字列レイアウトに格納する.
			DWRITE_LINE_SPACING new_sp;
			new_sp.leadingBefore = 0.0f;
			new_sp.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
			// 行間がゼロより大きいなら,　その値を設定する.
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
			// 行間がゼロなら, 既定値を設定する.
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

			// 行の計量を作成する.
			if (hr == S_OK) {
				// 行の計量の個数を得て, メモリを確保し, 改めて計量を得る.
				DWRITE_LINE_METRICS dummy;	// nullptr だとハングアップをおこすことがあるので, ダミーを使う.
				m_dwrite_text_layout->GetLineMetrics(&dummy, 1, &m_dwrite_line_cnt);
				m_dwrite_line_metrics = new DWRITE_LINE_METRICS[m_dwrite_line_cnt];
				hr = m_dwrite_text_layout->GetLineMetrics(m_dwrite_line_metrics, m_dwrite_line_cnt, &m_dwrite_line_cnt);
			}

			// ヒットテストの計量を作成する.
			if (hr == S_OK) {
				// ヒットテストの計量の個数は, 行の計量の個数を超えることはない (はず).
				// 文末が空白 (改行) だけのとき, 逆はありうる.
				m_dwrite_test_metrics = new DWRITE_HIT_TEST_METRICS[m_dwrite_line_cnt];
				hr = m_dwrite_text_layout->HitTestTextRange(0, get_text_len(), 0.0f, 0.0f, m_dwrite_test_metrics, m_dwrite_line_cnt, &m_dwrite_test_cnt);
			}

			// ヒットテストの計量は文字列末尾の改行はトリミングしてしまうので,
			// 必要なら, 行の計量をもとに, トリミングされた計量を補う.
			if (hr == S_OK && m_dwrite_test_cnt < m_dwrite_line_cnt) {
				float last_top;	// 最終行の上端
				float last_left;	// 最終行の左端
				uint32_t last_pos;	// 最終行の文字位置
				// ヒットテストの計量があるなら, その最終行から計算する.
				if (m_dwrite_test_cnt > 0) {
					last_top = m_dwrite_test_metrics[m_dwrite_test_cnt - 1].top + m_dwrite_test_metrics[m_dwrite_test_cnt - 1].height;
					last_pos = m_dwrite_test_metrics[m_dwrite_test_cnt - 1].textPosition + m_dwrite_line_metrics[m_dwrite_test_cnt - 1].length;
				}
				// ヒットテストの計量がなければ, 行の計量から計算する.
				else {
					if (m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR) {
						last_top = 0.0f;
						last_pos = 0;
						for (uint32_t i = 0; i + 1 < m_dwrite_line_cnt; i++) {
							last_top = last_top + m_dwrite_line_metrics[i - 1].height;
							last_pos = last_pos + m_dwrite_line_metrics[i - 1].length;
						}
						last_top = last_top + m_font_size;	// 最終行の高さは書体の大きさ
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
						last_top = 0.5 * (last_top + m_font_size);	// 最終行の高さは書体の大きさ
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
				// 段落のそろえが右よせなら, 最終行の左端には文字列レイアウトの幅を格納する.
				if (m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING) {
					//last_left = fabs(m_pos.x) - min(fabs(m_pos.x), 2.0f * m_text_pad.width);
					last_left = m_dwrite_text_layout->GetMaxWidth();
				}
				// 段落のそろえが中央または均等なら, 最終行の左端には文字列レイアウトの幅の半分を格納する.
				else if (m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER ||
					m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED) {
					//last_left = 0.5f * fabs(m_pos.x);
					last_left = m_dwrite_text_layout->GetMaxWidth() * 0.5f;
				}
				// 段落のそろえがそれ以外なら, 最終行の左端には 0 を格納する.
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
			// 属性値の変更がなくても, 選択された文字範囲が変更されたなら,
			//const auto end = m_select_trail ? m_select_end + 1 : m_select_end;
			//const auto s = min(m_select_start, end);
			//const auto e = max(m_select_start, end);
			//if (m_dwrite_selected_cnt == 0) {
			//	if (m_select_start != end) {
			//		updated = true;
			//	}
			//}
			//else {
			//	uint32_t s_len = 0;	// 選択範囲の長さ
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

	// 選択範囲された文字列を表示する.
	void ShapeText::draw_selection(const uint32_t sele_start, const uint32_t sele_end, const bool sele_trailing) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const color_brush = Shape::m_d2d_color_brush.get();
		ID2D1SolidColorBrush* const range_brush = Shape::m_d2d_range_brush.get();

		// 余白分をくわえた, 文字列の左上位置を計算する.
		const double h = min(m_text_pad.width, fabs(m_lineto.x) * 0.5f);
		const double v = min(m_text_pad.height, fabs(m_lineto.y) * 0.5f);
		D2D1_POINT_2F t_lt{
			m_lineto.x < 0.0f ? h + m_start.x + m_lineto.x : h + m_start.x,
			m_lineto.y < 0.0f ? v + m_start.y + m_lineto.y : v + m_start.y
		};

		// 選択範囲があれば選択範囲の計量を得る.
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

		// 選択範囲の計量をもとに範囲を塗りつぶす.
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

		// 文字列全体を透明化
		if (hr == S_OK && s < e) {
			constexpr D2D1_COLOR_F TRANSPALENT{ 0.0f, 0.0f, 0.0f, 0.0f };
			color_brush->SetColor(TRANSPALENT);
			hr = m_dwrite_text_layout->SetDrawingEffect(color_brush, DWRITE_TEXT_RANGE{ 0, len });
		}
		// 選択範囲だけ前景色を設定する.
		if (hr == S_OK && s < e) {
			DWRITE_TEXT_RANGE ran{
				static_cast<uint32_t>(s), static_cast<uint32_t>(e - s)
			};
			range_brush->SetColor(ShapeText::s_text_selected_foreground);
			hr = m_dwrite_text_layout->SetDrawingEffect(range_brush, ran);
		}
		// 文字列を表示したあと, 透明化を戻す
		if (hr == S_OK && s < e) {
			target->DrawTextLayout(t_lt, m_dwrite_text_layout.get(), color_brush);
			color_brush->SetColor(m_font_color);
			hr = m_dwrite_text_layout->SetDrawingEffect(color_brush, DWRITE_TEXT_RANGE{ 0, len });
		}
	}

	// 図形を表示する.
	void ShapeText::draw(void) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const color_brush = Shape::m_d2d_color_brush.get();
		ID2D1SolidColorBrush* const range_brush = Shape::m_d2d_range_brush.get();
		ID2D1Factory* factory;
		target->GetFactory(&factory);

		// 方形を描く.
		ShapeRect::draw();

		create_text_layout();

		// 余白分をくわえた, 文字列の左上位置を計算する.
		const double h = min(m_text_pad.width, fabs(m_lineto.x) * 0.5f);
		const double v = min(m_text_pad.height, fabs(m_lineto.y) * 0.5f);
		D2D1_POINT_2F t_lt{
			m_lineto.x < 0.0f ? h + m_start.x + m_lineto.x : h + m_start.x,
			m_lineto.y < 0.0f ? v + m_start.y + m_lineto.y : v + m_start.y
		};

		HRESULT hr = S_OK;

		// 文字列を表示する
		color_brush->SetColor(m_font_color);
		target->DrawTextLayout(t_lt, m_dwrite_text_layout.get(), color_brush);

		// 図形が選択されているなら, 文字列の補助線を表示する
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
				// 破線がずれて重なって表示されないように, 破線のオフセットを計算し,
				// 文字列の枠を辺ごとに表示する.
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

	// 書体の色を得る.
	bool ShapeText::get_font_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_font_color;
		return true;
	}

	// 書体名を得る.
	bool ShapeText::get_font_family(wchar_t*& val) const noexcept
	{
		val = m_font_family;
		return true;
	}

	// 書体の大きさを得る.
	bool ShapeText::get_font_size(float& val) const noexcept
	{
		val = m_font_size;
		return true;
	}

	// 書体の幅を得る.
	bool ShapeText::get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept
	{
		val = m_font_stretch;
		return true;
	}

	// 書体の字体を得る.
	bool ShapeText::get_font_style(DWRITE_FONT_STYLE& val) const noexcept
	{
		val = m_font_style;
		return true;
	}

	// 書体の太さを得る.
	bool ShapeText::get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept
	{
		val = m_font_weight;
		return true;
	}

	// 段落のそろえを得る.
	bool ShapeText::get_text_align_vert(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_vert;
		return true;
	}

	// 文字列のそろえを得る.
	bool ShapeText::get_text_align_horz(DWRITE_TEXT_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_horz;
		return true;
	}

	// 文字列を得る.
	bool ShapeText::get_text_content(wchar_t*& val) const noexcept
	{
		val = m_text;
		return true;
	}

	// 行間を得る.
	bool ShapeText::get_text_line_sp(float& val) const noexcept
	{
		val = m_text_line_sp;
		return true;
	}

	// 文字列の余白を得る.
	bool ShapeText::get_text_pad(D2D1_SIZE_F& val) const noexcept
	{
		val = m_text_pad;
		return true;
	}

	// 文字範囲を得る
	//bool ShapeText::get_text_selected(DWRITE_TEXT_RANGE& val) const noexcept
	//{
	//	val = m_text_selected_range;
	//	return true;
	//}

	// 図形が点を含むか判定する.
	// test_pt	判定される点
	// 戻り値	点を含む部位
	uint32_t ShapeText::hit_test(const D2D1_POINT_2F test_pt, const bool/*ctrl_key*/) const noexcept
	{
		const uint32_t loc = rect_loc_hit_test(m_start, m_lineto, test_pt, m_loc_width);
		if (loc != LOC_TYPE::LOC_SHEET) {
			return loc;
		}
		const float descent = m_dwrite_font_metrics.designUnitsPerEm == 0 ?
			0.0f : 
			(m_font_size * m_dwrite_font_metrics.descent / m_dwrite_font_metrics.designUnitsPerEm);

		// 文字列の矩形の左上点と右下の Y 値を得る.
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
			//const D2D1_POINT_2F v{	// 行の左上点
			//	max(left, left + tl - f), top + tt + bl + descent - m_font_size
			//};
			//const D2D1_POINT_2F w{	// 行の右下点
			//	min(right, left + tl + tw + f), top + tt + bl + descent
			//};
			const D2D1_POINT_2F v{	// 行の左上点
				min(left, left + tl - f), top + tt + bl + descent - m_font_size
			};
			const D2D1_POINT_2F w{	// 行の右下点
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
			const D2D1_POINT_2F v{	// 行の左上点
				left, top + tt + bl + descent - m_font_size
			};
			const D2D1_POINT_2F w{	// 行の右下点
				right, top + tt + bl + descent
			};
			if (pt_in_rect(pt, v, w)) {
				return LOC_TYPE::LOC_TEXT;
			}
		}
		*/
		return ShapeRect::hit_test(test_pt);
	}

	// 矩形に含まれるか判定する.
	// lt	矩形の左上位置
	// rb	矩形の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeText::is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		D2D1_POINT_2F p_lt;	// 左上位置

		if (m_dwrite_test_cnt > 0 && m_dwrite_test_cnt < UINT32_MAX) {
			const float descent = 
				m_dwrite_font_metrics.designUnitsPerEm == 0 ? 0.0f :
				(m_font_size * m_dwrite_font_metrics.descent / m_dwrite_font_metrics.designUnitsPerEm);

			// 文字列の各行が矩形に含まれる判定する.
			ShapeRect::get_bbox_lt(p_lt);
			for (uint32_t i = 0; i < m_dwrite_test_cnt; i++) {
				const auto tl = m_dwrite_test_metrics[i].left;
				const auto tt = m_dwrite_test_metrics[i].top;
				const auto tw = m_dwrite_test_metrics[i].width;
				const auto bl = m_dwrite_line_metrics[i].baseline;
				const double top = static_cast<double>(tt) + bl + descent - m_font_size;
				D2D1_POINT_2F t_lt;	// 文字列の左上位置
				pt_add(p_lt, tl, top, t_lt);
				if (!pt_in_rect(t_lt, lt, rb)) {
					return false;
				}
				D2D1_POINT_2F t_rb;	// 文字列の右下位置
				pt_add(t_lt, tw, m_font_size, t_rb);
				if (!pt_in_rect(t_rb, lt, rb)) {
					return false;
				}
			}
		}
		return ShapeRect::is_inside(lt, rb);
	}

	// 書体名が有効か判定する.
	// font	書体名
	// 戻り値	有効なら true
	bool ShapeText::is_available_font(wchar_t*& font) noexcept
	{
		if (font) {
			for (uint32_t i = 0; s_available_fonts[i]; i++) {
				// 有効な書体名とアドレスが一致したなら true を返す.
				if (font == s_available_fonts[i]) {
					return true;
				}
				// アドレスは違うが有効な書体名と一致したなら,
				if (wcscmp(font, s_available_fonts[i]) == 0) {
					// 引数の書体名は破棄し, かわりに有効な書体名を引数に格納し, true を返す.
					delete[] font;
					font = s_available_fonts[i];
					return true;
				}
			}
		}
		return false;
	}

	// 有効な書体名の配列を破棄する.
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

	// 有効な書体名の配列を設定する.
	// DWriteFactory のシステムフォントコレクションから,
	// 既定の地域・言語名に対応した書体名を得て, それらを配列に格納する.
	// 'en-us' と GetUserDefaultLocaleName で得られる, 2 つのロケールそれぞれの書体名を得る.
	// 次のように格納される.
	// "ＭＳ ゴシック\0MS Gothic\0"
	void ShapeText::set_available_fonts(void) noexcept
	{
		HRESULT hr = S_OK;

		//s_d2d_factory = d2d.m_d2d_factory.get();
		// 既定の地域・言語名を得る.
		wchar_t lang[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(lang, LOCALE_NAME_MAX_LENGTH);

		// システムが持つフォント集合を DWriteFactory から得る.
		winrt::com_ptr<IDWriteFontCollection> collection;

		hr = Shape::m_dwrite_factory->GetSystemFontCollection(collection.put());

		// フォント集合の要素数を得る.
		const uint32_t f_cnt = collection->GetFontFamilyCount();

		// 得られた要素数 + 1 の配列を確保する.
		s_available_fonts = new wchar_t* [static_cast<size_t>(f_cnt) + 1];

		// フォント集合の各要素について.
		for (uint32_t i = 0; hr == S_OK && i < f_cnt; i++) {

			// 要素から書体を得る.
			winrt::com_ptr<IDWriteFontFamily> font_family;
			if (hr == S_OK) {
				hr = collection->GetFontFamily(i, font_family.put());
			}

			// 書体からローカライズされた書体名を得る.
			winrt::com_ptr<IDWriteLocalizedStrings> loc_name;
			if (hr == S_OK) {
				hr = font_family->GetFamilyNames(loc_name.put());
			}

			// ローカライズされた書体名の位置を得る.
			UINT32 index_en_us = 0;
			UINT32 index_local = 0;
			BOOL exists = false;
			if (hr == S_OK) {
				hr = loc_name->FindLocaleName(L"en-us", &index_en_us, &exists);
			}
			if (exists != TRUE) {
				// en-us がない場合,
				// 0 を位置に格納する.
				index_en_us = 0;
			}
			if (hr == S_OK) {
				hr = loc_name->FindLocaleName(lang, &index_local, &exists);
			}
			if (exists != TRUE) {
				// 地域名がない場合,
				// en_us を開始位置に格納する.
				index_local = index_en_us;
			}

			// 開始位置より後ろの文字数を得る (ヌル文字は含まれない).
			UINT32 length_en_us = 0;
			UINT32 length_local = 0;
			if (hr == S_OK) {
				hr = loc_name->GetStringLength(index_en_us, &length_en_us);
			}
			if (hr == S_OK) {
				hr = loc_name->GetStringLength(index_local, &length_local);
			}

			// 文字数 + 1 の文字配列を確保し, 書体名の配列に格納する.
			s_available_fonts[i] = new wchar_t[static_cast<size_t>(length_en_us) + 1 + static_cast<size_t>(length_local) + 1];
			if (hr == S_OK) {
				hr = loc_name->GetString(index_en_us, s_available_fonts[i], length_en_us + 1);
			}
			if (hr == S_OK) {
				hr = loc_name->GetString(index_local, s_available_fonts[i] + static_cast<size_t>(length_en_us) + 1, length_local + 1);
			}

			// ローカライズされた書体名と書体を破棄する.
			loc_name = nullptr;
			font_family = nullptr;
		}

		// 有効な書体名の配列の末尾に終端としてヌルを格納する.
		s_available_fonts[f_cnt] = nullptr;
	}

	// 値を書体の色に格納する.
	bool ShapeText::set_font_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_font_color, val)) {
			m_font_color = val;
			return true;
		}
		return false;
	}

	// 値を書体名に格納する.
	bool ShapeText::set_font_family(wchar_t* const val) noexcept
	{
		// 値が書体名と同じか判定する.
		if (!equal(m_font_family, val)) {
			m_font_family = val;
			return true;
		}
		return false;
	}

	// 値を書体の大きさに格納する.
	bool ShapeText::set_font_size(const float val) noexcept
	{
		if (m_font_size != val) {
			m_font_size = val;
			return true;
		}
		return false;
	}

	// 値を書体の横幅に格納する.
	bool ShapeText::set_font_stretch(const DWRITE_FONT_STRETCH val) noexcept
	{
		if (m_font_stretch != val) {
			m_font_stretch = val;
			return true;
		}
		return false;
	}

	// 値を書体の字体に格納する.
	bool ShapeText::set_font_style(const DWRITE_FONT_STYLE val) noexcept
	{
		if (m_font_style != val) {
			m_font_style = val;
			return true;
		}
		return false;
	}

	// 値を書体の太さに格納する.
	bool ShapeText::set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept
	{
		if (m_font_weight != val) {
			m_font_weight = val;
			return true;
		}
		return false;
	}

	// 値を文字列に格納する.
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

	// 値を段落のそろえに格納する.
	bool ShapeText::set_text_align_vert(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept
	{
		if (m_text_align_vert != val) {
			m_text_align_vert = val;
			return true;
		}
		return false;
	}

	// 値を文字列のそろえに格納する.
	bool ShapeText::set_text_align_horz(const DWRITE_TEXT_ALIGNMENT val) noexcept
	{
		if (m_text_align_horz != val) {
			m_text_align_horz = val;
			return true;
		}
		return false;
	}

	// 値を行間に格納する.
	bool ShapeText::set_text_line_sp(const float val) noexcept
	{
		if (!equal(m_text_line_sp, val)) {
			m_text_line_sp = val;
			return true;
		}
		return false;
	}

	// 値を文字列の余白に格納する.
	bool ShapeText::set_text_pad(const D2D1_SIZE_F val) noexcept
	{
		if (!equal(m_text_pad, val)) {
			m_text_pad = val;
			return true;
		}
		return false;
	}

	// 値を選択された文字範囲に格納する.
	//bool ShapeText::set_text_selected(const DWRITE_TEXT_RANGE val) noexcept
	//{
	//	if (!equal(m_text_selected_range, val)) {
	//		m_text_selected_range = val;
	//		return true;
	//	}
	//	return false;
	//}

	// 図形を作成する.
	// start	始点
	// pos	終点への位置ベクトル
	// text	文字列
	// prop	属性
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

	// 図形をデータライターから読み込む.
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
		// 書体の色
		if (m_font_color.r < 0.0f || m_font_color.r > 1.0f ||
			m_font_color.g < 0.0f || m_font_color.g > 1.0f ||
			m_font_color.b < 0.0f || m_font_color.b > 1.0f ||
			m_font_color.a < 0.0f || m_font_color.a > 1.0f) {
			m_font_color = COLOR_BLACK;
		}
		// 書体名
		is_available_font(m_font_family);
		// 書体の大きさ
		if (m_font_size < 1.0f || m_font_size > FONT_SIZE_MAX) {
			m_font_size = FONT_SIZE_DEFVAL;
		}
		// 書体の幅
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
		// 字体
		if (!(m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC ||
			m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL ||
			m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE)) {
			m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;
		}
		// 書体の太さ
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
		// 段落のそろえ
		if (!(m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER ||
			m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR ||
			m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR)) {
			m_text_align_vert = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		}
		// 文字列のそろえ
		if (!(m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER ||
			m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED ||
			m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING ||
			m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING)) {
			m_text_align_horz = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
		}
		// 行間
		if (m_text_line_sp < 0.0f || m_text_line_sp > 127.5f) {
			m_text_line_sp = 0.0f;
		}
		// 文字列の余白
		if (m_text_pad.width < 0.0f || m_text_pad.width > 127.5 ||
			m_text_pad.height < 0.0f || m_text_pad.height > 127.5) {
			m_text_pad.width = 0.0f;
			m_text_pad.height = 0.0f;
		}
	}

	// 図形をデータライターに書き込む.
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

	// wchar_t 型の文字列 (UTF-16) を uint32_t 型の配列に変換する.
	std::vector<uint32_t> text_utf16_to_utf32(const wchar_t* w, const size_t w_len) noexcept
	{
		const auto utf16 = winrt::array_view<const wchar_t>(w, w + w_len);
		std::vector<uint32_t> utf32{};
		for (uint32_t i = 0; i < std::size(utf16); i++) {
			// サロゲートペアの処理
			// 上位が範囲内で
			if (utf16[i] >= 0xD800 && utf16[i] <= 0xDBFF) {
				// 下位も範囲内なら
				if (i + 1 < std::size(utf16) && utf16[i + 1] >= 0xDC00 && utf16[i + 1] <= 0xDFFF) {
					// 32 ビット値を取り出し格納する.
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

	// 字面を得る.
	// T	文字列フォーマットまたは文字列レイアウトのいずれか.
	template <typename T>
	bool text_get_font_face(T* src, const wchar_t* family, const DWRITE_FONT_WEIGHT weight, const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_STYLE style, IDWriteFontFace3*& face) noexcept
	{
		bool ret = false;

		// 文字列を書き込む.
		IDWriteFontCollection* coll = nullptr;
		if (src->GetFontCollection(&coll) == S_OK) {
			// 図形と一致する書体ファミリを得る.
			IDWriteFontFamily* fam = nullptr;
			UINT32 index;
			BOOL exists;
			if (coll->FindFamilyName(family, &index, &exists) == S_OK &&
				exists &&
				coll->GetFontFamily(index, &fam) == S_OK) {
				// 書体ファミリから, 太さと幅, 字体が一致する書体を得る.
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