//-------------------------------
// MainPage_xcvd.cpp
// 切り取りとコピー, 文字列の編集など
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// クリップボードにデータが含まれているか判定する.
	size_t MainPage::xcvd_contains(const winrt::hstring formats[], const size_t f_cnt) const
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
					return i;
				}
			}
		}
		return static_cast<size_t>(-1);
	}

	// 編集メニューの「コピー」が選択された.
	IAsyncAction MainPage::xcvd_copy_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;
		using winrt::Windows::Storage::Streams::DataWriter;
		using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;

		if (m_list_sel_cnt == 0) {
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
		// データライターに選択された図形のリストを書き込む.
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
			dt_pkg.SetData(CBF_GPD, winrt::box_value(ra_stream));
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
		xcvd_is_enabled();
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
		if (m_list_sel_cnt == 0) {
			// 選択された図形の数が 0 の場合,
			// 終了する.
			return;
		}
		// 選択された図形のリストを得る.
		SHAPE_LIST list_selected;
		slist_selected<Shape>(m_list_shapes, list_selected);
		m_dx_mutex.lock();
		for (auto s : list_selected) {
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_remove(s);
			}
			// 図形を削除して, その操作をスタックに積む.
			ustack_push_remove(s);
		}
		ustack_push_null();
		m_dx_mutex.unlock();

		// 選択された図形のリストを消去する.
		//list_selected.clear();
		xcvd_is_enabled();
		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
	}

	// 編集メニューの可否を設定する.
	// 選択の有無やクラスごとに図形を数え, メニュー項目の可否を判定する.
	void MainPage::xcvd_is_enabled(void)
	{
		using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;

		ustack_is_enable();

		uint32_t undeleted_cnt = 0;	// 消去フラグがない図形の数
		uint32_t selected_cnt = 0;	// 選択された図形の数
		uint32_t selected_group_cnt = 0;	// 選択されたグループ図形の数
		uint32_t runlength_cnt = 0;	// 選択された図形のランレングスの数
		uint32_t selected_text_cnt = 0;	// 選択された文字列図形の数
		uint32_t text_cnt = 0;	// 文字列図形の数
		bool fore_selected = false;	// 最前面の図形の選択フラグ
		bool back_selected = false;	// 最背面の図形の選択フラグ
		bool prev_selected = false;	// ひとつ背面の図形の選択フラグ
		slist_count(
			m_list_shapes,
			undeleted_cnt,
			selected_cnt,
			selected_group_cnt,
			runlength_cnt,
			selected_text_cnt,
			text_cnt,
			fore_selected,
			back_selected,
			prev_selected
		);

		// 消去されていない図形がひとつ以上ある場合.
		const auto exists_undeleted = (undeleted_cnt > 0);
		// 選択された図形がひとつ以上ある場合.
		const auto exists_selected = (selected_cnt > 0);
		// 選択された文字列図形がひとつ以上ある場合.
		const auto exists_selected_text = (selected_text_cnt > 0);
		// 文字列図形がひとつ以上ある場合.
		const auto exists_text = (text_cnt > 0);
		// 選択されてない図形がひとつ以上ある場合.
		const auto exists_unselected = (selected_cnt < undeleted_cnt);
		// 選択された図形がふたつ以上ある場合.
		const auto exists_selected_2 = (selected_cnt > 1);
		// 選択されたグループ図形がひとつ以上ある場合.
		const auto exists_selected_group = (selected_group_cnt > 0);
		// 前面に配置可能か判定する.
		// 1. 複数のランレングスがある.
		// 2. または, 少なくとも 1 つは選択された図形があり, 
		//    かつ最前面の図形は選択されいない.
		const auto enable_forward = (runlength_cnt > 1 || (exists_selected && fore_selected != true));
		// 背面に配置可能か判定する.
		// 1. 複数のランレングスがある.
		// 2. または, 少なくとも 1 つは選択された図形があり, 
		//    かつ最背面の図形は選択されいない.
		const auto enable_backward = (runlength_cnt > 1 || (exists_selected && back_selected != true));

		mfi_xcvd_cut().IsEnabled(exists_selected);
		mfi_xcvd_copy().IsEnabled(exists_selected);
		mfi_xcvd_paste().IsEnabled(xcvd_contains({ CBF_GPD, StandardDataFormats::Text(), StandardDataFormats::Bitmap() }) != static_cast<size_t>(-1));
		mfi_xcvd_delete().IsEnabled(exists_selected);
		mfi_select_all().IsEnabled(exists_unselected);
		mfi_group().IsEnabled(exists_selected_2);
		mfi_ungroup().IsEnabled(exists_selected_group);
		mfi_edit_text().IsEnabled(exists_selected_text);
		mfi_find_text().IsEnabled(exists_text);
		mfi_edit_text_frame().IsEnabled(exists_selected_text);
		mfi_bring_forward().IsEnabled(enable_forward);
		mfi_bring_to_front().IsEnabled(enable_forward);
		mfi_send_to_back().IsEnabled(enable_backward);
		mfi_send_backward().IsEnabled(enable_backward);
		mfi_summary_list().IsEnabled(exists_undeleted);
		m_list_sel_cnt = selected_cnt;
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
			const auto i = xcvd_contains({ CBF_GPD, StandardDataFormats::Text(), StandardDataFormats::Bitmap() });
			if (i == 0) {
				// クリップボードから読み込むためのデータリーダーを得て, データを読み込む.
				auto dt_object{ co_await Clipboard::GetContent().GetDataAsync(CBF_GPD) };
				auto ra_stream{ unbox_value<InMemoryRandomAccessStream>(dt_object) };
				auto in_stream{ ra_stream.GetInputStreamAt(0) };
				auto dt_reader{ DataReader(in_stream) };
				auto ra_size = static_cast<UINT32>(ra_stream.Size());
				auto operation{ co_await dt_reader.LoadAsync(ra_size) };
				// 図形のためのメモリの確保が別スレッドで行われた場合, D2DERR_WRONG_STATE を引き起こすことがある.
				// 図形を貼り付ける前に, スレッドをメインページの UI スレッドに変える.
				auto cd = this->Dispatcher();
				co_await winrt::resume_foreground(cd);
				// データリーダーに読み込めたか判定する.
				if (operation == ra_stream.Size()) {
					SHAPE_LIST slist_pasted;	// 貼り付けリスト

					// データリーダーから貼り付けリストを読み込み, それが空でないか判定する.
					if (slist_read(slist_pasted, dt_reader) && !slist_pasted.empty()) {
						m_dx_mutex.lock();
						// 得られたリストが空でない場合,
						// 図形リストの中の図形の選択をすべて解除する.
						unselect_all();
						// 得られたリストの各図形について以下を繰り返す.
						for (auto s : slist_pasted) {
							// 一覧が表示されてるか判定する.
							if (summary_is_visible()) {
								summary_append(s);
							}
							ustack_push_append(s);
							sheet_update_bbox(s);
						}
						ustack_push_null();
						m_dx_mutex.unlock();
						slist_pasted.clear();
						xcvd_is_enabled();
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
			else if (i == 1) {
				using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
				//const auto d_result = co_await cd_conf_paste_dialog().ShowAsync();
				//if (d_result == ContentDialogResult::Primary) {

					// クリップボードから読み込むためのデータリーダーを得る.
					auto text{ co_await Clipboard::GetContent().GetTextAsync() };

					// 文字列が空でないか判定する.
					if (!text.empty()) {
						unselect_all();

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
						pt_add(s_min, m_sheet_min, s_min);

						// 方眼に合わせるか判定する.
						D2D1_POINT_2F g_pos{};
						if (m_sheet_main.m_grid_snap) {
							// 左上位置を方眼の大きさで丸める.
							pt_round(s_min, m_sheet_main.m_grid_base + 1.0, g_pos);
							if (m_misc_pile_up < FLT_MIN) {
								s_min = g_pos;
							}
						}

						// 頂点を重ねる閾値がゼロより大きいか判定する.
						if (m_misc_pile_up >= FLT_MIN) {
							D2D1_POINT_2F v_pos;
							if (slist_neighbor(m_list_shapes, s_min, m_misc_pile_up / m_sheet_main.m_sheet_scale, v_pos)) {
								D2D1_POINT_2F v_vec;
								pt_sub(v_pos, s_min, v_vec);
								D2D1_POINT_2F g_vec;
								pt_sub(g_pos, s_min, g_vec);
								if (m_sheet_main.m_grid_snap && pt_abs2(g_vec) < pt_abs2(v_vec)) {
									s_min = g_pos;
								}
								else {
									s_min = v_pos;
								}
							}
							else if (m_sheet_main.m_grid_snap) {
								s_min = g_pos;
							}
						}
						t->set_pos_start(s_min);
						m_dx_mutex.lock();
						ustack_push_append(t);
						ustack_push_select(t);
						ustack_push_null();
						m_dx_mutex.unlock();
						// 一覧が表示されてるか判定する.
						if (summary_is_visible()) {
							summary_append(t);
							summary_select(t);
						}
						xcvd_is_enabled();
						sheet_update_bbox(t);
						sheet_panle_size();
						sheet_draw();
					}
					else {
						message_show(ICON_ALERT, L"str_err_paste", L"");
					}
				//}
			}
			else if (i == 2) {
				auto bitmap = co_await Clipboard::GetContent().GetBitmapAsync();
				auto bn_stream{ co_await bitmap.OpenReadAsync() };
				//auto ra_stream{ unbox_value<InMemoryRandomAccessStream>(bitmap) };
				auto in_stream{ bn_stream.GetInputStreamAt(0) };
				auto dt_reader{ DataReader(in_stream) };
				auto ra_size = static_cast<UINT32>(bn_stream.Size());
				auto operation{ co_await dt_reader.LoadAsync(ra_size) };
				if (operation == ra_size) {
					// 用紙の表示された部分の中心の位置を求める.
					const float scale = m_sheet_main.m_sheet_scale;
					const float act_w = static_cast<float>(scp_sheet_panel().ActualWidth());
					const float act_h = static_cast<float>(scp_sheet_panel().ActualHeight());
					const D2D1_POINT_2F c_pos{
						static_cast<FLOAT>((sb_horz().Value() + act_w * 0.5) / scale),
						static_cast<FLOAT>((sb_vert().Value() + act_h * 0.5) / scale)
					};
					ShapeBitmap* b = new ShapeBitmap(c_pos, dt_reader);
					m_dx_mutex.lock();
					ustack_push_append(b);
					ustack_push_select(b);
					ustack_push_null();
					m_dx_mutex.unlock();
					// 一覧が表示されてるか判定する.
					if (summary_is_visible()) {
						summary_append(b);
						summary_select(b);
					}
					xcvd_is_enabled();
					sheet_update_bbox(b);
					sheet_panle_size();
					sheet_draw();
				}
			}
		}
		catch (winrt::hresult_error const& e) {
			auto a = e.code();
		}
		//スレッドコンテキストを復元する.
		co_await context;
	}

}