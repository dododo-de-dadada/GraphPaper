//------------------------------
// MainPage_prop.cpp
// �}�`�̑��� (�j��, ���̑���, �[�_, ���̌���)
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

	// �������j���[�́u�[�̌`���v���I�����ꂽ.
	void MainPage::prop_cap_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		//CAP_STYLE new_val;
		D2D1_CAP_STYLE new_val;
		if (sender == rmfi_menu_cap_style_flat() || sender == rmfi_popup_cap_style_flat()) {
			new_val = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;
		}
		else if (sender == rmfi_menu_cap_style_square() || sender == rmfi_popup_cap_style_square()) {
			new_val = D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE;
		}
		else if (sender == rmfi_menu_cap_style_round() || sender == rmfi_popup_cap_style_round()) {
			new_val = D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND;
		}
		else if (sender == rmfi_menu_cap_style_triangle() || rmfi_popup_cap_style_triangle()) {
			new_val = D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE;
		}
		else {
			throw winrt::hresult_not_implemented();
			return;
		}
		prop_cap_style_is_checked(new_val);
		if (undo_push_set<UNDO_T::STROKE_CAP>(new_val)) {
			undo_push_null();
			undo_menu_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	// �������j���[�́u�[�̌`���v�Ɉ������.
	void MainPage::prop_cap_style_is_checked(const D2D1_CAP_STYLE& val)
	{
		rmfi_menu_cap_style_flat().IsChecked(equal(val, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT));
		rmfi_popup_cap_style_flat().IsChecked(equal(val, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT));
		rmfi_menu_cap_style_square().IsChecked(equal(val, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE));
		rmfi_popup_cap_style_square().IsChecked(equal(val, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE));
		rmfi_menu_cap_style_round().IsChecked(equal(val, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND));
		rmfi_popup_cap_style_round().IsChecked(equal(val, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND));
		rmfi_menu_cap_style_triangle().IsChecked(equal(val, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE));
		rmfi_popup_cap_style_triangle().IsChecked(equal(val, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE));
	}

	// �������j���[�́u���̌����̌`���v>�u��萧���v���I�����ꂽ.
	IAsyncAction MainPage::prop_join_limit_click_async(IInspectable const& sender, RoutedEventArgs const&)
	{
		//using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		//using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		//using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		const auto str_join_miter_limit{ ResourceLoader::GetForCurrentView().GetString(L"str_join_miter_limit") + L": " };
		const auto str_stroke_width{ ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " };
		const auto str_title{ ResourceLoader::GetForCurrentView().GetString(L"str_join_miter_limit") };
		wchar_t buf[32];

		m_prop_page.set_attr_to(&m_main_page);
		const auto unit = m_len_unit;
		const auto dpi = m_prop_d2d.m_logical_dpi;
		const auto g_len = m_prop_page.m_grid_base + 1.0f;
		float j_limit;
		m_prop_page.get_join_miter_limit(j_limit);
		j_limit -= 1.0f;

		dialog_slider_0().Minimum(0.0);
		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(j_limit);
		dialog_slider_0().Visibility(Visibility::Visible);
		//join_slider_set_header<0>(j_limit);
		swprintf_s(buf, L"%.1lf", static_cast<double>(j_limit) + 1.0);
		dialog_slider_0().Header(box_value(str_join_miter_limit + buf));

		float s_width;
		m_prop_page.get_stroke_width(s_width);

		dialog_slider_1().Minimum(0.0);
		dialog_slider_1().Maximum(MAX_VALUE);
		dialog_slider_1().TickFrequency(TICK_FREQ);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(s_width);
		dialog_slider_1().Visibility(Visibility::Visible);
		//join_slider_set_header<1>(s_width);
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, s_width, dpi, g_len, buf);
		dialog_slider_1().Header(box_value(str_stroke_width + buf));

		const auto samp_w = scp_dialog_panel().Width();
		const auto samp_h = scp_dialog_panel().Height();
		const auto mar = samp_w * 0.125;
		const D2D1_POINT_2F start{
			static_cast<FLOAT>(mar), static_cast<FLOAT>(mar)
		};
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(samp_w - 2.0 * mar), static_cast<FLOAT>(samp_h - 2.0 * mar)
		};
		POLY_OPTION p_opt{ 3, true, true, false, true };
		auto s = new ShapePoly(start, pos, &m_prop_page, p_opt);
		const float offset = static_cast<float>(samp_h / 16.0);
		const float samp_x = static_cast<float>(samp_w * 0.25);
		const float samp_y = static_cast<float>(samp_h * 0.5);
		s->set_select(true);
		s->set_pos_loc(D2D1_POINT_2F{ -samp_x, samp_y - offset }, LOC_TYPE::LOC_P0, m_snap_point, false);
		s->set_pos_loc(D2D1_POINT_2F{ samp_x, samp_y }, LOC_TYPE::LOC_P0 + 1, m_snap_point, false);
		s->set_pos_loc(D2D1_POINT_2F{ -samp_x, samp_y + offset }, LOC_TYPE::LOC_P0 + 2, m_snap_point, false);
		m_prop_page.m_shape_list.push_back(s);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif

		cd_dialog_prop().Title(box_value(str_title));
		m_mutex_event.lock();
		{
			const auto revoker0{
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_join_miter_limit](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					wchar_t buf[32];
					const float val = static_cast<float>(args.NewValue());
					swprintf_s(buf, L"%.1lf", static_cast<double>(val) + 1.0);
					dialog_slider_0().Header(box_value(str_join_miter_limit + buf));
					if (m_prop_page.back()->set_join_miter_limit(val + 1.0f)) {
						dialog_draw();
					}
				})
			};
			const auto revoker1{
				dialog_slider_1().ValueChanged(winrt::auto_revoke, [this, str_stroke_width](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					wchar_t buf[32];
					const float val = static_cast<float>(args.NewValue());
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_1().Header(box_value(str_stroke_width + buf));
					if (m_prop_page.back()->set_stroke_width(val)) {
						dialog_draw();
					}
				})
			};
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				float new_limit;
				float new_width;
				m_prop_page.back()->get_join_miter_limit(new_limit);
				m_prop_page.back()->get_stroke_width(new_width);
				const bool limit_changed = undo_push_set<UNDO_T::JOIN_LIMIT>(new_limit);
				const bool width_changed = undo_push_set<UNDO_T::STROKE_WIDTH>(new_width);
				if (limit_changed || width_changed) {
					undo_push_null();
					undo_menu_is_enabled();
					main_draw();
				}
			}
		}
		slist_clear(m_prop_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		m_mutex_event.unlock();
	}

	// �������j���[�́u�j���̔z�u�v���I�����ꂽ.
	IAsyncAction MainPage::prop_dash_pat_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_mutex_event.lock();

		const winrt::hstring str_dash_len{ ResourceLoader::GetForCurrentView().GetString(L"str_dash_len") + L": " };
		const winrt::hstring str_dash_gap{ ResourceLoader::GetForCurrentView().GetString(L"str_dash_gap") + L": " };
		const winrt::hstring str_dot_len{ ResourceLoader::GetForCurrentView().GetString(L"str_dot_len") + L": " };
		const winrt::hstring str_dot_gap{ ResourceLoader::GetForCurrentView().GetString(L"str_dot_gap") + L": " };
		const winrt::hstring str_stroke_width{ ResourceLoader::GetForCurrentView().GetString(L"mfsi_stroke_width/Text") + L": " };
		const winrt::hstring str_cap_style{ mfsi_menu_cap_style().Text() + L": " };

		// �܂�, �_�C�A���O�y�[�W�̑�����, ���C���y�[�W�Ɠ����ɂ���.
		m_prop_page.set_attr_to(&m_main_page);
		// ���{�}�`�̍쐬
		const auto p_width = scp_dialog_panel().Width();
		const auto p_height = scp_dialog_panel().Height();
		const auto mar = p_width * 0.125;
		const D2D1_POINT_2F start{
			static_cast<FLOAT>(mar), static_cast<FLOAT>(mar)
		};
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(p_width - 2.0 * mar), static_cast<FLOAT>(p_height - 2.0 * mar)
		};
		m_prop_page.m_shape_list.push_back(new ShapeLine(start, pos, &m_prop_page));
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		DASH_PAT d_patt;
		m_prop_page.get_dash_pat(d_patt);
		float s_width;
		m_prop_page.get_stroke_width(s_width);
		D2D1_DASH_STYLE d_style;
		m_prop_page.get_dash_style(d_style);
		D2D1_CAP_STYLE c_style;
		m_prop_page.get_stroke_cap(c_style);

		const auto unit = m_len_unit;
		const auto dpi = m_prop_d2d.m_logical_dpi;
		const auto g_len = m_prop_page.m_grid_base + 1.0f;
		wchar_t buf[32];

		dialog_slider_0().Minimum(0.0);
		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(d_patt.m_[0]);
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, d_patt.m_[0], dpi, g_len, buf);
		dialog_slider_0().Header(box_value(str_dash_len + buf));

		dialog_slider_1().Minimum(0.0);
		dialog_slider_1().Maximum(MAX_VALUE);
		dialog_slider_1().TickFrequency(TICK_FREQ);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(d_patt.m_[1]);
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, d_patt.m_[1], dpi, g_len, buf);
		dialog_slider_1().Header(box_value(str_dash_gap + buf));

		dialog_slider_2().Minimum(0.0);
		dialog_slider_2().Maximum(MAX_VALUE);
		dialog_slider_2().TickFrequency(TICK_FREQ);
		dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_2().Value(d_patt.m_[2]);
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, d_patt.m_[2], dpi, g_len, buf);
		dialog_slider_2().Header(box_value(str_dot_len + buf));

		dialog_slider_3().Minimum(0.0);
		dialog_slider_3().Maximum(MAX_VALUE);
		dialog_slider_3().TickFrequency(TICK_FREQ);
		dialog_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_3().Value(d_patt.m_[3]);
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, d_patt.m_[3], dpi, g_len, buf);
		dialog_slider_3().Header(box_value(str_dot_gap + buf));

		dialog_slider_4().Minimum(0.0);
		dialog_slider_4().Maximum(MAX_VALUE);
		dialog_slider_4().TickFrequency(TICK_FREQ);
		dialog_slider_4().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_4().Value(s_width);
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, s_width, dpi, g_len, buf);
		dialog_slider_4().Header(box_value(str_stroke_width + buf));
		dialog_slider_4().Visibility(Visibility::Visible);

		dialog_combo_box_0().Header(box_value(mfsi_menu_dash_style().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_dash_style_dash().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_dash_style_dot().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_dash_style_dash_dot().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_dash_style_dash_dot_dot().Text()));
		if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH) {
			dialog_combo_box_0().SelectedIndex(0);
			dialog_slider_0().Visibility(Visibility::Visible);
			dialog_slider_1().Visibility(Visibility::Visible);
		}
		else if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT) {
			dialog_combo_box_0().SelectedIndex(1);
			dialog_slider_2().Visibility(Visibility::Visible);
			dialog_slider_3().Visibility(Visibility::Visible);
		}
		else if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT) {
			dialog_combo_box_0().SelectedIndex(2);
			dialog_slider_0().Visibility(Visibility::Visible);
			dialog_slider_1().Visibility(Visibility::Visible);
			dialog_slider_2().Visibility(Visibility::Visible);
			dialog_slider_3().Visibility(Visibility::Visible);
		}
		else if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
			dialog_combo_box_0().SelectedIndex(3);
			dialog_slider_0().Visibility(Visibility::Visible);
			dialog_slider_1().Visibility(Visibility::Visible);
			dialog_slider_2().Visibility(Visibility::Visible);
			dialog_slider_3().Visibility(Visibility::Visible);
		}
		dialog_combo_box_0().Visibility(Visibility::Visible);

		dialog_combo_box_1().Header(box_value(str_cap_style));
		dialog_combo_box_1().Items().Append(box_value(rmfi_menu_cap_style_flat().Text()));
		dialog_combo_box_1().Items().Append(box_value(rmfi_menu_cap_style_square().Text()));
		dialog_combo_box_1().Items().Append(box_value(rmfi_menu_cap_style_round().Text()));
		if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT) {
			dialog_combo_box_1().SelectedIndex(0);
		}
		else if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
			dialog_combo_box_1().SelectedIndex(1);
		}
		else if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
			dialog_combo_box_1().SelectedIndex(2);
		}
		else if (c_style == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
			dialog_combo_box_1().Items().Append(box_value(rmfi_menu_cap_style_triangle().Text()));
			dialog_combo_box_1().SelectedIndex(3);
		}
		dialog_combo_box_1().Visibility(Visibility::Visible);

		cd_dialog_prop().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(L"str_dash_patern")));
		{
			const auto revoker0{
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_dash_len](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					DASH_PAT patt;
					m_prop_page.back()->get_dash_pat(patt);
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_0().Header(box_value(str_dash_len + buf));
					patt.m_[0] = static_cast<FLOAT>(val);
					if (m_prop_page.back()->set_dash_pat(patt)) {
						dialog_draw();
					}
				})
			};
			const auto revoker1{
				dialog_slider_1().ValueChanged(winrt::auto_revoke, [this, str_dash_gap](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					DASH_PAT patt;
					m_prop_page.back()->get_dash_pat(patt);
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_1().Header(box_value(str_dash_gap + buf));
					patt.m_[1] = static_cast<FLOAT>(val);
					if (m_prop_page.back()->set_dash_pat(patt)) {
						dialog_draw();
					}
				})
			};
			const auto revoker2{
				dialog_slider_2().ValueChanged(winrt::auto_revoke, [this, str_dot_len](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					DASH_PAT patt;
					m_prop_page.back()->get_dash_pat(patt);
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_2().Header(box_value(str_dot_len + buf));
					patt.m_[2] = static_cast<FLOAT>(val);
					if (m_prop_page.back()->set_dash_pat(patt)) {
						dialog_draw();
					}
				})
			};
			const auto revoker3{
				dialog_slider_3().ValueChanged(winrt::auto_revoke, [this, str_dot_gap](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					DASH_PAT patt;
					m_prop_page.back()->get_dash_pat(patt);
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_3().Header(box_value(str_dot_gap + buf));
					patt.m_[3] = static_cast<FLOAT>(val);
					if (m_prop_page.back()->set_dash_pat(patt)) {
						dialog_draw();
					}
				})
			};
			const auto revoker4{
				dialog_slider_4().ValueChanged(winrt::auto_revoke, [this, str_stroke_width](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_4().Header(box_value(str_stroke_width + buf));
					if (m_prop_page.back()->set_stroke_width(val)) {
						dialog_draw();
					}
				})
			};
			const auto revoker5{
				dialog_combo_box_0().SelectionChanged(winrt::auto_revoke, [this](IInspectable const&, SelectionChangedEventArgs const&) {
					if (dialog_combo_box_0().SelectedIndex() == 0) {
						if (m_prop_page.back()->set_dash_style(D2D1_DASH_STYLE_DASH)) {
							dialog_slider_0().Visibility(Visibility::Visible);
							dialog_slider_1().Visibility(Visibility::Visible);
							dialog_slider_2().Visibility(Visibility::Collapsed);
							dialog_slider_3().Visibility(Visibility::Collapsed);
							dialog_draw();
						}
					}
					else if (dialog_combo_box_0().SelectedIndex() == 1) {
						if (m_prop_page.back()->set_dash_style(D2D1_DASH_STYLE_DOT)) {
							dialog_slider_0().Visibility(Visibility::Collapsed);
							dialog_slider_1().Visibility(Visibility::Collapsed);
							dialog_slider_2().Visibility(Visibility::Visible);
							dialog_slider_3().Visibility(Visibility::Visible);
							dialog_draw();
						}
					}
					else if (dialog_combo_box_0().SelectedIndex() == 2) {
						if (m_prop_page.back()->set_dash_style(D2D1_DASH_STYLE_DASH_DOT)) {
							dialog_slider_0().Visibility(Visibility::Visible);
							dialog_slider_1().Visibility(Visibility::Visible);
							dialog_slider_2().Visibility(Visibility::Visible);
							dialog_slider_3().Visibility(Visibility::Visible);
							dialog_draw();
						}
					}
					else if (dialog_combo_box_0().SelectedIndex() == 3) {
						if (m_prop_page.back()->set_dash_style(D2D1_DASH_STYLE_DASH_DOT_DOT)) {
							dialog_slider_0().Visibility(Visibility::Visible);
							dialog_slider_1().Visibility(Visibility::Visible);
							dialog_slider_2().Visibility(Visibility::Visible);
							dialog_slider_3().Visibility(Visibility::Visible);
							dialog_draw();
						}
					}
				})
			};
			const auto revoker6{
				dialog_combo_box_1().SelectionChanged(winrt::auto_revoke, [this](IInspectable const&, SelectionChangedEventArgs const&) {
					if (dialog_combo_box_1().SelectedIndex() == 0) {
						if (m_prop_page.back()->set_stroke_cap(D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT)) {
							dialog_draw();
						}
					}
					else if (dialog_combo_box_1().SelectedIndex() == 1) {
						if (m_prop_page.back()->set_stroke_cap(D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE)) {
							dialog_draw();
						}
					}
					else if (dialog_combo_box_1().SelectedIndex() == 2) {
						if (m_prop_page.back()->set_stroke_cap(D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND)) {
							dialog_draw();
						}
					}
					else if (dialog_combo_box_1().SelectedIndex() == 3) {
						if (m_prop_page.back()->set_stroke_cap(D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE)) {
							dialog_draw();
						}
					}
				})
			};
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				DASH_PAT new_patt;
				float new_width;
				D2D1_DASH_STYLE new_dash;
				D2D1_CAP_STYLE new_cap;
				m_prop_page.back()->get_dash_pat(new_patt);
				m_prop_page.back()->get_stroke_width(new_width);
				m_prop_page.back()->get_dash_style(new_dash);
				m_prop_page.back()->get_stroke_cap(new_cap);
				prop_dash_style_is_checked(new_dash);
				prop_cap_style_is_checked(new_cap);
				const bool flag_patt = undo_push_set<UNDO_T::DASH_PAT>(new_patt);
				const bool flag_width = undo_push_set<UNDO_T::STROKE_WIDTH>(new_width);
				const bool flag_dash = undo_push_set<UNDO_T::DASH_STYLE>(new_dash);
				const bool flag_cap = undo_push_set<UNDO_T::STROKE_CAP>(new_cap);
				if (flag_patt || flag_width || flag_dash || flag_cap) {
					undo_push_null();
					undo_menu_is_enabled();
					main_draw();
				}
			}
		}
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		dialog_slider_2().Visibility(Visibility::Collapsed);
		dialog_slider_3().Visibility(Visibility::Collapsed);
		dialog_slider_4().Visibility(Visibility::Collapsed);
		dialog_combo_box_0().Visibility(Visibility::Collapsed);
		dialog_combo_box_0().Items().Clear();
		dialog_combo_box_1().Visibility(Visibility::Collapsed);
		dialog_combo_box_1().Items().Clear();
		status_bar_set_pos();
		slist_clear(m_prop_page.m_shape_list);
		m_mutex_event.unlock();
	}

	// �������j���[�́u�j���̌`���v�̃T�u���ڂ��I�����ꂽ.
	void MainPage::prop_dash_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		D2D1_DASH_STYLE d_style = static_cast<D2D1_DASH_STYLE>(-1);
		if (sender == rmfi_menu_dash_style_solid() || sender == rmfi_popup_dash_style_solid()) {
			d_style = D2D1_DASH_STYLE_SOLID;
		}
		else if (sender == rmfi_menu_dash_style_dash() || sender == rmfi_popup_dash_style_dash()) {
			d_style = D2D1_DASH_STYLE_DASH;
		}
		else if (sender == rmfi_menu_dash_style_dot() || sender == rmfi_popup_dash_style_dot()) {
			d_style = D2D1_DASH_STYLE_DOT;
		}
		else if (sender == rmfi_menu_dash_style_dash_dot() || sender == rmfi_popup_dash_style_dash_dot()) {
			d_style = D2D1_DASH_STYLE_DASH_DOT;
		}
		else if (sender == rmfi_menu_dash_style_dash_dot_dot() || sender == rmfi_popup_dash_style_dash_dot_dot()) {
			d_style = D2D1_DASH_STYLE_DASH_DOT_DOT;
		}
		if (d_style != static_cast<D2D1_DASH_STYLE>(-1)) {
			mfi_menu_dash_pat().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);
			if (undo_push_set<UNDO_T::DASH_STYLE>(d_style)) {
				undo_push_null();
				undo_menu_is_enabled();
				//xcvd_menu_is_enabled();
				main_draw();
			}
		}
		status_bar_set_pos();
	}

	// �������j���[�́u�j���̌`���v�Ɉ������.
	// d_style	�j���̌`��
	void MainPage::prop_dash_style_is_checked(const D2D1_DASH_STYLE d_style)
	{
		rmfi_menu_dash_style_solid().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		rmfi_popup_dash_style_solid().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		rmfi_menu_dash_style_dash().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH);
		rmfi_popup_dash_style_dash().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH);
		rmfi_menu_dash_style_dash_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT);
		rmfi_popup_dash_style_dash_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT);
		rmfi_menu_dash_style_dash_dot_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT);
		rmfi_popup_dash_style_dash_dot_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT);
		rmfi_menu_dash_style_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT);
		rmfi_popup_dash_style_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT);

		mfi_menu_dash_pat().IsEnabled(d_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		mfi_popup_dash_pat().IsEnabled(d_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
	}

	// �������j���[�́u���̌����̌`���v���I�����ꂽ.
	void MainPage::prop_join_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		D2D1_LINE_JOIN new_val;
		if (sender == rmfi_menu_join_style_bevel() || sender == rmfi_popup_join_style_bevel()) {
			new_val = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;
		}
		else if (sender == rmfi_menu_join_style_miter() || sender == rmfi_popup_join_style_miter()) {
			new_val = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER;
		}
		else if (sender == rmfi_menu_join_style_miter_or_bevel() || sender == rmfi_popup_join_style_miter_or_bevel()) {
			new_val = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL;
		}
		else if (sender == rmfi_menu_join_style_round() || sender == rmfi_popup_join_style_round()) {
			new_val = D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND;
		}
		else {
			throw winrt::hresult_not_implemented();
			return;
		}
		prop_join_style_is_checked(new_val);
		if (undo_push_set<UNDO_T::JOIN_STYLE>(new_val)) {
			undo_push_null();
			undo_menu_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	// �������j���[�́u���̌����v�̃T�u���ڂɈ������.
	// s_join	���̌���
	void MainPage::prop_join_style_is_checked(const D2D1_LINE_JOIN val)
	{
		rmfi_menu_join_style_bevel().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL);
		rmfi_popup_join_style_bevel().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL);
		rmfi_menu_join_style_miter_or_bevel().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL);
		rmfi_popup_join_style_miter_or_bevel().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL);
		rmfi_menu_join_style_miter().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
		rmfi_popup_join_style_miter().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
		rmfi_menu_join_style_round().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND);
		rmfi_popup_join_style_round().IsChecked(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND);
		mfi_menu_join_miter_limit().IsEnabled(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL);
		mfi_popup_join_miter_limit().IsEnabled(val == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL);
	}

	// �������j���[�́u�����v�̃T�u���ڂ��I�����ꂽ.
	void MainPage::prop_stroke_width_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		float s_width = -1.0f;
		if (sender == rmfi_menu_stroke_width_0px() || sender == rmfi_popup_stroke_width_0px()) {
			s_width = 0.0f;
		}
		else if (sender == rmfi_menu_stroke_width_1px() || sender == rmfi_popup_stroke_width_1px()) {
			s_width = 1.0f;
		}
		else if (sender == rmfi_menu_stroke_width_2px() || sender == rmfi_popup_stroke_width_2px()) {
			s_width = 2.0f;
		}
		else if (sender == rmfi_menu_stroke_width_3px() || sender == rmfi_popup_stroke_width_3px()) {
			s_width = 3.0f;
		}
		else if (sender == rmfi_menu_stroke_width_4px() || sender == rmfi_popup_stroke_width_4px()) {
			s_width = 4.0f;
		}
		else if (sender == rmfi_menu_stroke_width_8px() || sender == rmfi_popup_stroke_width_8px()) {
			s_width = 8.0f;
		}
		else if (sender == rmfi_menu_stroke_width_12px() || sender == rmfi_popup_stroke_width_12px()) {
			s_width = 12.0f;
		}
		else if (sender == rmfi_menu_stroke_width_16px() || sender == rmfi_popup_stroke_width_16px()) {
			s_width = 16.0f;
		}
		if (s_width >= 0.0f) {
			prop_stroke_width_is_checked(s_width);
			if (undo_push_set<UNDO_T::STROKE_WIDTH>(s_width)) {
				undo_push_null();
				undo_menu_is_enabled();
				//xcvd_menu_is_enabled();
				main_draw();
			}
		}
		status_bar_set_pos();
	}

	void MainPage::prop_stroke_width_is_checked(const float s_width) noexcept
	{
		rmfi_menu_stroke_width_0px().IsChecked(s_width == 0.0f);
		rmfi_popup_stroke_width_0px().IsChecked(s_width == 0.0f);
		rmfi_menu_stroke_width_1px().IsChecked(s_width == 1.0f);
		rmfi_popup_stroke_width_1px().IsChecked(s_width == 1.0f);
		rmfi_menu_stroke_width_2px().IsChecked(s_width == 2.0f);
		rmfi_popup_stroke_width_2px().IsChecked(s_width == 2.0f);
		rmfi_menu_stroke_width_3px().IsChecked(s_width == 3.0f);
		rmfi_popup_stroke_width_3px().IsChecked(s_width == 3.0f);
		rmfi_menu_stroke_width_4px().IsChecked(s_width == 4.0f);
		rmfi_popup_stroke_width_4px().IsChecked(s_width == 4.0f);
		rmfi_menu_stroke_width_8px().IsChecked(s_width == 8.0f);
		rmfi_popup_stroke_width_8px().IsChecked(s_width == 8.0f);
		rmfi_menu_stroke_width_12px().IsChecked(s_width == 12.0f);
		rmfi_popup_stroke_width_12px().IsChecked(s_width == 12.0f);
		rmfi_menu_stroke_width_16px().IsChecked(s_width == 16.0f);
		rmfi_popup_stroke_width_16px().IsChecked(s_width == 16.0f);
		const bool others = (s_width != 1.0f && s_width != 2.0f && s_width != 3.0f && s_width != 4.0f && s_width != 8.0f && s_width != 16.0f);
		rmfi_menu_stroke_width_other().IsChecked(others);
		rmfi_popup_stroke_width_other().IsChecked(others);
	}

	// �������j���[�́u�����v>�u���̑��v���I�����ꂽ.
	IAsyncAction MainPage::prop_stroke_width_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		const winrt::hstring str_stroke_width{ ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": "};
		const winrt::hstring str_title{ ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") };
		m_prop_page.set_attr_to(&m_main_page);
		const auto panel_w = scp_dialog_panel().Width();
		const auto panel_h = scp_dialog_panel().Height();
		const auto m = panel_w * 0.125;	// �]��
		const D2D1_POINT_2F start{	// �n�_
			static_cast<FLOAT>(m), static_cast<FLOAT>(m)
		};
		const D2D1_POINT_2F pos{	// �I�_�ւ̈ʒu�x�N�g��
			static_cast<FLOAT>(panel_w - 2.0 * m), static_cast<FLOAT>(panel_h - 2.0 * m)
		};
		m_prop_page.m_shape_list.push_back(new ShapeLine(start, pos, &m_prop_page));
		m_prop_page.back()->set_select(true);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
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

		cd_dialog_prop().Title(box_value(str_title));
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
					if (m_prop_page.back()->set_stroke_width(val)) {
						dialog_draw();
					}
				})
			};
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				float new_val;
				m_prop_page.back()->get_stroke_width(new_val);
				prop_stroke_width_is_checked(new_val);
				if (undo_push_set<UNDO_T::STROKE_WIDTH>(new_val)) {
					undo_push_null();
					undo_menu_is_enabled();
					//xcvd_menu_is_enabled();
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

	// �������j���[�́u�摜�̕s�����x...�v���I�����ꂽ.
	IAsyncAction MainPage::prop_image_opac_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		const winrt::hstring str_opacity{ ResourceLoader::GetForCurrentView().GetString(L"str_opacity") + L": " };
		const winrt::hstring str_title{ ResourceLoader::GetForCurrentView().GetString(L"str_image_opac") };
		const winrt::hstring str_color_code{ ResourceLoader::GetForCurrentView().GetString(L"str_color_code") };

		m_mutex_event.lock();
		m_prop_page.set_attr_to(&m_main_page);

		dialog_image_load_async(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()));

		const float val = static_cast<float>(conv_color_comp(m_prop_page.m_image_opac));
		dialog_slider_0().Minimum(0.0);
		dialog_slider_0().Maximum(255.0);
		dialog_slider_0().TickFrequency(1.0);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val);
		wchar_t buf[32];
		conv_col_to_str(m_color_code, val, buf);
		dialog_slider_0().Header(box_value(str_opacity + buf));
		dialog_slider_0().Visibility(Visibility::Visible);

		dialog_combo_box_0().Header(box_value(str_color_code));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_color_code_dec().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_color_code_hex().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_color_code_real().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_color_code_pct().Text()));
		if (m_color_code == COLOR_CODE::DEC) {
			dialog_combo_box_0().SelectedIndex(0);
		}
		else if (m_color_code == COLOR_CODE::HEX) {
			dialog_combo_box_0().SelectedIndex(1);
		}
		else if (m_color_code == COLOR_CODE::REAL) {
			dialog_combo_box_0().SelectedIndex(2);
		}
		else if (m_color_code == COLOR_CODE::PCT) {
			dialog_combo_box_0().SelectedIndex(3);
		}
		dialog_combo_box_0().Visibility(Visibility::Visible);

		cd_dialog_prop().Title(box_value(str_title));
		{
			const auto revoker{
				dialog_slider_0().ValueChanged(
					winrt::auto_revoke,
					[this, str_opacity](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
						const float val = static_cast<float>(args.NewValue());
						wchar_t buf[32];
						conv_col_to_str(m_color_code, val, buf);
						dialog_slider_0().Header(box_value(str_opacity + buf));
						if (m_prop_page.back()->set_image_opacity(val / COLOR_MAX)) {
							dialog_draw();
						}
					}
				)
			};
			const auto revoker4{
				dialog_combo_box_0().SelectionChanged(winrt::auto_revoke, [this, str_opacity](IInspectable const&, SelectionChangedEventArgs const&) {
					COLOR_CODE c_code;
					if (dialog_combo_box_0().SelectedIndex() == 0) {
						c_code = COLOR_CODE::DEC;
					}
					else if (dialog_combo_box_0().SelectedIndex() == 1) {
						c_code = COLOR_CODE::HEX;
					}
					else if (dialog_combo_box_0().SelectedIndex() == 2) {
						c_code = COLOR_CODE::REAL;
					}
					else if (dialog_combo_box_0().SelectedIndex() == 3) {
						c_code = COLOR_CODE::PCT;
					}
					else {
						return;
					}
					wchar_t buf[32];
					conv_col_to_str(c_code, dialog_slider_0().Value(), buf);
					dialog_slider_0().Header(box_value(str_opacity + buf));
				})
			};
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				float new_val;
				m_prop_page.back()->get_image_opacity(new_val);
				if (dialog_combo_box_0().SelectedIndex() == 0) {
					m_color_code = COLOR_CODE::DEC;
				}
				else if (dialog_combo_box_0().SelectedIndex() == 1) {
					m_color_code = COLOR_CODE::HEX;
				}
				else if (dialog_combo_box_0().SelectedIndex() == 2) {
					m_color_code = COLOR_CODE::REAL;
				}
				else if (dialog_combo_box_0().SelectedIndex() == 3) {
					m_color_code = COLOR_CODE::PCT;
				}
				color_code_is_checked(m_color_code);
				if (undo_push_set<UNDO_T::IMAGE_OPAC>(new_val)) {
					undo_push_null();
					undo_menu_is_enabled();
					main_draw();
				}
			}
		}
		slist_clear(m_prop_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_combo_box_0().Visibility(Visibility::Collapsed);
		dialog_combo_box_0().Items().Clear();
		main_draw();
		m_mutex_event.unlock();
	}

}