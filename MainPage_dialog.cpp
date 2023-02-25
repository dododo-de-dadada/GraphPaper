//-------------------------------
// MainPage_dialog.cpp
// �ݒ�_�C�A���O
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �ӂ��͎��̏����ŌĂ΂��.
	// 1. opend
	// 2. size_changed
	// 3. loaded
	// 4. scale_changed
	// 5. closed
	// ���Ȃ݂�, unloaded �͌Ă΂�Ȃ�.
	// �Ƃ��낪, �f�o�b�O�𑱂��Ă����, ���܂�,
	// 1. size_changed
	// 2. opened
	// 3. loaded
	// ���邢��,
	// 1. size_changed
	// 2. loaded
	// 3. opened
	// �̏��ԂŌĂ΂�邱�Ƃ�����, 
	// �ӂ��ƈႤ���ԂŌĂяo���ꂽ�Ƃ�, �X���b�v�`�F�[���p�l���ɂ͉����\������Ȃ��ɂ��ւ�炸, 
	// Direct2D �̓G���[��Ԃ��Ȃ�.
	// loaded ��, �X���b�v�`�F�[���p�l���� UpdateLayout ���Ăяo�����Ƃł��������ł��邩��.
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::Foundation::Uri;
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapDecoder;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::UI::Xaml::Window;

	// �ݒ�_�C�A���O�̃X���C�_�[�w�b�_�[�ɕ�������i�[����.
	// S	�X���C�_�[�̔ԍ�
	// text	������
	template<int S> 
	void MainPage::dialog_set_slider_header(const winrt::hstring& text)
	{
		if constexpr (S == 0) {
			dialog_slider_0().Header(box_value(text));
		}
		if constexpr (S == 1) {
			dialog_slider_1().Header(box_value(text));
		}
		if constexpr (S == 2) {
			dialog_slider_2().Header(box_value(text));
		}
		if constexpr (S == 3) {
			dialog_slider_3().Header(box_value(text));
		}
	}
	template void MainPage::dialog_set_slider_header<0>(const winrt::hstring& text);
	template void MainPage::dialog_set_slider_header<1>(const winrt::hstring& text);
	template void MainPage::dialog_set_slider_header<2>(const winrt::hstring& text);
	template void MainPage::dialog_set_slider_header<3>(const winrt::hstring& text);

