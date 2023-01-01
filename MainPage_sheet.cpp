//-------------------------------
// MainPage_sheet.cpp
// 表示の設定
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	//using winrt::Windows::Foundation::IAsyncAction;
	//using winrt::Windows::Foundation::IAsyncOperation;
	//using winrt::Windows::Graphics::Display::DisplayInformation;
	//using winrt::Windows::Storage::ApplicationData;
	using winrt::Windows::Storage::CreationCollisionOption;
	//using winrt::Windows::Storage::StorageFile;
	//using winrt::Windows::UI::Color;
	using winrt::Windows::UI::Text::FontStretch;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	//using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::UI::Xaml::Controls::TextBlock;
	using winrt::Windows::UI::Xaml::Controls::TextBox;
	//using winrt::Windows::UI::Xaml::Controls::TextChangedEventArgs;
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;
	using winrt::Windows::UI::Xaml::Setter;
	//using winrt::Windows::UI::Xaml::SizeChangedEventArgs;
	using winrt::Windows::Storage::ApplicationData;


	constexpr wchar_t DLG_TITLE[] = L"str_page";	// 表示の表題
	constexpr wchar_t FONT_FAMILY_DEFVAL[] = L"Segoe UI Variable";	// 書体名の規定値 (システムリソースに値が無かった場合)
	constexpr wchar_t FONT_STYLE_DEFVAL[] = L"BodyTextBlockStyle";	// 文字列の規定値を得るシステムリソース

	// 長さををピクセル単位の値に変換する.
	static double conv_len_to_val(const LEN_UNIT l_unit, const double val, const double dpi, const double g_len) noexcept;

	// 長さををピクセル単位の値に変換する.
	// 変換された値は, 0.5 ピクセル単位に丸められる.
	// l_unit	長さの単位
	// l_val	長さの値
	// dpi	DPI
	// g_len	方眼の大きさ
	// 戻り値	ピクセル単位の値
	static double conv_len_to_val(const LEN_UNIT l_unit, const double l_val, const double dpi, const double g_len) noexcept
	{
		double ret;

		if (l_unit == LEN_UNIT::INCH) {
			ret = l_val * dpi;
		}
		else if (l_unit == LEN_UNIT::MILLI) {
			ret = l_val * dpi / MM_PER_INCH;
		}
		else if (l_unit == LEN_UNIT::POINT) {
			ret = l_val * dpi / PT_PER_INCH;
		}
		else if (l_unit == LEN_UNIT::GRID) {
			ret = l_val * g_len;
		}
		else {
			ret = l_val;
		}
		return std::round(2.0 * ret) * 0.5;
	}

	// チェックマークを図形の属性関連のメニュー項目につける.
	void MainPage::page_setting_is_checked(void) noexcept
	{
		ARROW_STYLE a_style;
		m_main_page.get_arrow_style(a_style);
		arrow_style_is_checked(a_style);

		DWRITE_FONT_STYLE f_style;
		m_main_page.get_font_style(f_style);
		font_style_is_checked(f_style);

		D2D1_DASH_STYLE s_style;
		m_main_page.get_dash_style(s_style);
		dash_style_is_checked(s_style);

		D2D1_LINE_JOIN j_style;
		m_main_page.get_join_style(j_style);
		join_style_is_checked(j_style);

		float s_width;
		m_main_page.get_stroke_width(s_width);
		stroke_width_is_checked(s_width);

		DWRITE_TEXT_ALIGNMENT t_align_t;
		m_main_page.get_text_align_t(t_align_t);
		text_align_t_is_checked(t_align_t);

		DWRITE_PARAGRAPH_ALIGNMENT t_par_align;
		m_main_page.get_text_par_align(t_par_align);
		text_par_align_is_checked(t_par_align);

		GRID_EMPH g_emph;
		m_main_page.get_grid_emph(g_emph);
		grid_emph_is_checked(g_emph);

		GRID_SHOW g_show;
		m_main_page.get_grid_show(g_show);
		grid_show_is_checked(g_show);

		bool g_snap;
		m_main_page.get_grid_snap(g_snap);
		tmfi_grid_snap().IsChecked(g_snap);

		float scale;
		m_main_page.get_page_scale(scale);
		page_zoom_is_checked(scale);
	}

	// 方眼メニューの「ページの色」が選択された.
	IAsyncAction MainPage::page_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_dialog_page.set_attr_to(&m_main_page);
		const float val0 = m_dialog_page.m_page_color.r * COLOR_MAX;
		const float val1 = m_dialog_page.m_page_color.g * COLOR_MAX;
		const float val2 = m_dialog_page.m_page_color.b * COLOR_MAX;

		dialog_slider_0().Maximum(255.0);
		dialog_slider_0().TickFrequency(1.0);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val0);
		page_slider_set_header<UNDO_ID::PAGE_COLOR, 0>(val0);
		dialog_slider_1().Maximum(255.0);
		dialog_slider_1().TickFrequency(1.0);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(val1);
		page_slider_set_header<UNDO_ID::PAGE_COLOR, 1>(val1);
		dialog_slider_2().Maximum(255.0);
		dialog_slider_2().TickFrequency(1.0);
		dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_2().Value(val2);
		page_slider_set_header<UNDO_ID::PAGE_COLOR, 2>(val2);

		dialog_slider_0().Visibility(Visibility::Visible);
		dialog_slider_1().Visibility(Visibility::Visible);
		dialog_slider_2().Visibility(Visibility::Visible);
		const auto slider_0_token = dialog_slider_0().ValueChanged({ this, &MainPage::page_slider_val_changed<UNDO_ID::PAGE_COLOR, 0> });
		const auto slider_1_token = dialog_slider_1().ValueChanged({ this, &MainPage::page_slider_val_changed<UNDO_ID::PAGE_COLOR, 1> });
		const auto slider_2_token = dialog_slider_2().ValueChanged({ this, &MainPage::page_slider_val_changed<UNDO_ID::PAGE_COLOR, 2> });
		//m_sample_type = PROP_TYPE::NONE;
		//m_dialog_page.m_d2d.SetSwapChainPanel(scp_dialog_panel());
		//const auto samp_w = scp_dialog_panel().Width();
		//const auto samp_h = scp_dialog_panel().Height();
		//m_dialog_page.m_page_size.width = static_cast<FLOAT>(samp_w);
		//m_dialog_page.m_page_size.height = static_cast<FLOAT>(samp_h);

		cd_setting_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F setting_val;
			m_dialog_page.get_page_color(setting_val);
			D2D1_COLOR_F page_val;
			m_main_page.get_page_color(page_val);
			if (!equal(page_val, setting_val)) {
				ustack_push_set<UNDO_ID::PAGE_COLOR>(&m_main_page, setting_val);
				ustack_push_null();
				ustack_is_enable();
				page_draw();
			}
		}
		//m_dialog_page.m_d2d.Release();
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		dialog_slider_2().Visibility(Visibility::Collapsed);
		dialog_slider_0().ValueChanged(slider_0_token);
		dialog_slider_1().ValueChanged(slider_1_token);
		dialog_slider_2().ValueChanged(slider_2_token);
	}

	// 表示する.
	void MainPage::page_draw(void)
	{
#if defined(_DEBUG)
		if (!scp_page_panel().IsLoaded()) {
			return;
		}
#endif
		if (!m_mutex_draw.try_lock()) {
			// ロックできない場合
			return;
		}
		Shape::s_factory = m_main_d2d.m_d2d_factory.get();
		Shape::s_target = m_main_d2d.m_d2d_context.get();
		Shape::s_dw_factory = m_main_d2d.m_dwrite_factory.get();
		Shape::s_color_brush = m_main_page.m_color_brush.get();
		Shape::s_range_brush = m_main_page.m_range_brush.get();

		ID2D1RenderTarget* const target = Shape::s_target;
		ID2D1SolidColorBrush * const brush = Shape::s_color_brush;

		// デバイスコンテキストの描画状態を保存ブロックに保持する.
		target->SaveDrawingState(m_main_page.m_state_block.get());
		// デバイスコンテキストから変換行列を得る.
		D2D1_MATRIX_3X2_F tran;
		target->GetTransform(&tran);
		// 拡大率を変換行列の拡大縮小の成分に格納する.
		const auto page_scale = max(m_main_page.m_page_scale, 0.0f);
		tran.m11 = tran.m22 = page_scale;
		// スクロールの変分に拡大率を掛けた値を
		// 変換行列の平行移動の成分に格納する.
		D2D1_POINT_2F t_pos;
		pt_add(m_main_min, sb_horz().Value(), sb_vert().Value(), t_pos);
		pt_mul(t_pos, page_scale, t_pos);
		tran.dx = -t_pos.x;
		tran.dy = -t_pos.y;
		// 変換行列をデバイスコンテキストに格納する.
		target->SetTransform(&tran);
		// 描画を開始する.
		target->BeginDraw();
		m_main_page.draw();
		if (m_event_state == EVENT_STATE::PRESS_AREA) {
			const auto t_draw = m_drawing_tool;
			if (t_draw == DRAWING_TOOL::SELECT ||
				t_draw == DRAWING_TOOL::RECT ||
				t_draw == DRAWING_TOOL::TEXT ||
				t_draw == DRAWING_TOOL::RULER) {
				m_main_page.draw_auxiliary_rect(target, brush, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::BEZI) {
				m_main_page.draw_auxiliary_bezi(target, brush, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::ELLI) {
				m_main_page.draw_auxiliary_elli(target, brush, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::LINE) {
				m_main_page.draw_auxiliary_line(target, brush, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::RRECT) {
				m_main_page.draw_auxiliary_rrect(target, brush, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::POLY) {
				m_main_page.draw_auxiliary_poly(target, brush, m_event_pos_pressed, m_event_pos_curr, m_drawing_poly_opt);
			}
		}
		// 描画を終了する.
		HRESULT hr = target->EndDraw();
		// 保存された描画環境を元に戻す.
		target->RestoreDrawingState(m_main_page.m_state_block.get());
		if (hr != S_OK) {
			// 結果が S_OK でない場合,
			// 「描画できません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, L"str_err_draw", {});
		}
		else {
			// 結果が S_OK の場合,
			// スワップチェーンの内容を画面に表示する.
			m_main_d2d.Present();
			// ポインターの位置をステータスバーに格納する.
			status_bar_set_pos();
		}
		m_mutex_draw.unlock();
	}

	// 前景色を得る.
	const D2D1_COLOR_F& MainPage::page_foreground(void) const noexcept
	{
		return Shape::s_foreground_color;
	}

	// 表示の設定を初期化する.
	void MainPage::page_setting_init(void) noexcept
	{
		// 書体の属性を初期化する.
		{
			// リソースの取得に失敗した場合に備えて, 静的な既定値を書体属性に格納する.
			m_main_page.set_font_family(wchar_cpy(FONT_FAMILY_DEFVAL));
			m_main_page.set_font_size(FONT_SIZE_DEFVAL);
			m_main_page.set_font_stretch(DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL);
			m_main_page.set_font_style(DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL);
			m_main_page.set_font_weight(DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL);
			// BodyTextBlockStyle をリソースディクショナリから得る.
			auto resource = Resources().TryLookup(box_value(FONT_STYLE_DEFVAL));
			if (resource != nullptr) {
				auto style = resource.try_as<winrt::Windows::UI::Xaml::Style>();
				std::list<winrt::Windows::UI::Xaml::Style> stack;
				// リソースからスタイルを得る.
				while (style != nullptr) {
					// スタイルが空でない場合.
					// スタイルをスタックに積む.
					stack.push_back(style);
					// スタイルの継承元のスタイルを得る.
					style = style.BasedOn();
				}
				try {
					while (!stack.empty()) {
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
								const auto val = unbox_value<winrt::Windows::UI::Xaml::Media::FontFamily>(setter.Value());
								m_main_page.set_font_family(wchar_cpy(val.Source().c_str()));
							}
							else if (prop == TextBlock::FontSizeProperty()) {
								// プロパティーが FontSize の場合,
								// セッターの値から, 書体の大きさを得る.
								const auto val = unbox_value<float>(setter.Value());
								m_main_page.m_font_size = val;
							}
							else if (prop == TextBlock::FontStretchProperty()) {
								// プロパティーが FontStretch の場合,
								// セッターの値から, 書体の幅の伸縮を得る.
								const auto val = unbox_value<int32_t>(setter.Value());
								m_main_page.set_font_stretch(static_cast<DWRITE_FONT_STRETCH>(val));
							}
							else if (prop == TextBlock::FontStyleProperty()) {
								// プロパティーが FontStyle の場合,
								// セッターの値から, 字体を得る.
								const auto val = unbox_value<int32_t>(setter.Value());
								m_main_page.set_font_style(static_cast<DWRITE_FONT_STYLE>(val));
							}
							else if (prop == TextBlock::FontWeightProperty()) {
								// プロパティーが FontWeight の場合,
								// セッターの値から, 書体の太さを得る.
								const auto val = unbox_value<int32_t>(setter.Value());
								m_main_page.set_font_weight(static_cast<DWRITE_FONT_WEIGHT>(val));
								//Determine the type of a boxed value
								//auto prop = setter.Value().try_as<winrt::Windows::Foundation::IPropertyValue>();
								//auto prop_type = prop.Type();
								//if (prop_type == winrt::Windows::Foundation::PropertyType::Inspectable) {
								// ...
								//}
								//else if (prop_type == winrt::Windows::Foundation::PropertyType::Int32) {
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
			ShapeText::is_available_font(m_main_page.m_font_family);
		}

		{
			const IInspectable& accent_color = Resources().TryLookup(box_value(L"SystemAccentColor"));
			D2D1_COLOR_F grid_color;
			if (accent_color != nullptr) {
				const auto uwp_color = unbox_value<Color>(accent_color);
				conv_uwp_to_color(unbox_value<Color>(accent_color), grid_color);
				grid_color.a = 0.5f;
			}
			else {
				grid_color = GRID_COLOR_DEFVAL;
			}
			m_main_page.set_arrow_size(ARROW_SIZE_DEFVAL);
			m_main_page.set_arrow_style(ARROW_STYLE::NONE);
			m_main_page.set_corner_radius(D2D1_POINT_2F{ GRID_LEN_DEFVAL, GRID_LEN_DEFVAL });
			m_main_page.set_fill_color(Shape::s_background_color);
			m_main_page.set_font_color(Shape::s_foreground_color);
			m_main_page.set_grid_base(GRID_LEN_DEFVAL - 1.0);
			m_main_page.set_grid_color(grid_color);
			m_main_page.set_grid_emph(GRID_EMPH_0);
			m_main_page.set_grid_show(GRID_SHOW::BACK);
			m_main_page.set_grid_snap(true);
			m_main_page.set_page_color(Shape::s_background_color);
			m_main_page.set_page_scale(1.0);
			//const double dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			m_main_page.m_page_size = PAGE_SIZE_DEFVAL;
			m_main_page.set_stroke_cap(CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT});
			m_main_page.set_stroke_color(Shape::s_foreground_color);
			m_main_page.set_dash_cap(D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT);
			m_main_page.set_dash_patt(DASH_PATT_DEFVAL);
			m_main_page.set_dash_style(D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
			m_main_page.set_join_miter_limit(MITER_LIMIT_DEFVAL);
			m_main_page.set_join_style(D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
			m_main_page.set_stroke_width(1.0);
			m_main_page.set_text_par_align(DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			m_main_page.set_text_align_t(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
			m_main_page.set_text_line_sp(0.0);
			m_main_page.set_text_padding(TEXT_PADDING_DEFVAL);
		}
	}

	//------------------------------
	// 表示のスワップチェーンパネルのロードが始まった.
	//------------------------------
	void MainPage::page_panel_loading(IInspectable const&, IInspectable const&)
	{
	}

	//------------------------------
	// 表示のスワップチェーンパネルがロードされた.
	//------------------------------
	void MainPage::page_panel_loaded(IInspectable const& sender, RoutedEventArgs const&)
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif // _DEBUG

		m_main_d2d.SetSwapChainPanel(scp_page_panel());
		m_main_d2d.m_d2d_factory->CreateDrawingStateBlock(m_main_page.m_state_block.put());
		m_main_d2d.m_d2d_context->CreateSolidColorBrush({}, m_main_page.m_color_brush.put());
		m_main_d2d.m_d2d_context->CreateSolidColorBrush({}, m_main_page.m_range_brush.put());
		page_draw();
	}

#if defined(_DEBUG)
	//------------------------------
	// 表示のスワップチェーンパネルの寸法が変わった.
	// args	イベントの引数
	//------------------------------
	void MainPage::page_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
#else
	//------------------------------
	// 表示のスワップチェーンパネルの寸法が変わった.
	// args	イベントの引数
	//------------------------------
	void MainPage::page_panel_size_changed(IInspectable const&, SizeChangedEventArgs const& args)
#endif	// _DEBUG
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif	// _DEBUG
		const auto z = args.NewSize();
		const float w = z.Width;
		const float h = z.Height;
		scroll_set(w, h);
		if (scp_page_panel().IsLoaded()) {
			m_main_d2d.SetLogicalSize2(D2D1_SIZE_F{ w, h });
			page_draw();
		}
	}

	//------------------------------
	// 表示のスワップチェーンパネルの寸法が変わった.
	//------------------------------
	void MainPage::page_panel_scale_changed(IInspectable const&, IInspectable const&)
	{
		m_main_d2d.SetCompositionScale(scp_page_panel().CompositionScaleX(), scp_page_panel().CompositionScaleY());
	}

	//------------------------------
	// 表示の大きさを設定する.
	//------------------------------
	void MainPage::page_panel_size(void)
	{
		const float w = static_cast<float>(scp_page_panel().ActualWidth());
		const float h = static_cast<float>(scp_page_panel().ActualHeight());
		if (w > 0.0f && h > 0.0f) {
			scroll_set(w, h);
			m_main_d2d.SetLogicalSize2(D2D1_SIZE_F{ w, h });
		}
	}

	//------------------------------
	// 方眼メニューの「表示の大きさ」が選択された
	//------------------------------
	IAsyncAction MainPage::page_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_dialog_page.set_attr_to(&m_main_page);
		float g_base;
		m_main_page.get_grid_base(g_base);
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, m_main_page.m_page_size.width, m_main_d2d.m_logical_dpi, g_base + 1.0f, buf);
		tx_page_width().Text(buf);
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, m_main_page.m_page_size.height, m_main_d2d.m_logical_dpi, g_base + 1.0f, buf);
		tx_page_height().Text(buf);
		conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, PAGE_SIZE_MAX, m_main_d2d.m_logical_dpi, g_base + 1.0f, buf);
		tx_page_size_max().Text(buf);
		// この時点では, テキストボックスに正しい数値を格納しても, TextChanged は呼ばれない.
		// プライマリーボタンは使用可能にしておく.
		cd_page_size_dialog().IsPrimaryButtonEnabled(true);
		cd_page_size_dialog().IsSecondaryButtonEnabled(m_main_page.m_shape_list.size() > 0);
		const auto d_result = co_await cd_page_size_dialog().ShowAsync();
		if (d_result == ContentDialogResult::None) {
			// 「キャンセル」が押された場合,
			co_return;
		}
		else if (d_result == ContentDialogResult::Primary) {
			constexpr wchar_t INVALID_NUM[] = L"str_err_number";

			// 本来, 無効な数値が入力されている場合, 「適用」ボタンは不可になっているので
			// 必要ないエラーチェックだが, 念のため.
			if (swscanf_s(tx_page_width().Text().c_str(), L"%f", &m_main_page.m_page_size.width) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_width/Header");
				co_return;
			}
			if (swscanf_s(tx_page_height().Text().c_str(), L"%f", &m_main_page.m_page_size.height) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_height/Header");
				co_return;
			}
			// 表示の縦横の長さの値をピクセル単位の値に変換する.
			const float g_len = g_base + 1.0f;
			D2D1_SIZE_F p_size{
				static_cast<FLOAT>(conv_len_to_val(m_len_unit, m_main_page.m_page_size.width, m_main_d2d.m_logical_dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_val(m_len_unit, m_main_page.m_page_size.height, m_main_d2d.m_logical_dpi, g_len))
			};
			if (!equal(p_size, m_main_page.m_page_size)) {
				// 変換された値が表示の大きさと異なる場合,
				ustack_push_set<UNDO_ID::PAGE_SIZE>(&m_main_page, p_size);
				ustack_push_null();
				ustack_is_enable();
				page_bbox_update();
				page_panel_size();
				page_draw();
				status_bar_set_pos();
				status_bar_set_grid();
				status_bar_set_page();
				status_bar_set_unit();
			}
		}
		else if (d_result == ContentDialogResult::Secondary) {
			D2D1_POINT_2F b_min = { FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_max = { -FLT_MAX, -FLT_MAX };
			D2D1_POINT_2F b_size;

			slist_bound_all(m_main_page.m_shape_list, b_min, b_max);
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
				constexpr auto ANY = true;
				ustack_push_move({ dx, dy }, ANY);
				flag = true;
			}
			D2D1_POINT_2F p_min = { 0.0F, 0.0F };
			D2D1_POINT_2F p_max;
			pt_add(b_max, b_min, p_max);
			D2D1_SIZE_F p_size = { p_max.x, p_max.y };
			if (!equal(m_main_page.m_page_size, p_size)) {
				ustack_push_set<UNDO_ID::PAGE_SIZE>(&m_main_page, p_size);
				flag = true;
			}
			if (flag) {
				ustack_push_null();
				ustack_is_enable();
			}
			page_bbox_update();
			page_panel_size();
			page_draw();
			status_bar_set_page();
		}
	}

	//------------------------------
	// テキストボックス「表示の幅」「表示の高さ」の値が変更された.
	//------------------------------
	void MainPage::page_size_text_changed(IInspectable const& sender, TextChangedEventArgs const&)
	{
		const double dpi = m_main_d2d.m_logical_dpi;	// DPI
		double val;
		wchar_t buf[2];

		// テキストボックスの文字列を数値に変換する.
		const int cnt = swscanf_s(unbox_value<TextBox>(sender).Text().c_str(), L"%lf%1s", &val, buf, 2);
		// 文字列が数値に変換できたか判定する.
		// 変換できたなら,
		if (cnt == 1 && val > 0.0) {
			float g_base;
			m_main_page.get_grid_base(g_base);
			val = conv_len_to_val(m_len_unit, val, dpi, g_base + 1.0);
		}
		cd_page_size_dialog().IsPrimaryButtonEnabled(cnt == 1 && val >= 1.0 && val < PAGE_SIZE_MAX);
	}

	// 値をスライダーのヘッダーに格納する.
	// U	操作の種類
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <UNDO_ID U, int S>
	void MainPage::page_slider_set_header(const float val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring text;
		if constexpr (U == UNDO_ID::PAGE_COLOR) {
			constexpr wchar_t* HEADER[]{ L"str_color_r", L"str_color_g",L"str_color_b", L"str_opacity" };
			wchar_t buf[32];
			conv_col_to_str(m_color_code, val, buf);
			text = ResourceLoader::GetForCurrentView().GetString(HEADER[S]) + L": " + buf;
		}
		dialog_set_slider_header<S>(text);
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_ID U, int S>
	void MainPage::page_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_ID::PAGE_COLOR) {
			const auto val = static_cast<float>(args.NewValue());
			page_slider_set_header<U, S>(val);
			D2D1_COLOR_F s_color;
			m_dialog_page.get_page_color(s_color);
			if constexpr (S == 0) {
				s_color.r = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 1) {
				s_color.g = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 2) {
				s_color.b = static_cast<FLOAT>(val / COLOR_MAX);
			}
			m_dialog_page.set_page_color(s_color);
		}
		if (scp_dialog_panel().IsLoaded()) {
			dialog_draw();
		}
	}

	// 図形が含まれるよう表示の左上位置と右下位置を更新する.
	// s	図形
	void MainPage::page_bbox_update(const Shape* s) noexcept
	{
		s->get_bound(m_main_min, m_main_max, m_main_min, m_main_max);
	}

	// 表示の左上位置と右下位置を設定する.
	void MainPage::page_bbox_update(void) noexcept
	{
		slist_bound_view(m_main_page.m_shape_list, m_main_page.m_page_size, m_main_min, m_main_max);
	}

	void MainPage::page_zoom_is_checked(float scale)
	{
		rmfi_page_zoom_100().IsChecked(equal(scale, 1.0f));
		rmfi_page_zoom_150().IsChecked(equal(scale, 1.5f));
		rmfi_page_zoom_200().IsChecked(equal(scale, 2.0f));
		rmfi_page_zoom_300().IsChecked(equal(scale, 3.0f));
		rmfi_page_zoom_400().IsChecked(equal(scale, 4.0f));
		rmfi_page_zoom_075().IsChecked(equal(scale, 0.75f));
		rmfi_page_zoom_050().IsChecked(equal(scale, 0.5f));
		rmfi_page_zoom_025().IsChecked(equal(scale, 0.25f));
	}

	// 方眼メニューの「ページの倍率」が選択された.
	void MainPage::page_zoom_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		float scale;
		if (sender == rmfi_page_zoom_100()) {
			scale = 1.0f;
		}
		else if (sender == rmfi_page_zoom_150()) {
			scale = 1.5f;
		}
		else if (sender == rmfi_page_zoom_200()) {
			scale = 2.0f;
		}
		else if (sender == rmfi_page_zoom_300()) {
			scale = 3.0f;
		}
		else if (sender == rmfi_page_zoom_400()) {
			scale = 4.0f;
		}
		else if (sender == rmfi_page_zoom_075()) {
			scale = 0.75f;
		}
		else if (sender == rmfi_page_zoom_050()) {
			scale = 0.5f;
		}
		else if (sender == rmfi_page_zoom_025()) {
			scale = 0.25f;
		}
		else {
			return;
		}
		page_zoom_is_checked(scale);
		if (scale != m_main_page.m_page_scale) {
			m_main_page.m_page_scale = scale;
			page_panel_size();
			page_draw();
			status_bar_set_zoom();
		}
	}

	// 表示を拡大または縮小する.
	void MainPage::page_zoom_delta(const int32_t delta) noexcept
	{
		if (delta > 0 &&
			m_main_page.m_page_scale < 4.f / 1.1f - FLT_MIN) {
			m_main_page.m_page_scale *= 1.1f;
		}
		else if (delta < 0 &&
			m_main_page.m_page_scale > 0.25f * 1.1f + FLT_MIN) {
			m_main_page.m_page_scale /= 1.1f;
		}
		else {
			return;
		}
		page_panel_size();
		page_draw();
		status_bar_set_zoom();
	}

	// 方眼メニューの「用紙設定をリセット」が選択された.
	// 設定データを保存したファイルがある場合, それを削除する.
	IAsyncAction MainPage::page_setting_reset_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::Storage::StorageDeleteOption;

		winrt::Windows::Storage::IStorageItem setting_item{
			co_await ApplicationData::Current().LocalFolder().TryGetItemAsync(PAGE_SETTING)
		};
		if (setting_item != nullptr) {
			auto delete_file = setting_item.try_as<StorageFile>();
			if (delete_file != nullptr) {
				HRESULT hr = E_FAIL;
				try {
					co_await delete_file.DeleteAsync(StorageDeleteOption::PermanentDelete);
					mfi_page_setting_reset().IsEnabled(false);
					hr = S_OK;
				}
				catch (winrt::hresult_error const& e) {
					hr = e.code();
				}
				if (hr != S_OK) {

				}
				delete_file = nullptr;
			}
			setting_item = nullptr;
		}
		page_setting_init();
		page_draw();
	}

	// 方眼メニューの「表示設定を保存」が選択された
	// ローカルフォルダーにファイルを作成し, 設定データを保存する.
	IAsyncAction MainPage::page_setting_save_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		auto setting_file{ 
			co_await ApplicationData::Current().LocalFolder().CreateFileAsync(PAGE_SETTING, CreationCollisionOption::ReplaceExisting)
		};
		if (setting_file != nullptr) {
			co_await file_write_gpf_async<false, true>(setting_file);
			setting_file = nullptr;
			mfi_page_setting_reset().IsEnabled(true);
		}
		//using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		//auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		//mru_list.Clear();
	}

}