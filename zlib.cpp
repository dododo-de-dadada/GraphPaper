#include <algorithm>
#include <array>
#include <unordered_map>
#include "zlib.h"

// RFC 1951 DEFLATE Compressed Data Format Specification version 1.3 ���{���
// https://www.futomi.com/lecture/japanese/rfc1951.html
// https://wiki.suikawiki.org/n/DEFLATE
// �����̊J�����L
// https://7shi.hateblo.jp/entry/20110719/1311093479
// RFC 1950 ZLIB Compressed Data Format Specification version 3.3 ���{���
// https://www.futomi.com/lecture/japanese/rfc1950.html

namespace winrt::Zlib::implementation
{
	// �����\�̒萔
	static constexpr int ABC = 0;	// �A���t�@�x�b�g�̓Y����
	static constexpr int CLEN = 1;	// �������̓Y����
	static constexpr int CODE = 2;	// �����̓Y����
	static constexpr int FREQ = 3;	// �o���񐔂̓Y����
	static constexpr uint32_t LIT_ABC_MAX = 286;	// Max # of literal/length alphabet.
	static constexpr uint32_t LIT_ABC_MIN = 257;	// Min # of Literal/Length alphabet.
	static constexpr uint32_t DIST_ABC_MAX = 32;	// Max # of Distance alphabet.
	static constexpr uint32_t DIST_ABC_MIN = 1;	// Min # of Distance alphabet.
	static constexpr uint32_t CLEN_ABC_MAX = 19;	// Max # of Code Length alphabet.
	static constexpr uint32_t CLEN_ABC_MIN = 4;	// Min # of Code Length alphabet.
	static constexpr uint16_t CLEN_ABC_ORDER[CLEN_ABC_MAX]	// �������̃A���t�@�x�b�g�̏���
	{
		16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
	};

	static constexpr uint16_t FIXED_LIT_TBL[][3]	// �Œ�n�t�}�������\
	{
		// �Œ�n�t�}�������̍쐬
		//	uint16_t lit_code = 0;
		//	lit_cnt = 0;
		//	dist_cnt = 0;
		//	for (uint16_t i = 256; i <= 279; i++) {
		//		lit_tbl[lit_cnt][ABC] = i;
		//		lit_tbl[lit_cnt][CLEN] = 7;
		//		lit_tbl[lit_cnt++][CODE] = lit_code++;
		//	}
		//	lit_code <<= 1;
		//	for (uint16_t i = 0; i <= 143; i++) {
		//		lit_tbl[lit_cnt][ABC] = i;
		//		lit_tbl[lit_cnt][CLEN] = 8;
		//		lit_tbl[lit_cnt++][CODE] = lit_code++;
		//	}
		//	for (uint16_t i = 280; i <= 287; i++) {
		//		lit_tbl[lit_cnt][ABC] = i;
		//		lit_tbl[lit_cnt][CLEN] = 8;
		//		lit_tbl[lit_cnt++][CODE] = lit_code++;
		//	}
		//	lit_code <<= 1;
		//	for (uint16_t i = 144; i <= 255; i++) {
		//		lit_tbl[lit_cnt][ABC] = i;
		//		lit_tbl[lit_cnt][CLEN] = 9;
		//		lit_tbl[lit_cnt++][CODE] = lit_code++;
		//	}
		//	dist_tbl = lit_tbl + lit_cnt;
		//	dist_cnt = 0;
		//	for (uint16_t i = 0; i <= 31; i++) {
		//		dist_tbl[dist_cnt][ABC] = i;
		//		dist_tbl[dist_cnt][CLEN] = 5;
		//		dist_tbl[dist_cnt++][CODE] = i;
		//	}
		{256, 7, 0}, {257, 7, 1}, {258, 7, 2}, {259, 7, 3}, {260, 7, 4}, {261, 7, 5}, {262, 7, 6}, {263, 7, 7},
		{264, 7, 8}, {265, 7, 9}, {266, 7, 10}, {267, 7, 11},  {268, 7, 12}, {269, 7, 13},  {270, 7, 14}, {271, 7, 15},
		{272, 7, 16}, {273, 7, 17}, {274, 7, 18}, {275, 7, 19}, {276, 7, 20}, {277, 7, 21}, {278, 7, 22}, {279, 7, 23},
		{0, 8, 48}, {1, 8, 49}, {2, 8, 50}, {3, 8, 51}, {4, 8, 52}, {5, 8, 53}, {6, 8, 54}, {7, 8, 55},
		{8, 8, 56}, {9, 8, 57}, {10, 8, 58}, {11, 8, 59}, {12, 8, 60}, {13, 8, 61}, {14, 8, 62}, {15, 8, 63},
		{16, 8, 64}, {17, 8, 65}, {18, 8, 66}, {19, 8, 67}, {20, 8, 68}, {21, 8, 69}, {22, 8, 70}, {23, 8, 71},
		{24, 8, 72}, {25, 8, 73}, {26, 8, 74}, {27, 8, 75}, {28, 8, 76}, {29, 8, 77}, {30, 8, 78}, {31, 8, 79},
		{32, 8, 80}, {33, 8, 81}, {34, 8, 82}, {35, 8, 83}, {36, 8, 84}, {37, 8, 85}, {38, 8, 86}, {39, 8, 87},
		{40, 8, 88}, {41, 8, 89}, {42, 8, 90}, {43, 8, 91}, {44, 8, 92}, {45, 8, 93}, {46, 8, 94}, {47, 8, 95},
		{48, 8, 96}, {49, 8, 97}, {50, 8, 98}, {51, 8, 99}, {52, 8, 100}, {53, 8, 101}, {54, 8, 102}, {55, 8, 103},
		{56, 8, 104}, {57, 8, 105}, {58, 8, 106}, {59, 8, 107}, {60, 8, 108}, {61, 8, 109}, {62, 8, 110}, {63, 8, 111},
		{64, 8, 112}, {65, 8, 113}, {66, 8, 114}, {67, 8, 115}, {68, 8, 116}, {69, 8, 117}, {70, 8, 118}, {71, 8, 119},
		{72, 8, 120}, {73, 8, 121}, {74, 8, 122}, {75, 8, 123}, {76, 8, 124}, {77, 8, 125}, {78, 8, 126}, {79, 8, 127},
		{80, 8, 128}, {81, 8, 129}, {82, 8, 130}, {83, 8, 131}, {84, 8, 132}, {85, 8, 133}, {86, 8, 134}, {87, 8, 135},
		{88, 8, 136}, {89, 8, 137}, {90, 8, 138}, {91, 8, 139}, {92, 8, 140}, {93, 8, 141}, {94, 8, 142}, {95, 8, 143},
		{96, 8, 144}, {97, 8, 145}, {98, 8, 146}, {99, 8, 147}, {100, 8, 148}, {101, 8, 149}, {102, 8, 150}, {103, 8, 151},
		{104, 8, 152}, {105, 8, 153}, {106, 8, 154}, {107, 8, 155}, {108, 8, 156}, {109, 8, 157}, {110, 8, 158}, {111, 8, 159},
		{112, 8, 160}, {113, 8, 161}, {114, 8, 162}, {115, 8, 163}, {116, 8, 164}, {117, 8, 165}, {118, 8, 166}, {119, 8, 167},
		{120, 8, 168}, {121, 8, 169}, {122, 8, 170}, {123, 8, 171}, {124, 8, 172}, {125, 8, 173}, {126, 8, 174}, {127, 8, 175},
		{128, 8, 176}, {129, 8, 177}, {130, 8, 178}, {131, 8, 179}, {132, 8, 180}, {133, 8, 181}, {134, 8, 182}, {135, 8, 183},
		{136, 8, 184}, {137, 8, 185}, {138, 8, 186}, {139, 8, 187}, {140, 8, 188}, {141, 8, 189}, {142, 8, 190}, {143, 8, 191},
		{280, 8, 192}, {281, 8, 193}, {282, 8, 194}, {283, 8, 195}, {284, 8, 196}, {285, 8, 197}, {286, 8, 198}, {287, 8, 199},
		{144, 9, 400}, {145, 9, 401}, {146, 9, 402}, {147, 9, 403}, {148, 9, 404}, {149, 9, 405}, {150, 9, 406}, {151, 9, 407},
		{152, 9, 408}, {153, 9, 409}, {154, 9, 410}, {155, 9, 411}, {156, 9, 412}, {157, 9, 413}, {158, 9, 414}, {159, 9, 415},
		{160, 9, 416}, {161, 9, 417}, {162, 9, 418}, {163, 9, 419}, {164, 9, 420}, {165, 9, 421}, {166, 9, 422}, {167, 9, 423},
		{168, 9, 424}, {169, 9, 425}, {170, 9, 426}, {171, 9, 427}, {172, 9, 428}, {173, 9, 429}, {174, 9, 430}, {175, 9, 431},
		{176, 9, 432}, {177, 9, 433}, {178, 9, 434}, {179, 9, 435}, {180, 9, 436}, {181, 9, 437}, {182, 9, 438}, {183, 9, 439},
		{184, 9, 440}, {185, 9, 441}, {186, 9, 442}, {187, 9, 443}, {188, 9, 444}, {189, 9, 445}, {190, 9, 446}, {191, 9, 447},
		{192, 9, 448}, {193, 9, 449}, {194, 9, 450}, {195, 9, 451}, {196, 9, 452}, {197, 9, 453}, {198, 9, 454}, {199, 9, 455},
		{200, 9, 456}, {201, 9, 457}, {202, 9, 458}, {203, 9, 459}, {204, 9, 460}, {205, 9, 461}, {206, 9, 462}, {207, 9, 463},
		{208, 9, 464}, {209, 9, 465}, {210, 9, 466}, {211, 9, 467}, {212, 9, 468}, {213, 9, 469}, {214, 9, 470}, {215, 9, 471},
		{216, 9, 472}, {217, 9, 473}, {218, 9, 474}, {219, 9, 475}, {220, 9, 476}, {221, 9, 477}, {222, 9, 478}, {223, 9, 479},
		{224, 9, 480}, {225, 9, 481}, {226, 9, 482}, {227, 9, 483}, {228, 9, 484}, {229, 9, 485}, {230, 9, 486}, {231, 9, 487},
		{232, 9, 488}, {233, 9, 489}, {234, 9, 490}, {235, 9, 491}, {236, 9, 492}, {237, 9, 493}, {238, 9, 494}, {239, 9, 495},
		{240, 9, 496}, {241, 9, 497}, {242, 9, 498}, {243, 9, 499}, {244, 9, 500}, {245, 9, 501}, {246, 9, 502}, {247, 9, 503},
		{248, 9, 504}, {249, 9, 505}, {250, 9, 506}, {251, 9, 507}, {252, 9, 508}, {253, 9, 509}, {254, 9, 510}, {255, 9, 511},
		//------------------------------------------
		{0, 5, 0}, {1, 5, 1}, {2, 5, 2}, {3, 5, 3}, {4, 5, 4}, {5, 5, 5}, {6, 5, 6}, {7, 5, 7},
		{8, 5, 8}, {9, 5, 9}, {10, 5, 10}, {11, 5, 11}, {12, 5, 12}, {13, 5, 13}, {14, 5, 14}, {15, 5, 15},
		{16, 5, 16}, {17, 5, 17}, {18, 5, 18}, {19, 5, 19}, {20, 5, 20}, {21, 5, 21}, {22, 5, 22}, {23, 5, 23},
		{24, 5, 24}, {25, 5, 25}, {26, 5, 26}, {27, 5, 27}, {28, 5, 28}, {29, 5, 29}, {30, 5, 30}, {31, 5, 31}
	};

