//-------------------------------
// MainPage_sheet.cpp
// 用紙の各属性の設定
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::TextBox;

	constexpr wchar_t SHEET_TITLE[] = L"str_sheet";	// 用紙の表題

	// 長さををピクセル単位の値に変換する.
	static double conv_len_to_val(const LEN_UNIT l_unit, const double value, const double dpi, const double g_len) noexcept;

	// 長さををピクセル単位の値に変換する.
	// 変換された値は, 0.5 ピクセル単位に丸められる.
	// l_unit	長さの単位
	// value	長さの値
	// dpi	DPI
	// g_len	方眼の大きさ
	// 戻り値	ピクセル単位の値
	static double conv_len_to_val(const LEN_UNIT l_unit, const double value, const double dpi, const double g_len) noexcept
	{
		double ret;

		if (l_unit == LEN_UNIT::INCH) {
			ret = value * dpi;
		}
		else if (l_unit == LEN_UNIT::MILLI) {
			ret = value * dpi / MM_PER_INCH;
		}
		else if (l_unit == LEN_UNIT::POINT) {
			ret = value * dpi / PT_PER_INCH;
		}
		else if (l_unit == LEN_UNIT::GRID) {
			ret = value * g_len;
		}
		else {
			ret = value;
		}
		return std::round(2.0 * ret) * 0.5;
	}

	// 図形が含まれるよう用紙の左上位置と右下位置を更新する.
	// s	図形
	void MainPage::sheet_update_bbox(const Shape* s) noexcept
	{
		s->get_bound(m_sheet_min, m_sheet_max, m_sheet_min, m_sheet_max);
	}

	// 用紙の左上位置と右下位置を設定する.
	void MainPage::sheet_update_bbox(void) noexcept
	{
		s_list_bound(m_list_shapes, m_sheet_main.m_sheet_size, m_sheet_min, m_sheet_max);
	}

	// 用紙メニューの「用紙の色」が選択された.
	IAsyncAction MainPage::sheet_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		const float val0 = m_sample_sheet.m_sheet_color.r * COLOR_MAX;
		const float val1 = m_sample_sheet.m_sheet_color.g * COLOR_MAX;
		const float val2 = m_sample_sheet.m_sheet_color.b * COLOR_MAX;
		sample_slider_0().Value(val0);
		sample_slider_1().Value(val1);
		sample_slider_2().Value(val2);
		sheet_set_slider_header<UNDO_OP::SHEET_COLOR, 0>(val0);
		sheet_set_slider_header<UNDO_OP::SHEET_COLOR, 1>(val1);
		sheet_set_slider_header<UNDO_OP::SHEET_COLOR, 2>(val2);
		sample_slider_0().Visibility(VISIBLE);
		sample_slider_1().Visibility(VISIBLE);
		sample_slider_2().Visibility(VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::sheet_set_slider<UNDO_OP::SHEET_COLOR, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::sheet_set_slider<UNDO_OP::SHEET_COLOR, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::sheet_set_slider<UNDO_OP::SHEET_COLOR, 2> });
		m_sample_type = SAMP_TYPE::NONE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(SHEET_TITLE)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F sample_value;
			m_sample_sheet.get_sheet_color(sample_value);
			D2D1_COLOR_F sheet_value;
			m_sheet_main.get_sheet_color(sheet_value);
			if (equal(sheet_value, sample_value) != true) {
				undo_push_set<UNDO_OP::SHEET_COLOR>(&m_sheet_main, sample_value);
				undo_push_null();
				undo_menu_enable();
				sheet_draw();
			}
		}
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_1().Visibility(COLLAPSED);
		sample_slider_2().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sample_slider_2().ValueChanged(slider_2_token);
	}

	// 用紙を表示する.
	void MainPage::sheet_draw(void)
	{
#if defined(_DEBUG)
		if (m_sheet_dx.m_swapChainPanel.IsLoaded() != true) {
			return;
		}
#endif
		if (m_dx_mutex.try_lock() != true) {
			// ロックできない場合
			return;
		}

		auto const& dc = m_sheet_dx.m_d2dContext;
		// デバイスコンテキストの描画状態を保存ブロックに保持する.
		dc->SaveDrawingState(m_sheet_dx.m_state_block.get());
		// デバイスコンテキストから変換行列を得る.
		D2D1_MATRIX_3X2_F tran;
		dc->GetTransform(&tran);
		// 拡大率を変換行列の拡大縮小の成分に格納する.
		const auto sheet_scale = max(m_sheet_main.m_sheet_scale, 0.0f);
		tran.m11 = tran.m22 = sheet_scale;
		// スクロールの変分に拡大率を掛けた値を
		// 変換行列の平行移動の成分に格納する.
		D2D1_POINT_2F t_pos;
		pt_add(m_sheet_min, sb_horz().Value(), sb_vert().Value(), t_pos);
		pt_mul(t_pos, sheet_scale, t_pos);
		tran.dx = -t_pos.x;
		tran.dy = -t_pos.y;
		// 変換行列をデバイスコンテキストに格納する.
		dc->SetTransform(&tran);
		// 描画を開始する.
		dc->BeginDraw();
		// 用紙色で塗りつぶす.
		dc->Clear(m_sheet_main.m_sheet_color);
		GRID_SHOW g_show;
		m_sample_sheet.get_grid_show(g_show);
		if (equal(g_show, GRID_SHOW::BACK)) {
			// 方眼の表示が最背面に表示の場合,
			// 方眼を表示する.
			m_sheet_main.draw_grid(m_sheet_dx, { 0.0f, 0.0f });
		}
		// 部位の色をブラシに格納する.
		//D2D1_COLOR_F anch_color;
		//m_sheet_main.get_anchor_color(anch_color);
		//m_sheet_dx.m_anch_brush->SetColor(anch_color);
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				// 消去フラグが立っている場合,
				// 継続する.
				continue;
			}
			// 図形を表示する.
			s->draw(m_sheet_dx);
		}
		if (equal(g_show, GRID_SHOW::FRONT)) {
			// 方眼の表示が最前面に表示の場合,
			// 方眼を表示する.
			m_sheet_main.draw_grid(m_sheet_dx, { 0.0f, 0.0f });
		}
		if (pointer_state() == PBTN_STATE::PRESS_AREA) {
			const auto t_draw = tool_draw();
			if (t_draw == TOOL_DRAW::SELECT || t_draw == TOOL_DRAW::RECT || t_draw == TOOL_DRAW::TEXT || t_draw == TOOL_DRAW::RULER) {
				m_sheet_main.draw_auxiliary_rect(m_sheet_dx, pointer_pressed(), pointer_cur());
			}
			else if (t_draw == TOOL_DRAW::BEZI) {
				m_sheet_main.draw_auxiliary_bezi(m_sheet_dx, pointer_pressed(), pointer_cur());
			}
			else if (t_draw == TOOL_DRAW::ELLI) {
				m_sheet_main.draw_auxiliary_elli(m_sheet_dx, pointer_pressed(), pointer_cur());
			}
			else if (t_draw == TOOL_DRAW::LINE) {
				m_sheet_main.draw_auxiliary_line(m_sheet_dx, pointer_pressed(), pointer_cur());
			}
			else if (t_draw == TOOL_DRAW::RRECT) {
				m_sheet_main.draw_auxiliary_rrect(m_sheet_dx, pointer_pressed(), pointer_cur());
			}
			else if (t_draw == TOOL_DRAW::POLY) {
				m_sheet_main.draw_auxiliary_poly(m_sheet_dx, pointer_pressed(), pointer_cur(), tool_poly());
			}
		}
		// 描画を終了する.
		HRESULT hr = dc->EndDraw();
		// 保存された描画環境を元に戻す.
		dc->RestoreDrawingState(m_sheet_dx.m_state_block.get());
		if (hr == S_OK) {
			// 結果が S_OK の場合,
			// スワップチェーンの内容を画面に表示する.
			m_sheet_dx.Present();
			// ポインターの位置をステータスバーに格納する.
			sbar_set_curs();
		}
