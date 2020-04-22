//-------------------------------
// MainPage_file.cpp
// �t�@�C���̓ǂݏ���
//-------------------------------
#include "pch.h"
#include "MainPage.h"

// file_recent_read_async
// 	file_wait_cursor
// 	file_recent_get_async
// 		GetFileAsync
// 		winrt::resume_foreground
// 	file_read_async
// 		winrt::resume_background
// 		OpenAsync
// 		winrt::resume_foreground
// 		file_recent_add
// 			file_recent_update_menu
// 	file_finish_reading
// 	winrt::resume_foreground
// file_save_as_async
// 	file_wait_cursor
// 	file_recent_get_async
// 		GetFileAsync
// 		winrt::resume_foreground
// 	PickSaveFileAsync
// 	file_write_svg_async
// 		winrt::resume_background
// 		OpenAsync
// 		winrt::resume_foreground
// 	file_write_gpf_async
// 		winrt::resume_background
// 		OpenAsync
// 		winrt::resume_foreground
// 		file_recent_add
// 			file_recent_update_menu
// file_save_async
// 	file_recent_get_async
// 		GetFileAsync
// 		winrt::resume_foreground
// 	file_save_as_async
// 	file_wait_cursor
// 	file_write_gpf_async
// 		winrt::resume_background
// 		winrt::resume_foreground
// 		file_recent_add
// 			file_recent_update_menu
// new_click
// 	cd_conf_save().ShowAsync
// 	file_save_async
// 	file_recent_add
// 		file_recent_update_menu
// file_open_click_async
// 	cd_conf_save().ShowAsync
// 	file_save_async
// 	file_wait_cursor
// 	PickSingleFileAsync
// 	file_read_async
// 		winrt::resume_background
// 		OpenAsync
// 		winrt::resume_foreground
// 		file_recent_add
// 			file_recent_update_menu
// 		file_finish_reading
// mfi_file_recent_N_click
// 	file_recent_read_async
// file_save_as_click
// 	file_save_as_async
// file_save_click
// 	file_save_async
/*
�������A4��Windows�����^�C���񓯊�����^�C�v�iIAsyncXxx�j�̂����ꂩ��
co_await�����ꍇ�AC ++ / WinRT��co_await�̎��_�ŌĂяo���R���e�L�X�g���L���v�`�����܂��B
�܂��A�p�����ĊJ���ꂽ�Ƃ��ɁA���̃R���e�L�X�g�ɂ��邱�Ƃ��ۏ؂���܂��B
C ++ / WinRT�́A�Ăяo�����̃R���e�L�X�g�Ɋ��ɂ��邩�ǂ������m�F���A
�����łȂ��ꍇ�͐؂�ւ��܂��B
*/

