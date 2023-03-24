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

	// 画像メニューの「画像の縦横比を維持」が選択された.
	void MainPage::image_keep_aspect_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		m_image_keep_aspect = !m_image_keep_aspect;
		status_bar_set_pos();
	}

	// 画像メニューの「画像の縦横比を維持」に印をつける.
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
		main_panel_size();
		page_draw();
		status_bar_set_pos();
	}

	// 画像メニューの「不透明度...」が選択された.
	IAsyncAction MainPage::image_opac_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		const winrt::hstring str_opacity = ResourceLoader::GetForCurrentView().GetString(L"str_opacity") + L": ";
		const winrt::hstring str_title = ResourceLoader::GetForCurrentView().GetString(L"str_image_opac");
		m_dialog_page.set_attr_to(&m_main_page);

		dialog_image_load_async(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()));

		const float val = static_cast<float>(conv_color_comp(m_dialog_page.m_image_opac));
		dialog_slider_0().Minimum(0.0);
		dialog_slider_0().Maximum(255.0);
		dialog_slider_0().TickFrequency(1.0);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val);
		wchar_t buf[32];
		conv_col_to_str(m_color_base, val, buf);
		dialog_slider_0().Header(box_value(str_opacity + buf));
		dialog_slider_0().Visibility(Visibility::Visible);
		cd_setting_dialog().Title(box_value(str_title));
		m_mutex_event.lock();
		{
			const auto revoker{
				dialog_slider_0().ValueChanged(
					winrt::auto_revoke,
					[this, str_opacity](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
						const float val = static_cast<float>(args.NewValue());
						wchar_t buf[32];
						conv_col_to_str(m_color_base, val, buf);
						dialog_slider_0().Header(box_value(str_opacity + buf));
						if (m_dialog_page.m_shape_list.back()->set_image_opacity(val / COLOR_MAX)) {
							dialog_draw();
						}
					}
				)
			};
			if (co_await cd_setting_dialog().ShowAsync() == ContentDialogResult::Primary) {
				float new_val;
				m_dialog_page.m_shape_list.back()->get_image_opacity(new_val);
				if (ustack_push_set<UNDO_T::IMAGE_OPAC>(new_val)) {
					ustack_push_null();
					ustack_is_enable();
					xcvd_is_enabled();
					page_draw();
				}
			}
		}
		slist_clear(m_dialog_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		page_draw();
		m_mutex_event.unlock();
	}

	/*
	// 値をスライダーのヘッダーに格納する.
	// U	操作の識別子
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	void MainPage::image_slider_set_header(const float val)
	{
		constexpr wchar_t R[]{ L"str_opacity" };
		wchar_t buf[32];
		conv_col_to_str(m_color_base, val, buf);
		const winrt::hstring text = ResourceLoader::GetForCurrentView().GetString(R) + L": " + buf;
		dialog_set_slider_header<0>(text);
	}

	// スライダーの値が変更された.
	// U	操作の識別子
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	void MainPage::image_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		const float val = static_cast<float>(args.NewValue());
		image_slider_set_header(val);
		if (m_dialog_page.m_shape_list.back()->set_image_opacity(val / COLOR_MAX)) {
			dialog_draw();
		}
	}
	*/

}