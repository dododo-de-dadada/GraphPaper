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
	D2D1_COLOR_F ShapeText::s_text_selected_background{ ACCENT_COLOR };	// 文字範囲の背景色
	D2D1_COLOR_F ShapeText::s_text_selected_foreground{ COLOR_TEXT_RANGE };	// 文字範囲の文字色

	// ヒットテストの計量を作成する.
	static void tx_create_test_metrics(IDWriteTextLayout* text_lay, const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt);
	// ヒットテストの計量, 行の計量, 文字列選択の計量を作成する.
	static void tx_create_text_metrics(IDWriteTextLayout* text_lay, const uint32_t text_len, UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_met, DWRITE_LINE_METRICS*& line_met, UINT32& sele_cnt, DWRITE_HIT_TEST_METRICS*& sele_met, const DWRITE_TEXT_RANGE& sele_rng);
	// 書体の計量を得る
	static void tx_get_font_metrics(IDWriteTextLayout* text_lay, DWRITE_FONT_METRICS* font_met);

	// ヒットテストの計量を作成する.
	// text_lay	文字列レイアウト
	// text_rng	文字範囲
	// test_met	ヒットテストの計量
	// test_cnt	計量の要素数
	static void tx_create_test_metrics(IDWriteTextLayout* text_lay, const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt)
	{
		const uint32_t pos = text_rng.startPosition;
		const uint32_t len = text_rng.length;
		DWRITE_HIT_TEST_METRICS test[1];

		// まず計量の要素数を得る.
		// 最初の HitTestTextRange 関数呼び出しは, 失敗することが前提なので, check_hresult しない.
		// 配列を確保して, あらためて関数を呼び出し, 計量を得る.
		text_lay->HitTestTextRange(pos, len, 0, 0, test, 1, &test_cnt);
		test_met = new DWRITE_HIT_TEST_METRICS[test_cnt];
		winrt::check_hresult(text_lay->HitTestTextRange(pos, len, 0, 0, test_met, test_cnt, &test_cnt));
	}

	// 書体の計量を得る
	static void tx_get_font_metrics(IDWriteTextLayout* text_lay, DWRITE_FONT_METRICS* font_met)
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

	// ヒットテストの計量, 行の計量, 選択された文字範囲の計量を破棄する.
	// test_cnt	ヒットテストと行の計量の各要素数
	// test_met	ヒットテストの計量
	// line_met 行の計量
	// sele_cnt	選択された文字範囲の計量の要素数
	// sele_met 選択された文字範囲の計量
	void ShapeText::relese_metrics(void) noexcept
	{
		m_dw_test_cnt = 0;
		if (m_dw_test_metrics != nullptr) {
			delete[] m_dw_test_metrics;
			m_dw_test_metrics = nullptr;
		}
		if (m_dw_line_metrics != nullptr) {
			delete[] m_dw_line_metrics;
			m_dw_line_metrics = nullptr;
		}
		m_dw_selected_cnt = 0;
		if (m_dw_selected_metrics != nullptr) {
			delete[] m_dw_selected_metrics;
			m_dw_selected_metrics = nullptr;
		}
	}

	// ヒットテストの計量, 行の計量, 文字列選択の計量を得る.
	// text_lay	文字列レイアウト
	// text_len	文字列の長さ
	// test_cnt	ヒットテストの計量の要素数
	// test_met	ヒットテストの計量
	// line_met 行の計量
	// sele_cnt	選択された文字範囲の計量の要素数
	// sele_met 選択された文字範囲の計量
	// sele_rng	選択された文字範囲
	static void tx_create_text_metrics(
		IDWriteTextLayout* text_lay, const uint32_t text_len,
		UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_met, DWRITE_LINE_METRICS*& line_met,
		UINT32& sele_cnt, DWRITE_HIT_TEST_METRICS*& sele_met,
		const DWRITE_TEXT_RANGE& sele_rng)
	{
		if (text_lay != nullptr) {
			// ヒットテストの計量を作成する.
			tx_create_test_metrics(text_lay, { 0, text_len }, test_met, test_cnt);

			// 行の計量を作成する.
			UINT32 line_cnt;
			text_lay->GetLineMetrics(nullptr, 0, &line_cnt);
			line_met = new DWRITE_LINE_METRICS[line_cnt];
			text_lay->GetLineMetrics(line_met, line_cnt, &line_cnt);

			// 選択された文字範囲の計量を作成する.
			if (sele_rng.length > 0) {
				tx_create_test_metrics(text_lay, sele_rng, sele_met, sele_cnt);
			}
		}
	}

	// 枠を文字列に合わせる.
	// g_len	方眼の大きさ (1 以上ならば方眼の大きさに合わせる)
	// 戻り値	大きさが調整されたならば真.
	bool ShapeText::frame_fit(const float g_len) noexcept
	{
		// 文字列の大きさを計算し, 枠に格納する.
		D2D1_POINT_2F t_box{ 0.0f, 0.0f };	// 枠
		for (size_t i = 0; i < m_dw_test_cnt; i++) {
			t_box.x = fmax(t_box.x, m_dw_test_metrics[i].width);
			t_box.y += m_dw_test_metrics[i].height;
		}
		// 枠に左右のパディングを加える.
		const float sp = m_text_padding.width * 2.0f;	// 左右のパディング
		pt_add(t_box, sp, sp, t_box);
		if (g_len >= 1.0f) {
			// 枠を方眼の大きさに切り上げる.
			t_box.x = floor((t_box.x + g_len - 1.0f) / g_len) * g_len;
			t_box.y = floor((t_box.y + g_len - 1.0f) / g_len) * g_len;
		}
		// 図形の大きさを変更する.
		if (!equal(t_box, m_vec[0])) {
			D2D1_POINT_2F se;
			pt_add(m_start, t_box, se);
			set_pos_anc(se, ANC_TYPE::ANC_SE, 0.0f, false);
			return true;
		}
		return false;
	}

	// 図形を表示する.
	void ShapeText::draw(void)
	{
		ID2D1Factory2* const factory = Shape::s_factory;
		IDWriteFactory* const dw_factory = Shape::s_dw_factory;
		ID2D1RenderTarget* const target = Shape::s_target;
		ID2D1SolidColorBrush* const color_brush = Shape::s_color_brush;
		ID2D1SolidColorBrush* const range_brush = Shape::s_range_brush;

		// 方形を描く.
		ShapeRect::draw();

		// 文字列が空か判定する.
		if (m_text == nullptr || m_text[0] == L'\0') {
			// 位置の計量, 行の計量, 文字列選択の計量を破棄する.
			relese_metrics();
			// 文字列レイアウトが空でないなら破棄する.
			if (m_dw_text_layout != nullptr) {
				m_dw_text_layout = nullptr;
			}
		}
		else {
			// 新規作成
			// 文字列レイアウトが空か判定する.
			if (m_dw_text_layout == nullptr) {
				// 既定のロケール名を得る.
				wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
				GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);

				// 文字列フォーマットを作成する.
				// CreateTextFormat で DWRITE_FONT_STRETCH_UNDEFINED が指定された場合エラーになる.
				// 属性値がなんであれ, DWRITE_FONT_STRETCH_NORMAL でテキストフォーマットは作成する.
				winrt::com_ptr<IDWriteTextFormat> t_format;
				winrt::check_hresult(
					dw_factory->CreateTextFormat(
						m_font_family, static_cast<IDWriteFontCollection*>(nullptr),
						m_font_weight, m_font_style, DWRITE_FONT_STRETCH_NORMAL,
						m_font_size, locale_name, t_format.put())
				);

				// 文字列フォーマットから文字列レイアウトを作成する.
				const double text_w = std::fabs(m_vec[0].x) - 2.0 * m_text_padding.width;
				const double text_h = std::fabs(m_vec[0].y) - 2.0 * m_text_padding.height;
				const UINT32 text_len = wchar_len(m_text);
				winrt::check_hresult(dw_factory->CreateTextLayout(m_text, text_len, t_format.get(), static_cast<FLOAT>(max(text_w, 0.0)), static_cast<FLOAT>(max(text_h, 0.0)), m_dw_text_layout.put()));

				// 文字列フォーマットを破棄する.
				t_format = nullptr;

				// 文字の幅の伸縮, 文字のそろえ, 段落のそろえを文字列レイアウトに格納する.
				winrt::check_hresult(m_dw_text_layout->SetFontStretch(m_font_stretch, DWRITE_TEXT_RANGE{ 0, text_len }));
				winrt::check_hresult(m_dw_text_layout->SetTextAlignment(m_text_align_t));
				winrt::check_hresult(m_dw_text_layout->SetParagraphAlignment(m_text_par_align));

				// 行間を文字列レイアウトに格納する.
				winrt::com_ptr<IDWriteTextLayout3> t3;
				if (m_dw_text_layout.try_as(t3)) {
					DWRITE_LINE_SPACING new_sp;
					new_sp.leadingBefore = 0.0f;
					new_sp.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
					// 行間がゼロより大きいなら, 行間を設定する.
					if (m_text_line_sp >= FLT_MIN) {
						if (m_dw_font_metrics.designUnitsPerEm == 0) {
							tx_get_font_metrics(t3.get(), &m_dw_font_metrics);
						}
						const float descent = (m_dw_font_metrics.designUnitsPerEm == 0 ? 0.0f : m_font_size * m_dw_font_metrics.descent / m_dw_font_metrics.designUnitsPerEm);

						new_sp.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
						new_sp.height = m_font_size + m_text_line_sp;
						new_sp.baseline = m_font_size + m_text_line_sp - descent;
					}
					// 既定の行間を設定する.
					else {
						new_sp.method = DWRITE_LINE_SPACING_METHOD_DEFAULT;
						new_sp.height = 0.0f;
						new_sp.baseline = 0.0f;
					}
					winrt::check_hresult(t3->SetLineSpacing(&new_sp));
					t3 = nullptr;
				}
				// 位置の計量, 行の計量, 文字列選択の計量を破棄する.
				relese_metrics();
				tx_create_text_metrics(m_dw_text_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_text_selected_range);
			}

			// 変更.
			else {

				const uint32_t text_len = wchar_len(m_text);
				bool updated = false;

				// 書体名が変更されたなら文字列レイアウトに格納する.
				WCHAR font_family[256];
				winrt::check_hresult(m_dw_text_layout->GetFontFamilyName(0, font_family, 256));
				if (wcscmp(font_family, m_font_family) != 0) {
					winrt::check_hresult(m_dw_text_layout->SetFontFamilyName(m_font_family, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// 書体の大きさが変更されたなら文字列レイアウトに格納する.
				FLOAT font_size;
				winrt::check_hresult(m_dw_text_layout->GetFontSize(0, &font_size));
				if (!equal(font_size, m_font_size)) {
					winrt::check_hresult(m_dw_text_layout->SetFontSize(m_font_size, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// 書体の幅の伸縮が変更されたなら文字列レイアウトに格納する.
				DWRITE_FONT_STRETCH font_stretch;
				winrt::check_hresult(m_dw_text_layout->GetFontStretch(0, &font_stretch));
				if (!equal(font_stretch, m_font_stretch)) {
					winrt::check_hresult(m_dw_text_layout->SetFontStretch(m_font_stretch, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// 書体の字体が変更されたなら文字列レイアウトに格納する.
				DWRITE_FONT_STYLE font_style;
				winrt::check_hresult(m_dw_text_layout->GetFontStyle(0, &font_style));
				if (!equal(font_style, m_font_style)) {
					winrt::check_hresult(m_dw_text_layout->SetFontStyle(m_font_style, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// 書体の太さが変更されたなら文字列レイアウトに格納する.
				DWRITE_FONT_WEIGHT font_weight;
				winrt::check_hresult(m_dw_text_layout->GetFontWeight(0, &font_weight));
				if (!equal(font_weight, m_font_weight)) {
					winrt::check_hresult(m_dw_text_layout->SetFontWeight(m_font_weight, DWRITE_TEXT_RANGE{ 0, text_len }));
					if (!updated) {
						updated = true;
					}
				}

				// 文字列のパディングが変更されたなら文字列レイアウトに格納する.
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

				// 段落のそろえが変更されたなら文字列レイアウトに格納する.
				DWRITE_PARAGRAPH_ALIGNMENT para_align = m_dw_text_layout->GetParagraphAlignment();
				if (!equal(para_align, m_text_par_align)) {
					winrt::check_hresult(m_dw_text_layout->SetParagraphAlignment(m_text_par_align));
					if (!updated) {
						updated = true;
					}
				}

				// 文字のそろえが変更されたなら文字列レイアウトに格納する.
				DWRITE_TEXT_ALIGNMENT text_align = m_dw_text_layout->GetTextAlignment();
				if (!equal(text_align, m_text_align_t)) {
					winrt::check_hresult(m_dw_text_layout->SetTextAlignment(m_text_align_t));
					if (!updated) {
						updated = true;
					}
				}

				// 行間が変更されたなら文字列レイアウトに格納する.
				winrt::com_ptr<IDWriteTextLayout3> t3;
				if (m_dw_text_layout.try_as(t3)) {
					DWRITE_LINE_SPACING old_sp;
					winrt::check_hresult(t3->GetLineSpacing(&old_sp));

					DWRITE_LINE_SPACING new_sp;
					new_sp.leadingBefore = 0.0f;
					new_sp.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
					// 行間がゼロより大きいなら, 行間を設定する.
					if (m_text_line_sp >= FLT_MIN) {
						if (m_dw_font_metrics.designUnitsPerEm == 0) {
							tx_get_font_metrics(t3.get(), &m_dw_font_metrics);
						}
						const float descent = (m_dw_font_metrics.designUnitsPerEm == 0 ? 0.0f : m_font_size * m_dw_font_metrics.descent / m_dw_font_metrics.designUnitsPerEm);

						new_sp.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
						new_sp.height = m_font_size + m_text_line_sp;
						new_sp.baseline = m_font_size + m_text_line_sp - descent;
					}
					// 既定の行間を設定する.
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

				// 計量の要素数が 0 なのに選択された文字範囲が 0 でない,
				// または, 選択された文字範囲が, それまでの選択された文字範囲の計量と異なる.
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

				// 変更されたか判定する.
				if (updated) {
					// 位置の計量, 行の計量, 文字列選択の計量を破棄する.
					relese_metrics();
					tx_create_text_metrics(m_dw_text_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_text_selected_range);
				}
			}

			// 余白分をくわえて, 文字列の左上位置を計算する.
			D2D1_POINT_2F t_min;
			pt_add(m_start, m_vec[0], t_min);
			t_min.x = m_start.x < t_min.x ? m_start.x : t_min.x;
			t_min.y = m_start.y < t_min.y ? m_start.y : t_min.y;
			const FLOAT pw = m_text_padding.width;
			const FLOAT ph = m_text_padding.height;
			const double hm = min(pw, fabs(m_vec[0].x) * 0.5);
			const double vm = min(ph, fabs(m_vec[0].y) * 0.5);
			pt_add(t_min, hm, vm, t_min);

			// 選択された文字範囲があるなら, 背景を塗りつぶす.
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
							color_brush->SetColor(ShapeText::s_text_selected_foreground);
							target->DrawRectangle(rect, color_brush, 2.0, nullptr);
							color_brush->SetColor(ShapeText::s_text_selected_background);
							target->FillRectangle(rect, color_brush);
							break;
						}
					}
				}
				range_brush->SetColor(ShapeText::s_text_selected_foreground);
				winrt::check_hresult(m_dw_text_layout->SetDrawingEffect(range_brush, m_text_selected_range));
			}

			// 文字列を表示する
			color_brush->SetColor(m_font_color);
			target->DrawTextLayout(t_min, m_dw_text_layout.get(), color_brush);
			if (m_text_selected_range.length > 0) {
				winrt::check_hresult(m_dw_text_layout->SetDrawingEffect(nullptr, { 0, wchar_len(m_text) }));
			}

			// 図形が選択されているなら, 文字列の補助線を表示する
			if (is_selected()) {
				const uint32_t d_cnt = Shape::m_aux_style->GetDashesCount();
				if (d_cnt <= 0 || d_cnt > 6) {
					return;
				}

				D2D1_MATRIX_3X2_F tran;
				target->GetTransform(&tran);
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
					// 破線がずれて重なって表示されないように, 破線のオフセットを計算し,
					// 文字列の枠を辺ごとに表示する.
					D2D1_POINT_2F p[4];
					p[0].x = t_min.x + tm.left;
					p[0].y = static_cast<FLOAT>(t_min.y + tm.top + lm.baseline + descent - m_font_size);
					p[2].x = p[0].x + tm.width;
					p[2].y = p[0].y + m_font_size;
					p[1].x = p[2].x;
					p[1].y = p[0].y;
					p[3].x = p[0].x;
					p[3].y = p[2].y;
					color_brush->SetColor(ShapeText::s_text_selected_foreground);
					target->DrawRectangle({ p[0].x, p[0].y, p[2].x, p[2].y }, color_brush, s_width, nullptr);
					color_brush->SetColor(ShapeText::s_text_selected_background);
					s_prop.dashOffset = static_cast<FLOAT>(std::fmod(p[0].x, mod));
					winrt::com_ptr<ID2D1StrokeStyle1> s_style;
					factory->CreateStrokeStyle(&s_prop, d_arr, d_cnt, s_style.put());
					target->DrawLine(p[0], p[1], color_brush, s_width, s_style.get());
					target->DrawLine(p[3], p[2], color_brush, s_width, s_style.get());
					s_style = nullptr;
					s_prop.dashOffset = static_cast<FLOAT>(std::fmod(p[0].y, mod));
					factory->CreateStrokeStyle(&s_prop, d_arr, d_cnt, s_style.put());
					target->DrawLine(p[1], p[2], color_brush, s_width, s_style.get());
					target->DrawLine(p[0], p[3], color_brush, s_width, s_style.get());
					s_style = nullptr;
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

	// 書体の幅の伸縮を得る.
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
	bool ShapeText::get_text_par_align(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept
	{
		val = m_text_par_align;
		return true;
	}

	// 文字列のそろえを得る.
	bool ShapeText::get_text_align_t(DWRITE_TEXT_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_t;
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
	bool ShapeText::get_text_padding(D2D1_SIZE_F& val) const noexcept
	{
		val = m_text_padding;
		return true;
	}

	// 文字範囲を得る
	bool ShapeText::get_text_selected(DWRITE_TEXT_RANGE& val) const noexcept
	{
		val = m_text_selected_range;
		return true;
	}

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeText::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		const uint32_t anc = ShapeRect::hit_test_anc(t_pos);
		if (anc != ANC_TYPE::ANC_PAGE) {
			return anc;
		}
		const float descent = m_dw_font_metrics.designUnitsPerEm == 0 ? 0.0f : (m_font_size * m_dw_font_metrics.descent / m_dw_font_metrics.designUnitsPerEm);
		// 文字列の範囲の左上が原点になるよう, 判定する位置を移動する.
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

	// 範囲に含まれるか判定する.
	// area_min	範囲の左上位置
	// area_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
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
	void ShapeText::set_available_fonts(const D2D_UI& d2d)
	{
		//s_d2d_factory = d2d.m_d2d_factory.get();
		// 既定の地域・言語名を得る.
		wchar_t lang[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(lang, LOCALE_NAME_MAX_LENGTH);
		// システムフォントコレクションを DWriteFactory から得る.
		winrt::com_ptr<IDWriteFontCollection> collection;
		winrt::check_hresult(d2d.m_dwrite_factory->GetSystemFontCollection(collection.put()));
		// フォントコレクションの要素数を得る.
		const uint32_t f_cnt = collection->GetFontFamilyCount();
		// 得られた要素数 + 1 の配列を確保する.
		s_available_fonts = new wchar_t* [static_cast<size_t>(f_cnt) + 1];
		// フォントコレクションの各要素について.
		for (uint32_t i = 0; i < f_cnt; i++) {
			// 要素から書体を得る.
			winrt::com_ptr<IDWriteFontFamily> font_family;
			winrt::check_hresult(collection->GetFontFamily(i, font_family.put()));
			// 書体からローカライズされた書体名を得る.
			winrt::com_ptr<IDWriteLocalizedStrings> localized_name;
			winrt::check_hresult(font_family->GetFamilyNames(localized_name.put()));
			// ローカライズされた書体名の位置を得る.
			UINT32 index_en_us = 0;
			UINT32 index_local = 0;
			BOOL exists = false;
			winrt::check_hresult(localized_name->FindLocaleName(L"en-us", &index_en_us, &exists));
			if (exists != TRUE) {
				// en-us がない場合,
				// 0 を位置に格納する.
				index_en_us = 0;
			}
			winrt::check_hresult(localized_name->FindLocaleName(lang, &index_local, &exists));
			if (exists != TRUE) {
				// 地域名がない場合,
				// en_us を開始位置に格納する.
				index_local = index_en_us;
			}
			/*
			winrt::com_ptr<IDWriteFont> font;
			font_family->GetFont(0, font.put());
			DWRITE_FONT_METRICS1 metrics;
			font.as<IDWriteFont1>()->GetMetrics(&metrics);
			*/
			// 開始位置より後ろの文字数を得る (ヌル文字は含まれない).
			UINT32 length_en_us = 0;
			UINT32 length_local = 0;
			winrt::check_hresult(localized_name->GetStringLength(index_en_us, &length_en_us));
			winrt::check_hresult(localized_name->GetStringLength(index_local, &length_local));
			// 文字数 + 1 の文字配列を確保し, 書体名の配列に格納する.
			s_available_fonts[i] = new wchar_t[static_cast<size_t>(length_en_us) + 1 + static_cast<size_t>(length_local) + 1];
			winrt::check_hresult(localized_name->GetString(index_en_us, s_available_fonts[i], length_en_us + 1));
			winrt::check_hresult(localized_name->GetString(index_local, s_available_fonts[i] + length_en_us + 1, length_local + 1));
			// ローカライズされた書体名を破棄する.
			localized_name = nullptr;
			// 書体をを破棄する.
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

	// 値を段落のそろえに格納する.
	bool ShapeText::set_text_par_align(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept
	{
		if (m_text_par_align != val) {
			m_text_par_align = val;
			return true;
		}
		return false;
	}

	// 値を文字列のそろえに格納する.
	bool ShapeText::set_text_align_t(const DWRITE_TEXT_ALIGNMENT val) noexcept
	{
		if (m_text_align_t != val) {
			m_text_align_t = val;
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
	bool ShapeText::set_text_padding(const D2D1_SIZE_F val) noexcept
	{
		if (!equal(m_text_padding, val)) {
			m_text_padding = val;
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
	// b_pos	囲む領域の始点
	// b_vec	囲む領域の終点への差分
	// text	文字列
	// setting	属性
	ShapeText::ShapeText(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, wchar_t* const text, const ShapePage* setting) :
		ShapeRect::ShapeRect(b_pos, b_vec, setting),
		m_font_color(setting->m_font_color),
		m_font_family(setting->m_font_family),
		m_font_size(setting->m_font_size),
		m_font_stretch(setting->m_font_stretch),
		m_font_style(setting->m_font_style),
		m_font_weight(setting->m_font_weight),
		m_text_line_sp(setting->m_text_line_sp),
		m_text_padding(setting->m_text_padding),
		m_text(text),
		m_text_align_t(setting->m_text_align_t),
		m_text_par_align(setting->m_text_par_align),
		m_text_selected_range(DWRITE_TEXT_RANGE{ 0, 0 })
	{
		ShapeText::is_available_font(m_font_family);
	}

	// 図形をデータライターから読み込む.
	ShapeText::ShapeText(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		// 書体の色
		const D2D1_COLOR_F font_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		m_font_color.r = min(max(font_color.r, 0.0f), 1.0f);
		m_font_color.g = min(max(font_color.g, 0.0f), 1.0f);
		m_font_color.b = min(max(font_color.b, 0.0f), 1.0f);
		m_font_color.a = min(max(font_color.a, 0.0f), 1.0f);
		// 書体名
		const size_t font_family_len = dt_reader.ReadUInt32();
		uint8_t* font_family_data = new uint8_t[2 * (font_family_len + 1)];
		dt_reader.ReadBytes(array_view(font_family_data, font_family_data + 2 * font_family_len));
		m_font_family = reinterpret_cast<wchar_t*>(font_family_data);
		m_font_family[font_family_len] = L'\0';
		is_available_font(m_font_family);
		// 書体の大きさ
		m_font_size = dt_reader.ReadSingle();
		// 書体の幅
		m_font_stretch = static_cast<DWRITE_FONT_STRETCH>(dt_reader.ReadUInt32());
		// 字体
		m_font_style = static_cast<DWRITE_FONT_STYLE>(dt_reader.ReadUInt32());
		// 書体の太さ
		m_font_weight = static_cast<DWRITE_FONT_WEIGHT>(dt_reader.ReadUInt32());
		// 文字列
		const size_t text_len = dt_reader.ReadUInt32();
		uint8_t* text_data = new uint8_t[2 * (text_len + 1)];
		dt_reader.ReadBytes(array_view(text_data, text_data + 2 * text_len));
		m_text = reinterpret_cast<wchar_t*>(text_data);
		m_text[text_len] = L'\0';

		m_text_par_align = static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(dt_reader.ReadUInt32());
		m_text_align_t = static_cast<DWRITE_TEXT_ALIGNMENT>(dt_reader.ReadUInt32());
		m_text_line_sp = dt_reader.ReadSingle();
		m_text_padding = D2D1_SIZE_F{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		//dt_read(m_text_padding, dt_reader);
	}

	// 図形をデータライターに書き込む.
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
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_par_align));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_t));
		dt_writer.WriteSingle(m_text_line_sp);
		dt_write(m_text_padding, dt_writer);
	}

}
