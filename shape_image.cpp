#include <time.h>
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapBufferAccessMode;
	using winrt::Windows::Graphics::Imaging::BitmapEncoder;
	using winrt::Windows::Storage::Streams::RandomAccessStreamReference;

	// 点に最も近い, 線分上の点を求める.
	static void image_get_pos_on_line(
		const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b, 
		/*--->*/D2D1_POINT_2F& q) noexcept;

	winrt::com_ptr<IWICImagingFactory2> ShapeImage::wic_factory{	// WIC ファクトリ
		[]() {
			winrt::com_ptr<IWICImagingFactory2> factory;
			winrt::check_hresult(
				CoCreateInstance(
					CLSID_WICImagingFactory,
					nullptr,
					CLSCTX_INPROC_SERVER,
					__uuidof(IWICImagingFactory2),
					factory.put_void()
					//IID_PPV_ARGS(&factory)
				)
			);
			return factory;
		}()
	};

	// 点に最も近い, 線分上の点を求める.
	//
	// p	点
	// a	線分の始点
	// b	線分の終点
	// q	最も近い点
	static void image_get_pos_on_line(const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b,
		/*--->*/D2D1_POINT_2F& q) noexcept
	{
		//        p
		//        *
		// a      q       b
		// *------*-------*
		//
		// 線分 ab が垂直か判定する.
		if (a.x == b.x) {
			q.x = a.x;
			q.y = p.y;
		}
		// 線分 ab が水平か判定する.
		else if (a.y == b.y) {
			q.x = p.x;
			q.y = a.y;
		}
		else {
			const double ap_x = static_cast<double>(p.x) - static_cast<double>(a.x);
			const double ap_y = static_cast<double>(p.y) - static_cast<double>(a.y);
			const double ab_x = static_cast<double>(b.x) - static_cast<double>(a.x);
			const double ab_y = static_cast<double>(b.y) - static_cast<double>(a.y);
			const double ap_ab = ap_x * ab_x + ap_y * ab_y;	// ap.ab
			const double ab_ab = ab_x * ab_x + ab_y * ab_y;	// ab.ab
			pt_mul_add(D2D1_POINT_2F{ static_cast<FLOAT>(ab_x), static_cast<FLOAT>(ab_y) }, ap_ab / ab_ab, a, q);	// a + ap.ab / (|ab|^2) ab -> q
		}
	}

	// ビットマップをストリームに格納する.
	// C	true ならクリッピングする. false ならしない.
	// enc_id	符号化方式
	// stream	画像を格納するランダムアクセスストリーム
	// 戻り値	格納できたら true
	template <bool C>
	IAsyncOperation<bool> ShapeImage::copy(const winrt::guid enc_id, IRandomAccessStream& stream) const
	{
		// 格納先がクリップボードなら, 元データのままコピーする.
		// エクスポートなら, クリッピングしたデータをコピーする.
		// SVG には, 画像をクリッピングして表示する機能がないか, あっても煩雑.
		// ひょっとしたら, クリッピング矩形の小数点のせいで微妙に異なる画像になるかもしれない.
		// PDF には, 3 バイト RGB を提供できないので, 使えない.
		bool ret = false;	// 返値
		const int32_t c_left = [=]() {	// クリッピング方形の左の値
			if constexpr (C) {
				return static_cast<int32_t>(std::floorf(m_clip.left)); 
			}
			else {
				return 0;
			}
		}();
		const int32_t c_top = [=]() {	// クリッピング方形の上の値
			if constexpr (C) { 
				return static_cast<int32_t>(std::floorf(m_clip.top)); 
			}
			else {
				return 0;
			}
		}(); 
		const int32_t c_right = [=]() {	// クリッピング方形の右の値
			if constexpr (C) {
				return static_cast<int32_t>(std::ceilf(m_clip.right));
			}
			else {
				return m_orig.width;
			}
		}(); 
		const int32_t c_bottom = [=]() {	// クリッピング方形の下の値
			if constexpr (C) {
				return static_cast<int32_t>(std::ceilf(m_clip.bottom));
			}
			else {
				return m_orig.height;
			}
		}();
		if (c_left >= 0 && c_top >= 0 && c_right > c_left && c_bottom > c_top) {
			// バックグラウンドに切替かえて実行する.
			// UI スレッドのままでは, SoftwareBitmap が解放されるときエラーが起きる.
			winrt::apartment_context context;
			co_await winrt::resume_background();

			// SoftwareBitmap を作成する.
			// Windows では, SoftwareBitmap にしろ WIC にしろ
			// 3 バイト RGB はサポートされてない.
			const int32_t c_width = c_right - c_left;
			const int32_t c_height = c_bottom - c_top;
			SoftwareBitmap soft_bmp{
				SoftwareBitmap(BitmapPixelFormat::Bgra8, c_width, c_height, BitmapAlphaMode::Straight)
			};

			// ビットマップのバッファをロックする.
			auto soft_bmp_buf{
				soft_bmp.LockBuffer(BitmapBufferAccessMode::ReadWrite)
			};
			// バッファの参照を得る.
			auto soft_bmp_ref{
				soft_bmp_buf.CreateReference()
			};
			// 参照をメモリバッファアクセスに変換する.
			winrt::com_ptr<IMemoryBufferByteAccess> soft_bmp_mem{
				soft_bmp_ref.as<IMemoryBufferByteAccess>()
			};

			// メモリバッファアクセスからロックされた生バッファーを得る.
			BYTE* soft_bmp_ptr = nullptr;
			UINT32 capacity = 0;
			if (SUCCEEDED(soft_bmp_mem->GetBuffer(&soft_bmp_ptr, &capacity)))
			{
				// 図形の画像データを
				// SoftwareBitmap のバッファにコピーする.
				if constexpr (C) {
					// クリッピングあり
					auto dst_ptr = soft_bmp_ptr;
					auto src_ptr = m_bgra + 4ull * (m_orig.width * c_top + c_left);
					for (size_t y = 0; y < c_height; y++) {
						memcpy(dst_ptr, src_ptr, 4ull * c_width);
						dst_ptr += 4ull * c_width;
						src_ptr += 4ull * m_orig.width;
					}
				}
				else {
					// クリッピングなし
					memcpy(soft_bmp_ptr, m_bgra, capacity);
				}

				// バッファを解放する.
				// SoftwareBitmap がロックされたままになるので,
				// IMemoryBufferByteAccess は Release する必要がある.
				soft_bmp_buf.Close();
				soft_bmp_buf = nullptr;
				soft_bmp_ref.Close();
				soft_bmp_ref = nullptr;
				soft_bmp_mem->Release();
				soft_bmp_mem = nullptr;

				// ビットマップ符号器を作成する.
				BitmapEncoder bmp_enc{
					co_await BitmapEncoder::CreateAsync(enc_id, stream)
				};

				// 符号器にソフトウェアビットマップを格納する.
				bmp_enc.SetSoftwareBitmap(soft_bmp);
				try {
					// 符号化されたビットマップをストリームに書き出す.
					co_await bmp_enc.FlushAsync();
					ret = true;
				}
				catch (const winrt::hresult_error& e) {
					ret = e.code();
				}

				/*
				// 符号器を, 新しいサムネイルを生成するよう設定する.]
				bmp_enc.IsThumbnailGenerated(true);
				// 符号器にソフトウェアビットマップを格納する.
				bmp_enc.SetSoftwareBitmap(soft_bmp);
				try {
					// 符号化されたビットマップをストリームに書き出す.
					co_await bmp_enc.FlushAsync();
					ret = true;
				}
				catch (const winrt::hresult_error& e) {
					// サムネイルの自動生成が出来ないなら, false を格納する.
					if (err.code() == WINCODEC_ERR_UNSUPPORTEDOPERATION) {
						bmp_enc.IsThumbnailGenerated(false);
					}
				}
				if (!bmp_enc.IsThumbnailGenerated()) {
					// 再度やり直す.
					co_await bmp_enc.FlushAsync();
				}
				*/

				// 符号器を解放する.
				bmp_enc = nullptr;
			}
			// ビットマップを破棄する.
			soft_bmp.Close();
			soft_bmp = nullptr;

			// スレッドコンテキストを復元する.
			co_await context;
		}
		co_return ret;
	}
	template IAsyncOperation<bool> ShapeImage::copy<true>(
		const winrt::guid enc_id, IRandomAccessStream& stream) const;
	template IAsyncOperation<bool> ShapeImage::copy<false>(
		const winrt::guid enc_id, IRandomAccessStream& stream) const;

	// 図形を表示する.
	void ShapeImage::draw(void) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
		HRESULT hr = S_OK;

		// D2D ビットマップが空なら, 作成する.
		if (m_d2d_bitmap == nullptr) {
			const D2D1_BITMAP_PROPERTIES b_prop{
				D2D1::BitmapProperties(
					D2D1::PixelFormat(
						DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM,
						D2D1_ALPHA_MODE::D2D1_ALPHA_MODE_PREMULTIPLIED)
				)
			};
			const UINT32 pitch = 4u * m_orig.width;
			winrt::com_ptr<ID2D1Bitmap> bitmap;
			hr = target->CreateBitmap(m_orig, static_cast<void*>(m_bgra), pitch, b_prop, bitmap.put());
			m_d2d_bitmap = bitmap.try_as<ID2D1Bitmap1>();
			if (m_d2d_bitmap == nullptr) {
				return;
			}
		}

		const D2D1_RECT_F rect{
			m_start.x,
			m_start.y,
			m_start.x + m_view.width,
			m_start.y + m_view.height
		};
		target->DrawBitmap(m_d2d_bitmap.get(), rect, m_opac, D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, m_clip);

		// 選択された図形なら, 補助線と図形の部位を表示する.
		if (m_loc_show && is_selected()) {
			brush->SetColor(COLOR_WHITE);
			target->DrawRectangle(rect, brush, m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawRectangle(rect, brush, m_aux_width, m_aux_style.get());

			// 部位を表示する.
			// 0---1
			// |   |
			// 3---2
			D2D1_POINT_2F a_pos[4]{	// 方形の頂点
				{ m_start.x, m_start.y },
				{ m_start.x + m_view.width, m_start.y },
				{ m_start.x + m_view.width, m_start.y + m_view.height },
				{ m_start.x, m_start.y + m_view.height }
			};
			D2D1_POINT_2F a_mid;	// 方形の辺の中点
			pt_avg(a_pos[0], a_pos[3], a_mid);
			loc_draw_square(a_mid, target, brush);
			pt_avg(a_pos[0], a_pos[1], a_mid);
			loc_draw_square(a_mid, target, brush);
			pt_avg(a_pos[1], a_pos[2], a_mid);
			loc_draw_square(a_mid, target, brush);
			pt_avg(a_pos[2], a_pos[3], a_mid);
			loc_draw_square(a_mid, target, brush);
			loc_draw_square(a_pos[0], target, brush);
			loc_draw_square(a_pos[1], target, brush);
			loc_draw_square(a_pos[3], target, brush);
			loc_draw_square(a_pos[2], target, brush);
		}
	}

	// 境界矩形を得る.
	void ShapeImage::get_bbox(
		const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt,
		D2D1_POINT_2F& b_rb) const noexcept
	{
		D2D1_POINT_2F b[2]{
			m_start, D2D1_POINT_2F{ m_start.x + m_view.width, m_start.y + m_view.height }
		};
		if (b[0].x > b[1].x) {
			const auto less_x = b[1].x;
			b[1].x = b[0].x;
			b[0].x = less_x;
		}
		if (b[0].y > b[1].y) {
			const auto less_y = b[1].y;
			b[1].y = b[0].y;
			b[0].y = less_y;
		}
		b_lt.x = a_lt.x < b[0].x ? a_lt.x : b[0].x;
		b_lt.y = a_lt.y < b[0].y ? a_lt.y : b[0].y;
		b_rb.x = a_rb.x > b[1].x ? a_rb.x : b[1].x;
		b_rb.y = a_rb.y > b[1].y ? a_rb.y : b[1].y;
	}

	// 画像の不透明度を得る.
	bool ShapeImage::get_image_opacity(float& val) const noexcept
	{
		val = m_opac;
		return true;
	}

	// 画素の色を得る.
	// p	ページ座標での位置
	// val	画素の色
	// 戻り値	色を得られたなら true, そうでなければ false.
	bool ShapeImage::get_pixcel(const D2D1_POINT_2F p, D2D1_COLOR_F& val) const noexcept
	{
		// ページ座標での位置を, 元画像での位置に変換する.
		const double fx = round(m_clip.left + (p.x - m_start.x) * (m_clip.right - m_clip.left) / m_view.width);
		const double fy = round(m_clip.top + (p.y - m_start.y) * (m_clip.bottom - m_clip.top) / m_view.height);
		// 変換された位置が, 画像に収まるなら,
		if (fx >= 0.0 && fx <= m_orig.width && fy >= 0.0 && fy <= m_orig.height) {
			// 生データでの画素あたりの添え字に変換.
			const size_t i = m_orig.width * static_cast<size_t>(fy) + static_cast<size_t>(fx);
			// 色成分を D2D1_COLOR_F に変換.
			const uint8_t b = m_bgra[i * 4 + 0];
			const uint8_t g = m_bgra[i * 4 + 1];
			const uint8_t r = m_bgra[i * 4 + 2];
			const uint8_t a = m_bgra[i * 4 + 3];
			val.b = static_cast<float>(b) / 255.0f;
			val.g = static_cast<float>(g) / 255.0f;
			val.r = static_cast<float>(r) / 255.0f;
			val.a = (static_cast<float>(a) / 255.0f) * m_opac;
			return true;
		}
		return false;
	}

	// 指定した部位の点を得る.
	void ShapeImage::get_pos_loc(
		const uint32_t loc,	// 部位
		D2D1_POINT_2F& val	// 得られた値
	) const noexcept
	{
		if (loc == LOC_TYPE::LOC_NW) {
			val = m_start;
		}
		else if (loc == LOC_TYPE::LOC_NORTH) {
			val.x = m_start.x + m_view.width * 0.5f;
			val.y = m_start.y;
		}
		else if (loc == LOC_TYPE::LOC_NE) {
			val.x = m_start.x + m_view.width;
			val.y = m_start.y;
		}
		else if (loc == LOC_TYPE::LOC_EAST) {
			val.x = m_start.x + m_view.width;
			val.y = m_start.y + m_view.height * 0.5f;
		}
		else if (loc == LOC_TYPE::LOC_SE) {
			val.x = m_start.x + m_view.width;
			val.y = m_start.y + m_view.height;
		}
		else if (loc == LOC_TYPE::LOC_SOUTH) {
			val.x = m_start.x + m_view.width * 0.5f;
			val.y = m_start.y + m_view.height;
		}
		else if (loc == LOC_TYPE::LOC_SW) {
			val.x = m_start.x;
			val.y = m_start.y + m_view.height;
		}
		else if (loc == LOC_TYPE::LOC_WEST) {
			val.x = m_start.x;
			val.y = m_start.y + m_view.height * 0.5f;
		}
	}

	// 境界矩形の左上位置を得る.
	void ShapeImage::get_bbox_lt(D2D1_POINT_2F& val) const noexcept
	{
		const float ax = m_start.x;
		const float ay = m_start.y;
		const float bx = m_start.x + m_view.width;
		const float by = m_start.y + m_view.height;
		val.x = ax < bx ? ax : bx;
		val.y = ay < by ? ay : by;
	}

	// 近傍の頂点を見つける.
	// p	ある位置
	// dd	近傍とみなす距離 (の二乗値), これより離れた頂点は近傍とはみなさない.
	// val	ある位置の近傍にある頂点
	// 戻り値	見つかったら true
	bool ShapeImage::get_pos_nearest(const D2D1_POINT_2F p, double& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool found = false;
		D2D1_POINT_2F q[4];	// 図形の頂点.
		get_verts(q);
		for (size_t i = 0; i < 4; i++) {
			D2D1_POINT_2F r;	// pq 間のベクトル
			pt_sub(p, q[i], r);
			const double r_abs = pt_abs2(r);
			if (r_abs < dd) {
				dd = r_abs;
				val = q[i];
				if (!found) {
					found = true;
				}
			}
		}
		return found;
	}

	// 始点を得る.
	bool ShapeImage::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		val = m_start;
		return true;
	}

	// 頂点を得る.
	size_t ShapeImage::get_verts(D2D1_POINT_2F p[]) const noexcept
	{
		p[0] = m_start;
		p[1] = D2D1_POINT_2F{ m_start.x + m_view.width, m_start.y };
		p[2] = D2D1_POINT_2F{ m_start.x + m_view.width, m_start.y + m_view.height };
		p[3] = D2D1_POINT_2F{ m_start.x, m_start.y + m_view.height };
		return 4;
	}

	// 図形が点を含むか判定する.
	// 戻り値	点を含む部位
	uint32_t ShapeImage::hit_test(const D2D1_POINT_2F pt, const bool/*ctrl_key*/) const noexcept
	{
		D2D1_POINT_2F p[4];
		// 0---1
		// |   |
		// 3---2
		get_verts(p);
		if (loc_hit_test(pt, p[2], m_loc_width)) {
			return LOC_TYPE::LOC_SE;
		}
		else if (loc_hit_test(pt, p[3], m_loc_width)) {
			return LOC_TYPE::LOC_SW;
		}
		else if (loc_hit_test(pt, p[1], m_loc_width)) {
			return LOC_TYPE::LOC_NE;
		}
		else if (loc_hit_test(pt, p[0], m_loc_width)) {
			return LOC_TYPE::LOC_NW;
		}
		else {
			const auto e_width = m_loc_width * 0.5;
			D2D1_POINT_2F e[2];
			e[0].x = p[0].x;
			e[0].y = static_cast<FLOAT>(p[0].y - e_width);
			e[1].x = p[1].x;
			e[1].y = static_cast<FLOAT>(p[1].y + e_width);
			if (pt_in_rect(pt, e[0], e[1])) {
				return LOC_TYPE::LOC_NORTH;
			}
			e[0].x = static_cast<FLOAT>(p[1].x - e_width);
			e[0].y = p[1].y;
			e[1].x = static_cast<FLOAT>(p[2].x + e_width);
			e[1].y = p[2].y;
			if (pt_in_rect(pt, e[0], e[1])) {
				return LOC_TYPE::LOC_EAST;
			}
			e[0].x = p[3].x;
			e[0].y = static_cast<FLOAT>(p[3].y - e_width);
			e[1].x = p[2].x;
			e[1].y = static_cast<FLOAT>(p[2].y + e_width);
			if (pt_in_rect(pt, e[0], e[1])) {
				return LOC_TYPE::LOC_SOUTH;
			}
			e[0].x = static_cast<FLOAT>(p[0].x - e_width);
			e[0].y = p[0].y;
			e[1].x = static_cast<FLOAT>(p[3].x + e_width);
			e[1].y = p[3].y;
			if (pt_in_rect(pt, e[0], e[1])) {
				return LOC_TYPE::LOC_WEST;
			}
		}
		if (p[0].x > p[2].x) {
			const float less_x = p[2].x;
			p[2].x = p[0].x;
			p[0].x = less_x;
		}
		if (p[0].y > p[2].y) {
			const float less_y = p[2].y;
			p[2].y = p[0].y;
			p[0].y = less_y;
		}
		if (p[0].x <= pt.x && pt.x <= p[2].x &&
			p[0].y <= pt.y && pt.y <= p[2].y) {
			return LOC_TYPE::LOC_FILL;
		}
		return LOC_TYPE::LOC_SHEET;
	}

	// 矩形に含まれるか判定する.
	// lt	矩形の左上位置
	// rb	矩形の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeImage::is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		// 始点と終点とが範囲に含まれるか判定する.
		if (pt_in_rect(m_start, lt, rb)) {
			const D2D1_POINT_2F end{
				m_start.x + m_view.width, m_start.y + m_view.height
			};
			return pt_in_rect(end, lt, rb);
		}
		return false;
	}

	// 位置を移動する.
	// pos	位置ベクトル
	bool ShapeImage::move(const D2D1_POINT_2F pos) noexcept
	{
		D2D1_POINT_2F start;
		pt_add(m_start, pos, start);
		return set_pos_start(start);
	}

	// 原画像に戻す.
	void ShapeImage::revert(void) noexcept
	{
		m_view.width = static_cast<float>(m_orig.width);
		m_view.height = static_cast<float>(m_orig.height);
		m_clip.left = 0.0f;
		m_clip.top = 0.0f;
		m_clip.right = static_cast<float>(m_orig.width);
		m_clip.bottom = static_cast<float>(m_orig.height);
		m_opac = 1.0f;
	}

	// 値を画像の不透明度に格納する.
	bool ShapeImage::set_image_opacity(const float val) noexcept
	{
		if (!equal(m_opac, val)) {
			m_opac = val;
			return true;
		}
		return false;
	}

	// 値を, 指定した部位の点に格納する.
	// val	値
	// loc	部位
	// keep_aspect	画像の縦横比の維持/可変
	bool ShapeImage::set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float, const bool keep_aspect) noexcept
	{
		D2D1_POINT_2F new_p;
		pt_round(val, PT_ROUND, new_p);

		if (loc == LOC_TYPE::LOC_NW) {
			const auto new_w = m_start.x + m_view.width - new_p.x;
			const auto new_h = m_start.y + m_view.height - new_p.y;
			if (new_w > 0.0f && new_h > 0.0f) {
				if (keep_aspect) {
					const auto old_w = m_view.width;
					const auto old_h = m_view.height;
					if (abs(new_w - old_w) < abs(new_h - old_h)) {
						if (!equal(old_w, new_w)) {
							const auto fit_h = old_h * new_w / old_w;	// 比率を合わせた高さ
							m_start.x = new_p.x;	// new_p.x = m_start.x + m_view.width - new_w
							m_start.y = m_start.y + old_h - fit_h;
							m_view.width = new_w;
							m_view.height = fit_h;
							return true;
						}
					}
					else {
						if (!equal(old_h, new_h)) {
							const auto fit_w = old_w * new_h / old_h;	// 比率を合わせた高さ
							m_start.x = m_start.x + old_w - fit_w;
							m_start.y = new_p.y;
							m_view.width = fit_w;
							m_view.height = new_h;
							return true;
						}
					}
				}
				else {
					if (!equal(m_start, new_p)) {
						m_view.width = new_w;
						m_view.height = new_h;
						m_start = new_p;
						return true;
					}
				}
			}
			//else {
			//	if (!equal(m_start, new_p)) {
			//		m_start = new_p;
			//		return true;
			//	}
			//}
		}
		else if (loc == LOC_TYPE::LOC_SE) {
			const auto new_w = new_p.x - m_start.x;
			const auto new_h = new_p.y - m_start.y;
			if (new_w > 0.0f && new_h > 0.0f) {
				if (keep_aspect) {
					const auto old_w = m_view.width;
					const auto old_h = m_view.height;
					if (abs(new_w - old_w) < abs(new_h - old_h)) {
						if (!equal(old_w, new_w)) {
							const auto fit_h = old_h * new_w / old_w;
							m_view.width = new_w;
							m_view.height = fit_h;
							return true;
						}
					}
					else {
						if (!equal(old_h, new_h)) {
							const auto fit_w = old_w * new_h / old_h;
							m_view.width = fit_w;
							m_view.height = new_h;
							return true;
						}
					}
				}
				else {
					if (!equal(m_view.width, new_w) || !equal(m_view.height, new_h)) {
						m_view.width = new_w;
						m_view.height = new_h;
						return true;
					}
				}
			}
			//else {
			//	if (!equal(m_start, new_p)) {
			//		m_start = new_p;
			//		return true;
			//	}
			//}
		}
		else if (loc == LOC_TYPE::LOC_SW) {
			const auto new_w = m_start.x + m_view.width - new_p.x;
			const auto new_h = new_p.y - m_start.y;
			if (new_w > 0.0f && new_h > 0.0f) {
				if (keep_aspect) {
					const auto old_w = m_view.width;
					const auto old_h = m_view.height;
					if (abs(new_w - old_w) < abs(new_h - old_h)) {
						if (!equal(old_w, new_w)) {
							const auto fit_h = old_h * new_w / old_w;
							m_start.x = new_p.x;
							m_view.width = new_w;
							m_view.height = fit_h;
							return true;
						}
					}
					else {
						if (!equal(old_h, new_h)) {
							const auto fit_w = old_w * new_h / old_h;
							m_start.x = m_start.x + old_w - fit_w;
							m_view.width = fit_w;
							m_view.height = new_h;
							return true;
						}
					}
				}
				else {
					if (!equal(m_start.x, new_p.x)) {
						m_view.width = new_w;
						m_view.height = new_h;
						m_start.x = new_p.x;
						return true;
					}
				}
			}
			//else {
			//	if (!equal(m_start, new_p)) {
			//		m_start = new_p;
			//		return true;
			//	}
			//}
		}
		else if (loc == LOC_TYPE::LOC_NE) {
			const auto new_w = new_p.x - m_start.x;
			const auto new_h = m_start.y + m_view.height - new_p.y;
			if (new_w > 0.0f && new_h > 0.0f) {
				if (keep_aspect) {
					const auto old_w = m_view.width;
					const auto old_h = m_view.height;
					if (abs(new_w - old_w) < abs(new_h - old_h)) {
						if (!equal(old_w, new_w)) {
							const auto fit_h = old_h * new_w / old_w;
							m_start.y = m_start.y + old_h - fit_h;
							m_view.width = new_w;
							m_view.height = fit_h;
							return true;
						}
					}
					else {
						if (!equal(old_h, new_h)) {
							const auto fit_w = old_w * new_h / old_h;
							m_start.y = new_p.y;
							m_view.width = fit_w;
							m_view.height = new_h;
							return true;
						}
					}
				}
				else {
					if (!equal(m_start.y, new_p.y)) {
						m_start.y = new_p.y;
						m_view.width = new_w;
						m_view.height = new_h;
						return true;
					}
				}
			}
			//else {
			//	if (!equal(m_start, new_p)) {
			//		m_start = new_p;
			//		return true;
			//	}
			//}
		}
		else if (loc == LOC_TYPE::LOC_WEST) {
			auto new_w = m_start.x + m_view.width - new_p.x;
			if (new_w > 0.0) {
				if (keep_aspect) {
					const auto ratio_w = m_view.width / (m_clip.right - m_clip.left);	// 表示枠とクリッピングされた元画像の比率
					m_clip.left = m_clip.left - (new_w - m_view.width) / ratio_w;
					if (m_clip.left < 0.0) {
						m_clip.right = m_clip.right - m_clip.left;
						m_clip.left = 0.0;
					}
					if (m_clip.right > m_orig.width) {
						new_w = new_w - (m_clip.right - m_orig.width) * ratio_w;
						m_clip.right = m_orig.width;
					}
				}
				m_start.x = new_p.x;
				m_view.width = new_w;
				return true;
			}
			//else {
			//	if (!equal(m_start.x, new_p.x)) {
			//		m_start.x = new_p.x;
			//		return true;
			//	}
			//}
		}
		else if (loc == LOC_TYPE::LOC_NORTH) {
			auto new_h = m_start.y + m_view.height - new_p.y;
			if (new_h > 0.0) {
				if (keep_aspect) {
					const auto ratio_h = m_view.height / (m_clip.bottom - m_clip.top);	// 表示枠とクリッピングされた元画像の比率
					m_clip.top = m_clip.top - (new_h - m_view.height) / ratio_h;
					if (m_clip.top < 0.0) {
						m_clip.bottom = m_clip.bottom - m_clip.top;
						m_clip.top = 0.0;
					}
					if (m_clip.bottom > m_orig.height) {
						new_h = new_h - (m_clip.bottom - m_orig.height) * ratio_h;
						m_clip.bottom = m_orig.height;
					}
				}
				m_start.y = new_p.y;
				m_view.height = new_h;
				return true;
			}
			//else {
			//	if (!equal(m_start.y, new_p.y)) {
			//		m_start.y = new_p.y;
			//		return true;
			//	}
			//}
		}
		else if (loc == LOC_TYPE::LOC_SOUTH) {
			auto new_h = new_p.y - m_start.y;
			if (new_h > 0.0) {
				if (keep_aspect) {
					const auto ratio_h = m_view.height / (m_clip.bottom - m_clip.top);
					m_clip.bottom = m_clip.bottom + (new_h - m_view.height) / ratio_h;
					if (m_clip.bottom > m_orig.height) {
						m_clip.top = m_clip.top - (m_clip.bottom - m_orig.height);
						m_clip.bottom = m_orig.height;
					}
					if (m_clip.top < 0.0) {
						new_h = new_h + m_clip.top * ratio_h;
						m_clip.top = 0.0;
					}
				}
				m_start.y = new_p.y - new_h;
				m_view.height = new_h;
				return true;
			}
			//else {
			//	if (!equal(m_start.y, new_p.y)) {
			//		m_start.y = new_p.y;
			//		return true;
			//	}
			//}
		}
		else if (loc == LOC_TYPE::LOC_EAST) {
			auto new_w = new_p.x - m_start.x;
			if (new_w > 0.0) {
				if (keep_aspect) {
					const auto ratio_w = m_view.width / (m_clip.right - m_clip.left);
					m_clip.right = m_clip.right + (new_w - m_view.width) / ratio_w;
					if (m_clip.right > m_orig.width) {
						m_clip.left = m_clip.left - (m_clip.right - m_orig.width);
						m_clip.right = m_orig.width;
					}
					if (m_clip.left < 0.0) {
						new_w = new_w + m_clip.left * ratio_w;
						m_clip.left = 0.0;
					}
				}
				m_start.x = new_p.x - new_w;
				m_view.width = new_w;
				return true;
			}
			//else {
			//	if (!equal(m_start.y, new_p.y)) {
			//		m_start.y = new_p.y;
			//		return true;
			//	}
			//}
		}
		return false;
	}

	// 値を始点に格納する. 他の部位の位置も動く.
	bool ShapeImage::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F pt;
		pt_round(val, PT_ROUND, pt);
		if (!equal(m_start, pt)) {
			m_start = pt;
			return true;
		}
		return false;
	}

	// 図形を作成する.
	// pt	左上点
	// view	表示される大きさ
	// bmp	ビットマップ
	// opac	不透明度
	ShapeImage::ShapeImage(const D2D1_POINT_2F pt, const D2D1_SIZE_F view, const SoftwareBitmap& bmp, const float opac)
	{
		const uint32_t image_w = bmp.PixelWidth();
		const uint32_t image_h = bmp.PixelHeight();

		m_orig.width = image_w;
		m_orig.height = image_h;
		m_start = pt;
		m_view = view;
		m_clip.left = m_clip.top = 0;
		m_clip.right = static_cast<FLOAT>(image_w);
		m_clip.bottom = static_cast<FLOAT>(image_h);
		m_opac = opac < 0.0f ? 0.0f : (opac > 1.0f ? 1.0f : opac);
		const size_t bgra_size = 4ull * image_w * image_h;
		if (bgra_size > 0) {
			m_bgra = new uint8_t[bgra_size];
			// SoftwareBitmap のバッファをロックする.
			auto image_buf{
				bmp.LockBuffer(BitmapBufferAccessMode::ReadWrite)
			};
			auto image_ref{
				image_buf.CreateReference()
			};
			winrt::com_ptr<IMemoryBufferByteAccess> image_mem{
				image_ref.as<IMemoryBufferByteAccess>()
			};
			BYTE* image_data = nullptr;
			UINT32 capacity = 0;
			if (SUCCEEDED(image_mem->GetBuffer(&image_data, &capacity))) {
				// ロックしたバッファに画像データをコピーする.
				memcpy(m_bgra, image_data, capacity);
				// ロックしたバッファを解放する.
				image_buf.Close();
				image_buf = nullptr;
			}
			image_mem = nullptr;
		}
		else {
			m_bgra = nullptr;
		}
	}

	// 図形をデータリーダーから読み込む
	// dt_reader	データリーダー
	ShapeImage::ShapeImage(DataReader const& dt_reader) :
		ShapeSelect(dt_reader),
		m_start(D2D1_POINT_2F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_view(D2D1_SIZE_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_clip(D2D1_RECT_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle(), 
			dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_orig(D2D1_SIZE_U{ dt_reader.ReadUInt32(), dt_reader.ReadUInt32() }),
		m_opac(dt_reader.ReadSingle())
	{
		if (m_opac < 0.0f || m_opac > 1.0f) {
			m_opac = 1.0f;
		}
		const size_t pitch = 4ull * m_orig.width;
		m_bgra = new uint8_t[pitch * m_orig.height];
		std::vector<uint8_t> line_buf(pitch);
		const size_t height = m_orig.height;
		for (size_t i = 0; i < height; i++) {
			dt_reader.ReadBytes(line_buf);
			memcpy(m_bgra + pitch * i, line_buf.data(), pitch);
		}
	}

	// 図形をデータライターに書き込む.
	// dt_writer	データライター
	void ShapeImage::write(DataWriter const& dt_writer) const
	{
		ShapeSelect::write(dt_writer);
		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);
		dt_writer.WriteSingle(m_view.width);
		dt_writer.WriteSingle(m_view.height);
		dt_writer.WriteSingle(m_clip.left);
		dt_writer.WriteSingle(m_clip.top);
		dt_writer.WriteSingle(m_clip.right);
		dt_writer.WriteSingle(m_clip.bottom);
		dt_writer.WriteUInt32(m_orig.width);
		dt_writer.WriteUInt32(m_orig.height);
		dt_writer.WriteSingle(m_opac);

		const size_t pitch = 4ull * m_orig.width;
		std::vector<uint8_t> line_buf(pitch);
		const size_t height = m_orig.height;
		for (size_t i = 0; i < height; i++) {
			memcpy(line_buf.data(), m_bgra + pitch * i, pitch);
			dt_writer.WriteBytes(line_buf);
		}
	}

}