	constexpr uint32_t LEN_CODE_TBL[][4]	// ���������\
	{
		//			 �g��
		//	����		�r�b�g��	 ����
		// -----	-----	-------
		{	265,	 1,		 11,12 },	// 0
		{	266,	 1,		 13,14 },
		{	267,	 1,		 15,16 },
		{	268,	 1,		 17,18 },
		{	269,	 2,		 19,22 },
		{	270,	 2,		 23,26 },	// 5
		{	271,	 2,		 27,30 },
		{	272,	 2,		 31,34 },
		{	273,	 3,		 35,42 },
		{	274,	 3,		 43,50 },
		{	275,	 3,		 51,58 },	// 10
		{	276,	 3,		 59,66 },
		{	277,	 4,		 67,82 },
		{	278,	 4,		 83,98 },
		{	279,	 4,		 99,114 },
		{	280,	 4,		115,130 },	// 15
		{	281,	 5,		131,162 },
		{	282,	 5,		163,194 },
		{	283,	 5,		195,226 },
		{	284,	 5,		227,257 },
		//			 �g��
		// ����		�r�b�g��	 ����
		// ----		----	--------
		{	 4,		 1,	    5,6 },	// 20
		{	 5,		 1,	    7,8 },
		{	 6,		 2,	    9,12 },
		{	 7,		 2,	   13,16 },
		{	 8,		 3,	   17,24 },
		{	 9,		 3,	   25,32 },	// 25
		{	10,		 4,	   33,48 },
		{	11,		 4,	   49,64 },
		{	12,		 5,	   65,96 },
		{	13,		 5,	   97,128 },
		{	14,		 6,	  129,192 },	// 30
		{	15,		 6,	  193,256 },
		{	16,		 7,	  257,384 },
		{	17,		 7,	  385,512 },
		{	18,		 8,	  513,768 },
		{	19,		 8,	  769,1024 },	// 35
		{	20,		 9,	 1025,1536 },
		{	21,		 9,	 1537,2048 },
		{	22,		10,	 2049,3072 },
		{	23,		10,	 3073,4096 },
		{	24,		11,	 4097,6144 },	// 40
		{	25,		11,	 6145,8192 },
		{	26,		12,	 8193,12288 },
		{	27,		12,	12289,16384 },
		{	28,		13,	16385,24576 },
		{	29,		13,	24577,32768 },	// 45
		//			 �g��
		// ����		�r�b�g��	 ��
		// ----		----	--------
		{   16,      2,     3,6     },
		{	17,		 3,	    3,10    },
		{	18,		 7,	   11,138   }
	};

	// Bit sequance.

	inline uint32_t bit_get(const uint8_t data[], const size_t bit_pos, const size_t bit_len) noexcept;
	inline size_t bit_put(std::vector<uint8_t>& buf, const size_t bit_pos, const size_t bit_len, const uint32_t bit_val) noexcept;
	inline size_t bit_put(uint8_t data[], const size_t bit_pos, const size_t bit_len, const uint32_t bit_val) noexcept;
	inline uint32_t bit_rev(const uint32_t bit, const size_t len) noexcept;

	inline constexpr uint32_t XYZ_KEY(const uint8_t ptr[]) noexcept;

	// Deflate

	template <uint32_t B, uint32_t N, uint32_t M>
	static void deflate_huffman(uint32_t lit_tbl[N][M]) noexcept;
	template <bool TO_BUF>
	static size_t deflate_put_data(std::vector<uint8_t>& out_buf, size_t bit_pos, /*<---*/uint32_t lit_tbl[][4], uint32_t dist_tbl[][4], /*<---*/const uint8_t in_data[], const size_t in_len) noexcept;
	template <bool TO_BUF>
	static size_t deflate_put_len(std::vector<uint8_t>& out_buf, size_t bit_pos, /*<---*/uint32_t clen_tbl[][4], /*<---*/const uint32_t lit_tbl[][4], const uint32_t lit_cnt) noexcept;
	template <bool TO_BUF>
	static size_t deflate_put_abc(std::vector<uint8_t>& out_buf, const size_t bit_pos, const uint32_t abc, uint32_t lit_tbl[][4]);
	template <bool TO_BUF, int I>
	static size_t deflate_put_len(std::vector<uint8_t>& out_buf, const size_t bit_pos, const size_t word_len, uint32_t lit_tbl[][4]);

	// Inflate

	template <typename T, int M>
	static void inflate_huffman(T tbl[][M], const int cnt) noexcept;
	static size_t inflate_match(const uint8_t data[], const size_t bit_pos, const uint16_t tbl[][3], const size_t tbl_cnt, size_t& new_pos) noexcept;

	// �r�b�g����o�C�g�z�񂩂�ǂݍ���.
	// data	�o�C�g�z��
	// bit_pos	�r�b�g�ʒu
	// bit_len	�r�b�g�� (�Œ� 15 �r�b�g)
	inline uint32_t bit_get(const uint8_t data[], const size_t bit_pos, const size_t bit_len) noexcept
	{
		const auto ptr = data + bit_pos / 8;	// �o�C�g���E�|�C���^�[
		const auto pos = bit_pos % 8;	// �o�C�g���E�ŏ�]�����r�b�g�ʒu
		const auto len = pos + bit_len;	// �o�C�g���E���琔�����r�b�g��
		const uint32_t mask = UINT16_MAX >> (16 - bit_len);	// �r�b�g�}�X�N
		uint32_t val;	// �o�C�g���E�ɍ��킹���l

		// �r�b�g�񂪂ǂ̑傫���o�C�g���E���܂��������肷��.
		if (len <= 8) {
			val = static_cast<uint32_t>(ptr[0]);
		}
		else if (len <= 16) {
			val = static_cast<uint32_t>(ptr[0]) | (static_cast<uint32_t>(ptr[1]) << 8);
		}
		else if (len <= 24) {
			val = static_cast<uint32_t>(ptr[0]) | (static_cast<uint32_t>(ptr[1]) << 8) |
				(static_cast<uint32_t>(ptr[2]) << 16);
		}
		else if (len <= 32) {
			val = static_cast<uint32_t>(ptr[0]) | (static_cast<uint32_t>(ptr[1]) << 8) |
				(static_cast<uint32_t>(ptr[2]) << 16) | (static_cast<uint32_t>(ptr[3]) << 24);
		}
		else {
			val = 0;
		}
		return (val >> pos) & mask;
	}

	// �r�b�g��𓮓I�o�C�g�z��ɏ�������.
	inline size_t bit_put(std::vector<uint8_t>& buf, const size_t bit_pos, const size_t bit_len, const uint32_t bit_val) noexcept
	{
		const auto new_size = (bit_len + bit_pos + 7) / 8;
		const auto buf_size = std::size(buf);
		if (buf_size < new_size) {
			buf.resize(buf_size + 65536);
		}
		return bit_put(std::data(buf), bit_pos, bit_len, bit_val);
	}

	// Write a bit sequence to the byte array.
	inline size_t bit_put(uint8_t data[], const size_t bit_pos, const size_t bit_len, const uint32_t bit_val) noexcept
	{
		// bit_pos = 7
		// bit_len = 11
		// bit_val = 73
		//  0 1 2 3 4 5 6 7
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		// |1:0:0:1:0:0:1: | : : : : : : : | : : ...
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		// val
		//  0 1 2 3 4 5 6 7 0 1 2 3 4 5
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		// |0:0:0:0:0:0:0:1|0:0:1:0:0:1: : | : : ...
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		// mask
		//  0 1 2 3 4 5 6 7
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		// |1:1:1:1:1:1:1: | : : : : : : : | : : ...
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		const auto pos = bit_pos % 8;	// �o�C�g���E�ŏ�]�����r�b�g�ʒu
		uint32_t val = bit_val << pos;	// �o�C�g���E�ɍ��킹���l
		auto ptr = data + bit_pos / 8;	// �o�C�g���E�ɍ��킹���|�C���^�[
		const uint8_t mask = UINT8_MAX >> (8 - pos);	// �o�C�g���E�ɍ��킹���}�X�N
		*ptr &= mask;
		*ptr |= static_cast<uint8_t>(val);
		for (size_t len = bit_len + pos; len > 8; len -= 8) {
			*(++ptr) = static_cast<uint8_t>(val >>= 8);
		}
		return bit_pos + bit_len;
	}

	// Reverse the order of bit sequence.
	inline uint32_t bit_rev(const uint32_t bit_val, const size_t bit_len) noexcept
	{
		uint32_t rev_val = 0;
		for (size_t i = 0, j = bit_len - 1; i < bit_len; i++, j--) {
			rev_val |= ((bit_val >> i) & 1) << j;
		}
		return rev_val;
	}

	// Convert byte sequence to 3-byte unsigned integer value.
	inline constexpr uint32_t XYZ_KEY(const uint8_t ptr[]) noexcept
	{
		return (static_cast<uint32_t>(ptr[0]) << 16) | (static_cast<uint32_t>(ptr[1]) << 8) | static_cast<uint32_t>(ptr[2]);
	}

