//-------------------------------
// MainPage_image.cpp
// 画像
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	void MainPage::image_keep_aspect_is_checked(const bool keep_aspect)
	{
		tmfi_image_keep_aspect().IsChecked(keep_aspect);
		tmfi_image_keep_aspect_2().IsChecked(keep_aspect);
	}

	// 画像メニューの「縦横比を変えない」が選択された.
	void MainPage::image_keep_aspect_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		m_image_keep_aspect = !m_image_keep_aspect;
	}

	// 画像メニューの「元の大きさに戻す」が選択された.
	void MainPage::image_resize_origin_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		for (auto s : m_sheet_main.m_list_shapes) {
			if (s->is_deleted() || !s->is_selected() || typeid(*s) != typeid(ShapeImage)) {
				continue;
			}
			auto bm = static_cast<ShapeImage*>(s);
			ustack_push_anch(bm, ANCH_TYPE::ANCH_SHEET);
			bm->resize_origin();
		}
		ustack_push_null();
		sheet_panle_size();
		sheet_draw();
	}

	// 画像メニューの「不透明度...」が選択された.
	IAsyncAction MainPage::image_opac_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		const auto value = m_sample_sheet.m_image_opac * COLOR_MAX;
		sample_slider_0().Maximum(255.0);
		sample_slider_0().TickFrequency(1.0);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(value);
		image_slider_set_header<UNDO_OP::IMAGE_OPAC, 0>(value);

		sample_slider_0().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::image_slider_value_changed<UNDO_OP::IMAGE_OPAC, 0> });
		m_sample_type = SAMPLE_TYPE::IMAGE;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_image_opac")));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_value;
			//m_sample_shape->get_image_opacity(sample_value);
			m_sample_sheet.m_list_shapes.back()->get_image_opacity(sample_value);
			if (ustack_push_set<UNDO_OP::IMAGE_OPAC>(sample_value)) {
				ustack_push_null();
				ustack_is_enable();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		//delete m_sample_shape;
		delete m_sample_sheet.m_list_shapes.back();
		m_sample_sheet.m_list_shapes.clear();
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		//m_sample_shape = nullptr;
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sheet_draw();
	}

	// 値をスライダーのヘッダーに格納する.
	template <UNDO_OP U, int S> void MainPage::image_slider_set_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring text;

		if constexpr (U == UNDO_OP::IMAGE_OPAC) {
			constexpr wchar_t R[]{ L"str_opacity" };
			wchar_t buf[32];
			conv_col_to_str(m_misc_color_code, value, buf);
			text = ResourceLoader::GetForCurrentView().GetString(R) + L": " + buf;
		}
		if constexpr (S == 0) {
			sample_slider_0().Header(box_value(text));
		}
	}

	// スライダーの値が変更された.
	// U	操作の種類
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_OP U, int S> void MainPage::image_slider_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::IMAGE_OPAC) {
			if constexpr (S == 0) {
				const float value = static_cast<float>(args.NewValue());
				image_slider_set_header<U, S>(value);
				//m_sample_shape->set_image_opacity(value / COLOR_MAX);
				m_sample_sheet.m_list_shapes.back()->set_image_opacity(value / COLOR_MAX);
			}
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

}