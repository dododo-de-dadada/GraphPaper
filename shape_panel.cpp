#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �w�肵���F�ƕs�����x���甽�ΐF�𓾂�.
	static void get_opposite_color(const D2D1_COLOR_F& src, const double opa, D2D1_COLOR_F& dst) noexcept;

	// �w�肵���F�ƕs�����x���甽�ΐF�𓾂�.
	// src	�w�肵���F
	// opa	�w�肵���s�����x
	// dst	���ΐF
	static void get_opposite_color(const D2D1_COLOR_F& src, const double opa, D2D1_COLOR_F& dst) noexcept
	{
		dst.r = (src.r <= 0.5f ? 1.0f : 0.0f);
		dst.g = (src.g <= 0.5f ? 1.0f : 0.0f);
		dst.b = (src.b <= 0.5f ? 1.0f : 0.0f);
		dst.a = static_cast<FLOAT>(opa);
	}

	// �Ȑ��̕⏕��(����_�����Ԑ܂��)��\������.
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapePanel::draw_auxiliary_bezi(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		ID2D1Brush* br = dx.m_aux_brush.get();
		ID2D1StrokeStyle* ss = dx.m_aux_style.get();
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);
		D2D1_POINT_2F s_pos;
		D2D1_POINT_2F e_pos;

		e_pos.x = c_pos.x;
		e_pos.y = p_pos.y;
		dx.m_d2dContext->DrawLine(p_pos, e_pos, br, sw, ss);
		s_pos = e_pos;
		e_pos.x = p_pos.x;
		e_pos.y = c_pos.y;
		dx.m_d2dContext->DrawLine(s_pos, e_pos, br, sw, ss);
		s_pos = e_pos;
		dx.m_d2dContext->DrawLine(s_pos, c_pos, br, sw, ss);
	}

	// ���~�̕⏕����\������.
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapePanel::draw_auxiliary_elli(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		auto br = dx.m_aux_brush.get();
		auto ss = dx.m_aux_style.get();
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);
		D2D1_POINT_2F r;	// ���`
		D2D1_ELLIPSE e;		// ���~

		pt_sub(c_pos, p_pos, r);
		pt_scale(r, 0.5, r);
		pt_add(p_pos, r, e.point);
		e.radiusX = r.x;
		e.radiusY = r.y;
		dx.m_d2dContext->DrawEllipse(e, br, sw, ss);
	}

	// �����̕⏕����\������.
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapePanel::draw_auxiliary_line(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		auto br = dx.m_aux_brush.get();
		auto ss = dx.m_aux_style.get();
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);
		dx.m_d2dContext->DrawLine(p_pos, c_pos, br, sw, ss);
	}

	// �Ђ��`�̕⏕����\������.
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapePanel::draw_auxiliary_quad(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		auto br = dx.m_aux_brush.get();
		auto ss = dx.m_aux_style.get();
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);
		D2D1_POINT_2F m_pos;
		D2D1_POINT_2F q_pos[4];

		pt_avg(p_pos, c_pos, m_pos);
		q_pos[0] = { m_pos.x, p_pos.y };
		q_pos[1] = { c_pos.x, m_pos.y };
		q_pos[2] = { m_pos.x, c_pos.y };
		q_pos[3] = { p_pos.x, m_pos.y };
		dx.m_d2dContext->DrawLine(q_pos[0], q_pos[1], br, sw, ss);
		dx.m_d2dContext->DrawLine(q_pos[1], q_pos[2], br, sw, ss);
		dx.m_d2dContext->DrawLine(q_pos[2], q_pos[3], br, sw, ss);
		dx.m_d2dContext->DrawLine(q_pos[3], q_pos[0], br, sw, ss);
	}

	// ���`�̕⏕����\������.
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapePanel::draw_auxiliary_rect(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		auto br = dx.m_aux_brush.get();
		auto ss = dx.m_aux_style.get();
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);
		const D2D1_RECT_F rc = {
			p_pos.x, p_pos.y, c_pos.x, c_pos.y
		};
		dx.m_d2dContext->DrawRectangle(&rc, br, sw, ss);
	}

	// �p�ە��`�̕⏕����\������.
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	// c_rad	�p�۔��a
	void ShapePanel::draw_auxiliary_rrect(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		auto br = dx.m_aux_brush.get();
		auto ss = dx.m_aux_style.get();
		auto c_rad = m_corner_rad;
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);
		const double cx = c_pos.x;
		const double cy = c_pos.y;
		const double px = p_pos.x;
		const double py = p_pos.y;
		const double qx = cx - px;
		const double qy = cy - py;
		double rx = c_rad.x;
		double ry = c_rad.y;

		if (qx * rx < 0.0f) {
			rx = -rx;
		}
		if (qy * ry < 0.0f) {
			ry = -ry;
		}
		const D2D1_ROUNDED_RECT rr = {
			{ p_pos.x, p_pos.y, c_pos.x, c_pos.y },
			static_cast<FLOAT>(rx),
			static_cast<FLOAT>(ry)
		};
		dx.m_d2dContext->DrawRoundedRectangle(&rr, br, sw, ss);
	}

	// �������\������.
	// offset	������̂��炵��
	void ShapePanel::draw_grid_line(SHAPE_DX const& dx, const D2D1_POINT_2F offset)
	{
		const double pw = m_page_size.width;	// �y�[�W�̑傫��
		const double ph = m_page_size.height;
		// �g�傳��Ă� 1 �s�N�Z���ɂȂ�悤�g�嗦�̋t������g�̑����Ɋi�[����.
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);	// ������̑���
		D2D1_POINT_2F h_start, h_end;	// ���̕�����̊J�n�E�I���ʒu
		D2D1_POINT_2F v_start, v_end;	// �c�̕�����̊J�n�E�I���ʒu
		auto br = dx.m_shape_brush.get();

		m_grid_color.a = static_cast<FLOAT>(m_grid_opac);
		br->SetColor(m_grid_color);
		v_start.y = 0.0f;
		h_start.x = 0.0f;
		v_end.y = m_page_size.height;
		h_end.x = m_page_size.width;
		const double g_len = max(m_grid_len + 1.0, 1.0);
		// �����ȕ������\������.
		double x;
		for (uint32_t i = 0; (x = g_len * i + offset.x) < pw; i++) {
			v_start.x = v_end.x = static_cast<FLOAT>(x);
			dx.m_d2dContext->DrawLine(v_start, v_end, br, sw, nullptr);
		}
		// �����ȕ������\������.
		double y;
		for (uint32_t i = 0; (y = g_len * i + offset.y) < ph; i++) {
			h_start.y = h_end.y = static_cast<FLOAT>(y);
			dx.m_d2dContext->DrawLine(h_start, h_end, br, sw, nullptr);
		}
	}

	// ���̐��@�𓾂�.
	bool ShapePanel::get_arrow_size(ARROW_SIZE& val) const noexcept
	{
		val = m_arrow_size;
		return true;
	}

	// ���̌`���𓾂�.
	bool ShapePanel::get_arrow_style(ARROW_STYLE& val) const noexcept
	{
		val = m_arrow_style;
		return true;
	}

	// �p�۔��a�𓾂�.
	bool ShapePanel::get_corner_radius(D2D1_POINT_2F& val) const noexcept
	{
		val = m_corner_rad;
		return true;
	}

	// �h��Ԃ��̐F�𓾂�.
	bool ShapePanel::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}

	// ���̂̐F�𓾂�.
	bool ShapePanel::get_font_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_font_color;
		return true;
	}

	// ���̖��𓾂�.
	bool ShapePanel::get_font_family(wchar_t*& val) const noexcept
	{
		val = m_font_family;
		return true;
	}

	// ���̂̑傫���𓾂�.
	bool ShapePanel::get_font_size(double& val) const noexcept
	{
		val = m_font_size;
		return true;
	}

	// ���̂̐L�k�𓾂�.
	bool ShapePanel::get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept
	{
		val = m_font_stretch;
		return true;
	}

	// ���̂̎��̂𓾂�.
	bool ShapePanel::get_font_style(DWRITE_FONT_STYLE& val) const noexcept
	{
		val = m_font_style;
		return true;
	}

	// ���̂̑����𓾂�.
	bool ShapePanel::get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept
	{
		val = m_font_weight;
		return true;
	}

	// ����̈�ӂ̒����𓾂�.
	bool ShapePanel::get_grid_len(double& val) const noexcept
	{
		val = m_grid_len;
		return true;
	}

	// ������̕s�����x�𓾂�.
	bool ShapePanel::get_grid_opac(double& val) const noexcept
	{
		val = m_grid_opac;
		return true;
	}

	// ������̕\���𓾂�.
	bool ShapePanel::get_grid_show(GRID_SHOW& val) const noexcept
	{
		val = m_grid_show;
		return true;
	}

	// ����ւ̂��낦�𓾂�.
	bool ShapePanel::get_grid_snap(bool& val) const noexcept
	{
		val = m_grid_snap;
		return true;
	}

	// �y�[�W�̐F�𓾂�.
	bool ShapePanel::get_page_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_page_color;
		return true;
	}

	// �y�[�W�̊g�嗦�𓾂�.
	bool ShapePanel::get_page_scale(double& val) const noexcept
	{
		val = m_page_scale;
		return true;
	}

	// �y�[�W�̐��@�𓾂�.
	bool ShapePanel::get_page_size(D2D1_SIZE_F& val) const noexcept
	{
		val = m_page_size;
		return true;
	}

	// �y�[�W�̒P�ʂ𓾂�.
	bool ShapePanel::get_page_unit(UNIT& val) const noexcept
	{
		val = m_page_unit;
		return true;
	}

	// ���g�̐F�𓾂�.
	bool ShapePanel::get_stroke_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_stroke_color;
		return true;
	}

	// �j���̔z�u�𓾂�.
	bool ShapePanel::get_stroke_pattern(STROKE_PATTERN& val) const noexcept
	{
		val = m_stroke_pattern;
		return true;
	}

	// ���g�̌`���𓾂�.
	bool ShapePanel::get_stroke_style(D2D1_DASH_STYLE& val) const noexcept
	{
		val = m_stroke_style;
		return true;
	}

	// ���g�̑����𓾂�.
	bool ShapePanel::get_stroke_width(double& val) const noexcept
	{
		val = m_stroke_width;
		return true;
	}

	// �i���̑����𓾂�.
	bool ShapePanel::get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_p;
		return true;
	}

	// ������̂��낦�𓾂�.
	bool ShapePanel::get_text_align_t(DWRITE_TEXT_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_t;
		return true;
	}

	// �s�Ԃ𓾂�.
	bool ShapePanel::get_text_line(double& val) const noexcept
	{
		val = m_text_line;
		return true;
	}

	// ������̗]���𓾂�.
	bool ShapePanel::get_text_margin(D2D1_SIZE_F& val) const noexcept
	{
		val = m_text_mar;
		return true;
	}

	// �f�[�^���[�_�[����ǂݍ���.
	void ShapePanel::read(DataReader const& dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_grid_color, dt_reader);
		m_grid_len = dt_reader.ReadDouble();
		m_grid_opac = dt_reader.ReadDouble();
		read(m_grid_show, dt_reader);
		m_grid_snap = dt_reader.ReadBoolean();
		read(m_page_color, dt_reader);
		m_page_scale = dt_reader.ReadDouble();
		read(m_page_size, dt_reader);
		read(m_page_unit, dt_reader);

		read(m_arrow_size, dt_reader);	// ���̐��@
		read(m_arrow_style, dt_reader);	// ���̌`��
		read(m_corner_rad, dt_reader);	// �p�۔��a
		read(m_stroke_color, dt_reader);	// ���E�g�̐F
		read(m_stroke_pattern, dt_reader);	// �j���̔z�u
		read(m_stroke_style, dt_reader);	// �j���̌`��
		read(m_stroke_width, dt_reader);	// ���E�g�̑���
		read(m_fill_color, dt_reader);	// �h��Ԃ��̐F
		read(m_font_color, dt_reader);	// ���̂̐F
		read(m_font_family, dt_reader);	// ���̖�
		read(m_font_size, dt_reader);	// ���̂̑傫��
		read(m_font_stretch, dt_reader);	// ���̂̐L�k
		read(m_font_style, dt_reader);	// ���̂̎���
		read(m_font_weight, dt_reader);	// ���̂̑���
		read(m_text_align_p, dt_reader);	// �i���̂��낦
		read(m_text_align_t, dt_reader);	// ������̂��낦
		read(m_text_line, dt_reader);	// �s��
		read(m_text_mar, dt_reader);	// ������̗]��

		ShapeText::is_available_font(m_font_family);
	}

	// �l����̐��@�Ɋi�[����.
	void ShapePanel::set_arrow_size(const ARROW_SIZE& val)
	{
		m_arrow_size = val;
	}

	// �l����̌`���Ɋi�[����.
	void ShapePanel::set_arrow_style(const ARROW_STYLE val)
	{
		m_arrow_style = val;
	}

	// �l��h��Ԃ��̐F�Ɋi�[����.
	void ShapePanel::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		m_fill_color = val;
	}

	// �l�����̂̐F�Ɋi�[����.
	void ShapePanel::set_font_color(const D2D1_COLOR_F& val) noexcept
	{
		m_font_color = val;
	}

	// �l�����̖��Ɋi�[����.
	void ShapePanel::set_font_family(wchar_t* const val)
	{
		m_font_family = val;
	}

	// �l�����̂̑傫���Ɋi�[����.
	void ShapePanel::set_font_size(const double val)
	{
		m_font_size = val;
	}

	// �l�����̂̐L�k�Ɋi�[����.
	void ShapePanel::set_font_stretch(const DWRITE_FONT_STRETCH val)
	{
		m_font_stretch = val;
	}

	// ���̂̎��̂Ɋi�[����.
	void ShapePanel::set_font_style(const DWRITE_FONT_STYLE val)
	{
		m_font_style = val;
	}

	// �l�����̂̑����Ɋi�[����.
	void ShapePanel::set_font_weight(const DWRITE_FONT_WEIGHT val)
	{
		m_font_weight = val;
	}

	// �l�����̈�ӂ̒����Ɋi�[����.
	void ShapePanel::set_grid_len(const double val) noexcept
	{
		m_grid_len = val;
	}

	// �l�������̕s�����x�Ɋi�[����.
	void ShapePanel::set_grid_opac(const double val) noexcept
	{
		m_grid_opac = val;
	}

	// �l�������̕\���Ɋi�[����.
	void ShapePanel::set_grid_show(const GRID_SHOW val) noexcept
	{
		m_grid_show = val;
	}

	// �l�����ւ̂��낦�Ɋi�[����.
	void ShapePanel::set_grid_snap(const bool val) noexcept
	{
		m_grid_snap = val;
	}

	// �l���y�[�W, ����, �⏕���̐F�Ɋi�[����
	void ShapePanel::set_page_color(const D2D1_COLOR_F& val) noexcept
	{
		m_page_color = val;
		get_opposite_color(val, m_grid_opac, m_grid_color);
		get_opposite_color(val, ANCH_OPAC, m_anch_color);
		get_opposite_color(val, AUX_OPAC, m_aux_color);
	}

	// �l���y�[�W�̊g�嗦�Ɋi�[����.
	void ShapePanel::set_page_scale(const double val) noexcept
	{
		m_page_scale = val;
	}

	// �l���y�[�W�̐��@�Ɋi�[����.
	void ShapePanel::set_page_size(const D2D1_SIZE_F val) noexcept
	{
		m_page_size = val;
	}

	// �l���y�[�W�̒P�ʂɊi�[����.
	void ShapePanel::set_page_unit(const UNIT val) noexcept
	{
		m_page_unit = val;
	}

	// ���g�̐F�Ɋi�[����.
	void ShapePanel::set_stroke_color(const D2D1_COLOR_F& val) noexcept
	{
		m_stroke_color = val;
	}

	// �j���̔z�u�Ɋi�[����.
	void ShapePanel::set_stroke_pattern(const STROKE_PATTERN& val)
	{
		m_stroke_pattern = val;
	}

	// ���g�̌`���Ɋi�[����.
	void ShapePanel::set_stroke_style(const D2D1_DASH_STYLE val)
	{
		m_stroke_style = val;
	}

	// ���g�̑����Ɋi�[����.
	void ShapePanel::set_stroke_width(const double val) noexcept
	{
		m_stroke_width = val;
	}

	// �l��i���̂��낦�Ɋi�[����.
	void ShapePanel::set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT val)
	{
		m_text_align_p = val;
	}

	// ������̂��낦�Ɋi�[����.
	void ShapePanel::set_text_align_t(const DWRITE_TEXT_ALIGNMENT val)
	{
		m_text_align_t = val;
	}

	// �l���s�ԂɊi�[����.
	void ShapePanel::set_text_line(const double val)
	{
		m_text_line = val;
	}

	// �l�𕶎���̗]���Ɋi�[����.
	void ShapePanel::set_text_margin(const D2D1_SIZE_F val)
	{
		m_text_mar = val;
	}

	// �}�`�̑����l���i�[����.
	void ShapePanel::set_to_shape(Shape* s) noexcept
	{
		s->get_arrow_size(m_arrow_size);
		s->get_arrow_style(m_arrow_style);
		s->get_corner_radius(m_corner_rad);
		s->get_fill_color(m_fill_color);
		s->get_font_color(m_font_color);
		s->get_font_family(m_font_family);
		s->get_font_size(m_font_size);
		s->get_font_stretch(m_font_stretch);
		s->get_font_style(m_font_style);
		s->get_font_weight(m_font_weight);
		s->get_grid_opac(m_grid_opac);
		s->get_grid_len(m_grid_len);
		s->get_grid_show(m_grid_show);
		s->get_grid_snap(m_grid_snap);
		s->get_text_line(m_text_line);
		D2D1_COLOR_F color;
		if (s->get_page_color(color)) {
			set_page_color(color);
		}
		s->get_stroke_color(m_stroke_color);
		s->get_stroke_pattern(m_stroke_pattern);
		s->get_stroke_style(m_stroke_style);
		s->get_stroke_width(m_stroke_width);
		s->get_text_align_t(m_text_align_t);
		s->get_text_align_p(m_text_align_p);
		s->get_text_margin(m_text_mar);
	}

	// �f�[�^���[�_�[�ɏ�������.
	void ShapePanel::write(DataWriter const& dt_writer)
	{
		using winrt::GraphPaper::implementation::write;

		write(m_grid_color, dt_writer);
		dt_writer.WriteDouble(m_grid_len);
		dt_writer.WriteDouble(m_grid_opac);
		write(m_grid_show, dt_writer);
		dt_writer.WriteBoolean(m_grid_snap);
		write(m_page_color, dt_writer);
		dt_writer.WriteDouble(m_page_scale);
		write(m_page_size, dt_writer);
		write(m_page_unit, dt_writer);

		write(m_arrow_size, dt_writer);	// ���̐��@
		write(m_arrow_style, dt_writer);	// ���̌`��
		write(m_corner_rad, dt_writer);	// �p�۔��a
		write(m_stroke_color, dt_writer);	// ���g�̐F
		write(m_stroke_pattern, dt_writer);	// �j���̔z�u
		write(m_stroke_style, dt_writer);	// ���g�̌`��
		write(m_stroke_width, dt_writer);	// ���g�̑���
		write(m_fill_color, dt_writer);	// �h��Ԃ��̐F
		write(m_font_color, dt_writer);	// ���̂̐F
		write(m_font_family, dt_writer);	// ���̖�
		write(m_font_size, dt_writer);	// ���̂̑傫��
		write(m_font_stretch, dt_writer);	// ���̂̐L�k
		write(m_font_style, dt_writer);	// ���̂̎���
		write(m_font_weight, dt_writer);	// ���̂̑���
		write(m_text_align_p, dt_writer);	// �i���̂��낦
		write(m_text_align_t, dt_writer);	// ������̂��낦
		write(m_text_line, dt_writer);	// �s��
		write(m_text_mar, dt_writer);	// ������̗]��

	}
}