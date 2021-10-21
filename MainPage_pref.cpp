//-------------------------------
// MainPage_pref.cpp
// 設定の保存とリセット
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

	constexpr wchar_t FILE_NAME[] = L"ji32k7au4a83";	// アプリケーションデータを格納するファイル名

	// 設定データを保存するフォルダーを得る.
	static auto pref_local_folder(void);

	// 設定データを保存するフォルダーを得る.
	static auto pref_local_folder(void)
	{
		using winrt::Windows::Storage::ApplicationData;

		return ApplicationData::Current().LocalFolder();
	}

	// 用紙メニューの「設定をリセット」が選択された.
	// 設定データを保存したファイルがある場合, それを削除する.
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

	// 保存された設定データを読み込む.
	// 設定データを保存したファイルがある場合, それを読み込む.
	// 戻り値	読み込めたら S_OK.
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

	// 用紙メニューの「設定を保存」が選択された
	// ローカルフォルダーにファイルを作成し, 設定データを保存する.
	// s_file	ストレージファイル
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