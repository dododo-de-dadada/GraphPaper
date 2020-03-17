//-------------------------------
// MainPage_app.cpp
// �A�v���P�[�V�����̒��f�ƍĊJ
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t FILE_NAME[] = L"ji32k7au4a83";	// �A�v���P�[�V�����f�[�^���i�[����t�@�C����

	// �A�v���P�[�V�����f�[�^��ۑ�����t�H���_�[�𓾂�.
	static auto cache_folder(void);

	// �A�v���P�[�V�����f�[�^��ۑ�����t�H���_�[�𓾂�.
	static auto cache_folder(void)
	{
		using winrt::Windows::Storage::ApplicationData;
		return ApplicationData::Current().LocalCacheFolder();
	}

	// �A�v���P�[�V�������o�b�N�O���E���h�Ɉڂ���.
	void MainPage::app_entered_background(IInspectable const&/*sender*/, EnteredBackgroundEventArgs const&/*args*/)
	{
		std::lock_guard<std::mutex> lock(m_dx_mutex);
		m_page_dx.Trim();
		m_sample_dx.Trim();
	}

	// �A�v���P�[�V�����̒��f�̏������s��.
	// args	���f�n���h���[�ɓn���ꂽ����.
	// �߂�l	�Ȃ�
	IAsyncAction MainPage::app_suspending_async(SuspendingEventArgs const& args)
	{
		using concurrency::cancellation_token_source;
		using winrt::Windows::Foundation::TypedEventHandler;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionReason;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionResult;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionSession;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedEventArgs;
		using winrt::Windows::ApplicationModel::SuspendingDeferral;
		using winrt::Windows::Storage::StorageFolder;
		using winrt::Windows::Storage::CreationCollisionOption;

		// �R���[�`�����ŏ��ɌĂяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		// �������璆�f����Ɋւ�����𓾂�.
		auto const& s_operation = args.SuspendingOperation();
		// ���������𒆒f���邽�߂̃g�[�N���̌��𓾂�.
		auto ct_source = cancellation_token_source();
		// ���f����Ɋւ����񂩂�A�v���̒��f���Ǘ����邽�ߒ��f���� SuspendingDeferral �𓾂�.
		auto s_deferral = s_operation.GetDeferral();
		// �������s���邽�߂̉������s�Z�b�V�����𓾂�.
		auto e_session = ExtendedExecutionSession();
		// SavingData ���������s�Z�b�V�����̖ړI�Ɋi�[����.
		e_session.Reason(ExtendedExecutionReason::SavingData);
		// �uTo save data.�v���������s�Z�b�V�����̗��R�Ɋi�[����.
		e_session.Description(L"To save data.");
		// �������s�Z�b�V�������������ꂽ�Ƃ��̃R���[�`����o�^����.
		// RequestExtensionAsync �̒��ł��̃R���[�`���͌Ăяo�����̂�, 
		// ��ʊ֐��̃��[�J���ϐ��͎Q�Ƃł��� (���Ԃ�).
		auto handler = [&ct_source, &s_deferral](IInspectable const&, ExtendedExecutionRevokedEventArgs const& args)
		{
			using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedReason;
			// �Z�b�V�������������ꂽ�ꍇ,
			// �g�[�N���̌����L�����Z������.
			ct_source.cancel();
			switch (args.Reason()) {
			case ExtendedExecutionRevokedReason::Resumed:
				break;
			case ExtendedExecutionRevokedReason::SystemPolicy:
				break;
			}
			if (s_deferral != nullptr) {
				// ���f�������k���łȂ��ꍇ,
				// ���f��������������.
				// ���f������������ OS �ɒʒm���Ȃ����, 
				// �A�v���͍Ăђ��f������v���ł��Ȃ�.
				s_deferral.Complete();
				s_deferral = nullptr;
			}
		};
		auto s_token = e_session.Revoked(handler);

		// �������s�Z�b�V������v����, ���̌��ʂ𓾂�.
		auto hr = E_FAIL;
		switch (co_await e_session.RequestExtensionAsync()) {
		case ExtendedExecutionResult::Allowed:
			// �������s�Z�b�V�����������ꂽ�ꍇ,
			// �g�[�N���𒲂ׂ�, �L�����Z�����ꂽ�����肷��.
			if (ct_source.get_token().is_canceled()) {
				// �L�����Z�����ꂽ�ꍇ, ���f����.
				break;
			}
			try {
				// �L�����Z���łȂ��ꍇ,
				auto s_file{ co_await cache_folder().CreateFileAsync(FILE_NAME, CreationCollisionOption::ReplaceExisting) };
				if (s_file != nullptr) {
					hr = co_await file_write_suspend_async(s_file);
					s_file = nullptr;
					undo_clear();
					s_list_clear(m_list_shapes);
					ShapeText::release_available_fonts();
				}
			}
			catch (winrt::hresult_error const& e) {
				// �G���[�����������ꍇ, �G���[�R�[�h�����ʂɊi�[����.
				hr = e.code();
			}
			break;
		case ExtendedExecutionResult::Denied:
			break;
		}
		// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		// �X���b�h�ύX��, �Z�b�V���������O�ɂłȂ��ƃ_��.
		co_await winrt::resume_foreground(this->Dispatcher());
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			// �u���������[�N�v���b�Z�[�W�_�C�A���O��\������.
			cd_message_show(ICON_ALERT, L"Memory leak occurs", {});
		}
#endif
		if (hr == S_OK) {
			if (m_summary_visible) {
				summary_clear();
			}
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		if (e_session != nullptr) {
			// �������s�Z�b�V�������k���łȂ��ꍇ,
			// �������R���[�`�����Z�b�V������������, 
			// �Z�b�V���������.
			e_session.Revoked(s_token);
			e_session.Close();
			e_session = nullptr;
		}
		if (s_deferral != nullptr) {
			// ���f�������k���łȂ��ꍇ,
			// ���f��������������.
			// ���f������������ OS �ɒʒm���Ȃ����, 
			// �A�v���͍Ăђ��f������v���ł��Ȃ�.
			s_deferral.Complete();
			s_deferral = nullptr;
		}
	}

	// �A�v���P�[�V�������o�b�N�O���E���h����߂���.
	void MainPage::app_leaving_background(IInspectable const&/*sender*/, LeavingBackgroundEventArgs const&/*args*/)
	{
	}

	// �A�v���P�[�V�����̍ĊJ�̏������s��.
	// �A�v���N���̂Ƃ��͌Ă΂�Ȃ�.
	IAsyncAction MainPage::app_resuming_async(void)
	{
		// �R���[�`�����ŏ��ɌĂяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;

		// �L���ȏ��̖��̔z���ݒ肷��.
		ShapeText::set_available_fonts();

		auto hr = E_FAIL;
		auto item{ co_await cache_folder().TryGetItemAsync(FILE_NAME) };
		if (item != nullptr) {
			auto s_file = item.try_as<StorageFile>();
			if (s_file != nullptr) {
				// �X�g���[�W�t�@�C����񓯊��ɓǂ�.
				try {
					hr = co_await file_read_async(s_file, true, false);
					// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
					co_await winrt::resume_foreground(this->Dispatcher());
					finish_file_read();
				}
				catch (winrt::hresult_error const& e) {
					hr = e.code();
				}
				s_file = nullptr;
			}
			item = nullptr;
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
	}

}