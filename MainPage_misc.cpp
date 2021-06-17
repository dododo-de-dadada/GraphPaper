//-----------------------------
// MainPage_misc.cpp
// �����̒P��, �F�̕\�L, �X�e�[�^�X�o�[, �o�[�W�������
//-----------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ���̑����j���[�́u�o�[�W�������v���I�����ꂽ.
	IAsyncAction MainPage::about_graph_paper_click(IInspectable const&, RoutedEventArgs const&)
	{
		//using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		tb_version().Visibility(UI_VISIBLE);
		const auto def_btn = cd_sample_dialog().DefaultButton();
		const auto pri_text = cd_sample_dialog().PrimaryButtonText();
		const auto close_text = cd_sample_dialog().CloseButtonText();
		cd_sample_dialog().PrimaryButtonText(L"");
		cd_sample_dialog().CloseButtonText(L"OK");
		cd_sample_dialog().Title(box_value(L"GraphPaper"));
		m_sample_type = SAMPLE_TYPE::MISC;
		co_await cd_sample_dialog().ShowAsync();
		cd_sample_dialog().PrimaryButtonText(pri_text);
		cd_sample_dialog().CloseButtonText(close_text);
		cd_sample_dialog().DefaultButton(def_btn);
		tb_version().Visibility(UI_COLLAPSED);
		delete m_sample_shape;
#if defined(_DEBUG)
		debug_leak_cnt--;
#endif
		m_sample_shape = nullptr;
		// �o�[�W�������̃��b�Z�[�W�_�C�A���O��\������.
		//message_show(ICON_INFO, L"str_appname", L"str_version");
	}

	// ���̑����j���[�́u�F�̕\�L�v�̃T�u���ڂ��I�����ꂽ.
	void MainPage::color_code_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == rmfi_color_code_cent() || sender == rmfi_color_code_cent_2()) {
			m_color_code = COLOR_CODE::CENT;
		}
		else if (sender == rmfi_color_code_dec() || sender == rmfi_color_code_dec_2()) {
			m_color_code = COLOR_CODE::DEC;
		}
		else if (sender == rmfi_color_code_hex() || sender == rmfi_color_code_hex_2()) {
			m_color_code = COLOR_CODE::HEX;
		}
		else if (sender == rmfi_color_code_real() || sender == rmfi_color_code_real_2()) {
			m_color_code = COLOR_CODE::REAL;
		}
	}

	// ���̑����j���[�́u�F�̕\�L�v�Ɉ������.
	void MainPage::color_code_is_checked(const COLOR_CODE value)
	{
		rmfi_color_code_dec().IsChecked(value == COLOR_CODE::DEC);
		rmfi_color_code_dec_2().IsChecked(value == COLOR_CODE::DEC);
		rmfi_color_code_hex().IsChecked(value == COLOR_CODE::HEX);
		rmfi_color_code_hex_2().IsChecked(value == COLOR_CODE::HEX);
		rmfi_color_code_real().IsChecked(value == COLOR_CODE::REAL);
		rmfi_color_code_real_2().IsChecked(value == COLOR_CODE::REAL);
		rmfi_color_code_cent().IsChecked(value == COLOR_CODE::CENT);
		rmfi_color_code_cent_2().IsChecked(value == COLOR_CODE::CENT);
	}

	// ���̑����j���[�́u�����̒P�ʁv�̃T�u���ڂ��I�����ꂽ.
	void MainPage::len_unit_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		LEN_UNIT l_unit;
		if (sender == rmfi_len_unit_grid() || sender == rmfi_len_unit_grid_2()) {
			l_unit = LEN_UNIT::GRID;
		}
		else if (sender == rmfi_len_unit_inch() || sender == rmfi_len_unit_inch_2()) {
			l_unit = LEN_UNIT::INCH;
		}
		else if (sender == rmfi_len_unit_milli() || sender == rmfi_len_unit_milli_2()) {
			l_unit = LEN_UNIT::MILLI;
		}
		else if (sender == rmfi_len_unit_pixel() || sender == rmfi_len_unit_pixel_2()) {
			l_unit = LEN_UNIT::PIXEL;
		}
		else if (sender == rmfi_len_unit_point() || sender == rmfi_len_unit_point_2()) {
			l_unit = LEN_UNIT::POINT;
		}
		else {
			return;
		}
		if (m_len_unit == l_unit) {
			return;
		}
		m_len_unit = l_unit;
		status_bar_set_curs();
		status_bar_set_grid();
		status_bar_set_sheet();
		status_bar_set_unit();
	}

	// ���̑����j���[�́u�����̒P�ʁv�Ɉ������.
	void MainPage::len_unit_is_checked(const LEN_UNIT value)
	{
		rmfi_len_unit_grid().IsChecked(value == LEN_UNIT::GRID);
		rmfi_len_unit_grid_2().IsChecked(value == LEN_UNIT::GRID);
		rmfi_len_unit_inch().IsChecked(value == LEN_UNIT::INCH);
		rmfi_len_unit_inch_2().IsChecked(value == LEN_UNIT::INCH);
		rmfi_len_unit_milli().IsChecked(value == LEN_UNIT::MILLI);
		rmfi_len_unit_milli_2().IsChecked(value == LEN_UNIT::MILLI);
		rmfi_len_unit_pixel().IsChecked(value == LEN_UNIT::PIXEL);
		rmfi_len_unit_pixel_2().IsChecked(value == LEN_UNIT::PIXEL);
		rmfi_len_unit_point().IsChecked(value == LEN_UNIT::POINT);
		rmfi_len_unit_point_2().IsChecked(value == LEN_UNIT::POINT);
	}

	// ���̑����j���[�́u���_���d�˂�...�v���I�����ꂽ.
	IAsyncAction MainPage::pile_up_vert_click_async(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		pile_up_vert_set_header(m_pile_up_vert);
		sd_pile_up_vert().Value(static_cast<double>(m_pile_up_vert));
		const auto d_result{ co_await cd_pile_up_vert().ShowAsync() };
		if (d_result == ContentDialogResult::Primary) {
			m_pile_up_vert = static_cast<float>(sd_pile_up_vert().Value());
		}
	}

	void MainPage::pile_up_vert_set_header(const float value) noexcept
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, value, m_sheet_dx.m_logical_dpi, m_sample_sheet.m_grid_base + 1.0f, buf);
		const auto text = ResourceLoader::GetForCurrentView().GetString(L"str_pile_up_vert") + L": " + buf;
		sd_pile_up_vert().Header(box_value(text));
	}

	// �X���C�_�[�̒l���ύX���ꂽ.
	void MainPage::pile_up_vert_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args) noexcept
	{
		pile_up_vert_set_header(static_cast<float>(args.NewValue()));
	}


}