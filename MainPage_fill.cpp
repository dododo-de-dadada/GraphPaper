//-------------------------------
// MainPage_fill.cpp
// 塗りつぶしの設定
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t TITLE_FILL[] = L"str_fill";

	// 塗りつぶしメニューの「色」が選択された.
	IAsyncAction MainPage::mfi_fill_color_click(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		const double val0 = m_page_panel.m_fill_color.r * COLOR_MAX;
		const double val1 = m_page_panel.m_fill_color.g * COLOR_MAX;
		const double val2 = m_page_panel.m_fill_color.b * COLOR_MAX;
		const double val3 = m_page_panel.m_fill_color.a * COLOR_MAX;
		slider0().Value(val0);
		slider1().Value(val1);
		slider2().Value(val2);
		slider3().Value(val3);
		fill_set_slider_header<UNDO_OP::FILL_COLOR, 0>(val0);
		fill_set_slider_header<UNDO_OP::FILL_COLOR, 1>(val1);
		fill_set_slider_header<UNDO_OP::FILL_COLOR, 2>(val2);
		fill_set_slider_header<UNDO_OP::FILL_COLOR, 3>(val3);
		slider0().Visibility(VISIBLE);
		slider1().Visibility(VISIBLE);
		slider2().Visibility(VISIBLE);
		slider3().Visibility(VISIBLE);
		const auto slider0_token = slider0().ValueChanged({ this, &MainPage::fill_set_slider<UNDO_OP::FILL_COLOR, 0> });
		const auto slider1_token = slider1().ValueChanged({ this, &MainPage::fill_set_slider<UNDO_OP::FILL_COLOR, 1> });
		const auto slider2_token = slider2().ValueChanged({ this, &MainPage::fill_set_slider<UNDO_OP::FILL_COLOR, 2> });
		const auto slider3_token = slider3().ValueChanged({ this, &MainPage::fill_set_slider<UNDO_OP::FILL_COLOR, 3> });
		m_sample_type = SAMP_TYPE::FILL;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_FILL)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F sample_val;
			m_sample_shape->get_fill_color(sample_val);
			undo_push_value<UNDO_OP::FILL_COLOR>(sample_val);
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		slider0().Visibility(COLLAPSED);
		slider1().Visibility(COLLAPSED);
		slider2().Visibility(COLLAPSED);
		slider3().Visibility(COLLAPSED);
		slider0().ValueChanged(slider0_token);
		slider1().ValueChanged(slider1_token);
		slider2().ValueChanged(slider2_token);
		slider3().ValueChanged(slider3_token);
		page_draw();
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S>
	void MainPage::fill_set_slider_header(double val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == UNDO_OP::FILL_COLOR) {
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

	// 値をスライダーのヘッダーと図形に格納する.
	template <UNDO_OP U, int S>
	void MainPage::fill_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* s = m_sample_shape;
		const double val = args.NewValue();
		fill_set_slider_header<U, S>(val);
		if constexpr (U == UNDO_OP::FILL_COLOR) {
			D2D1_COLOR_F col;
			s->get_fill_color(col);
			if constexpr (S == 0) {
				col.r = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 1) {
				col.g = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 2) {
				col.b = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (U != UNDO_OP::PAGE_COLOR && S == 3) {
				col.a = static_cast<FLOAT>(val / COLOR_MAX);
			}
			s->set_fill_color(col);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

}