//-------------------------------
// MainPage_thread.cpp
// ウィンドウ切り替えのハンドラー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ウィンドウの実行/停止が切り替わった.
	void MainPage::thread_activated(IInspectable const&, WindowActivatedEventArgs const& args)
	{
		using winrt::Windows::UI::Core::CoreWindowActivationState;

		// 背後にあるウィンドウが直接クリックされた場合のみ PointerActivated,
		// それ以外の場合はすべて CodeActivated になる.
		const auto as = args.WindowActivationState();
		if (as == CoreWindowActivationState::PointerActivated
			|| as == CoreWindowActivationState::CodeActivated) {
		}
		else if (as == CoreWindowActivationState::Deactivated) {
		}
	}

	// ウィンドウの表示/非表示が切り替わった.
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