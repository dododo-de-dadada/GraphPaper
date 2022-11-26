//-------------------------------
//　MainPage_text.cpp
//　文字列の配置
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	//using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	//using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	constexpr float TEXT_LINE_SP_DELTA = 2.0f;	// 行の高さの変分 (DPIs)

	// 見本の図形を作成する.
	static void text_create_sample_shape(const float panel_w, const float panel_h, ShapeSheet& prop_sheet);

	// 見本の図形を作成する.
	// panel_w	見本を表示するパネルの幅
	// panel_h	見本を表示するパネルの高さ
	// prop_sheet	見本を表示するシート
	static void text_create_sample_shape(const float panel_w, const float panel_h, ShapeSheet& prop_sheet)
	{
		const auto padd_w = panel_w * 0.125;
		const auto padd_h = panel_h * 0.25;
		const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd_w), static_cast<FLOAT>(padd_h) };
		const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(panel_w - 2.0 * padd_w), static_cast<FLOAT>(panel_w - 2.0 * padd_h) };
		const auto pang = ResourceLoader::GetForCurrentView().GetString(L"str_pangram");
		const wchar_t* text = nullptr;
		if (pang.empty()) {
			text = L"The quick brown fox jumps over a lazy dog.";
		}
		else {
			text = pang.c_str();
		}
		prop_sheet.m_shape_list.push_back(new ShapeText(b_pos, b_vec, wchar_cpy(text), &prop_sheet));
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
	}

	// 書体メニューの「枠を文字列に合わせる」が選択された.
	void MainPage::text_frame_fit_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		auto flag = false;
		for (auto s : m_main_sheet.m_shape_list) {
			if (s->is_deleted()) {
				continue;
			}
			else if (!s->is_selected()) {
				continue;
			}
			else if (typeid(*s) != typeid(ShapeText)) {
				continue;
			}
			auto u = new UndoForm(s, ANC_TYPE::ANC_SE);
			if (static_cast<ShapeText*>(s)->frame_fit(m_main_sheet.m_grid_snap ? m_main_sheet.m_grid_base + 1.0f : 0.0f)) {
				m_ustack_undo.push_back(u);
				if (!flag) {
					flag = true;
				}
			}
			else {
				delete u;
			}
		}
		if (flag) {
			ustack_push_null();
			sheet_panle_size();
			sheet_draw();
		}
	}

	// 書体メニューの「段落のそろえ」が選択された.
	void MainPage::text_par_align_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_PARAGRAPH_ALIGNMENT val;
		if (sender == rmfi_text_align_top()) {
			val = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		}
		else if (sender == rmfi_text_align_bot()) {
			val = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR;
		}
		else if (sender == rmfi_text_align_mid()) {
			val = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		}
		else {
			return;
		}
		text_par_align_is_checked(val);
		if (ustack_push_set<UNDO_OP::TEXT_PAR_ALIGN>(val)) {
			ustack_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// 書体メニューの「段落のそろえ」に印をつける.
	// val	段落のそろえ
	void MainPage::text_par_align_is_checked(const DWRITE_PARAGRAPH_ALIGNMENT val)
	{
		rmfi_text_align_top().IsChecked(val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		//rmfi_text_align_top_2().IsChecked(val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		rmfi_text_align_bot().IsChecked(val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		//rmfi_text_align_bot_2().IsChecked(val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		rmfi_text_align_mid().IsChecked(val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		//rmfi_text_align_mid_2().IsChecked(val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	// 書体メニューの「文字列のそろえ」が選択された.
	void MainPage::text_align_t_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_TEXT_ALIGNMENT val;
		if (sender == rmfi_text_align_left()) {
			val = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
		}
		else if (sender == rmfi_text_align_right()) {
			val = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING;
		}
		else if (sender == rmfi_text_align_center()) {
			val = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER;
		}
		else if (sender == rmfi_text_align_just()) {
			val = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
		}
		else {
			return;
		}
		text_align_t_is_checked(val);
		if (ustack_push_set<UNDO_OP::TEXT_ALIGN_T>(val)) {
			ustack_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// 書体メニューの「文字列のそろえ」に印をつける.
	// t_align	文字列のそろえ
	void MainPage::text_align_t_is_checked(const DWRITE_TEXT_ALIGNMENT val)
	{
		rmfi_text_align_left().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
		//rmfi_text_align_left_2().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
		rmfi_text_align_right().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING);
		//rmfi_text_align_right_2().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING);
		rmfi_text_align_center().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
		//rmfi_text_align_center_2().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
		rmfi_text_align_just().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
		//rmfi_text_align_just_2().IsChecked(val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED);

	}

	// 書体メニューの「行間」>「行間...」が選択された.
	IAsyncAction MainPage::text_line_sp_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_prop_sheet.set_attr_to(&m_main_sheet);
		float val;
		m_prop_sheet.get_text_line_sp(val);

		prop_slider_0().Maximum(MAX_VALUE);
		prop_slider_0().TickFrequency(TICK_FREQ);
		prop_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_0().Value(val);
		text_slider_set_header<UNDO_OP::TEXT_LINE_SP, 0>(val);
		prop_slider_0().Visibility(UI_VISIBLE);

		const auto slider_0_token = prop_slider_0().ValueChanged({ this, &MainPage::text_slider_val_changed<UNDO_OP::TEXT_LINE_SP, 0> });
		text_create_sample_shape(static_cast<float>(scp_prop_panel().Width()), static_cast<float>(scp_prop_panel().Height()), m_prop_sheet);

		cd_prop_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_text_line_sp")));
		const auto d_result = co_await cd_prop_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float samp_val;
			m_prop_sheet.m_shape_list.back()->get_text_line_sp(samp_val);
			if (ustack_push_set<UNDO_OP::TEXT_LINE_SP>(samp_val)) {
				ustack_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		slist_clear(m_prop_sheet.m_shape_list);
		prop_slider_0().Visibility(UI_COLLAPSED);
		prop_slider_0().ValueChanged(slider_0_token);
		sheet_draw();
	}

	// 書体メニューの「余白」が選択された.
	IAsyncAction MainPage::text_padding_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_prop_sheet.set_attr_to(&m_main_sheet);
		D2D1_SIZE_F padding;
		m_prop_sheet.get_text_padding(padding);

		prop_slider_0().Maximum(MAX_VALUE);
		prop_slider_0().TickFrequency(TICK_FREQ);
		prop_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_0().Value(padding.width);
		text_slider_set_header<UNDO_OP::TEXT_MARGIN, 0>(padding.width);

		prop_slider_1().Maximum(MAX_VALUE);
		prop_slider_1().TickFrequency(TICK_FREQ);
		prop_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_1().Value(padding.height);
		text_slider_set_header<UNDO_OP::TEXT_MARGIN, 1>(padding.height);

		prop_slider_0().Visibility(UI_VISIBLE);
		prop_slider_1().Visibility(UI_VISIBLE);
		const auto slider_0_token = prop_slider_0().ValueChanged({ this, &MainPage::text_slider_val_changed<UNDO_OP::TEXT_MARGIN, 0> });
		const auto slider_1_token = prop_slider_1().ValueChanged({ this, &MainPage::text_slider_val_changed<UNDO_OP::TEXT_MARGIN, 1> });

		text_create_sample_shape(static_cast<float>(scp_prop_panel().Width()), static_cast<float>(scp_prop_panel().Height()), m_prop_sheet);

		cd_prop_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_text_padding")));
		const auto d_result = co_await cd_prop_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_SIZE_F samp_val;
			m_prop_sheet.m_shape_list.back()->get_text_padding(samp_val);
			if (ustack_push_set<UNDO_OP::TEXT_MARGIN>(samp_val)) {
				ustack_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		slist_clear(m_prop_sheet.m_shape_list);
		prop_slider_0().Visibility(UI_COLLAPSED);
		prop_slider_1().Visibility(UI_COLLAPSED);
		prop_slider_0().ValueChanged(slider_0_token);
		prop_slider_1().ValueChanged(slider_1_token);
		sheet_draw();
	}

	// 値をスライダーのヘッダーに格納する.
	// U	操作の種類
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <UNDO_OP U, int S>
	void MainPage::text_slider_set_header(const float val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring text;
		if constexpr (U == UNDO_OP::TEXT_MARGIN) {
			constexpr wchar_t* HEADER[] = { L"str_text_pad_horzorz", L"str_text_pad_vertert" };
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, val, m_main_sheet.m_d2d.m_logical_dpi, m_prop_sheet.m_grid_base + 1.0f, buf);
			text = ResourceLoader::GetForCurrentView().GetString(HEADER[S]) + L": " + buf;
		}
		if constexpr (U == UNDO_OP::TEXT_LINE_SP) {
			constexpr wchar_t HEADER[] = L"str_text_line_sp";
			if (val >= FLT_MIN) {
				wchar_t buf[32];
				conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, val, m_main_sheet.m_d2d.m_logical_dpi, m_prop_sheet.m_grid_base + 1.0f, buf);
				text = ResourceLoader::GetForCurrentView().GetString(HEADER) + L": " + buf;
			}
			else {
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				text = r_loader.GetString(HEADER) + L": " + r_loader.GetString(L"str_def_val");
			}
		}
		prop_set_slider_header<S>(text);
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S>
	void MainPage::text_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::TEXT_LINE_SP) {
			const float val = static_cast<float>(args.NewValue());
			text_slider_set_header<U, S>(val);
			//m_sample_shape->set_text_line_sp(val);
			m_prop_sheet.m_shape_list.back()->set_text_line_sp(val);
		}
		if constexpr (U == UNDO_OP::TEXT_MARGIN) {
			const float val = static_cast<float>(args.NewValue());
			text_slider_set_header<U, S>(val);
			D2D1_SIZE_F padding;
			m_prop_sheet.m_shape_list.back()->get_text_padding(padding);
			if constexpr (S == 0) {
				padding.width = static_cast<FLOAT>(val);
			}
			if constexpr (S == 1) {
				padding.height = static_cast<FLOAT>(val);
			}
			m_prop_sheet.m_shape_list.back()->set_text_padding(padding);
		}
		if (scp_prop_panel().IsLoaded()) {
			prop_sample_draw();
		}
	}

}