//-------------------------------
// MainPage_sheet.cpp
// 用紙の各属性の設定
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
	using winrt::Windows::Storage::ApplicationData;
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

	constexpr wchar_t DLG_TITLE[] = L"str_sheet";	// 用紙の表題
	constexpr wchar_t SHEET_PROP[] = L"sheet_prop.dat";	// 用紙設定を格納するファイル名
	constexpr wchar_t FONT_FAMILY_DEFVAL[] = L"Segoe UI Variable";	// 書体名の規定値 (システムリソースに値が無かった場合)
	constexpr wchar_t FONT_STYLE_DEFVAL[] = L"BodyTextBlockStyle";	// 文字列の規定値を得るシステムリソース

	// 長さををピクセル単位の値に変換する.
	static double conv_len_to_val(const LEN_UNIT l_unit, const double val, const double dpi, const double g_len) noexcept;
	// 設定データを保存するフォルダーを得る.
	static auto prop_local_folder(void);

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

	// 設定データを保存するフォルダーを得る.
	static auto prop_local_folder(void)
	{
		return ApplicationData::Current().LocalFolder();
	}

	// チェックマークを図形の属性関連のメニュー項目につける.
	void MainPage::sheet_attr_is_checked(void) noexcept
	{
		ARROW_STYLE a_style;
		m_main_sheet.get_arrow_style(a_style);
		arrow_style_is_checked(a_style);

		DWRITE_FONT_STYLE f_style;
		m_main_sheet.get_font_style(f_style);
		font_style_is_checked(f_style);

		D2D1_DASH_STYLE s_style;
		m_main_sheet.get_dash_style(s_style);
		dash_style_is_checked(s_style);

		D2D1_LINE_JOIN j_style;
		m_main_sheet.get_join_style(j_style);
		join_style_is_checked(j_style);

		float s_width;
		m_main_sheet.get_stroke_width(s_width);
		stroke_width_is_checked(s_width);

		DWRITE_TEXT_ALIGNMENT t_align_t;
		m_main_sheet.get_text_align_t(t_align_t);
		text_align_t_is_checked(t_align_t);

		DWRITE_PARAGRAPH_ALIGNMENT t_par_align;
		m_main_sheet.get_text_par_align(t_par_align);
		text_par_align_is_checked(t_par_align);

		GRID_EMPH g_emph;
		m_main_sheet.get_grid_emph(g_emph);
		grid_emph_is_checked(g_emph);

		GRID_SHOW g_show;
		m_main_sheet.get_grid_show(g_show);
		grid_show_is_checked(g_show);

		bool g_snap;
		m_main_sheet.get_grid_snap(g_snap);
		tmfi_grid_snap().IsChecked(g_snap);

		float scale;
		m_main_sheet.get_sheet_scale(scale);
		sheet_zoom_is_checked(scale);
	}

	// 用紙メニューの「用紙の色」が選択された.
	IAsyncAction MainPage::sheet_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_prop_sheet.set_attr_to(&m_main_sheet);
		const float val0 = m_prop_sheet.m_sheet_color.r * COLOR_MAX;
		const float val1 = m_prop_sheet.m_sheet_color.g * COLOR_MAX;
		const float val2 = m_prop_sheet.m_sheet_color.b * COLOR_MAX;

		prop_slider_0().Maximum(255.0);
		prop_slider_0().TickFrequency(1.0);
		prop_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_0().Value(val0);
		sheet_slider_set_header<UNDO_OP::SHEET_COLOR, 0>(val0);
		prop_slider_1().Maximum(255.0);
		prop_slider_1().TickFrequency(1.0);
		prop_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_1().Value(val1);
		sheet_slider_set_header<UNDO_OP::SHEET_COLOR, 1>(val1);
		prop_slider_2().Maximum(255.0);
		prop_slider_2().TickFrequency(1.0);
		prop_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_2().Value(val2);
		sheet_slider_set_header<UNDO_OP::SHEET_COLOR, 2>(val2);

		prop_slider_0().Visibility(UI_VISIBLE);
		prop_slider_1().Visibility(UI_VISIBLE);
		prop_slider_2().Visibility(UI_VISIBLE);
		const auto slider_0_token = prop_slider_0().ValueChanged({ this, &MainPage::sheet_slider_val_changed<UNDO_OP::SHEET_COLOR, 0> });
		const auto slider_1_token = prop_slider_1().ValueChanged({ this, &MainPage::sheet_slider_val_changed<UNDO_OP::SHEET_COLOR, 1> });
		const auto slider_2_token = prop_slider_2().ValueChanged({ this, &MainPage::sheet_slider_val_changed<UNDO_OP::SHEET_COLOR, 2> });
		//m_sample_type = PROP_TYPE::NONE;
		//m_prop_sheet.m_d2d.SetSwapChainPanel(scp_prop_panel());
		//const auto samp_w = scp_prop_panel().Width();
		//const auto samp_h = scp_prop_panel().Height();
		//m_prop_sheet.m_sheet_size.width = static_cast<FLOAT>(samp_w);
		//m_prop_sheet.m_sheet_size.height = static_cast<FLOAT>(samp_h);

		cd_prop_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_prop_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F samp_val;
			m_prop_sheet.get_sheet_color(samp_val);
			D2D1_COLOR_F sheet_val;
			m_main_sheet.get_sheet_color(sheet_val);
			if (!equal(sheet_val, samp_val)) {
				ustack_push_set<UNDO_OP::SHEET_COLOR>(&m_main_sheet, samp_val);
				ustack_push_null();
				ustack_is_enable();
				sheet_draw();
			}
		}
		//m_prop_sheet.m_d2d.Release();
		prop_slider_0().Visibility(UI_COLLAPSED);
		prop_slider_1().Visibility(UI_COLLAPSED);
		prop_slider_2().Visibility(UI_COLLAPSED);
		prop_slider_0().ValueChanged(slider_0_token);
		prop_slider_1().ValueChanged(slider_1_token);
		prop_slider_2().ValueChanged(slider_2_token);
	}

	// 用紙を表示する.
	void MainPage::sheet_draw(void)
	{
#if defined(_DEBUG)
		if (!scp_sheet_panel().IsLoaded()) {
			return;
		}
#endif
		if (!m_d2d_mutex.try_lock()) {
			// ロックできない場合
			return;
		}

		// デバイスコンテキストの描画状態を保存ブロックに保持する.
		m_main_sheet.m_d2d.m_d2d_context->SaveDrawingState(m_main_sheet.m_state_block.get());
		// デバイスコンテキストから変換行列を得る.
		D2D1_MATRIX_3X2_F tran;
		m_main_sheet.m_d2d.m_d2d_context->GetTransform(&tran);
		// 拡大率を変換行列の拡大縮小の成分に格納する.
		const auto sheet_scale = max(m_main_sheet.m_sheet_scale, 0.0f);
		tran.m11 = tran.m22 = sheet_scale;
		// スクロールの変分に拡大率を掛けた値を
		// 変換行列の平行移動の成分に格納する.
		D2D1_POINT_2F t_pos;
		pt_add(m_main_min, sb_horz().Value(), sb_vert().Value(), t_pos);
		pt_mul(t_pos, sheet_scale, t_pos);
		tran.dx = -t_pos.x;
		tran.dy = -t_pos.y;
		// 変換行列をデバイスコンテキストに格納する.
		m_main_sheet.m_d2d.m_d2d_context->SetTransform(&tran);
		// 描画を開始する.
		m_main_sheet.m_d2d.m_d2d_context->BeginDraw();
		m_main_sheet.draw(m_main_sheet);
		if (m_event_state == EVENT_STATE::PRESS_AREA) {
			const auto t_draw = m_drawing_tool;
			if (t_draw == DRAWING_TOOL::SELECT || t_draw == DRAWING_TOOL::RECT || t_draw == DRAWING_TOOL::TEXT || t_draw == DRAWING_TOOL::RULER) {
				m_main_sheet.draw_auxiliary_rect(m_main_sheet, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::BEZI) {
				m_main_sheet.draw_auxiliary_bezi(m_main_sheet, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::ELLI) {
				m_main_sheet.draw_auxiliary_elli(m_main_sheet, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::LINE) {
				m_main_sheet.draw_auxiliary_line(m_main_sheet, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::RRECT) {
				m_main_sheet.draw_auxiliary_rrect(m_main_sheet, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::POLY) {
				m_main_sheet.draw_auxiliary_poly(m_main_sheet, m_event_pos_pressed, m_event_pos_curr, m_drawing_poly_opt);
			}
		}
		// 描画を終了する.
		HRESULT hr = m_main_sheet.m_d2d.m_d2d_context->EndDraw();
		// 保存された描画環境を元に戻す.
		m_main_sheet.m_d2d.m_d2d_context->RestoreDrawingState(m_main_sheet.m_state_block.get());
		if (hr == S_OK) {
			// 結果が S_OK の場合,
			// スワップチェーンの内容を画面に表示する.
			m_main_sheet.m_d2d.Present();
			// ポインターの位置をステータスバーに格納する.
			status_bar_set_pos();
		}
#if defined(_DEBUG)
		else {
			// 結果が S_OK でない場合,
			// 「描画できません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, L"str_err_draw", {});
		}
#endif
		m_d2d_mutex.unlock();
	}

	// 前景色を得る.
	const D2D1_COLOR_F& MainPage::sheet_foreground(void) const noexcept
	{
		return Shape::s_foreground_color;
	}

	// 用紙の属性を初期化する.
	void MainPage::sheet_init(void) noexcept
	{
		// 書体の属性を初期化する.
		{
			// リソースの取得に失敗した場合に備えて, 静的な既定値を書体属性に格納する.
			m_main_sheet.set_font_family(wchar_cpy(FONT_FAMILY_DEFVAL));
			m_main_sheet.set_font_size(FONT_SIZE_DEFVAL);
			m_main_sheet.set_font_stretch(DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL);
			m_main_sheet.set_font_style(DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL);
			m_main_sheet.set_font_weight(DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL);
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
								m_main_sheet.set_font_family(wchar_cpy(val.Source().c_str()));
							}
							else if (prop == TextBlock::FontSizeProperty()) {
								// プロパティーが FontSize の場合,
								// セッターの値から, 書体の大きさを得る.
								const auto val = unbox_value<float>(setter.Value());
								m_main_sheet.m_font_size = val;
							}
							else if (prop == TextBlock::FontStretchProperty()) {
								// プロパティーが FontStretch の場合,
								// セッターの値から, 書体の伸縮を得る.
								const auto val = unbox_value<int32_t>(setter.Value());
								m_main_sheet.set_font_stretch(static_cast<DWRITE_FONT_STRETCH>(val));
							}
							else if (prop == TextBlock::FontStyleProperty()) {
								// プロパティーが FontStyle の場合,
								// セッターの値から, 字体を得る.
								const auto val = unbox_value<int32_t>(setter.Value());
								m_main_sheet.set_font_style(static_cast<DWRITE_FONT_STYLE>(val));
							}
							else if (prop == TextBlock::FontWeightProperty()) {
								// プロパティーが FontWeight の場合,
								// セッターの値から, 書体の太さを得る.
								const auto val = unbox_value<int32_t>(setter.Value());
								m_main_sheet.set_font_weight(static_cast<DWRITE_FONT_WEIGHT>(val));
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
			ShapeText::is_available_font(m_main_sheet.m_font_family);
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
			m_main_sheet.set_arrow_size(ARROW_SIZE_DEFVAL);
			m_main_sheet.set_arrow_style(ARROW_STYLE::NONE);
			m_main_sheet.set_corner_radius(D2D1_POINT_2F{ GRID_LEN_DEFVAL, GRID_LEN_DEFVAL });
			m_main_sheet.set_fill_color(Shape::s_background_color);
			m_main_sheet.set_font_color(Shape::s_foreground_color);
			m_main_sheet.set_grid_base(GRID_LEN_DEFVAL - 1.0);
			m_main_sheet.set_grid_color(grid_color);
			m_main_sheet.set_grid_emph(GRID_EMPH_0);
			m_main_sheet.set_grid_show(GRID_SHOW::BACK);
			m_main_sheet.set_grid_snap(true);
			m_main_sheet.set_sheet_color(Shape::s_background_color);
			m_main_sheet.set_sheet_scale(1.0);
			//const double dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			m_main_sheet.m_sheet_size = SHEET_SIZE_DEF_VAL;
			m_main_sheet.set_stroke_cap(CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT});
			m_main_sheet.set_stroke_color(Shape::s_foreground_color);
			m_main_sheet.set_dash_cap(D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT);
			m_main_sheet.set_dash_patt(DASH_PATT_DEFVAL);
			m_main_sheet.set_dash_style(D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
			m_main_sheet.set_join_limit(MITER_LIMIT_DEFVAL);
			m_main_sheet.set_join_style(D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
			m_main_sheet.set_stroke_width(1.0);
			m_main_sheet.set_text_par_align(DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			m_main_sheet.set_text_align_t(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
			m_main_sheet.set_text_line_sp(0.0);
			m_main_sheet.set_text_padding(TEXT_MARGIN_DEFVAL);
		}
	}

	//void MainPage::sheet_xaml_root_changed(winrt::Windows::UI::Xaml::XamlRoot const&, winrt::Windows::UI::Xaml::XamlRootChangedEventArgs const& /*args*/)
	//{
	//}

	//------------------------------
	// 用紙のスワップチェーンパネルのロードが始まった.
	//------------------------------
	void MainPage::sheet_panel_loading(IInspectable const&, IInspectable const&)
	{
		//auto xaml_root = scp_sheet_panel().XamlRoot();
		//xaml_root.Changed({ this, &MainPage::sheet_xaml_root_changed });
	}

	//------------------------------
	// 用紙のスワップチェーンパネルがロードされた.
	//------------------------------
	void MainPage::sheet_panel_loaded(IInspectable const& sender, RoutedEventArgs const&)
	{
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			return;
		}
#endif // _DEBUG
		//auto xaml_root = scp_sheet_panel().XamlRoot();
		//xaml_root.Changed({ this, &MainPage::sheet_xaml_root_changed });

		m_main_sheet.m_d2d.SetSwapChainPanel(scp_sheet_panel());
		m_main_sheet.m_d2d.m_d2d_factory->CreateDrawingStateBlock(m_main_sheet.m_state_block.put());
		m_main_sheet.m_d2d.m_d2d_context->CreateSolidColorBrush({}, m_main_sheet.m_color_brush.put());
		m_main_sheet.m_d2d.m_d2d_context->CreateSolidColorBrush({}, m_main_sheet.m_range_brush.put());
		sheet_draw();
	}

#if defined(_DEBUG)
	//------------------------------
	// 用紙のスワップチェーンパネルの寸法が変わった.
	// args	イベントの引数
	//------------------------------
	void MainPage::sheet_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
#else
	//------------------------------
	// 用紙のスワップチェーンパネルの寸法が変わった.
	// args	イベントの引数
	//------------------------------
	void MainPage::sheet_panel_size_changed(IInspectable const&, SizeChangedEventArgs const& args)
#endif	// _DEBUG
	{
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			return;
		}
#endif	// _DEBUG
		const auto z = args.NewSize();
		const float w = z.Width;
		const float h = z.Height;
		scroll_set(w, h);
		if (scp_sheet_panel().IsLoaded()) {
			m_main_sheet.m_d2d.SetLogicalSize2(D2D1_SIZE_F{ w, h });
			sheet_draw();
		}
	}

	//------------------------------
	// 用紙のスワップチェーンパネルの寸法が変わった.
	//------------------------------
	void MainPage::sheet_panel_scale_changed(IInspectable const&, IInspectable const&)
	{
		m_main_sheet.m_d2d.SetCompositionScale(scp_sheet_panel().CompositionScaleX(), scp_sheet_panel().CompositionScaleY());
	}

	//------------------------------
	// 用紙の大きさを設定する.
	//------------------------------
	void MainPage::sheet_panle_size(void)
	{
		const float w = static_cast<float>(scp_sheet_panel().ActualWidth());
		const float h = static_cast<float>(scp_sheet_panel().ActualHeight());
		if (w > 0.0f && h > 0.0f) {
			scroll_set(w, h);
			m_main_sheet.m_d2d.SetLogicalSize2(D2D1_SIZE_F{ w, h });
		}
	}

	//------------------------------
	// 用紙メニューの「用紙の大きさ」が選択された
	//------------------------------
	IAsyncAction MainPage::sheet_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_prop_sheet.set_attr_to(&m_main_sheet);
		float g_base;
		m_main_sheet.get_grid_base(g_base);
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, m_main_sheet.m_sheet_size.width, m_main_sheet.m_d2d.m_logical_dpi, g_base + 1.0f, buf);
		tx_sheet_width().Text(buf);
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, m_main_sheet.m_sheet_size.height, m_main_sheet.m_d2d.m_logical_dpi, g_base + 1.0f, buf);
		tx_sheet_height().Text(buf);
		conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, sheet_size_max(), m_main_sheet.m_d2d.m_logical_dpi, g_base + 1.0f, buf);
		tx_sheet_size_max().Text(buf);
		// この時点では, テキストボックスに正しい数値を格納しても, TextChanged は呼ばれない.
		// プライマリーボタンは使用可能にしておく.
		cd_sheet_size_dialog().IsPrimaryButtonEnabled(true);
		cd_sheet_size_dialog().IsSecondaryButtonEnabled(m_main_sheet.m_shape_list.size() > 0);
		const auto d_result = co_await cd_sheet_size_dialog().ShowAsync();
		if (d_result == ContentDialogResult::None) {
			// 「キャンセル」が押された場合,
			co_return;
		}
		else if (d_result == ContentDialogResult::Primary) {
			constexpr wchar_t INVALID_NUM[] = L"str_err_number";

			// 本来, 無効な数値が入力されている場合, 「適用」ボタンは不可になっているので
			// 必要ないエラーチェックだが, 念のため.
			if (swscanf_s(tx_sheet_width().Text().c_str(), L"%f", &m_main_sheet.m_sheet_size.width) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_sheet_width/Header");
				co_return;
			}
			if (swscanf_s(tx_sheet_height().Text().c_str(), L"%f", &m_main_sheet.m_sheet_size.height) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_sheet_height/Header");
				co_return;
			}
			// 用紙の縦横の長さの値をピクセル単位の値に変換する.
			const float g_len = g_base + 1.0f;
			D2D1_SIZE_F p_size{
				static_cast<FLOAT>(conv_len_to_val(m_len_unit, m_main_sheet.m_sheet_size.width, m_main_sheet.m_d2d.m_logical_dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_val(m_len_unit, m_main_sheet.m_sheet_size.height, m_main_sheet.m_d2d.m_logical_dpi, g_len))
			};
			if (!equal(p_size, m_main_sheet.m_sheet_size)) {
				// 変換された値が用紙の大きさと異なる場合,
				ustack_push_set<UNDO_OP::SHEET_SIZE>(&m_main_sheet, p_size);
				ustack_push_null();
				ustack_is_enable();
				sheet_update_bbox();
				sheet_panle_size();
				sheet_draw();
				status_bar_set_pos();
				status_bar_set_grid();
				status_bar_set_sheet();
				status_bar_set_unit();
			}
		}
		else if (d_result == ContentDialogResult::Secondary) {
			D2D1_POINT_2F b_min = { FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_max = { -FLT_MAX, -FLT_MAX };
			D2D1_POINT_2F b_size;

			slist_bound_all(m_main_sheet.m_shape_list, b_min, b_max);
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
				ustack_push_move({ dx, dy }, ALL);
				flag = true;
			}
			D2D1_POINT_2F p_min = { 0.0F, 0.0F };
			D2D1_POINT_2F p_max;
			pt_add(b_max, b_min, p_max);
			D2D1_SIZE_F p_size = { p_max.x, p_max.y };
			if (!equal(m_main_sheet.m_sheet_size, p_size)) {
				ustack_push_set<UNDO_OP::SHEET_SIZE>(&m_main_sheet, p_size);
				flag = true;
			}
			if (flag) {
				ustack_push_null();
				ustack_is_enable();
			}
			sheet_update_bbox();
			sheet_panle_size();
			sheet_draw();
			status_bar_set_sheet();
		}
	}

	//------------------------------
	// テキストボックス「用紙の幅」「用紙の高さ」の値が変更された.
	//------------------------------
	void MainPage::sheet_size_text_changed(IInspectable const& sender, TextChangedEventArgs const&)
	{
		const double dpi = m_main_sheet.m_d2d.m_logical_dpi;	// DPI
		double val;
		wchar_t buf[2];

		// テキストボックスの文字列を数値に変換する.
		const int cnt = swscanf_s(unbox_value<TextBox>(sender).Text().c_str(), L"%lf%1s", &val, buf, 2);
		// 文字列が数値に変換できたか判定する.
		// 変換できたなら,
		if (cnt == 1 && val > 0.0) {
			float g_base;
			m_main_sheet.get_grid_base(g_base);
			val = conv_len_to_val(m_len_unit, val, dpi, g_base + 1.0);
		}
		cd_sheet_size_dialog().IsPrimaryButtonEnabled(cnt == 1 && val >= 1.0 && val < sheet_size_max());
	}

	// 値をスライダーのヘッダーに格納する.
	// U	操作の種類
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <UNDO_OP U, int S>
	void MainPage::sheet_slider_set_header(const float val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring text;
		if constexpr (U == UNDO_OP::SHEET_COLOR) {
			constexpr wchar_t* HEADER[]{ L"str_color_r", L"str_color_g",L"str_color_b", L"str_opacity" };
			wchar_t buf[32];
			conv_col_to_str(m_color_code, val, buf);
			text = ResourceLoader::GetForCurrentView().GetString(HEADER[S]) + L": " + buf;
		}
		prop_set_slider_header<S>(text);
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S>
	void MainPage::sheet_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::SHEET_COLOR) {
			const auto val = static_cast<float>(args.NewValue());
			sheet_slider_set_header<U, S>(val);
			D2D1_COLOR_F s_color;
			m_prop_sheet.get_sheet_color(s_color);
			if constexpr (S == 0) {
				s_color.r = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 1) {
				s_color.g = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 2) {
				s_color.b = static_cast<FLOAT>(val / COLOR_MAX);
			}
			m_prop_sheet.set_sheet_color(s_color);
		}
		if (scp_prop_panel().IsLoaded()) {
			prop_sample_draw();
		}
	}

	// 図形が含まれるよう用紙の左上位置と右下位置を更新する.
	// s	図形
	void MainPage::sheet_update_bbox(const Shape* s) noexcept
	{
		s->get_bound(m_main_min, m_main_max, m_main_min, m_main_max);
	}

	// 用紙の左上位置と右下位置を設定する.
	void MainPage::sheet_update_bbox(void) noexcept
	{
		slist_bound_sheet(m_main_sheet.m_shape_list, m_main_sheet.m_sheet_size, m_main_min, m_main_max);
	}

	void MainPage::sheet_zoom_is_checked(float scale)
	{
		rmfi_sheet_zoom_100().IsChecked(equal(scale, 1.0f));
		rmfi_sheet_zoom_150().IsChecked(equal(scale, 1.5f));
		rmfi_sheet_zoom_200().IsChecked(equal(scale, 2.0f));
		rmfi_sheet_zoom_300().IsChecked(equal(scale, 3.0f));
		rmfi_sheet_zoom_400().IsChecked(equal(scale, 4.0f));
		rmfi_sheet_zoom_075().IsChecked(equal(scale, 0.75f));
		rmfi_sheet_zoom_050().IsChecked(equal(scale, 0.5f));
		rmfi_sheet_zoom_025().IsChecked(equal(scale, 0.25f));
	}

	// 用紙メニューの「表示倍率」が選択された.
	void MainPage::sheet_zoom_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		float scale;
		if (sender == rmfi_sheet_zoom_100()) {
			scale = 1.0f;
		}
		else if (sender == rmfi_sheet_zoom_150()) {
			scale = 1.5f;
		}
		else if (sender == rmfi_sheet_zoom_200()) {
			scale = 2.0f;
		}
		else if (sender == rmfi_sheet_zoom_300()) {
			scale = 3.0f;
		}
		else if (sender == rmfi_sheet_zoom_400()) {
			scale = 4.0f;
		}
		else if (sender == rmfi_sheet_zoom_075()) {
			scale = 0.75f;
		}
		else if (sender == rmfi_sheet_zoom_050()) {
			scale = 0.5f;
		}
		else if (sender == rmfi_sheet_zoom_025()) {
			scale = 0.25f;
		}
		else {
			return;
		}
		sheet_zoom_is_checked(scale);
		if (scale != m_main_sheet.m_sheet_scale) {
			m_main_sheet.m_sheet_scale = scale;
			sheet_panle_size();
			sheet_draw();
			status_bar_set_zoom();
		}
	}

	// 表示を拡大または縮小する.
	void MainPage::sheet_zoom_delta(const int32_t delta) noexcept
	{
		if (delta > 0 &&
			m_main_sheet.m_sheet_scale < 4.f / 1.1f - FLT_MIN) {
			m_main_sheet.m_sheet_scale *= 1.1f;
		}
		else if (delta < 0 &&
			m_main_sheet.m_sheet_scale > 0.25f * 1.1f + FLT_MIN) {
			m_main_sheet.m_sheet_scale /= 1.1f;
		}
		else {
			return;
		}
		sheet_panle_size();
		sheet_draw();
		status_bar_set_zoom();
	}

	// 用紙メニューの「用紙設定をリセット」が選択された.
	// 設定データを保存したファイルがある場合, それを削除する.
	IAsyncAction MainPage::sheet_prop_reset_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::Storage::StorageDeleteOption;

		winrt::Windows::Storage::IStorageItem prop_item{
			co_await prop_local_folder().TryGetItemAsync(SHEET_PROP)
		};
		if (prop_item != nullptr) {
			auto preg_file = prop_item.try_as<StorageFile>();
			if (preg_file != nullptr) {
				HRESULT hr = E_FAIL;
				try {
					co_await preg_file.DeleteAsync(StorageDeleteOption::PermanentDelete);
					mfi_sheet_prop_reset().IsEnabled(false);
					hr = S_OK;
				}
				catch (winrt::hresult_error const& e) {
					hr = e.code();
				}
				if (hr != S_OK) {

				}
				preg_file = nullptr;
			}
			prop_item = nullptr;
		}
		sheet_init();
		sheet_draw();
	}

	// 保存された用紙設定データを読み込む.
	// 設定データを保存したファイルがある場合, それを読み込む.
	// 戻り値	読み込めたら S_OK.
	IAsyncOperation<winrt::hresult> MainPage::sheet_prop_load_async(void)
	{
		mfi_sheet_prop_reset().IsEnabled(false);
		winrt::Windows::Storage::IStorageItem prop_item{
			co_await prop_local_folder().TryGetItemAsync(SHEET_PROP)
		};
		HRESULT hr = E_FAIL;
		if (prop_item != nullptr) {
			auto prop_file = prop_item.try_as<StorageFile>();
			if (prop_file != nullptr) {
				try {
					hr = co_await file_read_async<false, true>(prop_file);
				}
				catch (winrt::hresult_error const& e) {
					hr = e.code();
				}
				prop_file = nullptr;
				mfi_sheet_prop_reset().IsEnabled(true);
			}
			prop_item = nullptr;
		}
		co_return hr;
	}

	// 用紙メニューの「用紙設定を保存」が選択された
	// ローカルフォルダーにファイルを作成し, 設定データを保存する.
	// s_file	ストレージファイル
	IAsyncAction MainPage::sheet_prop_save_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		try {
			auto s_file{ 
				co_await prop_local_folder().CreateFileAsync(SHEET_PROP, CreationCollisionOption::ReplaceExisting)
			};
			if (s_file != nullptr) {
				co_await file_write_gpf_async<false, true>(s_file);
				s_file = nullptr;
				mfi_sheet_prop_reset().IsEnabled(true);
			}
		}
		catch (winrt::hresult_error const&) {
		}
		//using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		//auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		//mru_list.Clear();
	}

}