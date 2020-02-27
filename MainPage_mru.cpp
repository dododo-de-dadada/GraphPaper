//-------------------------------
// MainPage_mru.cpp
// �ŋߎg�����t�@�C��
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t ERR_RECENT[] = L"str_err_recent";	// �ŋߎg�����t�@�C���̃G���[���b�Z�[�W�̃��\�[�X��
	constexpr uint32_t MRU_MAX = 25;	// �ŋߎg�������X�g�̍ő吔.
	constexpr wchar_t UNTITLED[] = L"str_untitled";	// ����̃��\�[�X��

	// �ŋߎg�����t�@�C����ǂݍ���.
	// i	�ŋߎg�����t�@�C���̔ԍ� (�ł����߂� 0).
	void MainPage::file_read_recent(const uint32_t i)
	{
		static winrt::event_token save_token;
		static winrt::event_token dont_token;
		static winrt::event_token closed_token;
		static winrt::hstring mru_token;

		//	�ŋߎg�����t�@�C������g�[�N���𓾂�.
		mru_token = mru_get_token(i);
		if (mru_token.empty()) {
			// �v�f����̏ꍇ,
			// �ŋߎg�����t�@�C���̃G���[���b�Z�[�W�_�C�A���O��\������.
			cd_message_show(ERR_RECENT, to_hstring(i + 1));
			return;
		}
		if (m_stack_push == false) {
			// ����X�^�b�N�̍X�V�t���O���Ȃ��ꍇ,
			// �}�`�f�[�^�͕ύX����Ă��Ȃ��̂�,
			// �ŋߎg�����t�@�C����񓯊��ɓǂݍ���.
			auto _{ file_read_recent_async(mru_token) };
			return;
		}
		// �ۑ��m�F�_�C�A���O�����\�[�X���烍�[�h����.
		cd_load_conf_save();
		save_token = cd_conf_save().PrimaryButtonClick(
			[this](auto, auto)
			{
				// �t�@�C���ɔ񓯊��ɕۑ�����, �ŋߎg�����t�@�C����񓯊��ɓǂݍ���.
				auto _{ file_save_and_read_recent_async(mru_token) };
			}
		);
		dont_token = cd_conf_save().SecondaryButtonClick(
			[this](auto, auto)
			{
				// �ŋߎg�����t�@�C����񓯊��ɓǂݍ���.
				auto _{ file_read_recent_async(mru_token) };
			}
		);
		closed_token = cd_conf_save().Closed(
			[this](auto, auto)
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
		{ auto _{ cd_conf_save().ShowAsync() }; }
	}

	//	�ŋߎg�����t�@�C����񓯊��ɓǂݍ���.
	//	token	�ŋߎg�����t�@�C���̃g�[�N��
	//	�߂�l	�Ȃ�
	IAsyncAction MainPage::file_read_recent_async(const winrt::hstring token)
	{
		auto hr = E_FAIL;
		//	�ҋ@�J�[�\����\��, �\������O�̃J�[�\���𓾂�.
		auto const p_cur = file_wait_cursor();
		//	�R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		//	�X�g���[�W�t�@�C�����ŋߎg�����t�@�C���̃g�[�N�����瓾��.
		auto s_file{ co_await mru_get_file(token) };
		if (s_file != nullptr) {
			//	�X�g���[�W�t�@�C������łȂ��ꍇ,
			//	�X�g���[�W�t�@�C������񓯊��ɓǂݍ���.
			hr = co_await file_read_async(s_file);
			//	�X�g���[�W�t�@�C����j������.
			s_file = nullptr;
			finish_file_read();
			co_await winrt::resume_foreground(this->Dispatcher());
		}
		else {
			//	�X�g���[�W�t�@�C������̏ꍇ,
			//	�X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			//	�ŋߎg�����t�@�C���̃G���[���b�Z�[�W�_�C�A���O��\������.
			co_await winrt::resume_foreground(this->Dispatcher());
			cd_message_show(ERR_RECENT, token);
		}

		Window::Current().CoreWindow().PointerCursor(p_cur);
		// �X���b�h�R���e�L�X�g�𕜌�����.
		// �E�B���h�E�̃J�[�\���𕜌�����.
		co_await context;
	}

	//	�t�@�C���ɔ񓯊��ɕۑ�����, �ŋߎg�����t�@�C����񓯊��ɓǂݍ���.
	//	token	�ŋߎg�����t�@�C���̃g�[�N��
	//	�߂�l	�Ȃ�
	IAsyncAction MainPage::file_save_and_read_recent_async(winrt::hstring const& token)
	{
		//	�t�@�C���ɔ񓯊��ɕۑ�����
		if (co_await file_save_async() == S_OK) {
			//	�ۑ��ł����ꍇ,
			//	�ŋߎg�����t�@�C����񓯊��ɓǂݍ���.
			co_await file_read_recent_async(token);
		}
	}

	// �t�@�C�����j���[�́u�ŋߎg�����t�@�C�� 1�v���I�����ꂽ
	void MainPage::mfi_recent_files_1_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	�ŋߎg�����t�@�C�� (0) ��ǂݍ���.
		file_read_recent(0);
	}

	// �t�@�C�����j���[�́u�ŋߎg�����t�@�C�� 2�v���I�����ꂽ
	void MainPage::mfi_recent_files_2_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	�ŋߎg�����t�@�C�� (1) ��ǂݍ���.
		file_read_recent(1);
	}

	// �t�@�C�����j���[�́u�ŋߎg�����t�@�C�� 3�v���I�����ꂽ
	void MainPage::mfi_recent_files_3_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	�ŋߎg�����t�@�C�� (2) ��ǂݍ���.
		file_read_recent(2);
	}

	// �t�@�C�����j���[�́u�ŋߎg�����t�@�C�� 4�v���I�����ꂽ
	void MainPage::mfi_recent_files_4_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	�ŋߎg�����t�@�C�� (3) ��ǂݍ���.
		file_read_recent(3);
	}

	//	�t�@�C�����j���[�́u�ŋߎg�����t�@�C�� 5�v���I�����ꂽ
	void MainPage::mfi_recent_files_5_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	�ŋߎg�����t�@�C�� (4) ��ǂݍ���.
		file_read_recent(4);
	}

	//	�ŋߎg�����t�@�C���̃g�[�N������X�g���[�W�t�@�C���𓾂�.
	//	token	�ŋߎg�����t�@�C���̃g�[�N��
	//	�߂�l	�X�g���[�W�t�@�C��
	IAsyncOperation<StorageFile> MainPage::mru_get_file(const winrt::hstring token)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;

		//	�R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;

		//	�X�g���[�W�t�@�C���Ƀk�����i�[����.
		StorageFile s_file = nullptr;
		try {
			if (token.empty() == false) {
				//	�g�[�N������łȂ��ꍇ,
				//	�g�[�N������X�g���[�W�t�@�C���𓾂�.
				auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
				s_file = co_await mru_list.GetFileAsync(token);
			}
		}
		catch (winrt::hresult_error) {
		}
		//	�X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		co_await winrt::resume_foreground(this->Dispatcher());
		//	�擾�ł��Ă��o���Ȃ��Ă��ŋߎg�������X�g�̏��Ԃ͓���ւ��̂�,
		//	�ŋߎg�����t�@�C�����j�����X�V����.
		mru_update_menu_items();
		//	�X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		//	�X�g���[�W�t�@�C����Ԃ�.
		co_return s_file;
	}

	//	�X�g���[�W�t�@�C�����ŋߎg�����t�@�C���ɓo�^����.
	//	s_file	�X�g���[�W�t�@�C��
	//	�߂�l	�Ȃ�
	//	�ŋߎg�����t�@�C�����j���[�ƃE�B���h�E�^�C�g�����X�V�����.
	//	�X�g���[�W�t�@�C�����k���̏ꍇ, �ŋߎg�����t�@�C���͂��̂܂܂�, 
	//	�E�B���h�E�^�C�g���ɖ��肪�i�[�����.
	void MainPage::mru_add_file(StorageFile const& s_file)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::UI::ViewManagement::ApplicationView;
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		if (s_file != nullptr) {
			m_mru_token = mru_list.Add(s_file, s_file.Path());
			ApplicationView::GetForCurrentView().Title(s_file.Name());
		}
		else {
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			ApplicationView::GetForCurrentView().Title(r_loader.GetString(UNTITLED));
		}
		//	�ŋߎg�����t�@�C�����j�����X�V����.
		mru_update_menu_items();
	}

	//	�ŋߎg�����t�@�C������g�[�N���𓾂�.
	//	i	�ŋߎg�����t�@�C���̔ԍ� (�ł����߂� 0).
	//	�߂�l	i�@�Ԗڂ̃t�@�C���̃g�[�N��
	winrt::hstring MainPage::mru_get_token(const uint32_t i)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		auto const& mru_ent = mru_list.Entries();
		//	�ŋߎg�����t�@�C���̔ԍ��ƍŋߎg�������X�g�̗v�f�����r����.
		if (i < mru_ent.Size()) {
			//	�ԍ����v�f����菬�����ꍇ,
			//	�ŋߎg�������X�g����v�f�𓾂�.
			AccessListEntry item[1];
			mru_ent.GetMany(i, item);
			//	�v�f�̃g�[�N����Ԃ�.
			return item[0].Token;
		}
		// ��̕������Ԃ�.
		return {};
	}

	// �ŋߎg�����t�@�C�����j�����X�V����.
	void MainPage::mru_update_menu_items(void)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		auto const& mru_ent = mru_list.Entries();
		const auto ent_size = mru_ent.Size();
		AccessListEntry items[MRU_MAX];
		winrt::hstring data[MRU_MAX];
		mru_ent.GetMany(0, items);
		for (uint32_t i = MRU_MAX; i > 0; i--) {
			if (ent_size >= i) {
				data[i - 1] = winrt::to_hstring(i) + L" " + items[i - 1].Metadata;
			}
			else {
				data[i - 1] = winrt::to_hstring(i);
			}
		}
		mfi_recent_files_1().Text(data[0]);
		mfi_recent_files_2().Text(data[1]);
		mfi_recent_files_3().Text(data[2]);
		mfi_recent_files_4().Text(data[3]);
		mfi_recent_files_5().Text(data[4]);
	}

}