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
	static Shape* font_create_sample_shape(const float p_width, const float p_height, const Shape* prop) noexcept;

	// 見本の図形を作成する.
	// p_width	見本を表示するパネルの幅
	// p_height	見本を表示するパネルの高さ
	// page	見本を表示するシート
	static Shape* font_create_sample_shape(const float p_width, const float p_height, const Shape* prop) noexcept
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
		Shape* t = new ShapeText(start, pos, wchar_cpy(text), prop);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		return t;
	}

	void MainPage::font_stretch_is_checked(const DWRITE_FONT_STRETCH val)
	{
		if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_CONDENSED) {
			rmfi_menu_font_stretch_ultra_condensed().IsChecked(true);
			rmfi_popup_font_stretch_ultra_condensed().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_CONDENSED) {
			rmfi_menu_font_stretch_extra_condensed().IsChecked(true);
			rmfi_popup_font_stretch_extra_condensed().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_CONDENSED) {
			rmfi_menu_font_stretch_condensed().IsChecked(true);
			rmfi_popup_font_stretch_condensed().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_CONDENSED) {
			rmfi_menu_font_stretch_semi_condensed().IsChecked(true);
			rmfi_popup_font_stretch_semi_condensed().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL) {
			rmfi_menu_font_stretch_normal().IsChecked(true);
			rmfi_popup_font_stretch_normal().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_EXPANDED) {
			rmfi_menu_font_stretch_semi_expanded().IsChecked(true);
			rmfi_popup_font_stretch_semi_expanded().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXPANDED) {
			rmfi_menu_font_stretch_expanded().IsChecked(true);
			rmfi_popup_font_stretch_expanded().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_EXPANDED) {
			rmfi_menu_font_stretch_extra_expanded().IsChecked(true);
			rmfi_popup_font_stretch_extra_expanded().IsChecked(true);
		}
		else if (val == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_EXPANDED) {
			rmfi_menu_font_stretch_ultra_expanded().IsChecked(true);
			rmfi_popup_font_stretch_ultra_expanded().IsChecked(true);
		}
	}

	void MainPage::font_stretch_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_FONT_STRETCH f_stretch = static_cast<DWRITE_FONT_STRETCH>(-1);
		if (sender == rmfi_menu_font_stretch_ultra_condensed() || sender == rmfi_popup_font_stretch_ultra_condensed()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_CONDENSED;
		}
		else if (sender == rmfi_menu_font_stretch_extra_condensed() || sender == rmfi_popup_font_stretch_extra_condensed()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_CONDENSED;
		}
		else if (sender == rmfi_menu_font_stretch_condensed() || sender == rmfi_popup_font_stretch_condensed()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_CONDENSED;
		}
		else if (sender == rmfi_menu_font_stretch_semi_condensed() || sender == rmfi_popup_font_stretch_semi_condensed()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_CONDENSED;
		}
		else if (sender == rmfi_menu_font_stretch_normal() || sender == rmfi_popup_font_stretch_normal()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;
		}
		else if (sender == rmfi_menu_font_stretch_semi_expanded() || sender == rmfi_popup_font_stretch_semi_expanded()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_EXPANDED;
		}
		else if (sender == rmfi_menu_font_stretch_expanded() || sender == rmfi_popup_font_stretch_expanded()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXPANDED;
		}
		else if (sender == rmfi_menu_font_stretch_extra_expanded() || sender == rmfi_popup_font_stretch_extra_expanded()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_EXPANDED;
		}
		else if (sender == rmfi_menu_font_stretch_ultra_expanded() || sender == rmfi_popup_font_stretch_ultra_expanded()) {
			f_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_EXPANDED;
		}
		undo_push_null();
		if (f_stretch != static_cast<DWRITE_FONT_STRETCH>(-1) && undo_push_set<UNDO_T::FONT_STRETCH>(f_stretch)) {
			//undo_menu_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	void MainPage::font_weight_is_checked(DWRITE_FONT_WEIGHT val)
	{
		if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_THIN) {
			rmfi_menu_font_weight_thin().IsChecked(true);
			rmfi_popup_font_weight_thin().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_LIGHT) {
			rmfi_menu_font_weight_extra_light().IsChecked(true);
			rmfi_popup_font_weight_extra_light().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_LIGHT) {
			rmfi_menu_font_weight_light().IsChecked(true);
			rmfi_popup_font_weight_light().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL) {
			rmfi_menu_font_weight_normal().IsChecked(true);
			rmfi_popup_font_weight_normal().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_MEDIUM) {
			rmfi_menu_font_weight_medium().IsChecked(true);
			rmfi_popup_font_weight_medium().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_SEMI_BOLD) {
			rmfi_menu_font_weight_semi_bold().IsChecked(true);
			rmfi_popup_font_weight_semi_bold().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BOLD) {
			rmfi_menu_font_weight_bold().IsChecked(true);
			rmfi_popup_font_weight_bold().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BOLD) {
			rmfi_menu_font_weight_extra_bold().IsChecked(true);
			rmfi_popup_font_weight_extra_bold().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BLACK) {
			rmfi_menu_font_weight_black().IsChecked(true);
			rmfi_popup_font_weight_black().IsChecked(true);
		}
		else if (val == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BLACK) {
			rmfi_menu_font_weight_extra_black().IsChecked(true);
			rmfi_popup_font_weight_extra_black().IsChecked(true);
		}
	}

	void MainPage::font_weight_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_FONT_WEIGHT f_weight = static_cast<DWRITE_FONT_WEIGHT>(-1);
		if (sender == rmfi_menu_font_weight_thin() || sender == rmfi_popup_font_weight_thin()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_THIN;
		}
		else if (sender == rmfi_menu_font_weight_extra_light() || sender == rmfi_popup_font_weight_extra_light()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_LIGHT;
		}
		else if (sender == rmfi_menu_font_weight_light() || sender == rmfi_popup_font_weight_light()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_LIGHT;
		}
		else if (sender == rmfi_menu_font_weight_normal() || sender == rmfi_popup_font_weight_normal()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;
		}
		else if (sender == rmfi_menu_font_weight_medium() || sender == rmfi_popup_font_weight_medium()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_MEDIUM;
		}
		else if (sender == rmfi_menu_font_weight_semi_bold() || sender == rmfi_popup_font_weight_semi_bold()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_SEMI_BOLD;
		}
		else if (sender == rmfi_menu_font_weight_bold() || sender == rmfi_popup_font_weight_bold()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BOLD;
		}
		else if (sender == rmfi_menu_font_weight_extra_bold() || sender == rmfi_popup_font_weight_extra_bold()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BOLD;
		}
		else if (sender == rmfi_menu_font_weight_black() || sender == rmfi_popup_font_weight_black()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BLACK;
		}
		else if (sender == rmfi_menu_font_weight_extra_black() || sender == rmfi_popup_font_weight_extra_black()) {
			f_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BLACK;
		}
		undo_push_null();
		if (f_weight != static_cast<DWRITE_FONT_WEIGHT>(-1) && undo_push_set<UNDO_T::FONT_WEIGHT>(f_weight)) {
			//undo_menu_is_enabled();
			//xcvd_menu_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	//---------------------------------
	// 書体メニューの「書体名」が選択された.
	//---------------------------------
	IAsyncAction MainPage::font_family_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		const auto str_title{
			ResourceLoader::GetForCurrentView().GetString(L"str_font_family")
		};
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
		const Shape* prop = m_event_shape_pressed;
		if (prop == nullptr || typeid(*prop) != typeid(ShapeText)) {
			prop = &m_prop_page;
		}
		m_prop_page.m_shape_list.push_back(font_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), prop));
		cd_dialog_prop().Title(box_value(str_title));
		m_mutex_event.lock();
		{
			const auto revoker1 = lv_dialog_list().SelectionChanged(
				winrt::auto_revoke,
				[this](auto, auto)
				{
					auto i = lv_dialog_list().SelectedIndex();
					m_prop_page.slist_back()->set_font_family(ShapeText::s_available_fonts[i]);
					if (scp_dialog_panel().IsLoaded()) {
						dialog_draw();
					}
				}
			);
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				wchar_t* samp_val;
				m_prop_page.slist_back()->get_font_family(samp_val);
				undo_push_null();
				if (undo_push_set<UNDO_T::FONT_FAMILY>(samp_val)) {
					//undo_menu_is_enabled();
					//xcvd_menu_is_enabled();
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
		const auto str_font_size{
			ResourceLoader::GetForCurrentView().GetString(L"str_font_size") + L": "
		};
		const auto str_title{
			ResourceLoader::GetForCurrentView().GetString(L"str_font_size")
		};
		constexpr auto TICK_FREQ = 1.0;
		m_prop_page.set_attr_to(&m_main_page);
		const Shape* prop = m_event_shape_pressed;
		if (prop == nullptr || typeid(*prop) != typeid(ShapeText)) {
			prop = &m_prop_page;
		}
		m_prop_page.m_shape_list.push_back(font_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), prop));
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
					if (m_prop_page.slist_back()->set_font_size(val)) {
						dialog_draw();
					}
				})
			};
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				float samp_val;
				m_prop_page.slist_back()->get_font_size(samp_val);
				undo_push_null();
				if (undo_push_set<UNDO_T::FONT_SIZE>(samp_val)) {
					//undo_menu_is_enabled();
					//xcvd_menu_is_enabled();
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
			rmfi_menu_font_style_italic().IsChecked(true);
			rmfi_popup_font_style_italic().IsChecked(true);
		}
		else if (f_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL) {
			rmfi_menu_font_style_normal().IsChecked(true);
			rmfi_popup_font_style_normal().IsChecked(true);
		}
		else if (f_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE) {
			rmfi_menu_font_style_oblique().IsChecked(true);
			rmfi_popup_font_style_oblique().IsChecked(true);
		}
	}

	void MainPage::font_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_FONT_STYLE f_style = static_cast<DWRITE_FONT_STYLE>(-1);
		if (sender == rmfi_menu_font_style_normal() || sender == rmfi_popup_font_style_normal()) {
			f_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;
		}
		else if (sender == rmfi_menu_font_style_italic() || sender == rmfi_popup_font_style_italic()) {
			f_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC;
		}
		else if (sender == rmfi_menu_font_style_oblique() || sender == rmfi_popup_font_style_oblique()) {
			f_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE;
		}
		undo_push_null();
		if (f_style != static_cast<DWRITE_FONT_STYLE>(-1) && undo_push_set<UNDO_T::FONT_STYLE>(f_style)) {
			//undo_menu_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	constexpr float TEXT_LINE_SP_DELTA = 2.0f;	// 行の高さの変分 (DPIs)

	// 書体メニューの「段落のそろえ」が選択された.
	void MainPage::text_align_vert_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_PARAGRAPH_ALIGNMENT val = static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(-1);
		if (sender == rmfi_menu_text_align_top() || sender == rmfi_popup_text_align_top()) {
			val = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		}
		else if (sender == rmfi_menu_text_align_bot() || sender == rmfi_popup_text_align_bot()) {
			val = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR;
		}
		else if (sender == rmfi_menu_text_align_mid() || sender == rmfi_popup_text_align_mid()) {
			val = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		}
		if (val != static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(-1)) {
			text_align_vert_is_checked(val);
			undo_push_null();
			if (undo_push_set<UNDO_T::TEXT_ALIGN_P>(val)) {
				//undo_menu_is_enabled();
				//xcvd_menu_is_enabled();
				main_draw();
			}
		}
		status_bar_set_pos();
	}

	// 書体メニューの「段落のそろえ」に印をつける.
	// val	段落のそろえ
	void MainPage::text_align_vert_is_checked(const DWRITE_PARAGRAPH_ALIGNMENT val)
	{
		rmfi_menu_text_align_top().IsChecked(val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		rmfi_popup_text_align_top().IsChecked(val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		rmfi_menu_text_align_bot().IsChecked(val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		rmfi_popup_text_align_bot().IsChecked(val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		rmfi_menu_text_align_mid().IsChecked(val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		rmfi_popup_text_align_mid().IsChecked(val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	// 書体メニューの「文字列の折り返し」のサブ項目に印をつける
	void MainPage::text_word_wrap_is_checked(const DWRITE_WORD_WRAPPING val)
	{
		if (val == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_WRAP) {
			rmfi_menu_text_wrap().IsChecked(true);
			rmfi_popup_text_wrap().IsChecked(true);
		}
		else if (val == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_NO_WRAP) {
			rmfi_menu_text_no_wrap().IsChecked(true);
			rmfi_popup_text_no_wrap().IsChecked(true);
		}
		else if (val == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_CHARACTER) {
			rmfi_menu_text_wrap_char().IsChecked(true);
			rmfi_popup_text_wrap_char().IsChecked(true);
		}
	}

	// 書体メニューの「文字列のそろえ」が選択された.
	void MainPage::text_align_horz_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_TEXT_ALIGNMENT val = static_cast<DWRITE_TEXT_ALIGNMENT>(-1);
		if (sender == rmfi_menu_text_align_left() || sender == rmfi_popup_text_align_left()) {
			val = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
		}
		else if (sender == rmfi_menu_text_align_right() || sender == rmfi_popup_text_align_right()) {
			val = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING;
		}
		else if (sender == rmfi_menu_text_align_center() || sender == rmfi_popup_text_align_center()) {
			val = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER;
		}
		else if (sender == rmfi_menu_text_align_just() || sender == rmfi_popup_text_align_just()) {
			val = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
		}
		if (val != static_cast<DWRITE_TEXT_ALIGNMENT>(-1)) {
			text_align_horz_is_checked(val);
			undo_push_null();
			if (undo_push_set<UNDO_T::TEXT_ALIGN_T>(val)) {
				//undo_menu_is_enabled();
				//xcvd_menu_is_enabled();
				main_draw();
			}
		}
		status_bar_set_pos();
	}

	// 書体メニューの「文字列のそろえ」に印をつける.
	// t_align	文字列のそろえ
	void MainPage::text_align_horz_is_checked(const DWRITE_TEXT_ALIGNMENT val)
	{
		rmfi_menu_text_align_left().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
		rmfi_popup_text_align_left().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
		rmfi_menu_text_align_right().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING);
		rmfi_popup_text_align_right().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING);
		rmfi_menu_text_align_center().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
		rmfi_popup_text_align_center().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
		rmfi_menu_text_align_just().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
		rmfi_popup_text_align_just().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
	}

	// 書体メニューの「行間」>「行間...」が選択された.
	IAsyncAction MainPage::text_line_sp_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		const auto str_text_line_sp{
			ResourceLoader::GetForCurrentView().GetString(L"str_text_line_sp") + L": "
		};
		const auto str_title{
			ResourceLoader::GetForCurrentView().GetString(L"str_text_line_sp")
		};
		const auto str_def_val{
			ResourceLoader::GetForCurrentView().GetString(L"str_def_val")
		};
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_prop_page.set_attr_to(&m_main_page);
		float val;
		m_prop_page.get_text_line_sp(val);

		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val);
		const auto unit = m_len_unit;
		const auto dpi = m_prop_d2d.m_logical_dpi;
		const auto g_len = m_prop_page.m_grid_base + 1.0f;
		wchar_t buf[32];
		if (val >= FLT_MIN) {
			conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
		}
		else {
			wcscpy_s(buf, str_def_val.data());
		}
		dialog_slider_0().Header(box_value(str_text_line_sp + buf));
		dialog_slider_0().Visibility(Visibility::Visible);

		const Shape* prop = m_event_shape_pressed;
		if (prop == nullptr || typeid(*prop) != typeid(ShapeText)) {
			prop = &m_prop_page;
		}
		m_prop_page.m_shape_list.push_back(font_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), prop));
		cd_dialog_prop().Title(box_value(str_title));
		m_mutex_event.lock();
		{
			const auto revoker0{
				dialog_slider_0().ValueChanged(
					winrt::auto_revoke,
					[this, str_text_line_sp, str_def_val](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
						const auto unit = m_len_unit;
						const auto dpi = m_prop_d2d.m_logical_dpi;
						const auto g_len = m_prop_page.m_grid_base + 1.0f;
						const float val = static_cast<float>(args.NewValue());
						wchar_t buf[32];
						if (val >= FLT_MIN) {
							conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
						}
						else {
							wcscpy_s(buf, str_def_val.data());
						}
						dialog_slider_0().Header(box_value(str_text_line_sp + buf));
						if (m_prop_page.slist_back()->set_text_line_sp(val)) {
							dialog_draw();
						}
					}
				)
			};
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				float samp_val;
				m_prop_page.slist_back()->get_text_line_sp(samp_val);
				undo_push_null();
				if (undo_push_set<UNDO_T::TEXT_LINE_SP>(samp_val)) {
					//undo_menu_is_enabled();
					//xcvd_menu_is_enabled();
					main_draw();
				}
			}
		}
		slist_clear(m_prop_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		m_mutex_event.unlock();
	}

	// 書体メニューの「文字列の折り返し」のサブ項目が選択された
	void MainPage::text_word_wrap_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		bool changed = false;
		if (sender == rmfi_menu_text_wrap() || sender == rmfi_popup_text_wrap()) {
			changed = undo_push_set<UNDO_T::TEXT_WRAP>(DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_WRAP);
		}
		else if (sender == rmfi_menu_text_no_wrap() || sender == rmfi_popup_text_no_wrap()) {
			changed = undo_push_set<UNDO_T::TEXT_WRAP>(DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_NO_WRAP);
		}
		else if (sender == rmfi_menu_text_wrap_char() || sender == rmfi_popup_text_wrap_char()) {
			changed = undo_push_set<UNDO_T::TEXT_WRAP>(DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_CHARACTER);
		}
		if (changed) {
			//undo_menu_is_enabled();
			text_word_wrap_is_checked(m_main_page.m_text_word_wrap);
			main_draw();
		}
		status_bar_set_pos();
	}

	// 書体メニューの「余白...」が選択された.
	IAsyncAction MainPage::text_pad_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		const auto str_text_pad_horz{
			ResourceLoader::GetForCurrentView().GetString(L"str_text_pad_horz") + L": "
		};
		const auto str_text_pad_vert{
			ResourceLoader::GetForCurrentView().GetString(L"str_text_pad_vert") + L": "
		};
		const auto str_title{
			ResourceLoader::GetForCurrentView().GetString(L"str_text_padding")
		};
		const auto unit = m_len_unit;
		const auto dpi = m_main_d2d.m_logical_dpi;
		const auto g_len = m_prop_page.m_grid_base + 1.0f;
		wchar_t buf[32];

		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_prop_page.set_attr_to(&m_main_page);
		D2D1_SIZE_F pad;
		m_prop_page.get_text_pad(pad);

		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(pad.width);

		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, pad.width, dpi, g_len, buf);
		dialog_slider_0().Header(box_value(str_text_pad_horz + buf));

		dialog_slider_1().Maximum(MAX_VALUE);
		dialog_slider_1().TickFrequency(TICK_FREQ);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(pad.height);
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, pad.height, dpi, g_len, buf);
		dialog_slider_1().Header(box_value(str_text_pad_vert + buf));

		dialog_slider_0().Visibility(Visibility::Visible);
		dialog_slider_1().Visibility(Visibility::Visible);
		const Shape* prop = m_event_shape_pressed;
		if (prop == nullptr || typeid(*prop) != typeid(ShapeText)) {
			prop = &m_prop_page;
		}
		m_prop_page.m_shape_list.push_back(font_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), prop));
		cd_dialog_prop().Title(box_value(str_title));
		m_mutex_event.lock();
		{
			const auto revoker0{
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_text_pad_horz](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_0().Header(box_value(str_text_pad_horz + buf));
					D2D1_SIZE_F pad;
					m_prop_page.slist_back()->get_text_pad(pad);
					pad.width = static_cast<FLOAT>(val);
					if (m_prop_page.slist_back()->set_text_pad(pad)) {
						dialog_draw();
					}
				})
			};
			const auto revoker1{
				dialog_slider_1().ValueChanged(winrt::auto_revoke, [this, str_text_pad_vert](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_0().Header(box_value(str_text_pad_vert + buf));
					D2D1_SIZE_F pad;
					m_prop_page.slist_back()->get_text_pad(pad);
					pad.height = static_cast<FLOAT>(val);
					if (m_prop_page.slist_back()->set_text_pad(pad)) {
						dialog_draw();
					}
				})
			};
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				D2D1_SIZE_F samp_val;
				m_prop_page.slist_back()->get_text_pad(samp_val);
				undo_push_null();
				if (undo_push_set<UNDO_T::TEXT_PAD>(samp_val)) {
					//undo_menu_is_enabled();
					//xcvd_menu_is_enabled();
					main_draw();
				}
			}
		}
		slist_clear(m_prop_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		m_mutex_event.unlock();
	}

	// 書体メニューの「イタリック体」が選択された.
	/*
	void MainPage::font_style_italic_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (undo_push_set<UNDO_T::FONT_STYLE>(DWRITE_FONT_STYLE_ITALIC)) {
			undo_push_null();
			//undo_menu_is_enabled();
			//xcvd_menu_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	// 書体メニューの「字体「標準」が選択された.
	void MainPage::font_style_normal_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (undo_push_set<UNDO_T::FONT_STYLE>(DWRITE_FONT_STYLE_NORMAL)) {
			undo_push_null();
			//undo_menu_is_enabled();
			//xcvd_menu_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	// 書体メニューの「斜体」が選択された.
	void MainPage::font_style_oblique_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (undo_push_set<UNDO_T::FONT_STYLE>(DWRITE_FONT_STYLE_OBLIQUE)) {
			undo_push_null();
			//undo_menu_is_enabled();
			//xcvd_menu_is_enabled();
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
				if (undo_push_set<UNDO_T::FONT_WEIGHT>(new_val)) {
					undo_push_null();
					xcvd_menu_is_enabled();
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