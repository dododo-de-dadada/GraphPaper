//-------------------------------
// MainPage_thread.cpp
// �E�B���h�E�؂�ւ��̃n���h���[
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �E�B���h�E�̎��s/��~���؂�ւ����.
	void MainPage::thread_activated(IInspectable const&, WindowActivatedEventArgs const& args)
	{
		using winrt::Windows::UI::Core::CoreWindowActivationState;

		// �w��ɂ���E�B���h�E�����ڃN���b�N���ꂽ�ꍇ�̂� PointerActivated,
		// ����ȊO�̏ꍇ�͂��ׂ� CodeActivated �ɂȂ�.
		const auto as = args.WindowActivationState();
		if (as == CoreWindowActivationState::PointerActivated
			|| as == CoreWindowActivationState::CodeActivated) {
		}
		else if (as == CoreWindowActivationState::Deactivated) {
		}
	}

	// �E�B���h�E�̕\��/��\�����؂�ւ����.
	void MainPage::thread_visibility_changed(CoreWindow const& sender, VisibilityChangedEventArgs const& args)
	{
		if (sender == CoreWindow::GetForCurrentThread()) {
			m_window_visible = args.Visible();
			if (m_window_visible) {
				pointer_set();
				page_draw();
			}
		}
	}

}