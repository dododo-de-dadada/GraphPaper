#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::CachedFileManager;
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::Storage::Provider::FileUpdateStatus;

	//-------------------------------
	// 図形データを SVG としてストレージファイルに非同期に書き込む.
	// svg_file	書き込み先のファイル
	// 戻り値	書き込めた場合 S_OK
	//-------------------------------
	IAsyncOperation<winrt::hresult> MainPage::file_svg_write_async(StorageFile svg_file)
	{
		constexpr char XML_DEC[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" SVG_NEW_LINE;
		constexpr char DOCTYPE[] = "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" SVG_NEW_LINE;

		m_mutex_exit.lock();
		// コルーチンの開始時のスレッドコンテキストを保存する.
//		winrt::apartment_context context;
		// スレッドをバックグラウンドに変える.
//		co_await winrt::resume_background();
		HRESULT hr = E_FAIL;
		try {
			// ファイル更新の遅延を設定する.
			CachedFileManager::DeferUpdates(svg_file);
			// ストレージファイルを開いてランダムアクセスストリームを得る.
			const IRandomAccessStream& svg_stream{
				co_await svg_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			// ランダムアクセスストリームの先頭からデータライターを作成する.
			DataWriter dt_writer{
				DataWriter(svg_stream.GetOutputStreamAt(0))
			};
			// XML 宣言を書き込む.
			svg_dt_write(XML_DEC, dt_writer);
			// DOCTYPE を書き込む.
			svg_dt_write(DOCTYPE, dt_writer);
			// SVG 開始タグを書き込む.
			{
				const auto size = m_main_sheet.m_sheet_size;	// 用紙の大きさ
				const auto unit = m_len_unit;	// 長さの単位
				const auto dpi = m_main_sheet.m_d2d.m_logical_dpi;	// 論理 DPI
				const auto color = m_main_sheet.m_sheet_color;	// 背景色
				constexpr char SVG_TAG[] =
					"<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" ";
				constexpr char* SVG_UNIT_PX = "px";
				constexpr char* SVG_UNIT_IN = "in";
				constexpr char* SVG_UNIT_MM = "mm";
				constexpr char* SVG_UNIT_PT = "pt";

				// SVG タグの開始を書き込む.
				svg_dt_write(SVG_TAG, dt_writer);

				// 単位付きで幅と高さの属性を書き込む.
				char buf[256];
				double w;	// 単位変換後の幅
				double h;	// 単位変換後の高さ
				char* u;	// 単位
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
				// SVG で使用できる上記の単位以外はすべてピクセル.
				else {
					w = size.width;
					h = size.height;
					u = SVG_UNIT_PX;
				}
				sprintf_s(buf, "width=\"%lf%s\" height=\"%lf%s\" ", w, u, h, u);
				svg_dt_write(buf, dt_writer);

				// ピクセル単位の幅と高さを viewBox 属性として書き込む.
				svg_dt_write("viewBox=\"0 0 ", dt_writer);
				svg_dt_write(size.width, dt_writer);
				svg_dt_write(size.height, dt_writer);
				svg_dt_write("\" ", dt_writer);

				// 背景色をスタイル属性として書き込む.
				svg_dt_write("style=\"background-color:", dt_writer);
				svg_dt_write(color, dt_writer);

				// svg 開始タグの終了を書き込む.
				svg_dt_write("\" >" SVG_NEW_LINE, dt_writer);
			}

			// 図形リストの各図形について以下を繰り返す.
			for (auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				// 図形が画像か判定する.
				if (typeid(*s) == typeid(ShapeImage)) {
					// 提案される画像ファイル名を得る.
					// SVG ファイル名を xxxx.svg とすると,
					// 得られるファイル名は xxxx_yyyymmddhhmmss_999.bmp になる
					const size_t NAME_LEN = 1024;
					wchar_t image_name[NAME_LEN];
					{
						static uint32_t magic_num = 0;	// ミリ秒の代わり
						const time_t t = time(nullptr);
						struct tm tm;
						localtime_s(&tm, &t);
						swprintf(image_name, NAME_LEN - 20, L"%s", svg_file.Name().data());
						const wchar_t* const dot_ptr = wcsrchr(image_name, L'.');
						const size_t dot_len = (dot_ptr != nullptr ? dot_ptr - image_name : wcslen(image_name));	// ピリオドまでの長さ
						const size_t tail_len = dot_len + wcsftime(image_name + dot_len, NAME_LEN - 8 - dot_len, L"_%Y%m%d%H%M%S_", &tm);
						swprintf(image_name + tail_len, NAME_LEN - tail_len, L"%03d", magic_num++);
					}

					// 画像用のファイル保存ピッカーを開いて, ストレージファイルを得る.
					// ピッカーを開くので UI スレッドに変える.
					ShapeImage* const t = static_cast<ShapeImage*>(s);
					//					co_await winrt::resume_foreground(Dispatcher());
					StorageFile image_file{
						co_await file_pick_save_image_async(image_name)
					};
					//					co_await winrt::resume_background();
					if (image_file != nullptr) {
						CachedFileManager::DeferUpdates(image_file);
						IRandomAccessStream image_stream{
							co_await image_file.OpenAsync(FileAccessMode::ReadWrite)
						};
						const bool ret = co_await t->copy_to(m_enc_id, image_stream);
						// 遅延させたファイル更新を完了し, 結果を判定する.
						if (ret && co_await CachedFileManager::CompleteUpdatesAsync(image_file) == FileUpdateStatus::Complete) {
							wcscpy_s(image_name, NAME_LEN, image_file.Path().c_str());
						}
						image_stream.Close();
						image_stream = nullptr;
						image_file = nullptr;

						// スレッドコンテキストを復元する.
						//co_await context;
						t->svg_write(image_name, dt_writer);
					}
					else {
						t->svg_write(dt_writer);
					}
				}
				else {
					s->svg_write(dt_writer);
				}
			}
			// SVG 終了タグを書き込む.
			svg_dt_write("</svg>" SVG_NEW_LINE, dt_writer);
			// ストリームの現在位置をストリームの大きさに格納する.
			svg_stream.Size(svg_stream.Position());
			// バッファ内のデータをストリームに出力する.
			co_await dt_writer.StoreAsync();
			// ストリームをフラッシュする.
			co_await svg_stream.FlushAsync();
			// 遅延させたファイル更新を完了し, 結果を判定する.
			if (co_await CachedFileManager::CompleteUpdatesAsync(svg_file) == FileUpdateStatus::Complete) {
				// 完了した場合, S_OK を結果に格納する.
				hr = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, エラーコードを結果に格納する.
			hr = e.code();
		}
		// 結果が S_OK 以外か判定する.
		if (hr != S_OK) {
			// スレッドをメインページの UI スレッドに変える.
//			co_await winrt::resume_foreground(Dispatcher());
			// 「ファイルに書き込めません」メッセージダイアログを表示する.
			message_show(ICON_ALERT, RES_ERR_WRITE, svg_file.Path());
		}
		// スレッドコンテキストを復元する.
//		co_await context;
		m_mutex_exit.unlock();
		// 結果を返し終了する.
		co_return hr;
	}

}