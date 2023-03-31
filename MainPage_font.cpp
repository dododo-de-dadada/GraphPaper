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

	// 見本の図形を作成する.
	static void font_create_sample_shape(const float p_width, const float p_height, ShapePage& page);

	// 見本の図形を作成する.
	// p_width	見本を表示するパネルの幅
	// p_height	見本を表示するパネルの高さ
	// page	見本を表示するシート
	static void font_create_sample_shape(const float p_width, const float p_height, ShapePage& page)
	{
		const auto mar_w = p_width * 0.125;
		const auto mar_h = p_height * 0.125;
		const D2D1_POINT_2F start{
			static_cast<FLOAT>(mar_w), static_cast<FLOAT>(mar_h)
		};
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(p_width - 2.0 * mar_w), static_cast<FLOAT>(p_width - 2.0 * mar_h)
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

	void MainPage::font_stretch_is_checked(const DWRITE_FONT_STRETCH val)
	{
		if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_CONDENSED) {
			rmfi_font_stretch_ultra_condensed().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_CONDENSED) {
			rmfi_font_stretch_extra_condensed().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_CONDENSED) {
			rmfi_font_stretch_condensed().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_CONDENSED) {
			rmfi_font_stretch_semi_condensed().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL) {
			rmfi_font_stretch_normal().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_EXPANDED) {
			rmfi_font_stretch_semi_expanded().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXPANDED) {
			rmfi_font_stretch_expanded().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_EXPANDED) {
			rmfi_font_stretch_extra_expanded().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_EXPANDED) {
			rmfi_font_stretch_ultra_expanded().IsChecked(true);
		}
	}

	void MainPage::font_stretch_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_FONT_STRETCH f_stretch = static_cast<DWRITE_FONT_STRETCH>(-1);
		if (sender == rmfi_font_stretch_ultra_condensed()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_CONDENSED;
		}
		else if (sender == rmfi_font_stretch_extra_condensed()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_CONDENSED;
		}
		else if (sender == rmfi_font_stretch_condensed()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_CONDENSED;
		}
		else if (sender == rmfi_font_stretch_semi_condensed()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_CONDENSED;
		}
		else if (sender == rmfi_font_stretch_normal()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;
		}
		else if (sender == rmfi_font_stretch_semi_expanded()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_EXPANDED;
		}
		else if (sender == rmfi_font_stretch_expanded()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXPANDED;
		}
		else if (sender == rmfi_font_stretch_extra_expanded()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_EXPANDED;
		}
		else if (sender == rmfi_font_stretch_ultra_expanded()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_EXPANDED;
		}
		if (f_stretch != static_cast<DWRITE_FONT_STRETCH>(-1) && ustack_push_set<UNDO_T::FONT_STRETCH>(f_stretch)) {
			ustack_push_null();
			ustack_is_enable();
			main_draw();
		}
		status_bar_set_pos();
	}

	void MainPage::font_weight_is_checked(DWRITE_FONT_WEIGHT val)
	{
		if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_THIN) {
			rmfi_font_weight_thin().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_LIGHT) {
			rmfi_font_weight_extra_light().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_LIGHT) {
			rmfi_font_weight_light().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL) {
			rmfi_font_weight_normal().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_MEDIUM) {
			rmfi_font_weight_medium().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_SEMI_BOLD) {
			rmfi_font_weight_semi_bold().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BOLD) {
			rmfi_font_weight_bold().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BOLD) {
			rmfi_font_weight_extra_bold().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BLACK) {
			rmfi_font_weight_black().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BLACK) {
			rmfi_font_weight_extra_black().IsChecked(true);
		}
	}

	void MainPage::font_weight_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_FONT_WEIGHT f_weight = static_cast<DWRITE_FONT_WEIGHT>(-1);
		if (sender == rmfi_font_weight_thin()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_THIN;
		}
		else if (sender == rmfi_font_weight_extra_light()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_LIGHT;
		}
		else if (sender == rmfi_font_weight_light()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_LIGHT;
		}
		else if (sender == rmfi_font_weight_normal()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;
		}
		else if (sender == rmfi_font_weight_medium()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_MEDIUM;
		}
		else if (sender == rmfi_font_weight_semi_bold()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_SEMI_BOLD;
		}
		else if (sender == rmfi_font_weight_bold()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BOLD;
		}
		else if (sender == rmfi_font_weight_extra_bold()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BOLD;
		}
		else if (sender == rmfi_font_weight_black()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BLACK;
		}
		else if (sender == rmfi_font_weight_extra_black()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BLACK;
		}
		if (f_weight != static_cast<DWRITE_FONT_WEIGHT>(-1) && ustack_push_set<UNDO_T::FONT_WEIGHT>(f_weight)) {
			ustack_push_null();
			ustack_is_enable();
			//xcvd_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	//---------------------------------
	// 書体メニューの「書体名」が選択された.
	//---------------------------------
	IAsyncAction MainPage::font_family_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		const auto str_title{ ResourceLoader::GetForCurrentView().GetString(L"str_font_family") };
		m_prop_page.set_attr_to(&m_main_page);
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
		lv_dialog_list().Visibility(Visibility::Visible);
		font_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), m_prop_page);
		cd_dialog_prop().Title(box_value(str_title));
		m_mutex_event.lock();
		{
			const auto revoker1 = lv_dialog_list().SelectionChanged(
				winrt::auto_revoke,
				[this](auto, auto)
				{
					auto i = lv_dialog_list().SelectedIndex();
					m_prop_page.back()->set_font_family(ShapeText::s_available_fonts[i]);
					if (scp_dialog_panel().IsLoaded()) {
						dialog_draw();
					}
				}
			);
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				wchar_t* samp_val;
				m_prop_page.back()->get_font_family(samp_val);
				if (ustack_push_set<UNDO_T::FONT_FAMILY>(samp_val)) {
					ustack_push_null();
					ustack_is_enable();
					//xcvd_is_enabled();
					main_draw();
				}
			}
		}
		slist_clear(m_prop_page.m_shape_list);
		//lv_dialog_list().Loaded(loaded_token);
		//lv_dialog_list().SelectionChanged(changed_token);
		lv_dialog_list().Visibility(Visibility::Collapsed);
		lv_dialog_list().Items().Clear();
		//UnloadObject(cd_dialog_prop());
		main_draw();
		m_mutex_event.unlock();
	}

	//---------------------------------
	// 書体メニューの「大きさ」が選択された.
	//---------------------------------
	IAsyncAction MainPage::font_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		const auto str_font_size{ ResourceLoader::GetForCurrentView().GetString(L"str_font_size") + L": " };
		const auto str_title{ ResourceLoader::GetForCurrentView().GetString(L"str_font_size") };
		constexpr auto TICK_FREQ = 1.0;
		m_prop_page.set_attr_to(&m_main_page);
		font_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), m_prop_page);
		float f_size;
		m_prop_page.get_font_size(f_size);
		dialog_slider_4().Minimum(1.0f);
		dialog_slider_4().Maximum(FONT_SIZE_MAX);
		dialog_slider_4().TickFrequency(TICK_FREQ);
		dialog_slider_4().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_4().Value(f_size);
		const auto unit = m_len_unit;
		const auto dpi = m_prop_d2d.m_logical_dpi;
		const auto g_len = m_prop_page.m_grid_base + 1.0f;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, f_size, dpi, g_len, buf);
		dialog_slider_4().Header(box_value(str_font_size + buf));
		dialog_slider_4().Visibility(Visibility::Visible);
		cd_dialog_prop().Title(box_value(str_title));
		m_mutex_event.lock();
		{
			const auto revoker4{
				dialog_slider_4().ValueChanged(winrt::auto_revoke,
					[this, str_font_size](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					const auto val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_4().Header(box_value(str_font_size + buf));
					if (m_prop_page.back()->set_font_size(val)) {
						dialog_draw();
					}
				})
			};
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				float samp_val;
				m_prop_page.back()->get_font_size(samp_val);
				if (ustack_push_set<UNDO_T::FONT_SIZE>(samp_val)) {
					ustack_push_null();
					ustack_is_enable();
					//xcvd_is_enabled();
					main_draw();
				}
			}
		}
		slist_clear(m_prop_page.m_shape_list);
		dialog_slider_4().Visibility(Visibility::Collapsed);
		main_draw();
		m_mutex_event.unlock();
	}

	// 書体メニューの「字体」に印をつける.
	// f_style	書体の字体
	void MainPage::font_style_is_checked(const DWRITE_FONT_STYLE f_style)
	{
		if (f_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC) {
			rmfi_font_style_italic().IsChecked(true);
		}
		else if (f_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL) {
			rmfi_font_style_normal().IsChecked(true);
		}
		else if (f_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE) {
			rmfi_font_style_oblique().IsChecked(true);
		}
	}

	void MainPage::font_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_FONT_STYLE f_style = static_cast<DWRITE_FONT_STYLE>(-1);
		if (sender == rmfi_font_style_normal()) {
			f_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;
		}
		else if (sender == rmfi_font_style_italic()) {
			f_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC;
		}
		else if (sender == rmfi_font_style_oblique()) {
			f_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE;
		}
		if (f_style != static_cast<DWRITE_FONT_STYLE>(-1) && ustack_push_set<UNDO_T::FONT_STYLE>(f_style)) {
			ustack_push_null();
			ustack_is_enable();
			main_draw();
		}
		status_bar_set_pos();
	}

	// 書体メニューの「イタリック体」が選択された.
	/*
	void MainPage::font_style_italic_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (ustack_push_set<UNDO_T::FONT_STYLE>(DWRITE_FONT_STYLE_ITALIC)) {
			ustack_push_null();
			ustack_is_enable();
			//xcvd_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	// 書体メニューの「字体「標準」が選択された.
	void MainPage::font_style_normal_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (ustack_push_set<UNDO_T::FONT_STYLE>(DWRITE_FONT_STYLE_NORMAL)) {
			ustack_push_null();
			ustack_is_enable();
			//xcvd_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	// 書体メニューの「斜体」が選択された.
	void MainPage::font_style_oblique_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (ustack_push_set<UNDO_T::FONT_STYLE>(DWRITE_FONT_STYLE_OBLIQUE)) {
			ustack_push_null();
			ustack_is_enable();
			//xcvd_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}
	*/

	// 書体メニューの「太さ」が選択された.
	/*
	IAsyncAction MainPage::font_weight_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_prop_page.set_attr_to(&m_main_page);
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
		lv_dialog_list().Visibility(Visibility::Visible);
		font_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), m_prop_page);
		cd_dialog_prop().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(L"str_font_weight")));
		m_mutex_event.lock();
		{
			const auto revoker{
				lv_dialog_list().SelectionChanged(winrt::auto_revoke, [this](auto, auto) {
					uint32_t i = lv_dialog_list().SelectedIndex();
					m_prop_page.back()->set_font_weight(static_cast<DWRITE_FONT_WEIGHT>(FONT_WEIGHTS[i]));
					if (scp_dialog_panel().IsLoaded()) {
						dialog_draw();
					}
				})
			};
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				DWRITE_FONT_WEIGHT new_val;
				m_prop_page.back()->get_font_weight(new_val);
				if (ustack_push_set<UNDO_T::FONT_WEIGHT>(new_val)) {
					ustack_push_null();
					xcvd_is_enabled();
					main_draw();
				}
			}
		}
		slist_clear(m_prop_page.m_shape_list);
		//lv_dialog_list().Loaded(loaded_token);
		lv_dialog_list().Visibility(Visibility::Collapsed);
		lv_dialog_list().Items().Clear();
		status_bar_set_pos();
		m_mutex_event.unlock();
	}
	*/
}