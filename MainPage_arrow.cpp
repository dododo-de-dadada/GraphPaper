//-------------------------------
// MainPage_arrow.cpp
// 矢じりの形式と寸法を設定
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 線枠メニューの「矢じりの種類」に印をつける.
	// a_style	矢じりの形式
	void MainPage::arrow_style_check_menu(const ARROW_STYLE a_style)
	{
		rmfi_arrow_none().IsChecked(a_style == ARROW_STYLE::NONE);
		rmfi_arrow_opened().IsChecked(a_style == ARROW_STYLE::OPENED);
		rmfi_arrow_filled().IsChecked(a_style == ARROW_STYLE::FILLED);
		mfi_arrow_size().IsEnabled(a_style != ARROW_STYLE::NONE);

		rmfi_arrow_none_2().IsChecked(a_style == ARROW_STYLE::NONE);
		rmfi_arrow_opened_2().IsChecked(a_style == ARROW_STYLE::OPENED);
		rmfi_arrow_filled_2().IsChecked(a_style == ARROW_STYLE::FILLED);
		mfi_arrow_size_2().IsEnabled(a_style != ARROW_STYLE::NONE);
	}

	// ストロークメニューの「矢じりの種類」>「閉じた矢」が選択された.
	void MainPage::rmfi_arrow_filled_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		if (m_page_panel.m_arrow_style == ARROW_STYLE::NONE) {
			mfi_arrow_size().IsEnabled(true);
		}
		undo_push_value<U_OP::ARROW_STYLE>(ARROW_STYLE::FILLED);
	}

	// ストロークメニューの「矢じりの種類」>「なし」が選択された.
	void MainPage::rmfi_arrow_none_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		if (m_page_panel.m_arrow_style != ARROW_STYLE::NONE) {
			mfi_arrow_size().IsEnabled(false);
		}
		undo_push_value<U_OP::ARROW_STYLE>(ARROW_STYLE::NONE);
	}

	// ストロークメニューの「矢じりの種類」>「開いた」が選択された.
	void MainPage::rmfi_arrow_opened_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		if (m_page_panel.m_arrow_style == ARROW_STYLE::NONE) {
			mfi_arrow_size().IsEnabled(true);
		}
		undo_push_value<U_OP::ARROW_STYLE>(ARROW_STYLE::OPENED);
	}

	// ストロークメニューの「矢じりの大きさ」が選択された.
	void MainPage::mfi_arrow_size_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		static winrt::event_token slider0_token;
		static winrt::event_token slider1_token;
		static winrt::event_token slider2_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_samp();
		double val0 = m_page_panel.m_arrow_size.m_width;
		double val1 = m_page_panel.m_arrow_size.m_length;
		double val2 = m_page_panel.m_arrow_size.m_offset;
		slider0().Value(val0);
		slider1().Value(val1);
		slider2().Value(val2);
		arrow_set_slider<U_OP::ARROW_SIZE, 0>(val0);
		arrow_set_slider<U_OP::ARROW_SIZE, 1>(val1);
		arrow_set_slider<U_OP::ARROW_SIZE, 2>(val2);
		slider0().Visibility(VISIBLE);
		slider1().Visibility(VISIBLE);
		slider2().Visibility(VISIBLE);
		loaded_token = scp_samp_panel().Loaded(
			[this](auto, auto)
			{
				samp_panel_loaded();
				stroke_create_samp();
				draw_samp();
			}
		);
		slider0_token = slider0().ValueChanged(
			[this](auto, auto args)
			{
				arrow_set_slider<U_OP::ARROW_SIZE, 0>(m_samp_shape, args.NewValue());
			}
		);
		slider1_token = slider1().ValueChanged(
			[this](auto, auto args)
			{
				arrow_set_slider<U_OP::ARROW_SIZE, 1>(m_samp_shape, args.NewValue());
			}
		);
		slider2_token = slider2().ValueChanged(
			[this](auto, auto args)
			{
				arrow_set_slider<U_OP::ARROW_SIZE, 2>(m_samp_shape, args.NewValue());
			}
		);
		primary_token = cd_samp().PrimaryButtonClick(
			[this](auto, auto)
			{
				ARROW_SIZE val;
				m_samp_shape->get_arrow_size(val);
				undo_push_value<U_OP::ARROW_SIZE>(val);
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
				slider2().Visibility(COLLAPSED);
				scp_samp_panel().Loaded(loaded_token);
				slider0().ValueChanged(slider0_token);
				slider1().ValueChanged(slider1_token);
				slider2().ValueChanged(slider2_token);
				cd_samp().PrimaryButtonClick(primary_token);
				cd_samp().Closed(closed_token);
				UnloadObject(cd_samp());
				draw_page();
			}
		);
		auto const& r_loader = ResourceLoader::GetForCurrentView();
		tk_samp_caption().Text(r_loader.GetString(L"str_arrow"));
		show_cd_samp();
	}

	// 値をスライダーのヘッダーに格納する.
	template <U_OP U, int S>
	void MainPage::arrow_set_slider(double val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == U_OP::ARROW_SIZE) {
			if constexpr (S == 0) {
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_arrow_width") + L": ";
			}
			if constexpr (S == 1) {
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_arrow_length") + L": ";
			}
			if constexpr (S == 2) {
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_arrow_shift") + L": ";
			}
		}
		if constexpr (U == U_OP::ARROW_SIZE) {
			wchar_t buf[16];
			switch (m_samp_panel.m_page_unit) {
			case UNIT::GRID:
				swprintf_s(buf, FMT_GRID_UNIT, val / (m_samp_panel.m_grid_len + 1.0));
				hdr = hdr + buf;
				break;
			case UNIT::PIXEL:
				swprintf_s(buf, FMT_PIXEL_UNIT, val);
				hdr = hdr + buf;
				break;
			case UNIT::INCH:
				swprintf_s(buf, FMT_INCH_UNIT, val / m_samp_dx.m_logical_dpi);
				hdr = hdr + buf;
				break;
			case UNIT::MILLI:
				swprintf_s(buf, FMT_INCH_UNIT, val / m_samp_dx.m_logical_dpi * MM_PER_INCH);
				hdr = hdr + buf;
				break;
			case UNIT::POINT:
				swprintf_s(buf, FMT_INCH_UNIT, val / m_samp_dx.m_logical_dpi * PT_PER_INCH);
				hdr = hdr + buf;
				break;
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
	void MainPage::arrow_set_slider(Shape* s, const double val)
	{
		arrow_set_slider<U, S>(val);
		if constexpr (U == U_OP::ARROW_SIZE) {
			ARROW_SIZE size;
			s->get_arrow_size(size);
			if constexpr (S == 0) {
				size.m_width = static_cast<FLOAT>(val);
			}
			if constexpr (S == 1) {
				size.m_length = static_cast<FLOAT>(val);
			}
			if constexpr (S == 2) {
				size.m_offset = static_cast<FLOAT>(val);
			}
			s->set_arrow_size(size);
		}
		if (scp_samp_panel().IsLoaded()) {
			draw_samp();
		}
	}

}