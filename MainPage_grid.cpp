//-------------------------------
// MainPage_grid.cpp
// ����
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	//using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	//using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::UI::Xaml::Controls::ToggleMenuFlyoutItem;
	//using winrt::Windows::UI::Xaml::RoutedEventArgs;

	//constexpr float SLIDER_STEP = 0.5f;
	constexpr wchar_t TITLE_GRID[] = L"str_grid";

	// ���Ⴡ�j���[�́u����̋����v���I�����ꂽ.
	void MainPage::grid_emph_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		GRID_EMPH val;
		if (sender == rmfi_grid_emph_1()) {
			val = GRID_EMPH_0;
		}
		else if (sender == rmfi_grid_emph_2()) {
			val = GRID_EMPH_2;
		}
		else if (sender == rmfi_grid_emph_3()) {
			val = GRID_EMPH_3;
		}
		else {
			return;
		}
		grid_emph_is_checked(val);
		GRID_EMPH g_emph;
		m_main_page.get_grid_emph(g_emph);
		if (!equal(g_emph, val)) {
			ustack_push_set<UNDO_ID::GRID_EMPH>(&m_main_page, val);
			ustack_is_enable();
			page_draw();
		}
	}

	// ���Ⴡ�j���[�́u����̋����v�Ɉ������.
	// g_emph	����̋���
	void MainPage::grid_emph_is_checked(const GRID_EMPH& g_emph)
	{
		rmfi_grid_emph_1().IsChecked(g_emph.m_gauge_1 == 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_2().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_3().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 != 0);
	}

	// ���Ⴡ�j���[�́u����̐F�v���I�����ꂽ.
	IAsyncAction MainPage::grid_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_dialog_page.set_attr_to(&m_main_page);
		const auto val0 = m_dialog_page.m_grid_color.r * COLOR_MAX;
		const auto val1 = m_dialog_page.m_grid_color.g * COLOR_MAX;
		const auto val2 = m_dialog_page.m_grid_color.b * COLOR_MAX;
		const auto val3 = m_dialog_page.m_grid_color.a * COLOR_MAX;

		dialog_slider_0().Maximum(255.0);
		dialog_slider_0().TickFrequency(1.0);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val0);
		grid_slider_set_header<UNDO_ID::GRID_COLOR, 0>(val0);
		dialog_slider_1().Maximum(255.0);
		dialog_slider_1().TickFrequency(1.0);
		dialog_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_1().Value(val1);
		grid_slider_set_header<UNDO_ID::GRID_COLOR, 1>(val1);
		dialog_slider_2().Maximum(255.0);
		dialog_slider_2().TickFrequency(1.0);
		dialog_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_2().Value(val2);
		grid_slider_set_header<UNDO_ID::GRID_COLOR, 2>(val2);
		dialog_slider_3().Maximum(255.0);
		dialog_slider_3().TickFrequency(1.0);
		dialog_slider_3().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_3().Value(val3);
		grid_slider_set_header<UNDO_ID::GRID_COLOR, 3>(val3);

		dialog_slider_0().Visibility(Visibility::Visible);
		dialog_slider_1().Visibility(Visibility::Visible);
		dialog_slider_2().Visibility(Visibility::Visible);
		dialog_slider_3().Visibility(Visibility::Visible);
		const auto slider_0_token = dialog_slider_0().ValueChanged({ this, &MainPage::grid_slider_val_changed< UNDO_ID::GRID_COLOR, 0> });
		const auto slider_1_token = dialog_slider_1().ValueChanged({ this, &MainPage::grid_slider_val_changed< UNDO_ID::GRID_COLOR, 1> });
		const auto slider_2_token = dialog_slider_2().ValueChanged({ this, &MainPage::grid_slider_val_changed< UNDO_ID::GRID_COLOR, 2> });
		const auto slider_3_token = dialog_slider_3().ValueChanged({ this, &MainPage::grid_slider_val_changed< UNDO_ID::GRID_COLOR, 3> });

		cd_setting_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_GRID)));
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			if (!equal(m_main_page.m_grid_color, m_dialog_page.m_grid_color)) {
				ustack_push_set<UNDO_ID::GRID_COLOR>(&m_main_page, m_dialog_page.m_grid_color);
				ustack_is_enable();
				page_draw();
			}
		}

		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_1().Visibility(Visibility::Collapsed);
		dialog_slider_2().Visibility(Visibility::Collapsed);
		dialog_slider_3().Visibility(Visibility::Collapsed);
		dialog_slider_0().ValueChanged(slider_0_token);
		dialog_slider_1().ValueChanged(slider_1_token);
		dialog_slider_2().ValueChanged(slider_2_token);
		dialog_slider_3().ValueChanged(slider_3_token);
	}

	// ���Ⴡ�j���[�́u����̑傫���v>�u�傫���v���I�����ꂽ.
	IAsyncAction MainPage::grid_len_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		m_dialog_page.set_attr_to(&m_main_page);
		float g_base;
		m_dialog_page.get_grid_base(g_base);

		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(g_base);
		grid_slider_set_header<UNDO_ID::GRID_BASE, 0>(g_base);
		dialog_slider_0().Visibility(Visibility::Visible);
		const auto slider_0_token = dialog_slider_0().ValueChanged({ this, &MainPage::grid_slider_val_changed<UNDO_ID::GRID_BASE, 0> });
		//const auto samp_w = scp_dialog_panel().Width();
		//const auto samp_h = scp_dialog_panel().Height();

		cd_setting_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_GRID)));
		const auto d_result = co_await cd_setting_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float setting_val;
			float page_val;

			m_main_page.get_grid_base(page_val);
			m_dialog_page.get_grid_base(setting_val);
			if (!equal(page_val, setting_val)) {
				ustack_push_set<UNDO_ID::GRID_BASE>(&m_main_page, setting_val);
				ustack_is_enable();
				xcvd_is_enabled();
				page_draw();
			}

		}
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_slider_0().ValueChanged(slider_0_token);
	}

	// ���Ⴡ�j���[�́u����̑傫���v>�u���߂�v���I�����ꂽ.
	void MainPage::grid_len_con_click(IInspectable const&, RoutedEventArgs const&)
	{
		float g_base;
		m_main_page.get_grid_base(g_base);
		const float val = (g_base + 1.0f) * 0.5f - 1.0f;
		if (val >= 1.0f) {
			ustack_push_set<UNDO_ID::GRID_BASE>(&m_main_page, val);
			ustack_is_enable();
			page_draw();
		}
	}

	// ���Ⴡ�j���[�́u����̑傫���v>�u�L����v���I�����ꂽ.
	void MainPage::grid_len_exp_click(IInspectable const&, RoutedEventArgs const&)
	{
		float g_base;
		m_main_page.get_grid_base(g_base);
		const float val = (g_base + 1.0f) * 2.0f - 1.0f;
		if (val <= max(m_main_page.m_page_size.width, m_main_page.m_page_size.height)) {
			ustack_push_set<UNDO_ID::GRID_BASE>(&m_main_page, val);
			ustack_is_enable();
			page_draw();
		}
	}

	// �l���X���C�_�[�̃w�b�_�[�Ɋi�[����.
	// U	����̎��
	// S	�X���C�_�[�̔ԍ�
	// val	�i�[����l
	// �߂�l	�Ȃ�.
	template <UNDO_ID U, int S>
	void MainPage::grid_slider_set_header(const float val)
	{
		winrt::hstring text;

		if constexpr (U == UNDO_ID::GRID_BASE) {
			float g_base;
			m_main_page.get_grid_base(g_base);
			const float g_len = g_base + 1.0f;
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, val + 1.0f, m_main_d2d.m_logical_dpi, g_len, buf);
			text = ResourceLoader::GetForCurrentView().GetString(L"str_grid_length") + L": " + buf;
		}
		if constexpr (U == UNDO_ID::GRID_COLOR) {
			constexpr wchar_t* HEADER[]{ L"str_color_r", L"str_color_g",L"str_color_b", L"str_opacity" };
			wchar_t buf[32];
			conv_col_to_str(m_color_code, val, buf);
			text = ResourceLoader::GetForCurrentView().GetString(HEADER[S]) + L": " + buf;
		}
		dialog_set_slider_header<S>(text);
	}

	// �X���C�_�[�̒l���ύX���ꂽ.
	// U	����̎��
	// S	�X���C�_�[�̔ԍ�
	// args	ValueChanged �œn���ꂽ����
	// �߂�l	�Ȃ�
	template <UNDO_ID U, int S>
	void MainPage::grid_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_ID::GRID_BASE) {
			const float val = static_cast<float>(args.NewValue());
			grid_slider_set_header<U, S>(val);
			m_dialog_page.set_grid_base(val);
		}
		else if constexpr (U == UNDO_ID::GRID_COLOR) {
			const float val = static_cast<float>(args.NewValue());
			grid_slider_set_header<U, S>(val);
			D2D1_COLOR_F g_color;
			m_dialog_page.get_grid_color(g_color);
			if constexpr (S == 0) {
				g_color.r = val / COLOR_MAX;
			}
			else if constexpr (S == 1) {
				g_color.g = val / COLOR_MAX;
			}
			else if constexpr (S == 2) {
				g_color.b = val / COLOR_MAX;
			}
			else if constexpr (S == 3) {
				g_color.a = val / COLOR_MAX;
			}
			m_dialog_page.set_grid_color(g_color);
		}
		if (scp_dialog_panel().IsLoaded()) {
			dialog_draw();
		}
	}

	// ���Ⴡ�j���[�́u����̕\���v>�u�Ŕw�ʁv���I�����ꂽ.
	void MainPage::grid_show_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		GRID_SHOW val;
		if (sender == rmfi_grid_show_back()) {
			val = GRID_SHOW::BACK;
		}
		else if (sender == rmfi_grid_show_front()) {
			val = GRID_SHOW::FRONT;
		}
		else if (sender == rmfi_grid_show_hide()) {
			val = GRID_SHOW::HIDE;
		}
		else {
			return;
		}
		grid_show_is_checked(val);
		if (m_main_page.m_grid_show != val) {
			ustack_push_set<UNDO_ID::GRID_SHOW>(&m_main_page, val);
			ustack_is_enable();
			page_draw();
		}
	}

	// ���Ⴡ�j���[�́u����̕\���v�Ɉ������.
	// g_show	����̕\��
	void MainPage::grid_show_is_checked(const GRID_SHOW g_show)
	{
		rmfi_grid_show_back().IsChecked(g_show == GRID_SHOW::BACK);
		rmfi_grid_show_front().IsChecked(g_show == GRID_SHOW::FRONT);
		rmfi_grid_show_hide().IsChecked(g_show == GRID_SHOW::HIDE);
		//rmfi_grid_show_back_2().IsChecked(g_show == GRID_SHOW::BACK);
		//rmfi_grid_show_front_2().IsChecked(g_show == GRID_SHOW::FRONT);
		//rmfi_grid_show_hide_2().IsChecked(g_show == GRID_SHOW::HIDE);
	}

	// ���Ⴡ�j���[�́u����ɍ��킹��v���I�����ꂽ.
	void MainPage::grid_snap_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		m_main_page.m_grid_snap = unbox_value<ToggleMenuFlyoutItem>(sender).IsChecked();
	}

}