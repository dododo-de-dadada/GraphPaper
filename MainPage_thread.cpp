//-------------------------------
// MainPage_thread.cpp
// ウィンドウ切り替えのハンドラー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ウィンドウが前面に出された
	void MainPage::thread_activated(IInspectable const&, WindowActivatedEventArgs const& args)
	{
		using winrt::Windows::UI::Core::CoreWindowActivationState;

		// 背後にあるウィンドウが直接クリックされた場合のみ PointerActivated,
		// それ以外の場合はすべて CodeActivated になる.
		const auto as = args.WindowActivationState();
		if (as == CoreWindowActivationState::PointerActivated
			|| as == CoreWindowActivationState::CodeActivated) {
			//if (m_activated == false) {
			//	m_activated = true;
				/*
				constexpr wchar_t HEX[] = L"0123456789abcdef";
				wchar_t n[8 + 1];
				GUID guid;
				CoCreateGuid(&guid);
				n[0] = HEX[(guid.Data1 >> 28) & 15];
				n[1] = HEX[(guid.Data1 >> 24) & 15];
				n[2] = HEX[(guid.Data1 >> 20) & 15];
				n[3] = HEX[(guid.Data1 >> 16) & 15];
				n[4] = HEX[(guid.Data1 >> 12) & 15];
				n[5] = HEX[(guid.Data1 >> 8) & 15];
				n[6] = HEX[(guid.Data1 >> 4) & 15];
				n[7] = HEX[guid.Data1 & 15];
				n[8] = L'\0';
				*/
				//}
			set_pointer();
		}
		else if (as == CoreWindowActivationState::Deactivated) {
		}
	}

	// ウィンドウの表示状態が変わった.
	void MainPage::thread_visibility_changed(CoreWindow const& sender, VisibilityChangedEventArgs const& args)
	{
		if (sender == CoreWindow::GetForCurrentThread()) {
			m_window_visible = args.Visible();
			if (m_window_visible) {
				page_draw();
			}
		}
	}

}