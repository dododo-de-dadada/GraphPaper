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
		ID2D1Factory3* const d_factory, const D2D1_POINT_2F start, const D2D1_POINT_2F pos,
		ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo);
	// ��邵�� D2D �X�g���[�N�������쐬����.
	//static void line_create_arrow_stroke(
	//	ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, 
	//	const D2D1_LINE_JOIN s_join_style, const double s_join_miter_limit,
	//	ID2D1StrokeStyle** s_arrow_style);

	// ��邵�� D2D1 �p�X�W�I���g�����쐬����
	// factory	D2D �t�@�N�g���[
	// start	���̎n�_
	// pos	���̏I�[�ւ̈ʒu�x�N�g��
	// style	��邵�̌`��
	// size	��邵�̐��@
	// geo	�쐬���ꂽ�p�X�W�I���g��
	static void line_create_arrow_geom(
		ID2D1Factory3* const factory, const D2D1_POINT_2F start, const D2D1_POINT_2F pos,
		ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo)
	{
		D2D1_POINT_2F barb[2];	// ��邵�̕Ԃ��̒[�_
		D2D1_POINT_2F tip;	// ��邵�̐�[�_
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (ShapeLine::line_get_pos_arrow(start, pos, a_size, barb, tip)) {
			// �W�I���g���p�X���쐬����.
			winrt::check_hresult(
				factory->CreatePathGeometry(geo));
			winrt::check_hresult(
				(*geo)->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(
				barb[0],
				style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
				: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW
			);
			sink->AddLine(tip);
			sink->AddLine(barb[1]);
			sink->EndFigure(
				style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED
				: D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN
			);
			winrt::check_hresult(
				sink->Close());
			sink = nullptr;
		}
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
		pt_add(m_start, m_pos[0], end);
		target->DrawLine(m_start, end, brush, s_width, s_style);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			if (m_d2d_arrow_stroke == nullptr) {
				create_arrow_stroke();
			}
			if (m_d2d_arrow_geom == nullptr) {
				line_create_arrow_geom(
					static_cast<ID2D1Factory3*>(factory), m_start, m_pos[0], m_arrow_style,
					m_arrow_size, m_d2d_arrow_geom.put());
			}
			const auto a_geom = m_d2d_arrow_geom.get();
			if (m_d2d_arrow_geom != nullptr) {
				if (m_arrow_style == ARROW_STYLE::FILLED) {
					target->FillGeometry(a_geom, brush);
				}
				target->DrawGeometry(a_geom, brush, s_width, m_d2d_arrow_stroke.get());
			}
		}
		if (m_anc_show && is_selected()) {
			// �⏕����`��
			if (m_stroke_width >= Shape::m_anc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawLine(m_start, end, brush, 2.0f * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawLine(m_start, end, brush, m_aux_width, m_aux_style.get());
			}
			// �}�`�̕��ʂ�`��.
			D2D1_POINT_2F mid;	// ���_
			pt_mul_add(m_pos[0], 0.5, m_start, mid);
			anc_draw_rhombus(mid, target, brush);
			anc_draw_square(m_start, target, brush);
			anc_draw_square(end, target, brush);
		}
	}

	// �w�肳�ꂽ���ʂ̓_�𓾂�.
	// anc	�}�`�̕���
	// val	����ꂽ�_
	void ShapeLine::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		// �}�`�̕��ʂ��u�}�`�̊O���v�܂��́u�J�n�_�v�Ȃ��, �n�_�𓾂�.
		if (anc == ANC_TYPE::ANC_P0) {
			val = m_start;
		}
		else if (anc == ANC_TYPE::ANC_P0 + 1) {
			val.x = m_start.x + m_pos[0].x;
			val.y = m_start.y + m_pos[0].y;
		}
	}

	// �}�`���͂ދ�`�̍���_�𓾂�.
	// val	����_
	void ShapeLine::get_bound_lt(D2D1_POINT_2F& val) const noexcept
	{
		val.x = m_pos[0].x < 0.0 ? m_start.x + m_pos[0].x : m_start.x;
		val.y = m_pos[0].y < 0.0 ? m_start.y + m_pos[0].y : m_start.y;
	}

	// �n�_�𓾂�
	// val	�n�_
	// �߂�l	�˂� true
	bool ShapeLine::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		val = m_start;
		return true;
	}

	// �}�`���͂ދ�`�𓾂�.
	// a_lt	���̋�`�̍���_.
	// a_rb	���̋�`�̉E���_.
	// b_lt	��`�̍���_.
	// b_rb	��`�̉E���_.
	void ShapeLine::get_bound(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept
	{
		b_lt.x = m_start.x < a_lt.x ? m_start.x : a_lt.x;
		b_lt.y = m_start.y < a_lt.y ? m_start.y : a_lt.y;
		b_rb.x = m_start.x > a_rb.x ? m_start.x : a_rb.x;
		b_rb.y = m_start.y > a_rb.y ? m_start.y : a_rb.y;
		if (m_start.x + m_pos[0].x < b_lt.x) {
			b_lt.x = m_start.x + m_pos[0].x;
		}
		if (m_start.x + m_pos[0].x > b_rb.x) {
			b_rb.x = m_start.x + m_pos[0].x;
		}
		if (m_start.y + m_pos[0].y < b_lt.y) {
			b_lt.y = m_start.y + m_pos[0].y;
		}
		if (m_start.y + m_pos[0].y > b_rb.y) {
			b_rb.y = m_start.y + m_pos[0].y;
		}
	}

	// ���_�𓾂�.
	size_t ShapeLine::get_verts(D2D1_POINT_2F p[]) const noexcept
	{
		p[0] = m_start;
		p[1].x = m_start.x + m_pos[0].x;
		p[1].y = m_start.y + m_pos[0].y;
		p[2].x = static_cast<FLOAT>(m_start.x + 0.5 * m_pos[0].x);
		p[2].y = static_cast<FLOAT>(m_start.y + 0.5 * m_pos[0].y);
		return 3;
	}

	// �}�`���_���܂ނ����肷��.
	// t	���肳���_
	// �߂�l	�}�`�̕���
	uint32_t ShapeLine::hit_test(const D2D1_POINT_2F t) const noexcept
	{
		D2D1_POINT_2F end;	// �I�_
		pt_add(m_start, m_pos[0], end);
		if (anc_hit_test(t, end, m_anc_width)) {
			return ANC_TYPE::ANC_P0 + 1;
		}
		if (anc_hit_test(t, m_start, m_anc_width)) {
			return ANC_TYPE::ANC_P0;
		}
		const double e_width = 0.5 * max(m_stroke_width, m_anc_width);
		if (equal(m_stroke_cap, CAP_STYLE_SQUARE)) {
			D2D1_POINT_2F pos{ m_pos[0] };
			const double abs2 = pt_abs2(pos);
			pt_mul(
				abs2 >= FLT_MIN ? pos : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 >= FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				pos);
			const double dx = pos.x;	// �ӂ̕����x�N�g�� X ��
			const double dy = pos.y;	// �ӂ̕����x�N�g�� Y ��
			const double ox = dy;	// �ӂ̒����x�N�g�� X ��
			const double oy = -dx;	// �ӂ̒����x�N�g�� Y ��
			D2D1_POINT_2F q[4]{};	// ���点���ӂ̎l�ӌ`
			pt_add(m_start, -dx + ox, -dy + oy, q[0]);
			pt_add(m_start, -dx - ox, -dy - oy, q[1]);
			pt_add(end, dx - ox, dy - oy, q[2]);
			pt_add(end, dx + ox, dy + oy, q[3]);
			if (pt_in_poly(t, 4, q)) {
				return ANC_TYPE::ANC_STROKE;
			}
		}
		else if (equal(m_stroke_cap, CAP_STYLE_TRIANGLE)) {
			D2D1_POINT_2F p{ m_pos[0] };
			const double abs2 = pt_abs2(p);
			pt_mul(
				abs2 >= FLT_MIN ? p : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 >= FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				p);
			const double dx = p.x;	// �ӂ̕����x�N�g�� X ���W
			const double dy = p.y;	// �ӂ̕����x�N�g�� Y ���W
			const double ox = dy;	// �ӂ̒����x�N�g�� X ���W
			const double oy = -dx;	// �ӂ̒����x�N�g�� Y ���W
			D2D1_POINT_2F h[6]{};	// ���点���ӂ̘Z�p�`
			pt_add(m_start, ox, oy, h[0]);
			pt_add(m_start, -dx, -dy, h[1]);
			pt_add(m_start, -ox, -oy, h[2]);
			pt_add(end, -ox, -oy, h[3]);
			pt_add(end, dx, dy, h[4]);
			pt_add(end, ox, oy, h[5]);
			if (pt_in_poly(t, 6, h)) {
				return ANC_TYPE::ANC_STROKE;
			}
		}
		else {
			if (equal(m_stroke_cap, CAP_STYLE_ROUND)) {
				if (pt_in_circle(t, m_start, e_width) || pt_in_circle(t, end, e_width)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
			D2D1_POINT_2F p{ m_pos[0] };
			const double abs2 = pt_abs2(p);
			if (abs2 >= FLT_MIN) {
				pt_mul(p, e_width / sqrt(abs2), p);
				const double ox = p.y;	// �ӂ̒����x�N�g�� X ���W
				const double oy = -p.x;	// �ӂ̒����x�N�g�� Y ���W
				D2D1_POINT_2F q[4];	// ���点���ӂ̎l�ӌ`
				pt_add(m_start, ox, oy, q[0]);
				pt_add(m_start, -ox, -oy, q[1]);
				pt_add(end, -ox, -oy, q[2]);
				pt_add(end, ox, oy, q[3]);
				if (pt_in_poly(t, 4, q)) {
					return ANC_TYPE::ANC_STROKE;
				}
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// ��`�Ɋ܂܂�邩���肷��.
	// lt	��`�̍���ʒu
	// rb	��`�̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeLine::is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		if (pt_in_rect(m_start, lt, rb)) {
			D2D1_POINT_2F p;
			pt_add(m_start, m_pos[0], p);
			return pt_in_rect(p, lt, rb);
		}
		return false;
	}

	// �l������̌����̐�萧���Ɋi�[����.
	bool ShapeLine::set_join_miter_limit(const float& val) noexcept
	{
		if (ShapeStroke::set_join_miter_limit(val)) {
			if (m_d2d_arrow_stroke != nullptr) {
				m_d2d_arrow_stroke = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l������̌����Ɋi�[����.
	bool ShapeLine::set_join_style(const D2D1_LINE_JOIN& val) noexcept
	{
		if (ShapeStroke::set_join_style(val)) {
			m_d2d_arrow_stroke = nullptr;
			return true;
		}
		return false;
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����.
	// val	�l
	// anc	�}�`�̕���
	// snap_point	���̓_�Ƃ̊Ԋu (���̒l��藣�ꂽ�_�͖�������)
	bool ShapeLine::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float snap_point, const bool /*keep_aspect*/) noexcept
	{
		bool flag = false;
		if (anc == ANC_TYPE::ANC_P0) {
			if (!equal(m_start, val)) {
				const D2D1_POINT_2F end{
					m_start.x + m_pos[0].x, m_start.y + m_pos[0].y
				};
				m_pos[0].x = end.x - val.x;
				m_pos[0].y = end.y - val.y;
				m_start = val;
				flag = true;
			}
		}
		else if (anc == ANC_TYPE::ANC_P0 + 1) {
			const D2D1_POINT_2F end{
				m_start.x + m_pos[0].x, m_start.y + m_pos[0].y
			};
			if (!equal(end, val)) {
				m_pos[0].x = val.x - m_start.x;
				m_pos[0].y = val.y - m_start.y;
				flag = true;
			}
		}
		if (flag) {
			if (snap_point > FLT_MIN && pt_abs2(m_pos[0]) <= snap_point * snap_point) {
				if (anc == ANC_TYPE::ANC_P0) {
					m_start.x = m_start.x + m_pos[0].x;
					m_start.y = m_start.y + m_pos[0].y;
				}
				m_pos[0].x = 0.0f;
				m_pos[0].y = 0.0f;
			}
			m_d2d_arrow_geom = nullptr;
		}
		return flag;
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	bool ShapeLine::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F p;
		pt_round(val, PT_ROUND, p);
		if (!equal(m_start, p)) {
			m_start = p;
			m_d2d_arrow_geom = nullptr;
			return true;
		}
		return false;
	}

	// �l��[�̌`���Ɋi�[����.
	bool ShapeLine::set_stroke_cap(const CAP_STYLE& val) noexcept
	{
		if (ShapeStroke::set_stroke_cap(val)) {
			m_d2d_arrow_stroke = nullptr;
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
			m_d2d_arrow_geom = nullptr;
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// start	�n�_
	// pos	�I�_�ւ̈ʒu�x�N�g��
	// page	����̑����l
	ShapeLine::ShapeLine(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page) :
		ShapeArrow::ShapeArrow(page)
	{
		m_start = start;
		m_pos.resize(1, pos);
		m_pos.shrink_to_fit();
		page->get_arrow_style(m_arrow_style);
		page->get_arrow_size(m_arrow_size);
		m_d2d_arrow_geom = nullptr;
		m_d2d_arrow_stroke = nullptr;
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	// dt_reader	�ǂݍ��ރf�[�^���[�_�[
	ShapeLine::ShapeLine(DataReader const& dt_reader) :
		ShapeArrow::ShapeArrow(dt_reader)
	{
		m_start.x = dt_reader.ReadSingle();
		m_start.y = dt_reader.ReadSingle();
		const size_t vec_cnt = dt_reader.ReadUInt32();	// �v�f��
		m_pos.resize(vec_cnt);
		for (size_t i = 0; i < vec_cnt; i++) {
			m_pos[i].x = dt_reader.ReadSingle();
			m_pos[i].y = dt_reader.ReadSingle();
		}
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapeLine::write(DataWriter const& dt_writer) const
	{
		ShapeArrow::write(dt_writer);

		// �n�_
		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);

		// ���̓_�ւ̈ʒu�x�N�g��
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_pos.size()));
		for (const D2D1_POINT_2F vec : m_pos) {
			dt_writer.WriteSingle(vec.x);
			dt_writer.WriteSingle(vec.y);
		}

		//dt_writer.WriteInt32(static_cast<int32_t>(m_arrow_style));

		//dt_writer.WriteSingle(m_arrow_size.m_width);
		//dt_writer.WriteSingle(m_arrow_size.m_length);
		//dt_writer.WriteSingle(m_arrow_size.m_offset);
	}

	// �ߖT�̒��_��������.
	// pos	����ʒu
	// dd	�ߖT�Ƃ݂Ȃ����� (�̓��l), �����藣�ꂽ���_�͋ߖT�Ƃ݂͂Ȃ��Ȃ�.
	// val	����ʒu�̋ߖT�ɂ��钸�_
	// �߂�l	���������� true
	bool ShapeLine::get_pos_nearest(const D2D1_POINT_2F p, float& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool done = false;
		D2D1_POINT_2F d;
		pt_sub(m_start, p, d);
		float d_abs = static_cast<float>(pt_abs2(d));
		if (d_abs < dd) {
			dd = d_abs;
			val = m_start;
			done = true;
		}
		D2D1_POINT_2F q{ m_start.x + m_pos[0].x, m_start.y + m_pos[0].y };
		pt_sub(q, p, d);
		d_abs = static_cast<float>(pt_abs2(d));
		if (d_abs < dd) {
			dd = d_abs;
			val = q;
			done = true;
		}
		D2D1_POINT_2F r{ 
			static_cast<FLOAT>(m_start.x + 0.5 * m_pos[0].x),
			static_cast<FLOAT>(m_start.y + 0.5 * m_pos[0].y)
		};
		pt_sub(r, p, d);
		d_abs = static_cast<float>(pt_abs2(d));
		if (d_abs < dd) {
			dd = d_abs;
			val = r;
			done = true;
		}
		return done;
	}

}