	// ���k����.
	// out_buf	�o�̓o�b�t�@
	// in_data	���̓f�[�^
	// in_len	���̓f�[�^��
	void deflate(std::vector<uint8_t>& out_buf, /*<---*/const uint8_t in_data[], const size_t in_len) noexcept
	{
		constexpr size_t IN_BLOCK = static_cast<size_t>(65536) - 1;
		constexpr auto TO_TBL = false;
		constexpr auto TO_BUF = true;
		uint32_t clen_tbl[CLEN_ABC_MAX][4]{};	// �������A���t�@�x�b�g�ɑ΂��镄���\
		uint32_t lit_tbl[LIT_ABC_MAX][4]{};	// ���e����/�����A���t�@�x�b�g�ɑ΂��镄���\
		uint32_t dist_tbl[DIST_ABC_MAX][4]{};	// �����A���t�@�x�b�g�ɑ΂��镄���\
		std::vector<uint8_t> empty_buf;

		size_t bit_pos = out_buf.size() * 8;	// �r�b�g�ʒu
		for (size_t len = 0; len < in_len; len += IN_BLOCK) {

			// �e�\�̏�����.
			for (uint32_t i = 0; i < LIT_ABC_MAX; i++) {
				lit_tbl[i][ABC] = i;
				lit_tbl[i][CLEN] = 0;
				lit_tbl[i][CODE] = 0;
				lit_tbl[i][FREQ] = 0;
			}
			for (uint32_t i = 0; i < DIST_ABC_MAX; i++) {
				dist_tbl[i][ABC] = i;
				dist_tbl[i][CLEN] = 0;
				dist_tbl[i][CODE] = 0;
				dist_tbl[i][FREQ] = 0;
			}
			for (uint32_t i = 0; i < CLEN_ABC_MAX; i++) {
				clen_tbl[i][ABC] = i;
				clen_tbl[i][CLEN] = 0;
				clen_tbl[i][CODE] = 0;
				clen_tbl[i][FREQ] = 0;
			}

			// ���e����/�����A���t�@�x�b�g�Ƌ����A���t�@�x�b�g�̏o���񐔂𐔂���.
			const size_t block_len = std::min(in_len - len, IN_BLOCK);	// �u���b�N��
			deflate_put_data<TO_TBL>(empty_buf, 0, lit_tbl, dist_tbl, in_data + len, block_len);

			// �o���񐔂ɂ��ƂÂ��ĕ������𓾂�.
			// ���e����/�����A���t�@�x�b�g�̌������߂�.
			// �����̘A�����镄�����[���̃A���t�@�x�b�g�͌��Ɋ܂܂Ȃ�.
			// �������ɂ��ƂÂ��ăn�t�}��������ݒ肷��.
			// ������, ���͍ŏ��������ɂ͂Ȃ�Ȃ�.
			deflate_huffman<15, LIT_ABC_MAX>(lit_tbl);
			uint32_t lit_cnt = LIT_ABC_MAX;	// ���e����/�����A���t�@�x�b�g�̌�
			for (; lit_cnt > 0 && lit_tbl[lit_cnt - 1][CLEN] == 0; lit_cnt--);
			inflate_huffman(lit_tbl, lit_cnt);
			lit_cnt = std::max(LIT_ABC_MIN, lit_cnt);

			// �o���񐔂ɂ��ƂÂ��ĕ������𓾂�.
			// �����A���t�@�x�b�g�̌������߂�.
			// �����̘A�����镄�����[���̃A���t�@�x�b�g�͌��Ɋ܂܂Ȃ�.
			// �������ɂ��ƂÂ��ăn�t�}��������ݒ肷��.
			// ������, ���͍ŏ��������ɂ͂Ȃ�Ȃ�.
			deflate_huffman<15, DIST_ABC_MAX>(dist_tbl);
			uint32_t dist_cnt = DIST_ABC_MAX;	// �����A���t�@�x�b�g�̌�
			for (; dist_cnt > 0 && dist_tbl[dist_cnt - 1][CLEN] == 0; dist_cnt--);
			inflate_huffman(dist_tbl, dist_cnt);
			dist_cnt = std::max(DIST_ABC_MIN, dist_cnt);

			// �������A���t�@�x�b�g�̏o���񐔂𐔂���.
			deflate_put_len<TO_TBL>(empty_buf, 0, clen_tbl, lit_tbl, lit_cnt);
			deflate_put_len<TO_TBL>(empty_buf, 0, clen_tbl, dist_tbl, dist_cnt);

			// �o���񐔂ɂ��ƂÂ��ĕ������𓾂�.
			// �������ɂ��ƂÂ��ăn�t�}��������ݒ肷��.
			// �������A���t�@�x�b�g�̌������߂�.
			// �����̘A�����镄�����[���̃A���t�@�x�b�g�͌��Ɋ܂܂Ȃ�.
			// ������, �ŏ��������ɂ͂Ȃ炸, �A���t�@�x�b�g���ł͂Ȃ�.
			deflate_huffman<7, CLEN_ABC_MAX>(clen_tbl);
			inflate_huffman(clen_tbl, CLEN_ABC_MAX);
			uint32_t clen_cnt = CLEN_ABC_MAX;	// �������A���t�@�x�b�g�̌�
			for (; clen_cnt > CLEN_ABC_MIN; clen_cnt--) {
				if (clen_tbl[CLEN_ABC_ORDER[clen_cnt - 1]][CLEN] != 0) {
					break;
				}
			}

			//  3 �r�b�g�̃w�b�_�[ (�� 1 �r�b�g BFINAL, ���� 2 �r�b�g BTYPE) ���o�͂���.
			// �ŏI�u���b�N�Ȃ�, BFINAL �� 1.
			bit_put(out_buf, bit_pos, 1, in_len - len <= IN_BLOCK ? 1 : 0); bit_pos += 1;	// BFINAL
			bit_put(out_buf, bit_pos, 2, 2); bit_pos += 2;	// BTYPE
			// �e�A���t�@�x�b�g�̌�.
			bit_put(out_buf, bit_pos, 5, lit_cnt - LIT_ABC_MIN); bit_pos += 5;	// HLIT
			bit_put(out_buf, bit_pos, 5, dist_cnt - DIST_ABC_MIN); bit_pos += 5;	// HDIST
			bit_put(out_buf, bit_pos, 4, clen_cnt - CLEN_ABC_MIN); bit_pos += 4;	// HCLEN
			// �������A���t�@�x�b�g�̕��������o�͂���.
			for (size_t i = 0; i < clen_cnt; i++) {
				const int j = CLEN_ABC_ORDER[i];
				bit_put(out_buf, bit_pos, 3, clen_tbl[j][CLEN]); bit_pos += 3;
			}
			// ���e����/�����Ƌ�����, �e�A���t�@�x�b�g�̕������Ƃ��̊g���r�b�g���o�͂���.
			bit_pos = deflate_put_len<TO_BUF>(out_buf, bit_pos, clen_tbl, lit_tbl, lit_cnt);
			bit_pos = deflate_put_len<TO_BUF>(out_buf, bit_pos, clen_tbl, dist_tbl, dist_cnt);
			// �A���t�@�x�b�g�ɑΉ����镄�����o�b�t�@�ɏ�������.
			bit_pos = deflate_put_data<TO_BUF>(out_buf, bit_pos, lit_tbl, dist_tbl, in_data + len, std::min(in_len - len, IN_BLOCK));
		}
		// �o�̓o�b�t�@�̑傫�������킹��.
		out_buf.resize((bit_pos + 7) / 8);
		out_buf.shrink_to_fit();
	}

