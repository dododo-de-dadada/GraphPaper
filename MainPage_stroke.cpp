//------------------------------
// MainPage_stroke.cpp
// 線枠
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

	// 見本の図形を作成する.
	static void stroke_create_sample_shape(
		const float p_width, const float p_height, ShapePage& page);

	// 見本の図形を作成する.
	// p_width	見本を表示するパネルの幅
	// p_height	見本を表示するパネルの高さ
	// page	見本を表示するシート
	static void stroke_create_sample_shape(const float p_width, const float p_height, ShapePage& page)
	{
		const auto mar = p_width * 0.125;	// 余白
		const D2D1_POINT_2F start{
			static_cast<FLOAT>(mar), static_cast<FLOAT>(mar)
		};
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(p_width - 2.0 * mar), static_cast<FLOAT>(p_height - 2.0 * mar)
		};
		page.m_shape_list.push_back(new ShapeLine(start, pos, &page));
		page.m_shape_list.back()->set_select(true);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
	}

	// 線枠メニューの「太さ」のサブ項目が選択された.
	void MainPage::stroke_width_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		float s_width;
		if (sender == rmfi_stroke_width_0px()) {
			s_width = 0.0f;
		}
		else if (sender == rmfi_stroke_width_1px()) {
			s_width = 1.0f;
		}
		else if (sender == rmfi_stroke_width_2px()) {
			s_width = 2.0f;
		}
		else if (sender == rmfi_stroke_width_3px()) {
			s_width = 3.0f;
		}
		else if (sender == rmfi_stroke_width_4px()) {
			s_width = 4.0f;
		}
		else {
			auto _{ winrt::hresult_not_implemented() };
			return;
		}
		stroke_width_is_checked(s_width);
		if (ustack_push_set<UNDO_T::STROKE_WIDTH>(s_width)) {
			ustack_push_null();
			xcvd_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	void MainPage::stroke_width_is_checked(const float s_width) noexcept
	{
		rmfi_stroke_width_0px().IsChecked(s_width == 0.0f);
		rmfi_stroke_width_1px().IsChecked(s_width == 1.0f);
		rmfi_stroke_width_2px().IsChecked(s_width == 2.0f);
		rmfi_stroke_width_3px().IsChecked(s_width == 3.0f);
		rmfi_stroke_width_4px().IsChecked(s_width == 4.0f);
		rmfi_stroke_width_other().IsChecked(s_width != 1.0f && s_width != 2.0f && s_width != 3.0f && s_width != 4.0f);
	}

	// 線枠メニューの「太さ」>「その他」が選択された.
	IAsyncAction MainPage::stroke_width_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		const winrt::hstring str_stroke_width{ ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": "};
		const winrt::hstring str_title{ ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") };
		m_prop_page.set_attr_to(&m_main_page);
		stroke_create_sample_shape(static_cast<float>(scp_prop_panel().Width()), static_cast<float>(scp_prop_panel().Height()), m_prop_page);
		float s_width;
		m_prop_page.get_stroke_width(s_width);
		const auto dpi = m_prop_d2d.m_logical_dpi;
		const auto g_len = m_prop_page.m_grid_base + 1.0f;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(m_len_unit, s_width, dpi, g_len, buf);
		dialog_slider_0().Minimum(0.0);
		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().StepFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::StepValues);
		dialog_slider_0().Value(s_width);
		dialog_slider_0().Header(box_value(str_stroke_width + buf));
		dialog_slider_0().Visibility(Visibility::Visible);

		cd_setting_dialog().Title(box_value(str_title));
		m_mutex_event.lock();
		{
			const auto revoker0{
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_stroke_width](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_0().Header(box_value(str_stroke_width + buf));
					if (m_prop_page.m_shape_list.back()->set_stroke_width(val)) {
						prop_dialog_draw();
					}
				})
			};
			if (co_await cd_setting_dialog().ShowAsync() == ContentDialogResult::Primary) {
				float new_val;
				m_prop_page.m_shape_list.back()->get_stroke_width(new_val);
				stroke_width_is_checked(new_val);
				if (ustack_push_set<UNDO_T::STROKE_WIDTH>(new_val)) {
					ustack_push_null();
					xcvd_is_enabled();
					main_draw();
				}
			}
		}
		slist_clear(m_prop_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_0().StepFrequency(1.0);
		dialog_slider_0().Maximum(255.0);
		m_mutex_event.unlock();

	}

}