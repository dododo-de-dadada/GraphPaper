//-------------------------------
// MainPage_app.cpp
// �A�v���P�[�V�����̒��f�ƍĊJ
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using concurrency::cancellation_token_source;
	using winrt::Windows::ApplicationModel::EnteredBackgroundEventArgs;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionReason;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionResult;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionSession;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedEventArgs;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedReason;
	using winrt::Windows::ApplicationModel::LeavingBackgroundEventArgs;
	using winrt::Windows::ApplicationModel::SuspendingDeferral;
	using winrt::Windows::ApplicationModel::SuspendingEventArgs;
	using winrt::Windows::ApplicationModel::SuspendingOperation;
	using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::Foundation::TypedEventHandler;
	using winrt::Windows::Storage::ApplicationData;
	using winrt::Windows::Storage::CreationCollisionOption;
	using winrt::Windows::Storage::IStorageItem;
	using winrt::Windows::Storage::StorageFile;
	using winrt::Windows::Storage::StorageFolder;

	constexpr wchar_t FILE_NAME[] = L"ji32k7au4a83";	// �A�v���P�[�V�����f�[�^���i�[����t�@�C����

	// �A�v���P�[�V�������o�b�N�O���E���h�Ɉڂ���.
	void MainPage::app_entered_background(IInspectable const&/*sender*/, EnteredBackgroundEventArgs const&/*args*/)
	{
		m_d2d_mutex.lock();
		m_main_d2d.Trim();
		m_sample_dx.Trim();
		m_d2d_mutex.unlock();
	}

	// �A�v���P�[�V�������o�b�N�O���E���h����߂���.
	void MainPage::app_leaving_background(IInspectable const&/*sender*/, LeavingBackgroundEventArgs const&/*args*/)
	{
	}

	// �A�v���P�[�V�����̍ĊJ�̏������s��.
	// �A�v���N���̂Ƃ��͌Ă΂�Ȃ�.
	IAsyncAction MainPage::app_resuming_async(IInspectable const&, IInspectable const&)
	{
		winrt::apartment_context context;

		ShapeText::set_available_fonts(m_main_d2d);

		HRESULT ok = E_FAIL;
		IStorageItem data_storage{ co_await ApplicationData::Current().LocalCacheFolder().TryGetItemAsync(FILE_NAME) };
		if (data_storage != nullptr) {
			StorageFile data_file = data_storage.try_as<StorageFile>();
			if (data_file != nullptr) {
				// �X�g���[�W�t�@�C����񓯊��ɓǂ�.
				try {
					ok = co_await file_read_async<true, false>(data_file);
					// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
					//auto cd = this->Dispatcher();
					//co_await winrt::resume_foreground(cd);
					//file_finish_reading();
				}
				catch (winrt::hresult_error const& e) {
					ok = e.code();
				}
				data_file = nullptr;
			}
			data_storage = nullptr;
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
	}

	// �A�v���P�[�V�����̒��f�̏������s��.
	// args	���f�n���h���[�ɓn���ꂽ����.
	// �߂�l	�Ȃ�
	IAsyncAction MainPage::app_suspending_async(IInspectable const&, SuspendingEventArgs const& args)
	{
		winrt::apartment_context context;
		SuspendingOperation const& sus_operation = args.SuspendingOperation();
		cancellation_token_source cancel_src = cancellation_token_source();
		SuspendingDeferral sus_deferral = sus_operation.GetDeferral();
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

		// �������s�Z�b�V������v����, ���̌��ʂ𓾂�.
		HRESULT ok = E_FAIL;
		switch (co_await ext_session.RequestExtensionAsync()) {
		// �������s�Z�b�V�����������ꂽ�ꍇ.
		case ExtendedExecutionResult::Allowed:
			// �Z�b�V�������L�����Z�������肷��.
			if (cancel_src.get_token().is_canceled()) {
				break;
			}
			try {
				// �L�����Z���ȊO�Ȃ��,
				StorageFile data_file{ co_await ApplicationData::Current().LocalCacheFolder().CreateFileAsync(FILE_NAME, CreationCollisionOption::ReplaceExisting) };
				if (data_file != nullptr) {
					ok = co_await file_write_gpf_async<true, false>(data_file);
					data_file = nullptr;
					ustack_clear();
					slist_clear(m_main_sheet.m_shape_list);
					ShapeText::release_available_fonts();
				}
			}
			catch (winrt::hresult_error const& e) {
				ok = e.code();
			}
			break;
		case ExtendedExecutionResult::Denied:
			break;
		}
		// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		// �X���b�h�ύX��, �Z�b�V���������O�ɂłȂ��ƃ_��.
		//auto cd = this->Dispatcher();
		co_await winrt::resume_foreground(Dispatcher());
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			// �u���������[�N�v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_ALERT, DEBUG_MSG, {});
		}
#endif
		if (ok == S_OK) {
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_clear();
			}
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
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