	// �o���񐔂ɂ��ƂÂ��ĕ������𓾂�.
	// B	�����̍ő�r�b�g��
	// N	�����\�̍ő�s��
	// M	�����\�̗�
	template <uint32_t B, uint32_t N, uint32_t M>
	static void deflate_huffman(uint32_t lit_tbl[N][M]) noexcept
	{
		// �p�b�P�[�W�E�}�[�W�E�A���S���Y��
		// 
		// 1. �e�[�u�����o���񐔏��ɕ��ёւ���. �v�f����Ȃ疖���͖��������
		// 2. ���񂾗v�f 2 �������ԂɃp�b�P�[�W������.
		//    2 �̗v�f��, �A���t�@�x�b�g��A��, �o���񐔂����Z. 
		//    ���ꂼ��, �p�b�P�[�W�̃A���t�@�x�b�g�Əo���񐔂ɑ������.
		// 2. �p�b�P�[�W�����̃e�[�u���Ƀ}�[�W����.
		// 3. ��� 1 - 2 �� B - 1 ��J��Ԃ�.
		// 4. �e�[�u�����o���񐔏��ɕ��ёւ���. �v�f����Ȃ疖���͖��������
		// 5. �Ō�ɓ���ꂽ�e�[�u���Ɋ܂܂��A���t�@�x�b�g���A���t�@�x�b�g���ɐ�����.
		//    ���̊e�v���n�t�}�������̒����ɂȂ�.
		// 
		// �A���t�@�x�b�g�Ƃ��̏o���񐔂̑g��v�f�Ƃ���e�[�u��
		// +-----+-----+-----+-----+-----+
		// | A 2 | B 6 | C10 | D 8 | E 4 |
		// +-----+-----+-----+-----+-----+
		// 
		//                       ��Ȃ疖����
		// ���ёւ�                    ����      �p�b�P�[�W��
		// +-----+-----+-----+-----+ +-----+    +-----+-----+
		// | A 2 | E 4 | B 6 | D 8 |-| C10 | => |AE 6 |BD 14|
		// +-----+-----+-----+-----+ +-----+    +-----+-----+
		// 
		// �}�[�W
		// +-----+-----+-----+-----+-----+ +-----+-----+
		// | A 2 | B 6 | C10 | D 8 | E 4 |+|AE 6 |BD 14|
		// +-----+-----+-----+-----+-----+ +-----+-----+
		// 
		//                                   ��Ȃ疖����
		// ���ёւ�                                ����      �p�b�P�[�W��
		// +-----+-----+-----+-----+-----+-----+ +-----+    +-----+-----+-----+
		// | A 2 | E 4 | B 6 |AE 6 | D 8 | C10 |-|BD 14| => |AE 6 |BAE12|DC 18|
		// +-----+-----+-----+-----+-----+-----+ +-----+    +-----+-----+-----+
		// 
		// �}�[�W
		// +-----+-----+-----+-----+-----+ +-----+-----+-----+
		// | A 2 | B 6 | C10 | D 8 | E 4 |+|AE 6 |BAE12|DC 18|
		// +-----+-----+-----+-----+-----+ +-----+-----+-----+
		// 
		// ���ёւ�                                       �@�@�@ �p�b�P�[�W��
		// +-----+-----+-----+-----+-----+-----+-----+-----+    +-----+-----+-----+-------+
		// | A 2 | E 4 | B 6 |AE 6 | D 8 | C10 |BAE12|DC 18| => |AE 6 |BAE12|DC 18|BAEDC30|
		// +-----+-----+-----+-----+-----+-----+-----+-----+    +-----+-----+-----+-------+
		// 
		// �}�[�W
		// +-----+-----+-----+-----+-----+ +------+-----+-----+-------+
		// | A 2 | B 6 | C10 | D 8 | E 4 |+|AE 6 |BAE12|DC 18|BAEDC30|
		// +-----+-----+-----+-----+-----+ +------+-----+-----+-------+
		//                                                ��Ȃ疖����
		// ���ёւ�                                             ����
		// +-----+-----+-----+-----+-----+-----+-----+-----+ +-------+
		// | A 2 | E 4 | B 6 |AE 6 | D 8 | C10 |BAE12|DC 18|-|BAEDC30|
		// +-----+-----+-----+-----+-----+-----+-----+-----+ +-------+
		// 
		// ����
		// +---+---+
		// | A | 3 |
		// +---+---+
		// | B | 2 |
		// +---+---+
		// | C | 2 |
		// +---+---+
		// | D | 2 |
		// +---+---+
		// | E | 3 |
		// +---+---+

		// �p�b�P�[�W
		struct PACKAGE {
			std::vector<uint32_t> abc;
			uint32_t freq = 0;
		};

		constexpr uint32_t PACKAGE_MAX = []() {
			uint32_t a = N;
			uint32_t b = a;
			for (int i = 0; i < B - 1; i++) {
				b = b / 2;
				a = N + b;
				b = (a / 2) * 2;
			}
			return a;
		}();

		// ������
		std::vector<PACKAGE> tbl(PACKAGE_MAX);	// �X�^�b�N�x�����o���Ȃ����߂� vector
		std::array<PACKAGE, PACKAGE_MAX / 2> pkg;
		std::array<uint32_t, PACKAGE_MAX> idx;

		// �o���� 0 �̃A���t�@�x�b�g�͎�菜��.
		uint32_t tbl_cnt = 0;
		for (int i = 0; i < N; i++) {
			if (lit_tbl[i][FREQ] > 0) {
				tbl[tbl_cnt].abc.push_back(lit_tbl[i][ABC]);
				tbl[tbl_cnt].freq = lit_tbl[i][FREQ];
				idx[tbl_cnt++] = tbl_cnt;
			}
			lit_tbl[i][CLEN] = 0;
		}

		// ���ёւ�
		{
			auto end = std::begin(idx);
			std::advance(end, tbl_cnt);
			std::sort(std::begin(idx), end,
				[tbl](const uint32_t& a, const uint32_t& b) {
					return tbl[a].freq < tbl[b].freq;
				});
		}

		uint32_t a = tbl_cnt;
		uint32_t b = a;
		for (int k = 0; k < B - 1; k++) {
			// �}�[�W
			b = b / 2;
			for (uint32_t i = 0; i < b; i++) {
				const uint32_t i0 = idx[2 * static_cast<size_t>(i) + 0];
				const uint32_t i1 = idx[2 * static_cast<size_t>(i) + 1];
				pkg[i].abc.clear();
				//pkg[i].first.clear();
				for (int j = 0; j < tbl[i0].abc.size(); j++) {
					const uint32_t abc = tbl[i0].abc[j];
					pkg[i].abc.push_back(abc);
				}
				for (int j = 0; j < tbl[i1].abc.size(); j++) {
					const uint32_t abc = tbl[i1].abc[j];
					pkg[i].abc.push_back(abc);
				}
				pkg[i].freq = tbl[i0].freq + tbl[i1].freq;
			}
			a = tbl_cnt;
			for (uint32_t i = 0; i < b; i++) {
				tbl[a++] = pkg[i];
			}
			// ���ёւ�
			for (uint32_t i = 0; i < a; i++) {
				idx[i] = i;	// �Y�����̏�����
			}
			auto end = std::begin(idx);
			std::advance(end, a);
			std::sort(std::begin(idx), end,
				[tbl](const uint32_t& a, const uint32_t& b) {
					return tbl[a].freq < tbl[b].freq;
				});
			// ��Ȃ疖���͎�菜��
			b = (a / 2) * 2;
		}
		for (uint32_t i = 0; i < b; i++) {
			const int j = idx[i];
			for (int k = 0; k < tbl[j].abc.size(); k++) {
				const uint32_t abc = tbl[j].abc[k];
				lit_tbl[abc][CLEN]++;
			}
		}
	}

	static void deflate_match(
		const uint8_t in_end[],
		const uint8_t win_ptr[], const size_t win_len,
		std::list<const uint8_t*>& xyz_chain,
		size_t& word_pos, size_t& word_len)
	{
		int cnt_out = 0;
		// �y�A����, �o���ʒu���i�[���ꂽ�`�F�C�������o��.
		for (const auto ptr : xyz_chain) {
			// �ʒu���E�B���h�E���͈̔͊O�ɂȂ����Ȃ�,
			if (ptr < win_ptr) {
				// ���̌��𐔂�, ���̏o���ʒu�ɐi��.
				cnt_out++;
				continue;
			}
			// �P�꒷�� 1 �o�C�g�����₵�Ȃ����r����.
			// �n�b�V���ɂ���� 3 �o�C�g�͈�v�ς�.
			size_t len = 3;	// ��v��������
			for (;
				// �P��̍ő咷���ȉ�
				len < 258 &&
				// �f�[�^�̏I�[�͒����Ȃ�.
				win_ptr + win_len + len < in_end &&
				ptr[len] == win_ptr[win_len + len];
				len++) {
			}
			// ������ɂ���Œ���v�𓾂�.
			if (len >= word_len) {
				word_len = len;
				word_pos = ptr - win_ptr;
			}
		}

		//------------------------------
		// �`�F�C������, �͈͊O�ɂȂ����ʒu���폜����.
		//------------------------------
		for (; cnt_out > 0; cnt_out--) {
			xyz_chain.pop_front();
		}

	}

	// �A���t�@�x�b�g�ɑΉ����镄�����o�͂���.
	// out_buf	�o�̓o�b�t�@
	// bit_pos	�o�͂���r�b�g�ʒu
	// abc	�A���t�@�x�b�g
	// lit_tbl	�����\
	// �߂�l	�o�͂��������̃r�b�g��
	template <bool TO_BUF>
	static size_t deflate_put_abc(
		std::vector<uint8_t>& out_buf,
		const size_t bit_pos,
		const uint32_t abc,
		uint32_t lit_tbl[][4])
	{
		if constexpr (TO_BUF) {
			const auto clen = lit_tbl[abc][CLEN];
			const auto code = lit_tbl[abc][CODE];
			bit_put(out_buf, bit_pos, clen, bit_rev(code, clen));
			return bit_pos + clen;
		}
		else {
			if (lit_tbl[abc][FREQ] < UINT32_MAX) {
				lit_tbl[abc][FREQ]++;
			}
			return 0;
		}
	}

	// �����A���t�@�x�b�g�ɑΉ����镄���Ƃ��̊g���r�b�g���o�͂���.
	template <bool TO_BUF, int I>
	static size_t deflate_put_len(
		std::vector<uint8_t>& out_buf,
		const size_t bit_pos,
		const size_t word_len,
		uint32_t lit_tbl[][4])
	{
		constexpr uint32_t abc = LEN_CODE_TBL[I][0];
		if constexpr (TO_BUF) {
			const auto clen = lit_tbl[abc][CLEN];
			const auto code = lit_tbl[abc][CODE];
			constexpr auto ex_len = static_cast<size_t>(LEN_CODE_TBL[I][1]);	// �g���r�b�g��
			const auto ex_bit = static_cast<uint32_t>(word_len) - LEN_CODE_TBL[I][2];
			bit_put(out_buf, bit_pos, clen, bit_rev(code, clen));
			bit_put(out_buf, bit_pos + clen, ex_len, ex_bit);
			return bit_pos + clen + ex_len;
		}
		else {
			if (lit_tbl[abc][FREQ] < UINT32_MAX) {
				lit_tbl[abc][FREQ]++;
			}
			return 0;
		}

	}

	static void deflate_slide(const size_t word_len, const size_t win_max, const uint8_t* win_ptr, const size_t win_len, const uint8_t*& new_ptr, size_t& new_len)
	{
		auto len = win_len;
		auto ptr = win_ptr;
		//------------------------------
		// ���̃X���C�h
		//------------------------------
		// �X���C�h�������ő咷�ȉ��Ȃ�,
		if (len < win_max) {
			// �|�C���^�[�͂��̂܂܂�, �X���C�h�����̂ݑ��₷.
			len += word_len;
		}
		// ����ȊO�Ȃ�,
		else {
			// �����͂��̂܂܂ŃX���C�h���|�C���^�[�̂ݑ��₷.
			ptr += word_len;
		}
		// �X���C�h�������̓f�[�^����͂ݏo�����Ȃ�,
		if (len > win_max) {
			// �͂ݏo��������߂�.
			const size_t over_len = len - win_max;	// �͂ݏo��������
			len -= over_len;
			ptr += over_len;
		}
		new_len = len;
		new_ptr = ptr;
	}

	//---------------------------------
	// �Ȃ񂿂���ă����O�o�b�t�@
	// �ő�v�f���� RING_BUF_MAX - 1
	// RING_BUF_MAX �� 2 �� n ��Ɍ���
	// �ǉ��͐擪�݂̂�, �폜�͂ł��Ȃ�.
	// ��ꂽ�v�f�͎����I�ɍ폜�����.
	//---------------------------------
	template <class T, T S, size_t N>
	struct RING_BUF {
		T buf[N]{};
		size_t pos = 0;
		size_t before(const size_t i) const noexcept
		{
			return (i + (N - 1)) & (N - 1);
		}
		void push_front(T val) noexcept
		{
			pos = before(pos);
			buf[pos] = val;
			buf[before(pos)] = S;
		}
		T operator[](int i) const noexcept
		{
			return buf[(pos + i) & (N - 1)];
		}
	};

