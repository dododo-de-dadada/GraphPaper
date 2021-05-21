//-------------------------------
// MainPage_font.cpp
// 書体と文字列の配置
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::IInspectable;

	constexpr double SLIDER_STEP = 0.5;

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

	// 書体の太さの文字列配列
	constexpr wchar_t* FONT_WEIGHT_NAME[] = {
		L"str_font_weight_thin",
		L"str_font_weight_extra_light",
		L"str_font_weight_light",
		L"str_font_weight_semi_light",
		L"str_font_weight_normal",
		L"str_font_weight_medium",
		L"str_font_weight_demi_bold",
		L"str_font_weight_bold",
		L"str_font_weight_extra_bold",
		L"str_font_weight_black",
		L"str_font_weight_extra_black",
		nullptr
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

	// 書体の伸縮の文字列配列
	constexpr wchar_t* FONT_STRETCH_NAME[] = {
		L"str_font_stretch_undefined",
		L"str_font_stretch_ultra_condensed",
		L"str_font_stretch_extra_condensed",
		L"str_font_stretch_condensed",
		L"str_font_stretch_semi_condensed",
		L"str_font_stretch_normal",
		L"str_font_stretch_semi_expanded",
		L"str_font_stretch_expanded",
		L"str_font_stretch_extra_expanded",
		L"str_font_stretch_ultra_expanded",
		nullptr
	};

	constexpr wchar_t TITLE_FONT[] = L"str_font";

	// 書体メニューの「色」が選択された.
	IAsyncAction MainPage::font_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		const double val0 = m_sample_sheet.m_font_color.r * COLOR_MAX;
		const double val1 = m_sample_sheet.m_font_color.g * COLOR_MAX;
		const double val2 = m_sample_sheet.m_font_color.b * COLOR_MAX;
		const double val3 = m_sample_sheet.m_font_color.a * COLOR_MAX;
		sample_slider_0().Value(val0);
		sample_slider_1().Value(val1);
		sample_slider_2().Value(val2);
		sample_slider_3().Value(val3);
		font_set_slider_header<UNDO_OP::FONT_COLOR, 0>(val0);
		font_set_slider_header<UNDO_OP::FONT_COLOR, 1>(val1);
		font_set_slider_header<UNDO_OP::FONT_COLOR, 2>(val2);
		font_set_slider_header<UNDO_OP::FONT_COLOR, 3>(val3);
		sample_slider_0().Visibility(VISIBLE);
		sample_slider_1().Visibility(VISIBLE);
		sample_slider_2().Visibility(VISIBLE);
		sample_slider_3().Visibility(VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::font_set_slider<UNDO_OP::FONT_COLOR, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::font_set_slider<UNDO_OP::FONT_COLOR, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::font_set_slider<UNDO_OP::FONT_COLOR, 2> });
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::font_set_slider<UNDO_OP::FONT_COLOR, 3> });
		m_sample_type = SAMP_TYPE::FONT;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_FONT)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F sample_value;
			m_sample_shape->get_font_color(sample_value);
			undo_push_set<UNDO_OP::FONT_COLOR>(sample_value);
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_1().Visibility(COLLAPSED);
		sample_slider_2().Visibility(COLLAPSED);
		sample_slider_3().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sample_slider_2().ValueChanged(slider_2_token);
		sample_slider_3().ValueChanged(slider_3_token);
		sheet_draw();

	}

	// 書体メニューの「書体名」が選択された.
	IAsyncAction MainPage::font_family_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		for (uint32_t i = 0; wchar_t* name = ShapeText::get_available_font(i); i++) {
			auto item = box_value(winrt::hstring(name));
			lv_sample_list().Items().Append(item);
		}
		for (uint32_t i = 0; i < lv_sample_list().Items().Size(); i++) {
			IInspectable item[1];
			lv_sample_list().Items().GetMany(i, item);
			auto name = unbox_value<winrt::hstring>(item[0]).c_str();
			if (wcscmp(name, m_sheet_main.m_font_family) == 0) {
				// 書体名が同じ場合,
				// その書体をリストビューの選択済み項目に格納する.
				lv_sample_list().SelectedItem(item[0]);
				lv_sample_list().ScrollIntoView(item[0]);
				break;
			}
		}
		const auto loaded_token = lv_sample_list().Loaded({ this, &MainPage::sample_list_loaded });
		const auto changed_token = lv_sample_list().SelectionChanged(
			[this](auto, auto)
			{
				auto i = lv_sample_list().SelectedIndex();
				m_sample_shape->set_font_family(ShapeText::get_available_font(i));
				if (scp_sample_panel().IsLoaded()) {
					sample_draw();
				}
			}
		);
		lv_sample_list().Visibility(VISIBLE);
		m_sample_type = SAMP_TYPE::FONT;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_FONT)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			wchar_t* sample_value;
			m_sample_shape->get_font_family(sample_value);
			undo_push_set<UNDO_OP::FONT_FAMILY>(sample_value);
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		lv_sample_list().Loaded(loaded_token);
		lv_sample_list().SelectionChanged(changed_token);
		lv_sample_list().Visibility(COLLAPSED);
		lv_sample_list().Items().Clear();
		sheet_draw();
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S>
	void MainPage::font_set_slider_header(const double value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		if constexpr (U == UNDO_OP::FONT_SIZE) {
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), value, sample_dx().m_logical_dpi, m_sample_sheet.m_grid_base + 1.0, buf);
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			hdr = r_loader.GetString(L"str_size") + L": " + buf;
		}
		if constexpr (U == UNDO_OP::FONT_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				hdr = ResourceLoader::GetForCurrentView().GetString(L"str_col_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				hdr = ResourceLoader::GetForCurrentView().GetString(L"str_col_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				hdr = ResourceLoader::GetForCurrentView().GetString(L"str_col_b") + L": " + buf;
			}
			if constexpr (S == 3) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(color_code(), value, buf);
				hdr = ResourceLoader::GetForCurrentView().GetString(L"str_opacity") + L": " + buf;
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
	void MainPage::font_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* const s = m_sample_shape;
		const auto value = args.NewValue();
		font_set_slider_header<U, S>(value);
		if constexpr (U == UNDO_OP::FONT_SIZE) {
			s->set_font_size(value);
		}
		if constexpr (U == UNDO_OP::FONT_COLOR) {
			D2D1_COLOR_F color;
			s->get_font_color(color);
			if constexpr (S == 0) {
				color.r = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 1) {
				color.g = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 2) {
				color.b = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 3) {
				color.a = static_cast<FLOAT>(value / COLOR_MAX);
			}
			s->set_font_color(color);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// 書体メニューの「大きさ」が選択された.
	IAsyncAction MainPage::font_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		const double val0 = m_sample_sheet.m_font_size;
		sample_slider_0().Value(val0);
		font_set_slider_header<UNDO_OP::FONT_SIZE, 0>(val0);
		sample_slider_0().Visibility(VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::font_set_slider<UNDO_OP::FONT_SIZE, 0> });
		m_sample_type = SAMP_TYPE::FONT;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_FONT)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			double sample_value;
			m_sample_shape->get_font_size(sample_value);
			undo_push_set<UNDO_OP::FONT_SIZE>(sample_value);
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sheet_draw();
	}

	// 書体メニューの「伸縮」が選択された.
	IAsyncAction MainPage::font_stretch_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		for (uint32_t i = 0; FONT_STRETCH_NAME[i] != nullptr; i++) {
			auto item = box_value(ResourceLoader::GetForCurrentView().GetString(FONT_STRETCH_NAME[i]));
			lv_sample_list().Items().Append(item);
		}
		lv_sample_list().SelectedIndex(-1);
		const auto k = lv_sample_list().Items().Size();
		for (uint32_t i = 0; i < k; i++) {
			if (FONT_STRETCH[i] == m_sheet_main.m_font_stretch) {
				lv_sample_list().SelectedIndex(i);
				IInspectable item[1];
				lv_sample_list().Items().GetMany(i, item);
				lv_sample_list().ScrollIntoView(item[0]);
				break;
			}
		}
		const auto loaded_token = lv_sample_list().Loaded({ this, &MainPage::sample_list_loaded });
		const auto changed_token = lv_sample_list().SelectionChanged(
			[this](auto, auto args) {
				uint32_t i = lv_sample_list().SelectedIndex();
				m_sample_shape->set_font_stretch(static_cast<DWRITE_FONT_STRETCH>(FONT_STRETCH[i]));
				if (scp_sample_panel().IsLoaded()) {
					sample_draw();
				}
			}
		);
		lv_sample_list().Visibility(VISIBLE);
		m_sample_type = SAMP_TYPE::FONT;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_FONT)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			DWRITE_FONT_STRETCH sample_value;
			m_sample_shape->get_font_stretch(sample_value);
			undo_push_set<UNDO_OP::FONT_STRETCH>(sample_value);
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		lv_sample_list().Loaded(loaded_token);
		lv_sample_list().SelectionChanged(changed_token);
		lv_sample_list().Visibility(COLLAPSED);
		lv_sample_list().Items().Clear();
		sheet_draw();
	}

	// 書体メニューの「字体」に印をつける.
	// f_style	書体の字体
	void MainPage::font_style_check_menu(const DWRITE_FONT_STYLE f_style)
	{
		rmfi_font_style_italic().IsChecked(f_style == DWRITE_FONT_STYLE_ITALIC);
		rmfi_font_style_normal().IsChecked(f_style == DWRITE_FONT_STYLE_NORMAL);
		rmfi_font_style_oblique().IsChecked(f_style == DWRITE_FONT_STYLE_OBLIQUE);

		rmfi_font_style_italic_2().IsChecked(f_style == DWRITE_FONT_STYLE_ITALIC);
		rmfi_font_style_normal_2().IsChecked(f_style == DWRITE_FONT_STYLE_NORMAL);
		rmfi_font_style_oblique_2().IsChecked(f_style == DWRITE_FONT_STYLE_OBLIQUE);
	}

	// 書体メニューの「イタリック体」が選択された.
	void MainPage::font_style_italic_click(IInspectable const&, RoutedEventArgs const&)
	{
		undo_push_set<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE_ITALIC);
	}

	// 書体メニューの「標準」が選択された.
	void MainPage::font_style_normal_click(IInspectable const&, RoutedEventArgs const&)
	{
		undo_push_set<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE_NORMAL);
	}

	// 書体メニューの「斜体」が選択された.
	void MainPage::font_style_oblique_click(IInspectable const&, RoutedEventArgs const&)
	{
		undo_push_set<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE_OBLIQUE);
	}

	// 書体メニューの「太さ」が選択された.
	IAsyncAction MainPage::font_weight_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		for (uint32_t i = 0; FONT_WEIGHT_NAME[i] != nullptr; i++) {
			auto item = box_value(ResourceLoader::GetForCurrentView().GetString(FONT_WEIGHT_NAME[i]));
			lv_sample_list().Items().Append(item);
		}
		lv_sample_list().SelectedIndex(-1);
		const auto k = lv_sample_list().Items().Size();
		for (uint32_t i = 0; i < k; i++) {
			if (FONT_WEIGHTS[i] == m_sheet_main.m_font_weight) {
				lv_sample_list().SelectedIndex(i);
				IInspectable item[1];
				lv_sample_list().Items().GetMany(i, item);
				lv_sample_list().ScrollIntoView(item[0]);
				break;
			}
		}
		const auto loaded_token = lv_sample_list().Loaded({ this, &MainPage::sample_list_loaded });
		const auto changed_token = lv_sample_list().SelectionChanged(
			[this](auto, auto args) {
				uint32_t i = lv_sample_list().SelectedIndex();
				m_sample_shape->set_font_weight(static_cast<DWRITE_FONT_WEIGHT>(FONT_WEIGHTS[i]));
				if (scp_sample_panel().IsLoaded()) {
					sample_draw();
				}
			}
		);
		lv_sample_list().Visibility(VISIBLE);
		m_sample_type = SAMP_TYPE::FONT;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_FONT)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			DWRITE_FONT_WEIGHT sample_value;
			m_sample_shape->get_font_weight(sample_value);
			undo_push_set<UNDO_OP::FONT_WEIGHT>(sample_value);
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		lv_sample_list().Loaded(loaded_token);
		lv_sample_list().SelectionChanged(changed_token);
		lv_sample_list().Visibility(COLLAPSED);
		lv_sample_list().Items().Clear();
		sheet_draw();
	}

	// 書体メニューの「段落のそろえ」>「下よせ」が選択された.
	void MainPage::text_align_p_bot_click(IInspectable const&, RoutedEventArgs const&)
	{
		undo_push_set<UNDO_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
	}

	// 書体メニューの「段落のそろえ」に印をつける.
	// p_align	段落のそろえ
	void MainPage::text_align_p_check_menu(const DWRITE_PARAGRAPH_ALIGNMENT p_align)
	{
		rmfi_text_align_top().IsChecked(p_align == DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		rmfi_text_align_bot().IsChecked(p_align == DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		rmfi_text_align_mid().IsChecked(p_align == DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

		rmfi_text_align_top_2().IsChecked(p_align == DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		rmfi_text_align_bot_2().IsChecked(p_align == DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		rmfi_text_align_mid_2().IsChecked(p_align == DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	// 書体メニューの「段落のそろえ」>「中段」が選択された.
	void MainPage::text_align_p_mid_click(IInspectable const&, RoutedEventArgs const&)
	{
		undo_push_set<UNDO_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	// 書体メニューの「段落のそろえ」>「上よせ」が選択された.
	void MainPage::text_align_p_top_click(IInspectable const&, RoutedEventArgs const&)
	{
		undo_push_set<UNDO_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	}

	// 書体メニューの「文字列のそろえ」>「中央」が選択された.
	void MainPage::text_align_t_center_click(IInspectable const&, RoutedEventArgs const&)
	{
		undo_push_set<UNDO_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	// 書体メニューの「文字列のそろえ」に印をつける.
	// t_align	文字列のそろえ
	void MainPage::text_align_t_check_menu(const DWRITE_TEXT_ALIGNMENT t_align)
	{
		rmfi_text_align_left().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_LEADING);
		rmfi_text_align_right().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_TRAILING);
		rmfi_text_align_center().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_CENTER);
		rmfi_text_align_just().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_JUSTIFIED);

		rmfi_text_align_left_2().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_LEADING);
		rmfi_text_align_right_2().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_TRAILING);
		rmfi_text_align_center_2().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_CENTER);
		rmfi_text_align_just_2().IsChecked(t_align == DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
	}

	// 書体メニューの「文字列のそろえ」>「均等」が選択された.
	void MainPage::text_align_t_just_click(IInspectable const&, RoutedEventArgs const&)
	{
		undo_push_set<UNDO_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
	}

	// 書体メニューの「文字列のそろえ」>「左よせ」が選択された.
	void MainPage::text_align_t_left_click(IInspectable const&, RoutedEventArgs const&)
	{
		undo_push_set<UNDO_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT_LEADING);
	}

	// 書体メニューの「文字列のそろえ」>「右よせ」が選択された.
	void MainPage::text_align_t_right_click(IInspectable const&, RoutedEventArgs const&)
	{
		undo_push_set<UNDO_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT_TRAILING);
	}

	constexpr double TEXT_LINE_DELTA = 2;	// 行の高さの変分 (DPIs)

	// 書体メニューの「大きさを合わせる」が選択された.
	void MainPage::text_bbox_adjust_click(IInspectable const&, RoutedEventArgs const&)
	{
		auto flag = false;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			else if (s->is_selected() != true) {
				continue;
			}
			else if (typeid(*s) != typeid(ShapeText)) {
				continue;
			}
			auto u = new UndoAnchor(s, ANCH_TYPE::ANCH_SE);
			const auto size = sheet_size_max();
			if (static_cast<ShapeText*>(s)->adjust_bbox({ size, size })) {
				sheet_update_bbox(s);
				m_stack_undo.push_back(u);
				flag = true;
			}
			else {
				delete u;
			}
		}
		if (flag) {
			undo_push_null();
			sheet_panle_size();
			sheet_draw();
		}
	}

	// 書体メニューの「行の高さ」>「高さ」が選択された.
	IAsyncAction MainPage::text_line_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		const double val0 = m_sample_sheet.m_text_line / SLIDER_STEP;
		sample_slider_0().Value(val0);
		text_set_slider_header<UNDO_OP::TEXT_LINE, 0>(val0);
		sample_slider_0().Visibility(VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::text_set_slider<UNDO_OP::TEXT_LINE, 0> });
		m_sample_type = SAMP_TYPE::FONT;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_FONT)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			double sample_value;
			m_sample_shape->get_text_line(sample_value);
			undo_push_set<UNDO_OP::TEXT_LINE>(sample_value);
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sheet_draw();
	}

	// 書体メニューの「行の高さ」>「狭める」が選択された.
	void MainPage::text_line_con_click(IInspectable const&, RoutedEventArgs const&)
	{
		auto value = m_sheet_main.m_text_line - TEXT_LINE_DELTA;
		if (value <= FLT_MIN) {
			value = 0.0f;
		}
		if (m_sheet_main.m_text_line != value) {
			undo_push_set<UNDO_OP::TEXT_LINE>(value);
		}
	}

	// 書体メニューの「行の高さ」>「広げる」が選択された.
	void MainPage::text_line_exp_click(IInspectable const&, RoutedEventArgs const&)
	{
		auto value = m_sheet_main.m_text_line + TEXT_LINE_DELTA;
		if (m_sheet_main.m_text_line != value) {
			undo_push_set<UNDO_OP::TEXT_LINE>(value);
		}
	}

	// 書体メニューの「余白」が選択された.
	IAsyncAction MainPage::text_margin_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		const double val0 = m_sample_sheet.m_text_margin.width / SLIDER_STEP;
		const double val1 = m_sample_sheet.m_text_margin.height / SLIDER_STEP;
		sample_slider_0().Value(val0);
		sample_slider_1().Value(val1);
		text_set_slider_header<UNDO_OP::TEXT_MARGIN, 0>(val0);
		text_set_slider_header<UNDO_OP::TEXT_MARGIN, 1>(val1);
		sample_slider_0().Visibility(VISIBLE);
		sample_slider_1().Visibility(VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::text_set_slider<UNDO_OP::TEXT_MARGIN, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::text_set_slider<UNDO_OP::TEXT_MARGIN, 1> });
		m_sample_type = SAMP_TYPE::FONT;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_FONT)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_SIZE_F sample_value;
			m_sample_shape->get_text_margin(sample_value);
			undo_push_set<UNDO_OP::TEXT_MARGIN>(sample_value);
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_1().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sheet_draw();
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S>
	void MainPage::text_set_slider_header(const double value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring hdr;

		const double dpi = sheet_dx().m_logical_dpi;
		const double g_len = m_sample_sheet.m_grid_base + 1.0;
		if constexpr (U == UNDO_OP::TEXT_MARGIN) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), value * SLIDER_STEP, dpi, g_len, buf);
				hdr = ResourceLoader::GetForCurrentView().GetString(L"str_text_mar_horzorz") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), value * SLIDER_STEP, dpi, g_len, buf);
				hdr = ResourceLoader::GetForCurrentView().GetString(L"str_text_mar_vertert") + L": " + buf;
			}
		}
		if constexpr (U == UNDO_OP::TEXT_LINE) {
			if (value > FLT_MIN) {
				// 行の高さの単位は DIPs (96dpi 固定) なので,
				// これをピクセル単位に変換する.
				wchar_t buf[32];
				conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), value * SLIDER_STEP * dpi / 96.0, dpi, g_len, buf);
				hdr = ResourceLoader::GetForCurrentView().GetString(L"str_height") + L": " + buf;
			}
			else {
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_height") + L": " + r_loader.GetString(L"str_def_val");
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
	void MainPage::text_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		const double dpi = sheet_dx().m_logical_dpi;
		Shape* const s = m_sample_shape;
		const double value = args.NewValue();
		text_set_slider_header<U, S>(value);
		if constexpr (U == UNDO_OP::TEXT_LINE) {
			s->set_text_line(value * SLIDER_STEP * dpi / 96.0);
		}
		if constexpr (U == UNDO_OP::TEXT_MARGIN) {
			D2D1_SIZE_F margin;
			s->get_text_margin(margin);
			if constexpr (S == 0) {
				margin.width = static_cast<FLOAT>(value * SLIDER_STEP);
			}
			if constexpr (S == 1) {
				margin.height = static_cast<FLOAT>(value * SLIDER_STEP);
			}
			s->set_text_margin(margin);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

}