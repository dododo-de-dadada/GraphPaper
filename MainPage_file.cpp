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

	// �f�[�^���C�^�[�� SVG �J�n�^�O����������.
	static void file_dt_write_svg_tag(D2D1_SIZE_F const& size, D2D1_COLOR_F const& color, const double dpi, const LEN_UNIT unit, DataWriter const& dt_writer);

	// �f�[�^���C�^�[�� SVG �J�n�^�O����������.
	static void file_dt_write_svg_tag(D2D1_SIZE_F const& size, D2D1_COLOR_F const& color, const double dpi, const LEN_UNIT unit, DataWriter const& dt_writer)
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

		tool_draw_is_checked(m_tool_draw);
		tool_poly_is_checked(m_tool_poly);
		//tool_vert_snap_is_checked(m_tool_vert_snap);

		color_code_is_checked(m_color_code);
		status_bar_is_checked(m_status_bar);
		len_unit_is_checked(m_len_unit);

		sheet_attr_is_checked();

		wchar_t* unavailable_font;	// ���̖�
		if (slist_test_font(m_list_shapes, unavailable_font) != true) {
			// �u�����ȏ��̂��g�p����Ă��܂��v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_ALERT, ERR_FONT, unavailable_font);
		}
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			if (m_list_shapes.empty()) {
				summary_close_click(nullptr, nullptr);
			}
			else {
				summary_remake();
			}
		}
		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
		status_bar_set_curs();
		status_bar_set_draw();
		status_bar_set_grid();
		status_bar_set_sheet();
		status_bar_set_zoom();
		status_bar_set_unit();
		status_bar_visibility();
	}

	// �t�@�C�����j���[�́u�J���v���I�����ꂽ
	IAsyncAction MainPage::file_open_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::Storage::Pickers::FileOpenPicker;

		if (m_stack_updt && !co_await ask_for_conf_async()) {
			co_return;
		}
		// �ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
		auto const& prev_cur = file_wait_cursor();
		// �t�@�C���u�I�[�v���v�s�b�J�[���擾���ĊJ��.
		auto o_picker{FileOpenPicker()};
		o_picker.FileTypeFilter().Append(FT_GPF);
		// �_�u���N���b�N�Ńt�@�C�����I�����ꂽ�ꍇ,
		// co_await ���I������O��, PonterReleased �� PonterEntered ���Ă΂��.
		// ����̓s�b�J�[�� 2 �x�ڂ� Released ��҂����Ƀ_�u���N���b�N�𐬗������Ă��邽�߂��Ǝv����.
		//scp_sheet_panel().PointerReleased(m_token_event_released);
		//scp_sheet_panel().PointerEntered(m_token_event_entered);

		// �s�b�J�[��񓯊��ŕ\�����ăX�g���[�W�t�@�C�����擾����.
		// �X�g���[�W�t�@�C�����k���|�C���^�[�����肷��.
		// (�u����v�{�^���������ꂽ�ꍇ�X�g���[�W�t�@�C���� nullptr.)
		auto s_file{ co_await o_picker.PickSingleFileAsync() };
		if (s_file != nullptr) {
			// �X�g���[�W�t�@�C����񓯊��ɓǂ�.
			co_await file_read_async(s_file);
			// �X�g���[�W�t�@�C�����������.
			s_file = nullptr;
			//file_finish_reading();
		}
		o_picker = nullptr;
		Window::Current().CoreWindow().PointerCursor(prev_cur);
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

		auto hres = E_FAIL;
		winrt::apartment_context context;
		m_dx_mutex.lock();
		try {
			const auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::Read) };
			auto dt_reader{ DataReader(ra_stream.GetInputStreamAt(0)) };
			const auto ra_size = static_cast<uint32_t>(ra_stream.Size());
			co_await dt_reader.LoadAsync(ra_size);

			tool_read(dt_reader);
			find_text_read(dt_reader);
			//m_status_bar = static_cast<STATUS_BAR>(dt_reader.ReadUInt32());
			m_len_unit = static_cast<LEN_UNIT>(dt_reader.ReadUInt32());
			m_color_code = static_cast<COLOR_CODE>(dt_reader.ReadUInt16());
			m_status_bar = static_cast<STATUS_BAR>(dt_reader.ReadUInt16());

			m_sheet_main.read(dt_reader);
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			m_sheet_main.set_grid_base(max(g_base, 0.0F));
			m_sheet_main.m_sheet_scale = min(max(m_sheet_main.m_sheet_scale, 0.25f), 4.0f);
			m_sheet_main.m_sheet_size.width = max(min(m_sheet_main.m_sheet_size.width, sheet_size_max()), 1.0F);
			m_sheet_main.m_sheet_size.height = max(min(m_sheet_main.m_sheet_size.height, sheet_size_max()), 1.0F);

			undo_clear();
			slist_clear(m_list_shapes);
