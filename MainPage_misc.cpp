//-----------------------------
// MainPage_misc.cpp
// �����̒P��, �F�̕\�L, �X�e�[�^�X�o�[, �o�[�W�������
//-----------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ���̑����j���[�́u�F�̕\�L�v>�u�p�[�Z���g�v���I�����ꂽ.
	void MainPage::color_code_cent_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_color_code = COLOR_CODE::CENT;
	}

	// ���̑����j���[�́u�F�̕\�L�v�Ɉ������.
	void MainPage::color_code_check_menu(void)
	{
		rmfi_color_code_dec().IsChecked(m_color_code == COLOR_CODE::DEC);
		rmfi_color_code_hex().IsChecked(m_color_code == COLOR_CODE::HEX);
		rmfi_color_code_real().IsChecked(m_color_code == COLOR_CODE::REAL);
		rmfi_color_code_cent().IsChecked(m_color_code == COLOR_CODE::CENT);
		rmfi_color_code_dec_2().IsChecked(m_color_code == COLOR_CODE::DEC);
		rmfi_color_code_hex_2().IsChecked(m_color_code == COLOR_CODE::HEX);
		rmfi_color_code_real_2().IsChecked(m_color_code == COLOR_CODE::REAL);
		rmfi_color_code_cent_2().IsChecked(m_color_code == COLOR_CODE::CENT);
	}

	// ���̑����j���[�́u�F�̕\�L�v>�u10�i���v���I�����ꂽ.
	void MainPage::color_code_dec_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_color_code = COLOR_CODE::DEC;
	}

	// ���̑����j���[�́u�F�̕\�L�v>�u16�i���v���I�����ꂽ.
	void MainPage::color_code_hex_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_color_code = COLOR_CODE::HEX;
	}

	// ���̑����j���[�́u�F�̕\�L�v>�u�����v���I�����ꂽ.
	void MainPage::color_code_real_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_color_code = COLOR_CODE::REAL;
	}

	// ���̑����j���[�́u�����̒P�ʁv�Ɉ������.
	void MainPage::len_unit_check_menu(void)
	{
		rmfi_len_unit_grid().IsChecked(m_len_unit == LEN_UNIT::GRID);
		rmfi_len_unit_inch().IsChecked(m_len_unit == LEN_UNIT::INCH);
		rmfi_len_unit_milli().IsChecked(m_len_unit == LEN_UNIT::MILLI);
		rmfi_len_unit_pixel().IsChecked(m_len_unit == LEN_UNIT::PIXEL);
		rmfi_len_unit_point().IsChecked(m_len_unit == LEN_UNIT::POINT);
		rmfi_len_unit_grid_2().IsChecked(m_len_unit == LEN_UNIT::GRID);
		rmfi_len_unit_inch_2().IsChecked(m_len_unit == LEN_UNIT::INCH);
		rmfi_len_unit_milli_2().IsChecked(m_len_unit == LEN_UNIT::MILLI);
		rmfi_len_unit_pixel_2().IsChecked(m_len_unit == LEN_UNIT::PIXEL);
		rmfi_len_unit_point_2().IsChecked(m_len_unit == LEN_UNIT::POINT);
	}

	// ���̑����j���[�́u�����̒P�ʁv>�u����v���I�����ꂽ.
	void MainPage::len_unit_grid_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_len_unit == LEN_UNIT::GRID) {
			return;
		}
		m_len_unit = LEN_UNIT::GRID;
		stbar_set_curs();
		stbar_set_grid();
		stbar_set_page();
		stbar_set_unit();
	}

	// ���̑����j���[�́u�����̒P�ʁv>�u�C���`�v���I�����ꂽ.
	void MainPage::len_unit_inch_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_len_unit == LEN_UNIT::INCH) {
			return;
		}
		m_len_unit = LEN_UNIT::INCH;
		stbar_set_curs();
		stbar_set_grid();
		stbar_set_page();
		stbar_set_unit();
	}

	// ���̑����j���[�́u�����̒P�ʁv>�u�|�C���g�v���I�����ꂽ.
	void MainPage::len_unit_milli_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_len_unit == LEN_UNIT::MILLI) {
			return;
		}
		m_len_unit = LEN_UNIT::MILLI;
		stbar_set_curs();
		stbar_set_grid();
		stbar_set_page();
		stbar_set_unit();
	}

	// ���̑����j���[�́u�����̒P�ʁv>�u�s�N�Z���v���I�����ꂽ.
	void MainPage::len_unit_pixel_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_len_unit == LEN_UNIT::PIXEL) {
			return;
		}
		m_len_unit = LEN_UNIT::PIXEL;
		stbar_set_curs();
		stbar_set_grid();
		stbar_set_page();
		stbar_set_unit();
	}

	// ���̑����j���[�́u�����̒P�ʁv>�u�|�C���g�v���I�����ꂽ.
	void MainPage::len_unit_point_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_len_unit == LEN_UNIT::POINT) {
			return;
		}
		m_len_unit = LEN_UNIT::POINT;
		stbar_set_curs();
		stbar_set_grid();
		stbar_set_page();
		stbar_set_unit();
	}

}