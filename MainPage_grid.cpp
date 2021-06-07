//-------------------------------
// MainPage_grid.cpp
// ����
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::ToggleMenuFlyoutItem;

	constexpr wchar_t TITLE_GRID[] = L"str_grid";
	constexpr float SLIDER_STEP = 0.5f;

	// �p�����j���[�́u����̔Z���v���I�����ꂽ.
	IAsyncAction MainPage::grid_gray_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		float value;
		m_sample_sheet.get_grid_gray(value);
		const float val3 = value * COLOR_MAX;
		sample_slider_3().Value(val3);
		grid_set_slider_header<UNDO_OP::GRID_GRAY, 3>(val3);
		sample_slider_3().Visibility(UI_VISIBLE);
		const auto slider_3_token = sample_slider_3().ValueChanged({ this, &MainPage::grid_set_slider< UNDO_OP::GRID_GRAY, 3> });
		m_sample_type = SAMP_TYPE::NONE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_GRID)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_value;
			m_sample_sheet.get_grid_gray(sample_value);
			float sheet_value;
			m_sheet_main.get_grid_gray(sheet_value);
			if (!equal(sheet_value, sample_value)) {
				undo_push_set<UNDO_OP::GRID_GRAY>(&m_sheet_main, sample_value);
				undo_menu_enable();
				sheet_draw();
			}
		}
		sample_slider_3().Visibility(UI_COLLAPSED);
		sample_slider_3().ValueChanged(slider_3_token);
	}

	// �p�����j���[�́u����̑傫���v>�u�傫���v���I�����ꂽ.
	IAsyncAction MainPage::grid_len_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		float value;
		m_sample_sheet.get_grid_base(value);
		const float val0 = value / SLIDER_STEP;
		sample_slider_0().Value(val0);
		grid_set_slider_header<UNDO_OP::GRID_BASE, 0>(val0);
		sample_slider_0().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::grid_set_slider<UNDO_OP::GRID_BASE, 0> });
		m_sample_type = SAMP_TYPE::NONE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_GRID)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_value;
			float sheet_value;

			m_sheet_main.get_grid_base(sheet_value);
			m_sample_sheet.get_grid_base(sample_value);
			if (!equal(sheet_value, sample_value)) {
				undo_push_set<UNDO_OP::GRID_BASE>(&m_sheet_main, sample_value);
				undo_menu_enable();
				edit_menu_is_enabled();
				sheet_draw();
			}

		}
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
	}

	// �p�����j���[�́u����̑傫���v>�u���߂�v���I�����ꂽ.
	void MainPage::grid_len_con_click(IInspectable const&, RoutedEventArgs const&)
	{
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		const float value = (g_base + 1.0f) * 0.5f - 1.0f;
		if (value >= 1.0f) {
			undo_push_set<UNDO_OP::GRID_BASE>(&m_sheet_main, value);
			undo_menu_enable();
			sheet_draw();
		}
	}

	// �p�����j���[�́u����̑傫���v>�u�L����v���I�����ꂽ.
	void MainPage::grid_len_exp_click(IInspectable const&, RoutedEventArgs const&)
	{
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		const float value = (g_base + 1.0f) * 2.0f - 1.0f;
		if (value <= max(m_sheet_main.m_sheet_size.width, m_sheet_main.m_sheet_size.height)) {
			undo_push_set<UNDO_OP::GRID_BASE>(&m_sheet_main, value);
			undo_menu_enable();
			sheet_draw();
		}
	}

	// �p�����j���[�́u����̋����v���I�����ꂽ.
	void MainPage::grid_emph_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		GRID_EMPH value;
		if (sender == rmfi_grid_emph_1() || sender == rmfi_grid_emph_1_2()) {
			value = GRID_EMPH_0;
		}
		else if (sender == rmfi_grid_emph_2() || sender == rmfi_grid_emph_2_2()) {
			value = GRID_EMPH_2;
		}
		else if (sender == rmfi_grid_emph_3() || sender == rmfi_grid_emph_3_2()) {
			value = GRID_EMPH_3;
		}
		else {
			return;
		}
		GRID_EMPH g_emph;
		m_sheet_main.get_grid_emph(g_emph);
		if (!equal(g_emph, value)) {
			undo_push_set<UNDO_OP::GRID_EMPH>(&m_sheet_main, value);
			undo_menu_enable();
			sheet_draw();
		}
	}

	// �p�����j���[�́u����̋����v�Ɉ������.
	// g_emph	����̋���
	void MainPage::grid_emph_is_checked(const GRID_EMPH& g_emph)
	{
		rmfi_grid_emph_1().IsChecked(g_emph.m_gauge_1 == 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_2().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_3().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 != 0);

		rmfi_grid_emph_1_2().IsChecked(g_emph.m_gauge_1 == 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_2_2().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 == 0);
		rmfi_grid_emph_3_2().IsChecked(g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 != 0);
	}

	// �l���X���C�_�[�̃w�b�_�[�Ɋi�[����.
	// U	����
	// S	�X���C�_�[
	// value	�l
	template <UNDO_OP U, int S> void MainPage::grid_set_slider_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring text;

		if constexpr (U == UNDO_OP::GRID_BASE) {
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			const float g_len = g_base + 1.0f;
			wchar_t buf[32];
			conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), value * SLIDER_STEP + 1.0f, m_sheet_dx.m_logical_dpi, g_len, buf);
			text = ResourceLoader::GetForCurrentView().GetString(L"str_grid_length") + L": " + buf;
		}
		if constexpr (U == UNDO_OP::GRID_GRAY) {
			if constexpr (S == 3) {
				wchar_t buf[32];
				// �F�����̒l�𕶎���ɕϊ�����.
				conv_col_to_str(color_code(), value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_gray_scale") + L": " + buf;
			}
		}
		if constexpr (S == 0) {
			sample_slider_0().Header(box_value(text));
		}
		if constexpr (S == 1) {
			sample_slider_1().Header(box_value(text));
		}
		if constexpr (S == 2) {
			sample_slider_2().Header(box_value(text));
		}
		if constexpr (S == 3) {
			sample_slider_3().Header(box_value(text));
		}
	}

	// �l���X���C�_�[�̃w�b�_�[�ƁA���{�̐}�`�Ɋi�[����.
	// U	����̎��
	// S	�X���C�_�[�̔ԍ�
	// args	ValueChanged �œn���ꂽ����
	// �߂�l	�Ȃ�
	template <UNDO_OP U, int S> void MainPage::grid_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* const s = &m_sample_sheet;
		const float value = static_cast<float>(args.NewValue());

		grid_set_slider_header<U, S>(value);
		if constexpr (U == UNDO_OP::GRID_BASE) {
			s->set_grid_base(value * SLIDER_STEP);
		}
		if constexpr (U == UNDO_OP::GRID_GRAY) {
			s->set_grid_gray(value / COLOR_MAX);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// �p�����j���[�́u����̕\���v>�u�Ŕw�ʁv���I�����ꂽ.
	void MainPage::grid_show_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		GRID_SHOW value;
		if (sender == rmfi_grid_show_back() || sender == rmfi_grid_show_back_2()) {
			value = GRID_SHOW::BACK;
		}
		else if (sender == rmfi_grid_show_front() || sender == rmfi_grid_show_front_2()) {
			value = GRID_SHOW::FRONT;
		}
		else if (sender == rmfi_grid_show_hide() || sender == rmfi_grid_show_hide_2()) {
			value = GRID_SHOW::HIDE;
		}
		else {
			return;
		}
		GRID_SHOW g_show;
		m_sheet_main.get_grid_show(g_show);
		if (g_show != value) {
			undo_push_set<UNDO_OP::GRID_SHOW>(&m_sheet_main, value);
			undo_menu_enable();
			sheet_draw();
		}
	}

	// �p�����j���[�́u����̕\���v�Ɉ������.
	// g_show	����̕\��
	void MainPage::grid_show_is_checked(const GRID_SHOW g_show)
	{
		rmfi_grid_show_back().IsChecked(g_show == GRID_SHOW::BACK);
		rmfi_grid_show_front().IsChecked(g_show == GRID_SHOW::FRONT);
		rmfi_grid_show_hide().IsChecked(g_show == GRID_SHOW::HIDE);

		rmfi_grid_show_back_2().IsChecked(g_show == GRID_SHOW::BACK);
		rmfi_grid_show_front_2().IsChecked(g_show == GRID_SHOW::FRONT);
		rmfi_grid_show_hide_2().IsChecked(g_show == GRID_SHOW::HIDE);
	}

	// �p�����j���[�́u����ɂ��낦��v���I�����ꂽ.
	void MainPage::grid_snap_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		auto value = unbox_value<ToggleMenuFlyoutItem>(sender).IsChecked();
		bool g_snap;
		m_sheet_main.get_grid_snap(g_snap);
		if (g_snap != value) {
			m_sheet_main.set_grid_snap(value);
		}
		if (!g_snap) {
			return;
		}

		// �}�`���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		const double g_len = g_base + 1.0;
		auto flag = false;
		D2D1_POINT_2F p_min = sheet_min();
		D2D1_POINT_2F p_max = sheet_max();
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				// �����t���O�������Ă���ꍇ,
				// �ȉ��𖳎�����.
				continue;
			}
			if (s->is_selected() != true) {
				// �I���t���O���Ȃ��ꍇ,
				continue;
			}
			{
				D2D1_POINT_2F b_nw;
				D2D1_POINT_2F b_se;
				s->get_bound({ FLT_MAX, FLT_MAX }, { -FLT_MAX, -FLT_MAX }, b_nw, b_se);
				D2D1_POINT_2F b_ne{ b_se.x, b_nw.y };
				D2D1_POINT_2F b_sw{ b_nw.x, b_se.y };

				D2D1_POINT_2F g_nw;
				D2D1_POINT_2F g_se;
				D2D1_POINT_2F g_ne;
				D2D1_POINT_2F g_sw;
				pt_round(b_nw, g_len, g_nw);
				pt_round(b_se, g_len, g_se);
				pt_round(b_ne, g_len, g_ne);
				pt_round(b_sw, g_len, g_sw);

				D2D1_POINT_2F d_nw;
				D2D1_POINT_2F d_se;
				D2D1_POINT_2F d_ne;
				D2D1_POINT_2F d_sw;
				pt_sub(g_nw, b_nw, d_nw);
				pt_sub(g_se, b_se, d_se);
				pt_sub(g_ne, b_ne, d_ne);
				pt_sub(g_sw, b_sw, d_sw);

				double a_nw = pt_abs2(d_nw);
				double a_se = pt_abs2(d_se);
				double a_ne = pt_abs2(d_ne);
				double a_sw = pt_abs2(d_sw);
				D2D1_POINT_2F diff;
				if (a_se <= a_nw && a_se <= a_ne && a_nw <= a_sw) {
					diff = d_se;
				}
				else if (a_ne <= a_nw && a_ne <= a_se && a_nw <= a_sw) {
					diff = d_ne;
				}
				else if (a_sw <= a_nw && a_sw <= a_se && a_sw <= a_ne) {
					diff = d_sw;
				}
				else {
					diff = d_nw;
				}
				if (flag != true) {
					flag = true;
				}
				undo_push_set<UNDO_OP::START_POS>(s);
				s->move(diff);
			}
			/*
			D2D1_POINT_2F s_pos;
			s->get_min_pos(s_pos);
			D2D1_POINT_2F g_pos;
			pt_round(s_pos, g_len, g_pos);
			if (equal(g_pos, s_pos)) {
				// �J�n�ʒu�Ɗۂ߂��ʒu�������ꍇ,
				continue;
			}
			if (flag != true) {
				flag = true;
			}
			undo_push_set<UNDO_OP::START_POS>(s);
			D2D1_POINT_2F diff;
			pt_sub(g_pos, s_pos, diff);
			s->move(diff);
			*/
		}
		if (flag) {
			undo_push_null();
			undo_menu_enable();
			sheet_update_bbox();
			sheet_panle_size();
			sheet_draw();
		}
	}

}