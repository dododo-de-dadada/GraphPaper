//-------------------------------
// MainPage_font.cpp
// 書体の設定
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::IInspectable;

	// 書体の太さの配列
	constexpr std::underlying_type_t<DWRITE_FONT_WEIGHT> FONT_WEIGHTS[] = {
		DWRITE_FONT_WEIGHT_THIN,
		DWRITE_FONT_WEIGHT_EXTRA_LIGHT,
		DWRITE_FONT_WEIGHT_LIGHT,
		DWRITE_FONT_WEIGHT_SEMI_LIGHT,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_WEIGHT_MEDIUM,
		DWRITE_FONT_WEIGHT_DEMI_BOLD,
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_WEIGHT_EXTRA_BOLD,
		DWRITE_FONT_WEIGHT_BLACK,
		DWRITE_FONT_WEIGHT_EXTRA_BLACK
	};

	// 書体の伸縮の配列
	constexpr std::underlying_type_t<DWRITE_FONT_STRETCH> FONT_STRETCH[] = {
		DWRITE_FONT_STRETCH_UNDEFINED,
		DWRITE_FONT_STRETCH_ULTRA_CONDENSED,
		DWRITE_FONT_STRETCH_EXTRA_CONDENSED,
		DWRITE_FONT_STRETCH_CONDENSED,
		DWRITE_FONT_STRETCH_SEMI_CONDENSED,
		DWRITE_FONT_STRETCH_NORMAL,
		DWRITE_FONT_STRETCH_SEMI_EXPANDED,
		DWRITE_FONT_STRETCH_EXPANDED,
		DWRITE_FONT_STRETCH_EXTRA_EXPANDED,
		DWRITE_FONT_STRETCH_ULTRA_EXPANDED,
	};

	// 書体の見本を作成する.
	void MainPage::font_create_samp(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		//const auto dpi = m_samp_dx.m_logical_dpi;
		const D2D1_POINT_2F pos{ GRIDLEN_PX, GRIDLEN_PX };
		const auto w = scp_samp_panel().ActualWidth();
		const auto h = scp_samp_panel().ActualHeight();
		const D2D1_POINT_2F vec = {
			static_cast<FLOAT>(w - 2.0 * GRIDLEN_PX),
			static_cast<FLOAT>(h - 2.0 * GRIDLEN_PX)
		};
		auto const& r_loader = ResourceLoader::GetForCurrentView();
		const auto t = wchar_cpy(r_loader.GetString(L"str_pangram").c_str());
		m_samp_shape = new ShapeText(pos, vec, t, &m_samp_panel);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
	}

	// 書体メニューの「字体」に印をつける.
	// f_style	書体の字体
	void MainPage::font_style_check_menu(const DWRITE_FONT_STYLE f_style)
	{
		rmfi_font_italic().IsChecked(f_style == DWRITE_FONT_STYLE_ITALIC);
		rmfi_font_normal().IsChecked(f_style == DWRITE_FONT_STYLE_NORMAL);
		rmfi_font_oblique().IsChecked(f_style == DWRITE_FONT_STYLE_OBLIQUE);

		rmfi_font_italic_2().IsChecked(f_style == DWRITE_FONT_STYLE_ITALIC);
		rmfi_font_normal_2().IsChecked(f_style == DWRITE_FONT_STYLE_NORMAL);
		rmfi_font_oblique_2().IsChecked(f_style == DWRITE_FONT_STYLE_OBLIQUE);
	}

	// リストビュー「書体名」がロードされた.
	void MainPage::lv_font_family_loaded(IInspectable const& /*sender*/, RoutedEventArgs const& /*e*/)
	{
		lv_font_family().Items().Clear();
		for (uint32_t i = 0u; wchar_t* name = ShapeText::get_available_font(i); i++) {
			auto item = box_value(winrt::hstring(name));
			lv_font_family().Items().Append(item);
		}
		for (uint32_t i = 0; i < lv_font_family().Items().Size(); i++) {
			IInspectable item[1];
			lv_font_family().Items().GetMany(i, item);
			auto name = unbox_value<winrt::hstring>(item[0]).c_str();
			if (wcscmp(name, m_page_panel.m_font_family) == 0) {
				lv_font_family().SelectedItem(item[0]);
				lv_font_family().ScrollIntoView(item[0]);
				return;
			}
		}
		//for (auto item : lv_font_family().Items()) {
		//	auto hstr = unbox_value<winrt::hstring>(item);
		//	if (wcscmp(hstr.c_str(), m_page_panel.m_font_family) == 0) {
		//		lv_font_family().SelectedItem(item);
		//		lv_font_family().ScrollIntoView(item);
		//		return;
		//	}
		//}
		lv_font_family().SelectedIndex(-1);
	}

	// リストビュー「書体の伸縮」がロードされた.
	void MainPage::lv_font_stretch_loaded(IInspectable const& /*sender*/, winrt::Windows::UI::Xaml::RoutedEventArgs const& /*e*/)
	{
		const auto k = lv_font_stretch().Items().Size();
		for (uint32_t i = 0; i < k; i++) {
			if (FONT_STRETCH[i] == m_page_panel.m_font_stretch) {
				lv_font_stretch().SelectedIndex(i);
				IInspectable item[1];
				lv_font_stretch().Items().GetMany(i, item);
				lv_font_stretch().ScrollIntoView(item[0]);
				return;
			}
		}
		lv_font_stretch().SelectedIndex(-1);
	}

	// リストビュー「書体の太さ」がロードされた.
	void MainPage::lv_font_weight_loaded(IInspectable const& /*sender*/, winrt::Windows::UI::Xaml::RoutedEventArgs const& /*e*/)
	{
		const auto k = lv_font_weight().Items().Size();
		for (uint32_t i = 0; i < k; i++) {
			if (FONT_WEIGHTS[i] == m_page_panel.m_font_weight) {
				lv_font_weight().SelectedIndex(i);
				IInspectable item[1];
				lv_font_weight().Items().GetMany(i, item);
				lv_font_weight().ScrollIntoView(item[0]);
				return;
			}
		}
		lv_font_weight().SelectedIndex(-1);
	}

	// 書体メニューの「色」が選択された.
	void MainPage::mfi_font_color_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		static winrt::event_token slider0_token;
		static winrt::event_token slider1_token;
		static winrt::event_token slider2_token;
		static winrt::event_token slider3_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_samp();
		const double val0 = m_page_panel.m_font_color.r * COLOR_MAX;
		const double val1 = m_page_panel.m_font_color.g * COLOR_MAX;
		const double val2 = m_page_panel.m_font_color.b * COLOR_MAX;
		const double val3 = m_page_panel.m_font_color.a * COLOR_MAX;
		slider0().Value(val0);
		slider1().Value(val1);
		slider2().Value(val2);
		slider3().Value(val3);
		font_set_slider<U_OP::FONT_COLOR, 0>(val0);
		font_set_slider<U_OP::FONT_COLOR, 1>(val1);
		font_set_slider<U_OP::FONT_COLOR, 2>(val2);
		font_set_slider<U_OP::FONT_COLOR, 3>(val3);
		slider0().Visibility(VISIBLE);
		slider1().Visibility(VISIBLE);
		slider2().Visibility(VISIBLE);
		slider3().Visibility(VISIBLE);
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
				font_set_slider<U_OP::FONT_COLOR, 0>(m_samp_shape, args.NewValue());
			}
		);
		slider1_token = slider1().ValueChanged(
			[this](auto, auto args)
			{
				font_set_slider<U_OP::FONT_COLOR, 1>(m_samp_shape, args.NewValue());
			}
		);
		slider2_token = slider2().ValueChanged(
			[this](auto, auto args)
			{
				font_set_slider<U_OP::FONT_COLOR, 2>(m_samp_shape, args.NewValue());
			}
		);
		slider3_token = slider3().ValueChanged(
			[this](auto, auto args)
			{
				font_set_slider<U_OP::FONT_COLOR, 3>(m_samp_shape, args.NewValue());
			}
		);
		primary_token = cd_samp().PrimaryButtonClick(
			[this](auto, auto)
			{
				D2D1_COLOR_F val;
				m_samp_shape->get_font_color(val);
				undo_push_value<U_OP::FONT_COLOR>(val);
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
				slider3().Visibility(COLLAPSED);
				scp_samp_panel().Loaded(loaded_token);
				slider0().ValueChanged(slider0_token);
				slider1().ValueChanged(slider1_token);
				slider2().ValueChanged(slider2_token);
				slider3().ValueChanged(slider3_token);
				//cd_samp().Opened(opened_token);
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

	// 書体メニューの「書体名」が選択された.
	void MainPage::mfi_font_family_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		static winrt::event_token changed_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_samp();
		loaded_token = scp_samp_panel().Loaded(
			[this](auto, auto)
			{
				samp_panel_loaded();
				font_create_samp();
				draw_samp();
			}
		);
		changed_token = lv_font_family().SelectionChanged(
			[this](auto, auto)
			{
				//int32_t i = lv_font_family().SelectedIndex();
				auto i = lv_font_family().SelectedIndex();
				m_samp_shape->set_font_family(ShapeText::get_available_font(i));
				if (scp_samp_panel().IsLoaded()) {
					draw_samp();
				}
			}
		);
		primary_token = cd_samp().PrimaryButtonClick(
			[this](auto, auto)
			{
				wchar_t* val;
				m_samp_shape->get_font_family(val);
				undo_push_value<U_OP::FONT_FAMILY>(val);
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
				lv_font_family().SelectionChanged(changed_token);
				scp_samp_panel().Loaded(loaded_token);
				cd_samp().PrimaryButtonClick(primary_token);
				//cd_samp().Opened(opened_token);
				cd_samp().Closed(closed_token);
				lv_font_family().Visibility(COLLAPSED);
				UnloadObject(cd_samp());
				draw_page();
			}
		);
		lv_font_family().Visibility(VISIBLE);
		auto const& r_loader = ResourceLoader::GetForCurrentView();
		tk_samp_caption().Text(r_loader.GetString(L"str_font"));
		show_cd_samp();
	}

	// 書体メニューの「イタリック体」が選択された.
	void MainPage::rmfi_font_italic_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<U_OP::FONT_STYLE>(DWRITE_FONT_STYLE_ITALIC);
	}

	// 書体メニューの「標準」が選択された.
	void MainPage::rmfi_font_normal_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<U_OP::FONT_STYLE>(DWRITE_FONT_STYLE_NORMAL);
	}

	// 書体メニューの「斜体」が選択された.
	void MainPage::rmfi_font_oblique_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<U_OP::FONT_STYLE>(DWRITE_FONT_STYLE_OBLIQUE);
	}

	// 書体メニューの「大きさ」が選択された.
	void MainPage::mfi_font_size_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		static winrt::event_token slider0_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_samp();
		const double val0 = m_page_panel.m_font_size;
		slider0().Value(val0);
		font_set_slider<U_OP::FONT_SIZE, 0>(val0);
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
				font_set_slider<U_OP::FONT_SIZE, 0>(m_samp_shape, args.NewValue());
			}
		);
		primary_token = cd_samp().PrimaryButtonClick(
			[this](auto, auto)
			{
				double val;
				m_samp_shape->get_font_size(val);
				undo_push_value<U_OP::FONT_SIZE>(val);
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

	// 書体メニューの「伸縮」が選択された.
	void MainPage::mfi_font_stretch_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		static winrt::event_token changed_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_samp();
		loaded_token = scp_samp_panel().Loaded(
			[this](auto, auto)
			{
				samp_panel_loaded();
				font_create_samp();
				draw_samp();
			}
		);
		changed_token = lv_font_stretch().SelectionChanged(
			[this](auto, auto args) {
				uint32_t i = lv_font_stretch().SelectedIndex();
				m_samp_shape->set_font_stretch(static_cast<DWRITE_FONT_STRETCH>(FONT_STRETCH[i]));
				if (scp_samp_panel().IsLoaded()) {
					draw_samp();
				}
			}
		);
		primary_token = cd_samp().PrimaryButtonClick(
			[this](auto, auto)
			{
				DWRITE_FONT_STRETCH val;
				m_samp_shape->get_font_stretch(val);
				undo_push_value<U_OP::FONT_STRETCH>(val);
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
				scp_samp_panel().Loaded(loaded_token);
				lv_font_stretch().SelectionChanged(changed_token);
				cd_samp().PrimaryButtonClick(primary_token);
				cd_samp().Closed(closed_token);
				lv_font_stretch().Visibility(COLLAPSED);
				UnloadObject(cd_samp());
				draw_page();
			}
		);
		lv_font_stretch().Visibility(VISIBLE);
		auto const& r_loader = ResourceLoader::GetForCurrentView();
		tk_samp_caption().Text(r_loader.GetString(L"str_font"));
		show_cd_samp();
	}

	// 書体メニューの「太さ」が選択された.
	void MainPage::mfi_font_weight_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		static winrt::event_token changed_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_samp();
		loaded_token = scp_samp_panel().Loaded(
			[this](auto, auto)
			{
				samp_panel_loaded();
				font_create_samp();
				draw_samp();
			}
		);
		changed_token = lv_font_weight().SelectionChanged(
			[this](auto, auto args) {
				uint32_t i = lv_font_weight().SelectedIndex();
				m_samp_shape->set_font_weight(static_cast<DWRITE_FONT_WEIGHT>(FONT_WEIGHTS[i]));
				if (scp_samp_panel().IsLoaded()) {
					draw_samp();
				}
			}
		);
		primary_token = cd_samp().PrimaryButtonClick(
			[this](auto, auto)
			{
				DWRITE_FONT_WEIGHT val;
				m_samp_shape->get_font_weight(val);
				undo_push_value<U_OP::FONT_WEIGHT>(val);
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
				scp_samp_panel().Loaded(loaded_token);
				lv_font_weight().SelectionChanged(changed_token);
				cd_samp().PrimaryButtonClick(primary_token);
				//cd_samp().Opened(opened_token);
				cd_samp().Closed(closed_token);
				lv_font_weight().Visibility(COLLAPSED);
				UnloadObject(cd_samp());
				draw_page();
			}
		);
		lv_font_weight().Visibility(VISIBLE);
		auto const& r_loader = ResourceLoader::GetForCurrentView();
		tk_samp_caption().Text(r_loader.GetString(L"str_font"));
		show_cd_samp();
	}

	// 値をスライダーのヘッダーに格納する.
	template <U_OP U, int S>
	void MainPage::font_set_slider(const double val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == U_OP::FONT_SIZE) {
			wchar_t buf[16];
			const double pt = val / m_page_dx.m_logical_dpi * PT_PER_INCH;
			swprintf_s(buf, FMT_POINT_UNIT, pt);
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			hdr = r_loader.GetString(L"str_size") + L": " + buf;
		}
		if constexpr (U == U_OP::FONT_COLOR) {
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
				conv_val_to_col(COL_STYLE::CEN, val, buf, 16);
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
	template <U_OP U, int S>
	void MainPage::font_set_slider(Shape* s, const double val)
	{
		font_set_slider<U, S>(val);
		if constexpr (U == U_OP::FONT_SIZE) {
			s->set_font_size(val);
		}
		if constexpr (U == U_OP::FONT_COLOR) {
			D2D1_COLOR_F col;
			s->get_font_color(col);
			if constexpr (S == 0) {
				col.r = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 1) {
				col.g = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 2) {
				col.b = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 3) {
				col.a = static_cast<FLOAT>(val / COLOR_MAX);
			}
			s->set_font_color(col);
		}
		if (scp_samp_panel().IsLoaded()) {
			draw_samp();
		}
	}

}