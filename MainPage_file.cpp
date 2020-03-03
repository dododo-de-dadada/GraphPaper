//-------------------------------
// MainPage_file.cpp
// ファイルの読み書き, アプリの終了
//-------------------------------
#include "pch.h"
#include "MainPage.h"

//	file_open_async
//		file_wait_cursor
//		PickSingleFileAsync
//		file_read_async
//			winrt::resume_background
//			OpenAsync
//			winrt::resume_foreground
//			mru_add_file
//				mru_update_menu_items
//			finish_file_read
//	file_read_recent_async
//		file_wait_cursor
//		mru_get_file
//			GetFileAsync
//			winrt::resume_foreground
//		file_read_async
//			winrt::resume_background
//			OpenAsync
//			winrt::resume_foreground
//			mru_add_file
//				mru_update_menu_items
//			finish_file_read
//		winrt::resume_foreground
//	file_save_as_async
//		file_wait_cursor
//		mru_get_file
//			GetFileAsync
//			winrt::resume_foreground
//		PickSaveFileAsync
//		file_write_svg_async
//			winrt::resume_background
//			OpenAsync
//			winrt::resume_foreground
//		file_write_gpf_async
//			winrt::resume_background
//			OpenAsync
//			winrt::resume_foreground
//			mru_add_file
//				mru_update_menu_items
//	file_save_async
//		mru_get_file
//			GetFileAsync
//			winrt::resume_foreground
//		file_save_as_async
//		file_wait_cursor
//		file_write_gpf_async
//			winrt::resume_background
//			winrt::resume_foreground
//			mru_add_file
//				mru_update_menu_items
//	mfi_exit_click
//		file_save_and_exit_async
//			file_save_async
//			Exit
//		Exit
//	mfi_new_click
//		file_save_and_new_async
//			file_save_async
//			new_doc
//		new_doc
//	mfi_open_click
//		file_save_and_open_async
//			file_save_async
//			file_open_async
//		file_open_async
//	mfi_recent_files_N_click
//		file_read_recent
//			mru_get_token
//			file_save_and_read_recent_async
//				file_save_async
//				file_read_recent_async
//			file_read_recent_async
//	mfi_save_as_click
//		file_save_as_async
//	mfi_save_click
//		file_save_async
/*
ただし、4つのWindowsランタイム非同期操作タイプ（IAsyncXxx）のいずれかを
co_awaitした場合、C ++ / WinRTはco_awaitの時点で呼び出しコンテキストをキャプチャします。
また、継続が再開されたときに、そのコンテキストにいることが保証されます。
C ++ / WinRTは、呼び出し側のコンテキストに既にいるかどうかを確認し、
そうでない場合は切り替えます。
*/

