#include <cwctype> 
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	wchar_t** ShapeText::s_available_fonts = nullptr;	//利用可能な書体名

	//	テキストレイアウトからヒットテストのための計量の配列を得る.
	static void create_test_metrics(IDWriteTextLayout* t_layout, const DWRITE_TEXT_RANGE t_range, DWRITE_HIT_TEST_METRICS*& t_metrics, UINT32& m_count);
	//	文字列をデータライターに SVG として書き込む.
	static void write_svg_text(const wchar_t* t, const uint32_t t_len, const double x, const double y, const double dy, DataWriter const& dt_writer);

	static void get_font_descent(IDWriteTextLayout* text_layout, double& descent);

	//	テキストレイアウトからヒットテストのための計量の配列を得る.
	//	t_layout	もとになるテキストレイアウト
	//	t_range	文字列の範囲
	//	t_metrics	得られた計量の配列
	//	m_count	得られ配列の要素数
	static void create_test_metrics(IDWriteTextLayout* t_layout, const DWRITE_TEXT_RANGE t_range, DWRITE_HIT_TEST_METRICS*& t_metrics, UINT32& m_count)
	{
		const uint32_t pos = t_range.startPosition;
		const uint32_t len = t_range.length;
		DWRITE_HIT_TEST_METRICS test[1];

		//	失敗することが前提なので, 最初の HitTestTextRange 関数
		//	呼び出しは, check_hresult しない.
		t_layout->HitTestTextRange(pos, len, 0, 0, test, 1, &m_count);
		//	配列を確保して, 関数をあらためて呼び出す.
		t_metrics = new DWRITE_HIT_TEST_METRICS[m_count];
		winrt::check_hresult(
			t_layout->HitTestTextRange(pos, len, 0, 0, t_metrics, m_count, &m_count)
		);
	}

	static void get_font_descent(IDWriteTextLayout* text_layout, double& descent)
	{
		IDWriteFontCollection* fonts;
		text_layout->GetFontCollection(&fonts);
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
		font->Release();
		font = nullptr;
		const auto f_size = text_layout->GetFontSize();
		descent = f_size * ((static_cast<double>(metrics.descent)) / metrics.designUnitsPerEm);
	}

	//	図形を破棄する.
	ShapeText::~ShapeText(void)
	{
		m_dw_text_layout = nullptr;
		m_dw_linecnt = 0;
		if (m_dw_test_metrics != nullptr) {
				delete[] m_dw_test_metrics;
			m_dw_test_metrics = nullptr;
		}
		if (m_dw_line_metrics != nullptr) {
			delete[] m_dw_line_metrics;
			m_dw_line_metrics = nullptr;
		}
		m_dw_descent = 0.0;
		m_dw_range_linecnt = 0;
		if (m_dw_range_metrics != nullptr) {
			delete[] m_dw_range_metrics;
			m_dw_range_metrics = nullptr;
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

	// 計量の配列をテキストレイアウトから得る.
	void ShapeText::create_test_metrics(void)
	{
		using winrt::GraphPaper::implementation::create_test_metrics;

		m_dw_linecnt = 0;
		if (m_dw_test_metrics != nullptr) {
			delete[] m_dw_test_metrics;
			m_dw_test_metrics = nullptr;
		}
		if (m_dw_line_metrics != nullptr) {
			delete[] m_dw_line_metrics;
			m_dw_line_metrics = nullptr;
		}
		m_dw_range_linecnt = 0;
		if (m_dw_range_metrics != nullptr) {
			delete[] m_dw_range_metrics;
			m_dw_range_metrics = nullptr;
		}
		m_dw_descent = 0.0;
		if (m_dw_text_layout.get() != nullptr) {
			create_test_metrics(m_dw_text_layout.get(),
				{ 0, wchar_len(m_text) }, m_dw_test_metrics, m_dw_linecnt);

			get_font_descent(m_dw_text_layout.get(), m_dw_descent);
			m_dw_line_metrics = new DWRITE_LINE_METRICS[m_dw_linecnt];
			m_dw_text_layout->GetLineMetrics(m_dw_line_metrics, m_dw_linecnt, &m_dw_linecnt);

			if (m_sel_range.length > 0) {
				create_test_metrics(m_dw_text_layout.get(),
					m_sel_range, m_dw_range_metrics, m_dw_range_linecnt);
			}
		}
	}

	// テキストレイアウトを破棄して作成する.
	void ShapeText::create_text_layout(void)
	{
		using winrt::GraphPaper::implementation::create_test_metrics;

		m_dw_text_layout = nullptr;;
		m_dw_linecnt = 0;
		if (m_dw_test_metrics != nullptr) {
			delete[] m_dw_test_metrics;
			m_dw_test_metrics = nullptr;
		}
		if (m_dw_line_metrics != nullptr) {
			delete[] m_dw_line_metrics;
			m_dw_line_metrics = nullptr;
		}
		m_dw_range_linecnt = 0;
		if (m_dw_range_metrics != nullptr) {
			delete[] m_dw_range_metrics;
			m_dw_range_metrics = nullptr;
		}
		const uint32_t len = wchar_len(m_text);
		if (len == 0) {
			return;
		}
		wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);
		winrt::com_ptr<IDWriteTextFormat> format;
		// CreateTextFormat で,
		// DWRITE_FONT_STRETCH_UNDEFINED が指定された場合, エラーになることがある.
		// 属性値がなんであれ, DWRITE_FONT_STRETCH_NORMAL でテキストフォーマットは作成する.
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
				ls.baseline = static_cast<FLOAT>(m_text_line - m_dw_descent);
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
		m_dw_text_layout->SetTextAlignment(m_text_align_t);
		m_dw_text_layout->SetParagraphAlignment(m_text_align_p);
		DWRITE_TEXT_RANGE t_range{ 0, len };
		m_dw_text_layout->SetFontStretch(m_font_stretch, t_range);
		create_test_metrics(m_dw_text_layout.get(), t_range, m_dw_test_metrics, m_dw_linecnt);
		get_font_descent(m_dw_text_layout.get(), m_dw_descent);
		m_dw_line_metrics = new DWRITE_LINE_METRICS[m_dw_linecnt];
		m_dw_text_layout->GetLineMetrics(m_dw_line_metrics, m_dw_linecnt, &m_dw_linecnt);
		create_test_metrics(m_dw_text_layout.get(), m_sel_range, m_dw_range_metrics, m_dw_range_linecnt);
	}

	// 計量を破棄して作成する.
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

	// 文末の空白を取り除く.
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

	// 図形を表示する.
	void ShapeText::draw(SHAPE_DX& dx)
	{
		ShapeRect::draw(dx);
		if (m_dw_text_layout == nullptr) {
			// || m_text == nullptr || m_text[0] == '\0') {
			return;
		}
		D2D1_POINT_2F t_min;
		pt_add(m_pos, m_vec, t_min);
		pt_min(m_pos, t_min, t_min);
		auto hm = min(m_text_mar.width, fabs(m_vec.x) * 0.5);
		auto vm = min(m_text_mar.height, fabs(m_vec.y) * 0.5);
		pt_add(t_min, hm, vm, t_min);
//uint32_t line_cnt;
//m_dw_text_layout->GetLineMetrics(nullptr, 0, &line_cnt);
//auto l_met = new DWRITE_LINE_METRICS[line_cnt];
//m_dw_text_layout->GetLineMetrics(l_met, line_cnt, &line_cnt);
		if (m_sel_range.length > 0) {
			// 文字範囲の計量を塗りつぶしで表示する.
			const auto br = dx.m_shape_brush.get();
			br->SetColor(dx.m_range_bcolor);
			const auto cnt = m_dw_range_linecnt;
			const auto dc = dx.m_d2dContext.get();
			for (uint32_t i = 0; i < m_dw_range_linecnt; i++) {
				const auto& range = m_dw_range_metrics[i];
				for (uint32_t j = 0; j < m_dw_linecnt; j++) {
					const auto& test = m_dw_test_metrics[j];
					const auto& line = m_dw_line_metrics[j];
					if (test.textPosition <= range.textPosition && range.textPosition + range.length <= test.textPosition + test.length) {
						D2D1_RECT_F r;
						r.left = t_min.x + range.left;
						r.top = static_cast<FLOAT>(t_min.y + test.top + line.baseline + m_dw_descent - m_font_size);
						r.right = r.left + range.width;
						r.bottom = static_cast<FLOAT>(r.top + m_font_size);
						dc->FillRectangle(r, br);
						break;
					}
				}
			}
			dx.m_range_brush->SetColor(dx.m_range_tcolor);
			m_dw_text_layout->SetDrawingEffect(dx.m_range_brush.get(), m_sel_range);
		}
//delete[] l_met;
		dx.m_shape_brush->SetColor(m_font_color);
		dx.m_d2dContext->DrawTextLayout(t_min, m_dw_text_layout.get(), dx.m_shape_brush.get());
		if (m_sel_range.length > 0 && m_text != nullptr) {
			m_dw_text_layout->SetDrawingEffect(nullptr, { 0, wchar_len(m_text) });
		}
		if (is_selected() == false) {
			return;
		}
		// 文字列の計量を枠で表示する.
		const auto ss = dx.m_aux_style.get();
		auto d_cnt = ss->GetDashesCount();
		if (d_cnt <= 0 || d_cnt > 6) {
			return;
		}
		const auto dc = dx.m_d2dContext.get();
		const auto l_cnt = m_dw_linecnt;
		const auto br = dx.m_shape_brush.get();
		br->SetColor(dx.m_range_bcolor);
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
		for (uint32_t i = 0; i < l_cnt; i++) {
			auto const& test = m_dw_test_metrics[i];
			auto const& line = m_dw_line_metrics[i];
			winrt::com_ptr<ID2D1StrokeStyle> style;
			// 破線がずれて重なって表示されないように, 破線のオフセットを設定し,
			// 文字列の枠を辺ごとに表示する.
			D2D1_POINT_2F p[4];
			p[0].x = t_min.x + test.left;
			p[0].y = static_cast<FLOAT>(t_min.y + test.top + line.baseline + m_dw_descent - m_font_size);
			p[2].x = p[0].x + test.width;
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

	// 要素を利用可能な書体名から得る.
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
	bool ShapeText::get_font_size(double& val) const noexcept
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

	// 文字列を得る.
	bool ShapeText::get_text(wchar_t*& val) const noexcept
	{
		val = m_text;
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

	// 行間を得る.
	bool ShapeText::get_text_line_height(double& val) const noexcept
	{
		val = m_text_line;
		return true;
	}

	// 文字列の余白を得る.
	bool ShapeText::get_text_margin(D2D1_SIZE_F& val) const noexcept
	{
		val = m_text_mar;
		return true;
	}

	// 文字範囲を得る
	bool ShapeText::get_text_range(DWRITE_TEXT_RANGE& val) const noexcept
	{
		val = m_sel_range;
		return true;
	}

	// 位置を含むか調べる.
	// t_pos	調べる位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	ANCH_WHICH ShapeText::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const auto anchor = hit_test_anchor(t_pos, a_len);
		if (anchor != ANCH_WHICH::ANCH_OUTSIDE) {
			return anchor;
		}
		// 文字列図形の左上が原点になるよう, 調べる位置を移動する.
		D2D1_POINT_2F pos;
		ShapeStroke::get_min_pos(pos);
		pt_sub(t_pos, pos, pos);
		pt_sub(pos, m_text_mar, pos);
		const auto l_cnt = m_dw_linecnt;
		for (uint32_t i = 0; i < l_cnt; i++) {
			auto const& test = m_dw_test_metrics[i];
			auto const& line = m_dw_line_metrics[i];
			D2D1_POINT_2F r_max{
				test.left + test.width,
				static_cast<FLOAT>(test.top + line.baseline + m_dw_descent)
			};
			D2D1_POINT_2F r_min{
				test.left,
				static_cast<FLOAT>(r_max.y - m_font_size)
			};
			if (pt_in_rect(pos, r_min, r_max)) {
				return ANCH_WHICH::ANCH_TEXT;
			}
		}
		return ShapeRect::hit_test(t_pos, a_len);
	}

	// 範囲に含まれるか調べる.
	// a_min	範囲の左上位置
	// a_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeText::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		const uint32_t cnt = m_dw_linecnt;
		D2D1_POINT_2F pos;
		D2D1_POINT_2F h_min;
		D2D1_POINT_2F h_max;

		if (cnt > 0) {
			ShapeStroke::get_min_pos(pos);
			for (uint32_t i = 0; i < cnt; i++) {
				auto const& test = m_dw_test_metrics[i];
				auto const& line = m_dw_line_metrics[i];
				auto const top = static_cast<double>(test.top) + static_cast<double>(line.baseline) + m_dw_descent - m_font_size;
				pt_add(pos, test.left, top, h_min);
				if (pt_in_rect(h_min, a_min, a_max) == false) {
					return false;
				}
				pt_add(h_min, test.width, m_font_size, h_max);
				if (pt_in_rect(h_max, a_min, a_max) == false) {
					return false;
				}
			}
		}
		return ShapeRect::in_area(a_min, a_max);
	}

	// 書体名が利用可能か調べる.
	// font	書体名
	// 戻り値	利用可能なら true
	// 利用可能なら, 引数の書体名を破棄し, 利用可能な書体名配列の要素と置き換える.
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

	// 利用可能な書体名を破棄する.
	void ShapeText::release_available_fonts(void)
	{
		for (uint32_t i = 0; s_available_fonts[i]; i++) {
			delete[] s_available_fonts[i];
		}
		delete[] s_available_fonts;
		s_available_fonts = nullptr;
	}

	// 利用可能な書体名に格納する.
	// coll		DWRITE フォントコレクション
	// lang		地域・言語名
	void ShapeText::set_available_fonts(IDWriteFontCollection* coll, wchar_t lang[])
	{
		UINT32 index = 0;
		BOOL exists = false;
		// フォントコレクションの要素数を得る.
		// 得られた要素数の利用可能な書体名を確保する.
		const auto f_cnt = coll->GetFontFamilyCount();
		if (s_available_fonts != nullptr) {
			// 利用可能な書体名が空でなければ, それらを破棄する.
			for (auto i = 0; s_available_fonts[i] != nullptr; i++) {
				delete[] s_available_fonts[i];
			}
			delete[] s_available_fonts;
			s_available_fonts = nullptr;
		}
		s_available_fonts = new wchar_t* [static_cast<size_t>(f_cnt) + 1];
		// フォントコレクションの各要素について.
		for (uint32_t i = 0; i < f_cnt; i++) {
			// 要素から書体名を得る.
			// 書体名からローカライズされた書体名を得る.
			// ローカライズされた書体名から, 地域名をのぞいた書体名の開始位置を得る.
			// 地域名が含まれてなければ 0 を開始位置に格納する.
			// 開始位置より後ろの文字数を得る (ヌル文字は含まれない).
			// 文字数 + 1 の文字配列を確保し, 書体名の配列に格納する.
			// ローカライズされた書体名を破棄する.
			// フォントファミリーをを破棄する.
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
		// 利用可能な書体名の末尾に終端としてヌルを格納する.
		s_available_fonts[f_cnt] = nullptr;
	}

	// 値を書体の色に格納する.
	void ShapeText::set_font_color(const D2D1_COLOR_F& val) noexcept
	{
		m_font_color = val;
	}

	// 値を書体名に格納する.
	void ShapeText::set_font_family(wchar_t* const val)
	{
		// 値が書体名と同じか調べる.
		if (equal(m_font_family, val)) {
			// 同じなら終了する.
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

	// 値を書体の大きさに格納する.
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

	// 値を書体の横幅に格納する.
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

	// 値を書体の字体に格納する.
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

	// 値を書体の太さに格納する.
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

	// 値を指定した部位の位置に格納する. 他の部位の位置は動かない. 
	void ShapeText::set_pos(const D2D1_POINT_2F val, const ANCH_WHICH a)
	{
		ShapeRect::set_pos(val, a);
		create_text_metrics();
	}

	// 値を文字列に格納する.
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

	// 値を段落のそろえに格納する.
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

	// 値を文字列のそろえに格納する.
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

	// 値を行間に格納する.
	void ShapeText::set_text_line_height(const double val)
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
						ls.baseline = static_cast<FLOAT>(m_text_line - m_dw_descent);
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

	// 値を文字列の余白に格納する.
	void ShapeText::set_text_margin(const D2D1_SIZE_F val)
	{
		if (equal(m_text_mar, val)) {
			return;
		}
		m_text_mar = val;
		create_text_metrics();
	}

	// 値を文字範囲に格納する.
	void ShapeText::set_text_range(const DWRITE_TEXT_RANGE val)
	{
		if (equal(m_sel_range, val)) {
			return;
		}
		m_sel_range = val;
		create_test_metrics();
	}

	// 図形を作成する.
	// pos	開始位置
	// vec	終了ベクトル
	// text	文字列
	// attr	既定の属性値
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

	// 図形をデータライターから読み込む.
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

	// データライターに書き込む.
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
		//	垂直方向のずらし量を求める.
		//
		//	Chrome では, テキストタグに属性 alignment-baseline="text-before-edge" 
		//	を指定するだけで, 左上位置を基準にして Dwrite と同じように表示される.
		//	しかし, IE や Edge では, alignment-baseline 属性は期待した働きをしないので,
		//	上部からのベースラインまで値である, 垂直方向のずらし量 dy を
		//	行ごとに計算を必要がある.
		//	このとき, 上部からのベースラインの高さ = アセントにはならないので
		//	デセントを用いて計算する必要もある.
		//	テキストレイアウトからフォントメトリックスを取得して, 以下のように求める.
		//	ちなみに, designUnitsPerEm は, 配置 (Em) ボックスの単位あたりの大きさ.
		//	デセントは, フォント文字の配置ボックスの下部からベースラインまでの長さ.
		//	dy = その行のヒットテストメトリックスの高さ - フォントの大きさ × (デセント ÷ 単位大きさ) となる, はず.
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
		//	文字列全体の属性を指定するための g タグを開始する.
		write_svg("<g ", dt_writer);
		//	書体の色を書き込む.
		write_svg(m_font_color, "fill", dt_writer);
		//	書体名を書き込む.
		write_svg("font-family=\"", dt_writer);
		write_svg(m_font_family, wchar_len(m_font_family), dt_writer);
		write_svg("\" ", dt_writer);
		//	書体の大きさを書き込む.
		write_svg(m_font_size, "font-size", dt_writer);
		//	書体の伸縮を書き込む.
		const auto stretch = static_cast<int32_t>(m_font_stretch);
		write_svg(SVG_STRETCH[stretch], "font-stretch", dt_writer);
		//	書体の形式を書き込む.
		const auto style = static_cast<int32_t>(m_font_style);
		write_svg(SVG_STYLE[style], "font-style", dt_writer);
		//	書体の太さを書き込む.
		const auto weight = static_cast<uint32_t>(m_font_weight);
		write_svg(weight, "font-weight", dt_writer);
		write_svg("none", "stroke", dt_writer);
		write_svg(">" SVG_NL, dt_writer);
		//	書体を表示する左上位置に余白を加える.
		D2D1_POINT_2F pos;
		pt_add(m_pos, m_text_mar.width, m_text_mar.height, pos);
		for (uint32_t i = 0; i < m_dw_linecnt; i++) {
			const wchar_t* t = m_text + m_dw_test_metrics[i].textPosition;
			const uint32_t t_len = m_dw_test_metrics[i].length;
			const double px = static_cast<double>(pos.x);
			const double qx = static_cast<double>(m_dw_test_metrics[i].left);
			const double py = static_cast<double>(pos.y);
			const double qy = static_cast<double>(m_dw_test_metrics[i].top);
			//	文字列を表示する垂直なずらし位置を求める.
			const double dy = m_dw_line_metrics[i].baseline;
			//	文字列を書き込む.
			write_svg_text(t, t_len, px + qx, py + qy, dy, dt_writer);
		}
		write_svg("</g>" SVG_NL, dt_writer);
	}

	//	文字列をデータライターに SVG として書き込む.
	//	t	文字列
	//	t_len	文字数
	//	x, y	位置
	//	dy	垂直なずらし量
	//	dt_writer	データライター
	//	戻り値	なし
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
