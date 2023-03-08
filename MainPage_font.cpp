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
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

	//---------------------------------
	// 書体の幅の配列
	//---------------------------------
	constexpr std::underlying_type_t<DWRITE_FONT_STRETCH> FONT_STRETCH[] = {
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

	//---------------------------------
	// 書体の幅の文字列配列
	//---------------------------------
	constexpr wchar_t* FONT_STRETCH_NAME[] = {
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

	//---------------------------------
	// 書体の太さの文字列配列
	//---------------------------------
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

	//---------------------------------
	// 書体の太さの配列
	//---------------------------------
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

	//---------------------------------
	// 設定の図形を作成する.
	//---------------------------------
	static void font_create_sample_shape(
		const float p_width, const float p_height, ShapePage& page);

	// 見本の図形を作成する.
	// p_width	見本を表示するパネルの幅
	// p_height	見本を表示するパネルの高さ
	// page	見本を表示するシート
	static void font_create_sample_shape(
		const float p_width, const float p_height, ShapePage& page)
	{
		const auto padd_w = p_width * 0.125;
		const auto padd_h = p_height * 0.125;
		const D2D1_POINT_2F start{
			static_cast<FLOAT>(padd_w), static_cast<FLOAT>(padd_h)
		};
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(p_width - 2.0 * padd_w), static_cast<FLOAT>(p_width - 2.0 * padd_h)
		};
		const auto pangram = ResourceLoader::GetForCurrentView().GetString(L"str_pangram");
		const wchar_t* text = nullptr;
		if (pangram.empty()) {
			text = L"The quick brown fox jumps over a lazy dog.";
		}
		else {
			text = pangram.c_str();
		}
		page.m_shape_list.push_back(new ShapeText(start, pos, wchar_cpy(text), &page));
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
	}

	//---------------------------------
	// 書体メニューの「色」が選択された.
	//---------------------------------
	IAsyncAction MainPage::font_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto TICK_FREQ = 1.0;
		m_dialog_page.set_attr_to(&m_main_page);
		font_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), m_dialog_page);

		D2D1_COLOR_F f_color;
		m_dialog_page.get_font_color(f_color);
		const float val0 = static_cast<float>(conv_color_comp(f_color.r));
		const float val1 = static_cast<float>(conv_color_comp(f_color.g));
		const float val2 = static_cast<float>(conv_color_comp(f_color.b));
		const float val3 = static_cast<float>(conv_color_comp(f_color.a));
		float f_size;
		m_dialog_page.get_font_size(f_size);
		const float val4 = f_size;
		dialog_slider_0().Maximum(255.0);
		dialog_slider_0().TickFrequency(1.0);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val0);
		font_slider_set_header<0>(val0);
		dialog_slider_1().Maximum(255.0);
		dialog_slider_1().TickFrequency(1.0);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(val1);
		font_slider_set_header<1>(val1);
		dialog_slider_2().Maximum(255.0);
		dialog_slider_2().TickFrequency(1.0);
		dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_2().Value(val2);
		font_slider_set_header<2>(val2);
		dialog_slider_3().Maximum(255.0);
		dialog_slider_3().TickFrequency(1.0);
		dialog_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_3().Value(val3);
		font_slider_set_header<3>(val3);
		dialog_slider_4().Maximum(FONT_SIZE_MAX - 1.0f);
		dialog_slider_4().TickFrequency(TICK_FREQ);
		dialog_slider_4().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_4().Value(val4 - 1.0);
		font_slider_set_header<4>(val4 - 1.0f);

		dialog_slider_0().Visibility(Visibility::Visible);
		dialog_slider_1().Visibility(Visibility::Visible);
		dialog_slider_2().Visibility(Visibility::Visible);
		dialog_slider_3().Visibility(Visibility::Visible);
		dialog_slider_4().Visibility(Visibility::Visible);
		const auto token0{
			dialog_slider_0().ValueChanged(
				{ this, &MainPage::font_slider_value_changed<0> })
		};
		const auto token1{
			dialog_slider_1().ValueChanged(
				{ this, &MainPage::font_slider_value_changed<1> })
		};
		const auto token2{
			dialog_slider_2().ValueChanged(
				{ this, &MainPage::font_slider_value_changed<2> })
		};
		const auto token3{
			dialog_slider_3().ValueChanged(
				{ this, &MainPage::font_slider_value_changed<3> })
		};
		const auto token4{
			dialog_slider_4().ValueChanged(
				{ this, &MainPage::font_slider_value_changed<4> })
		};
		cd_setting_dialog().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(L"str_font_color")));
		m_mutex_event.lock();
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F new_color;
			m_dialog_page.m_shape_list.back()->get_font_color(new_color);
			float new_size;
			m_dialog_page.m_shape_list.back()->get_font_size(new_size);
			const bool flag_color = ustack_push_set<UNDO_ID::FONT_COLOR>(new_color);
			const bool flag_size = ustack_push_set<UNDO_ID::FONT_SIZE>(new_size);
			if (flag_color || flag_size) {
				ustack_push_null();
				xcvd_is_enabled();
				page_draw();
			}
		}
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		dialog_slider_2().Visibility(Visibility::Collapsed);
		dialog_slider_3().Visibility(Visibility::Collapsed);
		dialog_slider_4().Visibility(Visibility::Collapsed);
		dialog_slider_0().ValueChanged(token0);
		dialog_slider_1().ValueChanged(token1);
		dialog_slider_2().ValueChanged(token2);
		dialog_slider_3().ValueChanged(token3);
		dialog_slider_4().ValueChanged(token4);
		page_draw();

		m_mutex_event.unlock();
		slist_clear(m_dialog_page.m_shape_list);
	}

	//---------------------------------
	// 書体メニューの「書体名」が選択された.
	//---------------------------------
	IAsyncAction MainPage::font_family_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		//FindName(L"cd_setting_dialog");
		m_dialog_page.set_attr_to(&m_main_page);
		for (uint32_t i = 0; wchar_t* name = ShapeText::s_available_fonts[i]; i++) {
			// ローカル名をリストアイテムに格納する.
			auto item = box_value(winrt::hstring(name + wcslen(name) + 1));
			lv_dialog_list().Items().Append(item);
		}
		for (uint32_t i = 0; i < lv_dialog_list().Items().Size(); i++) {
			IInspectable item[1];
			lv_dialog_list().Items().GetMany(i, item);
			auto name = unbox_value<winrt::hstring>(item[0]).c_str();
			wchar_t* f_family;
			m_main_page.get_font_family(f_family);
			if (wcscmp(name, f_family + wcslen(f_family) + 1) == 0) {
				// 書体名が同じ場合,
				// その書体をリストビューの選択済み項目に格納する.
				lv_dialog_list().SelectedItem(item[0]);
				lv_dialog_list().ScrollIntoView(item[0]);
				break;
			}
		}
		const auto loaded_token = lv_dialog_list().Loaded({ this, &MainPage::dialog_list_loaded });
		const auto changed_token = lv_dialog_list().SelectionChanged(
			[this](auto, auto)
			{
				auto i = lv_dialog_list().SelectedIndex();
				m_dialog_page.m_shape_list.back()->set_font_family(ShapeText::s_available_fonts[i]);
				if (scp_dialog_panel().IsLoaded()) {
					dialog_draw();
				}
			}
		);
		lv_dialog_list().Visibility(Visibility::Visible);
		font_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), m_dialog_page);
		cd_setting_dialog().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(L"str_font_family")));
		m_mutex_event.lock();
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			wchar_t* samp_val;
			m_dialog_page.m_shape_list.back()->get_font_family(samp_val);
			if (ustack_push_set<UNDO_ID::FONT_FAMILY>(samp_val)) {
				ustack_push_null();
				xcvd_is_enabled();
				page_draw();
			}
		}
		slist_clear(m_dialog_page.m_shape_list);
		lv_dialog_list().Loaded(loaded_token);
		lv_dialog_list().SelectionChanged(changed_token);
		lv_dialog_list().Visibility(Visibility::Collapsed);
		lv_dialog_list().Items().Clear();
		//UnloadObject(cd_setting_dialog());
		page_draw();
		m_mutex_event.unlock();
	}

	//---------------------------------
	// 値をスライダーのヘッダーに格納する.
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	//---------------------------------
	template <int S>
	void MainPage::font_slider_set_header(const float val)
	{
		if constexpr (S == 4) {
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_NAME_APPEND>(
				m_len_unit, val + 1.0f, m_dialog_d2d.m_logical_dpi, 
				m_dialog_page.m_grid_base + 1.0f, buf);
			const auto text{
				ResourceLoader::GetForCurrentView().GetString(L"str_font_size") + L": " + buf
			};
			dialog_set_slider_header<4>(text);
		}
		else if constexpr (S == 0 || S == 1 || S == 2 || S == 3) {
			constexpr wchar_t* HEADER[]{ 
				L"str_color_r", L"str_color_g",L"str_color_b", L"str_opacity"
			};
			wchar_t buf[32];
			conv_col_to_str(m_color_code, val, buf);
			dialog_set_slider_header<S>(
				ResourceLoader::GetForCurrentView().GetString(HEADER[S]) + L": " + buf);
		}
	}

	//---------------------------------
	// スライダーの値が変更された.
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	//---------------------------------
	template <int S>
	void MainPage::font_slider_value_changed(
		IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (S == 4) {
			const auto val = static_cast<float>(args.NewValue());
			font_slider_set_header<4>(val);
			if (m_dialog_page.m_shape_list.back()->set_font_size(val + 1.0f)) {
				dialog_draw();
			}
		}
		else if constexpr (S == 0) {
			const auto val = static_cast<float>(args.NewValue());
			D2D1_COLOR_F f_color;
			m_dialog_page.m_shape_list.back()->get_font_color(f_color);
			font_slider_set_header<0>(val);
			f_color.r = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_dialog_page.m_shape_list.back()->set_font_color(f_color)) {
				dialog_draw();
			}
		}
		else if constexpr (S == 1) {
			const auto val = static_cast<float>(args.NewValue());
			D2D1_COLOR_F f_color;
			m_dialog_page.m_shape_list.back()->get_font_color(f_color);
			font_slider_set_header<1>(val);
			f_color.g = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_dialog_page.m_shape_list.back()->set_font_color(f_color)) {
				dialog_draw();
			}
		}
		else if constexpr (S == 2) {
			const auto val = static_cast<float>(args.NewValue());
			D2D1_COLOR_F f_color;
			m_dialog_page.m_shape_list.back()->get_font_color(f_color);
			font_slider_set_header<2>(val);
			f_color.b = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_dialog_page.m_shape_list.back()->set_font_color(f_color)) {
				dialog_draw();
			}
		}
		else if constexpr (S == 3) {
			const auto val = static_cast<float>(args.NewValue());
			D2D1_COLOR_F f_color;
			m_dialog_page.m_shape_list.back()->get_font_color(f_color);
			font_slider_set_header<3>(val);
			f_color.a = static_cast<FLOAT>(val / COLOR_MAX);
			if (m_dialog_page.m_shape_list.back()->set_font_color(f_color)) {
				dialog_draw();
			}
		}
	}

	//---------------------------------
	// 書体メニューの「大きさ」が選択された.
	//---------------------------------
	IAsyncAction MainPage::font_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto TICK_FREQ = 1.0;
		m_dialog_page.set_attr_to(&m_main_page);
		font_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), m_dialog_page);
		float f_size;
		m_dialog_page.get_font_size(f_size);
		dialog_slider_4().Maximum(FONT_SIZE_MAX - 1.0f);
		dialog_slider_4().TickFrequency(TICK_FREQ);
		dialog_slider_4().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_4().Value(f_size - 1.0);
		font_slider_set_header<4>(f_size - 1.0f);
		dialog_slider_4().Visibility(Visibility::Visible);
		const auto token4{
			dialog_slider_4().ValueChanged(
				{ this, &MainPage::font_slider_value_changed<4> })
		};
		cd_setting_dialog().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(L"str_font_size")));
		m_mutex_event.lock();
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float samp_val;
			m_dialog_page.m_shape_list.back()->get_font_size(samp_val);
			if (ustack_push_set<UNDO_ID::FONT_SIZE>(samp_val)) {
				ustack_push_null();
				xcvd_is_enabled();
				page_draw();
			}
		}
		slist_clear(m_dialog_page.m_shape_list);
		dialog_slider_4().Visibility(Visibility::Collapsed);
		dialog_slider_4().ValueChanged(token4);
		page_draw();
		m_mutex_event.unlock();
	}

	void MainPage::font_stretch_selection_changed(IInspectable const&, SelectionChangedEventArgs const& args)
	{
		uint32_t i = lv_dialog_list().SelectedIndex();
		if (m_dialog_page.m_shape_list.back()->set_font_stretch(
			static_cast<DWRITE_FONT_STRETCH>(FONT_STRETCH[i]))) {
			dialog_draw();
		}
	}

	//---------------------------------
	// 書体メニューの「書体の幅」が選択された.
	//---------------------------------
	IAsyncAction MainPage::font_stretch_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_dialog_page.set_attr_to(&m_main_page);
		for (uint32_t i = 0; FONT_STRETCH_NAME[i] != nullptr; i++) {
			auto item = box_value(ResourceLoader::GetForCurrentView().GetString(FONT_STRETCH_NAME[i]));
			lv_dialog_list().Items().Append(item);
		}
		lv_dialog_list().SelectedIndex(-1);
		const auto k = lv_dialog_list().Items().Size();
		for (uint32_t i = 0; i < k; i++) {
			DWRITE_FONT_STRETCH f_stretch;
			m_main_page.get_font_stretch(f_stretch);
			if (FONT_STRETCH[i] == f_stretch) {
				lv_dialog_list().SelectedIndex(i);
				IInspectable item[1];
				lv_dialog_list().Items().GetMany(i, item);
				lv_dialog_list().ScrollIntoView(item[0]);
				break;
			}
		}
		const auto loaded_token{
			lv_dialog_list().Loaded({ this, &MainPage::dialog_list_loaded })
		};
		const auto changed_token{
			lv_dialog_list().SelectionChanged({ this, &MainPage::font_stretch_selection_changed })
		};
		lv_dialog_list().Visibility(Visibility::Visible);
		font_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), m_dialog_page);
		cd_setting_dialog().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(L"str_font_stretch")));
		m_mutex_event.lock();
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			DWRITE_FONT_STRETCH samp_val;
			m_dialog_page.m_shape_list.back()->get_font_stretch(samp_val);
			if (ustack_push_set<UNDO_ID::FONT_STRETCH>(samp_val)) {
				ustack_push_null();
				xcvd_is_enabled();
				page_draw();
			}
		}
		slist_clear(m_dialog_page.m_shape_list);
		lv_dialog_list().Loaded(loaded_token);
		lv_dialog_list().SelectionChanged(changed_token);
		lv_dialog_list().Visibility(Visibility::Collapsed);
		lv_dialog_list().Items().Clear();
		page_draw();
		m_mutex_event.unlock();
	}

	// 書体メニューの「字体」に印をつける.
	// f_style	書体の字体
	void MainPage::font_style_is_checked(const DWRITE_FONT_STYLE f_style)
	{
		rmfi_font_style_italic().IsChecked(f_style == DWRITE_FONT_STYLE_ITALIC);
		rmfi_font_style_normal().IsChecked(f_style == DWRITE_FONT_STYLE_NORMAL);
		rmfi_font_style_oblique().IsChecked(f_style == DWRITE_FONT_STYLE_OBLIQUE);

		//rmfi_font_style_italic_2().IsChecked(f_style == DWRITE_FONT_STYLE_ITALIC);
		//rmfi_font_style_normal_2().IsChecked(f_style == DWRITE_FONT_STYLE_NORMAL);
		//rmfi_font_style_oblique_2().IsChecked(f_style == DWRITE_FONT_STYLE_OBLIQUE);
	}

	// 書体メニューの「イタリック体」が選択された.
	void MainPage::font_style_italic_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (ustack_push_set<UNDO_ID::FONT_STYLE>(DWRITE_FONT_STYLE_ITALIC)) {
			ustack_push_null();
			xcvd_is_enabled();
			page_draw();
		}
		status_bar_set_pos();
	}

	// 書体メニューの「標準」が選択された.
	void MainPage::font_style_normal_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (ustack_push_set<UNDO_ID::FONT_STYLE>(DWRITE_FONT_STYLE_NORMAL)) {
			ustack_push_null();
			xcvd_is_enabled();
			page_draw();
		}
		status_bar_set_pos();
	}

	// 書体メニューの「斜体」が選択された.
	void MainPage::font_style_oblique_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (ustack_push_set<UNDO_ID::FONT_STYLE>(DWRITE_FONT_STYLE_OBLIQUE)) {
			ustack_push_null();
			xcvd_is_enabled();
			page_draw();
		}
		status_bar_set_pos();
	}

	// 書体メニューの「太さ」が選択された.
	IAsyncAction MainPage::font_weight_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_dialog_page.set_attr_to(&m_main_page);
		for (uint32_t i = 0; FONT_WEIGHT_NAME[i] != nullptr; i++) {
			auto item = box_value(ResourceLoader::GetForCurrentView().GetString(FONT_WEIGHT_NAME[i]));
			lv_dialog_list().Items().Append(item);
		}
		lv_dialog_list().SelectedIndex(-1);
		const auto k = lv_dialog_list().Items().Size();
		for (uint32_t i = 0; i < k; i++) {
			DWRITE_FONT_WEIGHT f_weight;
			m_main_page.get_font_weight(f_weight);
			if (FONT_WEIGHTS[i] == f_weight) {
				lv_dialog_list().SelectedIndex(i);
				IInspectable item[1];
				lv_dialog_list().Items().GetMany(i, item);
				lv_dialog_list().ScrollIntoView(item[0]);
				break;
			}
		}
		const auto loaded_token = lv_dialog_list().Loaded({ this, &MainPage::dialog_list_loaded });
		const auto changed_token = lv_dialog_list().SelectionChanged(
			[this](auto, auto args) {
				uint32_t i = lv_dialog_list().SelectedIndex();
				m_dialog_page.m_shape_list.back()->set_font_weight(
					static_cast<DWRITE_FONT_WEIGHT>(FONT_WEIGHTS[i]));
				if (scp_dialog_panel().IsLoaded()) {
					dialog_draw();
				}
			}
		);
		lv_dialog_list().Visibility(Visibility::Visible);
		font_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), m_dialog_page);
		cd_setting_dialog().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(L"str_font_weight")));
		m_mutex_event.lock();
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			DWRITE_FONT_WEIGHT samp_val;
			m_dialog_page.m_shape_list.back()->get_font_weight(samp_val);
			if (ustack_push_set<UNDO_ID::FONT_WEIGHT>(samp_val)) {
				ustack_push_null();
				xcvd_is_enabled();
				page_draw();
			}
		}
		slist_clear(m_dialog_page.m_shape_list);
		lv_dialog_list().Loaded(loaded_token);
		lv_dialog_list().SelectionChanged(changed_token);
		lv_dialog_list().Visibility(Visibility::Collapsed);
		lv_dialog_list().Items().Clear();
		status_bar_set_pos();
		m_mutex_event.unlock();
	}
}