// GetFileFromPathAsync を E_ACCESSDENIED なしに使うには以下が必要になる.
// 1. XAMLテキストエディタを使って, Pakage.appxmanifest に broadFileSystemAccess を追加する.
//	Pakage.appxmanifest の <Package> のプロパティーに
//	xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities" を追加する
//	IgnorableNamespaces="uap mp" を IgnorableNamespaces="uap mp rescap" に変更する
// 2. もしなければ <Capabilities> タグを <Package> の子要素として追加する.
//	<Capabilities> の子要素として <rescap:Capability Name="broadFileSystemAccess" /> を加える
// 3. コンパイルするたびに, 設定を使ってアプリにファイルアクセスを許可する.
//	スタートメニュー > 設定 > プライバー > ファイル システム > ファイルシステムにアクセスできるアプリを選ぶ
//	表示されているアプリをオンにする.
// または,
//	スタートメニュー > 設定 > アプリ > アプリと機能
//	表示されているアプリをクリックする.
//	詳細オプション > アプリのアクセス許可
//	ファイル システムをオンにする.

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Core::CoreCursorType;

	static auto const& CUR_WAIT = CoreCursor(CoreCursorType::Wait, 0);	// 待機カーソル.
	constexpr wchar_t DESC_GPF[] = L"str_desc_gpf";	// 拡張子 gpf の説明
	constexpr wchar_t DESC_SVG[] = L"str_desc_svg";	// 拡張子 svg の説明
	constexpr wchar_t ERR_FONT[] = L"str_unavailable_font";	// 有効でない書体のエラーメッセージのリソース名
	constexpr wchar_t ERR_READ[] = L"str_err_read";	// 読み込みエラーメッセージのリソース名
	constexpr wchar_t ERR_WRITE[] = L"str_err_write";	// 書き込みエラーメッセージのリソース名
	constexpr wchar_t FT_GPF[] = L".gpf";	// 図形データファイルの拡張子
	constexpr wchar_t FT_SVG[] = L".svg";	// SVG ファイルの拡張子

	//	待機カーソルを表示, 表示する前のカーソルを得る.
	CoreCursor MainPage::file_wait_cursor(void) const
	{
		auto const& c_win = Window::Current().CoreWindow();
		auto p_cur = c_win.PointerCursor();
		c_win.PointerCursor(CUR_WAIT);
		return p_cur;
	}

	// ファイルを非同期に開く
	IAsyncAction MainPage::file_open_async(void)
	{
		using winrt::Windows::Storage::Pickers::FileOpenPicker;

		//	待機カーソルを表示, 表示する前のカーソルを得る.
		auto const& p_cur = file_wait_cursor();
		//	コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;
		//	ファイル「オープン」ピッカーを取得して開く.
		auto o_picker{ FileOpenPicker() };
		//o_picker.ViewMode(PickerViewMode::Thumbnail);
		//	ファイルタイプを「.gdf」に設定する.
		o_picker.FileTypeFilter().Append(FT_GPF);
		//	ピッカーを非同期で表示してストレージファイルを取得する.
		//	（「閉じる」ボタンが押された場合ストレージファイルとして nullptr が返る.）
		auto s_file{ co_await o_picker.PickSingleFileAsync() };
		if (s_file != nullptr) {
			//	ファイルがnullptrの場合、
			//	ファイルを非同期で読む.
			co_await file_read_async(s_file);
			//	ストレージファイルを解放する.
			s_file = nullptr;
		}
		//	ピッカーを解放する.
		o_picker = nullptr;
		//	スレッドコンテキストを復元する.
		co_await context;
		//	ウィンドウカーソルを復元する.
		Window::Current().CoreWindow().PointerCursor(p_cur);
	}

	//	ストレージファイルを非同期に読む.
	//	s_file	読み込むストレージファイル
	//	suspend	中断フラグ
	//	戻り値	読み込めたら S_OK.
	//	中断フラグが立っている場合, 操作スタックも保存する.
	IAsyncOperation<winrt::hresult> MainPage::file_read_async(StorageFile const& s_file, const bool suspend) noexcept
	{
		using winrt::Windows::Storage::FileAccessMode;

		auto hr = E_FAIL;
		//	コルーチンの開始時のスレッドコンテキストを保存する.
		//	スレッドをバックグラウンドに変更する.
		winrt::apartment_context context;
		co_await winrt::resume_background();
		try {
			//	ファイルを開いてランダムアクセスストリームを得る.
			//	ストリームから読み込むためのデータリーダーを作成する.
			//	データリーダーにファイルを読み込む.
			auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::Read) };
			auto dt_reader{ DataReader(ra_stream.GetInputStreamAt(0)) };
			co_await dt_reader.LoadAsync(static_cast<uint32_t>(ra_stream.Size()));

			text_find_read(dt_reader);
			stat_read(dt_reader);
			m_page_panel.read(dt_reader);
			//	操作スタックを消去する.
			undo_clear();
			s_list_clear(m_list_shapes);
#if defined(_DEBUG)
			if (debug_leak_cnt != 0) {
				//	メインページの UI スレッドにスレッドを変更する.
				co_await winrt::resume_foreground(this->Dispatcher());
				cd_message_show(L"Memory leak occurs", {});
				//	スレッドコンテキストを復元する.
				co_await context;
				//	結果を返し終了する.
				co_return hr;
			}
