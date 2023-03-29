//-------------------------------
// MainPage_import.cpp
// 画像をインポートする.
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Graphics::Imaging::BitmapDecoder;
	using winrt::Windows::Storage::Pickers::FileOpenPicker;
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::UI::Xaml::Window;

	//-------------------------------
	// ファイルメニューの「画像をインポートする...」が選択された.
	//-------------------------------
	IAsyncAction MainPage::file_import_image_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		// co_await してるにもかかわらず, ファイル開くピッカーが返値を戻すまで時間がかかる.
		// その間フォアグランドのスレッドが動作してしまう.
		m_mutex_event.lock();

		// ファイルオープンピッカーを取得して開く.
		FileOpenPicker open_picker{ FileOpenPicker() };
		open_picker.FileTypeFilter().Append(L".bmp");
		open_picker.FileTypeFilter().Append(L".gif");
		open_picker.FileTypeFilter().Append(L".jpg");
		open_picker.FileTypeFilter().Append(L".jpeg");
		open_picker.FileTypeFilter().Append(L".png");
		open_picker.FileTypeFilter().Append(L".tif");
		open_picker.FileTypeFilter().Append(L".tiff");

		// ピッカーを非同期に表示してストレージファイルを取得する.
		// (「閉じる」ボタンが押された場合ストレージファイルは nullptr.)
		StorageFile open_file{
			co_await open_picker.PickSingleFileAsync()
		};
		open_picker = nullptr;

		// ストレージファイルがヌルポインターか判定する.
		if (open_file != nullptr) {

			// 待機カーソルを表示, 表示する前のカーソルを得る.
			const CoreCursor& prev_cur = wait_cursor_show();
			unselect_all();

			IRandomAccessStream stream{
				co_await open_file.OpenAsync(FileAccessMode::Read) 
			};
			BitmapDecoder decoder{
				co_await BitmapDecoder::CreateAsync(stream) 
			};
			SoftwareBitmap bitmap{ 
				SoftwareBitmap::Convert(
					co_await decoder.GetSoftwareBitmapAsync(), BitmapPixelFormat::Bgra8) 
			};

			// 表示された部分の中心の位置を求める.
			const double scale = m_main_page.m_page_scale;
			const double win_x = sb_horz().Value();
			const double win_y = sb_vert().Value();
			const double win_w = scp_main_panel().ActualWidth();
			const double win_h = scp_main_panel().ActualHeight();
			const double image_w = bitmap.PixelWidth();
			const double image_h = bitmap.PixelHeight();
			const D2D1_POINT_2F ctr{
				static_cast<FLOAT>((win_x + (win_w - image_w) * 0.5) / scale),
				static_cast<FLOAT>((win_y + (win_h - image_h) * 0.5) / scale)
			};
			const D2D1_SIZE_F p_size{ 
				static_cast<FLOAT>(image_w), static_cast<FLOAT>(image_h)
			};
			ShapeImage* s = new ShapeImage(ctr, p_size, bitmap, 1.0);
#if (_DEBUG)
			debug_leak_cnt++;
#endif
			bitmap.Close();
			bitmap = nullptr;
			decoder = nullptr;
			stream.Close();
			stream = nullptr;

			{
				m_mutex_draw.lock();
				ustack_push_append(s);
				ustack_push_select(s);
				ustack_push_null();
				m_mutex_draw.unlock();
			}

			ustack_is_enable();

			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_append(s);
				summary_select(s);
			}
			xcvd_is_enabled();
			main_bbox_update(s);
			main_panel_size();
			main_draw();

			// カーソルを元に戻す.
			Window::Current().CoreWindow().PointerCursor(prev_cur);
		}
		m_mutex_event.unlock();
	};

}