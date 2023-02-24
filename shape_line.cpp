//------------------------------
// Shape_line.cpp
// �����Ɩ�邵
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ��邵�� D2D1 �p�X�W�I���g�����쐬����.
	static void line_create_arrow_geom(
		ID2D1Factory3* const d_factory, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_vec,
		ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo);
	// ��邵�� D2D �X�g���[�N�������쐬����.
	static void line_create_arrow_style(
		ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, 
		const D2D1_LINE_JOIN s_join_style, const double s_join_miter_limit,
		ID2D1StrokeStyle** s_arrow_style);
	// �������ʒu���܂ނ�, �������l�����Ĕ��肷��.
	static bool line_hit_test(const D2D1_POINT_2F test, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width, const CAP_STYLE& s_cap) noexcept;

	// ��邵�� D2D1 �p�X�W�I���g�����쐬����
	// d_factory	D2D �t�@�N�g���[
	// start	���̎n�_
	// e_pos	���̏I�[�ւ̈ʒu�x�N�g��
	// style	��邵�̌`��
	// size	��邵�̐��@
	// geo	�쐬���ꂽ�p�X�W�I���g��
	static void line_create_arrow_geom(
		ID2D1Factory3* const d_factory, const D2D1_POINT_2F start, const D2D1_POINT_2F e_pos,
		ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo)
	{
		D2D1_POINT_2F barbs[2];	// ��邵�̕Ԃ��̒[�_
		D2D1_POINT_2F tip;	// ��邵�̐�[�_
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (ShapeLine::line_get_pos_arrow(start, e_pos, a_size, barbs, tip)) {
			// �W�I���g���p�X���쐬����.
			winrt::check_hresult(d_factory->CreatePathGeometry(geo));
			winrt::check_hresult((*geo)->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(
				barbs[0],
				style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
				: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW
			);
			sink->AddLine(tip);
			sink->AddLine(barbs[1]);
			sink->EndFigure(
				style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED
				: D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN
			);
			winrt::check_hresult(sink->Close());
			sink = nullptr;
		}
	}

	// ��邵�� D2D �X�g���[�N�������쐬����.
	static void line_create_arrow_style(ID2D1Factory3* const factory, const CAP_STYLE c_style, const D2D1_LINE_JOIN j_style, const double j_miter_limit, ID2D1StrokeStyle** a_style)
	{
		// ��邵�̔j���̌`���͂��Ȃ炸�\���b�h�Ƃ���.
		const D2D1_STROKE_STYLE_PROPERTIES s_prop{
			c_style.m_start,	// startCap
			c_style.m_end,	// endCap
			D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// dashCap
			j_style,	// lineJoin
			static_cast<FLOAT>(j_miter_limit),	// miterLimit
			D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID,	// dashStyle
			0.0f	// dashOffset
		};
		winrt::check_hresult(
			factory->CreateStrokeStyle(s_prop, nullptr, 0, a_style)
		);
	}

	// ��邵�̐�[�ƕԂ��̈ʒu�����߂�.
	// a_end	��̌�[�̈ʒu
	// a_dir	��̐�[�ւ̃x�N�g��
	// a_size	��邵�̐��@
	// barb	�Ԃ��̈ʒu
	// tip		��[�̈ʒu
	bool ShapeLine::line_get_pos_arrow(
		const D2D1_POINT_2F a_end, const D2D1_POINT_2F a_dir, const ARROW_SIZE& a_size,
		/*--->*/D2D1_POINT_2F barb[2], D2D1_POINT_2F& tip) noexcept
	{
		const auto a_len = std::sqrt(pt_abs2(a_dir));	// ��̒���
		if (a_len >= FLT_MIN) {
			get_pos_barbs(a_dir, a_len, a_size.m_width, a_size.m_length, barb);
			if (a_size.m_offset >= a_len) {
				// ��邵�̐�[
				tip = a_end;
			}
			else {
				pt_mul_add(a_dir, 1.0 - a_size.m_offset / a_len, a_end, tip);
			}
			pt_add(barb[0], tip, barb[0]);
			pt_add(barb[1], tip, barb[1]);
			return true;
		}
		return false;
	}

	// �������ʒu���܂ނ�, �������l�����Ĕ��肷��.
	// test	���肷��ʒu
	// start	�����̎n�_
	// e_pos	�����̏I�_�̈ʒu�x�N�g��
	// s_width	�����̑���
	// s_cap	�����̒[�̌`��
	// �߂�l	�܂ޏꍇ true
	static bool line_hit_test(const D2D1_POINT_2F test, const D2D1_POINT_2F start, const D2D1_POINT_2F e_pos, const double s_width, const CAP_STYLE& s_cap) noexcept
	{
		const double e_width = max(s_width * 0.5, 0.5);
		if (equal(s_cap, CAP_SQUARE)) {
			D2D1_POINT_2F d_vec;	// ���������̃x�N�g��
			pt_sub(e_pos, start, d_vec);
			const double abs2 = pt_abs2(d_vec);
			pt_mul(
				abs2 >= FLT_MIN ? d_vec : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 >= FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				d_vec);
			const double dx = d_vec.x;
			const double dy = d_vec.y;
			const double ox = dy;
			const double oy = -dx;
			D2D1_POINT_2F e_side[4]{};
			pt_add(start, -dx + ox, -dy + oy, e_side[0]);
			pt_add(start, -dx - ox, -dy - oy, e_side[1]);
			pt_add(e_pos, dx - ox, dy - oy, e_side[2]);
			pt_add(e_pos, dx + ox, dy + oy, e_side[3]);
			return pt_in_poly(test, 4, e_side);
		}
		else if (equal(s_cap, CAP_TRIANGLE)) {
			D2D1_POINT_2F d_vec;	// ���������̃x�N�g��
			pt_sub(e_pos, start, d_vec);
			const double abs2 = pt_abs2(d_vec);
			pt_mul(
				abs2 >= FLT_MIN ? d_vec : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 >= FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				d_vec);
			const double dx = d_vec.x;
			const double dy = d_vec.y;
			const double ox = dy;
			const double oy = -dx;
			D2D1_POINT_2F e_side[6]{};
			pt_add(start, ox, oy, e_side[0]);
			pt_add(start, -dx, -dy, e_side[1]);
			pt_add(start, -ox, -oy, e_side[2]);
			pt_add(e_pos, -ox, -oy, e_side[3]);
			pt_add(e_pos, dx, dy, e_side[4]);
			pt_add(e_pos, ox, oy, e_side[5]);
			return pt_in_poly(test, 6, e_side);
		}
		else {
			if (equal(s_cap, CAP_ROUND)) {
				if (pt_in_circle(test, start, e_width) || pt_in_circle(test, e_pos, e_width)) {
					return true;
				}
			}
			D2D1_POINT_2F d_vec;	// �����x�N�g��
			pt_sub(e_pos, start, d_vec);
			const double abs2 = pt_abs2(d_vec);
			if (abs2 >= FLT_MIN) {
				pt_mul(d_vec, e_width / sqrt(abs2), d_vec);
				const double ox = d_vec.y;
				const double oy = -d_vec.x;
				D2D1_POINT_2F e_side[4];
				pt_add(start, ox, oy, e_side[0]);
				pt_add(start, -ox, -oy, e_side[1]);
				pt_add(e_pos, -ox, -oy, e_side[2]);
				pt_add(e_pos, ox, oy, e_side[3]);
				return pt_in_poly(test, 4, e_side);
			}
		}
		return false;
	}

	// �}�`��\������.
	void ShapeLine::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
		ID2D1Factory* factory;
		target->GetFactory(&factory);

		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}

		brush->SetColor(m_stroke_color);
		const auto s_style = m_d2d_stroke_style.get();
		const auto s_width = m_stroke_width;

		D2D1_POINT_2F end;	// �I�_
		pt_add(m_start, m_vec[0], end);
		target->DrawLine(m_start, end, brush, s_width, s_style);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			if (m_d2d_arrow_style == nullptr) {
				line_create_arrow_style(static_cast<ID2D1Factory3*>(factory), m_stroke_cap, m_join_style, m_join_miter_limit, m_d2d_arrow_style.put());
			}
			if (m_d2d_arrow_geom == nullptr) {
				line_create_arrow_geom(static_cast<ID2D1Factory3*>(factory), m_start, m_vec[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
			}
			const auto a_geom = m_d2d_arrow_geom.get();
			if (m_d2d_arrow_geom != nullptr) {
				if (m_arrow_style == ARROW_STYLE::FILLED) {
					target->FillGeometry(a_geom, brush);
				}
				target->DrawGeometry(a_geom, brush, s_width, m_d2d_arrow_style.get());
			}
		}
		if (m_anc_show && is_selected()) {
			D2D1_POINT_2F mid;	// ���_
			pt_mul_add(m_vec[0], 0.5, m_start, mid);
			anc_draw_square(m_start, target, brush);
			anc_draw_square(mid, target, brush);
			anc_draw_square(end, target, brush);
		}
	}

	// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
	// anc	�}�`�̕���
	// val	����ꂽ�ʒu
	void ShapeLine::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		if (anc == ANC_TYPE::ANC_PAGE || anc == ANC_TYPE::ANC_P0) {
			// �}�`�̕��ʂ��u�O���v�܂��́u�J�n�_�v�Ȃ��, �J�n�ʒu�𓾂�.
			val = m_start;
		}
		else if (anc > ANC_TYPE::ANC_P0) {
			const size_t a_cnt = anc - ANC_TYPE::ANC_P0;
			if (a_cnt < m_vec.size() + 1) {
				val = m_start;
				for (size_t i = 0; i < a_cnt; i++) {
					pt_add(val, m_vec[i], val);
				}
			}
		}
	}

	// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
	// val	�̈�̍���ʒu
	void ShapeLine::get_bound_lt(D2D1_POINT_2F& val) const noexcept
	{
		const size_t d_cnt = m_vec.size();	// �����̐�
		D2D1_POINT_2F v_pos = m_start;	// ���_�̈ʒu
		val = m_start;
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(v_pos, m_vec[i], v_pos);
			val.x = val.x < v_pos.x ? val.x : v_pos.x;
			val.y = val.y < v_pos.y ? val.y : v_pos.y;
		}
	}

	// �J�n�ʒu�𓾂�
	// �߂�l	�˂� true
	bool ShapeLine::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		val = m_start;
		return true;
	}

	// ��邵�̐��@�𓾂�.
	bool ShapeLine::get_arrow_size(ARROW_SIZE& val) const noexcept
	{
		val = m_arrow_size;
		return true;
	}

	// ��邵�̌`���𓾂�.
	bool ShapeLine::get_arrow_style(ARROW_STYLE& val) const noexcept
	{
		val = m_arrow_style;
		return true;
	}

	// �}�`���͂ޗ̈�𓾂�.