#if defined(_DEBUG)
		else {
			// 結果が S_OK でない場合,
			// 「描画できません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, L"str_err_draw", {});
		}
#endif
		m_dx_mutex.unlock();
	}

	// 前景色を得る.
	const D2D1_COLOR_F& MainPage::sheet_foreground(void) const noexcept
	{
		return m_sheet_dx.m_theme_foreground;
	}

	// 用紙とその他の属性を初期化する.
	void MainPage::sheet_init(void) noexcept
	{
		// 書体の属性を初期化する.
		{
			using winrt::Windows::UI::Xaml::Setter;
			using winrt::Windows::UI::Xaml::Controls::TextBlock;
			using winrt::Windows::UI::Xaml::Media::FontFamily;
			using winrt::Windows::UI::Text::FontWeight;
			using winrt::Windows::UI::Text::FontStretch;
			using winrt::Windows::UI::Xaml::Style;

			// リソースの取得に失敗した場合に備えて,
			// 固定の既定値を書体属性に格納する.
			m_sheet_main.set_font_family(wchar_cpy(L"Segoe UI"));
			m_sheet_main.set_font_size(FONT_SIZE_DEF);
			m_sheet_main.set_font_stretch(DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL);
			m_sheet_main.set_font_style(DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL);
			m_sheet_main.set_font_weight(DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL);
			// BodyTextBlockStyle をリソースディクショナリから得る.
			auto resource = Resources().TryLookup(box_value(L"BodyTextBlockStyle"));
			if (resource != nullptr) {
				auto style = resource.try_as<Style>();
				std::list<Style> stack;
				// リソースからスタイルを得る.
				while (style != nullptr) {
					// スタイルが空でない場合.
					// スタイルをスタックに積む.
					stack.push_back(style);
					// スタイルの継承元のスタイルを得る.
					style = style.BasedOn();
				}
				try {
					while (stack.empty() != true) {
						// スタックが空でない場合,
						// スタイルをスタックから取り出す.
						style = stack.back();
						stack.pop_back();
						// スタイルの中の各セッターについて.
						auto const& setters = style.Setters();
						for (auto const& base : setters) {
							auto const& setter = base.try_as<Setter>();
							// セッターのプロパティーを得る.
							auto const& prop = setter.Property();
							if (prop == TextBlock::FontFamilyProperty()) {
								// プロパティーが FontFamily の場合,
								// セッターの値から, 書体名を得る.
								auto value = unbox_value<FontFamily>(setter.Value());
								m_sheet_main.set_font_family(wchar_cpy(value.Source().c_str()));
							}
							else if (prop == TextBlock::FontSizeProperty()) {
								// プロパティーが FontSize の場合,
								// セッターの値から, 書体の大きさを得る.
								const auto value = unbox_value<float>(setter.Value());
								m_sheet_main.m_font_size = value;
							}
							else if (prop == TextBlock::FontStretchProperty()) {
								// プロパティーが FontStretch の場合,
								// セッターの値から, 書体の伸縮を得る.
								auto value = unbox_value<int32_t>(setter.Value());
								m_sheet_main.set_font_stretch(static_cast<DWRITE_FONT_STRETCH>(value));
							}
							else if (prop == TextBlock::FontStyleProperty()) {
								// プロパティーが FontStyle の場合,
								// セッターの値から, 字体を得る.
								auto value = unbox_value<int32_t>(setter.Value());
								m_sheet_main.set_font_style(static_cast<DWRITE_FONT_STYLE>(value));
							}
							else if (prop == TextBlock::FontWeightProperty()) {
								// プロパティーが FontWeight の場合,
								// セッターの値から, 書体の太さを得る.
								auto value = unbox_value<int32_t>(setter.Value());
								m_sheet_main.set_font_weight(static_cast<DWRITE_FONT_WEIGHT>(value));
								//Determine the type of a boxed value
								//auto prop = setter.Value().try_as<winrt::Windows::Foundation::IPropertyValue>();
								//if (prop.Type() == winrt::Windows::Foundation::PropertyType::Inspectable) {
								// ...
								//}
								//else if (prop.Type() == winrt::Windows::Foundation::PropertyType::int32) {
								// ...
								//}
							}
						}
					}
				}
				catch (winrt::hresult_error const&) {
				}
				stack.clear();
				style = nullptr;
				resource = nullptr;
			}
			ShapeText::is_available_font(m_sheet_main.m_font_family);
		}

		{
			m_sheet_main.set_arrow_size(ARROWHEAD_SIZE_DEF);
			m_sheet_main.set_arrow_style(ARROWHEAD_STYLE::NONE);
			m_sheet_main.set_corner_radius(D2D1_POINT_2F{ GRID_LEN_DEF, GRID_LEN_DEF });
			m_sheet_main.set_fill_color(m_sheet_dx.m_theme_background);
			m_sheet_main.set_font_color(sheet_foreground());
			m_sheet_main.set_grid_base(GRID_LEN_DEF - 1.0);
			m_sheet_main.set_grid_gray(GRID_GRAY_DEF);
			m_sheet_main.set_grid_emph(GRID_EMPH_0);
			m_sheet_main.set_grid_show(GRID_SHOW::BACK);
			m_sheet_main.set_grid_snap(true);
			m_sheet_main.set_sheet_color(m_sheet_dx.m_theme_background);
			m_sheet_main.set_sheet_scale(1.0);
			const double dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			m_sheet_main.m_sheet_size = SHEET_SIZE_DEF;
			m_sheet_main.set_stroke_color(sheet_foreground());
			m_sheet_main.set_stroke_join_limit(MITER_LIMIT_DEF);
			m_sheet_main.set_stroke_join_style(D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
			m_sheet_main.set_stroke_dash_patt(STROKE_DASH_PATT_DEF);
			m_sheet_main.set_stroke_dash_style(D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
			m_sheet_main.set_stroke_width(1.0);
			m_sheet_main.set_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			m_sheet_main.set_text_align_t(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
			m_sheet_main.set_text_line(0.0);
			m_sheet_main.set_text_margin(TEXT_MARGIN_DEF);
		}
		len_unit(LEN_UNIT::PIXEL);
		color_code(COLOR_CODE::DEC);
		status_bar(sbar_or(SBAR_FLAG::CURS, SBAR_FLAG::ZOOM));
	}

	// 値を用紙の右下位置に格納する.
	void MainPage::sheet_max(const D2D1_POINT_2F p_max) noexcept
	{
		m_sheet_max = p_max;
	}

	// 用紙の右下位置を得る.
	const D2D1_POINT_2F MainPage::sheet_max(void) const noexcept
	{
		return m_sheet_max;
	}

	// 値を用紙の左上位置に格納する.
	void MainPage::sheet_min(const D2D1_POINT_2F p_min) noexcept
	{
		m_sheet_min = p_min;
	}

	// 用紙の左上位置を得る.
	const D2D1_POINT_2F MainPage::sheet_min(void) const noexcept
	{
		return m_sheet_min;
	}

	// 用紙のスワップチェーンパネルがロードされた.
#if defined(_DEBUG)
	void MainPage::sheet_panel_loaded(IInspectable const& sender, RoutedEventArgs const&)
#else
	void MainPage::sheet_panel_loaded(IInspectable const&, RoutedEventArgs const&)
#endif
	{
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			return;
		}
#endif // _DEBUG
		m_sheet_dx.SetSwapChainPanel(scp_sheet_panel());
		sheet_draw();
	}

	// 用紙のスワップチェーンパネルの寸法が変わった.
#if defined(_DEBUG)
	void MainPage::sheet_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
#else
	void MainPage::sheet_panel_size_changed(IInspectable const&, SizeChangedEventArgs const& args)
#endif	// _DEBUG
	{
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			return;
		}
#endif	// _DEBUG
		const auto z = args.NewSize();
		const auto w = z.Width;
		const auto h = z.Height;
		scroll_set(w, h);
		if (scp_sheet_panel().IsLoaded() != true) {
			return;
		}
		m_sheet_dx.SetLogicalSize2({ w, h });
		sheet_draw();
	}

	// 用紙の大きさを設定する.
	void MainPage::sheet_panle_size(void)
	{
		const auto w = scp_sheet_panel().ActualWidth();
		const auto h = scp_sheet_panel().ActualHeight();
		scroll_set(w, h);
		m_sheet_dx.SetLogicalSize2({ static_cast<float>(w), static_cast<float>(h) });
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S> void MainPage::sheet_set_slider_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring hdr;
		if constexpr (U == UNDO_OP::SHEET_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_b") + L": " + buf;
			}
		}
		if constexpr (S == 0) {
			sample_slider_0().Header(box_value(hdr));
		}
		if constexpr (S == 1) {
			sample_slider_1().Header(box_value(hdr));
		}
		if constexpr (S == 2) {
			sample_slider_2().Header(box_value(hdr));
		}
		if constexpr (S == 3) {
			sample_slider_3().Header(box_value(hdr));
		}
	}

	// 値をスライダーのヘッダーと、見本の図形に格納する.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S> void MainPage::sheet_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* s = &m_sample_sheet;
		const auto value = static_cast<float>(args.NewValue());
		sheet_set_slider_header<U, S>(value);
		if constexpr (U == UNDO_OP::GRID_BASE) {
			s->set_grid_base(value);
		}
		if constexpr (U == UNDO_OP::GRID_GRAY) {
			s->set_grid_gray(value / COLOR_MAX);
		}
		if constexpr (U == UNDO_OP::SHEET_COLOR) {
			D2D1_COLOR_F color;
			s->get_sheet_color(color);
			if constexpr (S == 0) {
				color.r = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 1) {
				color.g = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 2) {
				color.b = static_cast<FLOAT>(value / COLOR_MAX);
			}
			s->set_sheet_color(color);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// 用紙メニューの「用紙の大きさ」が選択された
	IAsyncAction MainPage::sheet_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		double pw = m_sheet_main.m_sheet_size.width;
		double ph = m_sheet_main.m_sheet_size.height;
		const double dpi = m_sheet_dx.m_logical_dpi;
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		const auto g_len = g_base + 1.0;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), pw, dpi, g_len, buf);
		tx_sheet_width().Text(buf);
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), ph, dpi, g_len, buf);
		tx_sheet_height().Text(buf);
		conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), sheet_size_max(), dpi, g_len, buf);
		tx_sheet_size_max().Text(buf);
		// この時点では, テキストボックスに正しい数値を格納しても, 
		// TextChanged は呼ばれない.
		// プライマリーボタンは使用可能にしておく.
		cd_sheet_size().IsPrimaryButtonEnabled(true);
		cd_sheet_size().IsSecondaryButtonEnabled(m_list_shapes.size() > 0);
		const auto d_result = co_await cd_sheet_size().ShowAsync();
		if (d_result == ContentDialogResult::None) {
			// 「キャンセル」が押された場合,
			co_return;
		}
		else if (d_result == ContentDialogResult::Primary) {
			constexpr wchar_t INVALID_NUM[] = L"str_err_number";

			// 本来, 無効な数値が入力されている場合, 「適用」ボタンは不可になっているので
			// 必要ないエラーチェックだが, 念のため.
			if (swscanf_s(tx_sheet_width().Text().c_str(), L"%lf", &pw) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_sheet_width/Header");
				co_return;
			}
			if (swscanf_s(tx_sheet_height().Text().c_str(), L"%lf", &ph) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_sheet_height/Header");
				co_return;
			}
			// 用紙の縦横の長さの値をピクセル単位の値に変換する.
			D2D1_SIZE_F p_size{
				static_cast<FLOAT>(conv_len_to_val(len_unit(), pw, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_val(len_unit(), ph, dpi, g_len))
			};
			if (!equal(p_size, m_sheet_main.m_sheet_size)) {
				// 変換された値が用紙の大きさと異なる場合,
				undo_push_set<UNDO_OP::SHEET_SIZE>(&m_sheet_main, p_size);
				undo_push_null();
				undo_menu_enable();
				sheet_update_bbox();
				sheet_panle_size();
				sheet_draw();
				sbar_set_curs();
				sbar_set_grid();
				sbar_set_sheet();
				sbar_set_unit();
			}
		}
		else if (d_result == ContentDialogResult::Secondary) {
			D2D1_POINT_2F b_min = { FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_max = { -FLT_MAX, -FLT_MAX };
			D2D1_POINT_2F b_size;

			s_list_bound(m_list_shapes, b_min, b_max);
			pt_sub(b_max, b_min, b_size);
			if (b_size.x < 1.0F || b_size.y < 1.0F) {
				co_return;
			}
			float dx = 0.0F;
			float dy = 0.0F;
			if (b_min.x < 0.0F) {
				dx = -b_min.x;
				b_min.x = 0.0F;
				b_max.x += dx;
			}
			if (b_min.y < 0.0F) {
				dy = -b_min.y;
				b_min.y = 0.0F;
				b_max.y += dy;
			}
			bool flag = false;
			if (dx > 0.0F || dy > 0.0F) {
				constexpr auto ALL = true;
				undo_push_move({ dx, dy }, ALL);
				flag = true;
			}
			D2D1_POINT_2F p_min = { 0.0F, 0.0F };
			D2D1_POINT_2F p_max;
			pt_add(b_max, b_min, p_max);
			D2D1_SIZE_F p_size = { p_max.x, p_max.y };
			if (equal(m_sheet_main.m_sheet_size, p_size) != true) {
				undo_push_set<UNDO_OP::SHEET_SIZE>(&m_sheet_main, p_size);
				flag = true;
			}
			if (flag) {
				undo_push_null();
				undo_menu_enable();
			}
			sheet_update_bbox();
			sheet_panle_size();
			sheet_draw();
			sbar_set_sheet();
		}
	}

	// テキストボックス「用紙の幅」「用紙の高さ」の値が変更された.
	void MainPage::sheet_size_text_changed(IInspectable const& sender, TextChangedEventArgs const&)
	{
		const double dpi = m_sheet_dx.m_logical_dpi;
		double value;
		wchar_t buf[2];
		int cnt;
		// テキストボックスの文字列を数値に変換する.
		cnt = swscanf_s(unbox_value<TextBox>(sender).Text().c_str(), L"%lf%1s", &value, buf, 2);
		if (cnt == 1 && value > 0.0) {
			// 文字列が数値に変換できた場合,
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			value = conv_len_to_val(len_unit(), value, dpi, g_base + 1.0);
		}
		cd_sheet_size().IsPrimaryButtonEnabled(cnt == 1 && value >= 1.0 && value < sheet_size_max());
	}

}