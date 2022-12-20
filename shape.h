#pragma once
//------------------------------
// shape.h
// shape.cpp	�}�`�̂ЂȌ^, ���̑�
// shape_bezi.cpp	�x�W�F�Ȑ�
// shape_dt.cpp	�ǂݍ���, ��������.
// shape_elli.cpp	���~
// shape_group.cpp	�O���[�v
// shape_image.cpp	�摜
// shape_line.cpp	���� (��邵��)
// shape_path.cpp	�܂���̂ЂȌ^
// shape_pdf.cpp	PDF �ւ̏�������
// shape_poly.cpp	���p�`
// shape.rect.cpp	���`
// shape_rrect.cpp	�p�ە��`
// shape_ruler.cpp	��K
// shape_sheet.cpp	�p��
// shape_slist.cpp	�}�`���X�g
// shape_stroke.cpp	���g�̂ЂȌ^
// shape_svg.cpp	SVG �ւ̏�������
// shape_text.cpp	������
//------------------------------
#include <list>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include "d2d_ui.h"
//
// +-------------+
// | Shape*      |
// +------+------+
//        |
//        +---------------+---------------+
//        |               |               |
// +------+------+ +------+------+ +------+------+
// | ShapeSelect*| | ShapeGroup  | | ShapeSheet  |
// +------+------+ +-------------+ | .D2D_UI -------------> SwapChainPanel
//        |                        +-------------+
//        +---------------+
//        |               |
// +------+------+ +------+------+
// | ShapeStroke*| | ShapeImage  |
// +------+------+ +-------------+
//        |
//        +-------------------------------+
//        |                               |
// +------+------+                 +------+------+
// | ShapeLine*  |                 | ShapeRect*  |
// +------+------+                 +------+------+
//        |                               |
// +------+------+                        |
// | ShapePath*  |                        |
// +------+------+                        |
//        |                               |
//        +---------------+               +---------------+---------------+---------------+
//        |               |               |               |               |               |
// +------+------+ +------+------+ +------+------+ +------+------+ +------+------+ +------+------+
// | ShapePoly   | | ShapeBezi   | | ShapeElli   | | ShapeRRect  | | ShapeText   | | ShapeRuler  |
// +-------------+ +-------------+ +-------------+ +-------------+ +-------------+ +-------------+
//
// * ����͒��ۃN���X.
// ShapeSheet �̓����o�� D2D_UI ������, D2D_UI �̓X���b�v�`�F�[���p�l���ւ̎Q�Ƃ��ێ�����.

// SVG �̂��߂̃e�L�X�g���s�R�[�h
#define SVG_NEW_LINE	"\n"

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::IAsyncOperation;
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;
	using winrt::Windows::Foundation::Point;
	using winrt::Windows::Storage::Streams::IRandomAccessStream;
	using winrt::Windows::Graphics::Imaging::SoftwareBitmap;
#if defined(_DEBUG)
	extern uint32_t debug_leak_cnt;
	constexpr wchar_t DEBUG_MSG[] = L"Memory leak occurs";
