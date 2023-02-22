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

	IAsyncAction MainPage::rotation_click_async(Shape* s)
	{
		float rot;
		if (s != nullptr && s->get_rotation(rot)) {
			m_mutex_event.lock();
			m_dialog_page.set_attr_to(&m_main_page);

			const auto ds0_min = dialog_slider_0().Minimum();
			const auto ds0_max = dialog_slider_0().Maximum();
			const auto ds0_freq = dialog_slider_0().TickFrequency();
			const auto ds0_snap = dialog_slider_0().SnapsTo();
			const auto ds0_val = dialog_slider_0().Value();
			const auto ds0_vis = dialog_slider_0().Visibility();

			dialog_slider_0().Minimum(-44.9);
			dialog_slider_0().Maximum(44.9);
			dialog_slider_0().TickFrequency(0.5);
			dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
			dialog_slider_0().Value(rot);
			dialog_slider_0().Visibility(Visibility::Visible);

			const winrt::event_token ds0_tok{
				dialog_slider_0().ValueChanged({ this, &MainPage::rotation_slider_val_changed<UNDO_ID::ROTATION, 0> })
			};
			rotation_slider_set_header<UNDO_ID::ROTATION, 0>(rot);
			const auto samp_w = scp_dialog_panel().Width();
			const auto samp_h = scp_dialog_panel().Height();
			const auto center = samp_w * 0.5;
			const auto padd = samp_w * 0.125;
			const auto rx = (samp_w - padd) * 0.5;
			const auto ry = (samp_h - padd) * 0.5;
			const D2D1_POINT_2F start{
				static_cast<FLOAT>(center), static_cast<FLOAT>(padd)
			};
			const D2D1_POINT_2F b_vec{
				static_cast<FLOAT>(rx), static_cast<FLOAT>(ry)
			};
			m_dialog_page.m_shape_list.push_back(new ShapeQEllipse(start, b_vec, rot, s));
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			cd_setting_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_rotation")));
			const ContentDialogResult d_result = co_await cd_setting_dialog().ShowAsync();
			if (d_result == ContentDialogResult::Primary) {
				float samp_val;
				m_dialog_page.m_shape_list.back()->get_rotation(samp_val);
				ustack_push_set<UNDO_ID::ROTATION>(samp_val);
				ustack_push_null();
				xcvd_is_enabled();
				page_draw();
			}
			slist_clear(m_dialog_page.m_shape_list);
			dialog_slider_0().ValueChanged(ds0_tok);
			dialog_slider_0().Minimum(ds0_min);
			dialog_slider_0().Maximum(ds0_max);
			dialog_slider_0().TickFrequency(ds0_freq);
			dialog_slider_0().SnapsTo(ds0_snap);
			dialog_slider_0().Value(ds0_val);
			dialog_slider_0().Visibility(ds0_vis);
			status_bar_set_pos();
			m_mutex_event.unlock();
		}
	}
}