#if defined(_DEBUG)
			if (debug_leak_cnt != 0) {
				auto cd = this->Dispatcher();
				co_await winrt::resume_foreground(cd);
				message_show(ICON_ALERT, DEBUG_MSG, {});
				co_await context;
				m_dx_mutex.unlock();
				co_return hres;
			}
#endif
			if (sheet) {
				// �V�[�g�̂݃t���O�������Ă���ꍇ,
				hres = S_OK;
			}
			else if (slist_read(m_list_shapes, dt_reader)) {
				if (suspend) {
					// ���f�t���O�������Ă���ꍇ,
					undo_read(dt_reader);
				}
				hres = S_OK;
			}
			dt_reader.Close();
			ra_stream.Close();
		}
		catch (winrt::hresult_error const& e) {
			hres = e.code();
		}
		m_dx_mutex.unlock();
		if (hres != S_OK) {
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			message_show(ICON_ALERT, ERR_READ, s_file.Path());
		}
		else if (!suspend && !sheet) {
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			file_recent_add(s_file);
			file_finish_reading();
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// ���ʂ�Ԃ��I������.
		co_return hres;
	}

	// �X�g���[�W�t�@�C�����ŋߎg�����t�@�C���ɓo�^����.
	// s_file	�X�g���[�W�t�@�C��
	// �߂�l	�Ȃ�
	// �ŋߎg�����t�@�C�����j���[�ƃE�B���h�E�^�C�g�����X�V�����.
	// �X�g���[�W�t�@�C�����k���̏ꍇ, �E�B���h�E�^�C�g���ɖ��肪�i�[�����.
	void MainPage::file_recent_add(StorageFile const& s_file)
	{
		using winrt::Windows::UI::ViewManagement::ApplicationView;
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

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
		//using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

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

		if (m_stack_updt && !co_await ask_for_conf_async()) {
			co_return;
		}
		/*
		// ����X�^�b�N�̍X�V�t���O�������Ă��邩���肷��.
		if (m_stack_updt) {
			// �m�F�_�C�A���O��\����, ���ʂ𓾂�.
			const auto dres = co_await cd_conf_save_dialog().ShowAsync();
			// �_�C�A���O�̌��ʂ��L�����Z�������肷��.
			if (dres == ContentDialogResult::None) {
				co_return;
			}
			// �_�C�A���O�̌��ʂ��u�ۑ�����v�����肷��.
			else if (dres == ContentDialogResult::Primary) {
				// �t�@�C���ɔ񓯊��ɕۑ���, ���ʂ� S_OK �ȊO�����肷��.
				if (co_await file_save_async() != S_OK) {
					co_return;
				}
			}
		}
		*/
		auto const prev_cur = file_wait_cursor();
		auto s_file{ co_await file_recent_get_async(item[0].Token) };
		if (s_file != nullptr) {
			// �擾�ł����ꍇ,
			co_await file_read_async(s_file);
			s_file = nullptr;
		}
		else {
			// �擾�ł��Ȃ��ꍇ,
			message_show(ICON_ALERT, ERR_RECENT, item[0].Metadata);
		}

		// �E�B���h�E�̃J�[�\���𕜌�����.
		Window::Current().CoreWindow().PointerCursor(prev_cur);
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

		auto hres = E_FAIL;
		auto const& prev_cur = file_wait_cursor();	// �\������O�̃J�[�\��
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
				// �t�@�C���^�C�v�Ɋg���q SVG �Ƃ��̐�����ǉ�����.
				auto desc_svg = r_loader.GetString(DESC_SVG);
				auto type_svg = winrt::single_threaded_vector<winrt::hstring>({ FT_SVG });
				s_picker.FileTypeChoices().Insert(desc_svg, type_svg);
			}
			// �ŋߎg�����t�@�C���̃g�[�N�����󂩔��肷��.
			if (m_file_token_mru.empty()) {
				// ��Ă��ꂽ�t�@�C�����Ɋg���q���i�[����.
				s_picker.SuggestedFileName(FT_GPF);
			}
			else {
				// �X�g���[�W�t�@�C�����ŋߎg�����t�@�C���̃g�[�N�����瓾��.
				auto s_file{ co_await file_recent_get_async(m_file_token_mru) };
				if (s_file != nullptr) {
					// �t�@�C���^�C�v�� FT_GPF �����肷��.
					if (s_file.FileType() == FT_GPF) {
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
				// �t�@�C���^�C�v�� SVG �����肷��.
				if (s_file.FileType() == FT_SVG) {
					// �}�`�f�[�^�� SVG �Ƃ��ăX�g���[�W�t�@�C���ɔ񓯊��ɏ�������, ���ʂ𓾂�.
					hres = co_await file_dt_write_svg_async(s_file);
				}
				else {
					// �}�`�f�[�^���X�g���[�W�t�@�C���ɔ񓯊��ɏ�������, ���ʂ𓾂�.
					hres = co_await file_write_gpf_async(s_file);
				}
				// �X�g���[�W�t�@�C����j������.
				s_file = nullptr;
			}
			// �s�b�J�[��j������.
			s_picker = nullptr;
		}
		catch (winrt::hresult_error const& e) {
			// �G���[�����������ꍇ, �G���[�R�[�h�����ʂɊi�[����.
			hres = e.code();
		}
		// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		//co_await winrt::resume_foreground(this->Dispatcher());
		// �E�B���h�E�̃J�[�\���𕜌�����.
		Window::Current().CoreWindow().PointerCursor(prev_cur);
		// �X���b�h�R���e�L�X�g�𕜌�����.
		//co_await context;
		// ���ʂ�Ԃ��I������.
		co_return hres;
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
		auto s_file{ co_await file_recent_get_async(m_file_token_mru) };
		if (s_file == nullptr) {
			// �X�g���[�W�t�@�C������̏ꍇ,
			constexpr bool SVG_ALLOWED = true;
			// ���O��t���ăt�@�C���ɔ񓯊��ɕۑ�����
			co_return co_await file_save_as_async(!SVG_ALLOWED);
		}

		auto hres = E_FAIL;
		// �ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
		auto const& prev_cur = file_wait_cursor();
		// �}�`�f�[�^���X�g���[�W�t�@�C���ɔ񓯊��ɏ�������, ���ʂ𓾂�.
		hres = co_await file_write_gpf_async(s_file);
		// ���ʂ𔻒肷��.
		if (hres != S_OK) {
			// ���ʂ� S_OK �łȂ��ꍇ,
			// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			// �ŋߎg�����t�@�C���̃G���[���b�Z�[�W�_�C�A���O��\������.
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			// �u�t�@�C���ɏ������߂܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_ALERT, ERR_WRITE, m_file_token_mru);
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		//co_await context;
		// �E�B���h�E�̃J�[�\���𕜌�����.
		Window::Current().CoreWindow().PointerCursor(prev_cur);
		// ���ʂ�Ԃ��I������.
		co_return hres;
	}

	// �t�@�C�����j���[�́u�㏑���ۑ��v���I�����ꂽ
	void MainPage::file_save_click(IInspectable const&, RoutedEventArgs const&)
	{
		auto _{ file_save_async() };
	}

	// �}�`�f�[�^�� SVG �Ƃ��ăX�g���[�W�t�@�C���ɔ񓯊��ɏ�������.
	// file	�������ݐ�̃t�@�C��
	// �߂�l	�������߂��ꍇ S_OK
	IAsyncOperation<winrt::hresult> MainPage::file_dt_write_svg_async(StorageFile const& s_file)
	{
		using winrt::Windows::Storage::CachedFileManager;
		using winrt::Windows::Storage::FileAccessMode;
		using winrt::Windows::Storage::Provider::FileUpdateStatus;
		using winrt::GraphPaper::implementation::dt_write_svg;
		constexpr char XML_DEC[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" SVG_NEW_LINE;
		constexpr char DOCTYPE[] = "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" SVG_NEW_LINE;

		auto hres = E_FAIL;
		// �R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		// �X���b�h���o�b�N�O���E���h�ɕς���.
		try {
			co_await winrt::resume_background();
			// �t�@�C���X�V�̒x����ݒ肷��.
			CachedFileManager::DeferUpdates(s_file);
			// �X�g���[�W�t�@�C�����J���ă����_���A�N�Z�X�X�g���[���𓾂�.
			auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::ReadWrite) };
			// �����_���A�N�Z�X�X�g���[���̐擪����f�[�^���C�^�[���쐬����.
			auto dt_writer{ DataWriter(ra_stream.GetOutputStreamAt(0)) };
			// XML �錾����������.
			dt_write_svg(XML_DEC, dt_writer);
			// DOCTYPE ����������.
			dt_write_svg(DOCTYPE, dt_writer);
			// �f�[�^���C�^�[�� SVG �J�n�^�O����������.
			file_dt_write_svg_tag(m_sheet_main.m_sheet_size, m_sheet_main.m_sheet_color, m_sheet_dx.m_logical_dpi, m_len_unit, dt_writer);
			// �}�`���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
			for (auto s : m_list_shapes) {
				if (s->is_deleted()) {
					// �����t���O�������Ă���ꍇ,
					// �ȉ��𖳎�����.
					continue;
				}
				s->dt_write_svg(dt_writer);
			}
			// SVG �I���^�O����������.
			dt_write_svg("</svg>" SVG_NEW_LINE, dt_writer);
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
				hres = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// �G���[�����������ꍇ, �G���[�R�[�h�����ʂɊi�[����.
			hres = e.code();
		}
		if (hres != S_OK) {
			// ���ʂ� S_OK �łȂ��ꍇ,
			// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			// �u�t�@�C���ɏ������߂܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_ALERT, ERR_WRITE, s_file.Path());
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// ���ʂ�Ԃ��I������.
		co_return hres;
	}

	// �ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
	CoreCursor MainPage::file_wait_cursor(void) const
	{
		auto const& c_win = Window::Current().CoreWindow();
		auto prev_cur = c_win.PointerCursor();
		c_win.PointerCursor(CUR_WAIT);
		return prev_cur;
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
		auto hres = E_FAIL;
		// �R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ���
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

			tool_write(dt_writer);
			find_text_write(dt_writer);
			//dt_writer.WriteUInt32(static_cast<uint32_t>(m_status_bar));
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_len_unit));
			dt_writer.WriteUInt16(static_cast<uint16_t>(m_color_code));
			dt_writer.WriteUInt16(static_cast<uint16_t>(m_status_bar));
			m_sheet_main.write(dt_writer);
			if (suspend) {
				slist_write<!REDUCE>(m_list_shapes, dt_writer);
				undo_write(dt_writer);
			}
			else if (layout) {
			}
			else {
				slist_write<REDUCE>(m_list_shapes, dt_writer);
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
				hres = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// �G���[�����������ꍇ, 
			// �G���[�R�[�h�����ʂɊi�[����.
			hres = e.code();
		}
		if (hres != S_OK) {
			// ���ʂ� S_OK �łȂ��ꍇ,
			// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			auto cd = this->Dispatcher();
			co_await winrt::resume_foreground(cd);
			// �u�t�@�C���ɏ������߂܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_ALERT, ERR_WRITE, s_file.Path());
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
			m_stack_updt = false;
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		// ���ʂ�Ԃ��I������.
		co_return hres;
	}

}