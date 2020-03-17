//-------------------------------
// MainPage_file.cpp
// ファイルの読み書き
//-------------------------------
#include "pch.h"
#include "MainPage.h"

// file_read_recent_async
// 	file_wait_cursor
// 	file_recent_get_async
// 		GetFileAsync
// 		winrt::resume_foreground
// 	file_read_async
// 		winrt::resume_background
// 		OpenAsync
// 		winrt::resume_foreground
// 		file_recent_add
// 			file_recent_update_menu
// 	finish_file_read
// 	winrt::resume_foreground
// file_save_as_async
// 	file_wait_cursor
// 	file_recent_get_async
// 		GetFileAsync
// 		winrt::resume_foreground
// 	PickSaveFileAsync
// 	file_write_svg_async
// 		winrt::resume_background
// 		OpenAsync
// 		winrt::resume_foreground
// 	file_write_gpf_async
// 		winrt::resume_background
// 		OpenAsync
// 		winrt::resume_foreground
// 		file_recent_add
// 			file_recent_update_menu
// file_save_async
// 	file_recent_get_async
// 		GetFileAsync
// 		winrt::resume_foreground
// 	file_save_as_async
// 	file_wait_cursor
// 	file_write_gpf_async
// 		winrt::resume_background
// 		winrt::resume_foreground
// 		file_recent_add
// 			file_recent_update_menu
// mfi_new_click
// 	cd_conf_save().ShowAsync
// 	file_save_async
// 	file_recent_add
// 		file_recent_update_menu
// mfi_open_click_async
// 	cd_conf_save().ShowAsync
// 	file_save_async
// 	file_wait_cursor
// 	PickSingleFileAsync
// 	file_read_async
// 		winrt::resume_background
// 		OpenAsync
// 		winrt::resume_foreground
// 		file_recent_add
// 			file_recent_update_menu
// 		finish_file_read
// mfi_file_recent_N_click
// 	file_read_recent_async
// mfi_save_as_click
// 	file_save_as_async
// mfi_save_click
// 	file_save_async
/*
ただし、4つのWindowsランタイム非同期操作タイプ（IAsyncXxx）のいずれかを
co_awaitした場合、C ++ / WinRTはco_awaitの時点で呼び出しコンテキストをキャプチャします。
また、継続が再開されたときに、そのコンテキストにいることが保証されます。
C ++ / WinRTは、呼び出し側のコンテキストに既にいるかどうかを確認し、
そうでない場合は切り替えます。
*/

