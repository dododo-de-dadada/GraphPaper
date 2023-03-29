//-------------------------------
// MainPage_import.cpp
// �摜���C���|�[�g����.
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
	// �t�@�C�����j���[�́u�摜���C���|�[�g����...�v���I�����ꂽ.
	//-------------------------------
	IAsyncAction MainPage::file_import_image_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		// co_await ���Ă�ɂ�������炸, �t�@�C���J���s�b�J�[���Ԓl��߂��܂Ŏ��Ԃ�������.
		// ���̊ԃt�H�A�O�����h�̃X���b�h�����삵�Ă��܂�.
		m_mutex_event.lock();

		// �t�@�C���I�[�v���s�b�J�[���擾���ĊJ��.
		FileOpenPicker open_picker{ FileOpenPicker() };
		open_picker.FileTypeFilter().Append(L".bmp");
		open_picker.FileTypeFilter().Append(L".gif");
		open_picker.FileTypeFilter().Append(L".jpg");
		open_picker.FileTypeFilter().Append(L".jpeg");
		open_picker.FileTypeFilter().Append(L".png");
		open_picker.FileTypeFilter().Append(L".tif");
		open_picker.FileTypeFilter().Append(L".tiff");

		// �s�b�J�[��񓯊��ɕ\�����ăX�g���[�W�t�@�C�����擾����.
		// (�u����v�{�^���������ꂽ�ꍇ�X�g���[�W�t�@�C���� nullptr.)
		StorageFile open_file{
			co_await open_picker.PickSingleFileAsync()
		};
		open_picker = nullptr;

		// �X�g���[�W�t�@�C�����k���|�C���^�[�����肷��.
		if (open_file != nullptr) {

			// �ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
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

			// �\�����ꂽ�����̒��S�̈ʒu�����߂�.
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

			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_append(s);
				summary_select(s);
			}
			xcvd_is_enabled();
			main_bbox_update(s);
			main_panel_size();
			main_draw();

			// �J�[�\�������ɖ߂�.
			Window::Current().CoreWindow().PointerCursor(prev_cur);
		}
		m_mutex_event.unlock();
	};

}