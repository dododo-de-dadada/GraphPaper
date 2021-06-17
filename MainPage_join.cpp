#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t DLG_TITLE[] = L"str_line_join";
	constexpr float SLIDER_STEP = 0.5f;

	// ���g���j���[�́u�[�_�̎�ށv���I�����ꂽ.
	void MainPage::cap_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		CAP_STYLE new_value;
		if (sender == rmfi_cap_flat() || sender == rmfi_cap_flat_2()) {
			new_value = CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };
		}
		else if (sender == rmfi_cap_square() || sender == rmfi_cap_square_2()) {
			new_value = CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE };
		}
		else if (sender == rmfi_cap_round() || sender == rmfi_cap_round_2()) {
			new_value = CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND };
		}
		else if (sender == rmfi_cap_triangle() || sender == rmfi_cap_triangle_2()) {
			new_value = CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE };
		}
		else {
			return;
		}
		CAP_STYLE old_value;
		m_sheet_main.get_stroke_cap_style(old_value);
		if (undo_push_set<UNDO_OP::STROKE_CAP_STYLE>(new_value)) {
			undo_push_null();
			undo_menu_enable();
			sheet_draw();
		}
	}

	// ���g���j���[�́u�[�_�̎�ށv�Ɉ������.
	// s_cap	���̒P�_
	void MainPage::cap_style_is_checked(const CAP_STYLE& value)
	{
		rmfi_cap_flat().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT }));
		rmfi_cap_flat_2().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT }));
		rmfi_cap_square().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE }));
		rmfi_cap_square_2().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE }));
		rmfi_cap_round().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND }));
		rmfi_cap_round_2().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND }));
		rmfi_cap_triangle().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE }));
		rmfi_cap_triangle_2().IsChecked(equal(value, { D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE }));
	}

	// ���g���j���[�́u�Ȃ��̎�ށv>�u�z�Ԃ��̐����v���I�����ꂽ.
	IAsyncAction MainPage::join_limit_click_async(IInspectable const& sender, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_attr_to(&m_sheet_main);
		float value;
		m_sample_sheet.get_stroke_join_limit(value);
		const float val0 = (value - 1.0F) / SLIDER_STEP;
		sample_slider_0().Value(val0);
		sample_slider_0().Visibility(UI_VISIBLE);
		float s_width;
		m_sample_sheet.get_stroke_width(s_width);
		const float val1 = s_width / SLIDER_STEP;
		sample_slider_1().Value(val1);
		sample_slider_1().Visibility(UI_VISIBLE);
		join_slider_set_header<UNDO_OP::STROKE_JOIN_LIMIT, 0>(val0);
		join_slider_set_header<UNDO_OP::STROKE_WIDTH, 1>(val1);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::join_slider_value_changed<UNDO_OP::STROKE_JOIN_LIMIT, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::join_slider_value_changed<UNDO_OP::STROKE_WIDTH, 1> });
		m_sample_type = SAMPLE_TYPE::JOIN;
		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_line_join")));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float sample_limit;
			float sample_width;
			m_sample_shape->get_stroke_join_limit(sample_limit);
			m_sample_shape->get_stroke_width(sample_width);
			if (undo_push_set<UNDO_OP::STROKE_JOIN_LIMIT>(sample_limit) ||
				undo_push_set<UNDO_OP::STROKE_WIDTH>(sample_width)) {
				undo_push_null();
				undo_menu_enable();
				sheet_draw();
			}
		}
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().Visibility(UI_COLLAPSED);
		sample_slider_1().ValueChanged(slider_1_token);
		co_return;
	}

	// �l���X���C�_�[�̃w�b�_�[�Ɋi�[����.
	template <UNDO_OP U, int S> void MainPage::join_slider_set_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		winrt::hstring text;

		if constexpr (U == UNDO_OP::STROKE_JOIN_LIMIT) {
			constexpr size_t LEN = 32;
			wchar_t buf[LEN + 1];
			const float limit = value * SLIDER_STEP + 1.0f;
			swprintf_s(buf, LEN, L"%.1f", limit);
			text = ResourceLoader::GetForCurrentView().GetString(L"str_stroke_join_limit") + L": " + buf;
		}
		if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			const float g_len = g_base + 1.0f;
			constexpr size_t LEN = 32;
			wchar_t buf[LEN + 1];
			conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, value * SLIDER_STEP, m_sheet_dx.m_logical_dpi, g_len, buf);
			text = ResourceLoader::GetForCurrentView().GetString(L"str_stroke_width") + L": " + buf;
		}
		if constexpr (S == 0) {
			sample_slider_0().Header(box_value(text));
		}
		if constexpr (S == 1) {
			sample_slider_1().Header(box_value(text));
		}
	}

	// �X���C�_�[�̒l���ύX���ꂽ.
	// U	����̎��
	// S	�X���C�_�[�̔ԍ�
	// args	ValueChanged �œn���ꂽ����
	// �߂�l	�Ȃ�
	template <UNDO_OP U, int S> void MainPage::join_slider_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* sample = m_sample_shape;
		const float value = static_cast<float>(args.NewValue());
		join_slider_set_header<U, S>(value);
		if constexpr (U == UNDO_OP::STROKE_JOIN_LIMIT) {
			sample->set_stroke_join_limit(value * SLIDER_STEP + 1.0f);
		}
		else if constexpr (U == UNDO_OP::STROKE_WIDTH) {
			sample->set_stroke_width(value * SLIDER_STEP);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// ���g���j���[�́u�Ȃ��̎�ށv���I�����ꂽ.
	void MainPage::join_style_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		D2D1_LINE_JOIN new_value;
		if (sender == rmfi_join_bevel() || sender == rmfi_join_bevel_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;
			mfi_join_limit().IsEnabled(false);
			mfi_join_limit_2().IsEnabled(false);
		}
		else if (sender == rmfi_join_miter() || sender == rmfi_join_miter_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER;
			mfi_join_limit().IsEnabled(true);
			mfi_join_limit_2().IsEnabled(true);
		}
		else if (sender == rmfi_join_m_or_b() || sender == rmfi_join_m_or_b_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL;
			mfi_join_limit().IsEnabled(true);
			mfi_join_limit_2().IsEnabled(true);
		}
		else if (sender == rmfi_join_round() || sender == rmfi_join_round_2()) {
			new_value = D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND;
			mfi_join_limit().IsEnabled(false);
			mfi_join_limit_2().IsEnabled(false);
		}
		else {
			return;
		}
		D2D1_LINE_JOIN old_value;
		m_sheet_main.get_stroke_join_style(old_value);
		if (undo_push_set<UNDO_OP::STROKE_JOIN_STYLE>(new_value)) {
			undo_push_null();
			undo_menu_enable();
			sheet_draw();
		}
	}

	// ���g���j���[�́u�Ȃ��v�Ɉ������.
	// s_join	���̂Ȃ�
	void MainPage::join_style_is_checked(const D2D1_LINE_JOIN value)
	{
		rmfi_join_bevel().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL);
		rmfi_join_bevel_2().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL);
		rmfi_join_miter().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
		rmfi_join_miter_2().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
		rmfi_join_m_or_b().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL);
		rmfi_join_m_or_b_2().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL);
		rmfi_join_round().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND);
		rmfi_join_round_2().IsChecked(value == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND);
		mfi_join_limit().IsEnabled(rmfi_join_miter().IsChecked() || rmfi_join_m_or_b().IsChecked());
		mfi_join_limit_2().IsEnabled(rmfi_join_miter_2().IsChecked() || rmfi_join_m_or_b_2().IsChecked());
	}

}