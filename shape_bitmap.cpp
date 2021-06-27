#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	static void read_bitmap(DataReader const& dt_reader, D2D1_SIZE_U& b_size, uint8_t** buf)
	{
		using winrt::Windows::Storage::Streams::ByteOrder;

		// 2 バイトを読み込み, それが 'BM' か判定する.
		const char id_b = dt_reader.ReadByte();
		const char id_m = dt_reader.ReadByte();
		if (id_b == 'B' && id_m == 'M') {
			// データリーダーのバイトオーダーを得る.
			const auto b_order = dt_reader.ByteOrder();
			// バイトオーダーがリトルエンディアン以外か判定する.
			if (b_order != ByteOrder::LittleEndian) {
				dt_reader.ByteOrder(ByteOrder::LittleEndian);
			}
			uint32_t file_size = dt_reader.ReadUInt32();
			uint16_t reserve_0 = dt_reader.ReadUInt16();
			uint16_t reserve_1 = dt_reader.ReadUInt16();
			uint32_t data_offset = dt_reader.ReadUInt32();
			uint32_t header_len = dt_reader.ReadUInt32();
			uint32_t bitmap_w = 0;
			uint32_t bitmap_h = 0;
			uint16_t bit_per_pixel = 0;
			uint32_t image_size = 0;
			uint32_t comp_type = 0;
			uint32_t color_used = 0;
			uint8_t pallete[256][4];
			if (header_len == 0x28) {
				bitmap_w = dt_reader.ReadUInt32();
				bitmap_h = dt_reader.ReadUInt32();
				uint16_t plane_cnt = dt_reader.ReadUInt16();
				bit_per_pixel = dt_reader.ReadUInt16();
				comp_type = dt_reader.ReadUInt32();
				image_size = dt_reader.ReadUInt32();
				uint32_t res_w = dt_reader.ReadUInt32();
				uint32_t res_h = dt_reader.ReadUInt32();
				color_used = dt_reader.ReadUInt32();
				uint32_t color_important = dt_reader.ReadUInt32();
				for (size_t i = 0; i < color_used; i++) {
					pallete[i][0] = dt_reader.ReadByte();
					pallete[i][1] = dt_reader.ReadByte();
					pallete[i][2] = dt_reader.ReadByte();
					pallete[i][3] = dt_reader.ReadByte();
				}
			}
			else if (header_len == 0x0c) {
				bitmap_w = dt_reader.ReadUInt16();
				bitmap_h = dt_reader.ReadUInt16();
				uint16_t plane_cnt = dt_reader.ReadUInt16();
				bit_per_pixel = dt_reader.ReadUInt16();
				comp_type = 0;
				image_size = 0;	// 非圧縮の場合はゼロ.
				if (0 < bit_per_pixel && bit_per_pixel <= 8) {
					color_used = (1 << bit_per_pixel);
					for (size_t i = 0; i < color_used; i++) {
						pallete[i][0] = dt_reader.ReadByte();
						pallete[i][1] = dt_reader.ReadByte();
						pallete[i][2] = dt_reader.ReadByte();
						pallete[i][3] = 0xff;
					}
				}
			}
			const auto row_len = 4 * bitmap_w;
			const auto buf_len = row_len * bitmap_h;
			uint8_t* const buf_ptr = new uint8_t[buf_len];
			*buf = buf_ptr;
			if (comp_type == 0)	// BI_RGB, no pixel array compression used
			{
				std::vector<uint8_t> line_buf(row_len);
				for (size_t i = 1; i <= bitmap_h; i++) {
					dt_reader.ReadBytes(line_buf);
					memcpy(buf_ptr + buf_len - row_len * i, line_buf.data(), row_len);
				}
			}
			else if (image_size > 0) {
				size_t j = 0;
				size_t x = 0;
				size_t s = 0;
				for (size_t i = 0; i < image_size; i++) {
					const auto b = dt_reader.ReadByte();
					if (s == 0) {
						if (b == 0) {
							s = static_cast<size_t>(-1);
						}
						else {
							s = b;
						}
					}
					else if (s == static_cast<size_t>(-1)) {
						if (b == 0) {
							j = (j / row_len + 1) * row_len;
							s = 0;
						}
						else if (b == 1) {
							break;
						}
						else if (b == 2) {
							s = static_cast<size_t>(-2);
						}
						else if (b >= 3) {
							s = 512 + b - 3;
						}
					}
					else if (s == static_cast<size_t>(-2)) {
						x = b;
						s = static_cast<size_t>(-3);
					}
					else if (s == static_cast<size_t>(-3)) {
						j = b * row_len + x;
						s = 0;
					}
					else if (s > 512) {
						for (size_t k = 0; k < s && j < buf_len; k++, j += 4) {
							buf_ptr[j] = b;
						}
						s = 0;
					}
					else {
						for (size_t k = 0; k < s && j < buf_len; k++, j += 4) {
							buf_ptr[j] = b;
						}
						s = 0;
					}
				}
			}
			b_size.width = bitmap_w;
			b_size.height = bitmap_h;
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
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
					dx.m_logical_dpi,
					dx.m_logical_dpi
				)
			};
			dx.m_d2dContext->CreateBitmap(m_buf_size, static_cast<void*>(m_buf), 4 * m_buf_size.width, b_prop, m_dx_bitmap.put());
			if (m_dx_bitmap == nullptr) {
				return;
			}
		}
		D2D1_RECT_F dest{
			m_pos.x, m_pos.y,
			m_pos.x + m_size.width, m_pos.y + m_size.height
		};
		dx.m_d2dContext->DrawBitmap(m_dx_bitmap.get(), dest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, m_buf_rect);
		if (is_selected()) {
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
		if (anch == ANCH_TYPE::ANCH_P0) {
			value = m_pos;
		}
		else if (anch == ANCH_TYPE::ANCH_P0 + 1) {
			value.x = m_pos.x + m_size.width;
			value.y = m_pos.y;
		}
		else if (anch == ANCH_TYPE::ANCH_P0 + 2) {
			value.x = m_pos.x + m_size.width;
			value.y = m_pos.y + m_size.height;
		}
		else if (anch == ANCH_TYPE::ANCH_P0 + 3) {
			value.x = m_pos.x;
			value.y = m_pos.y + m_size.height;
		}
	}

	// 図形を囲む領域を得る.
	void ShapeBitmap::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		D2D1_POINT_2F v_pos[2]{
			m_pos,
			{ m_pos.x + m_size.width, m_pos.y + m_size.height }
		};
		pt_bound(v_pos[0], v_pos[1], v_pos[0], v_pos[1]);
		pt_min(a_min, v_pos[0], b_min);
		pt_max(a_max, v_pos[1], b_max);
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

	uint32_t ShapeBitmap::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		D2D1_POINT_2F v_pos[4]{
			{ m_pos.x, m_pos.y },
			{ m_pos.x + m_size.width, m_pos.y },
			{ m_pos.x + m_size.width, m_pos.y + m_size.height },
			{ m_pos.x, m_pos.y + m_size.height }
		};
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
		}
		if (m_pos.x <= t_pos.x && t_pos.x <= m_pos.x + m_size.width &&
			m_pos.y <= t_pos.y && t_pos.y <= m_pos.y + m_size.height) {
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

	// 値を, 部位の位置に格納する.
	bool ShapeBitmap::set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float dist)
	{
		if (anch == ANCH_TYPE::ANCH_NW) {
			m_size.width = m_size.width - (value.x - m_pos.x);
			m_size.height = m_size.height - (value.y - m_pos.y);
			m_pos = value;
		}
		else if (anch == ANCH_TYPE::ANCH_NORTH) {
			if (m_size.width >= FLT_MIN) {
				const auto s = m_buf_size.width / m_size.width;
				m_buf_rect.top = max((value.y - m_pos.y) * s, 0.0f);
				m_size.height = m_size.height - (value.y - m_pos.y);
				m_pos.y = value.y;
			}
		}
		else if (anch == ANCH_TYPE::ANCH_NE) {
			m_size.width = value.x - m_pos.x;
			m_size.height = m_pos.y + m_size.height - value.y;
			m_pos.y = value.y;
		}
		else if (anch == ANCH_TYPE::ANCH_EAST) {
			m_buf_rect.right = max(value.y - m_pos.y, 0,0f);
		}
		else if (anch == ANCH_TYPE::ANCH_SE) {
			m_size.width = value.x - m_pos.x;
			m_size.height = value.y - m_pos.y;
		}
		else if (anch == ANCH_TYPE::ANCH_SW) {
			m_size.width = m_pos.x + m_size.width - value.x;
			m_size.height = value.y - m_pos.y;
			m_pos.x = value.x;
		}
		else if (anch == ANCH_TYPE::ANCH_WEST) {

		}
		else {
			return false;
		}
		return true;
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
		read_bitmap(dt_reader, m_buf_size, &m_buf);

		m_buf_rect.left = 0;
		m_buf_rect.top = 0;
		m_buf_rect.right = m_buf_size.width;
		m_buf_rect.bottom = m_buf_size.height;
		m_size.width = m_buf_size.width;
		m_size.height = m_buf_size.height;
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