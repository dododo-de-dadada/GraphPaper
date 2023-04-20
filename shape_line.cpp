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
	static void line_create_arrow_geom(ID2D1Factory3* const d_factory, const D2D1_POINT_2F start, const D2D1_POINT_2F pos, ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo) noexcept;
	// ��邵�� D2D �X�g���[�N�������쐬����.
	//static void line_create_arrow_stroke(
	//	ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, 
	//	const D2D1_LINE_JOIN s_join_style, const double s_join_miter_limit,
	//	ID2D1StrokeStyle** s_arrow_style);

	// ��邵�� D2D1 �p�X�W�I���g�����쐬����
	static void line_create_arrow_geom(
		ID2D1Factory3* const factory,	// D2D �t�@�N�g���[
		const D2D1_POINT_2F start,	// ��̎n�_
		const D2D1_POINT_2F pos,	// ��̐�[�ւ̈ʒu�x�N�g��
		ARROW_STYLE style,	// ��邵�̌`��
		ARROW_SIZE& a_size,	// ��邵�̑傫��
		ID2D1PathGeometry** geo	// �쐬���ꂽ�p�X�W�I���g��
	) noexcept
	{
		D2D1_POINT_2F barb[2];	// ��邵�̕Ԃ��̒[�_
		D2D1_POINT_2F tip;	// ��邵�̐�[�_
		winrt::com_ptr<ID2D1GeometrySink> sink;
		HRESULT hr = S_OK;
		if (!ShapeLine::line_get_pos_arrow(start, pos, a_size, barb, tip)) {
			hr = E_FAIL;
		}
		if (hr == S_OK) {
			// �W�I���g���p�X���쐬����.
			hr = factory->CreatePathGeometry(geo);
		}
		if (hr == S_OK) {
			hr = (*geo)->Open(sink.put());
		}
		if (hr == S_OK) {
			const auto f_begin = (
				style == ARROW_STYLE::ARROW_FILLED
				? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
				: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW
				);
			const auto f_end = (
				style == ARROW_STYLE::ARROW_FILLED
				? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED
				: D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN
				);
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(barb[0], f_begin);
			sink->AddLine(tip);
			sink->AddLine(barb[1]);
			sink->EndFigure(f_end);
			hr = sink->Close();
		}
		sink = nullptr;
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
	void ShapeLine::draw(void) noexcept
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
		pt_add(m_start, m_pos, end);
		target->DrawLine(m_start, end, brush, s_width, s_style);
		if (m_arrow_style != ARROW_STYLE::ARROW_NONE) {
			if (m_d2d_arrow_stroke == nullptr) {
				create_arrow_stroke();
			}
			if (m_d2d_arrow_geom == nullptr) {
				line_create_arrow_geom(static_cast<ID2D1Factory3*>(factory), m_start, m_pos, m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
			}
			const auto a_geom = m_d2d_arrow_geom.get();
			if (m_d2d_arrow_geom != nullptr) {
				if (m_arrow_style == ARROW_STYLE::ARROW_FILLED) {
					target->FillGeometry(a_geom, brush);
				}
				target->DrawGeometry(a_geom, brush, s_width, m_d2d_arrow_stroke.get());
			}
		}
		if (m_loc_show && is_selected()) {
			// �⏕����`��
			if (m_stroke_width >= Shape::m_loc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawLine(m_start, end, brush, 2.0f * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawLine(m_start, end, brush, m_aux_width, m_aux_style.get());
			}
			// �}�`�̕��ʂ�`��.
			D2D1_POINT_2F mid;	// ���_
			pt_mul_add(m_pos, 0.5, m_start, mid);
			loc_draw_rhombus(mid, target, brush);
			loc_draw_square(m_start, target, brush);
			loc_draw_square(end, target, brush);
		}
	}

	// �w�肵�����ʂ̓_�𓾂�.
	void ShapeLine::get_pos_loc(
		const uint32_t loc,	// ����
		D2D1_POINT_2F& val	// ����ꂽ�_
	) const noexcept
	{
		// �}�`�̕��ʂ��u�}�`�̊O���v�܂��́u�J�n�_�v�Ȃ��, �n�_�𓾂�.
		if (loc == LOC_TYPE::LOC_START) {
			val = m_start;
		}
		else if (loc == LOC_TYPE::LOC_END) {
			val.x = m_start.x + m_pos.x;
			val.y = m_start.y + m_pos.y;
		}
	}

	// ���E��`�̍���_�𓾂�.
	// val	����_
	void ShapeLine::get_bbox_lt(D2D1_POINT_2F& val) const noexcept
	{
		val.x = m_pos.x < 0.0 ? m_start.x + m_pos.x : m_start.x;
		val.y = m_pos.y < 0.0 ? m_start.y + m_pos.y : m_start.y;
	}

	// �n�_�𓾂�
	// val	�n�_
	// �߂�l	�˂� true
	bool ShapeLine::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		val = m_start;
		return true;
	}

	// ���E��`�𓾂�.
	// a_lt	���̋�`�̍���_.
	// a_rb	���̋�`�̉E���_.
	// b_lt	��`�̍���_.
	// b_rb	��`�̉E���_.
	void ShapeLine::get_bbox(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept
	{
		b_lt.x = m_start.x < a_lt.x ? m_start.x : a_lt.x;
		b_lt.y = m_start.y < a_lt.y ? m_start.y : a_lt.y;
		b_rb.x = m_start.x > a_rb.x ? m_start.x : a_rb.x;
		b_rb.y = m_start.y > a_rb.y ? m_start.y : a_rb.y;
		if (m_start.x + m_pos.x < b_lt.x) {
			b_lt.x = m_start.x + m_pos.x;
		}
		if (m_start.x + m_pos.x > b_rb.x) {
			b_rb.x = m_start.x + m_pos.x;
		}
		if (m_start.y + m_pos.y < b_lt.y) {
			b_lt.y = m_start.y + m_pos.y;
		}
		if (m_start.y + m_pos.y > b_rb.y) {
			b_rb.y = m_start.y + m_pos.y;
		}
	}

	// ���_�𓾂�.
	size_t ShapeLine::get_verts(D2D1_POINT_2F p[]) const noexcept
	{
		p[0] = m_start;
		p[1].x = m_start.x + m_pos.x;
		p[1].y = m_start.y + m_pos.y;
		p[2].x = static_cast<FLOAT>(m_start.x + 0.5 * m_pos.x);
		p[2].y = static_cast<FLOAT>(m_start.y + 0.5 * m_pos.y);
		return 3;
	}

	// �}�`���_���܂ނ����肷��.
	// �߂�l	�_���܂ޕ���
	uint32_t ShapeLine::hit_test(
		const D2D1_POINT_2F t	// ���肳���_
	) const noexcept
	{
		D2D1_POINT_2F end;	// �I�_
		pt_add(m_start, m_pos, end);
		if (loc_hit_test(t, end, m_loc_width)) {
			return LOC_TYPE::LOC_END;
		}
		if (loc_hit_test(t, m_start, m_loc_width)) {
			return LOC_TYPE::LOC_START;
		}
		const double e_width = 0.5 * max(m_stroke_width, m_loc_width);
		if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE)) {
			D2D1_POINT_2F pos{ m_pos };
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
				return LOC_TYPE::LOC_STROKE;
			}
		}
		else if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE)) {
			D2D1_POINT_2F p{ m_pos };
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
				return LOC_TYPE::LOC_STROKE;
			}
		}
		else {
			if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND)) {
				if (pt_in_circle(t, m_start, e_width) || pt_in_circle(t, end, e_width)) {
					return LOC_TYPE::LOC_STROKE;
				}
			}
			D2D1_POINT_2F p{ m_pos };
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
					return LOC_TYPE::LOC_STROKE;
				}
			}
		}
		return LOC_TYPE::LOC_PAGE;
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
			pt_add(m_start, m_pos, p);
			return pt_in_rect(p, lt, rb);
		}
		return false;
	}

	// �l������̌����̐�萧���Ɋi�[����.
	bool ShapeLine::set_join_miter_limit(const float& val) noexcept
	{
		if (ShapeStroke::set_join_miter_limit(val)) {
			m_d2d_arrow_stroke = nullptr;
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

	// �l��, �w�肵�����ʂ̓_�Ɋi�[����.
	bool ShapeLine::set_pos_loc(
		const D2D1_POINT_2F val,	// �l
		const uint32_t loc,	// ����
		const float snap_point,	// �_��_�ɂ������邵�����l
		const bool /*keep_aspect*/
	) noexcept
	{
		bool flag = false;
		if (loc == LOC_TYPE::LOC_START) {
			if (!equal(m_start, val)) {
				const D2D1_POINT_2F end{
					m_start.x + m_pos.x, m_start.y + m_pos.y
				};
				m_pos.x = end.x - val.x;
				m_pos.y = end.y - val.y;
				m_start = val;
				flag = true;
			}
		}
		else if (loc == LOC_TYPE::LOC_END) {
			const D2D1_POINT_2F end{
				m_start.x + m_pos.x, m_start.y + m_pos.y
			};
			if (!equal(end, val)) {
				m_pos.x = val.x - m_start.x;
				m_pos.y = val.y - m_start.y;
				flag = true;
			}
		}
		if (flag) {
			const double ss = static_cast<double>(snap_point) * static_cast<double>(snap_point);
			if (ss > FLT_MIN && pt_abs2(m_pos) <= ss) {
				if (loc == LOC_TYPE::LOC_START) {
					m_start.x = m_start.x + m_pos.x;
					m_start.y = m_start.y + m_pos.y;
				}
				m_pos.x = 0.0f;
				m_pos.y = 0.0f;
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
	bool ShapeLine::set_stroke_cap(const D2D1_CAP_STYLE& val) noexcept
	//bool ShapeLine::set_stroke_cap(const CAP_STYLE& val) noexcept
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
	ShapeLine::ShapeLine(
		const D2D1_POINT_2F start,	// �n�_
		const D2D1_POINT_2F pos,	// �I�_�ւ̈ʒu�x�N�g��
		const Shape* prop	// ����
	) :
		ShapeArrow::ShapeArrow(prop)
	{
		m_start = start;
		m_pos = pos;
		prop->get_arrow_style(m_arrow_style);
		prop->get_arrow_size(m_arrow_size);
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
		m_pos.x = dt_reader.ReadSingle();
		m_pos.y = dt_reader.ReadSingle();
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapeLine::write(DataWriter const& dt_writer) const
	{
		ShapeArrow::write(dt_writer);

		// �n�_
		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);

		// �I�_�_�ւ̈ʒu�x�N�g��
		dt_writer.WriteSingle(m_pos.x);
		dt_writer.WriteSingle(m_pos.y);
	}

	// �ߖT�̒��_��������.
	// pos	����ʒu
	// dd	�ߖT�Ƃ݂Ȃ����� (�̓��l), �����藣�ꂽ���_�͋ߖT�Ƃ݂͂Ȃ��Ȃ�.
	// val	����ʒu�̋ߖT�ɂ��钸�_
	// �߂�l	���������� true
	bool ShapeLine::get_pos_nearest(const D2D1_POINT_2F p, double& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool done = false;
		D2D1_POINT_2F d;
		pt_sub(m_start, p, d);
		double d_abs = pt_abs2(d);
		if (d_abs < dd) {
			dd = d_abs;
			val = m_start;
			done = true;
		}
		D2D1_POINT_2F q{ m_start.x + m_pos.x, m_start.y + m_pos.y };
		pt_sub(q, p, d);
		d_abs = pt_abs2(d);
		if (d_abs < dd) {
			dd = d_abs;
			val = q;
			done = true;
		}
		D2D1_POINT_2F r{ 
			static_cast<FLOAT>(m_start.x + 0.5 * m_pos.x),
			static_cast<FLOAT>(m_start.y + 0.5 * m_pos.y)
		};
		pt_sub(r, p, d);
		d_abs = pt_abs2(d);
		if (d_abs < dd) {
			dd = d_abs;
			val = r;
			done = true;
		}
		return done;
	}

}