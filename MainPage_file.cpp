//-------------------------------
// MainPage_file.cpp
// �t�@�C���̓ǂݏ���, �A�v���̏I��
//-------------------------------
#include "pch.h"
#include "MainPage.h"

//	file_open_async
//		file_wait_cursor
//		PickSingleFileAsync
//		file_read_async
//			winrt::resume_background
//			OpenAsync
//			winrt::resume_foreground
//			mru_add_file
//				mru_update_menu_items
//			finish_file_read
//	file_read_recent_async
//		file_wait_cursor
//		mru_get_file
//			GetFileAsync
//			winrt::resume_foreground
//		file_read_async
//			winrt::resume_background
//			OpenAsync
//			winrt::resume_foreground
//			mru_add_file
//				mru_update_menu_items
//			finish_file_read
//		winrt::resume_foreground
//	file_save_as_async
//		file_wait_cursor
//		mru_get_file
//			GetFileAsync
//			winrt::resume_foreground
//		PickSaveFileAsync
//		file_write_svg_async
//			winrt::resume_background
//			OpenAsync
//			winrt::resume_foreground
//		file_write_gpf_async
//			winrt::resume_background
//			OpenAsync
//			winrt::resume_foreground
//			mru_add_file
//				mru_update_menu_items
//	file_save_async
//		mru_get_file
//			GetFileAsync
//			winrt::resume_foreground
//		file_save_as_async
//		file_wait_cursor
//		file_write_gpf_async
//			winrt::resume_background
//			winrt::resume_foreground
//			mru_add_file
//				mru_update_menu_items
//	mfi_exit_click
//		file_save_and_exit_async
//			file_save_async
//			Exit
//		Exit
//	mfi_new_click
//		file_save_and_new_async
//			file_save_async
//			new_doc
//		new_doc
//	mfi_open_click
//		file_save_and_open_async
//			file_save_async
//			file_open_async
//		file_open_async
//	mfi_recent_files_N_click
//		file_read_recent
//			mru_get_token
//			file_save_and_read_recent_async
//				file_save_async
//				file_read_recent_async
//			file_read_recent_async
//	mfi_save_as_click
//		file_save_as_async
//	mfi_save_click
//		file_save_async
/*
�������A4��Windows�����^�C���񓯊�����^�C�v�iIAsyncXxx�j�̂����ꂩ��
co_await�����ꍇ�AC ++ / WinRT��co_await�̎��_�ŌĂяo���R���e�L�X�g���L���v�`�����܂��B
�܂��A�p�����ĊJ���ꂽ�Ƃ��ɁA���̃R���e�L�X�g�ɂ��邱�Ƃ��ۏ؂���܂��B
C ++ / WinRT�́A�Ăяo�����̃R���e�L�X�g�Ɋ��ɂ��邩�ǂ������m�F���A
�����łȂ��ꍇ�͐؂�ւ��܂��B
*/

