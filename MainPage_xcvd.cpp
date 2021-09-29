//-------------------------------
// MainPage_xcvd.cpp
// 切り取りとコピー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
	using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;
	using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;
	using winrt::Windows::ApplicationModel::DataTransfer::DataPackageView;
	using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapDecoder;
	using winrt::Windows::Graphics::Imaging::BitmapEncoder;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::Storage::Streams::DataWriter;
	using winrt::Windows::Storage::Streams::IInputStream;
	using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;
	using winrt::Windows::Storage::Streams::IRandomAccessStreamWithContentType;
	using winrt::Windows::Storage::Streams::IOutputStream;
	using winrt::Windows::Storage::Streams::RandomAccessStreamReference;

	const winrt::param::hstring CLIPBOARD_SHAPES{ L"graph_paper_shapes_data" };	// 図形データのクリップボード書式
	//const winrt::param::hstring CLIPBOARD_TIFF{ L"TaggedImageFileFormat" };	// TIFF のクリップボード書式 (Windows10 ではたぶん使われない)

	// 貼り付ける位置を求める.
	static void xcvd_paste_pos(D2D1_POINT_2F& pos, const SHAPE_LIST& slist, const double grid_len, const float vert_stick);

	// 編集メニューの「コピー」が選択された.
	// プログラム終了したら中身が消える！
	IAsyncAction MainPage::xcvd_copy_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr bool REDUCED = true;

		if (m_list_sel_cnt == 0) {
			// 終了する.
			return;
		}
		// コルーチンが呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// 選択された図形のリストを得る.
		SHAPE_LIST list_selected;
		slist_selected<Shape>(m_sheet_main.m_shape_list, list_selected);
		// リストから降順に, 最初に見つかった文字列図形の文字列, あるいは画像図形の画像を得る.
		wchar_t* txt = nullptr;
		RandomAccessStreamReference img_ref = nullptr;
		for (auto it = list_selected.rbegin(); it != list_selected.rend(); it++) {
			Shape* s = *it;
			if (txt == nullptr) {
				// 文字列をポインターに格納する.
				s->get_text_content(txt);
			}
			if (img_ref == nullptr && typeid(*s) == typeid(ShapeImage)) {
				// ビットマップをストリーム参照に格納する.
				InMemoryRandomAccessStream img_stream{ InMemoryRandomAccessStream() };
				co_await static_cast<ShapeImage*>(s)->copy_to(BitmapEncoder::BmpEncoderId(), img_stream);
				if (img_stream.Size() > 0) {
					img_ref = RandomAccessStreamReference::CreateFromStream(img_stream);
				}
			}
			if (txt != nullptr && img_ref != nullptr) {
				// 文字列と画像図形, 両方とも見つかったなら中断する.
				break;
			}
		}
		// メモリストリームを作成して, データライターを得る.
		InMemoryRandomAccessStream mem_stream{ InMemoryRandomAccessStream() };
		IOutputStream out_stream{ mem_stream.GetOutputStreamAt(0) };
		DataWriter dt_writer{ DataWriter(out_stream) };
		// データライターに選択された図形のリストを書き込む.
		slist_write<REDUCED>(list_selected, /*--->*/dt_writer);
		// リストを破棄する.
		list_selected.clear();
		// メモリストリームにデータライターの内容を格納し, 格納したバイト数を得る.
		uint32_t n_byte{ co_await dt_writer.StoreAsync() };
		if (n_byte > 0) {
			// メインページの UI スレッドに変える.
			co_await winrt::resume_foreground(Dispatcher());
			// データパッケージを作成し, データパッケージにメモリストリームを格納する.
			DataPackage dt_pkg{ DataPackage() };
			dt_pkg.RequestedOperation(DataPackageOperation::Copy);
			dt_pkg.SetData(CLIPBOARD_SHAPES, winrt::box_value(mem_stream));
			// 文字列が得られたか判定する.
			if (txt != nullptr) {
				// データパッケージにテキストを格納する.
				dt_pkg.SetText(txt);
			}
			// 画像が得られたか判定する.
			if (img_ref != nullptr) {
				// データパッケージに画像を格納する.
				dt_pkg.SetBitmap(img_ref);
			}
			// クリップボードにデータパッケージを格納する.
			Clipboard::SetContent(dt_pkg);
			dt_pkg = nullptr;
		}
		// データライターを閉じる.
		dt_writer.Close();
		dt_writer = nullptr;
		// 出力ストリームを閉じる.
		// メモリストリームは閉じちゃダメ.
		out_stream.Close();
		out_stream = nullptr;
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
		// 選択された図形の数がゼロか判定する.
		if (m_list_sel_cnt == 0) {
			// 終了する.
			return;
		}
		// 選択された図形のリストを得る.
		SHAPE_LIST list_selected;
		slist_selected<Shape>(m_sheet_main.m_shape_list, list_selected);
		// リストの各図形について以下を繰り返す.
		m_dx_mutex.lock();
		for (auto s : list_selected) {
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_remove(s);
			}
			// 図形を削除し, その操作をスタックに積む.
			ustack_push_remove(s);
		}
		ustack_push_null();
		m_dx_mutex.unlock();

		xcvd_is_enabled();
		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
	}

	// 編集メニューの可否を設定する.
	// 枠を文字列に合わせる, と元の大きさに戻す.
	// 選択の有無やクラスごとに図形を数え, メニュー項目の可否を判定する.
	void MainPage::xcvd_is_enabled(void)
	{
		ustack_is_enable();

		uint32_t undeleted_cnt = 0;	// 消去フラグがない図形の数
		uint32_t selected_cnt = 0;	// 選択された図形の数
		uint32_t selected_group_cnt = 0;	// 選択されたグループ図形の数
		uint32_t runlength_cnt = 0;	// 選択された図形の連続の数
		uint32_t selected_text_cnt = 0;	// 選択された文字列図形の数
		uint32_t text_cnt = 0;	// 文字列図形の数
		uint32_t selected_image_cnt = 0;	// 選択された画像図形の数
		bool fore_selected = false;	// 最前面の図形の選択フラグ
		bool back_selected = false;	// 最背面の図形の選択フラグ
		bool prev_selected = false;	// ひとつ背面の図形の選択フラグ
		slist_count(
			m_sheet_main.m_shape_list,
			undeleted_cnt,
			selected_cnt,
			selected_group_cnt,
			runlength_cnt,
			selected_text_cnt,
			text_cnt,
			selected_image_cnt,
			fore_selected,
			back_selected,
			prev_selected
		);

		// 消去されていない図形がひとつ以上ある場合.
		const auto exists_undeleted = (undeleted_cnt > 0);
		// 選択された図形がひとつ以上ある場合.
		const auto exists_selected = (selected_cnt > 0);
		// 選択された文字列がひとつ以上ある場合.
		const auto exists_selected_text = (selected_text_cnt > 0);
		// 文字列がひとつ以上ある場合.
		const auto exists_text = (text_cnt > 0);
		// 選択された画像がひとつ以上ある場合.
		const auto exists_selected_image = (selected_image_cnt > 0);
		// 選択されてない図形がひとつ以上ある場合.
		const auto exists_unselected = (selected_cnt < undeleted_cnt);
		// 選択された図形がふたつ以上ある場合.
		const auto exists_selected_2 = (selected_cnt > 1);
		// 選択されたグループがひとつ以上ある場合.
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
		const DataPackageView& dp_view = Clipboard::GetContent();
		mfi_xcvd_paste().IsEnabled(
			dp_view.Contains(CLIPBOARD_SHAPES) ||
			dp_view.Contains(StandardDataFormats::Text()) ||
			dp_view.Contains(StandardDataFormats::Bitmap())); 
			//|| dp_view.Contains(CLIPBOARD_TIFF));
		mfi_xcvd_delete().IsEnabled(exists_selected);
		mfi_select_all().IsEnabled(exists_unselected);
		mfi_group().IsEnabled(exists_selected_2);
		mfi_ungroup().IsEnabled(exists_selected_group);
		mfi_edit_text().IsEnabled(exists_selected_text);
		mfi_find_text().IsEnabled(exists_text);
		mfi_text_fit_frame_to().IsEnabled(exists_selected_text);
		mfi_bring_forward().IsEnabled(enable_forward);
		mfi_bring_to_front().IsEnabled(enable_forward);
		mfi_send_to_back().IsEnabled(enable_backward);
		mfi_send_backward().IsEnabled(enable_backward);
		mfi_summary_list().IsEnabled(exists_undeleted);
		mfi_image_revert_origin().IsEnabled(exists_selected_image);
		m_list_sel_cnt = selected_cnt;
	}

	// 図形を貼り付ける.
	IAsyncAction MainPage::xcvd_paste_shape(void)
	{
		bool ok = false;	// 貼り付けの成功を判定

		// コルーチンが呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;

		// クリップボードから読み込むためのデータリーダーを得て, データを読み込む.
		IInspectable dt_object{ co_await Clipboard::GetContent().GetDataAsync(CLIPBOARD_SHAPES) };
		InMemoryRandomAccessStream ra_stream{ unbox_value<InMemoryRandomAccessStream>(dt_object) };
		if (ra_stream.Size() <= static_cast<uint64_t>(UINT32_MAX)) {
			IInputStream in_stream{ ra_stream.GetInputStreamAt(0) };
			DataReader dt_reader{ DataReader(in_stream) };
			uint32_t ra_size = static_cast<uint32_t>(ra_stream.Size());
			uint32_t operation{ co_await dt_reader.LoadAsync(ra_size) };
			// 図形のためのメモリの確保が別スレッドで行われた場合, D2DERR_WRONG_STATE を引き起こすことがある.
			// 図形を貼り付ける前に, スレッドをメインページの UI スレッドに変える.
			co_await winrt::resume_foreground(Dispatcher());
			// データリーダーに読み込めたか判定する.
			if (operation == ra_size) {
				// データリーダーから貼り付けリストを読み込み, それが空でないか判定する.
				SHAPE_LIST slist_pasted;	// 貼り付けリスト
				if (slist_read(slist_pasted, dt_reader) && !slist_pasted.empty()) {
					m_dx_mutex.lock();
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

	IAsyncAction MainPage::xcvd_paste_text(void)
	{
		// コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// クリップボードから読み込むためのデータリーダーを得る.
		const winrt::hstring text{ co_await Clipboard::GetContent().GetTextAsync() };
		if (!text.empty()) {
			unselect_all();

			// パネルの大きさで文字列図形を作成する,.
			const float scale = m_sheet_main.m_sheet_scale;
			const float win_x = static_cast<float>(sb_horz().Value());
			const float win_y = static_cast<float>(sb_vert().Value());
			const float win_w = static_cast<float>(scp_sheet_panel().ActualWidth());
			const float win_h = static_cast<float>(scp_sheet_panel().ActualHeight());
			const float min_x = m_sheet_min.x;
			const float min_y = m_sheet_min.y;
			auto t = new ShapeText(D2D1_POINT_2F{ 0.0f, 0.0f }, D2D1_POINT_2F{ win_w / scale, win_h / scale }, wchar_cpy(text.c_str()), &m_sheet_main);
#if (_DEBUG)
			debug_leak_cnt++;
#endif
			// 枠の大きさを文字列に合わせる.
			t->adjust_bbox(m_sheet_main.m_grid_snap ? m_sheet_main.m_grid_base + 1.0f : 0.0f);
			// パネルの中央になるよう左上位置を求める.
			D2D1_POINT_2F pos{
				static_cast<FLOAT>(min_x + (win_x + win_w * 0.5) / scale - t->m_vec[0].x * 0.5),
				static_cast<FLOAT>(min_y + (win_y + win_h * 0.5) / scale - t->m_vec[0].y * 0.5)
			};
			const double grid_len = (m_sheet_main.m_grid_snap ? m_sheet_main.m_grid_base + 1.0 : 0.0);
			const float vert_stick = m_misc_vert_stick / m_sheet_main.m_sheet_scale;
			xcvd_paste_pos(pos, /*<---*/m_sheet_main.m_shape_list, grid_len, vert_stick);
			t->set_pos_start(pos);
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
		co_await context;
	}

	// 貼り付ける位置を求める.
	// grid_len	方眼の大きさ
	// vert_stick	頂点をくっつける距離
	static void xcvd_paste_pos(D2D1_POINT_2F& pos, const SHAPE_LIST& slist, const double grid_len, const float vert_stick)
	{
		D2D1_POINT_2F v_pos;
		if (grid_len >= 1.0f && vert_stick >= FLT_MIN &&
			slist_find_vertex_closest(slist, pos, vert_stick, v_pos)) {
			// 図形の左上位置を方眼の大きさで丸め, 元の値との距離 (の自乗) を求める.
			D2D1_POINT_2F g_pos;
			pt_round(pos, grid_len, g_pos);
			D2D1_POINT_2F g_vec;
			pt_sub(g_pos, pos, g_vec);
			const double g_abs = pt_abs2(g_vec);
			// 近傍の頂点との距離 (の自乗) を求める.
			D2D1_POINT_2F v_vec;
			pt_sub(v_pos, pos, v_vec);
			const double v_abs = pt_abs2(v_vec);
			if (g_abs < v_abs) {
				pos = g_pos;
			}
			else {
				pos = v_pos;
			}
		}
		else if (grid_len >= 1.0f) {
			pt_round(pos, grid_len, pos);
		}
		else if (vert_stick >= FLT_MIN) {
			slist_find_vertex_closest(slist, pos, vert_stick, pos);
		}
	}

	IAsyncAction MainPage::xcvd_paste_image(void)
	{
		// コルーチンが最初に呼び出されたスレッドコンテキストを保存する.
		winrt::apartment_context context;

		unselect_all();

		// resume_background する前に UI から値を得る.
		const float win_w = static_cast<float>(scp_sheet_panel().ActualWidth());
		const float win_h = static_cast<float>(scp_sheet_panel().ActualHeight());
		const float win_x = static_cast<float>(sb_horz().Value());
		const float win_y = static_cast<float>(sb_vert().Value());
		const float min_x = m_sheet_min.x;
		const float min_y = m_sheet_min.y;

		// resume_background しないと GetBitmapAsync が失敗することがある.
		co_await winrt::resume_background();

		// クリップボードからビットマップ SoftwareBitmap を取り出す.
		RandomAccessStreamReference reference{ co_await Clipboard::GetContent().GetBitmapAsync() };
		IRandomAccessStreamWithContentType stream{ co_await reference.OpenReadAsync() };
		BitmapDecoder bmp_dec{ co_await BitmapDecoder::CreateAsync(stream) };
		SoftwareBitmap bmp{ co_await bmp_dec.GetSoftwareBitmapAsync(BitmapPixelFormat::Bgra8, BitmapAlphaMode::Straight) };

		// ウィンドウの真ん中に表示されるよう位置を求める.
		// 図形の大きさは元画像と同じにする.
		const float img_w = static_cast<float>(bmp.PixelWidth());
		const float img_h = static_cast<float>(bmp.PixelHeight());
		const float scale = m_sheet_main.m_sheet_scale;
		D2D1_POINT_2F pos{
			static_cast<FLOAT>(min_x + (win_x + win_w * 0.5) / scale - img_w * 0.5),
			static_cast<FLOAT>(min_y + (win_y + win_h * 0.5) / scale - img_h * 0.5)
		};
		const D2D1_SIZE_F view_size{ img_w, img_h };

		// ビットマップから図形を作成する.
		ShapeImage* img = new ShapeImage(pos, view_size, bmp, 1.0f);
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

		const double grid_len = (m_sheet_main.m_grid_snap ? m_sheet_main.m_grid_base + 1.0 : 0.0);
		const float vert_stick = m_misc_vert_stick / m_sheet_main.m_sheet_scale;
		xcvd_paste_pos(pos, /*<---*/m_sheet_main.m_shape_list, grid_len, vert_stick);
		img->set_pos_start(pos);

		m_dx_mutex.lock();
		ustack_push_append(img);
		ustack_push_select(img);
		ustack_push_null();
		m_dx_mutex.unlock();
		co_await winrt::resume_foreground(Dispatcher());
		ustack_is_enable();
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_append(img);
			summary_select(img);
		}
		xcvd_is_enabled();
		sheet_update_bbox(img);
		sheet_panle_size();
		sheet_draw();

		//スレッドコンテキストを復元する.
		co_await context;
	}

	// 編集メニューの「貼り付け」が選択された.
	void MainPage::xcvd_paste_click(IInspectable const&, RoutedEventArgs const&)
	{
		// Clipboard::GetContent() は, 
		// WinRT originate error 0x80040904
		// を引き起こすので, try ... catch 文が必要.
		try {
			// クリップボードに図形が含まれているか判定する.
			const DataPackageView& dp_view = Clipboard::GetContent();
			if (dp_view.Contains(CLIPBOARD_SHAPES)) {
				xcvd_paste_shape();
				return;
			}
			// クリップボードにテキストが含まれているか判定する.
			else if (dp_view.Contains(StandardDataFormats::Text())) {
				xcvd_paste_text();
				return;
			}
			// クリップボードに画像が含まれているか判定する.
			else if (dp_view.Contains(StandardDataFormats::Bitmap())) {// || dp_view.Contains(CLIPBOARD_TIFF)) {
				xcvd_paste_image();
				return;
			}
		}
		catch (winrt::hresult_error const&) {
		}
		message_show(ICON_ALERT, L"str_err_paste", L"");
	}

}