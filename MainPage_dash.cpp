//------------------------------
// MainPage_dash.cpp
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
	using winrt::Windows::UI::Xaml::Controls::Slider;

	// 値をスライダーのヘッダーに格納する.
	// U	操作の識別子
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	/*
	static void dash_slider_set_header(Slider const& slider, const wchar_t* res, const LEN_UNIT unit, const float dpi, const float g_len, const float val)
	{
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
		slider.Header(box_value(ResourceLoader::GetForCurrentView().GetString(res) + L": " + buf));
	}
	*/

	/*
	void MainPage::dash_style_selection_changed(
		IInspectable const&, SelectionChangedEventArgs const&) noexcept
	{
		if (dialog_combo_box().SelectedIndex() == 0) {
			if (m_prop_page.m_shape_list.back()->set_dash_style(D2D1_DASH_STYLE_DASH)) {
				dialog_slider_0().Visibility(Visibility::Visible);
				dialog_slider_1().Visibility(Visibility::Visible);
				dialog_slider_2().Visibility(Visibility::Collapsed);
				dialog_slider_3().Visibility(Visibility::Collapsed);
				prop_dialog_draw();
			}
		}
		else if (dialog_combo_box().SelectedIndex() == 1) {
			if (m_prop_page.m_shape_list.back()->set_dash_style(D2D1_DASH_STYLE_DOT)) {
				dialog_slider_0().Visibility(Visibility::Collapsed);
				dialog_slider_1().Visibility(Visibility::Collapsed);
				dialog_slider_2().Visibility(Visibility::Visible);
				dialog_slider_3().Visibility(Visibility::Visible);
				prop_dialog_draw();
			}
		}
		else if (dialog_combo_box().SelectedIndex() == 2) {
			if (m_prop_page.m_shape_list.back()->set_dash_style(D2D1_DASH_STYLE_DASH_DOT)) {
				dialog_slider_0().Visibility(Visibility::Visible);
				dialog_slider_1().Visibility(Visibility::Visible);
				dialog_slider_2().Visibility(Visibility::Visible);
				dialog_slider_3().Visibility(Visibility::Visible);
				prop_dialog_draw();
			}
		}
		else if (dialog_combo_box().SelectedIndex() == 3) {
			if (m_prop_page.m_shape_list.back()->set_dash_style(D2D1_DASH_STYLE_DASH_DOT_DOT)) {
				dialog_slider_0().Visibility(Visibility::Visible);
				dialog_slider_1().Visibility(Visibility::Visible);
				dialog_slider_2().Visibility(Visibility::Visible);
				dialog_slider_3().Visibility(Visibility::Visible);
				prop_dialog_draw();
			}
		}
	}
	*/

	// 線枠メニューの「破線の配置」が選択された.
	IAsyncAction MainPage::dash_pat_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_mutex_event.lock();
		// まず, ダイアログページの属性を, メインページと同じにする.
		m_prop_page.set_attr_to(&m_main_page);
		// 見本図形の作成
		const auto p_width = scp_prop_panel().Width();
		const auto p_height = scp_prop_panel().Height();
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
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		const winrt::hstring str_dash_len{ ResourceLoader::GetForCurrentView().GetString(L"str_dash_len") + L": " };
		const winrt::hstring str_dash_gap{ ResourceLoader::GetForCurrentView().GetString(L"str_dash_gap") + L": " };
		const winrt::hstring str_dot_len{ ResourceLoader::GetForCurrentView().GetString(L"str_dot_len") + L": " };
		const winrt::hstring str_dot_gap{ ResourceLoader::GetForCurrentView().GetString(L"str_dot_gap") + L": " };
		const winrt::hstring str_stroke_width{ ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " };
		DASH_PAT d_patt;
		m_prop_page.get_dash_pat(d_patt);
		float s_width;
		m_prop_page.get_stroke_width(s_width);
		D2D1_DASH_STYLE d_style;
		m_prop_page.get_dash_style(d_style);

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

		dialog_combo_box().Header(box_value(mfsi_dash_style().Text()));
		dialog_combo_box().Items().Append(box_value(rmfi_dash_style_dash().Text()));
		dialog_combo_box().Items().Append(box_value(rmfi_dash_style_dot().Text()));
		dialog_combo_box().Items().Append(box_value(rmfi_dash_style_dash_dot().Text()));
		dialog_combo_box().Items().Append(box_value(rmfi_dash_style_dash_dot_dot().Text()));
		if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH) {
			dialog_combo_box().SelectedIndex(0);
			dialog_slider_0().Visibility(Visibility::Visible);
			dialog_slider_1().Visibility(Visibility::Visible);
		}
		else if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT) {
			dialog_combo_box().SelectedIndex(1);
			dialog_slider_2().Visibility(Visibility::Visible);
			dialog_slider_3().Visibility(Visibility::Visible);
		}
		else if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT) {
			dialog_combo_box().SelectedIndex(2);
			dialog_slider_0().Visibility(Visibility::Visible);
			dialog_slider_1().Visibility(Visibility::Visible);
			dialog_slider_2().Visibility(Visibility::Visible);
			dialog_slider_3().Visibility(Visibility::Visible);
		}
		else if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
			dialog_combo_box().SelectedIndex(3);
			dialog_slider_0().Visibility(Visibility::Visible);
			dialog_slider_1().Visibility(Visibility::Visible);
			dialog_slider_2().Visibility(Visibility::Visible);
			dialog_slider_3().Visibility(Visibility::Visible);
		}
		dialog_slider_4().Visibility(Visibility::Visible);
		dialog_combo_box().Visibility(Visibility::Visible);
		cd_setting_dialog().Title(
			box_value(ResourceLoader::GetForCurrentView().GetString(L"str_dash_patern")));
		{
			const auto revoker0{
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_dash_len](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_prop_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					DASH_PAT patt;
					m_prop_page.m_shape_list.back()->get_dash_pat(patt);
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_0().Header(box_value(str_dash_len + buf));
					patt.m_[0] = static_cast<FLOAT>(val);
					if (m_prop_page.m_shape_list.back()->set_dash_pat(patt)) {
						prop_dialog_draw();
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
					m_prop_page.m_shape_list.back()->get_dash_pat(patt);
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_1().Header(box_value(str_dash_gap + buf));
					patt.m_[1] = static_cast<FLOAT>(val);
					if (m_prop_page.m_shape_list.back()->set_dash_pat(patt)) {
						prop_dialog_draw();
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
					m_prop_page.m_shape_list.back()->get_dash_pat(patt);
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_2().Header(box_value(str_dot_len + buf));
					patt.m_[2] = static_cast<FLOAT>(val);
					if (m_prop_page.m_shape_list.back()->set_dash_pat(patt)) {
						prop_dialog_draw();
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
					m_prop_page.m_shape_list.back()->get_dash_pat(patt);
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_3().Header(box_value(str_dot_gap + buf));
					patt.m_[3] = static_cast<FLOAT>(val);
					if (m_prop_page.m_shape_list.back()->set_dash_pat(patt)) {
						prop_dialog_draw();
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
					if (m_prop_page.m_shape_list.back()->set_stroke_width(val)) {
						prop_dialog_draw();
					}
				})
			};
			const auto revoker5{
				dialog_combo_box().SelectionChanged(winrt::auto_revoke, [this](IInspectable const&, SelectionChangedEventArgs const&) {
					if (dialog_combo_box().SelectedIndex() == 0) {
						if (m_prop_page.m_shape_list.back()->set_dash_style(D2D1_DASH_STYLE_DASH)) {
							dialog_slider_0().Visibility(Visibility::Visible);
							dialog_slider_1().Visibility(Visibility::Visible);
							dialog_slider_2().Visibility(Visibility::Collapsed);
							dialog_slider_3().Visibility(Visibility::Collapsed);
							prop_dialog_draw();
						}
					}
					else if (dialog_combo_box().SelectedIndex() == 1) {
						if (m_prop_page.m_shape_list.back()->set_dash_style(D2D1_DASH_STYLE_DOT)) {
							dialog_slider_0().Visibility(Visibility::Collapsed);
							dialog_slider_1().Visibility(Visibility::Collapsed);
							dialog_slider_2().Visibility(Visibility::Visible);
							dialog_slider_3().Visibility(Visibility::Visible);
							prop_dialog_draw();
						}
					}
					else if (dialog_combo_box().SelectedIndex() == 2) {
						if (m_prop_page.m_shape_list.back()->set_dash_style(D2D1_DASH_STYLE_DASH_DOT)) {
							dialog_slider_0().Visibility(Visibility::Visible);
							dialog_slider_1().Visibility(Visibility::Visible);
							dialog_slider_2().Visibility(Visibility::Visible);
							dialog_slider_3().Visibility(Visibility::Visible);
							prop_dialog_draw();
						}
					}
					else if (dialog_combo_box().SelectedIndex() == 3) {
						if (m_prop_page.m_shape_list.back()->set_dash_style(D2D1_DASH_STYLE_DASH_DOT_DOT)) {
							dialog_slider_0().Visibility(Visibility::Visible);
							dialog_slider_1().Visibility(Visibility::Visible);
							dialog_slider_2().Visibility(Visibility::Visible);
							dialog_slider_3().Visibility(Visibility::Visible);
							prop_dialog_draw();
						}
					}
				})
			};
			if (co_await cd_setting_dialog().ShowAsync() == ContentDialogResult::Primary) {
				DASH_PAT new_patt;
				float new_width;
				D2D1_DASH_STYLE new_style;
				m_prop_page.m_shape_list.back()->get_dash_pat(new_patt);
				m_prop_page.m_shape_list.back()->get_stroke_width(new_width);
				m_prop_page.m_shape_list.back()->get_dash_style(new_style);
				dash_style_is_checked(new_style);
				const bool flag_patt = ustack_push_set<UNDO_T::DASH_PAT>(new_patt);
				const bool flag_width = ustack_push_set<UNDO_T::STROKE_WIDTH>(new_width);
				const bool flag_style = ustack_push_set<UNDO_T::DASH_STYLE>(new_style);
				if (flag_patt || flag_width || flag_style) {
					ustack_push_null();
					xcvd_is_enabled();
					page_draw();
				}
			}
		}
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		dialog_slider_2().Visibility(Visibility::Collapsed);
		dialog_slider_3().Visibility(Visibility::Collapsed);
		dialog_slider_4().Visibility(Visibility::Collapsed);
		dialog_combo_box().Visibility(Visibility::Collapsed);
		dialog_combo_box().Items().Clear();
		status_bar_set_pos();
		slist_clear(m_prop_page.m_shape_list);
		m_mutex_event.unlock();
	}

	// 線枠メニューの「破線の形式」のサブ項目が選択された.
	void MainPage::dash_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		D2D1_DASH_STYLE d_style;
		if (sender == rmfi_dash_style_solid()) {
			d_style = D2D1_DASH_STYLE_SOLID;
		}
		else if (sender == rmfi_dash_style_dash()) {
			d_style = D2D1_DASH_STYLE_DASH;
		}
		else if (sender == rmfi_dash_style_dot()) {
			d_style = D2D1_DASH_STYLE_DOT;
		}
		else if (sender == rmfi_dash_style_dash_dot()) {
			d_style = D2D1_DASH_STYLE_DASH_DOT;
		}
		else if (sender == rmfi_dash_style_dash_dot_dot()) {
			d_style = D2D1_DASH_STYLE_DASH_DOT_DOT;
		}
		else {
			return;
		}
		mfi_dash_pat().IsEnabled(d_style != D2D1_DASH_STYLE_SOLID);
		if (ustack_push_set<UNDO_T::DASH_STYLE>(d_style)) {
			ustack_push_null();
			xcvd_is_enabled();
			page_draw();
		}
		status_bar_set_pos();
	}

	// 線枠メニューの「破線の形式」に印をつける.
	// d_style	破線の形式
	void MainPage::dash_style_is_checked(const D2D1_DASH_STYLE d_style)
	{
		rmfi_dash_style_solid().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
		rmfi_dash_style_dash().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH);
		rmfi_dash_style_dash_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT);
		rmfi_dash_style_dash_dot_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT);
		rmfi_dash_style_dot().IsChecked(d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT);

		mfi_dash_pat().IsEnabled(d_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
	}

	// スライダーの値が変更された.
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	/*
	template <int S>
	void MainPage::dash_slider_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (S == 0) {
			const float val = static_cast<float>(args.NewValue());
			DASH_PAT patt;
			m_prop_page.m_shape_list.back()->get_dash_pat(patt);
			dash_slider_set_header<S>(val);
			patt.m_[0] = static_cast<FLOAT>(val);
			if (m_prop_page.m_shape_list.back()->set_dash_pat(patt)) {
				prop_dialog_draw();
			}
		}
		else if constexpr (S == 1) {
			const float val = static_cast<float>(args.NewValue());
			DASH_PAT patt;
			m_prop_page.m_shape_list.back()->get_dash_pat(patt);
			dash_slider_set_header<S>(val);
			patt.m_[1] = static_cast<FLOAT>(val);
			if (m_prop_page.m_shape_list.back()->set_dash_pat(patt)) {
				prop_dialog_draw();
			}
		}
		else if constexpr (S == 2) {
			const float val = static_cast<float>(args.NewValue());
			DASH_PAT patt;
			m_prop_page.m_shape_list.back()->get_dash_pat(patt);
			dash_slider_set_header<S>(val);
			patt.m_[2] = patt.m_[4] = static_cast<FLOAT>(val);
			if (m_prop_page.m_shape_list.back()->set_dash_pat(patt)) {
				prop_dialog_draw();
			}
		}
		else if constexpr (S == 3) {
			const float val = static_cast<float>(args.NewValue());
			DASH_PAT patt;
			m_prop_page.m_shape_list.back()->get_dash_pat(patt);
			dash_slider_set_header<S>(val);
			patt.m_[3] = patt.m_[5] = static_cast<FLOAT>(val);
			if (m_prop_page.m_shape_list.back()->set_dash_pat(patt)) {
				prop_dialog_draw();
			}
		}
		else if constexpr (S == 4) {
			const float val = static_cast<float>(args.NewValue());
			dash_slider_set_header<S>(val);
			if (m_prop_page.m_shape_list.back()->set_stroke_width(val)) {
				prop_dialog_draw();
			}
		}
	}
		*/
}