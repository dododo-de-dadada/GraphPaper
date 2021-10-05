//-------------------------------
// MainPage_file.cpp
// �t�@�C���̓ǂݏ���
//-------------------------------
#include "pch.h"
#include "MainPage.h"

/*
�������A4��Windows�����^�C���񓯊�����^�C�v�iIAsyncXxx�j�̂����ꂩ��
co_await�����ꍇ�AC ++ / WinRT��co_await�̎��_�ŌĂяo���R���e�L�X�g���L���v�`�����܂��B
�܂��A�p�����ĊJ���ꂽ�Ƃ��ɁA���̃R���e�L�X�g�ɂ��邱�Ƃ��ۏ؂���܂��B
C ++ / WinRT�́A�Ăяo�����̃R���e�L�X�g�Ɋ��ɂ��邩���m�F���A
�����łȂ��ꍇ�͐؂�ւ��܂��B
*/

/*
Yes the behavior changed between the April 2018 and October 2018 releases, and the default is now Disabled.
This is a privacy constraint - we're very focused on maintaining the user's privacy.
The documentation for this is up-to-date: https://docs.microsoft.com/en-us/windows/uwp/files/file-access-permissions#accessing-additional-locations.
As of right now, if you want to detect whether the setting is enabled or disabled, 
you can simply try to access some file/folder to which this setting would grant you permission if enabled and deny permission if disabled (eg, "C:\").
If disabled, you can then launch the Settings app on the File System privacy page.
For example:

protected override async void OnNavigatedTo(NavigationEventArgs e)
{
	try
	{
		StorageFolder folder = await StorageFolder.GetFolderFromPathAsync(@"C:\");
		// do work
	}
	catch
	{
		MessageDialog dlg = new MessageDialog(
			"It seems you have not granted permission for this app to access the file system broadly. " +
			"Without this permission, the app will only be able to access a very limited set of filesystem locations. " +
			"You can grant this permission in the Settings app, if you wish. You can do this now or later. " +
			"If you change the setting while this app is running, it will terminate the app so that the " +
			"setting can be applied. Do you want to do this now?",
			"File system permissions");
		dlg.Commands.Add(new UICommand("Yes", new UICommandInvokedHandler(InitMessageDialogHandler), 0));
		dlg.Commands.Add(new UICommand("No", new UICommandInvokedHandler(InitMessageDialogHandler), 1));
		dlg.DefaultCommandIndex = 0;
		dlg.CancelCommandIndex = 1;
		await dlg.ShowAsync();
	}
}

private async void InitMessageDialogHandler(IUICommand command)
{
	if ((int)command.Id == 0)
	{
		await Launcher.LaunchUriAsync(new Uri("ms-settings:privacy-broadfilesystemaccess"));
	}
}
*/

