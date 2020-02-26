//-------------------------------
// MainPage_xcvd.cpp
// 切り取りとコピー, 貼り付け, 削除
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 選択された図形をクリップボードに非同期に保存する.
	// X	切り取りフラグ. フラグが立っている場合は切り取り, ない場合はコピー.
	constexpr uint32_t CUT = 0;
	constexpr uint32_t COPY = 1;
	template <uint32_t X>
	IAsyncAction MainPage::clipboard_copy_async(void)
	{
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;
		using winrt::Windows::Storage::Streams::DataWriter;
		using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;

		//	選択された図形のリストを得る.
		S_LIST_T sel_list;
		s_list_select<Shape>(m_list_shapes, sel_list);
		if (sel_list.empty()) {
			//	得られたリストが空の場合,
			//	終了する.
			return;
		}
		if constexpr (X == CUT) {
			// フラグが切り取りの場合,
			// 選択された図形を図形リストから削除する.
			for (auto s : sel_list) {
				if (m_summary_visible) {
					summary_remove(s);
				}
				undo_push_remove(s);
			}
			undo_push_null();
			s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
			set_page_panle_size();
			draw_page();
		}
		//	コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;
		//	出力ストリームを作成して, データライターを得る.
		auto ra_stream{ InMemoryRandomAccessStream() };
		auto out_stream{ ra_stream.GetOutputStreamAt(0) };
		auto dt_writer{ DataWriter(out_stream) };
		//	選択された図形のリストをデータライターに書き込む.
		constexpr bool REDUCED = true;
		s_list_write<REDUCED>(sel_list, dt_writer);
		//	書き込んだらリストは破棄する.
		sel_list.clear();
		//	書き込んだデータを出力ストリームに格納し, 格納したバイト数を得る.
		auto n_byte{ co_await dt_writer.StoreAsync() };
		//	図形リストから文字列を得る.
		auto text = s_list_text(m_list_shapes);
		//	スレッドをメインページの UI スレッドに変える.
		co_await winrt::resume_foreground(this->Dispatcher());
		if (n_byte > 0) {
			//	格納したバイト数が 0 を超える場合,
			//	データパッケージを作成する.
			auto dt_pkg{ DataPackage() };
			//	ストリームをデータパッケージに格納する.
			dt_pkg.RequestedOperation(DataPackageOperation::Copy);
			dt_pkg.SetData(FMT_DATA, winrt::box_value(ra_stream));
			if (text.empty() == false) {
				//	文字列が空でない場合,
				//	文字列をデータパッケージに格納する.
				dt_pkg.SetText(text);
			}
			//	データパッケージをクリップボードに格納する.
			Clipboard::SetContent(dt_pkg);
		}
		//	データライターを閉じる.
		dt_writer.Close();
		//	出力ストリームを閉じる.
		out_stream.Close();
		//	元に戻す/やり直すメニュー項目の使用可否を設定する.
		enable_undo_menu();
		//	編集メニュー項目の使用可否を設定する.
		enable_edit_menu();
		//	スレッドコンテキストを復元する.
		co_await context;
	}

	bool MainPage::clipboard_contains(winrt::hstring const c_formats[], const uint32_t c_count) const
	{
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;

		auto const& a_formats = Clipboard::GetContent().AvailableFormats();
		for (auto const& a_format : a_formats) {
			for (uint32_t i = 0; i < c_count; i++) {
				if (a_format == c_formats[i]) {
					return true;
				}
			}
		}
		return false;
	}

	// クリップボードに保存された図形を非同期に貼り付ける
	IAsyncAction MainPage::clipboard_paste_async(void)
	{
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackageView;
		using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;
		using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;
		using winrt::Windows::Storage::Streams::IRandomAccessStream;
		using winrt::Windows::Storage::Streams::DataReader;

		// コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// Clipboard::GetContent() は, 
		// WinRT originate error 0x80040904
		// を引き起こすので, try ... catch 文が必要.
			try {
				// 図形データがクリップボードに含まれているか調べる.
				auto pkg_view{ Clipboard::GetContent() };
				if (pkg_view.Contains(FMT_DATA)) {
					// クリップボードから読み込むためのデータリーダーを得る.
					auto dt_object{ co_await pkg_view.GetDataAsync(FMT_DATA) };
					auto ra_stream{ unbox_value<InMemoryRandomAccessStream>(dt_object) };
					auto in_stream{ ra_stream.GetInputStreamAt(0) };
					auto dt_reader{ DataReader(in_stream) };
					auto dt_size = static_cast<UINT32>(ra_stream.Size());
					auto operation{ co_await dt_reader.LoadAsync(dt_size) };
					//	図形のためのメモリの確保が別スレッドで行われた場合,
					//	D2DERR_WRONG_STATE を引き起こすことがある.
					//	図形を貼り付ける前に, スレッドをメインページの UI スレッドに変える.
					co_await winrt::resume_foreground(this->Dispatcher());
					if (operation == ra_stream.Size()) {
						S_LIST_T paste_list;	// 貼り付けリスト
						s_list_read(paste_list, dt_reader);
						if (paste_list.empty() == false) {
							// 貼り付けリストが空でない場合,
							// 図形リストの中の図形の選択をすべて解除し,
							// 貼り付けリストの図形を追加する.
							unselect_all();
							for (auto s : paste_list) {
								if (m_summary_visible) {
									summary_append(s);
								}
								undo_push_append(s);
								s->get_bound(m_page_min, m_page_max);
							}
							undo_push_null();
							paste_list.clear();
							enable_undo_menu();
							enable_edit_menu();
							set_page_panle_size();
							draw_page();
						}
					}
					const auto _{ dt_reader.DetachStream() };
					//	データリーダーを閉じる.
					dt_reader.Close();
					in_stream.Close();
				}
				else if (pkg_view.Contains(StandardDataFormats::Text())) {
					// クリップボードから読み込むためのデータリーダーを得る.
					/*
					テキストを直接貼り付ける
					auto text{ co_await pkg_view.GetTextAsync() };
					if (text.empty() == false) {
						wchar_t const* w = text.c_str();
						D2D1_POINT_2F d = { 100, 100 };
						D2D1_POINT_2F p;
						const double sh = sb_horz().Value();
						const double sv = sb_vert().Value();
						const double sc = m_page_panel.m_page_scale;
						const double aw = scp_page_panel().ActualWidth();
						const double ah = scp_page_panel().ActualHeight();
						p.x = static_cast<FLOAT>(sc * (sh + aw * 0.5) - d.x * 0.5);
						p.y = static_cast<FLOAT>(sc * (sv + ah * 0.5) - d.y * 0.5);
						auto t = new ShapeText(p, d, wchar_cpy(w), &m_page_panel);
						unselect_all();
						if (m_summary_visible) {
							summary_append(t);
						}
						undo_push_append(t);
						undo_push_null();
						enable_undo_menu();
						enable_edit_menu();
						t->get_bound(m_page_min, m_page_max);
						set_page_panle_size();
						draw_page();
					}
					*/
				}
			}
			catch (winrt::hresult_error const& e) {
				auto a = e.code();
			}
		//}
		//スレッドコンテキストを復元する.
		co_await context;
		co_return;
	}

	// 編集メニューの「コピー」が選択された.
	void MainPage::mfi_copy_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		auto _{ clipboard_copy_async<COPY>() };
	}

	// 編集メニューの「切り取る」が選択された.
	void MainPage::mfi_cut_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		auto _{ clipboard_copy_async<CUT>() };
	}

	void MainPage::delete_selected_shapes(void)
	{
		S_LIST_T sel_list;
		s_list_select<Shape>(m_list_shapes, sel_list);
		if (sel_list.size() == 0) {
			return;
		}
		for (auto s : sel_list) {
			if (m_summary_visible) {
				summary_remove(s);
			}
			undo_push_remove(s);
		}
		sel_list.clear();
		undo_push_null();
		enable_undo_menu();
		enable_edit_menu();
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		draw_page();
	}

	// 編集メニューの「削除」が選択された.
	void MainPage::mfi_delete_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		delete_selected_shapes();
	}

	// 編集メニューの「貼り付け」が選択された.
	void MainPage::mfi_paste_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		auto _{ clipboard_paste_async() };
	}

}