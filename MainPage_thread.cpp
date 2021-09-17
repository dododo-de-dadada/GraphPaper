//-------------------------------
// MainPage_thread.cpp
// �E�B���h�E�؂�ւ��̃n���h���[
//-------------------------------
#include "pch.h"
#include "MainPage.h"

// �܂�, thread_visibility_changed ���Ă΂�, ���� thread_activated


using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �E�B���h�E�̎��s/��~���؂�ւ����.
	void MainPage::thread_activated(IInspectable const&, WindowActivatedEventArgs const& args)
	{
		using winrt::Windows::UI::Core::CoreWindowActivationState;

		// �w��ɂ���E�B���h�E�����ڃN���b�N���ꂽ�ꍇ�̂� PointerActivated,
		// ����ȊO�̏ꍇ�͂��ׂ� CodeActivated �ɂȂ�.
		const auto a_state = args.WindowActivationState();
		if (a_state == CoreWindowActivationState::PointerActivated ||
			a_state == CoreWindowActivationState::CodeActivated) {
			if (!m_thread_activated) {
				m_thread_activated = true;

			}
			// �N���b�v�{�[�h�Ɏ󂯓���\�ȃt�H�[�}�b�g��ݒ肷��.
			// �N���b�v�{�[�h��, �o�b�N�O���E���h�ł͂���̓A�N�Z�X�ł��Ȃ�.
			// �f�o�b�O�Ȃ��Ŏ��s�����Ƃ�, MainPage �̃R���X�g���N�^�� Clipboard::GetContent ���ĂԂ�, 
			// �A�v���P�[�V�����̋N���Ɏ��s����.
			using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;
			using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;

			const auto& dp_view = Clipboard::GetContent();
			dp_view.SetAcceptedFormatId(CLIPBOARD_SHAPES);
			dp_view.SetAcceptedFormatId(StandardDataFormats::Text());
			dp_view.SetAcceptedFormatId(StandardDataFormats::Bitmap());
			dp_view.SetAcceptedFormatId(StandardDataFormats::StorageItems());
			//dp_view.SetAcceptedFormatId(CLIPBOARD_TIFF);
			if (dp_view.Contains(CLIPBOARD_SHAPES) ||
				dp_view.Contains(StandardDataFormats::Text()) ||
				dp_view.Contains(StandardDataFormats::Bitmap())) {
				//|| dp_view.Contains(CLIPBOARD_TIFF)) {
				mfi_xcvd_paste().IsEnabled(true);
			}
			else {
				mfi_xcvd_paste().IsEnabled(false);
			}
		}
		else if (a_state == CoreWindowActivationState::Deactivated) {
		}
	}

	// �E�B���h�E�̕\��/��\�����؂�ւ����.
	void MainPage::thread_visibility_changed(CoreWindow const& sender, VisibilityChangedEventArgs const& args)
	{
		if (sender == CoreWindow::GetForCurrentThread()) {
			m_thread_win_visible = args.Visible();
			if (m_thread_win_visible) {
				event_set_cursor();
				sheet_draw();
			}
		}
	}

}