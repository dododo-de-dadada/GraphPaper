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
	float Shape::s_anp_len = 6.0f;	// �A���J�[�|�C���g�̑傫��
	D2D1_COLOR_F Shape::s_background_color = COLOR_WHITE;	// �O�i�F (�A���J�[�̔w�i�F)
	D2D1_COLOR_F Shape::s_foreground_color = COLOR_BLACK;	// �w�i�F (�A���J�[�̑O�i�F)
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style = nullptr;	// �⏕���̌`��

	// �}�`�̕��ʁi�~�`�j��\������.
	// a_pos	���ʂ̈ʒu
	// dx		�}�`�̕`���
	void anp_draw_ellipse(const D2D1_POINT_2F a_pos, D2D_UI& dx)
	{
		const FLOAT rad = static_cast<FLOAT>(Shape::s_anp_len * 0.5 + 1.0);
		ID2D1SolidColorBrush* const brush = dx.m_solid_color_brush.get();
		brush->SetColor(Shape::s_background_color);
		dx.m_d2d_context->FillEllipse(D2D1_ELLIPSE{ a_pos, rad, rad }, brush);
		brush->SetColor(Shape::s_foreground_color);
		dx.m_d2d_context->FillEllipse(D2D1_ELLIPSE{ a_pos, rad - 1.0f, rad - 1.0f }, brush);
	}

	// �}�`�̕��� (���`) ��\������.
	// a_pos	���ʂ̈ʒu
	// dx		�}�`�̕`���
	void anp_draw_rect(const D2D1_POINT_2F a_pos, D2D_UI& dx)
	{
		D2D1_POINT_2F r_min;
		D2D1_POINT_2F r_max;
		pt_add(a_pos, -0.5 * Shape::s_anp_len, r_min);
		pt_add(r_min, Shape::s_anp_len, r_max);
		const D2D1_RECT_F rect{ r_min.x, r_min.y, r_max.x, r_max.y };
		ID2D1SolidColorBrush* const brush = dx.m_solid_color_brush.get();
		brush->SetColor(Shape::s_background_color);
		dx.m_d2d_context->DrawRectangle(rect, brush, 2.0, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		dx.m_d2d_context->FillRectangle(rect, brush);
	}

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
	/*
	void pt_bound(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c_min, D2D1_POINT_2F& c_max) noexcept
	{
		if (a.x < b.x) {
			c_min.x = a.x;
			c_max.x = b.x;
		}
		else {
			c_min.x = b.x;
			c_max.x = a.x;
		}
		if (a.y < b.y) {
			c_min.y = a.y;
			c_max.y = b.y;
		}
		else {
			c_min.y = b.y;
			c_max.y = a.y;
		}
	}
	*/
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
	/*
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
	*/
}
