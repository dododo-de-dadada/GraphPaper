#include <time.h>
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
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

	// Adler-32 �`�F�b�N�T��
	// Z �X�g���[���̃w�b�_�[����������.
	struct ADLER32 {
		static constexpr uint32_t INIT = 1;
		static uint32_t update(const uint32_t adler, const uint8_t b) noexcept
		{
			constexpr uint32_t ADLER32_BASE = 65521;
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

	// CRC-32 �`�F�b�N�T��
	// PNG �̃`�����N�f�[�^����������
	struct CRC32 {
		static constexpr uint32_t INIT = 0xffffffff;
		static uint32_t table[256];
		static void init(void)
		{
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
		static uint32_t update(const uint32_t crc, const uint8_t b) noexcept
		{
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
	uint32_t CRC32::table[256];

	// D2D1_RECT_F ���� D2D1_RECT_U ���쐬����.
	static const D2D1_RECT_U image_conv_rect(const D2D1_RECT_F& f, const D2D1_SIZE_U s);
	// �_�ɍł��߂�, ������̓_�����߂�.
	static void image_get_pos_on_line(const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& q);
	// �f�[�^���[�_�[���� DIB ��ǂݍ���.
	static void image_read_dib(DataReader const& dt_reader, D2D1_SIZE_U& b_size, uint8_t** buf);

	// D2D1_RECT_F ���� D2D1_RECT_U ���쐬����.
	// ����ʒu�� { 0, 0 } �ȏ�, �E���ʒu�� D2D1_SIZE_U �ȉ�.
	static const D2D1_RECT_U image_conv_rect(const D2D1_RECT_F& f, const D2D1_SIZE_U s)
	{
		return D2D1_RECT_U{
			max(static_cast<uint32_t>(floor(f.left)), 0),
			max(static_cast<uint32_t>(floor(f.top)), 0),
			min(static_cast<uint32_t>(ceil(f.right)), s.width),
			min(static_cast<uint32_t>(ceil(f.bottom)), s.height)
		};
	}

	// �_�ɍł��߂�, ������̓_�����߂�.
	static void image_get_pos_on_line(const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& q)
	{
		// ���� ab �����������肷��.
		if (a.x == b.x) {
			q.x = a.x;
			q.y = p.y;
		}
		// ���� ab �����������肷��.
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

	// �f�[�^���[�_�[���� DIB �f�[�^��ǂݍ���.
	static void image_read_dib(DataReader const& dt_reader, D2D1_SIZE_U& dib_size, uint8_t** dib_data)
	{
		using winrt::Windows::Storage::Streams::ByteOrder;

		// 2 �o�C�g��ǂݍ���, ���ꂪ 'BM' �����肷��.
		const char bf_type1 = dt_reader.ReadByte();
		const char bf_type2 = dt_reader.ReadByte();
		if (bf_type1 == 'B' && bf_type2 == 'M') {
			// �o�C�g�I�[�_�[�����g���G���f�B�A���ȊO�����肷��.
			const auto b_order = dt_reader.ByteOrder();
			if (b_order != ByteOrder::LittleEndian) {
				dt_reader.ByteOrder(ByteOrder::LittleEndian);
			}
			uint32_t bf_size = dt_reader.ReadUInt32();	// �t�@�C���S�̂̃T�C�Y
			uint16_t bf_reserved_0 = dt_reader.ReadUInt16();
			uint16_t bf_reserved_1 = dt_reader.ReadUInt16();
			uint32_t bf_offset = dt_reader.ReadUInt32();	// �t�@�C���擪����摜�f�[�^�܂ł̃I�t�Z�b�g (54 + �p���b�g)
			uint32_t bi_size = dt_reader.ReadUInt32();	// BITMAOINFOHEADER = 40 �܂��� BITMAPCOREHEADER = 12 �̃T�C�Y
			uint32_t bi_width = 0;
			uint32_t bi_height = 0;
			uint16_t bi_bit_cnt = 0;
			uint32_t bi_compress = 0;
			uint32_t bi_size_img = 0;
			uint32_t bi_color_used = 0;
			uint8_t pallete[256][4];
			// BITMAOINFOHEADER �����肷��.
			if (bi_size == 0x28) {
				bi_width = dt_reader.ReadUInt32();
				bi_height = dt_reader.ReadUInt32();
				uint16_t bi_planes = dt_reader.ReadUInt16();	// �v���[���̐�. ���Ȃ炸 1
				bi_bit_cnt = dt_reader.ReadUInt16();	// �F�� 1=���m�N��, 4=16�F, 8=256�F, 24�܂���32=�t���J���[
				bi_compress = dt_reader.ReadUInt32();	// ���k�^�C�v. (0=BI_RGB, 1=BI_RLE8, 2=BI_RLE4, 3=BI_BITFIELDS)
				bi_size_img = dt_reader.ReadUInt32();	// ���k���ꂽ�摜�f�[�^�̑傫�� (�񈳏k=BI_RGB �Ȃ�[��, �Ƃ͌���Ȃ�!)
				uint32_t x_px_per_meter = dt_reader.ReadUInt32();	// 96dpi�̂Ƃ� 3780. �ӂ��̓[��.
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
			// BITMAPCOREHEADER �����肷��.
			else if (bi_size == 0x0c) {
				bi_width = dt_reader.ReadUInt16();
				bi_height = dt_reader.ReadUInt16();
				uint16_t bi_planes = dt_reader.ReadUInt16();	// �v���[���̐�. ���Ȃ炸 1
				bi_bit_cnt = dt_reader.ReadUInt16();
				bi_compress = 0;
				bi_size_img = 0;	// �񈳏k�̏ꍇ�̓[��.
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
			// �o�C�g�I�[�_�[�����g���G���f�B�A���ȊO�����肷��.
			if (b_order != ByteOrder::LittleEndian) {
				// �o�C�g�I�[�_�[�����ɖ߂�.
				dt_reader.ByteOrder(b_order);
			}
		}
	}

	// �}�`��j������.
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

	// �}�`��\������.
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

	// �}�`���͂ޗ̈�𓾂�.
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

	// �摜�̕s�����x�𓾂�.
	bool ShapeImage::get_image_opacity(float& value) const noexcept
	{
		value = m_opac;
		return true;
	}

	// ���ʂ̈ʒu�𓾂�.
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

	// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
	void ShapeImage::get_pos_min(D2D1_POINT_2F& value) const noexcept
	{
		const D2D1_POINT_2F v_pos[2]{
			m_pos,
			{ m_pos.x + m_view.width, m_pos.y + m_view.height }
		};
		pt_min(v_pos[0], v_pos[1], value);
	}

	// �ߖT�̒��_�𓾂�.
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

	// �J�n�ʒu�𓾂�.
	bool ShapeImage::get_pos_start(D2D1_POINT_2F& value) const noexcept
	{
		value = m_pos;
		return true;
	}

	// ���_�𓾂�.
	size_t ShapeImage::get_verts(D2D1_POINT_2F v_pos[]) const noexcept
	{
		v_pos[0] = m_pos;
		v_pos[1] = D2D1_POINT_2F{ m_pos.x + m_view.width, m_pos.y };
		v_pos[2] = D2D1_POINT_2F{ m_pos.x + m_view.width, m_pos.y + m_view.height };
		v_pos[3] = D2D1_POINT_2F{ m_pos.x, m_pos.y + m_view.height };
		return 4;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���. �܂܂Ȃ��Ƃ��́u�}�`�̊O���v��Ԃ�.
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

	// ���������ړ�����.
	bool ShapeImage::move(const D2D1_POINT_2F value)
	{
		pt_add(m_pos, value, m_pos);
		return true;
	}

	// ���̑傫���ɖ߂�.
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

	// �l���������ꂽ������Ɋi�[����.
	bool ShapeImage::set_delete(const bool value) noexcept
	{
		m_is_deleted = value;
		return true;
	}

	// �l���摜�̕s�����x�Ɋi�[����.
	bool ShapeImage::set_image_opacity(const float value) noexcept
	{
		if (!equal(m_opac, value)) {
			m_opac = value;
			return true;
		}
		return false;
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����.
	// value	�l
	// anch	�}�`�̕���
	// limit	���E���� (���̒��_�Ƃ̋��������̒l�����ɂȂ�Ȃ�, ���̒��_�Ɉʒu�ɍ��킹��)
	// keep_aspect	�摜�}�`�̏c������ێ�
	bool ShapeImage::set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float limit, const bool keep_aspect)
	{
		bool flag = false;
		if (anch == ANCH_TYPE::ANCH_NW) {
			// �摜�̈ꕔ���ł��\������Ă��邩���肷��.
			// (�摜���܂������\������ĂȂ��ꍇ�̓X�P�[���̕ύX�͍s��Ȃ�.)
			const float bm_w = m_rect.right - m_rect.left;	// �\������Ă���摜�̕� (����)
			const float bm_h = m_rect.bottom - m_rect.top;	// �\������Ă���摜�̍��� (����)
			if (bm_w > 1.0f && bm_h > 1.0f) {
				const D2D1_POINT_2F s_pos{ m_pos };	// �n�_ (�}�`�̒��_)
				D2D1_POINT_2F pos;
				if (keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x + bm_w, s_pos.y + bm_h };	// �I�_ (�n�_�ƑΊp�ɂ���摜��̓_)
					image_get_pos_on_line(value, s_pos, e_pos, pos);
				}
				else {
					pos = value;
				}
				// �l�Ǝn�_�Ƃ̍���������, �������[�����傫�������肷��.
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					// �X�P�[���ύX��̕\���̐��@������, ���̏c���� 1 �s�N�Z���ȏ゠�邩���肷��.
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
			// �ύX���鍷�������߂�.
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

	// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
	bool ShapeImage::set_pos_start(const D2D1_POINT_2F value)
	{
		m_pos = value;
		return true;
	}

	// �l��I������Ă邩����Ɋi�[����.
	bool ShapeImage::set_select(const bool value) noexcept
	{
		m_is_selected = value;
		return true;
	}

	// �f�[�^���[�_�[����ǂݍ���
	// c_pos	�摜��z�u���钆�S�̈ʒu
	// dt_reader	�f�[�^���[�_�[
	ShapeImage::ShapeImage(const D2D1_POINT_2F c_pos, DataReader const& dt_reader)		
	{
		image_read_dib(dt_reader, m_size, &m_data);

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

	// �f�[�^���[�_�[����ǂݍ���
	// dt_reader	�f�[�^���[�_�[
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

	// �f�[�^���C�^�[�ɏ�������.
	// dt_writer	�f�[�^���C�^�[
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

	// �f�[�^���C�^�[�� DIB �Ƃ��ĉ摜�f�[�^����������.
	void ShapeImage::write_bmp(DataWriter const& dt_writer) const
	{
		using winrt::Windows::Storage::Streams::ByteOrder;

		constexpr std::array<uint8_t, 2> bf_type{ 'B', 'M' };
		constexpr uint32_t bi_size = 0x28;	// BITMAOINFOHEADER �̃T�C�Y (40 �o�C�g)
		const D2D1_RECT_U rect{ image_conv_rect(m_rect, m_size) };
		const uint32_t bi_width = rect.right - rect.left;
		const uint32_t bi_height = rect.bottom - rect.top;
		if (bi_width == 0 || bi_height == 0) {
			return;
		}
		const uint32_t data_width = 4 * m_size.width;
		const uint32_t bf_size = 12 + bi_size + bi_height * 4 * bi_width;
		dt_writer.ByteOrder(ByteOrder::LittleEndian);
		dt_writer.WriteBytes(bf_type);	// bf_type: �t�@�C���^�C�v
		dt_writer.WriteUInt32(bf_size);	// bf_size: �t�@�C���S�̂̃T�C�Y.
		dt_writer.WriteUInt16(0);	// bf_reserved1: �\�� 1 = 0
		dt_writer.WriteUInt16(0);	// bf_reserved2: �\�� 2 = 0
		dt_writer.WriteUInt32(54);	// bf_off_bits: �t�@�C���擪����摜�f�[�^�܂ł̃I�t�Z�b�g (54 + �p���b�g)
		dt_writer.WriteUInt32(bi_size);	// bi_size: BITMAOINFOHEADER �̃T�C�Y = 40
		dt_writer.WriteUInt32(bi_width);	// bi_width: �摜�̕�[�s�N�Z��]
		dt_writer.WriteUInt32(bi_height);	// bi_height: �摜�̍���[�s�N�Z��]
		dt_writer.WriteUInt16(1);	// bi_planes: �v���[����. ���Ȃ炸 1
		dt_writer.WriteUInt16(32);	// bi_bit_cnt: �F�r�b�g��[bit]
		dt_writer.WriteUInt32(0);	// bi_compress: ���kk�`��. (0=BI_RGB, 1=BI_RLE8, 2=BI_RLE4, 3=BI_BITFIELDS)
		dt_writer.WriteUInt32(0);	// bi_size_img: �摜�f�[�^�T�C�Y[byte]
		dt_writer.WriteUInt32(0);	// x_px_per_meter: �����𑜓x[dot/m]. 96dpi�̂Ƃ� 3780. �ӂ��̓[��.
		dt_writer.WriteUInt32(0);	// y_px_per_meter: �����𑜓x[dot/m]. �ӂ��̓[��
		dt_writer.WriteUInt32(0);	// bi_color_used: �i�[�p���b�g��[�g�p�F��]. �ӂ��̓[��	
		dt_writer.WriteUInt32(0);	// bi_color_important: 	�d�v�F��. �ӂ��̓[��
		for (size_t y = rect.bottom; y != rect.top; y--) {
			for (size_t x = rect.left; x < rect.right; x++) {
				const std::array<uint8_t, 4> bgra{
					m_data[(y - 1) * data_width + 4 * x + 0],
					m_data[(y - 1) * data_width + 4 * x + 1],
					m_data[(y - 1) * data_width + 4 * x + 2],
					m_data[(y - 1) * data_width + 4 * x + 3],
				};
				dt_writer.WriteBytes(bgra);
			}
		}
	}

	// �f�[�^���C�^�[�� PNG �Ƃ��ĉ摜�f�[�^����������.
	// #define SET_BYTE4(ptr, u) { (ptr)[0] = static_cast<uint8_t>(((u) >> 24) & 0xff); (ptr)[1] = static_cast<uint8_t>(((u) >> 16) & 0xff); (ptr)[2] = static_cast<uint8_t>(((u) >> 8) & 0xff); (ptr)[3] = static_cast<uint8_t>((u) & 0xff);}
	// PNG (Portable Network Graphics) Specification Version 1.0
	// RFC 1950 ZLIB Compressed Data Format Specification version 3.3 ���{���
	// RFC 1951 DEFLATE Compressed Data Format Specification version 1.3 ���{���
	void ShapeImage::write_png(DataWriter const& dt_writer) const
	{
		using winrt::Windows::Storage::Streams::ByteOrder;
		dt_writer.ByteOrder(ByteOrder::BigEndian);	// �l�b�g���[�N�o�C�g�I�[�_�[

		// CRC32 �̏�����
		static bool crc_computed = false;
		if (!crc_computed) {
			crc_computed = true;
			CRC32::init();
		}

		constexpr std::array<uint8_t, 8> PNG_HEADER{ 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
		constexpr std::array<uint8_t, 4> IHDR_TYPE{ 'I', 'H', 'D', 'R' };
		constexpr std::array<uint8_t, 4> IDAT_TYPE{ 'I', 'D', 'A', 'T' };
		constexpr std::array<uint8_t, 4> IEND_TYPE{ 'I', 'E', 'N', 'D' };
		constexpr std::array<uint8_t, 2> Z_CMF{ 72, 13 };	// ���k�����ƃt���O
		constexpr uint8_t PNG_FILTER_TYPE = 0;

		// �摜�̋�`
		const D2D1_RECT_U rect{ image_conv_rect(m_rect, m_size) };
		const auto rect_width = rect.right - rect.left;	// �摜�̃s�N�Z����
		const auto rect_height = rect.bottom - rect.top;	// �摜�̃s�N�Z������
		const auto data_width = 4ull * m_size.width;	// �摜�f�[�^�̃o�C�g��

		// PNG �w�b�_�[ (�t�@�C���V�O�l�`��) ����������.
		dt_writer.WriteBytes(PNG_HEADER);

		// IHDR �`�����N
		const std::array<uint8_t, 13> ihdr_data{
			static_cast<uint8_t>(rect_width >> 24),
			static_cast<uint8_t>(rect_width >> 16),
			static_cast<uint8_t>(rect_width >> 8),
			static_cast<uint8_t>(rect_width),
			static_cast<uint8_t>(rect_height >> 24),
			static_cast<uint8_t>(rect_height >> 16),
			static_cast<uint8_t>(rect_height >> 8),
			static_cast<uint8_t>(rect_height),
			8,	// �r�b�g�[�x: 8=
			6,	// �J���[�^�C�v: 6=�g�D���[�J���[�{�A���t�@�摜
			0,	// uint32_t compress_method = -1;
			0,	// uint32_t filter_method = -1;
			0	// uint32_t interlace_method = -1;
		};
		dt_writer.WriteUInt32(13);	// Length: Chunk Data �̃T�C�Y. ��� 13
		dt_writer.WriteBytes(IHDR_TYPE);
		dt_writer.WriteBytes(ihdr_data);
		dt_writer.WriteUInt32(CRC32::update(CRC32::update(CRC32::INIT, IHDR_TYPE), ihdr_data) ^ CRC32::INIT);

		// IDAT �`�����N�̑傫�������߂�.
		const uint16_t block_len = static_cast<uint16_t>(1 + 4 * rect_width);	// Deflate �u���b�N�̑傫��
		const uint32_t idat_len = 2 + (5 + block_len) * rect_height + 4;	// Z �X�g���[�� + Deflate �u���b�N + Z �`�F�b�N�T��

		// IDTA �`�����N�w�b�_�[����������.
		dt_writer.WriteUInt32(idat_len);
		dt_writer.WriteBytes(IDAT_TYPE);
		auto idat_crc = CRC32::update(CRC32::INIT, IDAT_TYPE);

		// Z �X�g���[���̈��k�����ƃt���O����������.
		//                CMF                         FLG
		//	+-----------------------------+-----------------------------+
		//	|CMINFO(4)     |CM(4)         |FLEVEL(2)|FDICT(1)|FCHECK(5) |
		//	+-----------------------------+-----------------------------+
		//	 0100           1000           00        0        01101
		dt_writer.WriteBytes(Z_CMF);
		idat_crc = CRC32::update(idat_crc, Z_CMF);

		for (size_t y = rect.top; y < rect.bottom; y++) {
			// �X�L�������C�����Ƃ� Deflate �u���b�N (�񈳏k) ����������.
			// �񈳏k�u���b�N
			//    7    6    5    4    3    2    1    0 �r�b�g
			// +------------------------------------------+
			// |PADDING(6)                |BTYPE(2) |BFINAL(1)
			// +------------------------------------------+
			//   |
			//   0   1   2   3   4 ...�o�C�g
			// +---+---+---+---+---+================================+
			// |   |  LEN  | NLEN  |...LEN bytes of literal data... |
			// +---+---+---+---+---+================================+
			// BFINAL(1)	1=�f�[�^�Z�b�g�̍Ō�̃u���b�N
			// BTYPE(2)�@0=�񈳏k�u���b�N
			// PADDING(6)	�񈳏k�̂Ƃ��̃o�C�g���E����
			// LEN(16)	�u���b�N���̃f�[�^�o�C�g�̐�
			// NLEN(16)	NLEN �́ALEN �̕␔
			// (LEN �� NLEN �̓��g���G���f�B�A��)
			const std::array<uint8_t, 1 + 2 + 2> block_header{
				static_cast<uint8_t>(y + 1 == rect.bottom ? 1 : 0),
				static_cast<uint8_t>(block_len),
				static_cast<uint8_t>(block_len >> 8),
				static_cast<uint8_t>(~block_len),
				static_cast<uint8_t>(~block_len >> 8),
			};
			dt_writer.WriteBytes(block_header);
			idat_crc = CRC32::update(idat_crc, block_header);

			// PNG �t�B���^�[�^�C�v���e�X�L�������C���̐擪�ɉ�����
			// Type	Name
			//  0	None
			//	1	Sub
			//	2	Up
			//	3	Average
			//	4	Paeth
			dt_writer.WriteByte(PNG_FILTER_TYPE);
			idat_crc = CRC32::update(idat_crc, PNG_FILTER_TYPE);
			for (size_t x = rect.left; x < rect.right; x++) {

				// �r�b�g�}�b�v�� BGRA �� PNG �� RGBA ��.
				const std::array<uint8_t, 4> rgba{
					m_data[y * data_width + 4 * x + 2],
					m_data[y * data_width + 4 * x + 1],
					m_data[y * data_width + 4 * x + 0],
					m_data[y * data_width + 4 * x + 3],
				};
				dt_writer.WriteBytes(rgba);
				idat_crc = CRC32::update(idat_crc, rgba);
			}
		}

		// Z-ADLER32 �`�F�b�N�T������������.
		const auto adler = ADLER32::update(ADLER32::INIT, Z_CMF);
		const std::array<uint8_t, 4> z_adler{
			static_cast<uint8_t>(adler >> 24),
			static_cast<uint8_t>(adler >> 16),
			static_cast<uint8_t>(adler >> 8),
			static_cast<uint8_t>(adler)
		};
		dt_writer.WriteBytes(z_adler);
		idat_crc = CRC32::update(idat_crc, z_adler);

		// IDAT �� CRC32 �`�F�b�N�T������������.
		dt_writer.WriteUInt32(idat_crc ^ CRC32::INIT);

		// IEND �`�����N������������.
		constexpr uint32_t IEND_DATA_LEN = 0;
		dt_writer.WriteUInt32(IEND_DATA_LEN);
		dt_writer.WriteBytes(IEND_TYPE);
		dt_writer.WriteUInt32(CRC32::update(CRC32::INIT, IEND_TYPE) ^ CRC32::INIT);
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void ShapeImage::write_svg(const wchar_t f_name[], DataWriter const& dt_writer) const
	{
		dt_write_svg("<image href=\"", dt_writer);
		dt_write_svg(f_name, wchar_len(f_name), dt_writer);
		dt_write_svg("\" ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_view.width, "width", dt_writer);
		dt_write_svg(m_view.height, "height", dt_writer);
		dt_write_svg(m_opac, "opacity", dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
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