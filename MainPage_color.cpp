//-------------------------------
// MainPage_color.cpp
// ê¸òg, ìhÇËÇ¬Ç‘Çµ, èëëÃ, ï˚ä·, ÉyÅ[ÉWÇÃêF
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
			p.m_shape_list.back()->get_fill_color(c);
		}
		else if constexpr (U == UNDO_T::FONT_COLOR) {
			p.m_shape_list.back()->get_font_color(c);
		}
		else if constexpr (U == UNDO_T::GRID_COLOR) {
			p.get_grid_color(c);
		}
		else if constexpr (U == UNDO_T::PAGE_COLOR) {
			p.get_page_color(c);
		}
		else if constexpr (U == UNDO_T::STROKE_COLOR) {
			p.m_shape_list.back()->get_stroke_color(c);
		}
	}

	template<UNDO_T U>
	static bool color_set(ShapePage& p, const D2D1_COLOR_F& c)
	{
		if constexpr (U == UNDO_T::FILL_COLOR) {
			return p.m_shape_list.back()->set_fill_color(c);
		}
		else if constexpr (U == UNDO_T::FONT_COLOR) {
			return p.m_shape_list.back()->set_font_color(c);
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
			return p.m_shape_list.back()->set_stroke_color(c);
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
		m_dialog_page.set_attr_to(&m_main_page);

		const auto p_width = scp_dialog_panel().Width();
		const auto p_height = scp_dialog_panel().Height();
		const auto pad = p_width * 0.125;
		const D2D1_POINT_2F start{
			static_cast<FLOAT>(pad), static_cast<FLOAT>(pad)
		};
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(p_width - 2.0 * pad), static_cast<FLOAT>(p_height - 2.0 * pad)
		};
		if constexpr (U == UNDO_T::FILL_COLOR) {
			m_dialog_page.m_shape_list.push_back(new ShapeRect(start, pos, &m_dialog_page));
			m_dialog_page.m_shape_list.back()->set_select(true);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
		}
		else if constexpr (U == UNDO_T::STROKE_COLOR) {
			m_dialog_page.m_shape_list.push_back(new ShapeLine(start, pos, &m_dialog_page));
			m_dialog_page.m_shape_list.back()->set_select(true);
		}
		else if constexpr (U == UNDO_T::FONT_COLOR) {
			const auto pangram = ResourceLoader::GetForCurrentView().GetString(L"str_pangram");
			if (pangram.empty()) {
				m_dialog_page.m_shape_list.push_back(new ShapeText(start, pos, wchar_cpy(L"The quick brown fox jumps over a lazy dog."), &m_dialog_page));
			}
			else {
				m_dialog_page.m_shape_list.push_back(new ShapeText(start, pos, wchar_cpy(pangram.data()), &m_dialog_page));
			}
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
		}

		D2D1_COLOR_F val;
		color_get<U>(m_dialog_page, val);
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
		const auto str_color_base{ mfsi_color_base().Text() };

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
		conv_col_to_str(m_color_base, val0, buf);
		dialog_slider_0().Header(box_value(str_color_r + buf));
		dialog_slider_1().Minimum(0.0);
		dialog_slider_1().Maximum(255.0);
		dialog_slider_1().TickFrequency(1.0);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(val1);
		conv_col_to_str(m_color_base, val1, buf);
		dialog_slider_1().Header(box_value(str_color_g + buf));
		dialog_slider_2().Minimum(0.0);
		dialog_slider_2().Maximum(255.0);
		dialog_slider_2().TickFrequency(1.0);
		dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_2().Value(val2);
		conv_col_to_str(m_color_base, val1, buf);
		dialog_slider_2().Header(box_value(str_color_b + buf));
		dialog_slider_3().Minimum(0.0);
		dialog_slider_3().Maximum(255.0);
		dialog_slider_3().TickFrequency(1.0);
		dialog_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_3().Value(val3);
		conv_col_to_str(m_color_base, val1, buf);
		dialog_slider_3().Header(box_value(str_opacity + buf));
		dialog_slider_0().Visibility(Visibility::Visible);
		dialog_slider_1().Visibility(Visibility::Visible);
		dialog_slider_2().Visibility(Visibility::Visible);
		dialog_slider_3().Visibility(Visibility::Visible);
		dialog_combo_box().Header(box_value(str_color_base));
		dialog_combo_box().Items().Append(box_value(rmfi_color_base_dec().Text()));
		dialog_combo_box().Items().Append(box_value(rmfi_color_base_hex().Text()));
		dialog_combo_box().Items().Append(box_value(rmfi_color_base_real().Text()));
		dialog_combo_box().Items().Append(box_value(rmfi_color_base_pct().Text()));
		if (m_color_base == COLOR_CODE::DEC) {
			dialog_combo_box().SelectedIndex(0);
		}
		else if (m_color_base == COLOR_CODE::HEX) {
			dialog_combo_box().SelectedIndex(1);
		}
		else if (m_color_base == COLOR_CODE::REAL) {
			dialog_combo_box().SelectedIndex(2);
		}
		else if (m_color_base == COLOR_CODE::PCT) {
			dialog_combo_box().SelectedIndex(3);
		}
		dialog_combo_box().Visibility(Visibility::Visible);

		cd_setting_dialog().Title(box_value(str_title));
		m_mutex_event.lock();
		{
			const auto revoker0{
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_color_r](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_col_to_str(m_color_base, val, buf);
					dialog_slider_0().Header(box_value(str_color_r + buf));
					D2D1_COLOR_F color;
					color_get<U>(m_dialog_page, color);
					color.r = static_cast<FLOAT>(val / COLOR_MAX);
					if (color_set<U>(m_dialog_page, color)) {
						dialog_draw();
					}
				})
			};
			const auto revoker1{
				dialog_slider_1().ValueChanged(winrt::auto_revoke, [this, str_color_g](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_col_to_str(m_color_base, val, buf);
					dialog_slider_1().Header(box_value(str_color_g + buf));
					D2D1_COLOR_F color;
					color_get<U>(m_dialog_page, color);
					color.g = static_cast<FLOAT>(val / COLOR_MAX);
					if (color_set<U>(m_dialog_page, color)) {
						dialog_draw();
					}
				})
			};
			const auto revoker2{
				dialog_slider_2().ValueChanged(winrt::auto_revoke, [this, str_color_b](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_col_to_str(m_color_base, val, buf);
					dialog_slider_2().Header(box_value(str_color_b + buf));
					D2D1_COLOR_F color;
					color_get<U>(m_dialog_page, color);
					color.b = static_cast<FLOAT>(val / COLOR_MAX);
					if (color_set<U>(m_dialog_page, color)) {
						dialog_draw();
					}
				})
			};
			const auto revoker3{
				dialog_slider_3().ValueChanged(winrt::auto_revoke, [this, str_opacity](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_col_to_str(m_color_base, val, buf);
					dialog_slider_3().Header(box_value(str_opacity + buf));
					D2D1_COLOR_F color;
					color_get<U>(m_dialog_page, color);
					color.a = static_cast<FLOAT>(val / COLOR_MAX);
					if (color_set<U>(m_dialog_page, color)) {
						dialog_draw();
					}
				})
			};
			const auto revoker4{
				dialog_combo_box().SelectionChanged(winrt::auto_revoke, [this, str_color_r, str_color_g, str_color_b, str_opacity](IInspectable const&, SelectionChangedEventArgs const&) {
					if (m_color_base != COLOR_CODE::DEC && dialog_combo_box().SelectedIndex() == 0) {
						m_color_base = COLOR_CODE::DEC;
					}
					else if (m_color_base != COLOR_CODE::HEX && dialog_combo_box().SelectedIndex() == 1) {
						m_color_base = COLOR_CODE::HEX;
					}
					else if (m_color_base != COLOR_CODE::REAL && dialog_combo_box().SelectedIndex() == 2) {
						m_color_base = COLOR_CODE::REAL;
					}
					else if (m_color_base != COLOR_CODE::PCT && dialog_combo_box().SelectedIndex() == 3) {
						m_color_base = COLOR_CODE::PCT;
					}
					else {
						return;
					}
					wchar_t buf[32];
					conv_col_to_str(m_color_base, dialog_slider_0().Value(), buf);
					dialog_slider_0().Header(box_value(str_color_r + buf));
					conv_col_to_str(m_color_base, dialog_slider_1().Value(), buf);
					dialog_slider_1().Header(box_value(str_color_g + buf));
					conv_col_to_str(m_color_base, dialog_slider_2().Value(), buf);
					dialog_slider_2().Header(box_value(str_color_b + buf));
					conv_col_to_str(m_color_base, dialog_slider_3().Value(), buf);
					dialog_slider_3().Header(box_value(str_opacity + buf));
				})
			};
			if (co_await cd_setting_dialog().ShowAsync() == ContentDialogResult::Primary) {
				D2D1_COLOR_F new_val;
				color_get<U>(m_dialog_page, new_val);
				if (color_ustack_set<U>(*this, new_val)) {
					ustack_push_null();
					xcvd_is_enabled();
					page_draw();
				}
			}
		}
		slist_clear(m_dialog_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		dialog_slider_2().Visibility(Visibility::Collapsed);
		dialog_slider_3().Visibility(Visibility::Collapsed);
		dialog_combo_box().Visibility(Visibility::Collapsed);
		dialog_combo_box().Items().Clear();
		page_draw();
		m_mutex_event.unlock();
	}

	// é¿ëÃâªÇ∑ÇÈ.
	template IAsyncAction MainPage::color_click_async<UNDO_T::FONT_COLOR>();
	template IAsyncAction MainPage::color_click_async<UNDO_T::FILL_COLOR>();
	template IAsyncAction MainPage::color_click_async<UNDO_T::GRID_COLOR>();
	template IAsyncAction MainPage::color_click_async<UNDO_T::PAGE_COLOR>();
	template IAsyncAction MainPage::color_click_async<UNDO_T::STROKE_COLOR>();
}