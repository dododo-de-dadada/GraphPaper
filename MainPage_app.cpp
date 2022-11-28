//-------------------------------
// MainPage_app.cpp
// �A�v���P�[�V�����̒��f�ƍĊJ
// �f�o�b�K�[�Łu���f�v�����邱�Ƃ͂ł��邪�u�ĊJ�v���ł��Ȃ�.
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using concurrency::cancellation_token_source;
	//using winrt::Windows::ApplicationModel::EnteredBackgroundEventArgs;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionReason;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionResult;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionSession;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedEventArgs;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedReason;
	//using winrt::Windows::ApplicationModel::LeavingBackgroundEventArgs;
	using winrt::Windows::ApplicationModel::SuspendingDeferral;
	//using winrt::Windows::ApplicationModel::SuspendingEventArgs;
	using winrt::Windows::ApplicationModel::SuspendingOperation;
	//using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::Foundation::TypedEventHandler;
	using winrt::Windows::Storage::ApplicationData;
	using winrt::Windows::Storage::CreationCollisionOption;
	using winrt::Windows::Storage::IStorageItem;
	//using winrt::Windows::Storage::StorageFile;
	using winrt::Windows::Storage::StorageFolder;

	constexpr wchar_t APP_DATA_FILE[] = L"ji32k7au4a83";	// �A�v���P�[�V�����f�[�^���i�[����t�@�C����

	//------------------------------
	// �A�v���P�[�V�������o�b�N�O���E���h�Ɉڂ���.
	//------------------------------
	void MainPage::app_entered_background(IInspectable const&/*sender*/, EnteredBackgroundEventArgs const&/*args*/)
	{
		m_mutex_draw.lock();
		m_main_sheet.m_d2d.Trim();
		m_prop_sheet.m_d2d.Trim();
		m_mutex_draw.unlock();
	}

	//------------------------------
	// �A�v���P�[�V�������o�b�N�O���E���h����߂���.
	//------------------------------
	void MainPage::app_leaving_background(IInspectable const&/*sender*/, LeavingBackgroundEventArgs const&/*args*/)
	{
	}

	//------------------------------
	// �A�v���P�[�V�����̍ĊJ�̏������s��.
	// �A�v���N���̂Ƃ��͌Ă΂�Ȃ�.
	//------------------------------
	IAsyncAction MainPage::app_resuming_async(IInspectable const&, IInspectable const&)
	{
		winrt::apartment_context context;

		ShapeText::set_available_fonts(m_main_sheet.m_d2d);

		// �A�v���P�[�V�����f�[�^��ǂݍ���.
		IStorageItem app_data_item{ co_await ApplicationData::Current().LocalCacheFolder().TryGetItemAsync(APP_DATA_FILE) };
		if (app_data_item != nullptr) {
			StorageFile app_data_file = app_data_item.try_as<StorageFile>();
			if (app_data_file != nullptr) {
				HRESULT hr = E_FAIL;
				try {
					hr = co_await file_read_async<true, false>(app_data_file);
				}
				catch (winrt::hresult_error const& e) {
					hr = e.code();
				}
				if (hr != S_OK) {
					constexpr wchar_t ERR_LOAD[] = L"str_err_load";	// �ݒ�ǂݍ��݂̃G���[���b�Z�[�W�̃��\�[�X��
					co_await winrt::resume_foreground(Dispatcher());
					message_show(ICON_ALERT, ERR_LOAD, {});
				}
				app_data_file = nullptr;
			}
			app_data_item = nullptr;
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
	}

	//------------------------------
	// �A�v���P�[�V�����̒��f�̏������s��.
	// args	���f�n���h���[�ɓn���ꂽ����.
	//------------------------------
	IAsyncAction MainPage::app_suspending_async(IInspectable const&, SuspendingEventArgs const& args)
	{
		SuspendingOperation const& sus_operation = args.SuspendingOperation();
		cancellation_token_source cancel_src = cancellation_token_source();
		SuspendingDeferral sus_deferral = sus_operation.GetDeferral();	// �������s�Z�b�V�����̒��f	

		// �������s�Z�b�V�����ɗ��R�Ɛ������i�[����.
		ExtendedExecutionSession ext_session = ExtendedExecutionSession();
		ext_session.Reason(ExtendedExecutionReason::SavingData);
		ext_session.Description(L"To save data.");

		// �������s�Z�b�V�������������ꂽ�Ƃ��̃R���[�`����o�^����.
		winrt::event_token ext_token = ext_session.Revoked(
			// RequestExtensionAsync �̒��ł��̃R���[�`���͌Ăяo�����̂�, ��ʊ֐��̃��[�J���ϐ��͎Q�Ƃł��� (���Ԃ�).
			[&cancel_src, &sus_deferral](IInspectable const&, ExtendedExecutionRevokedEventArgs const& args)
			{
				// �g�[�N���̌����L�����Z������.
				cancel_src.cancel();
				switch (args.Reason()) {
				case ExtendedExecutionRevokedReason::Resumed:
					break;
				case ExtendedExecutionRevokedReason::SystemPolicy:
					break;
				}
				if (sus_deferral != nullptr) {
					// ���f������������ OS �ɒʒm���Ȃ����, �A�v���͍Ăђ��f������v���ł��Ȃ�.
					sus_deferral.Complete();
					sus_deferral = nullptr;
				}
			}
		);

		HRESULT hr = E_FAIL;
		// �������s�Z�b�V������v����, ���̌��ʂ𓾂�.
		switch (co_await ext_session.RequestExtensionAsync()) {
		// �������s�Z�b�V�����������ꂽ�ꍇ.
		case ExtendedExecutionResult::Allowed:
			// �Z�b�V�������L�����Z�������肷��.
			if (cancel_src.get_token().is_canceled()) {
				// �L�����Z���Ȃ�Ή������Ȃ�.
				break;
			}
			try {
				// �L�����Z���ȊO�Ȃ��, �A�v���P�[�V�����f�[�^���i�[����X�g���[�W�t�@�C�����쐬����.
				StorageFile app_data_file{
					co_await ApplicationData::Current().LocalCacheFolder().CreateFileAsync(APP_DATA_FILE, CreationCollisionOption::ReplaceExisting) 
				};
				// �X�g���[�W�t�@�C�����쐬�ł����Ȃ�, �A�v���P�[�V�����f�[�^����������.
				if (app_data_file != nullptr) {
					hr = co_await file_write_gpf_async<true, false>(app_data_file);
					app_data_file = nullptr;
				}
			}
			catch (winrt::hresult_error const& e) {
				hr = e.code();
			}
			if (hr != S_OK) {
				// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
				winrt::apartment_context context;
				co_await winrt::resume_foreground(Dispatcher());

				constexpr wchar_t ERR_SAVE[] = L"str_err_save";	// �ݒ�ۑ��̃G���[���b�Z�[�W�̃��\�[�X��
				message_show(ICON_ALERT, ERR_SAVE, {});

				// �ꗗ���\������Ă�Ȃ����.
				if (summary_is_visible()) {
					summary_clear();
				}
				ustack_clear();
				slist_clear(m_main_sheet.m_shape_list);
				slist_clear(m_prop_sheet.m_shape_list);
#if defined(_DEBUG)
				if (debug_leak_cnt != 0) {
					// �u���������[�N�v���b�Z�[�W�_�C�A���O��\������.
					message_show(ICON_DEBUG, DEBUG_MSG, {});
				}
#endif
				sheet_draw();
				// �X���b�h�R���e�L�X�g�𕜌�����.
				co_await context;
			}
			break;
		case ExtendedExecutionResult::Denied:
			break;
		}

		if (ext_session != nullptr) {
			ext_session.Revoked(ext_token);
			ext_session.Close();
			ext_session = nullptr;
		}
		if (sus_deferral != nullptr) {
			// ���f������������ OS �ɒʒm���Ȃ����, 
			// �A�v���͍Ăђ��f������v���ł��Ȃ�.
			sus_deferral.Complete();
			sus_deferral = nullptr;
		}
	}

}