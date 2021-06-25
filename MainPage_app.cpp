//-------------------------------
// MainPage_appl.cpp
// �A�v���P�[�V�����̒��f�ƍĊJ
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t FILE_NAME[] = L"ji32k7au4a83";	// �A�v���P�[�V�����f�[�^���i�[����t�@�C����

	// �A�v���P�[�V�����f�[�^��ۑ�����t�H���_�[�𓾂�.
	static auto app_cache_folder(void);

	// �A�v���P�[�V�����f�[�^��ۑ�����t�H���_�[�𓾂�.
	static auto app_cache_folder(void)
	{
		return winrt::Windows::Storage::ApplicationData::Current().LocalCacheFolder();
	}

	// �A�v���P�[�V�������o�b�N�O���E���h�Ɉڂ���.
	void MainPage::app_entered_background(IInspectable const&/*sender*/, EnteredBackgroundEventArgs const&/*args*/)
	{
		m_dx_mutex.lock();
		m_sheet_dx.Trim();
		m_sample_dx.Trim();
		m_dx_mutex.unlock();
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

		ShapeText::set_available_fonts();

		auto hres = E_FAIL;
		auto item{ co_await app_cache_folder().TryGetItemAsync(FILE_NAME) };
		if (item != nullptr) {
			auto s_file = item.try_as<StorageFile>();
			if (s_file != nullptr) {
				// �X�g���[�W�t�@�C����񓯊��ɓǂ�.
				try {
					hres = co_await file_read_async(s_file, true, false);
					// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
					//auto cd = this->Dispatcher();
					//co_await winrt::resume_foreground(cd);
					//file_finish_reading();
				}
				catch (winrt::hresult_error const& e) {
					hres = e.code();
				}
				s_file = nullptr;
			}
			item = nullptr;
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
	}

	// �A�v���P�[�V�����̒��f�̏������s��.
	// args	���f�n���h���[�ɓn���ꂽ����.
	// �߂�l	�Ȃ�
	IAsyncAction MainPage::app_suspending_async(IInspectable const&, SuspendingEventArgs const& args)
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

		winrt::apartment_context context;
		auto const& s_operation = args.SuspendingOperation();
		auto ct_source = cancellation_token_source();
		auto s_deferral = s_operation.GetDeferral();
		auto e_session = ExtendedExecutionSession();
		e_session.Reason(ExtendedExecutionReason::SavingData);
		e_session.Description(L"To save data.");
		// �������s�Z�b�V�������������ꂽ�Ƃ��̃R���[�`����o�^����.
		// RequestExtensionAsync �̒��ł��̃R���[�`���͌Ăяo�����̂�, ��ʊ֐��̃��[�J���ϐ��͎Q�Ƃł��� (���Ԃ�).
		auto handler = [&ct_source, &s_deferral](IInspectable const&, ExtendedExecutionRevokedEventArgs const& args)
		{
			using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedReason;
			// �g�[�N���̌����L�����Z������.
			ct_source.cancel();
			switch (args.Reason()) {
			case ExtendedExecutionRevokedReason::Resumed:
				break;
			case ExtendedExecutionRevokedReason::SystemPolicy:
				break;
			}
			if (s_deferral != nullptr) {
				// ���f������������ OS �ɒʒm���Ȃ����, �A�v���͍Ăђ��f������v���ł��Ȃ�.
				s_deferral.Complete();
				s_deferral = nullptr;
			}
		};
		auto s_token = e_session.Revoked(handler);

		// �������s�Z�b�V������v����, ���̌��ʂ𓾂�.
		auto hres = E_FAIL;
		switch (co_await e_session.RequestExtensionAsync()) {
		case ExtendedExecutionResult::Allowed:
			// �������s�Z�b�V�����������ꂽ�ꍇ,
			if (ct_source.get_token().is_canceled()) {
				// �L�����Z�����ꂽ�ꍇ, ���f����.
				break;
			}
			try {
				// �L�����Z���łȂ��ꍇ,
				auto s_file{ co_await app_cache_folder().CreateFileAsync(FILE_NAME, CreationCollisionOption::ReplaceExisting) };
				if (s_file != nullptr) {
					hres = co_await file_write_gpf_async(s_file, true, false);
					s_file = nullptr;
					ustack_clear();
					slist_clear(m_list_shapes);
					ShapeText::release_available_fonts();
				}
			}
			catch (winrt::hresult_error const& e) {
				hres = e.code();
			}
			break;
		case ExtendedExecutionResult::Denied:
			break;
		}
		// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		// �X���b�h�ύX��, �Z�b�V���������O�ɂłȂ��ƃ_��.
		auto cd = this->Dispatcher();
		co_await winrt::resume_foreground(cd);
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			// �u���������[�N�v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_ALERT, DEBUG_MSG, {});
		}
#endif
		if (hres == S_OK) {
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_clear();
			}
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		if (e_session != nullptr) {
			e_session.Revoked(s_token);
			e_session.Close();
			e_session = nullptr;
		}
		if (s_deferral != nullptr) {
			// ���f������������ OS �ɒʒm���Ȃ����, 
			// �A�v���͍Ăђ��f������v���ł��Ȃ�.
			s_deferral.Complete();
			s_deferral = nullptr;
		}
	}

}