// GetFileFromPathAsync を E_ACCESSDENIED なしに使うには以下が必要になる.
// 1. XAMLテキストエディタを使って, Pakage.appxmanifest に broadFileSystemAccess を追加する.
// Pakage.appxmanifest の <Package> のプロパティーに
// xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities" を追加する
// IgnorableNamespaces="uap mp" を IgnorableNamespaces="uap mp rescap" に変更する
// 2. もしなければ <Capabilities> タグを <Package> の子要素として追加する.
// <Capabilities> の子要素として <rescap:Capability Name="broadFileSystemAccess" /> を加える
// 3. コンパイルするたびに, 設定を使ってアプリにファイルアクセスを許可する.
// スタートメニュー > 設定 > プライバー > ファイル システム > ファイルシステムにアクセスできるアプリを選ぶ
// 表示されているアプリをオンにする.
// または,
// スタートメニュー > 設定 > アプリ > アプリと機能
// 表示されているアプリをクリックする.
// 詳細オプション > アプリのアクセス許可
// ファイル システムをオンにする.

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Core::CoreCursorType;

	static auto const& CUR_WAIT = CoreCursor(CoreCursorType::Wait, 0);	// 待機カーソル.
	constexpr wchar_t DESC_GPF[] = L"str_desc_gpf";	// 拡張子 gpf の説明
	constexpr wchar_t DESC_SVG[] = L"str_desc_svg";	// 拡張子 svg の説明
	constexpr wchar_t ERR_FONT[] = L"str_err_font";	// 有効でない書体のエラーメッセージのリソース名
	constexpr wchar_t ERR_READ[] = L"str_err_read";	// 読み込みエラーメッセージのリソース名
	constexpr wchar_t ERR_WRITE[] = L"str_err_write";	// 書き込みエラーメッセージのリソース名
	constexpr wchar_t ERR_RECENT[] = L"str_err_recent";	// 最近使ったファイルのエラーメッセージのリソース名
	constexpr wchar_t FT_GPF[] = L".gpf";	// 図形データファイルの拡張子
	constexpr wchar_t FT_SVG[] = L".svg";	// SVG ファイルの拡張子
	constexpr uint32_t MRU_MAX = 25;	// 最近使ったリストの最大数.
	constexpr wchar_t UNTITLED[] = L"str_untitled";	// 無題のリソース名

	// SVG 開始タグをデータライターに書き込む.
	static void file_write_svg_tag(D2D1_SIZE_F const& size, D2D1_COLOR_F const& color, const double dpi, const LEN_UNIT unit, DataWriter const& dt_writer);

	// 待機カーソルを表示, 表示する前のカーソルを得る.
	CoreCursor MainPage::file_wait_cursor(void) const
	{
		auto const& c_win = Window::Current().CoreWindow();
		auto p_cur = c_win.PointerCursor();
		c_win.PointerCursor(CUR_WAIT);
		return p_cur;
	}

	// ストレージファイルを非同期に読む.
	// s_file	読み込むストレージファイル
	// suspend	中断フラグ
	// 戻り値	読み込めたら S_OK.
	// 中断フラグが立っている場合, 操作スタックも保存する.
	IAsyncOperation<winrt::hresult> MainPage::file_read_async(StorageFile const& s_file, const bool suspend, const bool layout) noexcept
	{
		using winrt::Windows::Storage::FileAccessMode;

		auto hr = E_FAIL;
		// コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// スレッドをバックグラウンドに変更する.
		co_await winrt::resume_background();
		try {
			// ファイルを開いてランダムアクセスストリームを得る.
			auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::Read) };
			// ストリームから読み込むためのデータリーダーを作成する.
			auto dt_reader{ DataReader(ra_stream.GetInputStreamAt(0)) };
			// データリーダーにファイルを読み込む.
			co_await dt_reader.LoadAsync(static_cast<uint32_t>(ra_stream.Size()));

			text_find_read(dt_reader);
			status_bar_read(dt_reader);
			m_page_unit = static_cast<LEN_UNIT>(dt_reader.ReadUInt32());
			m_col_style = static_cast<COL_STYLE>(dt_reader.ReadUInt32());
			m_page_layout.read(dt_reader);
			m_page_layout.m_page_scale = min(max(m_page_layout.m_page_scale, SCALE_MIN), SCALE_MAX);
			undo_clear();
			s_list_clear(m_list_shapes);
#if defined(_DEBUG)
			if (debug_leak_cnt != 0) {
				co_await winrt::resume_foreground(this->Dispatcher());
				cd_message_show(ICON_ALERT, L"Memory leak occurs", {});
				co_await context;
				co_return hr;
			}