// a_lt	���̗̈�̍���ʒu.
// a_rb	���̗̈�̉E���ʒu.
// b_lt	�͂ޗ̈�̍���ʒu.
// b_rb	�͂ޗ̈�̉E���ʒu.
	void ShapeLine::get_bound(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept
	{
		b_lt.x = m_start.x < a_lt.x ? m_start.x : a_lt.x;
		b_lt.y = m_start.y < a_lt.y ? m_start.y : a_lt.y;
		b_rb.x = m_start.x > a_rb.x ? m_start.x : a_rb.x;
		b_rb.y = m_start.y > a_rb.y ? m_start.y : a_rb.y;
		const size_t d_cnt = m_vec.size();	// �����̐�
		D2D1_POINT_2F pos = m_start;
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(pos, m_vec[i], pos);
			if (pos.x < b_lt.x) {
				b_lt.x = pos.x;
			}
			if (pos.x > b_rb.x) {
				b_rb.x = pos.x;
			}
			if (pos.y < b_lt.y) {
				b_lt.y = pos.y;
			}
			if (pos.y > b_rb.y) {
				b_rb.y = pos.y;
			}
		}
	}

	// ���_�𓾂�.
	size_t ShapeLine::get_verts(D2D1_POINT_2F v_pos[]) const noexcept
	{
		v_pos[0] = m_start;
		const size_t d_cnt = m_vec.size();
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(v_pos[i], m_vec[i], v_pos[i + 1]);
		}
		return d_cnt + 1;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeLine::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		D2D1_POINT_2F e_pos;
		pt_add(m_start, m_vec[0], e_pos);
		if (pt_in_anc(test, e_pos, m_anc_width)) {
			return ANC_TYPE::ANC_P0 + 1;
		}
		if (pt_in_anc(test, m_start, m_anc_width)) {
			return ANC_TYPE::ANC_P0;
		}
		const float s_width = static_cast<float>(max(static_cast<double>(m_stroke_width), m_anc_width));
		if (line_hit_test(test, m_start, e_pos, s_width, m_stroke_cap)) {
			return ANC_TYPE::ANC_STROKE;
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// area_lt	�͈͂̍���ʒu
	// area_rb	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeLine::in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept
	{
		if (pt_in_rect(m_start, area_lt, area_rb)) {
			D2D1_POINT_2F pos;
			pt_add(m_start, m_vec[0], pos);
			return pt_in_rect(pos, area_lt, area_rb);
		}
		return false;
	}

	// �l���邵�̐��@�Ɋi�[����.
	bool ShapeLine::set_arrow_size(const ARROW_SIZE& val) noexcept
	{
		if (!equal(m_arrow_size, val)) {
			m_arrow_size = val;
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l���邵�̌`���Ɋi�[����.
	bool ShapeLine::set_arrow_style(const ARROW_STYLE val) noexcept
	{
		if (m_arrow_style != val) {
			m_arrow_style = val;
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l������̌����̐�萧���Ɋi�[����.
	bool ShapeLine::set_join_miter_limit(const float& val) noexcept
	{
		if (ShapeStroke::set_join_miter_limit(val)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l������̌����Ɋi�[����.
	bool ShapeLine::set_join_style(const D2D1_LINE_JOIN& val) noexcept
	{
		if (ShapeStroke::set_join_style(val)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����.
	// val	�l
	// anc	�}�`�̕���
	// limit	���E���� (���̒��_�Ƃ̋��������̒l�����ɂȂ�Ȃ�, ���̒��_�Ɉʒu�ɍ��킹��)
	bool ShapeLine::set_pos_anc(
		const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool /*keep_aspect*/) 
		noexcept
	{
		bool flag = false;
		// �ύX���钸�_���ǂ̒��_�����肷��.
		const size_t d_cnt = m_vec.size();	// �����̐�
		if (anc >= ANC_TYPE::ANC_P0 && anc <= ANC_TYPE::ANC_P0 + d_cnt) {
			D2D1_POINT_2F p[N_GON_MAX];	// ���_�̈ʒu
			const size_t a_cnt = anc - ANC_TYPE::ANC_P0;	// �ύX���钸�_
			// �ύX���钸�_�܂ł�, �e���_�̈ʒu�𓾂�.
			p[0] = m_start;
			for (size_t i = 0; i < a_cnt; i++) {
				pt_add(p[i], m_vec[i], p[i + 1]);
			}
			// �l����ύX�O�̈ʒu������, �ύX���鍷���𓾂�.
			D2D1_POINT_2F d;
			pt_sub(val, p[a_cnt], d);
			pt_round(d, PT_ROUND, d);
			// �����̒������[�����傫�������肷��.
			if (pt_abs2(d) >= FLT_MIN) {
				// �ύX���钸�_���ŏ��̒��_�����肷��.
				if (a_cnt == 0) {
					// �ŏ��̒��_�̈ʒu�ɕύX����������.
					pt_add(m_start, d, m_start);
				}
				else {
					// ���_�̒��O�̍����ɕύX����������.
					pt_add(m_vec[a_cnt - 1], d, m_vec[a_cnt - 1]);
				}
				// �ύX����̂��Ō�̒��_�ȊO�����肷��.
				if (a_cnt < d_cnt) {
					// ���̒��_�������Ȃ��悤��,
					// �ύX���钸�_�̎��̒��_�ւ̍�������ύX��������.
					pt_sub(m_vec[a_cnt], d, m_vec[a_cnt]);
				}
				if (!flag) {
					flag = true;
				}
			}
			// ���E�������[���łȂ������肷��.
			if (limit >= FLT_MIN) {
				// �c��̒��_�̈ʒu�𓾂�.
				for (size_t i = a_cnt; i < d_cnt; i++) {
					pt_add(p[i], m_vec[i], p[i + 1]);
				}
				const double dd = static_cast<double>(limit) * static_cast<double>(limit);
				for (size_t i = 0; i < d_cnt + 1; i++) {
					// ���_��, �ύX���钸�_�����肷��.
					if (i == a_cnt) {
						continue;
					}
					// ���_�ƕύX���钸�_�Ƃ̋��������E�����ȏォ���肷��.
					//D2D1_POINT_2F v_vec;
					pt_sub(p[i], p[a_cnt], d);
					if (pt_abs2(d) >= dd) {
						continue;
					}
					// �ύX����̂��ŏ��̒��_�����肷��.
					if (a_cnt == 0) {
						pt_add(m_start, d, m_start);
					}
					else {
						pt_add(m_vec[a_cnt - 1], d, m_vec[a_cnt - 1]);
					}
					// �ύX����̂��Ō�̒��_�ȊO�����肷��.
					if (a_cnt < d_cnt) {
						pt_sub(m_vec[a_cnt], d, m_vec[a_cnt]);
					}
					if (!flag) {
						flag = true;
					}
					break;
				}
			}
		}
		if (flag) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
		}
		return flag;
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	bool ShapeLine::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F new_pos;
		pt_round(val, PT_ROUND, new_pos);
		if (!equal(m_start, new_pos)) {
			m_start = new_pos;
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l��[�̌`���Ɋi�[����.
	bool ShapeLine::set_stroke_cap(const CAP_STYLE& val) noexcept
	{
		if (ShapeStroke::set_stroke_cap(val)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �ʒu���ړ�����.
	// pos	�ʒu�x�N�g��
	bool ShapeLine::move(const D2D1_POINT_2F pos) noexcept
	{
		D2D1_POINT_2F start;
		pt_add(m_start, pos, start);
		if (set_pos_start(start)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// start	�͂ޗ̈�̎n�_
	// b_vec	�͂ޗ̈�̏I�_�ւ̍���
	// page	����̑����l
	ShapeLine::ShapeLine(const D2D1_POINT_2F start, const D2D1_POINT_2F b_vec, const Shape* page) :
		ShapeStroke::ShapeStroke(page)
	{
		m_start = start;
		m_vec.resize(1, b_vec);
		m_vec.shrink_to_fit();
		page->get_arrow_style(m_arrow_style);
		page->get_arrow_size(m_arrow_size);
		m_d2d_arrow_geom = nullptr;
		m_d2d_arrow_style = nullptr;
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	// dt_reader	�ǂݍ��ރf�[�^���[�_�[
	ShapeLine::ShapeLine(const Shape& page, DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(page, dt_reader),
		m_d2d_arrow_style(nullptr),
		m_d2d_arrow_geom(nullptr)
	{
		m_start.x = dt_reader.ReadSingle();
		m_start.y = dt_reader.ReadSingle();
		const size_t vec_cnt = dt_reader.ReadUInt32();	// �v�f��
		m_vec.resize(vec_cnt);
		for (size_t i = 0; i < vec_cnt; i++) {
			m_vec[i].x = dt_reader.ReadSingle();
			m_vec[i].y = dt_reader.ReadSingle();
		}

		m_arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadInt32());
		m_arrow_size.m_width = dt_reader.ReadSingle();
		m_arrow_size.m_length = dt_reader.ReadSingle();
		m_arrow_size.m_offset = dt_reader.ReadSingle();
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapeLine::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);

		// �J�n�ʒu
		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);

		// ���̈ʒu�ւ̍���
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_vec.size()));
		for (const D2D1_POINT_2F vec : m_vec) {
			dt_writer.WriteSingle(vec.x);
			dt_writer.WriteSingle(vec.y);
		}

		dt_writer.WriteInt32(static_cast<int32_t>(m_arrow_style));

		dt_writer.WriteSingle(m_arrow_size.m_width);
		dt_writer.WriteSingle(m_arrow_size.m_length);
		dt_writer.WriteSingle(m_arrow_size.m_offset);
	}

	// �ߖT�̒��_��������.
	// pos	����ʒu
	// dd	�ߖT�Ƃ݂Ȃ����� (�̓��l), �����藣�ꂽ���_�͋ߖT�Ƃ݂͂Ȃ��Ȃ�.
	// val	����ʒu�̋ߖT�ɂ��钸�_
	// �߂�l	���������� true
	bool ShapeLine::get_pos_nearest(const D2D1_POINT_2F p, float& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool done = false;
		D2D1_POINT_2F r;
		pt_sub(m_start, p, r);
		float r_abs = static_cast<float>(pt_abs2(r));
		if (r_abs < dd) {
			dd = r_abs;
			val = m_start;
			if (!done) {
				done = true;
			}
		}
		D2D1_POINT_2F q{ m_start };	// ���̓_
		for (const D2D1_POINT_2F pos : m_vec) {
			pt_add(q, pos, q);
			pt_sub(q, p, r);
			r_abs = static_cast<float>(pt_abs2(r));
			if (r_abs < dd) {
				dd = r_abs;
				val = q;
				if (!done) {
					done = true;
				}
			}
		}
		return done;
	}

}