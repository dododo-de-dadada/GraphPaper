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
	// p	点
	// a	線分の始点
	// b	線分の終点
	// q	最も近い点
	static void image_get_pos_on_line(
		const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b,
		/*--->*/D2D1_POINT_2F& q) noexcept
	{
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
	// stream	画像を格納するストリーム
	// 戻り値	格納できたら true
	template <bool C>
	IAsyncOperation<bool> ShapeImage::copy(const winrt::guid enc_id, IRandomAccessStream& stream) const
	{
		// クリップボードに格納するなら, 元データのまま.
		// 描画するときに DrawBitmap でクリッピングされる.
		// エクスポートするなら, クリッピングしたデータ.
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
			// 3 バイトの RGB はサポートされてない.
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
					for (size_t y = c_top; y < c_bottom; y++) {
						memcpy(soft_bmp_ptr + 4ull * c_width * y, 
							m_bgra + 4ull * m_orig.width * y + c_left,
							4ull * c_width);
					}
				}
				else {
					// クリッピングなし
					memcpy(soft_bmp_ptr, m_bgra, capacity);
				}

				// バッファを解放する.
				// SoftwareBitmap がロックされたままになるので,
				// IMemoryBufferByteAccess は Release() する必要がある.
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
				// 符号器を, 新しいサムネイルを生成するよう設定する.
				//bmp_enc.IsThumbnailGenerated(true);
				// 符号器にソフトウェアビットマップを格納する.
				bmp_enc.SetSoftwareBitmap(soft_bmp);
				try {
					// 符号化されたビットマップをストリームに書き出す.
					co_await bmp_enc.FlushAsync();
					ret = true;
				}
				catch (const winrt::hresult_error& e) {
					// サムネイルの自動生成が出来ないなら, false を格納する.
					//if (err.code() == WINCODEC_ERR_UNSUPPORTEDOPERATION) {
					//	bmp_enc.IsThumbnailGenerated(false);
					//}
					//else
					ret = e.code();
				}
				//if (!bmp_enc.IsThumbnailGenerated()) {
					// 再度やり直す.
				//	co_await bmp_enc.FlushAsync();
				//}
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
	void ShapeImage::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		if (m_d2d_bitmap == nullptr) {
			//const D2D1_BITMAP_PROPERTIES1 b_prop{
			//	D2D1::BitmapProperties1(
			//		D2D1_BITMAP_OPTIONS_NONE,
			//		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
			//	)
			//};
			const D2D1_BITMAP_PROPERTIES b_prop{
				D2D1::BitmapProperties(
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
				)
			};
			const UINT32 pitch = 4u * m_orig.width;
			winrt::com_ptr<ID2D1Bitmap> bitmap;
			target->CreateBitmap(m_orig, static_cast<void*>(m_bgra), pitch, b_prop, bitmap.put());
			m_d2d_bitmap = bitmap.as<ID2D1Bitmap1>();
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
		target->DrawBitmap(
			m_d2d_bitmap.get(), rect, m_opac,
			D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
			m_clip);

		if (m_anc_show && is_selected()) {
			brush->SetColor(COLOR_WHITE);
			target->DrawRectangle(rect, brush, m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawRectangle(rect, brush, m_aux_width, m_aux_style.get());
			const D2D1_POINT_2F a[4]{
				m_start,
				{ m_start.x + m_view.width, m_start.y },
				{ m_start.x + m_view.width, m_start.y + m_view.height },
				{ m_start.x, m_start.y + m_view.height },
			};
			anc_draw_square(a[0], target, brush);
			anc_draw_square(a[1], target, brush);
			anc_draw_square(a[2], target, brush);
			anc_draw_square(a[3], target, brush);
		}
	}

	// 図形を囲む領域を得る.
	void ShapeImage::get_bound(
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
		const double fx = round(
			m_clip.left + (p.x - m_start.x) * (m_clip.right - m_clip.left) / m_view.width);
		const double fy = round(
			m_clip.top + (p.y - m_start.y) * (m_clip.bottom - m_clip.top) / m_view.height);
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
			val.a = static_cast<float>(a) / 255.0f;
			return true;
		}
		return false;
	}

	// 部位の位置を得る.
	void ShapeImage::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		if (anc == ANC_TYPE::ANC_NW) {
			val = m_start;
		}
		else if (anc == ANC_TYPE::ANC_NORTH) {
			val.x = m_start.x + m_view.width * 0.5f;
			val.y = m_start.y;
		}
		else if (anc == ANC_TYPE::ANC_NE) {
			val.x = m_start.x + m_view.width;
			val.y = m_start.y;
		}
		else if (anc == ANC_TYPE::ANC_EAST) {
			val.x = m_start.x + m_view.width;
			val.y = m_start.y + m_view.height * 0.5f;
		}
		else if (anc == ANC_TYPE::ANC_SE) {
			val.x = m_start.x + m_view.width;
			val.y = m_start.y + m_view.height;
		}
		else if (anc == ANC_TYPE::ANC_SOUTH) {
			val.x = m_start.x + m_view.width * 0.5f;
			val.y = m_start.y + m_view.height;
		}
		else if (anc == ANC_TYPE::ANC_SW) {
			val.x = m_start.x;
			val.y = m_start.y + m_view.height;
		}
		else if (anc == ANC_TYPE::ANC_WEST) {
			val.x = m_start.x;
			val.y = m_start.y + m_view.height * 0.5f;
		}
	}

	// 図形を囲む領域の左上位置を得る.
	void ShapeImage::get_bound_lt(D2D1_POINT_2F& val) const noexcept
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
	bool ShapeImage::get_pos_nearest(const D2D1_POINT_2F p, float& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool found = false;
		D2D1_POINT_2F q[4];	// 図形の頂点.
		get_verts(q);
		for (size_t i = 0; i < 4; i++) {
			D2D1_POINT_2F r;	// pq 間のベクトル
			pt_sub(p, q[i], r);
			const float r_abs = static_cast<float>(pt_abs2(r));
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

	// 開始位置を得る.
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

	// 位置を含むか判定する.
	// test	判定する位置
	// 戻り値	位置を含む図形の部位. 含まないときは「図形の外側」を返す.
	uint32_t ShapeImage::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		D2D1_POINT_2F p[4];
		// 0---1
		// |   |
		// 3---2
		get_verts(p);
		if (pt_in_anc(test, p[2], m_anc_width)) {
			return ANC_TYPE::ANC_SE;
		}
		else if (pt_in_anc(test, p[3], m_anc_width)) {
			return ANC_TYPE::ANC_SW;
		}
		else if (pt_in_anc(test, p[1], m_anc_width)) {
			return ANC_TYPE::ANC_NE;
		}
		else if (pt_in_anc(test, p[0], m_anc_width)) {
			return ANC_TYPE::ANC_NW;
		}
		else {
			const auto e_width = m_anc_width * 0.5;
			D2D1_POINT_2F e[2];
			e[0].x = p[0].x;
			e[0].y = static_cast<FLOAT>(p[0].y - e_width);
			e[1].x = p[1].x;
			e[1].y = static_cast<FLOAT>(p[1].y + e_width);
			if (pt_in_rect(test, e[0], e[1])) {
				return ANC_TYPE::ANC_NORTH;
			}
			e[0].x = static_cast<FLOAT>(p[1].x - e_width);
			e[0].y = p[1].y;
			e[1].x = static_cast<FLOAT>(p[2].x + e_width);
			e[1].y = p[2].y;
			if (pt_in_rect(test, e[0], e[1])) {
				return ANC_TYPE::ANC_EAST;
			}
			e[0].x = p[3].x;
			e[0].y = static_cast<FLOAT>(p[3].y - e_width);
			e[1].x = p[2].x;
			e[1].y = static_cast<FLOAT>(p[2].y + e_width);
			if (pt_in_rect(test, e[0], e[1])) {
				return ANC_TYPE::ANC_SOUTH;
			}
			e[0].x = static_cast<FLOAT>(p[0].x - e_width);
			e[0].y = p[0].y;
			e[1].x = static_cast<FLOAT>(p[3].x + e_width);
			e[1].y = p[3].y;
			if (pt_in_rect(test, e[0], e[1])) {
				return ANC_TYPE::ANC_WEST;
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
		if (p[0].x <= test.x && test.x <= p[2].x &&
			p[0].y <= test.y && test.y <= p[2].y) {
			return ANC_TYPE::ANC_FILL;
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// 矩形範囲に含まれるか判定する.
	// lt	矩形の左上位置
	// rb	矩形の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeImage::in_area(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		// 始点と終点とが範囲に含まれるか判定する.
		return pt_in_rect(m_start, lt, rb) &&pt_in_rect(
				D2D1_POINT_2F{ m_start.x + m_view.width, m_start.y + m_view.height }, lt, rb);
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
		const float src_w = static_cast<float>(m_orig.width);
		const float src_h = static_cast<float>(m_orig.height);
		m_view.width = src_w;
		m_view.height = src_h;
		m_clip.left = m_clip.top = 0.0f;
		m_clip.right = src_w;
		m_clip.bottom = src_h;
		m_ratio.width = m_ratio.height = 1.0f;
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

	// 値を, 部位の位置に格納する.
	// val	値
	// anc	図形の部位
	// limit	他の頂点との限界距離 (この値未満になるなら, 図形の部位が指定する頂点を他の頂点に位置に合わせる)
	// keep_aspect	画像の縦横比を維持
	bool ShapeImage::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float /*limit*/, const bool keep_aspect) noexcept
	{
		D2D1_POINT_2F new_p;
		pt_round(val, PT_ROUND, new_p);

		bool flag = false;
		if (anc == ANC_TYPE::ANC_NW) {
			// 画像の一部分でも表示されているか判定する.
			// (画像がまったく表示されてない場合はスケールの変更は行わない.)
			const float image_w = m_clip.right - m_clip.left;	// 表示されている画像の幅 (原寸)
			const float image_h = m_clip.bottom - m_clip.top;	// 表示されている画像の高さ (原寸)
			if (image_w > 1.0f && image_h > 1.0f) {
				const D2D1_POINT_2F s{ m_start };	// 始点 (図形の頂点)
				D2D1_POINT_2F p;
				if (keep_aspect) {
					const D2D1_POINT_2F e{ s.x + image_w, s.y + image_h };	// 終点 (始点と対角にある画像上の点)
					image_get_pos_on_line(new_p, s, e, p);
				}
				else {
					p = new_p;
				}
				// 値と始点との差分 q を求め, 差分がゼロより大きいか判定する.
				D2D1_POINT_2F q;
				pt_sub(p, s, q);
				if (pt_abs2(q) >= FLT_MIN) {
					// スケール変更後の表示の寸法を求め, その縦横が 1 ピクセル以上あるか判定する.
					const float page_w = m_view.width - q.x;
					const float page_h = m_view.height - q.y;
					if (page_w >= 1.0f && page_h >= 1.0f) {
						m_view.width = page_w;
						m_view.height = page_h;
						m_start = p;
						m_ratio.width = page_w / image_w;
						m_ratio.height = page_h / image_h;
						if (!flag) {
							flag = true;
						}
					}
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_NE) {
			const float image_w = m_clip.right - m_clip.left;
			const float image_h = m_clip.bottom - m_clip.top;
			if (image_w > 1.0f && image_h > 1.0f) {
				const D2D1_POINT_2F s{ m_start.x + m_view.width, m_start.y };
				D2D1_POINT_2F p;
				if (keep_aspect) {
					const D2D1_POINT_2F e{ s.x - image_w, s.y + image_h };
					image_get_pos_on_line(new_p, s, e, p);
				}
				else {
					p = new_p;
				}
				D2D1_POINT_2F q;
				pt_sub(p, s, q);
				if (pt_abs2(q) >= FLT_MIN) {
					const float page_w = p.x - m_start.x;
					const float page_h = m_start.y + m_view.height - p.y;
					if (page_w >= 1.0f && page_h >= 1.0f) {
						m_view.width = page_w;
						m_view.height = page_h;
						m_start.y = p.y;
						m_ratio.width = page_w / image_w;
						m_ratio.height = page_h / image_h;
						if (!flag) {
							flag = true;
						}
					}
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_SE) {
			const float image_w = m_clip.right - m_clip.left;
			const float image_h = m_clip.bottom - m_clip.top;
			if (image_w > 1.0f && image_h > 1.0f) {
				const D2D1_POINT_2F s{ m_start.x + m_view.width, m_start.y + m_view.height };
				D2D1_POINT_2F p;
				if (keep_aspect) {
					const D2D1_POINT_2F e{ s.x - image_w, s.y - image_h };
					image_get_pos_on_line(new_p, s, e, p);
				}
				else {
					p = new_p;
				}
				D2D1_POINT_2F q;
				pt_sub(p, s, q);
				if (pt_abs2(q) >= FLT_MIN) {
					const float page_w = p.x - m_start.x;
					const float page_h = p.y - m_start.y;
					if (page_w >= 1.0f && page_h >= 1.0f) {
						m_view.width = page_w;
						m_view.height = page_h;
						m_ratio.width = page_w / image_w;
						m_ratio.height = page_h / image_h;
						if (!flag) {
							flag = true;
						}
					}
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_SW) {
			const float image_w = m_clip.right - m_clip.left;
			const float image_h = m_clip.bottom - m_clip.top;
			if (image_w > 1.0f && image_h > 1.0f) {
				const D2D1_POINT_2F s{ m_start.x, m_start.y + m_view.height };
				D2D1_POINT_2F p;
				if (keep_aspect) {
					const D2D1_POINT_2F e{ s.x + image_w, s.y - image_h };
					image_get_pos_on_line(new_p, s, e, p);
				}
				else {
					p = new_p;
				}
				D2D1_POINT_2F q;
				pt_sub(p, s, q);
				if (pt_abs2(q) >= FLT_MIN) {
					const float page_w = m_start.x + m_view.width - p.x;
					const float page_h = p.y - m_start.y;
					if (page_w >= 1.0f && page_h >= 1.0f) {
						m_view.width = page_w;
						m_view.height = page_h;
						m_start.x = p.x;
						m_ratio.width = page_w / image_w;
						m_ratio.height = page_h / image_h;
						if (!flag) {
							flag = true;
						}
					}
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_NORTH) {
			const float dy = (new_p.y - m_start.y);
			if (fabs(dy) >= FLT_MIN) {
				if (keep_aspect) {
					const float t = min(m_clip.top + dy / m_ratio.height, m_clip.bottom - 1.0f);	// 上辺
					m_clip.top = max(t, 0.0f);
					m_view.height = (m_clip.bottom - m_clip.top) * m_ratio.height;
					m_start.y = new_p.y;
					flag = true;
				}
				else {
					if (m_view.height > dy) {
						m_view.height -= dy;
					}
					else {
						m_view.height = 0.0f;
					}
					m_start.y = new_p.y;
					flag = true;
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_EAST) {
			const float dx = (new_p.x - (m_start.x + m_view.width));
			if (fabs(dx) >= FLT_MIN) {
				if (keep_aspect) {
					const float r = max(m_clip.right + dx / m_ratio.width, m_clip.left + 1.0f);	// 右辺
					m_clip.right = min(r, m_orig.width);
					m_view.width = (m_clip.right - m_clip.left) * m_ratio.width;
					m_start.x = new_p.x - m_view.width;
					flag = true;
				}
				else {
					if (new_p.x > m_start.x) {
						m_view.width = new_p.x - m_start.x;
					}
					else {
						m_view.width = 0.0f;
						m_start.x = new_p.x;
					}
					flag = true;
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_SOUTH) {
			const float dy = (new_p.y - (m_start.y + m_view.height));
			if (fabs(dy) >= FLT_MIN) {
				if (keep_aspect) {
					const float b = max(m_clip.bottom + dy / m_ratio.height, m_clip.top + 1.0f);	// 下辺
					m_clip.bottom = min(b, m_orig.height);
					m_view.height = (m_clip.bottom - m_clip.top) * m_ratio.height;
					m_start.y = new_p.y - m_view.height;
					flag = true;
				}
				else {
					if (new_p.y > m_start.y) {
						m_view.height = new_p.y - m_start.y;
					}
					else {
						m_view.height = 0.0f;
						m_start.y = new_p.y;
					}
					flag = true;
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_WEST) {
			const float dx = (new_p.x - m_start.x);
			if (fabs(dx) >= FLT_MIN) {
				if (keep_aspect) {
					const float l = min(m_clip.left + dx / m_ratio.width, m_clip.right - 1.0f);
					m_clip.left = max(l , 0.0f);
					m_view.width = (m_clip.right - m_clip.left) * m_ratio.width;
					m_start.x = new_p.x;
					flag = true;
				}
				else {
					if (m_view.width > dx) {
						m_view.width -= dx;
					}
					else {
						m_view.width = 0.0f;
					}
					m_start.x = new_p.x;
					flag = true;
				}
			}
		}
		return flag;
	}

	// 値を始点に格納する. 他の部位の位置も動く.
	bool ShapeImage::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F new_p;
		pt_round(val, PT_ROUND, new_p);
		if (!equal(m_start, new_p)) {
			m_start = new_p;
			return true;
		}
		return false;
	}

	// 図形を作成する.
	// start	左上位置
	// view	表示される大きさ
	// bmp	ビットマップ
	// opac	不透明度
	ShapeImage::ShapeImage(const D2D1_POINT_2F start, const D2D1_SIZE_F view, 
		const SoftwareBitmap& bmp, const float opac)
	{
		const uint32_t image_w = bmp.PixelWidth();
		const uint32_t image_h = bmp.PixelHeight();

		m_orig.width = image_w;
		m_orig.height = image_h;
		m_start = start;
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
			winrt::com_ptr<IMemoryBufferByteAccess> image_mem = 
				image_ref.as<IMemoryBufferByteAccess>();
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
		m_ratio(D2D1_SIZE_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
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
		dt_writer.WriteSingle(m_ratio.width);
		dt_writer.WriteSingle(m_ratio.height);
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