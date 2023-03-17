//-------------------------------
// MainPage_page.cpp
// 表示の設定
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::Storage::CreationCollisionOption;
	using winrt::Windows::UI::Text::FontStretch;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::UI::Xaml::Controls::TextBlock;
	using winrt::Windows::UI::Xaml::Controls::TextBox;
	using winrt::Windows::UI::Xaml::Setter;
	using winrt::Windows::Storage::ApplicationData;

	constexpr wchar_t DLG_TITLE[] = L"str_page_settings";	// 表示の表題
	constexpr wchar_t FONT_FAMILY_DEFVAL[] = L"Segoe UI Variable";	// 書体名の規定値 (システムリソースに値が無かった場合)
	constexpr wchar_t FONT_STYLE_DEFVAL[] = L"BodyTextBlockStyle";	// 文字列の規定値を得るシステムリソース

	void MainPage::page_background_pattern_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == tmfi_page_background_pattern()) {
			if (m_background_show != tmfi_page_background_pattern().IsChecked()) {
				m_background_show = tmfi_page_background_pattern().IsChecked();
				page_draw();
			}
		}
		else if (sender == rmfi_page_background_white()) {
			if (!equal(m_background_color, COLOR_WHITE)) {
				m_background_color = COLOR_WHITE;
				page_draw();
			}
		}
		else if (sender == rmfi_page_background_black()) {
			if (!equal(m_background_color, COLOR_BLACK)) {
				m_background_color = COLOR_BLACK;
				page_draw();
			}
		}
	}

	// 図形が含まれるよう表示の左上位置と右下位置を更新する.
	// s	図形
	void MainPage::page_bbox_update(const Shape* s) noexcept
	{
		s->get_bound(m_main_bbox_lt, m_main_bbox_rb, m_main_bbox_lt, m_main_bbox_rb);
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

		DWRITE_TEXT_ALIGNMENT t_align_horz;
		m_main_page.get_text_align_horz(t_align_horz);
		text_align_horz_is_checked(t_align_horz);

		DWRITE_PARAGRAPH_ALIGNMENT t_align_vert;
		m_main_page.get_text_align_vert(t_align_vert);
		text_align_vert_is_checked(t_align_vert);

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
		const float val0 = static_cast<float>(conv_color_comp(m_dialog_page.m_page_color.r));
		const float val1 = static_cast<float>(conv_color_comp(m_dialog_page.m_page_color.g));
		const float val2 = static_cast<float>(conv_color_comp(m_dialog_page.m_page_color.b));
		const float val3 = static_cast<float>(conv_color_comp(m_dialog_page.m_page_color.a));

		dialog_slider_0().Maximum(255.0);
		dialog_slider_0().TickFrequency(1.0);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val0);
		page_slider_set_header<0>(val0);
		dialog_slider_1().Maximum(255.0);
		dialog_slider_1().TickFrequency(1.0);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(val1);
		page_slider_set_header<1>(val1);
		dialog_slider_2().Maximum(255.0);
		dialog_slider_2().TickFrequency(1.0);
		dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_2().Value(val2);
		page_slider_set_header<2>(val2);
		dialog_slider_3().Maximum(255.0);
		dialog_slider_3().TickFrequency(1.0);
		dialog_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_3().Value(val3);
		page_slider_set_header<3>(val3);

		dialog_slider_0().Visibility(Visibility::Visible);
		dialog_slider_1().Visibility(Visibility::Visible);
		dialog_slider_2().Visibility(Visibility::Visible);
		dialog_slider_3().Visibility(Visibility::Visible);
		const auto slider_0_token = dialog_slider_0().ValueChanged(
			{ this, &MainPage::page_slider_val_changed<0> });
		const auto slider_1_token = dialog_slider_1().ValueChanged(
			{ this, &MainPage::page_slider_val_changed<1> });
		const auto slider_2_token = dialog_slider_2().ValueChanged(
			{ this, &MainPage::page_slider_val_changed<2> });
		const auto slider_3_token = dialog_slider_3().ValueChanged(
			{ this, &MainPage::page_slider_val_changed<3> });
		cd_setting_dialog().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		m_mutex_event.lock();
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F setting_val;
			m_dialog_page.get_page_color(setting_val);
			D2D1_COLOR_F page_val;
			m_main_page.get_page_color(page_val);
			if (!equal(page_val, setting_val)) {
				ustack_push_set<UNDO_T::PAGE_COLOR>(&m_main_page, setting_val);
				ustack_push_null();
				ustack_is_enable();
				page_draw();
			}
		}
		//m_dialog_page.m_d2d.Release();
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		dialog_slider_2().Visibility(Visibility::Collapsed);
		dialog_slider_3().Visibility(Visibility::Collapsed);
		dialog_slider_0().ValueChanged(slider_0_token);
		dialog_slider_1().ValueChanged(slider_1_token);
		dialog_slider_2().ValueChanged(slider_2_token);
		dialog_slider_3().ValueChanged(slider_3_token);
		status_bar_set_pos();
		m_mutex_event.unlock();
	}

	// 表示する.
	void MainPage::page_draw(void)
	{
		if (!scp_page_panel().IsLoaded()) {
			return;
		}
		if (!m_mutex_draw.try_lock()) {
			// ロックできない場合
			return;
		}

		const auto p_scale = max(m_main_page.m_page_scale, 0.0f);
		m_main_page.begin_draw(m_main_d2d.m_d2d_context.get(), true, m_background.get(), p_scale);
		m_main_d2d.m_d2d_context->SaveDrawingState(Shape::m_state_block.get());

		// 描画を開始する.
		m_main_d2d.m_d2d_context->BeginDraw();
		m_main_d2d.m_d2d_context->Clear(m_background_color);
		// 背景パターンを描画する,
		if (m_background_show) {
			const D2D1_RECT_F w_rect{
				0, 0, m_main_d2d.m_logical_width, m_main_d2d.m_logical_height
			};
			m_main_d2d.m_d2d_context->FillRectangle(w_rect, Shape::m_d2d_bitmap_brush.get());
		}
		// 変換行列に, 拡大率の値とスクロールの変分に拡大率を掛けた値を格納する.
		D2D1_MATRIX_3X2_F tran{};
		tran.m11 = tran.m22 = p_scale;
		D2D1_POINT_2F lt;	// 表示されている左上位置
		pt_add(m_main_bbox_lt, sb_horz().Value(), sb_vert().Value(), lt);
		pt_mul(lt, p_scale, lt);
		tran.dx = -lt.x;
		tran.dy = -lt.y;
		// 変換行列をデバイスコンテキストに格納する.
		m_main_d2d.m_d2d_context->SetTransform(&tran);

		m_main_page.draw();
		if (m_event_state == EVENT_STATE::PRESS_RECT) {
			if (m_drawing_tool == DRAWING_TOOL::SELECT ||
				m_drawing_tool == DRAWING_TOOL::RECT ||
				m_drawing_tool == DRAWING_TOOL::TEXT ||
				m_drawing_tool == DRAWING_TOOL::RULER) {
				m_main_page.auxiliary_draw_rect(
					//m_main_d2d.m_d2d_context.get(), Shape::m_d2d_color_brush.get(),
					m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::BEZIER) {
				m_main_page.auxiliary_draw_bezi(
					//m_main_d2d.m_d2d_context.get(), Shape::m_d2d_color_brush.get(),
					m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::ELLIPSE) {
				m_main_page.auxiliary_draw_elli(
					//m_main_d2d.m_d2d_context.get(), Shape::m_d2d_color_brush.get(),
					m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::LINE) {
				m_main_page.auxiliary_draw_line(
					//m_main_d2d.m_d2d_context.get(), Shape::m_d2d_color_brush.get(),
					m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::RRECT) {
				m_main_page.auxiliary_draw_rrect(
					//m_main_d2d.m_d2d_context.get(), Shape::m_d2d_color_brush.get(), 
					m_event_pos_pressed, m_event_pos_curr);
			}
			else if (m_drawing_tool == DRAWING_TOOL::POLY) {
				m_main_page.auxiliary_draw_poly(
					//m_main_d2d.m_d2d_context.get(), Shape::m_d2d_color_brush.get(),
					m_event_pos_pressed, m_event_pos_curr, m_drawing_poly_opt);
			}
			else if (m_drawing_tool == DRAWING_TOOL::ARC) {
				m_main_page.auxiliary_draw_arc(
					//m_main_d2d.m_d2d_context.get(), Shape::m_d2d_color_brush.get(),
					m_event_pos_pressed, m_event_pos_curr);
			}
		}
		/*
		if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
			ID2D1Factory* factory = Shape::s_d2d_factory;
			winrt::com_ptr<ID2D1PathGeometry> geom;
			winrt::check_hresult(
				factory->CreatePathGeometry(geom.put())
			);
			winrt::com_ptr<ID2D1GeometrySink> sink;
			winrt::check_hresult(
				geom->Open(sink.put())
			);
			D2D1_POINT_2F p{
				m_event_pos_curr.x + 2.0f,
				m_event_pos_curr.y - 2.0f
			};
			sink->BeginFigure(p, D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED);
			p.y -= 5.0f;
			sink->AddLine(p);
			p.x += 4.0f;
			p.y -= 4.0f;
			sink->AddLine(p);
			p.x += 10.0f;
			sink->AddLine(p);
			p.x -= 8.0f;
			p.y += 8.0f;
			sink->AddLine(p);
			sink->EndFigure(D2D1_FIGURE_END_CLOSED);
			sink->Close();
			sink = nullptr;
			brush->SetColor(m_eyedropper_color);
			target->FillGeometry(geom.get(), brush);
			geom = nullptr;
		}
		*/
		// 描画を終了する.
		const HRESULT hres = m_main_d2d.m_d2d_context->EndDraw();
		// 保存された描画環境を元に戻す.
		m_main_d2d.m_d2d_context->RestoreDrawingState(Shape::m_state_block.get());
		if (hres != S_OK) {
			// 結果が S_OK でない場合,
			// 「描画できません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, L"str_err_draw", {});
		}
		else {
			// 結果が S_OK の場合,
			// スワップチェーンの内容を画面に表示する.
			m_main_d2d.Present();
		}
		m_mutex_draw.unlock();
	}

	// 前景色を得る.
	//const D2D1_COLOR_F& MainPage::page_foreground(void) const noexcept
	//{
	//	return Shape::s_foreground_color;
	//}

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
								// セッターの値から, 書体の幅を得る.
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
			const IInspectable& accent_color = Resources().TryLookup(
				box_value(L"SystemAccentColor"));
			D2D1_COLOR_F grid_color;
			if (accent_color != nullptr) {
				const auto uwp_color = unbox_value<Color>(accent_color);
				conv_uwp_to_color(uwp_color, grid_color);
				grid_color.a = GRID_COLOR_DEFVAL.a;
			}
			else {
				grid_color = GRID_COLOR_DEFVAL;
			}
			m_main_page.set_arrow_size(ARROW_SIZE_DEFVAL);
			m_main_page.set_arrow_style(ARROW_STYLE::NONE);
			m_main_page.set_corner_radius(D2D1_POINT_2F{ GRID_LEN_DEFVAL, GRID_LEN_DEFVAL });
			m_main_page.set_fill_color(COLOR_WHITE);
			m_main_page.set_font_color(COLOR_BLACK);
			m_main_page.set_grid_base(GRID_LEN_DEFVAL - 1.0);
			m_main_page.set_grid_color(grid_color);
			m_main_page.set_grid_emph(GRID_EMPH_0);
			m_main_page.set_grid_show(GRID_SHOW::BACK);
			m_main_page.set_grid_snap(true);
			m_main_page.set_page_color(COLOR_WHITE);
			m_main_page.set_page_scale(1.0);
			//const double dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			m_main_page.set_page_size(PAGE_SIZE_DEFVAL);
			m_main_page.set_page_pad(D2D1_RECT_F{ 0.0f, 0.0f, 0.0f, 0.0f });
			m_main_page.set_stroke_cap(CAP_STYLE_FLAT);
			m_main_page.set_stroke_color(COLOR_BLACK);
			m_main_page.set_dash_cap(D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT);
			m_main_page.set_dash_pat(DASH_PAT_DEFVAL);
			m_main_page.set_dash_style(D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
			m_main_page.set_join_miter_limit(JOIN_MITER_LIMIT_DEFVAL);
			m_main_page.set_join_style(D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
			m_main_page.set_stroke_width(1.0);
			m_main_page.set_text_align_vert(
				DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			m_main_page.set_text_align_horz(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
			m_main_page.set_text_line_sp(0.0);
			m_main_page.set_text_pad(TEXT_PAD_DEFVAL);
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
		page_draw();
	}

	//------------------------------
	// 表示のスワップチェーンパネルの寸法が変わった.
	// args	イベントの引数
	//------------------------------
	void MainPage::page_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
	{
		if (sender != scp_page_panel()) {
			return;
		}
		const auto z = args.NewSize();
		const float w = z.Width;
		const float h = z.Height;
		scroll_set(w, h);
		if (scp_page_panel().IsLoaded()) {
			m_main_d2d.SetLogicalSize2(D2D1_SIZE_F{ w, h });
			page_draw();
		}
		status_bar_set_pos();
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
		const double g_len = m_main_page.m_grid_base + 1.0;
		const double dpi = m_main_d2d.m_logical_dpi;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(
			m_len_unit, m_main_page.m_page_size.width, dpi, g_len, buf);
		tx_page_size_width().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(
			m_len_unit, m_main_page.m_page_size.height, dpi, g_len, buf);
		tx_page_size_height().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(
			m_len_unit, m_main_page.m_page_pad.left, dpi, g_len, buf);
		tx_page_pad_left().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(
			m_len_unit, m_main_page.m_page_pad.top, dpi, g_len, buf);
		tx_page_pad_top().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(
			m_len_unit, m_main_page.m_page_pad.right, dpi, g_len, buf);
		tx_page_pad_right().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(
			m_len_unit, m_main_page.m_page_pad.bottom, dpi, g_len, buf);
		tx_page_pad_bottom().Text(buf);

		if (m_len_unit == LEN_UNIT::GRID) {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_grid()));
		}
		else if (m_len_unit == LEN_UNIT::INCH) {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_inch()));
		}
		else if (m_len_unit == LEN_UNIT::MILLI) {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_milli()));
		}
		else if (m_len_unit == LEN_UNIT::POINT) {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_point()));
		}
		else {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_pixel()));
		}


		// この時点では, テキストボックスに正しい数値を格納しても, TextChanged は呼ばれない.
		// プライマリーボタンは使用可能にしておく.
		cd_page_size_dialog().IsPrimaryButtonEnabled(true);
		cd_page_size_dialog().IsSecondaryButtonEnabled(m_main_page.m_shape_list.size() > 0);
		const auto d_result = co_await cd_page_size_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			const D2D1_SIZE_F old_val = m_main_page.m_page_size;
			constexpr wchar_t INVALID_NUM[] = L"str_err_number";

			// 本来, 無効な数値が入力されている場合, 「適用」ボタンは不可になっているので
			// 必要ないエラーチェックだが, 念のため.
			float new_width;
			if (swscanf_s(tx_page_size_width().Text().c_str(), L"%f", &new_width) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_size_width/Header");
				co_return;
			}
			float new_height;
			if (swscanf_s(tx_page_size_height().Text().c_str(), L"%f", &new_height) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_size_height/Header");
				co_return;
			}
			float new_left;
			if (swscanf_s(tx_page_pad_left().Text().c_str(), L"%f", &new_left) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_pad_left/Header");
				co_return;
			}
			float new_top;
			if (swscanf_s(tx_page_pad_top().Text().c_str(), L"%f", &new_top) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_pad_top/Header");
				co_return;
			}
			float new_right;
			if (swscanf_s(tx_page_pad_right().Text().c_str(), L"%f", &new_right) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_pad_right/Header");
				co_return;
			}
			float new_bottom;
			if (swscanf_s(tx_page_pad_bottom().Text().c_str(), L"%f", &new_bottom) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_pad_bottom/Header");
				co_return;
			}
			if (cbi_len_unit_grid().IsSelected()) {
				m_len_unit = LEN_UNIT::GRID;
			}
			else if (cbi_len_unit_inch().IsSelected()) {
				m_len_unit = LEN_UNIT::INCH;
			}
			else if (cbi_len_unit_milli().IsSelected()) {
				m_len_unit = LEN_UNIT::MILLI;
			}
			else if (cbi_len_unit_grid().IsSelected()) {
				m_len_unit = LEN_UNIT::GRID;
			}
			else if (cbi_len_unit_point().IsSelected()) {
				m_len_unit = LEN_UNIT::POINT;
			}
			else {
				m_len_unit = LEN_UNIT::PIXEL;
			}
			len_unit_is_checked(m_len_unit);

			// 表示の縦横の長さの値をピクセル単位の値に変換する.
			D2D1_SIZE_F p_size{
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_width, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_height, dpi, g_len))
			};
			D2D1_RECT_F p_pad{
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_left, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_top, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_right, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_bottom, dpi, g_len))
			};
			const bool flag_size = !equal(p_size, m_main_page.m_page_size);
			const bool flag_pad = !equal(p_pad, m_main_page.m_page_pad);
			if (flag_size || flag_pad) {
				if (flag_size) {
					ustack_push_set<UNDO_T::PAGE_SIZE>(&m_main_page, p_size);
				}
				if (flag_pad) {
					ustack_push_set<UNDO_T::PAGE_PAD>(&m_main_page, p_pad);
				}
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
			D2D1_POINT_2F b_lt = { FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_rb = { -FLT_MAX, -FLT_MAX };
			D2D1_POINT_2F b_size;

			slist_bound_all(m_main_page.m_shape_list, b_lt, b_rb);
			pt_sub(b_rb, b_lt, b_size);
			if (b_size.x < 1.0F || b_size.y < 1.0F) {
				co_return;
			}
			float dx = 0.0F;
			float dy = 0.0F;
			if (b_lt.x < 0.0F) {
				dx = -b_lt.x;
				b_lt.x = 0.0F;
				b_rb.x += dx;
			}
			if (b_lt.y < 0.0F) {
				dy = -b_lt.y;
				b_lt.y = 0.0F;
				b_rb.y += dy;
			}
			bool flag = false;
			if (dx > 0.0F || dy > 0.0F) {
				constexpr auto ANY = true;
				ustack_push_move({ dx, dy }, ANY);
				flag = true;
			}
			D2D1_POINT_2F p_max;
			pt_add(b_rb, b_lt, p_max);
			D2D1_SIZE_F p_size{
				m_main_page.m_page_pad.left + p_max.x + m_main_page.m_page_pad.right,
				m_main_page.m_page_pad.top + p_max.y + m_main_page.m_page_pad.bottom
			};
			if (!equal(m_main_page.m_page_size, p_size)) {
				ustack_push_set<UNDO_T::PAGE_SIZE>(&m_main_page, p_size);
				ustack_push_null();
				ustack_is_enable();
			}
			page_bbox_update();
			page_panel_size();
			page_draw();
			status_bar_set_page();
		}
		status_bar_set_pos();
	}

	//------------------------------
	// テキストボックス「ページの幅」「ページの高さ」の値が変更された.
	//------------------------------
	void MainPage::page_size_text_changed(IInspectable const&, TextChangedEventArgs const&)
	{
		const double dpi = m_main_d2d.m_logical_dpi;	// DPI
		const auto g_len = m_main_page.m_grid_base + 1.0;
		double w;
		if (swscanf_s(tx_page_size_width().Text().c_str(), L"%lf", &w) != 1) {
			return;
		}
		double h;
		if (swscanf_s(tx_page_size_height().Text().c_str(), L"%lf", &h) != 1) {
			return;
		}
		double l;
		if (swscanf_s(tx_page_pad_left().Text().c_str(), L"%lf", &l) != 1) {
			return;
		}
		double t;
		if (swscanf_s(tx_page_pad_top().Text().c_str(), L"%lf", &t) != 1) {
			return;
		}
		double r;
		if (swscanf_s(tx_page_pad_right().Text().c_str(), L"%lf", &r) != 1) {
			return;
		}
		double b;
		if (swscanf_s(tx_page_pad_bottom().Text().c_str(), L"%lf", &b) != 1) {
			return;
		}
		LEN_UNIT u;
		if (cbi_len_unit_grid().IsSelected()) {
			u = LEN_UNIT::GRID;
		}
		else if (cbi_len_unit_inch().IsSelected()) {
			u = LEN_UNIT::INCH;
		}
		else if (cbi_len_unit_milli().IsSelected()) {
			u = LEN_UNIT::MILLI;
		}
		else if (cbi_len_unit_point().IsSelected()) {
			u = LEN_UNIT::POINT;
		}
		else {
			u = LEN_UNIT::PIXEL;
		}
		if (u != LEN_UNIT::PIXEL) {
			w = conv_len_to_pixel(LEN_UNIT::GRID, w, dpi, g_len);
			h = conv_len_to_pixel(LEN_UNIT::GRID, h, dpi, g_len);
			l = conv_len_to_pixel(LEN_UNIT::GRID, l, dpi, g_len);
			t = conv_len_to_pixel(LEN_UNIT::GRID, t, dpi, g_len);
			r = conv_len_to_pixel(LEN_UNIT::GRID, r, dpi, g_len);
			b = conv_len_to_pixel(LEN_UNIT::GRID, b, dpi, g_len);
		}
		if (w >= 1.0 && w < PAGE_SIZE_MAX && l + r < w &&
			h >= 1.0 && h < PAGE_SIZE_MAX && t + b < h) {
			cd_page_size_dialog().IsPrimaryButtonEnabled(true);
		}
		else {
			cd_page_size_dialog().IsPrimaryButtonEnabled(false);
		}

	}

	// 値をスライダーのヘッダーに格納する.
	// U	操作の識別子
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <int S>
	void MainPage::page_slider_set_header(const float val)
	{
		constexpr wchar_t* T[]{
			L"str_color_r", L"str_color_g",L"str_color_b", L"str_opacity"
		};
		wchar_t buf[32];
		conv_col_to_str(m_color_code, val, buf);
		dialog_set_slider_header<S>(
			ResourceLoader::GetForCurrentView().GetString(T[S]) + L": " + buf);
	}

	// スライダーの値が変更された.
	// U	操作の識別子
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <int S>
	void MainPage::page_slider_val_changed(
		IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (S == 0) {
			const auto val = static_cast<float>(args.NewValue());
			page_slider_set_header<S>(val);
			D2D1_COLOR_F s_color;
			m_dialog_page.get_page_color(s_color);
			s_color.r = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_dialog_page.set_page_color(s_color)) {
				dialog_draw();
			}
		}
		else if constexpr (S == 1) {
			const auto val = static_cast<float>(args.NewValue());
			page_slider_set_header<S>(val);
			D2D1_COLOR_F s_color;
			m_dialog_page.get_page_color(s_color);
			s_color.g = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_dialog_page.set_page_color(s_color)) {
				dialog_draw();
			}
		}
		else if constexpr (S == 2) {
			const auto val = static_cast<float>(args.NewValue());
			page_slider_set_header<S>(val);
			D2D1_COLOR_F s_color;
			m_dialog_page.get_page_color(s_color);
			s_color.b = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_dialog_page.set_page_color(s_color)) {
				dialog_draw();
			}
		}
		else if constexpr (S == 3) {
			const auto val = static_cast<float>(args.NewValue());
			page_slider_set_header<S>(val);
			D2D1_COLOR_F s_color;
			m_dialog_page.get_page_color(s_color);
			s_color.a = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_dialog_page.set_page_color(s_color)) {
				dialog_draw();
			}
		}
	}

	// 表示の左上位置と右下位置を設定する.
	void MainPage::page_bbox_update(void) noexcept
	{
		slist_bound_page(
			m_main_page.m_shape_list, m_main_page.m_page_size, m_main_bbox_lt,
			m_main_bbox_rb);
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
		status_bar_set_pos();
	}

	// 表示を拡大または縮小する.
	void MainPage::page_zoom_delta(const int32_t delta) noexcept
	{
		if (delta > 0 &&
			m_main_page.m_page_scale < 16.f / 1.1f - FLT_MIN) {
			m_main_page.m_page_scale *= 1.1f;
		}
		else if (delta < 0 &&
			m_main_page.m_page_scale > 0.25f * 1.1f + FLT_MIN) {
			m_main_page.m_page_scale /= 1.1f;
		}
		else {
			return;
		}
		page_zoom_is_checked(m_main_page.m_page_scale);
		page_panel_size();
		page_draw();
		status_bar_set_pos();
		status_bar_set_zoom();
	}

	// 方眼メニューの「用紙設定をリセット」が選択された.
	// 設定データを保存したファイルがある場合, それを削除する.
	IAsyncAction MainPage::page_setting_reset_click_async(
		IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::Storage::StorageDeleteOption;

		winrt::Windows::Storage::IStorageItem setting_item{
			co_await ApplicationData::Current().LocalFolder().TryGetItemAsync(PAGE_SETTING)
		};
		if (setting_item != nullptr) {
			auto delete_file = setting_item.try_as<StorageFile>();
			if (delete_file != nullptr) {
				HRESULT hres = E_FAIL;
				try {
					co_await delete_file.DeleteAsync(StorageDeleteOption::PermanentDelete);
					mfi_page_setting_reset().IsEnabled(false);
					hres = S_OK;
				}
				catch (winrt::hresult_error const& e) {
					hres = e.code();
				}
				if (hres != S_OK) {

				}
				delete_file = nullptr;
			}
			setting_item = nullptr;
		}
		page_setting_init();
		page_draw();
		status_bar_set_pos();
	}

	// 方眼メニューの「ページ設定を保存」が選択された
	// ローカルフォルダーにファイルを作成し, 設定データを保存する.
	IAsyncAction MainPage::page_setting_save_click_async(
		IInspectable const&, RoutedEventArgs const&)
	{
		auto setting_file{ 
			co_await ApplicationData::Current().LocalFolder().CreateFileAsync(
				PAGE_SETTING, CreationCollisionOption::ReplaceExisting)
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