//-------------------------------
// MainPage_text.cpp
// 文字列レイアウトの設定
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr double TEXT_LINE_DELTA = 1.5;	// 行間の変分 (dpi)

	// 書体メニューの「行間」>「高さ」が選択された.
	void MainPage::mfi_text_line_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		static winrt::event_token slider0_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_samp();
		const double val0 = m_page_panel.m_text_line;
		slider0().Value(val0);
		text_set_slider<U_OP::TEXT_LINE, 0>(val0);
		slider0().Visibility(VISIBLE);
		loaded_token = scp_samp_panel().Loaded(
			[this](auto, auto)
			{
				samp_panel_loaded();
				font_create_samp();
				draw_samp();
			}
		);
		slider0_token = slider0().ValueChanged(
			[this](auto, auto args)
			{
				text_set_slider<U_OP::TEXT_LINE, 0>(m_samp_shape, args.NewValue());
			}
		);
		primary_token = cd_samp().PrimaryButtonClick(
			[this](auto, auto)
			{
				double val;
				m_samp_shape->get_text_line(val);
				undo_push_value<U_OP::TEXT_LINE>(val);
			}
		);
		closed_token = cd_samp().Closed(
			[this](auto, auto)
			{
				delete m_samp_shape;
#if defined(_DEBUG)
				debug_leak_cnt--;
#endif
				m_samp_shape = nullptr;
				slider0().Visibility(COLLAPSED);
				scp_samp_panel().Loaded(loaded_token);
				slider0().ValueChanged(slider0_token);
				cd_samp().PrimaryButtonClick(primary_token);
				cd_samp().Closed(closed_token);
				UnloadObject(cd_samp());
				draw_page();
			}
		);
		auto const& r_loader = ResourceLoader::GetForCurrentView();
		tk_samp_caption().Text(r_loader.GetString(L"str_font"));
		show_cd_samp();
	}

	// 書体メニューの「行間」>「狭める」が選択された.
	void MainPage::mfi_text_line_contract_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		auto h = m_page_panel.m_text_line <= FLT_MIN ? m_page_panel.m_font_size * 1.25 : m_page_panel.m_text_line;
		auto val = h - TEXT_LINE_DELTA;
		if (val <= FLT_MIN) {
			val = 0.0f;
		}
		undo_push_value<U_OP::TEXT_LINE>(val);
	}

	// 書体メニューの「行間」>「広げる」が選択された.
	void MainPage::mfi_text_line_expand_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		auto h = m_page_panel.m_text_line <= FLT_MIN ? m_page_panel.m_font_size * 1.25 : m_page_panel.m_text_line;
		auto val = h + TEXT_LINE_DELTA;
		undo_push_value<U_OP::TEXT_LINE>(val);
	}

	// 書体メニューの「余白」が選択された.
	void MainPage::mfi_text_margin_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		static winrt::event_token slider0_token;
		static winrt::event_token slider1_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_samp();
		const double val0 = m_page_panel.m_text_mar.width;
		const double val1 = m_page_panel.m_text_mar.height;
		slider0().Value(val0);
		slider1().Value(val1);
		text_set_slider<U_OP::TEXT_MARGIN, 0>(val0);
		text_set_slider<U_OP::TEXT_MARGIN, 1>(val1);
		slider0().Visibility(VISIBLE);
		slider1().Visibility(VISIBLE);
		loaded_token = scp_samp_panel().Loaded(
			[this](auto, auto)
			{
				samp_panel_loaded();
				font_create_samp();
				draw_samp();
			}
		);
		slider0_token = slider0().ValueChanged(
			[this](auto, auto args)
			{
				text_set_slider<U_OP::TEXT_MARGIN, 0>(m_samp_shape, args.NewValue());
			}
		);
		slider1_token = slider1().ValueChanged(
			[this](auto, auto args)
			{
				text_set_slider<U_OP::TEXT_MARGIN, 1>(m_samp_shape, args.NewValue());
			}
		);
		primary_token = cd_samp().PrimaryButtonClick(
			[this](auto, auto)
			{
				D2D1_SIZE_F val;
				m_samp_shape->get_text_margin(val);
				undo_push_value<U_OP::TEXT_MARGIN>(val);
			}
		);
		closed_token = cd_samp().Closed(
			[this](auto, auto)
			{
				delete m_samp_shape;
#if defined(_DEBUG)
				debug_leak_cnt--;
#endif
				m_samp_shape = nullptr;
				slider0().Visibility(COLLAPSED);
				slider1().Visibility(COLLAPSED);
				scp_samp_panel().Loaded(loaded_token);
				slider0().ValueChanged(slider0_token);
				slider1().ValueChanged(slider1_token);
				cd_samp().PrimaryButtonClick(primary_token);
				cd_samp().Closed(closed_token);
				UnloadObject(cd_samp());
				draw_page();
			}
		);
		auto const& r_loader = ResourceLoader::GetForCurrentView();
		tk_samp_caption().Text(r_loader.GetString(L"str_font"));
		show_cd_samp();
	}

	// 書体メニューの「段落のそろえ」>「下よせ」が選択された.
	void MainPage::rmfi_text_align_bottom_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<U_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
	}

	// 書体メニューの「文字列のそろえ」>「中央」が選択された.
	void MainPage::rmfi_text_align_center_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<U_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	// 書体メニューの「文字列のそろえ」>「均等」が選択された.
	void MainPage::rmfi_text_align_justified_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<U_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
	}

	// 書体メニューの「文字列のそろえ」>「左よせ」が選択された.
	void MainPage::rmfi_text_align_left_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<U_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT_LEADING);
	}

	// 書体メニューの「段落のそろえ」>「中段」が選択された.
	void MainPage::rmfi_text_align_middle_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<U_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	// 書体メニューの「文字列のそろえ」>「右よせ」が選択された.
	void MainPage::rmfi_text_align_right_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<U_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT_TRAILING);
	}

	// 書体メニューの「段落のそろえ」>「上よせ」が選択された.
	void MainPage::rmfi_text_align_top_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<U_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	}

	// 書体メニューの「段落のそろえ」に印をつける.
	// p_align	段落のそろえ
	void MainPage::text_align_p_check_menu(const DWRITE_PARAGRAPH_ALIGNMENT p_align)
	{
		rmfi_text_align_top().IsChecked(p_align == DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		rmfi_text_align_bottom().IsChecked(p_align == DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		rmfi_text_align_middle().IsChecked(p_align == DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

		rmfi_text_align_top_2().IsChecked(p_align == DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		rmfi_text_align_bottom_2().IsChecked(p_align == DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		rmfi_text_align_middle_2().IsChecked(p_align == DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	// 書体メニューの「文字列のそろえ」に印をつける.
	// t_align	文字列のそろえ
	void MainPage::text_align_t_check_menu(const DWRITE_TEXT_ALIGNMENT t_align)
	{
		rmfi_text_align_left().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_LEADING);
		rmfi_text_align_right().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_TRAILING);
		rmfi_text_align_center().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_CENTER);
		rmfi_text_align_justified().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_JUSTIFIED);

		rmfi_text_align_left_2().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_LEADING);
		rmfi_text_align_right_2().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_TRAILING);
		rmfi_text_align_center_2().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_CENTER);
		rmfi_text_align_justified_2().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
	}

	// 値をスライダーのヘッダーに格納する.
	template <U_OP U, int S>
	void MainPage::text_set_slider(const double val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == U_OP::TEXT_MARGIN) {
			const double dpi = m_page_dx.m_logical_dpi;
			//const double dpi = m_page_panel.m_dx.m_logical_dpi;
			double px;
			if constexpr (S == 0) {
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_text_mar_horzorz");
				px = val;
			}
			if constexpr (S == 1) {
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_text_mar_vertert");
				px = val;
			}
			wchar_t const* fmt = nullptr;
			switch (m_samp_panel.m_page_unit) {
			default:
			case UNIT::PIXEL:
				fmt = FMT_PX_UNIT;
				break;
			case UNIT::INCH:
				fmt = FMT_IN_UNIT;
				px /= dpi;
				break;
			case UNIT::MILLI:
				fmt = FMT_MM_UNIT;
				px /= dpi;
				px *= MM_PER_INCH;
				break;
			case UNIT::POINT:
				fmt = FMT_PT_UNIT;
				px /= dpi;
				px *= PT_PER_INCH;
				break;
			case UNIT::GRID:
				fmt = FMT_GD_UNIT;
				px /= m_samp_panel.m_grid_len;
				break;
			}
			wchar_t buf[16];
			swprintf_s(buf, fmt, px);
			hdr = hdr + L": " + buf;
		}
		if constexpr (U == U_OP::TEXT_LINE) {
			const double pt = val / m_page_dx.m_logical_dpi * PT_PER_INCH;
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			hdr = r_loader.GetString(L"str_height");
			if (pt > FLT_MIN) {
				wchar_t buf[16];
				swprintf_s(buf, FMT_PT_UNIT, pt);
				hdr = hdr + L": " + buf;
			}
			else {
				hdr = hdr + L": " + r_loader.GetString(L"str_def_val");
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
	template <U_OP U, int S>
	void MainPage::text_set_slider(Shape* s, const double val)
	{
		text_set_slider<U, S>(val);
		if constexpr (U == U_OP::TEXT_LINE) {
			s->set_text_line(val);
		}
		if constexpr (U == U_OP::TEXT_MARGIN) {
			D2D1_SIZE_F mar;
			s->get_text_margin(mar);
			if constexpr (S == 0) {
				mar.width = static_cast<FLOAT>(val);
			}
			if constexpr (S == 1) {
				mar.height = static_cast<FLOAT>(val);
			}
			s->set_text_margin(mar);
		}
		if (scp_samp_panel().IsLoaded()) {
			draw_samp();
		}
	}

}