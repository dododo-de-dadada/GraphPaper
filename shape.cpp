//------------------------------
// shape.cpp
// �}�`�̂ЂȌ^, ���̑�
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
#if defined(_DEBUG)
	uint32_t debug_deleted_cnt = 0;
	uint32_t debug_leak_cnt = 0;
	uint32_t debug_shape_cnt = 0;
#endif
	//D2D_UI* Shape::s_dx = nullptr;
	ID2D1Factory3* Shape::s_d2d_factory = nullptr;	// D2D1 �t�@�N�g��
	IDWriteFactory3* Shape::s_dwrite_factory = nullptr;	// DWRITE �t�@�N�g��
	float Shape::s_anch_len = 6.0f;
	D2D1_COLOR_F Shape::m_range_background = ACCENT_COLOR;	// �����͈͂̔w�i�F
	D2D1_COLOR_F Shape::m_range_foreground = COLOR_TEXT_SELECTED;	// �����͈͂̕����F
	D2D1_COLOR_F Shape::m_default_background = COLOR_WHITE;	// �O�i�F (�A���J�[�̔w�i�F)
	D2D1_COLOR_F Shape::m_default_foreground = COLOR_BLACK;	// �w�i�F (�A���J�[�̑O�i�F)
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style = nullptr;	// �⏕���̌`��
	//winrt::com_ptr<ID2D1DrawingStateBlock1> m_state_block = nullptr;	// �`���Ԃ̕ۑ��u���b�N
	//winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_range_brush = nullptr;	// �����͈͂̕����F�u���V
	//winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_solid_color_brush = nullptr;	// �}�`�̐F�u���V

	// ������ '0'...'9' �܂��� 'A'...'F', 'a'...'f' �����肷��.
	static bool is_hex(const wchar_t w, uint32_t& x) noexcept;

	// ��邵�̕Ԃ��̈ʒu�����߂�.
	// a_vec	��x�N�g��.
	// a_len	��x�N�g���̒���
	// h_width	��邵�̕� (�Ԃ��̊Ԃ̒���)
	// h_len	��邵�̒��� (��[����Ԃ��܂ł̎��x�N�g����ł̒���)
	// barbs[2]	�v�Z���ꂽ��邵�̕Ԃ��̈ʒu (��[����̃I�t�Z�b�g)
	void get_arrow_barbs(const D2D1_POINT_2F a_vec, const double a_len, const double h_width, const double h_len, D2D1_POINT_2F barbs[]) noexcept
	{
		if (a_len <= DBL_MIN) {
			constexpr D2D1_POINT_2F Z{ 0.0f, 0.0f };
			barbs[0] = Z;
			barbs[1] = Z;
		}
		else {
			const double hf = h_width * 0.5;	// ��邵�̕��̔����̑傫��
			const double sx = a_vec.x * -h_len;	// ��x�N�g�����邵�̒��������]
			const double sy = a_vec.x * hf;
			const double tx = a_vec.y * -h_len;
			const double ty = a_vec.y * hf;
			const double ax = 1.0 / a_len;
			barbs[0].x = static_cast<FLOAT>((sx - ty) * ax);
			barbs[0].y = static_cast<FLOAT>((tx + sy) * ax);
			barbs[1].x = static_cast<FLOAT>((sx + ty) * ax);
			barbs[1].y = static_cast<FLOAT>((tx - sy) * ax);
		}
	}

	// ��_�ň͂܂ꂽ���`�𓾂�.
	void pt_bound(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& r_min, D2D1_POINT_2F& r_max) noexcept
	{
		if (a.x < b.x) {
			r_min.x = a.x;
			r_max.x = b.x;
		}
		else {
			r_min.x = b.x;
			r_max.x = a.x;
		}
		if (a.y < b.y) {
			r_min.y = a.y;
			r_max.y = b.y;
		}
		else {
			r_min.y = b.y;
			r_max.y = a.y;
		}
	}

	// ���~�ɂ��ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// c_pos	���~�̒��S
	// rad	���~�̌a
	// �߂�l	�܂ޏꍇ true
	bool pt_in_elli(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept
	{
		const double dx = static_cast<double>(t_pos.x) - static_cast<double>(c_pos.x);
		const double dy = static_cast<double>(t_pos.y) - static_cast<double>(c_pos.y);
		const double xx = rad_x * rad_x;
		const double yy = rad_y * rad_y;
		return dx * dx * yy + dy * dy * xx <= xx * yy;
	}

	// ���p�`���ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// v_cnt	���_�̐�
	// v_pos	���_�̔z�� [v_cnt]
	// �߂�l	�܂ޏꍇ true
	// ���p�`�̊e�ӂ�, �w�肳�ꂽ�_���J�n�_�Ƃ��鐅�������������鐔�����߂�.
	bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t v_cnt, const D2D1_POINT_2F v_pos[]) noexcept
	{
		const double tx = t_pos.x;
		const double ty = t_pos.y;
		int i_cnt;	// ��_�̐�
		int i;

		double px = v_pos[v_cnt - 1].x;
		double py = v_pos[v_cnt - 1].y;
		i_cnt = 0;
		for (i = 0; i < v_cnt; i++) {
			double qx = v_pos[i].x;
			double qy = v_pos[i].y;
			// ���[�� 1. ������̕�. �_�� y �������ɂ��āA�n�_�ƏI�_�̊Ԃɂ��� (�������A�I�_�͊܂܂Ȃ�).
			// ���[�� 2. �������̕�. �_�� y �������ɂ��āA�n�_�ƏI�_�̊Ԃɂ��� (�������A�n�_�͊܂܂Ȃ�).
			if ((py <= ty && qy > ty) || (py > ty && qy <= ty)) {
				// ���[�� 3. �_��ʂ鐅�������ӂƏd�Ȃ� (���[�� 1, ���[�� 2 ���m�F���邱�Ƃ�, ���[�� 3 ���m�F�ł��Ă���).
				// ���[�� 4. �ӂ͓_�����E���ɂ���. ������, �d�Ȃ�Ȃ�.
				// �ӂ��_�Ɠ��������ɂȂ�ʒu����肵, ���̎���x�̒l�Ɠ_��x�̒l���r����.
				if (tx < px + (ty - py) / (qy - py) * (qx - px)) {
					i_cnt++;
				}
			}
			px = qx;
			py = qy;
		}
		return static_cast<bool>(i_cnt & 1);
	}

	// ���`���ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// r_min	���`�̂����ꂩ�̒��_
	// r_max	���`�̂�������̒��_
	// �߂�l	�܂ޏꍇ true
	bool pt_in_rect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept
	{
		double min_x;
		double max_x;
		double min_y;
		double max_y;

		if (r_min.x < r_max.x) {
			min_x = r_min.x;
			max_x = r_max.x;
		}
		else {
			min_x = r_max.x;
			max_x = r_min.x;
		}
		if (r_min.y < r_max.y) {
			min_y = r_min.y;
			max_y = r_max.y;
		}
		else {
			min_y = r_max.y;
			max_y = r_min.y;
		}
		return min_x <= t_pos.x && t_pos.x <= max_x && min_y <= t_pos.y && t_pos.y <= max_y;
	}

	// �w�肵���ʒu���܂ނ悤, ���`���g�傷��.
	// a	�܂܂��ʒu
	// r_min	���̕��`�̍���ʒu, ����ꂽ����ʒu
	// r_max	���̕��`�̉E���ʒu, ����ꂽ�E���ʒu
	void pt_inc(const D2D1_POINT_2F a, D2D1_POINT_2F& r_min, D2D1_POINT_2F& r_max) noexcept
	{
		if (a.x < r_min.x) {
			r_min.x = a.x;
		}
		if (a.x > r_max.x) {
			r_max.x = a.x;
		}
		if (a.y < r_min.y) {
			r_min.y = a.y;
		}
		if (a.y > r_max.y) {
			r_max.y = a.y;
		}
	}

	//	������𕡐�����.
	//	���̕����񂪃k���|�C���^�[, �܂��͌��̕������� 0 �̂Ƃ���,
	//	�k���|�C���^�[��Ԃ�.
	wchar_t* wchar_cpy(const wchar_t* const s)
	{
		const auto s_len = wchar_len(s);
		if (s_len == 0) {
			return nullptr;
		}
		auto t = new wchar_t[static_cast<size_t>(s_len) + 1];
		wcscpy_s(t, static_cast<size_t>(s_len) + 1, s);
		return t;
	}

	// ������ 0...9 �܂��� A...F, a...f �����肷��
	static bool is_hex(const wchar_t w, uint32_t& x) noexcept
	{
		if (isdigit(w)) {
			x = w - '0';
		}
		else if (w >= 'a' && w <= 'f') {
			x = w - 'a' + 10;
		}
		else if (w >= 'A' && w <= 'F') {
			x = w - 'A' + 10;
		}
		else {
			return false;
		}
		return true;
	}

	// ������𕡐�����.
	// �G�X�P�[�v������͕����R�[�h�ɕϊ�����.
	wchar_t* wchar_cpy_esc(const wchar_t* const s)
	{
		const auto s_len = wchar_len(s);
		if (s_len == 0) {
			return nullptr;
		}
		auto t = new wchar_t[static_cast<size_t>(s_len) + 1];
		auto st = 0;
		uint32_t j = 0;
		for (uint32_t i = 0; i < s_len && s[i] != '\0' && j < s_len; i++) {
			if (st == 0) {
				if (s[i] == '\\') {
					st = 1;
				}
				else {
					t[j++] = s[i];
				}
			}
			else if (st == 1) {
				// \0-9
				if (s[i] >= '0' && s[i] <= '8') {
					t[j] = s[i] - '0';
					st = 2;
				}
				// \x
				else if (s[i] == 'x') {
					st = 4;
				}
				// \u
				else if (s[i] == 'u') {
					st = 6;
				}
				// \a
				else if (s[i] == 'a') {
					t[j++] = '\a';
					st = 0;
				}
				// \b
				else if (s[i] == 'b') {
					t[j++] = '\b';
					st = 0;
				}
				// \f
				else if (s[i] == 'f') {
					t[j++] = '\f';
					st = 0;
				}
				// \n
				else if (s[i] == 'n') {
					t[j++] = '\n';
					st = 0;
				}
				// \r
				else if (s[i] == 'r') {
					t[j++] = '\r';
					st = 0;
				}
				// \s
				else if (s[i] == 's') {
					t[j++] = ' ';
					st = 0;
				}
				else if (s[i] == 't') {
					t[j++] = '\t';
					st = 0;
				}
				else if (s[i] == 'v') {
					t[j++] = '\v';
					st = 0;
				}
				else {
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 2) {
				if (s[i] >= '0' && s[i] <= '8') {
					t[j] = t[j] * 8 + s[i] - '0';
					st = 3;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 3) {
				if (s[i] >= '0' && s[i] <= '8') {
					t[j++] = t[j] * 8 + s[i] - '0';
					st = 0;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 4) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(x);
					st = 5;
				}
				else if (s[i] == '\\') {
					st = 1;
				}
				else {
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 5) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j++] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 0;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 6) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(x);
					st = 7;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 7) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 8;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 8) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 9;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 9) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j++] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 0;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
		}
		t[j] = '\0';
		return t;
	}

	// ������̒���.
	// �������k���|�C���^�̏ꍇ, 0 ��Ԃ�.
	uint32_t wchar_len(const wchar_t* const t) noexcept
	{
		return (t == nullptr || t[0] == '\0') ? 0 : static_cast<uint32_t>(wcslen(t));
	}

}
