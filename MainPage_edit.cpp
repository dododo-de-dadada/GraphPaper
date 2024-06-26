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
	using winrt::Windows::UI::Xaml::Visibility;

	// 操作メニューの「画像を原画像に戻す」が選択された.
	void MainPage::revert_image_to_original_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		bool push_null = false;
		for (SHAPE* const s : m_main_sheet.m_shape_list) {
			if (s->is_deleted() || !s->is_selected() || typeid(*s) != typeid(SHAPE_IMAGE)) {
				continue;
			}
			// 画像の現在の位置や大きさ、不透明度を操作スタックにプッシュする.
			if (!push_null) {
				undo_push_null();
				push_null = true;
			}
			undo_push_image(s);
			static_cast<SHAPE_IMAGE*>(s)->revert();
		}
		main_panel_size();
		main_sheet_draw();
		status_bar_set_pointer();
	}

	// 「反時計周りに円弧を描く」/「時計周りに円弧を描く」が選択された
	void MainPage::draw_arc_direction_click(const IInspectable& sender, const RoutedEventArgs& /*args*/)
	{
		if (sender == menu_draw_arc_ccw() || sender == popup_draw_arc_ccw()) {
			undo_push_null();
			if (undo_push_set<UNDO_T::ARC_DIR>(D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE)) {
				menu_is_enable();
				main_sheet_draw();
			}
		}
		else if (sender == menu_draw_arc_cw() || sender == popup_draw_arc_cw()) {
			undo_push_null();
			if (undo_push_set<UNDO_T::ARC_DIR>(D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE)) {
				menu_is_enable();
				main_sheet_draw();
			}
		}
	}


	void MainPage::open_or_close_poly_end_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		const D2D1_FIGURE_END END = (sender == menu_open_polygon() || sender == popup_open_polygon() ? D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED);
		bool push_null = false;
		for (SHAPE* s : m_main_sheet.m_shape_list) {
			if (s->is_deleted()) {
				continue;
			}
			if (!s->is_selected()) {
				continue;
			}
			D2D1_FIGURE_END end;
			if (s->get_poly_end(end) && end != END) {
				if (!push_null) {
					undo_push_null();
					push_null = true;
				}
				undo_push_set<UNDO_T::POLY_END>(s, END);
			}
		}
		if (push_null) {
			menu_is_enable();
			main_sheet_draw();
		}
	}

	/*
	IAsyncAction MainPage::edit_arc_async(SHAPE_ARC* t)
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

		m_mutex_event.lock();
		m_dialog_sheet.set_attr_to(&m_main_sheet);

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
		SHAPE* s = new SHAPE_ARC(start, pos, t);
		s->set_select(true);
		m_dialog_sheet.m_shape_list.push_back(s);
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
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_arc_start](auto const&, auto const& args) {
					// (IInspectable const&, RangeBaseValueChangedEventArgs const& args)
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					swprintf_s(buf, L"%f°", val);
					dialog_slider_0().Header(box_value(str_arc_start + buf));
					D2D1_SWEEP_DIRECTION dir;
					m_dialog_sheet.slist_back()->get_arc_dir(dir);
					if (dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
						if (m_dialog_sheet.slist_back()->set_arc_start(val)) {
							dialog_draw();
						}
					}
					else {
						if (m_dialog_sheet.slist_back()->set_arc_end(-val)) {
							dialog_draw();
						}
					}
				})
			};
			const auto revoker1{
				dialog_slider_1().ValueChanged(winrt::auto_revoke, [this, str_arc_end](auto const&, auto const& args) {
					// (IInspectable const&, RangeBaseValueChangedEventArgs const& args)
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					swprintf_s(buf, L"%f°", val);
					dialog_slider_1().Header(box_value(str_arc_end + buf));
					D2D1_SWEEP_DIRECTION dir;
					m_dialog_sheet.slist_back()->get_arc_dir(dir);
					if (dir == D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE) {
						if (m_dialog_sheet.slist_back()->set_arc_end(val)) {
							dialog_draw();
						}
					}
					else {
						if (m_dialog_sheet.slist_back()->set_arc_start(-val)) {
							dialog_draw();
						}
					}
				})
			};
			const auto revoker2{
				dialog_slider_2().ValueChanged(winrt::auto_revoke, [this, str_arc_rot](auto const&, auto const& args) {
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					swprintf_s(buf, L"%f°", val);
					dialog_slider_2().Header(box_value(str_arc_rot + buf));
					m_dialog_sheet.slist_back()->set_arc_rot(val);
					dialog_draw();
				})
			};
			const auto revoker3{
				dialog_radio_btns().SelectionChanged(winrt::auto_revoke, [this](auto const&, auto const&) {
					// (IInspectable const&, SelectionChangedEventArgs const&)
					if (dialog_radio_btns().SelectedIndex() == 0) {
						if (m_dialog_sheet.slist_back()->set_arc_dir(D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE)) {
							const auto val0 = dialog_slider_0().Value();
							const auto val1 = dialog_slider_1().Value();
							dialog_slider_0().Value(-val1);
							dialog_slider_1().Value(-val0);
							dialog_draw();
						}
					}
					else if (dialog_radio_btns().SelectedIndex() == 1) {
						if (m_dialog_sheet.slist_back()->set_arc_dir(D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE)) {
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
				SHAPE* s = m_dialog_sheet.slist_back();
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
				main_sheet_draw();
			}
		}
		slist_clear(m_dialog_sheet.m_shape_list);

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
		m_mutex_event.unlock();
		slist_clear(m_dialog_sheet.m_shape_list);
	}
	*/
}