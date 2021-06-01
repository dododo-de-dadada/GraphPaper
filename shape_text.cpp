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

	// テキストレイアウトから, ヒットテストのための計量の配列を得る.
	static void tx_create_test_metrics(IDWriteTextLayout* text_layout, const DWRITE_TEXT_RANGE text_range, DWRITE_HIT_TEST_METRICS*& test_metrics, UINT32& test_count);
	// テキストレイアウトから, 計量の配列を得る.
	static void tx_create_text_metrics(IDWriteTextLayout* text_layout, const uint32_t text_len, UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_metrics, UINT32& line_cnt, DWRITE_LINE_METRICS*& line_metrics, UINT32& range_cnt, DWRITE_HIT_TEST_METRICS*& range_metrics, float& descent, const DWRITE_TEXT_RANGE& sel_range);
	// 文字列をデータライターに SVG として書き込む.
	static void tx_write_svg(const wchar_t* t, const uint32_t t_len, const double x, const double y, const double dy, DataWriter const& dt_writer);
	// 書体のディセントをテキストレイアウトから得る.
	static void tx_get_font_descent(IDWriteTextLayout* text_layout, float& descent);

	// ヒットテストのための計量の配列をテキストレイアウトから得る.
	// text_layout	文字列レイアウト
	// text_range	文字列の範囲
	// test_metrics	ヒットテストのための計量
	// test_count	計量の要素数
	static void tx_create_test_metrics(
		IDWriteTextLayout* text_layout,
		const DWRITE_TEXT_RANGE text_range,
		DWRITE_HIT_TEST_METRICS*& test_metrics,
		UINT32& test_count)
	{
		const uint32_t pos = text_range.startPosition;
		const uint32_t len = text_range.length;
		DWRITE_HIT_TEST_METRICS test[1];

		// 失敗することが前提なので, 最初の HitTestTextRange 関数
		// 呼び出しは, check_hresult しない.
		text_layout->HitTestTextRange(pos, len, 0, 0, test, 1, &test_count);
		// 配列を確保して, 関数をあらためて呼び出す.
		test_metrics = new DWRITE_HIT_TEST_METRICS[test_count];
		winrt::check_hresult(text_layout->HitTestTextRange(pos, len, 0, 0, test_metrics, test_count, &test_count));
	}

	// 計量の配列をテキストレイアウトから得る.
	// text_layout	文字列レイアウト
	// text_len	文字列の長さ
	// test_cnt	ヒットテストの計量の要素数
	// test_metrics	ヒットテストの計量
	// line_cnt	行数
	// line_metrics 行の計量
	// range_cnt	選択範囲の計量の要素数
	// range_metrics 選択範囲の計量
	// font_descent	書体の高さ
	// sel_range	選択された範囲
	static void tx_create_text_metrics(
		IDWriteTextLayout* text_layout,
		const uint32_t text_len,
		UINT32& test_cnt, DWRITE_HIT_TEST_METRICS*& test_metrics,
		UINT32& line_cnt, DWRITE_LINE_METRICS*& line_metrics,
		UINT32& range_cnt, DWRITE_HIT_TEST_METRICS*& range_metrics,
		float& font_descent,
		const DWRITE_TEXT_RANGE& sel_range)
	{
		//using winrt::GraphPaper::implementation::create_test_metrics;

		test_cnt = 0;
		if (test_metrics != nullptr) {
			delete[] test_metrics;
			test_metrics = nullptr;
		}
		line_cnt = 0;
		if (line_metrics != nullptr) {
			delete[] line_metrics;
			line_metrics = nullptr;
		}
		range_cnt = 0;
		if (range_metrics != nullptr) {
			delete[] range_metrics;
			range_metrics = nullptr;
		}
		font_descent = 0.0f;
		if (text_layout != nullptr) {
			tx_create_test_metrics(text_layout, { 0, text_len }, test_metrics, test_cnt);

			tx_get_font_descent(text_layout, font_descent);

			text_layout->GetLineMetrics(nullptr, 0, &line_cnt);
			line_metrics = new DWRITE_LINE_METRICS[line_cnt];
			text_layout->GetLineMetrics(line_metrics, line_cnt, &line_cnt);

			if (sel_range.length > 0) {
				// 
				tx_create_test_metrics(text_layout, sel_range, range_metrics, range_cnt);
			}
		}
	}

	// 書体のディセントをテキストレイアウトから得る.
	//	text_layout	文字列レイアウト
	//	descent	得られたディセント
	static void tx_get_font_descent(IDWriteTextLayout* text_layout, float& font_descent)
	{
		winrt::com_ptr<IDWriteFontCollection> fonts;
		text_layout->GetFontCollection(fonts.put());
		IDWriteFontFamily* family;
		fonts->GetFontFamily(0, &family);
		fonts->Release();
		fonts = nullptr;
		IDWriteFont* font;
		family->GetFont(0, &font);
		family->Release();
		family = nullptr;
		DWRITE_FONT_METRICS metrics;
		font->GetMetrics(&metrics);
		font = nullptr;
		const auto font_size = text_layout->GetFontSize();
		font_descent = static_cast<float>(static_cast<double>(font_size) * metrics.descent / metrics.designUnitsPerEm);
	}

	// 図形を破棄する.
	ShapeText::~ShapeText(void)
	{
		m_dw_layout = nullptr;
		m_dw_test_cnt = 0;
		if (m_dw_test_metrics != nullptr) {
				delete[] m_dw_test_metrics;
			m_dw_test_metrics = nullptr;
		}
		m_dw_line_cnt = 0;
		if (m_dw_line_metrics != nullptr) {
			delete[] m_dw_line_metrics;
			m_dw_line_metrics = nullptr;
		}
		m_dw_descent = 0.0f;
		m_dw_selected_cnt = 0;
		if (m_dw_selected_metrics != nullptr) {
			delete[] m_dw_selected_metrics;
			m_dw_selected_metrics = nullptr;
		}
		if (m_font_family != nullptr) {
			if (is_available_font(m_font_family) != true) {
				// 有効な書体名でない場合,
				// 配列に含まれていない書体名なので破棄する.
				delete[] m_font_family;
			}
			m_font_family = nullptr;
		}
		if (m_text != nullptr) {
			delete[] m_text;
			m_text = nullptr;
		}
	}

	//	大きさを, それが文字列より小さくならないように, 調整する. 
	//	bound	調整前の大きさ, 調整後の大きさ.
	//	戻り値	大きさが調整されたならば真.
	bool ShapeText::adjust_bbox(const D2D1_SIZE_F& bound)
	{
		auto diff = m_diff[0];
		// 改行コードの数を求め, 最小の行数に格納する.
		uint32_t min_line_cnt = 1;
		for (uint32_t i = 0; i < m_dw_line_cnt; i++) {
			if (m_dw_line_metrics[i].newlineLength > 0) {
				min_line_cnt++;
			}
		}
		auto line_cnt = 0U;	// 行数 (改行コードによる改行および文字列が配置される方形の幅による改行をあわせた数)
		do {
			auto bound_height = 0.0;	// 文字列が配置される方形の高さ
			auto bound_width = 0.0;	// 文字列が配置される方形の幅
			auto new_line_pos = 0U;	// 改行コードによって改行された文字位置
			auto i = 0U;
			auto j = 0U;
			while (i < m_dw_line_cnt && j < m_dw_test_cnt) {
				// 改行の文字位置と行の高さを得る.
				auto line_height = 0.0;
				while (i < m_dw_line_cnt) {
					new_line_pos += m_dw_line_metrics[i].length;
					line_height = fmax(line_height, m_dw_line_metrics[i].height);
					if (m_dw_line_metrics[i++].newlineLength > 0) {
						break;
					}
				}
				// 高さに行ごとの高さを加える.
				bound_height += line_height;
				// 行ごとの幅を求める.
				auto line_width = 0.0;
				while (j < m_dw_test_cnt) {
					line_width += m_dw_test_metrics[j].width;
					if (m_dw_test_metrics[j].textPosition + m_dw_test_metrics[j++].length >= new_line_pos) {
						// 位置の計量ごとの終了位置が改行の文字位置に達した場合
						break;
					}
				}
				bound_width = fmax(bound_width, line_width);
			}
			bound_width += m_text_margin.width * 2.0;
			if (bound.width > FLT_MIN) {
				bound_width = fmin(bound_width, bound.width);
			}
			bound_height += m_text_margin.height * 2.0;
			if (bound.height > FLT_MIN) {
				bound_height = fmin(bound_height, bound.height);
			}
			// 行数を保存する.
			line_cnt = m_dw_line_cnt;
			D2D1_POINT_2F s_pos;
			get_start_pos(s_pos);
			pt_add(s_pos, bound_width, bound_height, s_pos);
			set_anchor_pos(s_pos, ANCH_TYPE::ANCH_SE);
		} while (m_dw_line_cnt < line_cnt && m_dw_line_cnt > min_line_cnt);
		// 行数が, 保存された行数より小さい, かつ最小の行数より大きい場合
		return equal(diff, m_diff[0]) != true;
	}

	// テキストレイアウトを破棄して作成する.
	void ShapeText::create_text_layout(IDWriteFactory3* d_factory)
	{
		m_dw_layout = nullptr;
		m_dw_test_cnt = 0;
		if (m_dw_test_metrics != nullptr) {
			delete[] m_dw_test_metrics;
			m_dw_test_metrics = nullptr;
		}
		m_dw_line_cnt = 0;
		if (m_dw_line_metrics != nullptr) {
			delete[] m_dw_line_metrics;
			m_dw_line_metrics = nullptr;
		}
		m_dw_selected_cnt = 0;
		if (m_dw_selected_metrics != nullptr) {
			delete[] m_dw_selected_metrics;
			m_dw_selected_metrics = nullptr;
		}
		const uint32_t text_len = wchar_len(m_text);
		if (text_len == 0) {
			return;
		}
		wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);
		winrt::com_ptr<IDWriteTextFormat> t_format;
		// CreateTextFormat で,
		// DWRITE_FONT_STRETCH_UNDEFINED が指定された場合, エラーになることがある.
		// 属性値がなんであれ, DWRITE_FONT_STRETCH_NORMAL でテキストフォーマットは作成する.
		winrt::check_hresult(
			d_factory->CreateTextFormat(m_font_family, static_cast<IDWriteFontCollection*>(nullptr),
				m_font_weight, m_font_style, DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL,
				m_font_size, locale_name, t_format.put())
		);
		const auto text_w = static_cast<FLOAT>(max(std::fabsf(m_diff[0].x) - 2.0 * m_text_margin.width, 0.0));
		const auto text_h = static_cast<FLOAT>(max(std::fabsf(m_diff[0].y) - 2.0 * m_text_margin.height, 0.0));
		winrt::check_hresult(d_factory->CreateTextLayout(m_text, text_len, t_format.get(), text_w, text_h, m_dw_layout.put()));
		t_format = nullptr;
		winrt::com_ptr<IDWriteTextLayout3> t3;
		if (m_dw_layout.try_as(t3)) {
			DWRITE_LINE_SPACING spacing;
			if (m_text_line_h > FLT_MIN) {
				spacing.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
				spacing.height = m_text_line_h;
				spacing.baseline = m_text_line_h - m_dw_descent;
			}
			else {
				spacing.method = DWRITE_LINE_SPACING_METHOD_DEFAULT;
				spacing.height = 0.0f;
				spacing.baseline = 0.0f;
			}
			spacing.leadingBefore = 0.0f;
			spacing.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
			t3->SetLineSpacing(&spacing);
		}
		m_dw_layout->SetTextAlignment(m_text_align_t);
		m_dw_layout->SetParagraphAlignment(m_text_align_p);
		DWRITE_TEXT_RANGE test_range{ 0, text_len };
		m_dw_layout->SetFontStretch(m_font_stretch, test_range);
		tx_create_test_metrics(m_dw_layout.get(), test_range, m_dw_test_metrics, m_dw_test_cnt);
		tx_get_font_descent(m_dw_layout.get(), m_dw_descent);
		m_dw_layout->GetLineMetrics(nullptr, 0, &m_dw_line_cnt);
		m_dw_line_metrics = new DWRITE_LINE_METRICS[m_dw_line_cnt];
		m_dw_layout->GetLineMetrics(m_dw_line_metrics, m_dw_line_cnt, &m_dw_line_cnt);
		tx_create_test_metrics(m_dw_layout.get(), m_select_range, m_dw_selected_metrics, m_dw_selected_cnt);
	}

	// 計量を破棄して作成する.
	void ShapeText::create_text_metrics(IDWriteFactory3* d_factory)
	{
		if (m_text != nullptr && m_text[0] != '\0') {
			if (m_dw_layout.get() == nullptr) {
				create_text_layout(d_factory);
			}
			else {
				const FLOAT margin_w = static_cast<FLOAT>(max(std::fabs(m_diff[0].x) - m_text_margin.width * 2.0, 0.0));
				const FLOAT margin_h = static_cast<FLOAT>(max(std::fabs(m_diff[0].y) - m_text_margin.height * 2.0, 0.0));
				bool flag = false;
				if (equal(margin_w, m_dw_layout->GetMaxWidth()) != true) {
					m_dw_layout->SetMaxWidth(margin_w);
					flag = true;
				}
				if (equal(margin_h, m_dw_layout->GetMaxHeight()) != true) {
					m_dw_layout->SetMaxHeight(margin_h);
					flag = true;
				}
				if (flag) {
					tx_create_text_metrics(m_dw_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_cnt, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_dw_descent, m_select_range);
				}
			}
		}
	}

	// 文末の空白を取り除く.
	void ShapeText::delete_bottom_blank(void) noexcept
	{
		if (m_text != nullptr) {
			auto i = wcslen(m_text);
			while (i-- > 0) {
				if (iswspace(m_text[i]) == 0) {
					break;
				}
				m_text[i] = L'\0';
			}
		}
	}

	void ShapeText::fill_range(SHAPE_DX& dx, const D2D1_POINT_2F t_min)
	{
		const auto rc = m_dw_selected_cnt;
		const auto dc = dx.m_d2dContext.get();
		const auto tc = m_dw_test_cnt;
		for (uint32_t i = 0; i < rc; i++) {
			const auto& rm = m_dw_selected_metrics[i];
			for (uint32_t j = 0; j < tc; j++) {
				const auto& tm = m_dw_test_metrics[j];
				const auto& lm = m_dw_line_metrics[j];
				if (tm.textPosition <= rm.textPosition && rm.textPosition + rm.length <= tm.textPosition + tm.length) {
					D2D1_RECT_F rect;
					rect.left = t_min.x + rm.left;
					rect.top = static_cast<FLOAT>(t_min.y + tm.top + lm.baseline + m_dw_descent - m_font_size);
					if (rm.width <= FLT_MIN) {
						const float sp_len = max(lm.trailingWhitespaceLength * m_font_size * 0.25f, 1.0f);
						rect.right = rect.left + sp_len;
					}
					else {
						rect.right = rect.left + rm.width;
					}
					rect.bottom = rect.top + m_font_size;
					dx.m_shape_brush->SetColor(dx.m_range_foreground);
					dc->DrawRectangle(rect, dx.m_shape_brush.get(), 2.0, nullptr);
					dx.m_shape_brush->SetColor(dx.m_range_background);
					dc->FillRectangle(rect, dx.m_shape_brush.get());
					break;
				}
			}
		}
		dx.m_range_brush->SetColor(dx.m_range_foreground);
		m_dw_layout->SetDrawingEffect(dx.m_range_brush.get(), m_select_range);
	}

	// 文字列の枠を表示する.
	void ShapeText::draw_frame(SHAPE_DX& dx, const D2D1_POINT_2F t_min)
	{
		const auto d_cnt = dx.m_aux_style->GetDashesCount();
		if (d_cnt <= 0 || d_cnt > 6) {
			return;
		}
		const auto dc = dx.m_d2dContext.get();
		D2D1_MATRIX_3X2_F tran;
		dc->GetTransform(&tran);
		auto s_width = static_cast<FLOAT>(1.0 / tran.m11);
		D2D1_STROKE_STYLE_PROPERTIES s_prop;
		s_prop.dashCap = dx.m_aux_style->GetDashCap();
		s_prop.dashOffset = dx.m_aux_style->GetDashOffset();
		s_prop.dashStyle = dx.m_aux_style->GetDashStyle();
		s_prop.endCap = dx.m_aux_style->GetEndCap();
		s_prop.lineJoin = dx.m_aux_style->GetLineJoin();
		s_prop.miterLimit = dx.m_aux_style->GetMiterLimit();
		s_prop.startCap = dx.m_aux_style->GetStartCap();
		FLOAT d_arr[6];
		dx.m_aux_style->GetDashes(d_arr, d_cnt);
		double mod = d_arr[0];
		for (uint32_t i = 1; i < d_cnt; i++) {
			mod += d_arr[i];
		}
		ID2D1Factory* fa;
		dx.m_aux_style->GetFactory(&fa);
		for (uint32_t i = 0; i < m_dw_test_cnt; i++) {
			auto const& tm = m_dw_test_metrics[i];
			auto const& lm = m_dw_line_metrics[i];
			// 破線がずれて重なって表示されないように, 破線のオフセットを設定し,
			// 文字列の枠を辺ごとに表示する.
			D2D1_POINT_2F p[4];
			p[0].x = t_min.x + tm.left;
			p[0].y = static_cast<FLOAT>(t_min.y + tm.top + lm.baseline + m_dw_descent - m_font_size);
			p[2].x = p[0].x + tm.width;
			p[2].y = p[0].y + m_font_size;
			p[1].x = p[2].x;
			p[1].y = p[0].y;
			p[3].x = p[0].x;
			p[3].y = p[2].y;
			dx.m_shape_brush->SetColor(dx.m_range_foreground);
			dc->DrawRectangle({ p[0].x, p[0].y, p[2].x, p[2].y }, dx.m_shape_brush.get(), s_width, nullptr);
			dx.m_shape_brush->SetColor(dx.m_range_background);
			s_prop.dashOffset = static_cast<FLOAT>(std::fmod(p[0].x, mod));
			winrt::com_ptr<ID2D1StrokeStyle> s_style;
			fa->CreateStrokeStyle(&s_prop, d_arr, d_cnt, s_style.put());
			dc->DrawLine(p[0], p[1], dx.m_shape_brush.get(), s_width, s_style.get());
			dc->DrawLine(p[3], p[2], dx.m_shape_brush.get(), s_width, s_style.get());
			s_style = nullptr;
			s_prop.dashOffset = static_cast<FLOAT>(std::fmod(p[0].y, mod));
			fa->CreateStrokeStyle(&s_prop, d_arr, d_cnt, s_style.put());
			dc->DrawLine(p[1], p[2], dx.m_shape_brush.get(), s_width, s_style.get());
			dc->DrawLine(p[0], p[3], dx.m_shape_brush.get(), s_width, s_style.get());
			s_style = nullptr;
		}

	}

	// 図形を表示する.
	//	dx	図形の描画環境
	//	戻り値	なし
	void ShapeText::draw(SHAPE_DX& dx)
	{
		ShapeRect::draw(dx);
		if (m_dw_layout == nullptr) {
			// || m_text == nullptr || m_text[0] == '\0') {
			return;
		}
		D2D1_POINT_2F t_min;
		pt_add(m_pos, m_diff[0], t_min);
		pt_min(m_pos, t_min, t_min);
		auto hm = min(m_text_margin.width, fabs(m_diff[0].x) * 0.5);
		auto vm = min(m_text_margin.height, fabs(m_diff[0].y) * 0.5);
		pt_add(t_min, hm, vm, t_min);
//uint32_t line_cnt;
//m_dw_layout->GetLineMetrics(nullptr, 0, &line_cnt);
//auto l_met = new DWRITE_LINE_METRICS[line_cnt];
//m_dw_layout->GetLineMetrics(l_met, line_cnt, &line_cnt);
		if (m_select_range.length > 0 && m_text != nullptr) {
			fill_range(dx, t_min);
		}
		dx.m_shape_brush->SetColor(m_font_color);
		dx.m_d2dContext->DrawTextLayout(t_min, m_dw_layout.get(), dx.m_shape_brush.get());
		if (m_select_range.length > 0 && m_text != nullptr) {
			m_dw_layout->SetDrawingEffect(nullptr, { 0, wchar_len(m_text) });
		}
		if (is_selected() != true) {
			return;
		}
		draw_frame(dx, t_min);
	}

	// 要素を有効な書体名から得る.
	wchar_t* ShapeText::get_available_font(const uint32_t i)
	{
		return s_available_fonts[i];
	}

	// 書体の色を得る.
	bool ShapeText::get_font_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_font_color;
		return true;
	}

	// 書体名を得る.
	bool ShapeText::get_font_family(wchar_t*& value) const noexcept
	{
		value = m_font_family;
		return true;
	}

	// 書体の大きさを得る.
	bool ShapeText::get_font_size(float& value) const noexcept
	{
		value = m_font_size;
		return true;
	}

	// 書体の伸縮を得る.
	bool ShapeText::get_font_stretch(DWRITE_FONT_STRETCH& value) const noexcept
	{
		value = m_font_stretch;
		return true;
	}

	// 書体の字体を得る.
	bool ShapeText::get_font_style(DWRITE_FONT_STYLE& value) const noexcept
	{
		value = m_font_style;
		return true;
	}

	// 書体の太さを得る.
	bool ShapeText::get_font_weight(DWRITE_FONT_WEIGHT& value) const noexcept
	{
		value = m_font_weight;
		return true;
	}

	// 文字列を得る.
	bool ShapeText::get_text(wchar_t*& value) const noexcept
	{
		value = m_text;
		return true;
	}

	// 段落のそろえを得る.
	bool ShapeText::get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& value) const noexcept
	{
		value = m_text_align_p;
		return true;
	}

	// 文字列のそろえを得る.
	bool ShapeText::get_text_align_t(DWRITE_TEXT_ALIGNMENT& value) const noexcept
	{
		value = m_text_align_t;
		return true;
	}

	// 行間を得る.
	bool ShapeText::get_text_line(float& value) const noexcept
	{
		value = m_text_line_h;
		return true;
	}

	// 文字列の余白を得る.
	bool ShapeText::get_text_margin(D2D1_SIZE_F& value) const noexcept
	{
		value = m_text_margin;
		return true;
	}

	// 文字範囲を得る
	bool ShapeText::get_text_range(DWRITE_TEXT_RANGE& value) const noexcept
	{
		value = m_select_range;
		return true;
	}

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeText::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const auto anchor = ShapeRect::hit_test_anchor(t_pos, a_len);
		if (anchor != ANCH_TYPE::ANCH_SHEET) {
			return anchor;
		}
		// 文字列の範囲の左上が原点になるよう, 判定する位置を移動する.
		D2D1_POINT_2F p_min;
		ShapeStroke::get_min_pos(p_min);
		pt_sub(t_pos, p_min, p_min);
		pt_sub(p_min, m_text_margin, p_min);
		for (uint32_t i = 0; i < m_dw_test_cnt; i++) {
			auto const& tm = m_dw_test_metrics[i];
			auto const& lm = m_dw_line_metrics[i];
			D2D1_POINT_2F r_max{ tm.left + tm.width, tm.top + lm.baseline + m_dw_descent };
			D2D1_POINT_2F r_min{ tm.left, r_max.y - m_font_size };
			if (pt_in_rect(p_min, r_min, r_max)) {
				return ANCH_TYPE::ANCH_TEXT;
			}
		}
		return ShapeRect::hit_test(t_pos, a_len);
	}

	// 範囲に含まれるか判定する.
	// a_min	範囲の左上位置
	// a_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeText::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		D2D1_POINT_2F p_min;
		D2D1_POINT_2F h_min;
		D2D1_POINT_2F h_max;

		if (m_dw_test_cnt > 0) {
			ShapeStroke::get_min_pos(p_min);
			for (uint32_t i = 0; i < m_dw_test_cnt; i++) {
				auto const& tm = m_dw_test_metrics[i];
				auto const& lm = m_dw_line_metrics[i];
				auto const top = tm.top + lm.baseline + m_dw_descent - m_font_size;
				pt_add(p_min, tm.left, top, h_min);
				if (pt_in_rect(h_min, a_min, a_max) != true) {
					return false;
				}
				pt_add(h_min, tm.width, m_font_size, h_max);
				if (pt_in_rect(h_max, a_min, a_max) != true) {
					return false;
				}
			}
		}
		return ShapeRect::in_area(a_min, a_max);
	}

	// 書体名が有効か判定する.
	// font	書体名
	// 戻り値	有効なら true
	// 有効なら, 引数の書体名を破棄し, 有効な書体名の配列の要素と置き換える.
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

	// 有効な書体名の配列を破棄する.
	void ShapeText::release_available_fonts(void)
	{
		if (s_available_fonts == nullptr) {
			return;
		}
		for (uint32_t i = 0; s_available_fonts[i]; i++) {
			delete[] s_available_fonts[i];
		}
		delete[] s_available_fonts;
		s_available_fonts = nullptr;
	}

	// 有効な書体名の配列を設定する.
	//
	// DWriteFactory のシステムフォントコレクションから,
	// 既定の地域・言語名に対応した書体を得て,
	// それらを配列に格納する.
	void ShapeText::set_available_fonts(void)
	{
		// 既定の地域・言語名を得る.
		wchar_t lang[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(lang, LOCALE_NAME_MAX_LENGTH);
		// システムフォントコレクションを DWriteFactory から得る.
		winrt::com_ptr<IDWriteFontCollection> collection;
		winrt::check_hresult(Shape::s_dwrite_factory->GetSystemFontCollection(collection.put()));
		// フォントコレクションの要素数を得る.
		const auto f_cnt = collection->GetFontFamilyCount();
		// 得られた要素数 + 1 の配列を確保する.
		s_available_fonts = new wchar_t* [static_cast<size_t>(f_cnt) + 1];
		// フォントコレクションの各要素について.
		for (uint32_t i = 0; i < f_cnt; i++) {
			// 要素から書体を得る.
			winrt::com_ptr<IDWriteFontFamily> font_family;
			winrt::check_hresult(
				collection->GetFontFamily(i, font_family.put())
			);
			// 書体からローカライズされた書体名を得る.
			winrt::com_ptr<IDWriteLocalizedStrings> localized_name;
			winrt::check_hresult(
				font_family->GetFamilyNames(localized_name.put())
			);
			// ローカライズされた書体名から, 地域名をのぞいた書体名の開始位置を得る.
			UINT32 index = 0;
			BOOL exists = false;
			winrt::check_hresult(
				localized_name->FindLocaleName(lang, &index, &exists)
			);
			if (exists != TRUE) {
				// 地域名がない場合,
				// 0 を開始位置に格納する.
				index = 0;
			}
			// 開始位置より後ろの文字数を得る (ヌル文字は含まれない).
			UINT32 length;
			winrt::check_hresult(
				localized_name->GetStringLength(index, &length)
			);
			// 文字数 + 1 の文字配列を確保し, 書体名の配列に格納する.
			s_available_fonts[i] = new wchar_t[static_cast<size_t>(length) + 1];
			winrt::check_hresult(
				localized_name->GetString(index, s_available_fonts[i], length + 1)
			);
			// ローカライズされた書体名を破棄する.
			localized_name = nullptr;
			// 書体をを破棄する.
			font_family = nullptr;
		}
		// 有効な書体名の配列の末尾に終端としてヌルを格納する.
		s_available_fonts[f_cnt] = nullptr;
	}

	// 値を書体の色に格納する.
	void ShapeText::set_font_color(const D2D1_COLOR_F& value) noexcept
	{
		m_font_color = value;
	}

	// 値を書体名に格納する.
	void ShapeText::set_font_family(wchar_t* const value)
	{
		// 値が書体名と同じか判定する.
		if (equal(m_font_family, value)) {
			// 同じなら終了する.
			return;
		}
		m_font_family = value;
		if (m_dw_layout.get() != nullptr) {
			const uint32_t text_len = wchar_len(m_text);
			const DWRITE_TEXT_RANGE t_range{ 0, text_len };
			m_dw_layout->SetFontFamilyName(m_font_family, t_range);
			tx_create_text_metrics(m_dw_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_cnt, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_dw_descent, m_select_range);
		}
		else {
			create_text_layout(s_dwrite_factory);
		}

	}

	// 値を書体の大きさに格納する.
	void ShapeText::set_font_size(const float value)
	{
		if (equal(m_font_size, value)) {
			return;
		}
		m_font_size = value;
		if (m_dw_layout.get() != nullptr) {
			//const FLOAT z = value;
			const uint32_t text_len = wchar_len(m_text);
			m_dw_layout->SetFontSize(value, { 0, text_len });
			tx_create_text_metrics(m_dw_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_cnt, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_dw_descent, m_select_range);
		}
		else {
			create_text_layout(s_dwrite_factory);
		}
	}

	// 値を書体の横幅に格納する.
	void ShapeText::set_font_stretch(const DWRITE_FONT_STRETCH value)
	{
		if (m_font_stretch == value) {
			return;
		}
		m_font_stretch = value;
		if (m_dw_layout.get() != nullptr) {
			const uint32_t text_len = wchar_len(m_text);
			m_dw_layout->SetFontStretch(value, { 0, text_len });
			tx_create_text_metrics(m_dw_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_cnt, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_dw_descent, m_select_range);
		}
		else {
			create_text_layout(s_dwrite_factory);
		}
	}

	// 値を書体の字体に格納する.
	void ShapeText::set_font_style(const DWRITE_FONT_STYLE value)
	{
		if (m_font_style == value) {
			return;
		}
		m_font_style = value;
		if (m_dw_layout.get() != nullptr) {
			const uint32_t text_len = wchar_len(m_text);
			m_dw_layout->SetFontStyle(value, { 0, text_len });
			tx_create_text_metrics(m_dw_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_cnt, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_dw_descent, m_select_range);
		}
		else {
			create_text_layout(s_dwrite_factory);
		}
	}

	// 値を書体の太さに格納する.
	void ShapeText::set_font_weight(const DWRITE_FONT_WEIGHT value)
	{
		if (m_font_weight == value) {
			return;
		}
		m_font_weight = value;
		if (m_dw_layout.get() != nullptr) {
			const uint32_t text_len = wchar_len(m_text);
			m_dw_layout->SetFontWeight(value, { 0, text_len });
			tx_create_text_metrics(m_dw_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_cnt, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_dw_descent, m_select_range);
		}
		else {
			create_text_layout(s_dwrite_factory);
		}
	}

	//	値を, 部位の位置に格納する. 他の部位の位置は動かない. 
	//	value	格納する値
	//	abch	図形の部位
	void ShapeText::set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch)
	{
		ShapeRect::set_anchor_pos(value, anch);
		create_text_metrics(s_dwrite_factory);
	}

	// 値を文字列に格納する.
	void ShapeText::set_text(wchar_t* const value)
	{
		if (equal(m_text, value)) {
			return;
		}
		m_text = value;
		m_select_range.startPosition = 0;
		m_select_range.length = 0;
		create_text_layout(s_dwrite_factory);
	}

	// 値を段落のそろえに格納する.
	void ShapeText::set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT value)
	{
		if (m_text_align_p == value) {
			return;
		}
		m_text_align_p = value;
		if (m_dw_layout.get() != nullptr) {
			m_dw_layout->SetParagraphAlignment(value);
			tx_create_text_metrics(m_dw_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_cnt, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_dw_descent, m_select_range);
		}
		else {
			create_text_layout(s_dwrite_factory);
		}
	}

	// 値を文字列のそろえに格納する.
	void ShapeText::set_text_align_t(const DWRITE_TEXT_ALIGNMENT value)
	{
		if (m_text_align_t == value) {
			return;
		}
		m_text_align_t = value;
		if (m_text != nullptr && m_text[0] != L'\0') {
			if (m_dw_layout.get() == nullptr) {
				create_text_layout(s_dwrite_factory);
			}
			else {
				m_dw_layout->SetTextAlignment(value);
				tx_create_text_metrics(m_dw_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_cnt, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_dw_descent, m_select_range);
			}
		}
	}

	// 値を行間に格納する.
	void ShapeText::set_text_line(const float value)
	{
		if (m_text_line_h == value) {
			return;
		}
		m_text_line_h = value;
		if (m_dw_layout.get() != nullptr) {
			winrt::com_ptr<IDWriteTextLayout3> t3;
			if (m_dw_layout.try_as(t3)) {
				DWRITE_LINE_SPACING l_spacing;
				if (m_text_line_h > 0.0f) {
					l_spacing.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
					l_spacing.height = m_text_line_h;
					l_spacing.baseline = m_text_line_h - m_dw_descent;
				}
				else {
					l_spacing.method = DWRITE_LINE_SPACING_METHOD_DEFAULT;
					l_spacing.height = 0.0f;
					l_spacing.baseline = 0.0f;
				}
				l_spacing.leadingBefore = 0.0f;
				l_spacing.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DEFAULT;
				t3->SetLineSpacing(&l_spacing);
			}
			tx_create_text_metrics(m_dw_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_cnt, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_dw_descent, m_select_range);
		}
		else {
			create_text_layout(s_dwrite_factory);
		}
	}

	// 値を文字列の余白に格納する.
	void ShapeText::set_text_margin(const D2D1_SIZE_F value)
	{
		if (equal(m_text_margin, value)) {
			return;
		}
		m_text_margin = value;
		create_text_metrics(s_dwrite_factory);
	}

	// 値を文字範囲に格納する.
	void ShapeText::set_text_range(const DWRITE_TEXT_RANGE value)
	{
		if (equal(m_select_range, value)) {
			return;
		}
		m_select_range = value;
		tx_create_text_metrics(m_dw_layout.get(), wchar_len(m_text), m_dw_test_cnt, m_dw_test_metrics, m_dw_line_cnt, m_dw_line_metrics, m_dw_selected_cnt, m_dw_selected_metrics, m_dw_descent, m_select_range);
	}

	// 図形を作成する.
	// b_pos	囲む領域の始点
	// b_diff	囲む領域の終点への差分
	// text	文字列
	// s_attr	属性
	ShapeText::ShapeText(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, wchar_t* const text, const ShapeSheet* s_attr) :
		ShapeRect::ShapeRect(b_pos, b_diff, s_attr),
		m_font_color(s_attr->m_font_color),
		m_font_family(s_attr->m_font_family),
		m_font_size(s_attr->m_font_size),
		m_font_stretch(s_attr->m_font_stretch),
		m_font_style(s_attr->m_font_style),
		m_font_weight(s_attr->m_font_weight),
		m_text_line_h(s_attr->m_text_line_h),
		m_text_margin(s_attr->m_text_margin),
		m_text(text),
		m_text_align_t(s_attr->m_text_align_t),
		m_text_align_p(s_attr->m_text_align_p),
		m_select_range()
	{
		create_text_layout(s_dwrite_factory);
	}

	// 図形をデータライターから読み込む.
	ShapeText::ShapeText(DataReader const& dt_reader) :
		ShapeRect::ShapeRect(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_font_color, dt_reader);
		read(m_font_family, dt_reader);
		is_available_font(m_font_family);
		m_font_size = dt_reader.ReadSingle();
		m_font_stretch = static_cast<DWRITE_FONT_STRETCH>(dt_reader.ReadUInt32());
		m_font_style = static_cast<DWRITE_FONT_STYLE>(dt_reader.ReadUInt32());
		m_font_weight = static_cast<DWRITE_FONT_WEIGHT>(dt_reader.ReadUInt32());
		read(m_text, dt_reader);
		m_text_align_p = static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(dt_reader.ReadUInt32());
		m_text_align_t = static_cast<DWRITE_TEXT_ALIGNMENT>(dt_reader.ReadUInt32());
		m_text_line_h = dt_reader.ReadSingle();
		read(m_text_margin, dt_reader);
		create_text_layout(s_dwrite_factory);
	}

	// データライターに書き込む.
	void ShapeText::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeRect::write(dt_writer);

		write(m_font_color, dt_writer);
		write(m_font_family, dt_writer);
		dt_writer.WriteSingle(m_font_size);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_stretch));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_style));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_weight));
		write(m_text, dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_p));
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_t));
		dt_writer.WriteSingle(m_text_line_h);
		write(m_text_margin, dt_writer);
	}

	// データライターに SVG タグとして書き込む.
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
		const double descent = m_font_size * ((static_cast<double>(metrics.descent)) / metrics.designUnitsPerEm);

		if (is_opaque(m_fill_color) || is_opaque(m_stroke_color)) {
			ShapeRect::write_svg(dt_writer);
		}
		// 文字列全体の属性を指定するための g タグを開始する.
		write_svg("<g ", dt_writer);
		// 書体の色を書き込む.
		write_svg(m_font_color, "fill", dt_writer);
		// 書体名を書き込む.
		write_svg("font-family=\"", dt_writer);
		write_svg(m_font_family, wchar_len(m_font_family), dt_writer);
		write_svg("\" ", dt_writer);
		// 書体の大きさを書き込む.
		write_svg(m_font_size, "font-size", dt_writer);
		// 書体の伸縮を書き込む.
		const auto stretch = static_cast<int32_t>(m_font_stretch);
		write_svg(SVG_STRETCH[stretch], "font-stretch", dt_writer);
		// 書体の形式を書き込む.
		const auto style = static_cast<int32_t>(m_font_style);
		write_svg(SVG_STYLE[style], "font-style", dt_writer);
		// 書体の太さを書き込む.
		const auto weight = static_cast<uint32_t>(m_font_weight);
		write_svg(weight, "font-weight", dt_writer);
		write_svg("none", "stroke", dt_writer);
		write_svg(">" SVG_NEW_LINE, dt_writer);
		// 書体を表示する左上位置に余白を加える.
		D2D1_POINT_2F nw_pos;
		pt_add(m_pos, m_text_margin.width, m_text_margin.height, nw_pos);
		for (uint32_t i = 0; i < m_dw_test_cnt; i++) {
			const auto& tm = m_dw_test_metrics[i];
			const wchar_t* t = m_text + tm.textPosition;
			const uint32_t t_len = tm.length;
			const auto px = static_cast<double>(nw_pos.x);
			const auto qx = static_cast<double>(tm.left);
			const auto py = static_cast<double>(nw_pos.y);
			const auto qy = static_cast<double>(tm.top);
			// 文字列を表示する垂直なずらし位置を求める.
			const auto dy = static_cast<double>(m_dw_line_metrics[i].baseline);
			// 文字列を書き込む.
			tx_write_svg(t, t_len, px + qx, py + qy, dy, dt_writer);
		}
		write_svg("</g>" SVG_NEW_LINE, dt_writer);
	}

	// 文字列をデータライターに SVG として書き込む.
	// t	文字列
	// t_len	文字数
	// x, y	位置
	// dy	垂直なずらし量
	// dt_writer	データライター
	// 戻り値	なし
	static void tx_write_svg(const wchar_t* t, const uint32_t t_len, const double x, const double y, const double dy, DataWriter const& dt_writer)
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
			// ent = "&quot;";
			//}
			//else if (c == L'\'') {
			// ent = "&apos;";
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
		write_svg("</text>" SVG_NEW_LINE, dt_writer);
	}

}
