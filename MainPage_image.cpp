//-------------------------------
// MainPage_image.cpp
// 画像
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

	// 画像メニューの「縦横比を変えない」が選択された.
	void MainPage::image_keep_aspect_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		m_image_keep_aspect = !m_image_keep_aspect;
		status_bar_set_pos();
	}

	// 画像メニューの「縦横比を変えない」に印をつける.
	void MainPage::image_keep_aspect_is_checked(const bool keep_aspect)
	{
		tmfi_image_keep_aspect().IsChecked(keep_aspect);
	}

	// 画像メニューの「原画像に戻す」が選択された.
	void MainPage::image_revert_to_original_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		for (Shape* const s : m_main_page.m_shape_list) {
			if (s->is_deleted() || !s->is_selected() || typeid(*s) != typeid(ShapeImage)) {
				continue;
			}
			// 画像の現在の位置や大きさ、不透明度を操作スタックにプッシュする.
			ustack_push_image(s);
			static_cast<ShapeImage*>(s)->revert();
		}
		ustack_push_null();
		page_panel_size();
		page_draw();
		status_bar_set_pos();
	}

	// 画像メニューの「不透明度...」が選択された.
	IAsyncAction MainPage::image_opac_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_dialog_page.set_attr_to(&m_main_page);
		const auto val = m_dialog_page.m_image_opac * COLOR_MAX;
		dialog_slider_0().Maximum(255.0);
		dialog_slider_0().TickFrequency(1.0);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val);
		image_slider_set_header<UNDO_ID::IMAGE_OPAC, 0>(val);
		dialog_check_box().IsChecked(m_dialog_page.m_image_opac_importing);

		dialog_slider_0().Visibility(Visibility::Visible);
		dialog_check_box().Visibility(Visibility::Visible);
		const auto slider_0_token = dialog_slider_0().ValueChanged({ this, &MainPage::image_slider_val_changed<UNDO_ID::IMAGE_OPAC, 0> });

		dialog_image_load_async(static_cast<float>(scp_dialog_panel().Width()), static_cast<float>(scp_dialog_panel().Height()));

		cd_setting_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_image_opac")));
		m_mutex_event.lock();
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float samp_val;
			m_dialog_page.m_shape_list.back()->get_image_opacity(samp_val);
			ustack_push_set<UNDO_ID::IMAGE_OPAC>(&m_main_page, samp_val);
			if (ustack_push_set<UNDO_ID::IMAGE_OPAC>(samp_val)) {
				ustack_push_null();
				ustack_is_enable();
				xcvd_is_enabled();
				page_draw();
			}
		}
		slist_clear(m_dialog_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_0().ValueChanged(slider_0_token);
		dialog_check_box().Visibility(Visibility::Collapsed);
		page_draw();
		m_mutex_event.unlock();
	}

	// 値をスライダーのヘッダーに格納する.
	// U	操作の識別子
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	template <UNDO_ID U, int S>
	void MainPage::image_slider_set_header(const float val)
	{
		winrt::hstring text;

		if constexpr (U == UNDO_ID::IMAGE_OPAC) {
			constexpr wchar_t R[]{ L"str_opacity" };
			wchar_t buf[32];
			conv_col_to_str(m_color_code, val, buf);
			text = ResourceLoader::GetForCurrentView().GetString(R) + L": " + buf;
		}
		dialog_set_slider_header<S>(text);
	}

	// スライダーの値が変更された.
	// U	操作の識別子
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	template <UNDO_ID U, int S>
	void MainPage::image_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_ID::IMAGE_OPAC) {
			if constexpr (S == 0) {
				const float val = static_cast<float>(args.NewValue());
				image_slider_set_header<U, S>(val);
				m_dialog_page.m_shape_list.back()->set_image_opacity(val / COLOR_MAX);
			}
		}
		if (scp_dialog_panel().IsLoaded()) {
			dialog_draw();
		}
	}

}