// GetFileFromPathAsync �� E_ACCESSDENIED �Ȃ��Ɏg���ɂ͈ȉ����K�v�ɂȂ�.
// 1. XAML�e�L�X�g�G�f�B�^���g����, Pakage.appxmanifest �� broadFileSystemAccess ��ǉ�����.
//	Pakage.appxmanifest �� <Package> �̃v���p�e�B�[��
//	xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities" ��ǉ�����
//	IgnorableNamespaces="uap mp" �� IgnorableNamespaces="uap mp rescap" �ɕύX����
// 2. �����Ȃ���� <Capabilities> �^�O�� <Package> �̎q�v�f�Ƃ��Ēǉ�����.
//	<Capabilities> �̎q�v�f�Ƃ��� <rescap:Capability Name="broadFileSystemAccess" /> ��������
// 3. �R���p�C�����邽�т�, �ݒ���g���ăA�v���Ƀt�@�C���A�N�Z�X��������.
//	�X�^�[�g���j���[ > �ݒ� > �v���C�o�[ > �t�@�C�� �V�X�e�� > �t�@�C���V�X�e���ɃA�N�Z�X�ł���A�v����I��
//	�\������Ă���A�v�����I���ɂ���.
// �܂���,
//	�X�^�[�g���j���[ > �ݒ� > �A�v�� > �A�v���Ƌ@�\
//	�\������Ă���A�v�����N���b�N����.
//	�ڍ׃I�v�V���� > �A�v���̃A�N�Z�X����
//	�t�@�C�� �V�X�e�����I���ɂ���.

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Core::CoreCursorType;

	static auto const& CUR_WAIT = CoreCursor(CoreCursorType::Wait, 0);	// �ҋ@�J�[�\��.
	constexpr wchar_t DESC_GPF[] = L"str_desc_gpf";	// �g���q gpf �̐���
	constexpr wchar_t DESC_SVG[] = L"str_desc_svg";	// �g���q svg �̐���
	constexpr wchar_t ERR_FONT[] = L"str_unavailable_font";	// �L���łȂ����̂̃G���[���b�Z�[�W�̃��\�[�X��
	constexpr wchar_t ERR_READ[] = L"str_err_read";	// �ǂݍ��݃G���[���b�Z�[�W�̃��\�[�X��
	constexpr wchar_t ERR_WRITE[] = L"str_err_write";	// �������݃G���[���b�Z�[�W�̃��\�[�X��
	constexpr wchar_t FT_GPF[] = L".gpf";	// �}�`�f�[�^�t�@�C���̊g���q
	constexpr wchar_t FT_SVG[] = L".svg";	// SVG �t�@�C���̊g���q

	//	�ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
	CoreCursor MainPage::file_wait_cursor(void) const
	{
		auto const& c_win = Window::Current().CoreWindow();
		auto p_cur = c_win.PointerCursor();
		c_win.PointerCursor(CUR_WAIT);
		return p_cur;
	}

	// �t�@�C����񓯊��ɊJ��
	IAsyncAction MainPage::file_open_async(void)
	{
		using winrt::Windows::Storage::Pickers::FileOpenPicker;

		//	�ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
		auto const& p_cur = file_wait_cursor();
		//	�R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		//	�t�@�C���u�I�[�v���v�s�b�J�[���擾���ĊJ��.
		auto o_picker{ FileOpenPicker() };
		//o_picker.ViewMode(PickerViewMode::Thumbnail);
		//	�t�@�C���^�C�v���u.gdf�v�ɐݒ肷��.
		o_picker.FileTypeFilter().Append(FT_GPF);
		//	�s�b�J�[��񓯊��ŕ\�����ăX�g���[�W�t�@�C�����擾����.
		//	�i�u����v�{�^���������ꂽ�ꍇ�X�g���[�W�t�@�C���Ƃ��� nullptr ���Ԃ�.�j
		auto s_file{ co_await o_picker.PickSingleFileAsync() };
		if (s_file != nullptr) {
			//	�t�@�C����nullptr�̏ꍇ�A
			//	�t�@�C����񓯊��œǂ�.
			co_await file_read_async(s_file);
			//	�X�g���[�W�t�@�C�����������.
			s_file = nullptr;
		}
		//	�s�b�J�[���������.
		o_picker = nullptr;
		//	�X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		//	�E�B���h�E�J�[�\���𕜌�����.
		Window::Current().CoreWindow().PointerCursor(p_cur);
	}

	//	�X�g���[�W�t�@�C����񓯊��ɓǂ�.
	//	s_file	�ǂݍ��ރX�g���[�W�t�@�C��
	//	suspend	���f�t���O
	//	�߂�l	�ǂݍ��߂��� S_OK.
	//	���f�t���O�������Ă���ꍇ, ����X�^�b�N���ۑ�����.
	IAsyncOperation<winrt::hresult> MainPage::file_read_async(StorageFile const& s_file, const bool suspend) noexcept
	{
		using winrt::Windows::Storage::FileAccessMode;

		auto hr = E_FAIL;
		//	�R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		//	�X���b�h���o�b�N�O���E���h�ɕύX����.
		winrt::apartment_context context;
		co_await winrt::resume_background();
		try {
			//	�t�@�C�����J���ă����_���A�N�Z�X�X�g���[���𓾂�.
			//	�X�g���[������ǂݍ��ނ��߂̃f�[�^���[�_�[���쐬����.
			//	�f�[�^���[�_�[�Ƀt�@�C����ǂݍ���.
			auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::Read) };
			auto dt_reader{ DataReader(ra_stream.GetInputStreamAt(0)) };
			co_await dt_reader.LoadAsync(static_cast<uint32_t>(ra_stream.Size()));

			text_find_read(dt_reader);
			stat_read(dt_reader);
			m_page_panel.read(dt_reader);
			//	����X�^�b�N����������.
			undo_clear();
			s_list_clear(m_list_shapes);
