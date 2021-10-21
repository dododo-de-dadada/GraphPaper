//-------------------------------
// MainPage_pref.cpp
// �ݒ�̕ۑ��ƃ��Z�b�g
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::Foundation::IAsyncOperation;
	using winrt::Windows::Storage::StorageFile;
	using winrt::Windows::UI::Xaml::RoutedEventArgs;

	constexpr wchar_t FILE_NAME[] = L"ji32k7au4a83";	// �A�v���P�[�V�����f�[�^���i�[����t�@�C����

	// �ݒ�f�[�^��ۑ�����t�H���_�[�𓾂�.
	static auto pref_local_folder(void);

	// �ݒ�f�[�^��ۑ�����t�H���_�[�𓾂�.
	static auto pref_local_folder(void)
	{
		using winrt::Windows::Storage::ApplicationData;

		return ApplicationData::Current().LocalFolder();
	}

	// �p�����j���[�́u�ݒ�����Z�b�g�v���I�����ꂽ.
	// �ݒ�f�[�^��ۑ������t�@�C��������ꍇ, ������폜����.
	IAsyncAction MainPage::pref_delete_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::Storage::StorageDeleteOption;

		auto item{ co_await pref_local_folder().TryGetItemAsync(FILE_NAME) };
		if (item != nullptr) {
			auto s_file = item.try_as<StorageFile>();
			if (s_file != nullptr) {
				try {
					co_await s_file.DeleteAsync(StorageDeleteOption::PermanentDelete);
					mfi_pref_delete().IsEnabled(false);
				}
				catch (winrt::hresult_error const&) {
				}
				s_file = nullptr;
			}
			item = nullptr;
		}
	}

	// �ۑ����ꂽ�ݒ�f�[�^��ǂݍ���.
	// �ݒ�f�[�^��ۑ������t�@�C��������ꍇ, �����ǂݍ���.
	// �߂�l	�ǂݍ��߂��� S_OK.
	IAsyncOperation<winrt::hresult> MainPage::pref_load_async(void)
	{
		mfi_pref_delete().IsEnabled(false);
		auto hr = E_FAIL;
		auto item{ co_await pref_local_folder().TryGetItemAsync(FILE_NAME) };
		if (item != nullptr) {
			auto s_file = item.try_as<StorageFile>();
			if (s_file != nullptr) {
				try {
					hr = co_await file_read_async<false, true>(s_file);
				}
				catch (winrt::hresult_error const& e) {
					hr = e.code();
				}
				s_file = nullptr;
				mfi_pref_delete().IsEnabled(true);
			}
			item = nullptr;
		}
		co_return hr;
	}

	// �p�����j���[�́u�ݒ��ۑ��v���I�����ꂽ
	// ���[�J���t�H���_�[�Ƀt�@�C�����쐬��, �ݒ�f�[�^��ۑ�����.
	// s_file	�X�g���[�W�t�@�C��
	IAsyncAction MainPage::pref_save_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::Storage::CreationCollisionOption;

		try {
			auto s_file{ co_await pref_local_folder().CreateFileAsync(FILE_NAME, CreationCollisionOption::ReplaceExisting) };
			if (s_file != nullptr) {
				co_await file_write_gpf_async<false, true>(s_file);
				s_file = nullptr;
				mfi_pref_delete().IsEnabled(true);
			}
		}
		catch (winrt::hresult_error const&) {
		}
		//using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		//auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		//mru_list.Clear();
	}

}