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
	void ShapeLayout::draw_auxiliary_bezi(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
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
	void ShapeLayout::draw_auxiliary_elli(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
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
	void ShapeLayout::draw_auxiliary_line(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		auto br = dx.m_aux_brush.get();
		auto ss = dx.m_aux_style.get();
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);
		dx.m_d2dContext->DrawLine(p_pos, c_pos, br, sw, ss);
	}

	// �Ђ��`�̕⏕����\������.
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapeLayout::draw_auxiliary_quad(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
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
	void ShapeLayout::draw_auxiliary_rect(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
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
	void ShapeLayout::draw_auxiliary_rrect(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
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
	void ShapeLayout::draw_grid(SHAPE_DX const& dx, const D2D1_POINT_2F offset)
	{
		const double pw = m_page_size.width;	// �y�[�W�̑傫��
		const double ph = m_page_size.height;
		// �g�傳��Ă� 1 �s�N�Z���ɂȂ�悤�g�嗦�̋t������g�̑����Ɋi�[����.
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);	// ������̑���
		D2D1_POINT_2F h_start, h_end;	// ���̕�����̊J�n�E�I���ʒu
		D2D1_POINT_2F v_start, v_end;	// �c�̕�����̊J�n�E�I���ʒu
		auto br = dx.m_shape_brush.get();

		D2D1_COLOR_F grid_color;
		get_grid_color(grid_color);
		br->SetColor(grid_color);
		v_start.y = 0.0f;
		h_start.x = 0.0f;
		v_end.y = m_page_size.height;
		h_end.x = m_page_size.width;
		const double g_len = max(m_grid_base + 1.0, 1.0);

		// �����ȕ������\������.
		float w;
		double x;
		for (uint32_t i = 0; (x = g_len * i + offset.x) < pw; i++) {
			if (m_grid_patt == GRID_PATT::PATT_3 && (i % 10) == 0) {
				w = 2.0F * sw;
			}
			else if (m_grid_patt == GRID_PATT::PATT_1 || (i % 2) == 0) {
				w = sw;
			}
			else {
				w = 0.5F * sw;
			}
			v_start.x = v_end.x = static_cast<FLOAT>(x);
			dx.m_d2dContext->DrawLine(v_start, v_end, br, w, nullptr);
		}
		// �����ȕ������\������.
		double y;
		for (uint32_t i = 0; (y = g_len * i + offset.y) < ph; i++) {
			if (m_grid_patt == GRID_PATT::PATT_3 && (i % 10) == 0) {
				w = 2.0F * sw;
			}
			else if (m_grid_patt == GRID_PATT::PATT_1 || (i % 2) == 0) {
				w = sw;
			}
			else {
				w = 0.5F * sw;
			}
			h_start.y = h_end.y = static_cast<FLOAT>(y);
			dx.m_d2dContext->DrawLine(h_start, h_end, br, w, nullptr);
		}

	}

	// ���ʂ̐F�𓾂�.
	void ShapeLayout::get_anchor_color(D2D1_COLOR_F& value) const noexcept
	{
		get_opposite_color(m_page_color, ANCH_OPAC, value);
	}

	// ���̐��@�𓾂�.
	bool ShapeLayout::get_arrow_size(ARROW_SIZE& value) const noexcept
	{
		value = m_arrow_size;
		return true;
	}

	// ���̌`���𓾂�.
	bool ShapeLayout::get_arrow_style(ARROW_STYLE& value) const noexcept
	{
		value = m_arrow_style;
		return true;
	}

	// �⏕���̐F�𓾂�.
	void ShapeLayout::get_auxiliary_color(D2D1_COLOR_F& value) const noexcept
	{
		get_opposite_color(m_page_color, AUX_OPAC, value);
	}

	// �p�۔��a�𓾂�.
	bool ShapeLayout::get_corner_radius(D2D1_POINT_2F& value) const noexcept
	{
		value = m_corner_rad;
		return true;
	}

	// �h��Ԃ��̐F�𓾂�.
	bool ShapeLayout::get_fill_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_fill_color;
		return true;
	}

	// ���̂̐F�𓾂�.
	bool ShapeLayout::get_font_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_font_color;
		return true;
	}

	// ���̖��𓾂�.
	bool ShapeLayout::get_font_family(wchar_t*& value) const noexcept
	{
		value = m_font_family;
		return true;
	}

	// ���̂̑傫���𓾂�.
	bool ShapeLayout::get_font_size(double& value) const noexcept
	{
		value = m_font_size;
		return true;
	}

	// ���̂̐L�k�𓾂�.
	bool ShapeLayout::get_font_stretch(DWRITE_FONT_STRETCH& value) const noexcept
	{
		value = m_font_stretch;
		return true;
	}

	// ���̂̎��̂𓾂�.
	bool ShapeLayout::get_font_style(DWRITE_FONT_STYLE& value) const noexcept
	{
		value = m_font_style;
		return true;
	}

	// ���̂̑����𓾂�.
	bool ShapeLayout::get_font_weight(DWRITE_FONT_WEIGHT& value) const noexcept
	{
		value = m_font_weight;
		return true;
	}

	// ����̊�̑傫���𓾂�.
	bool ShapeLayout::get_grid_base(double& value) const noexcept
	{
		value = m_grid_base;
		return true;
	}

	// ������̐F�𓾂�.
	void ShapeLayout::get_grid_color(D2D1_COLOR_F& value) const noexcept
	{
		get_opposite_color(m_page_color, m_grid_opac, value);
	}

	// ������̕s�����x�𓾂�.
	bool ShapeLayout::get_grid_opac(double& value) const noexcept
	{
		value = m_grid_opac;
		return true;
	}

	// ������̌`���𓾂�.
	bool ShapeLayout::get_grid_patt(GRID_PATT& value) const noexcept
	{
		value = m_grid_patt;
		return true;
	}

	// ������̕\���𓾂�.
	bool ShapeLayout::get_grid_show(GRID_SHOW& value) const noexcept
	{
		value = m_grid_show;
		return true;
	}

	// ����ւ̂��낦�𓾂�.
	bool ShapeLayout::get_grid_snap(bool& value) const noexcept
	{
		value = m_grid_snap;
		return true;
	}

	// �y�[�W�̐F�𓾂�.
	bool ShapeLayout::get_page_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_page_color;
		return true;
	}

	// �y�[�W�̊g�嗦�𓾂�.
	bool ShapeLayout::get_page_scale(double& value) const noexcept
	{
		value = m_page_scale;
		return true;
	}

	// �y�[�W�̐��@�𓾂�.
	bool ShapeLayout::get_page_size(D2D1_SIZE_F& value) const noexcept
	{
		value = m_page_size;
		return true;
	}

	// ���g�̐F�𓾂�.
	bool ShapeLayout::get_stroke_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_stroke_color;
		return true;
	}

	// �j���̔z�u�𓾂�.
	bool ShapeLayout::get_stroke_pattern(STROKE_PATTERN& value) const noexcept
	{
		value = m_stroke_pattern;
		return true;
	}

	// ���g�̌`���𓾂�.
	bool ShapeLayout::get_stroke_style(D2D1_DASH_STYLE& value) const noexcept
	{
		value = m_stroke_style;
		return true;
	}

	// ���g�̑����𓾂�.
	bool ShapeLayout::get_stroke_width(double& value) const noexcept
	{
		value = m_stroke_width;
		return true;
	}

	// �i���̑����𓾂�.
	bool ShapeLayout::get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& value) const noexcept
	{
		value = m_text_align_p;
		return true;
	}

	// ������̂��낦�𓾂�.
	bool ShapeLayout::get_text_align_t(DWRITE_TEXT_ALIGNMENT& value) const noexcept
	{
		value = m_text_align_t;
		return true;
	}

	// �s�Ԃ𓾂�.
	bool ShapeLayout::get_text_line_height(double& value) const noexcept
	{
		value = m_text_line;
		return true;
	}

	// ������̗]���𓾂�.
	bool ShapeLayout::get_text_margin(D2D1_SIZE_F& value) const noexcept
	{
		value = m_text_margin;
		return true;
	}

	// �f�[�^���[�_�[����ǂݍ���.
	void ShapeLayout::read(DataReader const& dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		D2D1_COLOR_F dummy;
		read(dummy, dt_reader);	// ����̐F
		m_grid_base = dt_reader.ReadDouble();	// ����̑傫��
		m_grid_opac = dt_reader.ReadDouble();
		read(m_grid_patt, dt_reader);
		read(m_grid_show, dt_reader);
		m_grid_snap = dt_reader.ReadBoolean();
		read(m_page_color, dt_reader);
		//D2D1_COLOR_F color;
		//read(color, dt_reader);
		//set_page_color(color);
		m_page_scale = dt_reader.ReadDouble();
		read(m_page_size, dt_reader);

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
		read(m_text_margin, dt_reader);	// ������̗]��

		ShapeText::is_available_font(m_font_family);
	}

	// �l����̐��@�Ɋi�[����.
	void ShapeLayout::set_arrow_size(const ARROW_SIZE& value)
	{
		m_arrow_size = value;
	}

	// �l����̌`���Ɋi�[����.
	void ShapeLayout::set_arrow_style(const ARROW_STYLE value)
	{
		m_arrow_style = value;
	}

	// �l��h��Ԃ��̐F�Ɋi�[����.
	void ShapeLayout::set_fill_color(const D2D1_COLOR_F& value) noexcept
	{
		m_fill_color = value;
	}

	// �l�����̂̐F�Ɋi�[����.
	void ShapeLayout::set_font_color(const D2D1_COLOR_F& value) noexcept
	{
		m_font_color = value;
	}

	// �l�����̖��Ɋi�[����.
	void ShapeLayout::set_font_family(wchar_t* const value)
	{
		m_font_family = value;
	}

	// �l�����̂̑傫���Ɋi�[����.
	void ShapeLayout::set_font_size(const double value)
	{
		m_font_size = value;
	}

	// �l�����̂̐L�k�Ɋi�[����.
	void ShapeLayout::set_font_stretch(const DWRITE_FONT_STRETCH value)
	{
		m_font_stretch = value;
	}

	// ���̂̎��̂Ɋi�[����.
	void ShapeLayout::set_font_style(const DWRITE_FONT_STYLE value)
	{
		m_font_style = value;
	}

	// �l�����̂̑����Ɋi�[����.
	void ShapeLayout::set_font_weight(const DWRITE_FONT_WEIGHT value)
	{
		m_font_weight = value;
	}

	// �l�����̊�̑傫���Ɋi�[����.
	void ShapeLayout::set_grid_base(const double value) noexcept
	{
		m_grid_base = value;
	}

	// �l�������̕s�����x�Ɋi�[����.
	void ShapeLayout::set_grid_opac(const double value) noexcept
	{
		m_grid_opac = value;
	}

	// �l�������̌`���Ɋi�[����.
	void ShapeLayout::set_grid_patt(const GRID_PATT value) noexcept
	{
		m_grid_patt = value;
	}

	// �l�������̕\���Ɋi�[����.
	void ShapeLayout::set_grid_show(const GRID_SHOW value) noexcept
	{
		m_grid_show = value;
	}

	// �l�����ւ̂��낦�Ɋi�[����.
	void ShapeLayout::set_grid_snap(const bool value) noexcept
	{
		m_grid_snap = value;
	}

	// �l���y�[�W, ����, �⏕���̐F�Ɋi�[����
	void ShapeLayout::set_page_color(const D2D1_COLOR_F& value) noexcept
	{
		m_page_color = value;
		m_page_color.a = 1.0F;
	}

	// �l���y�[�W�̊g�嗦�Ɋi�[����.
	void ShapeLayout::set_page_scale(const double value) noexcept
	{
		m_page_scale = value;
	}

	// �l���y�[�W�̐��@�Ɋi�[����.
	void ShapeLayout::set_page_size(const D2D1_SIZE_F value) noexcept
	{
		m_page_size = value;
	}

	// ���g�̐F�Ɋi�[����.
	void ShapeLayout::set_stroke_color(const D2D1_COLOR_F& value) noexcept
	{
		m_stroke_color = value;
	}

	// �j���̔z�u�Ɋi�[����.
	void ShapeLayout::set_stroke_pattern(const STROKE_PATTERN& value)
	{
		m_stroke_pattern = value;
	}

	// ���g�̌`���Ɋi�[����.
	void ShapeLayout::set_stroke_style(const D2D1_DASH_STYLE value)
	{
		m_stroke_style = value;
	}

	// ���g�̑����Ɋi�[����.
	void ShapeLayout::set_stroke_width(const double value) noexcept
	{
		m_stroke_width = value;
	}

	// �l��i���̂��낦�Ɋi�[����.
	void ShapeLayout::set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT value)
	{
		m_text_align_p = value;
	}

	// ������̂��낦�Ɋi�[����.
	void ShapeLayout::set_text_align_t(const DWRITE_TEXT_ALIGNMENT value)
	{
		m_text_align_t = value;
	}

	// �l���s�ԂɊi�[����.
	void ShapeLayout::set_text_line_height(const double value)
	{
		m_text_line = value;
	}

	// �l�𕶎���̗]���Ɋi�[����.
	void ShapeLayout::set_text_margin(const D2D1_SIZE_F value)
	{
		m_text_margin = value;
	}

	// �}�`�̑����l���i�[����.
	void ShapeLayout::set_to_shape(Shape* s) noexcept
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
		s->get_grid_base(m_grid_base);
		s->get_grid_opac(m_grid_opac);
		s->get_grid_patt(m_grid_patt);
		s->get_grid_show(m_grid_show);
		s->get_grid_snap(m_grid_snap);
		s->get_page_color(m_page_color);
		s->get_stroke_color(m_stroke_color);
		s->get_stroke_pattern(m_stroke_pattern);
		s->get_stroke_style(m_stroke_style);
		s->get_stroke_width(m_stroke_width);
		s->get_text_line_height(m_text_line);
		s->get_text_align_t(m_text_align_t);
		s->get_text_align_p(m_text_align_p);
		s->get_text_margin(m_text_margin);
	}

	// �f�[�^���[�_�[�ɏ�������.
	void ShapeLayout::write(DataWriter const& dt_writer)
	{
		using winrt::GraphPaper::implementation::write;

		D2D1_COLOR_F dummy;
		write(dummy, dt_writer);
		dt_writer.WriteDouble(m_grid_base);
		dt_writer.WriteDouble(m_grid_opac);
		write(m_grid_patt, dt_writer);
		write(m_grid_show, dt_writer);
		dt_writer.WriteBoolean(m_grid_snap);
		write(m_page_color, dt_writer);
		dt_writer.WriteDouble(m_page_scale);
		write(m_page_size, dt_writer);

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
		write(m_text_margin, dt_writer);	// ������̗]��

	}

}