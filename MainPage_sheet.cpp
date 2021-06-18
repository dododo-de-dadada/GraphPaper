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

	constexpr wchar_t DLG_TITLE[] = L"str_sheet";	// 用紙の表題

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

	// チェックマークを図形の属性関連のメニュー項目につける.
	void MainPage::sheet_attr_is_checked(void) noexcept
	{
		ARROW_STYLE a_style;
		m_sheet_main.get_arrow_style(a_style);
		arrow_style_is_checked(a_style);
		DWRITE_FONT_STYLE f_style;
		m_sheet_main.get_font_style(f_style);
		font_style_is_checked(f_style);
		D2D1_DASH_STYLE s_style;
		m_sheet_main.get_stroke_dash_style(s_style);
		stroke_dash_style_is_checked(s_style);
		D2D1_LINE_JOIN j_style;
		m_sheet_main.get_stroke_join_style(j_style);
		join_style_is_checked(j_style);
		DWRITE_TEXT_ALIGNMENT t_align_t;
		m_sheet_main.get_text_align_t(t_align_t);
		text_align_t_is_checked(t_align_t);
		DWRITE_PARAGRAPH_ALIGNMENT t_align_p;
		m_sheet_main.get_text_align_p(t_align_p);
		text_align_p_is_checked(t_align_p);
		GRID_EMPH g_emph;
		m_sheet_main.get_grid_emph(g_emph);
		grid_emph_is_checked(g_emph);
		GRID_SHOW g_show;
		m_sheet_main.get_grid_show(g_show);
		grid_show_is_checked(g_show);
		bool g_snap;
		m_sheet_main.get_grid_snap(g_snap);
		tmfi_grid_snap().IsChecked(g_snap);
		tmfi_grid_snap_2().IsChecked(g_snap);
	}

	// 用紙メニューの「用紙の色」が選択された.
	IAsyncAction MainPage::sheet_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		const float val0 = m_sample_sheet.m_sheet_color.r * COLOR_MAX;
		const float val1 = m_sample_sheet.m_sheet_color.g * COLOR_MAX;
		const float val2 = m_sample_sheet.m_sheet_color.b * COLOR_MAX;

		sample_slider_0().Maximum(255.0);
		sample_slider_0().TickFrequency(1.0);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(val0);
		sheet_slider_set_header<UNDO_OP::SHEET_COLOR, 0>(val0);
		sample_slider_1().Maximum(255.0);
		sample_slider_1().TickFrequency(1.0);
		sample_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_1().Value(val1);
		sheet_slider_set_header<UNDO_OP::SHEET_COLOR, 1>(val1);
		sample_slider_2().Maximum(255.0);
		sample_slider_2().TickFrequency(1.0);
		sample_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_2().Value(val2);
		sheet_slider_set_header<UNDO_OP::SHEET_COLOR, 2>(val2);

		sample_slider_0().Visibility(UI_VISIBLE);
		sample_slider_1().Visibility(UI_VISIBLE);
		sample_slider_2().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::sheet_slider_value_changed<UNDO_OP::SHEET_COLOR, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::sheet_slider_value_changed<UNDO_OP::SHEET_COLOR, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::sheet_slider_value_changed<UNDO_OP::SHEET_COLOR, 2> });
		m_sample_type = SAMPLE_TYPE::NONE;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
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
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_1().Visibility(UI_COLLAPSED);
		sample_slider_2().Visibility(UI_COLLAPSED);
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
		//m_sheet_main.get_anch_color(anch_color);
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
		if (m_event_state == EVENT_STATE::PRESS_AREA) {
			const auto t_draw = m_tool_draw;
			if (t_draw == DRAW_TOOL::SELECT || t_draw == DRAW_TOOL::RECT || t_draw == DRAW_TOOL::TEXT || t_draw == DRAW_TOOL::RULER) {
				m_sheet_main.draw_auxiliary_rect(m_sheet_dx, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAW_TOOL::BEZI) {
				m_sheet_main.draw_auxiliary_bezi(m_sheet_dx, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAW_TOOL::ELLI) {
				m_sheet_main.draw_auxiliary_elli(m_sheet_dx, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAW_TOOL::LINE) {
				m_sheet_main.draw_auxiliary_line(m_sheet_dx, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAW_TOOL::RRECT) {
				m_sheet_main.draw_auxiliary_rrect(m_sheet_dx, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAW_TOOL::POLY) {
				m_sheet_main.draw_auxiliary_poly(m_sheet_dx, m_event_pos_pressed, m_event_pos_curr, m_tool_poly);
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
			status_set_curs();
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
		return Shape::m_default_foreground;
	}

	// 用紙の属性を初期化する.
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
			m_sheet_main.set_font_size(DEF_FONT_SIZE);
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
			m_sheet_main.set_arrow_size(DEF_ARROW_SIZE);
			m_sheet_main.set_arrow_style(ARROW_STYLE::NONE);
			m_sheet_main.set_corner_radius(D2D1_POINT_2F{ DEF_GRID_LEN, DEF_GRID_LEN });
			m_sheet_main.set_fill_color(Shape::m_default_background);
			m_sheet_main.set_font_color(Shape::m_default_foreground);
			m_sheet_main.set_grid_base(DEF_GRID_LEN - 1.0);
			m_sheet_main.set_grid_gray(DEF_GRID_GRAY);
			m_sheet_main.set_grid_emph(GRID_EMPH_0);
			m_sheet_main.set_grid_show(GRID_SHOW::BACK);
			m_sheet_main.set_grid_snap(true);
			m_sheet_main.set_sheet_color(Shape::m_default_background);
			m_sheet_main.set_sheet_scale(1.0);
			const double dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			m_sheet_main.m_sheet_size = DEF_SHEET_SIZE;
			m_sheet_main.set_stroke_cap_style(CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT});
			m_sheet_main.set_stroke_color(Shape::m_default_foreground);
			m_sheet_main.set_stroke_dash_cap(D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT);
			m_sheet_main.set_stroke_dash_patt(DEF_STROKE_DASH_PATT);
			m_sheet_main.set_stroke_dash_style(D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
			m_sheet_main.set_stroke_join_limit(DEF_MITER_LIMIT);
			m_sheet_main.set_stroke_join_style(D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
			m_sheet_main.set_stroke_width(1.0);
			m_sheet_main.set_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			m_sheet_main.set_text_align_t(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
			m_sheet_main.set_text_line_sp(0.0);
			m_sheet_main.set_text_margin(DEF_TEXT_MARGIN);
		}
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
	template <UNDO_OP U, int S> void MainPage::sheet_slider_set_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring text;
		if constexpr (U == UNDO_OP::SHEET_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(m_misc_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_col_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(m_misc_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_col_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(m_misc_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_col_b") + L": " + buf;
			}
		}
		if constexpr (S == 0) {
			sample_slider_0().Header(box_value(text));
		}
		if constexpr (S == 1) {
			sample_slider_1().Header(box_value(text));
		}
		if constexpr (S == 2) {
			sample_slider_2().Header(box_value(text));
		}
		if constexpr (S == 3) {
			sample_slider_3().Header(box_value(text));
		}
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S> void MainPage::sheet_slider_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* s = &m_sample_sheet;
		const auto value = static_cast<float>(args.NewValue());
		sheet_slider_set_header<U, S>(value);
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

		m_sample_sheet.set_attr_to(&m_sheet_main);
		//double pw = m_sheet_main.m_sheet_size.width;
		//double ph = m_sheet_main.m_sheet_size.height;
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_HIDE>(m_misc_len_unit, m_sheet_main.m_sheet_size.width, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
		tx_sheet_width().Text(buf);
		conv_len_to_str<LEN_UNIT_HIDE>(m_misc_len_unit, m_sheet_main.m_sheet_size.height, m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
		tx_sheet_height().Text(buf);
		conv_len_to_str<LEN_UNIT_SHOW>(m_misc_len_unit, sheet_size_max(), m_sheet_dx.m_logical_dpi, g_base + 1.0f, buf);
		tx_sheet_size_max().Text(buf);
		// この時点では, テキストボックスに正しい数値を格納しても, 
		// TextChanged は呼ばれない.
		// プライマリーボタンは使用可能にしておく.
		cd_sheet_size_dialog().IsPrimaryButtonEnabled(true);
		cd_sheet_size_dialog().IsSecondaryButtonEnabled(m_list_shapes.size() > 0);
		const auto d_result = co_await cd_sheet_size_dialog().ShowAsync();
		if (d_result == ContentDialogResult::None) {
			// 「キャンセル」が押された場合,
			co_return;
		}
		else if (d_result == ContentDialogResult::Primary) {
			constexpr wchar_t INVALID_NUM[] = L"str_err_number";

			// 本来, 無効な数値が入力されている場合, 「適用」ボタンは不可になっているので
			// 必要ないエラーチェックだが, 念のため.
			if (swscanf_s(tx_sheet_width().Text().c_str(), L"%f", &m_sheet_main.m_sheet_size.width) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_sheet_width/Header");
				co_return;
			}
			if (swscanf_s(tx_sheet_height().Text().c_str(), L"%f", &m_sheet_main.m_sheet_size.height) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_sheet_height/Header");
				co_return;
			}
			// 用紙の縦横の長さの値をピクセル単位の値に変換する.
			const float g_len = g_base + 1.0f;
			D2D1_SIZE_F p_size{
				static_cast<FLOAT>(conv_len_to_val(m_misc_len_unit, m_sheet_main.m_sheet_size.width, m_sheet_dx.m_logical_dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_val(m_misc_len_unit, m_sheet_main.m_sheet_size.height, m_sheet_dx.m_logical_dpi, g_len))
			};
			if (!equal(p_size, m_sheet_main.m_sheet_size)) {
				// 変換された値が用紙の大きさと異なる場合,
				undo_push_set<UNDO_OP::SHEET_SIZE>(&m_sheet_main, p_size);
				undo_push_null();
				undo_menu_enable();
				sheet_update_bbox();
				sheet_panle_size();
				sheet_draw();
				status_set_curs();
				status_set_grid();
				status_set_sheet();
				status_set_unit();
			}
		}
		else if (d_result == ContentDialogResult::Secondary) {
			D2D1_POINT_2F b_min = { FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_max = { -FLT_MAX, -FLT_MAX };
			D2D1_POINT_2F b_size;

			slist_bound_all(m_list_shapes, b_min, b_max);
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
			status_set_sheet();
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
			value = conv_len_to_val(m_misc_len_unit, value, dpi, g_base + 1.0);
		}
		cd_sheet_size_dialog().IsPrimaryButtonEnabled(cnt == 1 && value >= 1.0 && value < sheet_size_max());
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
		slist_bound_sheet(m_list_shapes, m_sheet_main.m_sheet_size, m_sheet_min, m_sheet_max);
	}

	// 用紙メニューの「表示倍率」が選択された.
	void MainPage::sheet_zoom_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		float scale;
		if (sender == mfi_sheet_zoom_100() ||
			sender == mfi_sheet_zoom_100_2()) {
			scale = 1.0f;
		}
		else if (sender == mfi_sheet_zoom_150() ||
			sender == mfi_sheet_zoom_150_2()) {
			scale = 1.5f;
		}
		else if (sender == mfi_sheet_zoom_200() ||
			sender == mfi_sheet_zoom_200_2()) {
			scale = 2.0f;
		}
		else if (sender == mfi_sheet_zoom_300() ||
			sender == mfi_sheet_zoom_300_2()) {
			scale = 3.0f;
		}
		else if (sender == mfi_sheet_zoom_400() ||
			sender == mfi_sheet_zoom_400_2()) {
			scale = 4.0f;
		}
		else if (sender == mfi_sheet_zoom_075() ||
			sender == mfi_sheet_zoom_075_2()) {
			scale = 0.75f;
		}
		else if (sender == mfi_sheet_zoom_050() ||
			sender == mfi_sheet_zoom_050_2()) {
			scale = 0.5f;
		}
		else if (sender == mfi_sheet_zoom_025() ||
			sender == mfi_sheet_zoom_025_2()) {
			scale = 0.25f;
		}
		else {
			return;
		}
		if (scale != m_sheet_main.m_sheet_scale) {
			m_sheet_main.m_sheet_scale = scale;
			sheet_panle_size();
			sheet_draw();
			status_set_zoom();
		}
	}

	// 表示を拡大または縮小する.
	void MainPage::sheet_zoom_delta(const int32_t delta) noexcept
	{
		if (delta > 0 &&
			m_sheet_main.m_sheet_scale < 4.f / 1.1f - FLT_MIN) {
			m_sheet_main.m_sheet_scale *= 1.1f;
		}
		else if (delta < 0 &&
			m_sheet_main.m_sheet_scale > 0.25f * 1.1f + FLT_MIN) {
			m_sheet_main.m_sheet_scale /= 1.1f;
		}
		else {
			return;
		}
		sheet_panle_size();
		sheet_draw();
		status_set_zoom();
	}
}