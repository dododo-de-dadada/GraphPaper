//-------------------------------
// MainPage_font.cpp
// 書体または文字列の設定
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

	constexpr wchar_t TITLE_FONT[] = L"str_font";

	// 書体の見本を作成する.
	void MainPage::font_create_sample(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		const auto w = scp_sample_panel().ActualWidth();
		const auto h = scp_sample_panel().ActualHeight();
		const auto padding = w * 0.125;
		const D2D1_POINT_2F pos = {
			static_cast<FLOAT>(padding),
			static_cast<FLOAT>(padding)
		};
		const D2D1_POINT_2F vec = {
			static_cast<FLOAT>(w - 2.0 * padding),
			static_cast<FLOAT>(h - 2.0 * padding)
		};
		auto const& r_loader = ResourceLoader::GetForCurrentView();
		const auto t = wchar_cpy(r_loader.GetString(L"str_pangram").c_str());
		m_sample_shape = new ShapeText(pos, vec, t, &m_sample_panel);
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
		static winrt::event_token slider0_token;
		static winrt::event_token slider1_token;
		static winrt::event_token slider2_token;
		static winrt::event_token slider3_token;
		//static winrt::event_token c_style_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_sample();
		const double val0 = m_page_panel.m_font_color.r * COLOR_MAX;
		const double val1 = m_page_panel.m_font_color.g * COLOR_MAX;
		const double val2 = m_page_panel.m_font_color.b * COLOR_MAX;
		const double val3 = m_page_panel.m_font_color.a * COLOR_MAX;
		slider0().Value(val0);
		slider1().Value(val1);
		slider2().Value(val2);
		slider3().Value(val3);
		//cx_color_style().SelectedIndex(m_page_panel.m_col_style);
		font_set_slider<UNDO_OP::FONT_COLOR, 0>(val0);
		font_set_slider<UNDO_OP::FONT_COLOR, 1>(val1);
		font_set_slider<UNDO_OP::FONT_COLOR, 2>(val2);
		font_set_slider<UNDO_OP::FONT_COLOR, 3>(val3);
		slider0().Visibility(VISIBLE);
		slider1().Visibility(VISIBLE);
		slider2().Visibility(VISIBLE);
		slider3().Visibility(VISIBLE);
		//cx_color_style().Visibility(VISIBLE);
		loaded_token = scp_sample_panel().Loaded(
			[this](auto, auto)
			{
				sample_panel_loaded();
				font_create_sample();
				sample_draw();
			}
		);
		slider0_token = slider0().ValueChanged(
			[this](auto, auto args)
			{
				font_set_slider<UNDO_OP::FONT_COLOR, 0>(m_sample_shape, args.NewValue());
			}
		);
		slider1_token = slider1().ValueChanged(
			[this](auto, auto args)
			{
				font_set_slider<UNDO_OP::FONT_COLOR, 1>(m_sample_shape, args.NewValue());
			}
		);
		slider2_token = slider2().ValueChanged(
			[this](auto, auto args)
			{
				font_set_slider<UNDO_OP::FONT_COLOR, 2>(m_sample_shape, args.NewValue());
			}
		);
		slider3_token = slider3().ValueChanged(
			[this](auto, auto args)
			{
				font_set_slider<UNDO_OP::FONT_COLOR, 3>(m_sample_shape, args.NewValue());
			}
		);
		//c_style_token = cx_color_style().SelectionChanged(
		//	[this](auto, auto args)
		//	{
		//		m_sample_panel.m_col_style = static_cast<COL_STYLE>(cx_color_style().SelectedIndex());
		//		font_set_slider<UNDO_OP::FONT_COLOR, 0>(m_sample_shape, slider0().Value());
		//		font_set_slider<UNDO_OP::FONT_COLOR, 1>(m_sample_shape, slider1().Value());
		//		font_set_slider<UNDO_OP::FONT_COLOR, 2>(m_sample_shape, slider2().Value());
		//		font_set_slider<UNDO_OP::FONT_COLOR, 3>(m_sample_shape, slider3().Value());
		//	}
		//);
		primary_token = cd_sample().PrimaryButtonClick(
			[this](auto, auto)
			{
				//m_page_panel.m_col_style = m_sample_panel.m_col_style;
				D2D1_COLOR_F sample_val;
				m_sample_shape->get_font_color(sample_val);
				//D2D1_COLOR_F page_val;
				//m_page_shape->get_font_color(page_val);
				//if (equal(sample_val, page_val)) {
				//	return;
				//}
				undo_push_value<UNDO_OP::FONT_COLOR>(sample_val);
			}
		);
		closed_token = cd_sample().Closed(
			[this](auto, auto)
			{
				delete m_sample_shape;
#if defined(_DEBUG)
				debug_leak_cnt--;
#endif
				m_sample_shape = nullptr;
				slider0().Visibility(COLLAPSED);
				slider1().Visibility(COLLAPSED);
				slider2().Visibility(COLLAPSED);
				slider3().Visibility(COLLAPSED);
				//cx_color_style().Visibility(COLLAPSED);
				scp_sample_panel().Loaded(loaded_token);
				slider0().ValueChanged(slider0_token);
				slider1().ValueChanged(slider1_token);
				slider2().ValueChanged(slider2_token);
				slider3().ValueChanged(slider3_token);
				//cx_color_style().SelectionChanged(c_style_token);
				cd_sample().PrimaryButtonClick(primary_token);
				cd_sample().Closed(closed_token);
				UnloadObject(cd_sample());
				page_draw();
			}
		);
		show_cd_sample(TITLE_FONT);
	}

	// 書体メニューの「書体名」が選択された.
	void MainPage::mfi_font_family_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		static winrt::event_token changed_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_sample();
		loaded_token = scp_sample_panel().Loaded(
			[this](auto, auto)
			{
				sample_panel_loaded();
				font_create_sample();
				sample_draw();
			}
		);
		changed_token = lv_font_family().SelectionChanged(
			[this](auto, auto)
			{
				//int32_t i = lv_font_family().SelectedIndex();
				auto i = lv_font_family().SelectedIndex();
				m_sample_shape->set_font_family(ShapeText::get_available_font(i));
				if (scp_sample_panel().IsLoaded()) {
					sample_draw();
				}
			}
		);
		primary_token = cd_sample().PrimaryButtonClick(
			[this](auto, auto)
			{
				wchar_t* sample_val;
				m_sample_shape->get_font_family(sample_val);
				undo_push_value<UNDO_OP::FONT_FAMILY>(sample_val);
			}
		);
		closed_token = cd_sample().Closed(
			[this](auto, auto)
			{
				delete m_sample_shape;
#if defined(_DEBUG)
				debug_leak_cnt--;
#endif
				m_sample_shape = nullptr;
				lv_font_family().SelectionChanged(changed_token);
				scp_sample_panel().Loaded(loaded_token);
				cd_sample().PrimaryButtonClick(primary_token);
				//cd_sample().Opened(opened_token);
				cd_sample().Closed(closed_token);
				lv_font_family().Visibility(COLLAPSED);
				UnloadObject(cd_sample());
				page_draw();
			}
		);
		lv_font_family().Visibility(VISIBLE);
		show_cd_sample(TITLE_FONT);
	}

	// 書体メニューの「イタリック体」が選択された.
	void MainPage::rmfi_font_italic_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE_ITALIC);
	}

	// 書体メニューの「標準」が選択された.
	void MainPage::rmfi_font_normal_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE_NORMAL);
	}

	// 書体メニューの「斜体」が選択された.
	void MainPage::rmfi_font_oblique_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE_OBLIQUE);
	}

	// 書体メニューの「大きさ」が選択された.
	void MainPage::mfi_font_size_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		static winrt::event_token slider0_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_sample();
		const double val0 = m_page_panel.m_font_size;
		slider0().Value(val0);
		font_set_slider<UNDO_OP::FONT_SIZE, 0>(val0);
		slider0().Visibility(VISIBLE);
		loaded_token = scp_sample_panel().Loaded(
			[this](auto, auto)
			{
				sample_panel_loaded();
				font_create_sample();
				sample_draw();
			}
		);
		slider0_token = slider0().ValueChanged(
			[this](auto, auto args)
			{
				font_set_slider<UNDO_OP::FONT_SIZE, 0>(m_sample_shape, args.NewValue());
			}
		);
		primary_token = cd_sample().PrimaryButtonClick(
			[this](auto, auto)
			{
				double sample_val;
				m_sample_shape->get_font_size(sample_val);
				undo_push_value<UNDO_OP::FONT_SIZE>(sample_val);
			}
		);
		closed_token = cd_sample().Closed(
			[this](auto, auto)
			{
				delete m_sample_shape;
#if defined(_DEBUG)
				debug_leak_cnt--;
#endif
				m_sample_shape = nullptr;
				slider0().Visibility(COLLAPSED);
				scp_sample_panel().Loaded(loaded_token);
				slider0().ValueChanged(slider0_token);
				cd_sample().PrimaryButtonClick(primary_token);
				cd_sample().Closed(closed_token);
				UnloadObject(cd_sample());
				page_draw();
			}
		);
		show_cd_sample(TITLE_FONT);
	}

	// 書体メニューの「伸縮」が選択された.
	void MainPage::mfi_font_stretch_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		static winrt::event_token changed_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_sample();
		loaded_token = scp_sample_panel().Loaded(
			[this](auto, auto)
			{
				sample_panel_loaded();
				font_create_sample();
				sample_draw();
			}
		);
		changed_token = lv_font_stretch().SelectionChanged(
			[this](auto, auto args) {
				uint32_t i = lv_font_stretch().SelectedIndex();
				m_sample_shape->set_font_stretch(static_cast<DWRITE_FONT_STRETCH>(FONT_STRETCH[i]));
				if (scp_sample_panel().IsLoaded()) {
					sample_draw();
				}
			}
		);
		primary_token = cd_sample().PrimaryButtonClick(
			[this](auto, auto)
			{
				DWRITE_FONT_STRETCH sample_val;
				m_sample_shape->get_font_stretch(sample_val);
				undo_push_value<UNDO_OP::FONT_STRETCH>(sample_val);
			}
		);
		closed_token = cd_sample().Closed(
			[this](auto, auto)
			{
				delete m_sample_shape;
#if defined(_DEBUG)
				debug_leak_cnt--;
#endif
				m_sample_shape = nullptr;
				scp_sample_panel().Loaded(loaded_token);
				lv_font_stretch().SelectionChanged(changed_token);
				cd_sample().PrimaryButtonClick(primary_token);
				cd_sample().Closed(closed_token);
				lv_font_stretch().Visibility(COLLAPSED);
				UnloadObject(cd_sample());
				page_draw();
			}
		);
		lv_font_stretch().Visibility(VISIBLE);
		show_cd_sample(TITLE_FONT);
	}

	// 書体メニューの「太さ」が選択された.
	void MainPage::mfi_font_weight_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		static winrt::event_token changed_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_sample();
		loaded_token = scp_sample_panel().Loaded(
			[this](auto, auto)
			{
				sample_panel_loaded();
				font_create_sample();
				sample_draw();
			}
		);
		changed_token = lv_font_weight().SelectionChanged(
			[this](auto, auto args) {
				uint32_t i = lv_font_weight().SelectedIndex();
				m_sample_shape->set_font_weight(static_cast<DWRITE_FONT_WEIGHT>(FONT_WEIGHTS[i]));
				if (scp_sample_panel().IsLoaded()) {
					sample_draw();
				}
			}
		);
		primary_token = cd_sample().PrimaryButtonClick(
			[this](auto, auto)
			{
				DWRITE_FONT_WEIGHT sample_val;
				m_sample_shape->get_font_weight(sample_val);
				undo_push_value<UNDO_OP::FONT_WEIGHT>(sample_val);
			}
		);
		closed_token = cd_sample().Closed(
			[this](auto, auto)
			{
				delete m_sample_shape;
#if defined(_DEBUG)
				debug_leak_cnt--;
#endif
				m_sample_shape = nullptr;
				scp_sample_panel().Loaded(loaded_token);
				lv_font_weight().SelectionChanged(changed_token);
				cd_sample().PrimaryButtonClick(primary_token);
				//cd_sample().Opened(opened_token);
				cd_sample().Closed(closed_token);
				lv_font_weight().Visibility(COLLAPSED);
				UnloadObject(cd_sample());
				page_draw();
			}
		);
		lv_font_weight().Visibility(VISIBLE);
		show_cd_sample(TITLE_FONT);
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S>
	void MainPage::font_set_slider(const double val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == UNDO_OP::FONT_SIZE) {
			wchar_t buf[16];
			const double pt = val / m_page_dx.m_logical_dpi * PT_PER_INCH;
			swprintf_s(buf, FMT_POINT_UNIT, pt);
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			hdr = r_loader.GetString(L"str_size") + L": " + buf;
		}
		if constexpr (U == UNDO_OP::FONT_COLOR) {
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
	void MainPage::font_set_slider(Shape* s, const double val)
	{
		font_set_slider<U, S>(val);
		if constexpr (U == UNDO_OP::FONT_SIZE) {
			s->set_font_size(val);
		}
		if constexpr (U == UNDO_OP::FONT_COLOR) {
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
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	constexpr double TEXT_LINE_DELTA = 2;	// 行間の変分 (DPIs)
	constexpr wchar_t TITLE_PAGE[] = L"str_page";

	// 書体メニューの「行間」>「高さ」が選択された.
	void MainPage::mfi_text_line_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		static winrt::event_token slider0_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_sample();
		const double val0 = m_page_panel.m_text_line;
		slider0().Value(val0);
		text_set_slider<UNDO_OP::TEXT_LINE, 0>(val0);
		slider0().Visibility(VISIBLE);
		loaded_token = scp_sample_panel().Loaded(
			[this](auto, auto)
			{
				sample_panel_loaded();
				font_create_sample();
				sample_draw();
			}
		);
		slider0_token = slider0().ValueChanged(
			[this](auto, auto args)
			{
				text_set_slider<UNDO_OP::TEXT_LINE, 0>(m_sample_shape, args.NewValue());
			}
		);
		primary_token = cd_sample().PrimaryButtonClick(
			[this](auto, auto)
			{
				double sample_val;
				m_sample_shape->get_text_line(sample_val);
				undo_push_value<UNDO_OP::TEXT_LINE>(sample_val);
			}
		);
		closed_token = cd_sample().Closed(
			[this](auto, auto)
			{
				delete m_sample_shape;
#if defined(_DEBUG)
				debug_leak_cnt--;
#endif
				m_sample_shape = nullptr;
				slider0().Visibility(COLLAPSED);
				scp_sample_panel().Loaded(loaded_token);
				slider0().ValueChanged(slider0_token);
				cd_sample().PrimaryButtonClick(primary_token);
				cd_sample().Closed(closed_token);
				UnloadObject(cd_sample());
				page_draw();
			}
		);
		show_cd_sample(TITLE_PAGE);
	}

	//	書体メニューの「行間」>「狭める」が選択された.
	void MainPage::mfi_text_line_contract_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		auto val = m_page_panel.m_text_line - TEXT_LINE_DELTA;
		if (val <= FLT_MIN) {
			val = 0.0f;
		}
		if (m_page_panel.m_text_line != val) {
			undo_push_value<UNDO_OP::TEXT_LINE>(val);
		}
	}

	//	書体メニューの「行間」>「広げる」が選択された.
	void MainPage::mfi_text_line_expand_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		auto val = m_page_panel.m_text_line + TEXT_LINE_DELTA;
		if (m_page_panel.m_text_line != val) {
			undo_push_value<UNDO_OP::TEXT_LINE>(val);
		}
	}

	//	書体メニューの「余白」が選択された.
	void MainPage::mfi_text_margin_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		static winrt::event_token slider0_token;
		static winrt::event_token slider1_token;
		static winrt::event_token primary_token;
		static winrt::event_token loaded_token;
		static winrt::event_token closed_token;

		load_cd_sample();
		const double val0 = m_page_panel.m_text_mar.width;
		const double val1 = m_page_panel.m_text_mar.height;
		slider0().Value(val0);
		slider1().Value(val1);
		text_set_slider<UNDO_OP::TEXT_MARGIN, 0>(val0);
		text_set_slider<UNDO_OP::TEXT_MARGIN, 1>(val1);
		slider0().Visibility(VISIBLE);
		slider1().Visibility(VISIBLE);
		loaded_token = scp_sample_panel().Loaded(
			[this](auto, auto)
			{
				sample_panel_loaded();
				font_create_sample();
				sample_draw();
			}
		);
		slider0_token = slider0().ValueChanged(
			[this](auto, auto args)
			{
				text_set_slider<UNDO_OP::TEXT_MARGIN, 0>(m_sample_shape, args.NewValue());
			}
		);
		slider1_token = slider1().ValueChanged(
			[this](auto, auto args)
			{
				text_set_slider<UNDO_OP::TEXT_MARGIN, 1>(m_sample_shape, args.NewValue());
			}
		);
		primary_token = cd_sample().PrimaryButtonClick(
			[this](auto, auto)
			{
				D2D1_SIZE_F sample_val;
				m_sample_shape->get_text_margin(sample_val);
				undo_push_value<UNDO_OP::TEXT_MARGIN>(sample_val);
			}
		);
		closed_token = cd_sample().Closed(
			[this](auto, auto)
			{
				delete m_sample_shape;
#if defined(_DEBUG)
				debug_leak_cnt--;
#endif
				m_sample_shape = nullptr;
				slider0().Visibility(COLLAPSED);
				slider1().Visibility(COLLAPSED);
				scp_sample_panel().Loaded(loaded_token);
				slider0().ValueChanged(slider0_token);
				slider1().ValueChanged(slider1_token);
				cd_sample().PrimaryButtonClick(primary_token);
				cd_sample().Closed(closed_token);
				UnloadObject(cd_sample());
				page_draw();
			}
		);
		show_cd_sample(TITLE_PAGE);
	}

	// 書体メニューの「段落のそろえ」>「下よせ」が選択された.
	void MainPage::rmfi_text_align_bottom_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<UNDO_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
	}

	// 書体メニューの「文字列のそろえ」>「中央」が選択された.
	void MainPage::rmfi_text_align_center_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<UNDO_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	// 書体メニューの「文字列のそろえ」>「均等」が選択された.
	void MainPage::rmfi_text_align_justified_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<UNDO_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
	}

	// 書体メニューの「文字列のそろえ」>「左よせ」が選択された.
	void MainPage::rmfi_text_align_left_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<UNDO_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT_LEADING);
	}

	//	書体メニューの「段落のそろえ」>「中段」が選択された.
	void MainPage::rmfi_text_align_middle_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<UNDO_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	//	書体メニューの「文字列のそろえ」>「右よせ」が選択された.
	void MainPage::rmfi_text_align_right_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<UNDO_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT_TRAILING);
	}

	//	書体メニューの「段落のそろえ」>「上よせ」が選択された.
	void MainPage::rmfi_text_align_top_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		undo_push_value<UNDO_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	}

	//	書体メニューの「段落のそろえ」に印をつける.
	//	p_align	段落のそろえ
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
	template <UNDO_OP U, int S>
	void MainPage::text_set_slider(const double val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		const double dpi = m_page_dx.m_logical_dpi;
		const double g_len = m_sample_panel.m_grid_len + 1.0;
		double px;
		if constexpr (U == UNDO_OP::TEXT_MARGIN) {
			if constexpr (S == 0) {
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_text_mar_horzorz");
				px = val;
				wchar_t buf[16];
				conv_val_to_len(m_page_unit, px, dpi, g_len, buf, 16);
				hdr = hdr + L": " + buf;
			}
			if constexpr (S == 1) {
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_text_mar_vertert");
				px = val;
				wchar_t buf[16];
				conv_val_to_len(m_page_unit, px, dpi, g_len, buf, 16);
				hdr = hdr + L": " + buf;
			}
		}
		if constexpr (U == UNDO_OP::TEXT_LINE) {
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			hdr = r_loader.GetString(L"str_height");
			if (val > FLT_MIN) {
				//	行の高さの単位は DIPs (96dpi 固定) なので,
				//	これをピクセル単位に変換する.
				px = val * dpi / 96.0;
				wchar_t buf[16];
				conv_val_to_len(m_page_unit, px, dpi, g_len, buf, 16);
				hdr = hdr + L": " + buf;
			}
			else {
				hdr = hdr + L": " + r_loader.GetString(L"str_def_val");
				//goto SET;
			}
		}
		/*
		if (m_page_unit == LEN_UNIT::PIXEL) {
			wchar_t buf[16];
			swprintf_s(buf, FMT_PIXEL_UNIT, px);
			hdr = hdr + L": " + buf;
		}
		else if (m_page_unit == LEN_UNIT::INCH) {
			wchar_t buf[16];
			swprintf_s(buf, FMT_INCH_UNIT, px / dpi);
			hdr = hdr + L": " + buf;
		}
		else if (m_page_unit == LEN_UNIT::MILLI) {
			wchar_t buf[16];
			swprintf_s(buf, FMT_MILLI_UNIT, px * MM_PER_INCH / dpi);
			hdr = hdr + L": " + buf;
		}
		else if (m_page_unit == LEN_UNIT::POINT) {
			wchar_t buf[16];
			swprintf_s(buf, FMT_POINT_UNIT, px * PT_PER_INCH / dpi);
			hdr = hdr + L": " + buf;
		}
		else if (m_page_unit == LEN_UNIT::GRID) {
			wchar_t buf[16];
			swprintf_s(buf, FMT_GRID_UNIT, px / (m_sample_panel.m_grid_len + 1.0));
			hdr = hdr + L": " + buf;
		}
		wchar_t buf[16];
		conv_val_to_len(m_page_unit, dpi,g_len, buf, 16);
		hdr = hdr + L": " + buf;
	SET:
	*/
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
	void MainPage::text_set_slider(Shape* s, const double val)
	{
		text_set_slider<U, S>(val);
		if constexpr (U == UNDO_OP::TEXT_LINE) {
			s->set_text_line(val);
		}
		if constexpr (U == UNDO_OP::TEXT_MARGIN) {
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
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

}