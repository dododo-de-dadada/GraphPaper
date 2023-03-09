#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::CheckBox;

	// 編集メニューの「文字列の編集」が選択された.
	IAsyncAction MainPage::edit_text_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		ShapeText* s = static_cast<ShapeText*>(nullptr);	// 編集する文字列図形
		if (m_event_shape_prev != nullptr && typeid(*m_event_shape_prev) == typeid(ShapeText)) {
			// 前回ポインターが押されたのが文字列図形ならその図形.
			s = static_cast<ShapeText*>(m_event_shape_prev);
		}
		else {
			// 選択された図形のうち最前面にある文字列図形を得る.
			for (auto it = m_main_page.m_shape_list.rbegin(); 
				it != m_main_page.m_shape_list.rend(); it++) {
				auto t = *it;
				if (t->is_deleted()) {
					continue;
				}
				if (!t->is_selected()) {
					continue;
				}
				if (typeid(*t) == typeid(ShapeText)) {
					s = static_cast<ShapeText*>(t);
					break;
				}
			}
		}

		if (s != nullptr) {
			static winrt::event_token primary_token;
			static winrt::event_token closed_token;

			tx_edit_text().Text(s->m_text == nullptr ? L"" : s->m_text);
			tx_edit_text().SelectAll();
			ck_text_fit_frame_to_text().IsChecked(m_text_fit_frame_to_text);
			if (co_await cd_edit_text_dialog().ShowAsync() == ContentDialogResult::Primary) {
				auto text = wchar_cpy(tx_edit_text().Text().c_str());
				ustack_push_set<UNDO_ID::TEXT_CONTENT>(s, text);
				m_text_fit_frame_to_text = ck_text_fit_frame_to_text().IsChecked().GetBoolean();
				if (m_text_fit_frame_to_text) {
					ustack_push_position(s, ANC_TYPE::ANC_SE);
					s->fit_frame_to_text(
						m_main_page.m_grid_snap ? m_main_page.m_grid_base + 1.0f : 0.0f);
				}
				ustack_push_null();
				xcvd_is_enabled();
				page_draw();
			}
		}
		status_bar_set_pos();
	}

	//------------------------------
	// 値をスライダーのヘッダーに格納する.
	// S	スライダーの番号
	// val	格納する値
	template <int S>
	void MainPage::edit_arc_slider_set_header(const float val)
	{
		if constexpr (S == 0) {
			wchar_t buf[32];
			swprintf_s(buf, 32, L"%f°", val);
			const winrt::hstring text =
				ResourceLoader::GetForCurrentView().GetString(L"str_arc_start") + L": " + buf;
			dialog_slider_0().Header(box_value(text));
		}
		else if constexpr (S == 1) {
			wchar_t buf[32];
			swprintf_s(buf, 32, L"%f°", val);
			const winrt::hstring text =
				ResourceLoader::GetForCurrentView().GetString(L"str_arc_end") + L": " + buf;
			dialog_slider_1().Header(box_value(text));
		}
		else if constexpr (S == 2) {
			wchar_t buf[32];
			swprintf_s(buf, 32, L"%f°", val);
			const winrt::hstring text =
				ResourceLoader::GetForCurrentView().GetString(L"str_arc_rot") + L": " + buf;
			dialog_slider_2().Header(box_value(text));
		}
	}

	template<int S>
	void MainPage::edit_arc_slider_value_changed(
		IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (S == 0) {
			const float val = static_cast<float>(args.NewValue());
			edit_arc_slider_set_header<0>(val);
			D2D1_SWEEP_DIRECTION dir;
			m_dialog_page.m_shape_list.back()->get_arc_dir(dir);
			if (dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				if (m_dialog_page.m_shape_list.back()->set_arc_start(val)) {
					dialog_draw();
				}
			}
			else {
				if (m_dialog_page.m_shape_list.back()->set_arc_end(-val)) {
					dialog_draw();
				}
			}
		}
		else if constexpr (S == 1) {
			const float val = static_cast<float>(args.NewValue());
			edit_arc_slider_set_header<1>(val);
			D2D1_SWEEP_DIRECTION dir;
			m_dialog_page.m_shape_list.back()->get_arc_dir(dir);
			if (dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				if (m_dialog_page.m_shape_list.back()->set_arc_end(val)) {
					dialog_draw();
				}
			}
			else {
				if (m_dialog_page.m_shape_list.back()->set_arc_start(-val)) {
					dialog_draw();
				}
			}
		}
		else if constexpr (S == 2) {
			const float val = static_cast<float>(args.NewValue());
			edit_arc_slider_set_header<2>(val);
			m_dialog_page.m_shape_list.back()->set_arc_rot(val);
			dialog_draw();

		}
	}

	template<int S>
	void MainPage::edit_arc_checkbox_checked(IInspectable const&, RoutedEventArgs const&)
	{
		if constexpr (S == 0) {
			if (m_dialog_page.m_shape_list.back()->set_arc_dir(
				D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE)) {
				const auto val0 = dialog_slider_0().Value();
				const auto val1 = dialog_slider_1().Value();
				dialog_slider_0().Value(-val1);
				dialog_slider_1().Value(-val0);
				dialog_draw();
			}
		}
		else if constexpr (S == 1) {
			if (m_dialog_page.m_shape_list.back()->set_arc_dir(
				D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE)) {
				const auto val0 = dialog_slider_0().Value();
				const auto val1 = dialog_slider_1().Value();
				dialog_slider_0().Value(-val1);
				dialog_slider_1().Value(-val0);
				dialog_draw();
			}
		}
	}


	IAsyncAction MainPage::edit_arc_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		ShapeArc* t;	// 編集する円弧図形
		if (m_event_shape_prev != nullptr &&
			typeid(*m_event_shape_prev) == typeid(ShapeArc)) {
			t = static_cast<ShapeArc*>(m_event_shape_prev);
		}
		else {
			// 選択された図形のうち最前面にある円弧図形を得る.
			t = nullptr;
			for (auto it = m_main_page.m_shape_list.rbegin();
				it != m_main_page.m_shape_list.rend(); it++) {
				if ((*it)->is_deleted()) {
					continue;
				}
				if (!(*it)->is_selected()) {
					continue;
				}
				if (typeid(*(*it)) == typeid(ShapeArc)) {
					t = static_cast<ShapeArc*>(*it);
					break;
				}
			}
		}
		if (t != nullptr) {
			m_mutex_event.lock();
			m_dialog_page.set_attr_to(&m_main_page);

			float a_start;
			t->get_arc_start(a_start);
			float a_end;
			t->get_arc_end(a_end);
			float a_rot;
			t->get_arc_rot(a_rot);
			D2D1_SWEEP_DIRECTION a_dir;
			t->get_arc_dir(a_dir);

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
			Shape* s = new ShapeArc(start, pos, t);
			s->set_select(true);
			s->set_arc_dir(a_dir);
			s->set_arc_start(a_start);
			s->set_arc_end(a_end);
			s->set_arc_rot(a_rot);
			m_dialog_page.m_shape_list.push_back(s);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif

			const winrt::event_token token0{
				dialog_slider_0().ValueChanged(
					{ this, &MainPage::edit_arc_slider_value_changed<0> })
			};
			const winrt::event_token token1{
				dialog_slider_1().ValueChanged(
					{ this, &MainPage::edit_arc_slider_value_changed<1> })
			};
			const winrt::event_token token2{
				dialog_slider_2().ValueChanged(
					{ this, &MainPage::edit_arc_slider_value_changed<2> })
			};
			const winrt::event_token token3{
				dialog_check_box().Checked({ this, &MainPage::edit_arc_checkbox_checked<0> })
			};
			const winrt::event_token token4{
				dialog_check_box().Unchecked({ this, &MainPage::edit_arc_checkbox_checked<1> })
			};

			const auto min0 = dialog_slider_0().Minimum();
			const auto max0 = dialog_slider_0().Maximum();
			const auto freq0 = dialog_slider_0().TickFrequency();
			const auto snap0 = dialog_slider_0().SnapsTo();
			const auto val0 = dialog_slider_0().Value();
			const auto vis0 = dialog_slider_0().Visibility();
			const auto min1 = dialog_slider_1().Minimum();
			const auto max1 = dialog_slider_1().Maximum();
			const auto freq1 = dialog_slider_1().TickFrequency();
			const auto snap1 = dialog_slider_1().SnapsTo();
			const auto val1 = dialog_slider_1().Value();
			const auto vis1 = dialog_slider_1().Visibility();
			const auto min2 = dialog_slider_2().Minimum();
			const auto max2 = dialog_slider_2().Maximum();
			const auto freq2 = dialog_slider_2().TickFrequency();
			const auto snap2 = dialog_slider_2().SnapsTo();
			const auto val2 = dialog_slider_2().Value();
			const auto vis2 = dialog_slider_2().Visibility();
			const auto val3 = dialog_check_box().IsChecked();
			const auto vis3 = dialog_check_box().Visibility();

			dialog_slider_0().Minimum(0.0);
			dialog_slider_0().Maximum(45.0);
			dialog_slider_0().TickFrequency(0.5);
			dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
			if (a_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				dialog_slider_0().Value(a_start);
				edit_arc_slider_set_header<0>(a_start);
			}
			else {
				dialog_slider_0().Value(-a_end);
				edit_arc_slider_set_header<0>(-a_end);
			}
			dialog_slider_0().Visibility(Visibility::Visible);
			dialog_slider_1().Minimum(-45.0);
			dialog_slider_1().Maximum(0.0);
			dialog_slider_1().TickFrequency(0.5);
			dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
			if (a_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				dialog_slider_1().Value(a_end);
				edit_arc_slider_set_header<1>(a_end);
			}
			else {
				dialog_slider_1().Value(-a_start);
				edit_arc_slider_set_header<1>(-a_start);
			}
			dialog_slider_1().Visibility(Visibility::Visible);
			dialog_slider_2().Minimum(-44.5);
			dialog_slider_2().Maximum(44.5);
			dialog_slider_2().TickFrequency(0.5);
			dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
			dialog_slider_2().Value(a_rot);
			edit_arc_slider_set_header<2>(a_rot);
			dialog_slider_2().Visibility(Visibility::Visible);
			if (a_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				dialog_check_box().IsChecked(true);
			}
			else {
				dialog_check_box().IsChecked(false);
			}
			dialog_check_box().Visibility(Visibility::Visible);

			cd_setting_dialog().Title(
				box_value(ResourceLoader::GetForCurrentView().GetString(L"str_arc_rot")));
			const ContentDialogResult d_result = co_await cd_setting_dialog().ShowAsync();
			if (d_result == ContentDialogResult::Primary) {
				s = m_dialog_page.m_shape_list.back();
				// 注意: 順番が OK かどうか.
				D2D1_SWEEP_DIRECTION new_dir;
				s->get_arc_dir(new_dir);
				ustack_push_set<UNDO_ID::ARC_DIR>(new_dir);
				float new_val;
				s->get_arc_start(new_val);
				ustack_push_set<UNDO_ID::ARC_START>(new_val);
				s->get_arc_end(new_val);
				ustack_push_set<UNDO_ID::ARC_END>(new_val);
				s->get_arc_rot(new_val);
				ustack_push_set<UNDO_ID::ARC_ROT>(new_val);
				ustack_push_null();
				xcvd_is_enabled();
				page_draw();
			}
			slist_clear(m_dialog_page.m_shape_list);
			dialog_slider_0().ValueChanged(token0);
			dialog_slider_0().Minimum(min0);
			dialog_slider_0().Maximum(max0);
			dialog_slider_0().TickFrequency(freq0);
			dialog_slider_0().SnapsTo(snap0);
			dialog_slider_0().Value(val0);
			dialog_slider_0().Visibility(vis0);
			dialog_slider_1().ValueChanged(token1);
			dialog_slider_1().Minimum(min1);
			dialog_slider_1().Maximum(max1);
			dialog_slider_1().TickFrequency(freq1);
			dialog_slider_1().SnapsTo(snap1);
			dialog_slider_1().Value(val1);
			dialog_slider_1().Visibility(vis1);
			dialog_slider_2().ValueChanged(token2);
			dialog_slider_2().Minimum(min2);
			dialog_slider_2().Maximum(max2);
			dialog_slider_2().TickFrequency(freq2);
			dialog_slider_2().SnapsTo(snap2);
			dialog_slider_2().Value(val2);
			dialog_slider_2().Visibility(vis2);
			dialog_check_box().Checked(token3);
			dialog_check_box().Unchecked(token4);
			dialog_check_box().IsChecked(val3);
			dialog_check_box().Visibility(vis3);
			m_mutex_event.unlock();
			slist_clear(m_dialog_page.m_shape_list);
		}
		status_bar_set_pos();
	}
}