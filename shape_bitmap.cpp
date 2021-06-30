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
	// データリーダーから D2D1_RECT_F を読み込む.
	static void bitmap_read_rect(D2D1_RECT_F& rect, DataReader const& dt_reader);
	// データライターに D2D1_RECT_F を書き込む.
	static void bitmap_write_rect(const D2D1_RECT_F& rect, DataWriter const& dt_writer);

	// データリーダーから DIB データを読み込む.
	static void bitmap_read_dib(DataReader const& dt_reader, D2D1_SIZE_U& dib_size, uint8_t** dib_data)
	{
		using winrt::Windows::Storage::Streams::ByteOrder;

		// 2 バイトを読み込み, それが 'BM' か判定する.
		const char bf_type1 = dt_reader.ReadByte();
		const char bf_type2 = dt_reader.ReadByte();
		if (bf_type1 == 'B' && bf_type2 == 'M') {
			// データのバイトオーダーを得る.
			const auto b_order = dt_reader.ByteOrder();
			// バイトオーダーがリトルエンディアン以外か判定する.
			if (b_order != ByteOrder::LittleEndian) {
				dt_reader.ByteOrder(ByteOrder::LittleEndian);
			}
			uint32_t bf_size = dt_reader.ReadUInt32();	// ファイル全体のサイズ.
			uint16_t bf_reserved_0 = dt_reader.ReadUInt16();
			uint16_t bf_reserved_1 = dt_reader.ReadUInt16();
			uint32_t bf_offset = dt_reader.ReadUInt32();	// イメージデータまでのオフセット
			uint32_t bi_size = dt_reader.ReadUInt32();	// BITMAOINFOHEADER または BITMAPCOREHEADER のサイズ
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
				bi_bit_cnt = dt_reader.ReadUInt16();
				bi_compress = dt_reader.ReadUInt32();	// 圧縮タイプ. (0=BI_RGB, 1=BI_RLE8, 2=BI_RLE4, 3=BI_BITFIELDS)
				bi_size_img = dt_reader.ReadUInt32();
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
			const auto buf_len = row_len * bi_height;
			uint8_t* const buf_ptr = new uint8_t[buf_len];
			*dib_data = buf_ptr;
			if (bi_compress == BI_RGB || bi_compress == BI_BITFIELDS)	// BI_RGB, no pixel array compression used
			{
				std::vector<uint8_t> line_buf(row_len);
				for (size_t i = 1; i <= bi_height; i++) {
					dt_reader.ReadBytes(line_buf);
					memcpy(buf_ptr + buf_len - row_len * i, line_buf.data(), row_len);
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
								if (bi_compress == BI_RLE8 && j + 4 <= buf_len) {
									DIB_SET(buf_ptr, j, pallete, b1);
									j += 4;
								}
								else if (bi_compress == BI_RLE4 && j + 8 <= buf_len) {
									DIB_SET(buf_ptr, j, pallete, NIBBLE_HI(b1));
									DIB_SET(buf_ptr, j + 4, pallete, NIBBLE_LO(b1));
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
						if (bi_compress == BI_RLE8 && j + 4 <= buf_len) {
							DIB_SET(buf_ptr, j, pallete, b0);
							j += 4;
						}
						else if (bi_compress == BI_RLE4 && j + 8 <= buf_len) {
							DIB_SET(buf_ptr, j, pallete, NIBBLE_HI(b0));
							DIB_SET(buf_ptr, j + 4, pallete, NIBBLE_LO(b0));
							j += 8;
						}
						s = 0;
					}
					else {
						if (bi_compress == BI_RLE8 && j + 8 <= buf_len) {
							DIB_SET(buf_ptr, j, pallete, b0);
							DIB_SET(buf_ptr, j + 4, pallete, b1);
							j += 8;
							s -= 2;
						}
						else if (bi_compress == BI_RLE4 && j + 16 <= buf_len) {
							DIB_SET(buf_ptr, j, pallete, NIBBLE_HI(b0));
							DIB_SET(buf_ptr, j + 4, pallete, NIBBLE_LO(b0));
							DIB_SET(buf_ptr, j + 8, pallete, NIBBLE_HI(b1));
							DIB_SET(buf_ptr, j + 12, pallete, NIBBLE_LO(b1));
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

	// データリーダーから D2D1_RECT_F を読み込む.
	static void bitmap_read_rect(D2D1_RECT_F& rect, DataReader const& dt_reader)
	{
		rect.left = dt_reader.ReadSingle();
		rect.top = dt_reader.ReadSingle();
		rect.right = dt_reader.ReadSingle();
		rect.bottom = dt_reader.ReadSingle();
	}

	// データライターに D2D1_RECT_F を書き込む.
	static void bitmap_write_rect(const D2D1_RECT_F& rect, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(rect.left);
		dt_writer.WriteSingle(rect.top);
		dt_writer.WriteSingle(rect.right);
		dt_writer.WriteSingle(rect.bottom);
	}

	ShapeBitmap::~ShapeBitmap(void)
	{
		if (m_bm_data != nullptr) {
			delete m_bm_data;
			m_bm_data = nullptr;
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
			dx.m_d2dContext->CreateBitmap(m_bm_size, static_cast<void*>(m_bm_data), 4 * m_bm_size.width, b_prop, m_dx_bitmap.put());
			if (m_dx_bitmap == nullptr) {
				return;
			}
		}
		D2D1_RECT_F dest_rect{
			m_pos.x,
			m_pos.y,
			m_pos.x + m_view_size.width,
			m_pos.y + m_view_size.height
		};
		dx.m_d2dContext->DrawBitmap(m_dx_bitmap.get(), dest_rect, m_bm_opac, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, m_bm_rect);

		if (is_selected()) {
			dx.m_shape_brush->SetColor(Shape::m_default_background);
			dx.m_d2dContext->DrawRectangle(dest_rect, dx.m_shape_brush.get(), 1.0f, nullptr);
			dx.m_shape_brush->SetColor(Shape::m_default_foreground);
			dx.m_d2dContext->DrawRectangle(dest_rect, dx.m_shape_brush.get(), 1.0f, dx.m_aux_style.get());

			D2D1_POINT_2F v_pos[4]{
				m_pos,
				{ m_pos.x + m_view_size.width, m_pos.y },
				{ m_pos.x + m_view_size.width, m_pos.y + m_view_size.height },
				{ m_pos.x, m_pos.y + m_view_size.height },
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
			value.x = m_pos.x + m_view_size.width * 0.5f;
			value.y = m_pos.y;
		}
		else if (anch == ANCH_TYPE::ANCH_NE) {
			value.x = m_pos.x + m_view_size.width;
			value.y = m_pos.y;
		}
		else if (anch == ANCH_TYPE::ANCH_EAST) {
			value.x = m_pos.x + m_view_size.width;
			value.y = m_pos.y + m_view_size.height * 0.5f;
		}
		else if (anch == ANCH_TYPE::ANCH_SE) {
			value.x = m_pos.x + m_view_size.width;
			value.y = m_pos.y + m_view_size.height;
		}
		else if (anch == ANCH_TYPE::ANCH_SOUTH) {
			value.x = m_pos.x + m_view_size.width * 0.5f;
			value.y = m_pos.y + m_view_size.height;
		}
		else if (anch == ANCH_TYPE::ANCH_SW) {
			value.x = m_pos.x;
			value.y = m_pos.y + m_view_size.height;
		}
		else if (anch == ANCH_TYPE::ANCH_WEST) {
			value.x = m_pos.x;
			value.y = m_pos.y + m_view_size.height * 0.5f;
		}
	}

	// 画像の縦横比の維持を得る.
	//bool ShapeBitmap::get_bm_keep_aspect(bool& value) const noexcept
	//{
	//	value = m_bm_keep_aspect;
	//	return true;
	//}

	// 画像の不透明度を得る.
	bool ShapeBitmap::get_bm_opacity(float& value) const noexcept
	{
		value = m_bm_opac;
		return true;
	}

	// 図形を囲む領域を得る.
	void ShapeBitmap::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		D2D1_POINT_2F b_pos[2]{
			m_pos,
			{ m_pos.x + m_view_size.width, m_pos.y + m_view_size.height }
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
			const float v_dd = pt_abs2(v_vec);
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
			{ m_pos.x + m_view_size.width, m_pos.y + m_view_size.height }
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
		v_pos[1] = D2D1_POINT_2F{ m_pos.x + m_view_size.width, m_pos.y };
		v_pos[2] = D2D1_POINT_2F{ m_pos.x + m_view_size.width, m_pos.y + m_view_size.height };
		v_pos[3] = D2D1_POINT_2F{ m_pos.x, m_pos.y + m_view_size.height };
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

	// 値を画像の縦横比の維持に格納する.
	//bool ShapeBitmap::set_bm_keep_aspect(const bool value) noexcept
	//{
	//	const auto old_value = m_bm_keep_aspect;
	//	return (m_bm_keep_aspect = value) != old_value;
	//}
	bool ShapeBitmap::s_bm_keep_aspect = true;

	// 値を画像の不透明度に格納する.
	bool ShapeBitmap::set_bm_opacity(const float value) noexcept
	{
		if (!equal(m_bm_opac, value)) {
			m_bm_opac = value;
			return true;
		}
		return false;
	}

	// 値を消去フラグに格納する.
	bool ShapeBitmap::set_delete(const bool value) noexcept
	{
		m_is_deleted = value;
		return true;
	}

	static void neighbor(const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& q)
	{
		if (a.x == b.x) {
			q.x = a.x;
			q.y = p.y;
		}
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

	// 値を, 部位の位置に格納する.
	// value	値
	// anch	図形の部位
	// limit	限界距離 (他の頂点との距離がこの値未満になるなら, その頂点に位置に合わせる)
	bool ShapeBitmap::set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float limit)
	{
		bool flag = false;
		if (anch == ANCH_TYPE::ANCH_NW) {
			// 画像の一部分でも表示されているか判定する.
			// (画像がまったく表示されてない場合はスケールの変更は行わない.)
			if (m_bm_rect.left + FLT_MIN < m_bm_rect.right && m_bm_rect.top + FLT_MIN < m_bm_rect.bottom) {
				const float bm_w = m_bm_rect.right - m_bm_rect.left;	// 表示されている画像の幅 (原寸)
				const float bm_h = m_bm_rect.bottom - m_bm_rect.top;	// 表示されている画像の高さ (原寸)
				const D2D1_POINT_2F s_pos{ m_pos };	// 始点 (図形の頂点)
				D2D1_POINT_2F pos;
				if (s_bm_keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x + bm_w, s_pos.y + bm_h };	// 終点 (始点と対角にある画像上の点)
					neighbor(value, s_pos, e_pos, pos);
				}
				else {
					pos = value;
				}
				// 値と始点との差分を求め, 差分がゼロより大きいか判定する.
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					// スケール変更後の表示の寸法を求め, その縦横が 1 ピクセル以上あるか判定する.
					const float view_w = m_view_size.width - v_vec.x;
					const float view_h = m_view_size.height - v_vec.y;
					if (view_w >= 1.0f && view_h >= 1.0f) {
						m_view_size.width = view_w;
						m_view_size.height = view_h;
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
			const float dy = (value.y - m_pos.y) / m_ratio.height;
			if (fabs(dy) >= FLT_MIN) {
				const float rect_top = max(m_bm_rect.top + dy, 0.0f);
				m_bm_rect.top = min(rect_top, m_bm_rect.bottom);
				m_view_size.height = (m_bm_rect.bottom - m_bm_rect.top) * m_ratio.height;
				m_pos.y = value.y;
				flag = true;
			}
		}
		else if (anch == ANCH_TYPE::ANCH_NE) {
			if (m_bm_rect.left + FLT_MIN < m_bm_rect.right && m_bm_rect.top + FLT_MIN < m_bm_rect.bottom) {
				const float bm_w = m_bm_rect.right - m_bm_rect.left;
				const float bm_h = m_bm_rect.bottom - m_bm_rect.top;
				const D2D1_POINT_2F s_pos{ m_pos.x + m_view_size.width, m_pos.y };
				D2D1_POINT_2F pos;
				if (s_bm_keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x - bm_w, s_pos.y + bm_h };
					neighbor(value, s_pos, e_pos, pos);
				}
				else {
					pos = value;
				}
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					const float view_w = pos.x - m_pos.x;
					const float view_h = m_pos.y + m_view_size.height - pos.y;
					if (view_w >= 1.0f && view_h >= 1.0f) {
						m_view_size.width = view_w;
						m_view_size.height = view_h;
						m_pos.y = pos.y;
						m_ratio.width = view_w / bm_w;
						m_ratio.height = view_h / bm_h;
						flag = true;
					}
				}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_EAST) {
			const float dx = (value.x - (m_pos.x + m_view_size.width)) / m_ratio.width;
			if (fabs(dx) >= FLT_MIN) {
				const float rect_right = max(m_bm_rect.right + dx, m_bm_rect.left);
				m_bm_rect.right = min(rect_right, m_bm_size.width);
				m_view_size.width = (m_bm_rect.right - m_bm_rect.left) * m_ratio.width;
				m_pos.x = value.x - m_view_size.width;
				flag = true;
			}
		}
		else if (anch == ANCH_TYPE::ANCH_SE) {
			if (m_bm_rect.left + FLT_MIN < m_bm_rect.right && m_bm_rect.top + FLT_MIN < m_bm_rect.bottom) {
				const float bm_w = m_bm_rect.right - m_bm_rect.left;
				const float bm_h = m_bm_rect.bottom - m_bm_rect.top;
				const D2D1_POINT_2F s_pos{ m_pos.x + m_view_size.width, m_pos.y + m_view_size.height };
				D2D1_POINT_2F pos;
				if (s_bm_keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x - bm_w, s_pos.y - bm_h };
					neighbor(value, s_pos, e_pos, pos);
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
						m_view_size.width = view_w;
						m_view_size.height = view_h;
						m_ratio.width = view_w / bm_w;
						m_ratio.height = view_h / bm_h;
						flag = true;
					}
				}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_SOUTH) {
			const float dy = (value.y - (m_pos.y + m_view_size.height)) / m_ratio.height;
			if (fabs(dy) >= FLT_MIN) {
				const float rect_bottom = max(m_bm_rect.bottom + dy, m_bm_rect.top);
				m_bm_rect.bottom = min(rect_bottom, m_bm_size.height);
				m_view_size.height = (m_bm_rect.bottom - m_bm_rect.top) * m_ratio.height;
				m_pos.y = value.y - m_view_size.height;
				flag = true;
			}
		}
		else if (anch == ANCH_TYPE::ANCH_SW) {
			if (m_bm_rect.left + FLT_MIN < m_bm_rect.right && m_bm_rect.top + FLT_MIN < m_bm_rect.bottom) {
				const float bm_w = m_bm_rect.right - m_bm_rect.left;
				const float bm_h = m_bm_rect.bottom - m_bm_rect.top;
				const D2D1_POINT_2F s_pos{ m_pos.x, m_pos.y + m_view_size.height };
				D2D1_POINT_2F pos;
				if (s_bm_keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x + bm_w, s_pos.y - bm_h };
					neighbor(value, s_pos, e_pos, pos);
				}
				else {
					pos = value;
				}
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					const float view_w = m_pos.x + m_view_size.width - pos.x;
					const float view_h = pos.y - m_pos.y;
					if (view_w >= 1.0f && view_h >= 1.0f) {
						m_view_size.width = view_w;
						m_view_size.height = view_h;
						m_pos.x = pos.x;
						m_ratio.width = view_w / bm_w;
						m_ratio.height = view_h / bm_h;
						flag = true;
					}
				}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_WEST) {
			const float dx = (value.x - m_pos.x) / m_ratio.width;
			if (fabs(dx) >= FLT_MIN) {
				const float r_left = max(m_bm_rect.left + dx, 0.0f);
				m_bm_rect.left = min(r_left, m_bm_rect.right);
				m_view_size.width = (m_bm_rect.right - m_bm_rect.left) * m_ratio.width;
				m_pos.x = value.x;
				flag = true;
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

	// 値を選択フラグに格納する.
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
		bitmap_read_dib(dt_reader, m_bm_size, &m_bm_data);

		m_bm_rect.left = 0;
		m_bm_rect.top = 0;
		m_bm_rect.right = static_cast<FLOAT>(m_bm_size.width);
		m_bm_rect.bottom = static_cast<FLOAT>(m_bm_size.height);
		m_view_size.width = static_cast<FLOAT>(m_bm_size.width);
		m_view_size.height = static_cast<FLOAT>(m_bm_size.height);
		m_pos.x = c_pos.x - m_view_size.width * 0.5f;
		m_pos.y = c_pos.y - m_view_size.height * 0.5;
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
		dt_read(m_view_size, dt_reader);
		bitmap_read_rect(m_bm_rect, dt_reader);
		dt_read(m_bm_size, dt_reader);
		dt_read(m_ratio, dt_reader);

		const size_t row_size = 4 * m_bm_size.width;
		m_bm_data = new uint8_t[row_size * m_bm_size.height];
		std::vector<uint8_t> buf(row_size);
		for (size_t i = 0; i < m_bm_size.height; i++) {
			dt_reader.ReadBytes(buf);
			memcpy(m_bm_data + row_size * i, buf.data(), row_size);
		}
	}

	// データライターに書き込む.
	// dt_writer	データライター
	void ShapeBitmap::write(DataWriter const& dt_writer) const
	{
		dt_writer.WriteBoolean(m_is_deleted);
		dt_writer.WriteBoolean(m_is_selected);
		dt_write(m_pos, dt_writer);
		dt_write(m_view_size, dt_writer);
		bitmap_write_rect(m_bm_rect, dt_writer);
		dt_write(m_bm_size, dt_writer);
		dt_write(m_ratio, dt_writer);

		const size_t row_size = 4 * m_bm_size.width;
		std::vector<uint8_t> buf(row_size);
		for (size_t i = 0; i < m_bm_size.height; i++) {
			memcpy(buf.data(), m_bm_data + row_size * i, row_size);
			dt_writer.WriteBytes(buf);
		}
	}

}