	// ���e����/�����A���t�@�x�b�g�Ƌ����A���t�@�x�b�g�̏o���񐔂𐔂���.
	// �܂���, 
	// �A���t�@�x�b�g�ɑΉ����镄�����o�b�t�@�ɏ�������.
	// TO_BUF	true �Ȃ�΃o�b�t�@�ɏo��, false �Ȃ�Ώo���񐔂𐔂���.
	// out_buf	�o�̓o�b�t�@
	// bit_pos	�o�̓r�b�g�ʒu
	// lit_tbl	���e����/�����A���t�@�x�b�g�ɑ΂��镄���\ (�A���t�@�x�b�g��)
	// dist_tbl	�����A���t�@�x�b�g�ɑ΂��镄���\ (�A���t�@�x�b�g��)
	// in_data	���̓f�[�^
	// in_len	���̓f�[�^��
	template<bool TO_BUF>
	static size_t deflate_put_data(
		std::vector<uint8_t>& out_buf,
		size_t bit_pos,
		uint32_t lit_tbl[][4],
		uint32_t dist_tbl[][4],
		const uint8_t in_data[],
		const size_t in_len) noexcept
	{
		// �P�� (3 �o�C�g) �̏o���ʒu����������n�b�V��.
		// �L�[�ƒl�̃y�A���i�[�����.
		// �L�[��, �f�[�^ 3 �o�C�g�𐮐��ɂ����l.
		// �l��, �P��̏o���ʒu�����Ɋi�[�����`�F�C�� (���X�g)
		//
		//  �y�A
		// �L�[:�l    �o���ʒu
		// +---+--+   +-----+-----+-----+   +-----+
		// |AAA| ---->|addr0|addr1|addr2|...|addrK|
		// +---+--+   +-----+-----+-----+   +-----+
		// +---+--+   +-----+-----+   +-----+
		// |ABC| ---->|addr0|addr1|...|addrL|
		// +---+--+   +-----+-----+   +-----+
		//   :   :       :     :
		// +---+--+   +-----+-----+-----+-----+   +-----+
		// |XYZ| ---->|addr0|addr1|addr2|addr3|...|addrM|
		// +---+--+   +-----+-----+-----+-----+   +-----+
		//   :   :       :     :
		std::unordered_map<uint32_t,
			RING_BUF<const uint8_t*, nullptr, 32>> xyz_hash;

		// �����\�̏o���񐔂�������.
		// �����\�̓A���t�@�x�b�g���ɂȂ��Ă��邱��.
		for (int i = 0; i < LIT_ABC_MAX; i++) {
			lit_tbl[i][FREQ] = 0;
		}
		for (int i = 0; i < DIST_ABC_MAX; i++) {
			dist_tbl[i][FREQ] = 0;
		}

		// �X���C�f�B���O���A���S���Y��
		// 
		// in_data 
		//  |      win_ptr
		//  |       |------------win_len------------>|
		//  |       |                                |
		//  +-- - --+--------------=====-------------+=====------- - - -+
		//  |       |              ABCDE             |ABCDE...
		//  +-- - --+-------------+=====-------------+=====------- - - -+
		//  |       |             |                  |---->|
		//  |       |--word_pos-->|                  | word_len
		//  |
		//  |-- - --------------------in_len---------------------- - - >|
		const uint8_t* const in_end = in_data + in_len;	// ���̓f�[�^�̏I�[
		const size_t win_max = std::min(32768ull, in_len);	// �X���C�h���̍ő啝
		const uint8_t* win_ptr = in_data;	// �X���C�h���|�C���^�[
		size_t win_len = 0;	// �X���C�h����
		size_t word_len = 0;	// �P�꒷
		size_t word_pos = 0;	// (�X���C�h���̒��ł�) �P��ʒu
		//int lazy_cnt = 0;

		// ��v����P�꒷�� 3 �o�C�g�ȏ�Ȃ̂�,
		// �c��̃o�C�g���� 3 �o�C�g�����ɂȂ�Ȃ��ԌJ��Ԃ�.
		while (win_ptr + win_len <= in_end - 3) {

			/*
			//------------------------------
			// �X���C�h���̒�����, �Œ���v����P���T��.
			//------------------------------
			// �Œ���v�̃��j�A�T�[�`
			word_len = 1;	// �P�꒷
			const auto word_abc = win_ptr[win_len];	// �P��̍ŏ��̃o�C�g
			for (size_t pos = 0;	// ��v�����ʒu
				pos < win_len; pos++) {
				if (win_ptr[pos] != word_abc) {
					continue;
				}
				// �P�꒷�� 1 �o�C�g�����₵�Ȃ����r����.
				size_t len = 1;	// ��v�������� (1 �o�C�g�͈�v�ς�)
				for (;
					// �P��̍ő咷���ȉ�
					len < 258 &&
					// �f�[�^�̏I�[�͒����Ȃ�.
					win_ptr + win_len + len < in_end &&
					win_ptr[pos + len] == win_ptr[win_len + len];
					len++) {
				}
				// '>=' ��, ������ɂ���Œ���v�𓾂邽��.
				if (len >= word_len) {
					word_len = len;
					word_pos = pos;
				}
			}
			*/

			//------------------------------
			// �X���C�h���̒�����, �Œ���v����P���T��.
			//------------------------------
			// �Œ���v�̃n�b�V���T�[�`
			word_len = 1;	// �P�꒷
			const uint32_t xyz_key = XYZ_KEY(win_ptr + win_len);	// XYZ �n�b�V���L�[

			// �L�[�ɍ��v����v�f (�y�A) �� XYZ �n�b�V���ɒ��ɂ���Ȃ�,
			if (xyz_hash.count(xyz_key) > 0) {
				auto& chain = xyz_hash[xyz_key];	// �y�A�Ɋi�[���ꂽ�`�F�C��
				//int cnt = 0;
				// �`�F�C���Ɋi�[���ꂽ�e�ʒu�ɂ���.
				for (int i = 0; chain[i] != nullptr; i++) {
					const uint8_t* ptr = chain[i];
					//for (const auto ptr : chain) {
					if (ptr < win_ptr) {
						break;
					}
					// �P�꒷�� 1 �o�C�g�����₵�Ȃ����r����.
					// �n�b�V���ɂ���� 3 �o�C�g�͈�v�ς�.
					size_t len = 3;	// ��v��������
					for (;
						// �P��̍ő咷���ȉ�
						len < 258 &&
						// �f�[�^�̏I�[�͒����Ȃ�.
						win_ptr + win_len + len < in_end &&
						ptr[len] == win_ptr[win_len + len];
						len++) {
					}
					// ������ɂ���Œ���v�𓾂�.
					if (len > word_len) {
						word_len = len;
						word_pos = ptr - win_ptr;
					}
					//cnt++;
				}
				/*
				//------------------------------
				// �`�F�C������, �͈͊O�ɂȂ����ʒu���폜����.
				//------------------------------
				for (int i = chain.size(); i > cnt; i--) {
					chain.pop_back();
				}
				*/

				/*
				if (lazy_cnt > 0) {
					lazy_cnt--;
				}
				else {
					const uint8_t* lazy_ptr;
					size_t lazy_len;
					size_t lazy_word = word_len;

					int i = 1;
					for (; i < 258 && win_ptr + win_len + i + 3 < in_end; i++) {
						deflate_slide(i, win_max, win_ptr, win_len, lazy_ptr, lazy_len);

						const uint32_t lazy_key = XYZ_KEY(lazy_ptr + lazy_len);	// XYZ �n�b�V���L�[
						auto& lazy_chain = xyz_hash[lazy_key];	// �o���ʒu���i�[���ꂽ�`�F�C��
						for (const auto ptr : lazy_chain) {
							// �ʒu���E�B���h�E���͈̔͊O�Ȃ�,
							if (ptr < lazy_ptr) {
								// ��������.
								continue;
							}
							// �P�꒷�� 1 �o�C�g�����₵�Ȃ����r����.
							// �n�b�V���ɂ���� 3 �o�C�g�͈�v�ς�.
							size_t len = 3;	// ��v��������
							for (;
								// �P��̍ő咷���ȉ�
								len < 258 &&
								// �f�[�^�̏I�[�͒����Ȃ�.
								lazy_ptr + lazy_len + len < in_end &&
								ptr[len] == lazy_ptr[lazy_len + len];
								len++) {
							}
							// ��蒷���Œ���v�𓾂�.
							if (len > lazy_len) {
								lazy_len = len;
							}
						}
					}
					if (lazy_word > word_len + 1) {
						lazy_cnt = i;
						word_len = 1;
					}
				}
				*/
			}

			// �P�ꂪ������Ȃ�, �܂��͌������Ă��P�꒷�� 2 �o�C�g�ȉ��Ȃ�,
			if (word_len >= 0 && word_len <= 2) {
				//------------------------------
				// �A���t�@�x�b�g�ɑΉ����镄�������̂܂܏o��
				//------------------------------
				const uint32_t abc = win_ptr[win_len];	// �A���t�@�x�b�g
				bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, lit_tbl);
				word_len = 1;
			}

			// �P�꒷�� 3 �o�C�g�ȏ�̒P�ꂪ���������Ȃ�,
			else {
				//------------------------------
				// ��������, �������� (�Ƃ��̊g���r�b�g��) ���o�͂���.
				//------------------------------
				if (word_len >= 3 && word_len <= 10) {
					const uint32_t abc = 257 + (static_cast<uint32_t>(word_len) - 3);
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[0][2] && word_len <= LEN_CODE_TBL[0][3]) {
					bit_pos = deflate_put_len<TO_BUF, 0>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[1][2] && word_len <= LEN_CODE_TBL[1][3]) {
					bit_pos = deflate_put_len<TO_BUF, 1>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[2][2] && word_len <= LEN_CODE_TBL[2][3]) {
					bit_pos = deflate_put_len<TO_BUF, 2>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[3][2] && word_len <= LEN_CODE_TBL[3][3]) {
					bit_pos = deflate_put_len<TO_BUF, 3>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[4][2] && word_len <= LEN_CODE_TBL[4][3]) {
					bit_pos = deflate_put_len<TO_BUF, 4>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[5][2] && word_len <= LEN_CODE_TBL[5][3]) {
					bit_pos = deflate_put_len<TO_BUF, 5>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[6][2] && word_len <= LEN_CODE_TBL[6][3]) {
					bit_pos = deflate_put_len<TO_BUF, 6>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[7][2] && word_len <= LEN_CODE_TBL[7][3]) {
					bit_pos = deflate_put_len<TO_BUF, 7>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[8][2] && word_len <= LEN_CODE_TBL[8][3]) {
					bit_pos = deflate_put_len<TO_BUF, 8>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[9][2] && word_len <= LEN_CODE_TBL[9][3]) {
					bit_pos = deflate_put_len<TO_BUF, 9>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[10][2] && word_len <= LEN_CODE_TBL[10][3]) {
					bit_pos = deflate_put_len<TO_BUF, 10>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[11][2] && word_len <= LEN_CODE_TBL[11][3]) {
					bit_pos = deflate_put_len<TO_BUF, 11>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[12][2] && word_len <= LEN_CODE_TBL[12][3]) {
					bit_pos = deflate_put_len<TO_BUF, 12>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[13][2] && word_len <= LEN_CODE_TBL[13][3]) {
					bit_pos = deflate_put_len<TO_BUF, 13>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[14][2] && word_len <= LEN_CODE_TBL[14][3]) {
					bit_pos = deflate_put_len<TO_BUF, 14>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[15][2] && word_len <= LEN_CODE_TBL[15][3]) {
					bit_pos = deflate_put_len<TO_BUF, 15>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[16][2] && word_len <= LEN_CODE_TBL[16][3]) {
					bit_pos = deflate_put_len<TO_BUF, 16>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[17][2] && word_len <= LEN_CODE_TBL[17][3]) {
					bit_pos = deflate_put_len<TO_BUF, 17>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[18][2] && word_len <= LEN_CODE_TBL[18][3]) {
					bit_pos = deflate_put_len<TO_BUF, 18>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len >= LEN_CODE_TBL[19][2] && word_len <= LEN_CODE_TBL[19][3]) {
					bit_pos = deflate_put_len<TO_BUF, 19>(out_buf, bit_pos, word_len, lit_tbl);
				}
				else if (word_len == 258) {
					constexpr uint32_t abc = 285;
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, lit_tbl);
				}
				else {
					__debugbreak();
					return static_cast<size_t>(-1);
				}
				const auto dist = win_len - word_pos;	// ����
				if (dist >= 1 && dist <= 4) {
					const uint32_t abc = static_cast<uint32_t>(dist) - 1;
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[20][2] && dist <= LEN_CODE_TBL[20][3]) {
					bit_pos = deflate_put_len<TO_BUF, 20>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[21][2] && dist <= LEN_CODE_TBL[21][3]) {
					bit_pos = deflate_put_len<TO_BUF, 21>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[22][2] && dist <= LEN_CODE_TBL[22][3]) {
					bit_pos = deflate_put_len<TO_BUF, 22>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[23][2] && dist <= LEN_CODE_TBL[23][3]) {
					bit_pos = deflate_put_len<TO_BUF, 23>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[24][2] && dist <= LEN_CODE_TBL[24][3]) {
					bit_pos = deflate_put_len<TO_BUF, 24>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[25][2] && dist <= LEN_CODE_TBL[25][3]) {
					bit_pos = deflate_put_len<TO_BUF, 25>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[26][2] && dist <= LEN_CODE_TBL[26][3]) {
					bit_pos = deflate_put_len<TO_BUF, 26>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[27][2] && dist <= LEN_CODE_TBL[27][3]) {
					bit_pos = deflate_put_len<TO_BUF, 27>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[28][2] && dist <= LEN_CODE_TBL[28][3]) {
					bit_pos = deflate_put_len<TO_BUF, 28>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[29][2] && dist <= LEN_CODE_TBL[29][3]) {
					bit_pos = deflate_put_len<TO_BUF, 29>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[30][2] && dist <= LEN_CODE_TBL[30][3]) {
					bit_pos = deflate_put_len<TO_BUF, 30>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[31][2] && dist <= LEN_CODE_TBL[31][3]) {
					bit_pos = deflate_put_len<TO_BUF, 31>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[32][2] && dist <= LEN_CODE_TBL[32][3]) {
					bit_pos = deflate_put_len<TO_BUF, 32>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[33][2] && dist <= LEN_CODE_TBL[33][3]) {
					bit_pos = deflate_put_len<TO_BUF, 33>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[34][2] && dist <= LEN_CODE_TBL[34][3]) {
					bit_pos = deflate_put_len<TO_BUF, 34>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[35][2] && dist <= LEN_CODE_TBL[35][3]) {
					bit_pos = deflate_put_len<TO_BUF, 35>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[36][2] && dist <= LEN_CODE_TBL[36][3]) {
					bit_pos = deflate_put_len<TO_BUF, 36>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[37][2] && dist <= LEN_CODE_TBL[37][3]) {
					bit_pos = deflate_put_len<TO_BUF, 37>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[38][2] && dist <= LEN_CODE_TBL[38][3]) {
					bit_pos = deflate_put_len<TO_BUF, 38>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[39][2] && dist <= LEN_CODE_TBL[39][3]) {
					bit_pos = deflate_put_len<TO_BUF, 39>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[40][2] && dist <= LEN_CODE_TBL[40][3]) {
					bit_pos = deflate_put_len<TO_BUF, 40>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[41][2] && dist <= LEN_CODE_TBL[41][3]) {
					bit_pos = deflate_put_len<TO_BUF, 41>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[42][2] && dist <= LEN_CODE_TBL[42][3]) {
					bit_pos = deflate_put_len<TO_BUF, 42>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[43][2] && dist <= LEN_CODE_TBL[43][3]) {
					bit_pos = deflate_put_len<TO_BUF, 43>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[44][2] && dist <= LEN_CODE_TBL[44][3]) {
					bit_pos = deflate_put_len<TO_BUF, 44>(out_buf, bit_pos, dist, dist_tbl);
				}
				else if (dist >= LEN_CODE_TBL[45][2] && dist <= LEN_CODE_TBL[45][3]) {
					bit_pos = deflate_put_len<TO_BUF, 45>(out_buf, bit_pos, dist, dist_tbl);
				}
				else {
					__debugbreak();
					return static_cast<size_t>(-1);
				}
			}

