#include <algorithm>
#include <array>
#include <unordered_map>
#include "zlib.h"

// RFC 1951 DEFLATE Compressed Data Format Specification version 1.3 日本語訳
// https://www.futomi.com/lecture/japanese/rfc1951.html
// https://wiki.suikawiki.org/n/DEFLATE
// 七誌の開発日記
// https://7shi.hateblo.jp/entry/20110719/1311093479
// RFC 1950 ZLIB Compressed Data Format Specification version 3.3 日本語訳
// https://www.futomi.com/lecture/japanese/rfc1950.html

namespace winrt::Zlib::implementation
{
	// 符号表の定数
	static constexpr int ABC = 0;	// アルファベットの添え字
	static constexpr int CLEN = 1;	// 符号長の添え字
	static constexpr int CODE = 2;	// 符号の添え字
	static constexpr int FREQ = 3;	// 出現回数の添え字
	static constexpr uint32_t LIT_ABC_MAX = 286;	// Max # of literal/length alphabet.
	static constexpr uint32_t LIT_ABC_MIN = 257;	// Min # of Literal/Length alphabet.
	static constexpr uint32_t DIST_ABC_MAX = 32;	// Max # of Distance alphabet.
	static constexpr uint32_t DIST_ABC_MIN = 1;	// Min # of Distance alphabet.
	static constexpr uint32_t CLEN_ABC_MAX = 19;	// Max # of Code Length alphabet.
	static constexpr uint32_t CLEN_ABC_MIN = 4;	// Min # of Code Length alphabet.
	static constexpr uint16_t CLEN_ABC_ORDER[CLEN_ABC_MAX]	// 符号長のアルファベットの順番
	{
		16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
	};

