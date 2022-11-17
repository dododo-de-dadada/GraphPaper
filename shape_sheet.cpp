#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;

	// �����\������.
	// sh	�\������p��
	static void sheet_draw_grid(ShapeSheet const& sh,
		const float grid_base,
		const D2D1_COLOR_F grid_color,
		const GRID_EMPH grid_emph,
		const D2D1_POINT_2F grid_offset,
		const float sh_scale,
		const D2D1_SIZE_F sh_size
	);

	// �w�肵���F�ƕs�����x���甽�ΐF�𓾂�.
	//static void get_opposite_color(const D2D1_COLOR_F& src, const double opa, D2D1_COLOR_F& dst) noexcept;

	// �w�肵���F�ƕs�����x���甽�ΐF�𓾂�.
	// src	�w�肵���F
	// opa	�w�肵���s�����x
	// dst	���ΐF
	/*
	static void get_opposite_color(const D2D1_COLOR_F& src, const double opa, D2D1_COLOR_F& dst) noexcept
	{
		dst.r = 1.0f - src.r;
		dst.g = 1.0f - src.g;
		dst.b = 1.0f - src.b;
		dst.a = opa;
		const D3DCOLORVALUE cmp{
			dst.r * opa + src.r * (1.0 - opa),
			dst.g * opa + src.g * (1.0 - opa),
			dst.b * opa + src.b * (1.0 - opa),
			1.0f
		};
		const auto gray_src = src.r * 0.3f + src.g * 0.59f + src.b * 0.11f;
		const auto gray_dst = cmp.r * 0.3f + cmp.g * 0.59f + cmp.b * 0.11f;
		if (abs(gray_src - gray_dst) < 0.5f) {
			dst.r = dst.g = dst.b = gray_src < 0.5f ? 1.0 : 0.0f;
			dst.a = opa;
		}
		return;

		dst.r = (src.r <= 0.5f ? 1.0f : 0.0f);
		dst.g = (src.g <= 0.5f ? 1.0f : 0.0f);
		dst.b = (src.b <= 0.5f ? 1.0f : 0.0f);
		dst.a = static_cast<FLOAT>(opa);
		return;

		const auto R = src.r;
		const auto G = src.g;
		const auto B = src.b;
		const auto X = max(R, max(G, B)) + min(R, min(G, B));
		const auto Y = 0.29900 * R + 0.58700 * G + 0.11400 * B;
		const auto Cb = -0.168736 * R - 0.331264 * G + 0.5 * B;
		const auto Cr = 0.5 * R - 0.418688 * G - 0.081312 * B;
		//const auto _Y = 1.0 - Y;
		const auto _Cb = Cr;
		const auto _Cr = Cb;
		//const auto _R = _Y + 1.402 * _Cr;
		//const auto _G = _Y - 0.344136 * _Cb - 0.714136 * _Cr;
		//const auto _B = _Y + 1.772 * _Cb;

		const auto _R = (X - R);
		const auto _G = (X - G);
		const auto _B = (X - B);
		const auto _Y = 0.29900 * _R + 0.58700 * _G + 0.11400 * _B;
		if (abs(_Y - Y) > 0.2) {
			dst.r = _R;
			dst.g = _G;
			dst.b = _B;
		}
		else {
			dst.r = Y < 0.5 ? 1.0 : 0.0;
			dst.g = Y < 0.5 ? 1.0 : 0.0;
			dst.b = Y < 0.5 ? 1.0 : 0.0;
		}
		dst.a = opa;
		return;

	}
	*/

	// �Ȑ��̕⏕��(����_�����Ԑ܂��)��\������.
	// sh	�\������p��
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapeSheet::draw_auxiliary_bezi(ShapeSheet const& sh, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		// �\���{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_sheet_scale);	// ���̑���
		const D2D_UI& d2d = sh.m_d2d;
		ID2D1StrokeStyle1* const a_style = Shape::m_aux_style.get();
		D2D1_POINT_2F s_pos;
		D2D1_POINT_2F e_pos;

		e_pos.x = c_pos.x;
		e_pos.y = p_pos.y;
		sh.m_color_brush->SetColor(Shape::s_background_color);
		d2d.m_d2d_context->DrawLine(p_pos, e_pos, sh.m_color_brush.get(), s_width, nullptr);
		sh.m_color_brush->SetColor(Shape::s_foreground_color);
		d2d.m_d2d_context->DrawLine(p_pos, e_pos, sh.m_color_brush.get(), s_width, a_style);
		s_pos = e_pos;
		e_pos.x = p_pos.x;
		e_pos.y = c_pos.y;
		sh.m_color_brush->SetColor(Shape::s_background_color);
		d2d.m_d2d_context->DrawLine(s_pos, e_pos, sh.m_color_brush.get(), s_width, nullptr);
		sh.m_color_brush->SetColor(Shape::s_foreground_color);
		d2d.m_d2d_context->DrawLine(s_pos, e_pos, sh.m_color_brush.get(), s_width, a_style);
		s_pos = e_pos;
		sh.m_color_brush->SetColor(Shape::s_background_color);
		d2d.m_d2d_context->DrawLine(s_pos, c_pos, sh.m_color_brush.get(), s_width, nullptr);
		sh.m_color_brush->SetColor(Shape::s_foreground_color);
		d2d.m_d2d_context->DrawLine(s_pos, c_pos, sh.m_color_brush.get(), s_width, a_style);
	}

	// ���~�̕⏕����\������.
	// sh	�\������p��
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapeSheet::draw_auxiliary_elli(ShapeSheet const& sh, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		// �\���{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_sheet_scale);	// ���̑���
		const D2D_UI& d2d = sh.m_d2d;
		D2D1_POINT_2F rect;	// ���`
		D2D1_ELLIPSE elli;		// ���~

		pt_sub(c_pos, p_pos, rect);
		pt_mul(rect, 0.5, rect);
		pt_add(p_pos, rect, elli.point);
		elli.radiusX = rect.x;
		elli.radiusY = rect.y;
		sh.m_color_brush->SetColor(Shape::s_background_color);
		d2d.m_d2d_context->DrawEllipse(elli, sh.m_color_brush.get(), s_width, nullptr);
		sh.m_color_brush->SetColor(Shape::s_foreground_color);
		d2d.m_d2d_context->DrawEllipse(elli, sh.m_color_brush.get(), s_width, Shape::m_aux_style.get());
	}

	// �����̕⏕����\������.
	// sh	�\������p��
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapeSheet::draw_auxiliary_line(ShapeSheet const& sh, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		const D2D_UI& d2d = sh.m_d2d;
		// �\���{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_sheet_scale);	// ���̑���
		sh.m_color_brush->SetColor(Shape::s_background_color);
		d2d.m_d2d_context->DrawLine(p_pos, c_pos, sh.m_color_brush.get(), s_width, nullptr);
		sh.m_color_brush->SetColor(Shape::s_foreground_color);
		d2d.m_d2d_context->DrawLine(p_pos, c_pos, sh.m_color_brush.get(), s_width, Shape::m_aux_style.get());
	}

	// ���p�`�̕⏕����\������.
	// sh	�\������p��
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	// p_opt	���p�`�̑I����
	void ShapeSheet::draw_auxiliary_poly(ShapeSheet const& sh, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos, const POLY_OPTION& p_opt)
	{
		const D2D_UI& d2d = sh.m_d2d;
		// �\���{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_sheet_scale);	// ���̑���
		D2D1_POINT_2F v_pos[MAX_N_GON];	// ���_�̔z��

		D2D1_POINT_2F p_vec;
		pt_sub(c_pos, p_pos, p_vec);
		//D2D1_POINT_2F v_vec;
		ShapePoly::create_poly_by_bbox(p_pos, p_vec, p_opt, v_pos);//, v_vec);
		const auto i_start = (p_opt.m_end_closed ? p_opt.m_vertex_cnt - 1 : 0);
		const auto j_start = (p_opt.m_end_closed ? 0 : 1);
		for (size_t i = i_start, j = j_start; j < p_opt.m_vertex_cnt; i = j++) {
			sh.m_color_brush->SetColor(Shape::s_background_color);
			d2d.m_d2d_context->DrawLine(v_pos[i], v_pos[j], sh.m_color_brush.get(), s_width, nullptr);
			sh.m_color_brush->SetColor(Shape::s_foreground_color);
			d2d.m_d2d_context->DrawLine(v_pos[i], v_pos[j], sh.m_color_brush.get(), s_width, Shape::m_aux_style.get());
		}
	}

	// ���`�̕⏕����\������.
	// sh	�\������p��
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapeSheet::draw_auxiliary_rect(ShapeSheet const& sh, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		const D2D_UI& dx = sh.m_d2d;
		// �\���{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_sheet_scale);	// ���̑���
		const D2D1_RECT_F rc = {
			p_pos.x, p_pos.y, c_pos.x, c_pos.y
		};
		sh.m_color_brush->SetColor(Shape::s_background_color);
		dx.m_d2d_context->DrawRectangle(&rc, sh.m_color_brush.get(), s_width, nullptr);
		sh.m_color_brush->SetColor(Shape::s_foreground_color);
		dx.m_d2d_context->DrawRectangle(&rc, sh.m_color_brush.get(), s_width, Shape::m_aux_style.get());
	}

	// �p�ە��`�̕⏕����\������.
	// sh	�\������p��
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	// c_rad	�p�۔��a
	void ShapeSheet::draw_auxiliary_rrect(ShapeSheet const& sh, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		const D2D_UI& dx = sh.m_d2d;

		// �\���{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_sheet_scale);	// ���̑���
		const double cx = c_pos.x;
		const double cy = c_pos.y;
		const double px = p_pos.x;
		const double py = p_pos.y;
		const double qx = cx - px;
		const double qy = cy - py;
		auto c_rad = m_corner_rad;
		double rx = c_rad.x;
		double ry = c_rad.y;

		if (qx * rx < 0.0f) {
			rx = -rx;
		}
		if (qy * ry < 0.0f) {
			ry = -ry;
		}
		const D2D1_ROUNDED_RECT r_rect = {
			{ p_pos.x, p_pos.y, c_pos.x, c_pos.y },
			static_cast<FLOAT>(rx),
			static_cast<FLOAT>(ry)
		};
		sh.m_color_brush->SetColor(Shape::s_background_color);
		dx.m_d2d_context->DrawRoundedRectangle(&r_rect, sh.m_color_brush.get(), s_width, nullptr);
		sh.m_color_brush->SetColor(Shape::s_foreground_color);
		dx.m_d2d_context->DrawRoundedRectangle(&r_rect, sh.m_color_brush.get(), s_width, Shape::m_aux_style.get());
	}

	// �}�`��\������.
	// sh	�\������p��
	void ShapeSheet::draw(ShapeSheet const& sh)
	{
		const D2D_UI& d2d = sh.m_d2d;
		// �p���F�œh��Ԃ�.
		d2d.m_d2d_context->Clear(m_sheet_color);
		if (m_grid_show == GRID_SHOW::BACK) {
			// ����̕\�����Ŕw�ʂɕ\���̏ꍇ,
			// �����\������.
			sheet_draw_grid(sh,
				m_grid_base,
				m_grid_color,
				m_grid_emph,
				m_grid_offset,
				m_sheet_scale,
				m_sheet_size);
		}
		for (auto s : m_shape_list) {
			if (!s->is_deleted()) {
				// �}�`��\������.
				s->draw(sh);
			}
		}
		if (m_grid_show == GRID_SHOW::FRONT) {
			// ����̕\�����őO�ʂɕ\���̏ꍇ,
			// �����\������.
			sheet_draw_grid(sh,
				m_grid_base,
				m_grid_color,
				m_grid_emph,
				m_grid_offset,
				m_sheet_scale,
				m_sheet_size);
		}

	}

	// �����\������.
	// d2d	�`���
	// g_offset	����̂��炵��
	static void sheet_draw_grid(ShapeSheet const& sh,
		const float grid_base,
		const D2D1_COLOR_F grid_color,
		const GRID_EMPH grid_emph,
		const D2D1_POINT_2F grid_offset,
		const float sh_scale,
		const D2D1_SIZE_F sh_size
	)
	{
		const D2D_UI& d2d = sh.m_d2d;
		const double sh_w = sh_size.width;	// �p���̕�
		const double sh_h = sh_size.height;	// �p���̍���
		// �g�傳��Ă� 1 �s�N�Z���ɂȂ�悤�g�嗦�̋t������g�̑����Ɋi�[����.
		const FLOAT grid_w = static_cast<FLOAT>(1.0 / sh_scale);	// ����̑���
		D2D1_POINT_2F h_start, h_end;	// ���̕���̊J�n�E�I���ʒu
		D2D1_POINT_2F v_start, v_end;	// �c�̕���̊J�n�E�I���ʒu
		//auto const& brush = d2d.m_color_brush.get();

		//const auto max_val = max(m_sheet_color.r, max(m_sheet_color.g, m_sheet_color.b));
		//const auto min_val = min(m_sheet_color.r, min(m_sheet_color.g, m_sheet_color.b));
		//const auto sum_val = max_val + min_val;

		sh.m_color_brush->SetColor(grid_color);
		v_start.y = 0.0f;
		h_start.x = 0.0f;
		v_end.y = sh_size.height;
		h_end.x = sh_size.width;
		const double grid_len = max(grid_base + 1.0, 1.0);

		// �����ȕ����\������.
		float w;
		double x;
		for (uint32_t i = 0; (x = round((grid_len * i + grid_offset.x) / PT_ROUND) * PT_ROUND) < sh_w; i++) {
			if (grid_emph.m_gauge_2 != 0 && (i % grid_emph.m_gauge_2) == 0) {
				w = 2.0F * grid_w;
			}
			else if (grid_emph.m_gauge_1 != 0 && (i % grid_emph.m_gauge_1) == 0) {
				w = grid_w;
			}
			else {
				w = 0.5F * grid_w;
			}
			v_start.x = v_end.x = static_cast<FLOAT>(x);
			d2d.m_d2d_context->DrawLine(v_start, v_end, sh.m_color_brush.get(), w, nullptr);
		}
		// �����ȕ����\������.
		double y;
		for (uint32_t i = 0; (y = round((grid_len * i + grid_offset.y) / PT_ROUND) * PT_ROUND) < sh_h; i++) {
			if (grid_emph.m_gauge_2 != 0 && (i % grid_emph.m_gauge_2) == 0) {
				w = 2.0F * grid_w;
			}
			else if (grid_emph.m_gauge_1 != 0 && (i % grid_emph.m_gauge_1) == 0) {
				w = grid_w;
			}
			else {
				w = 0.5F * grid_w;
			}
			h_start.y = h_end.y = static_cast<FLOAT>(y);
			d2d.m_d2d_context->DrawLine(h_start, h_end, sh.m_color_brush.get(), w, nullptr);
		}

	}

	// ��邵�̐��@�𓾂�.
	bool ShapeSheet::get_arrow_size(ARROW_SIZE& val) const noexcept
	{
		val = m_arrow_size;
		return true;
	}

	// ��邵�̌`���𓾂�.
	bool ShapeSheet::get_arrow_style(ARROW_STYLE& val) const noexcept
	{
		val = m_arrow_style;
		return true;
	}

	// �摜�̕s�����x�𓾂�.
	bool ShapeSheet::get_image_opacity(float& val) const noexcept
	{
		val = m_image_opac;
		return true;
	}

	// �p�۔��a�𓾂�.
	bool ShapeSheet::get_corner_radius(D2D1_POINT_2F& val) const noexcept
	{
		val = m_corner_rad;
		return true;
	}

	// �h��Ԃ��̐F�𓾂�.
	bool ShapeSheet::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}

	// ���̂̐F�𓾂�.
	bool ShapeSheet::get_font_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_font_color;
		return true;
	}

	// ���̖��𓾂�.
	bool ShapeSheet::get_font_family(wchar_t*& val) const noexcept
	{
		val = m_font_family;
		return true;
	}

	// ���̂̑傫���𓾂�.
	bool ShapeSheet::get_font_size(float& val) const noexcept
	{
		val = m_font_size;
		return true;
	}

	// ���̂̐L�k�𓾂�.
	bool ShapeSheet::get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept
	{
		val = m_font_stretch;
		return true;
	}

	// ���̂̎��̂𓾂�.
	bool ShapeSheet::get_font_style(DWRITE_FONT_STYLE& val) const noexcept
	{
		val = m_font_style;
		return true;
	}

	// ���̂̑����𓾂�.
	bool ShapeSheet::get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept
	{
		val = m_font_weight;
		return true;
	}

	// ����̊�̑傫���𓾂�.
	bool ShapeSheet::get_grid_base(float& val) const noexcept
	{
		val = m_grid_base;
		return true;
	}

	// ����̐F�𓾂�.
	bool ShapeSheet::get_grid_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_grid_color;
		return true;
	}

	// ����̋����𓾂�.
	bool ShapeSheet::get_grid_emph(GRID_EMPH& val) const noexcept
	{
		val = m_grid_emph;
		return true;
	}

	// ����̕\���𓾂�.
	bool ShapeSheet::get_grid_show(GRID_SHOW& val) const noexcept
	{
		val = m_grid_show;
		return true;
	}

	// ����ɍ��킹��𓾂�.
	bool ShapeSheet::get_grid_snap(bool& val) const noexcept
	{
		val = m_grid_snap;
		return true;
	}

	// �p���̐F�𓾂�.
	bool ShapeSheet::get_sheet_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_sheet_color;
		return true;
	}

	// �p���̊g�嗦�𓾂�.
	bool ShapeSheet::get_sheet_scale(float& val) const noexcept
	{
		val = m_sheet_scale;
		return true;
	}

	// �p���̐��@�𓾂�.
	bool ShapeSheet::get_sheet_size(D2D1_SIZE_F& val) const noexcept
	{
		val = m_sheet_size;
		return true;
	}

	// �[�̌`���𓾂�.
	bool ShapeSheet::get_stroke_cap(CAP_STYLE& val) const noexcept
	{
		val = m_stroke_cap;
		return true;
	}

	// ���g�̐F�𓾂�.
	bool ShapeSheet::get_stroke_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_stroke_color;
		return true;
	}

	// �j���̒[�̌`���𓾂�.
	bool ShapeSheet::get_dash_cap(D2D1_CAP_STYLE& val) const noexcept
	{
		val = m_dash_cap;
		return true;
	}

	// �j���̔z�u�𓾂�.
	bool ShapeSheet::get_dash_patt(DASH_PATT& val) const noexcept
	{
		val = m_dash_patt;
		return true;
	}

	// ���g�̌`���𓾂�.
	bool ShapeSheet::get_dash_style(D2D1_DASH_STYLE& val) const noexcept
	{
		val = m_dash_style;
		return true;
	}

	// �����̂Ȃ��̃}�C�^�[�����𓾂�.
	bool ShapeSheet::get_join_limit(float& val) const noexcept
	{
		val = m_join_limit;
		return true;
	}

	// �����̂Ȃ��𓾂�.
	bool ShapeSheet::get_join_style(D2D1_LINE_JOIN& val) const noexcept
	{
		val = m_join_style;
		return true;
	}

	// ���g�̑����𓾂�.
	bool ShapeSheet::get_stroke_width(float& val) const noexcept
	{
		val = m_stroke_width;
		return true;
	}

	// �i���̑����𓾂�.
	bool ShapeSheet::get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_p;
		return true;
	}

	// ������̂��낦�𓾂�.
	bool ShapeSheet::get_text_align_t(DWRITE_TEXT_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_t;
		return true;
	}

	// �s�Ԃ𓾂�.
	bool ShapeSheet::get_text_line_sp(float& val) const noexcept
	{
		val = m_text_line_sp;
		return true;
	}

	// ������̗]���𓾂�.
	bool ShapeSheet::get_text_padding(D2D1_SIZE_F& val) const noexcept
	{
		val = m_text_padding;
		return true;
	}

	// �f�[�^���[�_�[����ǂݍ���.
	void ShapeSheet::read(DataReader const& dt_reader)
	{
		D2D1_COLOR_F dummy;
		dt_read(dummy, dt_reader);
		m_grid_base = dt_reader.ReadSingle();
		dt_read(m_grid_color, dt_reader);
		dt_read(m_grid_emph, dt_reader);
		m_grid_show = static_cast<GRID_SHOW>(dt_reader.ReadUInt32());
		m_grid_snap = dt_reader.ReadBoolean();
		dt_read(m_sheet_color, dt_reader);
		m_sheet_scale = dt_reader.ReadSingle();
		dt_read(m_sheet_size, dt_reader);

		dt_read(m_arrow_size, dt_reader);	// ��邵�̐��@
		m_arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadUInt32());	// ��邵�̌`��
		dt_read(m_corner_rad, dt_reader);	// �p�۔��a
		dt_read(m_stroke_cap, dt_reader);	// �[�̌`��
		dt_read(m_stroke_color, dt_reader);	// ���E�g�̐F
		m_dash_cap = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());	// �j���̒[�̌`��
		dt_read(m_dash_patt, dt_reader);	// �j���̔z�u
		m_dash_style = static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32());	// �j���̌`��
		m_join_style = static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32());	// ���̂Ȃ��̌`��
		m_join_limit = dt_reader.ReadSingle();	// ���̂Ȃ��̃}�C�^�[����
		m_stroke_width = dt_reader.ReadSingle();	// ���E�g�̑���
		dt_read(m_fill_color, dt_reader);	// �h��Ԃ��̐F
		dt_read(m_font_color, dt_reader);	// ���̂̐F
		dt_read(m_font_family, dt_reader);	// ���̖�
		m_font_size = dt_reader.ReadSingle();	// ���̂̑傫��
		m_font_stretch = static_cast<DWRITE_FONT_STRETCH>(dt_reader.ReadUInt32());	// ���̂̐L�k
		m_font_style = static_cast<DWRITE_FONT_STYLE>(dt_reader.ReadUInt32());	// ���̂̎���
		m_font_weight = static_cast<DWRITE_FONT_WEIGHT>(dt_reader.ReadUInt32());	// ���̂̑���
		m_text_align_p = static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(dt_reader.ReadUInt32());	// �i���̂��낦
		m_text_align_t = static_cast<DWRITE_TEXT_ALIGNMENT>(dt_reader.ReadUInt32());	// ������̂��낦
		m_text_line_sp = dt_reader.ReadSingle();	// �s��
		dt_read(m_text_padding, dt_reader);	// ������̗]��
		m_image_opac = dt_reader.ReadSingle();	// �摜�̕s������

		ShapeText::is_available_font(m_font_family);
	}

	// �l���邵�̐��@�Ɋi�[����.
	bool ShapeSheet::set_arrow_size(const ARROW_SIZE& val) noexcept
	{
		if (!equal(m_arrow_size, val)) {
			m_arrow_size = val;
			return true;
		}
		return false;
	}

	// �l���邵�̌`���Ɋi�[����.
	bool ShapeSheet::set_arrow_style(const ARROW_STYLE val) noexcept
	{
		const auto old_val = m_arrow_style;
		return (m_arrow_style = val) != old_val;
	}

	// �l���摜�̕s�����x�Ɋi�[����.
	bool ShapeSheet::set_image_opacity(const float val) noexcept
	{
		if (!equal(m_image_opac, val)) {
			m_image_opac = val;
			return true;
		}
		return false;
	}

	// �l���p�۔��a�Ɋi�[����.
	bool ShapeSheet::set_corner_radius(const D2D1_POINT_2F& val) noexcept
	{
		if (!equal(m_corner_rad, val)) {
			m_corner_rad = val;
			return true;
		}
		return false;
	}

	// �l��h��Ԃ��̐F�Ɋi�[����.
	bool ShapeSheet::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_fill_color, val)) {
			m_fill_color = val;
			return true;
		}
		return false;
	}

	// �l�����̂̐F�Ɋi�[����.
	bool ShapeSheet::set_font_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_font_color, val)) {
			m_font_color = val;
			return true;
		}
		return false;
	}

	// �l�����̖��Ɋi�[����.
	bool ShapeSheet::set_font_family(wchar_t* const val) noexcept
	{
		if (!equal(m_font_family, val)) {
			m_font_family = val;
			return true;
		}
		return false;
	}

	// �l�����̂̑傫���Ɋi�[����.
	bool ShapeSheet::set_font_size(const float val) noexcept
	{
		if (!equal(m_font_size, val)) {
			m_font_size = val;
			return true;
		}
		return false;
	}

	// �l�����̂̐L�k�Ɋi�[����.
	bool ShapeSheet::set_font_stretch(const DWRITE_FONT_STRETCH val) noexcept
	{
		const auto old_val = m_font_stretch;
		return (m_font_stretch = val) != old_val;
	}

	// ���̂̎��̂Ɋi�[����.
	bool ShapeSheet::set_font_style(const DWRITE_FONT_STYLE val) noexcept
	{
		const auto old_val = m_font_style;
		return (m_font_style = val) != old_val;
	}

	// �l�����̂̑����Ɋi�[����.
	bool ShapeSheet::set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept
	{
		const auto old_val = m_font_weight;
		return (m_font_weight = val) != old_val;
	}

	// �l�����̊�̑傫���Ɋi�[����.
	bool ShapeSheet::set_grid_base(const float val) noexcept
	{
		if (!equal(m_grid_base, val)) {
			m_grid_base = val;
			return true;
		}
		return false;
	}

	// �l�����̔Z�W�Ɋi�[����.
	bool ShapeSheet::set_grid_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_grid_color, val)) {
			m_grid_color = val;
			return true;
		}
		return false;
	}

	// �l�����̋����Ɋi�[����.
	bool ShapeSheet::set_grid_emph(const GRID_EMPH& val) noexcept
	{
		if (!equal(m_grid_emph, val)) {
			m_grid_emph = val;
			return true;
		}
		return false;
	}

	// �l�����̕\���Ɋi�[����.
	bool ShapeSheet::set_grid_show(const GRID_SHOW val) noexcept
	{
		const auto old_val = m_grid_show;
		return (m_grid_show = val) != old_val;
	}

	// �l�����ɍ��킹��Ɋi�[����.
	bool ShapeSheet::set_grid_snap(const bool val) noexcept
	{
		const auto old_val = m_grid_snap;
		return (m_grid_snap = val) != old_val;
	}

	// �l��, �p��, ����, �⏕���̊e�F�Ɋi�[����
	bool ShapeSheet::set_sheet_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_sheet_color, val)) {
			m_sheet_color = val;
			m_sheet_color.a = 1.0f;
			return true;
		}
		return false;
	}

	// �l��p���̊g�嗦�Ɋi�[����.
	bool ShapeSheet::set_sheet_scale(const float val) noexcept
	{
		if (!equal(m_sheet_scale,val)) {
			m_sheet_scale = val;
			return true;
		}
		return false;
	}

	// �l��p���̐��@�Ɋi�[����.
	bool ShapeSheet::set_sheet_size(const D2D1_SIZE_F val) noexcept
	{
		if (!equal(m_sheet_size, val)) {
			m_sheet_size = val;
			return true;
		}
		return false;
	}

	// �l��[�̌`���Ɋi�[����.
	bool ShapeSheet::set_stroke_cap(const CAP_STYLE& val) noexcept
	{
		if (!equal(m_stroke_cap, val)) {
			m_stroke_cap = val;
			return true;
		}
		return false;
	}

	// ���g�̐F�Ɋi�[����.
	bool ShapeSheet::set_stroke_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_stroke_color, val)) {
			m_stroke_color = val;
			return true;
		}
		return false;
	}

	// �j���̒[�̌`���Ɋi�[����.
	bool ShapeSheet::set_dash_cap(const D2D1_CAP_STYLE& val) noexcept
	{
		const auto old_val = m_dash_cap;
		return (m_dash_cap = val) != old_val;
	}

	// �j���̔z�u�Ɋi�[����.
	bool ShapeSheet::set_dash_patt(const DASH_PATT& val) noexcept
	{
		if (!equal(m_dash_patt, val)) {
			m_dash_patt = val;
			return true;
		}
		return false;
	}

	// ���g�̌`���Ɋi�[����.
	bool ShapeSheet::set_dash_style(const D2D1_DASH_STYLE val) noexcept
	{
		const auto old_val = m_dash_style;
		return (m_dash_style = val) != old_val;
	}

	// �l������̂Ȃ��̃}�C�^�[�����Ɋi�[����.
	bool ShapeSheet::set_join_limit(const float& val) noexcept
	{
		if (!equal(m_join_limit, val)) {
			m_join_limit = val;
			return true;
		}
		return false;
	}

	// �l������̂Ȃ��Ɋi�[����.
	bool ShapeSheet::set_join_style(const D2D1_LINE_JOIN& val) noexcept
	{
		const auto old_val = m_join_style;
		return (m_join_style = val) != old_val;
	}

	// ���g�̑����Ɋi�[����.
	bool ShapeSheet::set_stroke_width(const float val) noexcept
	{
		if (!equal(m_stroke_width, val)) {
			m_stroke_width = val;
			return true;
		}
		return false;
	}

	// �l��i���̂��낦�Ɋi�[����.
	bool ShapeSheet::set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept
	{
		const auto old_val = m_text_align_p;
		return (m_text_align_p = val) != old_val;
	}

	// ������̂��낦�Ɋi�[����.
	bool ShapeSheet::set_text_align_t(const DWRITE_TEXT_ALIGNMENT val) noexcept
	{
		const auto old_val = m_text_align_t;
		return (m_text_align_t = val) != old_val;
	}

	// �l���s�ԂɊi�[����.
	bool ShapeSheet::set_text_line_sp(const float val) noexcept
	{
		if (!equal(m_text_line_sp, val)) {
			m_text_line_sp = val;
			return true;
		}
		return false;
	}

	// �l�𕶎���̗]���Ɋi�[����.
	bool ShapeSheet::set_text_padding(const D2D1_SIZE_F val) noexcept
	{
		if (!equal(m_text_padding, val)) {
			m_text_padding = val;
			return true;
		}
		return false;
	}

	// �}�`�̑����l��p���Ɋi�[����.
	// s	�}�`
	void ShapeSheet::set_attr_to(const Shape* s) noexcept
	{
		s->get_arrow_size(m_arrow_size);
		s->get_arrow_style(m_arrow_style);
		s->get_dash_cap(m_dash_cap);
		s->get_dash_patt(m_dash_patt);
		s->get_dash_style(m_dash_style);
		s->get_corner_radius(m_corner_rad);
		s->get_fill_color(m_fill_color);
		s->get_font_color(m_font_color);
		s->get_font_family(m_font_family);
		s->get_font_size(m_font_size);
		s->get_font_stretch(m_font_stretch);
		s->get_font_style(m_font_style);
		s->get_font_weight(m_font_weight);
		s->get_grid_base(m_grid_base);
		s->get_grid_color(m_grid_color);
		s->get_grid_emph(m_grid_emph);
		s->get_grid_show(m_grid_show);
		s->get_grid_snap(m_grid_snap);
		s->get_image_opacity(m_image_opac);
		s->get_join_limit(m_join_limit);
		s->get_join_style(m_join_style);
		s->get_sheet_color(m_sheet_color);
		s->get_stroke_cap(m_stroke_cap);
		s->get_stroke_color(m_stroke_color);
		s->get_stroke_width(m_stroke_width);
		s->get_text_align_t(m_text_align_t);
		s->get_text_align_p(m_text_align_p);
		s->get_text_line_sp(m_text_line_sp);
		s->get_text_padding(m_text_padding);
	}

	// �f�[�^���[�_�[�ɏ�������.
	// dt_writer	�f�[�^���[�_�[
	void ShapeSheet::write(DataWriter const& dt_writer)
	{
		D2D1_COLOR_F dummy;
		dt_write(dummy, dt_writer);
		dt_writer.WriteSingle(m_grid_base);
		dt_write(m_grid_color, dt_writer);
		dt_write(m_grid_emph, dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_grid_show));
		dt_writer.WriteBoolean(m_grid_snap);
		dt_write(m_sheet_color, dt_writer);
		dt_writer.WriteSingle(m_sheet_scale);
		dt_write(m_sheet_size, dt_writer);

		dt_write(m_arrow_size, dt_writer);	// ��邵�̐��@
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));	// ��邵�̌`��
		dt_write(m_corner_rad, dt_writer);	// �p�۔��a
		dt_write(m_stroke_cap, dt_writer);	// �[�̌`��
		dt_write(m_stroke_color, dt_writer);	// ���g�̐F
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_dash_cap));	// �j���̒[�̌`��
		dt_write(m_dash_patt, dt_writer);	// �j���̔z�u
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_dash_style));	// ���g�̌`��
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_join_style));	// �����̂Ȃ�
		dt_writer.WriteSingle(m_join_limit);	// �����̃}�C�^�[����
		dt_writer.WriteSingle(m_stroke_width);	// ���g�̑���
		dt_write(m_fill_color, dt_writer);	// �h��Ԃ��̐F
		dt_write(m_font_color, dt_writer);	// ���̂̐F
		dt_write(m_font_family, dt_writer);	// ���̖�
		dt_writer.WriteSingle(m_font_size);	// ���̂̑傫��
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_stretch));	// ���̂̐L�k
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_style));	// ���̂̎���
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_weight));	// ���̂̑���
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_p));	// �i���̂��낦
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_t));	// ������̂��낦
		dt_writer.WriteSingle(m_text_line_sp);	// �s��
		dt_write(m_text_padding, dt_writer);	// ������̗]��
		dt_writer.WriteSingle(m_image_opac);	// �摜�̕s������
	}

}