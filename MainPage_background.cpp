#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �w�i�p�^�[���̉摜�u���V�𓾂�.
	void MainPage::background_get_brush()
	{
		// WIC �t�@�N�g�����g����, �摜�t�@�C����ǂݍ��� WIC �f�R�[�_�[���쐬����.
		// WIC �t�@�N�g���� ShapeImage ���m�ۂ��Ă�����̂��g�p����.
		winrt::com_ptr<IWICBitmapDecoder> wic_decoder;
		winrt::check_hresult(
			ShapeImage::wic_factory->CreateDecoderFromFilename(L"Assets/background.png", nullptr,
				GENERIC_READ, WICDecodeMetadataCacheOnDemand, wic_decoder.put())
		);
		// �ǂݍ��܂ꂽ�摜�̃t���[�����𓾂� (�ʏ�� 1 �t���[��).
		UINT f_cnt;
		winrt::check_hresult(
			wic_decoder->GetFrameCount(&f_cnt)
		);
		// �Ō�̃t���[���𓾂�.
		winrt::com_ptr<IWICBitmapFrameDecode> wic_frame;
		winrt::check_hresult(
			wic_decoder->GetFrame(f_cnt - 1, wic_frame.put())
		);
		wic_decoder = nullptr;
		// WIC �t�@�N�g�����g����, WIC �t�H�[�}�b�g�R���o�[�^�[���쐬����.
		winrt::check_hresult(
			ShapeImage::wic_factory->CreateFormatConverter(m_wic_background.put())
		);
		// WIC �t�H�[�}�b�g�R���o�[�^�[��, �����t���[�����i�[����.
		winrt::check_hresult(
			m_wic_background->Initialize(wic_frame.get(), GUID_WICPixelFormat32bppPBGRA,
				WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom)
		);
		wic_frame = nullptr;
	}

	// �w�i�F�̍��ڂɈ������
	void MainPage::background_color_is_checked(const bool checker_board, const D2D1_COLOR_F& color)
	{
		if (checker_board) {
			tmfi_background_pattern().IsChecked(true);
		}
		else {
			tmfi_background_pattern().IsChecked(false);
		}
		if (equal(color, COLOR_BLACK)) {
			rmfi_background_black().IsChecked(true);
		}
		else {
			rmfi_background_white().IsChecked(true);
		}
	}

	// �w�i�p�^�[�����N���b�N���ꂽ.
	void MainPage::background_pattern_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == tmfi_background_pattern()) {
			if (m_background_show != tmfi_background_pattern().IsChecked()) {
				m_background_show = tmfi_background_pattern().IsChecked();
				main_draw();
			}
		}
		else if (sender == rmfi_background_white()) {
			if (!equal(m_background_color, COLOR_WHITE)) {
				m_background_color = COLOR_WHITE;
				main_draw();
			}
		}
		else if (sender == rmfi_background_black()) {
			if (!equal(m_background_color, COLOR_BLACK)) {
				m_background_color = COLOR_BLACK;
				main_draw();
			}
		}
	}

}