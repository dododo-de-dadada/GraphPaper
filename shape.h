#pragma once
//------------------------------
// shape.h
// shape.cpp	�}�`�̂ЂȌ^
// shape_bezier.cpp	�x�W�F�Ȑ�
// shape_ellipse.cpp	���~
// shape_group.cpp	�O���[�v
// shape_image.cpp	�摜
// shape_line.cpp	���� (��邵��)
// shape_path.cpp	�܂���̂ЂȌ^
// shape_pdf.cpp	PDF �ւ̏�������
// shape_poly.cpp	���p�`
// shape.rect.cpp	���`
// shape_rrect.cpp	�p�ە��`
// shape_ruler.cpp	��K
// shape_page.cpp	�y�[�W
// shape_slist.cpp	�}�`���X�g
// shape_stroke.cpp	���g�̂ЂȌ^
// shape_svg.cpp	SVG �ւ̏�������
// shape_text.cpp	������
//------------------------------
#include <list>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <wincodec.h>
#include "d2d_ui.h"
#include "shape_pt.h"
//
// +-------------+
// | Shape*      |
// +------+------+
//        |
//        +---------------+---------------+
//        |               |               |
// +------+------+ +------+------+ +------+------+
// | ShapeSelect*| | ShapeGroup  | | ShapePage   |
// +------+------+ +-------------+ +-------------+
//        |
//        +---------------+
//        |               |
// +------+------+ +------+------+
// | ShapeStroke*| | ShapeImage  |
// +------+------+ +-------------+
//        |
//        +-----------------------------------------------+
//        |                                               |
// +------+------+                                 +------+------+
// | ShapeArrow* |                                 | ShapeRect   |
// +------+------+                                 +------+------+
//        |                                               |
//        +---------------+                               |
//        |               |                               |
// +------+------+ +------+------+                        |
// | ShapePath*  | | ShapeLine   |                        |
// +------+------+ +------+------+                        |
//        |                                               |
//        +---------------+---------------+               +---------------+---------------+---------------+
//        |               |               |               |               |               |               |
// +------+------+ +------+------+ +------+------+ +------+------+ +------+------+ +------+------+ +------+------+
// | ShapePoly   | | ShapeBezier | | ShapeArc    | | ShapeEllipse| | ShapeRRect  | | ShapeText   | | ShapeRuler  |
// +-------------+ +-------------+ +-------------+ +-------------+ +-------------+ +-------------+ +-------------+
//
// * ����� draw=0

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::IAsyncAction;
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
	constexpr double PT_ROUND = 1.0 / 16.0;	// �ʒu���ۂ߂�Ƃ��̔{��

	constexpr D2D1_COLOR_F COLOR_ACCENT{ 0.0f, 0x78 / 255.0f, 0xD4 / 255.0f, 1.0f };	// �����͈͂̔w�i�F SystemAccentColor �ŏ㏑��
	constexpr D2D1_COLOR_F COLOR_BLACK{ 0.0f, 0.0f, 0.0f, 1.0f };	// ��
	constexpr D2D1_COLOR_F COLOR_WHITE{ 1.0f, 1.0f, 1.0f, 1.0f };	// ��
	constexpr D2D1_COLOR_F COLOR_TEXT_RANGE{ 1.0f, 1.0f, 1.0f, 1.0f };	// �����͈͂̕����F

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

	// ����
	// ���̒�܂��Ă��Ȃ����p�`�̒��_������킷����, enum struct �łȂ� enum ��p����.
	// 
	//  NW    N    NE
	//   @----@----@
	//   |         |
	// W @         @ E
	//   |         |
	//   @----@----@
	//  SW    S    SE
	//
	enum LOC_TYPE : uint32_t {
		LOC_PAGE,		// �}�`�̊O�� (���J�[�\��)
		LOC_FILL,		// �}�`�̓��� (�ړ��J�[�\��)
		LOC_STROKE,	// ���g (�ړ��J�[�\��)
		LOC_TEXT,		// ������ (�ړ��J�[�\��)
		LOC_NW,		// ���`�̍���̒��_ (�k���쓌�J�[�\��)
		LOC_SE,		// ���`�̉E���̒��_ (�k���쓌�J�[�\��)
		LOC_NE,		// ���`�̉E��̒��_ (�k���쐼�J�[�\��)
		LOC_SW,		// ���`�̍����̒��_ (�k���쐼�J�[�\��)
		LOC_NORTH,		// ���`�̏�ӂ̒��_ (�㉺�J�[�\��)
		LOC_SOUTH,		// ���`�̉��ӂ̒��_ (�㉺�J�[�\��)
		LOC_EAST,		// ���`�̍��ӂ̒��_ (���E�J�[�\��)
		LOC_WEST,		// ���`�̉E�ӂ̒��_ (���E�J�[�\��)
		LOC_R_NW,		// ����̊p�ۂ̒��S�_ (�\���J�[�\��)
		LOC_R_NE,		// �E��̊p�ۂ̒��S�_ (�\���J�[�\��)
		LOC_R_SE,		// �E���̊p�ۂ̒��S�_ (�\���J�[�\��)
		LOC_R_SW,		// �����̊p�ۂ̒��S�_ (�\���J�[�\��)
		LOC_A_CENTER,	// �~�ʂ̒��S�_
		LOC_A_START,	// �~�ʂ̎n�_
		LOC_A_END,	// �~�ʂ̏I�_
		LOC_START,	// �����̎n�_
		LOC_END,	// �����̏I�_
		LOC_P0,	// �p�X�̎n�_ (�\���J�[�\��)
	};

	// ��邵�̑傫��
	struct ARROW_SIZE {
		//           |
		//  +--- @   |   @
		//  |     \  |  /   
		// length  \ | /
		//  |       \|/
		//  +---     @   ---+
		//           |      offset
		//           +   -- +
		//        |      |
		//        +------+
		//          width
		float m_width;		// �Ԃ��̕�
		float m_length;		// ��[����Ԃ��܂ł̒���
		float m_offset;		// ��[�̈ʒu
	};
	constexpr ARROW_SIZE ARROW_SIZE_DEFVAL{ 7.0, 16.0, 0.0 };	// ��邵�̑傫���̊���l
	constexpr float ARROW_SIZE_MAX = 127.5f;	// ��邵�̊e�傫���̍ő�l

	// ��邵�̌`��
	enum struct ARROW_STYLE : uint32_t {
		ARROW_NONE,	// ��邵�Ȃ�
		ARROW_OPENED,	// �J������邵
		ARROW_FILLED	// ������邵
	};

	// �j���̔z�u
	union DASH_PAT {
		float m_[6];
	};
	constexpr DASH_PAT DASH_PAT_DEFVAL{ { 4.0F, 3.0F, 1.0F, 3.0F, 1.0F, 3.0F } };	// �j���̔z�u�̊���l

	// ����̋���
	struct GRID_EMPH {
		uint32_t m_gauge_1;	// ��������Ԋu (����1)
		uint32_t m_gauge_2;	// ��������Ԋu (����2)
	};
	constexpr GRID_EMPH GRID_EMPH_0{ 0, 0 };	// �����Ȃ� (����l)
	constexpr GRID_EMPH GRID_EMPH_2{ 2, 0 };	// 2 �Ԗڂ̐�������
	constexpr GRID_EMPH GRID_EMPH_3{ 2, 10 };	// 2 �Ԗڂ� 10 �Ԗڂ̐�������

	// ����̕\��
	enum struct GRID_SHOW : uint32_t {
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
	constexpr D2D1_COLOR_F GRID_COLOR_DEFVAL{	// ����̐F�̊���l
		COLOR_ACCENT.r, COLOR_ACCENT.g, COLOR_ACCENT.b, 192.0f / 255.0f
	};
	constexpr float GRID_LEN_DEFVAL = 48.0f;	// ����̒����̊���l
	constexpr float JOIN_MITER_LIMIT_DEFVAL = 10.0f;	// ��萧���̊���l
	constexpr D2D1_SIZE_F TEXT_PAD_DEFVAL{ FONT_SIZE_DEFVAL / 4.0, FONT_SIZE_DEFVAL / 4.0 };	// ������̗]���̊���l
	constexpr size_t N_GON_MAX = 256;	// ���p�`�̒��_�̍ő吔 (�q�b�g����ŃX�^�b�N�𗘗p���邽��, �I�[�o�[�t���[���Ȃ��悤��������)
	constexpr float PAGE_SIZE_MAX = 32768.0f;	// �ő�̃y�[�W�傫��
	constexpr D2D1_SIZE_F PAGE_SIZE_DEFVAL{ 8.0f * 96.0f, 11.0f * 96.0f };	// �y�[�W�̑傫���̊���l
	constexpr float FONT_SIZE_MAX = 512.0f;	// ���̂̑傫���̍ő�l
	constexpr D2D1_LINE_JOIN JOIN_STYLE_DEFVAL = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;

	// COM �C���^�[�t�F�C�X IMemoryBufferByteAccess ��������
	MIDL_INTERFACE("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d")
		IMemoryBufferByteAccess : IUnknown
	{
		virtual HRESULT STDMETHODCALLTYPE GetBuffer(
			BYTE * *value,
			UINT32 * capacity
			);
	};

	// ���ʁi�~�`�j��\������.
	inline void loc_draw_circle(const D2D1_POINT_2F p, ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush) noexcept;
	// ���ʁi�Ђ��^�j��\������.
	inline void loc_draw_rhombus(const D2D1_POINT_2F p, ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush) noexcept;
	// ���� (���`) ��\������.
	inline void loc_draw_square(const D2D1_POINT_2F p, ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush) noexcept;
	// ���ʂ��_���܂ނ����肷��.
	inline bool loc_hit_test(const D2D1_POINT_2F t, const D2D1_POINT_2F loc, const double len) noexcept;
	// ���� (0.0...1.0) �̐F�����𐮐� (0...255) �ɕϊ�����.
	inline uint32_t conv_color_comp(const double c) noexcept;
	// 32�r�b�g���������������肷��.
	inline bool equal(const uint32_t a, const uint32_t b) noexcept { return a == b; };
	// �P���x�������������������肷��.
	inline bool equal(const float a, const float b) noexcept;
	// �{���x�������������������肷��.
	inline bool equal(const double a, const double b) noexcept;
	// �{���x�������������������肷��.
	inline bool equal(const D2D1_CAP_STYLE a, const D2D1_CAP_STYLE b) noexcept { return a == b; };
	// �{���x�������������������肷��.
	inline bool equal(const D2D1_LINE_JOIN a, const D2D1_LINE_JOIN b) noexcept { return a == b; };
	// �{���x�������������������肷��.
	inline bool equal(const DWRITE_FONT_STRETCH a, const DWRITE_FONT_STRETCH b) noexcept { return a == b; };
	// �{���x�������������������肷��.
	inline bool equal(const DWRITE_FONT_STYLE a, const DWRITE_FONT_STYLE b) noexcept { return a == b; };
	// �{���x�������������������肷��.
	inline bool equal(const DWRITE_FONT_WEIGHT a, const DWRITE_FONT_WEIGHT b) noexcept { return a == b; };
	// �{���x�������������������肷��.
	inline bool equal(const DWRITE_PARAGRAPH_ALIGNMENT a, const DWRITE_PARAGRAPH_ALIGNMENT b) noexcept { return a == b; };
	// �{���x�������������������肷��.
	inline bool equal(const DWRITE_TEXT_ALIGNMENT a, const DWRITE_TEXT_ALIGNMENT b) noexcept { return a == b; };
	// �{���x�������������������肷��.
	inline bool equal(const D2D1_SWEEP_DIRECTION a, const D2D1_SWEEP_DIRECTION b) noexcept { return a == b; };
	// �{���x�������������������肷��.
	inline bool equal(const D2D1_DASH_STYLE a, const D2D1_DASH_STYLE b) noexcept { return a == b; };
	// �{���x�������������������肷��.
	//inline bool equal(const GRID_EMPH a, const GRID_EMPH b) noexcept { return a.m_gauge_1 == b.m_gauge_1 && a.m_gauge_2 == b.m_gauge_2; };
	// �{���x�������������������肷��.
	inline bool equal(const GRID_SHOW a, const GRID_SHOW b) noexcept { return a == b; };
	// �{���x�������������������肷��.
	inline bool equal(const D2D1_FIGURE_END a, const D2D1_FIGURE_END b) noexcept { return a == b; };
	// �{���x�������������������肷��.
	inline bool equal(const ARROW_STYLE a, const ARROW_STYLE b) noexcept { return a == b; };
	// ���l�����肷��.
	//template<typename T> inline bool equal(const T a, const T b) noexcept { return a == b; };
	// ��邵�̑傫�������������肷��.
	inline bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept;
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
	inline bool equal(const GRID_EMPH a, const GRID_EMPH b) noexcept;
	// �j���̔z�u�����������肷��.
	inline bool equal(const DASH_PAT& a, const DASH_PAT& b) noexcept;
	// ���C�h�����񂪓��������肷��.
	inline bool equal(const wchar_t* a, const wchar_t* b) noexcept;
	// winrt �����񂪓��������肷��.
	inline bool equal(winrt::hstring const& a, const wchar_t* b) noexcept;
	// �F�̐��������������肷��.
	inline bool equal_color_comp(const FLOAT a, const FLOAT b) noexcept;
	// ���̕Ԃ��̈ʒu�����߂�.
	inline void get_pos_barbs(const D2D1_POINT_2F a, const double a_len, const double width, const double len, D2D1_POINT_2F barb[]) noexcept;
	// �F���s���������肷��.
	inline bool is_opaque(const D2D1_COLOR_F& color) noexcept;
	// ������𕡐�����. ���̕����񂪃k���|�C���^�[, �܂��͌��̕������� 0 �̂Ƃ���, �k���|�C���^�[��Ԃ�.
	inline wchar_t* wchar_cpy(const wchar_t* const s) noexcept;
	// ������̒���. �������k���|�C���^�̏ꍇ, 0 ��Ԃ�.
	inline uint32_t wchar_len(const wchar_t* const t) noexcept;

	//------------------------------
	// shape_text.cpp
	//------------------------------

	// wchar_t �^�̕����� (UTF-16) �� uint32_t �^�̔z�� (UTF-32) �ɕϊ�����. 
	std::vector<uint32_t> text_utf16_to_utf32(const wchar_t* w, const size_t w_len) noexcept;
	// ���ʂ𓾂�.
	template <typename T> bool text_get_font_face(T* src, const wchar_t* family, const DWRITE_FONT_WEIGHT weight, const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_STYLE style, IDWriteFontFace3*& face) noexcept;

	//------------------------------
	// shape_rect.cpp
	//------------------------------

	// ���`�̒��_�ƒ��ԓ_�̂���, �ǂ̕��ʂ��_���܂ނ����肷��.
	uint32_t rect_loc_hit_test(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const D2D1_POINT_2F t, const double a_len) noexcept;

	//------------------------------
	// shape_slist.cpp
	//------------------------------

	using SHAPE_LIST = std::list<struct Shape*>;

	// ���X�g���̍Ō�̐}�`�𓾂�.
	Shape* slist_back(SHAPE_LIST const& slist) noexcept;
	// ���X�g���̐}�`�̋��E��`�𓾂�.
	void slist_bbox_shape(SHAPE_LIST const& slist, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept;
	// ���X�g���̑I�����ꂽ�}�`�̋��E��`�𓾂�.
	bool slist_bbox_selected(SHAPE_LIST const& slist, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept;
	// ���X�g���̕�����}�`��, ���p�ł��Ȃ����̂��������Ȃ�΂��̏��̖��𓾂�.
	bool slist_check_avaiable_font(const SHAPE_LIST& slist, wchar_t*& unavailable_font) noexcept;
	// �}�`���X�g��������, �܂܂��}�`��j������.
	void slist_clear(SHAPE_LIST& slist) noexcept;
	// �}�`����ޕʂɐ�����.
	void slist_count(
		const SHAPE_LIST& slist, uint32_t& undeleted_cnt, uint32_t& selected_cnt,
		uint32_t& selected_group_cnt, uint32_t& runlength_cnt, uint32_t& selected_text_cnt, 
		uint32_t& text_cnt, uint32_t& selected_image_cnt, uint32_t& selected_arc_cnt,
		uint32_t& selected_poly_open_cnt, uint32_t& selected_poly_close_cnt, bool& fore_selected,
		bool& back_selected, bool& prev_selected) noexcept;
	// �擪����}�`�܂Ő�����.
	uint32_t slist_count(SHAPE_LIST const& slist, const Shape* s) noexcept;
	// �ŏ��̐}�`�����X�g���瓾��.
	Shape* slist_front(SHAPE_LIST const& slist) noexcept;
	// ���X�g���̐}�`���_���܂ނ����肷��.
	uint32_t slist_hit_test(SHAPE_LIST const& slist, const D2D1_POINT_2F t, Shape*& s) noexcept;
	// ���X�g�ɐ}�`��}������.
	void slist_insert(SHAPE_LIST& slist, Shape* const s_ins, const Shape* s_at) noexcept;
	// ���X�g���̑I�����ꂽ�}�`���ړ�����
	bool slist_move_selected(SHAPE_LIST const& slist, const D2D1_POINT_2F pos) noexcept;
	// ���X�g���̐}�`�̂��̎��̐}�`�𓾂�.
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
	// �I������ĂȂ��}�`�̒��_�̒����� �w�肵���_�ɍł��߂��_��������.
	bool slist_find_vertex_closest(const SHAPE_LIST& slist, const D2D1_POINT_2F& p, const double d, D2D1_POINT_2F& val) noexcept;

	//------------------------------
	// �}�`�̂ЂȌ^
	//------------------------------
	struct Shape {
		static winrt::com_ptr<IDWriteFactory> m_dwrite_factory;	// DWrite �t�@�N�g��
		static ID2D1RenderTarget* m_d2d_target;	// �`��Ώ�
		static winrt::com_ptr<ID2D1DrawingStateBlock> m_state_block;	// �`���Ԃ�ێ�����u���b�N
		static winrt::com_ptr<ID2D1SolidColorBrush> m_d2d_color_brush;	// �F�u���V (�^�[�Q�b�g�ˑ�)
		static winrt::com_ptr<ID2D1SolidColorBrush> m_d2d_range_brush;	// �I�����ꂽ�����F�̃u���V (�^�[�Q�b�g�ˑ�)
		static winrt::com_ptr<ID2D1BitmapBrush> m_d2d_bitmap_brush;	// �w�i�̉摜�u���V (�^�[�Q�b�g�ˑ�)
		static winrt::com_ptr<ID2D1StrokeStyle1> m_aux_style;	// �⏕���̌`��
		static float m_aux_width;	// �⏕���̑���
		static bool m_loc_show;	// ���ʂ�\��/��\��
		static float m_loc_width;	// ���ʂ̑傫��
		static float m_loc_square_inner;	// �}�`�̕��� (�����`) �̓����̕ӂ̔����̒���
		static float m_loc_square_outer;	// �}�`�̕��� (�����`) �̊O���̕ӂ̔����̒���
		static float m_loc_circle_inner;	// �}�`�̕��� (�~�`) �̓����̔��a
		static float m_loc_circle_outer;	// �}�`�̕��� (�~�`) �̊O���̔��a
		static float m_loc_rhombus_inner;	// �}�`�̕��� (�Ђ��^) �̒��S��������̒��_�܂ł̔����̒���
		static float m_loc_rhombus_outer;	// �}�`�̕��� (�Ђ��^) �̒��S����O���̒��_�܂ł̔����̒���

		// �}�`��j������.
		virtual ~Shape(void) noexcept {}	// �h���N���X�̃f�X�g���N�^���ĂԂ��߂ɉ��z�����K�v.
		// �`��O�ɕK�v�ȕϐ����i�[����.
		void begin_draw(ID2D1RenderTarget* const target, const bool located, IWICFormatConverter* const background, const double scale) noexcept;
		// �}�`��\������.
		virtual void draw(void) noexcept = 0;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		virtual size_t export_pdf(const D2D1_SIZE_F/*page_size*/, DataWriter const&/*dt_writer*/) { return 0; }
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void export_svg(DataWriter const& /*dt_writer*/) noexcept {}
		// �~�ʂ̕����𓾂�
		virtual bool get_arc_dir(D2D1_SWEEP_DIRECTION&/*val*/) const noexcept { return false; }
		// �~�ʂ̏I�_�̊p�x�𓾂�.
		virtual bool get_arc_end(float&/*val*/) const noexcept { return false; }
		// �X�Γx�𓾂�.
		virtual bool get_arc_rot(float&/*val*/) const noexcept { return false; }
		// �~�ʂ̎n�_�̊p�x�𓾂�.
		virtual bool get_arc_start(float&/*val*/) const noexcept { return false; }
		// ��邵�̐��@�𓾂�
		virtual bool get_arrow_size(ARROW_SIZE&/*val*/) const noexcept { return false; }
		// ��邵�̌`���𓾂�.
		virtual bool get_arrow_style(ARROW_STYLE&/*val*/) const noexcept { return false; }
		// ��邵�̕Ԃ��̌`���𓾂�
		virtual bool get_arrow_cap(D2D1_CAP_STYLE&/*val*/) const noexcept { return false; }
		// ��邵�̐�[�̌`���𓾂�.
		virtual bool get_arrow_join(D2D1_LINE_JOIN&/*val*/) const noexcept { return false; }
		// ���E��`�𓾂�.
		virtual void get_bbox(const D2D1_POINT_2F/*a_lt*/, const D2D1_POINT_2F/*a_rb*/, D2D1_POINT_2F&/*b_lt*/, D2D1_POINT_2F&/*b_rb*/) const noexcept {}
		// ���E��`�̍���_�𓾂�.
		virtual void get_bbox_lt(D2D1_POINT_2F&/*val*/) const noexcept {}
		// �p�۔��a�𓾂�.
		virtual bool get_corner_radius(D2D1_POINT_2F& /*val*/) const noexcept { return false; }
		// �j���̒[�̌`���𓾂�.
		//virtual bool get_dash_cap(D2D1_CAP_STYLE& /*val*/) const noexcept { return false; }
		// �j���̔z�u�𓾂�.
		virtual bool get_stroke_dash_pat(DASH_PAT& /*val*/) const noexcept { return false; }
		// �j���̌`���𓾂�.
		virtual bool get_stroke_dash(D2D1_DASH_STYLE& /*val*/) const noexcept { return false; }
		// �h��Ԃ��F�𓾂�.
		virtual bool get_fill_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// ���̂̐F�𓾂�.
		virtual bool get_font_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// ���̖��𓾂�.
		virtual bool get_font_family(wchar_t*& /*val*/) const noexcept { return false; }
		// ���̂̑傫���𓾂�.
		virtual bool get_font_size(float&/*val*/) const noexcept { return false; }
		// ���̂̕��𓾂�.
		virtual bool get_font_stretch(DWRITE_FONT_STRETCH&/*val*/) const noexcept { return false; }
		// ���̂̎��̂𓾂�.
		virtual bool get_font_style(DWRITE_FONT_STYLE&/*val*/) const noexcept { return false; }
		// ���̂̑����𓾂�.
		virtual bool get_font_weight(DWRITE_FONT_WEIGHT&/*val*/) const noexcept { return false; }
		// ����̊�̑傫���𓾂�.
		virtual bool get_grid_base(float&/*val*/) const noexcept { return false; }
		// ����̐F�𓾂�.
		virtual bool get_grid_color(D2D1_COLOR_F&/*val*/) const noexcept { return false; }
		// ����������𓾂�.
		virtual bool get_grid_emph(GRID_EMPH&/*val*/) const noexcept { return false; }
		// ����̕\���𓾂�.
		virtual bool get_grid_show(GRID_SHOW&/*val*/) const noexcept { return false; }
		// �摜�̕s�����x�𓾂�.
		virtual bool get_image_opacity(float&/*val*/) const noexcept { return false; }
		// �����̌����̐�萧���𓾂�.
		virtual bool get_stroke_join_limit(float&/*val*/) const noexcept { return false; }
		// �����̌����̌`���𓾂�.
		virtual bool get_stroke_join(D2D1_LINE_JOIN&/*val*/) const noexcept { return false; }
		// ���p�`�̏I�[�̌`���𓾂�.
		virtual bool get_poly_end(D2D1_FIGURE_END& /*val*/) const noexcept { return false; }
		// �ߖT�̒��_��������.
		virtual bool get_pos_nearest(const D2D1_POINT_2F/*p*/, double&/*dd*/, D2D1_POINT_2F&/*val*/) const noexcept { return false; }
		// �w�肵�����ʂ̓_�𓾂�.
		virtual	void get_pos_loc(const uint32_t/*loc*/, D2D1_POINT_2F&/*val*/) const noexcept {}
		// �n�_�𓾂�.
		virtual bool get_pos_start(D2D1_POINT_2F&/*val*/) const noexcept { return false; }
		// �y�[�W�̐F�𓾂�.
		virtual bool get_page_color(D2D1_COLOR_F&/*val*/) const noexcept { return false; }
		// �y�[�W�̗]���𓾂�.
		virtual bool get_page_margin(D2D1_RECT_F&/*val*/) const noexcept { return false; }
		// �y�[�W�̑傫���𓾂�.
		virtual bool get_page_size(D2D1_SIZE_F&/*val*/) const noexcept { return false; }
		// �[�̌`���𓾂�.
		//virtual bool get_stroke_cap(CAP_STYLE& /*val*/) const noexcept { return false; }
		virtual bool get_stroke_cap(D2D1_CAP_STYLE& /*val*/) const noexcept { return false; }
		// ���g�̐F�𓾂�.
		virtual bool get_stroke_color(D2D1_COLOR_F&/*val*/) const noexcept { return false; }
		// ���̂̑����𓾂�
		virtual bool get_stroke_width(float&/*val*/) const noexcept { return false; }
		// �i���̂��낦�𓾂�.
		virtual bool get_text_align_vert(DWRITE_PARAGRAPH_ALIGNMENT&/*val*/) const noexcept { return false; }
		// ������̂��낦�𓾂�.
		virtual bool get_text_align_horz(DWRITE_TEXT_ALIGNMENT&/*val*/) const noexcept { return false; }
		// ������𓾂�.
		virtual bool get_text_content(wchar_t*&/*val*/) const noexcept { return false; }
		// �s�Ԃ𓾂�.
		virtual bool get_text_line_sp(float&/*val*/) const noexcept { return false; }
		// ������̎��̗͂]���𓾂�.
		virtual bool get_text_pad(D2D1_SIZE_F&/*val*/) const noexcept { return false; }
		// �����͈͂𓾂�
		virtual bool get_text_selected(DWRITE_TEXT_RANGE&/*val*/) const noexcept { return false; }
		// ���_�𓾂�.
		virtual size_t get_verts(D2D1_POINT_2F/*p*/[]) const noexcept { return 0; };
		// �}�`���_���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F/*t*/) const noexcept { return LOC_TYPE::LOC_PAGE; }
		// ��`�Ɋ܂܂�邩���肷��.
		virtual bool is_inside(const D2D1_POINT_2F/*lt*/, const D2D1_POINT_2F/*rb*/) const noexcept { return false; }
		// �������ꂽ�����肷��.
		virtual bool is_deleted(void) const noexcept { return false; }
		// �I������Ă邩���肷��.
		virtual bool is_selected(void) const noexcept { return false; }
		// ���̏I�[�����邩���肷��.
		virtual bool exist_cap(void) const noexcept { return false; }
		// ���̘A�������邩���肷��.
		virtual bool exist_join(void) const noexcept { return false; }
		// �}�`���ړ�����.
		virtual	bool move(const D2D1_POINT_2F /*pos*/) noexcept { return false; }
		// �l���~�ʂ̎n�_�̊p�x�Ɋi�[����.
		virtual bool set_arc_start(const float/* val*/) noexcept { return false; }
		// �l���~�ʂ̏I�_�̊p�x�Ɋi�[����.
		virtual bool set_arc_end(const float/* val*/) noexcept { return false; }
		// �l���~�ʂ̊p�x�Ɋi�[����.
		virtual bool set_arc_rot(const float/*val*/) noexcept { return false; }
		// �l���邵�̐��@�Ɋi�[����.
		virtual bool set_arrow_size(const ARROW_SIZE&/*val*/) noexcept { return false; }
		// �l���邵�̌`���Ɋi�[����.
		virtual bool set_arrow_style(const ARROW_STYLE/*val*/) noexcept { return false; }
		// �l���邵�̕Ԃ��̌`���Ɋi�[����.
		virtual bool set_arrow_cap(const D2D1_CAP_STYLE/*val*/) noexcept { return false; }
		// �l���邵�̐�[�̌`���Ɋi�[����.
		virtual bool set_arrow_join(const D2D1_LINE_JOIN/*val*/) noexcept { return false; }
		// �l��[�̌`���Ɋi�[����.
		virtual bool set_stroke_cap(const D2D1_CAP_STYLE&/*val*/) noexcept { return false; }
		// �l���p�۔��a�Ɋi�[����.
		virtual bool set_corner_radius(const D2D1_POINT_2F&/*alue*/) noexcept { return false; }
		// �l��j���̒[�̌`���Ɋi�[����.
		//virtual bool set_dash_cap(const D2D1_CAP_STYLE&/*val*/) noexcept { return false; }
		// �l��j���̔z�u�Ɋi�[����.
		virtual bool set_stroke_dash_pat(const DASH_PAT&/*val*/) noexcept { return false; }
		// �l����g�̌`���Ɋi�[����.
		virtual bool set_stroke_dash(const D2D1_DASH_STYLE/*val*/) noexcept { return false; }
		// �l���������ꂽ������Ɋi�[����.
		virtual bool set_delete(const bool/*val*/) noexcept { return false; }
		// �l��h��Ԃ��F�Ɋi�[����.
		virtual bool set_fill_color(const D2D1_COLOR_F&/*val*/) noexcept { return false; }
		// �l�����̂̐F�Ɋi�[����.
		virtual bool set_font_color(const D2D1_COLOR_F&/*val*/) noexcept { return false; }
		// �l�����̖��Ɋi�[����.
		virtual bool set_font_family(wchar_t* const/*val*/) noexcept { return false; }
		// �l�����̂̑傫���Ɋi�[����.
		virtual bool set_font_size(const float/*val*/) noexcept { return false; }
		// �l�����̂̕��Ɋi�[����.
		virtual bool set_font_stretch(const DWRITE_FONT_STRETCH/*val*/) noexcept { return false; }
		// �l�����̂̎��̂Ɋi�[����.
		virtual bool set_font_style(const DWRITE_FONT_STYLE/*val*/) noexcept { return false; }
		// �l�����̂̑����Ɋi�[����.
		virtual bool set_font_weight(const DWRITE_FONT_WEIGHT/*val*/) noexcept { return false; }
		// �l�����̑傫���Ɋi�[����.
		virtual bool set_grid_base(const float/*val*/) noexcept { return false; }
		// �l�����̐F�Ɋi�[����.
		virtual bool set_grid_color(const D2D1_COLOR_F&/*val*/) noexcept { return false; }
		// �l�����̋����Ɋi�[����.
		virtual bool set_grid_emph(const GRID_EMPH&/*val*/) noexcept { return false; }
		// �l�����̕\���Ɋi�[����.
		virtual bool set_grid_show(const GRID_SHOW/*val*/) noexcept { return false; }
		// �l�����ɍ��킹��Ɋi�[����.
		//virtual bool set_snap_grid(const bool/*val*/) noexcept { return false; }
		// �摜�̕s�����x�𓾂�.
		virtual bool set_image_opacity(const float/*val*/) noexcept { return false; }
		// �l����̌����̐�萧���Ɋi�[����.
		virtual bool set_stroke_join_limit(const float&/*val*/) noexcept { return false; }
		// �l����̌����̌`���Ɋi�[����.
		virtual bool set_stroke_join(const D2D1_LINE_JOIN&/*val*/) noexcept { return false; }
		// ���p�`�̏I�[�𓾂�.
		virtual bool set_poly_end(const D2D1_FIGURE_END/*val*/) noexcept { return false; }
		// �l��, �w�肵�����ʂ̓_�Ɋi�[����.
		virtual bool set_pos_loc(const D2D1_POINT_2F/*val*/, const uint32_t/*anc*/, const float/*snap_point*/, const bool/*keep_aspect*/) noexcept { return false; }
		// �l���n�_�Ɋi�[����. ���̕��ʂ̓_������.
		virtual bool set_pos_start(const D2D1_POINT_2F/*val*/) noexcept { return false; }
		// �l���y�[�W�̐F�Ɋi�[����.
		virtual bool set_page_color(const D2D1_COLOR_F&/*val*/) noexcept { return false; }
		// �y�[�W�̗]���Ɋi�[����.
		virtual bool set_page_margin(const D2D1_RECT_F&/*val*/) noexcept { return false; }
		// �l���y�[�W�{���Ɋi�[����.
		//virtual bool set_page_scale(const float/*val*/) noexcept { return false; }
		// �l���y�[�W�̑傫���Ɋi�[����.
		virtual bool set_page_size(const D2D1_SIZE_F/*val*/) noexcept { return false; }
		// �l��I������Ă邩����Ɋi�[����.
		virtual bool set_select(const bool/*val*/) noexcept { return false; }
		// �l����g�̐F�Ɋi�[����.
		virtual bool set_stroke_color(const D2D1_COLOR_F&/*val*/) noexcept { return false; }
		// �l�����̂̑����Ɋi�[����.
		virtual bool set_stroke_width(const float/*val*/) noexcept { return false; }
		// �l���~�ʂ̕����Ɋi�[����.
		virtual bool set_arc_dir(const D2D1_SWEEP_DIRECTION/*val*/) noexcept { return false; }
		// �l��i���̂��낦�Ɋi�[����.
		virtual bool set_text_align_vert(const DWRITE_PARAGRAPH_ALIGNMENT/*val*/) noexcept
		{ return false; }
		// �l�𕶎���̂��낦�Ɋi�[����.
		virtual bool set_text_align_horz(const DWRITE_TEXT_ALIGNMENT/*val*/) noexcept { return false; }
		// �l�𕶎���Ɋi�[����.
		virtual bool set_text_content(wchar_t* const/*val*/) noexcept { return false; }
		// �l���s�ԂɊi�[����.
		virtual bool set_text_line_sp(const float/*val*/) noexcept { return false; }
		// �l�𕶎���̗]���Ɋi�[����.
		virtual bool set_text_pad(const D2D1_SIZE_F/*val*/) noexcept { return false; }
		// �l�𕶎��͈͂Ɋi�[����.
		virtual bool set_text_selected(const DWRITE_TEXT_RANGE/*val*/) noexcept { return false; }
		// �}�`���f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const&/*dt_writer*/) const {}
	};

	//------------------------------
	// �I���t���O
	//------------------------------
	struct ShapeSelect : Shape {
		bool m_is_deleted = false;	// �������ꂽ������
		bool m_is_selected = false;	// �I�����ꂽ������

		// �}�`��\������.
		virtual void draw(void) noexcept = 0;
		// �������ꂽ�����肷��.
		virtual bool is_deleted(void) const noexcept final override { return m_is_deleted; }
		// �I������Ă邩���肷��.
		virtual bool is_selected(void) const noexcept final override { return m_is_selected; }
		// �l���������ꂽ������Ɋi�[����.
		virtual bool set_delete(const bool val) noexcept final override
		{
			if (m_is_deleted != val) {
				m_is_deleted = val; 
				return true; 
			}
			return false;
		}
		// �l��I������Ă邩����Ɋi�[����.
		virtual bool set_select(const bool val) noexcept final override
		{
			if (m_is_selected != val) {
				m_is_selected = val;
				return true;
			}
			return false;
		}
		// �}�`���쐬����.
		ShapeSelect(void) {};	// �h���N���X������̂ŕK�v
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeSelect(const DataReader& dt_reader)
		{
			m_is_deleted = dt_reader.ReadBoolean();
			m_is_selected = dt_reader.ReadBoolean();
		}
		// �}�`���f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& dt_writer) const override
		{
			dt_writer.WriteBoolean(m_is_deleted);
			dt_writer.WriteBoolean(m_is_selected);
		}
	};

	//------------------------------
	// �摜
	//------------------------------
	struct ShapeImage : ShapeSelect {
		static winrt::com_ptr<IWICImagingFactory2> wic_factory;	// WIC �t�@�N�g���[

		D2D1_POINT_2F m_start;	// �n�_
		D2D1_SIZE_F m_view;	// �\�����@
		D2D1_RECT_F m_clip;	// �\������Ă����`
		D2D1_SIZE_U m_orig;	// �r�b�g�}�b�v�̌���
		uint8_t* m_bgra = nullptr;	// �r�b�g�}�b�v�̃f�[�^
		D2D1_SIZE_F m_ratio{ 1.0, 1.0 };	// �\�����@�ƌ����̏c����
		float m_opac = 1.0f;	// �r�b�g�}�b�v�̕s�����x (�A���t�@�l�Ə�Z)

		winrt::com_ptr<ID2D1Bitmap1> m_d2d_bitmap{ nullptr };	// D2D �r�b�g�}�b�v

		int m_pdf_image_cnt = 0;	// �摜�I�u�W�F�N�g�̌v�� (PDF �Ƃ��ďo�͂���Ƃ��̂ݎg�p)

		// �}�`��j������.
		ShapeImage::~ShapeImage(void)
		{
			if (m_bgra != nullptr) {
				delete m_bgra;
				m_bgra = nullptr;
			}
			if (m_d2d_bitmap != nullptr) {
				m_d2d_bitmap = nullptr;
			}
		} // ~Shape

		//------------------------------
		// shape_image.cpp
		//------------------------------

		// �X�g���[���Ɋi�[����.
		template <bool CLIP>
		IAsyncOperation<bool> copy(const winrt::guid enc_id, IRandomAccessStream& ra_stream) const;
		// �}�`��\������.
		virtual void draw(void) noexcept final override;
		// ���E��`�𓾂�.
		virtual void get_bbox(const D2D1_POINT_2F /*a_lt*/, const D2D1_POINT_2F /*a_rb*/, D2D1_POINT_2F& /*b_lt*/, D2D1_POINT_2F& /*b_rb*/) const noexcept final override;
		// ���E��`�̍���_�𓾂�.
		virtual void get_bbox_lt(D2D1_POINT_2F& /*val*/) const noexcept final override;
		// �摜�̕s�����x�𓾂�.
		virtual bool get_image_opacity(float& /*val*/) const noexcept final override;
		// ��f�̐F�𓾂�.
		bool get_pixcel(const D2D1_POINT_2F p, D2D1_COLOR_F& val) const noexcept;
		// �ߖT�̒��_��������.
		virtual bool get_pos_nearest(const D2D1_POINT_2F /*p*/, double& /*dd*/, D2D1_POINT_2F& /*val*/) const noexcept final override;
		// �w�肵�����ʂ̓_�𓾂�.
		virtual void get_pos_loc(const uint32_t /*loc*/, D2D1_POINT_2F&/*val*/) const noexcept final override;
		// �J�n�_�𓾂�.
		virtual bool get_pos_start(D2D1_POINT_2F& /*val*/) const noexcept final override;
		// ���_�𓾂�.
		virtual size_t get_verts(D2D1_POINT_2F /*p*/[]) const noexcept final override;
		// �}�`���_���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F /*t*/) const noexcept final override;
		// ��`�Ɋ܂܂�邩���肷��.
		virtual bool is_inside(const D2D1_POINT_2F/*lt*/, const D2D1_POINT_2F/*rb*/) const noexcept final override;
		// �}�`���ړ�����.
		virtual bool move(const D2D1_POINT_2F pos) noexcept final override;
		// ���摜�ɖ߂�.
		void revert(void) noexcept;
		// �l���摜�̕s�����x�Ɋi�[����.
		virtual bool set_image_opacity(const float val) noexcept final override;
		// �l��, �w�肵�����ʂ̓_�Ɋi�[����.
		virtual bool set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool keep_aspect) noexcept final override;
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		virtual bool set_pos_start(const D2D1_POINT_2F /*val*/) noexcept final override;
		// �}�`���쐬����.
		ShapeImage(const D2D1_POINT_2F p, const D2D1_SIZE_F page_size, const SoftwareBitmap& bitmap, const float opacity);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeImage(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& dt_writer) const override;

		//------------------------------
		// shape_pdf.cpp
		//------------------------------

		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;

		//------------------------------
		// shape_svg.cpp
		//------------------------------

		// �}�`���f�[�^���C�^�[�� SVG �t�@�C���Ƃ��ď�������.
		winrt::Windows::Foundation::IAsyncAction export_as_svg_async(const DataWriter& dt_writer);
	};

	//------------------------------
	// �\��
	//------------------------------
	struct ShapePage : Shape {
		SHAPE_LIST m_shape_list{};	// �}�`���X�g

		// ��邵
		ARROW_SIZE m_arrow_size{ ARROW_SIZE_DEFVAL };	// ��邵�̐��@
		ARROW_STYLE m_arrow_style = ARROW_STYLE::ARROW_NONE;	// ��邵�̌`��
		D2D1_CAP_STYLE m_arrow_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// ��邵�̕Ԃ��̌`��
		D2D1_LINE_JOIN m_arrow_join = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL;	// ��邵�̐�[�̌`��
		float m_arrow_join_limit = JOIN_MITER_LIMIT_DEFVAL;	// ��邵�̐�萧��

		// �h��Ԃ�
		D2D1_COLOR_F m_fill_color{ COLOR_WHITE };	// �h��Ԃ��F

		// ����
		D2D1_COLOR_F m_font_color{ COLOR_BLACK };	// ���̂̐F
		wchar_t* m_font_family = nullptr;	// ���̖� (�V�X�e�����\�[�X�ɒl�����������ꍇ)
		float m_font_size = FONT_SIZE_DEFVAL;	// ���̂̑傫�� (�V�X�e�����\�[�X�ɒl�����������ꍇ)
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// ���̂̕� (�V�X�e�����\�[�X�ɒl�����������ꍇ)
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// ���̂̎��� (�V�X�e�����\�[�X�ɒl�����������ꍇ)
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// ���̂̑��� (�V�X�e�����\�[�X�ɒl�����������ꍇ)

		// ���E�g
		//D2D1_CAP_STYLE m_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// �j���̒[�̌`��
		DASH_PAT m_dash_pat{ DASH_PAT_DEFVAL };	// �j���̔z�u
		D2D1_DASH_STYLE m_stroke_dash = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// �j���̌`��
		float m_stroke_join_limit = JOIN_MITER_LIMIT_DEFVAL;	// ���̌����̐�萧��
		D2D1_LINE_JOIN m_stroke_join = JOIN_STYLE_DEFVAL;	// ���̌����̌`��
		//CAP_STYLE m_stroke_cap{ CAP_STYLE_FLAT };	// ���̒[�̌`��
		D2D1_CAP_STYLE m_stroke_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// ���̒[�̌`��
		D2D1_COLOR_F m_stroke_color{ COLOR_BLACK };	// ���E�g�̐F
		float m_stroke_width = 1.0f;	// ���E�g�̑���

		// ������
		float m_text_line_sp = 0.0f;	// �s�� (DIPs 96dpi�Œ�)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_vert = 	// �i���̑���
			DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		DWRITE_TEXT_ALIGNMENT m_text_align_horz = 	// ������̑���
			DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
		D2D1_SIZE_F m_text_pad{ TEXT_PAD_DEFVAL };	// ������̍��E�Ə㉺�̗]��

		// �摜
		float m_image_opac = 1.0f;	// �摜�̕s�����x

		// ����
		float m_grid_base = GRID_LEN_DEFVAL - 1.0f;	// ����̊�̑傫�� (�� -1 �����l)
		D2D1_COLOR_F m_grid_color{ GRID_COLOR_DEFVAL };	// ����̐F
		GRID_EMPH m_grid_emph{ GRID_EMPH_0 };	// ����̋���
		D2D1_POINT_2F m_grid_offset{ 0.0f, 0.0f };	// ����̃I�t�Z�b�g
		GRID_SHOW m_grid_show = GRID_SHOW::BACK;	// ����̕\��
		//bool m_snap_grid = true;	// ����ɍ��킹��

		// �y�[�W
		D2D1_COLOR_F m_page_color{ COLOR_WHITE };	// �w�i�F
		D2D1_SIZE_F	m_page_size{ PAGE_SIZE_DEFVAL };	// �傫�� (MainPage �̃R���X�g���N�^�Őݒ�)
		D2D1_RECT_F m_page_margin{ 0.0f, 0.0f, 0.0f, 0.0f };	// �y�[�W�̓��]��

		// �}�`���X�g�̍Ō�̐}�`�𓾂�.
		Shape* back() const noexcept
		{
			return m_shape_list.back();
		}

		//------------------------------
		// shape_page.cpp
		//------------------------------

		// �Ȑ��̕⏕����\������.
		void auxiliary_draw_bezi(const D2D1_POINT_2F start, const D2D1_POINT_2F pos) noexcept;
		// ���~�̕⏕����\������.
		void auxiliary_draw_elli(const D2D1_POINT_2F start, const D2D1_POINT_2F pos) noexcept;
		// �����̕⏕����\������.
		void auxiliary_draw_line(const D2D1_POINT_2F start, const D2D1_POINT_2F pos) noexcept;
		// ���`�̕⏕����\������.
		void auxiliary_draw_rect(const D2D1_POINT_2F start, const D2D1_POINT_2F pos) noexcept;
		// ���p�`�̕⏕����\������.
		void auxiliary_draw_poly(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const POLY_OPTION& p_opt) noexcept;
		// �p�ە��`�̕⏕����\������.
		void auxiliary_draw_rrect(const D2D1_POINT_2F start, const D2D1_POINT_2F pos) noexcept;
		// �l���~�̕⏕����\������.
		void auxiliary_draw_arc(const D2D1_POINT_2F start, const D2D1_POINT_2F pos) noexcept;
		// �}�`��\������.
		virtual void draw(void) noexcept final override;
		// ��邵�̐��@�𓾂�.
		virtual bool get_arrow_size(ARROW_SIZE& val) const noexcept final override;
		// ��邵�̌`���𓾂�.
		virtual bool get_arrow_style(ARROW_STYLE& val) const noexcept final override;
		// ��邵�̕Ԃ��̌`���𓾂�.
		virtual bool get_arrow_cap(D2D1_CAP_STYLE& val) const noexcept final override
		{
			val = m_arrow_cap;
			return true;
		}
		// ��邵�̐�[�̌`���𓾂�.
		virtual bool get_arrow_join(D2D1_LINE_JOIN& val) const noexcept final override
		{
			val = m_arrow_join;
			return true;
		}
		// �[�̌`���𓾂�.
		//virtual bool get_stroke_cap(CAP_STYLE& val) const noexcept final override;
		virtual bool get_stroke_cap(D2D1_CAP_STYLE& val) const noexcept final override;
		// �j���̒[�̌`���𓾂�.
		//virtual bool get_dash_cap(D2D1_CAP_STYLE& val) const noexcept final override;
		// �j���̔z�u�𓾂�.
		virtual bool get_stroke_dash_pat(DASH_PAT& val) const noexcept final override;
		// �j���̌`���𓾂�.
		virtual bool get_stroke_dash(D2D1_DASH_STYLE& val) const noexcept final override;
		// �h��Ԃ��F�𓾂�.
		virtual bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// ���̂̐F�𓾂�.
		virtual bool get_font_color(D2D1_COLOR_F& val) const noexcept final override;
		// ���̖��𓾂�.
		virtual bool get_font_family(wchar_t*& val) const noexcept final override;
		// ���̂̑傫���𓾂�.
		virtual bool get_font_size(float& val) const noexcept final override;
		// ���̂̕��𓾂�.
		virtual bool get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept final override;
		// ���̂̎��̂𓾂�.
		virtual bool get_font_style(DWRITE_FONT_STYLE& val) const noexcept final override;
		// ���̂̑����𓾂�.
		virtual bool get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept final override;
		// ����̊�̑傫���𓾂�.
		virtual bool get_grid_base(float& val) const noexcept final override;
		// ����̐F�𓾂�.
		virtual bool get_grid_color(D2D1_COLOR_F& val) const noexcept final override;
		// ����̋����𓾂�.
		virtual bool get_grid_emph(GRID_EMPH& val) const noexcept final override;
		// ����̕\���̏�Ԃ𓾂�.
		virtual bool get_grid_show(GRID_SHOW& val) const noexcept final override;
		// ����ɍ��킹��𓾂�.
		//virtual bool get_snap_grid(bool& val) const noexcept final override;
		// �摜�̕s�����x�𓾂�.
		virtual bool get_image_opacity(float& val) const noexcept final override;
		// ���̌����̐�萧���𓾂�.
		virtual bool get_stroke_join_limit(float& val) const noexcept final override;
		// ���̌����̌`���𓾂�.
		virtual bool get_stroke_join(D2D1_LINE_JOIN& val) const noexcept final override;
		// �y�[�W�̐F�𓾂�.
		virtual bool get_page_color(D2D1_COLOR_F& val) const noexcept final override;
		// �y�[�W�̑傫���𓾂�.
		virtual bool get_page_size(D2D1_SIZE_F& val) const noexcept final override;
		// �y�[�W�̗]���𓾂�.
		virtual bool get_page_margin(D2D1_RECT_F& val) const noexcept final override
		{
			val = m_page_margin;
			return true;
		}
		// ���g�̐F�𓾂�.
		virtual bool get_stroke_color(D2D1_COLOR_F& val) const noexcept final override;
		// ���̂̑����𓾂�
		virtual bool get_stroke_width(float& val) const noexcept final override;
		// �i���̂��낦�𓾂�.
		virtual bool get_text_align_vert(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept final override;
		// ������̂��낦�𓾂�.
		virtual bool get_text_align_horz(DWRITE_TEXT_ALIGNMENT& val) const noexcept final override;
		// �s�Ԃ𓾂�.
		virtual bool get_text_line_sp(float& val) const noexcept final override;
		// ������̎��̗͂]���𓾂�.
		virtual bool get_text_pad(D2D1_SIZE_F& val) const noexcept final override;
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		void read(DataReader const& dt_reader);
		// �l���邵�̕Ԃ��̌`���Ɋi�[����.
		virtual bool set_arrow_cap(const D2D1_CAP_STYLE val) noexcept final override;
		// �l���邵�̐�[�̌`���Ɋi�[����.
		virtual bool set_arrow_join(const D2D1_LINE_JOIN val) noexcept final override;
		// �l���邵�̐��@�Ɋi�[����.
		virtual bool set_arrow_size(const ARROW_SIZE& val) noexcept final override;
		// �l���邵�̌`���Ɋi�[����.
		virtual bool set_arrow_style(const ARROW_STYLE val) noexcept final override;
		// �w�肵���}�`���瑮���l���i�[����.
		void set_attr_to(const Shape* s) noexcept;
		// �l���摜�̕s�����x�Ɋi�[����.
		virtual bool set_image_opacity(const float val) noexcept final override;
		// �l��[�̌`���Ɋi�[����.
		//virtual bool set_stroke_cap(const CAP_STYLE& val) noexcept final override;
		virtual bool set_stroke_cap(const D2D1_CAP_STYLE& val) noexcept final override;
		// �l��j���̒[�̌`���Ɋi�[����.
		//virtual bool set_dash_cap(const D2D1_CAP_STYLE& val) noexcept final override;
		// �l��j���̔z�u�Ɋi�[����.
		virtual bool set_stroke_dash_pat(const DASH_PAT& val) noexcept final override;
		// �l����g�̌`���Ɋi�[����.
		virtual bool set_stroke_dash(const D2D1_DASH_STYLE val) noexcept final override;
		// �l��h��Ԃ��F�Ɋi�[����.
		virtual bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// �l�����̂̐F�Ɋi�[����.
		virtual bool set_font_color(const D2D1_COLOR_F& val) noexcept final override;
		// ���̖��Ɋi�[����.
		virtual bool set_font_family(wchar_t* const val) noexcept final override;
		// ���̂̑傫���Ɋi�[����.
		virtual bool set_font_size(const float val) noexcept final override;
		// �l�����̂̕��Ɋi�[����.
		virtual bool set_font_stretch(const DWRITE_FONT_STRETCH val) noexcept final override;
		// �l�����̂̎��̂Ɋi�[����.
		virtual bool set_font_style(const DWRITE_FONT_STYLE val) noexcept final override;
		// �l�����̂̑����Ɋi�[����.
		virtual bool set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept final override;
		// �l�����̊�̑傫���Ɋi�[����.
		virtual bool set_grid_base(const float val) noexcept final override;
		// �l�����̐F�Ɋi�[����.
		virtual bool set_grid_color(const D2D1_COLOR_F& val) noexcept final override;
		// �l�����̋����Ɋi�[����.
		virtual bool set_grid_emph(const GRID_EMPH& val) noexcept final override;
		// �l�����̕\���Ɋi�[����.
		virtual bool set_grid_show(const GRID_SHOW val) noexcept final override;
		// �l�����ɍ��킹��Ɋi�[����.
		//virtual bool set_snap_grid(const bool val) noexcept final override;
		// �l����̌����̐�萧���Ɋi�[����.
		virtual bool set_stroke_join_limit(const float& val) noexcept final override;
		// �l����̌����̌`���Ɋi�[����.
		virtual bool set_stroke_join(const D2D1_LINE_JOIN& val) noexcept final override;
		// �l���y�[�W�̐F�Ɋi�[����.
		virtual bool set_page_color(const D2D1_COLOR_F& val) noexcept final override;
		// �l���y�[�W�̗]���Ɋi�[����.
		virtual bool set_page_margin(const D2D1_RECT_F& val) noexcept final override
		{
			if (!equal(m_page_margin, val)) {
				m_page_margin = val;
				return true;
			}
			return false;
		}
		// �l���y�[�W�̔{���Ɋi�[����.
		//virtual bool set_page_scale(const float val) noexcept final override;
		// �l���y�[�W�̑傫���Ɋi�[����.
		virtual bool set_page_size(const D2D1_SIZE_F val) noexcept final override;
		// �l����g�̐F�Ɋi�[����.
		virtual bool set_stroke_color(const D2D1_COLOR_F& val) noexcept final override;
		// �l�����̂̑����Ɋi�[����.
		virtual bool set_stroke_width(const float val) noexcept final override;
		// �l��i���̂��낦�Ɋi�[����.
		virtual bool set_text_align_vert(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept final override;
		// �l�𕶎���̂��낦�Ɋi�[����.
		virtual bool set_text_align_horz(const DWRITE_TEXT_ALIGNMENT val) noexcept final override;
		// �l���s�ԂɊi�[����.
		virtual bool set_text_line_sp(const float val) noexcept final override;
		// �l�𕶎���̗]���Ɋi�[����.
		virtual bool set_text_pad(const D2D1_SIZE_F val) noexcept final override;
		// �}�`���f�[�^���[�_�[�ɏ�������.
		virtual void write(DataWriter const& dt_writer) const final override;
		size_t export_pdf_page(const D2D1_COLOR_F& background, DataWriter const& dt_writer);
		size_t export_pdf_grid(const D2D1_COLOR_F& background, DataWriter const& dt_writer);
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void export_svg(DataWriter const& dt_writer) noexcept final override;
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
		virtual void draw(void) noexcept final override;
		// ���E��`�𓾂�.
		virtual void get_bbox(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept final override;
		// ���E��`�̍���_�𓾂�.
		virtual void get_bbox_lt(D2D1_POINT_2F& val) const noexcept final override;
		// �J�n�_�𓾂�.
		virtual bool get_pos_start(D2D1_POINT_2F& val) const noexcept final override;
		// ������}�`���܂ނ����肷��.
		bool has_text(void) noexcept;
		// �}�`���_���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// ��`�Ɋ܂܂�邩���肷��.
		virtual bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept final override;
		// ��������Ă��邩���肷��.
		virtual bool is_deleted(void) const noexcept final override
		{
			return m_list_grouped.size() == 0 || m_list_grouped.back()->is_deleted();
		}
		// �I������Ă��邩���肷��.
		virtual bool is_selected(void) const noexcept final override
		{
			return m_list_grouped.size() > 0 && m_list_grouped.back()->is_selected();
		}
		// ���������ړ�����.
		virtual bool move(const D2D1_POINT_2F pos) noexcept final override;
		// �l���������ꂽ������Ɋi�[����.
		virtual bool set_delete(const bool val) noexcept final override;
		// �l��I�����ꂽ������Ɋi�[����.
		virtual bool set_select(const bool val) noexcept final override;
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		virtual bool set_pos_start(const D2D1_POINT_2F val) noexcept final override;
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeGroup(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		virtual void write(const DataWriter& dt_writer) const final override;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		IAsyncAction export_as_svg_async(const DataWriter& dt_writer);
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer) final override;
	};

	//------------------------------
	// ���g�̂ЂȌ^
	//------------------------------
	struct ShapeStroke : ShapeSelect {
		//CAP_STYLE m_stroke_cap{ CAP_STYLE_FLAT };	// ���̒[�̌`��
		D2D1_CAP_STYLE m_stroke_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// ���̒[�̌`��
		D2D1_COLOR_F m_stroke_color{ COLOR_BLACK };	// ���E�g�̐F
		float m_stroke_width = 1.0f;	// ���E�g�̑���
		DASH_PAT m_dash_pat{ DASH_PAT_DEFVAL };	// �j���̔z�u
		D2D1_DASH_STYLE m_stroke_dash = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// �j���̌`��
		float m_stroke_join_limit = JOIN_MITER_LIMIT_DEFVAL;		// ���̌����̐�萧��
		D2D1_LINE_JOIN m_stroke_join = JOIN_STYLE_DEFVAL;	// ���̌����̌`��

		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_stroke_style{};	// D2D �X�g���[�N�X�^�C��

		// �}�`��j������.
		virtual ~ShapeStroke(void)
		{
			m_d2d_stroke_style = nullptr;
		} // ~Shape

		//------------------------------
		// shape_stroke.cpp
		//------------------------------

		// D2D �X�g���[�N�X�^�C�����쐬����.
		HRESULT create_stroke_style(ID2D1Factory* const factory) noexcept;
		// �}�`��\������.
		virtual void draw(void) noexcept override = 0;
		// �[�̌`���𓾂�.
		//bool get_stroke_cap(CAP_STYLE& val) const noexcept final override;
		bool get_stroke_cap(D2D1_CAP_STYLE& val) const noexcept final override;
		// �j���̒[�̌`���𓾂�.
		//bool get_dash_cap(D2D1_CAP_STYLE& val) const noexcept final override;
		// �j���̔z�u�𓾂�.
		bool get_stroke_dash_pat(DASH_PAT& val) const noexcept final override;
		// �j���̌`���𓾂�.
		bool get_stroke_dash(D2D1_DASH_STYLE& val) const noexcept final override;
		// ���̌����̐�萧���𓾂�.
		bool get_stroke_join_limit(float& val) const noexcept final override;
		// ���̌����̌`���𓾂�.
		bool get_stroke_join(D2D1_LINE_JOIN& val) const noexcept final override;
		// ���g�̐F�𓾂�.
		bool get_stroke_color(D2D1_COLOR_F& val) const noexcept final override;
		// ���g�̑����𓾂�.
		bool get_stroke_width(float& val) const noexcept final override;
		// �}�`���_���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept override;
		// �l��[�̌`���Ɋi�[����.
		//virtual bool set_stroke_cap(const CAP_STYLE& val) noexcept override;
		virtual bool set_stroke_cap(const D2D1_CAP_STYLE& val) noexcept override;
		// �l��j���̒[�̌`���Ɋi�[����.
		//bool set_dash_cap(const D2D1_CAP_STYLE& val) noexcept final override;
		// �l��j���̔z�u�Ɋi�[����.
		bool set_stroke_dash_pat(const DASH_PAT& val) noexcept final override;
		// �l����g�̌`���Ɋi�[����.
		bool set_stroke_dash(const D2D1_DASH_STYLE val) noexcept final override;
		// �l����̌����̐�萧���Ɋi�[����.
		virtual bool set_stroke_join_limit(const float& val) noexcept override;
		// �l����̌����̌`���Ɋi�[����.
		virtual bool set_stroke_join(const D2D1_LINE_JOIN& val) noexcept override;
		// �l����g�̐F�Ɋi�[����.
		bool set_stroke_color(const D2D1_COLOR_F& val) noexcept;
		// �l����g�̑����Ɋi�[����.
		bool set_stroke_width(const float val) noexcept;
		// �}�`���쐬����.
		ShapeStroke(const Shape* prop);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeStroke(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ���`
	//------------------------------
	struct ShapeOblong : ShapeStroke {
		D2D1_POINT_2F m_start{ 0.0f, 0.0f };	// �n�_
		D2D1_POINT_2F m_pos{ 0.0f, 0.0f };	// �Ίp�_�ւ̃x�N�g��
		D2D1_COLOR_F m_fill_color{ COLOR_WHITE };		// �h��Ԃ��F

		//------------------------------
		// shape_rect.cpp
		//------------------------------

		// �}�`���쐬����.
		ShapeOblong(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop);
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapeOblong(DataReader const& dt_reader);
		// ���ʂ�\������.
		void draw_loc(void) noexcept;
		// �}�`��\������.
		virtual void draw(void) noexcept override;
		// ���E��`�𓾂�.
		void get_bbox(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept final override;
		// �ߖT�̓_��������.
		bool get_pos_nearest(const D2D1_POINT_2F p, double& dd, D2D1_POINT_2F& val) const noexcept final override;
		// ���_�𓾂�.
		size_t get_verts(D2D1_POINT_2F p[]) const noexcept final override;
		// �}�`���_���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept override;
		// ��`�Ɋ܂܂�邩���肷��.
		virtual bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept override;
		// �h��Ԃ��F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// �l��h��Ԃ��F�Ɋi�[����.
		bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// �w�肵�����ʂ̓_�𓾂�.
		virtual void get_pos_loc(const uint32_t loc, D2D1_POINT_2F& val) const noexcept override;
		// ���E��`�̍���_�𓾂�.
		void get_bbox_lt(D2D1_POINT_2F& val) const noexcept final override;
		// �J�n�_�𓾂�
		bool get_pos_start(D2D1_POINT_2F& val) const noexcept final override;
		// ���������ړ�����.
		bool move(const D2D1_POINT_2F pos) noexcept;
		// �l��, �w�肵�����ʂ̓_�Ɋi�[����.
		virtual bool set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool keep_aspect) noexcept override;
		// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
		bool set_pos_start(const D2D1_POINT_2F val) noexcept;
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void export_svg(DataWriter const& dt_writer) noexcept override;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) override;
	};

	struct ShapeRect : ShapeOblong {
		ShapeRect(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop) :
			ShapeOblong(start, pos, prop)
		{}
		ShapeRect(DataReader const& dt_reader) :
			ShapeOblong(dt_reader)
		{}
		// ���̘A�������邩���肷��.
		virtual bool exist_join(void) const noexcept final override
		{
			return true;
		}

	};

	//------------------------------
	// ��K
	//------------------------------
	struct ShapeRuler : ShapeOblong {
		float m_grid_base = GRID_LEN_DEFVAL - 1.0f;	// ����̑傫�� (�� -1 �����l)
		wchar_t* m_font_family = nullptr;	// ���̖�
		float m_font_size = FONT_SIZE_DEFVAL;	// ���̂̑傫��

		winrt::com_ptr<IDWriteTextFormat> m_dwrite_text_format{};	// �e�L�X�g�t�H�[�}�b�g
		int m_pdf_text_cnt = 0;

		// �}�`��j������.
		ShapeRuler::~ShapeRuler(void)
		{
			m_dwrite_text_format = nullptr;
		} // ~ShapeStroke

		//------------------------------
		// shape_ruler.cpp
		//------------------------------

		// �����񃌃C�A�E�g���쐬����.
		HRESULT create_text_format(void) noexcept;
		// �}�`��\������.
		virtual void draw(void) noexcept final override;
		// �}�`���_���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// ���ʂ𓾂� (�g�p��� Release ����).
		bool get_font_face(IDWriteFontFace3*& face) const noexcept;
		// ���̖��𓾂�.
		bool get_font_family(wchar_t*& val) const noexcept final override;
		// ���̂̑傫���𓾂�.
		bool get_font_size(float& val) const noexcept final override;
		// �l�����̖��Ɋi�[����.
		bool set_font_family(wchar_t* const val) noexcept final override;
		// �l�����̂̑傫���Ɋi�[����.
		bool set_font_size(const float val) noexcept final override;
		// �}�`���쐬����.
		ShapeRuler(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeRuler(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(const DataWriter& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void export_svg(const DataWriter& dt_writer) noexcept final override;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		size_t export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer) final override;
	};

	//------------------------------
	// ���~
	//------------------------------
	struct ShapeEllipse : ShapeOblong {
		// �}�`���쐬����.
		ShapeEllipse(const D2D1_POINT_2F start,	// �n�_
			const D2D1_POINT_2F pos,	// �I�_�ւ̈ʒu�x�N�g��
			const Shape* prop	// ����
		) :
			ShapeOblong::ShapeOblong(start, pos, prop)
		{}
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeEllipse(DataReader const& dt_reader) :
			ShapeOblong::ShapeOblong(dt_reader)
		{}

		//------------------------------
		// shape_ellipse.cpp
		//------------------------------

		// �}�`��\������.
		virtual void draw(void) noexcept final override;
		// �}�`���_���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void export_svg(DataWriter const& dt_writer) noexcept final override;
	};

	//------------------------------
	// �p�ە��`
	//------------------------------
	struct ShapeRRect : ShapeOblong {
		D2D1_POINT_2F m_corner_radius{ GRID_LEN_DEFVAL, GRID_LEN_DEFVAL };		// �p�ە����̔��a

		//------------------------------
		// shape_rrect.cpp
		// �p�ە��`
		//------------------------------

		// �}�`��\������.
		virtual void draw(void) noexcept final override;
		// �p�۔��a�𓾂�.
		bool get_corner_radius(D2D1_POINT_2F& val) const noexcept final override;
		// �w�肵�����ʂ̓_�𓾂�.
		virtual void get_pos_loc(const uint32_t loc, D2D1_POINT_2F& val) const noexcept final override;
		// �}�`���_���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// �l��, �w�肵�����ʂ̓_�Ɋi�[����.
		virtual bool set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool keep_aspect) noexcept final override;
		// �}�`���쐬����.
		ShapeRRect(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeRRect(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void export_svg(DataWriter const& dt_writer) noexcept final override;
	};

	//------------------------------
	// ��邵
	//------------------------------
	struct ShapeArrow : ShapeStroke {
		ARROW_STYLE m_arrow_style = ARROW_STYLE::ARROW_NONE;	// ��邵�̌`��
		ARROW_SIZE m_arrow_size{ ARROW_SIZE_DEFVAL };	// ��邵�̐��@
		D2D1_CAP_STYLE m_arrow_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// ��邵�̕Ԃ��̌`��
		D2D1_LINE_JOIN m_arrow_join = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL;	// ��邵�̐�[�̌`��
		float m_arrow_join_limit = JOIN_MITER_LIMIT_DEFVAL;	// ��邵�̐�萧��

		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_arrow_stroke{ nullptr };	// ��邵�� D2D �X�g���[�N�X�^�C��
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_arrow_geom{ nullptr };	// ��邵�� D2D �p�X�W�I���g��

		// ���̏I�[�����邩���肷��.
		virtual bool exist_cap(void) const noexcept override { return true; }
		// ��邵�̃X�g���[�N���쐬����.
		HRESULT create_arrow_stroke(void) noexcept
		{
			m_d2d_arrow_stroke = nullptr;
			ID2D1Factory* factory = nullptr;
			Shape::m_d2d_target->GetFactory(&factory);

			// ��邵�͂��Ȃ炸���Ƃ���.
			const D2D1_STROKE_STYLE_PROPERTIES s_prop{
				m_arrow_cap,	// startCap
				m_arrow_cap,	// endCap
				D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// dashCap
				m_arrow_join,	// lineJoin
				static_cast<FLOAT>(m_stroke_join_limit),	// miterLimit
				D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID,	// dashStyle
				0.0f	// dashOffset
			};
			return factory->CreateStrokeStyle(s_prop, nullptr, 0, m_d2d_arrow_stroke.put());
		}
		// �}�`��\������.
		virtual void draw(void) noexcept = 0;

		virtual bool get_arrow_size(ARROW_SIZE& val) const noexcept final override
		{
			val = m_arrow_size;
			return true;
		}

		virtual bool get_arrow_style(ARROW_STYLE& val) const noexcept final override
		{
			val = m_arrow_style;
			return true;
		}
		virtual bool get_arrow_cap(D2D1_CAP_STYLE& val) const noexcept final override
		{
			val = m_arrow_cap;
			return true;
		}
		virtual bool get_arrow_join(D2D1_LINE_JOIN& val) const noexcept final override
		{
			val = m_arrow_join;
			return true;
		}

		// �}�`��j������.
		virtual ~ShapeArrow(void)
		{
			m_d2d_arrow_geom = nullptr;
			m_d2d_arrow_stroke = nullptr;
		} // ~ShapeStroke

		// �l���邵�̐��@�Ɋi�[����.
		bool set_arrow_size(const ARROW_SIZE& val) noexcept final override
		{
			if (!equal(m_arrow_size, val)) {
				m_arrow_size = val;
				m_d2d_arrow_geom = nullptr;
				return true;
			}
			return false;
		}

		// �l���邵�̌`���Ɋi�[����.
		bool set_arrow_style(const ARROW_STYLE val) noexcept override
		{
			if (m_arrow_style != val) {
				m_arrow_style = val;
				m_d2d_arrow_geom = nullptr;
				m_d2d_arrow_stroke = nullptr;
				return true;
			}
			return false;
		}

		// �l���邵�̕Ԃ��̌`���Ɋi�[����.
		bool set_arrow_cap(const D2D1_CAP_STYLE val) noexcept override
		{
			if (m_arrow_cap != val) {
				m_arrow_cap = val;
				m_d2d_arrow_stroke = nullptr;
				return true;
			}
			return false;
		}

		// �l���邵�̐�[�̌`���Ɋi�[����.
		bool set_arrow_join(const D2D1_LINE_JOIN val) noexcept override
		{
			if (m_arrow_join != val) {
				m_arrow_join = val;
				m_d2d_arrow_stroke = nullptr;
				return true;
			}
			return false;
		}

		ShapeArrow(const Shape* prop) :
			ShapeStroke(prop),
			m_arrow_style([prop]() {
				ARROW_STYLE a_style;
				prop->get_arrow_style(a_style);
				return a_style;
				}()),
			m_arrow_size([prop]() {
				ARROW_SIZE a_size;
				prop->get_arrow_size(a_size);
				return a_size;
			}()),
			m_arrow_cap([prop]() {
				D2D1_CAP_STYLE a_cap;
				prop->get_arrow_cap(a_cap);
				return a_cap;
			}()),
			m_arrow_join([prop]() {
				D2D1_LINE_JOIN a_join;
				prop->get_arrow_join(a_join);
				return a_join;
			}())
			{}

		ShapeArrow(const DataReader& dt_reader) :
			ShapeStroke(dt_reader)
		{
			const ARROW_STYLE a_style = static_cast<ARROW_STYLE>(dt_reader.ReadUInt32());
			const ARROW_SIZE a_size{
				dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle()
			};
			const D2D1_CAP_STYLE a_cap = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
			const D2D1_LINE_JOIN a_join = static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32());

			if (a_style == ARROW_STYLE::ARROW_NONE || a_style == ARROW_STYLE::ARROW_OPENED || a_style == ARROW_STYLE::ARROW_FILLED) {
				m_arrow_style = a_style;
			}
			if (a_size.m_width >= 0.0f &&
				a_size.m_length >= 0.0f &&
				a_size.m_offset >= 0.0f) {
				m_arrow_size = a_size;
			}
			if (a_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT ||
				a_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND ||
				a_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE ||
				a_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
				m_arrow_cap = a_cap;
			}
			if (a_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL ||
				a_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
				a_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL ||
				a_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				m_arrow_join = a_join;
			}
		}

		void write(DataWriter const& dt_writer) const
		{
			ShapeStroke::write(dt_writer);
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));
			dt_writer.WriteSingle(m_arrow_size.m_width);
			dt_writer.WriteSingle(m_arrow_size.m_length);
			dt_writer.WriteSingle(m_arrow_size.m_offset);
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_cap));
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_join));
		}

	};

	struct ShapeLine : ShapeArrow {
		D2D1_POINT_2F m_start{ 0.0f, 0.0f };	// �n�_
		D2D1_POINT_2F m_pos{ 0.0f, 0.0f };	// ���̓_�ւ̈ʒu�x�N�g��

		//------------------------------
		// shape_line.cpp
		//------------------------------

		// ��邵�̐�[�ƕԂ��̈ʒu�����߂�.
		static bool line_get_pos_arrow(const D2D1_POINT_2F a_end, const D2D1_POINT_2F a_pos, const ARROW_SIZE& a_size, /*--->*/D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept;
		// �}�`���쐬����.
		ShapeLine(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prpp);
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapeLine(DataReader const& dt_reader);
		// �}�`��\������.
		virtual void draw(void) noexcept override;
		// ���E��`�𓾂�.
		virtual void get_bbox(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept override;
		// �w�肵�����ʂ̓_�𓾂�.
		virtual void get_pos_loc(const uint32_t /*loc*/, D2D1_POINT_2F& val) const noexcept override;
		// ���E��`�̍���_�𓾂�.
		virtual void get_bbox_lt(D2D1_POINT_2F& val) const noexcept override;
		// �J�n�_�𓾂�.
		bool get_pos_start(D2D1_POINT_2F& val) const noexcept final override;
		// �ߖT�̒��_��������.
		virtual bool get_pos_nearest(const D2D1_POINT_2F p, double& dd, D2D1_POINT_2F& val) const noexcept override;
		// �_�𓾂�.
		virtual size_t get_verts(D2D1_POINT_2F p[]) const noexcept override;
		// �}�`���_���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept override;
		// ��`�Ɋ܂܂�邩���肷��.
		virtual bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept override;
		// �l��, �w�肵�����ʂ̓_�Ɋi�[����.
		virtual bool set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool keep_aspect) noexcept override;
		// �l���n�_�Ɋi�[����.
		virtual bool set_pos_start(const D2D1_POINT_2F val) noexcept override;
		// ���������ړ�����.
		virtual bool move(const D2D1_POINT_2F pos) noexcept override;
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer);
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void export_svg(DataWriter const& dt_writer) noexcept final override;
		// �l��[�̌`���Ɋi�[����.
		//bool set_stroke_cap(const CAP_STYLE& val) noexcept final override;
		bool set_stroke_cap(const D2D1_CAP_STYLE& val) noexcept final override;
		// �l����̌����̐�萧���Ɋi�[����.
		bool set_stroke_join_limit(const float& val) noexcept final override;
		// �l����̌����̌`���Ɋi�[����.
		bool set_stroke_join(const D2D1_LINE_JOIN& val) noexcept final override;
	};

	//------------------------------
	// �܂���̂ЂȌ^
	//------------------------------
	struct ShapePath : ShapeArrow {
		D2D1_POINT_2F m_start{ 0.0f, 0.0f };	// �n�_
		std::vector<D2D1_POINT_2F> m_pos{};	// ���̓_�ւ̈ʒu�x�N�g��
		D2D1_COLOR_F m_fill_color{ 1.0f, 1.0f, 1.0f, 0.0f };

		winrt::com_ptr<ID2D1PathGeometry> m_d2d_path_geom{ nullptr };	// �܂���� D2D �p�X�W�I���g��

		// �}�`��\������.
		virtual void draw(void) noexcept = 0;
		// �ߖT�̒��_��������.
		virtual bool get_pos_nearest(const D2D1_POINT_2F p, double& dd, D2D1_POINT_2F& val) const noexcept override;
		// �l��, �w�肵�����ʂ̓_�Ɋi�[����.
		virtual bool set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool keep_aspect) noexcept override;
		// ���_�𓾂�.
		virtual size_t get_verts(D2D1_POINT_2F p[]) const noexcept override;
		// ���E��`�𓾂�.
		void get_bbox(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept final override;
		// ���E��`�̍���_�𓾂�.
		void get_bbox_lt(D2D1_POINT_2F& val) const noexcept final override;
		// �w�肵�����ʂ̓_�𓾂�.
		virtual void get_pos_loc(const uint32_t /*loc*/, D2D1_POINT_2F& val) const noexcept override;
		// �}�`���쐬����.
		ShapePath(
			const Shape* prop,
			const bool e_closed) :
			ShapeArrow::ShapeArrow(prop)
		{
			prop->get_fill_color(m_fill_color);
			if (e_closed) {
				set_arrow_style(ARROW_STYLE::ARROW_NONE);
			}
		}

		// �}�`��j������.
		virtual ~ShapePath(void)
		{
			if (m_d2d_path_geom != nullptr) {
				m_d2d_path_geom = nullptr;
			}
		}	// ~ShapePath

		//------------------------------
		// shape_path.cpp
		// �܂���̂ЂȌ^
		//------------------------------

		// �h��Ԃ��F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// ���������ړ�����.
		bool move(const D2D1_POINT_2F pos) noexcept final override;
		// �l��h��Ԃ��F�Ɋi�[����.
		bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		bool set_pos_start(const D2D1_POINT_2F val) noexcept override;
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapePath(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ���p�`
	//------------------------------
	struct ShapePoly : ShapePath {
		D2D1_FIGURE_END m_end = D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED;	// �I�[�̏��
		// ���̘A�������邩���肷��.
		virtual bool exist_join(void) const noexcept final override
		{
			return true;
		}
		// ���̏I�[�����邩���肷��.
		virtual bool exist_cap(void) const noexcept final override
		{
			return m_end == D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN;
		}
		//------------------------------
		// shape_poly.cpp
		//------------------------------

		// ���̐�[�ƕԂ��̈ʒu�𓾂�.
		static bool poly_get_pos_arrow(const size_t p_cnt, const D2D1_POINT_2F p[], const ARROW_SIZE& a_size, D2D1_POINT_2F barb[], D2D1_POINT_2F& tip) noexcept;
		// ��`�����Ƃɑ��p�`���쐬����.
		static void poly_create_by_box(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const POLY_OPTION& p_opt, D2D1_POINT_2F p[]) noexcept;
		// �}�`��\������
		virtual void draw(void) noexcept final override;
		// �ӂ����Ă��邩�𓾂�.
		virtual bool get_poly_end(D2D1_FIGURE_END& val) const noexcept final override
		{
			val = m_end;
			return true;
		}
		// �}�`���_���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// ��`�Ɋ܂܂�邩���肷��.
		virtual bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept override;
		// �l���邵�̌`���Ɋi�[����.
		virtual bool set_arrow_style(const ARROW_STYLE val) noexcept final override;
		// �l��ӂ����Ă��邩�Ɋi�[����.
		virtual bool set_poly_end(const D2D1_FIGURE_END val) noexcept final override
		{
			if (m_end != val) {
				// ���p�`�����Ă���̂ɖ�邵�����Ă���Ȃ��, ��邵���O��.
				if (val == D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED && m_arrow_style != ARROW_STYLE::ARROW_NONE) {
					m_arrow_style = ARROW_STYLE::ARROW_NONE;
				}
				m_end = val;
				m_d2d_arrow_geom = nullptr;
				m_d2d_path_geom = nullptr;
				return true;
			}
			return false;
		}
		// �}�`���쐬����.
		ShapePoly(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop, const POLY_OPTION& p_opt);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapePoly(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& /*dt_writer*/) const;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void export_svg(DataWriter const& dt_writer) noexcept final override;
	};

	//------------------------------
	// �Ȑ�
	//------------------------------
	struct ShapeBezier : ShapePath {

		//------------------------------
		// SHAPE_bezier.cpp
		// �x�W�F�Ȑ�
		//------------------------------

		// ���̕Ԃ��Ɛ�[�̓_�𓾂�
		static bool bezi_get_pos_arrow(const D2D1_POINT_2F start, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_SIZE a_size, D2D1_POINT_2F arrow[3]) noexcept;
		// �}�`��\������.
		virtual void draw(void) noexcept final override;
		// �}�`���_���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// ��`�Ɋ܂܂�邩���肷��.
		virtual bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept final override;
		// �}�`���쐬����.
		ShapeBezier(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeBezier(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer) final override;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void export_svg(const DataWriter& dt_writer) noexcept final override;
		// �}�`���f�[�^���C�^�[�ɏ�������.
		virtual void write(const DataWriter& dt_writer) const final override;
	};

	// �~��
	struct ShapeArc : ShapePath {
		// ���f�B�A���Ȃ琸�x���s���������Ȃ̂Łu�x�v�ŕێ�����.
		D2D1_SIZE_F m_radius{ 0.0f, 0.0f };	// �W���`�ɂ����Ƃ��� X �� Y �������̔��a
		float m_angle_rot = 0.0f;	// �~�ʂ̌X�� (�x)
		float m_angle_start = 0.0f;	// �~�ʂ̎n�_ (�x)
		float m_angle_end = 0.0f;	// �~�ʂ̏I�_ (�x)
		D2D1_SWEEP_DIRECTION m_sweep_dir = D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE;	// �~�ʂ̕`�����
		D2D1_ARC_SIZE m_larg_flag = D2D1_ARC_SIZE::D2D1_ARC_SIZE_SMALL;	// �~�ʂ̑傫��

		winrt::com_ptr<ID2D1PathGeometry> m_d2d_fill_geom{ nullptr };	// �h��Ԃ��W�I���g��

		virtual bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept final override
		{
			D2D1_POINT_2F p[5];
			get_verts(p);
			return pt_in_rect(p[0], lt, rb) && pt_in_rect(p[1], lt, rb) && pt_in_rect(p[2], lt, rb) && pt_in_rect(p[3], lt, rb) && pt_in_rect(p[4], lt, rb);
		}

		virtual bool get_arc_dir(D2D1_SWEEP_DIRECTION& val) const noexcept final override
		{
			val = m_sweep_dir;
			return true;
		}
		// �~�ʂ̎n�_�̊p�x�𓾂�.
		virtual bool get_arc_start(float& val) const noexcept final override
		{
			val = m_angle_start;
			return true;
		}
		// �~�ʂ̏I�_�̊p�x�𓾂�.
		virtual bool get_arc_end(float& val) const noexcept final override
		{
			val = m_angle_end;
			return true;
		}
		// �~�ʂ̌X���𓾂�.
		virtual bool get_arc_rot(float& val) const noexcept final override
		{
			val = m_angle_rot;
			return true;
		}

		//------------------------------
		// SHAPE_arc.cpp
		// �~��
		//------------------------------

		// �w�肵�����ʂ̓_�𓾂�.
		virtual void get_pos_loc(const uint32_t loc, D2D1_POINT_2F& val) const noexcept final override;
		// �l��, �w�肵�����ʂ̓_�Ɋi�[����.
		virtual bool set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool keep_aspect) noexcept final override;
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		virtual bool set_pos_start(const D2D1_POINT_2F val) noexcept final override;
		// �l���~�ʂ̎n�_�̊p�x�Ɋi�[����.
		virtual bool set_arc_start(const float val) noexcept final override;
		// �l���~�ʂ̏I�_�̊p�x�Ɋi�[����.
		virtual bool set_arc_end(const float val) noexcept final override;
		// �l���~�ʂ̌X���Ɋi�[����.
		virtual bool set_arc_rot(const float val) noexcept final override;
		// �l���~�ʂ�`�������Ɋi�[����.
		virtual bool set_arc_dir(const D2D1_SWEEP_DIRECTION val) noexcept final override;
		// �}�`���_���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// �~�ʂ��x�W�F�Ȑ��ŋߎ�����.
		void alter_bezier(D2D1_POINT_2F& start, D2D1_BEZIER_SEGMENT& b_seg) const noexcept;
		// ���̕Ԃ��Ɛ�[�̈ʒu�𓾂�.
		static bool arc_get_pos_arrow(const D2D1_POINT_2F pos, const D2D1_POINT_2F ctr, const D2D1_SIZE_F rad, const double deg_start, const double deg_end, const double deg_rot, const ARROW_SIZE a_size, D2D1_POINT_2F arrow[]);
		// �}�`��`��
		virtual void draw(void) noexcept final override;
		// �}�`���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void export_svg(const DataWriter& dt_writer) noexcept final override;
		// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		size_t export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer);
		// �}�`���쐬����.
		ShapeArc(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeArc(const DataReader& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(const DataWriter& dt_writer) const final override;
		// ���_�𓾂�.
		size_t get_verts(D2D1_POINT_2F p[]) const noexcept final override;
	};

	//------------------------------
	// ������
	//------------------------------
	struct ShapeText : ShapeRect {
		static wchar_t** s_available_fonts;		// �L���ȏ��̖�
		static D2D1_COLOR_F s_text_selected_background;	// �����͈͂̔w�i�F
		static D2D1_COLOR_F s_text_selected_foreground;	// �����͈͂̕����F

		D2D1_COLOR_F m_font_color{ COLOR_BLACK };	// ���̂̐F
		wchar_t* m_font_family = nullptr;	// ���̖�
		float m_font_size = FONT_SIZE_DEFVAL;	// ���̂̑傫��
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// ���̂̕�
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// ���̂̎���
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// ���̂̑���
		wchar_t* m_text = nullptr;	// ������
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_vert = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// �i���̂��낦
		DWRITE_TEXT_ALIGNMENT m_text_align_horz = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;	// �����̂��낦
		float m_text_line_sp = 0.0f;	// �s�� (DIPs 96dpi�Œ�)
		D2D1_SIZE_F m_text_pad{ TEXT_PAD_DEFVAL };	// ������̏㉺�ƍ��E�̗]��
		DWRITE_TEXT_RANGE m_text_selected_range{ 0, 0 };	// �I�����ꂽ�����͈�

		DWRITE_FONT_METRICS m_dwrite_font_metrics{};	// ���̂̌v��
		DWRITE_LINE_METRICS* m_dwrite_line_metrics = nullptr;	// �s�̌v��
		UINT32 m_dwrite_selected_cnt = 0;	// �I�����ꂽ�����͈͂̌v�ʂ̗v�f��
		DWRITE_HIT_TEST_METRICS* m_dwrite_selected_metrics = nullptr;	// �I�����ꂽ�����͈͂̌v��
		UINT32 m_dwrite_test_cnt = 0;	// �ʒu�̌v�ʂ̗v�f��
		DWRITE_HIT_TEST_METRICS* m_dwrite_test_metrics = nullptr;	// �ʒu�̌v��
		winrt::com_ptr<IDWriteTextLayout> m_dwrite_text_layout{ nullptr };	// �����񃌃C�A�E�g

		int m_pdf_text_cnt = 0;	// PDF �̃t�H���g�ԍ� (PDF �o�͎��̂ݗ��p)

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
			if (m_dwrite_text_layout != nullptr) {
				m_dwrite_text_layout = nullptr;
			}
		} // ~ShapeStroke

		//------------------------------
		// shape_text.cpp
		// ������}�`
		//------------------------------

		// �����񃌃C�A�E�g���쐬����.
		void create_text_layout(void) noexcept;
		// �g�𕶎���ɍ��킹��.
		bool fit_frame_to_text(const float g_len) noexcept;
		float get_frame_width(void) const noexcept { return fabsf(m_pos.x); }
		float get_frame_height(void) const noexcept { return fabsf(m_pos.y); }
		// �}�`��\������.
		virtual void draw(void) noexcept final override;
		// ���̂̐F�𓾂�.
		bool get_font_color(D2D1_COLOR_F& val) const noexcept final override;
		// ���ʂ𓾂� (�g�p��� Release ����).
		bool get_font_face(IDWriteFontFace3*& face) const noexcept;
		// ���̖��𓾂�.
		bool get_font_family(wchar_t*& val) const noexcept final override;
		// ���̂̑傫���𓾂�.
		bool get_font_size(float& val) const noexcept final override;
		// ���̂̕��𓾂�.
		bool get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept final override;
		// ���̂̎��̂𓾂�.
		bool get_font_style(DWRITE_FONT_STYLE& val) const noexcept final override;
		// ���̂̑����𓾂�.
		bool get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept final override;
		// �i���̂��낦�𓾂�.
		bool get_text_align_vert(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept final override;
		// ������̂��낦�𓾂�.
		bool get_text_align_horz(DWRITE_TEXT_ALIGNMENT& val) const noexcept final override;
		// ������𓾂�.
		bool get_text_content(wchar_t*& val) const noexcept final override;
		// �s�Ԃ𓾂�.
		bool get_text_line_sp(float& val) const noexcept final override;
		// ������̗]���𓾂�.
		bool get_text_pad(D2D1_SIZE_F& val) const noexcept final override;
		// �I�����ꂽ�����͈͂𓾂�.
		bool get_text_selected(DWRITE_TEXT_RANGE& val) const noexcept final override;
		int get_text_len(void) const noexcept
		{
			return wchar_len(m_text);
		}
		// �w�肵���_��
		int get_text_pos(const D2D1_POINT_2F p) const noexcept
		{
			// �}�`�̊J�n�_�����_�Ƃ���.
			const double px = p.x - (m_pos.x >= 0.0f ? m_start.x : m_start.x + m_pos.x);
			const double py = p.y - (m_pos.y >= 0.0f ? m_start.y : m_start.y + m_pos.y);
			if (px >= 0.0 && px < m_pos.x && py >= 0.0 && py <= m_pos.y) {
				const int t_cnt = m_dwrite_test_cnt;
				const int t_len = static_cast<int>(wchar_len(m_text));
				DWRITE_HIT_TEST_METRICS h;
				FLOAT x, y;
				for (int i = 0; i < t_cnt; i++) {
					const auto t_start = m_dwrite_test_metrics[i].textPosition;	// �s���̕����̈ʒu
					const auto t_end = t_start + m_dwrite_test_metrics[i].length;	// �s���̕����̈ʒu
					const auto t_bot = m_dwrite_test_metrics[i].top + m_dwrite_test_metrics[i].height;
					// �_�� i �s�ڂ̕�����Ɋ܂܂�Ă���Ȃ�,
					if (py < t_bot) {
						// �s������s���̊e�����ɂ��ČJ��Ԃ�.
						for (uint32_t j = t_start; j < t_end; j++) {
							m_dwrite_text_layout->HitTestTextPosition(j, false, &x, &y, &h);
							if (px <= x + h.width * 0.5) {
								return j;
							}
						}
						// �s���𒴂��Ă���Ȃ�s���̕����̈ʒu, �����ĂȂ��Ȃ�s���̕����̈ʒu��Ԃ�.
						if (px >= m_dwrite_test_metrics[i].left + m_dwrite_test_metrics[i].width) {
							return t_end;
						}
						return t_start;
					}
				}
				//for (int i = 0; i < t_len; i++) {
				//	m_dwrite_text_layout->HitTestTextPosition(i, false, &x, &y, &h);
				//	if (py <= y + h.height) {
				//		if (px <= x + h.width * 0.5) {
				//			return i;
				//		}
				//	}
				//}
				return t_len;
			}
			return -1;
		}
		// �}�`���_���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// ��`�Ɋ܂܂�邩���肷��.
		bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept final override;
		// ���̖����L�������肵, �L���Ȃ�, �����̏��̖��͔j����, �L���ȏ��̖��̔z��̗v�f�ƒu��������.
		static bool is_available_font(wchar_t*& font) noexcept;
		// �L���ȏ��̖��̔z���j������.
		static void release_available_fonts(void) noexcept;
		// �v�ʂ�j������.
		void relese_metrics(void) noexcept;
		// �L���ȏ��̖��̔z���ݒ肷��.
		static void set_available_fonts(void) noexcept;
		// �l�����̂̐F�Ɋi�[����.
		bool set_font_color(const D2D1_COLOR_F& val) noexcept final override;
		// �l�����̖��Ɋi�[����.
		bool set_font_family(wchar_t* const val) noexcept final override;
		// �l�����̂̑傫���Ɋi�[����.
		bool set_font_size(const float val) noexcept final override;
		// �l�����̂̕��Ɋi�[����.
		bool set_font_stretch(const DWRITE_FONT_STRETCH val) noexcept final override;
		// �l�����̂̎��̂Ɋi�[����.
		bool set_font_style(const DWRITE_FONT_STYLE val) noexcept final override;
		// �l�����̂̑����Ɋi�[����.
		bool set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept final override;
		// �l��i���̂��낦�Ɋi�[����.
		bool set_text_align_vert(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept final override;
		// �l�𕶎���̂��낦�Ɋi�[����.
		bool set_text_align_horz(const DWRITE_TEXT_ALIGNMENT val) noexcept final override;
		// �l�𕶎���Ɋi�[����.
		bool set_text_content(wchar_t* const val) noexcept final override;
		// �l���s�ԂɊi�[����.
		bool set_text_line_sp(const float val) noexcept final override;
		// �l�𕶎���̗]���Ɋi�[����.
		bool set_text_pad(const D2D1_SIZE_F val) noexcept final override;
		// �l�𕶎��͈͂Ɋi�[����.
		bool set_text_selected(const DWRITE_TEXT_RANGE val) noexcept final override;
		// �}�`���쐬����.
		ShapeText(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, wchar_t* const text, const Shape* prop);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeText(DataReader const& dt_reader);
		// �}�`���f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� PDF �Ƃ��ď�������.
		size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void export_svg(DataWriter const& dt_writer) noexcept final override;
	};

	// ���� (�~�`) ��\������.
	// �~�`��, �}�`�̕⏕�I�ȕό`�Ɋւ�镔�ʂ��Ӗ�����.
	inline void loc_draw_circle(
		const D2D1_POINT_2F p,	// ���ʂ̓_
		ID2D1RenderTarget* const target,	// �`��^�[�Q�b�g
		ID2D1SolidColorBrush* const brush	// �F�u���V
	) noexcept
	{
		const auto c_inner = Shape::m_loc_circle_inner;	// �����̔��a
		const auto c_outer = Shape::m_loc_circle_outer;	// �O���̔��a
		const D2D1_ELLIPSE e_outer{	// �O���̉~
			p, static_cast<FLOAT>(c_outer), static_cast<FLOAT>(c_outer)
		};
		const D2D1_ELLIPSE e_inner{	// �����̉~
			p, static_cast<FLOAT>(c_inner), static_cast<FLOAT>(c_inner)
		};
		brush->SetColor(COLOR_WHITE);
		target->FillEllipse(e_outer, brush);
		brush->SetColor(COLOR_BLACK);
		target->FillEllipse(e_inner, brush);
	}

	// ���� (�Ђ��`) ��\������.
	// �Ђ��`��, �}�`�̈ړ��͍s�Ȃ���, �ό`�ɂ͊ւ��Ȃ��⏕�I�ȕ��ʂ��Ӗ�����.
	inline void loc_draw_rhombus(
		const D2D1_POINT_2F p,	// ���ʂ̓_
		ID2D1RenderTarget* const target,	// �`��^�[�Q�b�g
		ID2D1SolidColorBrush* const brush	// �F�u���V
	) noexcept
	{
		// �p�X�W�I���g�����g�킸, �΂߂̑��������ő�p����.
		const double w_outer = 2.0 * Shape::m_loc_square_outer;
		const double w_inner = 2.0 * Shape::m_loc_square_inner;
		const double d_outer = Shape::m_loc_rhombus_outer;
		const double d_inner = Shape::m_loc_rhombus_inner;
		const D2D1_POINT_2F q_inner{
			static_cast<FLOAT>(p.x - d_inner), static_cast<FLOAT>(p.y - d_inner)
		};
		const D2D1_POINT_2F r_inner{
			static_cast<FLOAT>(p.x + d_inner), static_cast<FLOAT>(p.y + d_inner)
		};
		const D2D1_POINT_2F q_outer{
			static_cast<FLOAT>(p.x - d_outer), static_cast<FLOAT>(p.y - d_outer)
		};
		const D2D1_POINT_2F r_outer{
			static_cast<FLOAT>(p.x + d_outer), static_cast<FLOAT>(p.y + d_outer)
		};
		brush->SetColor(COLOR_WHITE);
		target->DrawLine(q_outer, r_outer, brush, static_cast<FLOAT>(w_outer));
		brush->SetColor(COLOR_BLACK);
		target->DrawLine(q_inner, r_inner, brush, static_cast<FLOAT>(w_inner));
	}

	// ���� (�����`) ��\������.
	// �����`��, �}�`�̕ό`�Ɋւ�镔�ʂ��Ӗ�����.
	// p	���ʂ̓_
	// target	�`��^�[�Q�b�g
	// brush	�F�u���V
	inline void loc_draw_square(
		const D2D1_POINT_2F p,
		ID2D1RenderTarget* const target,
		ID2D1SolidColorBrush* const brush
	) noexcept
	{
		const double a_inner = Shape::m_loc_square_inner;
		const double a_outer = Shape::m_loc_square_outer;
		const D2D1_RECT_F s_inner{
			static_cast<FLOAT>(p.x - a_inner), static_cast<FLOAT>(p.y - a_inner),
			static_cast<FLOAT>(p.x + a_inner), static_cast<FLOAT>(p.y + a_inner)
		};
		const D2D1_RECT_F s_outer{
			static_cast<FLOAT>(p.x - a_outer), static_cast<FLOAT>(p.y - a_outer),
			static_cast<FLOAT>(p.x + a_outer), static_cast<FLOAT>(p.y + a_outer)
		};
		brush->SetColor(COLOR_WHITE);
		target->FillRectangle(s_outer, brush);
		brush->SetColor(COLOR_BLACK);
		target->FillRectangle(s_inner, brush);
	}

	// ���ʂ��_���܂ނ����肷��.
	// t	���肳���_
	// loc	���ʂ̓_
	// len	���ʂ̑傫��
	inline bool loc_hit_test(const D2D1_POINT_2F t, const D2D1_POINT_2F loc, const double len) noexcept
	{
		const double x = static_cast<double>(t.x) - static_cast<double>(loc.x);
		const double y = static_cast<double>(t.y) - static_cast<double>(loc.y);
		return -len * 0.5 <= x && x <= len * 0.5 && -len * 0.5 <= y && y <= len * 0.5;
	}

	// ���� (0.0...1.0) �̐F�����𐮐� (0...255) �ɕϊ�����.
	inline uint32_t conv_color_comp(const double c) noexcept
	{
		return min(static_cast<uint32_t>(floor(c * 256.0)), 255);
	}

	// ��邵�̑傫�������������肷��.
	inline bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept
	{
		return equal(a.m_width, b.m_width) && equal(a.m_length, b.m_length) && equal(a.m_offset, b.m_offset);
	}

	// ���̒[�_�����������肷��.
	//inline bool equal(const CAP_STYLE& a, const CAP_STYLE& b) noexcept
	//{
	//	return a.m_start == b.m_start && a.m_end == b.m_end;
	//}

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
	inline bool equal(const GRID_EMPH a, const GRID_EMPH b) noexcept
	{
		return a.m_gauge_1 == b.m_gauge_1 && a.m_gauge_2 == b.m_gauge_2;
	}

	// �j���̔z�u�����������肷��.
	inline bool equal(const DASH_PAT& a, const DASH_PAT& b) noexcept
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
		return conv_color_comp(a) == conv_color_comp(b);
	}

	// ���̕Ԃ��̓_�����߂�.
	inline void get_pos_barbs(
		const D2D1_POINT_2F a,	// ��x�N�g�� (�n�Y�����[�����ւ̈ʒu�x�N�g��)
		const double a_len,	// ��x�N�g���̒���
		const double width,	// ���̕� (�Ԃ��̊Ԃ̒���)
		const double len,	// ���̒��� (��[����Ԃ��܂ł̒���)
		D2D1_POINT_2F barb[]	// ���̕Ԃ��̈ʒu
	) noexcept
	{
		if (a_len <= DBL_MIN) {
			constexpr D2D1_POINT_2F Z{ 0.0f, 0.0f };
			barb[0] = Z;
			barb[1] = Z;
		}
		else {
			const double bw = width * 0.5;	// ��邵�̕��̔����̑傫��
			const double sx = a.x * -len;	// ��x�N�g�����邵�̒��������]
			const double sy = a.x * bw;
			const double tx = a.y * -len;
			const double ty = a.y * bw;
			const double ax = 1.0 / a_len;
			barb[0].x = static_cast<FLOAT>((sx - ty) * ax);
			barb[0].y = static_cast<FLOAT>((tx + sy) * ax);
			barb[1].x = static_cast<FLOAT>((sx + ty) * ax);
			barb[1].y = static_cast<FLOAT>((tx - sy) * ax);
		}
	}

	// �F���s���������肷��.
	// val	�F
	// �߂�l	�s�����Ȃ�� true, �����Ȃ�� false.
	inline bool is_opaque(const D2D1_COLOR_F& val) noexcept
	{
		return val.a * 256.0 >= 1.0;
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
		const size_t len = ((s == nullptr || s[0] == '\0') ? 0 : static_cast<uint32_t>(wcslen(s)));
		if (len > 0) {
			wchar_t* t;
			wcscpy_s(t = new wchar_t[len + 1], len + 1, s);
			return t;
		}
		return nullptr;
	}

}