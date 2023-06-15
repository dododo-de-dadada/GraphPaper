//-------------------------------
// MainPage_app.cpp
// �A�v���P�[�V�����̒��f�ƍĊJ
// Windows11 �ł̓f�o�b�K�[�Łu���f�v�����邱�Ƃ͂ł��邪�u�ĊJ�v���ł��Ȃ�.
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using concurrency::cancellation_token_source;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionReason;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionResult;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionSession;
	using winrt::Windows::ApplicationModel::ExtendedExecution::ExtendedExecutionRevokedReason;
	using winrt::Windows::ApplicationModel::SuspendingDeferral;
	using winrt::Windows::ApplicationModel::SuspendingOperation;
	using winrt::Windows::Foundation::TypedEventHandler;
	using winrt::Windows::Storage::ApplicationData;
	using winrt::Windows::Storage::CreationCollisionOption;
	using winrt::Windows::Storage::IStorageItem;
	using winrt::Windows::Storage::StorageFolder;

	constexpr wchar_t APP_DATA_FILE[] = L"ji32k7au4a83";	// �A�v���P�[�V�����f�[�^���i�[����t�@�C����

	//------------------------------
	// �A�v���P�[�V�������o�b�N�O���E���h�Ɉڂ���.
	//------------------------------
	void MainPage::app_entered_background(IInspectable const&/*sender*/, EnteredBackgroundEventArgs const&/*args*/)
	{
		m_mutex_draw.lock();
		m_main_d2d.Trim();
		m_dialog_d2d.Trim();
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

		ShapeText::set_available_fonts();

		// �A�v���P�[�V�����f�[�^��ǂݍ���.
		IStorageItem app_data_item{
			co_await ApplicationData::Current().LocalCacheFolder().TryGetItemAsync(APP_DATA_FILE) 
		};
		if (app_data_item != nullptr) {
			StorageFile app_data_file = app_data_item.try_as<StorageFile>();
			if (app_data_file != nullptr) {
				constexpr bool RESUME = true;
				constexpr bool SETTING_ONLY = true;
				co_await file_read_gpf_async<RESUME, !SETTING_ONLY>(app_data_file);
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
			[&cancel_src, &sus_deferral](auto const&, auto const& args)
			// IInspectable const&, ExtendedExecutionRevokedEventArgs const& args
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
		switch (co_await ext_session.RequestExtensionAsync()) {
		// �������s�Z�b�V�����������ꂽ�ꍇ.
		case ExtendedExecutionResult::Allowed:
			// �Z�b�V�������L�����Z���ȊO�Ȃ�
			if (!cancel_src.get_token().is_canceled()) {
				// �A�v���P�[�V�����f�[�^���i�[����X�g���[�W�t�@�C�����쐬����.
				StorageFile app_data_file{
					co_await ApplicationData::Current().LocalCacheFolder().CreateFileAsync(APP_DATA_FILE, CreationCollisionOption::ReplaceExisting) 
				};
				// �t�@�C�����쐬�ł����Ȃ�, �A�v���P�[�V�����f�[�^����������.
				if (app_data_file != nullptr) {
					co_await file_write_gpf_async<true, false>(app_data_file);
					app_data_file = nullptr;
				}
				// �t�@�C�����쐬�ł��Ȃ��Ȃ�, �G���[���b�Z�[�W��\������.
				else {
					message_show(ICON_ALERT, L"str_err_save", {});
				}
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