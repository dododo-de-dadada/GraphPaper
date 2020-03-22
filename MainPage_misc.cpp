//-------------------------------
// MainPage_app.cpp
// �A�v���P�[�V�����̒��f�ƍĊJ
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ���̑����j���[�́u�F�����̕\�L�v�Ɉ������.
	void MainPage::color_code_check_menu(void)
	{
		rmfi_color_dec().IsChecked(m_color_code == COLOR_CODE::DEC);
		rmfi_color_hex().IsChecked(m_color_code == COLOR_CODE::HEX);
		rmfi_color_real().IsChecked(m_color_code == COLOR_CODE::REAL);
		rmfi_color_cent().IsChecked(m_color_code == COLOR_CODE::CENT);
		rmfi_color_dec_2().IsChecked(m_color_code == COLOR_CODE::DEC);
		rmfi_color_hex_2().IsChecked(m_color_code == COLOR_CODE::HEX);
		rmfi_color_real_2().IsChecked(m_color_code == COLOR_CODE::REAL);
		rmfi_color_cent_2().IsChecked(m_color_code == COLOR_CODE::CENT);
	}

	// �F�����\�L��I������.
	template <COLOR_CODE C>
	void MainPage::color_code_click(void)
	{
		if (m_color_code == C) {
			return;
		}
		m_color_code = C;
	}

	// ���̑����j���[�́u�����̒P�ʁv�Ɉ������.
	void MainPage::len_unit_check_menu(void)
	{
		rmfi_len_grid().IsChecked(m_len_unit == LEN_UNIT::GRID);
		rmfi_len_inch().IsChecked(m_len_unit == LEN_UNIT::INCH);
		rmfi_len_milli().IsChecked(m_len_unit == LEN_UNIT::MILLI);
		rmfi_len_pixel().IsChecked(m_len_unit == LEN_UNIT::PIXEL);
		rmfi_len_point().IsChecked(m_len_unit == LEN_UNIT::POINT);
		rmfi_len_grid_2().IsChecked(m_len_unit == LEN_UNIT::GRID);
		rmfi_len_inch_2().IsChecked(m_len_unit == LEN_UNIT::INCH);
		rmfi_len_milli_2().IsChecked(m_len_unit == LEN_UNIT::MILLI);
		rmfi_len_pixel_2().IsChecked(m_len_unit == LEN_UNIT::PIXEL);
		rmfi_len_point_2().IsChecked(m_len_unit == LEN_UNIT::POINT);
	}

	// �����̒P�ʂ�I������.
	template <LEN_UNIT L>
	void MainPage::len_unit_click(void)
	{
		if (m_len_unit == L) {
			return;
		}
		m_len_unit = L;
		status_bar_set_curs();
		status_bar_set_grid();
		status_bar_set_page();
		status_bar_set_unit();
	}

	// ���̑����j���[�́u�F�����̕\�L�v>�u10�i���v���I�����ꂽ.
	void MainPage::rmfi_color_dec_click(IInspectable const&, RoutedEventArgs const&)
	{
		color_code_click<COLOR_CODE::DEC>();
	}

	// ���̑����j���[�́u�F�����̕\�L�v>�u16�i���v���I�����ꂽ.
	void MainPage::rmfi_color_hex_click(IInspectable const&, RoutedEventArgs const&)
	{
		color_code_click<COLOR_CODE::HEX>();
	}

	// ���̑����j���[�́u�F�����̕\�L�v>�u�����v���I�����ꂽ.
	void MainPage::rmfi_color_real_click(IInspectable const&, RoutedEventArgs const&)
	{
		color_code_click<COLOR_CODE::REAL>();
	}

	// ���̑����j���[�́u�F�����̕\�L�v>�u�p�[�Z���g�v���I�����ꂽ.
	void MainPage::rmfi_color_cent_click(IInspectable const&, RoutedEventArgs const&)
	{
		color_code_click<COLOR_CODE::CENT>();
	}

	// ���̑����j���[�́u�����̒P�ʁv>�u����v���I�����ꂽ.
	void MainPage::rmfi_len_grid_click(IInspectable const&, RoutedEventArgs const&)
	{
		len_unit_click<LEN_UNIT::GRID>();
	}

	// ���̑����j���[�́u�����̒P�ʁv>�u�C���`�v���I�����ꂽ.
	void MainPage::rmfi_len_inch_click(IInspectable const&, RoutedEventArgs const&)
	{
		len_unit_click<LEN_UNIT::INCH>();
	}

	// ���̑����j���[�́u�����̒P�ʁv>�u�|�C���g�v���I�����ꂽ.
	void MainPage::rmfi_len_milli_click(IInspectable const&, RoutedEventArgs const&)
	{
		len_unit_click<LEN_UNIT::MILLI>();
	}

	// ���̑����j���[�́u�����̒P�ʁv>�u�s�N�Z���v���I�����ꂽ.
	void MainPage::rmfi_len_pixel_click(IInspectable const&, RoutedEventArgs const&)
	{
		len_unit_click<LEN_UNIT::PIXEL>();
	}

	// ���̑����j���[�́u�����̒P�ʁv>�u�|�C���g�v���I�����ꂽ.
	void MainPage::rmfi_len_point_click(IInspectable const&, RoutedEventArgs const&)
	{
		len_unit_click<LEN_UNIT::POINT>();
	}

}