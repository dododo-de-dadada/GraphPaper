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
	IAsyncAction MainPage::xcvd_copy_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;
		using winrt::Windows::Storage::Streams::DataWriter;
		using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;

		if (m_cnt_selected == 0) {
			// 選択された図形の数が 0 の場合,
			// 終了する.
			return;
		}
		// 選択された図形のリストを得る.
		SHAPE_LIST list_selected;
		slist_selected<Shape>(m_list_shapes, list_selected);
		// コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// 出力ストリームを作成して, データライターを得る.
		auto ra_stream{ InMemoryRandomAccessStream() };
		auto out_stream{ ra_stream.GetOutputStreamAt(0) };
		auto dt_writer{ DataWriter(out_stream) };
		// 選択された図形のリストをデータライターに書き込む.
		constexpr bool REDUCED = true;
		slist_write<REDUCED>(list_selected, dt_writer);
		// 書き込んだらリストは破棄する.
		list_selected.clear();
		// 書き込んだデータを出力ストリームに格納し, 格納したバイト数を得る.
		auto n_byte{ co_await dt_writer.StoreAsync() };
		auto text = slist_selected_all_text(m_list_shapes);
		// スレッドをメインページの UI スレッドに変える.
		auto cd = this->Dispatcher();
		co_await winrt::resume_foreground(cd);
		if (n_byte > 0) {
			// 格納したバイト数が 0 を超える場合,
			// データパッケージを作成する.
			auto dt_pkg{ DataPackage() };
			// ストリームをデータパッケージに格納する.
			dt_pkg.RequestedOperation(DataPackageOperation::Copy);
			dt_pkg.SetData(CF_GPD, winrt::box_value(ra_stream));
			if (text.empty() != true) {
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
		edit_menu_is_enabled();
		// スレッドコンテキストを復元する.
		co_await context;
	}

	// 編集メニューの「切り取る」が選択された.
	//constexpr uint32_t CUT = 0;
	IAsyncAction MainPage::xcvd_cut_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		co_await xcvd_copy_click_async(nullptr, nullptr);
		xcvd_delete_click(nullptr, nullptr);
	}

	// 編集メニューの「削除」が選択された.
	void MainPage::xcvd_delete_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_cnt_selected == 0) {
			// 選択された図形の数が 0 の場合,
			// 終了する.
			return;
		}
		// 選択された図形のリストを得る.
		SHAPE_LIST list_selected;
		slist_selected<Shape>(m_list_shapes, list_selected);
		m_dx_mutex.lock();
		for (auto s : list_selected) {
			if (m_smry_atomic.load(std::memory_order_acquire)) {
				// 図形一覧の表示フラグが立っている場合,
				// 図形を一覧から消去する.
				smry_remove(s);
			}
			// 図形を削除して, その操作をスタックに積む.
			undo_push_remove(s);
		}
		undo_push_null();
		m_dx_mutex.unlock();

		// 選択された図形のリストを消去する.
		//list_selected.clear();
		// 編集メニュー項目の使用の可否を設定する.
		edit_menu_is_enabled();
		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
	}

	// 編集メニューの「貼り付け」が選択された.
	IAsyncAction MainPage::xcvd_paste_click_async(IInspectable const&, RoutedEventArgs const&)
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
			// 図形データがクリップボードに含まれているか判定する.
			if (xcvd_contains({ CF_GPD })) {
				// クリップボードから読み込むためのデータリーダーを得て, データを読み込む.
				auto dt_object{ co_await Clipboard::GetContent().GetDataAsync(CF_GPD) };
				auto ra_stream{ unbox_value<InMemoryRandomAccessStream>(dt_object) };
				auto in_stream{ ra_stream.GetInputStreamAt(0) };
				auto dt_reader{ DataReader(in_stream) };
				auto dt_size = static_cast<UINT32>(ra_stream.Size());
				auto operation{ co_await dt_reader.LoadAsync(dt_size) };
				// 図形のためのメモリの確保が別スレッドで行われた場合, D2DERR_WRONG_STATE を引き起こすことがある.
				// 図形を貼り付ける前に, スレッドをメインページの UI スレッドに変える.
				auto cd = this->Dispatcher();
				co_await winrt::resume_foreground(cd);
				// データリーダーに読み込めたか判定する.
				if (operation == ra_stream.Size()) {
					SHAPE_LIST slist_pasted;	// 貼り付けリスト

					// 貼り付けリストをデータリーダーから読み込み, それが空でないか判定する.
					if (slist_read(slist_pasted, dt_reader) && !slist_pasted.empty()) {
						m_dx_mutex.lock();
						// 得られたリストが空でない場合,
						// 図形リストの中の図形の選択をすべて解除する.
						unselect_all();
						// 得られたリストの各図形について以下を繰り返す.
						for (auto s : slist_pasted) {
							if (m_smry_atomic.load(std::memory_order_acquire)) {
								smry_append(s);
							}
							undo_push_append(s);
							sheet_update_bbox(s);
						}
						undo_push_null();
						m_dx_mutex.unlock();
						slist_pasted.clear();
						// 編集メニュー項目の使用の可否を設定する.
						edit_menu_is_enabled();
						sheet_panle_size();
						sheet_draw();
					}
					else {
						message_show(ICON_ALERT, L"str_err_paste", L"");
					}
				}
				const auto _{ dt_reader.DetachStream() };
				// データリーダーを閉じる.
				dt_reader.Close();
				in_stream.Close();
			}
			// クリップボードにテキストが含まれているか判定する.
			else if (xcvd_contains({ StandardDataFormats::Text() })) {
				using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
				const auto d_result = co_await cd_conf_paste().ShowAsync();
				if (d_result == ContentDialogResult::Primary) {

					// クリップボードから読み込むためのデータリーダーを得る.
					auto text{ co_await Clipboard::GetContent().GetTextAsync() };

					// 文字列が空でないか判定する.
					if (!text.empty()) {

						// パネルの大きさで文字列図形を作成する,.
						const float scale = m_sheet_main.m_sheet_scale;
						const float act_w = static_cast<float>(scp_sheet_panel().ActualWidth());
						const float act_h = static_cast<float>(scp_sheet_panel().ActualHeight());
						auto t = new ShapeText(D2D1_POINT_2F{ 0.0f, 0.0f }, D2D1_POINT_2F{ act_w / scale, act_h / scale }, wchar_cpy(text.c_str()), &m_sheet_main);
#if (_DEBUG)
						debug_leak_cnt++;
#endif

						// 枠の大きさを文字列に合わせる.
						t->adjust_bbox(m_sheet_main.m_grid_snap ? m_sheet_main.m_grid_base + 1.0f : 0.0f);
						// パネルの中央になるよう左上位置を求める.
						D2D1_POINT_2F s_min{
							static_cast<FLOAT>((sb_horz().Value() + act_w * 0.5) / scale - t->m_diff[0].x * 0.5),
							static_cast<FLOAT>((sb_vert().Value() + act_h * 0.5) / scale - t->m_diff[0].y * 0.5)
						};
						pt_add(s_min, sheet_min(), s_min);

						// 方眼に整列フラグが立っているか判定する.
						if (m_sheet_main.m_grid_snap) {
							float g_base;
							m_sheet_main.get_grid_base(g_base);
							const auto g_len = g_base + 1.0;
							pt_round(s_min, g_len, s_min);
						}

						// 求めた左上位置に移動する.
						t->set_start_pos(s_min);

						m_dx_mutex.lock();
						unselect_all();
						undo_push_append(t);
						undo_push_select(t);
						undo_push_null();
						m_dx_mutex.unlock();
						if (m_smry_atomic.load(std::memory_order_acquire)) {
							smry_append(t);
							smry_select(t);
						}
						edit_menu_is_enabled();
						sheet_update_bbox(t);
						sheet_panle_size();
						sheet_draw();
					}
					else {
						message_show(ICON_ALERT, L"str_err_paste", L"");
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

	// クリップボードにデータが含まれているか判定する.
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