//-------------------------------
// MainPage_app.cpp
// �A�v���P�[�V�����̒��f�ƍĊJ
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	static winrt::hstring app_data;	// �A�v���P�[�V�����f�[�^���i�[����t�@�C����
	constexpr auto APP_DATA = L"file_app_data";	// // �A�v���P�[�V�����f�[�^���i�[����t�@�C�����̃��\�[�X��

	//	�A�v���P�[�V�����f�[�^��ۑ�����t�H���_�[�𓾂�.
	static auto app_data_folder(void);

	//	�A�v���P�[�V�����f�[�^��ۑ�����t�H���_�[�𓾂�.
	static auto app_data_folder(void)
	{
		using winrt::Windows::Storage::ApplicationData;
		return ApplicationData::Current().LocalFolder();
	}

	//	�A�v���P�[�V�������o�b�N�O���E���h�Ɉڂ���.
	void MainPage::app_entered_background(IInspectable const&/*sender*/, EnteredBackgroundEventArgs const&/*args*/)
	{
		app_data = unbox_value<winrt::hstring>(Resources().Lookup(box_value(APP_DATA)));
		std::lock_guard<std::mutex> lock(m_dx_mutex);
		m_page_dx.Trim();
		m_sample_dx.Trim();
	}

	//	�A�v���P�[�V������񓯊��ɉ�������.
	//	s_op	���f����
	//	�߂�l	�Ȃ�
	//	���f�����, �A�v�����f�̃n���h���[�̈���
	//	SuspendingEventArgs ���瓾����.
	IAsyncAction MainPage::app_extended_session_async(SuspendingOperation const& s_op)
	{
		using concurrency::cancellation_token_source;
		using winrt::Windows::Foundation::TypedEventHandler;// <winrt::Windows::Foundation::IInspectable const&, winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedEventArgs const&>;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionReason;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionResult;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionSession;
		using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedEventArgs;
		using winrt::Windows::ApplicationModel::SuspendingDeferral;
		using winrt::Windows::Storage::StorageFolder;
		using winrt::Windows::Storage::CreationCollisionOption;
		//static cancellation_token_source ct_source;
		//static SuspendingDeferral s_deferral;
		//static winrt::event_token s_token;

		//	�R���[�`�����ŏ��ɌĂяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		//	���������𒆒f���邽�߂̃g�[�N���̌��𓾂�.
		auto ct_source = cancellation_token_source();
		//	�A�v���̒��f���Ǘ����邽�ߒ��f���� SuspendingDeferral �𒆒f���삩�瓾��.
		auto s_deferral = s_op.GetDeferral();
		//	�������s���邽�߂̃Z�b�V�����𓾂�.
		auto session = ExtendedExecutionSession();
		//	SavingData ���Z�b�V�����̖ړI�Ɋi�[����.
		session.Reason(ExtendedExecutionReason::SavingData);
		//	�uTo save data.�v���Z�b�V�����̗��R�Ɋi�[����.
		session.Description(L"To save data.");
		//	�Z�b�V�������������ꂽ�Ƃ��̃R���[�`����o�^����.
		//	RequestExtensionAsync �̒��ł��̃R���[�`���͌Ăяo�����̂�, 
		//	��ʊ֐��̃��[�J���ϐ��͎Q�Ƃł���, �͂�.
		auto handler = [&ct_source, &s_deferral](IInspectable const&, ExtendedExecutionRevokedEventArgs const& args)
		{
			using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedReason;
			//	�Z�b�V�������������ꂽ�ꍇ,
			//	�g�[�N���̌����L�����Z������.
			ct_source.cancel();
			switch (args.Reason()) {
			case ExtendedExecutionRevokedReason::Resumed:
				break;
			case ExtendedExecutionRevokedReason::SystemPolicy:
				break;
			}
			if (s_deferral != nullptr) {
				//	���f�������k���łȂ��ꍇ,
				//	���f��������������.
				//	���f������������ OS �ɒʒm���Ȃ����, 
				//	�A�v���͍Ăђ��f������v���ł��Ȃ�.
				s_deferral.Complete();
				s_deferral = nullptr;
			}
		};
		auto s_token = session.Revoked(handler);

		//	�Z�b�V������v����, ���̌��ʂ𓾂�.
		auto hr = E_FAIL;
		switch (co_await session.RequestExtensionAsync()) {
		case ExtendedExecutionResult::Allowed:
			//	�Z�b�V�����������ꂽ�ꍇ,
			//	�g�[�N���𒲂ׂ�, �L�����Z�����ꂽ�����肷��.
			if (ct_source.get_token().is_canceled()) {
				//	�L�����Z�����ꂽ�ꍇ, ���f����.
				break;
			}
			try {
				/*
				//	�L�����Z���łȂ��ꍇ,
				//	�A�v���P�[�V�����f�[�^���i�[���邽�߂̃��[�J���t�H���_�[�𓾂�.
				//	LocalFolder �́u�L���Ȕ͈͊O�̃f�[�^�ɃA�N�Z�X���悤�Ƃ��܂����v�����G���[���N������,
				//	�Ƃ肠����, �t�@�C�����쐬���ĕۑ��͂ł��Ă���.
				auto l_folder{ app_data_folder() };
				//	�X�g���[�W�t�@�C�������[�J���t�H���_�ɍ쐬����.
				auto s_file{ co_await l_folder.CreateFileAsync(app_data, CreationCollisionOption::ReplaceExisting) };
				//	�}�`�f�[�^���X�g���[�W�t�@�C���ɔ񓯊��ɏ�����, ���ʂ𓾂�.
				hr = co_await file_write_gpf_async(s_file, true);
				//	�t�@�C����j������.
				s_file = nullptr;
				//	�t�H���_�[��j������.
				l_folder = nullptr;
				//	����X�^�b�N����������.
				undo_clear();
				//	�}�`���X�g����������.
				s_list_clear(m_list_shapes);
				*/
			}
			catch (winrt::hresult_error const& e) {
				// �G���[�����������ꍇ, �G���[�R�[�h�����ʂɊi�[����.
				hr = e.code();
			}
			break;
		case ExtendedExecutionResult::Denied:
			break;
		}
		//	�X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		//	�X���b�h�ύX��, �Z�b�V���������O�ɂłȂ��ƃ_��.
		co_await winrt::resume_foreground(this->Dispatcher());
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			cd_message_show(L"icon_alert", L"Memory leak occurs", {});
		}