//#ifdef _DEBUG
//	enum struct DEBUG_DIALOG {
//		NIL,
//		LOADED,
//		OPENED,
//		CLOSED,
//		UNLOADED,
//		SIZE_CHANGED,
//		SCALE_CHANGED
//	} debug_dialog[124]{};
//	int debug_dialog_cnt = 0;
//#endif

	// �ݒ�_�C�A���O�̃X���b�v�`�F�[���p�l����\������
	void MainPage::dialog_draw(void)
	{
		if (!scp_dialog_panel().IsLoaded()) {
			return;
		}
		if (!m_mutex_draw.try_lock()) {
			// ���b�N�ł��Ȃ��ꍇ
			return;
		}
		// �`����̐ݒ�.
		m_dialog_page.begin_draw(m_dialog_d2d.m_d2d_context.get(), true, m_background.get(), 1.0f);
		m_dialog_d2d.m_d2d_context->SaveDrawingState(Shape::m_d2d_state_block.get());
		m_dialog_d2d.m_d2d_context->BeginDraw();
		m_dialog_d2d.m_d2d_context->Clear(m_background_color);
		const D2D1_RECT_F w_rect{
			0, 0, m_dialog_d2d.m_logical_width, m_dialog_d2d.m_logical_height
		};
		if (m_background_show) {
			// �w�i�p�^�[����`�悷��,
			m_dialog_d2d.m_d2d_context->FillRectangle(w_rect, Shape::m_d2d_bitmap_brush.get());
		}
		Shape::m_d2d_color_brush->SetColor(m_dialog_page.m_page_color);
		m_dialog_d2d.m_d2d_context->FillRectangle(w_rect, Shape::m_d2d_color_brush.get());

		const float offset = static_cast<FLOAT>(std::fmod(
			m_dialog_page.m_page_size.width * 0.5, m_dialog_page.m_grid_base + 1.0));
		m_dialog_page.m_grid_offset.x = offset;
		m_dialog_page.m_grid_offset.y = offset;
		m_dialog_page.m_page_padding.left = 0.0f;
		m_dialog_page.m_page_padding.top = 0.0f;
		m_dialog_page.m_page_padding.right = 0.0f;
		m_dialog_page.m_page_padding.bottom = 0.0f;
		m_dialog_page.draw();
		const auto hr = m_dialog_d2d.m_d2d_context->EndDraw();
		m_dialog_d2d.m_d2d_context->RestoreDrawingState(Shape::m_d2d_state_block.get());
		m_dialog_d2d.Present();
		m_mutex_draw.unlock();
	}

	// �_�C�A���O�̃��X�g�r���[�����[�h���ꂽ.
	void MainPage::dialog_list_loaded(IInspectable const&, RoutedEventArgs const&)
	{
		const auto item = lv_dialog_list().SelectedItem();
		if (item != nullptr) {
			lv_dialog_list().ScrollIntoView(item);
		}
	}

	// �����_�C�A���O���J���ꂽ.
	void MainPage::setting_dialog_opened(ContentDialog const& sender, ContentDialogOpenedEventArgs const&)
	{
//#ifdef _DEBUG
//		debug_dialog[debug_dialog_cnt++] = DEBUG_DIALOG::OPENED;
//#endif
	}

	// �����_�C�A���O������ꂽ.
	void MainPage::setting_dialog_closed(ContentDialog const&, ContentDialogClosedEventArgs const&)
	{
//#ifdef _DEBUG
//		debug_dialog[debug_dialog_cnt++] = DEBUG_DIALOG::CLOSED;
//		debug_dialog_cnt = 0;
//		if (debug_dialog[0] != DEBUG_DIALOG::OPENED) {
//			__debugbreak();
//		}
//#endif
	}

	// �����_�C�A���O������ꂽ.
	void MainPage::setting_dialog_unloaded(IInspectable const&, RoutedEventArgs const&)
	{
//#ifdef _DEBUG
//		debug_dialog[debug_dialog_cnt++] = DEBUG_DIALOG::UNLOADED;
//#endif
		m_dialog_d2d.Trim();
	}

	// �����̉摜��ǂݍ���
	// p_width	�p�l���̕�
	// p_height	�p�l���̍���
	IAsyncAction MainPage::dialog_image_load_async(const float p_width, const float p_height)
	{
		bool ok;
		winrt::apartment_context context;
		co_await winrt::resume_background();
		try {
			const StorageFile file{
				co_await StorageFile::GetFileFromApplicationUriAsync(Uri{ L"ms-appx:///Assets/4.1.05.tiff" })
			};
			const IRandomAccessStream stream{
				co_await file.OpenAsync(FileAccessMode::Read)
			};
			const BitmapDecoder decoder{
				co_await BitmapDecoder::CreateAsync(stream)
			};
			const SoftwareBitmap bitmap{
				co_await decoder.GetSoftwareBitmapAsync(BitmapPixelFormat::Bgra8, BitmapAlphaMode::Straight)
			};
			const D2D1_POINT_2F pos{
				static_cast<FLOAT>(p_width * 0.125), static_cast<FLOAT>(p_height * 0.125)
			};
			const D2D1_SIZE_F size{
				static_cast<float>(p_width * 0.75), static_cast<FLOAT>(p_height * 0.75)
			};
			ShapeImage* s = new ShapeImage(pos, size, bitmap, m_dialog_page.m_image_opac);
			bitmap.Close();

			m_dialog_page.m_shape_list.push_back(s);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			ok = true;
		}
		catch (winrt::hresult_error&) {
			ok = false;
		}
		co_await context;
		if (ok) {
			dialog_draw();
		}
	}

	// �����̃X���b�v�`�F�[���p�l�����ǂݍ��܂ꂽ.
	void MainPage::dialog_panel_loaded(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender != scp_dialog_panel()) {
			return;
		}
//#ifdef _DEBUG
//		debug_dialog[debug_dialog_cnt++] = DEBUG_DIALOG::LOADED;
//#endif
//		dialog_draw();
	}

	// �����̃X���b�v�`�F�[���p�l���̐��@���ς����.
	void MainPage::dialog_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
	{
		if (sender != scp_dialog_panel()) {
			return;
		}
//#ifdef _DEBUG
//		debug_dialog[debug_dialog_cnt++] = DEBUG_DIALOG::SIZE_CHANGED;
//#endif
		m_dialog_d2d.SetSwapChainPanel(scp_dialog_panel());
		const float w = args.NewSize().Width;
		const float h = args.NewSize().Height;
		m_dialog_page.m_page_size.width = w;
		m_dialog_page.m_page_size.height = h;
		m_dialog_d2d.SetLogicalSize2({ w, h });
		dialog_draw();
	}

	// �����̃X���b�v�`�F�[���p�l���̔{�����ς����.
	void MainPage::dialog_panel_scale_changed(IInspectable const&, IInspectable const&)
	{
//#ifdef _DEBUG
//		debug_dialog[debug_dialog_cnt++] = DEBUG_DIALOG::SCALE_CHANGED;
//#endif
		const float comp_x = scp_dialog_panel().CompositionScaleX();
		const float comp_y = scp_dialog_panel().CompositionScaleY();
		m_dialog_d2d.SetCompositionScale(comp_x, comp_y);

		dialog_draw();
	}

}