// GetFileFromPathAsync �� E_ACCESSDENIED �Ȃ��Ɏg���ɂ͈ȉ����K�v�ɂȂ�.
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
	using winrt::Windows::UI::Core::CoreCursorType;

	static auto const& CUR_WAIT = CoreCursor(CoreCursorType::Wait, 0);	// �ҋ@�J�[�\��.
	constexpr wchar_t DESC_GPF[] = L"str_desc_gpf";	// �g���q gpf �̐���
	constexpr wchar_t DESC_SVG[] = L"str_desc_svg";	// �g���q svg �̐���
	constexpr wchar_t ERR_FONT[] = L"str_err_font";	// �L���łȂ����̂̃G���[���b�Z�[�W�̃��\�[�X��
	constexpr wchar_t ERR_READ[] = L"str_err_read";	// �ǂݍ��݃G���[���b�Z�[�W�̃��\�[�X��
	constexpr wchar_t ERR_RECENT[] = L"str_err_recent";	// �ŋߎg�����t�@�C���̃G���[���b�Z�[�W�̃��\�[�X��
	constexpr wchar_t ERR_WRITE[] = L"str_err_write";	// �������݃G���[���b�Z�[�W�̃��\�[�X��
	constexpr wchar_t FT_GPF[] = L".gpf";	// �}�`�f�[�^�t�@�C���̊g���q
	constexpr wchar_t FT_SVG[] = L".svg";	// SVG �t�@�C���̊g���q
	constexpr uint32_t MRU_MAX = 25;	// �ŋߎg�������X�g�̍ő吔.
	constexpr wchar_t UNTITLED[] = L"str_untitled";	// ����̃��\�[�X��

	// SVG �J�n�^�O���f�[�^���C�^�[�ɏ�������.
	static void file_write_svg_tag(D2D1_SIZE_F const& size, D2D1_COLOR_F const& color, const double dpi, const LEN_UNIT unit, DataWriter const& dt_writer);

	// SVG �J�n�^�O���f�[�^���C�^�[�ɏ�������.
	static void file_write_svg_tag(D2D1_SIZE_F const& size, D2D1_COLOR_F const& color, const double dpi, const LEN_UNIT unit, DataWriter const& dt_writer)
	{
		constexpr char SVG_TAG[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" ";
		constexpr char* SVG_UNIT_PX = "px";
		constexpr char* SVG_UNIT_IN = "in";
		constexpr char* SVG_UNIT_MM = "mm";
		constexpr char* SVG_UNIT_PT = "pt";

		// SVG �^�O�̊J�n����������.
		write_svg(SVG_TAG, dt_writer);
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
		write_svg(buf, dt_writer);
		// �s�N�Z���P�ʂ̕��ƍ����� viewBox �����Ƃ��ď�������.
		write_svg("viewBox=\"0 0 ", dt_writer);
		write_svg(size.width, dt_writer);
		write_svg(size.height, dt_writer);
		write_svg("\" ", dt_writer);
		// �w�i�F���X�^�C�������Ƃ��ď�������.
		write_svg("style=\"background-color:", dt_writer);
		write_svg(color, dt_writer);
		// svg �J�n�^�O�̏I������������.
		write_svg("\" >" SVG_NL, dt_writer);
	}

	// �t�@�C���̓ǂݍ��݂��I������.
	void MainPage::file_finish_reading(void)
	{
		edit_menu_enable();
		color_code_check_menu();
		stroke_style_check_menu(m_page_sheet.m_stroke_style);
		arrow_style_check_menu(m_page_sheet.m_arrow_style);
		font_style_check_menu(m_page_sheet.m_font_style);
		grid_patt_check_menu(m_page_sheet.m_grid_patt);
		grid_show_check_menu(m_page_sheet.m_grid_show);
		stbar_check_menu(status_bar());
		text_align_t_check_menu(m_page_sheet.m_text_align_t);
		text_align_p_check_menu(m_page_sheet.m_text_align_p);
		tmfi_grid_snap().IsChecked(m_page_sheet.m_grid_snap);
		tmfi_grid_snap_2().IsChecked(m_page_sheet.m_grid_snap);
		len_unit_check_menu();

		// �}�`���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			wchar_t* value;	// ���̖�
			if (s->get_font_family(value)) {
				if (ShapeText::is_available_font(value) != true) {
					// �u�����ȏ��̂��g�p����Ă��܂��v���b�Z�[�W�_�C�A���O��\������.
					cd_message_show(ICON_ALERT, ERR_FONT, value);
					break;
				}
			}
		}
		if (m_mutex_summary.load(std::memory_order_acquire)) {
			//if (m_summary_visible) {
			if (m_list_shapes.empty()) {
				summary_close();
			}
			else {
				summary_remake();
			}
		}
		page_bound();
		page_panle_size();
		page_draw();
		stbar_set_curs();
		stbar_set_draw();
		stbar_set_grid();
		stbar_set_page();
		stbar_set_zoom();
		stbar_set_unit();
		stbar_visibility();
	}

	// �t�@�C�����j���[�́u�J���v���I�����ꂽ
	IAsyncAction MainPage::file_open_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::Storage::Pickers::FileOpenPicker;

		// SHCore.dll �X���b�h
		if (undo_pushed()) {
			// ����X�^�b�N�̍X�V�t���O�������Ă���ꍇ,
			// �ۑ��m�F�_�C�A���O��\������.
			const auto d_result = co_await cd_conf_save().ShowAsync();
			if (d_result == ContentDialogResult::None) {
				// �u�L�����Z���v�������ꂽ�ꍇ,
					// ���f����.
				co_return;
			}
			else if (d_result == ContentDialogResult::Primary) {
				// �u�ۑ�����v�������ꂽ�ꍇ,
				if (co_await file_save_async() != S_OK) {
					// �ۑ��ł��Ȃ������ꍇ,
					// ���f����.
					co_return;
				}
			}
		}
		// �ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
		auto const& p_cur = file_wait_cursor();
		// �t�@�C���u�I�[�v���v�s�b�J�[���擾���ĊJ��.
		auto o_picker{ FileOpenPicker() };
		o_picker.FileTypeFilter().Append(FT_GPF);
		// �_�u���N���b�N�Ńt�@�C�����I�����ꂽ�ꍇ,
		// co_await ���I������O��, PonterReleased �� PonterEntered ���Ă΂��.
		// ����̓s�b�J�[�� 2 �x�ڂ� Released ��҂����Ƀ_�u���N���b�N�𐬗������Ă��邽�߂��Ǝv����.
		//scp_page_panel().PointerReleased(m_token_pointer_released);
		//scp_page_panel().PointerEntered(m_token_pointer_entered);
		// �s�b�J�[��񓯊��ŕ\�����ăX�g���[�W�t�@�C�����擾����.
		// �u����v�{�^���������ꂽ�ꍇ�X�g���[�W�t�@�C���Ƃ��� nullptr ���Ԃ�.
		auto s_file{ co_await o_picker.PickSingleFileAsync() };
		if (s_file != nullptr) {
			// �X�g���[�W�t�@�C�����k���łȂ��ꍇ�A
			// �X�g���[�W�t�@�C����񓯊��ɓǂ�.
			co_await file_read_async(s_file);
			// �X�g���[�W�t�@�C�����������.
			s_file = nullptr;
			file_finish_reading();
		}
		o_picker = nullptr;
		//m_token_pointer_released = scp_page_panel().PointerReleased({ this, &MainPage::pointer_released });
		//m_token_pointer_entered = scp_page_panel().PointerEntered({ this, &MainPage::pointer_entered });
		Window::Current().CoreWindow().PointerCursor(p_cur);
	}

	// �X�g���[�W�t�@�C����񓯊��ɓǂ�.
	// s_file	�ǂݍ��ރX�g���[�W�t�@�C��
	// suspend	���f�t���O
	// sheet	�V�[�g�̂݃t���O
	// �߂�l	�ǂݍ��߂��� S_OK.
	// ���f�t���O�������Ă���ꍇ, ����X�^�b�N���ۑ�����.
	IAsyncOperation<winrt::hresult> MainPage::file_read_async(StorageFile const& s_file, const bool suspend, const bool sheet) noexcept
	{
		using winrt::Windows::Storage::FileAccessMode;

		m_mutex_page.lock();
		auto hr = E_FAIL;
		winrt::apartment_context context;
		try {
			co_await winrt::resume_background();
			auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::Read) };
			auto dt_reader{ DataReader(ra_stream.GetInputStreamAt(0)) };
			co_await dt_reader.LoadAsync(static_cast<uint32_t>(ra_stream.Size()));

			text_find_read(dt_reader);
			stbar_read(dt_reader);
			len_unit(static_cast<LEN_UNIT>(dt_reader.ReadUInt32()));
			color_code(static_cast<COLOR_CODE>(dt_reader.ReadUInt16()));
			status_bar(static_cast<STATUS_BAR>(dt_reader.ReadUInt16()));

			m_page_sheet.read(dt_reader);
			// �����ȃf�[�^��ǂݍ���ŃA�v���������邱�Ƃ��Ȃ��悤, �l�𐧌�����.
			m_page_sheet.m_grid_base = max(m_page_sheet.m_grid_base, 0.0F);
			m_page_sheet.m_page_scale = min(max(m_page_sheet.m_page_scale, SCALE_MIN), SCALE_MAX);
			m_page_sheet.m_page_size.width = max(min(m_page_sheet.m_page_size.width, page_size_max()), 1.0F);
			m_page_sheet.m_page_size.height = max(min(m_page_sheet.m_page_size.height, page_size_max()), 1.0F);

			undo_clear();
			s_list_clear(m_list_shapes);
