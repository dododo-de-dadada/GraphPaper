//-------------------------------
// MainPage_xcvd.cpp
// 切り取り (x) とコピー (c), 貼り付け (v), 削除 (d)
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapDecoder;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;
	using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;
	using winrt::Windows::ApplicationModel::DataTransfer::DataPackageView;
	using winrt::Windows::Storage::Streams::IInputStream;
	using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;
	using winrt::Windows::Storage::Streams::IOutputStream;
	using winrt::Windows::Storage::Streams::IRandomAccessStreamWithContentType;
	using winrt::Windows::Storage::Streams::RandomAccessStreamReference;
	using winrt::Windows::System::VirtualKey;
	using winrt::Windows::UI::Core::CoreVirtualKeyStates;
	using winrt::Windows::UI::Xaml::FocusState;

	using winrt::Windows::Graphics::Imaging::BitmapEncoder;

	const winrt::param::hstring CLIPBOARD_FORMAT_SHAPES{ L"graph_paper_shape_data" };	// 図形データのクリップボード書式

	// 貼り付ける位置を求める.
	static void xcvd_get_paste_pos(D2D1_POINT_2F& paste_pt, const D2D1_POINT_2F q, const SHAPE_LIST& slist, const double grid_len, const float interval);

	//------------------------------
	// 編集メニューの「コピー」が選択された.
	//------------------------------
	IAsyncAction MainPage::copy_click_async(IInspectable const& sender, RoutedEventArgs const& args)
	{
		if (! m_main_sheet_focused) {
			co_return;
		}

		// 編集中の文字列があって, 選択範囲があるなら, 選択範囲の文字列をクリップボードに格納する.
		if (m_core_text_focused != nullptr && core_text_selected_len() > 0) {
			winrt::hstring selected_text{ core_text_substr() };
			DataPackage dt_pack{ DataPackage() };
			dt_pack.RequestedOperation(DataPackageOperation::Copy);
			dt_pack.SetText(selected_text);
			Clipboard::SetContent(dt_pack);
		}
		// そうでなければ, 図形の内容の文字列や画像, そして選択された図形をクリップボードに格納す
		else {
			winrt::apartment_context context;

			// 選択された図形のリストを得る.
			SHAPE_LIST selected_list;
			slist_get_selected<Shape>(m_main_sheet.m_shape_list, selected_list);

			// リストから降順に, 最初に見つかった文字列とビットマップを得る.
			wchar_t* text_ptr = nullptr;
			RandomAccessStreamReference image_ref = nullptr;
			for (auto it = selected_list.rbegin(); it != selected_list.rend(); it++) {
				// 最初に見つかった文字列をポインターに格納する.
				if (text_ptr == nullptr) {
					(*it)->get_text_content(text_ptr);
				}
				// 最初に見つかったビットマップをメモリランダムアクセスストリームに格納し, その参照を得る.
				if (image_ref == nullptr && typeid(*it) == typeid(ShapeImage)) {
					InMemoryRandomAccessStream image_stream{
						InMemoryRandomAccessStream()
					};
					if (co_await static_cast<ShapeImage*>(*it)->copy<false>(BitmapEncoder::BmpEncoderId(), image_stream) && image_stream.Size() > 0) {
						image_ref = RandomAccessStreamReference::CreateFromStream(image_stream);
					}
				}
				// 文字列とビットマップ, 両方が見つかったら, それ以上は必要ないので中断する.
				if (text_ptr != nullptr && image_ref != nullptr) {
					break;
				}
			}

			// メモリストリームを作成して, そのデータライターを作成する.
			InMemoryRandomAccessStream mem_stream{
				InMemoryRandomAccessStream()
			};
			IOutputStream out_stream{
				mem_stream.GetOutputStreamAt(0)
			};
			DataWriter dt_writer{ DataWriter(out_stream) };

			// データライターに選択された図形のリストを書き込む.
			constexpr bool REDUCED = true;
			slist_write<REDUCED>(selected_list, /*--->*/dt_writer);

			// 選択された図形のリストを破棄する.
			selected_list.clear();

			// メモリストリームにデータライターの内容を格納し, 格納したバイト数を得る.
			uint32_t n_byte{
				co_await dt_writer.StoreAsync()
			};
			if (n_byte > 0) {

				// メインページの UI スレッドに変える.
				co_await winrt::resume_foreground(Dispatcher());

				// データパッケージを作成し, データパッケージにメモリストリームを格納する.
				DataPackage content{
					DataPackage()
				};
				content.RequestedOperation(DataPackageOperation::Copy);
				content.SetData(CLIPBOARD_FORMAT_SHAPES, winrt::box_value(mem_stream));

				// 文字列があるならデータパッケージにテキストを格納する.
				if (text_ptr != nullptr) {
					content.SetText(text_ptr);
				}

				// ビットマップがあるならデータパッケージにテキストを格納する.
				if (image_ref != nullptr) {
					content.SetBitmap(image_ref);
				}

				// クリップボードにデータパッケージを格納する.
				Clipboard::SetContent(content);
				content = nullptr;
			}

			// データライターを閉じる.
			dt_writer.Close();
			dt_writer = nullptr;

			// 出力ストリームを閉じる.
			// メモリストリームは閉じちゃダメ.
			out_stream.Close();
			out_stream = nullptr;

			// スレッドコンテキストを復元する.
			co_await context;
		}
		status_bar_set_pos();
	}

	//------------------------------
	// 編集メニューの「切り取り」が選択された.
	//------------------------------
	IAsyncAction MainPage::cut_click_async(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (!m_main_sheet_focused) {
			co_return;
		}

		co_await copy_click_async(nullptr, nullptr);

		if (m_core_text_focused != nullptr) {
			core_text_delete_selection();
		}
		else {
			delete_click(nullptr, nullptr);
		}
	}

	//------------------------------
	// 編集メニューの「削除」が選択された.
	//------------------------------
	void MainPage::delete_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (!m_main_sheet_focused) {
			return;
		}
		// XAML のキーボードアクセラレーターに削除キーは指定されていて,
		// CoreWWindow の KeyDow でなく, このハンドラーで処理される.
		if (m_core_text_focused != nullptr) {
			const auto shift_key = ((CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			if (shift_key) {
				core_text_delete_key<true>();
			}
			else {
				core_text_delete_key<false>();
			}
		}
		else {
			// 選択された図形の数がゼロか判定する.
			if (m_undo_select_cnt > 0) {
				undo_push_null();
				// 選択された図形のリストを得る.
				SHAPE_LIST selected_list;
				slist_get_selected<Shape>(m_main_sheet.m_shape_list, selected_list);
				// リストの各図形について以下を繰り返す.
				m_mutex_draw.lock();
				for (auto s : selected_list) {
					// 一覧が表示されてるか判定する.
					if (summary_is_visible()) {
						summary_remove(s);
					}
					// 図形を取り去り, その操作をスタックに積む.
					undo_push_remove(s);
				}
				m_mutex_draw.unlock();
				main_bbox_update();
				main_panel_size();
				menu_is_enable();
				main_draw();
				selected_list.clear();
			}
			status_bar_set_pos();
		}
	}

	//------------------------------
	// 編集メニューの「貼り付け」が選択された.
	//------------------------------
	void MainPage::paste_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (!m_main_sheet_focused) {
			return;
		}
		if (tx_find_text_what().FocusState() != FocusState::Unfocused ||
			tx_find_replace_with().FocusState() != FocusState::Unfocused) {
			return;
		}
		// Clipboard::GetContent() は, 
		// WinRT originate error 0x80040904
		// を引き起こすので, try ... catch 文が必要.
		try {
			// クリップボードに図形が含まれているか判定する.
			const auto& dp_view = Clipboard::GetContent();
			if (dp_view.Contains(CLIPBOARD_FORMAT_SHAPES)) {
				xcvd_paste_shape();
			}
			// クリップボードに文字列が含まれているか判定する.
			else if (dp_view.Contains(StandardDataFormats::Text())) {
				xcvd_paste_text();
			}
			// クリップボードにビットマップが含まれているか判定する.
			else if (dp_view.Contains(StandardDataFormats::Bitmap())) {
				xcvd_paste_image();
			}
			else {
				throw winrt::hresult_error();
			}
		}
		catch (winrt::hresult_error const&) {
			message_show(ICON_ALERT, L"str_err_paste", L"");
		}
		status_bar_set_pos();
	}

	//------------------------------
	// 画像を貼り付ける.
	//------------------------------
	IAsyncAction MainPage::xcvd_paste_image(void)
	{
		unselect_shape_all();

		// resume_background する前に UI 要素から値を得る.
		const float win_w = static_cast<float>(scp_main_panel().ActualWidth());
		const float win_h = static_cast<float>(scp_main_panel().ActualHeight());
		const float win_x = static_cast<float>(sb_horz().Value());
		const float win_y = static_cast<float>(sb_vert().Value());
		const float lt_x = m_main_bbox_lt.x;
		const float lt_y = m_main_bbox_lt.y;

		// resume_background しないと GetBitmapAsync が失敗することがある.
		winrt::apartment_context context;
		co_await winrt::resume_background();

		// クリップボードからビットマップ SoftwareBitmap を取り出す.
		RandomAccessStreamReference reference{
			co_await Clipboard::GetContent().GetBitmapAsync()
		};
		IRandomAccessStreamWithContentType stream{
			co_await reference.OpenReadAsync()
		};
		BitmapDecoder bmp_dec{
			co_await BitmapDecoder::CreateAsync(stream)
		};
		SoftwareBitmap bmp{
			co_await bmp_dec.GetSoftwareBitmapAsync(
				BitmapPixelFormat::Bgra8, BitmapAlphaMode::Straight)
		};

		// ウィンドウの真ん中に表示されるよう位置を求める.
		// 図形の大きさは元画像と同じにする.
		const float img_w = static_cast<float>(bmp.PixelWidth());
		const float img_h = static_cast<float>(bmp.PixelHeight());
		const float scale = m_main_scale;
		D2D1_POINT_2F lt{
			static_cast<FLOAT>(lt_x + (win_x + win_w * 0.5) / scale - img_w * 0.5),
			static_cast<FLOAT>(lt_y + (win_y + win_h * 0.5) / scale - img_h * 0.5)
		};

		// ビットマップから図形を作成する.
		ShapeImage* s = new ShapeImage(lt, D2D1_SIZE_F{ img_w, img_h }, bmp, 1.0f);
#if (_DEBUG)
		debug_leak_cnt++;
#endif
		// ビットマップを閉じ, ビットマップとデコーダーを解放する.
		bmp.Close();
		bmp = nullptr;
		bmp_dec = nullptr;
		stream.Close();
		stream = nullptr;
		reference = nullptr;

		const double g_len = (m_snap_grid ? m_main_sheet.m_grid_base + 1.0 : 0.0);
		xcvd_get_paste_pos(lt , /*<---*/lt, m_main_sheet.m_shape_list, g_len, m_snap_point / m_main_scale);
		s->set_pos_start(lt);
		{
			m_mutex_draw.lock();
			undo_push_null();
			undo_push_append(s);
			undo_push_select(s);
			m_mutex_draw.unlock();
		}

		co_await winrt::resume_foreground(Dispatcher());
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_append(s);
			summary_select(s);
		}
		main_bbox_update(s);
		main_panel_size();
		menu_is_enable();
		main_draw();

		//スレッドコンテキストを復元する.
		co_await context;
	}

	//------------------------------
	// 図形を貼り付ける.
	//------------------------------
	IAsyncAction MainPage::xcvd_paste_shape(void)
	{
		bool ok = false;	// 貼り付けの成功を判定

		// コルーチンが呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;

		// クリップボードから読み込むためのデータリーダーを得て, データを読み込む.
		IInspectable dt_object{
			co_await Clipboard::GetContent().GetDataAsync(CLIPBOARD_FORMAT_SHAPES)
		};
		InMemoryRandomAccessStream ra_stream{
			unbox_value<InMemoryRandomAccessStream>(dt_object)
		};
		if (ra_stream.Size() <= static_cast<uint64_t>(UINT32_MAX)) {
			IInputStream in_stream{
				ra_stream.GetInputStreamAt(0)
			};
			DataReader dt_reader{
				DataReader(in_stream)
			};
			uint32_t ra_size = static_cast<uint32_t>(ra_stream.Size());
			uint32_t operation{
				co_await dt_reader.LoadAsync(ra_size)
			};
			// データリーダーに読み込めたか判定する.
			if (operation == ra_size) {
				// 図形のためのメモリの確保が別スレッドで行われた場合, 
				// D2DERR_WRONG_STATE を引き起こすことがある.
				// 図形を貼り付ける前に, スレッドをメインページの UI スレッドに変える.
				co_await winrt::resume_foreground(Dispatcher());
				// データリーダーから貼り付けリストを読み込み, それが空でないか判定する.
				SHAPE_LIST slist_pasted;	// 貼り付けリスト
				if (slist_read(slist_pasted, dt_reader) && !slist_pasted.empty()) {
					m_mutex_draw.lock();
					// 図形リストの中の図形の選択をすべて解除する.
					undo_push_null();
					unselect_shape_all();
					// 得られたリストの各図形について以下を繰り返す.
					for (auto s : slist_pasted) {
						// 一覧が表示されてるか判定する.
						if (summary_is_visible()) {
							summary_append(s);
						}
						undo_push_append(s);
						main_bbox_update(s);
					}
					m_mutex_draw.unlock();
					main_panel_size();
					menu_is_enable();
					main_draw();
					slist_pasted.clear();
					ok = true;
				}
			}
			// データリーダーを閉じる.
			dt_reader.Close();
			in_stream.Close();
		}
		if (!ok) {
			message_show(ICON_ALERT, L"str_err_paste", L"");
		}
		co_await context;
	}

	//------------------------------
	// 文字列を貼り付ける
	//------------------------------
	IAsyncAction MainPage::xcvd_paste_text(void)
	{
		// コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// クリップボードから読み込むためのデータリーダーを得る.
		const winrt::hstring text{ co_await Clipboard::GetContent().GetTextAsync() };

		if (!text.empty()) {
			if (m_core_text_focused != nullptr) {
				core_text_insert(text.data(), static_cast<uint32_t>(text.size()));
			}
			// 文字列検索パネルのテキストボックスにフォーカスが無ければ文字列図形としてはりつける.
			else {

				undo_push_null();
				unselect_shape_all();

				// パネルの大きさで文字列図形を作成する,.
				const float scale = m_main_scale;
				//const float scale = m_main_sheet.m_sheet_scale;
				const float win_x = static_cast<float>(sb_horz().Value()) / scale;	// 用紙の表示されている左位置
				const float win_y = static_cast<float>(sb_vert().Value()) / scale;	// 用紙の表示されている上位置
				const float win_w = min(static_cast<float>(scp_main_panel().ActualWidth()) / scale, m_main_sheet.m_sheet_size.width); // 用紙の表示されている幅
				const float win_h = min(static_cast<float>(scp_main_panel().ActualHeight()) / scale, m_main_sheet.m_sheet_size.height); // 用紙の表示されている高さ
				const float lt_x = m_main_bbox_lt.x;
				const float lt_y = m_main_bbox_lt.y;
				ShapeText* t = new ShapeText(D2D1_POINT_2F{ 0.0f, 0.0f }, D2D1_POINT_2F{ win_w, win_h }, wchar_cpy(text.c_str()), &m_main_sheet);
#if (_DEBUG)
				debug_leak_cnt++;
#endif
				// 枠を文字列に合わせる.
				const double g_len = (m_snap_grid ? m_main_sheet.m_grid_base + 1.0 : 0.0);
				t->fit_frame_to_text(static_cast<FLOAT>(g_len));
				// パネルの中央になるよう左上位置を求める.
				D2D1_POINT_2F lt{
					static_cast<FLOAT>(lt_x + (win_x + win_w * 0.5) - t->m_lineto.x * 0.5),
					static_cast<FLOAT>(lt_y + (win_y + win_h * 0.5) - t->m_lineto.y * 0.5)
				};
				xcvd_get_paste_pos(lt, /*<---*/lt, m_main_sheet.m_shape_list, g_len, m_snap_point / scale);
				t->set_pos_start(lt);
				{
					m_mutex_draw.lock();
					undo_push_append(t);
					undo_push_select(t);
					m_mutex_draw.unlock();
				}

				// 一覧が表示されてるか判定する.
				if (summary_is_visible()) {
					summary_append(t);
					summary_select(t);
				}
				main_bbox_update(t);
				main_panel_size();
				menu_is_enable();
				main_draw();
			}
		}
		else {
			message_show(ICON_ALERT, L"str_err_paste", L"");
		}
		co_await context;
	}

	// 貼り付ける位置を求める.
	// paste_pt	求める点
	// q	貼り付けたい点
	// g_len	方眼の大きさ
	// interval	点に点をくっつける間隔
	static void xcvd_get_paste_pos(D2D1_POINT_2F& paste_pt, const D2D1_POINT_2F src_pt, const SHAPE_LIST& slist, const double g_len, const float interval)
	{
		D2D1_POINT_2F r;	// 最も近い点への位置ベクトル
		if (g_len >= 1.0f && interval >= FLT_MIN && slist_find_vertex_closest(slist, src_pt, interval, r)) {
			pt_sub(r, src_pt, r);
			D2D1_POINT_2F g;	// 最も近い方眼への位置ベクトル
			pt_round(src_pt, g_len, g);
			pt_sub(g, src_pt, g);
			if (pt_abs2(g) < pt_abs2(r)) {
				paste_pt = g;
			}
			else {
				paste_pt = r;
			}
		}
		else if (g_len >= 1.0f) {
			pt_round(src_pt, g_len, paste_pt);
		}
		else if (interval >= FLT_MIN) {
			slist_find_vertex_closest(slist, src_pt, interval, paste_pt);
		}
		else {
			paste_pt = src_pt;
		}
	}

	// 貼り付ける点を得る.
	void MainPage::xcvd_paste_pos(D2D1_POINT_2F& past_pt, const D2D1_POINT_2F src_pt) const noexcept
	{
		xcvd_get_paste_pos(past_pt, src_pt, m_main_sheet.m_shape_list, m_snap_grid ? m_main_sheet.m_grid_base + 1.0 : 0.0, m_snap_point / m_main_scale);
	}

}