#if defined(_DEBUG)
			if (debug_leak_cnt != 0) {
				//	���C���y�[�W�� UI �X���b�h�ɃX���b�h��ύX����.
				co_await winrt::resume_foreground(this->Dispatcher());
				cd_message_show(L"Memory leak occurs", {});
				//	�X���b�h�R���e�L�X�g�𕜌�����.
				co_await context;
				//	���ʂ�Ԃ��I������.
				co_return hr;
			}
#endif
			s_list_read(m_list_shapes, dt_reader);
			if (suspend) {
				//	���f�t���O�������Ă���ꍇ,
				//	�f�[�^���[�_�[���瑀��X�^�b�N��ǂݍ���.
				undo_read(dt_reader);
			}
			//	�f�[�^���[�_�[�����.
			dt_reader.Close();
			//	�X�g���[�������.
			ra_stream.Close();
			hr = S_OK;
		}
		catch (winrt::hresult_error const& e) {
			//	�G���[�����������ꍇ, 
			//	�G���[�R�[�h�����ʂɊi�[����.
			hr = e.code();
		}
		if (hr != S_OK) {
			//	���ʂ� S_OK �łȂ��ꍇ,
			//	�X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			//	�ǂݍ��ݎ��s�̃��b�Z�[�W�_�C�A���O��\������.
			co_await winrt::resume_foreground(this->Dispatcher());
			cd_message_show(ERR_READ, s_file.Path());
		}
		else if (suspend == false) {
			//	���f�t���O���Ȃ��ꍇ,
			//	�X���b�h�����C���y�[�W�� UI �X���b�h�ɕύX����.
			co_await winrt::resume_foreground(this->Dispatcher());
			//	�X�g���[�W�t�@�C�����ŋߎg�����t�@�C���ɒǉ�����.
			mru_add_file(s_file);
			finish_file_read();
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// ���ʂ�Ԃ��I������.
		co_return hr;
	}

	//	�t�@�C���ɔ񓯊��ɕۑ�����, �A�v���P�[�V�������I������.
	IAsyncAction MainPage::file_save_and_exit_async(void)
	{
		using winrt::Windows::UI::Xaml::Application;

		//	�t�@�C���ɔ񓯊��ɕۑ�����
		if (co_await file_save_async() == S_OK) {
			//	�ۑ��ł����ꍇ,
			//	�A�v���P�[�V�������I������.
			release();
			Application::Current().Exit();
		}
	}

	//	�t�@�C���ɔ񓯊��ɕۑ�����, �󔒂̕����ɂ���.
	IAsyncAction MainPage::file_save_and_new_async(void)
	{
		//	�t�@�C���ɔ񓯊��ɕۑ�����
		if (co_await file_save_async() == S_OK) {
			//	�ۑ��ł����ꍇ,
			//	�󔒂̕����ɂ���.
			new_doc();
		}
	}

	//	�t�@�C���ɔ񓯊��ɕۑ�����, �t�@�C����񓯊��ɊJ��.
	IAsyncAction MainPage::file_save_and_open_async(void)
	{
		//	�t�@�C���ɔ񓯊��ɕۑ�����
		if (co_await file_save_async() == S_OK) {
			//	�ۑ��ł����ꍇ,
			//	�t�@�C����񓯊��ɊJ��.
			co_await file_open_async();
		}
	}

	//	���O��t���ăt�@�C���ɔ񓯊��ɕۑ�����.
	//	permit_svg	SVG ���t���O.
	IAsyncOperation<winrt::hresult> MainPage::file_save_as_async(const bool permit_svg) noexcept
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::Storage::Pickers::FileSavePicker;

		auto hr = E_FAIL;
		//	�ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
		auto const& p_cur = file_wait_cursor();
		//	�R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		try {
			// �t�@�C���ۑ��s�b�J�[�𓾂�.
			// �t�@�C���^�C�v�Ɋg���q GPF �Ƃ��̐�����ǉ�����.
			auto s_picker{ FileSavePicker() };
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			auto desc_gpf = r_loader.GetString(DESC_GPF);
			auto type_gpf = winrt::single_threaded_vector<winrt::hstring>({ FT_GPF });
			s_picker.FileTypeChoices().Insert(desc_gpf, type_gpf);
			//	SVG ���t���O�������Ă��邩���肷��.
			if (permit_svg) {
				//	�t���O�������Ă���ꍇ, 
				//	�t�@�C���^�C�v�Ɋg���q SVG �Ƃ��̐�����ǉ�����.
				auto desc_svg = r_loader.GetString(DESC_SVG);
				auto type_svg = winrt::single_threaded_vector<winrt::hstring>({ FT_SVG });
				s_picker.FileTypeChoices().Insert(desc_svg, type_svg);
			}
			//	�ŋߎg�����t�@�C���̃g�[�N�����󂩔��肷��.
			if (m_mru_token.empty()) {
				//	�g�[�N������̏ꍇ, 
				//	��Ă��ꂽ�t�@�C�����Ɋg���q���i�[����.
				s_picker.SuggestedFileName(FT_GPF);
			}
			else {
				//	�g�[�N������łȂ��ꍇ,
				//	�X�g���[�W�t�@�C�����ŋߎg�����t�@�C���̃g�[�N�����瓾��.
				//	����ꂽ�t�@�C���̖��O��, ��Ă��ꂽ�t�@�C�����Ɋi�[����.
				auto s_file{ co_await mru_get_file(m_mru_token) };
				if (s_file != nullptr) {
					if (s_file.FileType() == FT_GPF) {
						s_picker.SuggestedFileName(s_file.Name());
					}
					s_file = nullptr;
				}
			}
			//	�t�@�C���ۑ��s�b�J�[��\����, �X�g���[�W�t�@�C���𓾂�.
			auto s_file{ co_await s_picker.PickSaveFileAsync() };
			//	�X�g���[�W�t�@�C�����擾���������肷��.
			if (s_file != nullptr) {
				//	�擾�����ꍇ,
				if (s_file.FileType() == FT_SVG) {
					//	�t�@�C���^�C�v�� SVG �̏ꍇ,
					//	SVG �Ƃ��ăX�g���[�W�t�@�C���ɔ񓯊��ɏ�������.
					hr = co_await file_write_svg_async(s_file);
				}
				else {
					//	�t�@�C���^�C�v�� SVG �ȊO�̏ꍇ,
					//	�}�`�f�[�^���X�g���[�W�t�@�C���ɔ񓯊��ɏ�������.
					hr = co_await file_write_gpf_async(s_file);
				}
				//	�X�g���[�W�t�@�C����j������.
				s_file = nullptr;
			}
			//	�s�b�J�[��j������.
			s_picker = nullptr;
		}
		catch (winrt::hresult_error const& e) {
			//	�G���[�����������ꍇ, �G���[�R�[�h�����ʂɊi�[����.
			hr = e.code();
		}
		//	�E�B���h�E�̃J�[�\���𕜌�����.
		co_await winrt::resume_foreground(this->Dispatcher());
		Window::Current().CoreWindow().PointerCursor(p_cur);
		//	�X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		//	���ʂ�Ԃ��I������.
		co_return hr;
	}

	// �t�@�C���ɔ񓯊��ɕۑ�����
	IAsyncOperation<winrt::hresult> MainPage::file_save_async(void) noexcept
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		// �ŋߎg�����t�@�C���̃g�[�N������X�g���[�W�t�@�C���𓾂�.
		auto s_file{ co_await mru_get_file(m_mru_token) };
		if (s_file == nullptr) {
			// �X�g���[�W�t�@�C������̏ꍇ,
			// ���O��t���ăt�@�C���ɔ񓯊��ɕۑ�����.
			co_return co_await file_save_as_async(false);
		}

		auto hr = E_FAIL;
		//	�ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
		auto const& p_cur = file_wait_cursor();
		//	�R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		// �X���b�h���o�b�N�O���E���h�ɕς���.
		//co_await winrt::resume_background();
		// �}�`�f�[�^���X�g���[�W�t�@�C���ɔ񓯊��ɏ�������, ���ʂ𓾂�.
		hr = co_await file_write_gpf_async(s_file);
		// ���ʂ𔻒肷��.
		if (hr != S_OK) {
			// ���ʂ� S_OK �łȂ��ꍇ,
			// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			// �ŋߎg�����t�@�C���̃G���[���b�Z�[�W�_�C�A���O��\������.
			co_await winrt::resume_foreground(this->Dispatcher());
			cd_message_show(ERR_WRITE, m_mru_token);
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// �E�B���h�E�̃J�[�\���𕜌�����.
		Window::Current().CoreWindow().PointerCursor(p_cur);
		// ���ʂ�Ԃ��I������.
		co_return hr;
	}

	//	�}�`�f�[�^���X�g���[�W�t�@�C���ɔ񓯊��ɏ�������.
	//	s_file	�X�g���[�W�t�@�C��
	//	suspend	���f�t���O
	//	�߂�l	�������݂ɐ��������� true
	IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async(StorageFile const& s_file, const bool suspend)
	{
		using winrt::Windows::Storage::CachedFileManager;
		using winrt::Windows::Storage::FileAccessMode;
		using winrt::Windows::Storage::Provider::FileUpdateStatus;
		constexpr auto REDUCE = true;

		//	E_FAIL �����ʂɊi�[����.
		auto hr = E_FAIL;
		//	�R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		// �X���b�h���o�b�N�O���E���h�ɕς���.
		co_await winrt::resume_background();
		try {
			// �t�@�C���X�V�̒x����ݒ肷��.
			// �X�g���[�W�t�@�C�����J���ă����_���A�N�Z�X�X�g���[���𓾂�.
			// �����_���A�N�Z�X�X�g���[������f�[�^���C�^�[���쐬����.
			CachedFileManager::DeferUpdates(s_file);
			auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::ReadWrite) };
			auto dt_writer{ DataWriter(ra_stream.GetOutputStreamAt(0)) };
			//dt_writer.WriteUInt32(static_cast<uint32_t>(m_col_style));
			text_find_write(dt_writer);
			stat_write(dt_writer);
			m_page_panel.write(dt_writer);
			if (suspend) {
				s_list_write<!REDUCE>(m_list_shapes, dt_writer);
				undo_write(dt_writer);
			}
			else {
				s_list_write<REDUCE>(m_list_shapes, dt_writer);
			}
			ra_stream.Size(ra_stream.Position());
			co_await dt_writer.StoreAsync();
			co_await ra_stream.FlushAsync();
			//	�f�[�^���C�^�[�����.
			dt_writer.Close();
			//	�X�g���[�������.
			ra_stream.Close();
			//	�x���������t�@�C���X�V����������.
			auto fu_status{ co_await CachedFileManager::CompleteUpdatesAsync(s_file) };
			if (fu_status == FileUpdateStatus::Complete) {
				//	���������ꍇ, 
				//	S_OK �����ʂɊi�[����.
				hr = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			//	�G���[�����������ꍇ, 
			//	�G���[�R�[�h�����ʂɊi�[����.
			hr = e.code();
		}
		if (hr != S_OK) {
			//	���ʂ� S_OK �łȂ��ꍇ,
			//	�X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			//	�������ݎ��s�̃��b�Z�[�W�_�C�A���O��\������.
			co_await winrt::resume_foreground(this->Dispatcher());
			cd_message_show(ERR_WRITE, s_file.Path());
		}
		else if (suspend == false) {
			//	���f�t���O���Ȃ��ꍇ,
			//	�X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			co_await winrt::resume_foreground(this->Dispatcher());
			mru_add_file(s_file);
			//	false �𑀍�X�^�b�N�̍X�V�t���O�Ɋi�[����.
			m_stack_push = false;
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// ���ʂ�Ԃ��I������.
		co_return hr;
	}

	//	SVG �Ƃ��ăf�[�^���C�^�[�ɏ�������.
	//	dt_writer	�f�[�^���C�^�[
	void MainPage::file_write_svg(DataWriter const& dt_writer)
	{
		constexpr char XML_DEC[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" SVG_NL;
		constexpr char DOCTYPE[] = "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" SVG_NL;
		constexpr char SVG_TAG[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" ";

		//	XML �錾����������.
		write_svg(XML_DEC, dt_writer);
		//	DOCTYPE ����������.
		write_svg(DOCTYPE, dt_writer);
		//	svg �^�O�̊J�n����������.
		write_svg(SVG_TAG, dt_writer);
		//	���ƍ����̑�������������.
		constexpr char* SVG_UNIT_PX = "px";
		constexpr char* SVG_UNIT_IN = "in";
		constexpr char* SVG_UNIT_MM = "mm";
		constexpr char* SVG_UNIT_PT = "pt";
		const auto dpi = m_page_dx.m_logical_dpi;
		double w, h;
		char buf[256];
		char* u;
		switch (m_page_unit) {
		default:
		case LEN_UNIT::PIXEL:
			w = m_page_panel.m_page_size.width;
			h = m_page_panel.m_page_size.height;
			u = SVG_UNIT_PX;
			break;
		case LEN_UNIT::INCH:
			w = m_page_panel.m_page_size.width / dpi;
			h = m_page_panel.m_page_size.height / dpi;
			u = SVG_UNIT_IN;
			break;
		case LEN_UNIT::MILLI:
			w = m_page_panel.m_page_size.width * MM_PER_INCH / dpi;
			h = m_page_panel.m_page_size.height * MM_PER_INCH / dpi;
			u = SVG_UNIT_MM;
			break;
		case LEN_UNIT::POINT:
			w = m_page_panel.m_page_size.width * PT_PER_INCH / dpi;
			h = m_page_panel.m_page_size.height * PT_PER_INCH / dpi;
			u = SVG_UNIT_PT;
			break;
		}
		std::snprintf(buf, sizeof(buf), "width=\"%lf%s\" height=\"%lf%s\" ", w, u, h, u);
		write_svg(buf, dt_writer);
		//	viewBox ��������������.
		write_svg("viewBox=\"0 0 ", dt_writer);
		write_svg(m_page_panel.m_page_size.width, dt_writer);
		write_svg(m_page_panel.m_page_size.height, dt_writer);
		write_svg("\" ", dt_writer);
		//	�w�i�F���X�^�C�������Ƃ��ď�������.
		write_svg("style=\"background-color:", dt_writer);
		write_svg(m_page_panel.m_page_color, dt_writer);
		//	svg �^�O�̏I������������.
		write_svg("\" >" SVG_NL, dt_writer);
		//	�����t���O�̂Ȃ��}�`�����ׂ� SVG �Ƃ��ď�������.
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			s->write_svg(dt_writer);
		}
		//	svg �̏I���^�O����������.
		write_svg("</svg>" SVG_NL, dt_writer);
	}

	//	SVG �Ƃ��ăX�g���[�W�t�@�C���ɔ񓯊��ɏ�������.
	//	file	�������ݐ�̃t�@�C��
	//	�߂�l	�������߂��ꍇ S_OK
	IAsyncOperation<winrt::hresult> MainPage::file_write_svg_async(StorageFile const& s_file)
	{
		using winrt::Windows::Storage::CachedFileManager;
		using winrt::Windows::Storage::FileAccessMode;
		using winrt::Windows::Storage::Provider::FileUpdateStatus;
		using winrt::GraphPaper::implementation::write_svg;

		auto hr = E_FAIL;
		//	�R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		//	�X���b�h���o�b�N�O���E���h�ɕς���.
		co_await winrt::resume_background();
		try {
			//	�t�@�C���X�V�̒x����ݒ肷��.
			//	�X�g���[�W�t�@�C�����J���ă����_���A�N�Z�X�X�g���[���𓾂�.
			//	�����_���A�N�Z�X�X�g���[���̐擪����f�[�^���C�^�[���쐬����.
			CachedFileManager::DeferUpdates(s_file);
			auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::ReadWrite) };
			auto dt_writer{ DataWriter(ra_stream.GetOutputStreamAt(0)) };
			//	SVG �Ƃ��ăf�[�^���C�^�[�ɏ�������.
			file_write_svg(dt_writer);
			//	�X�g���[���̌��݈ʒu���X�g���[���̑傫���Ɋi�[����.
			//	�o�b�t�@���̃f�[�^���X�g���[���ɏo�͂���.
			//	�X�g���[�����t���b�V������.
			//	�x���������t�@�C���X�V����������.
			ra_stream.Size(ra_stream.Position());
			co_await dt_writer.StoreAsync();
			co_await ra_stream.FlushAsync();
			auto fu_status{ co_await CachedFileManager::CompleteUpdatesAsync(s_file) };
			if (fu_status == FileUpdateStatus::Complete) {
				// ���������ꍇ, S_OK �����ʂɊi�[����.
				hr = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// �G���[�����������ꍇ, �G���[�R�[�h�����ʂɊi�[����.
			hr = e.code();
		}
		if (hr != S_OK) {
			// ���ʂ� S_OK �łȂ��ꍇ,
			// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			// �������ݎ��s�̃��b�Z�[�W�_�C�A���O��\������.
			co_await winrt::resume_foreground(this->Dispatcher());
			cd_message_show(ERR_WRITE, s_file.Path());
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// ���ʂ�Ԃ��I������.
		co_return hr;
	}

	// �t�@�C���̓ǂݍ��݂��I������.
	void MainPage::finish_file_read(void)
	{
		enable_undo_menu();
		enable_edit_menu();
		stroke_style_check_menu(m_page_panel.m_stroke_style);
		arrow_style_check_menu(m_page_panel.m_arrow_style);
		font_style_check_menu(m_page_panel.m_font_style);
		text_align_t_check_menu(m_page_panel.m_text_align_t);
		text_align_p_check_menu(m_page_panel.m_text_align_p);
		grid_show_check_menu(m_page_panel.m_grid_show);
		tmfi_grid_snap().IsChecked(m_page_panel.m_grid_snap);
		tmfi_grid_snap_2().IsChecked(m_page_panel.m_grid_snap);
		stat_check_menu(m_stat_bar);

		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			wchar_t* val;
			if (s->get_font_family(val)) {
				if (ShapeText::is_available_font(val) == false) {
					// �L���łȂ����̂̃G���[���b�Z�[�W�_�C�A���O��\������.
					cd_message_show(ERR_FONT, val);
					break;
				}
			}
		}
		if (m_summary_visible) {
			if (m_list_shapes.size() > 0) {
				summary_remake();
			}
			else {
				summary_close();
			}
		}
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		stat_set_curs();
		stat_set_grid();
		stat_set_page();
		stat_set_draw();
		stat_set_zoom();
		stat_set_unit();
		stat_visibility();
	}

	// �t�@�C�����j���[�́u�I���v���I�����ꂽ
	IAsyncAction MainPage::mfi_exit_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::UI::Xaml::Application;

		winrt::event_token save_token;
		winrt::event_token dont_token;
		winrt::event_token cancel_token;

		if (m_stack_push == false) {
			// ����X�^�b�N�̍X�V�t���O���Ȃ��ꍇ,
			// �}�`�f�[�^�͕ύX����Ă��Ȃ��̂�,
			// �A�v���P�[�V�������I������.
			release();
			Application::Current().Exit();
			co_return;
		}
		cd_load_conf_save();
		save_token = cd_conf_save().PrimaryButtonClick(
			[this](auto, auto)
			{
				// �t�@�C���ɔ񓯊��ɕۑ�����, �A�v���P�[�V�������I������.
				auto _{ file_save_and_exit_async() };
			}
		);
		dont_token = cd_conf_save().SecondaryButtonClick(
			[this](auto, auto)
			{
				// �A�v���P�[�V�������I������.
				release();
				Application::Current().Exit();
			}
		);
		cancel_token = cd_conf_save().CloseButtonClick(
		//closed_token = cd_conf_save().Closed(
			[this, &save_token, &dont_token, &cancel_token](auto, auto)
			{
				// �C�x���g�n���h���[�����ׂĉ�����,
				// �ۑ��m�F�_�C�A���O��j������.
				cd_conf_save().PrimaryButtonClick(save_token);
				cd_conf_save().SecondaryButtonClick(dont_token);
				cd_conf_save().CloseButtonClick(cancel_token);
				UnloadObject(cd_conf_save());
			}
		);
		// �ۑ��m�F�_�C�A���O��\������.
		//auto _{ cd_conf_save().ShowAsync() };
		co_await cd_conf_save().ShowAsync();
	}

	// �t�@�C�����j���[�́u�V�K�v���I�����ꂽ
	IAsyncAction MainPage::mfi_new_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		winrt::event_token save_token;
		winrt::event_token dont_token;
		winrt::event_token closed_token;

		auto dialog = winrt::Windows::UI::Xaml::Controls::ContentDialog();
		if (m_stack_push == false) {
			// ����X�^�b�N�̍X�V�t���O���Ȃ��ꍇ,
			// �}�`�f�[�^�͕ύX����Ă��Ȃ��̂�,
			// �󔒂̕����ɂ���.
			new_doc();
			co_return;
		}
		cd_load_conf_save();
		save_token = cd_conf_save().PrimaryButtonClick(
			[this](auto, auto)
			{
				//	�t�@�C���ɔ񓯊��ɕۑ�����, �󔒂̕����ɂ���.
				auto _{ file_save_and_new_async() };
			}
		);
		dont_token = cd_conf_save().SecondaryButtonClick(
			[this](auto, auto)
			{
				// �󔒂̕����ɂ���.
				new_doc();
			}
		);
		closed_token = cd_conf_save().Closed(
			[this, &save_token, &dont_token, &closed_token](auto, auto)
			{
				// �C�x���g�n���h���[�����ׂĉ�����,
				// �ۑ��m�F�_�C�A���O��j������.
				cd_conf_save().PrimaryButtonClick(save_token);
				cd_conf_save().SecondaryButtonClick(dont_token);
				cd_conf_save().Closed(closed_token);
				UnloadObject(cd_conf_save());
			}
		);
		// �ۑ��m�F�_�C�A���O��\������.
		co_await cd_conf_save().ShowAsync();
	}

	// �t�@�C�����j���[�́u�J���v���I�����ꂽ
	IAsyncAction MainPage::mfi_open_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		winrt::event_token save_token;
		winrt::event_token dont_token;
		winrt::event_token closed_token;

		if (m_stack_push == false) {
			//	����X�^�b�N�̍X�V�t���O���Ȃ��ꍇ,
			//	�}�`�f�[�^�͕ύX����Ă��Ȃ��̂�,
			//	�t�@�C����񓯊��ɊJ��.
			auto _{ file_open_async() };
			//return;
			//co_await file_open_async();
			co_return;
		}
		cd_load_conf_save();
		save_token = cd_conf_save().PrimaryButtonClick(
			[this](auto, auto)
			{
				//	�t�@�C���ɔ񓯊��ɕۑ�����, �t�@�C����񓯊��ɊJ��.
				auto _{ file_save_and_open_async() };
			}
		);
		dont_token = cd_conf_save().SecondaryButtonClick(
			[this](auto, auto)
			{
				//	�t�@�C����񓯊��ɊJ��.
				auto _{ file_open_async() };
			}
		);
		closed_token = cd_conf_save().Closed(
			[this, &save_token, &dont_token, &closed_token](auto, auto)
			{
				//	�C�x���g�n���h���[�����ׂĉ�����,
				//	�ۑ��m�F�_�C�A���O��j������.
				cd_conf_save().PrimaryButtonClick(save_token);
				cd_conf_save().SecondaryButtonClick(dont_token);
				cd_conf_save().Closed(closed_token);
				UnloadObject(cd_conf_save());
			}
		);
		//	�ۑ��m�F�_�C�A���O��\������.
		co_await cd_conf_save().ShowAsync();
		//{ auto _{ cd_conf_save().ShowAsync() }; }
	}

	//	�t�@�C�����j���[�́u���O��t���ĕۑ��v���I�����ꂽ
	void MainPage::mfi_save_as_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	���O��t���ăt�@�C���ɔ񓯊��ɕۑ�����.
		auto _{ file_save_as_async(true) };
	}

	//	�t�@�C�����j���[�́u�ۑ��v���I�����ꂽ
	void MainPage::mfi_save_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	�t�@�C���ɔ񓯊��ɕۑ�����
		auto _{ file_save_async() };
	}

	//	�󔒂̕����ɂ���.
	void MainPage::new_doc(void)
	{
		if (m_summary_visible) {
			//	�}�`�ꗗ�p�l���̕\���t���O�������Ă���ꍇ,
			//	�ꗗ�����.
			summary_close();
		}
		//	����X�^�b�N����������.
		undo_clear();
		//	�}�`���X�g����������.
		s_list_clear(m_list_shapes);
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			cd_message_show(L"Memory leak occurs", {});
		}
#endif
		m_page_min.x = 0.0;
		m_page_min.y = 0.0;
		m_page_max.x = m_page_panel.m_page_size.width;
		m_page_max.y = m_page_panel.m_page_size.height;
		m_mru_token.clear();
		mru_add_file(nullptr);
		finish_file_read();
	}

}