			//------------------------------
			// ���̃X���C�h
			//------------------------------
			const auto win_end = win_ptr + win_len;	// �X���C�h�O�̑��̏I�[
			// �X���C�h�������ő咷�ȉ��Ȃ�,
			if (win_len < win_max) {
				// �|�C���^�[�͂��̂܂܂�, �X���C�h�����̂ݑ��₷.
				win_len += word_len;
			}
			// ����ȊO�Ȃ�,
			else {
				// �����͂��̂܂܂ŃX���C�h���|�C���^�[�̂ݑ��₷.
				win_ptr += word_len;
			}
			// �X���C�h�������̓f�[�^����͂ݏo�����Ȃ�,
			if (win_len > win_max) {
				// �͂ݏo��������߂�.
				const size_t over_len = win_len - win_max;	// �͂ݏo��������
				win_len -= over_len;
				win_ptr += over_len;
			}

			//------------------------------
			// XYZ �n�b�V���ւ̒ǉ�
			//------------------------------
			const uint8_t* const new_end = win_ptr + win_len;	// �X���C�h��̑��̏I�[
			// ���̃X���C�h�ɂ���Đ������e�o�C�g�ɂ���,
			for (auto ptr = win_end; ptr < new_end && ptr + 3 <= in_end; ptr++) {
				// 3 �o�C�g�̃L�[��, XYZ �n�b�V������������.
				// ����΂��̃y�A��, �Ȃ���ΐV���ȃy�A�𓾂�.
				// ������ꂽ�y�A�Ɋi�[���ꂽ�`�F�C����, 
				// �L�[�𐶐������Ƃ��̈ʒu���i�[����.
				auto& chain = xyz_hash[XYZ_KEY(ptr)];
				chain.push_front(ptr);
			}
		}

		//------------------------------
		// �c��̃A���t�@�x�b�g (3 �o�C�g����) ���o�͂���.
		//------------------------------
		for (; win_ptr + win_len < in_end; win_len++) {
			const uint32_t abc = win_ptr[win_len];	// �A���t�@�x�b�g
			bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, lit_tbl);
		}
		//------------------------------
		// �u���b�N�̍Ō�
		//------------------------------
		const uint32_t abc = 256;	// �f�[�^�̏I�[
		bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, lit_tbl);
		return bit_pos;
	}

	// �A���t�@�x�b�g�ɑ΂��镄���\��, �������̃A���t�@�x�b�g�ɕϊ����ďo�͂���.
	template<bool TO_BUF>
	static size_t deflate_put_len(
		std::vector<uint8_t>& out_buf,
		size_t bit_pos,
		uint32_t clen_tbl[][4],
		const uint32_t lit_tbl[][4],
		const uint32_t lit_cnt) noexcept
	{
		//	0 - 15	:	0 - 15 �̕�������\���܂��B
		//		16	:	���O�̕������l�� 3 - 6 ��R�s�[���܂��B
		//				���Ɍ���� 2 �r�b�g�́A��������\���܂��B
		//			     (0 = 3, ... , 3 = 6)
		//					��F ���� 8, 16 (+2 �r�b�g 11), 16 (+2 �r�b�g 10) �́A
		//						�������l 8 �� 12 �� (1 + 6 + 5) �Ɋg������܂��B
		//		17	:	0 �̕������� 3 - 10 ��J�Ԃ��܂��B
		//				(3 �r�b�g��)
		//		18	:	0 �̕������� 11 - 138 ��J�Ԃ��܂��B
		//				(7 �r�b�g��)
		uint32_t repeat_cnt = 0;	// �A�������.
		for (uint32_t i = 0; i < lit_cnt; i += repeat_cnt) {
			if (lit_tbl[i][CLEN] == 0) {
				// �A������[���̕������̌��𐔂���.
				for (repeat_cnt = 1;
					repeat_cnt < 138 &&
					i + repeat_cnt < lit_cnt &&
					lit_tbl[i + repeat_cnt][CLEN] == 0;
					repeat_cnt++) {
				}
				// �A������[���𕄍�������.
				if (repeat_cnt >= 3 && repeat_cnt <= 10) {
					bit_pos = deflate_put_len<TO_BUF, 47>(out_buf, bit_pos, repeat_cnt, clen_tbl);
				}
				else if (repeat_cnt >= 11 && repeat_cnt <= 138) {
					bit_pos = deflate_put_len<TO_BUF, 48>(out_buf, bit_pos, repeat_cnt, clen_tbl);
				}
				else if (repeat_cnt == 2) {
					constexpr int abc = 0;	// �A���t�@�x�b�g
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
				}
				else if (repeat_cnt == 1) {
					constexpr int abc = 0;	// �A���t�@�x�b�g
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
				}
			}
			else {
				uint32_t changed;	// ���O�̕������ƈقȂ�Ȃ� 1. �����Ȃ� 0
				if (i == 0 || lit_tbl[i][CLEN] != lit_tbl[i - 1][CLEN]) {
					const uint32_t abc = lit_tbl[i][CLEN];	// �A���t�@�x�b�g
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
					changed = 1;
				}
				else {
					changed = 0;
				}
				// �A�����铙�����������̌��𐔂���.
				for (repeat_cnt = changed;
					repeat_cnt - changed < 6 &&
					i + repeat_cnt < lit_cnt &&
					lit_tbl[i + repeat_cnt][CLEN] == lit_tbl[i - (changed ^ 1)][CLEN];
					repeat_cnt++) {
				}
				if (repeat_cnt - changed >= 3 && repeat_cnt - changed <= 6) {
					bit_pos = deflate_put_len<TO_BUF, 46>(out_buf, bit_pos, repeat_cnt - changed, clen_tbl);
				}
				else if (repeat_cnt - changed == 2) {
					const uint32_t abc = lit_tbl[i][CLEN];	// �A���t�@�x�b�g
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
				}
				else if (repeat_cnt - changed == 1) {
					const uint32_t abc = lit_tbl[i][CLEN];	// �A���t�@�x�b�g
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
				}

			}
		}
		return bit_pos;
	}

	// �𓀂���.
	// out_data	�𓀂��ꂽ�o�̓o�C�g��
	// in_data	���k���ꂽ���̓o�C�g��
	// in_len	���ۂɓ��͂����o�C�g��
	// �߂�l	���ۂɏo�͂����o�C�g��
	size_t inflate(uint8_t out_data[], const uint8_t in_data[], size_t& in_len) noexcept
	{
		size_t bit_pos = 0;
		size_t out_pos = 0;
		for (;;) {
			const auto bfinal = bit_get(in_data, bit_pos, 1); bit_pos += 1;
			const auto btype = bit_get(in_data, bit_pos, 2); bit_pos += 2;
			// 00 - �񈳏k
			if (btype == 0) {
				// �����񈳏k�f�[�^�̒��� (8 �r�b�g) ��, �����̕␔ (8 �r�b�g) ��ǂݍ���.
				const uint32_t len = (static_cast<uint16_t>(in_data[2]) << 8) | in_data[1];
				const uint32_t nlen = (static_cast<uint16_t>(in_data[4]) << 8) | in_data[3];
				if ((len | nlen) == 65535) {
					memcpy(out_data, in_data, len);
				}
			}
			else {
				uint16_t lit_tbl[LIT_ABC_MAX + 2 + DIST_ABC_MAX][3]{};
				uint16_t(*dist_tbl)[3] = nullptr;
				uint32_t lit_cnt = 0;
				uint32_t dist_cnt = 0;
				// 01 - �Œ�n�t�}�������ň��k
				if (btype == 1) {
					memcpy(lit_tbl, FIXED_LIT_TBL, sizeof(FIXED_LIT_TBL));
					lit_cnt = LIT_ABC_MAX + 2;
					dist_tbl = lit_tbl + LIT_ABC_MAX + 2;
					dist_cnt = 32;
				}
				// 10 - �J�X�^���n�t�}�������ň��k
				else if (btype == 2) {
					// HLIT: ���e����/���������̌� (5 �r�b�g). ���ۂɂ� �� -257 ���� 0 �` 29 �܂ł͈̔͂̒l�� �T�r�b�g���̒��Ɏ��߂���
					// �܂�, ���e�����ƏI�������� 257 �Ƌ�����钷�������̐����Ӗ�����.
					const auto hlit = bit_get(in_data, bit_pos, 5); bit_pos += 5;
					if (hlit > 29) {
						return static_cast<size_t>(-1);
					}
					lit_cnt = hlit + LIT_ABC_MIN;
					// HDIST�F���������̌� (5 �r�b�g). ���ۂɂ� �� -1 ���� 0 �` 31 �܂ł͈̔͂̒l
					// �܂�, ���������͍Œ� 1 �͕K�v�Ȃ��Ƃ��Ӗ�����.
					const auto hdist = bit_get(in_data, bit_pos, 5); bit_pos += 5;
					dist_cnt = hdist + DIST_ABC_MIN;
					// HCLEN�F�������� (�����\�̒��������\)�̌� (4 �r�b�g). ���ۂɂ͌� -4 ���� 0 �` 15 �܂ł͈̔�
					const auto hclen = bit_get(in_data, bit_pos, 4); bit_pos += 4;
					const auto clen = hclen + CLEN_ABC_MIN;
					uint16_t clen_tbl[CLEN_ABC_MAX][3]{};	// �����̒��������\
					for (uint16_t i = 0; i < CLEN_ABC_MAX; i++) {
						//clen_tbl[i][ABC] = CLEN_ABC_ORDER[i];
						clen_tbl[i][ABC] = i;
						clen_tbl[i][CLEN] = 0;
						clen_tbl[i][CODE] = 0;
					}
					// �����\�̒��������\�̌���, �����̒��� (3 �r�b�g) ��ǂݍ���ŕ\�Ɋi�[����.
					for (size_t i = 0; i < clen; i++) {
						clen_tbl[CLEN_ABC_ORDER[i]][CLEN] = static_cast<uint16_t>(bit_get(in_data, bit_pos, 3)); bit_pos += 3;
					}
					inflate_huffman(std::data(clen_tbl), CLEN_ABC_MAX);
					// �\�̕��т��������łȂ��̂�, �ǂݍ��񂾃T�C�Y�����łȂ�, �S�s����ѕς���.
					std::qsort(clen_tbl, CLEN_ABC_MAX, sizeof(clen_tbl[0]),
						[](const void* a, const void* b) {
							const auto a_ptr = static_cast<const uint16_t*>(a);
					const auto b_ptr = static_cast<const uint16_t*>(b);
					const auto a_clen = a_ptr[CLEN] > 0 ? a_ptr[CLEN] : static_cast<uint16_t>(-1);
					const auto b_clen = b_ptr[CLEN] > 0 ? b_ptr[CLEN] : static_cast<uint16_t>(-1);
					return (a_clen < b_clen || (a_clen == b_clen && a_ptr[ABC] < b_ptr[ABC])) ? -1 : 1;
						});
					size_t clen_cnt = CLEN_ABC_MAX;
					for (; clen_cnt != 0 && clen_tbl[clen_cnt - 1][CLEN] == 0; clen_cnt--);


					// �����\�𕜍�����.
					// ���������O�X�������ɂ�镪�����킹�Č��� HLIT + 257 + HDIST + 1 �ɂȂ�܂�, �ȉ����J��Ԃ�.
					uint32_t lit_tail = 0;	// �\�̖���
					do {
						// �r�b�g���ǂݍ����, �����\�̒������v����s��������.
						const size_t j = inflate_match(in_data, bit_pos, clen_tbl, clen_cnt, bit_pos);
						if (j == -1) {
							return static_cast<size_t>(-1);
						}
						// ���������O�X����������Ă邩���肷��.
						const auto abc = clen_tbl[j][ABC];
						if (abc <= 15) {
							if (lit_tail + 1 > lit_cnt + dist_cnt) {
								return static_cast<size_t>(-1);
							}
							// ���������O�X����������ĂȂ��̂�, ����ꂽ���e�����ƕ����̒����𕄍��\�Ɋi�[����.
							lit_tbl[lit_tail][ABC] = static_cast<uint16_t>(lit_tail < lit_cnt ? lit_tail : (lit_tail - lit_cnt));
							lit_tbl[lit_tail][CLEN] = abc;
							lit_tbl[lit_tail++][CODE] = 0;
						}
						// ���������O�X������
						else if (abc == 16) {
							if (lit_tail == 0) {
								return static_cast<size_t>(-1);
							}
							// �ЂƂO�ɏo�������l�� 3 �` 6 ��J��Ԃ���� ( �g���r�b�g�F2 ).
							// �g���r�b�g��, ��������Ȃ��̂�, �r�b�g��̋t�]�͕K�v�Ȃ�.
							constexpr size_t EX_LEN = 2;
							const auto repeat_cnt = 3 + bit_get(in_data, bit_pos, EX_LEN); bit_pos += EX_LEN;
							if (lit_tail + repeat_cnt > lit_cnt + dist_cnt) {
								return static_cast<size_t>(-1);
							}
							for (size_t k = 0; k < repeat_cnt; k++) {
								lit_tbl[lit_tail][ABC] = static_cast<uint16_t>(lit_tail < lit_cnt ? lit_tail : (lit_tail - lit_cnt));
								lit_tbl[lit_tail][CLEN] = lit_tbl[lit_tail - 1][CLEN];
								lit_tbl[lit_tail++][CODE] = 0;
							}
						}
						else if (abc == 17) {
							// "0" �� 3 �`10 ��J��Ԃ���� ( �g���r�b�g�F3 ).
							constexpr size_t EX_LEN = 3;
							const auto repeat_cnt = 3 + bit_get(in_data, bit_pos, EX_LEN); bit_pos += EX_LEN;
							if (lit_tail + repeat_cnt > lit_cnt + dist_cnt) {
								return static_cast<size_t>(-1);
							}
							for (size_t k = 0; k < repeat_cnt; k++) {
								lit_tbl[lit_tail][ABC] = static_cast<uint16_t>(lit_tail < lit_cnt ? lit_tail : (lit_tail - lit_cnt));
								lit_tbl[lit_tail][CLEN] = 0;
								lit_tbl[lit_tail++][CODE] = 0;
							}
						}
						else if (abc == 18) {
							// "0" �� 11 �` 138 ��J��Ԃ���� ( �g���r�b�g�F7 ).
							constexpr size_t EX_LEN = 7;
							const auto repeat_cnt = 11 + bit_get(in_data, bit_pos, EX_LEN); bit_pos += EX_LEN;
							if (lit_tail + repeat_cnt > lit_cnt + dist_cnt) {
								return static_cast<size_t>(-1);
							}
							for (size_t k = 0; k < repeat_cnt; k++) {
								lit_tbl[lit_tail][ABC] = static_cast<uint16_t>(lit_tail < lit_cnt ? lit_tail : (lit_tail - lit_cnt));
								lit_tbl[lit_tail][CLEN] = 0;
								lit_tbl[lit_tail++][CODE] = 0;
							}
						}
						else {
							return static_cast<size_t>(-1);
						}
					} while (lit_tail < lit_cnt + dist_cnt);
					inflate_huffman(lit_tbl, lit_cnt);
					std::qsort(lit_tbl, lit_cnt, sizeof(lit_tbl[0]),
						[](const void* a, const void* b) {
							const auto a_ptr = static_cast<const uint16_t*>(a);
					const auto b_ptr = static_cast<const uint16_t*>(b);
					const auto a_clen = a_ptr[CLEN] > 0 ? a_ptr[CLEN] : static_cast<uint16_t>(UINT32_MAX);
					const auto b_clen = b_ptr[CLEN] > 0 ? b_ptr[CLEN] : static_cast<uint16_t>(UINT32_MAX);
					return (a_clen < b_clen || (a_clen == b_clen && a_ptr[ABC] < b_ptr[ABC])) ? -1 : 1;
						});
					dist_tbl = lit_tbl + lit_cnt;
					inflate_huffman(dist_tbl, dist_cnt);
					std::qsort(dist_tbl, dist_cnt, sizeof(dist_tbl[0]),
						[](const void* a, const void* b) {
							const auto a_ptr = static_cast<const uint16_t*>(a);
					const auto b_ptr = static_cast<const uint16_t*>(b);
					const auto a_clen = a_ptr[CLEN] > 0 ? a_ptr[CLEN] : static_cast<uint16_t>(UINT32_MAX);
					const auto b_clen = b_ptr[CLEN] > 0 ? b_ptr[CLEN] : static_cast<uint16_t>(UINT32_MAX);
					return (a_clen < b_clen || (a_clen == b_clen && a_ptr[ABC] < b_ptr[ABC])) ? -1 : 1;
						});
				}
				else {
					return static_cast<size_t>(-1);
				}
				for (;;) {
					size_t i = inflate_match(in_data, bit_pos, lit_tbl, lit_cnt, bit_pos);
					if (i == static_cast<size_t>(-1)) {
						return static_cast<size_t>(-1);
					}
					const uint32_t abc = lit_tbl[i][ABC];
					if (abc == 256) {
						in_len = (bit_pos + 7) / 8;
						if (bfinal) {
							return out_pos;
						}
						break;
					}
					else if (abc <= 255) {
						out_data[out_pos++] = static_cast<uint8_t>(abc);
					}
					else {
						// �P�꒷�����߂�.
						uint32_t word_len;
						if (abc >= 257 && abc <= 264) {
							//constexpr int k = 0;
							word_len = abc - 254;
							//len = 3ull + (2 << (1 + k)) + ((lit - (261 + 4 * k)) << k);
						}
						else if (abc >= 265 && abc <= 284) {
							const int k = (abc - 261) >> 2;
							word_len = 3ul + (2ul << (1 + k)) + ((abc - (261 + 4 * k)) << k) + bit_get(in_data, bit_pos, k); bit_pos += k;
						}
						else if (abc == 285) {
							word_len = 258;
						}
						else {
							return static_cast<size_t>(-1);
						}
						// ���������߂�.
						uint32_t dist;
						size_t j = inflate_match(in_data, bit_pos, dist_tbl, dist_cnt, bit_pos);
						const auto dist_abc = dist_tbl[j][ABC];
						if (dist_abc <= 3) {
							// ���� 1-4
							dist = dist_abc + 1;
							//constexpr int k = 0;
							//dist = 1 + (2 << k) + ((dist_lit - (2 + 2 * k)) << k);
						}
						else if (dist_abc >= 4 && dist_abc <= 29) {
							const int blen = (dist_abc - 2) >> 1;	// �g���r�b�g��
							dist = 1ul + (2ul << blen) + ((dist_abc - (2 + 2 * blen)) << blen);
							const auto ex_bit = bit_get(in_data, bit_pos, blen); bit_pos += blen;	// �g���r�b�g
							dist += ex_bit;
						}
						else {
							return static_cast<size_t>(-1);
						}
						if (dist > out_pos) {
							return static_cast<size_t>(-1);
						}
						// �O������R�s�[.
						for (uint32_t k = 0; k < word_len; k++) {
							out_data[out_pos + k] = out_data[out_pos - dist + k];
						}
						out_pos += word_len;
					}
				}
			}
		}
	}

	// �������ɂ��ƂÂ��ăn�t�}��������ݒ肷��.
	// T	�����\�̗�̌^. �L���Ȃ�� uint16_t, ���k�Ȃ�� uint32_t (�o���񐔂��i�[���邽��)
	// M	�����\�̗�
	// tbl	�����\ (�A���t�@�x�b�g��, �܂��͕�����/�A���t�@�x�b�g��)
	// cnt	�����\�̍s��
	// �߂�l	�����̒������[���łȂ��s��.
	template <typename T, int M>
	static void inflate_huffman(T tbl[][M], int cnt) noexcept
	{
		constexpr int MAX_BITS = 15;
		// 1) �e�������ɑ΂��镄���̐��𐔂��܂��Bbl_count[N] �ɁA1 �ȏ�̒��� N �̕����̐��������܂��B
		std::array<uint32_t, MAX_BITS + 1> bl_count{};
		for (size_t i = 0; i < cnt; i++) {
			bl_count[tbl[i][CLEN]]++;
		}
		// 2) �e�������ɑ΂���ł������������̐��l�������܂�
		std::array<uint32_t, MAX_BITS + 1> next_code{};
		uint32_t code = 0;
		bl_count[0] = 0;
		for (size_t bits = 1; bits <= MAX_BITS; bits++) {
			code = (code + bl_count[bits - 1]) << 1;
			next_code[bits] = code;
		}
		// 3) step 2 �Ō��肵����{�l���g���āA���������̂��ׂĂ̕����ɑ΂���A�������l���g���āA
		// ���ׂĂ̕����ɐ��l������t���܂��B�S���g���Ȃ������i�r�b�g���� 0 �ł������j�����́A
		// �l������t�����Ă͂����܂���B
		for (size_t n = 0; n < cnt; n++) {
			const auto len = tbl[n][CLEN];
			if (len != 0) {
				tbl[n][CODE] = static_cast<T>(next_code[len]);
				next_code[len]++;
			}
		}
	}


	// �r�b�g���ǂݍ����, �����\�����v����s��������.
	static size_t inflate_match(const uint8_t data[], const size_t bit_pos, const uint16_t tbl[][3], const size_t tbl_cnt, size_t& new_pos) noexcept
	{
		auto pos = bit_pos;
		// �����\�̒�����, �ŏ��̒�����������������, ���̕����̒������̃r�b�g���ǂݍ���.
		// �\��, ���������ŕ��ъ������Ă���̂�, �ŏ��̍s�ƂȂ�.
		// �ǂݍ��񂾃r�b�g���, �n�t�}�������Ȃ̂�, �r�b�g���т��t�]������.
		size_t j = 0;
		auto code = bit_rev(bit_get(data, pos, tbl[j][CLEN]), tbl[j][CLEN]); pos += tbl[j][CLEN];
		// �r�b�g��, �ŏ��̍s�̕����ƈ�v���邩���肷��.
		if (code != tbl[j][CODE]) {
			// �s��v�Ȃ�, ���̍s�̕����𔻒肷��.
			for (j = 1; j < tbl_cnt && tbl[j][CLEN] > 0; j++) {
				// �O�̍s��, �����̒�������v���邩���肷��.
				if (tbl[j][CLEN] > tbl[j - 1][CLEN]) {
					// ��v���Ȃ��Ȃ獷���̃r�b�g�ǂݍ���, �r�b�g��ɒǉ�����.
					const auto diff_len = tbl[j][CLEN] - tbl[j - 1][CLEN];
					code = (code << diff_len) | bit_rev(bit_get(data, pos, diff_len), diff_len); pos += diff_len;
				}
				// ��v�Ȃ璆�f����.
				if (code == tbl[j][CODE]) {
					break;
				}
			}
		}
		// ��v���镄�����Ȃ������肷��.
		if (j >= tbl_cnt || tbl[j][CLEN] == 0) {
			return static_cast<size_t>(-1);
		}
		new_pos = pos;
		return j;
	}

	// �o�C�g��� zlib �X�g���[���Ɉ��k����.
	void z_compress(std::vector<uint8_t>& z_buf, /*<---*/const uint8_t in_data[], const size_t in_len) noexcept
	{
		constexpr uint32_t CM = 8;
		const uint32_t CINFO = 7;
		constexpr uint32_t FDICT = 0;
		const uint32_t FLEVEL = 0;
		const uint8_t cmf = static_cast<uint8_t>(CM | (CINFO << 4));
		uint8_t flg = static_cast<uint8_t>((FDICT << 5) | (FLEVEL << 6));
		uint32_t fcheck;
		for (fcheck = 0; ((static_cast<uint32_t>(cmf) << 8) + static_cast<uint32_t>(flg) + fcheck) % 31 != 0; fcheck++);
		flg |= static_cast<uint8_t>(fcheck);
		z_buf.push_back(cmf);
		z_buf.push_back(flg);
		deflate(z_buf, /*<===*/ in_data, in_len);
		const uint32_t checksum = ADLER32::update(in_data, in_len);
		z_buf.push_back(static_cast<uint8_t>(checksum >> 24));
		z_buf.push_back(static_cast<uint8_t>(checksum >> 16));
		z_buf.push_back(static_cast<uint8_t>(checksum >> 8));
		z_buf.push_back(static_cast<uint8_t>(checksum));
	}

	// zlib �X�g���[�����o�C�g��ɉ𓀂���.
	// out_data	�𓀂��ꂽ�o�C�g��
	// z_data	���k���ꂽ�o�C�g��
	bool z_decompress(uint8_t out_data[], /*<---*/const uint8_t z_data[]) noexcept
	{
		// zlib �X�g���[���̍ŏ��� 2 �o�C�g.
		//               CMF                         FLG
		// +----------------------------+----------------------------+
		// |CMINFO(4)    |CM(4)         |FLEVEL(2)|FDICT(1)|FCHECK(5)|
		// +----------------------------+----------------------------+
		const uint8_t cmf = z_data[0];	// ���k������, ���k�����̏��
		const uint8_t flg = z_data[1];	// CMF �� FLG �̃`�F�b�N�r�b�g, �v���Z�b�g����, ���k���x��
		const uint8_t cm = (cmf & 0x0F);	// ���k����
		if (cm == 8)
		{
			//const uint8_t cinfo = (cmf >> 4);	// ���k�����̏��
			//const uint32_t window_size = 1ul << (static_cast<int>(cinfo) + 8);	// CM=8 �Ȃ�E�B���h�E�T�C�Y�̑ΐ�-1
			//const uint8_t fcheck = (flg & 0x1f);	// CMF �� FLG �̃`�F�b�N�r�b�g
			const uint8_t fdict = ((flg >> 5) & 1);	// �v���Z�b�g����
			//const uint8_t flevel = ((flg >> 6) & 3);	// ���k���x�� (0...3)
			// CMF �� FLG �� MSB �̏����Œ~�ς��ꂽ 16 �r�b�g�̕����Ȃ������iCMF*256 + FLG�j�Ƃ��Ă݂��Ƃ���,
			// ���̒l�� 31 �̔{���ƂȂ�悤�ɁAFCHECK �̒l����`����Ȃ���΂����܂���
			if (((static_cast<uint32_t>(cmf) << 8) + flg) % 31ul == 0) {
				// PNG �̓v���Z�b�g�������g��Ȃ��̂�, fdict �̓[��.
				if (fdict == 0) {
					const auto in_data = z_data + 2;
					size_t in_len = 0;
					const auto out_len = inflate(out_data, in_data, in_len);
					if (out_len < static_cast<size_t>(-1)) {
						// �W�J�����f�[�^�� ADLER32 �`�F�b�N�T���� zlib �X�g���[���̃`�F�b�N�T������v���邩���肷��.
						if (ADLER32::update(out_data, out_len) == BYTE_TO_U32(in_data + in_len)) {
							return true;
						}
					}
				}
				else {
					// fdict ���[���łȂ�, 
					// �܂薢�m�̎������g���Ă���̂�����,
					// �G���[.
				}
			}
		}
		return false;
	}
}