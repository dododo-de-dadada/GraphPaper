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

	// 長さををピクセル単位の値に変換する.
	static double conv_len_to_val(const LEN_UNIT l_unit, const double value, const double dpi, const double g_len) noexcept;

	// 長さををピクセル単位の値に変換する.
	// 変換された値は, 0.5 ピクセル単位に丸められる.
	// l_unit	長さの単位
	// value	長さの値
	// dpi	DPI
	// g_len	方眼の長さ
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
		const auto g_len = m_sample_layout.m_grid_base + 1.0;
		// ページの縦横の長さの値をピクセル単位の値に変換する.
		D2D1_SIZE_F p_size{
			static_cast<FLOAT>(conv_len_to_val(m_len_unit, pw, dpi, g_len)),
			static_cast<FLOAT>(conv_len_to_val(m_len_unit, ph, dpi, g_len))
		};
		if (equal(p_size, m_page_layout.m_page_size) == false) {
			// 変換された値がページの大きさと異なる場合,
			// 値をページレイアウトに格納して, その操作をスタックに積む.
			undo_push_set<UNDO_OP::PAGE_SIZE>(&m_page_layout, p_size);
			// 一連の操作の区切としてヌル操作をスタックに積む.
			undo_push_null();
			// 元に戻す/やり直しメニュー項目の使用の可否を設定する.
			enable_undo_menu();
		}
		s_list_bound(m_list_shapes, m_page_layout.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		stbar_set_curs();
		stbar_set_grid();
		stbar_set_page();
		stbar_set_unit();
	}

	// ページの寸法入力ダイアログの「図形に合わせる」ボタンが押された.
	void MainPage::cd_page_size_sec_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const&)
	{
		D2D1_POINT_2F b_min = { FLT_MAX, FLT_MAX };
		D2D1_POINT_2F b_max = { -FLT_MAX, -FLT_MAX };
		D2D1_POINT_2F p_size;

		s_list_bound(m_list_shapes, b_min, b_max);
		pt_sub(b_max, b_min, p_size);
		if (p_size.x < 1.0F || p_size.y < 1.0F) {
			return;
		}
		pt_min({ 0.0F, 0.0F }, b_min, m_page_min);
		pt_max(b_max, p_size, m_page_max);
		if (equal(m_page_layout.m_page_size, { p_size.x, p_size.y }) == false) {
			undo_push_set<UNDO_OP::PAGE_SIZE>(&m_page_layout, D2D1_SIZE_F{ p_size.x, p_size.y });
			undo_push_null();
			enable_undo_menu();
		}
		s_list_bound(m_list_shapes, m_page_layout.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		stbar_set_page();
	}

	// ページの「ページの単位と色の書式」ダイアログの「適用」ボタンが押された.
	// ページメニューの「色」が選択された.
	IAsyncAction MainPage::mfi_page_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_layout.set_to(&m_page_layout);
		const double val0 = m_sample_layout.m_page_color.r * COLOR_MAX;
		const double val1 = m_sample_layout.m_page_color.g * COLOR_MAX;
		const double val2 = m_sample_layout.m_page_color.b * COLOR_MAX;
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
			m_sample_layout.get_page_color(sample_value);
			D2D1_COLOR_F page_value;
			m_page_layout.get_page_color(page_value);
			if (equal(page_value, sample_value) == false) {
				undo_push_set<UNDO_OP::PAGE_COLOR>(&m_page_layout, sample_value);
				// 一連の操作の区切としてヌル操作をスタックに積む.
				undo_push_null();
				// 元に戻す/やり直しメニュー項目の使用の可否を設定する.
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
		m_sample_layout.set_to(&m_page_layout);
		const auto g_len = m_sample_layout.m_grid_base + 1.0;
		pw = m_sample_layout.m_page_size.width;
		ph = m_sample_layout.m_page_size.height;
		wchar_t buf[32];
		conv_val_to_len<!WITH_UNIT_NAME>(m_len_unit, pw, dpi, g_len, buf);
		tx_page_width().Text(buf);
		conv_val_to_len<!WITH_UNIT_NAME>(m_len_unit, ph, dpi, g_len, buf);
		tx_page_height().Text(buf);
		conv_val_to_len<WITH_UNIT_NAME>(m_len_unit, m_page_size_max, dpi, g_len, buf);
		tx_page_size_max().Text(buf);
		// この時点では, テキストボックスに正しい数値を格納しても, 
		// TextChanged は呼ばれない.
		// プライマリーボタンは使用可能にしておく.
		cd_page_size().IsPrimaryButtonEnabled(true);
		cd_page_size().IsSecondaryButtonEnabled(m_list_shapes.size() > 0);
		const auto _ = cd_page_size().ShowAsync();
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
		const auto scale = max(m_page_layout.m_page_scale, 0.0);
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
		dc->Clear(m_page_layout.m_page_color);
		if (m_page_layout.m_grid_show == GRID_SHOW::BACK) {
			// 方眼線の表示が最背面に表示の場合,
			// 方眼線を表示する.
			m_page_layout.draw_grid(m_page_dx, { 0.0f, 0.0f });
		}
		// 部位の色をブラシに格納する.
		//D2D1_COLOR_F anch_color;
		//m_page_layout.get_anchor_color(anch_color);
		//m_page_dx.m_anch_brush->SetColor(anch_color);
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				// 消去フラグが立っている場合,
				// 継続する.
				continue;
			}
			// 図形を表示する.
			s->draw(m_page_dx);
		}
		if (m_page_layout.m_grid_show == GRID_SHOW::FRONT) {
			// 方眼線の表示が最前面に表示の場合,
			// 方眼線を表示する.
			m_page_layout.draw_grid(m_page_dx, { 0.0f, 0.0f });
		}
		if (m_pointer_state == STATE_TRAN::PRESS_AREA) {
			// 押された状態が範囲を選択している場合,
			// 補助線の色をブラシに格納する.
			//D2D1_COLOR_F aux_color;
			//m_page_layout.get_auxiliary_color(aux_color);
			//m_page_dx.m_aux_brush->SetColor(aux_color);
			if (m_draw_tool == DRAW_TOOL::SELECT
				|| m_draw_tool == DRAW_TOOL::RECT
				|| m_draw_tool == DRAW_TOOL::TEXT
				|| m_draw_tool == DRAW_TOOL::SCALE) {
				// 選択ツール
				// または方形
				// または文字列の場合,
				// 方形の補助線を表示する.
				m_page_layout.draw_auxiliary_rect(m_page_dx, m_pointer_pressed, m_pointer_cur);
			}
			else if (m_draw_tool == DRAW_TOOL::BEZI) {
				// 曲線の場合,
				// 曲線の補助線を表示する.
				m_page_layout.draw_auxiliary_bezi(m_page_dx, m_pointer_pressed, m_pointer_cur);
			}
			else if (m_draw_tool == DRAW_TOOL::ELLI) {
				// だ円の場合,
				// だ円の補助線を表示する.
				m_page_layout.draw_auxiliary_elli(m_page_dx, m_pointer_pressed, m_pointer_cur);
			}
			else if (m_draw_tool == DRAW_TOOL::LINE) {
				// 直線の場合,
				// 直線の補助線を表示する.
				m_page_layout.draw_auxiliary_line(m_page_dx, m_pointer_pressed, m_pointer_cur);
			}
			else if (m_draw_tool == DRAW_TOOL::RRCT) {
				// 角丸方形の場合,
				// 角丸方形の補助線を表示する.
				m_page_layout.draw_auxiliary_rrect(m_page_dx, m_pointer_pressed, m_pointer_cur);
			}
			else if (m_draw_tool == DRAW_TOOL::QUAD) {
				// 四へん形の場合,
				// 四へん形の補助線を表示する.
				m_page_layout.draw_auxiliary_quad(m_page_dx, m_pointer_pressed, m_pointer_cur);
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
			stbar_set_curs();
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
				conv_val_to_col(m_color_code, value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_val_to_col(m_color_code, value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_val_to_col(m_color_code, value, buf);
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
		Shape* s = &m_sample_layout;
		const auto value = args.NewValue();
		page_set_slider_header<U, S>(value);
		if constexpr (U == UNDO_OP::GRID_BASE) {
			s->set_grid_base(value);
		}
		if constexpr (U == UNDO_OP::GRID_GRAY) {
			s->set_grid_gray(value / COLOR_MAX);
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

	// ページのスワップチェーンパネルがロードされた.
#if defined(_DEBUG)
	void MainPage::scp_page_panel_loaded(IInspectable const& sender, RoutedEventArgs const&)
#else
	void MainPage::scp_page_panel_loaded(IInspectable const&, RoutedEventArgs const&)
#endif
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif // _DEBUG
		m_page_dx.SetSwapChainPanel(scp_page_panel());
		page_draw();
	}

	// ページのスワップチェーンパネルの寸法が変わった.
#if defined(_DEBUG)
	void MainPage::scp_page_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
#else
	void MainPage::scp_page_panel_size_changed(IInspectable const&, SizeChangedEventArgs const& args)
#endif	// _DEBUG
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
			// 文字列が数値に変換できた場合,
			value = conv_len_to_val(m_len_unit, value, dpi, m_sample_layout.m_grid_base + 1.0);
		}
		cd_page_size().IsPrimaryButtonEnabled(cnt == 1 && value >= 1.0 && value < m_page_size_max);
	}

}