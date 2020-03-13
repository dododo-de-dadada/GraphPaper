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

	constexpr wchar_t TITLE_PAGE[] = L"str_page";

	// ページ寸法ダイアログの「適用」ボタンが押された.
	void MainPage::cd_page_size_pri_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const&)
	{
		constexpr wchar_t INVALID_NUM[] = L"str_err_number";
		const double dpi = m_page_dx.m_logical_dpi;

		// 本来, 無効な数値が入力されている場合, 「適用」ボタンは不可になっているので
		// 必要ないエラーチェックだが, 念のため.
		double pw;
		if (swscanf_s(tx_page_width().Text().c_str(), L"%lf", &pw) != 1) {
			// 「無効な数値です」メッセージダイアログを表示する.
			cd_message_show(ICON_ALERT, INVALID_NUM, L"tx_page_width/Header");
			return;
		}
		double ph;
		if (swscanf_s(tx_page_height().Text().c_str(), L"%lf", &ph) != 1) {
			// 「無効な数値です」メッセージダイアログを表示する.
			cd_message_show(ICON_ALERT, INVALID_NUM, L"tx_page_height/Header");
			return;
		}
		const auto g_len = m_sample_panel.m_grid_base + 1.0;
		// ページの縦横の長さの値をピクセル単位の値に変換する.
		D2D1_SIZE_F p_size{
			static_cast<FLOAT>(conv_len_to_val(m_page_unit, pw, dpi, g_len)),
			static_cast<FLOAT>(conv_len_to_val(m_page_unit, ph, dpi, g_len))
		};
		if (equal(p_size, m_page_panel.m_page_size) == false) {
			// 変換された値がページの大きさと異なる場合,
			// 値をページパネルに格納して, その操作をスタックに積む.
			undo_push_set<UNDO_OP::PAGE_SIZE>(&m_page_panel, p_size);
			// 一連の操作の区切としてヌル操作をスタックに積む.
			undo_push_null();
			// 元に戻す/やり直すメニュー項目の使用の可否を設定する.
			enable_undo_menu();
		}
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		// ポインターの位置をステータスバーに格納する.
		status_set_curs();
		// 方眼の大きさをステータスバーに格納する.
		status_set_grid();
		// ページの大きさをステータスバーに格納する.
		status_set_page();
		status_set_unit();
	}

	// ページの寸法入力ダイアログの「図形に合わせる」ボタンが押された.
	void MainPage::cd_page_size_sec_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const&)
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
			// 一連の操作の区切としてヌル操作をスタックに積む.
			undo_push_null();
			// 元に戻す/やり直すメニュー項目の使用の可否を設定する.
			enable_undo_menu();
		}
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		// ページの大きさをステータスバーに格納する.
		status_set_page();
	}

	// ページの「単位と書式」ダイアログの「適用」ボタンが押された.
	void MainPage::cd_page_unit_pri_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const&)
	{
		const auto p_unit = m_page_unit;
		m_page_unit = static_cast<LEN_UNIT>(cx_page_unit().SelectedIndex());
		m_col_style = static_cast<COL_STYLE>(cx_color_style().SelectedIndex());
		if (p_unit != m_page_unit) {
			// ポインターの位置をステータスバーに格納する.
			status_set_curs();
			// 方眼の大きさをステータスバーに格納する.
			status_set_grid();
			// ページの大きさをステータスバーに格納する.
			status_set_page();
			status_set_unit();
		}
	}

	// ページメニューの「色」が選択された.
	IAsyncAction MainPage::mfi_page_color_click(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		const double val0 = m_page_panel.m_page_color.r * COLOR_MAX;
		const double val1 = m_page_panel.m_page_color.g * COLOR_MAX;
		const double val2 = m_page_panel.m_page_color.b * COLOR_MAX;
		sample_slider_0().Value(val0);
		sample_slider_1().Value(val1);
		sample_slider_2().Value(val2);
		page_set_slider_header<UNDO_OP::PAGE_COLOR, 0>(val0);
		page_set_slider_header<UNDO_OP::PAGE_COLOR, 1>(val1);
		page_set_slider_header<UNDO_OP::PAGE_COLOR, 2>(val2);
		sample_slider_0().Visibility(VISIBLE);
		sample_slider_1().Visibility(VISIBLE);
		sample_slider_2().Visibility(VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::page_set_slider<UNDO_OP::PAGE_COLOR, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::page_set_slider<UNDO_OP::PAGE_COLOR, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::page_set_slider<UNDO_OP::PAGE_COLOR, 2> });
		m_sample_type = SAMP_TYPE::NONE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_PAGE)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F sample_value;
			m_sample_panel.get_page_color(sample_value);
			D2D1_COLOR_F page_value;
			m_page_panel.get_page_color(page_value);
			if (equal(page_value, sample_value) == false) {
				undo_push_set<UNDO_OP::PAGE_COLOR>(&m_page_panel, sample_value);
				// 一連の操作の区切としてヌル操作をスタックに積む.
				undo_push_null();
				// 元に戻す/やり直すメニュー項目の使用の可否を設定する.
				enable_undo_menu();
			}
		}
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_1().Visibility(COLLAPSED);
		sample_slider_2().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sample_slider_2().ValueChanged(slider_2_token);
		page_draw();
	}

	// ページメニューの「大きさ」が選択された
	void MainPage::mfi_page_size_click(IInspectable const&, RoutedEventArgs const&)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		//wchar_t const* format = nullptr;
		double pw;
		double ph;
		m_sample_panel.m_grid_base = m_page_panel.m_grid_base;
		const auto g_len = m_sample_panel.m_grid_base + 1.0;
		pw = m_sample_panel.m_page_size.width;
		ph = m_sample_panel.m_page_size.height;
		/*
		wchar_t const* format;
		switch (m_page_unit) {
		default:
		// return;
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

		wchar_t buf[32];
		swprintf_s(buf, format, pw);
		*/
		wchar_t buf[32];
		// ピクセル単位の長さを他の単位の文字列に変換する.
		conv_val_to_len<!WITH_UNIT_NAME>(m_page_unit, pw, dpi, g_len, buf);
		tx_page_width().Text(buf);
		// ピクセル単位の長さを他の単位の文字列に変換する.
		conv_val_to_len<!WITH_UNIT_NAME>(m_page_unit, ph, dpi, g_len, buf);
		tx_page_height().Text(buf);
		// ピクセル単位の長さを他の単位の文字列に変換する.
		conv_val_to_len<WITH_UNIT_NAME>(m_page_unit, m_page_size_max, dpi, g_len, buf);
		tx_page_size_max().Text(buf);
		// この時点では, テキストボックスに正しい数値を格納しても, 
		// TextChanged は呼ばれない.
		// プライマリーボタンは使用可能にしておく.
		cd_page_size().IsPrimaryButtonEnabled(true);
		cd_page_size().IsSecondaryButtonEnabled(m_list_shapes.size() > 0);
		const auto _ = cd_page_size().ShowAsync();
	}

	// ページメニューの「単位と書式」が選択された
	void MainPage::mfi_page_unit_click(IInspectable const&, RoutedEventArgs const&)
	{
		cx_page_unit().SelectedIndex(static_cast<uint32_t>(m_page_unit));
		cx_color_style().SelectedIndex(static_cast<uint32_t>(m_col_style));
		const auto _ = cd_page_unit().ShowAsync();
	}

	// ページと図形を表示する.
	void MainPage::page_draw(void)
	{
#if defined(_DEBUG)
		if (m_page_dx.m_swapChainPanel.IsLoaded() == false) {
			return;
		}
#endif
		std::lock_guard<std::mutex> lock(m_dx_mutex);

		auto const& dc = m_page_dx.m_d2dContext;
		// デバイスコンテキストの描画状態を保存ブロックに保持する.
		dc->SaveDrawingState(m_page_dx.m_state_block.get());
		// デバイスコンテキストから変換行列を得る.
		D2D1_MATRIX_3X2_F tran;
		dc->GetTransform(&tran);
		// 拡大率を変換行列の拡大縮小の成分に格納する.
		const auto scale = max(m_page_panel.m_page_scale, 0.0);
		tran.m11 = tran.m22 = static_cast<FLOAT>(scale);
		// スクロールの変分に拡大率を掛けた値を
		// 変換行列の平行移動の成分に格納する.
		D2D1_POINT_2F t_pos;
		pt_add(m_page_min, sb_horz().Value(), sb_vert().Value(), t_pos);
		pt_scale(t_pos, scale, t_pos);
		tran.dx = -t_pos.x;
		tran.dy = -t_pos.y;
		// 変換行列をデバイスコンテキストに格納する.
		dc->SetTransform(&tran);
		// 描画を開始する.
		dc->BeginDraw();
		// ページ色で塗りつぶす.
		dc->Clear(m_page_panel.m_page_color);
		if (m_page_panel.m_grid_show == GRID_SHOW::BACK) {
			// 方眼線の表示が最背面に表示の場合,
			// 方眼線を表示する.
			m_page_panel.draw_grid(m_page_dx, { 0.0f, 0.0f });
		}
		// 部位の色をブラシに格納する.
		m_page_dx.m_anch_brush->SetColor(m_page_panel.m_anch_color);
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				// 消去フラグが立っている場合,
				// 継続する.
				continue;
			}
			// 図形を表示する.
			s->draw(m_page_dx);
		}
		if (m_page_panel.m_grid_show == GRID_SHOW::FRONT) {
			// 方眼線の表示が最前面に表示の場合,
			// 方眼線を表示する.
			m_page_panel.draw_grid(m_page_dx, { 0.0f, 0.0f });
		}
		if (m_press_state == STATE_TRAN::PRESS_AREA) {
			// 押された状態が範囲を選択している場合,
			// 補助線の色をブラシに格納する.
			m_page_dx.m_aux_brush->SetColor(m_page_panel.m_aux_color);
			if (m_draw_tool == DRAW_TOOL::SELECT
				|| m_draw_tool == DRAW_TOOL::RECT
				|| m_draw_tool == DRAW_TOOL::TEXT
				|| m_draw_tool == DRAW_TOOL::SCALE) {
				// 選択ツール
				// または方形
				// または文字列の場合,
				// 方形の補助線を表示する.
				m_page_panel.draw_auxiliary_rect(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_tool == DRAW_TOOL::BEZI) {
				// 曲線の場合,
				// 曲線の補助線を表示する.
				m_page_panel.draw_auxiliary_bezi(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_tool == DRAW_TOOL::ELLI) {
				// だ円の場合,
				// だ円の補助線を表示する.
				m_page_panel.draw_auxiliary_elli(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_tool == DRAW_TOOL::LINE) {
				// 直線の場合,
				// 直線の補助線を表示する.
				m_page_panel.draw_auxiliary_line(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_tool == DRAW_TOOL::RRECT) {
				// 角丸方形の場合,
				// 角丸方形の補助線を表示する.
				m_page_panel.draw_auxiliary_rrect(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_tool == DRAW_TOOL::QUAD) {
				// 四へん形の場合,
				// 四へん形の補助線を表示する.
				m_page_panel.draw_auxiliary_quad(m_page_dx, m_press_pos, m_curr_pos);
			}
		}
		// 描画を終了する.
		HRESULT hr = dc->EndDraw();
		// 保存された描画環境を元に戻す.
		dc->RestoreDrawingState(m_page_dx.m_state_block.get());
		if (hr == S_OK) {
			// 結果が S_OK の場合,
			// スワップチェーンの内容を画面に表示する.
			m_page_dx.Present();
			// ポインターの位置をステータスバーに格納する.
			status_set_curs();
		}
#if defined(_DEBUG)
		else {
			// 結果が S_OK でない場合,
			// 「描画できません」メッセージダイアログを表示する.
			cd_message_show(ICON_ALERT, L"str_err_draw", {});
		}
#endif
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S>
	void MainPage::page_set_slider_header(const double value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring hdr;
		if constexpr (U == UNDO_OP::PAGE_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_val_to_col(m_col_style, value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_val_to_col(m_col_style, value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_val_to_col(m_col_style, value, buf);
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
	template <UNDO_OP U, int S>
	void MainPage::page_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* s = &m_sample_panel;
		const double value = args.NewValue();
		page_set_slider_header<U, S>(value);
		if constexpr (U == UNDO_OP::GRID_BASE) {
			s->set_grid_base(value);
		}
		if constexpr (U == UNDO_OP::GRID_OPAC) {
			s->set_grid_opac(value / COLOR_MAX);
		}
		if constexpr (U == UNDO_OP::PAGE_COLOR) {
			D2D1_COLOR_F color;
			s->get_page_color(color);
			if constexpr (S == 0) {
				color.r = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 1) {
				color.g = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 2) {
				color.b = static_cast<FLOAT>(value / COLOR_MAX);
			}
			s->set_page_color(color);
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
	void MainPage::tx_page_size_text_changed(IInspectable const& sender, TextChangedEventArgs const&)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		double value;
		wchar_t buf[2];
		int cnt;
		// テキストボックスの文字列を数値に変換する.
		cnt = swscanf_s(unbox_value<TextBox>(sender).Text().c_str(), L"%lf%1s", &value, buf, 2);
		if (cnt == 1 && value > 0.0) {
			// 値ををピクセル単位の値に変換する.
			value = conv_len_to_val(m_page_unit, value, dpi, m_sample_panel.m_grid_base + 1.0);
		}
		cd_page_size().IsPrimaryButtonEnabled(cnt == 1 && value >= 1.0 && value < m_page_size_max);
	}

}