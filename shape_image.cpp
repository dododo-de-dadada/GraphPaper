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
	static void image_get_pos_on_line(const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b, /*--->*/D2D1_POINT_2F& q) noexcept;

	winrt::com_ptr<IWICImagingFactory2> ShapeImage::wic_factory{	// WIC ファクトリ
		[]() {
			winrt::com_ptr<IWICImagingFactory2> factory;
			winrt::check_hresult(
				CoCreateInstance(
					CLSID_WICImagingFactory,
					nullptr,
					CLSCTX_INPROC_SERVER,
					IID_PPV_ARGS(&factory)
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
	// enc_id	画像の形式
	// stream	画像を格納するストリーム
	// 戻り値	格納できたら true
	template <bool C>
	IAsyncOperation<bool> ShapeImage::copy(const winrt::guid enc_id,
		/*--->*/IRandomAccessStream& stream) const
	{
		// クリップボードに格納するなら, 元データのまま.
		// 描画するときに DrawBitmap でクリッピングされる.
		// エクスポートするなら, クリッピングしたデータ.
		// SVG には, 画像をクリッピングして表示する機能がないか, あっても煩雑.
		// ひょっとしたら, クリッピング矩形の小数点のせいで微妙に異なる画像になるかもしれない.
		// PDF には, 3 バイト RGB を提供できないので, 使えない.
		bool ret = false;	// 返値
		const int32_t clip_l = [=]() {	// クリッピング方形の左の値
			if constexpr (C) {
				return static_cast<int32_t>(std::floorf(m_clip.left)); 
			}
			else {
				return 0;
			}
		}();
		const int32_t clip_t = [=]() {	// クリッピング方形の上の値
			if constexpr (C) { 
				return static_cast<int32_t>(std::floorf(m_clip.top)); 
			}
			else {
				return 0;
			}
		}(); 
		const int32_t clip_r = [=]() {	// クリッピング方形の右の値
			if constexpr (C) {
				return static_cast<int32_t>(std::ceilf(m_clip.right));
			}
			else {
				return m_orig.width;
			}
		}(); 
		const int32_t clip_b = [=]() {	// クリッピング方形の下の値
			if constexpr (C) {
				return static_cast<int32_t>(std::ceilf(m_clip.bottom));
			}
			else {
				return m_orig.height;
			}
		}();
		if (clip_l >= 0 && clip_t >= 0 && clip_r > clip_l && clip_b > clip_t) {
			// バックグラウンドに切替かえて実行する.
			// UI スレッドのままでは, SoftwareBitmap が解放されるときエラーが起きる.
			winrt::apartment_context context;
			co_await winrt::resume_background();

			// SoftwareBitmap を作成する.
			// Windows では, SoftwareBitmap にしろ WIC にしろ
			// 3 バイトの RGB はサポートされてない.
			const int32_t clip_w = clip_r - clip_l;
			const int32_t clip_h = clip_b - clip_t;
			SoftwareBitmap soft_bmp{
				SoftwareBitmap(BitmapPixelFormat::Bgra8, clip_w, clip_h, BitmapAlphaMode::Straight)
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
					const size_t src_pitch = 4ull * m_orig.width;
					const size_t dst_pitch = 4ull * clip_w;
					size_t i = 0;
					for (size_t y = clip_t; y < clip_b; y++) {
						memcpy(soft_bmp_ptr + dst_pitch * y, m_data + src_pitch * y + clip_l, dst_pitch);
					}
				}
				else {
					memcpy(soft_bmp_ptr, m_data, capacity);
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
	template IAsyncOperation<bool> ShapeImage::copy<true>(const winrt::guid enc_id, IRandomAccessStream& ra_stream) const;
	template IAsyncOperation<bool> ShapeImage::copy<false>(const winrt::guid enc_id, IRandomAccessStream& ra_stream) const;

	// 図形を表示する.
	void ShapeImage::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::s_target;
		ID2D1SolidColorBrush* const brush = Shape::s_color_brush;

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
			target->CreateBitmap(m_orig, static_cast<void*>(m_data), pitch, b_prop, bitmap.put());
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

		if (is_selected()) {
			brush->SetColor(Shape::s_background_color);
			target->DrawRectangle(dest_rect, brush, 1.0f, nullptr);
			brush->SetColor(Shape::s_foreground_color);
			target->DrawRectangle(dest_rect, brush, 1.0f, Shape::m_aux_style.get());

			const D2D1_POINT_2F v_pos[4]{
				m_start,
				{ m_start.x + m_view.width, m_start.y },
				{ m_start.x + m_view.width, m_start.y + m_view.height },
				{ m_start.x, m_start.y + m_view.height },
			};

			anc_draw_rect(v_pos[0], target, brush);
			anc_draw_rect(v_pos[1], target, brush);
			anc_draw_rect(v_pos[2], target, brush);
			anc_draw_rect(v_pos[3], target, brush);
		}
	}

	// 図形を囲む領域を得る.
	void ShapeImage::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
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
		b_min.x = a_min.x < b_pos[0].x ? a_min.x : b_pos[0].x;
		b_min.y = a_min.y < b_pos[0].y ? a_min.y : b_pos[0].y;
		b_max.x = a_max.x > b_pos[1].x ? a_max.x : b_pos[1].x;
		b_max.y = a_max.y > b_pos[1].y ? a_max.y : b_pos[1].y;
	}

	// 画像の不透明度を得る.
	bool ShapeImage::get_image_opacity(float& val) const noexcept
	{
		val = m_opac;
		return true;
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
	void ShapeImage::get_pos_min(D2D1_POINT_2F& val) const noexcept
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
		get_verts(v_pos);
		if (pt_in_anc(t_pos, v_pos[0])) {
			return ANC_TYPE::ANC_NW;
		}
		else if (pt_in_anc(t_pos, v_pos[1])) {
			return ANC_TYPE::ANC_NE;
		}
		else if (pt_in_anc(t_pos, v_pos[2])) {
			return ANC_TYPE::ANC_SE;
		}
		else if (pt_in_anc(t_pos, v_pos[3])) {
			return ANC_TYPE::ANC_SW;
		}
		else {
			const auto e_width = Shape::s_anc_len * 0.5f;
			D2D1_POINT_2F e_pos[2];
			e_pos[0].x = v_pos[0].x;
			e_pos[0].y = v_pos[0].y - e_width;
			e_pos[1].x = v_pos[1].x;
			e_pos[1].y = v_pos[1].y + e_width;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_NORTH;
			}
			e_pos[0].x = v_pos[1].x - e_width;
			e_pos[0].y = v_pos[1].y;
			e_pos[1].x = v_pos[2].x + e_width;
			e_pos[1].y = v_pos[2].y;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_EAST;
			}
			e_pos[0].x = v_pos[3].x;
			e_pos[0].y = v_pos[3].y - e_width;
			e_pos[1].x = v_pos[2].x;
			e_pos[1].y = v_pos[2].y + e_width;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_SOUTH;
			}
			e_pos[0].x = v_pos[0].x - e_width;
			e_pos[0].y = v_pos[0].y;
			e_pos[1].x = v_pos[3].x + e_width;
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
	// area_min	範囲の左上位置
	// area_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeImage::in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept
	{
		// 始点と終点とが範囲に含まれるか判定する.
		return pt_in_rect(m_start, area_min, area_max) &&
			pt_in_rect(D2D1_POINT_2F{ m_start.x + m_view.width, m_start.y + m_view.height }, area_min, area_max);
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
	// keep_aspect	画像図形の縦横比を維持
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
	// page_size	表示される大きさ
	// bmp	ビットマップ
	// opac	不透明度
	ShapeImage::ShapeImage(const D2D1_POINT_2F pos, const D2D1_SIZE_F page_size, const SoftwareBitmap& bmp, const float opac) //:
		//ShapeSelect()
	{
		const uint32_t image_w = bmp.PixelWidth();
		const uint32_t image_h = bmp.PixelHeight();

		m_orig.width = image_w;
		m_orig.height = image_h;
		m_start = pos;
		m_view = page_size;
		m_clip.left = m_clip.top = 0;
		m_clip.right = static_cast<FLOAT>(image_w);
		m_clip.bottom = static_cast<FLOAT>(image_h);
		m_opac = opac < 0.0f ? 0.0f : (opac > 1.0f ? 1.0f : opac);
		const size_t data_size = 4ull * image_w * image_h;
		if (data_size > 0) {
			m_data = new uint8_t[data_size];
			// SoftwareBitmap のバッファをロックする.
			auto image_buf{ bmp.LockBuffer(BitmapBufferAccessMode::ReadWrite) };
			auto image_ref{ image_buf.CreateReference() };
			winrt::com_ptr<IMemoryBufferByteAccess> image_mem = image_ref.as<IMemoryBufferByteAccess>();
			BYTE* image_data = nullptr;
			UINT32 capacity = 0;
			if (SUCCEEDED(image_mem->GetBuffer(&image_data, &capacity))) {
				// ロックしたバッファに画像データをコピーする.
				memcpy(m_data, image_data, capacity);
				// ロックしたバッファを解放する.
				image_buf.Close();
				image_buf = nullptr;
				//image_mem->Release();
				image_mem = nullptr;
			}
		}
		else {
			m_data = nullptr;
		}
	}

	// 図形をデータリーダーから読み込む
	// dt_reader	データリーダー
	ShapeImage::ShapeImage(DataReader const& dt_reader) :
		ShapeSelect(dt_reader),
		m_start(D2D1_POINT_2F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_view(D2D1_SIZE_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_clip(D2D1_RECT_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_orig(D2D1_SIZE_U{ dt_reader.ReadUInt32(), dt_reader.ReadUInt32() }),
		m_ratio(D2D1_SIZE_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_opac(dt_reader.ReadSingle())
	{
		if (m_opac < 0.0f || m_opac > 1.0f) {
			m_opac = 1.0f;
		}
		const size_t pitch = 4ull * m_orig.width;
		m_data = new uint8_t[pitch * m_orig.height];
		std::vector<uint8_t> line_buf(pitch);
		const size_t height = m_orig.height;
		for (size_t i = 0; i < height; i++) {
			dt_reader.ReadBytes(line_buf);
			memcpy(m_data + pitch * i, line_buf.data(), pitch);
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
			memcpy(line_buf.data(), m_data + pitch * i, pitch);
			dt_writer.WriteBytes(line_buf);
		}
	}

}