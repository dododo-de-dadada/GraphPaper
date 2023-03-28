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
	using winrt::Windows::UI::Text::FontStretch;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::UI::Xaml::Controls::TextBox;

	constexpr wchar_t DLG_TITLE[] = L"str_page_layouts";	// 表示の表題
	// 図形が含まれるよう表示の左上位置と右下位置を更新する.
	// s	図形
	void MainPage::page_bbox_update(const Shape* s) noexcept
	{
		s->get_bound(m_main_bbox_lt, m_main_bbox_rb, m_main_bbox_lt, m_main_bbox_rb);
	}

	// 表示する.
	void MainPage::page_draw(void)
	{
		if (!scp_main_panel().IsLoaded()) {
			return;
		}
		if (!m_mutex_draw.try_lock()) {
			// ロックできない場合
			return;
		}

		const auto p_scale = max(m_main_page.m_page_scale, 0.0f);
		m_main_page.begin_draw(
			m_main_d2d.m_d2d_context.get(), true, m_wic_background.get(), p_scale);
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
		//pt_add(m_main_bbox_lt, sb_horz().Value() - m_main_page.m_page_margin.left, sb_vert().Value() - m_main_page.m_page_margin.top, lt);
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

	//------------------------------
	// 表示の大きさを設定する.
	//------------------------------
	void MainPage::main_panel_size(void)
	{
		const float w = static_cast<float>(scp_main_panel().ActualWidth());
		const float h = static_cast<float>(scp_main_panel().ActualHeight());
		if (w > 0.0f && h > 0.0f) {
			scroll_set(w, h);
			m_main_d2d.SetLogicalSize2(D2D1_SIZE_F{ w, h });
		}
	}
	/*
	static void page_just_size(const SHAPE_LIST& s_list)
	{
		D2D1_POINT_2F b_lt = { FLT_MAX, FLT_MAX };
		D2D1_POINT_2F b_rb = { -FLT_MAX, -FLT_MAX };
		D2D1_POINT_2F b_size;

		slist_bound_shape(s_list, b_lt, b_rb);
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
		return false;
	}
	*/
	//------------------------------
	// レイアウトメニューの「ページの大きさ」が選択された
	//------------------------------
	IAsyncAction MainPage::page_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_prop_page.set_attr_to(&m_main_page);
		const double g_len = m_main_page.m_grid_base + 1.0;
		const double dpi = m_main_d2d.m_logical_dpi;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_size.width, dpi, g_len, buf);
		tx_page_size_width().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_size.height, dpi, g_len, buf);
		tx_page_size_height().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_margin.left, dpi, g_len, buf);
		tx_page_margin_left().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_margin.top, dpi, g_len, buf);
		tx_page_margin_top().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_margin.right, dpi, g_len, buf);
		tx_page_margin_right().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_margin.bottom, dpi, g_len, buf);
		tx_page_margin_bottom().Text(buf);

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
			constexpr wchar_t INVALID_NUM[] = L"str_err_number";

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

			float new_left;
			if (swscanf_s(tx_page_margin_left().Text().c_str(), L"%f", &new_left) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_left/Header");
				co_return;
			}
			float new_top;
			if (swscanf_s(tx_page_margin_top().Text().c_str(), L"%f", &new_top) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_top/Header");
				co_return;
			}
			float new_right;
			if (swscanf_s(tx_page_margin_right().Text().c_str(), L"%f", &new_right) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_right/Header");
				co_return;
			}
			float new_bottom;
			if (swscanf_s(tx_page_margin_bottom().Text().c_str(), L"%f", &new_bottom) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_bottom/Header");
				co_return;
			}
			// 表示の縦横の長さの値をピクセル単位の値に変換する.
			D2D1_RECT_F p_mar{
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_left, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_top, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_right, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_bottom, dpi, g_len))
			};

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
			D2D1_SIZE_F p_size{
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_width, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_height, dpi, g_len))
			};

			const bool flag_size = !equal(p_size, m_main_page.m_page_size);
			const bool flag_mar = !equal(p_mar, m_main_page.m_page_margin);
			if (flag_size || flag_mar) {
				if (flag_size) {
					ustack_push_set<UNDO_T::PAGE_SIZE>(&m_main_page, p_size);
				}
				if (flag_mar) {
					ustack_push_set<UNDO_T::PAGE_PAD>(&m_main_page, p_mar);
				}
				ustack_push_null();
				ustack_is_enable();
				page_bbox_update();
				main_panel_size();
				page_draw();
				status_bar_set_pos();
				status_bar_set_grid();
				status_bar_set_page();
				status_bar_set_unit();
			}
		}
		else if (d_result == ContentDialogResult::Secondary) {
			constexpr wchar_t INVALID_NUM[] = L"str_err_number";

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

			float new_left;
			if (swscanf_s(tx_page_margin_left().Text().c_str(), L"%f", &new_left) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_left/Header");
				co_return;
			}
			float new_top;
			if (swscanf_s(tx_page_margin_top().Text().c_str(), L"%f", &new_top) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_top/Header");
				co_return;
			}
			float new_right;
			if (swscanf_s(tx_page_margin_right().Text().c_str(), L"%f", &new_right) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_right/Header");
				co_return;
			}
			float new_bottom;
			if (swscanf_s(tx_page_margin_bottom().Text().c_str(), L"%f", &new_bottom) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_bottom/Header");
				co_return;
			}
			// 長さの値をピクセル単位の値に変換する.
			D2D1_RECT_F p_mar{	// ページの余白
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_left, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_top, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_right, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_bottom, dpi, g_len))
			};

			// 図形全体をちょうど含む矩形の左上点と右下点を得る.
			D2D1_POINT_2F b_lt = { FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_rb = { -FLT_MAX, -FLT_MAX };
			//D2D1_POINT_2F b_size;
			slist_bound_shape(m_main_page.m_shape_list, b_lt, b_rb);

			// 矩形の大きさがゼロ,なら中断する.
			if (b_rb.x - b_lt.x < 1.0F || b_rb.y - b_lt.y < 1.0F) {
				co_return;
			}

			// 左上点の座標のいずれかが負ならば, 原点となるよう矩形を移動する.
			float dx = 0.0f;	// 矩形を移動した距離
			float dy = 0.0f;	// 矩形を移動した距離
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

			// 左上点の値を, 図形を取り囲むパディングとみなし,
			// 左上点の座標のいずれかが正ならば, その分だけ右下点を右下に移動する.
			if (b_lt.x > 0.0f) {
				b_rb.x += b_lt.x;
			}
			if (b_lt.y > 0.0f) {
				b_rb.y += b_lt.y;
			}

			// 右下点に余白を加えた値がページの大きさとなる.
			D2D1_SIZE_F p_size{	// ページの大きさ.
				new_left + b_rb.x + new_right,
				new_top + b_rb.y + new_bottom
			};

			// ページの大きさ, 余白, いずれかが異なる.
			// あるいは矩形が移動したなら
			const bool size_changed = !equal(p_size, m_main_page.m_page_size);
			const bool mar_chanfed = !equal(p_mar, m_main_page.m_page_margin);
			if (size_changed || mar_chanfed || dx > 0.0f || dy > 0.0f) {
				// 矩形が移動したなら, 図形が矩形に収まるよう, 図形も移動させる.
				if (dx > 0.0f || dy > 0.0f) {
					constexpr auto ANY = true;
					ustack_push_move({ dx, dy }, ANY);
				}
				// ページの大きさが異なるなら, 更新する.
				if (size_changed) {
					ustack_push_set<UNDO_T::PAGE_SIZE>(&m_main_page, p_size);
				}
				// ページの余白が異なるなら, 更新する.
				if (mar_chanfed) {
					ustack_push_set<UNDO_T::PAGE_PAD>(&m_main_page, p_mar);
				}
				ustack_push_null();
				ustack_is_enable();
				page_bbox_update();
				main_panel_size();
				page_draw();
				status_bar_set_grid();
				status_bar_set_page();
			}
			status_bar_set_unit();
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
		if (swscanf_s(tx_page_margin_left().Text().c_str(), L"%lf", &l) != 1) {
			return;
		}
		double t;
		if (swscanf_s(tx_page_margin_top().Text().c_str(), L"%lf", &t) != 1) {
			return;
		}
		double r;
		if (swscanf_s(tx_page_margin_right().Text().c_str(), L"%lf", &r) != 1) {
			return;
		}
		double b;
		if (swscanf_s(tx_page_margin_bottom().Text().c_str(), L"%lf", &b) != 1) {
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
		conv_col_to_str(m_color_notation, val, buf);
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
			m_prop_page.get_page_color(s_color);
			s_color.r = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_prop_page.set_page_color(s_color)) {
				prop_dialog_draw();
			}
		}
		else if constexpr (S == 1) {
			const auto val = static_cast<float>(args.NewValue());
			page_slider_set_header<S>(val);
			D2D1_COLOR_F s_color;
			m_prop_page.get_page_color(s_color);
			s_color.g = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_prop_page.set_page_color(s_color)) {
				prop_dialog_draw();
			}
		}
		else if constexpr (S == 2) {
			const auto val = static_cast<float>(args.NewValue());
			page_slider_set_header<S>(val);
			D2D1_COLOR_F s_color;
			m_prop_page.get_page_color(s_color);
			s_color.b = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_prop_page.set_page_color(s_color)) {
				prop_dialog_draw();
			}
		}
		else if constexpr (S == 3) {
			const auto val = static_cast<float>(args.NewValue());
			page_slider_set_header<S>(val);
			D2D1_COLOR_F s_color;
			m_prop_page.get_page_color(s_color);
			s_color.a = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_prop_page.set_page_color(s_color)) {
				prop_dialog_draw();
			}
		}
	}

	// 表示の左上位置と右下位置を設定する.
	void MainPage::page_bbox_update(void) noexcept
	{
		// 方眼が原点のため, ページの大きさから, 余白を引く.
		D2D1_SIZE_F rb{
			m_main_page.m_page_size.width - m_main_page.m_page_margin.left,
			m_main_page.m_page_size.height - m_main_page.m_page_margin.top
		};
		slist_bound_page(m_main_page.m_shape_list, rb, m_main_bbox_lt, m_main_bbox_rb);
		if (m_main_bbox_lt.x > -m_main_page.m_page_margin.left) {
			m_main_bbox_lt.x = -m_main_page.m_page_margin.left;
		}
		if (m_main_bbox_lt.y > -m_main_page.m_page_margin.top) {
			m_main_bbox_lt.y = -m_main_page.m_page_margin.top;
		}
	}

}