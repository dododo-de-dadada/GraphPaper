//-------------------------------
// MainPage_page.cpp
// ページの設定
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
		UNIT unit;
		D2D1_SIZE_F page;

		swscanf_s(tx_page_width().Text().c_str(), L"%lf", &pw);
		swscanf_s(tx_page_height().Text().c_str(), L"%lf", &ph);
		switch (unit = static_cast<UNIT>(cx_page_unit().SelectedIndex())) {
		default:
			return;
		case UNIT::PIXEL:
			page.width = static_cast<FLOAT>(pw);
			page.height = static_cast<FLOAT>(ph);
			break;
		case UNIT::INCH:
			page.width = static_cast<FLOAT>(std::round(pw * dpi));
			page.height = static_cast<FLOAT>(std::round(ph * dpi));
			break;
		case UNIT::MILLI:
			page.width = static_cast<FLOAT>(std::round(pw / MM_PER_INCH * dpi));
			page.height = static_cast<FLOAT>(std::round(ph / MM_PER_INCH * dpi));
			break;
		case UNIT::POINT:
			page.width = static_cast<FLOAT>(std::round(pw / PT_PER_INCH * dpi));
			page.height = static_cast<FLOAT>(std::round(ph / PT_PER_INCH * dpi));
			break;
		case UNIT::GRID:
			page.width = static_cast<FLOAT>(std::round(pw * (m_samp_panel.m_grid_len + 1.0)));
			page.height = static_cast<FLOAT>(std::round(ph * (m_samp_panel.m_grid_len + 1.0)));
			break;
		}
		m_page_panel.m_page_unit = unit;
		if (equal(m_page_panel.m_page_size, page) == false) {
			undo_push_set<U_OP::PAGE_SIZE>(&m_page_panel, page);
			undo_push_null();
			enable_undo_menu();
		}
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		draw_page();
		stat_set_curs();
		stat_set_page();
		stat_set_unit();
		stat_set_grid();
	}

	// ページ寸法ダイアログの「図形に合わせる」ボタンが押された.
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
		const auto unit = static_cast<UNIT>(cx_page_unit().SelectedIndex());
		m_page_panel.m_page_unit = unit;
		if (equal(m_page_panel.m_page_size, page) == false) {
			undo_push_set<U_OP::PAGE_SIZE>(&m_page_panel, page);
			undo_push_null();
			enable_undo_menu();
		}
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		draw_page();
		stat_set_page();
	}

	// ページ寸法ダイアログの「単位」コンボボックスが変更された.
	void MainPage::cx_page_unit_changed(IInspectable const& /*sender*/, SelectionChangedEventArgs const& /*args*/)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		//const double dpi = m_page_panel.m_dx.m_logical_dpi;
		static double pw;
		static double ph;
		static int si = -1;
		wchar_t const* fmt = L"";

		if (static_cast<int32_t>(m_samp_panel.m_page_unit) == cx_page_unit().SelectedIndex()) {
			return;
		}
		if (swscanf_s(tx_page_width().Text().c_str(), L"%lf", &pw) != 1
			|| swscanf_s(tx_page_height().Text().c_str(), L"%lf", &ph) != 1) {
			return;
		}
		switch (m_samp_panel.m_page_unit) {
		case UNIT::PIXEL:
			break;
		case UNIT::INCH:
			pw *= dpi;
			ph *= dpi;
			break;
		case UNIT::MILLI:
			pw = pw / MM_PER_INCH * dpi;
			ph = ph / MM_PER_INCH * dpi;
			break;
		case UNIT::POINT:
			pw = pw / PT_PER_INCH * dpi;
			ph = ph / PT_PER_INCH * dpi;
			break;
		case UNIT::GRID:
			pw *= m_samp_panel.m_grid_len + 1.0;
			ph *= m_samp_panel.m_grid_len + 1.0;
			break;
		}
		pw = std::round(pw);
		ph = std::round(ph);
		m_samp_panel.m_page_unit = static_cast<UNIT>(cx_page_unit().SelectedIndex());
		switch (m_samp_panel.m_page_unit) {
		case UNIT::PIXEL:
			fmt = FMT_PX;
			break;
		case UNIT::INCH:
			fmt = FMT_IN;
			pw /= dpi;
			ph /= dpi;
			break;
		case UNIT::MILLI:
			fmt = FMT_MM;
			pw = pw / dpi * MM_PER_INCH;
			ph = ph / dpi * MM_PER_INCH;
			break;
		case UNIT::POINT:
			fmt = FMT_PT;
			pw = pw / dpi * PT_PER_INCH;
			ph = ph / dpi * PT_PER_INCH;
			break;
		case UNIT::GRID:
			fmt = FMT_GD;
			pw /= m_samp_panel.m_grid_len + 1.0;
			ph /= m_samp_panel.m_grid_len + 1.0;
			break;
		}
		wchar_t buf[16];
		swprintf_s(buf, fmt, pw);
		tx_page_width().Text(buf);
		swprintf_s(buf, fmt, ph);
		tx_page_height().Text(buf);
	}

	// ページメニューの「色」が選択された.
	void MainPage::mfi_page_color_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		static winrt::event_token slider0_token;
		static winrt::event_token slider1_token;
		static winrt::event_token slider2_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_samp();
		const double val0 = m_page_panel.m_page_color.r * COLOR_MAX;
		const double val1 = m_page_panel.m_page_color.g * COLOR_MAX;
		const double val2 = m_page_panel.m_page_color.b * COLOR_MAX;
		slider0().Value(val0);
		slider1().Value(val1);
		slider2().Value(val2);
		page_set_slider<U_OP::PAGE_COLOR, 0>(val0);
		page_set_slider<U_OP::PAGE_COLOR, 1>(val1);
		page_set_slider<U_OP::PAGE_COLOR, 2>(val2);
		slider0().Visibility(VISIBLE);
		slider1().Visibility(VISIBLE);
		slider2().Visibility(VISIBLE);
		loaded_token = scp_samp_panel().Loaded(
			[this](auto, auto)
			{
				samp_panel_loaded();
				draw_samp();
			}
		);
		slider0_token = slider0().ValueChanged(
			[this](auto, auto args)
			{
				page_set_slider<U_OP::PAGE_COLOR, 0>(&m_samp_panel, args.NewValue());
			}
		);
		slider1_token = slider1().ValueChanged(
			[this](auto, auto args)
			{
				page_set_slider<U_OP::PAGE_COLOR, 1>(&m_samp_panel, args.NewValue());
			}
		);
		slider2_token = slider2().ValueChanged(
			[this](auto, auto args)
			{
				page_set_slider<U_OP::PAGE_COLOR, 2>(&m_samp_panel, args.NewValue());
			}
		);
		primary_token = cd_samp().PrimaryButtonClick(
			[this](auto, auto)
			{
				D2D1_COLOR_F samp_val;
				D2D1_COLOR_F page_val;

				m_page_panel.get_page_color(page_val);
				m_samp_panel.get_page_color(samp_val);
				if (equal(page_val, samp_val)) {
					return;
				}
				undo_push_set<U_OP::PAGE_COLOR>(&m_page_panel, samp_val);
				undo_push_null();
				enable_undo_menu();
				draw_page();
			}
		);
		closed_token = cd_samp().Closed(
			[this](auto, auto)
			{
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
		auto r_loader = ResourceLoader::GetForCurrentView();
		tk_samp_caption().Text(r_loader.GetString(L"str_page"));
		show_cd_samp();
	}

	// ページメニューの「大きさ」が選択された
	void MainPage::mfi_page_size_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		//const double dpi = m_page_panel.m_dx.m_logical_dpi;
		wchar_t const* fmt = nullptr;
		double pw;
		double ph;
		m_samp_panel.m_page_unit = m_page_panel.m_page_unit;
		m_samp_panel.m_grid_len = m_page_panel.m_grid_len;
		pw = m_page_panel.m_page_size.width;
		ph = m_page_panel.m_page_size.height;
		switch (m_samp_panel.m_page_unit) {
		default:
			return;
		case UNIT::PIXEL:
			fmt = FMT_PX;
			break;
		case UNIT::INCH:
			fmt = FMT_IN;
			pw = pw / dpi;
			ph = ph / dpi;
			break;
		case UNIT::MILLI:
			fmt = FMT_MM;
			pw = pw / dpi * MM_PER_INCH;
			ph = ph / dpi * MM_PER_INCH;
			break;
		case UNIT::POINT:
			fmt = FMT_PT;
			pw = pw / dpi * PT_PER_INCH;
			ph = ph / dpi * PT_PER_INCH;
			break;
		case UNIT::GRID:
			fmt = FMT_GD;
			pw /= m_samp_panel.m_grid_len + 1.0;
			ph /= m_samp_panel.m_grid_len + 1.0;
			break;
		}
		wchar_t buf[16];
		swprintf_s(buf, fmt, pw);
		tx_page_width().Text(buf);
		swprintf_s(buf, fmt, ph);
		tx_page_height().Text(buf);
		// この時点では, テキストボックスに正しい数値を格納しても, 
		// TextChanged は呼ばれない.
		// プライマリーボタンは使用可能にしておく.
		cd_page_size().IsPrimaryButtonEnabled(true);
		cx_page_unit().SelectedIndex(static_cast<int32_t>(m_samp_panel.m_page_unit));
		cd_page_size().IsSecondaryButtonEnabled(m_list_shapes.size() > 0);
		const auto _ = cd_page_size().ShowAsync();
	}

	// 値をスライダーのヘッダーに格納する.
	template <U_OP U, int S>
	void MainPage::page_set_slider(double val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == U_OP::GRID_LEN) {
			auto r_loader = ResourceLoader::GetForCurrentView();
			hdr = r_loader.GetString(L"str_length");
			val += 1.0;
			if (m_samp_panel.m_page_unit == UNIT::PIXEL) {
				wchar_t buf[16];
				swprintf_s(buf, FMT_PX_UNIT, val);
				hdr = hdr + buf;
			}
			else if (m_samp_panel.m_page_unit == UNIT::GRID) {
				wchar_t buf[16];
				swprintf_s(buf, FMT_GD_UNIT, val / (m_samp_panel.m_grid_len + 1.0));
				hdr = hdr + buf;
			}
			else {
				wchar_t buf[16];
				const double inch = val / m_samp_dx.m_logical_dpi;
				switch (m_samp_panel.m_page_unit) {
				case UNIT::INCH:
					swprintf_s(buf, FMT_IN_UNIT, inch);
					hdr = hdr + buf;
					break;
				case UNIT::MILLI:
					swprintf_s(buf, FMT_MM_UNIT, inch * MM_PER_INCH);
					hdr = hdr + buf;
					break;
				case UNIT::POINT:
					swprintf_s(buf, FMT_PT_UNIT, inch * PT_PER_INCH);
					hdr = hdr + buf;
					break;
				}
			}
		}
		if constexpr (U == U_OP::PAGE_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[16];
				swprintf_s(buf, FMT_RGB, val);
				auto r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_red") + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[16];
				swprintf_s(buf, FMT_RGB, val);
				auto r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_green") + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[16];
				swprintf_s(buf, FMT_RGB, val);
				auto r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_blue") + buf;
			}
		}
		if constexpr (U == U_OP::GRID_OPAC) {
			if constexpr (S == 3) {
				wchar_t buf[16];
				swprintf_s(buf, FMT_PERCENT, val / COLOR_MAX * 100.0);
				auto r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_opacity") + buf;
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
	void MainPage::page_set_slider(Shape* s, const double val)
	{
		page_set_slider<U, S>(val);
		if constexpr (U == U_OP::GRID_LEN) {
			s->set_grid_len(val);
		}
		if constexpr (U == U_OP::GRID_OPAC) {
			s->set_grid_opac(val / COLOR_MAX);
		}
		if constexpr (U == U_OP::PAGE_COLOR) {
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
			//m_samp_dx.m_anch_brush->SetColor(m_samp_panel.m_anch_color);
			//m_samp_dx.m_aux_brush->SetColor(m_samp_panel.m_aux_color);
		}
		if (scp_samp_panel().IsLoaded()) {
			draw_samp();
		}
	}

	// ページのパネルがロードされた.
	void MainPage::scp_page_panel_loaded(IInspectable const& sender, RoutedEventArgs const&/*args*/)
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif // _DEBUG)
		m_page_dx.SetSwapChainPanel(scp_page_panel());
		//m_page_panel.m_dx.SetSwapChainPanel(scp_page_panel());
		draw_page();
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
		draw_page();
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
			switch (static_cast<UNIT>(cx_page_unit().SelectedIndex())) {
			case UNIT::INCH:
				value = std::round(value * dpi);
				break;
			case UNIT::MILLI:
				value = std::round(value * dpi / MM_PER_INCH);
				break;
			case UNIT::POINT:
				value = std::round(value * dpi / PT_PER_INCH);
				break;
			case UNIT::GRID:
				value = std::round(value * (m_page_panel.m_grid_len + 1.0));
				break;
			default:
				value = std::round(value);
				break;
			}
		}
		cd_page_size().IsPrimaryButtonEnabled(cnt == 1 && value >= 1.0 && value < 32768.0);
	}

	/*
	void MainPage::rmfi_unit_inch_click(IInspectable const&, RoutedEventArgs const&)
	{
		set_page_unit<UNIT::INCH>();
	}

	void MainPage::rmfi_unit_milli_click(IInspectable const&, RoutedEventArgs const&)
	{
		set_page_unit<UNIT::MILLI>();
	}

	void MainPage::rmfi_unit_pixel_click(IInspectable const&, RoutedEventArgs const&)
	{
		set_page_unit<UNIT::PIXEL>();
	}

	void MainPage::rmfi_unit_point_click(IInspectable const&, RoutedEventArgs const&)
	{
		set_page_unit<UNIT::POINT>();
	}
	*/

}