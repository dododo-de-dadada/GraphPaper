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
		ShapeText* s = static_cast<ShapeText*>(nullptr);
		if (m_event_shape_prev != nullptr && typeid(*m_event_shape_prev) == typeid(ShapeText)) {
			// 前回ポインターが押されたのが文字列図形の場合,
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
		else {
			status_bar_set_pos();
		}
	}

	//------------------------------
	// 値をスライダーのヘッダーに格納する.
	// U	操作の識別子
	// S	スライダーの番号
	// val	格納する値
	//------------------------------
	template <int S>
	void MainPage::edit_arc_slider_set_header(const float val)
	{
		if constexpr (S == 0) {
			wchar_t buf[32];
			swprintf_s(buf, 32, L"%f°", val);
			const winrt::hstring text =
				ResourceLoader::GetForCurrentView().GetString(L"str_deg_start") + L": " + buf;
			dialog_slider_0().Header(box_value(text));
		}
		else if constexpr (S == 1) {
			wchar_t buf[32];
			swprintf_s(buf, 32, L"%f°", val);
			const winrt::hstring text =
				ResourceLoader::GetForCurrentView().GetString(L"str_deg_end") + L": " + buf;
			dialog_slider_1().Header(box_value(text));
		}
		else if constexpr (S == 2) {
			wchar_t buf[32];
			swprintf_s(buf, 32, L"%f°", val);
			const winrt::hstring text =
				ResourceLoader::GetForCurrentView().GetString(L"str_deg_rot") + L": " + buf;
			dialog_slider_2().Header(box_value(text));
		}
	}

	IAsyncAction MainPage::edit_arc_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		ShapeQEllipse* t;
		if (m_event_shape_prev != nullptr &&
			typeid(*m_event_shape_prev) == typeid(ShapeQEllipse)) {
			t = static_cast<ShapeQEllipse*>(m_event_shape_prev);
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
				if (typeid(*(*it)) == typeid(ShapeQEllipse)) {
					t = static_cast<ShapeQEllipse*>(*it);
					break;
				}
			}
		}
		if (t != nullptr) {
			m_mutex_event.lock();

			float deg_start;
			t->get_arc_start(deg_start);
			float deg_end;
			t->get_arc_end(deg_end);
			float deg_rot;
			t->get_arc_rot(deg_rot);
			D2D1_SWEEP_DIRECTION dir;
			t->get_arc_dir(dir);
			m_dialog_page.set_attr_to(&m_main_page);

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
			Shape* s = new ShapeQEllipse(start, pos, t);
			s->set_arc_dir(dir);
			s->set_arc_start(deg_start);
			s->set_arc_end(deg_end);
			s->set_arc_rot(deg_rot);

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
			const auto dcb_val = dialog_check_box().IsChecked();
			const auto dcb_vis = dialog_check_box().Visibility();

			dialog_slider_0().Minimum(0.0);
			dialog_slider_0().Maximum(45.0);
			dialog_slider_0().TickFrequency(0.5);
			dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
			if (dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				dialog_slider_0().Value(deg_start);
				edit_arc_slider_set_header<0>(deg_start);
			}
			else {
				dialog_slider_0().Value(-deg_end);
				edit_arc_slider_set_header<0>(-deg_end);
			}
			dialog_slider_0().Visibility(Visibility::Visible);
			dialog_slider_1().Minimum(-45.0);
			dialog_slider_1().Maximum(0.0);
			dialog_slider_1().TickFrequency(0.5);
			dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
			if (dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				dialog_slider_1().Value(deg_end);
				edit_arc_slider_set_header<1>(deg_end);
			}
			else {
				dialog_slider_1().Value(-deg_start);
				edit_arc_slider_set_header<1>(-deg_start);
			}
			dialog_slider_1().Visibility(Visibility::Visible);
			dialog_slider_2().Minimum(-44.5);
			dialog_slider_2().Maximum(44.5);
			dialog_slider_2().TickFrequency(0.5);
			dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
			dialog_slider_2().Value(deg_rot);
			edit_arc_slider_set_header<2>(deg_rot);
			dialog_slider_2().Visibility(Visibility::Visible);
			dialog_check_box().IsChecked(dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE);
			dialog_check_box().Visibility(Visibility::Visible);

			const winrt::event_token ds0_tok{
				dialog_slider_0().ValueChanged(
					[=](IInspectable const& sender, RangeBaseValueChangedEventArgs const& args) {
						const float val = static_cast<float>(args.NewValue());
						edit_arc_slider_set_header<0>(val);
						D2D1_SWEEP_DIRECTION dir;
						m_dialog_page.m_shape_list.back()->get_arc_dir(dir);
						if (dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
							m_dialog_page.m_shape_list.back()->set_arc_start(val);
						}
						else {
							m_dialog_page.m_shape_list.back()->set_arc_end(-val);
						}
						dialog_draw();
					}
				)
			};
			const winrt::event_token ds1_tok{
				dialog_slider_1().ValueChanged(
					[=](IInspectable const& sender, RangeBaseValueChangedEventArgs const& args) {
						const float val = static_cast<float>(args.NewValue());
						edit_arc_slider_set_header<1>(val);
						D2D1_SWEEP_DIRECTION dir;
						m_dialog_page.m_shape_list.back()->get_arc_dir(dir);
						if (dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
							m_dialog_page.m_shape_list.back()->set_arc_end(val);
						}
						else {
							m_dialog_page.m_shape_list.back()->set_arc_start(-val);
						}
						dialog_draw();
					}
				)
			};
			const winrt::event_token ds2_tok{
				dialog_slider_2().ValueChanged(
					[=](IInspectable const& sender, RangeBaseValueChangedEventArgs const& args) {
						const float val = static_cast<float>(args.NewValue());
						edit_arc_slider_set_header<2>(val);
						m_dialog_page.m_shape_list.back()->set_arc_rot(val);
						dialog_draw();
					}
				)
			};
			const winrt::event_token dcb_tok_checked{
				dialog_check_box().Checked(
					[=](IInspectable const& sender,  RoutedEventArgs const& args) {
						Shape* s = m_dialog_page.m_shape_list.back();
						if (s->set_arc_dir(D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE)) {
							const auto val0 = dialog_slider_0().Value();
							const auto val1 = dialog_slider_1().Value();
							dialog_slider_0().Value(-val1);
							dialog_slider_1().Value(-val0);
							dialog_draw();
						}
					}
				)
			};
			const winrt::event_token dcb_tok_unchecked{
				dialog_check_box().Unchecked(
					[=](IInspectable const& sender,  RoutedEventArgs const& args) {
						Shape* s = m_dialog_page.m_shape_list.back();
						if (s->set_arc_dir(
							D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE)) {
							const auto val0 = dialog_slider_0().Value();
							const auto val1 = dialog_slider_1().Value();
							dialog_slider_0().Value(-val1);
							dialog_slider_1().Value(-val0);
							dialog_draw();
						}
					}
				)
			};
			m_dialog_page.m_shape_list.push_back(s);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			cd_setting_dialog().Title(
				box_value(ResourceLoader::GetForCurrentView().GetString(L"str_deg_rot")));
			const ContentDialogResult d_result = co_await cd_setting_dialog().ShowAsync();
			if (d_result == ContentDialogResult::Primary) {
				s = m_dialog_page.m_shape_list.back();
				// 注意: 順番が OK かどうか.
				D2D1_SWEEP_DIRECTION dir;
				s->get_arc_dir(dir);
				ustack_push_set<UNDO_ID::ARC_DIR>(dir);
				float samp_val;
				s->get_arc_start(samp_val);
				ustack_push_set<UNDO_ID::ARC_START>(samp_val);
				s->get_arc_end(samp_val);
				ustack_push_set<UNDO_ID::ARC_END>(samp_val);
				s->get_arc_rot(samp_val);
				ustack_push_set<UNDO_ID::ARC_ROT>(samp_val);
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
			dialog_check_box().Checked(dcb_tok_checked);
			dialog_check_box().Unchecked(dcb_tok_unchecked);
			dialog_check_box().IsChecked(dcb_val);
			dialog_check_box().Visibility(dcb_vis);
			status_bar_set_pos();
			m_mutex_event.unlock();
		}
	}
}