#if defined(_DEBUG)
			if (debug_leak_cnt != 0) {
				auto cd = this->Dispatcher();
				co_await winrt::resume_foreground(cd);
				cd_message_show(ICON_ALERT, L"Memory leak occurs", {});
				co_await context;
				m_mutex_page.unlock();
				co_return hr;
			}
#endif
			if (sheet) {
				// �V�[�g�̂݃t���O�������Ă���ꍇ,
				hr = S_OK;
			}
			else if (s_list_read(m_list_shapes, dt_reader)) {
				if (suspend) {
					// ���f�t���O�������Ă���ꍇ,
					undo_read(dt_reader);
				}
				hr = S_OK;
			}
			dt_reader.Close();
			ra_stream.Close();
		}
		catch (winrt::hresult_error const& e) {
			hr = e.code();
		}
		if (hr != S_OK) {
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			cd_message_show(ICON_ALERT, ERR_READ, s_file.Path());
		}
		else if (suspend != true && sheet != true) {
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			file_recent_add(s_file);
			file_finish_reading();
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// ���ʂ�Ԃ��I������.
		m_mutex_page.unlock();
		co_return hr;
	}

	// �t�@�C�����j���[�́u�ŋߎg�����t�@�C�� 1�v���I�����ꂽ
	void MainPage::file_recent_1_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �ŋߎg�����t�@�C�� (0) ��ǂݍ���.
		auto _{ file_recent_read_async(0) };
	}

	// �t�@�C�����j���[�́u�ŋߎg�����t�@�C�� 2�v���I�����ꂽ
	void MainPage::file_recent_2_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �ŋߎg�����t�@�C�� (1) ��ǂݍ���.
		auto _{ file_recent_read_async(1) };
	}

	// �t�@�C�����j���[�́u�ŋߎg�����t�@�C�� 3�v���I�����ꂽ
	void MainPage::file_recent_3_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �ŋߎg�����t�@�C�� (2) ��ǂݍ���.
		auto _{ file_recent_read_async(2) };
	}

	// �t�@�C�����j���[�́u�ŋߎg�����t�@�C�� 4�v���I�����ꂽ
	void MainPage::file_recent_4_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �ŋߎg�����t�@�C�� (3) ��ǂݍ���.
		auto _{ file_recent_read_async(3) };
	}

	// �t�@�C�����j���[�́u�ŋߎg�����t�@�C�� 5�v���I�����ꂽ
	void MainPage::file_recent_5_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �ŋߎg�����t�@�C�� (4) ��ǂݍ���.
		auto _{ file_recent_read_async(4) };
	}

	// �X�g���[�W�t�@�C�����ŋߎg�����t�@�C���ɓo�^����.
	// s_file	�X�g���[�W�t�@�C��
	// �߂�l	�Ȃ�
	// �ŋߎg�����t�@�C�����j���[�ƃE�B���h�E�^�C�g�����X�V�����.
	// �X�g���[�W�t�@�C�����k���̏ꍇ, �ŋߎg�����t�@�C���͂��̂܂܂�, 
	// �E�B���h�E�^�C�g���ɖ��肪�i�[�����.
	void MainPage::file_recent_add(StorageFile const& s_file)
	{
		using winrt::Windows::UI::ViewManagement::ApplicationView;

		if (s_file != nullptr) {
			using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
			auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
			//if (mru_list.ContainsItem(m_token_mru)) {
				//mru_list.Remove(m_token_mru);
				//mru_list.AddOrReplace(m_token_mru, s_file, s_file.Path());
			//}
			//else {
			m_token_mru = mru_list.Add(s_file, s_file.Path());
			//}
			ApplicationView::GetForCurrentView().Title(s_file.Name());
		}
		else {
			using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			ApplicationView::GetForCurrentView().Title(r_loader.GetString(UNTITLED));
		}
		// �ŋߎg�����t�@�C�����j�����X�V����.
		file_recent_update_menu();
	}

	// �ŋߎg�����t�@�C���̃g�[�N������X�g���[�W�t�@�C���𓾂�.
	// token	�ŋߎg�����t�@�C���̃g�[�N��
	// �߂�l	�X�g���[�W�t�@�C��
	IAsyncOperation<StorageFile> MainPage::file_recent_get_async(const winrt::hstring token)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;

		// �R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;

		// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		//co_await winrt::resume_foreground(this->Dispatcher());
		// �X�g���[�W�t�@�C���Ƀk�����i�[����.
		StorageFile s_file = nullptr;
		try {
			if (token.empty() != true) {
				// �g�[�N������łȂ��ꍇ,
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
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		// SHCore.dll �X���b�h
		auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		auto const& mru_entries = mru_list.Entries();
		if (i >= mru_entries.Size()) {
			// �t�@�C���̔ԍ����ŋߎg�����t�@�C���̐��ȏ�̏ꍇ,
			cd_message_show(ICON_ALERT, ERR_RECENT, to_hstring(i + 1));
			co_return;
		}
		// �ŋߎg�������X�g����v�f�𓾂�.
		AccessListEntry item[1];
		mru_entries.GetMany(i, item);

		if (undo_pushed()) {
			// ����X�^�b�N�̍X�V�t���O�������Ă���ꍇ,
			const auto d_result = co_await cd_conf_save().ShowAsync();
			if (d_result == ContentDialogResult::None) {
				// �u�L�����Z���v�������ꂽ�ꍇ,
				co_return;
			}
			else if (d_result == ContentDialogResult::Primary) {
				// �u�ۑ�����v�������ꂽ�ꍇ,
				if (co_await file_save_async() != S_OK) {
					co_return;
				}
			}
		}
		auto const p_cur = file_wait_cursor();
		auto s_file{ co_await file_recent_get_async(item[0].Token) };
		if (s_file != nullptr) {
			// �擾�ł����ꍇ,
			co_await file_read_async(s_file);
			s_file = nullptr;
			file_finish_reading();
		}
		else {
			// �擾�ł��Ȃ��ꍇ,
			cd_message_show(ICON_ALERT, ERR_RECENT, item[0].Metadata);
		}

		// �E�B���h�E�̃J�[�\���𕜌�����.
		Window::Current().CoreWindow().PointerCursor(p_cur);
	}

	// �ŋߎg�����t�@�C�����j�����X�V����.
	void MainPage::file_recent_update_menu(void)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

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
	// svg_allowed	SVG ���e�t���O.
	IAsyncOperation<winrt::hresult> MainPage::file_save_as_async(const bool svg_allowed) noexcept
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::Storage::Pickers::FileSavePicker;

		auto hr = E_FAIL;
		// �ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
		auto const& p_cur = file_wait_cursor();
		// �R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		//winrt::apartment_context context;
		//co_await resume_background(this->Dispatcher());
		try {
			// �t�@�C���ۑ��s�b�J�[�𓾂�.
			// �t�@�C���^�C�v�Ɋg���q GPF �Ƃ��̐�����ǉ�����.
			auto s_picker{ FileSavePicker() };
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			auto desc_gpf = r_loader.GetString(DESC_GPF);
			auto type_gpf = winrt::single_threaded_vector<winrt::hstring>({ FT_GPF });
			s_picker.FileTypeChoices().Insert(desc_gpf, type_gpf);
			// SVG ���e�t���O�������Ă��邩���肷��.
			if (svg_allowed) {
				// �t���O�������Ă���ꍇ, 
				// �t�@�C���^�C�v�Ɋg���q SVG �Ƃ��̐�����ǉ�����.
				auto desc_svg = r_loader.GetString(DESC_SVG);
				auto type_svg = winrt::single_threaded_vector<winrt::hstring>({ FT_SVG });
				s_picker.FileTypeChoices().Insert(desc_svg, type_svg);
			}
			// �ŋߎg�����t�@�C���̃g�[�N�����󂩔��肷��.
			if (m_token_mru.empty()) {
				// �g�[�N������̏ꍇ, 
				// ��Ă��ꂽ�t�@�C�����Ɋg���q���i�[����.
				s_picker.SuggestedFileName(FT_GPF);
			}
			else {
				// �g�[�N������łȂ��ꍇ,
				// �X�g���[�W�t�@�C�����ŋߎg�����t�@�C���̃g�[�N�����瓾��.
				auto s_file{ co_await file_recent_get_async(m_token_mru) };
				if (s_file != nullptr) {
					// �擾�����ꍇ,
					if (s_file.FileType() == FT_GPF) {
						// ���t�@�C���^�C�v�� GPF �̏ꍇ,
						// �t�@�C������, ��Ă���t�@�C�����Ɋi�[����.
						s_picker.SuggestedFileName(s_file.Name());
					}
					s_file = nullptr;
				}
			}
			// �t�@�C���ۑ��s�b�J�[��\����, �X�g���[�W�t�@�C���𓾂�.
			auto s_file{ co_await s_picker.PickSaveFileAsync() };
			// �X�g���[�W�t�@�C�����擾���������肷��.
			if (s_file != nullptr) {
				// �擾�����ꍇ,
				if (s_file.FileType() == FT_SVG) {
					// �t�@�C���^�C�v�� SVG �̏ꍇ,
					// �}�`�f�[�^�� SVG �Ƃ��ăX�g���[�W�t�@�C���ɔ񓯊��ɏ�������, ���ʂ𓾂�.
					hr = co_await file_write_svg_async(s_file);
				}
				else {
					// �t�@�C���^�C�v�� SVG �ȊO�̏ꍇ,
				// �}�`�f�[�^���X�g���[�W�t�@�C���ɔ񓯊��ɏ�������, ���ʂ𓾂�.
					hr = co_await file_write_gpf_async(s_file);
				}
				// �X�g���[�W�t�@�C����j������.
				s_file = nullptr;
			}
			// �s�b�J�[��j������.
			s_picker = nullptr;
		}
		catch (winrt::hresult_error const& e) {
			// �G���[�����������ꍇ, �G���[�R�[�h�����ʂɊi�[����.
			hr = e.code();
		}
		// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		//co_await winrt::resume_foreground(this->Dispatcher());
		// �E�B���h�E�̃J�[�\���𕜌�����.
		Window::Current().CoreWindow().PointerCursor(p_cur);
		// �X���b�h�R���e�L�X�g�𕜌�����.
		//co_await context;
		// ���ʂ�Ԃ��I������.
		co_return hr;
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
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		// �ŋߎg�����t�@�C���̃g�[�N������X�g���[�W�t�@�C���𓾂�.
		auto s_file{ co_await file_recent_get_async(m_token_mru) };
		if (s_file == nullptr) {
			// �X�g���[�W�t�@�C������̏ꍇ,
			constexpr bool SVG_ALLOWED = true;
			// ���O��t���ăt�@�C���ɔ񓯊��ɕۑ�����
			co_return co_await file_save_as_async(!SVG_ALLOWED);
		}

		auto hr = E_FAIL;
		// �ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
		auto const& p_cur = file_wait_cursor();
		// �}�`�f�[�^���X�g���[�W�t�@�C���ɔ񓯊��ɏ�������, ���ʂ𓾂�.
		hr = co_await file_write_gpf_async(s_file);
		// ���ʂ𔻒肷��.
		if (hr != S_OK) {
			// ���ʂ� S_OK �łȂ��ꍇ,
			// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			// �ŋߎg�����t�@�C���̃G���[���b�Z�[�W�_�C�A���O��\������.
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			// �u�t�@�C���ɏ������߂܂���v���b�Z�[�W�_�C�A���O��\������.
			cd_message_show(ICON_ALERT, ERR_WRITE, m_token_mru);
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		//co_await context;
		// �E�B���h�E�̃J�[�\���𕜌�����.
		Window::Current().CoreWindow().PointerCursor(p_cur);
		// ���ʂ�Ԃ��I������.
		co_return hr;
	}

	// �t�@�C�����j���[�́u�㏑���ۑ��v���I�����ꂽ
	void MainPage::file_save_click(IInspectable const&, RoutedEventArgs const&)
	{
		auto _{ file_save_async() };
	}

	// �ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
	CoreCursor MainPage::file_wait_cursor(void) const
	{
		auto const& c_win = Window::Current().CoreWindow();
		auto p_cur = c_win.PointerCursor();
		c_win.PointerCursor(CUR_WAIT);
		return p_cur;
	}

	// �}�`�f�[�^���X�g���[�W�t�@�C���ɔ񓯊��ɏ�������.
	// s_file	�X�g���[�W�t�@�C��
	// suspend	���f�t���O
	// �߂�l	�������݂ɐ��������� true
	IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async(StorageFile const& s_file, const bool suspend, const bool layout)
	{
		using winrt::Windows::Storage::CachedFileManager;
		using winrt::Windows::Storage::FileAccessMode;
		using winrt::Windows::Storage::Provider::FileUpdateStatus;
		constexpr auto REDUCE = true;

		// E_FAIL �����ʂɊi�[����.
		auto hr = E_FAIL;
		// �R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
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

			text_find_write(dt_writer);
			stbar_write(dt_writer);
			dt_writer.WriteUInt32(static_cast<uint32_t>(len_unit()));
			dt_writer.WriteUInt16(static_cast<uint16_t>(color_code()));
			dt_writer.WriteUInt16(static_cast<uint16_t>(status_bar()));
			m_page_sheet.write(dt_writer);
			if (suspend) {
				s_list_write<!REDUCE>(m_list_shapes, dt_writer);
				undo_write(dt_writer);
			}
			else if (layout) {
			}
			else {
				s_list_write<REDUCE>(m_list_shapes, dt_writer);
			}
			ra_stream.Size(ra_stream.Position());
			co_await dt_writer.StoreAsync();
			co_await ra_stream.FlushAsync();
			// �f�[�^���C�^�[�����.
			dt_writer.Close();
			// �X�g���[�������.
			ra_stream.Close();
			// �x���������t�@�C���X�V����������.
			auto fu_status{ co_await CachedFileManager::CompleteUpdatesAsync(s_file) };
			if (fu_status == FileUpdateStatus::Complete) {
				// ���������ꍇ, 
				// S_OK �����ʂɊi�[����.
				hr = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// �G���[�����������ꍇ, 
			// �G���[�R�[�h�����ʂɊi�[����.
			hr = e.code();
		}
		if (hr != S_OK) {
			// ���ʂ� S_OK �łȂ��ꍇ,
			// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			// �u�t�@�C���ɏ������߂܂���v���b�Z�[�W�_�C�A���O��\������.
			cd_message_show(ICON_ALERT, ERR_WRITE, s_file.Path());
		}
		else if (suspend != true && layout != true) {
			// ���f�t���O���Ȃ��ꍇ,
			// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			// �X�g���[�W�t�@�C�����ŋߎg�����t�@�C���ɓo�^����.
			// �����ŃG���[���o��.
			file_recent_add(s_file);
			// false �𑀍�X�^�b�N�̍X�V�t���O�Ɋi�[����.
			undo_pushed(false);
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// ���ʂ�Ԃ��I������.
		co_return hr;
	}

	// �}�`�f�[�^�� SVG �Ƃ��ăX�g���[�W�t�@�C���ɔ񓯊��ɏ�������.
	// file	�������ݐ�̃t�@�C��
	// �߂�l	�������߂��ꍇ S_OK
	IAsyncOperation<winrt::hresult> MainPage::file_write_svg_async(StorageFile const& s_file)
	{
		using winrt::Windows::Storage::CachedFileManager;
		using winrt::Windows::Storage::FileAccessMode;
		using winrt::Windows::Storage::Provider::FileUpdateStatus;
		using winrt::GraphPaper::implementation::write_svg;
		constexpr char XML_DEC[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" SVG_NL;
		constexpr char DOCTYPE[] = "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" SVG_NL;

		auto hr = E_FAIL;
		// �R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		// �X���b�h���o�b�N�O���E���h�ɕς���.
		co_await winrt::resume_background();
		try {
			// �t�@�C���X�V�̒x����ݒ肷��.
			CachedFileManager::DeferUpdates(s_file);
			// �X�g���[�W�t�@�C�����J���ă����_���A�N�Z�X�X�g���[���𓾂�.
			auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::ReadWrite) };
			// �����_���A�N�Z�X�X�g���[���̐擪����f�[�^���C�^�[���쐬����.
			auto dt_writer{ DataWriter(ra_stream.GetOutputStreamAt(0)) };
			// XML �錾����������.
			write_svg(XML_DEC, dt_writer);
			// DOCTYPE ����������.
			write_svg(DOCTYPE, dt_writer);
			// SVG �J�n�^�O���f�[�^���C�^�[�ɏ�������.
			file_write_svg_tag(m_page_sheet.m_page_size, m_page_sheet.m_page_color, page_dpi(), len_unit(), dt_writer);
			// �}�`���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
			for (auto s : m_list_shapes) {
				if (s->is_deleted()) {
					// �����t���O�������Ă���ꍇ,
					// �ȉ��𖳎�����.
					continue;
				}
				s->write_svg(dt_writer);
			}
			// SVG �I���^�O����������.
			write_svg("</svg>" SVG_NL, dt_writer);
			// �X�g���[���̌��݈ʒu���X�g���[���̑傫���Ɋi�[����.
			ra_stream.Size(ra_stream.Position());
			// �o�b�t�@���̃f�[�^���X�g���[���ɏo�͂���.
			co_await dt_writer.StoreAsync();
			// �X�g���[�����t���b�V������.
			co_await ra_stream.FlushAsync();
			// �x���������t�@�C���X�V����������.
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
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			// �u�t�@�C���ɏ������߂܂���v���b�Z�[�W�_�C�A���O��\������.
			cd_message_show(ICON_ALERT, ERR_WRITE, s_file.Path());
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// ���ʂ�Ԃ��I������.
		co_return hr;
	}

}