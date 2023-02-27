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
			wchar_t buf[32];
			swprintf_s(buf, 32, L"%f°", val);
			if constexpr (S == 0) {
				const winrt::hstring text =
					ResourceLoader::GetForCurrentView().GetString(L"str_deg_start") + L": " + buf;
				dialog_slider_0().Header(box_value(text));
			}
			if constexpr (S == 1) {
				const winrt::hstring text =
					ResourceLoader::GetForCurrentView().GetString(L"str_deg_end") + L": " + buf;
				dialog_slider_1().Header(box_value(text));
			}
			else if constexpr (S == 2) {
				const winrt::hstring text =
					ResourceLoader::GetForCurrentView().GetString(L"str_deg_rot") + L": " + buf;
				dialog_slider_2().Header(box_value(text));
			}
		}
	}

	//------------------------------
	// スライダーの値が変更された.
	// U	操作の識別子
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	//------------------------------
	template <UNDO_ID U, int S> void MainPage::rotation_slider_val_changed(
		IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		// 値をスライダーのヘッダーに格納する.
		if constexpr (U == UNDO_ID::ROTATION) {
			const float val = static_cast<float>(args.NewValue());
			if constexpr (S == 0) {
				rotation_slider_set_header<U, S>(val);
				static_cast<ShapeQEllipse*>(m_dialog_page.m_shape_list.back())->set_deg_start(val);
			}
			else if constexpr (S == 1) {
				rotation_slider_set_header<U, S>(val);
				static_cast<ShapeQEllipse*>(m_dialog_page.m_shape_list.back())->set_deg_end(val);
			}
			else if constexpr (S == 2) {
				rotation_slider_set_header<U, S>(val);
				m_dialog_page.m_shape_list.back()->set_deg_rotation(val);
			}
		}
		if (scp_dialog_panel().IsLoaded()) {
			dialog_draw();
		}
	}

	IAsyncAction MainPage::rotation_click_async(Shape* s)
	{
		if (s != nullptr && typeid(*s) == typeid(ShapeQEllipse)) {
			ShapeQEllipse* t = static_cast<ShapeQEllipse*>(s);
			float deg_start;
			t->get_deg_start(deg_start);
			float deg_end;
			t->get_deg_end(deg_end);
			float deg_rot;
			t->get_deg_rotation(deg_rot);
			m_mutex_event.lock();
			m_dialog_page.set_attr_to(&m_main_page);

			const auto ds0_min = dialog_slider_0().Minimum();
			const auto ds0_max = dialog_slider_0().Maximum();
			const auto ds0_freq = dialog_slider_0().TickFrequency();
			const auto ds0_snap = dialog_slider_0().SnapsTo();
			const auto ds0_val = dialog_slider_0().Value();
			const auto ds0_vis = dialog_slider_0().Visibility();
			const auto ds1_min = dialog_slider_1().Minimum();
			const auto ds1_max = dialog_slider_1().Maximum();
			const auto ds1_freq = dialog_slider_1().TickFrequency();
			const auto ds1_snap = dialog_slider_1().SnapsTo();
			const auto ds1_val = dialog_slider_1().Value();
			const auto ds1_vis = dialog_slider_1().Visibility();
			const auto ds2_min = dialog_slider_2().Minimum();
			const auto ds2_max = dialog_slider_2().Maximum();
			const auto ds2_freq = dialog_slider_2().TickFrequency();
			const auto ds2_snap = dialog_slider_2().SnapsTo();
			const auto ds2_val = dialog_slider_2().Value();
			const auto ds2_vis = dialog_slider_2().Visibility();

			dialog_slider_0().Minimum(-44.9);
			dialog_slider_0().Maximum(44.9);
			dialog_slider_0().TickFrequency(0.5);
			dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
			dialog_slider_0().Value(deg_start);
			dialog_slider_0().Visibility(Visibility::Visible);
			dialog_slider_1().Minimum(-44.9);
			dialog_slider_1().Maximum(44.9);
			dialog_slider_1().TickFrequency(0.5);
			dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
			dialog_slider_1().Value(deg_end);
			dialog_slider_1().Visibility(Visibility::Visible);
			dialog_slider_2().Minimum(-44.9);
			dialog_slider_2().Maximum(44.9);
			dialog_slider_2().TickFrequency(0.5);
			dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
			dialog_slider_2().Value(deg_rot);
			dialog_slider_2().Visibility(Visibility::Visible);

			const winrt::event_token ds0_tok{
				dialog_slider_0().ValueChanged(
					{ this, &MainPage::rotation_slider_val_changed<UNDO_ID::ROTATION, 0> })
			};
			const winrt::event_token ds1_tok{
				dialog_slider_1().ValueChanged(
					{ this, &MainPage::rotation_slider_val_changed<UNDO_ID::ROTATION, 1> })
			};
			const winrt::event_token ds2_tok{
				dialog_slider_2().ValueChanged(
					{ this, &MainPage::rotation_slider_val_changed<UNDO_ID::ROTATION, 2> })
			};
			rotation_slider_set_header<UNDO_ID::ROTATION, 0>(deg_start);
			rotation_slider_set_header<UNDO_ID::ROTATION, 1>(deg_end);
			rotation_slider_set_header<UNDO_ID::ROTATION, 2>(deg_rot);
			const auto samp_w = scp_dialog_panel().Width();
			const auto samp_h = scp_dialog_panel().Height();
			const auto center = samp_w * 0.5;
			const auto padd = samp_w * 0.125;
			const auto rx = (samp_w - padd) * 0.5;
			const auto ry = (samp_h - padd) * 0.5;
			const D2D1_POINT_2F start{
				static_cast<FLOAT>(center), static_cast<FLOAT>(padd)
			};
			const D2D1_POINT_2F pos{
				static_cast<FLOAT>(rx), static_cast<FLOAT>(ry)
			};
			m_dialog_page.m_shape_list.push_back(new ShapeQEllipse(start, pos, s));
			static_cast<ShapeQEllipse*>(m_dialog_page.m_shape_list.back())->set_deg_start(deg_start);
			static_cast<ShapeQEllipse*>(m_dialog_page.m_shape_list.back())->set_deg_end(deg_end);
			static_cast<ShapeQEllipse*>(m_dialog_page.m_shape_list.back())->set_deg_rotation(deg_rot);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			cd_setting_dialog().Title(
				box_value(ResourceLoader::GetForCurrentView().GetString(L"str_deg_rot")));
			const ContentDialogResult d_result = co_await cd_setting_dialog().ShowAsync();
			if (d_result == ContentDialogResult::Primary) {
				float samp_val;
				m_dialog_page.m_shape_list.back()->get_deg_rotation(samp_val);
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
			dialog_slider_1().ValueChanged(ds1_tok);
			dialog_slider_1().Minimum(ds1_min);
			dialog_slider_1().Maximum(ds1_max);
			dialog_slider_1().TickFrequency(ds1_freq);
			dialog_slider_1().SnapsTo(ds1_snap);
			dialog_slider_1().Value(ds1_val);
			dialog_slider_1().Visibility(ds1_vis);
			dialog_slider_2().ValueChanged(ds2_tok);
			dialog_slider_2().Minimum(ds2_min);
			dialog_slider_2().Maximum(ds2_max);
			dialog_slider_2().TickFrequency(ds2_freq);
			dialog_slider_2().SnapsTo(ds2_snap);
			dialog_slider_2().Value(ds2_val);
			dialog_slider_2().Visibility(ds2_vis);
			status_bar_set_pos();
			m_mutex_event.unlock();
		}
	}
}