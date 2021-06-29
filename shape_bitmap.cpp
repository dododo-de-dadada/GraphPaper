#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
#define NIBBLE_HI(b)	((b >> 4) & 0x0f)
#define NIBBLE_LO(b)	(b & 0x0f)

	static void bitmap_read(DataReader const& dt_reader, D2D1_SIZE_U& b_size, uint8_t** buf)
	{
		using winrt::Windows::Storage::Streams::ByteOrder;

		// 2 バイトを読み込み, それが 'BM' か判定する.
		const char bf_type1 = dt_reader.ReadByte();
		const char bf_type2 = dt_reader.ReadByte();
		if (bf_type1 == 'B' && bf_type2 == 'M') {
			// データリーダーのバイトオーダーを得る.
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
					pallete[i][3] = dt_reader.ReadByte();	// A
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
				}
			}
			const size_t row_len = 4 * bi_width;;
			const auto buf_len = row_len * bi_height;
			uint8_t* const buf_ptr = new uint8_t[buf_len];
			*buf = buf_ptr;
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
									buf_ptr[j + 0] = pallete[b1][0];
									buf_ptr[j + 1] = pallete[b1][1];
									buf_ptr[j + 2] = pallete[b1][2];
									buf_ptr[j + 3] = pallete[b1][3];	// 0
									j += 4;
								}
								else if (bi_compress == BI_RLE4 && j + 8 <= buf_len) {
									const auto nh = NIBBLE_HI(b1);
									const auto nl = NIBBLE_LO(b1);
									buf_ptr[j + 0] = pallete[nh][0];
									buf_ptr[j + 1] = pallete[nh][1];
									buf_ptr[j + 2] = pallete[nh][2];
									buf_ptr[j + 3] = pallete[nh][3];	// 0
									buf_ptr[j + 4] = pallete[nl][0];
									buf_ptr[j + 5] = pallete[nl][1];
									buf_ptr[j + 6] = pallete[nl][2];
									buf_ptr[j + 7] = pallete[nl][3];	// 0
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
							buf_ptr[j + 0] = pallete[b0][0];
							buf_ptr[j + 1] = pallete[b0][1];
							buf_ptr[j + 2] = pallete[b0][2];
							buf_ptr[j + 3] = pallete[b0][3];
							j += 4;
						}
						else if (bi_compress == BI_RLE4 && j + 8 <= buf_len) {
							const auto nh = NIBBLE_HI(b0);
							const auto nl = NIBBLE_LO(b0);
							buf_ptr[j + 0] = pallete[nh][0];
							buf_ptr[j + 1] = pallete[nh][1];
							buf_ptr[j + 2] = pallete[nh][2];
							buf_ptr[j + 3] = pallete[nh][3];
							buf_ptr[j + 4] = pallete[nl][0];
							buf_ptr[j + 5] = pallete[nl][1];
							buf_ptr[j + 6] = pallete[nl][2];
							buf_ptr[j + 7] = pallete[nl][3];
							j += 8;
						}
						s = 0;
					}
					else {
						if (bi_compress == BI_RLE8 && j + 8 <= buf_len) {
							buf_ptr[j + 0] = pallete[b0][0];
							buf_ptr[j + 1] = pallete[b0][1];
							buf_ptr[j + 2] = pallete[b0][2];
							buf_ptr[j + 3] = pallete[b0][3];
							buf_ptr[j + 4] = pallete[b1][0];
							buf_ptr[j + 5] = pallete[b1][1];
							buf_ptr[j + 6] = pallete[b1][2];
							buf_ptr[j + 7] = pallete[b1][3];
							j += 8;
							s -= 2;
						}
						else if (bi_compress == BI_RLE4 && j + 16 <= buf_len) {
							const auto b0_nh = NIBBLE_HI(b0);
							const auto b0_nl = NIBBLE_LO(b0);
							const auto b1_nh = NIBBLE_HI(b1);
							const auto b1_nl = NIBBLE_LO(b1);
							buf_ptr[j + 0] = pallete[b0_nh][0];
							buf_ptr[j + 1] = pallete[b0_nh][1];
							buf_ptr[j + 2] = pallete[b0_nh][2];
							buf_ptr[j + 3] = pallete[b0_nh][3];
							buf_ptr[j + 4] = pallete[b0_nl][0];
							buf_ptr[j + 5] = pallete[b0_nl][1];
							buf_ptr[j + 6] = pallete[b0_nl][2];
							buf_ptr[j + 7] = pallete[b0_nl][3];
							buf_ptr[j + 8] = pallete[b1_nh][0];
							buf_ptr[j + 9] = pallete[b1_nh][1];
							buf_ptr[j + 10] = pallete[b1_nh][2];
							buf_ptr[j + 11] = pallete[b1_nh][3];
							buf_ptr[j + 12] = pallete[b1_nl][0];
							buf_ptr[j + 13] = pallete[b1_nl][1];
							buf_ptr[j + 14] = pallete[b1_nl][2];
							buf_ptr[j + 15] = pallete[b1_nl][3];
							j += 16;
							s -= 4;
						}
					}
				}
			}
			b_size.width = bi_width;
			b_size.height = bi_height;
			// バイトオーダーがリトルエンディアン以外か判定する.
			if (b_order != ByteOrder::LittleEndian) {
				// バイトオーダーを元に戻す.
				dt_reader.ByteOrder(b_order);
			}
		}
	}

	ShapeBitmap::~ShapeBitmap(void)
	{
		if (m_buf != nullptr) {
			delete m_buf;
			m_buf = nullptr;
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
					//D2D1_BITMAP_OPTIONS_NONE | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
					//96.0f,
					//96.0f
				)
			};
			dx.m_d2dContext->CreateBitmap(m_buf_size, static_cast<void*>(m_buf), 4 * m_buf_size.width, b_prop, m_dx_bitmap.put());
			if (m_dx_bitmap == nullptr) {
				return;
			}
		}
		D2D1_RECT_F dest_rect{
			m_pos.x,
			m_pos.y,
			m_pos.x + m_size.width,
			m_pos.y + m_size.height
		};
		dx.m_d2dContext->DrawBitmap(m_dx_bitmap.get(), dest_rect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, m_buf_rect);

		if (is_selected()) {
			dx.m_shape_brush->SetColor(Shape::m_default_background);
			dx.m_d2dContext->DrawRectangle(dest_rect, dx.m_shape_brush.get(), 1.0f, nullptr);
			dx.m_shape_brush->SetColor(Shape::m_default_foreground);
			dx.m_d2dContext->DrawRectangle(dest_rect, dx.m_shape_brush.get(), 1.0f, dx.m_aux_style.get());

			D2D1_POINT_2F v_pos[4]{
				m_pos,
				{ m_pos.x + m_size.width, m_pos.y },
				{ m_pos.x + m_size.width, m_pos.y + m_size.height },
				{ m_pos.x, m_pos.y + m_size.height },
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
			value.x = m_pos.x + m_size.width * 0.5f;
			value.y = m_pos.y;
		}
		else if (anch == ANCH_TYPE::ANCH_NE) {
			value.x = m_pos.x + m_size.width;
			value.y = m_pos.y;
		}
		else if (anch == ANCH_TYPE::ANCH_EAST) {
			value.x = m_pos.x + m_size.width;
			value.y = m_pos.y + m_size.height * 0.5f;
		}
		else if (anch == ANCH_TYPE::ANCH_SE) {
			value.x = m_pos.x + m_size.width;
			value.y = m_pos.y + m_size.height;
		}
		else if (anch == ANCH_TYPE::ANCH_SOUTH) {
			value.x = m_pos.x + m_size.width * 0.5f;
			value.y = m_pos.y + m_size.height;
		}
		else if (anch == ANCH_TYPE::ANCH_SW) {
			value.x = m_pos.x;
			value.y = m_pos.y + m_size.height;
		}
		else if (anch == ANCH_TYPE::ANCH_WEST) {
			value.x = m_pos.x;
			value.y = m_pos.y + m_size.height * 0.5f;
		}
	}

	// 図形を囲む領域を得る.
	void ShapeBitmap::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		D2D1_POINT_2F b_pos[2]{
			m_pos,
			{ m_pos.x + m_size.width, m_pos.y + m_size.height }
		};
		pt_bound(b_pos[0], b_pos[1], b_pos[0], b_pos[1]);
		pt_min(a_min, b_pos[0], b_min);
		pt_max(a_max, b_pos[1], b_max);
	}

	// 近傍の頂点を得る.
	bool ShapeBitmap::get_neighbor(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& value) const noexcept
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
			{ m_pos.x + m_size.width, m_pos.y + m_size.height }
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
		v_pos[1] = D2D1_POINT_2F{ m_pos.x + m_size.width, m_pos.y };
		v_pos[2] = D2D1_POINT_2F{ m_pos.x + m_size.width, m_pos.y + m_size.height };
		v_pos[3] = D2D1_POINT_2F{ m_pos.x, m_pos.y + m_size.height };
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
			D2D1_POINT_2F ap;
			D2D1_POINT_2F ab;
			pt_sub(p, a, ap);
			pt_sub(b, a, ab);
			const double ap_ab = ap.x * ab.x + ap.y * ab.y;
			const double ab_ab = ab.x * ab.x + ab.y * ab.y;
			pt_mul(ab, ap_ab / ab_ab, a, q);
		}
	}

	// 値を, 部位の位置に格納する.
	bool ShapeBitmap::set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float dist)
	{
		bool flag = false;
		if (anch == ANCH_TYPE::ANCH_NW) {
			const float bw = (m_buf_rect.right == m_buf_rect.left ? m_buf_size.width : (m_buf_rect.right - m_buf_rect.left));
			const float bh = (m_buf_rect.bottom == m_buf_rect.top ? m_buf_size.height : (m_buf_rect.bottom - m_buf_rect.top));
			D2D1_POINT_2F v_pos[2]{
				m_pos,
				{ m_pos.x + bw, m_pos.y + bh }
			};
			D2D1_POINT_2F val;
			neighbor(value, v_pos[0], v_pos[1], val);
			D2D1_POINT_2F v_vec;
			pt_sub(val, v_pos[0], v_vec);
			if (pt_abs2(v_vec) >= FLT_MIN) {
				const float sw = m_size.width - v_vec.x;
				const float sh = m_size.height - v_vec.y;
				if (sw >= 1.0f && sh >= 1.0f) {
					m_size.width = sw;
					m_size.height = sh;
					m_pos = val;
					m_scale_x = sw / bw;
					m_scale_y = sh / bh;
					flag = true;
				}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_NORTH) {
			float vy = (value.y - m_pos.y) / m_scale_y;
			if (fabs(vy) >= FLT_MIN) {
				const float sh = (m_buf_rect.bottom - m_buf_rect.top) * m_scale_y;
				//if (sh >= 1.0f) {
					m_buf_rect.top = max(m_buf_rect.top + vy, 0.0f);
					m_size.height = sh;
					m_pos.y = value.y;
					flag = true;
				//}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_NE) {
			const float bw = (m_buf_rect.right == m_buf_rect.left ? m_buf_size.width : (m_buf_rect.right - m_buf_rect.left));
			const float bh = (m_buf_rect.bottom == m_buf_rect.top ? m_buf_size.height : (m_buf_rect.bottom - m_buf_rect.top));
			D2D1_POINT_2F v_pos[2]{
				{ m_pos.x + m_size.width, m_pos.y },
				{ m_pos.x + m_size.width - bw, m_pos.y + bh }
			};
			D2D1_POINT_2F val;
			neighbor(value, v_pos[0], v_pos[1], val);
			D2D1_POINT_2F v_vec;
			pt_sub(val, v_pos[0], v_vec);
			if (pt_abs2(v_vec) >= FLT_MIN) {
				const float sw = val.x - m_pos.x;
				const float sh = m_pos.y + m_size.height - val.y;
				if (sw >= 1.0f && sh >= 1.0f) {
					m_size.width = sw;
					m_size.height = sh;
					m_pos.y = val.y;
					m_scale_x = sw / bw;
					m_scale_y = sh / bh;
					flag = true;
				}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_EAST) {
			float vx = (value.x - (m_pos.x + m_size.width)) / m_scale_x;
			if (fabs(vx) >= FLT_MIN) {
				const float sw = (m_buf_rect.right - m_buf_rect.left) * m_scale_x;
				//if (sw >= 1.0f) {
					m_buf_rect.right = min(m_buf_rect.right + vx, m_buf_size.width);
					m_size.width = sw;
					m_pos.x = value.x - sw;
					flag = true;
				//}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_SE) {
			const float bw = (m_buf_rect.right == m_buf_rect.left ? m_buf_size.width : (m_buf_rect.right - m_buf_rect.left));
			const float bh = (m_buf_rect.bottom == m_buf_rect.top ? m_buf_size.height : (m_buf_rect.bottom - m_buf_rect.top));
			D2D1_POINT_2F v_pos[2]{
				{ m_pos.x + m_size.width, m_pos.y + m_size.height },
				{ m_pos.x + m_size.width - bw, m_pos.y + m_size.height - bh }
			};
			D2D1_POINT_2F val;
			neighbor(value, v_pos[0], v_pos[1], val);
			D2D1_POINT_2F v_vec;
			pt_sub(val, v_pos[0], v_vec);
			if (pt_abs2(v_vec) >= FLT_MIN) {
				const float sw = val.x - m_pos.x;
				const float sh = val.y - m_pos.y;
				if (sw >= 1.0f && sh >= 1.0f) {
					m_size.width = sw;
					m_size.height = sh;
					m_scale_x = sw / bw;
					m_scale_y = sh / bh;
					flag = true;
				}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_SOUTH) {
			float vy = (value.y - (m_pos.y + m_size.height)) / m_scale_y;
			if (fabs(vy) >= FLT_MIN) {
				const float sh = (m_buf_rect.bottom - m_buf_rect.top) * m_scale_y;
				//if (sh >= 1.0f) {
					m_buf_rect.bottom = min(m_buf_rect.bottom + vy, m_buf_size.height);
					m_size.height = sh;
					m_pos.y = value.y - sh;
					flag = true;
				//}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_SW) {
			const float bw = (m_buf_rect.right == m_buf_rect.left ? m_buf_size.width : (m_buf_rect.right - m_buf_rect.left));
			const float bh = (m_buf_rect.bottom == m_buf_rect.top ? m_buf_size.height : (m_buf_rect.bottom - m_buf_rect.top));
			D2D1_POINT_2F v_pos[2]{
				{ m_pos.x, m_pos.y + m_size.height },
				{ m_pos.x + bw, m_pos.y + m_size.height - bh }
			};
			D2D1_POINT_2F val;
			neighbor(value, v_pos[0], v_pos[1], val); 
			D2D1_POINT_2F v_vec;
			pt_sub(val, v_pos[0], v_vec);
			if (pt_abs2(v_vec) >= FLT_MIN) {
				const float sw = m_pos.x + m_size.width - val.x;
				const float sh = val.y - m_pos.y;
				if (sw >= 1.0f && sh >= 1.0f) {
					m_size.width = sw;
					m_size.height = sh;
					m_pos.x = val.x;
					m_scale_x = sw / bw;
					m_scale_y = sh / bh;
					flag = true;
				}
			}
		}
		else if (anch == ANCH_TYPE::ANCH_WEST) {
			float vx = (value.x - m_pos.x) / m_scale_x;
			if (fabs(vx) >= FLT_MIN) {
				const float sw = (m_buf_rect.right - m_buf_rect.left) * m_scale_x;
				//if (sw >= 1.0f) {
					m_buf_rect.left = max(m_buf_rect.left + (value.x - m_pos.x) / m_scale_x, 0.0f);
					m_size.width = sw;
					m_pos.x = value.x;
					flag = true;
				//}
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

	ShapeBitmap::ShapeBitmap(const D2D1_POINT_2F c_pos, DataReader const& dt_reader)
	{
		bitmap_read(dt_reader, m_buf_size, &m_buf);

		m_buf_rect.left = 0;
		m_buf_rect.top = 0;
		m_buf_rect.right = static_cast<FLOAT>(m_buf_size.width);
		m_buf_rect.bottom = static_cast<FLOAT>(m_buf_size.height);
		m_size.width = static_cast<FLOAT>(m_buf_size.width);
		m_size.height = static_cast<FLOAT>(m_buf_size.height);
		m_pos.x = c_pos.x - m_size.width * 0.5;
		m_pos.y = c_pos.y - m_size.height * 0.5;
	}

	// コンストラクター
	ShapeBitmap::ShapeBitmap(DataReader const& dt_reader)
	{
		m_is_deleted = dt_reader.ReadBoolean();
		m_is_selected = dt_reader.ReadBoolean();
		dt_read(m_pos, dt_reader);
		dt_read(m_size, dt_reader);
		dt_read(m_buf_size, dt_reader);

		const size_t row_size = 4 * m_buf_size.width;
		m_buf = new uint8_t[row_size * m_buf_size.height];
		std::vector<uint8_t> buf(row_size);
		for (size_t i = 0; i < m_buf_size.height; i++) {
			dt_reader.ReadBytes(buf);
			memcpy(m_buf + row_size * i, buf.data(), row_size);
		}
	}

	void ShapeBitmap::write(DataWriter const& dt_writer) const
	{
		dt_writer.WriteBoolean(m_is_deleted);
		dt_writer.WriteBoolean(m_is_selected);
		dt_write(m_pos, dt_writer);
		dt_write(m_size, dt_writer);
		dt_write(m_buf_size, dt_writer);

		const size_t row_size = 4 * m_buf_size.width;
		std::vector<uint8_t> buf(row_size);
		for (size_t i = 0; i < m_buf_size.height; i++) {
			memcpy(buf.data(), m_buf + row_size * i, row_size);
			dt_writer.WriteBytes(buf);
		}
	}

}