#endif
			if (layout || s_list_read(m_list_shapes, dt_reader)) {
				if (suspend) {
					// 中断フラグが立っている場合,
					// データリーダーから操作スタックを読み込む.
					undo_read(dt_reader);
				}
				hr = S_OK;
			}
			// データリーダーを閉じる.
			dt_reader.Close();
			// ストリームを閉じる.
			ra_stream.Close();
		}
		catch (winrt::hresult_error const& e) {
			hr = e.code();
		}
		if (hr != S_OK) {
			co_await winrt::resume_foreground(this->Dispatcher());
			cd_message_show(ICON_ALERT, ERR_READ, s_file.Path());
		}
		else if (suspend == false && layout == false) {
			co_await winrt::resume_foreground(this->Dispatcher());
			file_recent_add(s_file);
			finish_file_read();
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return hr;
	}

	// 名前を付けてファイルに非同期に保存する.
	// svg_allowed	SVG 許容フラグ.
	IAsyncOperation<winrt::hresult> MainPage::file_save_as_async(const bool svg_allowed) noexcept
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::Storage::Pickers::FileSavePicker;

		auto hr = E_FAIL;
		// 待機カーソルを表示, 表示する前のカーソルを得る.
		auto const& p_cur = file_wait_cursor();
		// コルーチンの開始時のスレッドコンテキストを保存する.
		//winrt::apartment_context context;
		//co_await resume_background(this->Dispatcher());
		try {
			// ファイル保存ピッカーを得る.
			// ファイルタイプに拡張子 GPF とその説明を追加する.
			auto s_picker{ FileSavePicker() };
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			auto desc_gpf = r_loader.GetString(DESC_GPF);
			auto type_gpf = winrt::single_threaded_vector<winrt::hstring>({ FT_GPF });
			s_picker.FileTypeChoices().Insert(desc_gpf, type_gpf);
			// SVG 許容フラグが立っているか判定する.
			if (svg_allowed) {
				// フラグが立っている場合, 
				// ファイルタイプに拡張子 SVG とその説明を追加する.
				auto desc_svg = r_loader.GetString(DESC_SVG);
				auto type_svg = winrt::single_threaded_vector<winrt::hstring>({ FT_SVG });
				s_picker.FileTypeChoices().Insert(desc_svg, type_svg);
			}
			// 最近使ったファイルのトークンが空か判定する.
			if (m_token_mru.empty()) {
				// トークンが空の場合, 
				// 提案されたファイル名に拡張子を格納する.
				s_picker.SuggestedFileName(FT_GPF);
			}
			else {
				// トークンが空でない場合,
				// ストレージファイルを最近使ったファイルのトークンから得る.
				auto s_file{ co_await file_recent_get_async(m_token_mru) };
				if (s_file != nullptr) {
					// 取得した場合,
					if (s_file.FileType() == FT_GPF) {
						// かつファイルタイプが GPF の場合,
						// ファイル名を, 提案するファイル名に格納する.
						s_picker.SuggestedFileName(s_file.Name());
					}
					s_file = nullptr;
				}
			}
			// ファイル保存ピッカーを表示し, ストレージファイルを得る.
			auto s_file{ co_await s_picker.PickSaveFileAsync() };
			// ストレージファイルを取得したか判定する.
			if (s_file != nullptr) {
				// 取得した場合,
				if (s_file.FileType() == FT_SVG) {
					// ファイルタイプが SVG の場合,
					// 図形データを SVG としてストレージファイルに非同期に書き込み, 結果を得る.
					hr = co_await file_write_svg_async(s_file);
				}
				else {
					// ファイルタイプが SVG 以外の場合,
				// 図形データをストレージファイルに非同期に書き込み, 結果を得る.
					hr = co_await file_write_gpf_async(s_file);
				}
				// ストレージファイルを破棄する.
				s_file = nullptr;
			}
			// ピッカーを破棄する.
			s_picker = nullptr;
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, エラーコードを結果に格納する.
			hr = e.code();
		}
		// スレッドをメインページの UI スレッドに変える.
		//co_await winrt::resume_foreground(this->Dispatcher());
		// ウィンドウのカーソルを復元する.
		Window::Current().CoreWindow().PointerCursor(p_cur);
		// スレッドコンテキストを復元する.
		//co_await context;
		// 結果を返し終了する.
		co_return hr;
	}

	// ファイルに非同期に保存する
	IAsyncOperation<winrt::hresult> MainPage::file_save_async(void) noexcept
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		// 最近使ったファイルのトークンからストレージファイルを得る.
		auto s_file{ co_await file_recent_get_async(m_token_mru) };
		if (s_file == nullptr) {
			// ストレージファイルが空の場合,
			constexpr bool SVG_ALLOWED = true;
			// 名前を付けてファイルに非同期に保存する
			co_return co_await file_save_as_async(!SVG_ALLOWED);
		}

		auto hr = E_FAIL;
		// 待機カーソルを表示, 表示する前のカーソルを得る.
		auto const& p_cur = file_wait_cursor();
		// 図形データをストレージファイルに非同期に書き込み, 結果を得る.
		hr = co_await file_write_gpf_async(s_file);
		// 結果を判定する.
		if (hr != S_OK) {
			// 結果が S_OK でない場合,
			// スレッドをメインページの UI スレッドに変える.
			// 最近使ったファイルのエラーメッセージダイアログを表示する.
			co_await winrt::resume_foreground(this->Dispatcher());
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			cd_message_show(ICON_ALERT, ERR_WRITE, m_token_mru);
		}
		// スレッドコンテキストを復元する.
		//co_await context;
		// ウィンドウのカーソルを復元する.
		Window::Current().CoreWindow().PointerCursor(p_cur);
		// 結果を返し終了する.
		co_return hr;
	}

	// 図形データをストレージファイルに非同期に書き込む.
	// s_file	ストレージファイル
	// suspend	中断フラグ
	// 戻り値	書き込みに成功したら true
	IAsyncOperation<winrt::hresult> MainPage::file_write_gpf_async(StorageFile const& s_file, const bool suspend, const bool layout)
	{
		using winrt::Windows::Storage::CachedFileManager;
		using winrt::Windows::Storage::FileAccessMode;
		using winrt::Windows::Storage::Provider::FileUpdateStatus;
		constexpr auto REDUCE = true;

		// E_FAIL を結果に格納する.
		auto hr = E_FAIL;
		// コルーチンの開始時のスレッドコンテキストを保存する.
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
			
			text_find_write(dt_writer);
			status_write(dt_writer);
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_page_unit));
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_col_style));
			m_page_layout.write(dt_writer);
			if (suspend) {
				s_list_write<!REDUCE>(m_list_shapes, dt_writer);
				undo_write(dt_writer);
			}
			else if (layout) {
			}
			else {
				s_list_write<REDUCE>(m_list_shapes, dt_writer);
			}
			ra_stream.Size(ra_stream.Position());
			co_await dt_writer.StoreAsync();
			co_await ra_stream.FlushAsync();
			// データライターを閉じる.
			dt_writer.Close();
			// ストリームを閉じる.
			ra_stream.Close();
			// 遅延させたファイル更新を完了する.
			auto fu_status{ co_await CachedFileManager::CompleteUpdatesAsync(s_file) };
			if (fu_status == FileUpdateStatus::Complete) {
				// 完了した場合, 
				// S_OK を結果に格納する.
				hr = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, 
			// エラーコードを結果に格納する.
			hr = e.code();
		}
		if (hr != S_OK) {
			// 結果が S_OK でない場合,
			// スレッドをメインページの UI スレッドに変える.
			co_await winrt::resume_foreground(this->Dispatcher());
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			cd_message_show(ICON_ALERT, ERR_WRITE, s_file.Path());
		}
		else if (suspend == false && layout == false) {
			// 中断フラグがない場合,
			// スレッドをメインページの UI スレッドに変える.
			co_await winrt::resume_foreground(this->Dispatcher());
			// ストレージファイルを最近使ったファイルに登録する.
			// ここでエラーが出る.
			file_recent_add(s_file);
			// false を操作スタックの更新フラグに格納する.
			m_stack_push = false;
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return hr;
	}

	// SVG としてデータライターに書き込む.
	// dt_writer	データライター
	/*
	void MainPage::file_write_svg(DataWriter const& dt_writer)
	{
		constexpr char XML_DEC[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" SVG_NL;
		constexpr char DOCTYPE[] = "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" SVG_NL;
		constexpr char SVG_TAG[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" ";

		// XML 宣言を書き込む.
		write_svg(XML_DEC, dt_writer);
		// DOCTYPE を書き込む.
		write_svg(DOCTYPE, dt_writer);
		// svg タグの開始を書き込む.
		write_svg(SVG_TAG, dt_writer);
		// 幅と高さの属性を書き込む.
		char buf[256];
		constexpr char* SVG_UNIT_PX = "px";
		constexpr char* SVG_UNIT_IN = "in";
		constexpr char* SVG_UNIT_MM = "mm";
		constexpr char* SVG_UNIT_PT = "pt";
		const auto dpi = m_page_dx.m_logical_dpi;
		double w, h;
		char* u;
		if (m_page_unit == LEN_UNIT::INCH) {
			w = m_page_layout.m_page_size.width / dpi;
			h = m_page_layout.m_page_size.height / dpi;
			u = SVG_UNIT_IN;
		}
		else if (m_page_unit == LEN_UNIT::MILLI) {
			w = m_page_layout.m_page_size.width * MM_PER_INCH / dpi;
			h = m_page_layout.m_page_size.height * MM_PER_INCH / dpi;
			u = SVG_UNIT_MM;
		}
		else if (m_page_unit == LEN_UNIT::POINT) {
			w = m_page_layout.m_page_size.width * PT_PER_INCH / dpi;
			h = m_page_layout.m_page_size.height * PT_PER_INCH / dpi;
			u = SVG_UNIT_PT;
		}
		else if (m_page_unit == LEN_UNIT::PIXEL) {
			w = m_page_layout.m_page_size.width;
			h = m_page_layout.m_page_size.height;
			u = SVG_UNIT_PX;
		}
		else {
			w = m_page_layout.m_page_size.width;
			h = m_page_layout.m_page_size.height;
			u = SVG_UNIT_PX;
		}
		std::snprintf(buf, sizeof(buf) - 1, "width=\"%lf%s\" height=\"%lf%s\" ", w, u, h, u);
		write_svg(buf, dt_writer);
		// viewBox 属性を書き込む.
		write_svg("viewBox=\"0 0 ", dt_writer);
		write_svg(m_page_layout.m_page_size.width, dt_writer);
		write_svg(m_page_layout.m_page_size.height, dt_writer);
		write_svg("\" ", dt_writer);
		// 背景色をスタイル属性として書き込む.
		write_svg("style=\"background-color:", dt_writer);
		write_svg(m_page_layout.m_page_color, dt_writer);
		// svg タグの終了を書き込む.
		write_svg("\" >" SVG_NL, dt_writer);
		// 消去フラグのない図形をすべて SVG として書き込む.
		// 図形リストの各図形について以下を繰り返す.
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				// 消去フラグが立っている場合,
				// 以下を無視する.
				continue;
			}
			s->write_svg(dt_writer);
		}
		// svg の終了タグを書き込む.
		write_svg("</svg>" SVG_NL, dt_writer);
	}
	*/

	// SVG 開始タグをデータライターに書き込む.
	static void file_write_svg_tag(D2D1_SIZE_F const& size, D2D1_COLOR_F const& color, const double dpi, const LEN_UNIT unit, DataWriter const& dt_writer)
	{
		constexpr char SVG_TAG[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" ";
		constexpr char* SVG_UNIT_PX = "px";
		constexpr char* SVG_UNIT_IN = "in";
		constexpr char* SVG_UNIT_MM = "mm";
		constexpr char* SVG_UNIT_PT = "pt";

		// SVG タグの開始を書き込む.
		write_svg(SVG_TAG, dt_writer);
		// 単位付きで幅と高さの属性を書き込む.
		char buf[256];
		double w, h;
		char* u;
		if (unit == LEN_UNIT::INCH) {
			w = size.width / dpi;
			h = size.height / dpi;
			u = SVG_UNIT_IN;
		}
		else if (unit == LEN_UNIT::MILLI) {
			w = size.width * MM_PER_INCH / dpi;
			h = size.height * MM_PER_INCH / dpi;
			u = SVG_UNIT_MM;
		}
		else if (unit == LEN_UNIT::POINT) {
			w = size.width * PT_PER_INCH / dpi;
			h = size.height * PT_PER_INCH / dpi;
			u = SVG_UNIT_PT;
		}
		else {
			w = size.width;
			h = size.height;
			u = SVG_UNIT_PX;
		}
		sprintf_s(buf, "width=\"%lf%s\" height=\"%lf%s\" ", w, u, h, u);
		write_svg(buf, dt_writer);
		// ピクセル単位の幅と高さを viewBox 属性として書き込む.
		write_svg("viewBox=\"0 0 ", dt_writer);
		write_svg(size.width, dt_writer);
		write_svg(size.height, dt_writer);
		write_svg("\" ", dt_writer);
		// 背景色をスタイル属性として書き込む.
		write_svg("style=\"background-color:", dt_writer);
		write_svg(color, dt_writer);
		// svg 開始タグの終了を書き込む.
		write_svg("\" >" SVG_NL, dt_writer);
	}

	// 図形データを SVG としてストレージファイルに非同期に書き込む.
	// file	書き込み先のファイル
	// 戻り値	書き込めた場合 S_OK
	IAsyncOperation<winrt::hresult> MainPage::file_write_svg_async(StorageFile const& s_file)
	{
		using winrt::Windows::Storage::CachedFileManager;
		using winrt::Windows::Storage::FileAccessMode;
		using winrt::Windows::Storage::Provider::FileUpdateStatus;
		using winrt::GraphPaper::implementation::write_svg;
		constexpr char XML_DEC[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" SVG_NL;
		constexpr char DOCTYPE[] = "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" SVG_NL;

		auto hr = E_FAIL;
		// コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;
		// スレッドをバックグラウンドに変える.
		co_await winrt::resume_background();
		try {
			// ファイル更新の遅延を設定する.
			CachedFileManager::DeferUpdates(s_file);
			// ストレージファイルを開いてランダムアクセスストリームを得る.
			auto ra_stream{ co_await s_file.OpenAsync(FileAccessMode::ReadWrite) };
			// ランダムアクセスストリームの先頭からデータライターを作成する.
			auto dt_writer{ DataWriter(ra_stream.GetOutputStreamAt(0)) };
			// XML 宣言を書き込む.
			write_svg(XML_DEC, dt_writer);
			// DOCTYPE を書き込む.
			write_svg(DOCTYPE, dt_writer);
			// SVG 開始タグをデータライターに書き込む.
			file_write_svg_tag(m_page_layout.m_page_size, m_page_layout.m_page_color, m_page_dx.m_logical_dpi, m_page_unit, dt_writer);
			// 図形リストの各図形について以下を繰り返す.
			for (auto s : m_list_shapes) {
				if (s->is_deleted()) {
					// 消去フラグが立っている場合,
					// 以下を無視する.
					continue;
				}
				s->write_svg(dt_writer);
			}
			// SVG 終了タグを書き込む.
			write_svg("</svg>" SVG_NL, dt_writer);
			// ストリームの現在位置をストリームの大きさに格納する.
			ra_stream.Size(ra_stream.Position());
			// バッファ内のデータをストリームに出力する.
			co_await dt_writer.StoreAsync();
			// ストリームをフラッシュする.
			co_await ra_stream.FlushAsync();
			// 遅延させたファイル更新を完了する.
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
			co_await winrt::resume_foreground(this->Dispatcher());
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			cd_message_show(ICON_ALERT, ERR_WRITE, s_file.Path());
		}
		// スレッドコンテキストを復元する.
		co_await context;
		// 結果を返し終了する.
		co_return hr;
	}

	// ファイルの読み込みが終了した.
	void MainPage::finish_file_read(void)
	{
		enable_edit_menu();
		stroke_style_check_menu(m_page_layout.m_stroke_style);
		arrow_style_check_menu(m_page_layout.m_arrow_style);
		font_style_check_menu(m_page_layout.m_font_style);
		grid_show_check_menu(m_page_layout.m_grid_show);
		status_check_menu(m_status_bar);
		text_align_t_check_menu(m_page_layout.m_text_align_t);
		text_align_p_check_menu(m_page_layout.m_text_align_p);
		tmfi_grid_snap().IsChecked(m_page_layout.m_grid_snap);
		tmfi_grid_snap_2().IsChecked(m_page_layout.m_grid_snap);

		// 図形リストの各図形について以下を繰り返す.
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			wchar_t* value;	// 書体名
			if (s->get_font_family(value)) {
				if (ShapeText::is_available_font(value) == false) {
					// 「無効な書体が使用されています」メッセージダイアログを表示する.
					cd_message_show(ICON_ALERT, ERR_FONT, value);
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
		s_list_bound(m_list_shapes, m_page_layout.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		status_set_curs();
		status_set_draw();
		status_set_grid();
		status_set_page();
		status_set_zoom();
		status_set_unit();
		status_visibility();
	}

	// ファイルメニューの「開く」が選択された
	IAsyncAction MainPage::mfi_open_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::Storage::Pickers::FileOpenPicker;

		// SHCore.dll スレッド
		if (m_stack_push) {
			// 操作スタックの更新フラグが立っている場合,
			// 保存確認ダイアログを表示する.
			const auto d_result = co_await cd_conf_save().ShowAsync();
			if (d_result == ContentDialogResult::None) {
				// 「キャンセル」が押された場合,
					// 中断する.
				co_return;
			}
			else if (d_result == ContentDialogResult::Primary) {
				// 「保存する」が押された場合,
				if (co_await file_save_async() != S_OK) {
					// 保存できなかった場合,
					// 中断する.
					co_return;
				}
			}
		}
		// 待機カーソルを表示, 表示する前のカーソルを得る.
		auto const& p_cur = file_wait_cursor();
		// ファイル「オープン」ピッカーを取得して開く.
		auto o_picker{ FileOpenPicker() };
		o_picker.FileTypeFilter().Append(FT_GPF);
		// ピッカーを非同期で表示してストレージファイルを取得する.
		// （「閉じる」ボタンが押された場合ストレージファイルとして nullptr が返る.）
		auto s_file{ co_await o_picker.PickSingleFileAsync() };
		if (s_file != nullptr) {
			// ストレージファイルがヌルでない場合、
			// ストレージファイルを非同期に読む.
			co_await file_read_async(s_file);
			// ストレージファイルを解放する.
			s_file = nullptr;
		}
		o_picker = nullptr;
		Window::Current().CoreWindow().PointerCursor(p_cur);
	}

	// ファイルメニューの「名前を付けて保存」が選択された
	void MainPage::mfi_save_as_click(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr bool SVG_ALLOWED = true;
		auto _{ file_save_as_async(SVG_ALLOWED) };
	}

	// ファイルメニューの「上書き保存」が選択された
	void MainPage::mfi_save_click(IInspectable const&, RoutedEventArgs const&)
	{
		auto _{ file_save_async() };
	}

	// 最近使ったファイルを読み込む.
	// i	最近使ったファイルの番号 (最も直近が 0).
	IAsyncAction MainPage::file_read_recent_async(const uint32_t i)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		// SHCore.dll スレッド
		auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		auto const& mru_entries = mru_list.Entries();
		if (i >= mru_entries.Size()) {
			// ファイルの番号が最近使ったファイルの数以上の場合,
			cd_message_show(ICON_ALERT, ERR_RECENT, to_hstring(i + 1));
			co_return;
		}
		// 最近使ったリストから要素を得る.
		AccessListEntry item[1];
		mru_entries.GetMany(i, item);

		if (m_stack_push) {
			// 操作スタックの更新フラグが立っている場合,
			const auto d_result = co_await cd_conf_save().ShowAsync();
			if (d_result == ContentDialogResult::None) {
				// 「キャンセル」が押された場合,
				co_return;
			}
			else if (d_result == ContentDialogResult::Primary) {
				// 「保存する」が押された場合,
				if (co_await file_save_async() != S_OK) {
					co_return;
				}
			}
		}
		auto hr = E_FAIL;
		auto const p_cur = file_wait_cursor();
		auto s_file{ co_await file_recent_get_async(item[0].Token) };
		if (s_file != nullptr) {
			// 取得できた場合,
			hr = co_await file_read_async(s_file);
			s_file = nullptr;
			finish_file_read();
		}
		else {
			// 取得できない場合,
			cd_message_show(ICON_ALERT, ERR_RECENT, item[0].Metadata);
		}

		// ウィンドウのカーソルを復元する.
		Window::Current().CoreWindow().PointerCursor(p_cur);
	}

	// ファイルメニューの「最近使ったファイル 1」が選択された
	void MainPage::mfi_file_recent_1_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 最近使ったファイル (0) を読み込む.
		auto _{ file_read_recent_async(0) };
	}

	// ファイルメニューの「最近使ったファイル 2」が選択された
	void MainPage::mfi_file_recent_2_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 最近使ったファイル (1) を読み込む.
		auto _{ file_read_recent_async(1) };
	}

	// ファイルメニューの「最近使ったファイル 3」が選択された
	void MainPage::mfi_file_recent_3_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 最近使ったファイル (2) を読み込む.
		auto _{ file_read_recent_async(2) };
	}

	// ファイルメニューの「最近使ったファイル 4」が選択された
	void MainPage::mfi_file_recent_4_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 最近使ったファイル (3) を読み込む.
		auto _{ file_read_recent_async(3) };
	}

	// ファイルメニューの「最近使ったファイル 5」が選択された
	void MainPage::mfi_file_recent_5_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 最近使ったファイル (4) を読み込む.
		auto _{ file_read_recent_async(4) };
	}

	// 最近使ったファイルのトークンからストレージファイルを得る.
	// token	最近使ったファイルのトークン
	// 戻り値	ストレージファイル
	IAsyncOperation<StorageFile> MainPage::file_recent_get_async(const winrt::hstring token)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;

		// コルーチンの開始時のスレッドコンテキストを保存する.
		winrt::apartment_context context;

		// スレッドをメインページの UI スレッドに変える.
		//co_await winrt::resume_foreground(this->Dispatcher());
		// ストレージファイルにヌルを格納する.
		StorageFile s_file = nullptr;
		try {
			if (token.empty() == false) {
				// トークンが空でない場合,
				// トークンからストレージファイルを得る.
				auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
				if (mru_list.ContainsItem(token)) {
					s_file = co_await mru_list.GetFileAsync(token);
				}
			}
		}
		catch (winrt::hresult_error) {
		}
		// 取得できても出来なくても最近使ったリストの順番は入れ替わるので,
		// 最近使ったファイルメニュを更新する.
		file_recent_update_menu();
		// スレッドコンテキストを復元する.
		co_await context;
		// ストレージファイルを返す.
		co_return s_file;
	}

	// ストレージファイルを最近使ったファイルに登録する.
	// s_file	ストレージファイル
	// 戻り値	なし
	// 最近使ったファイルメニューとウィンドウタイトルも更新される.
	// ストレージファイルがヌルの場合, 最近使ったファイルはそのままで, 
	// ウィンドウタイトルに無題が格納される.
	void MainPage::file_recent_add(StorageFile const& s_file)
	{
		using winrt::Windows::UI::ViewManagement::ApplicationView;

		if (s_file != nullptr) {
			using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
			auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
			//if (mru_list.ContainsItem(m_token_mru)) {
				//mru_list.Remove(m_token_mru);
				//mru_list.AddOrReplace(m_token_mru, s_file, s_file.Path());
			//}
			//else {
				m_token_mru = mru_list.Add(s_file, s_file.Path());
			//}
			ApplicationView::GetForCurrentView().Title(s_file.Name());
		}
		else {
			using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
			auto const& r_loader = ResourceLoader::GetForCurrentView();
			ApplicationView::GetForCurrentView().Title(r_loader.GetString(UNTITLED));
		}
		// 最近使ったファイルメニュを更新する.
		file_recent_update_menu();
	}

	// 最近使ったファイルメニュを更新する.
	void MainPage::file_recent_update_menu(void)
	{
		using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		using winrt::Windows::Storage::AccessCache::AccessListEntry;

		auto const& mru_entries = StorageApplicationPermissions::MostRecentlyUsedList().Entries();
		const auto ent_size = mru_entries.Size();
		AccessListEntry items[MRU_MAX];
		winrt::hstring data[MRU_MAX];
		mru_entries.GetMany(0, items);
		for (uint32_t i = MRU_MAX; i > 0; i--) {
			data[i - 1] = winrt::to_hstring(i) + L":";
			if (ent_size >= i) {
				data[i - 1] = data[i - 1] + L" " + items[i - 1].Metadata;
			}
		}
		mfi_file_recent_1().Text(data[0]);
		mfi_file_recent_2().Text(data[1]);
		mfi_file_recent_3().Text(data[2]);
		mfi_file_recent_4().Text(data[3]);
		mfi_file_recent_5().Text(data[4]);

	}

}