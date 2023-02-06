#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

	//------------------------------
// 値をスライダーのヘッダーに格納する.
// U	操作の識別子
// S	スライダーの番号
// val	格納する値
//------------------------------
	template <UNDO_ID U, int S>
	void MainPage::rotation_slider_set_header(const float val)
	{
		if constexpr (U == UNDO_ID::ROTATION) {
			constexpr wchar_t* SLIDER_HEADER[] = {
				L"str_rotation"
			};
			wchar_t buf[32];
			swprintf_s(buf, 32, L"%f°", val);
			const winrt::hstring text = ResourceLoader::GetForCurrentView().GetString(SLIDER_HEADER[S]) + L": " + buf;
			if constexpr (S == 0) {
				dialog_slider_0().Header(box_value(text));
			}
		}
	}

	//------------------------------
// スライダーの値が変更された.
// U	操作の識別子
// S	スライダーの番号
// args	ValueChanged で渡された引数
//------------------------------
	template <UNDO_ID U, int S> void MainPage::rotation_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		// 値をスライダーのヘッダーに格納する.
		if constexpr (U == UNDO_ID::ROTATION) {
			const float val = static_cast<float>(args.NewValue());
			float rot;
			m_dialog_page.m_shape_list.back()->get_rotation(rot);
			if constexpr (S == 0) {
				rotation_slider_set_header<U, S>(val);
				rot = static_cast<FLOAT>(val);
			}
			m_dialog_page.m_shape_list.back()->set_rotation(rot);
		}
		if (scp_dialog_panel().IsLoaded()) {
			dialog_draw();
		}
	}

	IAsyncAction MainPage::rotation_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		float rot;
		if (m_event_shape_pressed != nullptr &&
			m_event_shape_pressed->get_rotation(rot)) {
			dialog_slider_0().Maximum(180);
			dialog_slider_0().TickFrequency(1);
			dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
			dialog_slider_0().Value(rot);
			rotation_slider_set_header<UNDO_ID::ROTATION, 0>(rot);

			dialog_slider_0().Visibility(Visibility::Visible);
			const winrt::event_token slider_0_token{
				dialog_slider_0().ValueChanged({ this, &MainPage::rotation_slider_val_changed<UNDO_ID::ROTATION, 0> })
			};
			//const double r = M_PI * rot / 180.0;
			const auto samp_w = scp_dialog_panel().Width();
			const auto samp_h = scp_dialog_panel().Height();
			const auto padd = samp_w * 0.125;
			//const double c = cos(-r);
			//const double s = sin(-r);
			//const double px = c * samp_w * 0.5 + s * padd;
			//const double py = -s * samp_w * 0.5 + c * padd;
			//const double qx = c * (samp_w - padd) + s * samp_h * 0.5;
			//const double qy = -s * (samp_w - padd) + c * samp_h * 0.5;
			//const D2D1_POINT_2F b_pos{ px, py };
			//const D2D1_POINT_2F b_vec{ qx, qy };
			const D2D1_POINT_2F b_pos{ padd, padd };
			const D2D1_POINT_2F b_vec{ samp_h - padd, samp_w - padd };
			m_dialog_page.m_shape_list.push_back(new ShapeQCircle(b_pos, b_vec, rot, &m_dialog_page));
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif

			//cd_setting_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_rotation")));
			const ContentDialogResult d_result{
				co_await cd_setting_dialog().ShowAsync()
			};
			if (d_result == ContentDialogResult::Primary) {
				float samp_val;
				m_dialog_page.m_shape_list.back()->get_rotation(samp_val);
				ustack_push_set<UNDO_ID::ROTATION>(m_event_shape_pressed, samp_val);
				ustack_push_null();
				xcvd_is_enabled();
				page_draw();
			}
			slist_clear(m_dialog_page.m_shape_list);
			dialog_slider_0().Visibility(Visibility::Collapsed);
			dialog_slider_0().ValueChanged(slider_0_token);
			status_bar_set_pos();
		}
	}
}