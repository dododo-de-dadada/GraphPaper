//-------------------------------
//　MainPage_text.cpp
//　文字列の配置
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t DLG_TITLE[] = L"str_text";
	//constexpr float SLIDER_STEP = 0.5f;
	constexpr float TEXT_LINE_H_DELTA = 2.0f;	// 行の高さの変分 (DPIs)

	// 編集メニューの「枠の大きさを合わせる」が選択された.
	void MainPage::edit_text_frame_click(IInspectable const&, RoutedEventArgs const&)
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
			if (static_cast<ShapeText*>(s)->adjust_bbox(m_sheet_main.m_grid_snap ? m_sheet_main.m_grid_base + 1.0f : 0.0f)) {
				m_stack_undo.push_back(u);
				if (!flag) {
					flag = true;
				}
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

	// 書体メニューの「段落のそろえ」が選択された.
	void MainPage::text_align_p_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_PARAGRAPH_ALIGNMENT value;
		if (sender == rmfi_text_align_top() || sender == rmfi_text_align_top_2()) {
			value = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		}
		else if (sender == rmfi_text_align_bot() || sender == rmfi_text_align_bot_2()) {
			value = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR;
		}
		else if (sender == rmfi_text_align_mid() || sender == rmfi_text_align_mid_2()) {
			value = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		}
		else {
			return;
		}
		if (undo_push_set<UNDO_OP::TEXT_ALIGN_P>(value)) {
			undo_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// 書体メニューの「段落のそろえ」に印をつける.
	// value	段落のそろえ
	void MainPage::text_align_p_is_checked(const DWRITE_PARAGRAPH_ALIGNMENT value)
	{
		rmfi_text_align_top().IsChecked(value == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		rmfi_text_align_top_2().IsChecked(value == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		rmfi_text_align_bot().IsChecked(value == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		rmfi_text_align_bot_2().IsChecked(value == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		rmfi_text_align_mid().IsChecked(value == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		rmfi_text_align_mid_2().IsChecked(value == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	// 書体メニューの「文字列のそろえ」が選択された.
	void MainPage::text_align_t_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_TEXT_ALIGNMENT value;
		if (sender == rmfi_text_align_left() || sender == rmfi_text_align_left_2()) {
			value = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
		}
		else if (sender == rmfi_text_align_right() || sender == rmfi_text_align_right_2()) {
			value = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING;
		}
		else if (sender == rmfi_text_align_center() || sender == rmfi_text_align_center_2()) {
			value = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER;
		}
		else if (sender == rmfi_text_align_just() || sender == rmfi_text_align_just_2()) {
			value = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
		}
		else {
			return;
		}
		if (undo_push_set<UNDO_OP::TEXT_ALIGN_T>(value)) {
			undo_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// 書体メニューの「文字列のそろえ」に印をつける.
	// t_align	文字列のそろえ
	void MainPage::text_align_t_is_checked(const DWRITE_TEXT_ALIGNMENT value)
	{
		rmfi_text_align_left().IsChecked(value == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
		rmfi_text_align_left_2().IsChecked(value == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
		rmfi_text_align_right().IsChecked(value == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING);
		rmfi_text_align_right_2().IsChecked(value == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING);
		rmfi_text_align_center().IsChecked(value == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
		rmfi_text_align_center_2().IsChecked(value == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
		rmfi_text_align_just().IsChecked(value == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
		rmfi_text_align_just_2().IsChecked(value == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED);

	}

	// 書体メニューの「行間」>「行間...」が選択された.
	IAsyncAction MainPage::text_line_sp_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		float value;
		m_sample_sheet.get_text_line_sp(value);

		sample_slider_0().Maximum(127.5);
		sample_slider_0().TickFrequency(0.5);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(value);
		text_slider_set_header<UNDO_OP::TEXT_LINE_H, 0>(value);
		sample_slider_0().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::text_slider_value_changed<UNDO_OP::TEXT_LINE_H, 0> });
		m_sample_type = SAMPLE_TYPE::FONT;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_value;
			m_sample_shape->get_text_line_sp(sample_value);
			if (undo_push_set<UNDO_OP::TEXT_LINE_H>(sample_value)) {
				undo_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sheet_draw();
	}

	// 書体メニューの「行間」が選択された.
	/*
	void MainPage::text_line_sp_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		float value;
		m_sheet_main.get_text_line_sp(value);
		if (sender == mfi_text_line_sp_con() || sender == mfi_text_line_sp_con_2()) {
			value = max(value - m_sheet_main.m_font_size, 0.0f);
		}
		else if (sender == mfi_text_line_sp_exp() || sender == mfi_text_line_sp_exp_2()) {
			value = value + m_sheet_main.m_font_size;
		}
		if (undo_push_set<UNDO_OP::TEXT_LINE_H>(value)) {
			undo_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}
	*/
	// 書体メニューの「余白」が選択された.
	IAsyncAction MainPage::text_margin_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		D2D1_SIZE_F t_margin;
		m_sample_sheet.get_text_margin(t_margin);

		sample_slider_0().Maximum(127.5);
		sample_slider_0().TickFrequency(0.5);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(t_margin.width);
		text_slider_set_header<UNDO_OP::TEXT_MARGIN, 0>(t_margin.width);

		sample_slider_1().Maximum(127.5);
		sample_slider_1().TickFrequency(0.5);
		sample_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_1().Value(t_margin.height);
		text_slider_set_header<UNDO_OP::TEXT_MARGIN, 1>(t_margin.height);

		sample_slider_0().Visibility(UI_VISIBLE);
		sample_slider_1().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::text_slider_value_changed<UNDO_OP::TEXT_MARGIN, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::text_slider_value_changed<UNDO_OP::TEXT_MARGIN, 1> });
		m_sample_type = SAMPLE_TYPE::FONT;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_SIZE_F sample_value;
			m_sample_shape->get_text_margin(sample_value);
			if (undo_push_set<UNDO_OP::TEXT_MARGIN>(sample_value)) {
				undo_push_null();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_1().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sheet_draw();
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S> void MainPage::text_slider_set_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring text;
		if constexpr (U == UNDO_OP::TEXT_MARGIN) {
			constexpr wchar_t* HEADER[] = { L"str_text_mar_horzorz", L"str_text_mar_vertert" };
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_misc_len_unit, value, m_sheet_dx.m_logical_dpi, m_sample_sheet.m_grid_base + 1.0f, buf);
			text = ResourceLoader::GetForCurrentView().GetString(HEADER[S]) + L": " + buf;
		}
		if constexpr (U == UNDO_OP::TEXT_LINE_H) {
			constexpr wchar_t HEADER[] = L"str_text_line_sp";
			if (value >= FLT_MIN) {
				wchar_t buf[32];
				conv_len_to_str<LEN_UNIT_SHOW>(m_misc_len_unit, value, m_sheet_dx.m_logical_dpi, m_sample_sheet.m_grid_base + 1.0f, buf);
				text = ResourceLoader::GetForCurrentView().GetString(HEADER) + L": " + buf;
			}
			else {
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				text = r_loader.GetString(HEADER) + L": " + r_loader.GetString(L"str_def_val");
			}
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
	template <UNDO_OP U, int S> void MainPage::text_slider_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::TEXT_LINE_H) {
			const float value = static_cast<float>(args.NewValue());
			text_slider_set_header<U, S>(value);
			m_sample_shape->set_text_line_sp(value);
		}
		if constexpr (U == UNDO_OP::TEXT_MARGIN) {
			const float value = static_cast<float>(args.NewValue());
			text_slider_set_header<U, S>(value);
			D2D1_SIZE_F margin;
			m_sample_shape->get_text_margin(margin);
			if constexpr (S == 0) {
				margin.width = static_cast<FLOAT>(value);
			}
			if constexpr (S == 1) {
				margin.height = static_cast<FLOAT>(value);
			}
			m_sample_shape->set_text_margin(margin);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

}