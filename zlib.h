#pragma once
#include <vector>

namespace winrt::Zlib::implementation
{
	// Adler-32 チェックサム
	// zlib ストリームの非圧縮データを検査する.
	struct ADLER32 {
		static constexpr uint32_t INIT = 1;
		static uint32_t update(const uint32_t adler, const uint8_t b) noexcept
		{
			constexpr uint32_t ADLER32_BASE = 65521;
			const uint32_t s1 = ((adler & 0xffff) + b) % ADLER32_BASE;
			const uint32_t s2 = (((adler >> 16) & 0xffff) + s1) % ADLER32_BASE;
			return (s2 << 16) + s1;
		}
		static uint32_t update(uint32_t adler, const uint8_t buf[], const size_t len) noexcept
		{
			for (size_t i = 0; i < len; i++) {
				adler = update(adler, buf[i]);
			}
			return adler;
		}
	};

	// Convert byte sequence to uint32_t value.
	inline constexpr uint32_t BYTE_TO_U32(const uint8_t ptr[]) noexcept
	{
		return (static_cast<uint32_t>(ptr[0]) << 24) | (static_cast<uint32_t>(ptr[1]) << 16) | (static_cast<uint32_t>(ptr[2]) << 8) | static_cast<uint32_t>(ptr[3]);
	}

	// Deflate & Inflate

	void deflate(std::vector<uint8_t>& out_buf, /*<---*/const uint8_t in_data[], const size_t in_len) noexcept;
	size_t inflate(uint8_t out_data[], /*--->*/const uint8_t in_data[], size_t& in_len) noexcept;

	void z_compress(std::vector<uint8_t>& z_buf, /*<---*/const uint8_t in_data[], const size_t in_len) noexcept;
	bool z_decompress(uint8_t out_data[], /*<---*/const uint8_t z_data[]) noexcept;
}