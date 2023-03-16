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
	//using winrt::GraphPaper::implementation::get_font_face;

	wchar_t** ShapeText::s_available_fonts = nullptr;	//有効な書体名
	D2D1_COLOR_F ShapeText::s_text_selected_background{ COLOR_ACCENT };	// 文字範囲の背景色
	D2D1_COLOR_F ShapeText::s_text_selected_foreground{ COLOR_TEXT_RANGE };	// 文字範囲の文字色

	// ヒットテストの計量を作成する.
	static void text_create_test_metrics(IDWriteTextLayout* text_lay, 
		const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt);
	// ヒットテストの計量, 行の計量, 文字列選択の計量を作成する.
	static void text_create_text_metrics(IDWriteTextLayout* text_lay, const uint32_t text_len,
		UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_met, DWRITE_LINE_METRICS*& line_met, 
		UINT32& sele_cnt, DWRITE_HIT_TEST_METRICS*& sele_met, const DWRITE_TEXT_RANGE& sele_rng);
	// 書体の計量を得る.
	static void text_get_font_metrics(IDWriteTextLayout* text_lay, DWRITE_FONT_METRICS* font_met);

	//------------------------------
	// ヒットテストの計量を作成する.
	// text_lay	文字列レイアウト
	// text_rng	文字範囲
	// test_met	ヒットテストの計量
	// test_cnt	計量の要素数
	//------------------------------
	static void text_create_test_metrics(IDWriteTextLayout* text_lay, 
		const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt)
	{
		const uint32_t pos = text_rng.startPosition;
		const uint32_t len = text_rng.length;
		DWRITE_HIT_TEST_METRICS test[1];

		// まず計量の要素数を得る.
		// 最初の HitTestTextRange 関数呼び出しは, 失敗することが前提なので, check_hresult しない.
		// 配列を確保して, あらためて関数を呼び出し, 計量を得る.
		text_lay->HitTestTextRange(pos, len, 0, 0, test, 1, &test_cnt);
		test_met = new DWRITE_HIT_TEST_METRICS[test_cnt];
		winrt::check_hresult(
			text_lay->HitTestTextRange(pos, len, 0, 0, test_met, test_cnt, &test_cnt));
	}

	//------------------------------
	// 書体の計量を得る
	// text_lay	テキストレイアウト
	// font_met 書体の計量
	//------------------------------
	static void text_get_font_metrics(IDWriteTextLayout* text_lay, DWRITE_FONT_METRICS* font_met)
	{
		// 文字列レイアウト ---> 書体リスト ---> 書体ファミリー ---> 書体を得る.
		winrt::com_ptr<IDWriteFontCollection> fonts;
		text_lay->GetFontCollection(fonts.put());
		winrt::com_ptr<IDWriteFontFamily> fam;
		fonts->GetFontFamily(0, fam.put());
		fonts = nullptr;
		winrt::com_ptr<IDWriteFont> font;
		fam->GetFont(0, font.put());
		fam = nullptr;
		// 書体の計量を得る.
		font->GetMetrics(font_met);
		font = nullptr;
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
	// text_lay	文字列レイアウト
	// text_len	文字列の長さ
	// test_cnt	ヒットテストの計量の要素数
	// test_met	ヒットテストの計量
	// line_met 行の計量
	// sele_cnt	選択された文字範囲の計量の要素数
	// sele_met 選択された文字範囲の計量
	// sele_rng	選択された文字範囲
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
			// ヒットテストの計量を作成する.
			text_create_test_metrics(text_lay, 
				{ 0, text_len }, test_met, test_cnt);

			// 行の計量を作成する.
			UINT32 line_cnt;
			text_lay->GetLineMetrics(nullptr, 0, &line_cnt);
			line_met = new DWRITE_LINE_METRICS[line_cnt];
			text_lay->GetLineMetrics(line_met, line_cnt, &line_cnt);

			// 選択された文字範囲の計量を作成する.
			if (sele_rng.length > 0) {
				text_create_test_metrics(text_lay,
					sele_rng, sele_met, sele_cnt);
			}
		}
	}

	//------------------------------
	// 枠を文字列に合わせる.
	// g_len	方眼の大きさ (1 以上ならば方眼の大きさに合わせる)
	// 戻り値	大きさが調整されたならば真.
	//------------------------------
	bool ShapeText::fit_frame_to_text(const float g_len) noexcept
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
		if (!equal(t_box, m_pos)) {
			D2D1_POINT_2F se;
			pt_add(m_start, t_box, se);
			set_pos_anc(se, ANC_TYPE::ANC_SE, 0.0f, false);
			return true;
		}
		return false;
	}

	//------------------------------
	// 文字列レイアウトを作成する.
	//------------------------------
	void ShapeText::create_text_layout(void)
	{
		IDWriteFactory* const dwrite_factory = Shape::m_dwrite_factory.get();

		// 新規作成
		// 文字列レイアウトが空か判定する.
		if (m_dwrite_text_layout == nullptr) {
			// 既定のロケール名を得る.
			wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
			GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);

			// 文字列フォーマットを作成する.
			// CreateTextFormat で DWRITE_FONT_STRETCH_UNDEFINED が指定された場合エラーになる.
			// 属性値がなんであれ, DWRITE_FONT_STRETCH_NORMAL でテキストフォーマットは作成する.
			winrt::com_ptr<IDWriteTextFormat> t_format;
			winrt::check_hresult(
				dwrite_factory->CreateTextFormat(
					m_font_family, static_cast<IDWriteFontCollection*>(nullptr),
					m_font_weight, m_font_style, m_font_stretch,
					m_font_size, locale_name, t_format.put())
			);

			// 文字列フォーマットから文字列レイアウトを作成する.
			const double text_w = std::fabs(m_pos.x) - 2.0 * m_text_pad.width;
			const double text_h = std::fabs(m_pos.y) - 2.0 * m_text_pad.height;
			const UINT32 text_len = wchar_len(m_text);
			winrt::check_hresult(
				dwrite_factory->CreateTextLayout(
					m_text, text_len, t_format.get(), static_cast<FLOAT>(max(text_w, 0.0)),
					static_cast<FLOAT>(max(text_h, 0.0)), m_dwrite_text_layout.put()));

			// 文字列フォーマットを破棄する.
			t_format = nullptr;

			winrt::com_ptr<IDWriteTextLayout3> t3;
			if (m_dwrite_text_layout.try_as(t3)) {
				// 文字の幅, 文字のそろえ, 段落のそろえを文字列レイアウトに格納する.
				winrt::check_hresult(
					t3->SetFontStretch(m_font_stretch, DWRITE_TEXT_RANGE{ 0, text_len }));
				winrt::check_hresult(
					t3->SetTextAlignment(m_text_align_horz));
				winrt::check_hresult(
					t3->SetParagraphAlignment(m_text_align_vert));

				// 行間を文字列レイアウトに格納する.
				DWRITE_LINE_SPACING new_sp;
				new_sp.leadingBefore = 0.0f;
				new_sp.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
				// 行間がゼロより大きいなら, 行間を設定する.
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
				// 既定の行間を設定する.
				else {
					new_sp.method = DWRITE_LINE_SPACING_METHOD_DEFAULT;
					new_sp.height = 0.0f;
					new_sp.baseline = 0.0f;
				}
				winrt::check_hresult(
					t3->SetLineSpacing(&new_sp));
				t3 = nullptr;
			}
			// 位置の計量, 行の計量, 文字列選択の計量を破棄する.
			relese_metrics();
			text_create_text_metrics(
				m_dwrite_text_layout.get(), wchar_len(m_text), m_dwrite_test_cnt,
				m_dwrite_test_metrics, m_dwrite_line_metrics, m_dwrite_selected_cnt,
				m_dwrite_selected_metrics, m_text_selected_range);
		}

		// 変更.
		else {

			winrt::com_ptr<IDWriteTextLayout3> t3;
			if (m_dwrite_text_layout.try_as(t3)) {
				const uint32_t text_len = wchar_len(m_text);
				bool updated = false;

				// 書体名が変更されたなら文字列レイアウトに格納する.
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

				// 書体の大きさが変更されたなら文字列レイアウトに格納する.
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

				// 書体の幅が変更されたなら文字列レイアウトに格納する.
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

				// 書体の字体が変更されたなら文字列レイアウトに格納する.
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

				// 書体の太さが変更されたなら文字列レイアウトに格納する.
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

				// 文字列のパディングが変更されたなら文字列レイアウトに格納する.
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

				// 段落のそろえが変更されたなら文字列レイアウトに格納する.
				DWRITE_PARAGRAPH_ALIGNMENT para_align = t3->GetParagraphAlignment();
				if (!equal(para_align, m_text_align_vert)) {
					winrt::check_hresult(
						t3->SetParagraphAlignment(m_text_align_vert));
					if (!updated) {
						updated = true;
					}
				}

				// 文字のそろえが変更されたなら文字列レイアウトに格納する.
				DWRITE_TEXT_ALIGNMENT text_align = t3->GetTextAlignment();
				if (!equal(text_align, m_text_align_horz)) {
					winrt::check_hresult(
						t3->SetTextAlignment(m_text_align_horz));
					if (!updated) {
						updated = true;
					}
				}

				// 行間が変更されたなら文字列レイアウトに格納する.
				DWRITE_LINE_SPACING old_sp;
				winrt::check_hresult(
					t3->GetLineSpacing(&old_sp));

				DWRITE_LINE_SPACING new_sp;
				new_sp.leadingBefore = 0.0f;
				new_sp.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;

				// 行間がゼロより大きいなら, 行間を設定する.
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

				// 既定の行間を設定する.
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

				// 計量の要素数が 0 なのに選択された文字範囲が 0 でない
				// または, 選択範囲の長さが, それまでの選択された文字範囲の計量と異なるなら,
				// 更新フラグをたてる.
				if (m_dwrite_selected_cnt == 0) {
					if (m_text_selected_range.length > 0) {
						if (!updated) {
							updated = true;
						}
					}
				}
				else {
					uint32_t s_len = 0;	// 選択範囲の長さ
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

	// 図形を表示する.
	void ShapeText::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const color_brush = Shape::m_d2d_color_brush.get();
		ID2D1SolidColorBrush* const range_brush = Shape::m_d2d_range_brush.get();
		ID2D1Factory* factory;
		target->GetFactory(&factory);

		// 方形を描く.
		ShapeRect::draw();

		// 文字列が空か判定する.
		if (m_text == nullptr || m_text[0] == L'\0') {
			relese_metrics();
			// 文字列レイアウトが空でないなら破棄する.
			if (m_dwrite_text_layout != nullptr) {
				m_dwrite_text_layout = nullptr;
			}
		}
		else {
			create_text_layout();

			// 余白分をくわえて, 文字列の左上位置を計算する.
			D2D1_POINT_2F t_lt;
			pt_add(m_start, m_pos, t_lt);
			t_lt.x = m_start.x < t_lt.x ? m_start.x : t_lt.x;
			t_lt.y = m_start.y < t_lt.y ? m_start.y : t_lt.y;
			const FLOAT pw = m_text_pad.width;
			const FLOAT ph = m_text_pad.height;
			const double hm = min(pw, fabs(m_pos.x) * 0.5);
			const double vm = min(ph, fabs(m_pos.y) * 0.5);
			pt_add(t_lt, hm, vm, t_lt);

			// 選択された文字範囲があるなら, 背景を塗りつぶす.
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

			// 文字列を表示する
			color_brush->SetColor(m_font_color);
			target->DrawTextLayout(t_lt, m_dwrite_text_layout.get(), color_brush);
			if (m_text_selected_range.length > 0) {
				winrt::check_hresult(
					m_dwrite_text_layout->SetDrawingEffect(nullptr, { 0, wchar_len(m_text) }));
			}

			// 図形が選択されているなら, 文字列の補助線を表示する
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
					// 破線がずれて重なって表示されないように, 破線のオフセットを計算し,
					// 文字列の枠を辺ごとに表示する.
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
	bool ShapeText::get_text_selected(DWRITE_TEXT_RANGE& val) const noexcept
	{
		val = m_text_selected_range;
		return true;
	}

	// 位置を含むか判定する.
	// test	判定する位置
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeText::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		const uint32_t anc = rect_hit_test_anc(m_start, m_pos, test, m_anc_width);
		if (anc != ANC_TYPE::ANC_PAGE) {
			return anc;
		}
		const float descent = m_dwrite_font_metrics.designUnitsPerEm == 0 ? 0.0f : 
			(m_font_size * m_dwrite_font_metrics.descent / m_dwrite_font_metrics.designUnitsPerEm);

		// 文字列の範囲の左上が原点になるよう, 判定する位置を移動する.
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

	// 矩形範囲に含まれるか判定する.
	// lt	矩形の左上位置
	// rb	矩形の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeText::in_area(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		D2D1_POINT_2F p_lt;	// 左上位置

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
		return ShapeRect::in_area(lt, rb);
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
	// 既定の地域・言語名に対応した書体名を得て,
	// それらを配列に格納する.
	// 'en-us' と GetUserDefaultLocaleName で得られたロケールと, 2 つの書体名を得る.
	// 次のように格納される.
	// "ＭＳ ゴシック\0MS Gothic\0"
	void ShapeText::set_available_fonts(void)
	{
		//s_d2d_factory = d2d.m_d2d_factory.get();
		// 既定の地域・言語名を得る.
		wchar_t lang[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(lang, LOCALE_NAME_MAX_LENGTH);

		// システムが持つフォント集合を DWriteFactory から得る.
		winrt::com_ptr<IDWriteFontCollection> collection;
		winrt::check_hresult(
			Shape::m_dwrite_factory->GetSystemFontCollection(collection.put()));

		// フォント集合の要素数を得る.
		const uint32_t f_cnt = collection->GetFontFamilyCount();

		// 得られた要素数 + 1 の配列を確保する.
		s_available_fonts = new wchar_t* [static_cast<size_t>(f_cnt) + 1];

		// フォント集合の各要素について.
		for (uint32_t i = 0; i < f_cnt; i++) {

			// 要素から書体を得る.
			winrt::com_ptr<IDWriteFontFamily> font_family;
			winrt::check_hresult(
				collection->GetFontFamily(i, font_family.put()));

			// 書体からローカライズされた書体名を得る.
			winrt::com_ptr<IDWriteLocalizedStrings> localized_name;
			winrt::check_hresult(
				font_family->GetFamilyNames(localized_name.put()));

			// ローカライズされた書体名の位置を得る.
			UINT32 index_en_us = 0;
			UINT32 index_local = 0;
			BOOL exists = false;
			winrt::check_hresult(
				localized_name->FindLocaleName(L"en-us", &index_en_us, &exists));
			if (exists != TRUE) {
				// en-us がない場合,
				// 0 を位置に格納する.
				index_en_us = 0;
			}
			winrt::check_hresult(
				localized_name->FindLocaleName(lang, &index_local, &exists));
			if (exists != TRUE) {
				// 地域名がない場合,
				// en_us を開始位置に格納する.
				index_local = index_en_us;
			}

			// 開始位置より後ろの文字数を得る (ヌル文字は含まれない).
			UINT32 length_en_us = 0;
			UINT32 length_local = 0;
			winrt::check_hresult(
				localized_name->GetStringLength(index_en_us, &length_en_us));
			winrt::check_hresult(
				localized_name->GetStringLength(index_local, &length_local));

			// 文字数 + 1 の文字配列を確保し, 書体名の配列に格納する.
			s_available_fonts[i] = new wchar_t[length_en_us + 1 + length_local + 1];
			winrt::check_hresult(
				localized_name->GetString(index_en_us, s_available_fonts[i], length_en_us + 1));
			winrt::check_hresult(
				localized_name->GetString(index_local, s_available_fonts[i] + length_en_us + 1,
					length_local + 1));

			// ローカライズされた書体名と書体を破棄する.
			localized_name = nullptr;
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
			//if (m_dwrite_text_layout != nullptr) {
			//	m_dwrite_text_layout = nullptr;
			//}
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
			//if (m_dwrite_text_layout != nullptr) {
			//	m_dwrite_text_layout = nullptr;
			//}
			return true;
		}
		return false;
	}

	// 値を書体の字体に格納する.
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

	// 値を書体の太さに格納する.
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

	// 値を文字列に格納する.
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
	bool ShapeText::set_text_selected(const DWRITE_TEXT_RANGE val) noexcept
	{
		if (!equal(m_text_selected_range, val)) {
			m_text_selected_range = val;
			return true;
		}
		return false;
	}

	// 図形を作成する.
	// start	始点
	// pos	対角点への位置ベクトル
	// text	文字列
	// page	属性
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

	// 図形をデータライターから読み込む.
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
			m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_DEMI_BOLD ||
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
	bool text_get_font_face(
		T* src, const wchar_t* family, const DWRITE_FONT_WEIGHT weight,
		const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_STYLE style, IDWriteFontFace3*& face)
		noexcept
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
		IDWriteTextFormat* t, const wchar_t* family, const DWRITE_FONT_WEIGHT weight,
		const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_STYLE style, IDWriteFontFace3*& face)
		noexcept;
	template bool text_get_font_face<IDWriteTextLayout>(
		IDWriteTextLayout* t, const wchar_t* family, const DWRITE_FONT_WEIGHT weight,
		const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_STYLE style, IDWriteFontFace3*& face)
		noexcept;
}