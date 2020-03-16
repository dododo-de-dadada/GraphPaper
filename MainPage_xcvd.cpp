//-------------------------------
// MainPage_xcvd.cpp
// 切り取りとコピー, 貼り付け, 削除
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 編集メニューの「コピー」が選択された.
	IAsyncAction MainPage::mfi_xcvd_copy_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;
		using winrt::Windows::Storage::Streams::DataWriter;
		using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;

		if (m_list_selected == 0) {
			// 選択された図形の数が 0 の場合,
			// 終了する.
			return;
		}
		// 選択された図形のリストを得る.
		S_LIST_T list_selected;
		s_list_selected<Shape>(m_list_shapes, list_selected);
		// コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// 出力ストリームを作成して, データライターを得る.
		auto ra_stream{ InMemoryRandomAccessStream() };
		auto out_stream{ ra_stream.GetOutputStreamAt(0) };
		auto dt_writer{ DataWriter(out_stream) };
		// 選択された図形のリストをデータライターに書き込む.
		constexpr bool REDUCED = true;
		s_list_write<REDUCED>(list_selected, dt_writer);
		// 書き込んだらリストは破棄する.
		list_selected.clear();
		// 書き込んだデータを出力ストリームに格納し, 格納したバイト数を得る.
		auto n_byte{ co_await dt_writer.StoreAsync() };
		// 図形リストから文字列を得る.
		auto text = s_list_text(m_list_shapes);
		// スレッドをメインページの UI スレッドに変える.
		co_await winrt::resume_foreground(this->Dispatcher());
		if (n_byte > 0) {
			// 格納したバイト数が 0 を超える場合,
			// データパッケージを作成する.
			auto dt_pkg{ DataPackage() };
			// ストリームをデータパッケージに格納する.
			dt_pkg.RequestedOperation(DataPackageOperation::Copy);
			dt_pkg.SetData(CF_GPD, winrt::box_value(ra_stream));
			if (text.empty() == false) {
				// 文字列が空でない場合,
				// 文字列をデータパッケージに格納する.
				dt_pkg.SetText(text);
			}
			// データパッケージをクリップボードに格納する.
			Clipboard::SetContent(dt_pkg);
		}
		// データライターを閉じる.
		dt_writer.Close();
		// 出力ストリームを閉じる.
		out_stream.Close();
		// 編集メニュー項目の使用の可否を設定する.
		enable_edit_menu();
		// スレッドコンテキストを復元する.
		co_await context;
	}

	// 編集メニューの「切り取る」が選択された.
	//constexpr uint32_t CUT = 0;
	IAsyncAction MainPage::mfi_xcvd_cut_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		co_await mfi_xcvd_copy_click_async(nullptr, nullptr);
		mfi_xcvd_delete_click(nullptr, nullptr);
	}

	// 編集メニューの「削除」が選択された.
	void MainPage::mfi_xcvd_delete_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_list_selected == 0) {
			// 選択された図形の数が 0 の場合,
			// 終了する.
			return;
		}
		// 選択された図形のリストを得る.
		S_LIST_T list_selected;
		s_list_selected<Shape>(m_list_shapes, list_selected);
		// 得られたリストの各図形について以下を繰り返す.
		for (auto s : list_selected) {
			if (m_summary_visible) {
				// 図形一覧の表示フラグが立っている場合,
				// 図形を一覧から消去する.
				summary_remove(s);
			}
			// 図形を削除して, その操作をスタックに積む.
			undo_push_remove(s);
		}
		// 選択された図形のリストを消去する.
		list_selected.clear();
		undo_push_null();
		// 編集メニュー項目の使用の可否を設定する.
		enable_edit_menu();
		s_list_bound(m_list_shapes, m_page_layout.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
	}

	// 編集メニューの「貼り付け」が選択された.
	IAsyncAction MainPage::mfi_xcvd_paste_click_async(IInspectable const&, RoutedEventArgs const&)
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
			if (xcvd_contains({ CF_GPD })) {
				// クリップボードから読み込むためのデータリーダーを得る.
				auto dt_object{ co_await Clipboard::GetContent().GetDataAsync(CF_GPD) };
				auto ra_stream{ unbox_value<InMemoryRandomAccessStream>(dt_object) };
				auto in_stream{ ra_stream.GetInputStreamAt(0) };
				auto dt_reader{ DataReader(in_stream) };
				auto dt_size = static_cast<UINT32>(ra_stream.Size());
				auto operation{ co_await dt_reader.LoadAsync(dt_size) };
				// 図形のためのメモリの確保が別スレッドで行われた場合,
				// D2DERR_WRONG_STATE を引き起こすことがある.
				// 図形を貼り付ける前に, スレッドをメインページの UI スレッドに変える.
				co_await winrt::resume_foreground(this->Dispatcher());
				if (operation == ra_stream.Size()) {
					S_LIST_T list_pasted;	// 貼り付けリスト
					if (s_list_read(list_pasted, dt_reader) == false) {

					}
					else if (list_pasted.empty() == false) {
						// 得られたリストが空でない場合,
						// 図形リストの中の図形の選択をすべて解除する.
						unselect_all();
						// 得られたリストの各図形について以下を繰り返す.
						for (auto s : list_pasted) {
							if (m_summary_visible) {
								summary_append(s);
							}
							undo_push_append(s);
							s->get_bound(m_page_min, m_page_max);
						}
						undo_push_null();
						list_pasted.clear();
						// 編集メニュー項目の使用の可否を設定する.
						enable_edit_menu();
						set_page_panle_size();
						page_draw();
					}
				}
				const auto _{ dt_reader.DetachStream() };
				// データリーダーを閉じる.
				dt_reader.Close();
				in_stream.Close();
			}
			else if (xcvd_contains({ StandardDataFormats::Text() })) {
				using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
				const auto d_result = co_await cd_conf_paste().ShowAsync();
				if (d_result == ContentDialogResult::Primary) {
					// クリップボードから読み込むためのデータリーダーを得る.
					auto text{ co_await Clipboard::GetContent().GetTextAsync() };
					if (text.empty() == false) {
						// 文字列が空でない場合,
						const double scale = m_page_layout.m_page_scale;
						const auto g_len = m_page_layout.m_grid_base + 1.0;
						D2D1_POINT_2F pos{
							static_cast<FLOAT>(scale * (sb_horz().Value() + scp_page_panel().ActualWidth() * 0.5) - g_len),
							static_cast<FLOAT>(scale * (sb_vert().Value() + scp_page_panel().ActualHeight() * 0.5) - g_len)
						};
						D2D1_POINT_2F diff{
							static_cast<FLOAT>(2.0 * g_len),
							static_cast<FLOAT>(2.0 * g_len)
						};
						if (m_page_layout.m_grid_snap) {
							pt_round(pos, g_len, pos);
						}
						auto t = new ShapeText(pos, diff, wchar_cpy(text.c_str()), &m_page_layout);
#if (_DEBUG)
						debug_leak_cnt++;
#endif
						unselect_all();
						if (m_summary_visible) {
							summary_append(t);
						}
						undo_push_append(t);
						undo_push_null();
						enable_edit_menu();
						t->get_bound(m_page_min, m_page_max);
						set_page_panle_size();
						page_draw();
					}
				}
			}
		}
		catch (winrt::hresult_error const& e) {
			auto a = e.code();
		}
		//スレッドコンテキストを復元する.
		co_await context;
	}

	// クリップボードにデータが含まれているか調べる.
	bool MainPage::xcvd_contains(const winrt::hstring formats[], const size_t f_cnt) const
	{
		// DataPackageView::Contains を使用すると, 次の内部エラーが発生する.
		// WinRT originate error - 0x8004006A : '指定された形式が DataPackage に含まれていません。
		// DataPackageView.Contains または DataPackageView.AvailableFormats を使って、その形式が存在することを確かめてください。
		// 念のため, DataPackageView::AvailableFormats を使う.
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;

		auto const& a_formats = Clipboard::GetContent().AvailableFormats();
		for (auto const& a_format : a_formats) {
			for (size_t i = 0; i < f_cnt; i++) {
				if (a_format == formats[i]) {
					return true;
				}
			}
		}
		return false;
	}

}