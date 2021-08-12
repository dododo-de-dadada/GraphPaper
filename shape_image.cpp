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

#define DIB_SET(buf, i, pal, b)\
	{\
		const auto pal_i = (b);\
		(buf)[i + 0] = (pal)[pal_i][0];\
		(buf)[i + 1] = (pal)[pal_i][1];\
		(buf)[i + 2] = (pal)[pal_i][2];\
		(buf)[i + 3] = (pal)[pal_i][3];\
	}
#define NIBBLE_HI(b)	((b >> 4) & 0x0f)
#define NIBBLE_LO(b)	(b & 0x0f)

	// D2D1_RECT_F から D2D1_RECT_U を作成する.
	static const D2D1_RECT_U image_conv_rect(const D2D1_RECT_F& f, const D2D1_SIZE_U s);
	// 点に最も近い, 線分上の点を求める.
	static void image_get_pos_on_line(const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& q);

	// D2D1_RECT_F から D2D1_RECT_U を作成する.
	// 左上位置は { 0, 0 } 以上, 右下位置は D2D1_SIZE_U 以下.
	static const D2D1_RECT_U image_conv_rect(const D2D1_RECT_F& f, const D2D1_SIZE_U s)
	{
		return D2D1_RECT_U{
			max(static_cast<uint32_t>(floor(f.left)), 0),
			max(static_cast<uint32_t>(floor(f.top)), 0),
			min(static_cast<uint32_t>(ceil(f.right)), s.width),
			min(static_cast<uint32_t>(ceil(f.bottom)), s.height)
		};
	}

	// 点に最も近い, 線分上の点を求める.
	static void image_get_pos_on_line(const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& q)
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
			const D2D1_POINT_2F ap{ p.x - a.x, p.y - a.y };	// p - a -> ap
			const D2D1_POINT_2F ab{ b.x - a.x, b.y - a.y };	// b - a -> ab
			const double ap_ab = ap.x * ab.x + ap.y * ab.y;	// ap.ab
			const double ab_ab = ab.x * ab.x + ab.y * ab.y;	// ab.ab
			pt_mul(ab, ap_ab / ab_ab, a, q);	// a + ap.ab / (|ab|^2) -> q
		}
	}

	// 図形を破棄する.
	ShapeImage::~ShapeImage(void)
	{
		if (m_data != nullptr) {
			delete m_data;
			m_data = nullptr;
		}
		if (m_dx_bitmap != nullptr) {
			m_dx_bitmap = nullptr;
		}
	}

	// ストリームに格納する.
	// enc_id	画像の形式 (BitmapEncoder に定義されている)
	// ra_stream	画像を格納するストリーム
	IAsyncAction ShapeImage::copy_to(const winrt::guid enc_id, IRandomAccessStream& ra_stream)
	{
		// SoftwareBitmap を作成する.
		const uint32_t bmp_w = m_size.width;
		const uint32_t bmp_h = m_size.height;
		SoftwareBitmap bmp{ SoftwareBitmap(BitmapPixelFormat::Bgra8, bmp_w, bmp_h, BitmapAlphaMode::Straight) };

		// ビットマップのバッファをロックする.
		auto bmp_buf{ bmp.LockBuffer(BitmapBufferAccessMode::ReadWrite) };
		auto bmp_ref{ bmp_buf.CreateReference() };
		winrt::com_ptr<IMemoryBufferByteAccess> bmp_mem = bmp_ref.as<IMemoryBufferByteAccess>();

		// ロックされたバッファーを得る.
		BYTE* bmp_data = nullptr;
		UINT32 capacity = 0;
		if (SUCCEEDED(bmp_mem->GetBuffer(&bmp_data, &capacity)))
		{
			// バッファに画像データをコピーする.
			memcpy(bmp_data, m_data, capacity);

			// バッファを解放する.
			bmp_buf.Close();
			bmp_buf = nullptr;
			bmp_mem->Release();
			bmp_mem = nullptr;

			// ビットマップエンコーダーにビットマップを格納する.
			BitmapEncoder bmp_enc{ co_await BitmapEncoder::CreateAsync(enc_id, ra_stream) };
			bmp_enc.IsThumbnailGenerated(true);
			bmp_enc.SetSoftwareBitmap(bmp);
			try {
				co_await bmp_enc.FlushAsync();
			}
			catch (winrt::hresult_error& err) {
				if (err.code() == WINCODEC_ERR_UNSUPPORTEDOPERATION) {
					bmp_enc.IsThumbnailGenerated(false);
				}
			}
			if (!bmp_enc.IsThumbnailGenerated()) {
				co_await bmp_enc.FlushAsync();
			}
			bmp_enc = nullptr;
		}
		bmp.Close();
		bmp = nullptr;
	}

	// 図形を表示する.
	void ShapeImage::draw(SHAPE_DX& dx)
	{
		if (m_dx_bitmap == nullptr) {
			const D2D1_BITMAP_PROPERTIES1 b_prop{
				D2D1::BitmapProperties1(
					D2D1_BITMAP_OPTIONS_NONE,
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
				)
			};
			dx.m_d2dContext->CreateBitmap(m_size, static_cast<void*>(m_data), 4 * m_size.width, b_prop, m_dx_bitmap.put());
			if (m_dx_bitmap == nullptr) {
				return;
			}
		}
		D2D1_RECT_F dest_rect{
			m_pos.x,
			m_pos.y,
			m_pos.x + m_view.width,
			m_pos.y + m_view.height
		};
		dx.m_d2dContext->DrawBitmap(m_dx_bitmap.get(), dest_rect, m_opac, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, m_rect);

		if (is_selected()) {
			dx.m_shape_brush->SetColor(Shape::m_default_background);
			dx.m_d2dContext->DrawRectangle(dest_rect, dx.m_shape_brush.get(), 1.0f, nullptr);
			dx.m_shape_brush->SetColor(Shape::m_default_foreground);
			dx.m_d2dContext->DrawRectangle(dest_rect, dx.m_shape_brush.get(), 1.0f, dx.m_aux_style.get());

			D2D1_POINT_2F v_pos[4]{
				m_pos,
				{ m_pos.x + m_view.width, m_pos.y },
				{ m_pos.x + m_view.width, m_pos.y + m_view.height },
				{ m_pos.x, m_pos.y + m_view.height },
			};

			anch_draw_rect(v_pos[0], dx);
			anch_draw_rect(v_pos[1], dx);
			anch_draw_rect(v_pos[2], dx);
			anch_draw_rect(v_pos[3], dx);
		}
	}

	// 図形を囲む領域を得る.
	void ShapeImage::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		D2D1_POINT_2F b_pos[2]{
			m_pos,
			{ m_pos.x + m_view.width, m_pos.y + m_view.height }
		};
		pt_bound(b_pos[0], b_pos[1], b_pos[0], b_pos[1]);
		pt_min(a_min, b_pos[0], b_min);
		pt_max(a_max, b_pos[1], b_max);
	}

	// 画像の不透明度を得る.
	bool ShapeImage::get_image_opacity(float& value) const noexcept
	{
		value = m_opac;
		return true;
	}

	// 部位の位置を得る.
	void ShapeImage::get_pos_anch(const uint32_t anch, D2D1_POINT_2F& value) const noexcept
	{
		if (anch == ANCH_TYPE::ANCH_NW) {
			value = m_pos;
		}
		else if (anch == ANCH_TYPE::ANCH_NORTH) {
			value.x = m_pos.x + m_view.width * 0.5f;
			value.y = m_pos.y;
		}
		else if (anch == ANCH_TYPE::ANCH_NE) {
			value.x = m_pos.x + m_view.width;
			value.y = m_pos.y;
		}
		else if (anch == ANCH_TYPE::ANCH_EAST) {
			value.x = m_pos.x + m_view.width;
			value.y = m_pos.y + m_view.height * 0.5f;
		}
		else if (anch == ANCH_TYPE::ANCH_SE) {
			value.x = m_pos.x + m_view.width;
			value.y = m_pos.y + m_view.height;
		}
		else if (anch == ANCH_TYPE::ANCH_SOUTH) {
			value.x = m_pos.x + m_view.width * 0.5f;
			value.y = m_pos.y + m_view.height;
		}
		else if (anch == ANCH_TYPE::ANCH_SW) {
			value.x = m_pos.x;
			value.y = m_pos.y + m_view.height;
		}
		else if (anch == ANCH_TYPE::ANCH_WEST) {
			value.x = m_pos.x;
			value.y = m_pos.y + m_view.height * 0.5f;
		}
	}

	// 図形を囲む領域の左上位置を得る.
	void ShapeImage::get_pos_min(D2D1_POINT_2F& value) const noexcept
	{
		const D2D1_POINT_2F v_pos[2]{
			m_pos,
			{ m_pos.x + m_view.width, m_pos.y + m_view.height }
		};
		pt_min(v_pos[0], v_pos[1], value);
	}

	// 近傍の頂点を得る.
	bool ShapeImage::get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& value) const noexcept
	{
		bool flag = false;
		D2D1_POINT_2F v_pos[4];
		get_verts(v_pos);
		for (size_t i = 0; i < 4; i++) {
			D2D1_POINT_2F v_vec;
			pt_sub(pos, v_pos[i], v_vec);
			const auto v_dd = static_cast<float>(pt_abs2(v_vec));
			if (v_dd < dd) {
				dd = v_dd;
				value = v_pos[i];
				flag = true;
			}
		}
		return flag;
	}

	// 開始位置を得る.
	bool ShapeImage::get_pos_start(D2D1_POINT_2F& value) const noexcept
	{
		value = m_pos;
		return true;
	}

	// 頂点を得る.
	size_t ShapeImage::get_verts(D2D1_POINT_2F v_pos[]) const noexcept
	{
		v_pos[0] = m_pos;
		v_pos[1] = D2D1_POINT_2F{ m_pos.x + m_view.width, m_pos.y };
		v_pos[2] = D2D1_POINT_2F{ m_pos.x + m_view.width, m_pos.y + m_view.height };
		v_pos[3] = D2D1_POINT_2F{ m_pos.x, m_pos.y + m_view.height };
		return 4;
	}

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// 戻り値	位置を含む図形の部位. 含まないときは「図形の外側」を返す.
	uint32_t ShapeImage::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		D2D1_POINT_2F v_pos[4];
		get_verts(v_pos);
		if (pt_in_anch(t_pos, v_pos[0])) {
			return ANCH_TYPE::ANCH_NW;
		}
		else if (pt_in_anch(t_pos, v_pos[1])) {
			return ANCH_TYPE::ANCH_NE;
		}
		else if (pt_in_anch(t_pos, v_pos[2])) {
			return ANCH_TYPE::ANCH_SE;
		}
		else if (pt_in_anch(t_pos, v_pos[3])) {
			return ANCH_TYPE::ANCH_SW;
		}
		else {
			const auto e_width = Shape::s_anch_len * 0.5f;
			D2D1_POINT_2F e_pos[2];
			e_pos[0].x = v_pos[0].x;
			e_pos[0].y = v_pos[0].y - e_width;
			e_pos[1].x = v_pos[1].x;
			e_pos[1].y = v_pos[1].y + e_width;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANCH_TYPE::ANCH_NORTH;
			}
			e_pos[0].x = v_pos[1].x - e_width;
			e_pos[0].y = v_pos[1].y;
			e_pos[1].x = v_pos[2].x + e_width;
			e_pos[1].y = v_pos[2].y;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANCH_TYPE::ANCH_EAST;
			}
			e_pos[0].x = v_pos[3].x;
			e_pos[0].y = v_pos[3].y - e_width;
			e_pos[1].x = v_pos[2].x;
			e_pos[1].y = v_pos[2].y + e_width;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANCH_TYPE::ANCH_SOUTH;
			}
			e_pos[0].x = v_pos[0].x - e_width;
			e_pos[0].y = v_pos[0].y;
			e_pos[1].x = v_pos[3].x + e_width;
			e_pos[1].y = v_pos[3].y;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANCH_TYPE::ANCH_WEST;
			}
		}
		pt_bound(v_pos[0], v_pos[2], v_pos[0], v_pos[2]);
		if (v_pos[0].x <= t_pos.x && t_pos.x <= v_pos[2].x &&
			v_pos[0].y <= t_pos.y && t_pos.y <= v_pos[2].y) {
			return ANCH_TYPE::ANCH_FILL;
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// 範囲に含まれるか判定する.
	bool ShapeImage::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		return pt_in_rect(m_pos, a_min, a_max) &&
			pt_in_rect(D2D1_POINT_2F{ m_pos.x + m_view.width, m_pos.y + m_view.height }, a_min, a_max);
	}

	// 差分だけ移動する.
	bool ShapeImage::move(const D2D1_POINT_2F value)
	{
		pt_add(m_pos, value, m_pos);
		return true;
	}

	// 元の大きさに戻す.
	void ShapeImage::resize_origin(void) noexcept
	{
		const float size_w = static_cast<float>(m_size.width);
		const float size_h = static_cast<float>(m_size.height);
		m_view.width = size_w;
		m_view.height = size_h;
		m_rect.left = m_rect.top = 0.0f;
		m_rect.right = size_w;
		m_rect.bottom = size_h;
		m_ratio.width = m_ratio.height = 1.0f;
	}

	// 値を消去されたか判定に格納する.
	bool ShapeImage::set_delete(const bool value) noexcept
	{
		m_is_deleted = value;
		return true;
	}

	// 値を画像の不透明度に格納する.
	bool ShapeImage::set_image_opacity(const float value) noexcept
	{
		if (!equal(m_opac, value)) {
			m_opac = value;
			return true;
		}
		return false;
	}

	// 値を, 部位の位置に格納する.
	// value	値
	// anch	図形の部位
	// limit	限界距離 (他の頂点との距離がこの値未満になるなら, その頂点に位置に合わせる)
	// keep_aspect	画像図形の縦横比を維持
	bool ShapeImage::set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float limit, const bool keep_aspect)
	{
		bool flag = false;
		if (anch == ANCH_TYPE::ANCH_NW) {
			// 画像の一部分でも表示されているか判定する.
			// (画像がまったく表示されてない場合はスケールの変更は行わない.)
			const float bm_w = m_rect.right - m_rect.left;	// 表示されている画像の幅 (原寸)
			const float bm_h = m_rect.bottom - m_rect.top;	// 表示されている画像の高さ (原寸)
			if (bm_w > 1.0f && bm_h > 1.0f) {
				const D2D1_POINT_2F s_pos{ m_pos };	// 始点 (図形の頂点)
				D2D1_POINT_2F pos;
				if (keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x + bm_w, s_pos.y + bm_h };	// 終点 (始点と対角にある画像上の点)
					image_get_pos_on_line(value, s_pos, e_pos, pos);
				}
				else {
					pos = value;
				}
				// 値と始点との差分を求め, 差分がゼロより大きいか判定する.
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					// スケール変更後の表示の寸法を求め, その縦横が 1 ピクセル以上あるか判定する.
					const float view_w = m_view.width - v_vec.x;
					const float view_h = m_view.height - v_vec.y;
					if (view_w >= 1.0f && view_h >= 1.0f) {
						m_view.width = view_w;
						m_view.height = view_h;
						m_pos = pos;
						m_ratio.width = view_w / bm_w;
						m_ratio.height = view_h / bm_h;
						flag = true;
					}
				}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_NORTH) {
			// 変更する差分を求める.
			const float dy = (value.y - m_pos.y);
			if (fabs(dy) >= FLT_MIN) {
				const float rect_top = min(m_rect.top + dy / m_ratio.height, m_rect.bottom - 1.0f);
				m_rect.top = max(rect_top, 0.0f);
				m_view.height = (m_rect.bottom - m_rect.top) * m_ratio.height;
				m_pos.y = value.y;
				flag = true;
			}
		}
		else if (anch == ANCH_TYPE::ANCH_NE) {
			const float bm_w = m_rect.right - m_rect.left;
			const float bm_h = m_rect.bottom - m_rect.top;
			if (bm_w > 1.0f && bm_h > 1.0f) {
				const D2D1_POINT_2F s_pos{ m_pos.x + m_view.width, m_pos.y };
				D2D1_POINT_2F pos;
				if (keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x - bm_w, s_pos.y + bm_h };
					image_get_pos_on_line(value, s_pos, e_pos, pos);
				}
				else {
					pos = value;
				}
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					const float view_w = pos.x - m_pos.x;
					const float view_h = m_pos.y + m_view.height - pos.y;
					if (view_w >= 1.0f && view_h >= 1.0f) {
						m_view.width = view_w;
						m_view.height = view_h;
						m_pos.y = pos.y;
						m_ratio.width = view_w / bm_w;
						m_ratio.height = view_h / bm_h;
						flag = true;
					}
				}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_EAST) {
			const float dx = (value.x - (m_pos.x + m_view.width));
			if (fabs(dx) >= FLT_MIN) {
				const float rect_right = max(m_rect.right + dx / m_ratio.width, m_rect.left + 1.0f);
				m_rect.right = min(rect_right, m_size.width);
				//m_view.width = m_view.width + dx;
				m_view.width = (m_rect.right - m_rect.left) * m_ratio.width;
				m_pos.x = value.x - m_view.width;
				flag = true;
				if (fabs(m_rect.bottom - m_rect.top) < 1.0 || fabs(m_rect.right - m_rect.left) < 1.0) {
					auto debug = 1;
				}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_SE) {
			const float bm_w = m_rect.right - m_rect.left;
			const float bm_h = m_rect.bottom - m_rect.top;
			if (bm_w > 1.0f && bm_h > 1.0f) {
				const D2D1_POINT_2F s_pos{ m_pos.x + m_view.width, m_pos.y + m_view.height };
				D2D1_POINT_2F pos;
				if (keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x - bm_w, s_pos.y - bm_h };
					image_get_pos_on_line(value, s_pos, e_pos, pos);
				}
				else {
					pos = value;
				}
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					const float view_w = pos.x - m_pos.x;
					const float view_h = pos.y - m_pos.y;
					if (view_w >= 1.0f && view_h >= 1.0f) {
						m_view.width = view_w;
						m_view.height = view_h;
						m_ratio.width = view_w / bm_w;
						m_ratio.height = view_h / bm_h;
						flag = true;
					}
				}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_SOUTH) {
			const float dy = (value.y - (m_pos.y + m_view.height));
			if (fabs(dy) >= FLT_MIN) {
				const float rect_bottom = max(m_rect.bottom + dy / m_ratio.height, m_rect.top + 1.0f);
				m_rect.bottom = min(rect_bottom, m_size.height);
				m_view.height = (m_rect.bottom - m_rect.top) * m_ratio.height;
				m_pos.y = value.y - m_view.height;
				flag = true;
				if (fabs(m_rect.bottom - m_rect.top) < 1.0 || fabs(m_rect.right - m_rect.left) < 1.0) {
					auto debug = 1;
				}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_SW) {
			const float bm_w = m_rect.right - m_rect.left;
			const float bm_h = m_rect.bottom - m_rect.top;
			if (bm_w > 1.0f && bm_h > 1.0f) {
				const D2D1_POINT_2F s_pos{ m_pos.x, m_pos.y + m_view.height };
				D2D1_POINT_2F pos;
				if (keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x + bm_w, s_pos.y - bm_h };
					image_get_pos_on_line(value, s_pos, e_pos, pos);
				}
				else {
					pos = value;
				}
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					const float view_w = m_pos.x + m_view.width - pos.x;
					const float view_h = pos.y - m_pos.y;
					if (view_w >= 1.0f && view_h >= 1.0f) {
						m_view.width = view_w;
						m_view.height = view_h;
						m_pos.x = pos.x;
						m_ratio.width = view_w / bm_w;
						m_ratio.height = view_h / bm_h;
						flag = true;
					}
				}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_WEST) {
			const float dx = (value.x - m_pos.x);
			if (fabs(dx) >= FLT_MIN) {
				const float r_left = min(m_rect.left + dx / m_ratio.width, m_rect.right - 1.0f);
				m_rect.left = max(r_left, 0.0f);
				m_view.width = (m_rect.right - m_rect.left) * m_ratio.width;
				m_pos.x = value.x;
				flag = true;
				if (fabs(m_rect.bottom - m_rect.top) < 1.0 || fabs(m_rect.right - m_rect.left) < 1.0) {
					auto debug = 1;
				}
			}
		}
		return flag;
	}

	// 値を始点に格納する. 他の部位の位置も動く.
	bool ShapeImage::set_pos_start(const D2D1_POINT_2F value)
	{
		m_pos = value;
		return true;
	}

	// 値を選択されてるか判定に格納する.
	bool ShapeImage::set_select(const bool value) noexcept
	{
		m_is_selected = value;
		return true;
	}

	// 図形を作成する.
	// pos	左上位置
	// view_size	表示される大きさ
	// bmp	ビットマップ
	ShapeImage::ShapeImage(const D2D1_POINT_2F pos, const D2D1_SIZE_F view_size, const SoftwareBitmap& bmp)
	{
		m_size.width = bmp.PixelWidth();
		m_size.height = bmp.PixelHeight();
		m_pos = pos;
		m_view = view_size;
		m_rect.left = m_rect.top = 0;
		m_rect.right = static_cast<FLOAT>(bmp.PixelWidth());
		m_rect.bottom = static_cast<FLOAT>(bmp.PixelHeight());
		m_data = new uint8_t[4ull * bmp.PixelWidth() * bmp.PixelHeight()];

		// SoftwareBitmap のバッファをロックする.
		auto bmp_buf{ bmp.LockBuffer(BitmapBufferAccessMode::ReadWrite) };
		auto bmp_ref{ bmp_buf.CreateReference() };
		winrt::com_ptr<IMemoryBufferByteAccess> bmp_mem = bmp_ref.as<IMemoryBufferByteAccess>();
		BYTE* bmp_data = nullptr;
		UINT32 capacity = 0;
		if (SUCCEEDED(bmp_mem->GetBuffer(&bmp_data, &capacity)))
		{
			// ロックしたバッファに画像データをコピーする.
			memcpy(m_data, bmp_data, capacity);
			// ロックしたバッファをkaiする.
			bmp_buf.Close();
			bmp_buf = nullptr;
			bmp_mem->Release();
			bmp_mem = nullptr;
		}
	}

	// データリーダーから読み込む
	// dt_reader	データリーダー
	ShapeImage::ShapeImage(DataReader const& dt_reader)
	{
		m_is_deleted = dt_reader.ReadBoolean();
		m_is_selected = dt_reader.ReadBoolean();
		dt_read(m_pos, dt_reader);
		dt_read(m_view, dt_reader);
		dt_read(m_rect, dt_reader);
		dt_read(m_size, dt_reader);
		dt_read(m_ratio, dt_reader);

		const size_t row_size = 4ull * m_size.width;
		m_data = new uint8_t[row_size * m_size.height];
		std::vector<uint8_t> buf(row_size);
		for (size_t i = 0; i < m_size.height; i++) {
			dt_reader.ReadBytes(buf);
			memcpy(m_data + row_size * i, buf.data(), row_size);
		}
	}

	// データライターに書き込む.
	// dt_writer	データライター
	void ShapeImage::write(DataWriter const& dt_writer) const
	{
		dt_writer.WriteBoolean(m_is_deleted);
		dt_writer.WriteBoolean(m_is_selected);
		dt_write(m_pos, dt_writer);
		dt_write(m_view, dt_writer);
		dt_write(m_rect, dt_writer);
		dt_write(m_size, dt_writer);
		dt_write(m_ratio, dt_writer);

		const size_t row_size = 4 * m_size.width;
		std::vector<uint8_t> buf(row_size);
		for (size_t i = 0; i < m_size.height; i++) {
			memcpy(buf.data(), m_data + row_size * i, row_size);
			dt_writer.WriteBytes(buf);
		}
	}

	// データライターに SVG として書き込む.
	// file_name	画像ファイル名
	// dt_write		データライター
	void ShapeImage::write_svg(const wchar_t file_name[], DataWriter const& dt_writer) const
	{
		dt_write_svg("<image href=\"", dt_writer);
		dt_write_svg(file_name, wchar_len(file_name), dt_writer);
		dt_write_svg("\" ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_view.width, "width", dt_writer);
		dt_write_svg(m_view.height, "height", dt_writer);
		dt_write_svg(m_opac, "opacity", dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
	}

	// データライターに SVG として書き込む.
	// 画像なしの場合.
	// dt_write		データライター
	void ShapeImage::write_svg(DataWriter const& dt_writer) const
	{
		constexpr char RECT[] =
			"<rect x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" "
			"stroke=\"black\" fill=\"white\" />" SVG_NEW_LINE;
		constexpr char TEXT[] =
			"<text x=\"50%\" y=\"50%\" text-anchor=\"middle\" dominant-baseline=\"central\">"
			"NO IMAGE</text>" SVG_NEW_LINE;
		dt_write_svg("<!--" SVG_NEW_LINE, dt_writer);
		write_svg(nullptr, dt_writer);
		dt_write_svg("-->" SVG_NEW_LINE, dt_writer);
		dt_write_svg("<svg ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_view.width, "width", dt_writer);
		dt_write_svg(m_view.height, "height", dt_writer);
		dt_write_svg("viewBox=\"0 0 ", dt_writer);
		dt_write_svg(m_view.width, dt_writer);
		dt_write_svg(m_view.height, dt_writer);
		dt_write_svg("\">" SVG_NEW_LINE, dt_writer);
		dt_write_svg(RECT, dt_writer);
		dt_write_svg(TEXT, dt_writer);
		dt_write_svg("</svg>" SVG_NEW_LINE, dt_writer);
	}

}