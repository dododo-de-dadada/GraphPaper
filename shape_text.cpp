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
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;

	wchar_t** ShapeText::s_available_fonts = nullptr;	//有効な書体名
	D2D1_COLOR_F ShapeText::s_text_selected_background{ ACCENT_COLOR };	// 文字範囲の背景色
	D2D1_COLOR_F ShapeText::s_text_selected_foreground{ COLOR_TEXT_SELECTED };	// 文字範囲の文字色

	// ヒットテストの計量を得る.
	static void tx_create_test_metrics(IDWriteTextLayout* text_lay, const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt);
	// ヒットテストの計量, 行の計量, 文字列選択の計量を得る.
	static void tx_create_text_metrics(IDWriteTextLayout* text_lay, const uint32_t text_len, UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_met, DWRITE_LINE_METRICS*& line_met, UINT32& sele_cnt, DWRITE_HIT_TEST_METRICS*& sele_met, const DWRITE_TEXT_RANGE& sele_rng);
	// 文字列をデータライターに SVG として書き込む.
	static void tx_dt_write_svg(const wchar_t* t, const uint32_t t_len, const double x, const double y, const double dy, DataWriter const& dt_writer);
	// 書体のディセントを得る.
	static void tx_get_font_descent(IDWriteTextLayout* text_layout, float& descent);
	// ヒットテストの計量, 行の計量, 文字列選択の計量を破棄する.
	static void tx_relese_metrics(UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_metrics, DWRITE_LINE_METRICS*& line_metrics, UINT32& sele_cnt, DWRITE_HIT_TEST_METRICS*& sele_metrics) noexcept;

	// ヒットテストの計量を得る.
	// text_lay	文字列レイアウト
	// text_rng	文字範囲
	// test_met	ヒットテストの計量
	// test_cnt	計量の要素数
	static void tx_create_test_metrics(IDWriteTextLayout* text_lay, const DWRITE_TEXT_RANGE text_rng, DWRITE_HIT_TEST_METRICS*& test_met, UINT32& test_cnt)
	{
		const uint32_t pos = text_rng.startPosition;
		const uint32_t len = text_rng.length;
		DWRITE_HIT_TEST_METRICS test[1];

		// 計量の配列要素数を得る.
		// 最初の HitTestTextRange 関数呼び出しは, 失敗することが前提なので, check_hresult しない.
		// 要素数分の配列を確保して, あらためて関数を呼び出し, 計量を得る.
		text_lay->HitTestTextRange(pos, len, 0, 0, test, 1, &test_cnt);
		test_met = new DWRITE_HIT_TEST_METRICS[test_cnt];
		winrt::check_hresult(text_lay->HitTestTextRange(pos, len, 0, 0, test_met, test_cnt, &test_cnt));
	}

	// ヒットテストの計量, 行の計量, 選択された文字範囲の計量を破棄する.
	// test_cnt	ヒットテストおよび行の計量の大きさ
	// test_met	ヒットテストの計量
	// line_met 行の計量
	// sele_cnt	選択された文字範囲の計量の大きさ
	// sele_met 選択された文字範囲の計量
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
		tx_relese_metrics(test_cnt, test_met, line_met, sele_cnt, sele_met);
		if (text_lay != nullptr) {
			tx_create_test_metrics(text_lay, { 0, text_len }, test_met, test_cnt);

			UINT32 line_cnt;
			text_lay->GetLineMetrics(nullptr, 0, &line_cnt);
			line_met = new DWRITE_LINE_METRICS[line_cnt];
			text_lay->GetLineMetrics(line_met, line_cnt, &line_cnt);

			if (sele_rng.length > 0) {
				tx_create_test_metrics(text_lay, sele_rng, sele_met, sele_cnt);
			}
		}
	}

	// 文字列をデータライターに SVG として書き込む.
	// t	文字列
	// t_len	文字数
	// x, y	位置
	// dy	垂直なずらし量
	// dt_writer	データライター
	// 戻り値	なし
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

	// 書体のディセントを得る.
	// text_lay	文字列レイアウト
	// font_des	得られたディセント
	static void tx_get_font_descent(IDWriteTextLayout* text_lay, float& font_des)
	{
		winrt::com_ptr<IDWriteFontCollection> fonts;
		text_lay->GetFontCollection(fonts.put());

		IDWriteFontFamily* fam;
		fonts->GetFontFamily(0, &fam);
		fonts->Release();
		fonts = nullptr;

		IDWriteFont* font;
		fam->GetFont(0, &font);
		fam->Release();
		fam = nullptr;

		DWRITE_FONT_METRICS met;
		font->GetMetrics(&met);
		font->Release();
		font = nullptr;

		const double font_size = text_lay->GetFontSize();
		font_des = static_cast<float>(font_size * met.descent / met.designUnitsPerEm);
	}

	// 図形を破棄する.
	ShapeText::~ShapeText(void)
	{
		m_dw_layout = nullptr;
		tx_relese_metrics(m_dw_test_cnt, m_dw_test_metrics, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics);

		// 書体名がヌルでないか判定する.
		if (m_font_family != nullptr) {
			// 書体名が有効でないか判定する.
			if (!is_available_font(m_font_family)) {
				// 書体名配列に含まれてない名前なのでここで破棄する.
				delete[] m_font_family;
			}
			// ヌルを書体名に格納する.
			m_font_family = nullptr;
		}
		// 文字列が空か判定する.
		if (m_text != nullptr) {
			delete[] m_text;
			m_text = nullptr;
		}
	}

	// 枠の大きさを文字列に合わせる.
	// g_len	方眼の大きさ (1 以上ならば方眼の大きさに合わせる)
	// 戻り値	大きさが調整されたならば真.
	bool ShapeText::adjust_bbox(const float g_len) noexcept
	{
		const float sp = m_text_padding.width * 2.0f;
		D2D1_POINT_2F t_box{ 0.0f, 0.0f };
		for (size_t i = 0; i < m_dw_test_cnt; i++) {
			t_box.x = fmax(t_box.x, m_dw_test_metrics[i].width);
			t_box.y += m_dw_test_metrics[i].height;
		}
		pt_add(t_box, sp, sp, t_box);
		if (g_len >= 1.0f) {
			t_box.x = floor((t_box.x + g_len - 1.0f) / g_len) * g_len;
			t_box.y = floor((t_box.y + g_len - 1.0f) / g_len) * g_len;
		}
		if (!equal(t_box, m_vec[0])) {
			D2D1_POINT_2F se;
			pt_add(m_pos, t_box, se);
			set_pos_anc(se, ANC_TYPE::ANC_SE, 0.0f, false);
			return true;
		}
		return false;
	}

	// 書体や文字列の属性が変更されたか判定する.
	bool ShapeText::is_updated(void) const
	{
		const uint32_t text_len = wchar_len(m_text);
		bool updated = false;
		WCHAR font_family[256];
		winrt::check_hresult(m_dw_layout->GetFontFamilyName(0, font_family, 256));
		if (wcscmp(font_family, m_font_family) != 0) {
			winrt::check_hresult(m_dw_layout->SetFontFamilyName(m_font_family, DWRITE_TEXT_RANGE{ 0, text_len }));
			if (!updated) {
				updated = true;
			}
		}

		FLOAT font_size;
		winrt::check_hresult(m_dw_layout->GetFontSize(0, &font_size));
		if (!equal(font_size, m_font_size)) {
			winrt::check_hresult(m_dw_layout->SetFontSize(m_font_size, DWRITE_TEXT_RANGE{ 0, text_len }));
			if (!updated) {
				updated = true;
			}
		}

		DWRITE_FONT_STRETCH font_stretch;
		winrt::check_hresult(m_dw_layout->GetFontStretch(0, &font_stretch));
		if (!equal(font_stretch, m_font_stretch)) {
			winrt::check_hresult(m_dw_layout->SetFontStretch(m_font_stretch, DWRITE_TEXT_RANGE{ 0, text_len }));
			if (!updated) {
				updated = true;
			}
		}

		DWRITE_FONT_STYLE font_style;
		winrt::check_hresult(m_dw_layout->GetFontStyle(0, &font_style));
		if (!equal(font_style, m_font_style)) {
			winrt::check_hresult(m_dw_layout->SetFontStyle(m_font_style, DWRITE_TEXT_RANGE{ 0, text_len }));
			if (!updated) {
				updated = true;
			}
		}

		DWRITE_FONT_WEIGHT font_weight;
		winrt::check_hresult(m_dw_layout->GetFontWeight(0, &font_weight));
		if (!equal(font_weight, m_font_weight)) {
			winrt::check_hresult(m_dw_layout->SetFontWeight(m_font_weight, DWRITE_TEXT_RANGE{ 0, text_len }));
			if (!updated) {
				updated = true;
			}
		}

		const FLOAT pad_w = static_cast<FLOAT>(max(std::fabs(m_vec[0].x) - m_text_padding.width * 2.0, 0.0));
		const FLOAT pad_h = static_cast<FLOAT>(max(std::fabs(m_vec[0].y) - m_text_padding.height * 2.0, 0.0));
		if (!equal(pad_w, m_dw_layout->GetMaxWidth())) {
			winrt::check_hresult(m_dw_layout->SetMaxWidth(pad_w));
			if (!updated) {
				updated = true;
			}
		}
		if (!equal(pad_h, m_dw_layout->GetMaxHeight())) {
			winrt::check_hresult(m_dw_layout->SetMaxHeight(pad_h));
			if (!updated) {
				updated = true;
			}
		}

		DWRITE_PARAGRAPH_ALIGNMENT para_align = m_dw_layout->GetParagraphAlignment();
		if (!equal(para_align, m_text_align_p)) {
			winrt::check_hresult(m_dw_layout->SetParagraphAlignment(m_text_align_p));
			if (!updated) {
				updated = true;
			}
		}

		DWRITE_TEXT_ALIGNMENT text_align = m_dw_layout->GetTextAlignment();
		if (!equal(text_align, m_text_align_t)) {
			winrt::check_hresult(m_dw_layout->SetTextAlignment(m_text_align_t));
			if (!updated) {
				updated = true;
			}
		}

		winrt::com_ptr<IDWriteTextLayout3> t3;
		if (m_dw_layout.try_as(t3)) {
			DWRITE_LINE_SPACING src_spacing;
			winrt::check_hresult(t3->GetLineSpacing(&src_spacing));
			DWRITE_LINE_SPACING dst_spacing;
			if (m_text_line_sp >= FLT_MIN) {
				float descent = 0.0;
				tx_get_font_descent(m_dw_layout.get(), descent);

				dst_spacing.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
				dst_spacing.height = m_font_size + m_text_line_sp;
				dst_spacing.baseline = m_font_size + m_text_line_sp - descent;
			}
			else {
				dst_spacing.method = DWRITE_LINE_SPACING_METHOD_DEFAULT;
				dst_spacing.height = 0.0f;
				dst_spacing.baseline = 0.0f;
			}
			dst_spacing.leadingBefore = 0.0f;
			dst_spacing.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
			if (memcmp(&src_spacing, &dst_spacing, sizeof(src_spacing)) != 0) {
				winrt::check_hresult(t3->SetLineSpacing(&dst_spacing));
				if (!updated) {
					updated = true;
				}
			}
		}

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
		return updated;
	}

	// 文末の空白を取り除く.
	void ShapeText::delete_bottom_blank(void) noexcept
	{
		if (m_text != nullptr) {
			size_t i = wcslen(m_text);
			while (i-- > 0) {
				if (iswspace(m_text[i]) == 0) {
					break;
				}
				m_text[i] = L'\0';
			}
		}
	}

	// 図形を表示する.
	// d2d	図形の描画環境
	// 戻り値	なし
	void ShapeText::draw(D2D_UI& d2d)
	{
		// 方形を描く.
		ShapeRect::draw(d2d);

		// 文字列が空か判定する.
		if (m_text == nullptr || m_text[0] == L'\0') {
			// 位置の計量, 行の計量, 文字列選択の計量を破棄する.
			tx_relese_metrics(m_dw_test_cnt, m_dw_test_metrics, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics);
			// 文字列レイアウトが空でないなら破棄する.
			if (m_dw_layout == nullptr) {
				m_dw_layout = nullptr;
			}
		}
		else {
			// 文字列レイアウトが空か判定する.
			if (m_dw_layout == nullptr) {
				// 位置の計量, 行の計量, 文字列選択の計量を破棄する.
				tx_relese_metrics(m_dw_test_cnt, m_dw_test_metrics, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics);
				// 文字列フォーマットを作成する.
				wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
				GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);
				winrt::com_ptr<IDWriteTextFormat> t_format;
				// CreateTextFormat で DWRITE_FONT_STRETCH_UNDEFINED が指定された場合,
				// エラーになることがある.
				// 属性値がなんであれ, DWRITE_FONT_STRETCH_NORMAL でテキストフォーマットは作成する.
				winrt::check_hresult(
					d2d.m_dwrite_factory->CreateTextFormat(m_font_family, static_cast<IDWriteFontCollection*>(nullptr),
						m_font_weight, m_font_style, DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL,
						m_font_size, locale_name, t_format.put())
				);
				// 文字列フォーマットから文字列レイアウトを作成する.
				const FLOAT text_w = static_cast<FLOAT>(max(std::fabsf(m_vec[0].x) - 2.0 * m_text_padding.width, 0.0));
				const FLOAT text_h = static_cast<FLOAT>(max(std::fabsf(m_vec[0].y) - 2.0 * m_text_padding.height, 0.0));
				const UINT32 text_len = wchar_len(m_text);
				winrt::check_hresult(d2d.m_dwrite_factory->CreateTextLayout(m_text, text_len, t_format.get(), text_w, text_h, m_dw_layout.put()));
				// 文字列フォーマットを破棄する.
				t_format = nullptr;

				// 文字列レイアウトに文字そろえを格納する.
				winrt::check_hresult(m_dw_layout->SetTextAlignment(m_text_align_t));
				// 文字列レイアウトに段落そろえを格納する.
				winrt::check_hresult(m_dw_layout->SetParagraphAlignment(m_text_align_p));
				// 文字列レイアウトに書体の伸縮をを格納する.
				DWRITE_TEXT_RANGE text_range{ 0, text_len };
				winrt::check_hresult(m_dw_layout->SetFontStretch(m_font_stretch, text_range));
				tx_create_test_metrics(m_dw_layout.get(), text_range, m_dw_test_metrics, m_dw_test_cnt);

				UINT32 line_cnt;
				m_dw_layout->GetLineMetrics(nullptr, 0, &line_cnt);
				m_dw_line_metrics = new DWRITE_LINE_METRICS[line_cnt];
				winrt::check_hresult(m_dw_layout->GetLineMetrics(m_dw_line_metrics, line_cnt, &line_cnt));
				m_text_line_sp = m_dw_line_metrics[0].height - m_font_size;
				tx_create_test_metrics(m_dw_layout.get(), m_text_selected_range, m_dw_selected_metrics, m_dw_selected_cnt);
			}

			// 書体や文字列の属性が変更されたか判定する.
			else if (is_updated()) {
				// 位置の計量, 行の計量, 文字列選択の計量を破棄する.
				tx_relese_metrics(m_dw_test_cnt, m_dw_test_metrics, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics);
				tx_create_text_metrics(m_dw_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_text_selected_range);
			}

			// 文字列レイアウトが IDWriteTextLayout3 に変換できるか判定する.
			winrt::com_ptr<IDWriteTextLayout3> t3;
			if (m_dw_layout.try_as(t3)) {
				float descent = 0.0f;
				tx_get_font_descent(m_dw_layout.get(), descent);

				// 行間を設定する.
				const DWRITE_LINE_SPACING spacing{
					m_text_line_sp >= FLT_MIN ? DWRITE_LINE_SPACING_METHOD_UNIFORM : DWRITE_LINE_SPACING_METHOD_DEFAULT,
					m_text_line_sp >= FLT_MIN ? m_font_size + m_text_line_sp : 0.0f,
					m_text_line_sp >= FLT_MIN ? m_font_size + m_text_line_sp - descent : 0.0f,
					0.0f,
					DWRITE_FONT_LINE_GAP_USAGE_DEFAULT
				};
				winrt::check_hresult(t3->SetLineSpacing(&spacing));
			}

			// 余白分をくわえて, 文字列の左上位置を計算する.
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

			if (m_text_selected_range.length > 0) {

				// 選択された文字範囲を塗りつぶす.
				float descent = 0.0f;
				tx_get_font_descent(m_dw_layout.get(), descent);
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
							d2d.m_solid_color_brush->SetColor(ShapeText::s_text_selected_foreground);
							d2d.m_d2d_context->DrawRectangle(rect, d2d.m_solid_color_brush.get(), 2.0, nullptr);
							d2d.m_solid_color_brush->SetColor(ShapeText::s_text_selected_background);
							d2d.m_d2d_context->FillRectangle(rect, d2d.m_solid_color_brush.get());
							break;
						}
					}
				}
				d2d.m_range_brush->SetColor(ShapeText::s_text_selected_foreground);
				winrt::check_hresult(m_dw_layout->SetDrawingEffect(d2d.m_range_brush.get(), m_text_selected_range));
			}

			// 文字列を表示する
			d2d.m_solid_color_brush->SetColor(m_font_color);
			d2d.m_d2d_context->DrawTextLayout(t_min, m_dw_layout.get(), d2d.m_solid_color_brush.get());
			if (m_text_selected_range.length > 0 && m_text != nullptr) {
				winrt::check_hresult(m_dw_layout->SetDrawingEffect(nullptr, { 0, wchar_len(m_text) }));
			}

			if (is_selected()) {
				// 文字列のフレームを描く
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
				float descent = 0.0f;
				tx_get_font_descent(m_dw_layout.get(), descent);
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
					d2d.m_solid_color_brush->SetColor(ShapeText::s_text_selected_foreground);
					d2d.m_d2d_context->DrawRectangle({ p[0].x, p[0].y, p[2].x, p[2].y }, d2d.m_solid_color_brush.get(), s_width, nullptr);
					d2d.m_solid_color_brush->SetColor(ShapeText::s_text_selected_background);
					s_prop.dashOffset = static_cast<FLOAT>(std::fmod(p[0].x, mod));
					winrt::com_ptr<ID2D1StrokeStyle1> s_style;
					d2d.m_d2d_factory->CreateStrokeStyle(&s_prop, d_arr, d_cnt, s_style.put());
					d2d.m_d2d_context->DrawLine(p[0], p[1], d2d.m_solid_color_brush.get(), s_width, s_style.get());
					d2d.m_d2d_context->DrawLine(p[3], p[2], d2d.m_solid_color_brush.get(), s_width, s_style.get());
					s_style = nullptr;
					s_prop.dashOffset = static_cast<FLOAT>(std::fmod(p[0].y, mod));
					d2d.m_d2d_factory->CreateStrokeStyle(&s_prop, d_arr, d_cnt, s_style.put());
					d2d.m_d2d_context->DrawLine(p[1], p[2], d2d.m_solid_color_brush.get(), s_width, s_style.get());
					d2d.m_d2d_context->DrawLine(p[0], p[3], d2d.m_solid_color_brush.get(), s_width, s_style.get());
					s_style = nullptr;
				}
			}
		}
	}

	// 要素を有効な書体名から得る.
	wchar_t* ShapeText::get_available_font(const uint32_t i)
	{
		return s_available_fonts[i];
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

	// 書体の伸縮を得る.
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
	bool ShapeText::get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_p;
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
		if (anc != ANC_TYPE::ANC_SHEET) {
			return anc;
		}
		float descent = 0.0f;
		tx_get_font_descent(m_dw_layout.get(), descent);
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
			float descent = 0.0f;
			tx_get_font_descent(m_dw_layout.get(), descent);

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
	// 有効なら, 引数の書体名を破棄し, 有効な書体名の配列の要素と置き換える.
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

	// 有効な書体名の配列を破棄する.
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

	// 有効な書体名の配列を設定する.
	//
	// DWriteFactory のシステムフォントコレクションから,
	// 既定の地域・言語名に対応した書体を得て,
	// それらを配列に格納する.
	void ShapeText::set_available_fonts(const D2D_UI& d2d)
	{
		// 既定の地域・言語名を得る.
		wchar_t lang[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(lang, LOCALE_NAME_MAX_LENGTH);
		// システムフォントコレクションを DWriteFactory から得る.
		winrt::com_ptr<IDWriteFontCollection> collection;
		winrt::check_hresult(d2d.m_dwrite_factory->GetSystemFontCollection(collection.put()));
		//winrt::check_hresult(Shape::s_dwrite_factory->GetSystemFontCollection(collection.put()));
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
			// ローカライズされた書体名から, 地域名をのぞいた書体名の開始位置を得る.
			UINT32 index = 0;
			BOOL exists = false;
			winrt::check_hresult(localized_name->FindLocaleName(lang, &index, &exists));
			if (exists != TRUE) {
				// 地域名がない場合,
				// 0 を開始位置に格納する.
				index = 0;
			}
			// 開始位置より後ろの文字数を得る (ヌル文字は含まれない).
			UINT32 length;
			winrt::check_hresult(localized_name->GetStringLength(index, &length));
			// 文字数 + 1 の文字配列を確保し, 書体名の配列に格納する.
			s_available_fonts[i] = new wchar_t[static_cast<size_t>(length) + 1];
			winrt::check_hresult(localized_name->GetString(index, s_available_fonts[i], length + 1));
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
			if (m_dw_layout != nullptr) {
				m_dw_layout = nullptr;
			}
			return true;
		}
		return false;
	}

	// 値を段落のそろえに格納する.
	bool ShapeText::set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept
	{
		if (m_text_align_p != val) {
			m_text_align_p = val;
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
	// s_attr	属性
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
		m_text_selected_range(DWRITE_TEXT_RANGE{ 0, 0 }),
		m_dw_layout(nullptr)
	{
	}

	// 図形をデータライターから読み込む.
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
		m_dw_layout = nullptr;
	}

	// データライターに書き込む.
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

	// データライターに SVG タグとして書き込む.
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
		// 垂直方向のずらし量を求める.
		//
		// Chrome では, テキストタグに属性 alignment-baseline="text-before-edge" 
		// を指定するだけで, 左上位置を基準にして Dwrite と同じように表示される.
		// しかし, IE や Edge では, alignment-baseline 属性は期待した働きをしないので,
		// 上部からのベースラインまで値である, 垂直方向のずらし量 dy を
		// 行ごとに計算を必要がある.
		// このとき, 上部からのベースラインの高さ = アセントにはならないので
		// デセントを用いて計算する必要もある.
		// テキストレイアウトからフォントメトリックスを取得して, 以下のように求める.
		// ちなみに, designUnitsPerEm は, 配置 (Em) ボックスの単位あたりの大きさ.
		// デセントは, フォント文字の配置ボックスの下部からベースラインまでの長さ.
		// dy = その行のヒットテストメトリックスの高さ - フォントの大きさ × (デセント ÷ 単位大きさ) となる, はず.
		IDWriteFontCollection* fonts;
		m_dw_layout->GetFontCollection(&fonts);
		IDWriteFontFamily* family;
		fonts->GetFontFamily(0, &family);
		IDWriteFont* font;
		family->GetFont(0, &font);
		DWRITE_FONT_METRICS metrics;
		font->GetMetrics(&metrics);
		//const double descent = m_font_size * ((static_cast<double>(metrics.descent)) / metrics.designUnitsPerEm);

		if (is_opaque(m_fill_color) || is_opaque(m_stroke_color)) {
			ShapeRect::write_svg(dt_writer);
		}
		// 文字列全体の属性を指定するための g タグを開始する.
		dt_write_svg("<g ", dt_writer);
		// 書体の色を書き込む.
		dt_write_svg(m_font_color, "fill", dt_writer);
		// 書体名を書き込む.
		dt_write_svg("font-family=\"", dt_writer);
		dt_write_svg(m_font_family, wchar_len(m_font_family), dt_writer);
		dt_write_svg("\" ", dt_writer);
		// 書体の大きさを書き込む.
		dt_write_svg(m_font_size, "font-size", dt_writer);
		// 書体の伸縮を書き込む.
		const int32_t stretch = static_cast<int32_t>(m_font_stretch);
		dt_write_svg(SVG_STRETCH[stretch], "font-stretch", dt_writer);
		// 書体の形式を書き込む.
		const int32_t style = static_cast<int32_t>(m_font_style);
		dt_write_svg(SVG_STYLE[style], "font-style", dt_writer);
		// 書体の太さを書き込む.
		const uint32_t weight = static_cast<uint32_t>(m_font_weight);
		dt_write_svg(weight, "font-weight", dt_writer);
		dt_write_svg("none", "stroke", dt_writer);
		dt_write_svg(">" SVG_NEW_LINE, dt_writer);
		// 書体を表示する左上位置に余白を加える.
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
			// 文字列を表示する垂直なずらし位置を求める.
			const double dy = static_cast<double>(m_dw_line_metrics[i].baseline);
			// 文字列を書き込む.
			tx_dt_write_svg(t, t_len, px + qx, py + qy, dy, dt_writer);
		}
		dt_write_svg("</g>" SVG_NEW_LINE, dt_writer);
	}

}