// �ȉ��̏��͌Â�.
// GetFileFromPathAsync, CreateFileAsync �� E_ACCESSDENIED �Ȃ��Ɏg���ɂ͈ȉ����K�v�ɂȂ�.
// 1. XAML�e�L�X�g�G�f�B�^���g����, Pakage.appxmanifest �� broadFileSystemAccess ��ǉ�����.
// Pakage.appxmanifest �� <Package> �̃v���p�e�B�[��
// xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities" ��ǉ�����
// IgnorableNamespaces="uap mp" �� IgnorableNamespaces="uap mp rescap" �ɕύX����
// 2. �����Ȃ���� <Capabilities> �^�O�� <Package> �̎q�v�f�Ƃ��Ēǉ�����.
// <Capabilities> �̎q�v�f�Ƃ��� <rescap:Capability Name="broadFileSystemAccess" /> ��������
// 3. �R���p�C�����邽�т�, �ݒ���g���ăA�v���Ƀt�@�C���A�N�Z�X��������.
// �X�^�[�g���j���[ > �ݒ� > �v���C�o�[ > �t�@�C�� �V�X�e�� > �t�@�C���V�X�e���ɃA�N�Z�X�ł���A�v����I��
// �\������Ă���A�v�����I���ɂ���.
// �܂���,
// �X�^�[�g���j���[ > �ݒ� > �A�v�� > �A�v���Ƌ@�\
// �\������Ă���A�v�����N���b�N����.
// �ڍ׃I�v�V���� > �A�v���̃A�N�Z�X����
// �t�@�C�� �V�X�e�����I���ɂ���.

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::Foundation::Uri;
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapBufferAccessMode;
	using winrt::Windows::Graphics::Imaging::BitmapDecoder;
	using winrt::Windows::Graphics::Imaging::BitmapEncoder;
	using winrt::Windows::Graphics::Imaging::BitmapInterpolationMode;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::Graphics::Imaging::BitmapRotation;
	using winrt::Windows::Graphics::Imaging::SoftwareBitmap;
	using winrt::Windows::Storage::AccessCache::AccessListEntry;
	using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
	using winrt::Windows::Storage::CachedFileManager;
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::Storage::Pickers::FileOpenPicker;
	using winrt::Windows::Storage::Pickers::FileSavePicker;
	using winrt::Windows::Storage::Pickers::PickerLocationId;
	using winrt::Windows::Storage::Provider::FileUpdateStatus;
	using winrt::Windows::Storage::StorageFolder;
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;
	using winrt::Windows::System::Launcher;
	using winrt::Windows::UI::Core::CoreCursorType;
	using winrt::Windows::UI::ViewManagement::ApplicationView;

	static CoreCursor const& CUR_WAIT = CoreCursor(CoreCursorType::Wait, 0);	// �ҋ@�J�[�\��.
	constexpr wchar_t DESC_GPF[] = L"str_desc_gpf";	// �g���q gpf �̐���
	constexpr wchar_t DESC_SVG[] = L"str_desc_svg";	// �g���q svg �̐���
	constexpr wchar_t ERR_FONT[] = L"str_err_font";	// �L���łȂ����̂̃G���[���b�Z�[�W�̃��\�[�X��
	constexpr wchar_t ERR_READ[] = L"str_err_read";	// �ǂݍ��݃G���[���b�Z�[�W�̃��\�[�X��
	constexpr wchar_t ERR_RECENT[] = L"str_err_recent";	// �ŋߎg�����t�@�C���̃G���[���b�Z�[�W�̃��\�[�X��
	constexpr wchar_t ERR_WRITE[] = L"str_err_write";	// �������݃G���[���b�Z�[�W�̃��\�[�X��
	constexpr wchar_t EXT_BMP[] = L".bmp";	// �摜�t�@�C���̊g���q
	constexpr wchar_t EXT_GIF[] = L".gif";	// �摜�t�@�C���̊g���q
	constexpr wchar_t EXT_GPF[] = L".gpf";	// �}�`�f�[�^�t�@�C���̊g���q
	constexpr wchar_t EXT_JPEG[] = L".jpeg";	// �摜�t�@�C���̊g���q
	constexpr wchar_t EXT_JPG[] = L".jpg";	// �摜�t�@�C���̊g���q
	constexpr wchar_t EXT_PNG[] = L".png";	// �摜�t�@�C���̊g���q
	constexpr wchar_t EXT_SVG[] = L".svg";	// SVG �t�@�C���̊g���q
	constexpr wchar_t EXT_TIF[] = L".tif";	// �摜�t�@�C���̊g���q
	constexpr wchar_t EXT_TIFF[] = L".tiff";	// �摜�t�@�C���̊g���q
	constexpr uint32_t MRU_MAX = 25;	// �ŋߎg�������X�g�̍ő吔.
	constexpr wchar_t UNTITLED[] = L"str_untitled";	// ����̃��\�[�X��
	static const IVector<winrt::hstring> TYPE_BMP{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_BMP })
	};
	static const IVector<winrt::hstring> TYPE_GIF{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_GIF })
	};
	static const IVector<winrt::hstring> TYPE_JPG{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_JPG, EXT_JPEG })
	};
	static const IVector<winrt::hstring> TYPE_PNG{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_PNG })
	};
	static const IVector<winrt::hstring> TYPE_TIF{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_TIF, EXT_TIFF })
	};
	static const IVector<winrt::hstring> TYPE_GPF{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_GPF })
	};
	static const IVector<winrt::hstring> TYPE_SVG{
		winrt::single_threaded_vector<winrt::hstring>({ EXT_SVG })
	};
	static winrt::guid enc_id = BitmapEncoder::BmpEncoderId();

	// �f�[�^���C�^�[�� SVG �J�n�^�O����������.
	static void file_write_svg_tag(D2D1_SIZE_F const& size, D2D1_COLOR_F const& color, const double dpi, const LEN_UNIT unit, DataWriter const& dt_writer);

	// �f�[�^���C�^�[�� SVG �J�n�^�O����������.
	static void file_write_svg_tag(D2D1_SIZE_F const& size, D2D1_COLOR_F const& color, const double dpi, const LEN_UNIT unit, DataWriter const& dt_writer)
	{
		constexpr char SVG_TAG[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" ";
		constexpr char* SVG_UNIT_PX = "px";
		constexpr char* SVG_UNIT_IN = "in";
		constexpr char* SVG_UNIT_MM = "mm";
		constexpr char* SVG_UNIT_PT = "pt";

		// SVG �^�O�̊J�n����������.
		dt_write_svg(SVG_TAG, dt_writer);

		// �P�ʕt���ŕ��ƍ����̑�������������.
		char buf[256];
		double w, h;
		char* u;
		if (unit == LEN_UNIT::INCH) {
			w = size.width / dpi;
			h = size.height / dpi;
			u = SVG_UNIT_IN;
		}
		else if (unit == LEN_UNIT::MILLI) {
			w = size.width * MM_PER_INCH / dpi;
			h = size.height * MM_PER_INCH / dpi;
			u = SVG_UNIT_MM;
		}
		else if (unit == LEN_UNIT::POINT) {
			w = size.width * PT_PER_INCH / dpi;
			h = size.height * PT_PER_INCH / dpi;
			u = SVG_UNIT_PT;
		}
		else {
			w = size.width;
			h = size.height;
			u = SVG_UNIT_PX;
		}
		sprintf_s(buf, "width=\"%lf%s\" height=\"%lf%s\" ", w, u, h, u);
		dt_write_svg(buf, dt_writer);

		// �s�N�Z���P�ʂ̕��ƍ����� viewBox �����Ƃ��ď�������.
		dt_write_svg("viewBox=\"0 0 ", dt_writer);
		dt_write_svg(size.width, dt_writer);
		dt_write_svg(size.height, dt_writer);
		dt_write_svg("\" ", dt_writer);

		// �w�i�F���X�^�C�������Ƃ��ď�������.
		dt_write_svg("style=\"background-color:", dt_writer);
		dt_write_svg(color, dt_writer);

		// svg �J�n�^�O�̏I������������.
		dt_write_svg("\" >" SVG_NEW_LINE, dt_writer);
	}

	// �t�@�C���̓ǂݍ��݂��I������.
	void MainPage::file_finish_reading(void)
	{
		xcvd_is_enabled();

		drawing_tool_is_checked(m_drawing_tool);
		drawing_poly_opt_is_checked(m_drawing_poly_opt);
		misc_color_is_checked(m_color_code);
		status_bar_is_checked(m_status_bar);
		misc_len_is_checked(m_len_unit);
		image_keep_aspect_is_checked(m_image_keep_aspect);
		
		sheet_attr_is_checked();

		wchar_t* unavailable_font;	// ���̖�
		if (slist_test_font(m_main_sheet.m_shape_list, unavailable_font) != true) {
			// �u�����ȏ��̂��g�p����Ă��܂��v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_ALERT, ERR_FONT, unavailable_font);
		}

		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			if (m_main_sheet.m_shape_list.empty()) {
				summary_close_click(nullptr, nullptr);
			}
			else if (lv_summary_list() != nullptr) {
				summary_remake();
			}
			else {
				// ���\�[�X����}�`�̈ꗗ�p�l����������.
				auto _{ FindName(L"gd_summary_panel") };
				gd_summary_panel().Visibility(UI_VISIBLE);
			}
		}

		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
		status_set_curs();
		status_set_draw();
		status_set_grid();
		status_set_sheet();
		status_set_zoom();
		status_set_unit();
		status_bar_visibility();
	}

	// �t�@�C�����j���[�́u�摜���C���|�[�g����...�v���I�����ꂽ.
	IAsyncAction MainPage::file_import_img_click(IInspectable const&, RoutedEventArgs const&)
	{
		winrt::apartment_context context;
		// �t�@�C���u�I�[�v���v�s�b�J�[���擾���ĊJ��.
		FileOpenPicker open_picker{ FileOpenPicker() };
		open_picker.FileTypeFilter().Append(EXT_BMP);
		open_picker.FileTypeFilter().Append(EXT_GIF);
		open_picker.FileTypeFilter().Append(EXT_JPG);
		open_picker.FileTypeFilter().Append(EXT_JPEG);
		open_picker.FileTypeFilter().Append(EXT_PNG);
		open_picker.FileTypeFilter().Append(EXT_TIF);
		open_picker.FileTypeFilter().Append(EXT_TIFF);
		// �s�b�J�[��񓯊��ɕ\�����ăX�g���[�W�t�@�C�����擾����.
		// (�u����v�{�^���������ꂽ�ꍇ�X�g���[�W�t�@�C���� nullptr.)
		StorageFile open_file{ co_await open_picker.PickSingleFileAsync() };
		// �X�g���[�W�t�@�C�����k���|�C���^�[�����肷��.
		if (open_file != nullptr) {
			CoreCursor const& prev_cur = file_wait_cursor();
			unselect_all();

			const double win_w = scp_sheet_panel().ActualWidth();
			const double win_h = scp_sheet_panel().ActualHeight();
			const double win_x = sb_horz().Value();
			const double win_y = sb_vert().Value();
			co_await winrt::resume_background();

			IRandomAccessStream stream{ co_await open_file.OpenAsync(FileAccessMode::Read) };
			BitmapDecoder decoder{ co_await BitmapDecoder::CreateAsync(stream) };
			SoftwareBitmap bitmap{ SoftwareBitmap::Convert(co_await decoder.GetSoftwareBitmapAsync(), BitmapPixelFormat::Bgra8) };

			// �p���̕\�����ꂽ�����̒��S�̈ʒu�����߂�.
			const double scale = m_main_sheet.m_sheet_scale;
			const double img_w = bitmap.PixelWidth();
			const double img_h = bitmap.PixelHeight();
			const D2D1_POINT_2F center_pos{
				static_cast<FLOAT>((win_x + (win_w - img_w) * 0.5) / scale),
				static_cast<FLOAT>((win_y + (win_h - img_h) * 0.5) / scale)
			};
			const D2D1_SIZE_F view_size{ static_cast<FLOAT>(img_w), static_cast<FLOAT>(img_h) };
			ShapeImage* img = new ShapeImage(center_pos, view_size, bitmap, m_main_sheet.m_image_opac);
#if (_DEBUG)
			debug_leak_cnt++;
#endif
			bitmap.Close();
			bitmap = nullptr;
			decoder = nullptr;
			stream.Close();
			stream = nullptr;

			m_d2d_mutex.lock();
			ustack_push_append(img);
			ustack_push_select(img);
			ustack_push_null();
			m_d2d_mutex.unlock();
			co_await winrt::resume_foreground(Dispatcher());

			ustack_is_enable();
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_append(img);
				summary_select(img);
			}
			xcvd_is_enabled();
			sheet_update_bbox(img);
			sheet_panle_size();
			sheet_draw();

			// �J�[�\�������ɖ߂�.
			Window::Current().CoreWindow().PointerCursor(prev_cur);
		}

		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
	};

	// �t�@�C�����j���[�́u�J��...�v���I�����ꂽ
	IAsyncAction MainPage::file_open_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		// �X�^�b�N�ɑ���̑g���ς܂�Ă���, ���m�F�_�C�A���O�̉������u�L�����Z���v�����肷��.
		if (m_ustack_updt && !co_await ask_for_conf_async()) {
			co_return;
		}
		// �ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
		CoreCursor const& prev_cur = file_wait_cursor();
		// �t�@�C���u�I�[�v���v�s�b�J�[���擾���ĊJ��.
		FileOpenPicker open_picker{ FileOpenPicker() };
		open_picker.FileTypeFilter().Append(EXT_GPF);
		// �_�u���N���b�N�Ńt�@�C�����I�����ꂽ�ꍇ,
		// co_await ���I������O��, PonterReleased �� PonterEntered ���Ă΂��.
		// ����̓s�b�J�[�� 2 �x�ڂ� Released ��҂����Ƀ_�u���N���b�N�𐬗������Ă��邽�߂��Ǝv����.
		//scp_sheet_panel().PointerReleased(m_token_event_released);
		//scp_sheet_panel().PointerEntered(m_token_event_entered);

		// �s�b�J�[��񓯊��ŕ\�����ăX�g���[�W�t�@�C�����擾����.
		// (�u����v�{�^���������ꂽ�ꍇ�X�g���[�W�t�@�C���� nullptr.)
		StorageFile open_file{ co_await open_picker.PickSingleFileAsync() };
		// �X�g���[�W�t�@�C�����k���|�C���^�[�����肷��.
		if (open_file != nullptr) {
			// �X�g���[�W�t�@�C����񓯊��ɓǂ�.
			co_await file_read_async<false, false>(open_file);
			// �X�g���[�W�t�@�C�����������.
			open_file = nullptr;
		}
		open_picker = nullptr;
		Window::Current().CoreWindow().PointerCursor(prev_cur);
	}

	// �X�g���[�W�t�@�C����񓯊��ɓǂ�.
	// SUSPEND	���C�t�T�C�N�������f�̂Ƃ� true
	// SETTING	�u�ݒ��ۑ��v�̂Ƃ� true
	// s_file	�ǂݍ��ރX�g���[�W�t�@�C��
	// �߂�l	�ǂݍ��߂��� S_OK.
	template <bool SUSPEND, bool SETTING>
	IAsyncOperation<winrt::hresult> MainPage::file_read_async(StorageFile s_file) noexcept
	{
		HRESULT ok = E_FAIL;
		winrt::apartment_context context;
		m_d2d_mutex.lock();
		try {
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_close_click(nullptr, nullptr);
			}
			ustack_clear();
			slist_clear(m_main_sheet.m_shape_list);

			const auto& ra_stream{ co_await s_file.OpenAsync(FileAccessMode::Read) };
			auto dt_reader{ DataReader(ra_stream.GetInputStreamAt(0)) };
			co_await dt_reader.LoadAsync(static_cast<uint32_t>(ra_stream.Size()));

			//tool_read(dt_reader);
			m_drawing_tool = static_cast<DRAWING_TOOL>(dt_reader.ReadUInt32());
			dt_read(m_drawing_poly_opt, dt_reader);
			//find_text_read(dt_reader);
			dt_read(m_find_text, dt_reader);
			dt_read(m_find_repl, dt_reader);
			uint16_t f_bit = dt_reader.ReadUInt16();
			m_edit_text_frame = ((f_bit & 1) != 0);
			m_find_text_case = ((f_bit & 2) != 0);
			m_find_text_wrap = ((f_bit & 4) != 0);

			m_len_unit = static_cast<LEN_UNIT>(dt_reader.ReadUInt32());
			m_color_code = static_cast<COLOR_CODE>(dt_reader.ReadUInt16());
			m_vert_stick = dt_reader.ReadSingle();
			m_status_bar = static_cast<STATUS_BAR>(dt_reader.ReadUInt16());
			m_image_keep_aspect = dt_reader.ReadBoolean();	// �摜�̏c����̈ێ�

			const auto s_atom = dt_reader.ReadBoolean();
			m_summary_atomic.store(s_atom, std::memory_order_release);

			m_main_sheet.read(dt_reader);

			m_main_sheet.m_grid_base = max(m_main_sheet.m_grid_base, 0.0f);
			m_main_sheet.m_sheet_scale = min(max(m_main_sheet.m_sheet_scale, 0.25f), 4.0f);
			m_main_sheet.m_sheet_size.width = max(min(m_main_sheet.m_sheet_size.width, sheet_size_max()), 1.0F);
			m_main_sheet.m_sheet_size.height = max(min(m_main_sheet.m_sheet_size.height, sheet_size_max()), 1.0F);

#if defined(_DEBUG)
			if (debug_leak_cnt != 0) {
				co_await winrt::resume_foreground(Dispatcher());
				message_show(ICON_ALERT, DEBUG_MSG, {});
				co_await context;
				m_d2d_mutex.unlock();
				co_return ok;
			}
#endif
			// �V�[�g�̂ݓǂݍ��ނ����肷��.
			if constexpr (SETTING) {
				ok = S_OK;
			}
			else {
				if (slist_read(m_main_sheet.m_shape_list, dt_reader)) {
					// ���f���ꂽ�����肷��.
					if constexpr (SUSPEND) {
						ustack_read(dt_reader);
					}
					ok = S_OK;
				}
			}
			dt_reader.Close();
			ra_stream.Close();
		}
		catch (winrt::hresult_error const& e) {
			ok = e.code();
		}
		m_d2d_mutex.unlock();
		if (ok != S_OK) {
			co_await winrt::resume_foreground(Dispatcher());
			message_show(ICON_ALERT, ERR_READ, s_file.Path());
		}
		else {
			if constexpr (!SUSPEND && !SETTING) {
				co_await winrt::resume_foreground(Dispatcher());
				file_recent_add(s_file);
				file_finish_reading();
			}
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// ���ʂ�Ԃ��I������.
		co_return ok;
	}
	template IAsyncOperation<winrt::hresult> MainPage::file_read_async<false, false>(StorageFile s_file) noexcept;
	template IAsyncOperation<winrt::hresult> MainPage::file_read_async<false, true>(StorageFile s_file) noexcept;
	template IAsyncOperation<winrt::hresult> MainPage::file_read_async<true, false>(StorageFile s_file) noexcept;

	// �X�g���[�W�t�@�C�����ŋߎg�����t�@�C���ɓo�^����.
	// s_file	�X�g���[�W�t�@�C��
	// �߂�l	�Ȃ�
	// �ŋߎg�����t�@�C�����j���[�ƃE�B���h�E�^�C�g�����X�V�����.
	// �X�g���[�W�t�@�C�����k���̏ꍇ, �E�B���h�E�^�C�g���ɖ��肪�i�[�����.
	void MainPage::file_recent_add(StorageFile const& s_file)
	{
		if (s_file != nullptr) {
			m_file_token_mru = StorageApplicationPermissions::MostRecentlyUsedList().Add(s_file, s_file.Path());
			ApplicationView::GetForCurrentView().Title(s_file.Name());
		}
		else {
			ApplicationView::GetForCurrentView().Title(ResourceLoader::GetForCurrentView().GetString(UNTITLED));
		}
		file_recent_update_menu();
	}

	// �t�@�C�����j���[�́u�ŋߎg�����t�@�C���v�̃T�u���ڂ��I�����ꂽ.
	void MainPage::file_recent_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		uint32_t n;
		if (sender == mfi_file_recent_1()) {
			n = 0;
		}
		else if (sender == mfi_file_recent_2()) {
			n = 1;
		}
		else if (sender == mfi_file_recent_3()) {
			n = 2;
		}
		else if (sender == mfi_file_recent_4()) {
			n = 3;
		}
		else if (sender == mfi_file_recent_5()) {
			n = 4;
		}
		else {
			return;
		}
		auto _{ file_recent_read_async(n) };
	}

	// �ŋߎg�����t�@�C���̃g�[�N������X�g���[�W�t�@�C���𓾂�.
	// token	�ŋߎg�����t�@�C���̃g�[�N��
	// �߂�l	�X�g���[�W�t�@�C��
	IAsyncOperation<StorageFile> MainPage::file_recent_get_async(const winrt::hstring token)
	{
		// �R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;

		// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		//co_await winrt::resume_foreground(this->Dispatcher());
		// �X�g���[�W�t�@�C���Ƀk�����i�[����.
		StorageFile s_file = nullptr;
		try {
			// �g�[�N������ȊO�����肷��.
			if (!token.empty()) {
				// �g�[�N������X�g���[�W�t�@�C���𓾂�.
				auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
				if (mru_list.ContainsItem(token)) {
					s_file = co_await mru_list.GetFileAsync(token);
				}
			}
		}
		catch (winrt::hresult_error) {
		}
		// �擾�ł��Ă��o���Ȃ��Ă��ŋߎg�������X�g�̏��Ԃ͓���ւ��̂�,
		// �ŋߎg�����t�@�C�����j�����X�V����.
		file_recent_update_menu();
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// �X�g���[�W�t�@�C����Ԃ�.
		co_return s_file;
	}

	// �ŋߎg�����t�@�C����ǂݍ���.
	// i	�ŋߎg�����t�@�C���̔ԍ� (�ł����߂� 0).
	IAsyncAction MainPage::file_recent_read_async(const uint32_t i)
	{
		// SHCore.dll �X���b�h
		auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		auto const& mru_entries = mru_list.Entries();
		if (i >= mru_entries.Size()) {
			// �t�@�C���̔ԍ����ŋߎg�����t�@�C���̐��ȏ�̏ꍇ,
			message_show(ICON_ALERT, ERR_RECENT, to_hstring(i + 1));
			co_return;
		}
		// �ŋߎg�������X�g����v�f�𓾂�.
		AccessListEntry item[1];
		mru_entries.GetMany(i, item);

		// �X�^�b�N�ɑ���̑g���ς܂�Ă���, ���m�F�_�C�A���O�̉������u�L�����Z���v�����肷��.
		if (m_ustack_updt && !co_await ask_for_conf_async()) {
			co_return;
		}
		auto const prev_cur = file_wait_cursor();
		auto s_file{ co_await file_recent_get_async(item[0].Token) };
		// �X�g���[�W�t�@�C��������ꂽ�����肷��.
		if (s_file != nullptr) {
			co_await file_read_async<false, false>(s_file);
			s_file = nullptr;
		}
		else {
			// �擾�ł��Ȃ��Ȃ��,
			message_show(ICON_ALERT, ERR_RECENT, item[0].Metadata);
		}

		// �E�B���h�E�̃J�[�\���𕜌�����.
		Window::Current().CoreWindow().PointerCursor(prev_cur);
	}

	// �ŋߎg�����t�@�C�����j�����X�V����.
	void MainPage::file_recent_update_menu(void)
	{
		auto const& mru_entries = StorageApplicationPermissions::MostRecentlyUsedList().Entries();
		const auto ent_size = mru_entries.Size();
		AccessListEntry items[MRU_MAX];
		winrt::hstring data[MRU_MAX];
		mru_entries.GetMany(0, items);
		for (uint32_t i = MRU_MAX; i > 0; i--) {
			data[i - 1] = winrt::to_hstring(i) + L":";
			if (ent_size >= i) {
				data[i - 1] = data[i - 1] + L" " + items[i - 1].Metadata;
			}
		}
		mfi_file_recent_1().Text(data[0]);
		mfi_file_recent_2().Text(data[1]);
		mfi_file_recent_3().Text(data[2]);
		mfi_file_recent_4().Text(data[3]);
		mfi_file_recent_5().Text(data[4]);
	}

	// ���O��t���ăt�@�C���ɔ񓯊��ɕۑ�����.
	// svg_allowed	SVG �ւ̕ۑ�������.
	IAsyncOperation<winrt::hresult> MainPage::file_save_as_async(const bool svg_allowed) noexcept
	{
		HRESULT ok = E_FAIL;
		auto const& prev_cur = file_wait_cursor();	// �\������O�̃J�[�\��
		// �R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		//winrt::apartment_context context;
		//co_await winrt::resume_background();
		try {
			// �t�@�C���ۑ��s�b�J�[�𓾂�.
			// �t�@�C���^�C�v�Ɋg���q GPF �Ƃ��̐�����ǉ�����.
			FileSavePicker s_picker{
				FileSavePicker()
			};
			const winrt::hstring desc_gpf{
				ResourceLoader::GetForCurrentView().GetString(DESC_GPF)
			};
			s_picker.FileTypeChoices().Insert(desc_gpf, TYPE_GPF);
			const PickerLocationId loc_id = PickerLocationId::DocumentsLibrary;
			s_picker.SuggestedStartLocation(loc_id);
			// SVG �ւ̕ۑ������������肷��.
			if (svg_allowed) {
				// �t�@�C���^�C�v�Ɋg���q SVG �Ƃ��̐�����ǉ�����.
				const winrt::hstring desc_svg{
					ResourceLoader::GetForCurrentView().GetString(DESC_SVG)
				};
				s_picker.FileTypeChoices().Insert(desc_svg, TYPE_SVG);
			}
			// �ŋߎg�����t�@�C���̃g�[�N�����󂩔��肷��.
			if (m_file_token_mru.empty()) {
				// ��Ă��ꂽ�t�@�C�����Ɋg���q���i�[����.
				s_picker.SuggestedFileName(EXT_GPF);
			}
			else {
				// �X�g���[�W�t�@�C�����ŋߎg�����t�@�C���̃g�[�N�����瓾��.
				auto s_file{ co_await file_recent_get_async(m_file_token_mru) };
				if (s_file != nullptr) {
					// �t�@�C���^�C�v�� EXT_GPF �����肷��.
					if (s_file.FileType() == EXT_GPF) {
						// �t�@�C������, ��Ă���t�@�C�����Ɋi�[����.
						s_picker.SuggestedFileName(s_file.Name());
					}
					s_file = nullptr;
				}
			}
			// �t�@�C���ۑ��s�b�J�[��\����, �X�g���[�W�t�@�C���𓾂�.
			StorageFile s_file{
				co_await s_picker.PickSaveFileAsync()
			};
			// �X�g���[�W�t�@�C�����擾���������肷��.
			if (s_file != nullptr) {
				// �t�@�C���^�C�v�� SVG �����肷��.
				if (s_file.FileType() == EXT_SVG) {
					//for (const auto s : m_main_sheet.m_shape_list) {
					//	if (s->is_deleted() || typeid(*s) != typeid(ShapeImage)) {
					//		continue;
					//	}
					//	message_show(ICON_INFO, L"str_info_image_found", tx_find_text_what().Text());
					//	break;
					//}
					// �}�`�f�[�^�� SVG �Ƃ��ăX�g���[�W�t�@�C���ɔ񓯊��ɏ�������, ���ʂ𓾂�.
					ok = co_await file_write_svg_async(s_file);
				}
				else {
					// �}�`�f�[�^���X�g���[�W�t�@�C���ɔ񓯊��ɏ�������, ���ʂ𓾂�.
					ok = co_await file_write_gpf_async<false, false>(s_file);
				}
				// �X�g���[�W�t�@�C����j������.
				s_file = nullptr;
			}
			// �s�b�J�[��j������.
			s_picker = nullptr;
		}
		catch (winrt::hresult_error const& e) {
			// �G���[�����������ꍇ, �G���[�R�[�h�����ʂɊi�[����.
			ok = e.code();
		}
		// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		//co_await winrt::resume_foreground(Dispatcher());
		// �E�B���h�E�̃J�[�\���𕜌�����.
		Window::Current().CoreWindow().PointerCursor(prev_cur);
		// �X���b�h�R���e�L�X�g�𕜌�����.
		//co_await context;
		// ���ʂ�Ԃ��I������.
		co_return ok;
	}

	// �t�@�C�����j���[�́u���O��t���ĕۑ��v���I�����ꂽ
	void MainPage::file_save_as_click(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr bool SVG_ALLOWED = true;
		auto _{ file_save_as_async(SVG_ALLOWED) };
	}

	// �t�@�C���ɔ񓯊��ɕۑ�����
	IAsyncOperation<winrt::hresult> MainPage::file_save_async(void) noexcept
	{
		// �ŋߎg�����t�@�C���̃g�[�N������X�g���[�W�t�@�C���𓾂�.
		StorageFile s_file{
			co_await file_recent_get_async(m_file_token_mru)
		};
		if (s_file == nullptr) {
			// �X�g���[�W�t�@�C������̏ꍇ,
			constexpr bool SVG_ALLOWED = true;
			// ���O��t���ăt�@�C���ɔ񓯊��ɕۑ�����
			co_return co_await file_save_as_async(!SVG_ALLOWED);
		}

		HRESULT ok = E_FAIL;	// ����
		// �ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
		CoreCursor const& prev_cur = file_wait_cursor();
		// �}�`�f�[�^���X�g���[�W�t�@�C���ɔ񓯊��ɏ�������, ���ʂ𓾂�.
		ok = co_await file_write_gpf_async<false, false>(s_file);
		// ���ʂ� S_OK �ȊO�����肷��.
		if (ok != S_OK) {
			// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			// �ŋߎg�����t�@�C���̃G���[���b�Z�[�W�_�C�A���O��\������.
			co_await winrt::resume_foreground(Dispatcher());
			// �u�t�@�C���ɏ������߂܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_ALERT, ERR_WRITE, m_file_token_mru);
		}
		// �E�B���h�E�̃J�[�\���𕜌�����.
		Window::Current().CoreWindow().PointerCursor(prev_cur);
		// ���ʂ�Ԃ��I������.
		co_return ok;
	}

	// �t�@�C�����j���[�́u�㏑���ۑ��v���I�����ꂽ
	void MainPage::file_save_click(IInspectable const&, RoutedEventArgs const&)
	{
		auto _{ file_save_async() };
	}

	// �t�@�C���ɉ摜�}�`�̉摜��ۑ�����.
	// s	�摜�}�`
	// sug_name	�t�@�C���Z�[�u�s�b�J�[�ɓn�����t�@�C����
	// img_name	�t�@�C���Z�[�u�s�b�J�[�őI�����ꂽ�t�@�C����
	IAsyncOperation<winrt::hresult> MainPage::file_save_img_async(ShapeImage* s, const wchar_t sug_name[], wchar_t img_name[], const size_t name_len)
	{
		// �R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		FileSavePicker img_picker{
			FileSavePicker()
		};
		// ResourceLoader::GetForCurrentView �̓t�H�A�O���E���h.
		co_await winrt::resume_foreground(Dispatcher());
		const ResourceLoader& res_loader = ResourceLoader::GetForCurrentView();
		const winrt::hstring desc_bmp{
			res_loader.GetString(L"str_desc_bmp")
		};
		const winrt::hstring desc_gif{
			res_loader.GetString(L"str_desc_gif")
		};
		const winrt::hstring desc_jpg{
			res_loader.GetString(L"str_desc_jpg")
		};
		const winrt::hstring desc_png{
			res_loader.GetString(L"str_desc_png")
		};
		const winrt::hstring desc_tif{
			res_loader.GetString(L"str_desc_tif")
		};
		if (enc_id == BitmapEncoder::GifEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_gif, TYPE_GIF);
		}
		else if (enc_id == BitmapEncoder::JpegEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_jpg, TYPE_JPG);
		}
		else if (enc_id == BitmapEncoder::PngEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_png, TYPE_PNG);
		}
		else if (enc_id == BitmapEncoder::TiffEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_tif, TYPE_TIF);
		}
		else {
			img_picker.FileTypeChoices().Insert(desc_bmp, TYPE_BMP);
			enc_id = BitmapEncoder::BmpEncoderId();
		}
		if (enc_id != BitmapEncoder::BmpEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_bmp, TYPE_BMP);
		}
		if (enc_id != BitmapEncoder::GifEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_gif, TYPE_GIF);
		}
		if (enc_id != BitmapEncoder::JpegEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_jpg, TYPE_JPG);
		}
		if (enc_id != BitmapEncoder::PngEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_png, TYPE_PNG);
		}
		if (enc_id != BitmapEncoder::TiffEncoderId()) {
			img_picker.FileTypeChoices().Insert(desc_tif, TYPE_TIF);
		}
		img_picker.SuggestedFileName(sug_name);
		StorageFile img_file{
			co_await img_picker.PickSaveFileAsync()
		};
		if (img_file == nullptr) {
			co_await context;
			co_return E_FAIL;
		}
		// �ۑ�����t�@�C���`��
		if (std::find(TYPE_BMP.begin(), TYPE_BMP.end(), img_file.FileType()) != TYPE_BMP.end()) {
			enc_id = BitmapEncoder::BmpEncoderId();
		}
		else if (std::find(TYPE_GIF.begin(), TYPE_GIF.end(), img_file.FileType()) != TYPE_GIF.end()) {
			enc_id = BitmapEncoder::GifEncoderId();
		}
		else if (std::find(TYPE_JPG.begin(), TYPE_JPG.end(), img_file.FileType()) != TYPE_JPG.end()) {
			enc_id = BitmapEncoder::JpegEncoderId();
		}
		else if (std::find(TYPE_PNG.begin(), TYPE_PNG.end(), img_file.FileType()) != TYPE_PNG.end()) {
			enc_id = BitmapEncoder::PngEncoderId();
		}
		else if (std::find(TYPE_TIF.begin(), TYPE_TIF.end(), img_file.FileType()) != TYPE_TIF.end()) {
			enc_id = BitmapEncoder::TiffEncoderId();
		}
		else {
			co_await context;
			co_return E_FAIL;
		}
		HRESULT ok = E_FAIL;
		co_await winrt::resume_background();
		CachedFileManager::DeferUpdates(img_file);
		IRandomAccessStream img_stream{
			co_await img_file.OpenAsync(FileAccessMode::ReadWrite)
		};
		co_await s->copy_to(enc_id, img_stream);
		// �x���������t�@�C���X�V��������, ���ʂ𔻒肷��.
		if (co_await CachedFileManager::CompleteUpdatesAsync(img_file) == FileUpdateStatus::Complete) {
			ok = S_OK;
			wcscpy_s(img_name, name_len, img_file.Path().c_str());
		}
		img_stream.Close();
		img_stream = nullptr;
		img_file = nullptr;
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// ���ʂ�Ԃ��I������.
		co_return ok;
	}

	// �ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
	CoreCursor MainPage::file_wait_cursor(void) const
	{
		CoreWindow const& c_win = Window::Current().CoreWindow();
		CoreCursor prev_cur = c_win.PointerCursor();
		c_win.PointerCursor(CUR_WAIT);
		return prev_cur;
	}

	// �}�`�f�[�^���X�g���[�W�t�@�C���ɔ񓯊��ɏ�������.
	// SUSPEND	���C�t�T�C�N�������f�̂Ƃ� true
	// SETTING	�u�ݒ��ۑ��v�̂Ƃ� true
	// s_file	�X�g���[�W�t�@�C��
	// �߂�l	�������݂ɐ��������� true
	template <bool SUSPEND, bool SETTING>
	IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async(StorageFile s_file)
	{
		constexpr auto REDUCE = true;

		// E_FAIL �����ʂɊi�[����.
		HRESULT ok = E_FAIL;
		// �R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ���
		winrt::apartment_context context;
		// �X���b�h���o�b�N�O���E���h�ɕς���.
		co_await winrt::resume_background();
		try {
			// �t�@�C���X�V�̒x����ݒ肷��.
			// �X�g���[�W�t�@�C��->�����_���A�N�Z�X�X�g���[��->�f�[�^���C�^�[���쐬����.
			CachedFileManager::DeferUpdates(s_file);
			const IRandomAccessStream& ra_stream{
				co_await s_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			DataWriter dt_writer{
				DataWriter(ra_stream.GetOutputStreamAt(0))
			};
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_drawing_tool));
			dt_write(m_drawing_poly_opt, dt_writer);
			dt_write(m_find_text, dt_writer);
			dt_write(m_find_repl, dt_writer);
			uint16_t f_bit = 0;
			if (m_edit_text_frame) {
				f_bit |= 1;
			}
			if (m_find_text_case) {
				f_bit |= 2;
			}
			if (m_find_text_wrap) {
				f_bit |= 4;
			}
			dt_writer.WriteUInt16(f_bit);
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_len_unit));
			dt_writer.WriteUInt16(static_cast<uint16_t>(m_color_code));
			dt_writer.WriteSingle(m_vert_stick);
			dt_writer.WriteUInt16(static_cast<uint16_t>(m_status_bar));
			dt_writer.WriteBoolean(m_image_keep_aspect);

			dt_writer.WriteBoolean(summary_is_visible());

			m_main_sheet.write(dt_writer);
			if constexpr (SUSPEND) {
				// �������ꂽ�}�`���܂߂ď�������.
				slist_write<!REDUCE>(m_main_sheet.m_shape_list, dt_writer);
				ustack_write(dt_writer);
			}
			else if constexpr (!SETTING) {
				// �������ꂽ�}�`�͏Ȃ��ď�������.
				slist_write<REDUCE>(m_main_sheet.m_shape_list, dt_writer);
			}
			ra_stream.Size(ra_stream.Position());
			co_await dt_writer.StoreAsync();
			co_await ra_stream.FlushAsync();
			// �f�[�^���C�^�[�����.
			dt_writer.Close();
			dt_writer = nullptr;
			// �X�g���[�������.
			ra_stream.Close();
			// �x���������t�@�C���X�V����������.
			if (co_await CachedFileManager::CompleteUpdatesAsync(s_file) == FileUpdateStatus::Complete) {
				// ���������ꍇ, 
				// S_OK �����ʂɊi�[����.
				ok = S_OK;
			}
		}
		// �G���[�����������ꍇ, 
		catch (winrt::hresult_error const& err) {
			ok = err.code();
		}
		// ���ʂ� S_OK �ȊO�����肷��.
		if (ok != S_OK) {
			// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			co_await winrt::resume_foreground(Dispatcher());
			// �u�t�@�C���ɏ������߂܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_ALERT, ERR_WRITE, s_file.Path());
		}
		else {
			// ���f�ł͂Ȃ�, ���ݒ�ł͂Ȃ������肷��.
			if constexpr (!SUSPEND && !SETTING) {
				// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
				co_await winrt::resume_foreground(Dispatcher());
				// �X�g���[�W�t�@�C�����ŋߎg�����t�@�C���ɓo�^����.
				// �����ŃG���[���o��.
				file_recent_add(s_file);
				// false ���X�^�b�N���X�V���ꂽ������Ɋi�[����.
				m_ustack_updt = false;
			}
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// ���ʂ�Ԃ��I������.
		co_return ok;
	}
	template IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async<false, false>(StorageFile s_file);
	template IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async<true, false>(StorageFile s_file);
	template IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async<false, true>(StorageFile s_file);

	// �}�`�f�[�^�� SVG �Ƃ��ăX�g���[�W�t�@�C���ɔ񓯊��ɏ�������.
	// file	�������ݐ�̃t�@�C��
	// �߂�l	�������߂��ꍇ S_OK
	IAsyncOperation<winrt::hresult> MainPage::file_write_svg_async(StorageFile s_file)
	{
		constexpr char XML_DEC[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" SVG_NEW_LINE;
		constexpr char DOCTYPE[] = "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" SVG_NEW_LINE;
		HRESULT ok = E_FAIL;
		// �R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		// �X���b�h���o�b�N�O���E���h�ɕς���.
		co_await winrt::resume_background();
		try {
			// �t�@�C���X�V�̒x����ݒ肷��.
			CachedFileManager::DeferUpdates(s_file);
			// �X�g���[�W�t�@�C�����J���ă����_���A�N�Z�X�X�g���[���𓾂�.
			const IRandomAccessStream& ra_stream{
				co_await s_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			// �����_���A�N�Z�X�X�g���[���̐擪����f�[�^���C�^�[���쐬����.
			DataWriter dt_writer{
				DataWriter(ra_stream.GetOutputStreamAt(0))
			};
			// XML �錾����������.
			dt_write_svg(XML_DEC, dt_writer);
			// DOCTYPE ����������.
			dt_write_svg(DOCTYPE, dt_writer);
			// �f�[�^���C�^�[�� SVG �J�n�^�O����������.
			file_write_svg_tag(m_main_sheet.m_sheet_size, m_main_sheet.m_sheet_color, m_main_d2d.m_logical_dpi, m_len_unit, dt_writer);
			// ���� (����) ����������.
			//char buf[64];
			//const auto t = time(nullptr);
			//struct tm tm;
			//localtime_s(&tm, &t);
			//strftime(buf, 64, "<!-- %m/%d/%Y %H:%M:%S -->" SVG_NEW_LINE, &tm);
			//dt_write_svg(buf, dt_writer);
			// �}�`���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
			for (auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				// �}�`���摜�����肷��.
				if (typeid(*s) == typeid(ShapeImage)) {
					static uint32_t magic_num = 0;	// �~���b�̑���
					constexpr size_t MAX_LEN = 1024 - 8;
					wchar_t img_name[MAX_LEN + 8];
					const time_t t = time(nullptr);
					struct tm tm;
					localtime_s(&tm, &t);

					swprintf(img_name, MAX_LEN, L"%s", s_file.Name().data());
					const wchar_t* const dot_ptr = wcsrchr(img_name, L'.');
					const size_t dot_len = (dot_ptr != nullptr ? dot_ptr - img_name : wcslen(img_name));
					const size_t tail_len = dot_len + wcsftime(img_name + dot_len, MAX_LEN - dot_len, L"%Y%m%d%H%M%S", &tm);
					swprintf(img_name + tail_len, 8, L"%03d", magic_num++);
					//const auto s_folder{ co_await s_file.GetParentAsync() };
					ShapeImage* const img = static_cast<ShapeImage*>(s);
					if (co_await file_save_img_async(img, img_name, img_name, MAX_LEN) == S_OK) {
						img->write_svg(img_name, dt_writer);
					}
					else {
						img->write_svg(dt_writer);
					}
				}
				else {
					s->write_svg(dt_writer);
				}
			}
			// SVG �I���^�O����������.
			dt_write_svg("</svg>" SVG_NEW_LINE, dt_writer);
			// �X�g���[���̌��݈ʒu���X�g���[���̑傫���Ɋi�[����.
			ra_stream.Size(ra_stream.Position());
			// �o�b�t�@���̃f�[�^���X�g���[���ɏo�͂���.
			co_await dt_writer.StoreAsync();
			// �X�g���[�����t���b�V������.
			co_await ra_stream.FlushAsync();
			// �x���������t�@�C���X�V��������, ���ʂ𔻒肷��.
			if (co_await CachedFileManager::CompleteUpdatesAsync(s_file) == FileUpdateStatus::Complete) {
				// ���������ꍇ, S_OK �����ʂɊi�[����.
				ok = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// �G���[�����������ꍇ, �G���[�R�[�h�����ʂɊi�[����.
			ok = e.code();
		}
		// ���ʂ� S_OK �ȊO�����肷��.
		if (ok != S_OK) {
			// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			co_await winrt::resume_foreground(Dispatcher());
			// �u�t�@�C���ɏ������߂܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_ALERT, ERR_WRITE, s_file.Path());
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// ���ʂ�Ԃ��I������.
		co_return ok;
	}

	// �t�@�C���V�X�e���ւ̃A�N�Z�X�����m�F����, �ݒ�𑣂�.
	/*
	IAsyncAction MainPage::file_check_broad_access(void) const
	{
		bool err = false;
		try {
			co_await StorageFolder::GetFolderFromPathAsync(L"C:\\");
		}
		catch (winrt::hresult_error const&) {
			err = true;
		}
		if (err) {
			Uri ms_setting{ L"ms-settings:privacy-broadfilesystemaccess" };
			co_await Launcher::LaunchUriAsync(ms_setting);
		}
		co_return;
	}
	*/

}