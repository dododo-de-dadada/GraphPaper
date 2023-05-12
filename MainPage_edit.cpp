#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::CheckBox;
	using winrt::Windows::UI::Xaml::Controls::Slider;

	// 編集メニューの「文字列の編集」が選択された.
	IAsyncAction MainPage::meth_text_edit_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		ShapeText* s = static_cast<ShapeText*>(nullptr);	// 編集する文字列図形
		if (m_event_shape_last != nullptr && typeid(*m_event_shape_last) == typeid(ShapeText)) {
			// 前回ポインターが押されたのが文字列図形ならその図形.
			s = static_cast<ShapeText*>(m_event_shape_last);
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
			ck_fit_text_frame().IsChecked(m_fit_text_frame);
			if (co_await cd_edit_text_dialog().ShowAsync() == ContentDialogResult::Primary) {
				const auto len = tx_edit_text().Text().size();
				undo_push_null();
				undo_push_text_select(s, len, len, false);
				undo_push_set<UNDO_T::TEXT_CONTENT>(s, wchar_cpy(tx_edit_text().Text().c_str()));
				m_fit_text_frame = ck_fit_text_frame().IsChecked().GetBoolean();
				if (m_fit_text_frame) {
					undo_push_position(s, LOC_TYPE::LOC_SE);
					s->fit_frame_to_text(m_snap_grid ? m_main_page.m_grid_base + 1.0f : 0.0f);
				}
				undo_menu_is_enabled();
				main_draw();
			}
		}
		status_bar_set_pos();
	}

	void MainPage::meth_poly_end_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == mfi_menu_meth_poly_close() || sender == mfi_popup_meth_poly_close()) {
			undo_push_null();
			if (undo_push_set<UNDO_T::POLY_END>(D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED)) {
			//if (undo_push_set<UNDO_T::POLY_END>(true)) {
				mfi_menu_meth_poly_close().IsEnabled(false);
				mfi_menu_meth_poly_open().IsEnabled(true);
				undo_menu_is_enabled();
				main_draw();
			}
		}
		else if (sender == mfi_menu_meth_poly_open() || sender == mfi_popup_popup_poly_open()) {
			undo_push_null();
			if (undo_push_set<UNDO_T::POLY_END>(D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN)) {
			//if (undo_push_set<UNDO_T::POLY_END>(false)) {
				mfi_menu_meth_poly_close().IsEnabled(true);
				mfi_menu_meth_poly_open().IsEnabled(false);
				undo_menu_is_enabled();
				main_draw();
			}
		}
	}

	IAsyncAction MainPage::meth_arc_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		const auto str_arc_start{
			ResourceLoader::GetForCurrentView().GetString(L"str_arc_start") + L": "
		};
		const auto str_arc_end{
			ResourceLoader::GetForCurrentView().GetString(L"str_arc_end") + L": "
		};
		const auto str_arc_rot{
			ResourceLoader::GetForCurrentView().GetString(L"str_arc_rot") + L": "
		};
		const auto str_arc_sweep_direction{
			ResourceLoader::GetForCurrentView().GetString(L"str_arc_sweep_direction")
		};
		const auto str_arc_clockwize{
			ResourceLoader::GetForCurrentView().GetString(L"str_arc_clockwize")
		};
		const auto str_arc_counter_clockwize{
			ResourceLoader::GetForCurrentView().GetString(L"str_arc_counter_clockwize")
		};
		const auto str_title{
			ResourceLoader::GetForCurrentView().GetString(L"str_arc_rotation")
		};

		ShapeArc* t;	// 編集する円弧図形
		if (m_event_shape_last != nullptr &&
			typeid(*m_event_shape_last) == typeid(ShapeArc)) {
			t = static_cast<ShapeArc*>(m_event_shape_last);
		}
		else {
			// 選択された図形のうち最前面にある円弧図形を得る.
			t = nullptr;
			for (auto it = m_main_page.m_shape_list.rbegin();
				it != m_main_page.m_shape_list.rend(); it++) {
				Shape* s = *it;
				if (s->is_deleted()) {
					continue;
				}
				if (!s->is_selected()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeArc)) {
					t = static_cast<ShapeArc*>(s);
					break;
				}
			}
		}
		if (t != nullptr) {
			m_mutex_event.lock();
			m_prop_page.set_attr_to(&m_main_page);

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
			const auto ctr = samp_w * 0.5;
			const auto mar = samp_w * 0.125;
			const auto rx = (samp_w - mar) * 0.5;
			const auto ry = (samp_h - mar) * 0.5;
			const D2D1_POINT_2F start{
				static_cast<FLOAT>(ctr), static_cast<FLOAT>(mar)
			};
			const D2D1_POINT_2F pos{
				static_cast<FLOAT>(rx), static_cast<FLOAT>(ry)
			};
			Shape* s = new ShapeArc(start, pos, t);
			s->set_select(true);
			m_prop_page.m_shape_list.push_back(s);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif

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
			//const auto val3 = dialog_check_box().IsChecked();
			//const auto vis3 = dialog_check_box().Visibility();
			//const auto vis3 = dialog_radio_buttons().Visibility();
			wchar_t buf[32];

			dialog_slider_0().Minimum(0.0);
			dialog_slider_0().Maximum(45.0);
			dialog_slider_0().TickFrequency(0.5);
			dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
			dialog_slider_0().Visibility(Visibility::Visible);
			if (a_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				dialog_slider_0().Value(a_start);
				swprintf_s(buf, L"%f°", a_start);
				dialog_slider_0().Header(box_value(str_arc_start + buf));
			}
			else {
				dialog_slider_0().Value(-a_end);
				swprintf_s(buf, L"%f°", -a_end);
				dialog_slider_0().Header(box_value(str_arc_start + buf));
			}
			dialog_slider_1().Minimum(-45.0);
			dialog_slider_1().Maximum(0.0);
			dialog_slider_1().TickFrequency(0.5);
			dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
			if (a_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				dialog_slider_1().Value(a_end);
				swprintf_s(buf, L"%f°", a_end);
				dialog_slider_1().Header(box_value(str_arc_end + buf));
			}
			else {
				dialog_slider_1().Value(-a_start);
				swprintf_s(buf, L"%f°", -a_start);
				dialog_slider_1().Header(box_value(str_arc_end + buf));
			}
			dialog_slider_1().Visibility(Visibility::Visible);
			dialog_slider_2().Minimum(-44.5);
			dialog_slider_2().Maximum(44.5);
			dialog_slider_2().TickFrequency(0.5);
			dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
			dialog_slider_2().Value(a_rot);
			swprintf_s(buf, L"%f°", a_rot);
			dialog_slider_2().Header(box_value(str_arc_rot + buf));
			dialog_slider_2().Visibility(Visibility::Visible);

			dialog_radio_btns().Header(box_value(str_arc_sweep_direction));
			dialog_radio_btn_0().Content(box_value(str_arc_clockwize));
			dialog_radio_btn_1().Content(box_value(str_arc_counter_clockwize));
			dialog_radio_btns().Visibility(Visibility::Visible);
			if (a_dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
				dialog_radio_btns().SelectedIndex(0);
			}
			else {
				dialog_radio_btns().SelectedIndex(1);
			}
			cd_dialog_prop().Title(box_value(str_title));
			{
				const auto revoker0{
					dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_arc_start](auto, auto args) {
						// (IInspectable const&, RangeBaseValueChangedEventArgs const& args)
						const float val = static_cast<float>(args.NewValue());
						wchar_t buf[32];
						swprintf_s(buf, L"%f°", val);
						dialog_slider_0().Header(box_value(str_arc_start + buf));
						D2D1_SWEEP_DIRECTION dir;
						m_prop_page.back()->get_arc_dir(dir);
						if (dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
							if (m_prop_page.back()->set_arc_start(val)) {
								dialog_draw();
							}
						}
						else {
							if (m_prop_page.back()->set_arc_end(-val)) {
								dialog_draw();
							}
						}
					})
				};
				const auto revoker1{
					dialog_slider_1().ValueChanged(winrt::auto_revoke, [this, str_arc_end](auto, auto args) {
						// (IInspectable const&, RangeBaseValueChangedEventArgs const& args)
						const float val = static_cast<float>(args.NewValue());
						wchar_t buf[32];
						swprintf_s(buf, L"%f°", val);
						dialog_slider_1().Header(box_value(str_arc_end + buf));
						D2D1_SWEEP_DIRECTION dir;
						m_prop_page.back()->get_arc_dir(dir);
						if (dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
							if (m_prop_page.back()->set_arc_end(val)) {
								dialog_draw();
							}
						}
						else {
							if (m_prop_page.back()->set_arc_start(-val)) {
								dialog_draw();
							}
						}
					})
				};
				const auto revoker2{
					dialog_slider_2().ValueChanged(winrt::auto_revoke, [this, str_arc_rot](auto, auto args) {
						const float val = static_cast<float>(args.NewValue());
						wchar_t buf[32];
						swprintf_s(buf, L"%f°", val);
						dialog_slider_2().Header(box_value(str_arc_rot + buf));
						m_prop_page.back()->set_arc_rot(val);
						dialog_draw();
					})
				};
				const auto revoker3{
					dialog_radio_btns().SelectionChanged(winrt::auto_revoke, [this](auto, auto) {
						// (IInspectable const&, SelectionChangedEventArgs const&)
						if (dialog_radio_btns().SelectedIndex() == 0) {
							if (m_prop_page.back()->set_arc_dir(D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE)) {
								const auto val0 = dialog_slider_0().Value();
								const auto val1 = dialog_slider_1().Value();
								dialog_slider_0().Value(-val1);
								dialog_slider_1().Value(-val0);
								dialog_draw();
							}
						}
						else if (dialog_radio_btns().SelectedIndex() == 1) {
							if (m_prop_page.back()->set_arc_dir(D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE)) {
								const auto val0 = dialog_slider_0().Value();
								const auto val1 = dialog_slider_1().Value();
								dialog_slider_0().Value(-val1);
								dialog_slider_1().Value(-val0);
								dialog_draw();
							}
						}
					})
				};
				if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
					undo_push_null();
					s = m_prop_page.back();
					// 注意: 順番が OK かどうか.
					D2D1_SWEEP_DIRECTION new_dir;
					s->get_arc_dir(new_dir);
					undo_push_set<UNDO_T::ARC_DIR>(new_dir);
					float new_val;
					s->get_arc_start(new_val);
					undo_push_set<UNDO_T::ARC_START>(new_val);
					s->get_arc_end(new_val);
					undo_push_set<UNDO_T::ARC_END>(new_val);
					s->get_arc_rot(new_val);
					undo_push_set<UNDO_T::ARC_ROT>(new_val);
					undo_menu_is_enabled();
					main_draw();
				}
			}
			slist_clear(m_prop_page.m_shape_list);
			//dialog_slider_0().ValueChanged(token0);
			//dialog_slider_1().ValueChanged(token1);
			//dialog_slider_2().ValueChanged(token2);
			//dialog_radio_btns().SelectionChanged(token3);
			dialog_slider_0().Minimum(min0);
			dialog_slider_0().Maximum(max0);
			dialog_slider_0().TickFrequency(freq0);
			dialog_slider_0().SnapsTo(snap0);
			dialog_slider_0().Value(val0);
			dialog_slider_0().Visibility(vis0);
			dialog_slider_1().Minimum(min1);
			dialog_slider_1().Maximum(max1);
			dialog_slider_1().TickFrequency(freq1);
			dialog_slider_1().SnapsTo(snap1);
			dialog_slider_1().Value(val1);
			dialog_slider_1().Visibility(vis1);
			dialog_slider_2().Minimum(min2);
			dialog_slider_2().Maximum(max2);
			dialog_slider_2().TickFrequency(freq2);
			dialog_slider_2().SnapsTo(snap2);
			dialog_slider_2().Value(val2);
			dialog_slider_2().Visibility(vis2);
			dialog_radio_btns().Visibility(Visibility::Collapsed);
			//dialog_check_box().Checked(token3);
			//dialog_check_box().Unchecked(token4);
			//dialog_check_box().IsChecked(val3);
			//dialog_check_box().Visibility(vis3);
			m_mutex_event.unlock();
			slist_clear(m_prop_page.m_shape_list);
		}
		status_bar_set_pos();
	}

	// 操作メニューの「枠を文字列に合わせる」が選択された.
	void MainPage::meth_text_fit_frame_click(IInspectable const&, RoutedEventArgs const&)
	{
		auto flag = false;
		//const auto g_len = (m_main_page.m_snap_grid ? m_main_page.m_grid_base + 1.0f : 0.0f);
		const auto g_len = (m_snap_grid ? m_main_page.m_grid_base + 1.0f : 0.0f);
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted()) {
				continue;
			}
			else if (!s->is_selected()) {
				continue;
			}
			else if (typeid(*s) != typeid(ShapeText)) {
				continue;
			}
			auto u = new UndoDeform(s, LOC_TYPE::LOC_SE);
			if (static_cast<ShapeText*>(s)->fit_frame_to_text(g_len)) {
				undo_push_null();
				m_ustack_undo.push_back(u);
				if (!flag) {
					flag = true;
				}
			}
			else {
				delete u;
			}
		}
		if (flag) {
			undo_menu_is_enabled();
			main_panel_size();
			main_draw();
		}
		status_bar_set_pos();
	}


}