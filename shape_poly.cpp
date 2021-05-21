//------------------------------
// Shape_poly.cpp
// ���p�`
//------------------------------
#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ���p�`�̕ӂ̖@���x�N�g���𓾂�.
	static bool poly_get_nvec(const size_t m, const D2D1_POINT_2F v_pos[], D2D1_POINT_2F n_vec[]) noexcept;

	// ���s����x�N�g���𓾂�.
	static D2D1_POINT_2F poly_pt_orth(const D2D1_POINT_2F vec) { return { -vec.y, vec.x }; }

	// ���p�`�̕ӂ��ʒu���܂ނ����肷��.
	static bool poly_test_side(const D2D1_POINT_2F t_pos, const size_t v_cnt, const bool v_end, const D2D1_POINT_2F v_pos[], const D2D1_POINT_2F n_vec[], const double s_width, D2D1_POINT_2F exp_side[]) noexcept;

	// ���p�`�̊p���ʒu���܂ނ����肷��.
	static bool poly_test_corner(const D2D1_POINT_2F t_pos, const size_t exp_cnt, const bool exp_end, const D2D1_POINT_2F exp_side[], const D2D1_POINT_2F n_vec[], const double ext_len) noexcept;

	//	���p�`�̊e�ӂ̖@���x�N�g���𓾂�.
	//	v_cnt	���_�̐�
	//	v_pos	���_�̔z�� [v_cnt]
	//	n_vec	�e�ӂ̖@���x�N�g���̔z�� [v_cnt]
	//	�߂�l	�@���x�N�g���𓾂��Ȃ� true, ���ׂĂ̒��_���d�Ȃ��Ă����Ȃ� false
	static bool poly_get_nvec(const size_t v_cnt, const D2D1_POINT_2F v_pos[], D2D1_POINT_2F n_vec[]) noexcept
	{
		// ���p�`�̊e�ӂ̒����Ɩ@���x�N�g��, 
		// �d�����Ȃ����_�̐������߂�.
		std::vector<double> s_len(v_cnt);	// �e�ӂ̒���
		int q_cnt = 1;
		for (size_t i = 0; i < v_cnt; i++) {
			// ���̒��_�Ƃ̍��������߂�.
			D2D1_POINT_2F q_sub;
			pt_sub(v_pos[(i + 1) % v_cnt], v_pos[i], q_sub);
			// �����̒��������߂�.
			s_len[i] = sqrt(pt_abs2(q_sub));
			if (s_len[i] > FLT_MIN) {
				// �����̒����� 0 ���傫���Ȃ�, 
				// �d�����Ȃ����_�̐����C���N�������g����.
				q_cnt++;
			}
			// �����ƒ��s����x�N�g���𐳋K�����Ė@���x�N�g���Ɋi�[����.
			pt_mul(poly_pt_orth(q_sub), 1.0 / s_len[i], n_vec[i]);
		}
		if (q_cnt == 1) {
			// ���ׂĂ̒��_���d�Ȃ����Ȃ�, false ��Ԃ�.
			return false;
		}
		for (size_t i = 0; i < v_cnt; i++) {
			if (s_len[i] <= FLT_MIN) {
				// �ӂ̒����� 0 �Ȃ��, �אڂ���O��̕ӂ̒�����
				// ������ 0 �łȂ��ӂ�T��, �����̖@���x�N�g����������, 
				// ���� 0 �̕ӂ̖@���x�N�g���Ƃ���.
				size_t prev;
				for (size_t j = 1; s_len[prev = ((i - j) % v_cnt)] < FLT_MIN; j++);
				size_t next;
				for (size_t j = 1; s_len[next = ((i + j) % v_cnt)] < FLT_MIN; j++);
				pt_add(n_vec[prev], n_vec[next], n_vec[i]);
				auto len = sqrt(pt_abs2(n_vec[i]));
				if (len > FLT_MIN) {
					pt_mul(n_vec[i], 1.0 / len, n_vec[i]);
				}
				else {
					// �����x�N�g�����[���x�N�g���ɂȂ�Ȃ�,
					// �O���̗אڂ���ӂ̖@���x�N�g���ɒ�������x�N�g����@���x�N�g���Ƃ���.
					n_vec[i] = poly_pt_orth(n_vec[prev]);
				}
			}
		}
		return true;
	}

	// ���p�`�̕ӂ��ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// v_cnt	���_�̐�
	// v_end	�ӂ����Ă��邩����
	// v_pos	���_�̔z�� [v_cnt]
	// n_vec	�e�ӂ̖@���x�N�g���̔z�� [v_cnt]
	// s_width	�ӂ̑���
	// exp_side	�g�������ӂ̔z�� [v_cnt �~ 4]
	static bool poly_test_side(const D2D1_POINT_2F t_pos, const size_t v_cnt, const bool v_end, const D2D1_POINT_2F v_pos[], const D2D1_POINT_2F n_vec[], const double s_width, D2D1_POINT_2F exp_side[]) noexcept
	{
		const auto cnt = (v_end ? v_cnt : v_cnt - 1);
		for (size_t i = 0; i < cnt; i++) {
			// ���Ƃ̕ӂ̕Е��̒[�_��, �@���x�N�g���ɂ����Đ��t�̗������Ɉړ���, ����ꂽ�ʒu���g�������ӂɊi�[����.
			D2D1_POINT_2F nor;
			pt_mul(n_vec[i], s_width, nor);
			const auto j = i * 4;
			pt_add(v_pos[i], nor, exp_side[j + 0]);
			pt_sub(v_pos[i], nor, exp_side[j + 1]);
			// ��������̒[�_��, �����悤�ɂ��Ċg�������ӂ�����������.
			const auto k = (i + 1) % v_cnt;
			pt_sub(v_pos[k], nor, exp_side[j + 2]);
			pt_add(v_pos[k], nor, exp_side[j + 3]);
			// �ʒu���g�������ӂɊ܂܂�邩���肷��.
			if (pt_in_poly(t_pos, 4, exp_side + j)) {
				// �܂܂��Ȃ� true ��Ԃ�.
				return true;
			}
		}
		return false;
	}

	// ���p�`�̊p���ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// exp_cnt	�g�������ӂ̐�
	// exp_end	�g�������ӂ����Ă��邩����
	// exp_side	�g�������ӂ̔z�� [exp_cnt �~ 4]
	// n_vec	�e�ӂ̖@���x�N�g���̔z�� [exp_cnt]
	// ext_len	�p�𒴂��ĉ������钷��
	static bool poly_test_corner(const D2D1_POINT_2F t_pos, const size_t exp_cnt, const bool exp_end, const D2D1_POINT_2F exp_side[], const D2D1_POINT_2F n_vec[], const double ext_len) noexcept
	{
		D2D1_POINT_2F ext_side[4];	// �g�������ӂ�����ɉ���������
		D2D1_POINT_2F ext_vec;	// �g�������ӂɕ��s�ȃx�N�g��

		for (size_t i = (exp_end ? 0 : 1), j = (exp_end ? exp_cnt - 1 : 0); i < exp_cnt; j = i++) {
			// ���钸�_�ɗאڂ���ӂɂ���.
			// �g�������ӂ̈���̒[��, ���������ӂɊi�[����.
			ext_side[0] = exp_side[4 * j + 3];
			ext_side[1] = exp_side[4 * j + 2];
			// �@���x�N�g���ƒ��s����x�N�g����,
			// �������钷���̕������{��,
			// ���s�ȃx�N�g���Ɋi�[����.
			pt_mul(poly_pt_orth(n_vec[j]), ext_len, ext_vec);
			// �i�[�����ʒu�𕽍s�ȃx�N�g���ɉ����ĉ�����,
			// ���������ӂɊi�[����.
			pt_sub(ext_side[1], ext_vec, ext_side[2]);
			pt_sub(ext_side[0], ext_vec, ext_side[3]);
			// �ʒu�����������ӂɊ܂܂�邩���肷��.
			if (pt_in_poly(t_pos, 4, ext_side) != true) {
				// �܂܂�Ȃ��Ȃ�p������.
				continue;
			}
			// �אڂ�������Е��̕ӂɂ���.
			// �g�������ӂ̒[��, ���������ӂ̒[�Ɋi�[����.
			ext_side[2] = exp_side[4 * i + 1];
			ext_side[3] = exp_side[4 * i + 0];
			// �@���x�N�g���ƒ��s����x�N�g�� (��قǂƂ͋t����) �𓾂�,
			// �p���������钷���̕������{��,
			// ���s�ȃx�N�g���Ɋi�[����.
			pt_mul(poly_pt_orth(n_vec[i]), ext_len, ext_vec);
			// �i�[�����ʒu�𕽍s�ȃx�N�g���ɉ����ĉ�����,
			// ���������ӂ̂�������̒[�Ɋi�[����.
			pt_add(ext_side[3], ext_vec, ext_side[0]);
			pt_add(ext_side[2], ext_vec, ext_side[1]);
			// �ʒu�����������ӂɊ܂܂�邩���肷��.
			if (pt_in_poly(t_pos, 4, ext_side)) {
				// �܂܂��Ȃ� true ��Ԃ�.
				return true;
			}
		}
		return false;
	}

	// �̈�����Ƃɑ��p�`���쐬����.
	// b_pos	�̈�̊J�n�ʒu
	// b_diff	�̈�̏I���ʒu�ւ̍���
	// v_cnt	���p�`�̒��_�̐�
	// v_up	���_����ɍ쐬���邩����
	// v_reg	�����p�`���쐬���邩����
	// v_clock	���v����ō쐬���邩����
	// v_pos	���_�̔z�� [v_cnt]
	void ShapePoly::create_poly_by_bbox(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const size_t v_cnt, const bool v_up, const bool v_reg, const bool v_clock, D2D1_POINT_2F v_pos[]) noexcept
	{
		if (v_cnt == 0) {
			return;
		}
		// ���_�𒆐S�Ƃ��鐳�~�����Ƃ�, ���p�`�̒��_��, ���̑傫�������߂�.
		D2D1_POINT_2F v_min{ 0.0, 0.0 };
		D2D1_POINT_2F v_max{ 0.0, 0.0 };
		D2D1_POINT_2F v_diff;
		const double s = v_up ? (M_PI / 2.0) : (M_PI / 2.0 + M_PI / v_cnt);
		for (uint32_t i = 0; i < v_cnt; i++) {
			const double t = 2.0 * M_PI / v_cnt * i;
			const double r = v_clock ? s - t : s + t;
			v_pos[i].x = static_cast<FLOAT>(cos(r));
			v_pos[i].y = static_cast<FLOAT>(-sin(r));
			pt_inc(v_pos[i], v_min, v_max);
		}
		pt_sub(v_max, v_min, v_diff);

		if (pt_abs2(v_diff) > FLT_MIN) {
			const double scale_x = v_reg ? fmin(b_diff.x, b_diff.y) / fmax(v_diff.x, v_diff.y) : b_diff.x / v_diff.x;
			const double scale_y = v_reg ? fmin(b_diff.x, b_diff.y) / fmax(v_diff.x, v_diff.y) : b_diff.y / v_diff.y;
			for (uint32_t i = 0; i < v_cnt; i++) {
				pt_sub(v_pos[i], v_min, v_pos[i]);
				v_pos[i].x = static_cast<FLOAT>(v_pos[i].x * scale_x);
				v_pos[i].y = static_cast<FLOAT>(v_pos[i].y * scale_y);
				pt_add(v_pos[i], b_pos, v_pos[i]);
			}
		}
	}

	// �p�X�W�I���g�����쐬����.
	void ShapePoly::create_path_geometry(ID2D1Factory3* const d_factory)
	{
		const size_t v_cnt = m_diff.size() + 1;	// ���_�̐� (�����̐� + 1)
		std::vector<D2D1_POINT_2F> v_pos(v_cnt);	// ���_�̔z��

		m_path_geom = nullptr;
		v_pos[0] = m_pos;
		for (size_t i = 1; i < v_cnt; i++) {
			pt_add(v_pos[i - 1], m_diff[i - 1], v_pos[i]);
		}
		winrt::com_ptr<ID2D1GeometrySink> sink;
		winrt::check_hresult(d_factory->CreatePathGeometry(m_path_geom.put()));
		m_path_geom->Open(sink.put());
		sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
		const auto figure_begin = is_opaque(m_fill_color) ? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW;
		sink->BeginFigure(v_pos[0], figure_begin);
		for (size_t i = 1; i < v_cnt; i++) {
			sink->AddLine(v_pos[i]);
		}
		// Shape ��Ŏn�_�ƏI�_���d�˂��Ƃ�,
		// �p�X�Ɏn�_�������Ȃ���, LINE_JOINT ���ւ�Ȃ��ƂɂȂ�.
		//sink->AddLine(v_pos[0]);
		sink->EndFigure(m_end_closed ? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
		sink->Close();
		sink = nullptr;
	}

	// �}�`��\������.
	// dx	�}�`�̕`���
	void ShapePoly::draw(SHAPE_DX& dx)
	{
		if (is_opaque(m_fill_color)) {
			dx.m_shape_brush->SetColor(m_fill_color);
			dx.m_d2dContext->FillGeometry(m_path_geom.get(), dx.m_shape_brush.get(), nullptr);
		}
		if (is_opaque(m_stroke_color)) {
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawGeometry(
				m_path_geom.get(),
				dx.m_shape_brush.get(),
				static_cast<FLOAT>(m_stroke_width),
				m_d2d_stroke_style.get());
		}
		if (is_selected() != true) {
			return;
		}
		D2D1_POINT_2F a_pos { m_pos };	// �}�`�̕��ʂ̈ʒu
		anchor_draw_rect(a_pos, dx);
		const size_t d_cnt = m_diff.size();	// �����̐�
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(a_pos, m_diff[i], a_pos);
			anchor_draw_rect(a_pos, dx);
		}
	}

	// �h��Ԃ��F�𓾂�.
	// value	����ꂽ�l
	// �߂�l	����ꂽ�Ȃ� true
	bool ShapePoly::get_fill_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_fill_color;
		return true;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// a_len	���ʂ̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapePoly::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		constexpr D2D1_POINT_2F PZ{ 0.0f, 0.0f };	// ��_
		const size_t v_cnt = m_diff.size() + 1;	// ���_�̐� (�����̐� + 1)
		std::vector<D2D1_POINT_2F> v_pos(v_cnt);	// ���_�̔z��
		size_t j = static_cast<size_t>(-1);	// �_���܂ޒ��_�̓Y����
		// ���肷��ʒu�����_�ƂȂ�悤, ���s�ړ��������p�`�̒��_�𓾂�.
		pt_sub(m_pos, t_pos, v_pos[0]);
		if (pt_in_anch(v_pos[0], a_len)) {
			j = 0;
		}
		for (size_t i = 1; i < v_cnt; i++) {
			pt_add(v_pos[i - 1], m_diff[i - 1], v_pos[i]);
			if (pt_in_anch(v_pos[i], a_len)) {
				j = i;
			}
		}
		if (j != -1) {
			const auto anch = ANCH_TYPE::ANCH_P0 + j;
			return static_cast<uint32_t>(anch);
		}
		if (is_opaque(m_stroke_color)) {
			// ���������傤�ǐ��̑����ɂȂ�悤��,�@�������ӂ��v�Z����.
			const auto e_width = max(max(m_stroke_width, a_len) * 0.5, 0.5);	// �g�����镝

			// �e�ӂ̖@���x�N�g���𓾂�.
			std::vector<D2D1_POINT_2F> n_vec(v_cnt);	// �@���x�N�g��
			poly_get_nvec(v_cnt, v_pos.data(), n_vec.data());

			// ���p�`�̊e�ӂ��ʒu���܂ނ����肷��.
			std::vector<D2D1_POINT_2F> q_exp(v_cnt * 4);	// ��������
			if (poly_test_side(PZ, v_cnt, m_end_closed, v_pos.data(), n_vec.data(), e_width, q_exp.data())) {
				// �܂ނȂ� ANCH_STROKE ��Ԃ�.
				return ANCH_TYPE::ANCH_STROKE;
			}

			// ���p�`�̊p���ʒu���܂ނ����肷��.
			// �p�𒴂��ĉ������钷���͐��̑����� 5 �{.
			// ���������, D2D �̕`��ƈ�v����.
			const auto ext_len = m_stroke_width * 5.0;	// �p�𒴂��ĉ������钷��
			if (poly_test_corner(PZ, v_cnt, m_end_closed, q_exp.data(), n_vec.data(), ext_len)) {
				// �܂ނȂ� ANCH_STROKE ��Ԃ�.
				return ANCH_TYPE::ANCH_STROKE;
			}
		}
		// �ӂ��s����, �܂��͈ʒu���ӂɊ܂܂�Ă��Ȃ��Ȃ�,
		// �h��Ԃ��F���s���������肷��.
		if (is_opaque(m_fill_color)) {
			if (pt_in_poly(PZ, v_cnt, v_pos.data())) {
				// �܂܂��Ȃ� ANCH_FILL ��Ԃ�.
				return ANCH_TYPE::ANCH_FILL;
			}
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapePoly::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		if (!pt_in_rect(m_pos, a_min, a_max)) {
			return false;
		}
		const size_t n = m_diff.size();	// �����̐�
		D2D1_POINT_2F e_pos = m_pos;
		for (size_t i = 0; i < n; i++) {
			pt_add(e_pos, m_diff[i], e_pos);	// ���̈ʒu
			if (!pt_in_rect(e_pos, a_min, a_max)) {
				return false;
			}
		}
		return true;
	}

	// �h��Ԃ��̐F�Ɋi�[����.
	void ShapePoly::set_fill_color(const D2D1_COLOR_F& value) noexcept
	{
		if (equal(m_fill_color, value)) {
			return;
		}
		m_fill_color = value;
		create_path_geometry(s_d2d_factory);
	}

	// �}�`���쐬����.
	// s_pos	�J�n�ʒu
	// diff	�I���ʒu�ւ̍���
	// attr	�����l
	// v_cnt	���_�̐�
	// v_reg	�����p�`�ɍ�}���邩����
	// v_up	���_����ɍ�}���邩����
	// v_end	�ӂ���č�}���邩����
	ShapePoly::ShapePoly(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, const ShapeSheet* attr, const TOOL_POLY& t_poly) :
		ShapePath::ShapePath(t_poly.m_vertex_cnt - 1, attr),
		m_end_closed(t_poly.m_closed),
		m_fill_color(attr->m_fill_color)
	{
		std::vector<D2D1_POINT_2F> v_pos(t_poly.m_vertex_cnt);	// ���_�̔z��
		create_poly_by_bbox(s_pos, diff, t_poly.m_vertex_cnt, t_poly.m_vertex_up, t_poly.m_regular, t_poly.m_clockwise, v_pos.data());

		m_pos = v_pos[0];
		for (size_t i = 1; i < t_poly.m_vertex_cnt; i++) {
			pt_sub(v_pos[i], v_pos[i - 1], m_diff[i - 1]);
		}
		create_path_geometry(s_d2d_factory);
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	// dt_reader	�f�[�^���[�_�[
	ShapePoly::ShapePoly(DataReader const& dt_reader) :
		ShapePath::ShapePath(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;
		read(m_end_closed, dt_reader);
		read(m_fill_color, dt_reader);
		create_path_geometry(s_d2d_factory);
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapePoly::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapePath::write(dt_writer);
		write(m_end_closed, dt_writer);
		write(m_fill_color, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapePoly::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<path d=\"", dt_writer);
		write_svg(m_pos, "M", dt_writer);
		const size_t d_cnt = m_diff.size();	// �����̐�
		for (size_t i = 0; i < d_cnt; i++) {
			write_svg(m_diff[i], "l", dt_writer);
		}
		write_svg("Z\" ", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg(m_fill_color, "fill", dt_writer);
		write_svg("/>" SVG_NEW_LINE, dt_writer);
	}

}