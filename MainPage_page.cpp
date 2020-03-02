//-------------------------------
// MainPage_page.cpp
// ページの設定と表示
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::TextBox;

	// ページ寸法ダイアログの「適用」ボタンが押された.
	void MainPage::cd_page_size_pri_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const& /*args*/)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		double pw;
		double ph;
		D2D1_SIZE_F page;

		//	無効な数値が入力されている場合, 「適用」ボタンは不可になっているので
		//	本来は必要ないエラーチェックだが, 念のため.
		if (swscanf_s(tx_page_width().Text().c_str(), L"%lf", &pw) != 1) {
			cd_message_show(L"str_err_number", L"tx_page_width/Header");
			return;
		}
		if (swscanf_s(tx_page_height().Text().c_str(), L"%lf", &ph) != 1) {
			cd_message_show(L"str_err_number", L"tx_page_height/Header");
			return;
		}
		switch (m_page_unit) {
		default:
			return;
		case LEN_UNIT::PIXEL:
			page.width = static_cast<FLOAT>(std::round(pw));
			page.height = static_cast<FLOAT>(std::round(ph));
			break;
		case LEN_UNIT::INCH:
			page.width = static_cast<FLOAT>(std::round(pw * dpi));
			page.height = static_cast<FLOAT>(std::round(ph * dpi));
			break;
		case LEN_UNIT::MILLI:
			page.width = static_cast<FLOAT>(std::round(pw / MM_PER_INCH * dpi));
			page.height = static_cast<FLOAT>(std::round(ph / MM_PER_INCH * dpi));
			break;
		case LEN_UNIT::POINT:
			page.width = static_cast<FLOAT>(std::round(pw / PT_PER_INCH * dpi));
			page.height = static_cast<FLOAT>(std::round(ph / PT_PER_INCH * dpi));
			break;
		case LEN_UNIT::GRID:
			page.width = static_cast<FLOAT>(std::round(pw * (m_sample_panel.m_grid_len + 1.0)));
			page.height = static_cast<FLOAT>(std::round(ph * (m_sample_panel.m_grid_len + 1.0)));
			break;
		}
		if (equal(m_page_panel.m_page_size, page) == false) {
			undo_push_set<UNDO_OP::PAGE_SIZE>(&m_page_panel, page);
			undo_push_null();
			enable_undo_menu();
		}
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		stat_set_curs();
		stat_set_page();
		stat_set_unit();
		stat_set_grid();
	}

	// ページの寸法入力ダイアログの「図形に合わせる」ボタンが押された.
	void MainPage::cd_page_size_sec_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const& /*args*/)
	{
		D2D1_POINT_2F b_min = { FLT_MAX, FLT_MAX };
		D2D1_POINT_2F b_max = { -FLT_MAX, -FLT_MAX };
		D2D1_POINT_2F p_max;

		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_min, b_max);
		}
		pt_add(b_max, b_min, p_max);
		if (p_max.x < 1.0 || p_max.y < 1.0) {
			return;
		}
		pt_min({ 0.0f, 0.0f }, b_min, m_page_min);
		pt_max(b_max, p_max, m_page_max);
		const D2D1_SIZE_F page = { p_max.x, p_max.y };
		if (equal(m_page_panel.m_page_size, page) == false) {
			undo_push_set<UNDO_OP::PAGE_SIZE>(&m_page_panel, page);
			undo_push_null();
			enable_undo_menu();
		}
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		stat_set_page();
	}

	// ページの「単位と書式」ダイアログの「適用」ボタンが押された.
	void MainPage::cd_page_unit_pri_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const& /*args*/)
	{
		auto p_unit = m_page_unit;
		m_page_unit = static_cast<LEN_UNIT>(cx_page_unit().SelectedIndex());
		m_col_style = static_cast<COL_STYLE>(cx_color_style().SelectedIndex());
		if (p_unit != m_page_unit) {
			stat_set_curs();
			stat_set_grid();
			stat_set_page();
			stat_set_unit();
		}
	}

	// ページメニューの「色」が選択された.
	void MainPage::mfi_page_color_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		static winrt::event_token slider0_token;
		static winrt::event_token slider1_token;
		static winrt::event_token slider2_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_sample();
		const double val0 = m_page_panel.m_page_color.r * COLOR_MAX;
		const double val1 = m_page_panel.m_page_color.g * COLOR_MAX;
		const double val2 = m_page_panel.m_page_color.b * COLOR_MAX;
		slider0().Value(val0);
		slider1().Value(val1);
		slider2().Value(val2);
		page_set_slider<UNDO_OP::PAGE_COLOR, 0>(val0);
		page_set_slider<UNDO_OP::PAGE_COLOR, 1>(val1);
		page_set_slider<UNDO_OP::PAGE_COLOR, 2>(val2);
		slider0().Visibility(VISIBLE);
		slider1().Visibility(VISIBLE);
		slider2().Visibility(VISIBLE);
		loaded_token = scp_sample_panel().Loaded(
			[this](auto, auto)
			{
				sample_panel_loaded();
				sample_draw();
			}
		);
		slider0_token = slider0().ValueChanged(
			[this](auto, auto args)
			{
				page_set_slider<UNDO_OP::PAGE_COLOR, 0>(&m_sample_panel, args.NewValue());
			}
		);
		slider1_token = slider1().ValueChanged(
			[this](auto, auto args)
			{
				page_set_slider<UNDO_OP::PAGE_COLOR, 1>(&m_sample_panel, args.NewValue());
			}
		);
		slider2_token = slider2().ValueChanged(
			[this](auto, auto args)
			{
				page_set_slider<UNDO_OP::PAGE_COLOR, 2>(&m_sample_panel, args.NewValue());
			}
		);
		primary_token = cd_sample().PrimaryButtonClick(
			[this](auto, auto)
			{
				D2D1_COLOR_F sample_val;
				m_sample_panel.get_page_color(sample_val);
				D2D1_COLOR_F page_val;
				m_page_panel.get_page_color(page_val);
				if (equal(page_val, sample_val)) {
					return;
				}
				undo_push_set<UNDO_OP::PAGE_COLOR>(&m_page_panel, sample_val);
				undo_push_null();
				enable_undo_menu();
				page_draw();
			}
		);
		closed_token = cd_sample().Closed(
			[this](auto, auto)
			{
				slider0().Visibility(COLLAPSED);
				slider1().Visibility(COLLAPSED);
				slider2().Visibility(COLLAPSED);
				scp_sample_panel().Loaded(loaded_token);
				slider0().ValueChanged(slider0_token);
				slider1().ValueChanged(slider1_token);
				slider2().ValueChanged(slider2_token);
				cd_sample().PrimaryButtonClick(primary_token);
				cd_sample().Closed(closed_token);
				UnloadObject(cd_sample());
				page_draw();
			}
		);
		show_cd_sample(L"str_page");
	}

	// ページメニューの「大きさ」が選択された
	void MainPage::mfi_page_size_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		//wchar_t const* format = nullptr;
		double pw;
		double ph;
		m_sample_panel.m_grid_len = m_page_panel.m_grid_len;
		const auto g_len = m_sample_panel.m_grid_len + 1.0;
		pw = m_page_panel.m_page_size.width;
		ph = m_page_panel.m_page_size.height;
		wchar_t const* format;
		switch (m_page_unit) {
		default:
		//	return;
		//case LEN_UNIT::PIXEL:
			format = FMT_PIXEL;
			break;
		case LEN_UNIT::INCH:
			format = FMT_INCH;
			pw = pw / dpi;
			ph = ph / dpi;
			break;
		case LEN_UNIT::MILLI:
			format = FMT_MILLI;
			pw = pw / dpi * MM_PER_INCH;
			ph = ph / dpi * MM_PER_INCH;
			break;
		case LEN_UNIT::POINT:
			format = FMT_POINT;
			pw = pw / dpi * PT_PER_INCH;
			ph = ph / dpi * PT_PER_INCH;
			break;
		case LEN_UNIT::GRID:
			format = FMT_GRID;
			pw /= m_sample_panel.m_grid_len + 1.0;
			ph /= m_sample_panel.m_grid_len + 1.0;
			break;
		}
		wchar_t buf[16];
		swprintf_s(buf, format, pw);
		tx_page_width().Text(buf);
		swprintf_s(buf, format, ph);
		tx_page_height().Text(buf);
		tk_page_unit().Text(get_unit_name());
		// この時点では, テキストボックスに正しい数値を格納しても, 
		// TextChanged は呼ばれない.
		// プライマリーボタンは使用可能にしておく.
		cd_page_size().IsPrimaryButtonEnabled(true);
		cd_page_size().IsSecondaryButtonEnabled(m_list_shapes.size() > 0);
		const auto _ = cd_page_size().ShowAsync();
	}

	// ページメニューの「単位と書式」が選択された
	void MainPage::mfi_page_unit_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		cx_page_unit().SelectedIndex(m_page_unit);
		cx_color_style().SelectedIndex(m_col_style);
		const auto _ = cd_page_unit().ShowAsync();
	}

	//	ページと図形を表示する.
	void MainPage::page_draw(void)
	{
#if defined(_DEBUG)
		if (m_page_dx.m_swapChainPanel.IsLoaded() == false) {
			return;
		}
#endif
		std::lock_guard<std::mutex> lock(m_dx_mutex);

		auto const& dc = m_page_dx.m_d2dContext;
		//	デバイスコンテキストの描画状態を保存ブロックに保持する.
		dc->SaveDrawingState(m_page_dx.m_state_block.get());
		//	デバイスコンテキストから変換行列を得る.
		D2D1_MATRIX_3X2_F tran;
		dc->GetTransform(&tran);
		//	拡大率を変換行列の拡大縮小の成分に格納する.
		const auto scale = max(m_page_panel.m_page_scale, 0.0);
		tran.m11 = tran.m22 = static_cast<FLOAT>(scale);
		//	スクロールの変分に拡大率を掛けた値を
		//	変換行列の平行移動の成分に格納する.
		D2D1_POINT_2F d;
		pt_add(m_page_min, sb_horz().Value(), sb_vert().Value(), d);
		pt_scale(d, scale, d);
		tran.dx = -d.x;
		tran.dy = -d.y;
		//	変換行列をデバイスコンテキストに格納する.
		dc->SetTransform(&tran);
		//	描画を開始する.
		dc->BeginDraw();
		//	ページ色で塗りつぶす.
		dc->Clear(m_page_panel.m_page_color);
		if (m_page_panel.m_grid_show == GRID_SHOW::BACK) {
			//	方眼線の表示が最背面に表示の場合,
			//	方眼線を表示する.
			m_page_panel.draw_grid_line(m_page_dx, { 0.0f, 0.0f });
		}
		//	部位の色をブラシに格納する.
		m_page_dx.m_anch_brush->SetColor(m_page_panel.m_anch_color);
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				//	消去フラグが立っている場合,
				//	継続する.
				continue;
			}
			//	図形を表示する.
			s->draw(m_page_dx);
		}
		if (m_page_panel.m_grid_show == GRID_SHOW::FRONT) {
			//	方眼線の表示が最前面に表示の場合,
			//	方眼線を表示する.
			m_page_panel.draw_grid_line(m_page_dx, { 0.0f, 0.0f });
		}
		if (m_press_state == S_TRAN::PRESS_AREA) {
			//	押された状態が範囲を選択している場合,
			//	補助線の色をブラシに格納する.
			m_page_dx.m_aux_brush->SetColor(m_page_panel.m_aux_color);
			if (m_draw_shape == DRAW_SELECT
				|| m_draw_shape == DRAW_RECT
				|| m_draw_shape == DRAW_TEXT
				|| m_draw_shape == DRAW_SCALE) {
				//	選択ツール
				//	または方形
				//	または文字列の場合,
				//	方形の補助線を表示する.
				m_page_panel.draw_auxiliary_rect(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_shape == DRAW_BEZI) {
				//	曲線の場合,
				//	曲線の補助線を表示する.
				m_page_panel.draw_auxiliary_bezi(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_shape == DRAW_ELLI) {
				//	だ円の場合,
				//	だ円の補助線を表示する.
				m_page_panel.draw_auxiliary_elli(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_shape == DRAW_LINE) {
				//	直線の場合,
				//	直線の補助線を表示する.
				m_page_panel.draw_auxiliary_line(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_shape == DRAW_RRECT) {
				//	角丸方形の場合,
				//	角丸方形の補助線を表示する.
				m_page_panel.draw_auxiliary_rrect(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_shape == DRAW_QUAD) {
				//	四へん形の場合,
				//	四へん形の補助線を表示する.
				m_page_panel.draw_auxiliary_quad(m_page_dx, m_press_pos, m_curr_pos);
			}
		}
		//	描画を終了する.
		HRESULT hr = dc->EndDraw();
		//	保存された描画環境を元に戻す.
		dc->RestoreDrawingState(m_page_dx.m_state_block.get());
		if (hr == S_OK) {
			//	結果が S_OK の場合,
			//	スワップチェーンの内容を画面に表示する.
			m_page_dx.Present();
			//	ポインターの位置をスタックバーに格納する.
			stat_set_curs();
		}
#if defined(_DEBUG)
		else {
			//	結果が S_OK でない場合,
			//	メッセージダイアログを表示する.
			cd_message_show(L"Cannot draw", {});
		}
#endif
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S>
	void MainPage::page_set_slider(double val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring hdr;
		if constexpr (U == UNDO_OP::PAGE_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[16];
				conv_val_to_col(m_col_style, val, buf, 16);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[16];
				conv_val_to_col(m_col_style, val, buf, 16);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[16];
				conv_val_to_col(m_col_style, val, buf, 16);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_b") + L": " + buf;
			}
		}
		if constexpr (S == 0) {
			slider0().Header(box_value(hdr));
		}
		if constexpr (S == 1) {
			slider1().Header(box_value(hdr));
		}
		if constexpr (S == 2) {
			slider2().Header(box_value(hdr));
		}
		if constexpr (S == 3) {
			slider3().Header(box_value(hdr));
		}
	}

	// 値をスライダーのヘッダーと図形に格納する.
	template <UNDO_OP U, int S>
	void MainPage::page_set_slider(Shape* s, const double val)
	{
		page_set_slider<U, S>(val);
		if constexpr (U == UNDO_OP::GRID_LEN) {
			s->set_grid_len(val);
		}
		if constexpr (U == UNDO_OP::GRID_OPAC) {
			s->set_grid_opac(val / COLOR_MAX);
		}
		if constexpr (U == UNDO_OP::PAGE_COLOR) {
			D2D1_COLOR_F col;
			s->get_page_color(col);
			if constexpr (S == 0) {
				col.r = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 1) {
				col.g = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 2) {
				col.b = static_cast<FLOAT>(val / COLOR_MAX);
			}
			s->set_page_color(col);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// ページのパネルがロードされた.
	void MainPage::scp_page_panel_loaded(IInspectable const& sender, RoutedEventArgs const&/*args*/)
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif // _DEBUG
		m_page_dx.SetSwapChainPanel(scp_page_panel());
		page_draw();
	}

	// ページのパネルの寸法が変わった.
	void MainPage::scp_page_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif	// _DEBUG
		const auto z = args.NewSize();
		const auto w = z.Width;
		const auto h = z.Height;
		scroll_set(w, h);
		if (scp_page_panel().IsLoaded() == false) {
			return;
		}
		m_page_dx.SetLogicalSize2({ w, h });
		//m_page_panel.m_dx.SetLogicalSize2({ w, h });
		page_draw();
	}

	// ページの大きさを設定する.
	void MainPage::set_page_panle_size(void)
	{
		const auto w = scp_page_panel().ActualWidth();
		const auto h = scp_page_panel().ActualHeight();
		scroll_set(w, h);
		m_page_dx.SetLogicalSize2({ static_cast<float>(w), static_cast<float>(h) });
	}

	// テキストボックス「ページの幅」「ページの高さ」の値が変更された.
	void MainPage::tx_page_size_text_changed(IInspectable const& sender, TextChangedEventArgs const& /*args*/)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		double value;
		wchar_t ws[2];
		int cnt;
		cnt = swscanf_s(unbox_value<TextBox>(sender).Text().c_str(), L"%lf%1s", &value, ws, 2);
		if (cnt == 1 && value > 0.0) {
			switch (m_page_unit) {
			case LEN_UNIT::INCH:
				value = std::round(value * dpi);
				break;
			case LEN_UNIT::MILLI:
				value = std::round(value * dpi / MM_PER_INCH);
				break;
			case LEN_UNIT::POINT:
				value = std::round(value * dpi / PT_PER_INCH);
				break;
			case LEN_UNIT::GRID:
				value = std::round(value * (m_page_panel.m_grid_len + 1.0));
				break;
			default:
				value = std::round(value);
				break;
			}
		}
		cd_page_size().IsPrimaryButtonEnabled(cnt == 1 && value >= 1.0 && value < 32768.0);
	}

}