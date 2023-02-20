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
	static void image_get_pos_on_line(const D2D1_POINT_2F p, const D2D1_POINT_2F a, 
		const D2D1_POINT_2F b, /*--->*/D2D1_POINT_2F& q) noexcept;

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
	static void image_get_pos_on_line(const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b, /*--->*/D2D1_POINT_2F& q) noexcept
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
	template IAsyncOperation<bool> ShapeImage::copy<true>(const winrt::guid enc_id, IRandomAccessStream& stream) const;
	template IAsyncOperation<bool> ShapeImage::copy<false>(const winrt::guid enc_id, IRandomAccessStream& stream) const;

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
		const D2D1_RECT_F dest_rect{
			m_start.x,
			m_start.y,
			m_start.x + m_view.width,
			m_start.y + m_view.height
		};
		target->DrawBitmap(m_d2d_bitmap.get(), dest_rect, m_opac, D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, m_clip);

		if (m_anc_show && is_selected()) {
			const D2D1_POINT_2F v_pos[4]{
				m_start,
				{ m_start.x + m_view.width, m_start.y },
				{ m_start.x + m_view.width, m_start.y + m_view.height },
				{ m_start.x, m_start.y + m_view.height },
			};
			anc_draw_square(v_pos[0], target, brush);
			anc_draw_square(v_pos[1], target, brush);
			anc_draw_square(v_pos[2], target, brush);
			anc_draw_square(v_pos[3], target, brush);
			brush->SetColor(COLOR_WHITE);
			target->DrawRectangle(dest_rect, brush, Shape::m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawRectangle(dest_rect, brush, Shape::m_aux_width, Shape::m_aux_style.get());
		}
	}

	// 図形を囲む領域を得る.
	void ShapeImage::get_bound(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept
	{
		D2D1_POINT_2F b_pos[2]{
			m_start,
			{ m_start.x + m_view.width, m_start.y + m_view.height }
		};
		if (b_pos[0].x > b_pos[1].x) {
			const auto less_x = b_pos[1].x;
			b_pos[1].x = b_pos[0].x;
			b_pos[0].x = less_x;
		}
		if (b_pos[0].y > b_pos[1].y) {
			const auto less_y = b_pos[1].y;
			b_pos[1].y = b_pos[0].y;
			b_pos[0].y = less_y;
		}
		b_lt.x = a_lt.x < b_pos[0].x ? a_lt.x : b_pos[0].x;
		b_lt.y = a_lt.y < b_pos[0].y ? a_lt.y : b_pos[0].y;
		b_rb.x = a_rb.x > b_pos[1].x ? a_rb.x : b_pos[1].x;
		b_rb.y = a_rb.y > b_pos[1].y ? a_rb.y : b_pos[1].y;
	}

	// 画像の不透明度を得る.
	bool ShapeImage::get_image_opacity(float& val) const noexcept
	{
		val = m_opac;
		return true;
	}

	// 画素の色を得る.
	// pos	ページ座標での位置
	// val	画素の色
	// 戻り値	色を得られたなら true, そうでなければ false.
	bool ShapeImage::get_pixcel(const D2D1_POINT_2F pos, D2D1_COLOR_F& val) const noexcept
	{
		// ページ座標での位置を, 元画像での位置に変換する.
		const double fx = round(m_clip.left + (pos.x - m_start.x) * (m_clip.right - m_clip.left) / m_view.width);
		const double fy = round(m_clip.top + (pos.y - m_start.y) * (m_clip.bottom - m_clip.top) / m_view.height);
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
	void ShapeImage::get_pos_lt(D2D1_POINT_2F& val) const noexcept
	{
		const float ax = m_start.x;
		const float ay = m_start.y;
		const float bx = m_start.x + m_view.width;
		const float by = m_start.y + m_view.height;
		val.x = ax < bx ? ax : bx;
		val.y = ay < by ? ay : by;
	}

	// 近傍の頂点を見つける.
	// pos	ある位置
	// dd	近傍とみなす距離 (の二乗値), これより離れた頂点は近傍とはみなさない.
	// val	ある位置の近傍にある頂点
	// 戻り値	見つかったら true
	bool ShapeImage::get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool found = false;
		D2D1_POINT_2F v_pos[4];
		get_verts(v_pos);
		for (size_t i = 0; i < 4; i++) {
			D2D1_POINT_2F vec;
			pt_sub(pos, v_pos[i], vec);
			const float vv = static_cast<float>(pt_abs2(vec));
			if (vv < dd) {
				dd = vv;
				val = v_pos[i];
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
	size_t ShapeImage::get_verts(D2D1_POINT_2F v_pos[]) const noexcept
	{
		v_pos[0] = m_start;
		v_pos[1] = D2D1_POINT_2F{ m_start.x + m_view.width, m_start.y };
		v_pos[2] = D2D1_POINT_2F{ m_start.x + m_view.width, m_start.y + m_view.height };
		v_pos[3] = D2D1_POINT_2F{ m_start.x, m_start.y + m_view.height };
		return 4;
	}

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// 戻り値	位置を含む図形の部位. 含まないときは「図形の外側」を返す.
	uint32_t ShapeImage::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		D2D1_POINT_2F v_pos[4];
		// 0---1
		// |   |
		// 3---2
		get_verts(v_pos);
		if (pt_in_anc(t_pos, v_pos[2], m_anc_width)) {
			return ANC_TYPE::ANC_SE;
		}
		else if (pt_in_anc(t_pos, v_pos[3], m_anc_width)) {
			return ANC_TYPE::ANC_SW;
		}
		else if (pt_in_anc(t_pos, v_pos[1], m_anc_width)) {
			return ANC_TYPE::ANC_NE;
		}
		else if (pt_in_anc(t_pos, v_pos[0], m_anc_width)) {
			return ANC_TYPE::ANC_NW;
		}
		else {
			const auto e_width = m_anc_width * 0.5;
			D2D1_POINT_2F e_pos[2];
			e_pos[0].x = v_pos[0].x;
			e_pos[0].y = static_cast<FLOAT>(v_pos[0].y - e_width);
			e_pos[1].x = v_pos[1].x;
			e_pos[1].y = static_cast<FLOAT>(v_pos[1].y + e_width);
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_NORTH;
			}
			e_pos[0].x = static_cast<FLOAT>(v_pos[1].x - e_width);
			e_pos[0].y = v_pos[1].y;
			e_pos[1].x = static_cast<FLOAT>(v_pos[2].x + e_width);
			e_pos[1].y = v_pos[2].y;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_EAST;
			}
			e_pos[0].x = v_pos[3].x;
			e_pos[0].y = static_cast<FLOAT>(v_pos[3].y - e_width);
			e_pos[1].x = v_pos[2].x;
			e_pos[1].y = static_cast<FLOAT>(v_pos[2].y + e_width);
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_SOUTH;
			}
			e_pos[0].x = static_cast<FLOAT>(v_pos[0].x - e_width);
			e_pos[0].y = v_pos[0].y;
			e_pos[1].x = static_cast<FLOAT>(v_pos[3].x + e_width);
			e_pos[1].y = v_pos[3].y;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_WEST;
			}
		}
		//pt_bound(v_pos[0], v_pos[2], v_pos[0], v_pos[2]);
		if (v_pos[0].x > v_pos[2].x) {
			const float less_x = v_pos[2].x;
			v_pos[2].x = v_pos[0].x;
			v_pos[0].x = less_x;
		}
		if (v_pos[0].y > v_pos[2].y) {
			const float less_y = v_pos[2].y;
			v_pos[2].y = v_pos[0].y;
			v_pos[0].y = less_y;
		}
		if (v_pos[0].x <= t_pos.x && t_pos.x <= v_pos[2].x &&
			v_pos[0].y <= t_pos.y && t_pos.y <= v_pos[2].y) {
			return ANC_TYPE::ANC_FILL;
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// 範囲に含まれるか判定する.
	// area_lt	範囲の左上位置
	// area_rb	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeImage::in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept
	{
		// 始点と終点とが範囲に含まれるか判定する.
		return pt_in_rect(m_start, area_lt, area_rb) &&
			pt_in_rect(D2D1_POINT_2F{ m_start.x + m_view.width, m_start.y + m_view.height }, area_lt, area_rb);
	}

	// 差分だけ移動する.
	bool ShapeImage::move(const D2D1_POINT_2F val) noexcept
	{
		pt_add(m_start, val, m_start);
		return true;
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
		D2D1_POINT_2F new_pos;
		pt_round(val, PT_ROUND, new_pos);
		bool flag = false;
		if (anc == ANC_TYPE::ANC_NW) {
			// 画像の一部分でも表示されているか判定する.
			// (画像がまったく表示されてない場合はスケールの変更は行わない.)
			const float image_w = m_clip.right - m_clip.left;	// 表示されている画像の幅 (原寸)
			const float image_h = m_clip.bottom - m_clip.top;	// 表示されている画像の高さ (原寸)
			if (image_w > 1.0f && image_h > 1.0f) {
				const D2D1_POINT_2F s_pos{ m_start };	// 始点 (図形の頂点)
				D2D1_POINT_2F pos;
				if (keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x + image_w, s_pos.y + image_h };	// 終点 (始点と対角にある画像上の点)
					image_get_pos_on_line(new_pos, s_pos, e_pos, pos);
				}
				else {
					pos = new_pos;
				}
				// 値と始点との差分を求め, 差分がゼロより大きいか判定する.
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					// スケール変更後の表示の寸法を求め, その縦横が 1 ピクセル以上あるか判定する.
					const float page_w = m_view.width - v_vec.x;
					const float page_h = m_view.height - v_vec.y;
					if (page_w >= 1.0f && page_h >= 1.0f) {
						m_view.width = page_w;
						m_view.height = page_h;
						m_start = pos;
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
				const D2D1_POINT_2F s_pos{ m_start.x + m_view.width, m_start.y };
				D2D1_POINT_2F pos;
				if (keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x - image_w, s_pos.y + image_h };
					image_get_pos_on_line(new_pos, s_pos, e_pos, pos);
				}
				else {
					pos = new_pos;
				}
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					const float page_w = pos.x - m_start.x;
					const float page_h = m_start.y + m_view.height - pos.y;
					if (page_w >= 1.0f && page_h >= 1.0f) {
						m_view.width = page_w;
						m_view.height = page_h;
						m_start.y = pos.y;
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
				const D2D1_POINT_2F s_pos{ m_start.x + m_view.width, m_start.y + m_view.height };
				D2D1_POINT_2F pos;
				if (keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x - image_w, s_pos.y - image_h };
					image_get_pos_on_line(new_pos, s_pos, e_pos, pos);
				}
				else {
					pos = new_pos;
				}
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					const float page_w = pos.x - m_start.x;
					const float page_h = pos.y - m_start.y;
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
				const D2D1_POINT_2F s_pos{ m_start.x, m_start.y + m_view.height };
				D2D1_POINT_2F pos;
				if (keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x + image_w, s_pos.y - image_h };
					image_get_pos_on_line(new_pos, s_pos, e_pos, pos);
				}
				else {
					pos = new_pos;
				}
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					const float page_w = m_start.x + m_view.width - pos.x;
					const float page_h = pos.y - m_start.y;
					if (page_w >= 1.0f && page_h >= 1.0f) {
						m_view.width = page_w;
						m_view.height = page_h;
						m_start.x = pos.x;
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
			// 変更する差分を求める.
			const float dy = (new_pos.y - m_start.y);
			if (fabs(dy) >= FLT_MIN) {
				const float rect_top = min(m_clip.top + dy / m_ratio.height, m_clip.bottom - 1.0f);
				m_clip.top = max(rect_top, 0.0f);
				m_view.height = (m_clip.bottom - m_clip.top) * m_ratio.height;
				m_start.y = new_pos.y;
				if (!flag) {
					flag = true;
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_EAST) {
			const float dx = (new_pos.x - (m_start.x + m_view.width));
			if (fabs(dx) >= FLT_MIN) {
				const float rect_right = max(m_clip.right + dx / m_ratio.width, m_clip.left + 1.0f);
				m_clip.right = min(rect_right, m_orig.width);
				m_view.width = (m_clip.right - m_clip.left) * m_ratio.width;
				m_start.x = new_pos.x - m_view.width;
				if (!flag) {
					flag = true;
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_SOUTH) {
			const float dy = (new_pos.y - (m_start.y + m_view.height));
			if (fabs(dy) >= FLT_MIN) {
				const float rect_bottom = max(m_clip.bottom + dy / m_ratio.height, m_clip.top + 1.0f);
				m_clip.bottom = min(rect_bottom, m_orig.height);
				m_view.height = (m_clip.bottom - m_clip.top) * m_ratio.height;
				m_start.y = new_pos.y - m_view.height;
				if (!flag) {
					flag = true;
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_WEST) {
			const float dx = (new_pos.x - m_start.x);
			if (fabs(dx) >= FLT_MIN) {
				const float rect_left = min(m_clip.left + dx / m_ratio.width, m_clip.right - 1.0f);
				m_clip.left = max(rect_left, 0.0f);
				m_view.width = (m_clip.right - m_clip.left) * m_ratio.width;
				m_start.x = new_pos.x;
				if (!flag) {
					flag = true;
				}
			}
		}
		return flag;
	}

	// 値を始点に格納する. 他の部位の位置も動く.
	bool ShapeImage::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F new_pos;
		pt_round(val, PT_ROUND, new_pos);
		if (!equal(m_start, new_pos)) {
			m_start = new_pos;
			return true;
		}
		return false;
	}

	// 図形を作成する.
	// pos	左上位置
	// view	表示される大きさ
	// bmp	ビットマップ
	// opac	不透明度
	ShapeImage::ShapeImage(const D2D1_POINT_2F pos, const D2D1_SIZE_F view, 
		const SoftwareBitmap& bmp, const float opac)
	{
		const uint32_t image_w = bmp.PixelWidth();
		const uint32_t image_h = bmp.PixelHeight();

		m_orig.width = image_w;
		m_orig.height = image_h;
		m_start = pos;
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