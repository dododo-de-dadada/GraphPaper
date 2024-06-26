//-------------------------------
// MainPage_color.cpp
// 線枠, 塗りつぶし, 書体, 方眼, 用紙の色
//-------------------------------
#include "pch.h"
#include "MainPage.h"
#include "undo.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::UI::Xaml::Visibility;

	// 色を得る.
	// U	操作の種類
	// sheet	用紙図形
	// color	得られた色
	template<UNDO_T U>
	static void color_get(const SHAPE_SHEET& sheet, D2D1_COLOR_F& color)
	{
		if constexpr (U == UNDO_T::FILL_COLOR) {
			sheet.slist_back()->get_fill_color(color);
		}
		else if constexpr (U == UNDO_T::FONT_COLOR) {
			sheet.slist_back()->get_font_color(color);
		}
		else if constexpr (U == UNDO_T::GRID_COLOR) {
			sheet.get_grid_color(color);
		}
		else if constexpr (U == UNDO_T::SHEET_COLOR) {
			sheet.get_sheet_color(color);
		}
		else if constexpr (U == UNDO_T::STROKE_COLOR) {
			sheet.slist_back()->get_stroke_color(color);
		}
	}

	template<UNDO_T U>
	static bool color_set(SHAPE_SHEET& sheet, const D2D1_COLOR_F& color)
	{
		if constexpr (U == UNDO_T::FILL_COLOR) {
			return sheet.slist_back()->set_fill_color(color);
		}
		else if constexpr (U == UNDO_T::FONT_COLOR) {
			return sheet.slist_back()->set_font_color(color);
		}
		else if constexpr (U == UNDO_T::GRID_COLOR) {
			return sheet.set_grid_color(color);
		}
		else if constexpr (U == UNDO_T::IMAGE_OPAC) {
			return sheet.set_image_opacity(color);
		}
		else if constexpr (U == UNDO_T::SHEET_COLOR) {
			return sheet.set_sheet_color(color);
		}
		else if constexpr (U == UNDO_T::STROKE_COLOR) {
			return sheet.slist_back()->set_stroke_color(color);
		}
		else {
			return false;
		}
	}

	template<UNDO_T U>
	static bool color_ustack_set(MainPage& that, const D2D1_COLOR_F& c)
	{
		if constexpr (U == UNDO_T::FILL_COLOR) {
			that.undo_push_null();
			return that.undo_push_set<UNDO_T::FILL_COLOR>(c);
		}
		else if constexpr (U == UNDO_T::FONT_COLOR) {
			that.undo_push_null();
			return that.undo_push_set<UNDO_T::FONT_COLOR>(c);
		}
		else if constexpr (U == UNDO_T::GRID_COLOR) {
			if (!equal(that.m_main_sheet.m_grid_color, c)) {
				that.undo_push_null();
				that.undo_push_set<UNDO_T::GRID_COLOR>(&that.m_main_sheet, c);
				return true;
			}
			return false;
		}
		else if constexpr (U == UNDO_T::SHEET_COLOR) {
			if (!equal(that.m_main_sheet.m_sheet_color, c)) {
				that.undo_push_null();
				that.undo_push_set<UNDO_T::SHEET_COLOR>(&that.m_main_sheet, c);
				return true;
			}
			return false;
		}
		else if constexpr (U == UNDO_T::STROKE_COLOR) {
			that.undo_push_null();
			return that.undo_push_set<UNDO_T::STROKE_COLOR>(c);
		}
		else {
			return false;
		}

	}

	template<UNDO_T U>
	IAsyncAction MainPage::color_click_async(void)
	{
		m_mutex_event.lock();
		m_dialog_sheet.set_attr_to(&m_main_sheet);

		const auto p_width = scp_dialog_panel().Width();
		const auto p_height = scp_dialog_panel().Height();
		const auto mar = p_width * 0.125;
		const D2D1_POINT_2F start{
			static_cast<FLOAT>(mar), static_cast<FLOAT>(mar)
		};
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(p_width - 2.0 * mar), static_cast<FLOAT>(p_height - 2.0 * mar)
		};
		if constexpr (U == UNDO_T::FILL_COLOR) {
			m_dialog_sheet.m_shape_list.push_back(new ShapeRect(start, pos, &m_dialog_sheet));
			m_dialog_sheet.slist_back()->set_select(true);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
		}
		else if constexpr (U == UNDO_T::STROKE_COLOR) {
			m_dialog_sheet.m_shape_list.push_back(new ShapeLine(start, pos, &m_dialog_sheet));
			m_dialog_sheet.slist_back()->set_select(true);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
		}
		else if constexpr (U == UNDO_T::FONT_COLOR) {
			const auto pangram = ResourceLoader::GetForCurrentView().GetString(L"str_pangram");
			if (pangram.empty()) {
				m_dialog_sheet.m_shape_list.push_back(new ShapeText(start, pos, wchar_cpy(L"The quick brown fox jumps over a lazy dog."), &m_dialog_sheet));
			}
			else {
				m_dialog_sheet.m_shape_list.push_back(new ShapeText(start, pos, wchar_cpy(pangram.data()), &m_dialog_sheet));
			}
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
		}

		D2D1_COLOR_F val;
		color_get<U>(m_dialog_sheet, val);
		const float val0 = static_cast<float>(conv_color_comp(val.r));
		const float val1 = static_cast<float>(conv_color_comp(val.g));
		const float val2 = static_cast<float>(conv_color_comp(val.b));
		const float val3 = static_cast<float>(conv_color_comp(val.a));

		dialog_slider_0().Minimum(0.0);
		dialog_slider_0().Maximum(255.0);
		dialog_slider_0().TickFrequency(1.0);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val0);

		const auto str_color_r{
			ResourceLoader::GetForCurrentView().GetString(L"str_color_r") + L": "
		};
		const auto str_color_g{
			ResourceLoader::GetForCurrentView().GetString(L"str_color_g") + L": "
		};
		const auto str_color_b{
			ResourceLoader::GetForCurrentView().GetString(L"str_color_b") + L": "
		};
		const auto str_opacity{
			ResourceLoader::GetForCurrentView().GetString(L"str_opacity") + L": "
		};
		const auto str_color_code{
			mfsi_menu_color_code().Text() + L": "
		};

		wchar_t* res = nullptr;
		if constexpr (U == UNDO_T::FILL_COLOR) {
			res = L"str_fill_color";
		}
		else if constexpr (U == UNDO_T::FONT_COLOR) {
			res = L"str_font_color";
		}
		else if constexpr (U == UNDO_T::GRID_COLOR) {
			res = L"str_grid_color";
		}
		else if constexpr (U == UNDO_T::STROKE_COLOR) {
			res = L"str_stroke_color";
		}
		else if constexpr (U == UNDO_T::SHEET_COLOR) {
			res = L"str_sheet_color";
		}
		const auto str_title{
			ResourceLoader::GetForCurrentView().GetString(res)
		};

		wchar_t buf[32];
		conv_col_to_str(m_color_code, val0, buf);
		dialog_slider_0().Header(box_value(str_color_r + buf));
		dialog_slider_1().Minimum(0.0);
		dialog_slider_1().Maximum(255.0);
		dialog_slider_1().TickFrequency(1.0);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(val1);
		conv_col_to_str(m_color_code, val1, buf);
		dialog_slider_1().Header(box_value(str_color_g + buf));
		dialog_slider_2().Minimum(0.0);
		dialog_slider_2().Maximum(255.0);
		dialog_slider_2().TickFrequency(1.0);
		dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_2().Value(val2);
		conv_col_to_str(m_color_code, val2, buf);
		dialog_slider_2().Header(box_value(str_color_b + buf));
		dialog_slider_3().Minimum(0.0);
		dialog_slider_3().Maximum(255.0);
		dialog_slider_3().TickFrequency(1.0);
		dialog_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_3().Value(val3);
		conv_col_to_str(m_color_code, val3, buf);
		dialog_slider_3().Header(box_value(str_opacity + buf));

		dialog_slider_0().Visibility(Visibility::Visible);
		dialog_slider_1().Visibility(Visibility::Visible);
		dialog_slider_2().Visibility(Visibility::Visible);
		dialog_slider_3().Visibility(Visibility::Visible);

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
			const auto revoker0{
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_color_r](auto const&, auto const& args) {
					// (IInspectable const&, RangeBaseValueChangedEventArgs const& args)
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_col_to_str(m_color_code, val, buf);
					dialog_slider_0().Header(box_value(str_color_r + buf));
					D2D1_COLOR_F color;
					color_get<U>(m_dialog_sheet, color);
					color.r = static_cast<FLOAT>(val / COLOR_MAX);
					if (color_set<U>(m_dialog_sheet, color)) {
						dialog_draw();
					}
				})
			};
			const auto revoker1{
				dialog_slider_1().ValueChanged(winrt::auto_revoke, [this, str_color_g](auto const&, auto const& args) {
					// (IInspectable const&, RangeBaseValueChangedEventArgs const& args)
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_col_to_str(m_color_code, val, buf);
					dialog_slider_1().Header(box_value(str_color_g + buf));
					D2D1_COLOR_F color;
					color_get<U>(m_dialog_sheet, color);
					color.g = static_cast<FLOAT>(val / COLOR_MAX);
					if (color_set<U>(m_dialog_sheet, color)) {
						dialog_draw();
					}
				})
			};
			const auto revoker2{
				dialog_slider_2().ValueChanged(winrt::auto_revoke, [this, str_color_b](auto const&, auto const& args) {
					// (IInspectable const&, RangeBaseValueChangedEventArgs const& args)
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_col_to_str(m_color_code, val, buf);
					dialog_slider_2().Header(box_value(str_color_b + buf));
					D2D1_COLOR_F color;
					color_get<U>(m_dialog_sheet, color);
					color.b = static_cast<FLOAT>(val / COLOR_MAX);
					if (color_set<U>(m_dialog_sheet, color)) {
						dialog_draw();
					}
				})
			};
			const auto revoker3{
				dialog_slider_3().ValueChanged(winrt::auto_revoke, [this, str_opacity](auto const&, auto const& args) {
					// (IInspectable const&, RangeBaseValueChangedEventArgs const& args)
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_col_to_str(m_color_code, val, buf);
					dialog_slider_3().Header(box_value(str_opacity + buf));
					D2D1_COLOR_F color;
					color_get<U>(m_dialog_sheet, color);
					color.a = static_cast<FLOAT>(val / COLOR_MAX);
					if (color_set<U>(m_dialog_sheet, color)) {
						dialog_draw();
					}
				})
			};
			const auto revoker4{
				dialog_combo_box_0().SelectionChanged(winrt::auto_revoke, [this, str_color_r, str_color_g, str_color_b, str_opacity](auto const&, auto const&) {
					// (IInspectable const&, SelectionChangedEventArgs const&)
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
					wchar_t buf[4][32];
					conv_col_to_str(c_code, dialog_slider_0().Value(), buf[0]);
					conv_col_to_str(c_code, dialog_slider_1().Value(), buf[1]);
					conv_col_to_str(c_code, dialog_slider_2().Value(), buf[2]);
					conv_col_to_str(c_code, dialog_slider_3().Value(), buf[3]);
					dialog_slider_0().Header(box_value(str_color_r + buf[0]));
					dialog_slider_1().Header(box_value(str_color_g + buf[1]));
					dialog_slider_2().Header(box_value(str_color_b + buf[2]));
					dialog_slider_3().Header(box_value(str_opacity + buf[3]));
				})
			};
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				D2D1_COLOR_F new_val;
				color_get<U>(m_dialog_sheet, new_val);
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
				if (color_ustack_set<U>(*this, new_val)) {
					main_sheet_draw();
				}
			}
		}
		slist_clear(m_dialog_sheet.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		dialog_slider_2().Visibility(Visibility::Collapsed);
		dialog_slider_3().Visibility(Visibility::Collapsed);
		dialog_combo_box_0().Visibility(Visibility::Collapsed);
		dialog_combo_box_0().Items().Clear();
		main_sheet_draw();
		m_mutex_event.unlock();
	}

	// 実体化する.
	template IAsyncAction MainPage::color_click_async<UNDO_T::FONT_COLOR>();
	template IAsyncAction MainPage::color_click_async<UNDO_T::FILL_COLOR>();
	template IAsyncAction MainPage::color_click_async<UNDO_T::GRID_COLOR>();
	template IAsyncAction MainPage::color_click_async<UNDO_T::SHEET_COLOR>();
	template IAsyncAction MainPage::color_click_async<UNDO_T::STROKE_COLOR>();

	// その他メニューの「色の基数」のサブ項目が選択された.
	void MainPage::color_code_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == rmfi_menu_color_code_pct()) {
			m_color_code = COLOR_CODE::PCT;
		}
		else if (sender == rmfi_menu_color_code_dec()) {
			m_color_code = COLOR_CODE::DEC;
		}
		else if (sender == rmfi_menu_color_code_hex()) {
			m_color_code = COLOR_CODE::HEX;
		}
		else if (sender == rmfi_menu_color_code_real()) {
			m_color_code = COLOR_CODE::REAL;
		}
		else {
			throw winrt::hresult_not_implemented();
			return;
		}
		status_bar_set_pointer();
	}

	// 属性メニューの「画像の不透明度...」が選択された.
	IAsyncAction MainPage::color_image_opac_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		const winrt::hstring str_opacity{ 
			ResourceLoader::GetForCurrentView().GetString(L"str_opacity") + L": "
		};
		const winrt::hstring str_title{
			ResourceLoader::GetForCurrentView().GetString(L"str_image_opac")
		};
		const winrt::hstring str_color_code{
			mfsi_menu_color_code().Text() + L": "
		};

		m_mutex_event.lock();
		m_dialog_sheet.set_attr_to(&m_main_sheet);

		dialog_image_load_async(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()));

		const float val = static_cast<float>(conv_color_comp(m_dialog_sheet.m_image_opac));
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
					[this, str_opacity](auto const&, auto const& args) {
						// IInspectable const&, RangeBaseValueChangedEventArgs const& args
						const float val = static_cast<float>(args.NewValue());
						wchar_t buf[32];
						conv_col_to_str(m_color_code, val, buf);
						dialog_slider_0().Header(box_value(str_opacity + buf));
						if (m_dialog_sheet.slist_back()->set_image_opacity(val / COLOR_MAX)) {
							dialog_draw();
						}
					}
				)
			};
			const auto revoker4{
				dialog_combo_box_0().SelectionChanged(winrt::auto_revoke, [this, str_opacity](auto const&, auto const&) {
					// IInspectable const&, SelectionChangedEventArgs const&
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
				m_dialog_sheet.slist_back()->get_image_opacity(new_val);
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
				undo_push_null();
				if (undo_push_set<UNDO_T::IMAGE_OPAC>(new_val)) {
					main_sheet_draw();
				}
			}
		}
		slist_clear(m_dialog_sheet.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_combo_box_0().Visibility(Visibility::Collapsed);
		dialog_combo_box_0().Items().Clear();
		main_sheet_draw();
		m_mutex_event.unlock();
	}

}