#endif
		if (hr == S_OK) {
			if (m_summary_visible) {
				summary_clear();
			}
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		if (session != nullptr) {
			//	�Z�b�V�������k���łȂ��ꍇ,
			//	�������R���[�`�����Z�b�V������������, 
			//	�Z�b�V���������.
			session.Revoked(s_token);
			session.Close();
			session = nullptr;
		}
		if (s_deferral != nullptr) {
			//	���f�������k���łȂ��ꍇ,
			//	���f��������������.
			//	���f������������ OS �ɒʒm���Ȃ����, 
			//	�A�v���͍Ăђ��f������v���ł��Ȃ�.
			s_deferral.Complete();
			s_deferral = nullptr;
		}
	}

	// �A�v���P�[�V�������o�b�N�O���E���h����߂���.
	void MainPage::app_leaving_background(IInspectable const&/*sender*/, LeavingBackgroundEventArgs const&/*args*/)
	{
	}

	// �A�v���P�[�V�������ĊJ���ꂽ.
	// �A�v���N���̂Ƃ��͌Ă΂�Ȃ�.
	void MainPage::app_resuming(IInspectable const&, IInspectable const& /*object*/)
	{
		// �A�v���P�[�V������񓯊��ɍĊJ����.
		auto _{ app_resuming_async() };
	}

	// �A�v���P�[�V������񓯊��ɍĊJ����.
	IAsyncAction MainPage::app_resuming_async(void)
	{
		// �R���[�`�����ŏ��ɌĂяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		// E_FAIL �����ʂɊi�[����.
		auto hr = E_FAIL;
		try {
			// �A�v���p�ɍ쐬���ꂽ���[�J���f�[�^�t�H���_�[�𓾂�.
			auto l_folder{ app_data_folder() };
			auto s_file{ co_await l_folder.GetFileAsync(app_data) };
			co_await file_read_async(s_file, true);
			s_file = nullptr;
			l_folder = nullptr;
			// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			co_await winrt::resume_foreground(this->Dispatcher());
			finish_file_read();
		}
		catch (winrt::hresult_error const& e) {
			// �G���[�����������ꍇ, �G���[�R�[�h�����ʂɊi�[����.
			hr = e.code();
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
	}

	// �A�v���P�[�V���������f���ꂽ.
	// �A�v���̌��݂̏�Ԃ�ۑ�����.
	// EnteredBackground ����.
	void MainPage::app_suspending(IInspectable const&, SuspendingEventArgs const& args)
	{
		// �������璆�f����𓾂�.
		// ����ꂽ���f������w�肵��,
		// �A�v���P�[�V������񓯊��ɉ�������.
		auto _{ app_extended_session_async(args.SuspendingOperation()) };
	}

}