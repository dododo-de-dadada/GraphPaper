#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 背景パターンの画像ブラシを得る.
	void MainPage::background_get_brush()
	{
		// WIC ファクトリを使って, 画像ファイルを読み込み WIC デコーダーを作成する.
		// WIC ファクトリは ShapeImage が確保しているものを使用する.
		winrt::com_ptr<IWICBitmapDecoder> wic_decoder;
		winrt::check_hresult(
			ShapeImage::wic_factory->CreateDecoderFromFilename(L"Assets/background.png", nullptr,
				GENERIC_READ, WICDecodeMetadataCacheOnDemand, wic_decoder.put())
		);
		// 読み込まれた画像のフレーム数を得る (通常は 1 フレーム).
		UINT f_cnt;
		winrt::check_hresult(
			wic_decoder->GetFrameCount(&f_cnt)
		);
		// 最後のフレームを得る.
		winrt::com_ptr<IWICBitmapFrameDecode> wic_frame;
		winrt::check_hresult(
			wic_decoder->GetFrame(f_cnt - 1, wic_frame.put())
		);
		wic_decoder = nullptr;
		// WIC ファクトリを使って, WIC フォーマットコンバーターを作成する.
		winrt::check_hresult(
			ShapeImage::wic_factory->CreateFormatConverter(m_wic_background.put())
		);
		// WIC フォーマットコンバーターに, 得たフレームを格納する.
		winrt::check_hresult(
			m_wic_background->Initialize(wic_frame.get(), GUID_WICPixelFormat32bppPBGRA,
				WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom)
		);
		wic_frame = nullptr;
	}

	// 背景色の項目に印をつける
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

	// 背景パターンがクリックされた.
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