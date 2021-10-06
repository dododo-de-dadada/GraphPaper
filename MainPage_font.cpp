//-------------------------------
// MainPage_font.cpp
// 書体と文字列の配置
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

	constexpr wchar_t DLG_TITLE[] = L"str_font";

	// 書体の伸縮の配列
	constexpr std::underlying_type_t<DWRITE_FONT_STRETCH> FONT_STRETCH[] = {
		DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_UNDEFINED,
		DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_CONDENSED,
		DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_CONDENSED,
		DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_CONDENSED,
		DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_CONDENSED,
		DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL,
		DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_EXPANDED,
		DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXPANDED,
		DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_EXPANDED,
		DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_EXPANDED,
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

	//constexpr float SLIDER_STEP = 0.5f;

	// 書体メニューの「色」が選択された.
	IAsyncAction MainPage::font_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_sample_sheet.set_attr_to(&m_main_sheet);

		D2D1_COLOR_F f_color;
		m_sample_sheet.get_font_color(f_color);
		const float val0 = f_color.r * COLOR_MAX;
		const float val1 = f_color.g * COLOR_MAX;
		const float val2 = f_color.b * COLOR_MAX;
		const float val3 = f_color.a * COLOR_MAX;
		sample_slider_0().Maximum(255.0);
		sample_slider_0().TickFrequency(1.0);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(val0);
		font_slider_set_header<UNDO_OP::FONT_COLOR, 0>(val0);
		sample_slider_1().Maximum(255.0);
		sample_slider_1().TickFrequency(1.0);
		sample_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_1().Value(val1);
		font_slider_set_header<UNDO_OP::FONT_COLOR, 1>(val1);
		sample_slider_2().Maximum(255.0);
		sample_slider_2().TickFrequency(1.0);
		sample_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_2().Value(val2);
		font_slider_set_header<UNDO_OP::FONT_COLOR, 2>(val2);
		sample_slider_3().Maximum(255.0);
		sample_slider_3().TickFrequency(1.0);
		sample_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_3().Value(val3);
		font_slider_set_header<UNDO_OP::FONT_COLOR, 3>(val3);

		sample_slider_0().Visibility(UI_VISIBLE);
		sample_slider_1().Visibility(UI_VISIBLE);
		sample_slider_2().Visibility(UI_VISIBLE);
		sample_slider_3().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::font_slider_value_changed<UNDO_OP::FONT_COLOR, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::font_slider_value_changed<UNDO_OP::FONT_COLOR, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::font_slider_value_changed<UNDO_OP::FONT_COLOR, 2> });
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::font_slider_value_changed<UNDO_OP::FONT_COLOR, 3> });
		m_sample_type = SAMPLE_TYPE::FONT;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F sample_value;
			//m_sample_shape->get_font_color(sample_value);
			m_sample_sheet.m_shape_list.back()->get_font_color(sample_value);
			if (ustack_push_set<UNDO_OP::FONT_COLOR>(sample_value)) {
				ustack_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		//delete m_sample_shape;
		delete m_sample_sheet.m_shape_list.back();
		m_sample_sheet.m_shape_list.clear();
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		//m_sample_shape = nullptr;
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_1().Visibility(UI_COLLAPSED);
		sample_slider_2().Visibility(UI_COLLAPSED);
		sample_slider_3().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sample_slider_2().ValueChanged(slider_2_token);
		sample_slider_3().ValueChanged(slider_3_token);
		sheet_draw();

	}

	// 書体メニューの「書体名」が選択された.
	IAsyncAction MainPage::font_family_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_sample_sheet.set_attr_to(&m_main_sheet);
		for (uint32_t i = 0; wchar_t* name = ShapeText::get_available_font(i); i++) {
			auto item = box_value(winrt::hstring(name));
			lv_sample_list().Items().Append(item);
		}
		for (uint32_t i = 0; i < lv_sample_list().Items().Size(); i++) {
			IInspectable item[1];
			lv_sample_list().Items().GetMany(i, item);
			auto name = unbox_value<winrt::hstring>(item[0]).c_str();
			wchar_t* f_family;
			m_main_sheet.get_font_family(f_family);
			if (wcscmp(name, f_family) == 0) {
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
				//m_sample_shape->set_font_family(ShapeText::get_available_font(i));
				m_sample_sheet.m_shape_list.back()->set_font_family(ShapeText::get_available_font(i));
				if (scp_sample_panel().IsLoaded()) {
					sample_draw();
				}
			}
		);
		lv_sample_list().Visibility(UI_VISIBLE);
		m_sample_type = SAMPLE_TYPE::FONT;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			wchar_t* sample_value;
			//m_sample_shape->get_font_family(sample_value);
			m_sample_sheet.m_shape_list.back()->get_font_family(sample_value);
			if (ustack_push_set<UNDO_OP::FONT_FAMILY>(sample_value)) {
				ustack_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		//delete m_sample_shape;
		delete m_sample_sheet.m_shape_list.back();
		m_sample_sheet.m_shape_list.clear();
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		//m_sample_shape = nullptr;
		lv_sample_list().Loaded(loaded_token);
		lv_sample_list().SelectionChanged(changed_token);
		lv_sample_list().Visibility(UI_COLLAPSED);
		lv_sample_list().Items().Clear();
		sheet_draw();
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S> void MainPage::font_slider_set_header(const float value)
	{
		winrt::hstring text;

		if constexpr (U == UNDO_OP::FONT_SIZE) {
			wchar_t buf[32];
			float g_base;
			m_sample_sheet.get_grid_base(g_base);
			conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, value, m_sample_dx.m_logical_dpi, g_base + 1.0f, buf);
			text = ResourceLoader::GetForCurrentView().GetString(L"str_size") + L": " + buf;
		}
		if constexpr (U == UNDO_OP::FONT_COLOR) {
			constexpr wchar_t* HEADER[]{ L"str_color_r", L"str_color_g",L"str_color_b", L"str_opacity" };
			//if constexpr (S == 0) {
				wchar_t buf[32];
				// 色成分の値を文字列に変換する.
				conv_col_to_str(m_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(HEADER[S]) + L": " + buf;
			//}
			//if constexpr (S == 1) {
			//	wchar_t buf[32];
			//	// 色成分の値を文字列に変換する.
			//	conv_col_to_str(m_color_code, value, buf);
			//	text = ResourceLoader::GetForCurrentView().GetString(L"str_color_g") + L": " + buf;
			//}
			//if constexpr (S == 2) {
			//	wchar_t buf[32];
			//	// 色成分の値を文字列に変換する.
			//	conv_col_to_str(m_color_code, value, buf);
			//	text = ResourceLoader::GetForCurrentView().GetString(L"str_color_b") + L": " + buf;
			//}
			//if constexpr (S == 3) {
			//	wchar_t buf[32];
			//	// 色成分の値を文字列に変換する.
			//	conv_col_to_str(m_color_code, value, buf);
			//	text = ResourceLoader::GetForCurrentView().GetString(L"str_opacity") + L": " + buf;
			//}
		}
		if constexpr (S == 0) {
			sample_slider_0().Header(box_value(text));
		}
		if constexpr (S == 1) {
			sample_slider_1().Header(box_value(text));
		}
		if constexpr (S == 2) {
			sample_slider_2().Header(box_value(text));
		}
		if constexpr (S == 3) {
			sample_slider_3().Header(box_value(text));
		}
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S> void MainPage::font_slider_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::FONT_SIZE) {
			if constexpr (S == 0) {
				const auto value = static_cast<float>(args.NewValue());
				font_slider_set_header<U, S>(value);
				//m_sample_shape->set_font_size(value);
				m_sample_sheet.m_shape_list.back()->set_font_size(value);
			}
		}
		else if constexpr (U == UNDO_OP::FONT_COLOR) {
			const auto value = static_cast<float>(args.NewValue());
			D2D1_COLOR_F f_color;
			//m_sample_shape->get_font_color(f_color);
			m_sample_sheet.m_shape_list.back()->get_font_color(f_color);
			if constexpr (S == 0) {
				font_slider_set_header<U, S>(value);
				f_color.r = static_cast<FLOAT>(value / COLOR_MAX);
			}
			else if constexpr (S == 1) {
				font_slider_set_header<U, S>(value);
				f_color.g = static_cast<FLOAT>(value / COLOR_MAX);
			}
			else if constexpr (S == 2) {
				font_slider_set_header<U, S>(value);
				f_color.b = static_cast<FLOAT>(value / COLOR_MAX);
			}
			else if constexpr (S == 3) {
				font_slider_set_header<U, S>(value);
				f_color.a = static_cast<FLOAT>(value / COLOR_MAX);
			}
			//m_sample_shape->set_font_color(f_color);
			m_sample_sheet.m_shape_list.back()->set_font_color(f_color);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// 書体メニューの「大きさ」が選択された.
	IAsyncAction MainPage::font_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_sample_sheet.set_attr_to(&m_main_sheet);
		float f_size;
		m_sample_sheet.get_font_size(f_size);

		sample_slider_0().Maximum(MAX_VALUE);
		sample_slider_0().TickFrequency(TICK_FREQ);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(f_size);
		font_slider_set_header<UNDO_OP::FONT_SIZE, 0>(f_size);
		sample_slider_0().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::font_slider_value_changed<UNDO_OP::FONT_SIZE, 0> });
		m_sample_type = SAMPLE_TYPE::FONT;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_value;
			//m_sample_shape->get_font_size(sample_value);
			m_sample_sheet.m_shape_list.back()->get_font_size(sample_value);
			if (ustack_push_set<UNDO_OP::FONT_SIZE>(sample_value)) {
				ustack_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		//delete m_sample_shape;
		delete m_sample_sheet.m_shape_list.back();
		m_sample_sheet.m_shape_list.clear();
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		//m_sample_shape = nullptr;
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sheet_draw();
	}

	// 書体メニューの「伸縮」が選択された.
	IAsyncAction MainPage::font_stretch_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_sample_sheet.set_attr_to(&m_main_sheet);
		for (uint32_t i = 0; FONT_STRETCH_NAME[i] != nullptr; i++) {
			auto item = box_value(ResourceLoader::GetForCurrentView().GetString(FONT_STRETCH_NAME[i]));
			lv_sample_list().Items().Append(item);
		}
		lv_sample_list().SelectedIndex(-1);
		const auto k = lv_sample_list().Items().Size();
		for (uint32_t i = 0; i < k; i++) {
			DWRITE_FONT_STRETCH f_stretch;
			m_main_sheet.get_font_stretch(f_stretch);
			if (FONT_STRETCH[i] == f_stretch) {
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
				//m_sample_shape->set_font_stretch(static_cast<DWRITE_FONT_STRETCH>(FONT_STRETCH[i]));
				m_sample_sheet.m_shape_list.back()->set_font_stretch(static_cast<DWRITE_FONT_STRETCH>(FONT_STRETCH[i]));
				if (scp_sample_panel().IsLoaded()) {
					sample_draw();
				}
			}
		);
		lv_sample_list().Visibility(UI_VISIBLE);
		m_sample_type = SAMPLE_TYPE::FONT;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			DWRITE_FONT_STRETCH sample_value;
			//m_sample_shape->get_font_stretch(sample_value);
			m_sample_sheet.m_shape_list.back()->get_font_stretch(sample_value);
			if (ustack_push_set<UNDO_OP::FONT_STRETCH>(sample_value)) {
				ustack_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		//delete m_sample_shape;
		delete m_sample_sheet.m_shape_list.back();
		m_sample_sheet.m_shape_list.clear();
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		//m_sample_shape = nullptr;
		lv_sample_list().Loaded(loaded_token);
		lv_sample_list().SelectionChanged(changed_token);
		lv_sample_list().Visibility(UI_COLLAPSED);
		lv_sample_list().Items().Clear();
		sheet_draw();
	}

	// 書体メニューの「字体」に印をつける.
	// f_style	書体の字体
	void MainPage::font_style_is_checked(const DWRITE_FONT_STYLE f_style)
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
		if (ustack_push_set<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE_ITALIC)) {
			ustack_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// 書体メニューの「標準」が選択された.
	void MainPage::font_style_normal_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (ustack_push_set<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE_NORMAL)) {
			ustack_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// 書体メニューの「斜体」が選択された.
	void MainPage::font_style_oblique_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (ustack_push_set<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE_OBLIQUE)) {
			ustack_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// 書体メニューの「太さ」が選択された.
	IAsyncAction MainPage::font_weight_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_sample_sheet.set_attr_to(&m_main_sheet);
		for (uint32_t i = 0; FONT_WEIGHT_NAME[i] != nullptr; i++) {
			auto item = box_value(ResourceLoader::GetForCurrentView().GetString(FONT_WEIGHT_NAME[i]));
			lv_sample_list().Items().Append(item);
		}
		lv_sample_list().SelectedIndex(-1);
		const auto k = lv_sample_list().Items().Size();
		for (uint32_t i = 0; i < k; i++) {
			DWRITE_FONT_WEIGHT f_weight;
			m_main_sheet.get_font_weight(f_weight);
			if (FONT_WEIGHTS[i] == f_weight) {
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
				//m_sample_shape->set_font_weight(static_cast<DWRITE_FONT_WEIGHT>(FONT_WEIGHTS[i]));
				m_sample_sheet.m_shape_list.back()->set_font_weight(static_cast<DWRITE_FONT_WEIGHT>(FONT_WEIGHTS[i]));
				if (scp_sample_panel().IsLoaded()) {
					sample_draw();
				}
			}
		);
		lv_sample_list().Visibility(UI_VISIBLE);
		m_sample_type = SAMPLE_TYPE::FONT;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			DWRITE_FONT_WEIGHT sample_value;
			//m_sample_shape->get_font_weight(sample_value);
			m_sample_sheet.m_shape_list.back()->get_font_weight(sample_value);
			if (ustack_push_set<UNDO_OP::FONT_WEIGHT>(sample_value)) {
				ustack_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		//delete m_sample_shape;
		delete m_sample_sheet.m_shape_list.back();
		m_sample_sheet.m_shape_list.clear();
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		//m_sample_shape = nullptr;
		lv_sample_list().Loaded(loaded_token);
		lv_sample_list().SelectionChanged(changed_token);
		lv_sample_list().Visibility(UI_COLLAPSED);
		lv_sample_list().Items().Clear();
		sheet_draw();
	}

}