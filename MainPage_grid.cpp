//-------------------------------
// MainPage_grid.cpp
// 方眼の設定
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::ToggleMenuFlyoutItem;

	constexpr wchar_t TITLE_GRID[] = L"str_grid";
	constexpr double SLIDER_STEP = 0.5;

	// 値をスライダーのヘッダーに格納する.
	// U	操作
	// S	スライダー
	// value	値
	template <UNDO_OP U, int S>
	void MainPage::grid_set_slider_header(double value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == UNDO_OP::GRID_BASE) {
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			hdr = r_loader.GetString(L"str_grid_length");
			const double dpi = m_page_dx.m_logical_dpi;
			const double g_len = m_page_layout.m_grid_base + 1.0;
			wchar_t buf[32];
			// ピクセル単位の長さを他の単位の文字列に変換する.
			conv_val_to_len<WITH_UNIT_NAME>(len_unit(), value * SLIDER_STEP + 1.0, dpi, g_len, buf);
			hdr = hdr + L": " + buf;
		}
		if constexpr (U == UNDO_OP::GRID_GRAY) {
			if constexpr (S == 3) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_val_to_col(color_code(), value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_gray_scale") + L": " + buf;
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
	void MainPage::grid_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* s = &m_sample_layout;
		const double value = args.NewValue();

		grid_set_slider_header<U, S>(value);
		if constexpr (U == UNDO_OP::GRID_BASE) {
			s->set_grid_base(value * SLIDER_STEP);
		}
		if constexpr (U == UNDO_OP::GRID_GRAY) {
			s->set_grid_gray(value / COLOR_MAX);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// ページメニューの「方眼線の表示」に印をつける.
	// g_show	方眼線の表示
	void MainPage::grid_show_check_menu(const GRID_SHOW g_show)
	{
		rmfi_grid_show_back().IsChecked(g_show == GRID_SHOW::BACK);
		rmfi_grid_show_front().IsChecked(g_show == GRID_SHOW::FRONT);
		rmfi_grid_show_hide().IsChecked(g_show == GRID_SHOW::HIDE);

		rmfi_grid_show_back_2().IsChecked(g_show == GRID_SHOW::BACK);
		rmfi_grid_show_front_2().IsChecked(g_show == GRID_SHOW::FRONT);
		rmfi_grid_show_hide_2().IsChecked(g_show == GRID_SHOW::HIDE);
	}

	// ページメニューの「方眼線の形式」に印をつける.
	// g_show	方眼線の表示
	void MainPage::grid_patt_check_menu(const GRID_PATT g_patt)
	{
		rmfi_grid_patt_1().IsChecked(g_patt == GRID_PATT::PATT_1);
		rmfi_grid_patt_2().IsChecked(g_patt == GRID_PATT::PATT_2);
		rmfi_grid_patt_3().IsChecked(g_patt == GRID_PATT::PATT_3);

		rmfi_grid_patt_1_2().IsChecked(g_patt == GRID_PATT::PATT_1);
		rmfi_grid_patt_2_2().IsChecked(g_patt == GRID_PATT::PATT_2);
		rmfi_grid_patt_3_2().IsChecked(g_patt == GRID_PATT::PATT_3);
	}

	// ページメニューの「方眼の大きさ」>「大きさ」が選択された.
	IAsyncAction MainPage::mfi_grid_len_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_layout.set_to(&m_page_layout);
		const double val0 = m_sample_layout.m_grid_base / SLIDER_STEP;
		sample_slider_0().Value(val0);
		grid_set_slider_header<UNDO_OP::GRID_BASE, 0>(val0);
		sample_slider_0().Visibility(VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::grid_set_slider<UNDO_OP::GRID_BASE, 0> });
		m_sample_type = SAMP_TYPE::NONE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_GRID)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			double sample_value;
			double page_value;

			m_page_layout.get_grid_base(page_value);
			m_sample_layout.get_grid_base(sample_value);
			if (equal(page_value, sample_value) != true) {
				undo_push_set<UNDO_OP::GRID_BASE>(&m_page_layout, sample_value);
				// 一連の操作の区切としてヌル操作をスタックに積む.
				//undo_push_null();
				// 元に戻す/やり直しメニュー項目の使用の可否を設定する.
				enable_undo_menu();
			}

		}
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		page_draw();
	}

	// ページメニューの「方眼の大きさ」>「狭める」が選択された.
	void MainPage::mfi_grid_len_con_click(IInspectable const&, RoutedEventArgs const&)
	{
		const double value = (m_page_layout.m_grid_base + 1.0) * 0.5 - 1.0;
		if (value < 1.0) {
			return;
		}
		undo_push_set<UNDO_OP::GRID_BASE>(&m_page_layout, value);
		enable_undo_menu();
		page_draw();
	}

	// ページメニューの「方眼の大きさ」>「広げる」が選択された.
	void MainPage::mfi_grid_len_exp_click(IInspectable const&, RoutedEventArgs const&)
	{
		const double value = (m_page_layout.m_grid_base + 1.0) * 2.0 - 1.0;
		if (value > max(m_page_layout.m_page_size.width, m_page_layout.m_page_size.height)) {
			// 方眼の一片の長さが, ページの幅か高さの大きいほうの値を超える場合,
			// 中断する.
			return;
		}
		undo_push_set<UNDO_OP::GRID_BASE>(&m_page_layout, value);
		enable_undo_menu();
		page_draw();
	}

	// ページメニューの「方眼線の濃さ」が選択された.
	IAsyncAction MainPage::mfi_grid_gray_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_layout.set_to(&m_page_layout);
		const double val3 = m_sample_layout.m_grid_gray * COLOR_MAX;
		sample_slider_3().Value(val3);
		grid_set_slider_header<UNDO_OP::GRID_GRAY, 3>(val3);
		sample_slider_3().Visibility(VISIBLE);
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::grid_set_slider< UNDO_OP::GRID_GRAY, 3> });
		m_sample_type = SAMP_TYPE::NONE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_GRID)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			double sample_value;
			m_sample_layout.get_grid_gray(sample_value);
			double page_value;
			m_page_layout.get_grid_gray(page_value);
			if (equal(page_value, sample_value) != true) {
				undo_push_set<UNDO_OP::GRID_GRAY>(&m_page_layout, sample_value);
				// 一連の操作の区切としてヌル操作をスタックに積む.
				//undo_push_null();
				// 元に戻す/やり直しメニュー項目の使用の可否を設定する.
				enable_undo_menu();
			}
		}
		sample_slider_3().Visibility(COLLAPSED);
		sample_slider_3().ValueChanged(slider_3_token);
		page_draw();
	}

	// ページメニューの「方眼線の形式」>「強調なし」が選択された.
	void MainPage::rmfi_grid_patt_1_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_layout.m_grid_patt == GRID_PATT::PATT_1) {
			return;
		}
		undo_push_set<UNDO_OP::GRID_PATT>(&m_page_layout, GRID_PATT::PATT_1);
		enable_undo_menu();
		page_draw();
	}

	// ページメニューの「方眼線の形式」>「2番目を強調」が選択された.
	void MainPage::rmfi_grid_patt_2_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_layout.m_grid_patt == GRID_PATT::PATT_2) {
			return;
		}
		undo_push_set<UNDO_OP::GRID_PATT>(&m_page_layout, GRID_PATT::PATT_2);
		enable_undo_menu();
		page_draw();
	}

	// ページメニューの「方眼線の形式」>「2番目と5番目を強調」が選択された.
	void MainPage::rmfi_grid_patt_3_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_layout.m_grid_patt == GRID_PATT::PATT_3) {
			return;
		}
		undo_push_set<UNDO_OP::GRID_PATT>(&m_page_layout, GRID_PATT::PATT_3);
		enable_undo_menu();
		page_draw();
	}

	// ページメニューの「方眼線の表示」>「最背面」が選択された.
	void MainPage::rmfi_grid_show_back_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_layout.m_grid_show == GRID_SHOW::BACK) {
			return;
		}
		undo_push_set<UNDO_OP::GRID_SHOW>(&m_page_layout, GRID_SHOW::BACK);
		enable_undo_menu();
		page_draw();
	}

	// ページメニューの「方眼線の表示」>「最前面」が選択された.
	void MainPage::rmfi_grid_show_front_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_layout.m_grid_show == GRID_SHOW::FRONT) {
			return;
		}
		undo_push_set<UNDO_OP::GRID_SHOW>(&m_page_layout, GRID_SHOW::FRONT);
		// 一連の操作の区切としてヌル操作をスタックに積む.
		//undo_push_null();
		// 元に戻す/やり直しメニュー項目の使用の可否を設定する.
		enable_undo_menu();
		page_draw();
	}

	// ページメニューの「方眼線の表示」>「隠す」が選択された.
	void MainPage::rmfi_grid_show_hide_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_layout.m_grid_show == GRID_SHOW::HIDE) {
			return;
		}
		undo_push_set<UNDO_OP::GRID_SHOW>(&m_page_layout, GRID_SHOW::HIDE);
		// 一連の操作の区切としてヌル操作をスタックに積む.
		//undo_push_null();
		// 元に戻す/やり直しメニュー項目の使用の可否を設定する.
		enable_undo_menu();
		page_draw();
	}

	// ページメニューの「方眼にそろえる」が選択された.
	void MainPage::tmfi_grid_snap_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		auto g_snap = unbox_value<ToggleMenuFlyoutItem>(sender).IsChecked();
		if (m_page_layout.m_grid_snap != g_snap) {
			m_page_layout.m_grid_snap = g_snap;
		}
		if (m_page_layout.m_grid_snap != true) {
			return;
		}

		// 図形リストの各図形について以下を繰り返す.
		const double g_len = m_page_layout.m_grid_base + 1.0;
		auto flag = false;
		D2D1_POINT_2F p_min = page_min();
		D2D1_POINT_2F p_max = page_max();
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				// 消去フラグが立っている場合,
				// 以下を無視する.
				continue;
			}
			if (s->is_selected() != true) {
				// 選択フラグがない場合,
				continue;
			}
			D2D1_POINT_2F s_pos;
			s->get_min_pos(s_pos);
			D2D1_POINT_2F g_pos;
			pt_round(s_pos, g_len, g_pos);
			if (equal(g_pos, s_pos)) {
				// 開始位置と丸めた位置が同じ場合,
				continue;
			}
			if (flag != true) {
				flag = true;
			}
			undo_push_set<UNDO_OP::START_POS>(s);
			D2D1_POINT_2F diff;
			pt_sub(g_pos, s_pos, diff);
			s->move(diff);
		}
		if (flag != true) {
			return;
		}
		undo_push_null();
		enable_undo_menu();
		page_bound();
		page_panle_size();
		page_draw();
	}

}