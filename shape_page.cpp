#include "pch.h"
#include "shape.h"
//
// �y�[�W�̑傫����, ����̑傫���ɗ]�� (�}�[�W��) �𑫂�������.
//
//         margin_left         margin_right
//            |<->|               |<->|
//        + - @-----------------------+ - +
// margin_top |                       |   |
//        + - |   0---+---+---+---+   |   |
//            |   |   |   |   |   |   |   |
//            |   +---+---+---+---+   |   |
//            |   |   |   |   |   |   |   |
//            |   +---+---+---+---+   |   |
//            |   |   |   |   |   |   | page_h
//            |   +---+---+---+---+   |   |
//            |   |   |   |   |   |   |   |
//            |   +---+---+---+---+   |   |
//            |   |   |   |   |   |   |   |
//        + - |   +---+---+---+---+   |   |
// margin_bot |                       |   |
//        + - +-----------------------@ - +
//            |<------ page_w ------->|
//    0 ��, ���_
//    @ ��, �}�`���y�[�W���͂ݏo���Ȃ�������, ���E��`�̍���_�ƉE���_
using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	static void page_draw_grid(ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush, const float g_len, const D2D1_COLOR_F g_color, const GRID_EMPH g_emph, const D2D1_POINT_2F g_offset, const D2D1_SIZE_F g_size) noexcept;

	// �Ȑ��̕⏕��(����_�����Ԑ܂��)��\������.
	void ShapePage::auxiliary_draw_bezi(
		const D2D1_POINT_2F pressed,	// �|�C���^�[�������ꂽ�_
		const D2D1_POINT_2F current	// �|�C���^�[�̌��݂̓_
	) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
		HRESULT hr = S_OK;
		if (Shape::m_aux_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			hr = static_cast<ID2D1Factory3*>(factory)->CreateStrokeStyle(AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT,
				Shape::m_aux_style.put());
		}
		if (hr == S_OK) {
			const FLOAT aw = m_aux_width;
			// �y�[�W�̔{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
			D2D1_POINT_2F s;
			D2D1_POINT_2F e;

			e.x = current.x;
			e.y = pressed.y;
			brush->SetColor(COLOR_WHITE);
			target->DrawLine(pressed, e, brush, aw, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawLine(pressed, e, brush, aw, m_aux_style.get());
			s = e;
			e.x = pressed.x;
			e.y = current.y;
			brush->SetColor(COLOR_WHITE);
			target->DrawLine(s, e, brush, aw, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawLine(s, e, brush, aw, m_aux_style.get());
			s = e;
			brush->SetColor(COLOR_WHITE);
			target->DrawLine(s, current, brush, aw, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawLine(s, current, brush, aw, m_aux_style.get());
		}
	}

	// ���~�̕⏕����\������.
	void ShapePage::auxiliary_draw_elli(
		const D2D1_POINT_2F pressed,	// �|�C���^�[�������ꂽ�_
		const D2D1_POINT_2F current	// �|�C���^�[�̌��݂̓_
	) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
		HRESULT hr = S_OK;
		if (Shape::m_aux_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			hr = static_cast<ID2D1Factory3*>(factory)->CreateStrokeStyle(AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT,
				Shape::m_aux_style.put());
		}
		if (hr == S_OK) {
			// �y�[�W�̔{���ɂ�����炸�����ڂ̑�����ς��Ȃ�����, ���̋t������̑����Ɋi�[����.
			D2D1_MATRIX_3X2_F tran;
			target->GetTransform(&tran);
			const FLOAT s_width = static_cast<FLOAT>(1.0 / tran._11);	// ���̑���
			//const D2D_UI& d2d = sh.m_d2d;
			D2D1_POINT_2F rect;	// ���`
			D2D1_ELLIPSE elli;		// ���~

			pt_sub(current, pressed, rect);
			pt_mul(rect, 0.5, rect);
			pt_add(pressed, rect, elli.point);
			elli.radiusX = rect.x;
			elli.radiusY = rect.y;
			brush->SetColor(COLOR_WHITE);
			target->DrawEllipse(elli, brush, s_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawEllipse(elli, brush, s_width, Shape::m_aux_style.get());
		}
	}

	// �����̕⏕����\������.
	void ShapePage::auxiliary_draw_line(
		const D2D1_POINT_2F pressed,	// �|�C���^�[�������ꂽ�ʒu
		const D2D1_POINT_2F current	// �|�C���^�[�̌��݈ʒu
	) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
		HRESULT hr = S_OK;
		if (Shape::m_aux_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			hr = static_cast<ID2D1Factory3*>(factory)->CreateStrokeStyle(AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT,
				Shape::m_aux_style.put());
		}
		if (hr == S_OK) {
			brush->SetColor(COLOR_WHITE);
			target->DrawLine(pressed, current, brush, m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawLine(pressed, current, brush, m_aux_width, m_aux_style.get());
		}
	}

	// ���p�`�̕⏕����\������.
	void ShapePage::auxiliary_draw_poly(
		const D2D1_POINT_2F pressed,	// �|�C���^�[�������ꂽ�ʒu
		const D2D1_POINT_2F current,	// �|�C���^�[�̌��݈ʒu
		const POLY_OPTION& p_opt	// ���p�`�̑I����
	) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
		HRESULT hr = S_OK;
		if (Shape::m_aux_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			hr = static_cast<ID2D1Factory3*>(factory)->CreateStrokeStyle(AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT,
				Shape::m_aux_style.put());
		}
		if (hr == S_OK) {
			D2D1_POINT_2F pos;	// ���݈ʒu�ւ̈ʒu�x�N�g��
			pt_sub(current, pressed, pos);
			D2D1_POINT_2F poly[N_GON_MAX] ;	// ���_�̔z��
			ShapePoly::poly_create_by_box(pressed, pos, p_opt, poly);
			if (p_opt.m_vertex_cnt == 2) {
				brush->SetColor(COLOR_WHITE);
				target->DrawLine(poly[0], poly[1], brush, m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawLine(poly[0], poly[1], brush, m_aux_width, m_aux_style.get());
			}
			else {
				const auto i_start = (p_opt.m_end_closed ? p_opt.m_vertex_cnt - 1 : 0);
				const auto j_start = (p_opt.m_end_closed ? 0 : 1);
				for (size_t i = i_start, j = j_start; j < p_opt.m_vertex_cnt; i = j++) {
					brush->SetColor(COLOR_WHITE);
					target->DrawLine(poly[i], poly[j], brush, m_aux_width, nullptr);
					brush->SetColor(COLOR_BLACK);
					target->DrawLine(poly[i], poly[j], brush, m_aux_width, m_aux_style.get());
				}
			}
		}
	}

	// ���`�̕⏕����\������.
	void ShapePage::auxiliary_draw_rect(
		const D2D1_POINT_2F pressed,	// �|�C���^�[�������ꂽ�ʒu
		const D2D1_POINT_2F current	// �|�C���^�[�̌��݈ʒu
	) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
		HRESULT hr = S_OK;
		if (Shape::m_aux_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			hr = static_cast<ID2D1Factory3*>(factory)->CreateStrokeStyle(AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT,
				Shape::m_aux_style.put());
		}
		if (hr == S_OK) {
			const D2D1_RECT_F rc = {
				pressed.x, pressed.y, current.x, current.y
			};
			brush->SetColor(COLOR_WHITE);
			target->DrawRectangle(&rc, brush, m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawRectangle(&rc, brush, m_aux_width, m_aux_style.get());
		}
	}

	// �p�ە��`�̕⏕����\������.
	void ShapePage::auxiliary_draw_rrect(
		const D2D1_POINT_2F pressed,	// �|�C���^�[�������ꂽ�ʒu
		const D2D1_POINT_2F current	// �|�C���^�[�̌��݈ʒu
	) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
		HRESULT hr = S_OK;
		if (Shape::m_aux_style == nullptr) {
			ID2D1Factory* factory;
			target->GetFactory(&factory);
			hr = static_cast<ID2D1Factory3*>(factory)->CreateStrokeStyle(AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT,
				Shape::m_aux_style.put());
		}
		if (hr == S_OK) {
			const double cx = current.x;
			const double cy = current.y;
			const double px = pressed.x;
			const double py = pressed.y;
			const double qx = cx - px;
			const double qy = cy - py;
			//auto c_rad = m_corner_radius;
			D2D1_POINT_2F c_rad{ m_grid_base + 1.0f, m_grid_base + 1.0f };
			double rx = c_rad.x;
			double ry = c_rad.y;

			if (qx * rx < 0.0f) {
				rx = -rx;
			}
			if (qy * ry < 0.0f) {
				ry = -ry;
			}
			const D2D1_ROUNDED_RECT rr{
				D2D1_RECT_F{ pressed.x, pressed.y, current.x, current.y },
				static_cast<FLOAT>(rx),
				static_cast<FLOAT>(ry)
			};
			brush->SetColor(COLOR_WHITE);
			target->DrawRoundedRectangle(&rr, brush, Shape::m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawRoundedRectangle(&rr, brush, Shape::m_aux_width, Shape::m_aux_style.get());
		}
	}

	// �~�ʂ̕⏕����\������.
	void ShapePage::auxiliary_draw_arc(
		const D2D1_POINT_2F pressed,	// �|�C���^�[�������ꂽ�ʒu
		const D2D1_POINT_2F current	// �|�C���^�[�̌��݈ʒu
	) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
		ID2D1Factory* factory;
		HRESULT hr = S_OK;
		target->GetFactory(&factory);
		if (Shape::m_aux_style == nullptr) {
			hr = static_cast<ID2D1Factory3*>(factory)->CreateStrokeStyle(AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT,
				Shape::m_aux_style.put());
		}
		winrt::com_ptr<ID2D1PathGeometry> geom;
		if (hr == S_OK) {
			//D2D1_MATRIX_3X2_F tran;
			//target->GetTransform(&tran);
			//const FLOAT s_width = static_cast<FLOAT>(1.0 / tran._11);	// ���̑���
			hr = factory->CreatePathGeometry(geom.put());
		}
		winrt::com_ptr<ID2D1GeometrySink> sink;
		if (hr == S_OK) {
			hr = geom->Open(sink.put());
		}
		if (hr == S_OK) {
			const D2D1_ARC_SEGMENT arc{
				current,
				D2D1_SIZE_F{ fabsf(current.x - pressed.x), fabsf(current.y - pressed.y) },
				0.0f,
				D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE,
				D2D1_ARC_SIZE::D2D1_ARC_SIZE_SMALL
			};
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(pressed, D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
			sink->AddArc(arc);
			sink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
			hr = sink->Close();
		}
		if (hr == S_OK) {
			brush->SetColor(COLOR_WHITE);
			target->DrawGeometry(geom.get(), brush, Shape::m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawGeometry(geom.get(), brush, Shape::m_aux_width, Shape::m_aux_style.get());
		}
		sink = nullptr;
		geom = nullptr;
	}

	// �}�`��\������.
	void ShapePage::draw(void) noexcept
	{
		// �y�[�W�̐F�Ńy�[�W��h��Ԃ�.
		m_d2d_color_brush->SetColor(m_page_color);
		D2D1_RECT_F p_rect{	// �y�[�W�̋�`
			-m_page_margin.left,
			-m_page_margin.top,
			-m_page_margin.left + m_page_size.width,
			-m_page_margin.top + m_page_size.height
		};
		m_d2d_target->FillRectangle(p_rect, m_d2d_color_brush.get());

		// ����̕\�����Ŕw�ʂȂ�, �ŏ��ɕ����\������.
		if (m_grid_show == GRID_SHOW::BACK) {
			D2D1_SIZE_F g_size{
				m_page_size.width - (m_page_margin.left + m_page_margin.right),
				m_page_size.height - (m_page_margin.top + m_page_margin.bottom)
			};
			page_draw_grid(m_d2d_target, m_d2d_color_brush.get(), m_grid_base + 1.0f, m_grid_color, m_grid_emph, m_grid_offset, g_size);
		}

		// �}�`��\������.
		for (auto s : m_shape_list) {
			if (!s->is_deleted()) {
				s->draw();
			}
		}

		// ����̕\�����őO�ʂȂ�, �Ō�ɕ����\������.
		if (m_grid_show == GRID_SHOW::FRONT) {
			D2D1_SIZE_F g_size{
				m_page_size.width - (m_page_margin.left + m_page_margin.right),
				m_page_size.height - (m_page_margin.top + m_page_margin.bottom)
			};
			page_draw_grid(m_d2d_target, m_d2d_color_brush.get(), m_grid_base + 1.0f, m_grid_color, m_grid_emph, m_grid_offset, g_size);
		}
	}

	// �����\������.
	static void page_draw_grid(
		ID2D1RenderTarget* const target,	// D2D �`��Ώ�
		ID2D1SolidColorBrush* const brush,	// �F�u���V
		const float g_len,	// ����̑傫��
		const D2D1_COLOR_F g_color,	// ����̐F
		const GRID_EMPH g_emph,	// ����̋����̌`��
		const D2D1_POINT_2F g_offset,	// ����̂��炵�l
		const D2D1_SIZE_F g_size	// �����\�������`�̑傫��
	) noexcept
	{
		const FLOAT g_width = 1.0f;	// ����̑���
		D2D1_POINT_2F h_start, h_end;	// ���̕���̊J�n�E�I���ʒu
		D2D1_POINT_2F v_start, v_end;	// �c�̕���̊J�n�E�I���ʒu
		brush->SetColor(g_color);
		v_start.y = 0.0f;
		h_start.x = 0.0f;
		const double page_h = g_size.height;
		const double page_w = g_size.width;
		const double offs_x = g_offset.x;
		const double offs_y = g_offset.y;
		v_end.y = g_size.height - 1.0f;
		h_end.x = g_size.width - 1.0f;

		// �����ȕ����\������.
		double w;
		double x;
		for (uint32_t i = 0; (x = round((static_cast<double>(g_len) * i + offs_x) / PT_ROUND) * PT_ROUND) <= page_w; i++) {
			if (g_emph.m_gauge_2 != 0 && (i % g_emph.m_gauge_2) == 0) {
				w = 2.0 * g_width;
			}
			else if (g_emph.m_gauge_1 != 0 && (i % g_emph.m_gauge_1) == 0) {
				w = g_width;
			}
			else {
				w = 0.5 * g_width;
			}
			v_start.x = v_end.x = static_cast<FLOAT>(x);
			target->DrawLine(v_start, v_end, brush, static_cast<FLOAT>(w), nullptr);
		}
		// �����ȕ����\������.
		double y;
		for (uint32_t i = 0; (y = round((static_cast<double>(g_len) * i + offs_y) / PT_ROUND) * PT_ROUND) <= page_h; i++) {
			if (g_emph.m_gauge_2 != 0 && (i % g_emph.m_gauge_2) == 0) {
				w = 2.0 * g_width;
			}
			else if (g_emph.m_gauge_1 != 0 && (i % g_emph.m_gauge_1) == 0) {
				w = g_width;
			}
			else {
				w = 0.5 * g_width;
			}
			h_start.y = h_end.y = static_cast<FLOAT>(y);
			target->DrawLine(h_start, h_end, brush, static_cast<FLOAT>(w), nullptr);
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

	// �h��Ԃ��F�𓾂�.
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

	// ���̂̕��𓾂�.
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

	// �y�[�W�̐F�𓾂�.
	bool ShapePage::get_page_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_page_color;
		return true;
	}

	// �y�[�W�̑傫���𓾂�.
	bool ShapePage::get_page_size(D2D1_SIZE_F& val) const noexcept
	{
		val = m_page_size;
		return true;
	}

	// �[�̌`���𓾂�.
	//bool ShapePage::get_stroke_cap(CAP_STYLE& val) const noexcept
	bool ShapePage::get_stroke_cap(D2D1_CAP_STYLE& val) const noexcept
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
	//bool ShapePage::get_dash_cap(D2D1_CAP_STYLE& val) const noexcept
	//{
	//	val = m_dash_cap;
	//	return true;
	//}

	// �j���̔z�u�𓾂�.
	bool ShapePage::get_dash_pat(DASH_PAT& val) const noexcept
	{
		val = m_dash_pat;
		return true;
	}

	// ���g�̌`���𓾂�.
	bool ShapePage::get_dash_style(D2D1_DASH_STYLE& val) const noexcept
	{
		val = m_dash_style;
		return true;
	}

	// �����̌����̐�萧���𓾂�.
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
	bool ShapePage::get_text_align_vert(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_vert;
		return true;
	}

	// ������̂��낦�𓾂�.
	bool ShapePage::get_text_align_horz(DWRITE_TEXT_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_horz;
		return true;
	}

	// �s�Ԃ𓾂�.
	bool ShapePage::get_text_line_sp(float& val) const noexcept
	{
		val = m_text_line_sp;
		return true;
	}

	// ������̗]���𓾂�.
	bool ShapePage::get_text_pad(D2D1_SIZE_F& val) const noexcept
	{
		val = m_text_pad;
		return true;
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	void ShapePage::read(DataReader const& dt_reader)
	{
		// ����̑傫��
		const auto g_base = dt_reader.ReadSingle();
		if (g_base >= 0.0f && g_base <= 127.5f) {
			m_grid_base = g_base;
		}
		// ����̐F
		const D2D1_COLOR_F g_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if ((g_color.r >= 0.0f && g_color.r <= 1.0f) && (g_color.g >= 0.0f && g_color.g <= 1.0f) &&
			(g_color.b >= 0.0f && g_color.b <= 1.0f) && (g_color.a >= 0.0f && g_color.a <= 1.0f)) {
			m_grid_color = g_color;
		}

		// ����̋���
		const GRID_EMPH g_emph{
			dt_reader.ReadUInt32(),
			dt_reader.ReadUInt32()
		};
		if (equal(g_emph, GRID_EMPH_0) || equal(g_emph, GRID_EMPH_2) || 
			equal(g_emph, GRID_EMPH_3)) {
			m_grid_emph = g_emph;
		}
		// ����̕\��
		const GRID_SHOW g_show = static_cast<GRID_SHOW>(dt_reader.ReadUInt32());
		if (g_show == GRID_SHOW::HIDE || g_show == GRID_SHOW::BACK || g_show == GRID_SHOW::FRONT) {
			m_grid_show = g_show;
		}
		// ����ɍ��킹��.
		//m_snap_grid = dt_reader.ReadBoolean();
		// �y�[�W�̐F
		const D2D1_COLOR_F p_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (p_color.r >= 0.0f && p_color.r <= 1.0f && p_color.g >= 0.0f && p_color.g <= 1.0f &&
			p_color.b >= 0.0f && p_color.b <= 1.0f && p_color.a >= 0.0f && p_color.a <= 1.0f) {
			m_page_color = p_color;
		}
		// �y�[�W�̑傫��
		const D2D1_SIZE_F p_size{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (p_size.width >= 1.0f && p_size.width <= PAGE_SIZE_MAX &&
			p_size.height >= 1.0f && p_size.height <= PAGE_SIZE_MAX) {
			m_page_size = p_size;
		}
		// �y�[�W�̓��]��
		const D2D1_RECT_F p_mar{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (p_mar.left >= 0.0f && p_mar.right >= 0.0f && 
			p_mar.left + p_mar.right < m_page_size.width &&
			p_mar.top >= 0.0f && p_mar.bottom >= 0.0f &&
			p_mar.top + p_mar.bottom < m_page_size.height) {
			m_page_margin = p_mar;
		}
		// ��邵�̐��@
		const ARROW_SIZE a_size{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (a_size.m_width >= 0.0f && a_size.m_width <= 127.5f &&
			a_size.m_length >= 0.0f && a_size.m_length <= 127.5f &&
			a_size.m_offset >= 0.0f && a_size.m_offset <= 127.5f) {
			m_arrow_size = a_size;
		}
		// ��邵�̌`��
		const ARROW_STYLE a_style = static_cast<ARROW_STYLE>(dt_reader.ReadUInt32());
		if (a_style == ARROW_STYLE::ARROW_NONE ||
			a_style == ARROW_STYLE::ARROW_OPENED ||
			a_style == ARROW_STYLE::ARROW_FILLED) {
			m_arrow_style = a_style;
		}
		// ��邵�̕Ԃ��̌`��
		const D2D1_CAP_STYLE a_cap = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
		if (a_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT ||
			a_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE ||
			a_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND ||
			a_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
			m_arrow_cap = a_cap;
		}
		// ��邵�̐�[�̌`��
		const D2D1_LINE_JOIN a_join = static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32());
		if (a_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL ||
			//a_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
			a_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL ||
			a_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
			m_arrow_join = a_join;
		}
		// �[�̌`��
		const D2D1_CAP_STYLE s_cap = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
		if (s_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT ||
			s_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND ||
			s_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE ||
			s_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
			m_stroke_cap = s_cap;
		}
		// ���E�g�̐F
		const D2D1_COLOR_F s_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (s_color.r >= 0.0f && s_color.r <= 1.0f && s_color.g >= 0.0f && s_color.g <= 1.0f &&
			s_color.b >= 0.0f && s_color.b <= 1.0f && s_color.a >= 0.0f && s_color.a <= 1.0f) {
			m_stroke_color = s_color;
		}
		// ���E�g�̑���
		const float s_width = dt_reader.ReadSingle();
		if (s_width >= 0.0f && s_width <= 127.5f) {
			m_stroke_width = s_width;
		}
		// �j���̒[�̌`��
		//const D2D1_CAP_STYLE d_cap = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
		//if (d_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT ||
		//	d_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND ||
		//	d_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE ||
		//	d_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
		//	m_dash_cap = d_cap;
		//}
		// �j���̔z�u
		const DASH_PAT d_patt{
			{
				dt_reader.ReadSingle(), dt_reader.ReadSingle(),
				dt_reader.ReadSingle(), dt_reader.ReadSingle(),
				dt_reader.ReadSingle(), dt_reader.ReadSingle()
			}
		};
		if (d_patt.m_[0] >= 0.0f && d_patt.m_[1] >= 0.0f && d_patt.m_[2] >= 0.0f &&
			d_patt.m_[3] >= 0.0f && d_patt.m_[4] >= 0.0f && d_patt.m_[5] >= 0.0f) {
			m_dash_pat = d_patt;
		}
		// �j���̌`��
		const D2D1_DASH_STYLE d_style = static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32());
		if (d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID ||
			d_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM) {
			m_dash_style = d_style;
		}
		// ���̌����̌`��
		const D2D1_LINE_JOIN j_style = static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32());
		if (j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL ||
			//j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
			j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL ||
			j_style == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
			m_join_style = j_style;
		}
		// ���̌����̐�萧������
		const float j_limit = dt_reader.ReadSingle();
		if (j_limit >= 1.0f && j_limit <= 128.5f) {
			m_join_miter_limit = j_limit;
		}
		// �h��Ԃ��F
		const D2D1_COLOR_F f_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (f_color.r >= 0.0f && f_color.r <= 1.0f && f_color.g >= 0.0f && f_color.g <= 1.0f &&
			f_color.b >= 0.0f && f_color.b <= 1.0f && f_color.a >= 0.0f && f_color.a <= 1.0f) {
			m_fill_color = f_color;
		}
		// ���̂̐F
		const D2D1_COLOR_F font_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (font_color.r >= 0.0f && font_color.r <= 1.0f &&
			font_color.g >= 0.0f && font_color.g <= 1.0f &&
			font_color.b >= 0.0f && font_color.b <= 1.0f &&
			font_color.a >= 0.0f && font_color.a <= 1.0f) {
			m_font_color = font_color;
		}

		// ���̖�
		const size_t font_family_len = dt_reader.ReadUInt32();
		uint8_t* font_family_data = new uint8_t[2 * (font_family_len + 1)];
		dt_reader.ReadBytes(array_view(font_family_data, font_family_data + 2 * font_family_len));
		m_font_family = reinterpret_cast<wchar_t*>(font_family_data);
		m_font_family[font_family_len] = L'\0';
		ShapeText::is_available_font(m_font_family);

		// ���̂̑傫��
		const float font_size = dt_reader.ReadSingle();
		if (font_size >= 1.0f && font_size <= 128.5f) {
			m_font_size = font_size;
		}

		// ���̂̕�
		const DWRITE_FONT_STRETCH f_stretch =
			static_cast<DWRITE_FONT_STRETCH>(dt_reader.ReadUInt32());
		if (f_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_CONDENSED ||
			f_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXPANDED ||
			f_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_CONDENSED ||
			f_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_EXPANDED ||
			f_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL ||
			f_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_CONDENSED ||
			f_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_EXPANDED ||
			f_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_CONDENSED ||
			f_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_EXPANDED) {
			m_font_stretch = f_stretch;
		}
		// ���̂̎���
		const DWRITE_FONT_STYLE f_style = static_cast<DWRITE_FONT_STYLE>(dt_reader.ReadUInt32());
		if (f_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC ||
			f_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL ||
			f_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE) {
			m_font_style = f_style;
		}
		// ���̂̑���
		const DWRITE_FONT_WEIGHT f_weight = 
			static_cast<DWRITE_FONT_WEIGHT>(dt_reader.ReadUInt32());
		if (f_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_THIN ||
			f_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_LIGHT ||
			f_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_LIGHT ||
			f_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_SEMI_LIGHT ||
			f_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL ||
			f_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_MEDIUM ||
			f_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_SEMI_BOLD ||
			f_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BOLD ||
			f_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BOLD ||
			f_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BLACK ||
			f_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BLACK) {
			m_font_weight = f_weight;
		}
		// �i���̂��낦
		const DWRITE_PARAGRAPH_ALIGNMENT t_align_vert =
			static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(dt_reader.ReadUInt32());
		if (t_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER ||
			t_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR ||
			t_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR) {
			m_text_align_vert = t_align_vert;
		}
		// ������̂��낦
		const DWRITE_TEXT_ALIGNMENT t_align_horz = 
			static_cast<DWRITE_TEXT_ALIGNMENT>(dt_reader.ReadUInt32());
		if (t_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER ||
			t_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED ||
			t_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING ||
			t_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING) {
			m_text_align_horz = t_align_horz;
		}
		// �s��
		const float t_line_sp = dt_reader.ReadSingle();
		if (t_line_sp >= 0.0f && t_line_sp <= 127.5f) {
			m_text_line_sp = t_line_sp;
		}
		// ������̗]��
		const D2D1_SIZE_F t_pad{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (t_pad.width >= 0.0 && t_pad.width <= 127.5 &&
			t_pad.height >= 0.0 && t_pad.height <= 127.5) {
			m_text_pad = t_pad;
		}
		// �摜�̕s������
		const float i_opac = dt_reader.ReadSingle();
		if (i_opac >= 0.0f && i_opac <= 1.0f) {
			m_image_opac = i_opac;
		}
		ShapeText::is_available_font(m_font_family);
	}

	// �l���邵�̕Ԃ��̌`���Ɋi�[����.
	bool ShapePage::set_arrow_cap(const D2D1_CAP_STYLE val) noexcept
	{
		if (!equal(m_arrow_cap, val)) {
			m_arrow_cap = val;
			return true;
		}
		return false;
	}

	// �l���邵�̐�[�̌`���Ɋi�[����.
	bool ShapePage::set_arrow_join(const D2D1_LINE_JOIN val) noexcept
	{
		if (!equal(m_arrow_join, val)) {
			m_arrow_join = val;
			return true;
		}
		return false;
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
	/*
	bool ShapePage::set_corner_radius(const D2D1_POINT_2F& val) noexcept
	{
		if (!equal(m_corner_radius, val)) {
			m_corner_radius = val;
			return true;
		}
		return false;
	}
	*/

	// �l��h��Ԃ��F�Ɋi�[����.
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

	// �l�����̂̕��Ɋi�[����.
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

	// �l��, �\��, ����, �⏕���̊e�F�Ɋi�[����
	bool ShapePage::set_page_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_page_color, val)) {
			m_page_color = val;
			return true;
		}
		return false;
	}

	// �l���y�[�W�̔{���Ɋi�[����.
	/*
	bool ShapePage::set_page_scale(const float val) noexcept
	{
		if (!equal(m_page_scale,val)) {
			m_page_scale = val;
			return true;
		}
		return false;
	}
	*/

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
	//bool ShapePage::set_stroke_cap(const CAP_STYLE& val) noexcept
	bool ShapePage::set_stroke_cap(const D2D1_CAP_STYLE& val) noexcept
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
	//bool ShapePage::set_dash_cap(const D2D1_CAP_STYLE& val) noexcept
	//{
	//	const auto old_val = m_dash_cap;
	//	return (m_dash_cap = val) != old_val;
	//}

	// �j���̔z�u�Ɋi�[����.
	bool ShapePage::set_dash_pat(const DASH_PAT& val) noexcept
	{
		if (!equal(m_dash_pat, val)) {
			m_dash_pat = val;
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

	// �l������̌����̐�萧���Ɋi�[����.
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
	bool ShapePage::set_text_align_vert(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept
	{
		const auto old_val = m_text_align_vert;
		return (m_text_align_vert = val) != old_val;
	}

	// ������̂��낦�Ɋi�[����.
	bool ShapePage::set_text_align_horz(const DWRITE_TEXT_ALIGNMENT val) noexcept
	{
		const auto old_val = m_text_align_horz;
		return (m_text_align_horz = val) != old_val;
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
	bool ShapePage::set_text_pad(const D2D1_SIZE_F val) noexcept
	{
		if (!equal(m_text_pad, val)) {
			m_text_pad = val;
			return true;
		}
		return false;
	}

	// �}�`�̑����l���i�[����.
	void ShapePage::set_attr_to(const Shape* s) noexcept
	{
		if (s != nullptr && s != this) {
			s->get_arrow_size(m_arrow_size);
			s->get_arrow_style(m_arrow_style);
			s->get_arrow_cap(m_arrow_cap);
			s->get_arrow_join(m_arrow_join);
			//s->get_dash_cap(m_dash_cap);
			s->get_dash_pat(m_dash_pat);
			s->get_dash_style(m_dash_style);
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
			s->get_image_opacity(m_image_opac);
			s->get_join_miter_limit(m_join_miter_limit);
			s->get_join_style(m_join_style);
			s->get_page_color(m_page_color);
			s->get_page_margin(m_page_margin);
			s->get_stroke_cap(m_stroke_cap);
			s->get_stroke_color(m_stroke_color);
			s->get_stroke_width(m_stroke_width);
			s->get_text_align_horz(m_text_align_horz);
			s->get_text_align_vert(m_text_align_vert);
			s->get_text_line_sp(m_text_line_sp);
			s->get_text_pad(m_text_pad);
		}
	}

	// �f�[�^���[�_�[�ɏ�������.
	// dt_writer	�f�[�^���[�_�[
	void ShapePage::write(DataWriter const& dt_writer) const
	{
		// ����̊�̑傫��
		dt_writer.WriteSingle(m_grid_base);
		// ����̐F
		dt_writer.WriteSingle(m_grid_color.r);
		dt_writer.WriteSingle(m_grid_color.g);
		dt_writer.WriteSingle(m_grid_color.b);
		dt_writer.WriteSingle(m_grid_color.a);
		// ����̋���
		dt_writer.WriteUInt32(m_grid_emph.m_gauge_1);
		dt_writer.WriteUInt32(m_grid_emph.m_gauge_2);
		// ����̕\��
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_grid_show));
		// ����ɍ��킹��
		//dt_writer.WriteBoolean(m_snap_grid);
		// �y�[�W�̐F
		dt_writer.WriteSingle(m_page_color.r);
		dt_writer.WriteSingle(m_page_color.g);
		dt_writer.WriteSingle(m_page_color.b);
		dt_writer.WriteSingle(m_page_color.a);
		// �y�[�W�̊g�嗦
		//dt_writer.WriteSingle(m_page_scale);
		// �y�[�W�̑傫��
		dt_writer.WriteSingle(m_page_size.width);
		dt_writer.WriteSingle(m_page_size.height);
		// �y�[�W�̓��]��
		dt_writer.WriteSingle(m_page_margin.left);
		dt_writer.WriteSingle(m_page_margin.top);
		dt_writer.WriteSingle(m_page_margin.right);
		dt_writer.WriteSingle(m_page_margin.bottom);
		// ��邵�̑傫��
		dt_writer.WriteSingle(m_arrow_size.m_width);
		dt_writer.WriteSingle(m_arrow_size.m_length);
		dt_writer.WriteSingle(m_arrow_size.m_offset);
		// ��邵�̌`��
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));
		// ��邵�̕Ԃ��̌`��
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_cap));
		// ��邵�̐�[�̌`��
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_join));
		// ���̒[�̌`��
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_stroke_cap));
		//dt_writer.WriteUInt32(m_stroke_cap.m_end);
		// ���E�g�̐F
		dt_writer.WriteSingle(m_stroke_color.r);
		dt_writer.WriteSingle(m_stroke_color.g);
		dt_writer.WriteSingle(m_stroke_color.b);
		dt_writer.WriteSingle(m_stroke_color.a);
		// ���E�g�̑���
		dt_writer.WriteSingle(m_stroke_width);
		// �j���̒[�̌`��
		//dt_writer.WriteUInt32(m_dash_cap);
		// �j���̔z�u
		dt_writer.WriteSingle(m_dash_pat.m_[0]);
		dt_writer.WriteSingle(m_dash_pat.m_[1]);
		dt_writer.WriteSingle(m_dash_pat.m_[2]);
		dt_writer.WriteSingle(m_dash_pat.m_[3]);
		dt_writer.WriteSingle(m_dash_pat.m_[4]);
		dt_writer.WriteSingle(m_dash_pat.m_[5]);
		// �j���̌`��
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_dash_style));
		// �����̌���
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_join_style));
		// �����̐�萧��
		dt_writer.WriteSingle(m_join_miter_limit);
		// �h��Ԃ��F
		dt_writer.WriteSingle(m_fill_color.r);
		dt_writer.WriteSingle(m_fill_color.g);
		dt_writer.WriteSingle(m_fill_color.b);
		dt_writer.WriteSingle(m_fill_color.a);
		// ���̂̐F
		dt_writer.WriteSingle(m_font_color.r);
		dt_writer.WriteSingle(m_font_color.g);
		dt_writer.WriteSingle(m_font_color.b);
		dt_writer.WriteSingle(m_font_color.a);
		// ���̖�
		const uint32_t f_len = wchar_len(m_font_family);
		const uint8_t* f_data = reinterpret_cast<const uint8_t*>(m_font_family);
		dt_writer.WriteUInt32(f_len);
		dt_writer.WriteBytes(array_view(f_data, f_data + 2 * static_cast<size_t>(f_len)));
		// ���̂̑傫��
		dt_writer.WriteSingle(m_font_size);
		// ���̂̕�
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_stretch));
		// ���̂̎���
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_style));
		// ���̂̑���
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_weight));
		// �i���̂��낦
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_vert));
		// ������̂��낦
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_horz));
		// �s��
		dt_writer.WriteSingle(m_text_line_sp);
		// ������̗]��
		dt_writer.WriteSingle(m_text_pad.width);
		dt_writer.WriteSingle(m_text_pad.height);
		// �摜�̕s������
		dt_writer.WriteSingle(m_image_opac);
	}

}