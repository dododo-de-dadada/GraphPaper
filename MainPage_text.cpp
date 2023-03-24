//-------------------------------
//　MainPage_text.cpp
//　文字列の配置
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

	constexpr float TEXT_LINE_SP_DELTA = 2.0f;	// 行の高さの変分 (DPIs)

	// 見本の図形を作成する.
	static void text_create_sample_shape(const float p_width, const float p_height, ShapePage& page);

	//------------------------------
	// 見本の図形を作成する.
	// p_width	見本を表示するパネルの幅
	// p_height	見本を表示するパネルの高さ
	// page	見本を表示するページ
	//------------------------------
	static void text_create_sample_shape(const float p_width, const float p_height, ShapePage& page)
	{
		const auto pad_w = p_width * 0.125;
		const auto pad_h = p_height * 0.125;
		const D2D1_POINT_2F start{
			static_cast<FLOAT>(pad_w), static_cast<FLOAT>(pad_h)
		};
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(p_width - 2.0 * pad_w), static_cast<FLOAT>(p_width - 2.0 * pad_h) 
		};
		const auto pang = ResourceLoader::GetForCurrentView().GetString(L"str_pangram");
		const wchar_t* text = nullptr;
		if (pang.empty()) {
			text = L"The quick brown fox jumps over a lazy dog.";
		}
		else {
			text = pang.c_str();
		}
		page.m_shape_list.push_back(new ShapeText(start, pos, wchar_cpy(text), &page));
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
	}

	// 書体メニューの「枠を文字列に合わせる」が選択された.
	void MainPage::text_fit_frame_to_text_click(IInspectable const&, RoutedEventArgs const&)
	{
		auto flag = false;
		const auto g_len = (m_main_page.m_grid_snap ? m_main_page.m_grid_base + 1.0f : 0.0f);
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
			auto u = new UndoDeform(s, ANC_TYPE::ANC_SE);
			if (static_cast<ShapeText*>(s)->fit_frame_to_text(g_len)) {
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
			ustack_push_null();
			main_panel_size();
			page_draw();
		}
		status_bar_set_pos();
	}

	// 書体メニューの「段落のそろえ」が選択された.
	void MainPage::text_align_vert_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_PARAGRAPH_ALIGNMENT val;
		if (sender == rmfi_text_align_top()) {
			val = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		}
		else if (sender == rmfi_text_align_bot()) {
			val = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR;
		}
		else if (sender == rmfi_text_align_mid()) {
			val = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		}
		else {
			winrt::hresult_not_implemented{};
			return;
		}
		text_align_vert_is_checked(val);
		if (ustack_push_set<UNDO_T::TEXT_PAR_ALIGN>(val)) {
			ustack_push_null();
			xcvd_is_enabled();
			page_draw();
		}
		status_bar_set_pos();
	}

	// 書体メニューの「段落のそろえ」に印をつける.
	// val	段落のそろえ
	void MainPage::text_align_vert_is_checked(const DWRITE_PARAGRAPH_ALIGNMENT val)
	{
		rmfi_text_align_top().IsChecked(
			val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		rmfi_text_align_bot().IsChecked(
			val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		rmfi_text_align_mid().IsChecked(
			val == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	// 書体メニューの「文字列のそろえ」が選択された.
	void MainPage::text_align_horz_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		DWRITE_TEXT_ALIGNMENT val;
		if (sender == rmfi_text_align_left()) {
			val = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
		}
		else if (sender == rmfi_text_align_right()) {
			val = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING;
		}
		else if (sender == rmfi_text_align_center()) {
			val = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER;
		}
		else if (sender == rmfi_text_align_just()) {
			val = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
		}
		else {
			winrt::hresult_not_implemented();
			return;
		}
		text_align_horz_is_checked(val);
		if (ustack_push_set<UNDO_T::TEXT_ALIGN_T>(val)) {
			ustack_push_null();
			xcvd_is_enabled();
			page_draw();
		}
		status_bar_set_pos();
	}

	// 書体メニューの「文字列のそろえ」に印をつける.
	// t_align	文字列のそろえ
	void MainPage::text_align_horz_is_checked(const DWRITE_TEXT_ALIGNMENT val)
	{
		rmfi_text_align_left().IsChecked(
			val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
		rmfi_text_align_right().IsChecked(
			val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING);
		rmfi_text_align_center().IsChecked(
			val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER);
		rmfi_text_align_just().IsChecked(
			val == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
	}

	// 書体メニューの「行間」>「行間...」が選択された.
	IAsyncAction MainPage::text_line_sp_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		const auto str_text_line_sp{ ResourceLoader::GetForCurrentView().GetString(L"str_text_line_sp") + L": "};
		const auto str_title{ ResourceLoader::GetForCurrentView().GetString(L"str_text_line_sp") };
		const auto str_def_val{ ResourceLoader::GetForCurrentView().GetString(L"str_def_val") };
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_dialog_page.set_attr_to(&m_main_page);
		float val;
		m_dialog_page.get_text_line_sp(val);

		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val);
		const auto unit = m_len_unit;
		const auto dpi = m_dialog_d2d.m_logical_dpi;
		const auto g_len = m_dialog_page.m_grid_base + 1.0f;
		wchar_t buf[32];
		if (val >= FLT_MIN) {
			conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
		}
		else {
			wcscpy_s(buf, str_def_val.data());
		}
		dialog_slider_0().Header(box_value(str_text_line_sp + buf));
		dialog_slider_0().Visibility(Visibility::Visible);

		text_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()), 
			static_cast<float>(scp_dialog_panel().Height()), m_dialog_page);

		cd_setting_dialog().Title(box_value(str_title));
		m_mutex_event.lock();
		{
			const auto revoker0{
				dialog_slider_0().ValueChanged(
					winrt::auto_revoke,
					[this, str_text_line_sp, str_def_val](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
						const auto unit = m_len_unit;
						const auto dpi = m_dialog_d2d.m_logical_dpi;
						const auto g_len = m_dialog_page.m_grid_base + 1.0f;
						const float val = static_cast<float>(args.NewValue());
						wchar_t buf[32];
						if (val >= FLT_MIN) {
							conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
						}
						else {
							wcscpy_s(buf, str_def_val.data());
						}
						dialog_slider_0().Header(box_value(str_text_line_sp + buf));
						if (m_dialog_page.m_shape_list.back()->set_text_line_sp(val)) {
							dialog_draw();
						}
					}
				)
			};
			if (co_await cd_setting_dialog().ShowAsync() == ContentDialogResult::Primary) {
				float samp_val;
				m_dialog_page.m_shape_list.back()->get_text_line_sp(samp_val);
				if (ustack_push_set<UNDO_T::TEXT_LINE_SP>(samp_val)) {
					ustack_push_null();
					xcvd_is_enabled();
					page_draw();
				}
			}
		}
		slist_clear(m_dialog_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		m_mutex_event.unlock();
	}

	// 書体メニューの「余白」が選択された.
	IAsyncAction MainPage::text_pad_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		const auto str_text_pad_horz{ ResourceLoader::GetForCurrentView().GetString(L"str_text_pad_horz") + L": " };
		const auto str_text_pad_vert{ ResourceLoader::GetForCurrentView().GetString(L"str_text_pad_vert") + L": " };
		const auto str_title{ ResourceLoader::GetForCurrentView().GetString(L"str_text_padding") };
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_dialog_page.set_attr_to(&m_main_page);
		D2D1_SIZE_F pad;
		m_dialog_page.get_text_pad(pad);

		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(pad.width);
		constexpr wchar_t* HEADER[] = { L"str_text_pad_horz", L"str_text_pad_vert" };
		const auto unit = m_len_unit;
		const auto dpi = m_main_d2d.m_logical_dpi;
		const auto g_len = m_dialog_page.m_grid_base + 1.0f;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, pad.width, dpi, g_len, buf);
		dialog_slider_0().Header(box_value(str_text_pad_horz + buf));

		dialog_slider_1().Maximum(MAX_VALUE);
		dialog_slider_1().TickFrequency(TICK_FREQ);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(pad.height);
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, pad.height, dpi, g_len, buf);
		dialog_slider_1().Header(box_value(str_text_pad_vert + buf));

		dialog_slider_0().Visibility(Visibility::Visible);
		dialog_slider_1().Visibility(Visibility::Visible);
		text_create_sample_shape(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()), m_dialog_page);

		cd_setting_dialog().Title(box_value(str_title));
		m_mutex_event.lock();
		{
			const auto revoker0{
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_text_pad_horz](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_dialog_d2d.m_logical_dpi;
					const auto g_len = m_dialog_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_0().Header(box_value(str_text_pad_horz + buf));
					D2D1_SIZE_F pad;
					m_dialog_page.m_shape_list.back()->get_text_pad(pad);
					pad.width = static_cast<FLOAT>(val);
					if (m_dialog_page.m_shape_list.back()->set_text_pad(pad)) {
						dialog_draw();
					}
				})
			};
			const auto revoker1{
				dialog_slider_1().ValueChanged(winrt::auto_revoke, [this, str_text_pad_vert](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_dialog_d2d.m_logical_dpi;
					const auto g_len = m_dialog_page.m_grid_base + 1.0f;
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
					dialog_slider_0().Header(box_value(str_text_pad_vert + buf));
					D2D1_SIZE_F pad;
					m_dialog_page.m_shape_list.back()->get_text_pad(pad);
					pad.height = static_cast<FLOAT>(val);
					if (m_dialog_page.m_shape_list.back()->set_text_pad(pad)) {
						dialog_draw();
					}
				})
			};
			if (co_await cd_setting_dialog().ShowAsync() == ContentDialogResult::Primary) {
				D2D1_SIZE_F samp_val;
				m_dialog_page.m_shape_list.back()->get_text_pad(samp_val);
				if (ustack_push_set<UNDO_T::TEXT_PAD>(samp_val)) {
					ustack_push_null();
					xcvd_is_enabled();
					page_draw();
				}
			}
		}
		slist_clear(m_dialog_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		m_mutex_event.unlock();
	}

	// スライダーの値が変更された.
	// U	操作の識別子
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	/*
	template <UNDO_T U, int S>
	void MainPage::text_slider_val_changed(
		IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_T::TEXT_LINE_SP && S == 0) {
			const float val = static_cast<float>(args.NewValue());
			text_slider_set_header<UNDO_T::TEXT_LINE_SP, 0>(val);
			if (m_dialog_page.m_shape_list.back()->set_text_line_sp(val)) {
				dialog_draw();
			}
		}
		else if constexpr (U == UNDO_T::TEXT_PAD && S == 0) {
			const float val = static_cast<float>(args.NewValue());
			text_slider_set_header<UNDO_T::TEXT_PAD, 0>(val);
			D2D1_SIZE_F pad;
			m_dialog_page.m_shape_list.back()->get_text_pad(pad);
			pad.width = static_cast<FLOAT>(val);
			if (m_dialog_page.m_shape_list.back()->set_text_pad(pad)) {
				dialog_draw();
			}
		}
		else if constexpr (U == UNDO_T::TEXT_PAD && S == 1) {
			const float val = static_cast<float>(args.NewValue());
			text_slider_set_header<UNDO_T::TEXT_PAD, 1>(val);
			D2D1_SIZE_F pad;
			m_dialog_page.m_shape_list.back()->get_text_pad(pad);
			pad.height = static_cast<FLOAT>(val);
			if (m_dialog_page.m_shape_list.back()->set_text_pad(pad)) {
				dialog_draw();
			}
		}
	}
	*/

}