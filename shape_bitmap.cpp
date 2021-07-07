#include <time.h>
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
#define NIBBLE_HI(b)	((b >> 4) & 0x0f)
#define NIBBLE_LO(b)	(b & 0x0f)
#define DIB_SET(buf, i, pal, b)\
	{\
		const auto pal_i = (b);\
		(buf)[i + 0] = (pal)[pal_i][0];\
		(buf)[i + 1] = (pal)[pal_i][1];\
		(buf)[i + 2] = (pal)[pal_i][2];\
		(buf)[i + 3] = (pal)[pal_i][3];\
	}

	// データリーダーから DIB を読み込む.
	static void bitmap_read_dib(DataReader const& dt_reader, D2D1_SIZE_U& b_size, uint8_t** buf);

	// データリーダーから DIB データを読み込む.
	static void bitmap_read_dib(DataReader const& dt_reader, D2D1_SIZE_U& dib_size, uint8_t** dib_data)
	{
		using winrt::Windows::Storage::Streams::ByteOrder;

		// 2 バイトを読み込み, それが 'BM' か判定する.
		const char bf_type1 = dt_reader.ReadByte();
		const char bf_type2 = dt_reader.ReadByte();
		if (bf_type1 == 'B' && bf_type2 == 'M') {
			// バイトオーダーがリトルエンディアン以外か判定する.
			const auto b_order = dt_reader.ByteOrder();
			if (b_order != ByteOrder::LittleEndian) {
				dt_reader.ByteOrder(ByteOrder::LittleEndian);
			}
			uint32_t bf_size = dt_reader.ReadUInt32();	// ファイル全体のサイズ
			uint16_t bf_reserved_0 = dt_reader.ReadUInt16();
			uint16_t bf_reserved_1 = dt_reader.ReadUInt16();
			uint32_t bf_offset = dt_reader.ReadUInt32();	// ファイル先頭から画像データまでのオフセット (54 + パレット)
			uint32_t bi_size = dt_reader.ReadUInt32();	// BITMAOINFOHEADER = 40 または BITMAPCOREHEADER = 12 のサイズ
			uint32_t bi_width = 0;
			uint32_t bi_height = 0;
			uint16_t bi_bit_cnt = 0;
			uint32_t bi_compress = 0;
			uint32_t bi_size_img = 0;
			uint32_t bi_color_used = 0;
			uint8_t pallete[256][4];
			// BITMAOINFOHEADER か判定する.
			if (bi_size == 0x28) {
				bi_width = dt_reader.ReadUInt32();
				bi_height = dt_reader.ReadUInt32();
				uint16_t bi_planes = dt_reader.ReadUInt16();	// プレーンの数. かならず 1
				bi_bit_cnt = dt_reader.ReadUInt16();	// 色数 1=モノクロ, 4=16色, 8=256色, 24または32=フルカラー
				bi_compress = dt_reader.ReadUInt32();	// 圧縮タイプ. (0=BI_RGB, 1=BI_RLE8, 2=BI_RLE4, 3=BI_BITFIELDS)
				bi_size_img = dt_reader.ReadUInt32();	// 圧縮された画像データの大きさ (非圧縮=BI_RGB ならゼロ, とは限らない!)
				uint32_t x_px_per_meter = dt_reader.ReadUInt32();	// 96dpiのとき 3780. ふつうはゼロ.
				uint32_t y_px_per_meter = dt_reader.ReadUInt32();
				bi_color_used = dt_reader.ReadUInt32();
				uint32_t bi_color_important = dt_reader.ReadUInt32();
				if (bi_color_used == 0) {
					if (0 < bi_bit_cnt && bi_bit_cnt <= 8) {
						bi_color_used = (1 << bi_bit_cnt);
					}
				}
				for (size_t i = 0; i < bi_color_used; i++) {
					pallete[i][0] = dt_reader.ReadByte();	// B
					pallete[i][1] = dt_reader.ReadByte();	// G
					pallete[i][2] = dt_reader.ReadByte();	// R
					const auto opac = dt_reader.ReadByte();	// A
					pallete[i][3] = (bi_bit_cnt == 32 ? opac : 0xff);
				}
			}
			// BITMAPCOREHEADER か判定する.
			else if (bi_size == 0x0c) {
				bi_width = dt_reader.ReadUInt16();
				bi_height = dt_reader.ReadUInt16();
				uint16_t bi_planes = dt_reader.ReadUInt16();	// プレーンの数. かならず 1
				bi_bit_cnt = dt_reader.ReadUInt16();
				bi_compress = 0;
				bi_size_img = 0;	// 非圧縮の場合はゼロ.
				if (0 < bi_bit_cnt && bi_bit_cnt <= 8) {
					bi_color_used = (1 << bi_bit_cnt);
				}
				for (size_t i = 0; i < bi_color_used; i++) {
					pallete[i][0] = dt_reader.ReadByte();	// B
					pallete[i][1] = dt_reader.ReadByte();	// G
					pallete[i][2] = dt_reader.ReadByte();	// R
					pallete[i][2] = 0xff;
				}
			}
			const size_t row_len = 4 * bi_width;;
			const auto dst_len = row_len * bi_height;
			uint8_t* const dst_buf = new uint8_t[dst_len];
			*dib_data = dst_buf;
			if (bi_compress == BI_RGB || bi_compress == BI_BITFIELDS)	// BI_RGB, no pixel array compression used
			{
				if (bi_bit_cnt == 32) {
					std::vector<uint8_t> src_buf(row_len);
					for (size_t i = dst_len; i >= row_len; i -= row_len) {
						dt_reader.ReadBytes(src_buf);
						memcpy(dst_buf + i - row_len, src_buf.data(), row_len);
					}
					src_buf.clear();
					src_buf.shrink_to_fit();
				}
				else if (bi_bit_cnt == 24) {
					const size_t src_len = (static_cast<size_t>(bi_width) * 3 + (4 - 1)) / 4 * 4;
					std::vector<uint8_t> src_buf(src_len);
					for (size_t i = dst_len; i >= row_len; i -= row_len) {
						dt_reader.ReadBytes(src_buf);
						for (size_t j = 0, k = 0; j < row_len - 3 && k < src_len; j += 4, k += 3) {
							memcpy(dst_buf + i - row_len + j, src_buf.data() + k, 3);
							dst_buf[i - row_len + j + 3] = 0xff;
						}
					}
					src_buf.clear();
					src_buf.shrink_to_fit();
				}
			}
			else if ((bi_compress == BI_RLE8 || bi_compress == BI_RLE4) && bi_size_img > 0) {
				size_t j = 0;
				size_t s = 0;
				for (size_t i = 0; i < bi_size_img; i += 2) {
					const auto b0 = dt_reader.ReadByte();
					const auto b1 = dt_reader.ReadByte();
					if (s == 0) {
						if (b0 == 0) {
							if (b1 == 0) {
								j = (j / row_len + 1) * row_len;
							}
							else if (b1 == 1) {
								break;
							}
							else if (b1 == 2) {
								s = 1;
							}
							else {
								s = b1;
							}
						}
						else {
							for (size_t k = 0; k < b0; k++) { 
								if (bi_compress == BI_RLE8 && j + 4 <= dst_len) {
									DIB_SET(dst_buf, j, pallete, b1);
									j += 4;
								}
								else if (bi_compress == BI_RLE4 && j + 8 <= dst_len) {
									DIB_SET(dst_buf, j, pallete, NIBBLE_HI(b1));
									DIB_SET(dst_buf, j + 4, pallete, NIBBLE_LO(b1));
									j += 8;
								}
							}
						}
					}
					else if (s == 1) {
						j = b0 * row_len + b1;
						s = 0;
					}
					else if (s == 3) {
						if (bi_compress == BI_RLE8 && j + 4 <= dst_len) {
							DIB_SET(dst_buf, j, pallete, b0);
							j += 4;
						}
						else if (bi_compress == BI_RLE4 && j + 8 <= dst_len) {
							DIB_SET(dst_buf, j, pallete, NIBBLE_HI(b0));
							DIB_SET(dst_buf, j + 4, pallete, NIBBLE_LO(b0));
							j += 8;
						}
						s = 0;
					}
					else {
						if (bi_compress == BI_RLE8 && j + 8 <= dst_len) {
							DIB_SET(dst_buf, j, pallete, b0);
							DIB_SET(dst_buf, j + 4, pallete, b1);
							j += 8;
							s -= 2;
						}
						else if (bi_compress == BI_RLE4 && j + 16 <= dst_len) {
							DIB_SET(dst_buf, j, pallete, NIBBLE_HI(b0));
							DIB_SET(dst_buf, j + 4, pallete, NIBBLE_LO(b0));
							DIB_SET(dst_buf, j + 8, pallete, NIBBLE_HI(b1));
							DIB_SET(dst_buf, j + 12, pallete, NIBBLE_LO(b1));
							j += 16;
							s -= 4;
						}
					}
				}
			}
			dib_size.width = bi_width;
			dib_size.height = bi_height;
			// バイトオーダーがリトルエンディアン以外か判定する.
			if (b_order != ByteOrder::LittleEndian) {
				// バイトオーダーを元に戻す.
				dt_reader.ByteOrder(b_order);
			}
		}
	}

	ShapeBitmap::~ShapeBitmap(void)
	{
		if (m_data != nullptr) {
			delete m_data;
			m_data = nullptr;
		}
		if (m_dx_bitmap != nullptr) {
			m_dx_bitmap = nullptr;
		}
	}

	void ShapeBitmap::draw(SHAPE_DX& dx)
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

	// 部位の位置を得る.
	void ShapeBitmap::get_pos_anch(const uint32_t anch, D2D1_POINT_2F& value) const noexcept
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

	// 画像の不透明度を得る.
	bool ShapeBitmap::get_image_opacity(float& value) const noexcept
	{
		value = m_opac;
		return true;
	}

	// 図形を囲む領域を得る.
	void ShapeBitmap::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		D2D1_POINT_2F b_pos[2]{
			m_pos,
			{ m_pos.x + m_view.width, m_pos.y + m_view.height }
		};
		pt_bound(b_pos[0], b_pos[1], b_pos[0], b_pos[1]);
		pt_min(a_min, b_pos[0], b_min);
		pt_max(a_max, b_pos[1], b_max);
	}

	// 近傍の頂点を得る.
	bool ShapeBitmap::get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& value) const noexcept
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

	// 図形を囲む領域の左上位置を得る.
	void ShapeBitmap::get_pos_min(D2D1_POINT_2F& value) const noexcept
	{
		const D2D1_POINT_2F v_pos[2]{
			m_pos,
			{ m_pos.x + m_view.width, m_pos.y + m_view.height }
		};
		pt_min(v_pos[0], v_pos[1], value);
	}

	// 開始位置を得る.
	bool ShapeBitmap::get_pos_start(D2D1_POINT_2F& value) const noexcept
	{
		value = m_pos;
		return true;
	}

	// 頂点を得る.
	size_t ShapeBitmap::get_verts(D2D1_POINT_2F v_pos[]) const noexcept
	{
		v_pos[0] = m_pos;
		v_pos[1] = D2D1_POINT_2F{ m_pos.x + m_view.width, m_pos.y };
		v_pos[2] = D2D1_POINT_2F{ m_pos.x + m_view.width, m_pos.y + m_view.height };
		v_pos[3] = D2D1_POINT_2F{ m_pos.x, m_pos.y + m_view.height };
		return 4;
	}

	uint32_t ShapeBitmap::hit_test(const D2D1_POINT_2F t_pos) const noexcept
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

	// 差分だけ移動する.
	bool ShapeBitmap::move(const D2D1_POINT_2F value)
	{
		pt_add(m_pos, value, m_pos);
		return true;
	}

	// 値を画像の不透明度に格納する.
	bool ShapeBitmap::set_image_opacity(const float value) noexcept
	{
		if (!equal(m_opac, value)) {
			m_opac = value;
			return true;
		}
		return false;
	}

	// 値を消去されたか判定に格納する.
	bool ShapeBitmap::set_delete(const bool value) noexcept
	{
		m_is_deleted = value;
		return true;
	}

	// 点に最も近い, 線分上の点を求める.
	static void get_pos_on_line(const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& q)
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

	// 元の大きさに戻す.
	void ShapeBitmap::resize_origin(void) noexcept
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

	// 値を, 部位の位置に格納する.
	// value	値
	// anch	図形の部位
	// limit	限界距離 (他の頂点との距離がこの値未満になるなら, その頂点に位置に合わせる)
	// keep_aspect	画像図形の縦横比を維持
	bool ShapeBitmap::set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float limit, const bool keep_aspect)
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
					get_pos_on_line(value, s_pos, e_pos, pos);
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
if (fabs(m_rect.bottom - m_rect.top) < 1.0 || fabs(m_rect.right - m_rect.left) < 1.0) {
	auto debug = 1;
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
					get_pos_on_line(value, s_pos, e_pos, pos);
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
					get_pos_on_line(value, s_pos, e_pos, pos);
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
					get_pos_on_line(value, s_pos, e_pos, pos);
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
	bool ShapeBitmap::set_pos_start(const D2D1_POINT_2F value)
	{
		m_pos = value;
		return true;
	}

	// 値を選択されてるか判定に格納する.
	bool ShapeBitmap::set_select(const bool value) noexcept
	{
		m_is_selected = value;
		return true;
	}

	// データリーダーから読み込む
	// c_pos	画像を配置する位置 (画像の中心)
	// dt_reader	データリーダー
	ShapeBitmap::ShapeBitmap(const D2D1_POINT_2F c_pos, DataReader const& dt_reader)		
	{
		bitmap_read_dib(dt_reader, m_size, &m_data);

		m_rect.left = 0;
		m_rect.top = 0;
		m_rect.right = static_cast<FLOAT>(m_size.width);
		m_rect.bottom = static_cast<FLOAT>(m_size.height);
		m_view.width = static_cast<FLOAT>(m_size.width);
		m_view.height = static_cast<FLOAT>(m_size.height);
		m_pos.x = c_pos.x - m_view.width * 0.5f;
		m_pos.y = c_pos.y - m_view.height * 0.5f;
		m_ratio.width = 1.0f;
		m_ratio.height = 1.0f;
	}

	// データリーダーから読み込む
	// dt_reader	データリーダー
	ShapeBitmap::ShapeBitmap(DataReader const& dt_reader)
	{
		m_is_deleted = dt_reader.ReadBoolean();
		m_is_selected = dt_reader.ReadBoolean();
		dt_read(m_pos, dt_reader);
		dt_read(m_view, dt_reader);
		dt_read(m_rect, dt_reader);
		dt_read(m_size, dt_reader);
		dt_read(m_ratio, dt_reader);

		const size_t row_size = 4 * m_size.width;
		m_data = new uint8_t[row_size * m_size.height];
		std::vector<uint8_t> buf(row_size);
		for (size_t i = 0; i < m_size.height; i++) {
			dt_reader.ReadBytes(buf);
			memcpy(m_data + row_size * i, buf.data(), row_size);
		}
	}

	// データライターに書き込む.
	// dt_writer	データライター
	void ShapeBitmap::write(DataWriter const& dt_writer) const
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

	void ShapeBitmap::write_bmp(DataWriter const& dt_writer) const
	{
		const D2D1_RECT_U rect{
			static_cast<uint32_t>(roundf(m_rect.left)),
			static_cast<uint32_t>(roundf(m_rect.top)),
			static_cast<uint32_t>(roundf(m_rect.right)),
			static_cast<uint32_t>(roundf(m_rect.bottom))
		};
		constexpr uint32_t bi_size = 0x28;	// BITMAOINFOHEADER のサイズ (40 バイト)
		const uint32_t bi_width = rect.right - rect.left;
		const uint32_t bi_height = rect.bottom - rect.top;
		if (bi_width == 0 || bi_height == 0) {
			return;
		}
		using winrt::Windows::Storage::Streams::ByteOrder;
		dt_writer.ByteOrder(ByteOrder::LittleEndian);
		dt_writer.WriteByte('B');
		dt_writer.WriteByte('M');
		const uint32_t bf_size = 12 + bi_size + bi_height * 4 * bi_width;
		dt_writer.WriteUInt32(bf_size);	// bf_size: ファイル全体のサイズ.
		dt_writer.WriteUInt16(0);	// 予約 1 = 0
		dt_writer.WriteUInt16(0);	// 予約 2 = 0
		dt_writer.WriteUInt32(54);	// bf_offset: ファイル先頭から画像データまでのオフセット (54 + パレット)
		dt_writer.WriteUInt32(bi_size);	// bi_size: BITMAOINFOHEADER のサイズ = 40
		dt_writer.WriteUInt32(bi_width); // bi_width
		dt_writer.WriteUInt32(bi_height); //bi_height = dt_reader.ReadUInt32();
		dt_writer.WriteUInt16(1); //uint16_t bi_planes = dt_reader.ReadUInt16();	// プレーンの数. かならず 1
		dt_writer.WriteUInt16(32); //32? bi_bit_cnt = dt_reader.ReadUInt16();
		dt_writer.WriteUInt32(0);	//bi_compress = dt_reader.ReadUInt32();	// 圧縮タイプ. (0=BI_RGB, 1=BI_RLE8, 2=BI_RLE4, 3=BI_BITFIELDS)
		dt_writer.WriteUInt32(0);	//bi_size_img = dt_reader.ReadUInt32();
		dt_writer.WriteUInt32(0); //uint32_t x_px_per_meter = dt_reader.ReadUInt32();	// 96dpiのとき 3780. ふつうはゼロ.
		dt_writer.WriteUInt32(0); //uint32_t y_px_per_meter = dt_reader.ReadUInt32();
		dt_writer.WriteUInt32(0); //bi_color_used = dt_reader.ReadUInt32();
		dt_writer.WriteUInt32(0); //uint32_t bi_color_important = dt_reader.ReadUInt32();
		for (size_t y = rect.bottom; y != rect.top; y--) {
			for (size_t x = rect.left; x < rect.right; x++) {
				dt_writer.WriteByte(m_data[(y - 1) * 4 * bi_width + 4 * x + 0]);
				dt_writer.WriteByte(m_data[(y - 1) * 4 * bi_width + 4 * x + 1]);
				dt_writer.WriteByte(m_data[(y - 1) * 4 * bi_width + 4 * x + 2]);
				dt_writer.WriteByte(m_data[(y - 1) * 4 * bi_width + 4 * x + 3]);
			}
		}
	}

	struct ADLER32 {
		static constexpr uint32_t INIT = 1;
		static uint32_t update(const uint32_t adler, const uint8_t b) noexcept
		{
			#define ADLER32_BASE 65521
			const uint32_t s1 = ((adler & 0xffff) + b) % ADLER32_BASE;
			const uint32_t s2 = (((adler >> 16) & 0xffff) + s1) % ADLER32_BASE;
			return (s2 << 16) + s1;
		}
		template <size_t N>
		static uint32_t update(uint32_t adler, const std::array<uint8_t, N>& buf) noexcept
		{
			for (const auto b : buf) {
				adler = update(adler, b);
			}
			return adler;
		}
	};

	struct CRC32 {
		static constexpr uint32_t INIT = 0xffffffff;
		static bool computed;
		static uint32_t table[256];
		static uint32_t update(const uint32_t crc, const uint8_t b) noexcept
		{
			if (!computed) {
				computed = true;
				for (int n = 0; n < 256; n++) {
					auto c = static_cast<uint32_t>(n);
					for (int k = 0; k < 8; k++) {
						if (c & 1) {
							c = (0xedb88320L ^ (c >> 1));
						}
						else {
							c >>= 1;
						}
					}
					table[n] = c;
				}
			}
			return table[(crc ^ b) & 0xff] ^ (crc >> 8);
		}

		template<size_t N>
		static uint32_t update(uint32_t crc, const std::array<uint8_t, N>& buf) noexcept
		{
			for (const auto b : buf) {
				crc = update(crc, b);
			}
			return crc;
		}
	};
	bool CRC32::computed = false;
	uint32_t CRC32::table[256];
	/*
#define BYTE4(ptr) ((static_cast<uint32_t>((ptr)[0]) << 24) | (static_cast<uint32_t>((ptr)[1]) << 16) | (static_cast<uint32_t>((ptr)[2]) << 8) | (static_cast<uint32_t>((ptr)[3])))

	void ShapeBitmap::read_png(DataReader const& dt_reader) const
	{
		constexpr uint8_t PNG[8]{
			137, L'P', L'N', L'G', 13, 10, 26, 10
		};
		if (PNG[0] != dt_reader.ReadByte() ||
			PNG[1] != dt_reader.ReadByte() ||
			PNG[2] != dt_reader.ReadByte() ||
			PNG[3] != dt_reader.ReadByte() ||
			PNG[4] != dt_reader.ReadByte() ||
			PNG[5] != dt_reader.ReadByte() ||
			PNG[6] != dt_reader.ReadByte() ||
			PNG[7] != dt_reader.ReadByte()) {
			return;
		}
		constexpr size_t MAX_LENGTH = ((2 << 30) - 1);
		uint8_t chunk_buf[MAX_LENGTH + 4];
		uint8_t* const chunk_data = chunk_buf + 4;
		uint8_t pallete[256 * 3];
		uint8_t opacity[256];
		memset(opacity, 0xff, 256);
		CHUNK_CRC crc;
		uint32_t width = -1;
		uint32_t height = -1;
		uint32_t bit_depth = -1;
		uint32_t color_type = -1;
		uint32_t compress_method = -1;
		uint32_t filter_method = -1;
		uint32_t interlace_method = -1;
		uint32_t white_point_x = -1;
		uint32_t white_point_y = -1;
		uint32_t red_x = -1;
		uint32_t red_y = -1;
		uint32_t green_x = -1;
		uint32_t green_y = -1;
		uint32_t blue_x = -1;
		uint32_t blue_y = -1;
		int chunk_st = 0;
		bool mult_cHRM = false;
		bool mult_gAMA = false;
		bool mult_iCCP = false;
		bool mult_sBIT = false;
		bool mult_sRGB = false;
		bool mult_bKGD = false;
		bool mult_hIST = false;
		bool mult_tRNS = false;
		bool mult_pHYs = false;
		bool mult_tIME = false;
		while (chunk_st != 4) {
			const size_t chunk_len = dt_reader.ReadUInt32();
			if (chunk_len > MAX_LENGTH) {
				break;
			}
			for (size_t i = 0; i < 4; i++) {
				chunk_buf[i] = dt_reader.ReadByte();
			}
			for (size_t i = 0; i < chunk_len; i++) {
				chunk_data[i] = dt_reader.ReadByte();
			}
			if (dt_reader.ReadUInt32() != crc.check(chunk_buf, chunk_len)) {
				break;
			}
			if (chunk_st == 0) {
				if (memcmp(chunk_buf, "IHDR", 4) == 0 && chunk_len == 13) {
					chunk_st = 1;
					width = BYTE4(chunk_data);
					if (width == 0 || width >= MAX_LENGTH) {
						break;
					}
					height = BYTE4(chunk_data + 4);
					if (height == 0 || height >= MAX_LENGTH) {
						break;
					}
					bit_depth = chunk_data[8];
					color_type = chunk_data[9];
					if (color_type == 0 && (bit_depth == 1 || bit_depth == 2 || bit_depth == 4 || bit_depth == 8 || bit_depth == 16)) {
					}
					else if (color_type == 2 && (bit_depth == 8 || bit_depth == 16)) {
					}
					else if (color_type == 3 && (bit_depth == 1 || bit_depth == 2 || bit_depth == 4 || bit_depth == 8)) {
					}
					else if (color_type == 4 && (bit_depth == 8 || bit_depth == 16)) {
					}
					else if (color_type == 6 && (bit_depth == 8 || bit_depth == 16)) {
					}
					else {
						break;
					}
					compress_method = chunk_data[10];
					if (compress_method != 0) {
						break;
					}
					filter_method = chunk_data[11];
					if (filter_method != 0) {
						break;
					}
					interlace_method = chunk_data[12];
					if (interlace_method != 0 && interlace_method != 1) {
						break;
					}
				}
				else {
					break;
				}
			}
			else if (chunk_st == 1) {
				if (memcmp(chunk_buf, "PLTE", 4) == 0 && chunk_len <= 768 && chunk_len % 3 == 0) {
					chunk_st = 2;
					memcpy(pallete, chunk_data, chunk_len);
				}
				else if (memcmp(chunk_buf, "IDAT", 4) == 0) {
					chunk_st = 3;
				}
				else if (!mult_cHRM && memcmp(chunk_buf, "cHRM", 4) == 0) {
					mult_cHRM = true;
				}
				else if (!mult_gAMA && memcmp(chunk_buf, "gAMA", 4) == 0) {
					mult_gAMA = true;
				}
				else if (!mult_iCCP && memcmp(chunk_buf, "iCCP", 4) == 0) {
					mult_iCCP = true;
				}
				else if (!mult_sBIT && memcmp(chunk_buf, "sBIT", 4) == 0) {
					mult_sBIT = true;
				}
				else if (!mult_sBIT && memcmp(chunk_buf, "sRGB", 4) == 0) {
					mult_sBIT = true;
				}
				else if (!mult_pHYs && memcmp(chunk_buf, "pHYs", 4) == 0) {
					mult_pHYs = true;
				}
				else if (memcmp(chunk_buf, "sPLT", 4) == 0) {
				}
				else if (!mult_tIME && memcmp(chunk_buf, "tIME", 4) == 0) {
					mult_tIME = true;
				}
				else if (memcmp(chunk_buf, "iTXt", 4) == 0) {
				}
				else {
					break;
				}
			}
			else if (chunk_st == 2) {
				if (memcmp(chunk_buf, "IDAT", 4) == 0) {
					chunk_st = 3;
				}
				else if (!mult_bKGD && memcmp(chunk_buf, "bKGD", 4) == 0) {
					mult_bKGD = true;
				}
				else if (!mult_hIST && memcmp(chunk_buf, "hIST", 4) == 0) {
					mult_hIST = true;
				}
				else if (!mult_tRNS && memcmp(chunk_buf, "tRNS", 4) == 0 && chunk_len <= 256 && (color_type != 4 && color_type != 6)) {
					mult_tRNS = true;
					memcpy(opacity, chunk_data, chunk_len);
				}
				else if (!mult_pHYs && memcmp(chunk_buf, "pHYs", 4) == 0) {
					mult_pHYs = true;
				}
				else if (memcmp(chunk_buf, "sPLT", 4) == 0) {
				}
				else if (!mult_tIME && memcmp(chunk_buf, "tIME", 4) == 0) {
					mult_tIME = true;
				}
				else if (memcmp(chunk_buf, "iTXt", 4) == 0) {
				}
				else {
					break;
				}
			}
			else if (chunk_st == 3) {
				if (memcmp(chunk_buf, "IDAT", 4) == 0) {
				}
				else if (memcmp(chunk_buf, "IEND", 4) == 0) {
					chunk_st = 4;
				}
				else if (!mult_tIME && memcmp(chunk_buf, "tIME", 4) == 0) {
					mult_tIME = true;
				}
				else if (memcmp(chunk_buf, "iTXt", 4) == 0) {
				}
				else {
					break;
				}
			}
		}
	}
	*/
	size_t put(uint8_t buf[], const size_t i, const size_t n, const size_t m)
	{
		return 0;
	}
	size_t put_len(uint8_t buf[], const size_t i, const size_t len)
	{
		return 0;
	}
	size_t put_pos(uint8_t buf[], const size_t i, const size_t pos)
	{
		return 0;
	}
	size_t compress(const uint8_t i_buf[], const size_t i_len, uint8_t o_buf[])
	{
		const uint8_t* window_ptr = i_buf;
		size_t window_len = 0;
		size_t o = 0;
		while (window_len < 3) {
			o = put(o_buf, o, 1, 0);
			o = put(o_buf, o, 8, i_buf[window_len++]);
		}
		size_t plane_len = 3;
		for (size_t i = 0; i + plane_len <= window_len; i++) {
			if (memcmp(window_ptr + i, i_buf + window_len, plane_len) == 0) {
				o = put_len(o_buf, o, plane_len);
				o = put_pos(o_buf, o, plane_len);
				window_ptr += plane_len;
			}
		}
	}
#define SET_BYTE4(ptr, u) { (ptr)[0] = static_cast<uint8_t>(((u) >> 24) & 0xff); (ptr)[1] = static_cast<uint8_t>(((u) >> 16) & 0xff); (ptr)[2] = static_cast<uint8_t>(((u) >> 8) & 0xff); (ptr)[3] = static_cast<uint8_t>((u) & 0xff);}
	void ShapeBitmap::write_png(DataWriter const& dt_writer) const
	{
		using winrt::Windows::Storage::Streams::ByteOrder;

		dt_writer.ByteOrder(ByteOrder::BigEndian);	// ネットワークバイトオーダー
		// PNG ファイルシグネチャ
		constexpr std::array<uint8_t, 8> png{ 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
		dt_writer.WriteBytes(png);
		// IHDR チャンク
		uint32_t chunk_checksum = CRC32::INIT;
		dt_writer.WriteUInt32(13);	// Length: Chunk Data のサイズ. 常に 13
		constexpr std::array<uint8_t, 4> ihdr_type{ 'I', 'H', 'D', 'R' };
		dt_writer.WriteBytes(ihdr_type);
		chunk_checksum = CRC32::update(chunk_checksum, ihdr_type);
		const std::array<uint8_t, 13> ihdr_data{
			static_cast<uint8_t>(m_size.width >> 24),
			static_cast<uint8_t>(m_size.width >> 16),
			static_cast<uint8_t>(m_size.width >> 8),
			static_cast<uint8_t>(m_size.width),
			static_cast<uint8_t>(m_size.height >> 24),
			static_cast<uint8_t>(m_size.height >> 16),
			static_cast<uint8_t>(m_size.height >> 8),
			static_cast<uint8_t>(m_size.height),
			8,	// ビット深度
			6,	// カラータイプ: トゥルーカラー＋アルファ 画像
			0,	// uint32_t compress_method = -1;
			0,	// uint32_t filter_method = -1;
			0	// uint32_t interlace_method = -1;
		};
		dt_writer.WriteBytes(ihdr_data);
		chunk_checksum = CRC32::update(chunk_checksum, ihdr_data);
		dt_writer.WriteUInt32(chunk_checksum ^ CRC32::INIT);

		chunk_checksum = CRC32::INIT;
		const size_t data_len = 4ull * m_size.width * m_size.height;
		const uint32_t idat_len = 2ull + 4ull + data_len + (data_len + 65534ull) / 65535ull * 5;
		dt_writer.WriteUInt32(idat_len);
		constexpr std::array<uint8_t, 4> idat_type{ 'I', 'D', 'A', 'T' };
		dt_writer.WriteBytes(idat_type);
		chunk_checksum = CRC32::update(chunk_checksum, idat_type);

		// なんちゃって zlib ストリーム
		constexpr std::array<uint8_t, 2> zlib_stream{ 190, 94 };	// 圧縮方式とフラグ
		dt_writer.WriteBytes(zlib_stream);
		chunk_checksum = CRC32::update(chunk_checksum, zlib_stream);
		// なんちゃって Deflate
		constexpr size_t b_size = 65532;	// 4 の倍数
		for (size_t i = 0; i < data_len; i += 4) {
			if (i % b_size == 0) {
				const uint8_t  block_hdr = data_len <= i + b_size ?
					1 : 0;
				const uint16_t block_len = data_len <= i + b_size ?
					static_cast<uint16_t>(data_len - i) :
					static_cast<uint16_t>(b_size);
				const std::array<uint8_t, 5> deflate{
					block_hdr,
					static_cast<uint8_t>(block_len >> 8),
					static_cast<uint8_t>(block_len),
					static_cast<uint8_t>(~block_len >> 8),
					static_cast<uint8_t>(~block_len),
				};
				dt_writer.WriteBytes(deflate);
				chunk_checksum = CRC32::update(chunk_checksum, deflate);
			}
			const std::array<uint8_t, 4> argb{
				0xff,
				m_data[i + 1],
				m_data[i + 2],
				0xff
			};
			//const std::array<uint8_t, 4> argb{
			//	m_data[i],
			//	m_data[i + 1],
			//	m_data[i + 2],
			//	m_data[i + 3]
			//};
			dt_writer.WriteBytes(argb);
			chunk_checksum = CRC32::update(chunk_checksum, argb);
		}
		const auto zlib_checksum = ADLER32::update(ADLER32::INIT, zlib_stream);
		const std::array<uint8_t, 4> zlib_adler{
			static_cast<uint8_t>(zlib_checksum >> 24),
			static_cast<uint8_t>(zlib_checksum >> 16),
			static_cast<uint8_t>(zlib_checksum >> 8),
			static_cast<uint8_t>(zlib_checksum)
		};
		dt_writer.WriteBytes(zlib_adler);
		chunk_checksum = CRC32::update(chunk_checksum, zlib_adler);
		dt_writer.WriteUInt32(chunk_checksum ^ CRC32::INIT);


		chunk_checksum = CRC32::INIT;
		dt_writer.WriteUInt32(0);
		constexpr std::array<uint8_t, 4> iend_type{ 'I', 'E', 'N', 'D' };
		dt_writer.WriteBytes(iend_type);
		chunk_checksum = CRC32::update(chunk_checksum, iend_type);
		dt_writer.WriteUInt32(chunk_checksum ^ CRC32::INIT);
	}

	void ShapeBitmap::write_svg(const wchar_t f_name[], DataWriter const& dt_writer) const
	{
		dt_write_svg("<image href=\"", dt_writer);
		const auto len = static_cast<uint32_t>(wcslen(f_name));
		dt_write_svg(f_name, len, dt_writer);
		dt_write_svg("\" ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_view.width, "width", dt_writer);
		dt_write_svg(m_view.height, "height", dt_writer);
		dt_write_svg(m_opac, "opacity", dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
	}
}