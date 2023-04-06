//-------------------------------
// MainPage_color.cpp
// 線枠, 塗りつぶし, 書体, 方眼, ページの色
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

	template<UNDO_T U>
	static void color_get(const ShapePage& p, D2D1_COLOR_F& c)
	{
		if constexpr (U == UNDO_T::FILL_COLOR) {
			p.back()->get_fill_color(c);
		}
		else if constexpr (U == UNDO_T::FONT_COLOR) {
			p.back()->get_font_color(c);
		}
		else if constexpr (U == UNDO_T::GRID_COLOR) {
			p.get_grid_color(c);
		}
		else if constexpr (U == UNDO_T::PAGE_COLOR) {
			p.get_page_color(c);
		}
		else if constexpr (U == UNDO_T::STROKE_COLOR) {
			p.back()->get_stroke_color(c);
		}
	}

	template<UNDO_T U>
	static bool color_set(ShapePage& p, const D2D1_COLOR_F& c)
	{
		if constexpr (U == UNDO_T::FILL_COLOR) {
			return p.back()->set_fill_color(c);
		}
		else if constexpr (U == UNDO_T::FONT_COLOR) {
			return p.back()->set_font_color(c);
		}
		else if constexpr (U == UNDO_T::GRID_COLOR) {
			return p.set_grid_color(c);
		}
		else if constexpr (U == UNDO_T::IMAGE_OPAC) {
			return p.set_image_opacity(c);
		}
		else if constexpr (U == UNDO_T::PAGE_COLOR) {
			return p.set_page_color(c);
		}
		else if constexpr (U == UNDO_T::STROKE_COLOR) {
			return p.back()->set_stroke_color(c);
		}
		else {
			return false;
		}
	}

	template<UNDO_T U>
	static bool color_ustack_set(MainPage& that, const D2D1_COLOR_F& c)
	{
		if constexpr (U == UNDO_T::FILL_COLOR) {
			return that.ustack_push_set<UNDO_T::FILL_COLOR>(c);
		}
		else if constexpr (U == UNDO_T::FONT_COLOR) {
			return that.ustack_push_set<UNDO_T::FONT_COLOR>(c);
		}
		else if constexpr (U == UNDO_T::GRID_COLOR) {
			if (!equal(that.m_main_page.m_grid_color, c)) {
				that.ustack_push_set<UNDO_T::GRID_COLOR>(&that.m_main_page, c);
				return true;
			}
			return false;
		}
		else if constexpr (U == UNDO_T::PAGE_COLOR) {
			if (!equal(that.m_main_page.m_page_color, c)) {
				that.ustack_push_set<UNDO_T::PAGE_COLOR>(&that.m_main_page, c);
				return true;
			}
			return false;
		}
		else if constexpr (U == UNDO_T::STROKE_COLOR) {
			return that.ustack_push_set<UNDO_T::STROKE_COLOR>(c);
		}
		else {
			return false;
		}

	}

	template<UNDO_T U>
	IAsyncAction MainPage::color_click_async(void)
	{
		m_mutex_event.lock();
		m_prop_page.set_attr_to(&m_main_page);

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
			m_prop_page.m_shape_list.push_back(new ShapeRect(start, pos, &m_prop_page));
			m_prop_page.back()->set_select(true);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
		}
		else if constexpr (U == UNDO_T::STROKE_COLOR) {
			m_prop_page.m_shape_list.push_back(new ShapeLine(start, pos, &m_prop_page));
			m_prop_page.back()->set_select(true);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
		}
		else if constexpr (U == UNDO_T::FONT_COLOR) {
			const auto pangram = ResourceLoader::GetForCurrentView().GetString(L"str_pangram");
			if (pangram.empty()) {
				m_prop_page.m_shape_list.push_back(new ShapeText(start, pos, wchar_cpy(L"The quick brown fox jumps over a lazy dog."), &m_prop_page));
			}
			else {
				m_prop_page.m_shape_list.push_back(new ShapeText(start, pos, wchar_cpy(pangram.data()), &m_prop_page));
			}
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
		}

		D2D1_COLOR_F val;
		color_get<U>(m_prop_page, val);
		const float val0 = static_cast<float>(conv_color_comp(val.r));
		const float val1 = static_cast<float>(conv_color_comp(val.g));
		const float val2 = static_cast<float>(conv_color_comp(val.b));
		const float val3 = static_cast<float>(conv_color_comp(val.a));

		dialog_slider_0().Minimum(0.0);
		dialog_slider_0().Maximum(255.0);
		dialog_slider_0().TickFrequency(1.0);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val0);

		const auto str_color_r{ ResourceLoader::GetForCurrentView().GetString(L"str_color_r") + L": " };
		const auto str_color_g{ ResourceLoader::GetForCurrentView().GetString(L"str_color_g") + L": " };
		const auto str_color_b{ ResourceLoader::GetForCurrentView().GetString(L"str_color_b") + L": " };
		const auto str_opacity{ ResourceLoader::GetForCurrentView().GetString(L"str_opacity") + L": " };
		const auto str_color_code{ mfsi_color_code().Text() };

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
		else if constexpr (U == UNDO_T::PAGE_COLOR) {
			res = L"str_page_color";
		}
		const auto str_title{ ResourceLoader::GetForCurrentView().GetString(res) };

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
		dialog_combo_box_0().Items().Append(box_value(rmfi_color_code_dec().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_color_code_hex().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_color_code_real().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_color_code_pct().Text()));
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
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_color_r](auto, auto args) {
					// (IInspectable const&, RangeBaseValueChangedEventArgs const& args)
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_col_to_str(m_color_code, val, buf);
					dialog_slider_0().Header(box_value(str_color_r + buf));
					D2D1_COLOR_F color;
					color_get<U>(m_prop_page, color);
					color.r = static_cast<FLOAT>(val / COLOR_MAX);
					if (color_set<U>(m_prop_page, color)) {
						dialog_draw();
					}
				})
			};
			const auto revoker1{
				dialog_slider_1().ValueChanged(winrt::auto_revoke, [this, str_color_g](auto, auto args) {
					// (IInspectable const&, RangeBaseValueChangedEventArgs const& args)
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_col_to_str(m_color_code, val, buf);
					dialog_slider_1().Header(box_value(str_color_g + buf));
					D2D1_COLOR_F color;
					color_get<U>(m_prop_page, color);
					color.g = static_cast<FLOAT>(val / COLOR_MAX);
					if (color_set<U>(m_prop_page, color)) {
						dialog_draw();
					}
				})
			};
			const auto revoker2{
				dialog_slider_2().ValueChanged(winrt::auto_revoke, [this, str_color_b](auto, auto args) {
					// (IInspectable const&, RangeBaseValueChangedEventArgs const& args)
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_col_to_str(m_color_code, val, buf);
					dialog_slider_2().Header(box_value(str_color_b + buf));
					D2D1_COLOR_F color;
					color_get<U>(m_prop_page, color);
					color.b = static_cast<FLOAT>(val / COLOR_MAX);
					if (color_set<U>(m_prop_page, color)) {
						dialog_draw();
					}
				})
			};
			const auto revoker3{
				dialog_slider_3().ValueChanged(winrt::auto_revoke, [this, str_opacity](auto, auto args) {
					// (IInspectable const&, RangeBaseValueChangedEventArgs const& args)
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_col_to_str(m_color_code, val, buf);
					dialog_slider_3().Header(box_value(str_opacity + buf));
					D2D1_COLOR_F color;
					color_get<U>(m_prop_page, color);
					color.a = static_cast<FLOAT>(val / COLOR_MAX);
					if (color_set<U>(m_prop_page, color)) {
						dialog_draw();
					}
				})
			};
			const auto revoker4{
				dialog_combo_box_0().SelectionChanged(winrt::auto_revoke, [this, str_color_r, str_color_g, str_color_b, str_opacity](auto, auto) {
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
				color_get<U>(m_prop_page, new_val);
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
				if (color_ustack_set<U>(*this, new_val)) {
					ustack_push_null();
					ustack_is_enable();
					main_draw();
				}
			}
		}
		slist_clear(m_prop_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		dialog_slider_2().Visibility(Visibility::Collapsed);
		dialog_slider_3().Visibility(Visibility::Collapsed);
		dialog_combo_box_0().Visibility(Visibility::Collapsed);
		dialog_combo_box_0().Items().Clear();
		main_draw();
		m_mutex_event.unlock();
	}

	// 実体化する.
	template IAsyncAction MainPage::color_click_async<UNDO_T::FONT_COLOR>();
	template IAsyncAction MainPage::color_click_async<UNDO_T::FILL_COLOR>();
	template IAsyncAction MainPage::color_click_async<UNDO_T::GRID_COLOR>();
	template IAsyncAction MainPage::color_click_async<UNDO_T::PAGE_COLOR>();
	template IAsyncAction MainPage::color_click_async<UNDO_T::STROKE_COLOR>();
}