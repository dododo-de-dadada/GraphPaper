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

	//	値をスライダーのヘッダーに格納する.
	//	U	操作
	//	S	スライダー
	//	val	値
	template <UNDO_OP U, int S>
	void MainPage::grid_set_slider(double val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == UNDO_OP::GRID_LEN) {
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			hdr = r_loader.GetString(L"str_grid_length");
			val += 1.0;
			const auto dpi = m_sample_dx.m_logical_dpi;
			const auto g_len = m_page_panel.m_grid_size + 1.0;
			wchar_t buf[16];
			conv_val_to_len(m_page_unit, val, dpi, g_len, buf, 16);
			hdr = hdr + L": " + buf;
		}
		if constexpr (U == UNDO_OP::GRID_OPAC) {
			if constexpr (S == 3) {
				wchar_t buf[16];
				conv_val_to_col(m_col_style, val, buf, 16);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_opacity") + L": " + buf;
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

	//	値をスライダーのヘッダーと図形に格納する.
	//	U	操作
	//	S	スライダー
	//	s	図形	
	//	val	値
	//	戻り値	なし
	template <UNDO_OP U, int S>
	void MainPage::grid_set_slider(Shape* s, const double val)
	{
		grid_set_slider<U, S>(val);
		if constexpr (U == UNDO_OP::GRID_LEN) {
			s->set_grid_size(val);
		}
		if constexpr (U == UNDO_OP::GRID_OPAC) {
			s->set_grid_opac(val / COLOR_MAX);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// ページメニューの「方眼の表示」に印をつける.
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

	// ページメニューの「方眼の大きさ」>「大きさ」が選択された.
	void MainPage::mfi_grid_len_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		static winrt::event_token slider0_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_sample();
		const double val0 = m_page_panel.m_grid_size;
		slider0().Value(val0);
		grid_set_slider<UNDO_OP::GRID_LEN, 0>(val0);
		slider0().Visibility(VISIBLE);
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
				grid_set_slider<UNDO_OP::GRID_LEN, 0>(&m_sample_panel, args.NewValue());
			}
		);
		primary_token = cd_sample().PrimaryButtonClick(
			[this](auto, auto)
			{
				double sample_val;
				double page_val;

				m_page_panel.get_grid_size(page_val);
				m_sample_panel.get_grid_size(sample_val);
				if (equal(page_val, sample_val)) {
					return;
				}
				undo_push_set<UNDO_OP::GRID_LEN>(&m_page_panel, sample_val);
				undo_push_null();
				enable_undo_menu();
				page_draw();
			}
		);
		closed_token = cd_sample().Closed(
			[this](auto, auto)
			{
				scp_sample_panel().Loaded(loaded_token);
				slider0().Visibility(COLLAPSED);
				slider0().ValueChanged(slider0_token);
				cd_sample().PrimaryButtonClick(primary_token);
				cd_sample().Closed(closed_token);
				UnloadObject(cd_sample());
				page_draw();
			}
		);
		show_cd_sample(TITLE_GRID);
	}

	// ページメニューの「方眼の大きさ」>「狭める」が選択された.
	void MainPage::mfi_grid_len_contract_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		const double val = (m_page_panel.m_grid_size + 1.0) * 0.5 - 1.0;
		if (val < 1.0) {
			return;
		}
		undo_push_set<UNDO_OP::GRID_LEN>(&m_page_panel, val);
		undo_push_null();
		enable_undo_menu();
		page_draw();
	}

	// ページメニューの「方眼の大きさ」>「広げる」が選択された.
	void MainPage::mfi_grid_len_expand_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		const double val = (m_page_panel.m_grid_size + 1.0) * 2.0 - 1.0;
		if (val > max(m_page_panel.m_page_size.width, m_page_panel.m_page_size.height)) {
			//	方眼の一片の長さが, ページの幅か高さの大きいほうの値を超える場合,
			//	中断する.
			return;
		}
		undo_push_set<UNDO_OP::GRID_LEN>(&m_page_panel, val);
		undo_push_null();
		enable_undo_menu();
		page_draw();
	}

	// ページメニューの「方眼線の濃さ」が選択された.
	void MainPage::mfi_grid_opac_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		static winrt::event_token slider3_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_sample();
		const double val3 = m_sample_panel.m_grid_opac * COLOR_MAX;
		slider3().Value(val3);
		grid_set_slider<UNDO_OP::GRID_OPAC, 3>(val3);
		slider3().Visibility(VISIBLE);
		loaded_token = scp_sample_panel().Loaded(
			[this](auto, auto)
			{
				sample_panel_loaded();
				sample_draw();
			}
		);
		slider3_token = slider3().ValueChanged(
			[this](auto, auto args)
			{
				grid_set_slider<UNDO_OP::GRID_OPAC, 3>(&m_sample_panel, args.NewValue());
			}
		);
		primary_token = cd_sample().PrimaryButtonClick(
			[this](auto, auto)
			{
				//m_page_panel.m_col_style = m_sample_panel.m_col_style;
				double sample_val;
				m_sample_panel.get_grid_opac(sample_val);
				double page_val;
				m_page_panel.get_grid_opac(page_val);
				if (equal(page_val, sample_val)) {
					return;
				}
				undo_push_set<UNDO_OP::GRID_OPAC>(&m_page_panel, sample_val);
				undo_push_null();
				enable_undo_menu();
				page_draw();
			}
		);
		closed_token = cd_sample().Closed(
			[this](auto, auto)
			{
				scp_sample_panel().Loaded(loaded_token);
				slider3().Visibility(COLLAPSED);
				//cx_color_style().Visibility(COLLAPSED);
				slider3().ValueChanged(slider3_token);
				//cx_color_style().SelectionChanged(c_style_token);
				cd_sample().PrimaryButtonClick(primary_token);
				cd_sample().Closed(closed_token);
				UnloadObject(cd_sample());
				page_draw();
			}
		);
		show_cd_sample(TITLE_GRID);
	}

	// ページメニューの「方眼線の表示」>「最背面」が選択された.
	void MainPage::rmfi_grid_show_back_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		if (m_page_panel.m_grid_show == GRID_SHOW::BACK) {
			return;
		}
		undo_push_set<UNDO_OP::GRID_SHOW>(&m_page_panel, GRID_SHOW::BACK);
		undo_push_null();
		enable_undo_menu();
		page_draw();
	}

	// ページメニューの「方眼線の表示」>「最前面」が選択された.
	void MainPage::rmfi_grid_show_front_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		if (m_page_panel.m_grid_show == GRID_SHOW::FRONT) {
			return;
		}
		undo_push_set<UNDO_OP::GRID_SHOW>(&m_page_panel, GRID_SHOW::FRONT);
		undo_push_null();
		enable_undo_menu();
		page_draw();
	}

	// ページメニューの「方眼線の表示」>「隠す」が選択された.
	void MainPage::rmfi_grid_show_hide_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		if (m_page_panel.m_grid_show == GRID_SHOW::HIDE) {
			return;
		}
		undo_push_set<UNDO_OP::GRID_SHOW>(&m_page_panel, GRID_SHOW::HIDE);
		undo_push_null();
		enable_undo_menu();
		page_draw();
	}

	// ページメニューの「方眼にそろえる」が選択された.
	void MainPage::tmfi_grid_snap_click(IInspectable const& sender, RoutedEventArgs const& /*args*/)
	{
		auto g_snap = unbox_value<ToggleMenuFlyoutItem>(sender).IsChecked();
		if (m_page_panel.m_grid_snap != g_snap) {
			m_page_panel.m_grid_snap = g_snap;
		}
		if (m_page_panel.m_grid_snap == false) {
			return;
		}
		const double g_len = m_page_panel.m_grid_size + 1.0;
		auto flag = true;	// 未変更
		D2D1_POINT_2F s_pos;
		D2D1_POINT_2F g_pos;
		D2D1_POINT_2F d;

		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() == false) {
				continue;
			}
			s->get_min_pos(s_pos);
			pt_round(s_pos, g_len, g_pos);
			if (equal(g_pos, s_pos)) {
				continue;
			}
			if (flag == true) {
				flag = false;
			}
			undo_push_set<UNDO_OP::START_POS>(s);
			pt_sub(g_pos, s_pos, d);
			s->move(d);
		}
		if (flag == true) {
			return;
		}
		undo_push_null();
		enable_undo_menu();
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
	}

}