#endif
	constexpr double PT_ROUND = 1.0 / 16.0;

	// �O���錾
	struct Shape;
	struct ShapeBezi;
	struct ShapeElli;
	struct ShapeGroup;
	struct ShapeImage;
	struct ShapeLine;
	struct ShapePath;
	struct ShapePoly;
	struct ShapeRect;
	struct ShapeRRect;
	struct ShapeRuler;
	struct ShapeSelect;
	struct ShapeSheet;
	struct ShapeStroke;
	struct ShapeText;

	constexpr D2D1_COLOR_F ACCENT_COLOR{ 0.0f, 0x78 / 255.0f, 0xD4 / 255.0f, 1.0f };	// �����͈͂̔w�i�F SystemAccentColor

	// �⏕��
	constexpr FLOAT AUXILIARY_SEG_DASHES[]{ 4.0f, 4.0f };	// �⏕���̔j���̔z�u
	constexpr UINT32 AUXILIARY_SEG_DASHES_CONT = sizeof(AUXILIARY_SEG_DASHES) / sizeof(AUXILIARY_SEG_DASHES[0]);	// �⏕���̔j���̔z�u�̗v�f��
	constexpr float AUXILIARY_SEG_OPAC = 0.975f;	// �⏕���̕s�����x
	constexpr D2D1_STROKE_STYLE_PROPERTIES1 AUXILIARY_SEG_STYLE	// �⏕���̐��̓���
	{
		D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// startCap
		D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// endCap
		D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND,	// dashCap
		D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL,	// lineJoin
		1.0f,	// miterLimit
		D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM,	// dashStyle
		0.0f,	// dashOffset
		D2D1_STROKE_TRANSFORM_TYPE::D2D1_STROKE_TRANSFORM_TYPE_NORMAL
	};

	// �}�`�̕��� (�A���J�[�|�C���g)
	// ���̒�܂��Ă��Ȃ����p�`�̒��_������킷����, enum struct �łȂ� enum ��p����.
	enum ANC_TYPE {
		ANC_SHEET,		// �}�`�̊O�� (���J�[�\��)
		ANC_FILL,		// �}�`�̓��� (�ړ��J�[�\��)
		ANC_STROKE,	// ���g (�ړ��J�[�\��)
		ANC_TEXT,		// ������ (�ړ��J�[�\��)
		ANC_NW,		// ���`�̍���̒��_ (�k���쓌�J�[�\��)
		ANC_SE,		// ���`�̉E���̒��_ (�k���쓌�J�[�\��)
		ANC_NE,		// ���`�̉E��̒��_ (�k���쐼�J�[�\��)
		ANC_SW,		// ���`�̍����̒��_ (�k���쐼�J�[�\��)
		ANC_NORTH,		// ���`�̏�ӂ̒��_ (�㉺�J�[�\��)
		ANC_SOUTH,		// ���`�̉��ӂ̒��_ (�㉺�J�[�\��)
		ANC_EAST,		// ���`�̍��ӂ̒��_ (���E�J�[�\��)
		ANC_WEST,		// ���`�̉E�ӂ̒��_ (���E�J�[�\��)
		ANC_R_NW,		// ����̊p�ۂ̒��S�_ (�\���J�[�\��)
		ANC_R_NE,		// �E��̊p�ۂ̒��S�_ (�\���J�[�\��)
		ANC_R_SE,		// �E���̊p�ۂ̒��S�_ (�\���J�[�\��)
		ANC_R_SW,		// �����̊p�ۂ̒��S�_ (�\���J�[�\��)
		ANC_P0,	// �p�X�̊J�n�_ (�\���J�[�\��)
	};

	// ��邵�̐��@
	struct ARROW_SIZE {
		float m_width;		// �Ԃ��̕�
		float m_length;		// ��[����t�����܂ł̒���
		float m_offset;		// ��[�̂��炵��
	};
	constexpr ARROW_SIZE ARROW_SIZE_DEFVAL{ 7.0, 16.0, 0.0 };	// ��邵�̐��@�̊���l

	// ��邵�̌`��
	enum struct ARROW_STYLE {
		NONE,	// �Ȃ�
		OPENED,	// �J������邵
		FILLED	// ������邵
	};

	// �����̒[�_
	// (SVG �� PDF ��, �n�_�I�_�̋�ʂ��ł��Ȃ�)
	struct CAP_STYLE {
		D2D1_CAP_STYLE m_start;	// �n�_
		D2D1_CAP_STYLE m_end;	// �I�_
	};

	constexpr D2D1_COLOR_F COLOR_BLACK{ 0.0f, 0.0f, 0.0f, 1.0f };	// ��
	constexpr D2D1_COLOR_F COLOR_WHITE{ 1.0f, 1.0f, 1.0f, 1.0f };	// ��
	constexpr D2D1_COLOR_F COLOR_TEXT_SELECTED = { 1.0f, 1.0f, 1.0f, 1.0f };	// �����͈͂̕����F


	// �j���̔z�u
	union DASH_PATT {
		float m_[6];
	};
	constexpr DASH_PATT DASH_PATT_DEFVAL{ { 4.0F, 3.0F, 1.0F, 3.0F, 1.0F, 3.0F } };	// �j���̔z�u�̊���l

	// ����̋���
	struct GRID_EMPH {
		uint16_t m_gauge_1;	// ��������Ԋu (����1)
		uint16_t m_gauge_2;	// ��������Ԋu (����2)
	};
	constexpr GRID_EMPH GRID_EMPH_0{ 0, 0 };	// �����Ȃ�
	constexpr GRID_EMPH GRID_EMPH_2{ 2, 0 };	// ����2������
	constexpr GRID_EMPH GRID_EMPH_3{ 2, 10 };	// ����2�� 10 �Ԗڂ�����

	// ����̕\��
	enum struct GRID_SHOW {
		HIDE,	// �\���Ȃ�
		BACK,	// �Ŕw�ʂɕ\��
		FRONT	// �őO�ʂɕ\��
	};

	// ���p�`�̍쐬���@
	struct POLY_OPTION {
		uint32_t m_vertex_cnt;	// ��}���鑽�p�`�̒��_�̐�.
		bool m_regular;	// �����p�`�ō�}����.
		bool m_vertex_up;	// ���_����ɍ�}����.
		bool m_end_closed;	// �ӂ���č�}����.
		bool m_clockwise;	// ���_�����v���ɍ�}����.
	};
	constexpr POLY_OPTION POLY_OPTION_DEFVAL{ 3, true, true, true, true };	// ���p�`�̍쐬���@�̊���l

	constexpr float COLOR_MAX = 255.0f;	// �F�����̍ő�l
	constexpr double PT_PER_INCH = 72.0;	// 1 �C���`������̃|�C���g��
	constexpr double MM_PER_INCH = 25.4;	// 1 �C���`������̃~�����[�g����
	constexpr float FONT_SIZE_DEFVAL = static_cast<float>(12.0 * 96.0 / 72.0);	// ���̂̑傫���̊���l (�V�X�e�����\�[�X�ɒl�����������ꍇ)
	constexpr D2D1_COLOR_F GRID_COLOR_DEFVAL{ ACCENT_COLOR.r, ACCENT_COLOR.g, ACCENT_COLOR.b, 0.5f };	// ����̐F�̊���l
	constexpr float GRID_LEN_DEFVAL = 48.0f;	// ����̒����̊���l
	constexpr float MITER_LIMIT_DEFVAL = 10.0f;	// �}�C�^�[�����䗦�̊���l
	constexpr D2D1_SIZE_F TEXT_MARGIN_DEFVAL{ FONT_SIZE_DEFVAL / 4.0, FONT_SIZE_DEFVAL / 4.0 };	// ������̗]���̊���l
	constexpr size_t MAX_N_GON = 256;	// ���p�`�̒��_�̍ő吔 (�q�b�g����ŃX�^�b�N�𗘗p���邽��, �I�[�o�[�t���[���Ȃ��悤��������)

	MIDL_INTERFACE("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d")
		IMemoryBufferByteAccess : IUnknown
	{
		virtual HRESULT STDMETHODCALLTYPE GetBuffer(
			BYTE * *value,
			UINT32 * capacity
			);
	};

	// �}�`�̕��ʁi�~�`�j��\������.
	inline void anc_draw_ellipse(const D2D1_POINT_2F a_pos, const ShapeSheet& sh);
	// �}�`�̕��� (���`) ��\������.
	inline void anc_draw_rect(const D2D1_POINT_2F a_pos, const ShapeSheet& sh);
	// �P���x�������������������肷��.
	inline bool equal(const float a, const float b) noexcept;
	// �{���x�������������������肷��.
	inline bool equal(const double a, const double b) noexcept;
	// ���l�����肷��.
	template<typename T> inline bool equal(const T a, const T b) noexcept { return a == b; };
	// ��邵�̐��@�����������肷��.
	inline bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept;
	// ���̒P�_�����������肷��.
	inline bool equal(const CAP_STYLE& a, const CAP_STYLE& b) noexcept;
	// �F�����������肷��.
	inline bool equal(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) noexcept;
	// �ʒu�����������肷��.
	inline bool equal(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept;
	// ���`�����������肷��.
	inline bool equal(const D2D1_RECT_F& a, const D2D1_RECT_F& b) noexcept;
	// ���@�����������肷��.
	inline bool equal(const D2D1_SIZE_F a, const D2D1_SIZE_F b) noexcept;
	// �����͈͂����������肷��.
	inline bool equal(const DWRITE_TEXT_RANGE a, const DWRITE_TEXT_RANGE b) noexcept;
	// ����̋��������������肷��.
	inline bool equal(const GRID_EMPH& a, const GRID_EMPH& b) noexcept;
	// �j���̔z�u�����������肷��.
	inline bool equal(const DASH_PATT& a, const DASH_PATT& b) noexcept;
	// ���C�h�����񂪓��������肷��.
	inline bool equal(const wchar_t* a, const wchar_t* b) noexcept;
	// winrt �����񂪓��������肷��.
	inline bool equal(winrt::hstring const& a, const wchar_t* b) noexcept;
	// �F�̐��������������肷��.
	inline bool equal_color_comp(const FLOAT a, const FLOAT b) noexcept;
	// ��邵�̕Ԃ��̈ʒu�����߂�.
	inline void get_arrow_barbs(const D2D1_POINT_2F a_vec, const double a_len, const double h_width, const double h_len, D2D1_POINT_2F barbs[]) noexcept;
	// �F���s���������肷��.
	inline bool is_opaque(const D2D1_COLOR_F& color) noexcept;
	// �x�N�g���̒��� (�̎���l) �𓾂�
	inline double pt_abs2(const D2D1_POINT_2F a) noexcept;
	// �ʒu�Ɉʒu�𑫂�
	inline void pt_add(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// �ʒu�ɃX�J���[�l�𑫂�
	inline void pt_add(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept;
	// �ʒu��X����Y���̒l�𑫂�
	inline void pt_add(const D2D1_POINT_2F a, const double x, const double y, D2D1_POINT_2F& c) noexcept;
	// ��_�̒��_�𓾂�.
	inline void pt_avg(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// �ʒu���}�`�̕��ʂɊ܂܂�邩���肷��.
	inline bool pt_in_anc(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos) noexcept;
	// �ʒu�����~�Ɋ܂܂�邩���肷��.
	inline bool pt_in_ellipse(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept;
	// �ʒu���~�Ɋ܂܂�邩���肷��.
	inline bool pt_in_circle(const D2D1_POINT_2F t_vec, const double rad) noexcept;
	// �ʒu���~�Ɋ܂܂�邩���肷��.
	inline bool pt_in_circle(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad) noexcept;
	// ���p�`���ʒu���܂ނ����肷��.
	inline bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t p_cnt, const D2D1_POINT_2F p_pos[]) noexcept;
	// ���`���ʒu���܂ނ����肷��.
	inline bool pt_in_rect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept;
	// ���`���ʒu���܂ނ����肷��.
	inline bool pt_in_rect2(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept;
	// �ʒu���X�J���[�{�Ɋۂ߂�.
	inline void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& round) noexcept;
	// �ʒu�ɃX�J���[�l���|��, �ʂ̈ʒu�𑫂�.
	inline void pt_mul_add(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
	// �_�ɃX�J���[�l���|��, �ʂ̈ʒu�𑫂�.
	inline void pt_mul_add(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
	// �ʒu�ɃX�J���[�l���|����.
	inline void pt_mul(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept;
	// ���@�ɒl���|����.
	inline void pt_mul(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& c) noexcept;
	// �ʒu����ʒu������.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// �ʒu����傫��������.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& c) noexcept;
	// ������𕡐�����. ���̕����񂪃k���|�C���^�[, �܂��͌��̕������� 0 �̂Ƃ���, �k���|�C���^�[��Ԃ�.
	inline wchar_t* wchar_cpy(const wchar_t* const s) noexcept;
	// ������̒���. �������k���|�C���^�̏ꍇ, 0 ��Ԃ�.
	inline uint32_t wchar_len(const wchar_t* const t) noexcept;

	//------------------------------
	// shape_dt.cpp
	// �ǂݍ���, ��������.
	//------------------------------

	// �f�[�^���[�_�[�����邵�̐��@��ǂݍ���.
	void dt_read(ARROW_SIZE& val, /*<---*/DataReader const& dt_reader);
	// �f�[�^���[�_�[����[�̌`����ǂݍ���.
	void dt_read(CAP_STYLE& val, /*<---*/DataReader const& dt_reader);
	// �f�[�^���[�_�[����F��ǂݍ���.
	void dt_read(D2D1_COLOR_F& val, /*<---*/DataReader const& dt_reader);
	// �f�[�^���[�_�[����ʒu��ǂݍ���.
	void dt_read(D2D1_POINT_2F& val, /*<---*/DataReader const& dt_reader);
	// �f�[�^���[�_�[������`��ǂݍ���.
	void dt_read(D2D1_RECT_F& val, /*<---*/DataReader const& dt_reader);
	// �f�[�^���[�_�[���琡�@��ǂݍ���.
	void dt_read(D2D1_SIZE_F& val, /*<---*/DataReader const& dt_reader);
	// �f�[�^���[�_�[���琡�@��ǂݍ���.
	void dt_read(D2D1_SIZE_U& val, /*<---*/DataReader const& dt_reader);
	// �f�[�^���[�_�[����j���̔z�u��ǂݍ���.
	void dt_read(DASH_PATT& val, /*<---*/DataReader const& dt_reader);
	// �f�[�^���[�_�[���當���͈͂�ǂݍ���.
	void dt_read(DWRITE_TEXT_RANGE& val, /*<---*/DataReader const& dt_reader);
	// �f�[�^���[�_�[�������̌`����ǂݍ���.
	void dt_read(GRID_EMPH& val, /*<---*/DataReader const& dt_reader);
	// �f�[�^���[�_�[����ʒu�z���ǂݍ���.
	void dt_read(std::vector<D2D1_POINT_2F>& val, /*<---*/DataReader const& dt_reader);
	// �f�[�^���[�_�[���當�����ǂݍ���.
	void dt_read(wchar_t*& val, /*<---*/DataReader const& dt_reader);
	// �f�[�^���C�^�[�ɖ�邵�̐��@����������.
	void dt_write(const ARROW_SIZE& val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɒ[�̌`������������.
	void dt_write(const CAP_STYLE& val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɐF����������.
	void dt_write(const D2D1_COLOR_F& val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�Ɉʒu����������.
	void dt_write(const D2D1_POINT_2F val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɕ��`����������.
	void dt_write(const D2D1_RECT_F val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɐ��@����������.
	void dt_write(const D2D1_SIZE_F val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɐ��@����������.
	void dt_write(const D2D1_SIZE_U val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɔj���̔z�u����������.
	void dt_write(const DASH_PATT& val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɕ�����͈͂���������.
	void dt_write(const DWRITE_TEXT_RANGE val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɕ���̌`������������.
	void dt_write(const GRID_EMPH val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�Ɉʒu�z�����������.
	void dt_write(const std::vector<D2D1_POINT_2F>& val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɕ��������������.
	void dt_write(const wchar_t* val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɕ��������������.
	size_t dt_write(const char val[], DataWriter const& dt_writer);

	//-------------------------------
	// shape_svg.cpp
	//-------------------------------

	// �f�[�^���C�^�[�� SVG �Ƃ��đ������ƃV���O���o�C�g���������������.
	void svg_dt_write(const char* val, const char* name, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��ăV���O���o�C�g���������������.
	void svg_dt_write(const char* val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��đ������ƐF����������.
	void svg_dt_write(const D2D1_COLOR_F val, const char* name, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��ĐF����������.
	void svg_dt_write(const D2D1_COLOR_F val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��Ĕj���̌`���Ɣz�u����������.
	void svg_dt_write(const D2D1_DASH_STYLE style, const DASH_PATT& patt, const double width, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��Ė��߂ƈʒu����������.
	void svg_dt_write(const D2D1_POINT_2F val, const char* cmd, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��đ������ƈʒu����������.
	void svg_dt_write(const D2D1_POINT_2F val, const char* name_x, const char* name_y, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��đ������ƕ��������l����������
	void svg_dt_write(const double val, const char* name, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��ĕ�����������������.
	void svg_dt_write(const float val, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��đ������� 32 �r�b�g����������������.
	void svg_dt_write(const uint32_t val, const char* name, /*--->*/DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��ă}���`�o�C�g���������������.
	void svg_dt_write(const wchar_t val[], const uint32_t v_len, /*--->*/DataWriter const& dt_writer);

	//------------------------------
	// shape_slist.cpp
	// �}�`���X�g
	//------------------------------

	using SHAPE_LIST = std::list<struct Shape*>;

	// �}�`���X�g�̒��̕�����}�`��, ���p�ł��Ȃ����̂��������Ȃ�΂��̏��̖��𓾂�.
	bool slist_test_avaiable_font(const SHAPE_LIST& slist, wchar_t*& unavailable_font) noexcept;
	// �Ō�̐}�`�𓾂�.
	Shape* slist_back(SHAPE_LIST const& slist) noexcept;
	// �}�`���X�g��������, �܂܂��}�`��j������.
	void slist_clear(SHAPE_LIST& slist) noexcept;
	// �}�`����ޕʂɐ�����.
	void slist_count(const SHAPE_LIST& slist, uint32_t& undeleted_cnt, uint32_t& selected_cnt, uint32_t& selected_group_cnt, uint32_t& runlength_cnt, uint32_t& selected_text_cnt, uint32_t& text_cnt, uint32_t& selected_image_cnt, bool& fore_selected, bool& back_selected, bool& prev_selected) noexcept;
	// �擪����}�`�܂Ő�����.
	uint32_t slist_count(SHAPE_LIST const& slist, const Shape* s) noexcept;
	// �ŏ��̐}�`�����X�g���瓾��.
	Shape* slist_front(SHAPE_LIST const& slist) noexcept;
	// �}�`�Ɨp�����͂ޗ̈�𓾂�.
	void slist_bound_sheet(SHAPE_LIST const& slist, const D2D1_SIZE_F sh_size, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;
	// ���ׂĂ̐}�`���͂ޗ̈�����X�g���瓾��.
	void slist_bound_all(SHAPE_LIST const& slist, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;
	// �I�����ꂽ�}�`���͂ޗ̈�����X�g���瓾��.
	bool slist_bound_selected(SHAPE_LIST const& slist, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;
	// �ʒu���܂ސ}�`�Ƃ��̕��ʂ𓾂�.
	uint32_t slist_hit_test(SHAPE_LIST const& slist, const D2D1_POINT_2F t_pos, Shape*& s) noexcept;
	// �}�`��}������.
	void slist_insert(SHAPE_LIST& slist, Shape* const s_ins, const Shape* s_at) noexcept;
	// �I�����ꂽ�}�`�����������ړ�����.
	bool slist_move(SHAPE_LIST const& slist, const D2D1_POINT_2F b_vec) noexcept;
	// �}�`�̂��̎��̐}�`�𓾂�.
	Shape* slist_next(SHAPE_LIST const& slist, const Shape* s) noexcept;
	// �}�`�̂��̑O�̐}�`�𓾂�.
	Shape* slist_prev(SHAPE_LIST const& slist, const Shape* s) noexcept;
	// �f�[�^���[�_�[����}�`���X�g��ǂݍ���.
	bool slist_read(SHAPE_LIST& slist, DataReader const& dt_reader);
	// �}�`�����X�g����폜��, �폜�����}�`�̎��̐}�`�𓾂�.
	Shape* slist_remove(SHAPE_LIST& slist, const Shape* s) noexcept;
	// �I�����ꂽ�}�`�̃��X�g�𓾂�.
	template <typename T> void slist_get_selected(SHAPE_LIST const& slist, SHAPE_LIST& t_list) noexcept;
	// �f�[�^���C�^�[�ɐ}�`���X�g����������. REDUCE �Ȃ�������ꂽ�}�`�͏Ȃ�.
	template <bool REDUCE> void slist_write(const SHAPE_LIST& slist, DataWriter const& dt_writer);
	// ���X�g�̒��̐}�`�̏��Ԃ𓾂�.
	template <typename S, typename T> bool slist_match(SHAPE_LIST const& slist, S s, T& t);
	// �I������ĂȂ��}�`�̒��_�̒����� �w�肵���ʒu�ɍł��߂����_��������.
	bool slist_find_vertex_closest(const SHAPE_LIST& slist, const D2D1_POINT_2F& c_pos, const float dist, D2D1_POINT_2F& val) noexcept;

	//------------------------------
	// �}�`�̂ЂȌ^
	//------------------------------
	struct Shape {
		static float s_anc_len;	// �}�`�̕��ʂ̑傫��
		static D2D1_COLOR_F s_background_color;	// �O�i�F
		static D2D1_COLOR_F s_foreground_color;	// �w�i�F
		static winrt::com_ptr<ID2D1StrokeStyle1> m_aux_style;	// �⏕���̌`��

		// �}�`��j������.
		virtual ~Shape(void) noexcept {}	// �h���N���X������̂ŕK�v
		// �}�`��\������.
		virtual void draw(ShapeSheet const& sh) = 0;
		// ��邵�̐��@�𓾂�
		virtual bool get_arrow_size(ARROW_SIZE& /*val*/) const noexcept { return false; }
		// ��邵�̌`���𓾂�.
		virtual bool get_arrow_style(ARROW_STYLE& /*val*/) const noexcept { return false; }
		// �}�`���͂ޗ̈�𓾂�.
		virtual void get_bound(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/, D2D1_POINT_2F& /*b_min*/, D2D1_POINT_2F& /*b_max*/) const noexcept {}
		// �[�̌`���𓾂�.
		virtual bool get_stroke_cap(CAP_STYLE& /*val*/) const noexcept { return false; }
		// �p�۔��a�𓾂�.
		virtual bool get_corner_radius(D2D1_POINT_2F& /*val*/) const noexcept { return false; }
		// �j���̒[�̌`���𓾂�.
		virtual bool get_dash_cap(D2D1_CAP_STYLE& /*val*/) const noexcept { return false; }
		// �j���̔z�u�𓾂�.
		virtual bool get_dash_patt(DASH_PATT& /*val*/) const noexcept { return false; }
		// �j���̌`���𓾂�.
		virtual bool get_dash_style(D2D1_DASH_STYLE& /*val*/) const noexcept { return false; }
		// �h��Ԃ��F�𓾂�.
		virtual bool get_fill_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// ���̂̐F�𓾂�.
		virtual bool get_font_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// ���̖��𓾂�.
		virtual bool get_font_family(wchar_t*& /*val*/) const noexcept { return false; }
		// ���̂̑傫���𓾂�.
		virtual bool get_font_size(float& /*val*/) const noexcept { return false; }
		// ���̂̕��̐L�k�𓾂�.
		virtual bool get_font_stretch(DWRITE_FONT_STRETCH& /*val*/) const noexcept { return false; }
		// ���̂̎��̂𓾂�.
		virtual bool get_font_style(DWRITE_FONT_STYLE& /*val*/) const noexcept { return false; }
		// ���̂̑����𓾂�.
		virtual bool get_font_weight(DWRITE_FONT_WEIGHT& /*val*/) const noexcept { return false; }
		// ����̊�̑傫���𓾂�.
		virtual bool get_grid_base(float& /*val*/) const noexcept { return false; }
		// ����̐F�𓾂�.
		virtual bool get_grid_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// ����������𓾂�.
		virtual bool get_grid_emph(GRID_EMPH& /*val*/) const noexcept { return false; }
		// ����̕\���𓾂�.
		virtual bool get_grid_show(GRID_SHOW& /*val*/) const noexcept { return false; }
		// ����ɍ��킹��𓾂�.
		virtual bool get_grid_snap(bool& /*val*/) const noexcept { return false; }
		// �摜�̕s�����x�𓾂�.
		virtual bool get_image_opacity(float& /*val*/) const noexcept { return false; }
		// �����̌����̃}�C�^�[�����𓾂�.
		virtual bool get_join_miter_limit(float& /*val*/) const noexcept { return false; }
		// �����̌����𓾂�.
		virtual bool get_join_style(D2D1_LINE_JOIN& /*val*/) const noexcept { return false; }
		// �ߖT�̒��_��������.
		virtual bool get_pos_nearest(const D2D1_POINT_2F /*pos*/, float& /*dd*/, D2D1_POINT_2F& /*val*/) const noexcept { return false; }
		// ���ʂ̈ʒu�𓾂�.
		virtual	void get_pos_anc(const uint32_t /*anc*/, D2D1_POINT_2F&/*val*/) const noexcept {}
		// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
		virtual void get_pos_min(D2D1_POINT_2F& /*val*/) const noexcept {}
		// �J�n�ʒu�𓾂�.
		virtual bool get_pos_start(D2D1_POINT_2F& /*val*/) const noexcept { return false; }
		// �p���̐F�𓾂�.
		virtual bool get_sheet_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// �p���̊g�嗦�𓾂�.
		virtual bool get_sheet_scale(float& /*val*/) const noexcept { return false; }
		// �p���̑傫���𓾂�.
		virtual bool get_sheet_size(D2D1_SIZE_F& /*val*/) const noexcept { return false; }
		// ���g�̐F�𓾂�.
		virtual bool get_stroke_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// ���̂̑����𓾂�
		virtual bool get_stroke_width(float& /*val*/) const noexcept { return false; }
		// �i���̂��낦�𓾂�.
		virtual bool get_text_par_align(DWRITE_PARAGRAPH_ALIGNMENT& /*val*/) const noexcept { return false; }
		// ������̂��낦�𓾂�.
		virtual bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& /*val*/) const noexcept { return false; }
		// ������𓾂�.
		virtual bool get_text_content(wchar_t*& /*val*/) const noexcept { return false; }
		// �s�Ԃ𓾂�.
		virtual bool get_text_line_sp(float& /*val*/) const noexcept { return false; }
		// ������̎��̗͂]���𓾂�.
		virtual bool get_text_padding(D2D1_SIZE_F& /*val*/) const noexcept { return false; }
		// �����͈͂𓾂�
		virtual bool get_text_selected(DWRITE_TEXT_RANGE& /*val*/) const noexcept { return false; }
		// �����͈͂𓾂�
		virtual bool get_font_collection(IDWriteFontCollection** /*val*/) const noexcept { return false; }
		// ���_�𓾂�.
		virtual size_t get_verts(D2D1_POINT_2F /*v_pos*/[]) const noexcept { return 0; };
		// �ʒu���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F /*t_pos*/) const noexcept { return ANC_TYPE::ANC_SHEET; }
		// �͈͂Ɋ܂܂�邩���肷��.
		virtual bool in_area(const D2D1_POINT_2F /*area_min*/, const D2D1_POINT_2F /*area_max*/) const noexcept { return false; }
		// �������ꂽ�����肷��.
		virtual bool is_deleted(void) const noexcept { return false; }
		// �I������Ă邩���肷��.
		virtual bool is_selected(void) const noexcept { return false; }
		// ���������ړ�����.
		virtual	bool move(const D2D1_POINT_2F /*val*/) noexcept { return false; }
		// �l���邵�̐��@�Ɋi�[����.
		virtual bool set_arrow_size(const ARROW_SIZE& /*val*/) noexcept { return false; }
		// �l���邵�̌`���Ɋi�[����.
		virtual bool set_arrow_style(const ARROW_STYLE /*val*/) noexcept { return false; }
		// �l��[�̌`���Ɋi�[����.
		virtual bool set_stroke_cap(const CAP_STYLE& /*val*/) noexcept { return false; }
		// �l���p�۔��a�Ɋi�[����.
		virtual bool set_corner_radius(const D2D1_POINT_2F& /*alue*/) noexcept { return false; }
		// �l��j���̒[�̌`���Ɋi�[����.
		virtual bool set_dash_cap(const D2D1_CAP_STYLE& /*val*/) noexcept { return false; }
		// �l��j���̔z�u�Ɋi�[����.
		virtual bool set_dash_patt(const DASH_PATT& /*val*/) noexcept { return false; }
		// �l����g�̌`���Ɋi�[����.
		virtual bool set_dash_style(const D2D1_DASH_STYLE /*val*/) noexcept { return false; }
		// �l���������ꂽ������Ɋi�[����.
		virtual bool set_delete(const bool /*val*/) noexcept { return false; }
		// �l��h��Ԃ��F�Ɋi�[����.
		virtual bool set_fill_color(const D2D1_COLOR_F& /*val*/) noexcept { return false; }
		// �l�����̂̐F�Ɋi�[����.
		virtual bool set_font_color(const D2D1_COLOR_F& /*val*/) noexcept { return false; }
		// �l�����̖��Ɋi�[����.
		virtual bool set_font_family(wchar_t* const /*val*/) noexcept { return false; }
		// �l�����̂̑傫���Ɋi�[����.
		virtual bool set_font_size(const float /*val*/) noexcept { return false; }
		// �l�����̂̕��̐L�k�Ɋi�[����.
		virtual bool set_font_stretch(const DWRITE_FONT_STRETCH /*val*/) noexcept { return false; }
		// �l�����̂̎��̂Ɋi�[����.
		virtual bool set_font_style(const DWRITE_FONT_STYLE /*val*/) noexcept { return false; }
		// �l�����̂̑����Ɋi�[����.
		virtual bool set_font_weight(const DWRITE_FONT_WEIGHT /*val*/) noexcept { return false; }
		// �l�����̑傫���Ɋi�[����.
		virtual bool set_grid_base(const float /*val*/) noexcept { return false; }
		// �l�����̐F�Ɋi�[����.
		virtual bool set_grid_color(const D2D1_COLOR_F& /*val*/) noexcept { return false; }
		// �l�����̋����Ɋi�[����.
		virtual bool set_grid_emph(const GRID_EMPH& /*val*/) noexcept { return false; }
		// �l�����̕\���Ɋi�[����.
		virtual bool set_grid_show(const GRID_SHOW /*val*/) noexcept { return false; }
		// �l�����ɍ��킹��Ɋi�[����.
		virtual bool set_grid_snap(const bool /*val*/) noexcept { return false; }
		// �摜�̕s�����x�𓾂�.
		virtual bool set_image_opacity(const float /*val*/) noexcept { return false; }
		// �l������̌����̃}�C�^�[�����Ɋi�[����.
		virtual bool set_join_miter_limit(const float& /*val*/) noexcept { return false; }
		// �l������̌����Ɋi�[����.
		virtual bool set_join_style(const D2D1_LINE_JOIN& /*val*/) noexcept { return false; }
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		virtual bool set_pos_anc(const D2D1_POINT_2F /*val*/, const uint32_t /*anc*/, const float /*limit*/, const bool /*keep_aspect*/) noexcept { return false; }
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		virtual bool set_pos_start(const D2D1_POINT_2F /*val*/) noexcept { return false; }
		// �l��p���̐F�Ɋi�[����.
		virtual bool set_sheet_color(const D2D1_COLOR_F& /*val*/) noexcept { return false; }
		// �l��p���̊g�嗦�Ɋi�[����.
		virtual bool set_sheet_scale(const float /*val*/) noexcept { return false; }
		// �l��p���̑傫���Ɋi�[����.
		virtual bool set_sheet_size(const D2D1_SIZE_F /*val*/) noexcept { return false; }
		// �l��I������Ă邩����Ɋi�[����.
		virtual bool set_select(const bool /*val*/) noexcept { return false; }
		// �l����g�̐F�Ɋi�[����.
		virtual bool set_stroke_color(const D2D1_COLOR_F& /*val*/) noexcept { return false; }
		// �l�����̂̑����Ɋi�[����.
		virtual bool set_stroke_width(const float /*val*/) noexcept { return false; }
		// �l��i���̂��낦�Ɋi�[����.
		virtual bool set_text_par_align(const DWRITE_PARAGRAPH_ALIGNMENT /*val*/) noexcept { return false; }
		// �l�𕶎���̂��낦�Ɋi�[����.
		virtual bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT /*val*/) noexcept { return false; }
		// �l�𕶎���Ɋi�[����.
		virtual bool set_text_content(wchar_t* const /*val*/) noexcept { return false; }
		// �l���s�ԂɊi�[����.
		virtual bool set_text_line_sp(const float /*val*/) noexcept { return false; }
		// �l�𕶎���̗]���Ɋi�[����.
		virtual bool set_text_padding(const D2D1_SIZE_F /*val*/) noexcept { return false; }
		// �l�𕶎��͈͂Ɋi�[����.
		virtual bool set_text_selected(const DWRITE_TEXT_RANGE /*val*/) noexcept { return false; }
		// �}�`���f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& /*dt_writer*/) const {}
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		virtual size_t pdf_write(const ShapeSheet& /*sheet*/, DataWriter const& /*dt_writer*/) const { return 0; }
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void svg_write(DataWriter const& /*dt_writer*/) const {}
	};

	//------------------------------
	// �I���t���O
	//------------------------------
	struct ShapeSelect : Shape {
		bool m_is_deleted = false;	// �������ꂽ������
		bool m_is_selected = false;	// �I�����ꂽ������

		// �}�`��\������.
		virtual void draw(ShapeSheet const& sh) = 0;
		// �������ꂽ�����肷��.
		bool is_deleted(void) const noexcept final override { return m_is_deleted; }
		// �I������Ă邩���肷��.
		bool is_selected(void) const noexcept final override { return m_is_selected; }
		// �l���������ꂽ������Ɋi�[����.
		bool set_delete(const bool val) noexcept final override { if (m_is_deleted != val) { m_is_deleted = val;  return true; } return false; }
		// �l��I������Ă邩����Ɋi�[����.
		bool set_select(const bool val) noexcept final override { if (m_is_selected != val) { m_is_selected = val; return true; } return false; }
		// �}�`���쐬����.
		ShapeSelect(void) {};	// �h���N���X������̂ŕK�v
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeSelect(const DataReader& dt_reader) :
			m_is_deleted(dt_reader.ReadBoolean()),
			m_is_selected(dt_reader.ReadBoolean())
			{}
	};

	//------------------------------
	// �摜
	//------------------------------
	struct ShapeImage : ShapeSelect {
		D2D1_POINT_2F m_pos;	// �n�_�̈ʒu
		D2D1_SIZE_F m_view;	// �\�����@
		D2D1_RECT_F m_clip;	// �\������Ă����`
		D2D1_SIZE_U m_orig;	// �r�b�g�}�b�v�̌���
		uint8_t* m_data = nullptr;	// �r�b�g�}�b�v�̃f�[�^
		D2D1_SIZE_F m_ratio{ 1.0, 1.0 };	// �\�����@�ƌ����̏c����
		float m_opac = 1.0f;	// �r�b�g�}�b�v�̕s�����x (�A���t�@�l�Ə�Z)
		winrt::com_ptr<ID2D1Bitmap1> m_d2d_bitmap{ nullptr };	// D2D �r�b�g�}�b�v

		int m_pdf_obj = 0;

		// �}�`��j������.
		ShapeImage::~ShapeImage(void)
		{
			if (m_data != nullptr) {
				delete m_data;
				m_data = nullptr;
			}
			if (m_d2d_bitmap != nullptr) {
				//m_d2d_bitmap->Release();
				m_d2d_bitmap = nullptr;
			}
		} // ~Shape

		//------------------------------
		// shape_image.cpp
		//------------------------------

		// �X�g���[���Ɋi�[����.
		IAsyncOperation<bool> copy_to(const winrt::guid enc_id, IRandomAccessStream& ra_stream);
		// �}�`��\������.
		void draw(ShapeSheet const& sh) final override;
		// �}�`���͂ޗ̈�𓾂�.
		void get_bound(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/, D2D1_POINT_2F& /*b_min*/, D2D1_POINT_2F& /*b_max*/) const noexcept final override;
		// �摜�̕s�����x�𓾂�.
		bool get_image_opacity(float& /*val*/) const noexcept final override;
		// �ߖT�̒��_��������.
		bool get_pos_nearest(const D2D1_POINT_2F /*pos*/, float& /*dd*/, D2D1_POINT_2F& /*val*/) const noexcept final override;
		// ���ʂ̈ʒu�𓾂�.
		void get_pos_anc(const uint32_t /*anc*/, D2D1_POINT_2F&/*val*/) const noexcept final override;
		// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
		void get_pos_min(D2D1_POINT_2F& /*val*/) const noexcept final override;
		// �J�n�ʒu�𓾂�.
		bool get_pos_start(D2D1_POINT_2F& /*val*/) const noexcept final override;
		// ���_�𓾂�.
		size_t get_verts(D2D1_POINT_2F /*v_pos*/[]) const noexcept final override;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F /*t_pos*/) const noexcept final override;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F /*area_min*/, const D2D1_POINT_2F /*area_max*/) const noexcept final override;
		// ���������ړ�����.
		bool move(const D2D1_POINT_2F val) noexcept final override;
		// ���摜�ɖ߂�.
		void revert(void) noexcept;
		// �l���摜�̕s�����x�Ɋi�[����.
		bool set_image_opacity(const float val) noexcept final override;
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept final override;
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		bool set_pos_start(const D2D1_POINT_2F /*val*/) noexcept final override;
		// �}�`���쐬����.
		ShapeImage(const D2D1_POINT_2F pos, const D2D1_SIZE_F view_size, const SoftwareBitmap& bitmap, const float opacity);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeImage(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		size_t pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� SVG �t�@�C���Ƃ��ď�������.
		void svg_write(const wchar_t f_name[], DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void svg_write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// �p��
	//------------------------------
	struct ShapeSheet : Shape {
		SHAPE_LIST m_shape_list;	// �}�`���X�g

		// ��邵
		ARROW_SIZE m_arrow_size{ ARROW_SIZE_DEFVAL };	// ��邵�̐��@
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// ��邵�̌`��

		// �p��
		D2D1_POINT_2F m_corner_rad{ GRID_LEN_DEFVAL, GRID_LEN_DEFVAL };	// �p�۔��a

		// �h��Ԃ�
		D2D1_COLOR_F m_fill_color{ COLOR_WHITE };	// �h��Ԃ��̐F

		// ����
		D2D1_COLOR_F m_font_color{ COLOR_BLACK };	// ���̂̐F
		wchar_t* m_font_family = nullptr;	// ���̖� (�V�X�e�����\�[�X�ɒl�����������ꍇ)
		float m_font_size = FONT_SIZE_DEFVAL;	// ���̂̑傫�� (�V�X�e�����\�[�X�ɒl�����������ꍇ)
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// ���̂̕��̐L�k (�V�X�e�����\�[�X�ɒl�����������ꍇ)
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// ���̂̎��� (�V�X�e�����\�[�X�ɒl�����������ꍇ)
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// ���̂̑��� (�V�X�e�����\�[�X�ɒl�����������ꍇ)

		// ���g
		D2D1_CAP_STYLE m_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// �j���̒[�̌`��
		DASH_PATT m_dash_patt{ DASH_PATT_DEFVAL };	// �j���̔z�u
		D2D1_DASH_STYLE m_dash_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// �j���̌`��
		float m_join_miter_limit = MITER_LIMIT_DEFVAL;	// ���̌����̃}�C�^�[����
		D2D1_LINE_JOIN m_join_style = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER;	// ���g�̌���
		CAP_STYLE m_stroke_cap{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };	// �[�̌`��
		D2D1_COLOR_F m_stroke_color{ COLOR_BLACK };	// ���g�̐F
		float m_stroke_width = 1.0f;	// ���g�̑���

		// ������
		float m_text_line_sp = 0.0f;	// �s�� (DIPs 96dpi�Œ�)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_par_align = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// �i���̑���
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;	// ������̑���
		D2D1_SIZE_F m_text_padding{ TEXT_MARGIN_DEFVAL };	// ������̍��E�Ə㉺�̗]��

		// �摜
		float m_image_opac = 1.0f;	// �摜�̕s�����x
		bool m_image_opac_importing = false;	// �摜���C���|�[�g����Ƃ��ɕs�����x��K�p����.

		// ����
		float m_grid_base = GRID_LEN_DEFVAL - 1.0f;	// ����̊�̑傫�� (�� -1 �����l)
		D2D1_COLOR_F m_grid_color{ GRID_COLOR_DEFVAL };	// ����̐F
		GRID_EMPH m_grid_emph{ GRID_EMPH_0 };	// ����̋���
		D2D1_POINT_2F m_grid_offset{ 0.0f, 0.0f };	// ����̃I�t�Z�b�g
		GRID_SHOW m_grid_show = GRID_SHOW::BACK;	// ����̕\��
		bool m_grid_snap = true;	// ����ɍ��킹��

		// �p��
		D2D1_COLOR_F m_sheet_color{ COLOR_WHITE };	// �w�i�F
		float m_sheet_scale = 1.0f;	// �g�嗦
		D2D1_SIZE_F	m_sheet_size{ 0.0f, 0.0f };	// �傫�� (MainPage �̃R���X�g���N�^�Őݒ�)

		D2D_UI m_d2d;	// �`���
		winrt::com_ptr<ID2D1DrawingStateBlock1> m_state_block{ nullptr };	// �`���Ԃ̕ۑ��u���b�N
		winrt::com_ptr<ID2D1SolidColorBrush> m_range_brush{ nullptr };	// �I�����ꂽ�����͈͂̐F�u���V
		winrt::com_ptr<ID2D1SolidColorBrush> m_color_brush{ nullptr };	// �}�`�̐F�u���V

		//------------------------------
		// shape_sheet.cpp
		//------------------------------

		static constexpr float size_max(void) noexcept { return 32767.0F; }
		// �}�`��\������.
		void draw(ShapeSheet const& sh) final override;
		// �Ȑ��̕⏕����\������.
		void draw_auxiliary_bezi(ShapeSheet const& sh, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// ���~�̕⏕����\������.
		void draw_auxiliary_elli(ShapeSheet const& sh, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// �����̕⏕����\������.
		void draw_auxiliary_line(ShapeSheet const& sh, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// ���`�̕⏕����\������.
		void draw_auxiliary_rect(ShapeSheet const& sh, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// ���p�`�̕⏕����\������.
		void draw_auxiliary_poly(ShapeSheet const& sh, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos, const POLY_OPTION& p_opt);
		// �p�ە��`�̕⏕����\������.
		void draw_auxiliary_rrect(ShapeSheet const& sh, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// ��邵�̐��@�𓾂�.
		bool get_arrow_size(ARROW_SIZE& val) const noexcept final override;
		// ��邵�̌`���𓾂�.
		bool get_arrow_style(ARROW_STYLE& val) const noexcept final override;
		// �[�̌`���𓾂�.
		bool get_stroke_cap(CAP_STYLE& val) const noexcept final override;
		// �p�۔��a�𓾂�.
		bool get_corner_radius(D2D1_POINT_2F& val) const noexcept final override;
		// �j���̒[�̌`���𓾂�.
		bool get_dash_cap(D2D1_CAP_STYLE& val) const noexcept final override;
		// �j���̔z�u�𓾂�.
		bool get_dash_patt(DASH_PATT& val) const noexcept final override;
		// �j���̌`���𓾂�.
		bool get_dash_style(D2D1_DASH_STYLE& val) const noexcept final override;
		// �h��Ԃ��F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// ���̂̐F�𓾂�.
		bool get_font_color(D2D1_COLOR_F& val) const noexcept final override;
		// ���̖��𓾂�.
		bool get_font_family(wchar_t*& val) const noexcept final override;
		// ���̂̑傫���𓾂�.
		bool get_font_size(float& val) const noexcept final override;
		// ���̂̕��̐L�k�𓾂�.
		bool get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept final override;
		// ���̂̎��̂𓾂�.
		bool get_font_style(DWRITE_FONT_STYLE& val) const noexcept final override;
		// ���̂̑����𓾂�.
		bool get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept final override;
		// ����̊�̑傫���𓾂�.
		bool get_grid_base(float& val) const noexcept final override;
		// ����̐F�𓾂�.
		bool get_grid_color(D2D1_COLOR_F& val) const noexcept final override;
		// ����̋����𓾂�.
		bool get_grid_emph(GRID_EMPH& val) const noexcept final override;
		// ����̕\���̏�Ԃ𓾂�.
		bool get_grid_show(GRID_SHOW& val) const noexcept final override;
		// ����ɍ��킹��𓾂�.
		bool get_grid_snap(bool& val) const noexcept final override;
		// �摜�̕s�����x�𓾂�.
		bool get_image_opacity(float& val) const noexcept final override;
		// �����̌����̃}�C�^�[�����𓾂�.
		bool get_join_miter_limit(float& val) const noexcept final override;
		// �����̌����𓾂�.
		bool get_join_style(D2D1_LINE_JOIN& val) const noexcept final override;
		// �p���̐F�𓾂�.
		bool get_sheet_color(D2D1_COLOR_F& val) const noexcept final override;
		// �p���̐F�𓾂�.
		bool get_sheet_size(D2D1_SIZE_F& val) const noexcept final override;
		// �p���̊g�嗦�𓾂�.
		bool get_sheet_scale(float& val) const noexcept final override;
		// ���g�̐F�𓾂�.
		bool get_stroke_color(D2D1_COLOR_F& val) const noexcept final override;
		// ���̂̑����𓾂�
		bool get_stroke_width(float& val) const noexcept final override;
		// �i���̂��낦�𓾂�.
		bool get_text_par_align(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept final override;
		// ������̂��낦�𓾂�.
		bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& val) const noexcept final override;
		// �s�Ԃ𓾂�.
		bool get_text_line_sp(float& val) const noexcept final override;
		// ������̎��̗͂]���𓾂�.
		bool get_text_padding(D2D1_SIZE_F& val) const noexcept final override;
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		void read(DataReader const& dt_reader);
		// �l���邵�̐��@�Ɋi�[����.
		bool set_arrow_size(const ARROW_SIZE& val) noexcept final override;
		// �l���邵�̌`���Ɋi�[����.
		bool set_arrow_style(const ARROW_STYLE val) noexcept final override;
		// �}�`�̑����l���i�[����.
		void set_attr_to(const Shape* s) noexcept;
		// �l���摜�̕s�����x�Ɋi�[����.
		bool set_image_opacity(const float val) noexcept final override;
		// �l��[�̌`���Ɋi�[����.
		bool set_stroke_cap(const CAP_STYLE& val) noexcept final override;
		// �l���p�۔��a�Ɋi�[����.
		bool set_corner_radius(const D2D1_POINT_2F& val) noexcept final override;
		// �l��j���̒[�̌`���Ɋi�[����.
		bool set_dash_cap(const D2D1_CAP_STYLE& val) noexcept final override;
		// �l��j���̔z�u�Ɋi�[����.
		bool set_dash_patt(const DASH_PATT& val) noexcept final override;
		// �l����g�̌`���Ɋi�[����.
		bool set_dash_style(const D2D1_DASH_STYLE val) noexcept final override;
		// �l��h��Ԃ��F�Ɋi�[����.
		bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// �l�����̂̐F�Ɋi�[����.
		bool set_font_color(const D2D1_COLOR_F& val) noexcept final override;
		// ���̖��Ɋi�[����.
		bool set_font_family(wchar_t* const val) noexcept final override;
		// ���̂̑傫���Ɋi�[����.
		bool set_font_size(const float val) noexcept final override;
		// �l�����̂̕��̐L�k�Ɋi�[����.
		bool set_font_stretch(const DWRITE_FONT_STRETCH val) noexcept final override;
		// �l�����̂̎��̂Ɋi�[����.
		bool set_font_style(const DWRITE_FONT_STYLE val) noexcept final override;
		// �l�����̂̑����Ɋi�[����.
		bool set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept final override;
		// �l�����̊�̑傫���Ɋi�[����.
		bool set_grid_base(const float val) noexcept final override;
		// �l�����̐F�Ɋi�[����.
		bool set_grid_color(const D2D1_COLOR_F& val) noexcept final override;
		// �l�����̋����Ɋi�[����.
		bool set_grid_emph(const GRID_EMPH& val) noexcept final override;
		// �l�����̕\���Ɋi�[����.
		bool set_grid_show(const GRID_SHOW val) noexcept final override;
		// �l�����ɍ��킹��Ɋi�[����.
		bool set_grid_snap(const bool val) noexcept final override;
		// �l������̌����̃}�C�^�[�����Ɋi�[����.
		bool set_join_miter_limit(const float& val) noexcept final override;
		// �l������̌����Ɋi�[����.
		bool set_join_style(const D2D1_LINE_JOIN& val) noexcept final override;
		// �l��p���̐F�Ɋi�[����.
		bool set_sheet_color(const D2D1_COLOR_F& val) noexcept final override;
		// �l��p���̊g�嗦�Ɋi�[����.
		bool set_sheet_scale(const float val) noexcept final override;
		// �l��p���̐��@�Ɋi�[����.
		bool set_sheet_size(const D2D1_SIZE_F val) noexcept final override;
		// �l����g�̐F�Ɋi�[����.
		bool set_stroke_color(const D2D1_COLOR_F& val) noexcept final override;
		// �l�����̂̑����Ɋi�[����.
		bool set_stroke_width(const float val) noexcept final override;
		// �l��i���̂��낦�Ɋi�[����.
		bool set_text_par_align(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept final override;
		// �l�𕶎���̂��낦�Ɋi�[����.
		bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT val) noexcept final override;
		// �l���s�ԂɊi�[����.
		bool set_text_line_sp(const float val) noexcept final override;
		// �l�𕶎���̗]���Ɋi�[����.
		bool set_text_padding(const D2D1_SIZE_F val) noexcept final override;
		// �}�`���f�[�^���[�_�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �O���[�v�}�`
	//------------------------------
	struct ShapeGroup : Shape {
		SHAPE_LIST m_list_grouped{};	// �O���[�v�����ꂽ�}�`�̃��X�g

		// �}�`���쐬����
		ShapeGroup(void) {}
		// �}�`��j������
		ShapeGroup::~ShapeGroup(void)
		{
			slist_clear(m_list_grouped);
		} // ~Shape

		//------------------------------
		// shape_group.cpp
		//------------------------------

		// �}�`��\������.
		void draw(ShapeSheet const& sh) final override;
		// �}�`���͂ޗ̈�𓾂�.
		void get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept final override;
		// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
		void get_pos_min(D2D1_POINT_2F& val) const noexcept final override;
		// �J�n�ʒu�𓾂�.
		bool get_pos_start(D2D1_POINT_2F& val) const noexcept final override;
		// ������}�`���܂ނ����肷��.
		bool has_text(void) noexcept;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept final override;
		// ��������Ă��邩���肷��.
		bool is_deleted(void) const noexcept final override { return m_list_grouped.size() == 0 || m_list_grouped.back()->is_deleted(); }
		// �I������Ă��邩���肷��.
		bool is_selected(void) const noexcept final override { return m_list_grouped.size() > 0 && m_list_grouped.back()->is_selected(); }
		// ���������ړ�����.
		bool move(const D2D1_POINT_2F val) noexcept final override;
		// �l���������ꂽ������Ɋi�[����.
		bool set_delete(const bool val) noexcept final override;
		// �l��I�����ꂽ������Ɋi�[����.
		bool set_select(const bool val) noexcept final override;
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		bool set_pos_start(const D2D1_POINT_2F val) noexcept final override;
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeGroup(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void svg_write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ���g�̂ЂȌ^
	//------------------------------
	struct ShapeStroke : ShapeSelect {
		D2D1_POINT_2F m_pos{ 0.0f, 0.0f };	// �J�n�ʒu
		std::vector<D2D1_POINT_2F> m_vec{};	// ���̈ʒu�ւ̍���
		CAP_STYLE m_stroke_cap{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };	// �[�̌`��
		D2D1_COLOR_F m_stroke_color{ COLOR_BLACK };	// ���g�̐F
		D2D1_CAP_STYLE m_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// �j���̒[�̌`��
		DASH_PATT m_dash_patt{ DASH_PATT_DEFVAL };	// �j���̔z�u
		D2D1_DASH_STYLE m_dash_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// �j���̌`��
		float m_join_miter_limit = MITER_LIMIT_DEFVAL;		// ���̌����̃}�C�^�[�����̔䗦
		D2D1_LINE_JOIN m_join_style = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;	// ���̌���
		float m_stroke_width = 1.0f;	// ���g�̑���

		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_stroke_style{};	// D2D �X�g���[�N�X�^�C��

		// �}�`��j������.
		virtual ~ShapeStroke(void)
		{
			if (m_d2d_stroke_style != nullptr) {
				//m_d2d_stroke_style->Release();
				m_d2d_stroke_style = nullptr;
			}
		} // ~Shape

		//------------------------------
		// shape_stroke.cpp
		//------------------------------

		// D2D �X�g���[�N�X�^�C�����쐬����.
		void create_stroke_style(D2D_UI const& d2d);
		// �}�`��\������.
		virtual void draw(ShapeSheet const& sh) override = 0;
		// �}�`���͂ޗ̈�𓾂�.
		void get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept final override;
		// �[�̌`���𓾂�.
		bool get_stroke_cap(CAP_STYLE& val) const noexcept final override;
		// �j���̒[�̌`���𓾂�.
		bool get_dash_cap(D2D1_CAP_STYLE& val) const noexcept final override;
		// �j���̔z�u�𓾂�.
		bool get_dash_patt(DASH_PATT& val) const noexcept final override;
		// �j���̌`���𓾂�.
		bool get_dash_style(D2D1_DASH_STYLE& val) const noexcept final override;
		// �����̌����̃}�C�^�[�����𓾂�.
		bool get_join_miter_limit(float& val) const noexcept final override;
		// �����̌����𓾂�.
		bool get_join_style(D2D1_LINE_JOIN& val) const noexcept final override;
		// �ߖT�̒��_��������.
		virtual bool get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& val) const noexcept override;
		// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
		void get_pos_min(D2D1_POINT_2F& val) const noexcept final override;
		// ���ʂ̈ʒu�𓾂�.
		virtual void get_pos_anc(const uint32_t /*anc*/, D2D1_POINT_2F& val) const noexcept override;
		// �J�n�ʒu�𓾂�.
		bool get_pos_start(D2D1_POINT_2F& val) const noexcept final override;
		// ���g�̐F�𓾂�.
		bool get_stroke_color(D2D1_COLOR_F& val) const noexcept final override;
		// ���g�̑����𓾂�.
		bool get_stroke_width(float& val) const noexcept final override;
		// ���_�𓾂�.
		virtual size_t get_verts(D2D1_POINT_2F v_pos[]) const noexcept override;
		// �ʒu���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept override;
		// �͈͂Ɋ܂܂�邩���肷��.
		virtual bool in_area(const D2D1_POINT_2F /*area_min*/, const D2D1_POINT_2F /*area_max*/) const noexcept override;
		// ���������ړ�����.
		virtual	bool move(const D2D1_POINT_2F val) noexcept override;
		// �l��[�̌`���Ɋi�[����.
		virtual	bool set_stroke_cap(const CAP_STYLE& val) noexcept override;
		// �l��j���̒[�̌`���Ɋi�[����.
		bool set_dash_cap(const D2D1_CAP_STYLE& val) noexcept final override;
		// �l��j���̔z�u�Ɋi�[����.
		bool set_dash_patt(const DASH_PATT& val) noexcept final override;
		// �l����g�̌`���Ɋi�[����.
		bool set_dash_style(const D2D1_DASH_STYLE val) noexcept final override;
		// �l������̌����̃}�C�^�[�����Ɋi�[����.
		virtual bool set_join_miter_limit(const float& val) noexcept override;
		// �l������̌����Ɋi�[����.
		virtual bool set_join_style(const D2D1_LINE_JOIN& val) noexcept override;
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		virtual bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept override;
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		virtual bool set_pos_start(const D2D1_POINT_2F val) noexcept override;
		// �l����g�̐F�Ɋi�[����.
		bool set_stroke_color(const D2D1_COLOR_F& val) noexcept;
		// �l����g�̑����Ɋi�[����.
		bool set_stroke_width(const float val) noexcept;
		// �}�`���쐬����.
		ShapeStroke(const ShapeSheet* s_sheet);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeStroke(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		size_t pdf_write_stroke(DataWriter const& /*dt_writer*/) const;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void svg_write_stroke(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ��������
	//------------------------------
	struct ShapeLine : ShapeStroke {
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// ��邵�̌`��
		ARROW_SIZE m_arrow_size{ ARROW_SIZE_DEFVAL };	// ��邵�̐��@

		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_arrow_style{ nullptr };	// ��邵�� D2D �X�g���[�N�X�^�C��
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_arrow_geom{ nullptr };	// ��邵�� D2D �p�X�W�I���g��

		// �}�`��j������.
		virtual ~ShapeLine(void)
		{
			if (m_d2d_arrow_geom != nullptr) {
				//m_d2d_arrow_geom->Release();
				m_d2d_arrow_geom = nullptr;
			}
			if (m_d2d_arrow_style != nullptr) {
				//m_d2d_arrow_style->Release();
				m_d2d_arrow_style = nullptr;
			}
		} // ~ShapeStroke

		//------------------------------
		// shape_line.cpp
		//------------------------------

		// ��邵�̐�[�ƕԂ��̈ʒu�����߂�.
		static bool line_get_arrow_pos(const D2D1_POINT_2F a_pos, const D2D1_POINT_2F a_vec, const ARROW_SIZE& a_size, D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept;

		// �}�`���쐬����.
		ShapeLine(const ShapeSheet* s_sheet, const bool a_none = false) :
			ShapeStroke(s_sheet),
			m_arrow_style(a_none ? ARROW_STYLE::NONE : s_sheet->m_arrow_style),
			m_arrow_size(s_sheet->m_arrow_size)
		{}
		// �}�`���쐬����.
		ShapeLine(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet);
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapeLine(DataReader const& dt_reader);
		// �}�`��\������.
		virtual void draw(ShapeSheet const& sh) override;
		// ��邵�̐��@�𓾂�.
		bool get_arrow_size(ARROW_SIZE& size) const noexcept final override;
		// ��邵�̌`���𓾂�.
		bool get_arrow_style(ARROW_STYLE& val) const noexcept final override;
		// �ʒu���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept override;
		// �͈͂Ɋ܂܂�邩���肷��.
		virtual bool in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept override;
		// �l���邵�̐��@�Ɋi�[����.
		virtual bool set_arrow_size(const ARROW_SIZE& val) noexcept override;
		// �l���邵�̌`���Ɋi�[����.
		virtual bool set_arrow_style(const ARROW_STYLE val) noexcept override;
		// �l��, ���ʂ̈ʒu�Ɋi�[����. 
		virtual bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept override;
		// �l���n�_�Ɋi�[����.
		virtual bool set_pos_start(const D2D1_POINT_2F val) noexcept override;
		// ���������ړ�����.
		virtual bool move(const D2D1_POINT_2F val) noexcept override;
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		virtual size_t pdf_write(const ShapeSheet& sheet, DataWriter const& /*dt_writer*/) const;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void svg_write(DataWriter const& dt_writer) const;
		// �����f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		size_t pdf_write_barbs(const ShapeSheet& sheet, const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const;
		// �����f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void svg_write_barbs(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const;
		// �l��[�̌`���Ɋi�[����.
		bool set_stroke_cap(const CAP_STYLE& val) noexcept final override;
		// �l������̌����̃}�C�^�[�����Ɋi�[����.
		bool set_join_miter_limit(const float& val) noexcept final override;
		// �l������̌����Ɋi�[����.
		bool set_join_style(const D2D1_LINE_JOIN& val) noexcept final override;
	};

	//------------------------------
	// ���`
	//------------------------------
	struct ShapeRect : ShapeStroke {
		D2D1_COLOR_F m_fill_color{ COLOR_WHITE };		// �h��Ԃ��F

		//------------------------------
		// shape_rect.cpp
		//------------------------------

		// �}�`���쐬����.
		ShapeRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet);
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapeRect(DataReader const& dt_reader);
		// �}�`��\������.
		virtual void draw(ShapeSheet const& sh) override;
		// �ߖT�̒��_��������.
		bool get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& val) const noexcept final override;
		// ���_�𓾂�.
		size_t get_verts(D2D1_POINT_2F v_pos[]) const noexcept final override;
		// �ʒu���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept override;
		// �}�`�̕��ʂ��ʒu���܂ނ����肷��.
		uint32_t hit_test_anc(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		virtual bool in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept override;
		// �h��Ԃ��̐F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// �l��h��Ԃ��̐F�Ɋi�[����.
		bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// ���ʂ̈ʒu�𓾂�.
		virtual void get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept override;
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		virtual bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept override;
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void svg_write(DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		virtual size_t pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ��K
	// �쐬�������Ƃŕ�����̑����̕ύX�͂ł��Ȃ�.
	//------------------------------
	struct ShapeRuler : ShapeRect {
		float m_grid_base = GRID_LEN_DEFVAL - 1.0f;	// ����̑傫�� (�� -1 �����l)
		wchar_t* m_font_family = nullptr;	// ���̖�
		float m_font_size = FONT_SIZE_DEFVAL;	// ���̂̑傫��

		winrt::com_ptr<IDWriteTextFormat> m_dw_text_format{};	// �e�L�X�g�t�H�[�}�b�g

		// �}�`��j������.
		ShapeRuler::~ShapeRuler(void)
		{
			if (m_dw_text_format != nullptr) {
				//m_dw_text_format->Release();
				m_dw_text_format = nullptr;
			}
		} // ~ShapeStroke

		bool get_font_collection(IDWriteFontCollection** val) const noexcept final override
		{
			winrt::check_hresult(m_dw_text_format->GetFontCollection(val));
			return true;
		}

		//------------------------------
		// shape_ruler.cpp
		//------------------------------

		// �}�`��\������.
		void draw(ShapeSheet const& sh) final override;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		// ���̖��𓾂�.
		bool get_font_family(wchar_t*& val) const noexcept final override;
		// ���̂̑傫���𓾂�.
		bool get_font_size(float& val) const noexcept final override;
		// �l�����̖��Ɋi�[����.
		bool set_font_family(wchar_t* const val) noexcept final override;
		// �l�����̂̑傫���Ɋi�[����.
		bool set_font_size(const float val) noexcept final override;
		// �}�`���쐬����.
		ShapeRuler(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeRuler(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void svg_write(DataWriter const& /*dt_writer*/) const {}
	};

	//------------------------------
	// ���~
	//------------------------------
	struct ShapeElli : ShapeRect {
		// �}�`���쐬����.
		ShapeElli(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet) :
			ShapeRect::ShapeRect(b_pos, b_vec, s_sheet)
		{}
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeElli(DataReader const& dt_reader) :
			ShapeRect::ShapeRect(dt_reader)
		{}

		//------------------------------
		// shape_elli.cpp
		//------------------------------

		// �}�`��\������.
		void draw(ShapeSheet const& sh) final override;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		size_t pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void svg_write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// �p�ە��`
	//------------------------------
	struct ShapeRRect : ShapeRect {
		D2D1_POINT_2F m_corner_rad{ GRID_LEN_DEFVAL, GRID_LEN_DEFVAL };		// �p�ە����̔��a

		//------------------------------
		// shape_rrect.cpp
		// �p�ە��`
		//------------------------------

		// �}�`��\������.
		void draw(ShapeSheet const& sh) final override;
		// �p�۔��a�𓾂�.
		bool get_corner_radius(D2D1_POINT_2F& val) const noexcept final override;
		// ���ʂ̈ʒu�𓾂�.
		void get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept final override;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		//	�l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept final override;
		// �}�`���쐬����.
		ShapeRRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeRRect(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		size_t pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const final override;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void svg_write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// �܂���̂ЂȌ^
	//------------------------------
	struct ShapePath : ShapeLine {
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_path_geom{ nullptr };	// �܂���� D2D �p�X�W�I���g��

		// �}�`���쐬����.
		ShapePath(const ShapeSheet* s_sheet, const bool s_closed) :
			ShapeLine::ShapeLine(s_sheet, s_closed)
		{}
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapePath(DataReader const& dt_reader) :
			ShapeLine::ShapeLine(dt_reader)
		{}

		// �}�`��j������.
		virtual ~ShapePath(void)
		{
			if (m_d2d_path_geom != nullptr) {
				//m_d2d_path_geom->Release();
				m_d2d_path_geom = nullptr;
			}
			// ~ShapeLine
		}

		//------------------------------
		// shape_path.cpp
		// �܂���̂ЂȌ^
		//------------------------------

		// �p�X�W�I���g�����쐬����.
		virtual void create_path_geometry(const D2D_UI& /*d2d*/) = 0;
		// ���������ړ�����.
		bool move(const D2D1_POINT_2F val) noexcept final override;
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept final override;
		// �l���邵�̐��@�Ɋi�[����.
		bool set_arrow_size(const ARROW_SIZE& val) noexcept final override;
		// �l���邵�̌`���Ɋi�[����.
		virtual bool set_arrow_style(const ARROW_STYLE val) noexcept override;
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		bool set_pos_start(const D2D1_POINT_2F val) noexcept final override;
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ���p�`
	//------------------------------
	struct ShapePoly : ShapePath {
		bool m_end_closed;	// �ӂ����Ă��邩����
		D2D1_COLOR_F m_fill_color;

		//------------------------------
		// shape_poly.cpp
		//------------------------------

		static bool poly_get_arrow_barbs(const size_t v_cnt, const D2D1_POINT_2F v_pos[], const ARROW_SIZE& a_size, D2D1_POINT_2F& h_tip, D2D1_POINT_2F h_barbs[]) noexcept;
		// �p�X�W�I���g�����쐬����.
		void create_path_geometry(const D2D_UI& d2d) final override;
		// ���`�����Ƃɑ��p�`���쐬����.
		static void create_poly_by_bbox(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const POLY_OPTION& p_opt, D2D1_POINT_2F v_pos[]/*, D2D1_POINT_2F& v_vec*/) noexcept;
		// �}�`��\������
		void draw(ShapeSheet const& sh) final override;
		// �h��Ԃ��F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		// �͈͂Ɋ܂܂�邩���肷��.
		virtual bool in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept override;
		// �l���邵�̌`���Ɋi�[����.
		bool set_arrow_style(const ARROW_STYLE val) noexcept final override;
		// �l��h��Ԃ��F�Ɋi�[����.
		bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// �}�`���쐬����.
		ShapePoly(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet, const POLY_OPTION& p_opt);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapePoly(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& /*dt_writer*/) const;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		size_t pdf_write(const ShapeSheet& sheet, DataWriter const& /*dt_writer*/) const final override;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void svg_write(DataWriter const& /*dt_writer*/) const;
	};

	//------------------------------
	// �Ȑ�
	//------------------------------
	struct ShapeBezi : ShapePath {

		//------------------------------
		// shape_bezi.cpp
		// �x�W�F�Ȑ�
		//------------------------------

		static bool bezi_calc_arrow(const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_SIZE a_size, D2D1_POINT_2F barbs[3]) noexcept;
		// �p�X�W�I���g�����쐬����.
		void create_path_geometry(const D2D_UI& d2d) final override;
		// �}�`��\������.
		void draw(ShapeSheet const& sh) final override;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept final override;
		// �}�`���쐬����.
		ShapeBezi(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeBezi(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		size_t pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void svg_write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ������
	//------------------------------
	struct ShapeText : ShapeRect {
		//static ID2D1Factory* s_d2d_factory;	//
		//static ID2D1DeviceContext2* s_d2d_context;	//
		static wchar_t** s_available_fonts;		// �L���ȏ��̖�
		static D2D1_COLOR_F s_text_selected_background;	// �����͈͂̔w�i�F
		static D2D1_COLOR_F s_text_selected_foreground;	// �����͈͂̕����F

		D2D1_COLOR_F m_font_color{ COLOR_BLACK };	// ���̂̐F
		wchar_t* m_font_family = nullptr;	// ���̖�
		float m_font_size = FONT_SIZE_DEFVAL;	// ���̂̑傫��
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// ���̂̕��̐L�k
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// ���̂̎���
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// ���̂̑���
		wchar_t* m_text = nullptr;	// ������
		float m_text_line_sp = 0.0f;	// �s�� (DIPs 96dpi�Œ�)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_par_align = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// �i���̂��낦
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;	// �����̂��낦
		D2D1_SIZE_F m_text_padding{ TEXT_MARGIN_DEFVAL };	// ������̏㉺�ƍ��E�̗]��
		DWRITE_TEXT_RANGE m_text_selected_range{ 0, 0 };	// �I�����ꂽ�����͈�

		DWRITE_FONT_METRICS m_dw_font_metrics{};
		DWRITE_LINE_METRICS* m_dw_line_metrics = nullptr;	// �s�̌v��
		UINT32 m_dw_selected_cnt = 0;	// �I�����ꂽ�����͈͂̌v�ʂ̗v�f��
		DWRITE_HIT_TEST_METRICS* m_dw_selected_metrics = nullptr;	// �I�����ꂽ�����͈͂̌v��
		UINT32 m_dw_test_cnt = 0;	// �ʒu�̌v�ʂ̗v�f��
		DWRITE_HIT_TEST_METRICS* m_dw_test_metrics = nullptr;	// �ʒu�̌v��
		winrt::com_ptr<IDWriteTextLayout> m_dw_text_layout{ nullptr };	// �����񃌃C�A�E�g

		int m_pdf_font_num = 0;	// PDF �̃t�H���g�ԍ� (PDF �o�͎��̂ݗ��p)

		// �}�`��j������.
		~ShapeText(void)
		{
			relese_metrics();

			// ���̖���j������.
			if (m_font_family != nullptr) {
				// �L���ȏ��̖��Ɋ܂܂�ĂȂ����̖��Ȃ�j������.
				if (!is_available_font(m_font_family)) {
					delete[] m_font_family;
				}
				m_font_family = nullptr;
			}
			// �������j������.
			if (m_text != nullptr) {
				delete[] m_text;
				m_text = nullptr;
			}

			// �����񃌃C�A�E�g��j������.
			if (m_dw_text_layout != nullptr) {
				//m_dw_text_layout->Release();
				m_dw_text_layout = nullptr;
			}
		} // ~ShapeStroke

		bool get_font_collection(IDWriteFontCollection** val) const noexcept final override
		{
			winrt::check_hresult(m_dw_text_layout->GetFontCollection(val));
			return true;
		}

		//------------------------------
		// shape_text.cpp
		// ������}�`
		//------------------------------

		// �g�𕶎���ɍ��킹��.
		bool frame_fit(const float g_len) noexcept;
		// �}�`��\������.
		void draw(ShapeSheet const& sh) final override;
		// ���̂̐F�𓾂�.
		bool get_font_color(D2D1_COLOR_F& val) const noexcept final override;
		// ���̖��𓾂�.
		bool get_font_family(wchar_t*& val) const noexcept final override;
		// ���̂̑傫���𓾂�.
		bool get_font_size(float& val) const noexcept final override;
		// ���̂̕��̐L�k�𓾂�.
		bool get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept final override;
		// ���̂̎��̂𓾂�.
		bool get_font_style(DWRITE_FONT_STYLE& val) const noexcept final override;
		// ���̂̑����𓾂�.
		bool get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept final override;
		// �i���̂��낦�𓾂�.
		bool get_text_par_align(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept final override;
		// ������̂��낦�𓾂�.
		bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& val) const noexcept final override;
		// ������𓾂�.
		bool get_text_content(wchar_t*& val) const noexcept final override;
		// �s�Ԃ𓾂�.
		bool get_text_line_sp(float& val) const noexcept final override;
		// ������̗]���𓾂�.
		bool get_text_padding(D2D1_SIZE_F& val) const noexcept final override;
		// �I�����ꂽ�����͈͂𓾂�.
		bool get_text_selected(DWRITE_TEXT_RANGE& val) const noexcept final override;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept final override;
		// ���̖����L�������肵, �L���Ȃ�, �����̏��̖��͔j����, �L���ȏ��̖��̔z��̗v�f�ƒu��������.
		static bool is_available_font(wchar_t*& font) noexcept;
		// �L���ȏ��̖��̔z���j������.
		static void release_available_fonts(void) noexcept;
		// �v�ʂ�j������.
		void relese_metrics(void) noexcept;
		// �L���ȏ��̖��̔z���ݒ肷��.
		static void set_available_fonts(const D2D_UI& d2d);
		// �l�����̂̐F�Ɋi�[����.
		bool set_font_color(const D2D1_COLOR_F& val) noexcept final override;
		// �l�����̖��Ɋi�[����.
		bool set_font_family(wchar_t* const val) noexcept final override;
		// �l�����̂̑傫���Ɋi�[����.
		bool set_font_size(const float val) noexcept final override;
		// �l�����̂̕��̐L�k�Ɋi�[����.
		bool set_font_stretch(const DWRITE_FONT_STRETCH val) noexcept final override;
		// �l�����̂̎��̂Ɋi�[����.
		bool set_font_style(const DWRITE_FONT_STYLE val) noexcept final override;
		// �l�����̂̑����Ɋi�[����.
		bool set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept final override;
		// �l��i���̂��낦�Ɋi�[����.
		bool set_text_par_align(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept final override;
		// �l�𕶎���̂��낦�Ɋi�[����.
		bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT val) noexcept final override;
		// �l�𕶎���Ɋi�[����.
		bool set_text_content(wchar_t* const val) noexcept final override;
		// �l���s�ԂɊi�[����.
		bool set_text_line_sp(const float val) noexcept final override;
		// �l�𕶎���̗]���Ɋi�[����.
		bool set_text_padding(const D2D1_SIZE_F val) noexcept final override;
		// �l�𕶎��͈͂Ɋi�[����.
		bool set_text_selected(const DWRITE_TEXT_RANGE val) noexcept final override;
		// �}�`���쐬����.
		ShapeText(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, wchar_t* const text, const ShapeSheet* s_sheet);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeText(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		size_t pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void svg_write(DataWriter const& dt_writer) const;
	};

	// �}�`�̕��ʁi�~�`�j��\������.
	// a_pos	���ʂ̈ʒu
	// sh	�\������p��
	inline void anc_draw_ellipse(const D2D1_POINT_2F a_pos, const ShapeSheet& sh)
	{
		const FLOAT r = static_cast<FLOAT>(Shape::s_anc_len * 0.5 + 1.0);	// ���a
		sh.m_color_brush->SetColor(Shape::s_background_color);
		sh.m_d2d.m_d2d_context->FillEllipse(D2D1_ELLIPSE{ a_pos, r, r }, sh.m_color_brush.get());
		sh.m_color_brush->SetColor(Shape::s_foreground_color);
		sh.m_d2d.m_d2d_context->FillEllipse(D2D1_ELLIPSE{ a_pos, r - 1.0f, r - 1.0f }, sh.m_color_brush.get());
	}

	// �}�`�̕��� (���`) ��\������.
	// a_pos	���ʂ̈ʒu
	// sh	�\������p��
	inline void anc_draw_rect(const D2D1_POINT_2F a_pos, const ShapeSheet& sh)
	{
		D2D1_POINT_2F r_min;
		D2D1_POINT_2F r_max;
		pt_add(a_pos, -0.5 * Shape::s_anc_len, r_min);
		pt_add(r_min, Shape::s_anc_len, r_max);
		const D2D1_RECT_F rect{ r_min.x, r_min.y, r_max.x, r_max.y };
		sh.m_color_brush->SetColor(Shape::s_background_color);
		sh.m_d2d.m_d2d_context->DrawRectangle(rect, sh.m_color_brush.get(), 2.0, nullptr);
		sh.m_color_brush->SetColor(Shape::s_foreground_color);
		sh.m_d2d.m_d2d_context->FillRectangle(rect, sh.m_color_brush.get());
	}

	// ��邵�̐��@�����������肷��.
	inline bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept
	{
		return equal(a.m_width, b.m_width) && equal(a.m_length, b.m_length) && equal(a.m_offset, b.m_offset);
	}

	// �[�̌`�������������肷��.
	inline bool equal(const CAP_STYLE& a, const CAP_STYLE& b) noexcept
	{
		return a.m_start == b.m_start && a.m_end == b.m_end;
	}

	// �F�����������肷��.
	inline bool equal(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) noexcept
	{
		return equal_color_comp(a.a, b.a) && equal_color_comp(a.r, b.r) && equal_color_comp(a.g, b.g) && equal_color_comp(a.b, b.b);
	}

	// �ʒu�����������肷��.
	inline bool equal(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept
	{
		return equal(a.x, b.x) && equal(a.y, b.y);
	}

	// ���`�����������肷��.
	inline bool equal(const D2D1_RECT_F& a, const D2D1_RECT_F& b) noexcept
	{
		return equal(a.left, b.left) && equal(a.top, b.top) && equal(a.right, b.right) && equal(a.bottom, b.bottom);
	}

	// ���@�����������肷��.
	inline bool equal(const D2D1_SIZE_F a, const D2D1_SIZE_F b) noexcept
	{
		return equal(a.width, b.width) && equal(a.height, b.height);
	}

	// �{���x�������������������肷��.
	inline bool equal(const double a, const double b) noexcept
	{
		return fabs(a - b) <= FLT_EPSILON * fmax(1.0, fmax(fabs(a), fabs(b)));
	}

	// �����͈͂����������肷��.
	inline bool equal(const DWRITE_TEXT_RANGE a, const DWRITE_TEXT_RANGE b) noexcept
	{
		return a.startPosition == b.startPosition && a.length == b.length;
	}

	// �P���x�������������������肷��.
	inline bool equal(const float a, const float b) noexcept
	{
		return fabs(a - b) <= FLT_EPSILON * fmax(1.0f, fmax(fabs(a), fabs(b)));
	}

	// ����̌`�������������肷��.
	inline bool equal(const GRID_EMPH& a, const GRID_EMPH& b) noexcept
	{
		return a.m_gauge_1 == b.m_gauge_1 && a.m_gauge_2 == b.m_gauge_2;
	}

	// �j���̔z�u�����������肷��.
	inline bool equal(const DASH_PATT& a, const DASH_PATT& b) noexcept
	{
		return equal(a.m_[0], b.m_[0]) && equal(a.m_[1], b.m_[1]) && equal(a.m_[2], b.m_[2]) && equal(a.m_[3], b.m_[3]) && equal(a.m_[4], b.m_[4]) && equal(a.m_[5], b.m_[5]);
	}

	// �����񂪓��������肷��.
	inline bool equal(const wchar_t* a, const wchar_t* b) noexcept
	{
		return a == b || (a != nullptr && b != nullptr && wcscmp(a, b) == 0);
	}

	// �����񂪓��������肷��.
	inline bool equal(winrt::hstring const& a, const wchar_t* b) noexcept
	{
		return a == (b == nullptr ? L"" : b);
	}

	// �F�̐��������������肷��.
	inline bool equal_color_comp(const FLOAT a, const FLOAT b) noexcept
	{
		return fabs(b - a) * 128.0f < 1.0f;
	}

	// ��邵�̕Ԃ��̈ʒu�����߂�.
	// a_vec	��x�N�g��.
	// a_len	��x�N�g���̒���
	// h_width	��邵�̕� (�Ԃ��̊Ԃ̒���)
	// h_len	��邵�̒��� (��[����Ԃ��܂ł̎��x�N�g����ł̒���)
	// barbs[2]	�v�Z���ꂽ��邵�̕Ԃ��̈ʒu (��[����̃I�t�Z�b�g)
	inline void get_arrow_barbs(const D2D1_POINT_2F a_vec, const double a_len, const double h_width, const double h_len, D2D1_POINT_2F barbs[]) noexcept
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

	// �F���s���������肷��.
	// val	�F
	// �߂�l	�s�����Ȃ�� true, �����Ȃ�� false.
	inline bool is_opaque(const D2D1_COLOR_F& val) noexcept
	{
		const uint32_t a = static_cast<uint32_t>(round(val.a * 255.0f));
		return (a & 0xff) > 0;
	}

	// �x�N�g���̒��� (�̎���l) �𓾂�
	// a	�x�N�g��
	// �߂�l	���� (�̎���l) 
	inline double pt_abs2(const D2D1_POINT_2F a) noexcept
	{
		const double ax = a.x;
		const double ay = a.y;
		return ax * ax + ay * ay;
	}

	// �ʒu���ʒu�ɑ���
	inline void pt_add(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x + b.x;
		c.y = a.y + b.y;
	}

	// �X�J���[�l���ʒu�ɑ���
	inline void pt_add(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(a.x + b);
		c.y = static_cast<FLOAT>(a.y + b);
	}

	// 2 �̒l���ʒu�ɑ���
	inline void pt_add(const D2D1_POINT_2F a, const double x, const double y, D2D1_POINT_2F& b) noexcept
	{
		b.x = static_cast<FLOAT>(a.x + x);
		b.y = static_cast<FLOAT>(a.y + y);
	}

	// ��_�Ԃ̒��_�����߂�.
	inline void pt_avg(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>((a.x + b.x) * 0.5);
		c.y = static_cast<FLOAT>((a.y + b.y) * 0.5);
	}

	// �}�`�̕��ʂ��ʒu���܂ނ����肷��.
	inline bool pt_in_anc(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos) noexcept
	{
		const double a = Shape::s_anc_len * 0.5;
		const double dx = static_cast<double>(t_pos.x) - static_cast<double>(a_pos.x);
		const double dy = static_cast<double>(t_pos.y) - static_cast<double>(a_pos.y);
		return -a <= dx && dx <= a && -a <= dy && dy <= a;
	}

	// �ʒu���~�Ɋ܂܂�邩���肷��.
	inline bool pt_in_circle(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double r) noexcept
	{
		const double dx = static_cast<double>(t_pos.x) - static_cast<double>(c_pos.x);
		const double dy = static_cast<double>(t_pos.y) - static_cast<double>(c_pos.y);
		return dx * dx + dy * dy <= r * r;
	}

	// �ʒu���~�Ɋ܂܂�邩���肷��.
	inline bool pt_in_circle(const D2D1_POINT_2F t_vec, const double r) noexcept
	{
		const double dx = t_vec.x;
		const double dy = t_vec.y;
		return dx * dx + dy * dy <= r * r;
	}

	// ���~�ɂ��ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// c_pos	���~�̒��S
	// rad	���~�̌a
	// �߂�l	�܂ޏꍇ true
	inline bool pt_in_ellipse(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept
	{
		const double dx = static_cast<double>(t_pos.x) - c_pos.x;
		const double dy = static_cast<double>(t_pos.y) - c_pos.y;
		const double xx = rad_x * rad_x;
		const double yy = rad_y * rad_y;
		return dx * dx * yy + dy * dy * xx <= xx * yy;
	}

	// ���p�`���ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// v_cnt	���_�̐�
	// v_pos	���_�̔z�� [v_cnt]
	// �߂�l	�܂ޏꍇ true
	// ���p�`�̊e�ӂ�, �w�肳�ꂽ�_���J�n�_�Ƃ��鐅�������������鐔�����߂�.
	inline bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t v_cnt, const D2D1_POINT_2F v_pos[]) noexcept
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
	// r_min	���`�̍���ʒu
	// r_max	���`�̉E���ʒu
	// �߂�l	�܂ޏꍇ true
	inline bool pt_in_rect2(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept
	{
		return r_min.x <= t_pos.x && t_pos.x <= r_max.x && r_min.y <= t_pos.y && t_pos.y <= r_max.y;
	}

	// ���`���ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// r_min	���`�̂����ꂩ�̒��_
	// r_max	r_min �ɑ΂��đΊp�ɂ��钸�_
	// �߂�l	�܂ޏꍇ true
	inline bool pt_in_rect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept
	{
		const double min_x = r_min.x < r_max.x ? r_min.x : r_max.x;
		const double max_x = r_min.x < r_max.x ? r_max.x : r_min.x;
		const double min_y = r_min.y < r_max.y ? r_min.y : r_max.y;
		const double max_y = r_min.y < r_max.y ? r_max.y : r_min.y;
		return min_x <= t_pos.x && t_pos.x <= max_x && min_y <= t_pos.y && t_pos.y <= max_y;
	}

	// �ʒu�ɃX�J���[���|����, �ʒu��������.
	// a	�ʒu
	// b	�X�J���[�l
	// c	������ʒu
	// d	����
	inline void pt_mul_add(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.x * b + c.x);
		d.y = static_cast<FLOAT>(a.y * b + c.y);
	}

	// �ʒu�ɃX�J���[���|����.
	// a	�ʒu
	// b	�X�J���[�l
	// c	����
	inline void pt_mul(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(a.x * b);
		c.y = static_cast<FLOAT>(a.y * b);
	}

	// ���@�ɃX�J���[�l���|����.
	// a	���@
	// b	�X�J���[�l
	// c	����
	inline void pt_mul(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& c) noexcept
	{
		c.width = static_cast<FLOAT>(a.width * b);
		c.height = static_cast<FLOAT>(a.height * b);
	}

	// �_�ɃX�J���[���|����, �ʒu��������.
	// a	�ʒu
	// b	�X�J���[�l
	// c	������ʒu
	// d	����
	inline void pt_mul_add(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.X * b + c.x);
		d.y = static_cast<FLOAT>(a.Y * b + c.y);
	}

	// �ʒu���X�J���[�{�Ɋۂ߂�.
	// a	�ʒu
	// b	�X�J���[�l
	// c	����
	inline void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(std::round(a.x / b) * b);
		c.y = static_cast<FLOAT>(std::round(a.y / b) * b);
	}

	// �ʒu����ʒu������.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.x;
		c.y = a.y - b.y;
	}

	// �ʒu����傫��������.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.width;
		c.y = a.y - b.height;
	}

	// ������̒���.
	// �������k���|�C���^�̏ꍇ, 0 ��Ԃ�.
	inline uint32_t wchar_len(const wchar_t* const t) noexcept
	{
		return (t == nullptr || t[0] == '\0') ? 0 : static_cast<uint32_t>(wcslen(t));
	}

	// ������𕡐�����.
	// ���̕����񂪃k���|�C���^�[, �܂��͌��̕������� 0 �̂Ƃ���,
	// �k���|�C���^�[��Ԃ�.
	inline wchar_t* wchar_cpy(const wchar_t* const s) noexcept
	{
		const size_t s_len = (s == nullptr || s[0] == '\0') ? 0 : static_cast<uint32_t>(wcslen(s));
		if (s_len > 0) {
			wchar_t* t;
			wcscpy_s(t = new wchar_t[s_len + 1], s_len + 1, s);
			return t;
		}
		return nullptr;
	}

}