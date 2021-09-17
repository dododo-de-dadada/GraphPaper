//-------------------------------
// MainPage_thread.cpp
// ウィンドウ切り替えのハンドラー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

// まず, thread_visibility_changed が呼ばれ, つぎに thread_activated


using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ウィンドウの実行/停止が切り替わった.
	void MainPage::thread_activated(IInspectable const&, WindowActivatedEventArgs const& args)
	{
		using winrt::Windows::UI::Core::CoreWindowActivationState;

		// 背後にあるウィンドウが直接クリックされた場合のみ PointerActivated,
		// それ以外の場合はすべて CodeActivated になる.
		const auto a_state = args.WindowActivationState();
		if (a_state == CoreWindowActivationState::PointerActivated ||
			a_state == CoreWindowActivationState::CodeActivated) {
			if (!m_thread_activated) {
				m_thread_activated = true;

			}
			// クリップボードに受け入れ可能なフォーマットを設定する.
			// クリップボードは, バックグラウンドではからはアクセスできない.
			// デバッグなしで実行したとき, MainPage のコンストラクタで Clipboard::GetContent を呼ぶと, 
			// アプリケーションの起動に失敗する.
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

	// ウィンドウの表示/非表示が切り替わった.
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