#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	// �����\������.
	static void page_draw_grid(
		ID2D1RenderTarget* const target,
		ID2D1SolidColorBrush* const brush,
		const float grid_base,
		const D2D1_COLOR_F grid_color,
		const GRID_EMPH grid_emph,
		const D2D1_POINT_2F grid_offset,
		const float page_scale,
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
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapePage::draw_auxiliary_bezi(
		ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
		const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		// �y�[�W�̔{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_page_scale);	// ���̑���
		ID2D1StrokeStyle1* const a_style = Shape::m_aux_style.get();
		D2D1_POINT_2F s_pos;
		D2D1_POINT_2F e_pos;

		e_pos.x = c_pos.x;
		e_pos.y = p_pos.y;
		brush->SetColor(Shape::s_background_color);
		target->DrawLine(p_pos, e_pos, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawLine(p_pos, e_pos, brush, s_width, a_style);
		s_pos = e_pos;
		e_pos.x = p_pos.x;
		e_pos.y = c_pos.y;
		brush->SetColor(Shape::s_background_color);
		target->DrawLine(s_pos, e_pos, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawLine(s_pos, e_pos, brush, s_width, a_style);
		s_pos = e_pos;
		brush->SetColor(Shape::s_background_color);
		target->DrawLine(s_pos, c_pos, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawLine(s_pos, c_pos, brush, s_width, a_style);
	}

	// ���~�̕⏕����\������.
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapePage::draw_auxiliary_elli(
		ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
		const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		// �y�[�W�̔{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_page_scale);	// ���̑���
		//const D2D_UI& d2d = sh.m_d2d;
		D2D1_POINT_2F rect;	// ���`
		D2D1_ELLIPSE elli;		// ���~

		pt_sub(c_pos, p_pos, rect);
		pt_mul(rect, 0.5, rect);
		pt_add(p_pos, rect, elli.point);
		elli.radiusX = rect.x;
		elli.radiusY = rect.y;
		brush->SetColor(Shape::s_background_color);
		target->DrawEllipse(elli, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawEllipse(elli, brush, s_width, Shape::m_aux_style.get());
	}

	// �����̕⏕����\������.
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapePage::draw_auxiliary_line(
		ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
		const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		// �y�[�W�̔{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_page_scale);	// ���̑���
		brush->SetColor(Shape::s_background_color);
		target->DrawLine(p_pos, c_pos, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawLine(p_pos, c_pos, brush, s_width, Shape::m_aux_style.get());
	}

	// ���p�`�̕⏕����\������.
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	// p_opt	���p�`�̑I����
	void ShapePage::draw_auxiliary_poly(
		ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
		const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos, const POLY_OPTION& p_opt)
	{
		// �y�[�W�̔{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_page_scale);	// ���̑���
		D2D1_POINT_2F v_pos[MAX_N_GON];	// ���_�̔z��

		D2D1_POINT_2F p_vec;
		pt_sub(c_pos, p_pos, p_vec);
		//D2D1_POINT_2F v_vec;
		ShapePoly::poly_by_bbox(p_pos, p_vec, p_opt, v_pos);
		const auto i_start = (p_opt.m_end_closed ? p_opt.m_vertex_cnt - 1 : 0);
		const auto j_start = (p_opt.m_end_closed ? 0 : 1);
		for (size_t i = i_start, j = j_start; j < p_opt.m_vertex_cnt; i = j++) {
			brush->SetColor(Shape::s_background_color);
			target->DrawLine(v_pos[i], v_pos[j], brush, s_width, nullptr);
			brush->SetColor(Shape::s_foreground_color);
			target->DrawLine(v_pos[i], v_pos[j], brush, s_width, Shape::m_aux_style.get());
		}
	}

	// ���`�̕⏕����\������.
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	void ShapePage::draw_auxiliary_rect(
		ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
		const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		// �y�[�W�̔{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_page_scale);	// ���̑���
		const D2D1_RECT_F rc = {
			p_pos.x, p_pos.y, c_pos.x, c_pos.y
		};
		brush->SetColor(Shape::s_background_color);
		target->DrawRectangle(&rc, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawRectangle(&rc, brush, s_width, Shape::m_aux_style.get());
	}

	// �p�ە��`�̕⏕����\������.
	// p_pos	�|�C���^�[�������ꂽ�ʒu
	// c_pos	�|�C���^�[�̌��݈ʒu
	// c_rad	�p�۔��a
	void ShapePage::draw_auxiliary_rrect(
		ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
		const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		// �y�[�W�̔{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_page_scale);	// ���̑���
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
		brush->SetColor(Shape::s_background_color);
		target->DrawRoundedRectangle(&r_rect, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawRoundedRectangle(&r_rect, brush, s_width, Shape::m_aux_style.get());
	}

	// �}�`��\������.
	void ShapePage::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::s_target;
		ID2D1SolidColorBrush* const brush = Shape::s_color_brush;

		// �y�[�W�̐F�œh��Ԃ�.
		target->Clear(m_page_color);
		if (m_grid_show == GRID_SHOW::BACK) {
			// ����̕\�����Ŕw�ʂɕ\���̏ꍇ,
			// �����\������.
			page_draw_grid(
				target,
				brush,
				m_grid_base,
				m_grid_color,
				m_grid_emph,
				m_grid_offset,
				m_page_scale,
				m_page_size);
		}
		for (auto s : m_shape_list) {
			if (!s->is_deleted()) {
				// �}�`��\������.
				s->draw();
			}
		}
		if (m_grid_show == GRID_SHOW::FRONT) {
			// ����̕\�����őO�ʂɕ\���̏ꍇ,
			// �����\������.
			page_draw_grid(
				target,
				brush,
				m_grid_base,
				m_grid_color,
				m_grid_emph,
				m_grid_offset,
				m_page_scale,
				m_page_size);
		}

	}

	// �����\������.
	// d2d	�`���
	// g_offset	����̂��炵��
	static void page_draw_grid(
		ID2D1RenderTarget* const target,
		ID2D1SolidColorBrush* const brush,
		const float grid_base,
		const D2D1_COLOR_F grid_color,
		const GRID_EMPH grid_emph,
		const D2D1_POINT_2F grid_offset,
		const float page_scale,
		const D2D1_SIZE_F page_size
	)
	{
		// �g�傳��Ă� 1 �s�N�Z���ɂȂ�悤�g�嗦�̋t������g�̑����Ɋi�[����.
		const FLOAT grid_w = static_cast<FLOAT>(1.0 / page_scale);	// ����̑���
		D2D1_POINT_2F h_start, h_end;	// ���̕���̊J�n�E�I���ʒu
		D2D1_POINT_2F v_start, v_end;	// �c�̕���̊J�n�E�I���ʒu
		brush->SetColor(grid_color);
		v_start.y = 0.0f;
		h_start.x = 0.0f;
		const auto page_h = page_size.height;
		const auto page_w = page_size.width;
		v_end.y = page_size.height;
		h_end.x = page_size.width;
		const double grid_len = max(grid_base + 1.0, 1.0);

		// �����ȕ����\������.
		float w;
		double x;
		for (uint32_t i = 0; (x = round((grid_len * i + grid_offset.x) / PT_ROUND) * PT_ROUND) < page_w; i++) {
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
			target->DrawLine(v_start, v_end, brush, w, nullptr);
		}
		// �����ȕ����\������.
		double y;
		for (uint32_t i = 0; (y = round((grid_len * i + grid_offset.y) / PT_ROUND) * PT_ROUND) < page_h; i++) {
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
			target->DrawLine(h_start, h_end, brush, w, nullptr);
		}

	}

	// ��邵�̐��@�𓾂�.
	bool ShapePage::get_arrow_size(ARROW_SIZE& val) const noexcept
	{
		val = m_arrow_size;
		return true;
	}

	// ��邵�̌`���𓾂�.
	bool ShapePage::get_arrow_style(ARROW_STYLE& val) const noexcept
	{
		val = m_arrow_style;
		return true;
	}

	// �摜�̕s�����x�𓾂�.
	bool ShapePage::get_image_opacity(float& val) const noexcept
	{
		val = m_image_opac;
		return true;
	}

	// �p�۔��a�𓾂�.
	bool ShapePage::get_corner_radius(D2D1_POINT_2F& val) const noexcept
	{
		val = m_corner_rad;
		return true;
	}

	// �h��Ԃ��̐F�𓾂�.
	bool ShapePage::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}

	// ���̂̐F�𓾂�.
	bool ShapePage::get_font_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_font_color;
		return true;
	}

	// ���̖��𓾂�.
	bool ShapePage::get_font_family(wchar_t*& val) const noexcept
	{
		val = m_font_family;
		return true;
	}

	// ���̂̑傫���𓾂�.
	bool ShapePage::get_font_size(float& val) const noexcept
	{
		val = m_font_size;
		return true;
	}

	// ���̂̕��̐L�k�𓾂�.
	bool ShapePage::get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept
	{
		val = m_font_stretch;
		return true;
	}

	// ���̂̎��̂𓾂�.
	bool ShapePage::get_font_style(DWRITE_FONT_STYLE& val) const noexcept
	{
		val = m_font_style;
		return true;
	}

	// ���̂̑����𓾂�.
	bool ShapePage::get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept
	{
		val = m_font_weight;
		return true;
	}

	// ����̊�̑傫���𓾂�.
	bool ShapePage::get_grid_base(float& val) const noexcept
	{
		val = m_grid_base;
		return true;
	}

	// ����̐F�𓾂�.
	bool ShapePage::get_grid_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_grid_color;
		return true;
	}

	// ����̋����𓾂�.
	bool ShapePage::get_grid_emph(GRID_EMPH& val) const noexcept
	{
		val = m_grid_emph;
		return true;
	}

	// ����̕\���𓾂�.
	bool ShapePage::get_grid_show(GRID_SHOW& val) const noexcept
	{
		val = m_grid_show;
		return true;
	}

	// ����ɍ��킹��𓾂�.
	bool ShapePage::get_grid_snap(bool& val) const noexcept
	{
		val = m_grid_snap;
		return true;
	}

	// �y�[�W�̐F�𓾂�.
	bool ShapePage::get_page_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_page_color;
		return true;
	}

	// �y�[�W�{���𓾂�.
	bool ShapePage::get_page_scale(float& val) const noexcept
	{
		val = m_page_scale;
		return true;
	}

	// �y�[�W�̑傫���𓾂�.
	bool ShapePage::get_page_size(D2D1_SIZE_F& val) const noexcept
	{
		val = m_page_size;
		return true;
	}

	// �[�̌`���𓾂�.
	bool ShapePage::get_stroke_cap(CAP_STYLE& val) const noexcept
	{
		val = m_stroke_cap;
		return true;
	}

	// ���g�̐F�𓾂�.
	bool ShapePage::get_stroke_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_stroke_color;
		return true;
	}

	// �j���̒[�̌`���𓾂�.
	bool ShapePage::get_dash_cap(D2D1_CAP_STYLE& val) const noexcept
	{
		val = m_dash_cap;
		return true;
	}

	// �j���̔z�u�𓾂�.
	bool ShapePage::get_dash_patt(DASH_PATT& val) const noexcept
	{
		val = m_dash_patt;
		return true;
	}

	// ���g�̌`���𓾂�.
	bool ShapePage::get_dash_style(D2D1_DASH_STYLE& val) const noexcept
	{
		val = m_dash_style;
		return true;
	}

	// �����̌����̃}�C�^�[�����𓾂�.
	bool ShapePage::get_join_miter_limit(float& val) const noexcept
	{
		val = m_join_miter_limit;
		return true;
	}

	// �����̌����𓾂�.
	bool ShapePage::get_join_style(D2D1_LINE_JOIN& val) const noexcept
	{
		val = m_join_style;
		return true;
	}

	// ���g�̑����𓾂�.
	bool ShapePage::get_stroke_width(float& val) const noexcept
	{
		val = m_stroke_width;
		return true;
	}

	// �i���̑����𓾂�.
	bool ShapePage::get_text_par_align(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept
	{
		val = m_text_par_align;
		return true;
	}

	// ������̂��낦�𓾂�.
	bool ShapePage::get_text_align_t(DWRITE_TEXT_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_t;
		return true;
	}

	// �s�Ԃ𓾂�.
	bool ShapePage::get_text_line_sp(float& val) const noexcept
	{
		val = m_text_line_sp;
		return true;
	}

	// ������̗]���𓾂�.
	bool ShapePage::get_text_padding(D2D1_SIZE_F& val) const noexcept
	{
		val = m_text_padding;
		return true;
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	void ShapePage::read(DataReader const& dt_reader)
	{
		// ����̑傫��
		const auto grid_base = dt_reader.ReadSingle();
		m_grid_base = max(grid_base, 0.0f);
		// ����̐F
		const D2D1_COLOR_F grid_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		m_grid_color.r = min(max(grid_color.r, 0.0f), 1.0f);
		m_grid_color.g = min(max(grid_color.g, 0.0f), 1.0f);
		m_grid_color.b = min(max(grid_color.b, 0.0f), 1.0f);
		m_grid_color.a = min(max(grid_color.a, 0.0f), 1.0f);
		// ����̋���
		m_grid_emph = GRID_EMPH{
			dt_reader.ReadUInt32(),
			dt_reader.ReadUInt32()
		};
		// ����̕\��
		m_grid_show = static_cast<GRID_SHOW>(dt_reader.ReadUInt32());
		// ����ɍ��킹��.
		m_grid_snap = dt_reader.ReadBoolean();
		// �y�[�W�̐F
		const D2D1_COLOR_F page_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		m_page_color.r = min(max(page_color.r, 0.0f), 1.0f);
		m_page_color.g = min(max(page_color.g, 0.0f), 1.0f);
		m_page_color.b = min(max(page_color.b, 0.0f), 1.0f);
		m_page_color.a = min(max(page_color.a, 0.0f), 1.0f);
		// �y�[�W�̔{��
		const auto page_scale = dt_reader.ReadSingle();
		m_page_scale = max(min(page_scale, 4.0f), 0.25f);
		// �y�[�W�̑傫��
		const D2D1_SIZE_F page_size{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		m_page_size.width = max(min(page_size.width, PAGE_SIZE_MAX), 1.0f);
		m_page_size.height = max(min(page_size.height, PAGE_SIZE_MAX), 1.0f);
		// ��邵�̐��@
		m_arrow_size = ARROW_SIZE{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		// ��邵�̌`��
		m_arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadUInt32());
		// �p�۔��a
		m_corner_rad = D2D1_POINT_2F{
			dt_reader.ReadSingle(), dt_reader.ReadSingle()
		};
		// �[�̌`��
		m_stroke_cap = CAP_STYLE{
			static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32()),
			static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32())
		};
		// ���E�g�̐F
		const D2D1_COLOR_F stroke_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		m_stroke_color.r = min(max(stroke_color.r, 0.0f), 1.0f);
		m_stroke_color.g = min(max(stroke_color.g, 0.0f), 1.0f);
		m_stroke_color.b = min(max(stroke_color.b, 0.0f), 1.0f);
		m_stroke_color.a = min(max(stroke_color.a, 0.0f), 1.0f);
		// �j���̒[�̌`��
		m_dash_cap = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
		// �j���̔z�u
		m_dash_patt = DASH_PATT{
			{
				dt_reader.ReadSingle(), dt_reader.ReadSingle(),
				dt_reader.ReadSingle(), dt_reader.ReadSingle(),
				dt_reader.ReadSingle(), dt_reader.ReadSingle()
			}
		};
		// �j���̌`��
		m_dash_style = static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32());
		// ���̌����̌`��
		m_join_style = static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32());
		// ���̌����̃}�C�^�[��������
		m_join_miter_limit = dt_reader.ReadSingle();
		// ���E�g�̑���
		m_stroke_width = dt_reader.ReadSingle();
		// �h��Ԃ��̐F
		const D2D1_COLOR_F fill_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		m_fill_color.r = min(max(fill_color.r, 0.0f), 1.0f);
		m_fill_color.g = min(max(fill_color.g, 0.0f), 1.0f);
		m_fill_color.b = min(max(fill_color.b, 0.0f), 1.0f);
		m_fill_color.a = min(max(fill_color.a, 0.0f), 1.0f);
		// ���̂̐F
		const D2D1_COLOR_F font_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		m_font_color.r = min(max(font_color.r, 0.0f), 1.0f);
		m_font_color.g = min(max(font_color.g, 0.0f), 1.0f);
		m_font_color.b = min(max(font_color.b, 0.0f), 1.0f);
		m_font_color.a = min(max(font_color.a, 0.0f), 1.0f);
		// ���̖�
		const size_t font_family_len = dt_reader.ReadUInt32();
		uint8_t* font_family_data = new uint8_t[2 * (font_family_len + 1)];
		dt_reader.ReadBytes(array_view(font_family_data, font_family_data + 2 * font_family_len));
		m_font_family = reinterpret_cast<wchar_t*>(font_family_data);
		m_font_family[font_family_len] = L'\0';
		// ���̂̑傫��
		m_font_size = dt_reader.ReadSingle();
		// ���̂̕��̐L�k
		m_font_stretch = static_cast<DWRITE_FONT_STRETCH>(dt_reader.ReadUInt32());
		// ���̂̎���
		m_font_style = static_cast<DWRITE_FONT_STYLE>(dt_reader.ReadUInt32());
		// ���̂̑���
		m_font_weight = static_cast<DWRITE_FONT_WEIGHT>(dt_reader.ReadUInt32());
		// �i���̂��낦
		m_text_par_align = static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(dt_reader.ReadUInt32());
		// ������̂��낦
		m_text_align_t = static_cast<DWRITE_TEXT_ALIGNMENT>(dt_reader.ReadUInt32());
		// �s��
		m_text_line_sp = dt_reader.ReadSingle();
		// ������̗]��
		m_text_padding = D2D1_SIZE_F{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		// �摜�̕s������
		m_image_opac = dt_reader.ReadSingle();

		ShapeText::is_available_font(m_font_family);
	}

	// �l���邵�̐��@�Ɋi�[����.
	bool ShapePage::set_arrow_size(const ARROW_SIZE& val) noexcept
	{
		if (!equal(m_arrow_size, val)) {
			m_arrow_size = val;
			return true;
		}
		return false;
	}

	// �l���邵�̌`���Ɋi�[����.
	bool ShapePage::set_arrow_style(const ARROW_STYLE val) noexcept
	{
		const auto old_val = m_arrow_style;
		return (m_arrow_style = val) != old_val;
	}

	// �l���摜�̕s�����x�Ɋi�[����.
	bool ShapePage::set_image_opacity(const float val) noexcept
	{
		if (!equal(m_image_opac, val)) {
			m_image_opac = val;
			return true;
		}
		return false;
	}

	// �l���p�۔��a�Ɋi�[����.
	bool ShapePage::set_corner_radius(const D2D1_POINT_2F& val) noexcept
	{
		if (!equal(m_corner_rad, val)) {
			m_corner_rad = val;
			return true;
		}
		return false;
	}

	// �l��h��Ԃ��̐F�Ɋi�[����.
	bool ShapePage::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_fill_color, val)) {
			m_fill_color = val;
			return true;
		}
		return false;
	}

	// �l�����̂̐F�Ɋi�[����.
	bool ShapePage::set_font_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_font_color, val)) {
			m_font_color = val;
			return true;
		}
		return false;
	}

	// �l�����̖��Ɋi�[����.
	bool ShapePage::set_font_family(wchar_t* const val) noexcept
	{
		if (!equal(m_font_family, val)) {
			m_font_family = val;
			return true;
		}
		return false;
	}

	// �l�����̂̑傫���Ɋi�[����.
	bool ShapePage::set_font_size(const float val) noexcept
	{
		if (!equal(m_font_size, val)) {
			m_font_size = val;
			return true;
		}
		return false;
	}

	// �l�����̂̕��̐L�k�Ɋi�[����.
	bool ShapePage::set_font_stretch(const DWRITE_FONT_STRETCH val) noexcept
	{
		const auto old_val = m_font_stretch;
		return (m_font_stretch = val) != old_val;
	}

	// ���̂̎��̂Ɋi�[����.
	bool ShapePage::set_font_style(const DWRITE_FONT_STYLE val) noexcept
	{
		const auto old_val = m_font_style;
		return (m_font_style = val) != old_val;
	}

	// �l�����̂̑����Ɋi�[����.
	bool ShapePage::set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept
	{
		const auto old_val = m_font_weight;
		return (m_font_weight = val) != old_val;
	}

	// �l�����̊�̑傫���Ɋi�[����.
	bool ShapePage::set_grid_base(const float val) noexcept
	{
		if (!equal(m_grid_base, val)) {
			m_grid_base = val;
			return true;
		}
		return false;
	}

	// �l�����̔Z�W�Ɋi�[����.
	bool ShapePage::set_grid_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_grid_color, val)) {
			m_grid_color = val;
			return true;
		}
		return false;
	}

	// �l�����̋����Ɋi�[����.
	bool ShapePage::set_grid_emph(const GRID_EMPH& val) noexcept
	{
		if (!equal(m_grid_emph, val)) {
			m_grid_emph = val;
			return true;
		}
		return false;
	}

	// �l�����̕\���Ɋi�[����.
	bool ShapePage::set_grid_show(const GRID_SHOW val) noexcept
	{
		if (m_grid_show != val) {
			m_grid_show = val;
			return true;
		}
		return false;
	}

	// �l�����ɍ��킹��Ɋi�[����.
	bool ShapePage::set_grid_snap(const bool val) noexcept
	{
		if (m_grid_snap != val) {
			m_grid_snap = val;
			return true;
		}
		return false;
	}

	// �l��, �\��, ����, �⏕���̊e�F�Ɋi�[����
	bool ShapePage::set_page_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_page_color, val)) {
			m_page_color = val;
			m_page_color.a = 1.0f;
			return true;
		}
		return false;
	}

	// �l���y�[�W�̔{���Ɋi�[����.
	bool ShapePage::set_page_scale(const float val) noexcept
	{
		if (!equal(m_page_scale,val)) {
			m_page_scale = val;
			return true;
		}
		return false;
	}

	// �l��\���̑傫���Ɋi�[����.
	bool ShapePage::set_page_size(const D2D1_SIZE_F val) noexcept
	{
		if (!equal(m_page_size, val)) {
			m_page_size = val;
			return true;
		}
		return false;
	}

	// �l��[�̌`���Ɋi�[����.
	bool ShapePage::set_stroke_cap(const CAP_STYLE& val) noexcept
	{
		if (!equal(m_stroke_cap, val)) {
			m_stroke_cap = val;
			return true;
		}
		return false;
	}

	// ���g�̐F�Ɋi�[����.
	bool ShapePage::set_stroke_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_stroke_color, val)) {
			m_stroke_color = val;
			return true;
		}
		return false;
	}

	// �j���̒[�̌`���Ɋi�[����.
	bool ShapePage::set_dash_cap(const D2D1_CAP_STYLE& val) noexcept
	{
		const auto old_val = m_dash_cap;
		return (m_dash_cap = val) != old_val;
	}

	// �j���̔z�u�Ɋi�[����.
	bool ShapePage::set_dash_patt(const DASH_PATT& val) noexcept
	{
		if (!equal(m_dash_patt, val)) {
			m_dash_patt = val;
			return true;
		}
		return false;
	}

	// ���g�̌`���Ɋi�[����.
	bool ShapePage::set_dash_style(const D2D1_DASH_STYLE val) noexcept
	{
		const auto old_val = m_dash_style;
		return (m_dash_style = val) != old_val;
	}

	// �l������̌����̃}�C�^�[�����Ɋi�[����.
	bool ShapePage::set_join_miter_limit(const float& val) noexcept
	{
		if (!equal(m_join_miter_limit, val)) {
			m_join_miter_limit = val;
			return true;
		}
		return false;
	}

	// �l������̌����Ɋi�[����.
	bool ShapePage::set_join_style(const D2D1_LINE_JOIN& val) noexcept
	{
		const auto old_val = m_join_style;
		return (m_join_style = val) != old_val;
	}

	// ���g�̑����Ɋi�[����.
	bool ShapePage::set_stroke_width(const float val) noexcept
	{
		if (!equal(m_stroke_width, val)) {
			m_stroke_width = val;
			return true;
		}
		return false;
	}

	// �l��i���̂��낦�Ɋi�[����.
	bool ShapePage::set_text_par_align(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept
	{
		const auto old_val = m_text_par_align;
		return (m_text_par_align = val) != old_val;
	}

	// ������̂��낦�Ɋi�[����.
	bool ShapePage::set_text_align_t(const DWRITE_TEXT_ALIGNMENT val) noexcept
	{
		const auto old_val = m_text_align_t;
		return (m_text_align_t = val) != old_val;
	}

	// �l���s�ԂɊi�[����.
	bool ShapePage::set_text_line_sp(const float val) noexcept
	{
		if (!equal(m_text_line_sp, val)) {
			m_text_line_sp = val;
			return true;
		}
		return false;
	}

	// �l�𕶎���̗]���Ɋi�[����.
	bool ShapePage::set_text_padding(const D2D1_SIZE_F val) noexcept
	{
		if (!equal(m_text_padding, val)) {
			m_text_padding = val;
			return true;
		}
		return false;
	}

	// �}�`�̑����l��\���Ɋi�[����.
	// s	�}�`
	void ShapePage::set_attr_to(const Shape* s) noexcept
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
		s->get_join_miter_limit(m_join_miter_limit);
		s->get_join_style(m_join_style);
		s->get_page_color(m_page_color);
		s->get_stroke_cap(m_stroke_cap);
		s->get_stroke_color(m_stroke_color);
		s->get_stroke_width(m_stroke_width);
		s->get_text_align_t(m_text_align_t);
		s->get_text_par_align(m_text_par_align);
		s->get_text_line_sp(m_text_line_sp);
		s->get_text_padding(m_text_padding);
	}

	// �f�[�^���[�_�[�ɏ�������.
	// dt_writer	�f�[�^���[�_�[
	void ShapePage::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(m_grid_base);
		dt_write(m_grid_color, dt_writer);
		dt_write(m_grid_emph, dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_grid_show));
		dt_writer.WriteBoolean(m_grid_snap);
		dt_write(m_page_color, dt_writer);
		dt_writer.WriteSingle(m_page_scale);
		dt_write(m_page_size, dt_writer);

		dt_write(m_arrow_size, dt_writer);	// ��邵�̐��@
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));	// ��邵�̌`��
		dt_write(m_corner_rad, dt_writer);	// �p�۔��a
		dt_write(m_stroke_cap, dt_writer);	// �[�̌`��
		dt_write(m_stroke_color, dt_writer);	// ���g�̐F
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_dash_cap));	// �j���̒[�̌`��
		dt_write(m_dash_patt, dt_writer);	// �j���̔z�u
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_dash_style));	// ���g�̌`��
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_join_style));	// �����̌���
		dt_writer.WriteSingle(m_join_miter_limit);	// �����̃}�C�^�[����
		dt_writer.WriteSingle(m_stroke_width);	// ���g�̑���
		dt_write(m_fill_color, dt_writer);	// �h��Ԃ��̐F
		dt_write(m_font_color, dt_writer);	// ���̂̐F
		dt_write(m_font_family, dt_writer);	// ���̖�
		dt_writer.WriteSingle(m_font_size);	// ���̂̑傫��
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_stretch));	// ���̂̕��̐L�k
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_style));	// ���̂̎���
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_weight));	// ���̂̑���
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_par_align));	// �i���̂��낦
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_t));	// ������̂��낦
		dt_writer.WriteSingle(m_text_line_sp);	// �s��
		dt_write(m_text_padding, dt_writer);	// ������̗]��
		dt_writer.WriteSingle(m_image_opac);	// �摜�̕s������
	}

}