#endif
			s_list_read(m_list_shapes, dt_reader);
			if (suspend) {
				//	中断フラグが立っている場合,
				//	データリーダーから操作スタックを読み込む.
				undo_read(dt_reader);
			}
			//	データリーダーを閉じる.
			dt_reader.Close();
			//	ストリームを閉じる.
			ra_stream.Close();
			hr = S_OK;
		}
		catch (winrt::hresult_error const& e) {
			//	エラーが発生した場合, 
			//	エラーコードを結果に格納する.
			hr = e.code();
		}
		if (hr != S_OK) {
			//	結果が S_OK でない場合,
			//	スレッドをメインページの UI スレッドに変える.
			//	読み込み失敗のメッセージダイアログを表示する.
			co_await winrt::resume_foreground(this->Dispatcher());
			cd_message_show(ERR_READ, s_file.Path());
		}
		else if (suspend == false) {
			//	中断フラグがない場合,
			//	スレッドをメインページの UI スレッドに変更える.
			co_await winrt::resume_foreground(this->Dispatcher());
			//	ストレージファイルを最近使ったファイルに追加する.
			mru_add_file(s_file);
			finish_file_read();
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return hr;
	}

	//	ファイルに非同期に保存して, アプリケーションを終了する.
	IAsyncAction MainPage::file_save_and_exit_async(void)
	{
		using winrt::Windows::UI::Xaml::Application;

		//	ファイルに非同期に保存する
		if (co_await file_save_async() == S_OK) {
			//	保存できた場合,
			//	アプリケーションを終了する.
			release();
			Application::Current().Exit();
		}
	}

	//	ファイルに非同期に保存して, 空白の文書にする.
	IAsyncAction MainPage::file_save_and_new_async(void)
	{
		//	ファイルに非同期に保存する
		if (co_await file_save_async() == S_OK) {
			//	保存できた場合,
			//	空白の文書にする.
			new_doc();
		}
	}

	//	ファイルに非同期に保存して, ファイルを非同期に開く.
	IAsyncAction MainPage::file_save_and_open_async(void)
	{
		//	ファイルに非同期に保存する
		if (co_await file_save_async() == S_OK) {
			//	保存できた場合,
			//	ファイルを非同期に開く.
			co_await file_open_async();
		}
	}

	//	名前を付けてファイルに非同期に保存する.
	//	permit_svg	SVG 許可フラグ.
	IAsyncOperation<winrt::hresult> MainPage::file_save_as_async(const bool permit_svg) noexcept
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::Storage::Pickers::FileSavePicker;

		auto hr = E_FAIL;
		//	待機カーソルを表示, 表示する前のカーソルを得る.
		auto const& p_cur = file_wait_cursor();
		//	コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;
		try {
			// ファイル保存ピッカーを得る.
			// ファイルタイプに拡張子 GPF とその説明を追加する.
			auto s_picker{ FileSavePicker() };
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			auto desc_gpf = r_loader.GetString(DESC_GPF);
			auto type_gpf = winrt::single_threaded_vector<winrt::hstring>({ FT_GPF });
			s_picker.FileTypeChoices().Insert(desc_gpf, type_gpf);
			//	SVG 許可フラグが立っているか判定する.
			if (permit_svg) {
				//	フラグが立っている場合, 
				//	ファイルタイプに拡張子 SVG とその説明を追加する.
				auto desc_svg = r_loader.GetString(DESC_SVG);
				auto type_svg = winrt::single_threaded_vector<winrt::hstring>({ FT_SVG });
				s_picker.FileTypeChoices().Insert(desc_svg, type_svg);
			}
			//	最近使ったファイルのトークンが空か判定する.
			if (m_mru_token.empty()) {
				//	トークンが空の場合, 
				//	提案されたファイル名に拡張子を格納する.
				s_picker.SuggestedFileName(FT_GPF);
			}
			else {
				//	トークンが空でない場合,
				//	ストレージファイルを最近使ったファイルのトークンから得る.
				//	得られたファイルの名前を, 提案されたファイル名に格納する.
				auto s_file{ co_await mru_get_file(m_mru_token) };
				if (s_file != nullptr) {
					if (s_file.FileType() == FT_GPF) {
						s_picker.SuggestedFileName(s_file.Name());
					}
					s_file = nullptr;
				}
			}
			//	ファイル保存ピッカーを表示し, ストレージファイルを得る.
			auto s_file{ co_await s_picker.PickSaveFileAsync() };
			//	ストレージファイルを取得したか判定する.
			if (s_file != nullptr) {
				//	取得した場合,
				if (s_file.FileType() == FT_SVG) {
					//	ファイルタイプが SVG の場合,
					//	SVG としてストレージファイルに非同期に書き込む.
					hr = co_await file_write_svg_async(s_file);
				}
				else {
					//	ファイルタイプが SVG 以外の場合,
					//	図形データをストレージファイルに非同期に書き込む.
					hr = co_await file_write_gpf_async(s_file);
				}
				//	ストレージファイルを破棄する.
				s_file = nullptr;
			}
			//	ピッカーを破棄する.
			s_picker = nullptr;
		}
		catch (winrt::hresult_error const& e) {
			//	エラーが発生した場合, エラーコードを結果に格納する.
			hr = e.code();
		}
		//	ウィンドウのカーソルを復元する.
		co_await winrt::resume_foreground(this->Dispatcher());
		Window::Current().CoreWindow().PointerCursor(p_cur);
		//	スレッドコンテキストを復元する.
		co_await context;
		//	結果を返し終了する.
		co_return hr;
	}

	// ファイルに非同期に保存する
	IAsyncOperation<winrt::hresult> MainPage::file_save_async(void) noexcept
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		// 最近使ったファイルのトークンからストレージファイルを得る.
		auto s_file{ co_await mru_get_file(m_mru_token) };
		if (s_file == nullptr) {
			// ストレージファイルが空の場合,
			// 名前を付けてファイルに非同期に保存する.
			co_return co_await file_save_as_async(false);
		}

		auto hr = E_FAIL;
		//	待機カーソルを表示, 表示する前のカーソルを得る.
		auto const& p_cur = file_wait_cursor();
		//	コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// スレッドをバックグラウンドに変える.
		//co_await winrt::resume_background();
		// 図形データをストレージファイルに非同期に書き込み, 結果を得る.
		hr = co_await file_write_gpf_async(s_file);
		// 結果を判定する.
		if (hr != S_OK) {
			// 結果が S_OK でない場合,
			// スレッドをメインページの UI スレッドに変える.
			// 最近使ったファイルのエラーメッセージダイアログを表示する.
			co_await winrt::resume_foreground(this->Dispatcher());
			cd_message_show(ERR_WRITE, m_mru_token);
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// ウィンドウのカーソルを復元する.
		Window::Current().CoreWindow().PointerCursor(p_cur);
		// 結果を返し終了する.
		co_return hr;
	}

	//	図形データをストレージファイルに非同期に書き込む.
	//	s_file	ストレージファイル
	//	suspend	中断フラグ
	//	戻り値	書き込みに成功したら true
	IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async(StorageFile const& s_file, const bool suspend)
	{
		using winrt::Windows::Storage::CachedFileManager;
		using winrt::Windows::Storage::FileAccessMode;
		using winrt::Windows::Storage::Provider::FileUpdateStatus;
		constexpr auto REDUCE = true;

		//	E_FAIL を結果に格納する.
		auto hr = E_FAIL;
		//	コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// スレッドをバックグラウンドに変える.
		co_await winrt::resume_background();
		try {
			// ファイル更新の遅延を設定する.
			// ストレージファイルを開いてランダムアクセスストリームを得る.
			// ランダムアクセスストリームからデータライターを作成する.
			CachedFileManager::DeferUpdates(s_file);
			auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::ReadWrite) };
			auto dt_writer{ DataWriter(ra_stream.GetOutputStreamAt(0)) };
			//dt_writer.WriteUInt32(static_cast<uint32_t>(m_col_style));
			text_find_write(dt_writer);
			stat_write(dt_writer);
			m_page_panel.write(dt_writer);
			if (suspend) {
				s_list_write<!REDUCE>(m_list_shapes, dt_writer);
				undo_write(dt_writer);
			}
			else {
				s_list_write<REDUCE>(m_list_shapes, dt_writer);
			}
			ra_stream.Size(ra_stream.Position());
			co_await dt_writer.StoreAsync();
			co_await ra_stream.FlushAsync();
			//	データライターを閉じる.
			dt_writer.Close();
			//	ストリームを閉じる.
			ra_stream.Close();
			//	遅延させたファイル更新を完了する.
			auto fu_status{ co_await CachedFileManager::CompleteUpdatesAsync(s_file) };
			if (fu_status == FileUpdateStatus::Complete) {
				//	完了した場合, 
				//	S_OK を結果に格納する.
				hr = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			//	エラーが発生した場合, 
			//	エラーコードを結果に格納する.
			hr = e.code();
		}
		if (hr != S_OK) {
			//	結果が S_OK でない場合,
			//	スレッドをメインページの UI スレッドに変える.
			//	書き込み失敗のメッセージダイアログを表示する.
			co_await winrt::resume_foreground(this->Dispatcher());
			cd_message_show(ERR_WRITE, s_file.Path());
		}
		else if (suspend == false) {
			//	中断フラグがない場合,
			//	スレッドをメインページの UI スレッドに変える.
			co_await winrt::resume_foreground(this->Dispatcher());
			mru_add_file(s_file);
			//	false を操作スタックの更新フラグに格納する.
			m_stack_push = false;
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return hr;
	}

	//	SVG としてデータライターに書き込む.
	//	dt_writer	データライター
	void MainPage::file_write_svg(DataWriter const& dt_writer)
	{
		constexpr char XML_DEC[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" SVG_NL;
		constexpr char DOCTYPE[] = "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" SVG_NL;
		constexpr char SVG_TAG[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" ";

		//	XML 宣言を書き込む.
		write_svg(XML_DEC, dt_writer);
		//	DOCTYPE を書き込む.
		write_svg(DOCTYPE, dt_writer);
		//	svg タグの開始を書き込む.
		write_svg(SVG_TAG, dt_writer);
		//	幅と高さの属性を書き込む.
		constexpr char* SVG_UNIT_PX = "px";
		constexpr char* SVG_UNIT_IN = "in";
		constexpr char* SVG_UNIT_MM = "mm";
		constexpr char* SVG_UNIT_PT = "pt";
		const auto dpi = m_page_dx.m_logical_dpi;
		double w, h;
		char buf[256];
		char* u;
		switch (m_page_unit) {
		default:
		case LEN_UNIT::PIXEL:
			w = m_page_panel.m_page_size.width;
			h = m_page_panel.m_page_size.height;
			u = SVG_UNIT_PX;
			break;
		case LEN_UNIT::INCH:
			w = m_page_panel.m_page_size.width / dpi;
			h = m_page_panel.m_page_size.height / dpi;
			u = SVG_UNIT_IN;
			break;
		case LEN_UNIT::MILLI:
			w = m_page_panel.m_page_size.width * MM_PER_INCH / dpi;
			h = m_page_panel.m_page_size.height * MM_PER_INCH / dpi;
			u = SVG_UNIT_MM;
			break;
		case LEN_UNIT::POINT:
			w = m_page_panel.m_page_size.width * PT_PER_INCH / dpi;
			h = m_page_panel.m_page_size.height * PT_PER_INCH / dpi;
			u = SVG_UNIT_PT;
			break;
		}
		std::snprintf(buf, sizeof(buf), "width=\"%lf%s\" height=\"%lf%s\" ", w, u, h, u);
		write_svg(buf, dt_writer);
		//	viewBox 属性を書き込む.
		write_svg("viewBox=\"0 0 ", dt_writer);
		write_svg(m_page_panel.m_page_size.width, dt_writer);
		write_svg(m_page_panel.m_page_size.height, dt_writer);
		write_svg("\" ", dt_writer);
		//	背景色をスタイル属性として書き込む.
		write_svg("style=\"background-color:", dt_writer);
		write_svg(m_page_panel.m_page_color, dt_writer);
		//	svg タグの終了を書き込む.
		write_svg("\" >" SVG_NL, dt_writer);
		//	消去フラグのない図形をすべて SVG として書き込む.
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			s->write_svg(dt_writer);
		}
		//	svg の終了タグを書き込む.
		write_svg("</svg>" SVG_NL, dt_writer);
	}

	//	SVG としてストレージファイルに非同期に書き込む.
	//	file	書き込み先のファイル
	//	戻り値	書き込めた場合 S_OK
	IAsyncOperation<winrt::hresult> MainPage::file_write_svg_async(StorageFile const& s_file)
	{
		using winrt::Windows::Storage::CachedFileManager;
		using winrt::Windows::Storage::FileAccessMode;
		using winrt::Windows::Storage::Provider::FileUpdateStatus;
		using winrt::GraphPaper::implementation::write_svg;

		auto hr = E_FAIL;
		//	コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;
		//	スレッドをバックグラウンドに変える.
		co_await winrt::resume_background();
		try {
			//	ファイル更新の遅延を設定する.
			//	ストレージファイルを開いてランダムアクセスストリームを得る.
			//	ランダムアクセスストリームの先頭からデータライターを作成する.
			CachedFileManager::DeferUpdates(s_file);
			auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::ReadWrite) };
			auto dt_writer{ DataWriter(ra_stream.GetOutputStreamAt(0)) };
			//	SVG としてデータライターに書き込む.
			file_write_svg(dt_writer);
			//	ストリームの現在位置をストリームの大きさに格納する.
			//	バッファ内のデータをストリームに出力する.
			//	ストリームをフラッシュする.
			//	遅延させたファイル更新を完了する.
			ra_stream.Size(ra_stream.Position());
			co_await dt_writer.StoreAsync();
			co_await ra_stream.FlushAsync();
			auto fu_status{ co_await CachedFileManager::CompleteUpdatesAsync(s_file) };
			if (fu_status == FileUpdateStatus::Complete) {
				// 完了した場合, S_OK を結果に格納する.
				hr = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, エラーコードを結果に格納する.
			hr = e.code();
		}
		if (hr != S_OK) {
			// 結果が S_OK でない場合,
			// スレッドをメインページの UI スレッドに変える.
			// 書き込み失敗のメッセージダイアログを表示する.
			co_await winrt::resume_foreground(this->Dispatcher());
			cd_message_show(ERR_WRITE, s_file.Path());
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return hr;
	}

	// ファイルの読み込みが終了した.
	void MainPage::finish_file_read(void)
	{
		enable_undo_menu();
		enable_edit_menu();
		stroke_style_check_menu(m_page_panel.m_stroke_style);
		arrow_style_check_menu(m_page_panel.m_arrow_style);
		font_style_check_menu(m_page_panel.m_font_style);
		text_align_t_check_menu(m_page_panel.m_text_align_t);
		text_align_p_check_menu(m_page_panel.m_text_align_p);
		grid_show_check_menu(m_page_panel.m_grid_show);
		tmfi_grid_snap().IsChecked(m_page_panel.m_grid_snap);
		tmfi_grid_snap_2().IsChecked(m_page_panel.m_grid_snap);
		stat_check_menu(m_stat_bar);

		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			wchar_t* val;
			if (s->get_font_family(val)) {
				if (ShapeText::is_available_font(val) == false) {
					// 有効でない書体のエラーメッセージダイアログを表示する.
					cd_message_show(ERR_FONT, val);
					break;
				}
			}
		}
		if (m_summary_visible) {
			if (m_list_shapes.size() > 0) {
				summary_remake();
			}
			else {
				summary_close();
			}
		}
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		stat_set_curs();
		stat_set_grid();
		stat_set_page();
		stat_set_draw();
		stat_set_zoom();
		stat_set_unit();
		stat_visibility();
	}

	// ファイルメニューの「終了」が選択された
	IAsyncAction MainPage::mfi_exit_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		using winrt::Windows::UI::Xaml::Application;

		winrt::event_token save_token;
		winrt::event_token dont_token;
		winrt::event_token cancel_token;

		if (m_stack_push == false) {
			// 操作スタックの更新フラグがない場合,
			// 図形データは変更されていないので,
			// アプリケーションを終了する.
			release();
			Application::Current().Exit();
			co_return;
		}
		cd_load_conf_save();
		save_token = cd_conf_save().PrimaryButtonClick(
			[this](auto, auto)
			{
				// ファイルに非同期に保存して, アプリケーションを終了する.
				auto _{ file_save_and_exit_async() };
			}
		);
		dont_token = cd_conf_save().SecondaryButtonClick(
			[this](auto, auto)
			{
				// アプリケーションを終了する.
				release();
				Application::Current().Exit();
			}
		);
		cancel_token = cd_conf_save().CloseButtonClick(
		//closed_token = cd_conf_save().Closed(
			[this, &save_token, &dont_token, &cancel_token](auto, auto)
			{
				// イベントハンドラーをすべて解除し,
				// 保存確認ダイアログを破棄する.
				cd_conf_save().PrimaryButtonClick(save_token);
				cd_conf_save().SecondaryButtonClick(dont_token);
				cd_conf_save().CloseButtonClick(cancel_token);
				UnloadObject(cd_conf_save());
			}
		);
		// 保存確認ダイアログを表示する.
		//auto _{ cd_conf_save().ShowAsync() };
		co_await cd_conf_save().ShowAsync();
	}

	// ファイルメニューの「新規」が選択された
	IAsyncAction MainPage::mfi_new_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		winrt::event_token save_token;
		winrt::event_token dont_token;
		winrt::event_token closed_token;

		auto dialog = winrt::Windows::UI::Xaml::Controls::ContentDialog();
		if (m_stack_push == false) {
			// 操作スタックの更新フラグがない場合,
			// 図形データは変更されていないので,
			// 空白の文書にする.
			new_doc();
			co_return;
		}
		cd_load_conf_save();
		save_token = cd_conf_save().PrimaryButtonClick(
			[this](auto, auto)
			{
				//	ファイルに非同期に保存して, 空白の文書にする.
				auto _{ file_save_and_new_async() };
			}
		);
		dont_token = cd_conf_save().SecondaryButtonClick(
			[this](auto, auto)
			{
				// 空白の文書にする.
				new_doc();
			}
		);
		closed_token = cd_conf_save().Closed(
			[this, &save_token, &dont_token, &closed_token](auto, auto)
			{
				// イベントハンドラーをすべて解除し,
				// 保存確認ダイアログを破棄する.
				cd_conf_save().PrimaryButtonClick(save_token);
				cd_conf_save().SecondaryButtonClick(dont_token);
				cd_conf_save().Closed(closed_token);
				UnloadObject(cd_conf_save());
			}
		);
		// 保存確認ダイアログを表示する.
		co_await cd_conf_save().ShowAsync();
	}

	// ファイルメニューの「開く」が選択された
	IAsyncAction MainPage::mfi_open_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		winrt::event_token save_token;
		winrt::event_token dont_token;
		winrt::event_token closed_token;

		if (m_stack_push == false) {
			//	操作スタックの更新フラグがない場合,
			//	図形データは変更されていないので,
			//	ファイルを非同期に開く.
			auto _{ file_open_async() };
			//return;
			//co_await file_open_async();
			co_return;
		}
		cd_load_conf_save();
		save_token = cd_conf_save().PrimaryButtonClick(
			[this](auto, auto)
			{
				//	ファイルに非同期に保存して, ファイルを非同期に開く.
				auto _{ file_save_and_open_async() };
			}
		);
		dont_token = cd_conf_save().SecondaryButtonClick(
			[this](auto, auto)
			{
				//	ファイルを非同期に開く.
				auto _{ file_open_async() };
			}
		);
		closed_token = cd_conf_save().Closed(
			[this, &save_token, &dont_token, &closed_token](auto, auto)
			{
				//	イベントハンドラーをすべて解除し,
				//	保存確認ダイアログを破棄する.
				cd_conf_save().PrimaryButtonClick(save_token);
				cd_conf_save().SecondaryButtonClick(dont_token);
				cd_conf_save().Closed(closed_token);
				UnloadObject(cd_conf_save());
			}
		);
		//	保存確認ダイアログを表示する.
		co_await cd_conf_save().ShowAsync();
		//{ auto _{ cd_conf_save().ShowAsync() }; }
	}

	//	ファイルメニューの「名前を付けて保存」が選択された
	void MainPage::mfi_save_as_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	名前を付けてファイルに非同期に保存する.
		auto _{ file_save_as_async(true) };
	}

	//	ファイルメニューの「保存」が選択された
	void MainPage::mfi_save_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	ファイルに非同期に保存する
		auto _{ file_save_async() };
	}

	//	空白の文書にする.
	void MainPage::new_doc(void)
	{
		if (m_summary_visible) {
			//	図形一覧パネルの表示フラグが立っている場合,
			//	一覧を閉じる.
			summary_close();
		}
		//	操作スタックを消去する.
		undo_clear();
		//	図形リストを消去する.
		s_list_clear(m_list_shapes);
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			cd_message_show(L"Memory leak occurs", {});
		}
#endif
		m_page_min.x = 0.0;
		m_page_min.y = 0.0;
		m_page_max.x = m_page_panel.m_page_size.width;
		m_page_max.y = m_page_panel.m_page_size.height;
		m_mru_token.clear();
		mru_add_file(nullptr);
		finish_file_read();
	}

}