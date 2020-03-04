//-------------------------------
// MainPage_arrow.cpp
// 矢じりの形式と寸法を設定
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t TITLE_ARROWHEAD[] = L"str_arrowhead";

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S>
	void MainPage::arrow_set_slider_header(const double val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == UNDO_OP::ARROW_SIZE) {
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
		if constexpr (U == UNDO_OP::ARROW_SIZE) {
			wchar_t buf[16];
			const auto dpi = m_sample_dx.m_logical_dpi;
			const auto g_len = m_sample_panel.m_grid_size + 1.0;
			conv_val_to_len(m_page_unit, val, dpi, g_len, buf, 16);
			hdr = hdr + buf;
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
	void MainPage::arrow_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* s = m_sample_shape;
		const double val = args.NewValue();
		arrow_set_slider_header<U, S>(val);
		if constexpr (U == UNDO_OP::ARROW_SIZE) {
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
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

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

	// ストロークメニューの「矢じりの大きさ」が選択された.
	IAsyncAction MainPage::mfi_arrow_size_click(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		const double val0 = m_page_panel.m_arrow_size.m_width;
		const double val1 = m_page_panel.m_arrow_size.m_length;
		const double val2 = m_page_panel.m_arrow_size.m_offset;
		slider0().Value(val0);
		slider1().Value(val1);
		slider2().Value(val2);
		arrow_set_slider_header<UNDO_OP::ARROW_SIZE, 0>(val0);
		arrow_set_slider_header<UNDO_OP::ARROW_SIZE, 1>(val1);
		arrow_set_slider_header<UNDO_OP::ARROW_SIZE, 2>(val2);
		slider0().Visibility(VISIBLE);
		slider1().Visibility(VISIBLE);
		slider2().Visibility(VISIBLE);
		const auto slider0_token = slider0().ValueChanged({ this, &MainPage::arrow_set_slider< UNDO_OP::ARROW_SIZE, 0> });
		const auto slider1_token = slider1().ValueChanged({ this, &MainPage::arrow_set_slider< UNDO_OP::ARROW_SIZE, 1> });
		const auto slider2_token = slider2().ValueChanged({ this, &MainPage::arrow_set_slider< UNDO_OP::ARROW_SIZE, 2> });
		m_sample_type = SAMP_TYPE::STROKE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_ARROWHEAD)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			ARROW_SIZE sample_val;
			m_sample_shape->get_arrow_size(sample_val);
			undo_push_value<UNDO_OP::ARROW_SIZE>(sample_val);
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		slider0().Visibility(COLLAPSED);
		slider1().Visibility(COLLAPSED);
		slider2().Visibility(COLLAPSED);
		slider0().ValueChanged(slider0_token);
		slider1().ValueChanged(slider1_token);
		slider2().ValueChanged(slider2_token);
		page_draw();
	}

	//	ストロークメニューの「矢じりの種類」>「閉じた矢」が選択された.
	void MainPage::rmfi_arrow_filled_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_panel.m_arrow_style == ARROW_STYLE::NONE) {
			mfi_arrow_size().IsEnabled(true);
			mfi_arrow_size_2().IsEnabled(true);
		}
		undo_push_value<UNDO_OP::ARROW_STYLE>(ARROW_STYLE::FILLED);
	}

	//	ストロークメニューの「矢じりの種類」>「なし」が選択された.
	void MainPage::rmfi_arrow_none_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_panel.m_arrow_style != ARROW_STYLE::NONE) {
			mfi_arrow_size().IsEnabled(false);
			mfi_arrow_size_2().IsEnabled(false);
		}
		undo_push_value<UNDO_OP::ARROW_STYLE>(ARROW_STYLE::NONE);
	}

	//	ストロークメニューの「矢じりの種類」>「開いた」が選択された.
	void MainPage::rmfi_arrow_opened_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_page_panel.m_arrow_style == ARROW_STYLE::NONE) {
			mfi_arrow_size().IsEnabled(true);
			mfi_arrow_size_2().IsEnabled(true);
		}
		undo_push_value<UNDO_OP::ARROW_STYLE>(ARROW_STYLE::OPENED);
	}

}