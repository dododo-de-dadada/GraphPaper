//-------------------------------
// MainPage_arrow.cpp
// 矢じるしの形式と寸法
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t TITLE_ARROWHEAD[] = L"str_arrow_size";

	//	値をスライダーのヘッダーに格納する.
	//	value	値
	template <UNDO_OP U, int S> void MainPage::arrow_slider_set_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		if constexpr (U == UNDO_OP::ARROW_SIZE) {
			constexpr wchar_t* HEADER[] = {
				L"str_arrow_width",
				L"str_arrow_length",
				L"str_arrow_offset"
			};
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_misc_len_unit, value, m_sheet_dx.m_logical_dpi, m_sheet_main.m_grid_base + 1.0f, buf);
			const auto text = ResourceLoader::GetForCurrentView().GetString(HEADER[S]) + L": " + buf;
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
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S> void MainPage::arrow_slider_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		// 値をスライダーのヘッダーに格納する.
		if constexpr (U == UNDO_OP::ARROW_SIZE) {
			const auto value = static_cast<float>(args.NewValue());
			ARROW_SIZE a_size;
			m_sample_shape->get_arrow_size(a_size);
			if constexpr (S == 0) {
				arrow_slider_set_header<U, S>(value);
				a_size.m_width = static_cast<FLOAT>(value);
			}
			else if constexpr (S == 1) {
				arrow_slider_set_header<U, S>(value);
				a_size.m_length = static_cast<FLOAT>(value);
			}
			else if constexpr (S == 2) {
				arrow_slider_set_header<U, S>(value);
				a_size.m_offset = static_cast<FLOAT>(value);
			}
			m_sample_shape->set_arrow_size(a_size);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// 線枠メニューの「矢じるしの大きさ」が選択された.
	IAsyncAction MainPage::arrow_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		ARROW_SIZE a_size;
		m_sample_sheet.get_arrow_size(a_size);

		sample_slider_0().Maximum(MAX_VALUE);
		sample_slider_0().TickFrequency(TICK_FREQ);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(a_size.m_width);
		arrow_slider_set_header<UNDO_OP::ARROW_SIZE, 0>(a_size.m_width);
		sample_slider_1().Maximum(MAX_VALUE);
		sample_slider_1().TickFrequency(TICK_FREQ);
		sample_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_1().Value(a_size.m_length);
		arrow_slider_set_header<UNDO_OP::ARROW_SIZE, 1>(a_size.m_length);
		sample_slider_2().Maximum(MAX_VALUE);
		sample_slider_2().TickFrequency(TICK_FREQ);
		sample_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_2().Value(a_size.m_offset);
		arrow_slider_set_header<UNDO_OP::ARROW_SIZE, 2>(a_size.m_offset);

		sample_slider_0().Visibility(UI_VISIBLE);
		sample_slider_1().Visibility(UI_VISIBLE);
		sample_slider_2().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::arrow_slider_value_changed< UNDO_OP::ARROW_SIZE, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::arrow_slider_value_changed< UNDO_OP::ARROW_SIZE, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::arrow_slider_value_changed< UNDO_OP::ARROW_SIZE, 2> });
		m_sample_type = SAMPLE_TYPE::STROKE;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_ARROWHEAD)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			ARROW_SIZE sample_value;
			m_sample_shape->get_arrow_size(sample_value);
			if (ustack_push_set<UNDO_OP::ARROW_SIZE>(sample_value)) {
				ustack_push_null();
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
		sample_slider_2().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sample_slider_2().ValueChanged(slider_2_token);
		sheet_draw();
	}

	// 線枠メニューの「矢じるしの種類」のサブ項目が選択された.
	void MainPage::arrow_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		ARROW_STYLE a_style;
		if (sender == rmfi_arrow_style_none() || sender == rmfi_arrow_style_none_2()) {
			a_style = ARROW_STYLE::NONE;
		}
		else if (sender == rmfi_arrow_style_opened() || sender == rmfi_arrow_style_opened_2()) {
			a_style = ARROW_STYLE::OPENED;
		}
		else if (sender == rmfi_arrow_style_filled() || sender == rmfi_arrow_style_filled_2()) {
			a_style = ARROW_STYLE::FILLED;
		}
		else {
			return;
		}
		mfi_arrow_size().IsEnabled(a_style != ARROW_STYLE::NONE);
		mfi_arrow_size_2().IsEnabled(a_style != ARROW_STYLE::NONE);
		if (ustack_push_set<UNDO_OP::ARROW_STYLE>(a_style)) {
			ustack_push_null();
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// 線枠メニューの「矢じるしの種類」に印をつける.
	// a_style	矢じるしの形式
	void MainPage::arrow_style_is_checked(const ARROW_STYLE value)
	{
		rmfi_arrow_style_none().IsChecked(value == ARROW_STYLE::NONE);
		rmfi_arrow_style_none_2().IsChecked(value == ARROW_STYLE::NONE);
		rmfi_arrow_style_opened().IsChecked(value == ARROW_STYLE::OPENED);
		rmfi_arrow_style_opened_2().IsChecked(value == ARROW_STYLE::OPENED);
		rmfi_arrow_style_filled().IsChecked(value == ARROW_STYLE::FILLED);
		rmfi_arrow_style_filled_2().IsChecked(value == ARROW_STYLE::FILLED);
		mfi_arrow_size().IsEnabled(value != ARROW_STYLE::NONE);
		mfi_arrow_size_2().IsEnabled(value != ARROW_STYLE::NONE);
	}

}