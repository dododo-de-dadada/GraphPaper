//------------------------------
// shape.cpp
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
	// �}�`���g�p���� D2D �t�@�N�g���ւ̎Q��.
	ID2D1Factory3* Shape::s_d2d_factory = nullptr;
	IDWriteFactory3* Shape::s_dwrite_factory = nullptr;	// DWRITE �t�@�N�g��
#if defined(_DEBUG)
	uint32_t debug_leak_cnt = 0;
	uint32_t debug_shape_cnt = 0;
	uint32_t debug_deleted_cnt = 0;
#endif

	// �F�̐��������������ׂ�.
	static bool equal_component(const FLOAT a, const FLOAT b) noexcept;

	// UWP �̃u���V�� D2D1_COLOR_F �ɕϊ�����.
	bool cast_to(const Brush& a, D2D1_COLOR_F& b) noexcept
	{
		using winrt::Windows::UI::Xaml::Media::SolidColorBrush;

		const auto s = a.try_as<SolidColorBrush>();
		if (s == nullptr) {
			return false;
		}
		const auto c = s.Color();
		b.r = static_cast<FLOAT>(static_cast<double>(c.R) / COLOR_MAX);
		b.g = static_cast<FLOAT>(static_cast<double>(c.G) / COLOR_MAX);
		b.b = static_cast<FLOAT>(static_cast<double>(c.B) / COLOR_MAX);
		b.a = static_cast<FLOAT>(static_cast<double>(c.A) / COLOR_MAX);
		return true;
	}

	// UWP �̃u���V�� D2D1_COLOR_F �ɕϊ�����.
	void cast_to(const Color& a, D2D1_COLOR_F& b) noexcept
	{
		b.r = static_cast<FLOAT>(static_cast<double>(a.R) / COLOR_MAX);
		b.g = static_cast<FLOAT>(static_cast<double>(a.G) / COLOR_MAX);
		b.b = static_cast<FLOAT>(static_cast<double>(a.B) / COLOR_MAX);
		b.a = static_cast<FLOAT>(static_cast<double>(a.A) / COLOR_MAX);
	}

	// ���ʂ̕��`��\������.
	// a_pos	���ʂ̈ʒu
	// dx		�}�`�̕`���
	void anchor_draw_rect(const D2D1_POINT_2F a_pos, SHAPE_DX& dx)
	{
		D2D1_POINT_2F r_min;
		pt_add(a_pos, -0.5 * dx.m_anch_len, r_min);
		D2D1_POINT_2F r_max;
		pt_add(r_min, dx.m_anch_len, r_max);
		const D2D1_RECT_F r{ r_min.x, r_min.y, r_max.x, r_max.y };

		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawRectangle(r, dx.m_shape_brush.get(), 2.0, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->FillRectangle(r, dx.m_shape_brush.get());
	}

	// �ۂ����ʂ�\������.
	// a_pos	���ʂ̈ʒu
	// dx		�}�`�̕`���
	void anchor_draw_rounded(const D2D1_POINT_2F& a_pos, SHAPE_DX& dx)
	{
		const FLOAT rad = static_cast<FLOAT>(dx.m_anch_len * 0.5 + 1.0);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->FillEllipse({ a_pos, rad, rad }, dx.m_shape_brush.get());
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->FillEllipse({ a_pos, rad - 1.0F, rad - 1.0F }, dx.m_shape_brush.get());
	}

	// ���̐��@�����������ׂ�.
	bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept
	{
		return equal(a.m_width, b.m_width) && equal(a.m_length, b.m_length) && equal(a.m_offset, b.m_offset);
	}

	// ���̌`�������������ׂ�.
	bool equal(const ARROW_STYLE a, const ARROW_STYLE b) noexcept
	{
		return a == b;
	}

	// �u�[���l�����������ׂ�.
	bool equal(const bool a, const bool b) noexcept
	{
		return a == b;
	}

	// �F�����������ׂ�.
	bool equal(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) noexcept
	{
		return equal_component(a.a, b.a)
			&& equal_component(a.r, b.r)
			&& equal_component(a.g, b.g)
			&& equal_component(a.b, b.b);
	}

	// �j���̌`�������������ׂ�.
	bool equal(const D2D1_DASH_STYLE a, const D2D1_DASH_STYLE b) noexcept
	{
		return a == b;
	}

	// �ʒu�����������ׂ�.
	bool equal(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept
	{
		return equal(a.x, b.x) && equal(a.y, b.y);
	}

	// ���@�����������ׂ�.
	bool equal(const D2D1_SIZE_F a, const D2D1_SIZE_F b) noexcept
	{
		return equal(a.width, b.width) && equal(a.height, b.height);
	}

	// �{���x�������������������ׂ�.
	bool equal(const double a, const double b) noexcept
	{
		return fabs(a - b) <= FLT_EPSILON * fmax(1.0, fmax(fabs(a), fabs(b)));
	}

	// ���̂̐L�k�����������ׂ�.
	bool equal(const DWRITE_FONT_STRETCH a, const DWRITE_FONT_STRETCH b) noexcept
	{
		return a == b;
	}

	// ���̂̎��̂����������ׂ�.
	bool equal(const DWRITE_FONT_STYLE a, const DWRITE_FONT_STYLE b) noexcept
	{
		return a == b;
	}

	// ���̂̑��������������ׂ�.
	bool equal(const DWRITE_FONT_WEIGHT a, const DWRITE_FONT_WEIGHT b) noexcept
	{
		return a == b;
	}

	// �i���̐��񂪓��������ׂ�.
	bool equal(const DWRITE_PARAGRAPH_ALIGNMENT a, const DWRITE_PARAGRAPH_ALIGNMENT b) noexcept
	{
		return a == b;
	}

	// ������̐��񂪓��������ׂ�.
	bool equal(const DWRITE_TEXT_ALIGNMENT a, const DWRITE_TEXT_ALIGNMENT b) noexcept
	{
		return a == b;
	}

	// �����͈͂����������ׂ�.
	bool equal(const DWRITE_TEXT_RANGE a, const DWRITE_TEXT_RANGE b) noexcept
	{
		return a.startPosition == b.startPosition && a.length == b.length;
	}

	// �P���x�������������������ׂ�.
	bool equal(const float a, const float b) noexcept
	{
		return fabs(a - b) <= FLT_EPSILON * fmax(1.0f, fmax(fabs(a), fabs(b)));
	}

	// ����̌`�������������ׂ�.
	bool equal(const GRID_PATT a, const GRID_PATT b) noexcept
	{
		return a == b;
	}

	// ����̕\�������������ׂ�.
	bool equal(const GRID_SHOW a, const GRID_SHOW b) noexcept
	{
		return a == b;
	}

	// �j���̔z�u�����������ׂ�.
	bool equal(const STROKE_PATT& a, const STROKE_PATT& b) noexcept
	{
		return equal(a.m_[0], b.m_[0])
			&& equal(a.m_[1], b.m_[1])
			&& equal(a.m_[2], b.m_[2])
			&& equal(a.m_[3], b.m_[3])
			&& equal(a.m_[4], b.m_[4])
			&& equal(a.m_[5], b.m_[5]);
	}

	// ���������������ׂ�.
	bool equal(const uint32_t a, const uint32_t b) noexcept
	{
		return a == b;
	}

	// �����񂪓��������ׂ�.
	bool equal(const wchar_t* a, const wchar_t* b) noexcept
	{
		return a == b || (a != nullptr && b != nullptr && wcscmp(a, b) == 0);
	}

	// �����񂪓��������ׂ�.
	bool equal(winrt::hstring const& a, const wchar_t* b) noexcept
	{
		return a == (b == nullptr ? L"" : b);
	}

	// �F�̐��������������ׂ�.
	static bool equal_component(const FLOAT a, const FLOAT b) noexcept
	{
		return fabs(b - a) < 1.0f / 128.0f;
	}

	// ���̎��Ɛ��@�����ƂɕԂ��̈ʒu���v�Z����.
	// axis	���̎��x�N�g��.
	// axis_len	���x�N�g���̒���
	// barb_width	���̕� (�Ԃ��̗��[�̒���)
	// head_len	���̒��� (��[����Ԃ��܂ł̎��x�N�g����ł̒���)
	// barbs	�v�Z���ꂽ���̕Ԃ��̈ʒu (��[����̃I�t�Z�b�g)
	void get_arrow_barbs(const D2D1_POINT_2F axis, const double axis_len, const double barb_width, const double head_len, D2D1_POINT_2F barbs[]) noexcept
	{
		if (axis_len <= DBL_MIN) {
			constexpr D2D1_POINT_2F Z = { 0.0f, 0.0f };
			barbs[0] = Z;
			barbs[1] = Z;
		}
		else {
			const double hf = barb_width * 0.5;
			const double sx = axis.x * -head_len;
			const double sy = axis.x * hf;
			const double tx = axis.y * -head_len;
			const double ty = axis.y * hf;
			const double ax = 1.0 / axis_len;
			barbs[0].x = static_cast<FLOAT>((sx - ty) * ax);
			barbs[0].y = static_cast<FLOAT>((tx + sy) * ax);
			barbs[1].x = static_cast<FLOAT>((sx + ty) * ax);
			barbs[1].y = static_cast<FLOAT>((tx - sy) * ax);
		}
	}

	// �F���s���������ׂ�.
	// a	���ׂ�F
	// �߂�l	�s�����̏ꍇ true
	bool is_opaque(const D2D1_COLOR_F& a) noexcept
	{
		const auto aa = static_cast<uint32_t>(round(a.a * 255.0f)) & 0xff;
		return aa > 0;
	}

	// �x�N�g���̒��� (�̎���l) �𓾂�
	// a	�x�N�g��
	// �߂�l	���� (�̎���l) 
	double pt_abs2(const D2D1_POINT_2F a) noexcept
	{
		const double ax = a.x;
		const double ay = a.y;
		return ax * ax + ay * ay;
	}

	// �ʒu���ʒu�ɑ���
	void pt_add(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x + b.x;
		c.y = a.y + b.y;
	}

	// �X�J���[�l���ʒu�ɑ���
	void pt_add(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		pt_add(a, b, b, c);
	}

	// 2 �̒l���ʒu�ɑ���
	void pt_add(const D2D1_POINT_2F a, const double x, const double y, D2D1_POINT_2F& b) noexcept
	{
		b.x = static_cast<FLOAT>(a.x + x);
		b.y = static_cast<FLOAT>(a.y + y);
	}

	// ��_�Ԃ̒��_�𓾂�.
	void pt_avg(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		const double ax = a.x;
		const double ay = a.y;
		c.x = static_cast<FLOAT>((ax + b.x) * 0.5);
		c.y = static_cast<FLOAT>((ay + b.y) * 0.5);
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

	// ��_�̓��ς𓾂�.
	double pt_dot(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept
	{
		const double ax = a.x;
		const double ay = a.y;
		return ax * b.x + ay * b.y;
	}

	// ���ʂ����_���܂ނ����ׂ�
	// a_pos	���ʂ̈ʒu
	// a_len	���ʂ̈�ӂ̒���.
	// �߂�l	�܂ޏꍇ true
	// �A���J�[������ 0 ����łȂ���΂Ȃ�Ȃ�.
	bool pt_in_anch(const D2D1_POINT_2F a_pos, const double a_len) noexcept
	{
		D2D1_POINT_2F a_min;	// ���ʂ̈ʒu�𒆓_�Ƃ�����`�̍���_
		pt_add(a_pos, a_len * -0.5, a_min);
		return a_min.x <= 0.0f && 0.0f <= a_min.x + a_len
			&& a_min.y <= 0.0f && 0.0f <= a_min.y + a_len;
	}

	// ���ʂ��ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// a_pos	���ʂ̈ʒu
	// a_len	���ʂ̈�ӂ̒���.
	// �߂�l	�܂ޏꍇ true
	// �A���J�[������ 0 ����łȂ���΂Ȃ�Ȃ�.
	bool pt_in_anch(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos, const double a_len) noexcept
	{
		D2D1_POINT_2F a_tran;
		pt_sub(a_pos, t_pos, a_tran);
		return pt_in_anch(a_tran, a_len);
	}

	// ���~�ɂ��ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// c_pos	���~�̒��S
	// rad	���~�̌a
	// �߂�l	�܂ޏꍇ true
	bool pt_in_elli(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept
	{
		// ���S�_�����_�ɂȂ�悤���ׂ�ʒu���ړ�����.
		const double tx = t_pos.x;
		const double ty = t_pos.y;
		const double cx = c_pos.x;
		const double cy = c_pos.y;
		double px = tx - cx;
		double py = ty - cy;
		const double rx = fabs(rad_x);
		const double ry = fabs(rad_y);
		if (ry <= FLT_MIN) {
			return fabs(py) <= FLT_MIN && fabs(px) <= rx;
		}
		else if (rx <= FLT_MIN) {
			return fabs(py) <= ry && fabs(px) <= FLT_MIN;
		}
		px /= rx;
		py /= ry;
		return px * px + py * py <= 1.0;
	}

	// �������ʒu���܂ނ�, �������l�����Ē��ׂ�.
	// t_pos	���ׂ�ʒu
	// s_pos	�����̎n�[
	// e_pos	�����̏I�[
	// s_width	�����̑���
	// �߂�l	�܂ޏꍇ true
	bool pt_in_line(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width) noexcept
	{
		D2D1_POINT_2F d_pos;	// ���������̃x�N�g��
		pt_sub(e_pos, s_pos, d_pos);
		const double abs = pt_abs2(d_pos);
		if (abs <= FLT_MIN) {
			return equal(t_pos, s_pos);
		}
		// �����̖@���x�N�g�������߂�.
		// �@���x�N�g���̒�����, ���̑����̔����Ƃ���.
		// ������ 0.5 �����̏ꍇ��, 0.5 �Ƃ���.
		pt_scale(d_pos, max(s_width * 0.5, 0.5) / sqrt(abs), d_pos);
		const double nx = d_pos.y;
		const double ny = -d_pos.x;
		// �����̗��[����, �@���x�N�g���̕���, �܂��͂��̋t�̕����ɂ���_�����߂�.
		// ���߂� 4 �_����Ȃ�l�ӌ`���ʒu���܂ނ����ׂ�.
		D2D1_POINT_2F q_pos[4];
		pt_add(s_pos, nx, ny, q_pos[0]);
		pt_add(e_pos, nx, ny, q_pos[1]);
		pt_add(e_pos, -nx, -ny, q_pos[2]);
		pt_add(s_pos, -nx, -ny, q_pos[3]);
		return pt_in_quad(t_pos, q_pos);
	}

	// �l�ւ�`���ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// q_pos	�l�ӌ`�̒��_
	// �߂�l	�܂ޏꍇ true
	// �l�ւ�`�̊e�ӂ�, �w�肳�ꂽ�_���J�n�_�Ƃ��鐅�������������鐔�����߂�.
	bool pt_in_quad(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F q_pos[]) noexcept
	{
		D2D1_POINT_2F p_pos;
		int cnt;
		int i;

		cnt = 0;
		for (p_pos = q_pos[3], i = 0; i < 4; p_pos = q_pos[i++]) {
			// ������̕ӁB�_P��y�������ɂ��āA�n�_�ƏI�_�̊Ԃɂ���B�������A�I�_�͊܂܂Ȃ��B(���[��1)
			if ((p_pos.y <= t_pos.y && q_pos[i].y > t_pos.y)
				// �������̕ӁB�_P��y�������ɂ��āA�n�_�ƏI�_�̊Ԃɂ���B�������A�n�_�͊܂܂Ȃ��B(���[��2)
				|| (p_pos.y > t_pos.y&& q_pos[i].y <= t_pos.y)) {
				// ���[��1,���[��2���m�F���邱�ƂŁA���[��3���m�F�ł��Ă���B
				// �ӂ͓_p�����E���ɂ���B�������A�d�Ȃ�Ȃ��B(���[��4)
				// �ӂ��_p�Ɠ��������ɂȂ�ʒu����肵�A���̎���x�̒l�Ɠ_p��x�̒l���r����B
				if (t_pos.x < p_pos.x + (t_pos.y - p_pos.y) / (q_pos[i].y - p_pos.y) * (q_pos[i].x - p_pos.x)) {
					cnt++;
				}
			}
		}
		return static_cast<bool>(cnt & 1);
	}

	// ���`���ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
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
		return min_x <= t_pos.x && t_pos.x <= max_x
			&& min_y <= t_pos.y && t_pos.y <= max_y;
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

	// ��_�̈ʒu���ׂĂ��ꂼ��傫���l�𓾂�.
	// a	��ׂ����̈ʒu
	// b	��ׂ��������̈ʒu
	// c	����ꂽ�ʒu
	void pt_max(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x > b.x ? a.x : b.x;
		c.y = a.y > b.y ? a.y : b.y;
	}

	// ��_�̈ʒu���ׂĂ��ꂼ�ꏬ�����l�𓾂�.
	// a	��ׂ�������̈ʒu
	// b	��������̈ʒu
	// c	����ꂽ�ʒu
	void pt_min(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x < b.x ? a.x : b.x;
		c.y = a.y < b.y ? a.y : b.y;
	}

	// �ʒu���X�J���[�{�Ɋۂ߂�.
	// a	�ۂ߂���ʒu
	// b	�ۂ߂�X�J���[�l
	// c	����ꂽ�ʒu
	void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(std::round(a.x / b) * b);
		c.y = static_cast<FLOAT>(std::round(a.y / b) * b);
	}

	// �ʒu�ɃX�J���[���|����, �ʒu��������.
	// a	�|������ʒu
	// b	�|����X�J���[�l
	// c	������ʒu
	// d	����ꂽ�ʒu
	void pt_scale(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.x * b + c.x);
		d.y = static_cast<FLOAT>(a.y * b + c.y);
	}

	// �ʒu�ɃX�J���[���|����.
	// a	�|������ʒu
	// b	�|����X�J���[�l
	// c	����ꂽ�ʒu
	void pt_scale(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(a.x * b);
		c.y = static_cast<FLOAT>(a.y * b);
	}

	// ���@�ɃX�J���[�l���|����.
	// a	�|�����鐡�@
	// b	�|����X�J���[�l
	// c	����ꂽ���@
	void pt_scale(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& c) noexcept
	{
		c.width = static_cast<FLOAT>(a.width * b);
		c.height = static_cast<FLOAT>(a.height * b);
	}

	// �_�ɃX�J���[���|����, �ʒu��������.
	// a	�|������ʒu
	// b	�|����X�J���[�l
	// c	������ʒu
	// d	����ꂽ�ʒu
	void pt_scale(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.X * b + c.x);
		d.y = static_cast<FLOAT>(a.Y * b + c.y);
	}

	// �ʒu����ʒu������.
	void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.x;
		c.y = a.y - b.y;
	}

	// �ʒu����傫��������.
	void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.width;
		c.y = a.y - b.height;
	}

	// ���̐��@���f�[�^���[�_�[����ǂݍ���.
	void read(ARROW_SIZE& value, DataReader const& dt_reader)
	{
		value.m_width = dt_reader.ReadSingle();
		value.m_length = dt_reader.ReadSingle();
		value.m_offset = dt_reader.ReadSingle();
	}

	// ���̌`�����f�[�^���[�_�[����ǂݍ���.
	void read(ARROW_STYLE& value, DataReader const& dt_reader)
	{
		value = static_cast<ARROW_STYLE>(dt_reader.ReadUInt32());
	}

	// �u�[���l���f�[�^���[�_�[����ǂݍ���.
	void read(bool& value, DataReader const& dt_reader)
	{
		value = dt_reader.ReadBoolean();
	}

	// �F���f�[�^���[�_�[����ǂݍ���.
	void read(D2D1_COLOR_F& value, DataReader const& dt_reader)
	{
		value.a = dt_reader.ReadSingle();
		value.r = dt_reader.ReadSingle();
		value.g = dt_reader.ReadSingle();
		value.b = dt_reader.ReadSingle();
		value.a = min(max(value.a, 0.0F), 1.0F);
		value.r = min(max(value.r, 0.0F), 1.0F);
		value.g = min(max(value.g, 0.0F), 1.0F);
		value.b = min(max(value.b, 0.0F), 1.0F);
	}

	// ���g�̌`�����f�[�^���[�_�[����ǂݍ���.
	void read(D2D1_DASH_STYLE& value, DataReader const& dt_reader)
	{
		value = static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32());
	}

	// �ʒu���f�[�^���[�_�[����ǂݍ���.
	void read(D2D1_POINT_2F& value, DataReader const& dt_reader)
	{
		value.x = dt_reader.ReadSingle();
		value.y = dt_reader.ReadSingle();
	}

	// ���@���f�[�^���[�_�[����ǂݍ���.
	void read(D2D1_SIZE_F& value, DataReader const& dt_reader)
	{
		value.width = dt_reader.ReadSingle();
		value.height = dt_reader.ReadSingle();
	}

	// �{���x�����������f�[�^���[�_�[����ǂݍ���.
	void read(double& value, DataReader const& dt_reader)
	{
		value = dt_reader.ReadDouble();
	}

	// ���̂̐L�k���f�[�^���[�_�[����ǂݍ���.
	void read(DWRITE_FONT_STRETCH& value, DataReader const& dt_reader)
	{
		value = static_cast<DWRITE_FONT_STRETCH>(dt_reader.ReadUInt32());
	}

	// ���̂̎��̂��f�[�^���[�_�[����ǂݍ���.
	void read(DWRITE_FONT_STYLE& value, DataReader const& dt_reader)
	{
		value = static_cast<DWRITE_FONT_STYLE>(dt_reader.ReadUInt32());
	}

	// ���̂̑������f�[�^���[�_�[����ǂݍ���.
	void read(DWRITE_FONT_WEIGHT& value, DataReader const& dt_reader)
	{
		value = static_cast<DWRITE_FONT_WEIGHT>(dt_reader.ReadUInt32());
	}

	// �i���̐�����f�[�^���[�_�[����ǂݍ���.
	void read(DWRITE_PARAGRAPH_ALIGNMENT& value, DataReader const& dt_reader)
	{
		value = static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(dt_reader.ReadUInt32());
	}

	// ������̐�����f�[�^���[�_�[����ǂݍ���.
	void read(DWRITE_TEXT_ALIGNMENT& value, DataReader const& dt_reader)
	{
		value = static_cast<DWRITE_TEXT_ALIGNMENT>(dt_reader.ReadUInt32());
	}

	// ������͈͂��f�[�^���[�_�[����ǂݍ���.
	void read(DWRITE_TEXT_RANGE& value, DataReader const& dt_reader)
	{
		value.startPosition = dt_reader.ReadUInt32();
		value.length = dt_reader.ReadUInt32();
	}

	// �j���̔z�u���f�[�^���[�_�[����ǂݍ���.
	void read(STROKE_PATT& value, DataReader const& dt_reader)
	{
		value.m_[0] = dt_reader.ReadSingle();
		value.m_[1] = dt_reader.ReadSingle();
		value.m_[2] = dt_reader.ReadSingle();
		value.m_[3] = dt_reader.ReadSingle();
		value.m_[4] = dt_reader.ReadSingle();
		value.m_[5] = dt_reader.ReadSingle();
	}

	// 32 �r�b�g�������f�[�^���[�_�[����ǂݍ���.
	void read(uint32_t& value, DataReader const& dt_reader)
	{
		value = dt_reader.ReadUInt32();
	}

	// ��������f�[�^���[�_�[����ǂݍ���.
	void read(wchar_t*& value, DataReader const& dt_reader)
	{
		uint32_t n;	// ������

		n = dt_reader.ReadUInt32();
		if (n > 0) {
			value = new wchar_t[static_cast<size_t>(n) + 1];
			if (value != nullptr) {
				for (uint32_t i = 0; i < n; i++) {
					value[i] = dt_reader.ReadUInt16();
				}
				value[n] = L'\0';
			}
		}
		else {
			value = nullptr;
		}
	}

	// ����̌`�����f�[�^���[�_�[����ǂݍ���.
	void read(GRID_PATT& value, DataReader const& dt_reader)
	{
		value = static_cast<GRID_PATT>(dt_reader.ReadUInt16());
		if (value == GRID_PATT::PATT_1 || value == GRID_PATT::PATT_2 || value == GRID_PATT::PATT_3) {
			return;
		}
		value = GRID_PATT::PATT_1;
	}

	// ����̕\�����f�[�^���[�_�[����ǂݍ���.
	void read(GRID_SHOW& value, DataReader const& dt_reader)
	{
		value = static_cast<GRID_SHOW>(dt_reader.ReadUInt16());
		if (value == GRID_SHOW::BACK || value == GRID_SHOW::FRONT || value == GRID_SHOW::HIDE) {
			return;
		}
		value = GRID_SHOW::BACK;
	}

	// ������𕡐�����.
	// ���̕����񂪃k���|�C���^�[, �܂��͌��̕������� 0 �̂Ƃ���,
	// �k���|�C���^�[��Ԃ�.
	wchar_t* wchar_cpy(const wchar_t* const s)
	{
		const auto i = wchar_len(s);
		if (i == 0) {
			return nullptr;
		}
		const auto j = static_cast<size_t>(i) + 1;
		auto t = new wchar_t[j];
		wcscpy_s(t, j, s);
		return t;
	}

	// ������̒���.
	// �������k���|�C���^�̏ꍇ, 0 ��Ԃ�.
	uint32_t wchar_len(const wchar_t* const t) noexcept
	{
		return (t == nullptr || t[0] == '\0') ? 0 : static_cast<uint32_t>(wcslen(t));
	}

	// ���̐��@���f�[�^���C�^�[�ɏ�������.
	void write(const ARROW_SIZE& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.m_width);
		dt_writer.WriteSingle(value.m_length);
		dt_writer.WriteSingle(value.m_offset);
	}

	// ���̌`����f�[�^���C�^�[�ɏ�������.
	void write(const ARROW_STYLE value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// �u�[���l���f�[�^���C�^�[�ɏ�������.
	void write(const bool value, DataWriter const& dt_writer)
	{
		dt_writer.WriteBoolean(value);
	}

	// �F���f�[�^���C�^�[�ɏ�������.
	void write(const D2D1_COLOR_F& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.a);
		dt_writer.WriteSingle(value.r);
		dt_writer.WriteSingle(value.g);
		dt_writer.WriteSingle(value.b);
	}

	// �j���̌`����f�[�^���C�^�[�ɏ�������.
	void write(const D2D1_DASH_STYLE value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// �ʒu���f�[�^���C�^�[�ɏ�������.
	void write(const D2D1_POINT_2F value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.x);
		dt_writer.WriteSingle(value.y);
	}

	// ���@���f�[�^���C�^�[�ɏ�������.
	void write(const D2D1_SIZE_F value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.width);
		dt_writer.WriteSingle(value.height);
	}

	// �{���x�����������f�[�^���C�^�[�ɏ�������.
	void write(const double value, DataWriter const& dt_writer)
	{
		dt_writer.WriteDouble(value);
	}

	// ���̂̐L�k���f�[�^���C�^�[�ɏ�������.
	void write(const DWRITE_FONT_STRETCH value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// ���̂̌`����f�[�^���C�^�[�ɏ�������.
	void write(const DWRITE_FONT_STYLE value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// ���̂̑������f�[�^���C�^�[�ɏ�������.
	void write(const DWRITE_FONT_WEIGHT value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// �i���̐�����f�[�^���C�^�[�ɏ�������.
	void write(const DWRITE_PARAGRAPH_ALIGNMENT value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// ������̐�����f�[�^���C�^�[�ɏ�������.
	void write(const DWRITE_TEXT_ALIGNMENT value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// ������͈͂��f�[�^���C�^�[�ɏ�������.
	void write(const DWRITE_TEXT_RANGE value, DataWriter const& dt_writer)
	{
		dt_writer.WriteInt32(value.startPosition);
		dt_writer.WriteInt32(value.length);
	}

	// ����̌`�����f�[�^���C�^�[�ɏ�������.
	void write(const GRID_PATT value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt16(static_cast<uint16_t>(value));
	}

	// ����̕\�����f�[�^���C�^�[�ɏ�������.
	void write(const GRID_SHOW value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt16(static_cast<uint16_t>(value));
	}

	// �j���̔z�u���f�[�^���C�^�[�ɏ�������.
	void write(const STROKE_PATT& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.m_[0]);
		dt_writer.WriteSingle(value.m_[1]);
		dt_writer.WriteSingle(value.m_[2]);
		dt_writer.WriteSingle(value.m_[3]);
		dt_writer.WriteSingle(value.m_[4]);
		dt_writer.WriteSingle(value.m_[5]);
	}

	// 32 �r�b�g�������f�[�^���C�^�[�ɏ�������.
	void write(const uint32_t value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(value));
	}

	// ��������f�[�^���C�^�[�ɏ�������
	void write(const wchar_t* value, DataWriter const& dt_writer)
	{
		const uint32_t len = wchar_len(value);

		if (len > 0) {
			dt_writer.WriteUInt32(len);
			for (uint32_t i = 0; i < len; i++) {
				dt_writer.WriteUInt16(value[i]);
			}
		}
		else {
			dt_writer.WriteUInt32(0);
		}
	}

	// �������ƃV���O���o�C�g��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	// val	�V���O���o�C�g������
	// attr	����
	void write_svg(const char* value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		std::snprintf(buf, sizeof(buf), "%s=\"%s\" ", name, value);
		write_svg(buf, dt_writer);
	}

	// �V���O���o�C�g��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const char* value, DataWriter const& dt_writer)
	{
		for (uint32_t i = 0; value[i] != '\0'; i++) {
			dt_writer.WriteByte(value[i]);
		}
	}

	// �������ƐF���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_COLOR_F value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		const uint32_t vr = static_cast<uint32_t>(std::round(value.r * 255.0)) & 0xff;
		const uint32_t vb = static_cast<uint32_t>(std::round(value.b * 255.0)) & 0xff;
		const uint32_t vg = static_cast<uint32_t>(std::round(value.g * 255.0)) & 0xff;
		sprintf_s(buf, "%s=\"#%02x%02x%02x\" ", name, vr, vg, vb);
		write_svg(buf, dt_writer);
		if (is_opaque(value) == false) {
			std::snprintf(buf, sizeof(buf), "%s-opacity=\"%.3f\" ", name, value.a);
			write_svg(buf, dt_writer);
		}
	}

	// �F���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_COLOR_F value, DataWriter const& dt_writer)
	{
		char buf[8];
		const uint32_t vr = static_cast<uint32_t>(std::round(value.r * 255.0)) & 0xff;
		const uint32_t vb = static_cast<uint32_t>(std::round(value.b * 255.0)) & 0xff;
		const uint32_t vg = static_cast<uint32_t>(std::round(value.g * 255.0)) & 0xff;
		sprintf_s(buf, "#%02x%02x%02x", vr, vg, vb);
		write_svg(buf, dt_writer);
	}

	// �j���̌`���Ɣz�u���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_DASH_STYLE style, const STROKE_PATT& patt, const double width, DataWriter const& dt_writer)
	{
		if (width <= FLT_MIN) {
			return;
		}
		const double a[]{
			patt.m_[0] * width,
			patt.m_[1] * width,
			patt.m_[2] * width,
			patt.m_[3] * width
		};
		char buf[256];
		if (style == D2D1_DASH_STYLE_DASH) {
			sprintf_s(buf, "stroke-dasharray=\"%.0f %.0f\" ", a[0], a[1]);
		}
		else if (style == D2D1_DASH_STYLE_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f\" ", a[2], a[3]);
		}
		else if (style == D2D1_DASH_STYLE_DASH_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f %.0f %.0f\" ", a[0], a[1], a[2], a[3]);
		}
		else if (style == D2D1_DASH_STYLE_DASH_DOT_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f %.0f %.0f %.0f %.0f\" ", a[0], a[1], a[2], a[3], a[2], a[3]);
		}
		else {
			return;
		}
		write_svg(buf, dt_writer);
	}

	// ���߂ƈʒu���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_POINT_2F value, const char* cmd, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s%f %f ", cmd, value.x, value.y);
		write_svg(buf, dt_writer);
	}

	// �������ƈʒu���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_POINT_2F value, const char* name_x, const char* name_y, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%f\" %s=\"%f\" ", name_x, value.x, name_y, value.y);
		write_svg(buf, dt_writer);
	}

	// �������ƕ��������l���f�[�^���C�^�[�� SVG �Ƃ��ď�������
	void write_svg(const double value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%f\" ", name, value);
		write_svg(buf, dt_writer);
	}

	// �����������f�[�^���C�^�[�ɏ�������
	void write_svg(const float value, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%f ", value);
		write_svg(buf, dt_writer);
	}

	// �������� 32 �r�b�g���������f�[�^���C�^�[�� SVG �Ƃ��ď�������
	void write_svg(const uint32_t value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%u\" ", name, value);
		write_svg(buf, dt_writer);
	}

	// �}���`�o�C�g��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	// val	�}���`�o�C�g������
	// v_len	������̕�����
	// dt_writer	�f�[�^���C�^�[
	void write_svg(const wchar_t* value, const uint32_t v_len, DataWriter const& dt_writer)
	{
		if (v_len > 0) {
			const auto s_len = WideCharToMultiByte(CP_UTF8, 0, value, v_len, (char*)NULL, 0, NULL, NULL);
			auto s = new char[static_cast<size_t>(s_len) + 1];
			WideCharToMultiByte(CP_UTF8, 0, value, v_len, static_cast<LPSTR>(s), s_len, NULL, NULL);
			s[s_len] = '\0';
			write_svg(s, dt_writer);
			delete[] s;
		}
	}

}