	static constexpr uint16_t FIXED_LIT_TBL[][3]	// 固定ハフマン符号表
	{
		// 固定ハフマン符号の作成
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

	constexpr uint32_t LEN_CODE_TBL[][4]	// 長さ符号表
	{
		//			 拡張
		//	符号		ビット数	 長さ
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
		//			 拡張
		// 符号		ビット数	 距離
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
		//			 拡張
		// 符号		ビット数	 回数
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

	// ビット列をバイト配列から読み込む.
	// data	バイト配列
	// bit_pos	ビット位置
	// bit_len	ビット長 (最長 15 ビット)
	inline uint32_t bit_get(const uint8_t data[], const size_t bit_pos, const size_t bit_len) noexcept
	{
		const auto ptr = data + bit_pos / 8;	// バイト境界ポインター
		const auto pos = bit_pos % 8;	// バイト境界で剰余したビット位置
		const auto len = pos + bit_len;	// バイト境界から数えたビット長
		const uint32_t mask = UINT16_MAX >> (16 - bit_len);	// ビットマスク
		uint32_t val;	// バイト境界に合わせた値

		// ビット列がどの大きさバイト境界をまたぐか判定する.
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

	// ビット列を動的バイト配列に書き込む.
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
		const auto pos = bit_pos % 8;	// バイト境界で剰余したビット位置
		uint32_t val = bit_val << pos;	// バイト境界に合わせた値
		auto ptr = data + bit_pos / 8;	// バイト境界に合わせたポインター
		const uint8_t mask = UINT8_MAX >> (8 - pos);	// バイト境界に合わせたマスク
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

	// 圧縮する.
	// out_buf	出力バッファ
	// in_data	入力データ
	// in_len	入力データ長
	void deflate(std::vector<uint8_t>& out_buf, /*<---*/const uint8_t in_data[], const size_t in_len) noexcept
	{
		constexpr size_t IN_BLOCK = static_cast<size_t>(65536) - 1;
		constexpr auto TO_TBL = false;
		constexpr auto TO_BUF = true;
		uint32_t clen_tbl[CLEN_ABC_MAX][4]{};	// 符号長アルファベットに対する符号表
		uint32_t lit_tbl[LIT_ABC_MAX][4]{};	// リテラル/長さアルファベットに対する符号表
		uint32_t dist_tbl[DIST_ABC_MAX][4]{};	// 距離アルファベットに対する符号表
		std::vector<uint8_t> empty_buf;

		size_t bit_pos = out_buf.size() * 8;	// ビット位置
		for (size_t len = 0; len < in_len; len += IN_BLOCK) {

			// 各表の初期化.
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

			// リテラル/長さアルファベットと距離アルファベットの出現回数を数える.
			const size_t block_len = std::min(in_len - len, IN_BLOCK);	// ブロック長
			deflate_put_data<TO_TBL>(empty_buf, 0, lit_tbl, dist_tbl, in_data + len, block_len);

			// 出現回数にもとづいて符号長を得る.
			// リテラル/長さアルファベットの個数を求める.
			// 末尾の連続する符号長ゼロのアルファベットは個数に含まない.
			// 符号長にもとづいてハフマン符号を設定する.
			// ただし, 個数は最小個数未満にはならない.
			deflate_huffman<15, LIT_ABC_MAX>(lit_tbl);
			uint32_t lit_cnt = LIT_ABC_MAX;	// リテラル/長さアルファベットの個数
			for (; lit_cnt > 0 && lit_tbl[lit_cnt - 1][CLEN] == 0; lit_cnt--);
			inflate_huffman(lit_tbl, lit_cnt);
			lit_cnt = std::max(LIT_ABC_MIN, lit_cnt);

			// 出現回数にもとづいて符号長を得る.
			// 距離アルファベットの個数を求める.
			// 末尾の連続する符号長ゼロのアルファベットは個数に含まない.
			// 符号長にもとづいてハフマン符号を設定する.
			// ただし, 個数は最小個数未満にはならない.
			deflate_huffman<15, DIST_ABC_MAX>(dist_tbl);
			uint32_t dist_cnt = DIST_ABC_MAX;	// 距離アルファベットの個数
			for (; dist_cnt > 0 && dist_tbl[dist_cnt - 1][CLEN] == 0; dist_cnt--);
			inflate_huffman(dist_tbl, dist_cnt);
			dist_cnt = std::max(DIST_ABC_MIN, dist_cnt);

			// 符号長アルファベットの出現回数を数える.
			deflate_put_len<TO_TBL>(empty_buf, 0, clen_tbl, lit_tbl, lit_cnt);
			deflate_put_len<TO_TBL>(empty_buf, 0, clen_tbl, dist_tbl, dist_cnt);

			// 出現回数にもとづいて符号長を得る.
			// 符号長にもとづいてハフマン符号を設定する.
			// 符号長アルファベットの個数を求める.
			// 末尾の連続する符号長ゼロのアルファベットは個数に含まない.
			// ただし, 最小個数未満にはならず, アルファベット順ではない.
			deflate_huffman<7, CLEN_ABC_MAX>(clen_tbl);
			inflate_huffman(clen_tbl, CLEN_ABC_MAX);
			uint32_t clen_cnt = CLEN_ABC_MAX;	// 符号長アルファベットの個数
			for (; clen_cnt > CLEN_ABC_MIN; clen_cnt--) {
				if (clen_tbl[CLEN_ABC_ORDER[clen_cnt - 1]][CLEN] != 0) {
					break;
				}
			}

			//  3 ビットのヘッダー (第 1 ビット BFINAL, 次の 2 ビット BTYPE) を出力する.
			// 最終ブロックなら, BFINAL は 1.
			bit_put(out_buf, bit_pos, 1, in_len - len <= IN_BLOCK ? 1 : 0); bit_pos += 1;	// BFINAL
			bit_put(out_buf, bit_pos, 2, 2); bit_pos += 2;	// BTYPE
			// 各アルファベットの個数.
			bit_put(out_buf, bit_pos, 5, lit_cnt - LIT_ABC_MIN); bit_pos += 5;	// HLIT
			bit_put(out_buf, bit_pos, 5, dist_cnt - DIST_ABC_MIN); bit_pos += 5;	// HDIST
			bit_put(out_buf, bit_pos, 4, clen_cnt - CLEN_ABC_MIN); bit_pos += 4;	// HCLEN
			// 符号長アルファベットの符号長を出力する.
			for (size_t i = 0; i < clen_cnt; i++) {
				const int j = CLEN_ABC_ORDER[i];
				bit_put(out_buf, bit_pos, 3, clen_tbl[j][CLEN]); bit_pos += 3;
			}
			// リテラル/長さと距離の, 各アルファベットの符号長とその拡張ビットを出力する.
			bit_pos = deflate_put_len<TO_BUF>(out_buf, bit_pos, clen_tbl, lit_tbl, lit_cnt);
			bit_pos = deflate_put_len<TO_BUF>(out_buf, bit_pos, clen_tbl, dist_tbl, dist_cnt);
			// アルファベットに対応する符号をバッファに書き込む.
			bit_pos = deflate_put_data<TO_BUF>(out_buf, bit_pos, lit_tbl, dist_tbl, in_data + len, std::min(in_len - len, IN_BLOCK));
		}
		// 出力バッファの大きさを合わせる.
		out_buf.resize((bit_pos + 7) / 8);
		out_buf.shrink_to_fit();
	}

	// 出現回数にもとづいて符号長を得る.
	// B	符号の最大ビット長
	// N	符号表の最大行数
	// M	符号表の列数
	template <uint32_t B, uint32_t N, uint32_t M>
	static void deflate_huffman(uint32_t lit_tbl[N][M]) noexcept
	{
		// パッケージ・マージ・アルゴリズム
		// 
		// 1. テーブルを出現回数順に並び替える. 要素が奇数個なら末尾は無視される
		// 2. 並んだ要素 2 つずつを順番にパッケージ化する.
		//    2 つの要素の, アルファベットを連結, 出現回数を加算. 
		//    それぞれ, パッケージのアルファベットと出現回数に代入する.
		// 2. パッケージを元のテーブルにマージする.
		// 3. 上の 1 - 2 を B - 1 回繰り返す.
		// 4. テーブルを出現回数順に並び替える. 要素が奇数個なら末尾は無視される
		// 5. 最後に得られたテーブルに含まれるアルファベットをアルファベット毎に数える.
		//    その各計がハフマン符号の長さになる.
		// 
		// アルファベットとその出現回数の組を要素とするテーブル
		// +-----+-----+-----+-----+-----+
		// | A 2 | B 6 | C10 | D 8 | E 4 |
		// +-----+-----+-----+-----+-----+
		// 
		//                       奇数個なら末尾は
		// 並び替え                    無視      パッケージ化
		// +-----+-----+-----+-----+ +-----+    +-----+-----+
		// | A 2 | E 4 | B 6 | D 8 |-| C10 | => |AE 6 |BD 14|
		// +-----+-----+-----+-----+ +-----+    +-----+-----+
		// 
		// マージ
		// +-----+-----+-----+-----+-----+ +-----+-----+
		// | A 2 | B 6 | C10 | D 8 | E 4 |+|AE 6 |BD 14|
		// +-----+-----+-----+-----+-----+ +-----+-----+
		// 
		//                                   奇数個なら末尾は
		// 並び替え                                無視      パッケージ化
		// +-----+-----+-----+-----+-----+-----+ +-----+    +-----+-----+-----+
		// | A 2 | E 4 | B 6 |AE 6 | D 8 | C10 |-|BD 14| => |AE 6 |BAE12|DC 18|
		// +-----+-----+-----+-----+-----+-----+ +-----+    +-----+-----+-----+
		// 
		// マージ
		// +-----+-----+-----+-----+-----+ +-----+-----+-----+
		// | A 2 | B 6 | C10 | D 8 | E 4 |+|AE 6 |BAE12|DC 18|
		// +-----+-----+-----+-----+-----+ +-----+-----+-----+
		// 
		// 並び替え                                       　　　 パッケージ化
		// +-----+-----+-----+-----+-----+-----+-----+-----+    +-----+-----+-----+-------+
		// | A 2 | E 4 | B 6 |AE 6 | D 8 | C10 |BAE12|DC 18| => |AE 6 |BAE12|DC 18|BAEDC30|
		// +-----+-----+-----+-----+-----+-----+-----+-----+    +-----+-----+-----+-------+
		// 
		// マージ
		// +-----+-----+-----+-----+-----+ +------+-----+-----+-------+
		// | A 2 | B 6 | C10 | D 8 | E 4 |+|AE 6 |BAE12|DC 18|BAEDC30|
		// +-----+-----+-----+-----+-----+ +------+-----+-----+-------+
		//                                                奇数個なら末尾は
		// 並び替え                                             無視
		// +-----+-----+-----+-----+-----+-----+-----+-----+ +-------+
		// | A 2 | E 4 | B 6 |AE 6 | D 8 | C10 |BAE12|DC 18|-|BAEDC30|
		// +-----+-----+-----+-----+-----+-----+-----+-----+ +-------+
		// 
		// 結果
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

		// パッケージ
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

		// 初期化
		std::vector<PACKAGE> tbl(PACKAGE_MAX);	// スタック警告を出さないために vector
		std::array<PACKAGE, PACKAGE_MAX / 2> pkg;
		std::array<uint32_t, PACKAGE_MAX> idx;

		// 出現回数 0 のアルファベットは取り除く.
		uint32_t tbl_cnt = 0;
		for (int i = 0; i < N; i++) {
			if (lit_tbl[i][FREQ] > 0) {
				tbl[tbl_cnt].abc.push_back(lit_tbl[i][ABC]);
				tbl[tbl_cnt].freq = lit_tbl[i][FREQ];
				idx[tbl_cnt++] = tbl_cnt;
			}
			lit_tbl[i][CLEN] = 0;
		}

		// 並び替え
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
			// マージ
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
			// 並び替え
			for (uint32_t i = 0; i < a; i++) {
				idx[i] = i;	// 添え字の初期化
			}
			auto end = std::begin(idx);
			std::advance(end, a);
			std::sort(std::begin(idx), end,
				[tbl](const uint32_t& a, const uint32_t& b) {
					return tbl[a].freq < tbl[b].freq;
				});
			// 奇数個なら末尾は取り除く
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
		// ペアから, 出現位置が格納されたチェインを取り出す.
		for (const auto ptr : xyz_chain) {
			// 位置がウィンドウ窓の範囲外になったなら,
			if (ptr < win_ptr) {
				// その個数を数え, 次の出現位置に進む.
				cnt_out++;
				continue;
			}
			// 単語長を 1 バイトずつ増やしながら比較する.
			// ハッシュによって 3 バイトは一致済み.
			size_t len = 3;	// 一致した長さ
			for (;
				// 単語の最大長さ以下
				len < 258 &&
				// データの終端は超えない.
				win_ptr + win_len + len < in_end &&
				ptr[len] == win_ptr[win_len + len];
				len++) {
			}
			// より後方にある最長一致を得る.
			if (len >= word_len) {
				word_len = len;
				word_pos = ptr - win_ptr;
			}
		}

		//------------------------------
		// チェインから, 範囲外になった位置を削除する.
		//------------------------------
		for (; cnt_out > 0; cnt_out--) {
			xyz_chain.pop_front();
		}

	}

	// アルファベットに対応する符号を出力する.
	// out_buf	出力バッファ
	// bit_pos	出力するビット位置
	// abc	アルファベット
	// lit_tbl	符号表
	// 戻り値	出力した符号のビット長
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

	// 長さアルファベットに対応する符号とその拡張ビットを出力する.
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
			constexpr auto ex_len = static_cast<size_t>(LEN_CODE_TBL[I][1]);	// 拡張ビット長
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
		// 窓のスライド
		//------------------------------
		// スライド窓長が最大長以下なら,
		if (len < win_max) {
			// ポインターはそのままで, スライド窓長のみ増やす.
			len += word_len;
		}
		// それ以外なら,
		else {
			// 長さはそのままでスライド窓ポインターのみ増やす.
			ptr += word_len;
		}
		// スライド窓が入力データからはみ出したなら,
		if (len > win_max) {
			// はみ出した分を戻す.
			const size_t over_len = len - win_max;	// はみ出した長さ
			len -= over_len;
			ptr += over_len;
		}
		new_len = len;
		new_ptr = ptr;
	}

	//---------------------------------
	// なんちゃってリングバッファ
	// 最大要素数は RING_BUF_MAX - 1
	// RING_BUF_MAX は 2 の n 乗に限定
	// 追加は先頭のみで, 削除はできない.
	// 溢れた要素は自動的に削除される.
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

	// リテラル/長さアルファベットと距離アルファベットの出現回数を数える.
	// または, 
	// アルファベットに対応する符号をバッファに書き込む.
	// TO_BUF	true ならばバッファに出力, false ならば出現回数を数える.
	// out_buf	出力バッファ
	// bit_pos	出力ビット位置
	// lit_tbl	リテラル/長さアルファベットに対する符号表 (アルファベット順)
	// dist_tbl	距離アルファベットに対する符号表 (アルファベット順)
	// in_data	入力データ
	// in_len	入力データ長
	template<bool TO_BUF>
	static size_t deflate_put_data(
		std::vector<uint8_t>& out_buf,
		size_t bit_pos,
		uint32_t lit_tbl[][4],
		uint32_t dist_tbl[][4],
		const uint8_t in_data[],
		const size_t in_len) noexcept
	{
		// 単語 (3 バイト) の出現位置を検索するハッシュ.
		// キーと値のペアが格納される.
		// キーは, データ 3 バイトを整数にした値.
		// 値は, 単語の出現位置を順に格納したチェイン (リスト)
		//
		//  ペア
		// キー:値    出現位置
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

		// 符号表の出現回数を初期化.
		// 符号表はアルファベット順になっていること.
		for (int i = 0; i < LIT_ABC_MAX; i++) {
			lit_tbl[i][FREQ] = 0;
		}
		for (int i = 0; i < DIST_ABC_MAX; i++) {
			dist_tbl[i][FREQ] = 0;
		}

		// スライディング窓アルゴリズム
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
		const uint8_t* const in_end = in_data + in_len;	// 入力データの終端
		const size_t win_max = std::min(32768ull, in_len);	// スライド窓の最大幅
		const uint8_t* win_ptr = in_data;	// スライド窓ポインター
		size_t win_len = 0;	// スライド窓幅
		size_t word_len = 0;	// 単語長
		size_t word_pos = 0;	// (スライド窓の中での) 単語位置
		//int lazy_cnt = 0;

		// 一致する単語長は 3 バイト以上なので,
		// 残りのバイト数が 3 バイト未満にならない間繰り返す.
		while (win_ptr + win_len <= in_end - 3) {

			/*
			//------------------------------
			// スライド窓の中から, 最長一致する単語を探す.
			//------------------------------
			// 最長一致のリニアサーチ
			word_len = 1;	// 単語長
			const auto word_abc = win_ptr[win_len];	// 単語の最初のバイト
			for (size_t pos = 0;	// 一致した位置
				pos < win_len; pos++) {
				if (win_ptr[pos] != word_abc) {
					continue;
				}
				// 単語長を 1 バイトずつ増やしながら比較する.
				size_t len = 1;	// 一致した長さ (1 バイトは一致済み)
				for (;
					// 単語の最大長さ以下
					len < 258 &&
					// データの終端は超えない.
					win_ptr + win_len + len < in_end &&
					win_ptr[pos + len] == win_ptr[win_len + len];
					len++) {
				}
				// '>=' は, より後方にある最長一致を得るため.
				if (len >= word_len) {
					word_len = len;
					word_pos = pos;
				}
			}
			*/

			//------------------------------
			// スライド窓の中から, 最長一致する単語を探す.
			//------------------------------
			// 最長一致のハッシュサーチ
			word_len = 1;	// 単語長
			const uint32_t xyz_key = XYZ_KEY(win_ptr + win_len);	// XYZ ハッシュキー

			// キーに合致する要素 (ペア) が XYZ ハッシュに中にあるなら,
			if (xyz_hash.count(xyz_key) > 0) {
				auto& chain = xyz_hash[xyz_key];	// ペアに格納されたチェイン
				//int cnt = 0;
				// チェインに格納された各位置について.
				for (int i = 0; chain[i] != nullptr; i++) {
					const uint8_t* ptr = chain[i];
					//for (const auto ptr : chain) {
					if (ptr < win_ptr) {
						break;
					}
					// 単語長を 1 バイトずつ増やしながら比較する.
					// ハッシュによって 3 バイトは一致済み.
					size_t len = 3;	// 一致した長さ
					for (;
						// 単語の最大長さ以下
						len < 258 &&
						// データの終端は超えない.
						win_ptr + win_len + len < in_end &&
						ptr[len] == win_ptr[win_len + len];
						len++) {
					}
					// より後方にある最長一致を得る.
					if (len > word_len) {
						word_len = len;
						word_pos = ptr - win_ptr;
					}
					//cnt++;
				}
				/*
				//------------------------------
				// チェインから, 範囲外になった位置を削除する.
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

						const uint32_t lazy_key = XYZ_KEY(lazy_ptr + lazy_len);	// XYZ ハッシュキー
						auto& lazy_chain = xyz_hash[lazy_key];	// 出現位置が格納されたチェイン
						for (const auto ptr : lazy_chain) {
							// 位置がウィンドウ窓の範囲外なら,
							if (ptr < lazy_ptr) {
								// 無視する.
								continue;
							}
							// 単語長を 1 バイトずつ増やしながら比較する.
							// ハッシュによって 3 バイトは一致済み.
							size_t len = 3;	// 一致した長さ
							for (;
								// 単語の最大長さ以下
								len < 258 &&
								// データの終端は超えない.
								lazy_ptr + lazy_len + len < in_end &&
								ptr[len] == lazy_ptr[lazy_len + len];
								len++) {
							}
							// より長い最長一致を得る.
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

			// 単語が見つからない, または見つかっても単語長が 2 バイト以下なら,
			if (word_len >= 0 && word_len <= 2) {
				//------------------------------
				// アルファベットに対応する符号をそのまま出力
				//------------------------------
				const uint32_t abc = win_ptr[win_len];	// アルファベット
				bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, lit_tbl);
				word_len = 1;
			}

			// 単語長が 3 バイト以上の単語が見つかったなら,
			else {
				//------------------------------
				// 長さ符号, 距離符号 (とその拡張ビット列) を出力する.
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
				const auto dist = win_len - word_pos;	// 距離
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
			// 窓のスライド
			//------------------------------
			const auto win_end = win_ptr + win_len;	// スライド前の窓の終端
			// スライド窓長が最大長以下なら,
			if (win_len < win_max) {
				// ポインターはそのままで, スライド窓長のみ増やす.
				win_len += word_len;
			}
			// それ以外なら,
			else {
				// 長さはそのままでスライド窓ポインターのみ増やす.
				win_ptr += word_len;
			}
			// スライド窓が入力データからはみ出したなら,
			if (win_len > win_max) {
				// はみ出した分を戻す.
				const size_t over_len = win_len - win_max;	// はみ出した長さ
				win_len -= over_len;
				win_ptr += over_len;
			}

			//------------------------------
			// XYZ ハッシュへの追加
			//------------------------------
			const uint8_t* const new_end = win_ptr + win_len;	// スライド後の窓の終端
			// 窓のスライドによって生じた各バイトについて,
			for (auto ptr = win_end; ptr < new_end && ptr + 3 <= in_end; ptr++) {
				// 3 バイトのキーで, XYZ ハッシュを検索する.
				// あればそのペアを, なければ新たなペアを得る.
				// 得えられたペアに格納されたチェインに, 
				// キーを生成したときの位置を格納する.
				auto& chain = xyz_hash[XYZ_KEY(ptr)];
				chain.push_front(ptr);
			}
		}

		//------------------------------
		// 残りのアルファベット (3 バイト未満) を出力する.
		//------------------------------
		for (; win_ptr + win_len < in_end; win_len++) {
			const uint32_t abc = win_ptr[win_len];	// アルファベット
			bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, lit_tbl);
		}
		//------------------------------
		// ブロックの最後
		//------------------------------
		const uint32_t abc = 256;	// データの終端
		bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, lit_tbl);
		return bit_pos;
	}

	// アルファベットに対する符号表を, 符号長のアルファベットに変換して出力する.
	template<bool TO_BUF>
	static size_t deflate_put_len(
		std::vector<uint8_t>& out_buf,
		size_t bit_pos,
		uint32_t clen_tbl[][4],
		const uint32_t lit_tbl[][4],
		const uint32_t lit_cnt) noexcept
	{
		//	0 - 15	:	0 - 15 の符号長を表します。
		//		16	:	直前の符号長値を 3 - 6 回コピーします。
		//				次に現れる 2 ビットは、反復長を表します。
		//			     (0 = 3, ... , 3 = 6)
		//					例： 符号 8, 16 (+2 ビット 11), 16 (+2 ビット 10) は、
		//						符号長値 8 が 12 個 (1 + 6 + 5) に拡張されます。
		//		17	:	0 の符号長を 3 - 10 回繰返します。
		//				(3 ビット長)
		//		18	:	0 の符号長を 11 - 138 回繰返します。
		//				(7 ビット長)
		uint32_t repeat_cnt = 0;	// 連続する個数.
		for (uint32_t i = 0; i < lit_cnt; i += repeat_cnt) {
			if (lit_tbl[i][CLEN] == 0) {
				// 連続するゼロの符号長の個数を数える.
				for (repeat_cnt = 1;
					repeat_cnt < 138 &&
					i + repeat_cnt < lit_cnt &&
					lit_tbl[i + repeat_cnt][CLEN] == 0;
					repeat_cnt++) {
				}
				// 連続するゼロを符号化する.
				if (repeat_cnt >= 3 && repeat_cnt <= 10) {
					bit_pos = deflate_put_len<TO_BUF, 47>(out_buf, bit_pos, repeat_cnt, clen_tbl);
				}
				else if (repeat_cnt >= 11 && repeat_cnt <= 138) {
					bit_pos = deflate_put_len<TO_BUF, 48>(out_buf, bit_pos, repeat_cnt, clen_tbl);
				}
				else if (repeat_cnt == 2) {
					constexpr int abc = 0;	// アルファベット
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
				}
				else if (repeat_cnt == 1) {
					constexpr int abc = 0;	// アルファベット
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
				}
			}
			else {
				uint32_t changed;	// 直前の符号長と異なるなら 1. 同じなら 0
				if (i == 0 || lit_tbl[i][CLEN] != lit_tbl[i - 1][CLEN]) {
					const uint32_t abc = lit_tbl[i][CLEN];	// アルファベット
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
					changed = 1;
				}
				else {
					changed = 0;
				}
				// 連続する等しい符号長の個数を数える.
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
					const uint32_t abc = lit_tbl[i][CLEN];	// アルファベット
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
				}
				else if (repeat_cnt - changed == 1) {
					const uint32_t abc = lit_tbl[i][CLEN];	// アルファベット
					bit_pos = deflate_put_abc<TO_BUF>(out_buf, bit_pos, abc, clen_tbl);
				}

			}
		}
		return bit_pos;
	}

	// 解凍する.
	// out_data	解凍された出力バイト列
	// in_data	圧縮された入力バイト列
	// in_len	実際に入力したバイト数
	// 戻り値	実際に出力したバイト数
	size_t inflate(uint8_t out_data[], const uint8_t in_data[], size_t& in_len) noexcept
	{
		size_t bit_pos = 0;
		size_t out_pos = 0;
		for (;;) {
			const auto bfinal = bit_get(in_data, bit_pos, 1); bit_pos += 1;
			const auto btype = bit_get(in_data, bit_pos, 2); bit_pos += 2;
			// 00 - 非圧縮
			if (btype == 0) {
				// 続く非圧縮データの長さ (8 ビット) と, 長さの補数 (8 ビット) を読み込む.
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
				// 01 - 固定ハフマン符号で圧縮
				if (btype == 1) {
					memcpy(lit_tbl, FIXED_LIT_TBL, sizeof(FIXED_LIT_TBL));
					lit_cnt = LIT_ABC_MAX + 2;
					dist_tbl = lit_tbl + LIT_ABC_MAX + 2;
					dist_cnt = 32;
				}
				// 10 - カスタムハフマン符号で圧縮
				else if (btype == 2) {
					// HLIT: リテラル/長さ符号の個数 (5 ビット). 実際には 個数 -257 した 0 ～ 29 までの範囲の値が ５ビット長の中に収められる
					// つまり, リテラルと終了符号の 257 個と許される長さ符号の数を意味する.
					const auto hlit = bit_get(in_data, bit_pos, 5); bit_pos += 5;
					if (hlit > 29) {
						return static_cast<size_t>(-1);
					}
					lit_cnt = hlit + LIT_ABC_MIN;
					// HDIST：距離符号の個数 (5 ビット). 実際には 個数 -1 した 0 ～ 31 までの範囲の値
					// つまり, 距離符号は最低 1 つは必要なことを意味する.
					const auto hdist = bit_get(in_data, bit_pos, 5); bit_pos += 5;
					dist_cnt = hdist + DIST_ABC_MIN;
					// HCLEN：長さ符号 (符号表の長さ符号表)の個数 (4 ビット). 実際には個数 -4 した 0 ～ 15 までの範囲
					const auto hclen = bit_get(in_data, bit_pos, 4); bit_pos += 4;
					const auto clen = hclen + CLEN_ABC_MIN;
					uint16_t clen_tbl[CLEN_ABC_MAX][3]{};	// 符号の長さ符号表
					for (uint16_t i = 0; i < CLEN_ABC_MAX; i++) {
						//clen_tbl[i][ABC] = CLEN_ABC_ORDER[i];
						clen_tbl[i][ABC] = i;
						clen_tbl[i][CLEN] = 0;
						clen_tbl[i][CODE] = 0;
					}
					// 符号表の長さ符号表の個数分, 符号の長さ (3 ビット) を読み込んで表に格納する.
					for (size_t i = 0; i < clen; i++) {
						clen_tbl[CLEN_ABC_ORDER[i]][CLEN] = static_cast<uint16_t>(bit_get(in_data, bit_pos, 3)); bit_pos += 3;
					}
					inflate_huffman(std::data(clen_tbl), CLEN_ABC_MAX);
					// 表の並びが符号順でないので, 読み込んだサイズだけでなく, 全行を並び変える.
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


					// 符号表を復号する.
					// ランレングス復号化による分も合わせて個数が HLIT + 257 + HDIST + 1 になるまで, 以下を繰り返す.
					uint32_t lit_tail = 0;	// 表の末尾
					do {
						// ビット列を読み込んで, 符号表の中から一致する行を見つける.
						const size_t j = inflate_match(in_data, bit_pos, clen_tbl, clen_cnt, bit_pos);
						if (j == -1) {
							return static_cast<size_t>(-1);
						}
						// ランレングス符号化されてるか判定する.
						const auto abc = clen_tbl[j][ABC];
						if (abc <= 15) {
							if (lit_tail + 1 > lit_cnt + dist_cnt) {
								return static_cast<size_t>(-1);
							}
							// ランレングス符号化されてないので, 得られたリテラルと符号の長さを符号表に格納する.
							lit_tbl[lit_tail][ABC] = static_cast<uint16_t>(lit_tail < lit_cnt ? lit_tail : (lit_tail - lit_cnt));
							lit_tbl[lit_tail][CLEN] = abc;
							lit_tbl[lit_tail++][CODE] = 0;
						}
						// ランレングス符号化
						else if (abc == 16) {
							if (lit_tail == 0) {
								return static_cast<size_t>(-1);
							}
							// ひとつ前に出現した値が 3 ～ 6 回繰り返される ( 拡張ビット：2 ).
							// 拡張ビットは, 符号じゃないので, ビット列の逆転は必要ない.
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
							// "0" が 3 ～10 回繰り返される ( 拡張ビット：3 ).
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
							// "0" が 11 ～ 138 回繰り返される ( 拡張ビット：7 ).
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
						// 単語長を求める.
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
						// 距離を求める.
						uint32_t dist;
						size_t j = inflate_match(in_data, bit_pos, dist_tbl, dist_cnt, bit_pos);
						const auto dist_abc = dist_tbl[j][ABC];
						if (dist_abc <= 3) {
							// 距離 1-4
							dist = dist_abc + 1;
							//constexpr int k = 0;
							//dist = 1 + (2 << k) + ((dist_lit - (2 + 2 * k)) << k);
						}
						else if (dist_abc >= 4 && dist_abc <= 29) {
							const int blen = (dist_abc - 2) >> 1;	// 拡張ビット長
							dist = 1ul + (2ul << blen) + ((dist_abc - (2 + 2 * blen)) << blen);
							const auto ex_bit = bit_get(in_data, bit_pos, blen); bit_pos += blen;	// 拡張ビット
							dist += ex_bit;
						}
						else {
							return static_cast<size_t>(-1);
						}
						if (dist > out_pos) {
							return static_cast<size_t>(-1);
						}
						// 前方からコピー.
						for (uint32_t k = 0; k < word_len; k++) {
							out_data[out_pos + k] = out_data[out_pos - dist + k];
						}
						out_pos += word_len;
					}
				}
			}
		}
	}

	// 符号長にもとづいてハフマン符号を設定する.
	// T	符号表の列の型. 伸張ならば uint16_t, 圧縮ならば uint32_t (出現回数を格納するため)
	// M	符号表の列数
	// tbl	符号表 (アルファベット順, または符号長/アルファベット順)
	// cnt	符号表の行数
	// 戻り値	符号の長さがゼロでない行数.
	template <typename T, int M>
	static void inflate_huffman(T tbl[][M], int cnt) noexcept
	{
		constexpr int MAX_BITS = 15;
		// 1) 各符号長に対する符号の数を数えます。bl_count[N] に、1 以上の長さ N の符号の数を代入します。
		std::array<uint32_t, MAX_BITS + 1> bl_count{};
		for (size_t i = 0; i < cnt; i++) {
			bl_count[tbl[i][CLEN]]++;
		}
		// 2) 各符号長に対する最も小さい符号の数値を見つけます
		std::array<uint32_t, MAX_BITS + 1> next_code{};
		uint32_t code = 0;
		bl_count[0] = 0;
		for (size_t bits = 1; bits <= MAX_BITS; bits++) {
			code = (code + bl_count[bits - 1]) << 1;
			next_code[bits] = code;
		}
		// 3) step 2 で決定した基本値を使って、同じ長さのすべての符号に対する連続した値を使って、
		// すべての符号に数値を割り付けます。全く使われなかった（ビット長が 0 であった）符号は、
		// 値を割り付けられてはいけません。
		for (size_t n = 0; n < cnt; n++) {
			const auto len = tbl[n][CLEN];
			if (len != 0) {
				tbl[n][CODE] = static_cast<T>(next_code[len]);
				next_code[len]++;
			}
		}
	}


	// ビット列を読み込んで, 符号表から一致する行を見つける.
	static size_t inflate_match(const uint8_t data[], const size_t bit_pos, const uint16_t tbl[][3], const size_t tbl_cnt, size_t& new_pos) noexcept
	{
		auto pos = bit_pos;
		// 符号表の中から, 最小の長さをもつ符号を見つけ, その符号の長さ分のビット列を読み込む.
		// 表は, 長さ昇順で並び換えられているので, 最初の行となる.
		// 読み込んだビット列は, ハフマン符号なので, ビット並びを逆転させる.
		size_t j = 0;
		auto code = bit_rev(bit_get(data, pos, tbl[j][CLEN]), tbl[j][CLEN]); pos += tbl[j][CLEN];
		// ビット列が, 最初の行の符号と一致するか判定する.
		if (code != tbl[j][CODE]) {
			// 不一致なら, 次の行の符号を判定する.
			for (j = 1; j < tbl_cnt && tbl[j][CLEN] > 0; j++) {
				// 前の行と, 符号の長さが一致するか判定する.
				if (tbl[j][CLEN] > tbl[j - 1][CLEN]) {
					// 一致しないなら差分のビット読み込み, ビット列に追加する.
					const auto diff_len = tbl[j][CLEN] - tbl[j - 1][CLEN];
					code = (code << diff_len) | bit_rev(bit_get(data, pos, diff_len), diff_len); pos += diff_len;
				}
				// 一致なら中断する.
				if (code == tbl[j][CODE]) {
					break;
				}
			}
		}
		// 一致する符号がないか判定する.
		if (j >= tbl_cnt || tbl[j][CLEN] == 0) {
			return static_cast<size_t>(-1);
		}
		new_pos = pos;
		return j;
	}

	// バイト列を zlib ストリームに圧縮する.
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

	// zlib ストリームをバイト列に解凍する.
	// out_data	解凍されたバイト列
	// z_data	圧縮されたバイト列
	bool z_decompress(uint8_t out_data[], /*<---*/const uint8_t z_data[]) noexcept
	{
		// zlib ストリームの最初の 2 バイト.
		//               CMF                         FLG
		// +----------------------------+----------------------------+
		// |CMINFO(4)    |CM(4)         |FLEVEL(2)|FDICT(1)|FCHECK(5)|
		// +----------------------------+----------------------------+
		const uint8_t cmf = z_data[0];	// 圧縮方式と, 圧縮方式の情報
		const uint8_t flg = z_data[1];	// CMF と FLG のチェックビット, プリセット辞書, 圧縮レベル
		const uint8_t cm = (cmf & 0x0F);	// 圧縮方式
		if (cm == 8)
		{
			//const uint8_t cinfo = (cmf >> 4);	// 圧縮方式の情報
			//const uint32_t window_size = 1ul << (static_cast<int>(cinfo) + 8);	// CM=8 ならウィンドウサイズの対数-1
			//const uint8_t fcheck = (flg & 0x1f);	// CMF と FLG のチェックビット
			const uint8_t fdict = ((flg >> 5) & 1);	// プリセット辞書
			//const uint8_t flevel = ((flg >> 6) & 3);	// 圧縮レベル (0...3)
			// CMF と FLG を MSB の順序で蓄積された 16 ビットの符号なし整数（CMF*256 + FLG）としてみたときに,
			// この値が 31 の倍数となるように、FCHECK の値が定義されなければいけません
			if (((static_cast<uint32_t>(cmf) << 8) + flg) % 31ul == 0) {
				// PNG はプリセット辞書を使わないので, fdict はゼロ.
				if (fdict == 0) {
					const auto in_data = z_data + 2;
					size_t in_len = 0;
					const auto out_len = inflate(out_data, in_data, in_len);
					if (out_len < static_cast<size_t>(-1)) {
						// 展開したデータの ADLER32 チェックサムと zlib ストリームのチェックサムが一致するか判定する.
						if (ADLER32::update(out_data, out_len) == BYTE_TO_U32(in_data + in_len)) {
							return true;
						}
					}
				}
				else {
					// fdict がゼロでない, 
					// つまり未知の辞書が使われているのだから,
					// エラー.
				}
			}
		}
		return false;
	}
}