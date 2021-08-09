//------------------------------
// shape.h
// shape.cpp	�}�`�̂ЂȌ^, ���̑�
// shape_anch.cpp	�}�`�̕��ʂ�\��
// shape_bezi.cpp	�x�W�F�Ȑ�
// shape_dt.cpp	�ǂݍ���, ��������.
// shape_dx.cpp	�}�`�̕`���
// shape_elli.cpp	���~
// shape_group.cpp	�O���[�v
// shape_image.cpp	�摜
// shape_line.cpp	����
// shape_path.cpp	�܂���̂ЂȌ^
// shape_poly.cpp	���p�`
// shape.rect.cpp	���`
// shape_rrect.cpp	�p�ە��`
// shape_ruler.cpp	��K
// shape_sheet.cpp	�p��
// shape_slist.cpp	�}�`���X�g
// shape_stroke.cpp	���g�̂ЂȌ^
// shape_text.cpp	������
//------------------------------
#pragma once
#include <list>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include "shape_dx.h"
//
// +-------------+
// | SHAPE_DX    |
// +-------------+
//
// +-------------+
// | Shape*      |
// +------+------+
//        |
//        +---------------+---------------+---------------+
//        |               |               |               |
// +------+------+ +------+------+ +------+------+ +------+------+
// | ShapeStroke*| | ShapeImage  | | ShapeGroup  | | ShapeSheet  |
// +------+------+ +-------------+ +-------------+ +-------------+
//        |
//        +-------------------------------+
//        |                               |
// +------+------+                        |
// | ShapeLineA  |                        |
// +------+------+                        |
//        |                               |
// +------+------+                 +------+------+
// | ShapePath*  |                 | ShapeRect   |
// +------+------+                 +------+------+
//        |                               |
//        +---------------+               +---------------+---------------+---------------+
//        |               |               |               |               |               |
// +------+------+ +------+------+ +------+------+ +------+------+ +------+------+ +------+------+
// | ShapePoly   | | ShapeBezi   | | ShapeElli   | | ShapeRRect  | | ShapeText   | | ShapeRuler  |
// +-------------+ +-------------+ +-------------+ +-------------+ +-------------+ +-------------+
//

// SVG �̂��߂̃e�L�X�g���s�R�[�h
#define SVG_NEW_LINE	"\n"

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::Point;
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;
	using winrt::Windows::UI::Color;
	using winrt::Windows::UI::Xaml::Media::Brush;
	using winrt::Windows::Graphics::Imaging::SoftwareBitmap;
	using winrt::Windows::Storage::Streams::IRandomAccessStream;
	using winrt::Windows::Foundation::IAsyncAction;

#if defined(_DEBUG)
	extern uint32_t debug_leak_cnt;
	constexpr wchar_t DEBUG_MSG[] = L"Memory leak occurs";
#endif
	constexpr double PT_ROUND = 1.0 / 16.0;

	// �O���錾
	struct Shape;
	struct ShapeLineA;
	struct ShapePath;
	struct ShapeRect;
	struct ShapeStroke;

	// �A���J�[ (�}�`�̕���) �̎��
	// �܂���̒��_������킷����, enum struct �łȂ� enum ��p����.
	enum ANCH_TYPE {
		ANCH_SHEET,		// �}�`�̊O�� (���J�[�\��)
		ANCH_FILL,		// �}�`�̓��� (�ړ��J�[�\��)
		ANCH_STROKE,	// ���g (�ړ��J�[�\��)
		ANCH_TEXT,		// ������ (�ړ��J�[�\��)
		ANCH_NW,		// ���`�̍���̒��_ (�k���쓌�J�[�\��)
		ANCH_SE,		// ���`�̉E���̒��_ (�k���쓌�J�[�\��)
		ANCH_NE,		// ���`�̉E��̒��_ (�k���쐼�J�[�\��)
		ANCH_SW,		// ���`�̍����̒��_ (�k���쐼�J�[�\��)
		ANCH_NORTH,		// ���`�̏�ӂ̒��_ (�㉺�J�[�\��)
		ANCH_SOUTH,		// ���`�̉��ӂ̒��_ (�㉺�J�[�\��)
		ANCH_EAST,		// ���`�̍��ӂ̒��_ (���E�J�[�\��)
		ANCH_WEST,		// ���`�̉E�ӂ̒��_ (���E�J�[�\��)
		ANCH_R_NW,		// ����̊p�ۂ̒��S�_ (�\���J�[�\��)
		ANCH_R_NE,		// �E��̊p�ۂ̒��S�_ (�\���J�[�\��)
		ANCH_R_SE,		// �E���̊p�ۂ̒��S�_ (�\���J�[�\��)
		ANCH_R_SW,		// �����̊p�ۂ̒��S�_ (�\���J�[�\��)
		ANCH_P0,	// �p�X�̊J�n�_ (�\���J�[�\��)
	};

	// ���`�̒��_�̔z��
	constexpr uint32_t ANCH_MIDDLE[4]{
		ANCH_TYPE::ANCH_SOUTH,
		ANCH_TYPE::ANCH_EAST,
		ANCH_TYPE::ANCH_WEST,
		ANCH_TYPE::ANCH_NORTH
	};

	// ���`�̒��_�̔z��
	constexpr uint32_t ANCH_CORNER[4]{
		ANCH_TYPE::ANCH_SE,
		ANCH_TYPE::ANCH_NE,
		ANCH_TYPE::ANCH_SW,
		ANCH_TYPE::ANCH_NW
	};

	// ��邵�̐��@
	struct ARROW_SIZE {
		float m_width;		// �Ԃ��̕�
		float m_length;		// ��[����t�����܂ł̒���
		float m_offset;		// ��[�̃I�t�Z�b�g
	};

	// ��邵�̌`��
	enum struct ARROW_STYLE {
		NONE,	// �Ȃ�
		OPENED,	// �J������邵
		FILLED	// ������邵
	};

	// �����̒P�_
	// (SVG ��, �n�_�I�_�̋�ʂ��ł��Ȃ�)
	struct CAP_STYLE {
		D2D1_CAP_STYLE m_start;	// �n�_
		D2D1_CAP_STYLE m_end;	// �I�_
	};

	// ����̋���
	struct GRID_EMPH {
		uint16_t m_gauge_1;	// ��������Ԋu (����1)
		uint16_t m_gauge_2;	// ��������Ԋu (����2)
	};
	constexpr GRID_EMPH GRID_EMPH_0{ 0, 0 };	// �����Ȃ�
	constexpr GRID_EMPH GRID_EMPH_2{ 2, 0 };	// 2 �Ԗڂ�����
	constexpr GRID_EMPH GRID_EMPH_3{ 2, 10 };	// 2 �Ԗڂ� 10 �Ԗڂ�����

	// ����̕\��
	enum struct GRID_SHOW {
		HIDE,	// �\���Ȃ�
		BACK,	// �Ŕw�ʂɕ\��
		FRONT	// �őO�ʂɕ\��
	};

	// ���p�`�̑I����
	struct POLY_OPTION {
		uint32_t m_vertex_cnt;	// ��}���鑽�p�`�̒��_�̐�.
		bool m_regular;	// �����p�`�ō�}����.
		bool m_vertex_up;	// ���_����ɍ�}����.
		bool m_end_closed;	// �ӂ���č�}����.
		bool m_clockwise;	// ���_�����v���ɍ�}����.
	};

	// �j���̗l��
	union DASH_PATT {
		float m_[6];
	};

	constexpr float COLOR_MAX = 255.0f;	// �F�����̍ő�l
	constexpr double PT_PER_INCH = 72.0;	// 1 �C���`������̃|�C���g��
	constexpr double MM_PER_INCH = 25.4;	// 1 �C���`������̃~�����[�g����
	constexpr ARROW_SIZE DEF_ARROW_SIZE{ 7.0, 16.0, 0.0 };
	constexpr float DEF_FONT_SIZE = static_cast<float>(12.0 * 96.0 / 72.0);
	constexpr D2D1_COLOR_F DEF_GRID_COLOR{ ACCENT_COLOR.r, ACCENT_COLOR.g, ACCENT_COLOR.b, 0.5f };	// ����̐F
	constexpr float DEF_GRID_LEN = 48.0f;
	constexpr float DEF_MITER_LIMIT = 10.0f;	// �}�C�^�[�����̔䗦
	constexpr POLY_OPTION DEF_POLY_OPTION{ 3, true, true, true, true };	// ���p�`�̑I����
	constexpr DASH_PATT DEF_DASH_PATT{ { 4.0F, 3.0F, 1.0F, 3.0F, 1.0F, 3.0F } };
	constexpr D2D1_SIZE_F DEF_TEXT_MARGIN{ DEF_FONT_SIZE / 4.0, DEF_FONT_SIZE / 4.0 };
	constexpr size_t MAX_N_GON = 256;	// ���p�`�̒��_�̍ő吔 (�q�b�g����ŃX�^�b�N�𗘗p���邽��, �I�[�o�[�t���[���Ȃ��悤��������)

	MIDL_INTERFACE("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d")
		IMemoryBufferByteAccess : IUnknown
	{
		virtual HRESULT STDMETHODCALLTYPE GetBuffer(
			BYTE * *value,
			UINT32 * capacity
			);
	};

	//------------------------------
	// shape.cpp
	//------------------------------

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
	// �j���̗l�������������肷��.
	inline bool equal(const DASH_PATT& a, const DASH_PATT& b) noexcept;
	// ���C�h�����񂪓��������肷��.
	inline bool equal(const wchar_t* a, const wchar_t* b) noexcept;
	// winrt �����񂪓��������肷��.
	inline bool equal(winrt::hstring const& a, const wchar_t* b) noexcept;
	// �F�̐��������������肷��.
	inline bool equal_color_comp(const FLOAT a, const FLOAT b) noexcept;
	// ��邵�̕Ԃ��̈ʒu�����߂�.
	void get_arrow_barbs(const D2D1_POINT_2F a_vec, const double a_len, const double h_width, const double h_len, D2D1_POINT_2F barbs[]) noexcept;
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
	// ��_�ň͂܂ꂽ���`�𓾂�.
	void pt_bound(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;
	// �ʒu���}�`�̕��ʂɊ܂܂�邩���肷��.
	inline bool pt_in_anch(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos) noexcept;
	// �ʒu�����~�Ɋ܂܂�邩���肷��.
	bool pt_in_elli(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept;
	// �ʒu���~�Ɋ܂܂�邩���肷��.
	inline bool pt_in_circle(const D2D1_POINT_2F t_vec, const double rad) noexcept;
	// �ʒu���~�Ɋ܂܂�邩���肷��.
	inline bool pt_in_circle(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad) noexcept;
	// ���p�`���ʒu���܂ނ����肷��.
	bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t p_cnt, const D2D1_POINT_2F p_pos[]) noexcept;
	// ���`���ʒu���܂ނ����肷��.
	bool pt_in_rect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept;
	// �w�肵���ʒu���܂ނ悤, ���`���g�傷��.
	void pt_inc(const D2D1_POINT_2F a, D2D1_POINT_2F& r_min, D2D1_POINT_2F& r_max) noexcept;
	// ��_�̂��ꂼ��傫���l�����ʒu�𓾂�.
	inline void pt_max(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& r_min) noexcept;
	// �ʒu�̕������t�ɂ���.
	inline void pt_neg(const D2D1_POINT_2F a, D2D1_POINT_2F& b) noexcept;
	// ��_�̂��ꂼ�ꏬ�����l�����ʒu�𓾂�.
	inline void pt_min(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& r_min) noexcept;
	// �ʒu���X�J���[�{�Ɋۂ߂�.
	inline void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& round) noexcept;
	// �ʒu�ɃX�J���[�l���|��, �ʂ̈ʒu�𑫂�.
	inline void pt_mul(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
	// �ʒu�ɃX�J���[�l���|����.
	inline void pt_mul(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept;
	// ���@�ɒl���|����.
	inline void pt_mul(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& c) noexcept;
	// �_�ɃX�J���[�l���|��, �ʂ̈ʒu�𑫂�.
	inline void pt_mul(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
	// �ʒu����ʒu������.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& sub) noexcept;
	// �ʒu����傫��������.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& sub) noexcept;
	// ������𕡐�����. ���̕����񂪃k���|�C���^�[, �܂��͌��̕������� 0 �̂Ƃ���, �k���|�C���^�[��Ԃ�.
	wchar_t* wchar_cpy(const wchar_t* const s);
	// ������𕡐�����. �G�X�P�[�v������͕����R�[�h�ɕϊ�����.
	wchar_t* wchar_cpy_esc(const wchar_t* const s);
	// ������̒���. �������k���|�C���^�̏ꍇ, 0 ��Ԃ�.
	uint32_t wchar_len(const wchar_t* const t) noexcept;

	//------------------------------
	// shape_dt.cpp
	// �ǂݍ���, ��������.
	//------------------------------

	// �f�[�^���[�_�[�����邵�̐��@��ǂݍ���.
	void dt_read(ARROW_SIZE& value, DataReader const& dt_reader);
	// �f�[�^���[�_�[����[�̌`����ǂݍ���.
	void dt_read(CAP_STYLE& value, DataReader const& dt_reader);
	// �f�[�^���[�_�[����F��ǂݍ���.
	void dt_read(D2D1_COLOR_F& value, DataReader const& dt_reader);
	// �f�[�^���[�_�[����ʒu��ǂݍ���.
	void dt_read(D2D1_POINT_2F& value, DataReader const& dt_reader);
	// �f�[�^���[�_�[������`��ǂݍ���.
	void dt_read(D2D1_RECT_F& value, DataReader const& dt_reader);
	// �f�[�^���[�_�[���琡�@��ǂݍ���.
	void dt_read(D2D1_SIZE_F& value, DataReader const& dt_reader);
	// �f�[�^���[�_�[���琡�@��ǂݍ���.
	void dt_read(D2D1_SIZE_U& value, DataReader const& dt_reader);
	// �f�[�^���[�_�[���當���͈͂�ǂݍ���.
	void dt_read(DWRITE_TEXT_RANGE& value, DataReader const& dt_reader);
	// �f�[�^���[�_�[�������̌`����ǂݍ���.
	void dt_read(GRID_EMPH& value, DataReader const& dt_reader);
	// �f�[�^���[�_�[����j���̗l����ǂݍ���.
	void dt_read(DASH_PATT& value, DataReader const& dt_reader);
	// �f�[�^���[�_�[���瑽�p�`�̑I������ǂݍ���.
	void dt_read(POLY_OPTION& value, DataReader const& dt_reader);
	// �f�[�^���[�_�[���當�����ǂݍ���.
	void dt_read(wchar_t*& value, DataReader const& dt_reader);
	// �f�[�^���[�_�[����ʒu�z���ǂݍ���.
	void dt_read(std::vector<D2D1_POINT_2F>& value, DataReader const& dt_reader);
	// �f�[�^���C�^�[�ɖ�邵�̐��@����������.
	void dt_write(const ARROW_SIZE& value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɒ[�̌`������������.
	void dt_write(const CAP_STYLE& value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɐF����������.
	void dt_write(const D2D1_COLOR_F& value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�Ɉʒu����������.
	void dt_write(const D2D1_POINT_2F value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɕ��`����������.
	void dt_write(const D2D1_RECT_F value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɐ��@����������.
	void dt_write(const D2D1_SIZE_F value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɐ��@����������.
	void dt_write(const D2D1_SIZE_U value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɕ�����͈͂���������.
	void dt_write(const DWRITE_TEXT_RANGE value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɕ���̌`������������.
	void dt_write(const GRID_EMPH value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɔj���̗l������������.
	void dt_write(const DASH_PATT& value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɑ��p�`�̑I��������������.
	void dt_write(const POLY_OPTION& value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�ɕ��������������.
	void dt_write(const wchar_t* value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�Ɉʒu�z�����������.
	void dt_write(const std::vector<D2D1_POINT_2F>& value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��ăV���O���o�C�g���������������.
	void dt_write_svg(const char* value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��ă}���`�o�C�g���������������.
	void dt_write_svg(const wchar_t value[], const uint32_t v_len, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��đ������ƃV���O���o�C�g���������������.
	void dt_write_svg(const char* value, const char* name, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��Ė��߂ƈʒu����������.
	void dt_write_svg(const D2D1_POINT_2F value, const char* cmd, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��đ������ƈʒu����������.
	void dt_write_svg(const D2D1_POINT_2F value, const char* name_x, const char* name_y, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��đ������ƐF����������.
	void dt_write_svg(const D2D1_COLOR_F value, const char* name, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��ĐF����������.
	void dt_write_svg(const D2D1_COLOR_F value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��ĕ�����������������
	void dt_write_svg(const float value, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��đ������ƕ��������l����������
	void dt_write_svg(const double value, const char* name, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��đ������� 32 �r�b�g����������������
	void dt_write_svg(const uint32_t value, const char* name, DataWriter const& dt_writer);
	// �f�[�^���C�^�[�� SVG �Ƃ��Ĕj���̌`���Ɨl������������.
	void dt_write_svg(const D2D1_DASH_STYLE style, const DASH_PATT& patt, const double width, DataWriter const& dt_writer);

	//------------------------------
	// shape_anch.cpp
	// �}�`�̕��ʂ�\��
	//------------------------------

	// �}�`�̕��� (���`) ��\������.
	void anch_draw_rect(const D2D1_POINT_2F a_pos, SHAPE_DX& dx);
	// �}�`�̕��ʁi�~�`�j��\������.
	void anch_draw_ellipse(const D2D1_POINT_2F c_pos, SHAPE_DX& dx);

	//------------------------------
	// shape_slist.cpp
	// �}�`���X�g
	//------------------------------

	using SHAPE_LIST = std::list<struct Shape*>;

	// ���p�\�ȏ��̖������肵, ���p�ł��Ȃ����̂��������Ȃ�΂���𓾂�.
	bool slist_test_font(const SHAPE_LIST& slist, wchar_t*& unavailable_font) noexcept;

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
	void slist_bound_sheet(SHAPE_LIST const& slist, const D2D1_SIZE_F sheet_size, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;

	// �}�`���͂ޗ̈�����X�g���瓾��.
	void slist_bound_all(SHAPE_LIST const& slist, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;

	// �}�`���͂ޗ̈�����X�g���瓾��.
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
	template <typename S> void slist_selected(SHAPE_LIST const& slist, SHAPE_LIST& t_list) noexcept;

	// �f�[�^���C�^�[�ɐ}�`���X�g����������. REDUCE �Ȃ�������ꂽ�}�`�͏������܂Ȃ�.
	template <bool REDUCE> void slist_write(const SHAPE_LIST& slist, DataWriter const& dt_writer);

	// ���X�g�̒��̐}�`�̏��Ԃ𓾂�.
	template <typename S, typename T> bool slist_match(SHAPE_LIST const& slist, S s, T& t);

	// �I�����ꂽ������}�`����, ���������s�ŘA������������𓾂�.
	// winrt::hstring slist_selected_all_text(SHAPE_LIST const& slist) noexcept;

	// �I������ĂȂ��}�`�̒��_�̒����� �w�肵���ʒu�ɍł��߂����_��������.
	bool slist_find_vertex_closest(const SHAPE_LIST& slist, const D2D1_POINT_2F& c_pos, const float dist, D2D1_POINT_2F& value) noexcept;

	//------------------------------
	// �}�`�̂ЂȌ^
	//------------------------------
	struct Shape {
		static SHAPE_DX* s_dx;
		static ID2D1Factory3* s_d2d_factory;	// D2D �t�@�N�g���̃L���b�V��
		static IDWriteFactory3* s_dwrite_factory;	// DWRITE �t�@�N�g���̃L���b�V��
		static D2D1_COLOR_F s_anch_color;	// �}�`�̕��ʂ̐F
		static float s_anch_len;	// �}�`�̕��ʂ̑傫��
		//static winrt::com_ptr<ID2D1StrokeStyle1> m_aux_style;	// �⏕���̌`��
		static D2D1_COLOR_F m_range_background;	// �����͈͂̔w�i�F
		static D2D1_COLOR_F m_range_foreground;	// �����͈͂̕����F
		static D2D1_COLOR_F m_default_background;	// �O�i�F
		static D2D1_COLOR_F m_default_foreground;	// �w�i�F

		// �}�`��j������.
		virtual ~Shape(void) {}
		// �}�`��\������
		virtual void draw(SHAPE_DX& /*dx*/) {}
		// ��邵�̐��@�𓾂�
		virtual bool get_arrow_size(ARROW_SIZE& /*value*/) const noexcept { return false; }
		// ��邵�̌`���𓾂�.
		virtual bool get_arrow_style(ARROW_STYLE& /*value*/) const noexcept { return false; }
		// �}�`���͂ޗ̈�𓾂�.
		virtual void get_bound(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/, D2D1_POINT_2F& /*b_min*/, D2D1_POINT_2F& /*b_max*/) const noexcept {}
		// �[�̌`���𓾂�.
		virtual bool get_cap_style(CAP_STYLE& /*value*/) const noexcept { return false; }
		// �p�۔��a�𓾂�.
		virtual bool get_corner_radius(D2D1_POINT_2F& /*value*/) const noexcept { return false; }
		// �j���̒[�̌`���𓾂�.
		virtual bool get_dash_cap(D2D1_CAP_STYLE& /*value*/) const noexcept { return false; }
		// �j���̗l���𓾂�.
		virtual bool get_dash_patt(DASH_PATT& /*value*/) const noexcept { return false; }
		// �j���̌`���𓾂�.
		virtual bool get_dash_style(D2D1_DASH_STYLE& /*value*/) const noexcept { return false; }
		// �h��Ԃ��F�𓾂�.
		virtual bool get_fill_color(D2D1_COLOR_F& /*value*/) const noexcept { return false; }
		// ���̂̐F�𓾂�.
		virtual bool get_font_color(D2D1_COLOR_F& /*value*/) const noexcept { return false; }
		// ���̖��𓾂�.
		virtual bool get_font_family(wchar_t*& /*value*/) const noexcept { return false; }
		// ���̂̑傫���𓾂�.
		virtual bool get_font_size(float& /*value*/) const noexcept { return false; }
		// ���̂̉����𓾂�.
		virtual bool get_font_stretch(DWRITE_FONT_STRETCH& /*value*/) const noexcept { return false; }
		// ���̂̎��̂𓾂�.
		virtual bool get_font_style(DWRITE_FONT_STYLE& /*value*/) const noexcept { return false; }
		// ���̂̑����𓾂�.
		virtual bool get_font_weight(DWRITE_FONT_WEIGHT& /*value*/) const noexcept { return false; }
		// ����̊�̑傫���𓾂�.
		virtual bool get_grid_base(float& /*value*/) const noexcept { return false; }
		// ����̐F�𓾂�.
		virtual bool get_grid_color(D2D1_COLOR_F& /*value*/) const noexcept { return false; }
		// ����������𓾂�.
		virtual bool get_grid_emph(GRID_EMPH& /*value*/) const noexcept { return false; }
		// ����̕\���𓾂�.
		virtual bool get_grid_show(GRID_SHOW& /*value*/) const noexcept { return false; }
		// ����ɍ��킹��𓾂�.
		virtual bool get_grid_snap(bool& /*value*/) const noexcept { return false; }
		// �摜�̕s�����x�𓾂�.
		virtual bool get_image_opacity(float& /*value*/) const noexcept { return false; }
		// �����̂Ȃ��̃}�C�^�[�����𓾂�.
		virtual bool get_join_limit(float& /*value*/) const noexcept { return false; }
		// �����̂Ȃ��𓾂�.
		virtual bool get_join_style(D2D1_LINE_JOIN& /*value*/) const noexcept { return false; }
		// �ߖT�̒��_�𓾂�.
		virtual bool get_pos_nearest(const D2D1_POINT_2F /*pos*/, float& /*dd*/, D2D1_POINT_2F& /*value*/) const noexcept { return false; }
		// ���ʂ̈ʒu�𓾂�.
		virtual	void get_pos_anch(const uint32_t /*anch*/, D2D1_POINT_2F&/*value*/) const noexcept {}
		// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
		virtual void get_pos_min(D2D1_POINT_2F& /*value*/) const noexcept {}
		// �J�n�ʒu�𓾂�.
		virtual bool get_pos_start(D2D1_POINT_2F& /*value*/) const noexcept { return false; }
		// �p���̐F�𓾂�.
		virtual bool get_sheet_color(D2D1_COLOR_F& /*value*/) const noexcept { return false; }
		// �p���̊g�嗦�𓾂�.
		virtual bool get_sheet_scale(float& /*value*/) const noexcept { return false; }
		// �p���̑傫���𓾂�.
		virtual bool get_sheet_size(D2D1_SIZE_F& /*value*/) const noexcept { return false; }
		// ���g�̐F�𓾂�.
		virtual bool get_stroke_color(D2D1_COLOR_F& /*value*/) const noexcept { return false; }
		// ���̂̑����𓾂�
		virtual bool get_stroke_width(float& /*value*/) const noexcept { return false; }
		// �i���̂��낦�𓾂�.
		virtual bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& /*value*/) const noexcept { return false; }
		// ������̂��낦�𓾂�.
		virtual bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& /*value*/) const noexcept { return false; }
		// ������𓾂�.
		virtual bool get_text_content(wchar_t*& /*value*/) const noexcept { return false; }
		// �s�Ԃ𓾂�.
		virtual bool get_text_line_sp(float& /*value*/) const noexcept { return false; }
		// ������̎��̗͂]���𓾂�.
		virtual bool get_text_padding(D2D1_SIZE_F& /*value*/) const noexcept { return false; }
		// �����͈͂𓾂�
		virtual bool get_text_range(DWRITE_TEXT_RANGE& /*value*/) const noexcept { return false; }
		// ���_�𓾂�.
		virtual size_t get_verts(D2D1_POINT_2F /*v_pos*/[]) const noexcept { return 0; };
		// �ʒu���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F /*t_pos*/) const noexcept { return ANCH_TYPE::ANCH_SHEET; }
		// �͈͂Ɋ܂܂�邩���肷��.
		virtual bool in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept { return false; }
		// �������ꂽ�����肷��.
		virtual bool is_deleted(void) const noexcept { return false; }
		// �I������Ă邩���肷��.
		virtual bool is_selected(void) const noexcept { return false; }
		// ���������ړ�����.
		virtual	bool move(const D2D1_POINT_2F /*value*/) { return false; }
		// �l���邵�̐��@�Ɋi�[����.
		virtual bool set_arrow_size(const ARROW_SIZE& /*value*/) { return false; }
		// �l���邵�̌`���Ɋi�[����.
		virtual bool set_arrow_style(const ARROW_STYLE /*value*/) { return false; }
		// �l��[�̌`���Ɋi�[����.
		virtual bool set_cap_style(const CAP_STYLE& /*value*/) { return false; }
		// �l���p�۔��a�Ɋi�[����.
		virtual bool set_corner_radius(const D2D1_POINT_2F& /*alue*/) noexcept { return false; }
		// �l��j���̒[�̌`���Ɋi�[����.
		virtual bool set_dash_cap(const D2D1_CAP_STYLE& /*value*/) { return false; }
		// �l��j���̗l���Ɋi�[����.
		virtual bool set_dash_patt(const DASH_PATT& /*value*/) { return false; }
		// �l����g�̌`���Ɋi�[����.
		virtual bool set_dash_style(const D2D1_DASH_STYLE /*value*/) { return false; }
		// �l���������ꂽ������Ɋi�[����.
		virtual bool set_delete(const bool /*value*/) noexcept { return false; }
		// �l��h��Ԃ��F�Ɋi�[����.
		virtual bool set_fill_color(const D2D1_COLOR_F& /*value*/) noexcept { return false; }
		// �l�����̂̐F�Ɋi�[����.
		virtual bool set_font_color(const D2D1_COLOR_F& /*value*/) noexcept { return false; }
		// �l�����̖��Ɋi�[����.
		virtual bool set_font_family(wchar_t* const /*value*/) { return false; }
		// �l�����̂̑傫���Ɋi�[����.
		virtual bool set_font_size(const float /*value*/) { return false; }
		// �l�����̂̉����Ɋi�[����.
		virtual bool set_font_stretch(const DWRITE_FONT_STRETCH /*value*/) { return false; }
		// �l�����̂̎��̂Ɋi�[����.
		virtual bool set_font_style(const DWRITE_FONT_STYLE /*value*/) { return false; }
		// �l�����̂̑����Ɋi�[����.
		virtual bool set_font_weight(const DWRITE_FONT_WEIGHT /*value*/) { return false; }
		// �l�����̑傫���Ɋi�[����.
		virtual bool set_grid_base(const float /*value*/) noexcept { return false; }
		// �l�����̐F�Ɋi�[����.
		virtual bool set_grid_color(const D2D1_COLOR_F& /*value*/) noexcept { return false; }
		// �l�����̋����Ɋi�[����.
		virtual bool set_grid_emph(const GRID_EMPH& /*value*/) noexcept { return false; }
		// �l�����̕\���Ɋi�[����.
		virtual bool set_grid_show(const GRID_SHOW /*value*/) noexcept { return false; }
		// �l�����ɍ��킹��Ɋi�[����.
		virtual bool set_grid_snap(const bool /*value*/) noexcept { return false; }
		// �摜�̕s�����x�𓾂�.
		virtual bool set_image_opacity(const float /*value*/) noexcept { return false; }
		// �l������̂Ȃ��̃}�C�^�[�����Ɋi�[����.
		virtual bool set_join_limit(const float& /*value*/) { return false; }
		// �l������̂Ȃ��Ɋi�[����.
		virtual bool set_join_style(const D2D1_LINE_JOIN& /*value*/) { return false; }
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		virtual bool set_pos_anch(const D2D1_POINT_2F /*value*/, const uint32_t /*anch*/, const float /*limit*/, const bool /*keep_aspect*/) { return false; }
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		virtual bool set_pos_start(const D2D1_POINT_2F /*value*/) { return false; }
		// �l��p���̐F�Ɋi�[����.
		virtual bool set_sheet_color(const D2D1_COLOR_F& /*value*/) noexcept { return false; }
		// �l��p���̊g�嗦�Ɋi�[����.
		virtual bool set_sheet_scale(const float /*value*/) noexcept { return false; }
		// �l��p���̑傫���Ɋi�[����.
		virtual bool set_sheet_size(const D2D1_SIZE_F /*value*/) noexcept { return false; }
		// �l��I������Ă邩����Ɋi�[����.
		virtual bool set_select(const bool /*value*/) noexcept { return false; }
		// �l����g�̐F�Ɋi�[����.
		virtual bool set_stroke_color(const D2D1_COLOR_F& /*value*/) noexcept { return false; }
		// �l�����̂̑����Ɋi�[����.
		virtual bool set_stroke_width(const float /*value*/) noexcept { return false; }
		// �l��i���̂��낦�Ɋi�[����.
		virtual bool set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT /*value*/) { return false; }
		// �l�𕶎���̂��낦�Ɋi�[����.
		virtual bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT /*value*/) { return false; }
		// �l�𕶎���Ɋi�[����.
		virtual bool set_text_content(wchar_t* const /*value*/) { return false; }
		// �l���s�ԂɊi�[����.
		virtual bool set_text_line_sp(const float /*value*/) { return false; }
		// �l�𕶎���̗]���Ɋi�[����.
		virtual bool set_text_padding(const D2D1_SIZE_F /*value*/) { return false; }
		// �l�𕶎��͈͂Ɋi�[����.
		virtual bool set_text_range(const DWRITE_TEXT_RANGE /*value*/) { return false; }
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& /*dt_writer*/) const {}
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void write_svg(DataWriter const& /*dt_writer*/) const {}
	};

	//------------------------------
	// �摜
	//------------------------------
	struct ShapeImage : Shape {
		bool m_is_deleted = false;	// �������ꂽ������
		bool m_is_selected = false;	// �I�����ꂽ������
		D2D1_POINT_2F m_pos;	// �n�_�̈ʒu
		D2D1_SIZE_F m_view;	// �\�����@
		D2D1_RECT_F m_rect;	// �r�b�g�}�b�v�̋�`
		D2D1_SIZE_U m_size;	// �r�b�g�}�b�v�̌���
		uint8_t* m_data;	// �r�b�g�}�b�v�̃f�[�^
		float m_opac = 1.0f;	// �r�b�g�}�b�v�̕s�����x (�A���t�@�l�Ə�Z)
		D2D1_SIZE_F m_ratio{ 1.0, 1.0 };	// �\�����@�ƌ����̏c����
		winrt::com_ptr<ID2D1Bitmap1> m_dx_bitmap{ nullptr };

		// �}�`��j������.
		~ShapeImage(void);
		// �X�g���[���Ɋi�[����.
		IAsyncAction copy_to(const winrt::guid enc_id, IRandomAccessStream& ra_stream);
		// �}�`��\������.
		void draw(SHAPE_DX& dx);
		// �}�`���͂ޗ̈�𓾂�.
		void get_bound(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/, D2D1_POINT_2F& /*b_min*/, D2D1_POINT_2F& /*b_max*/) const noexcept;
		// �摜�̕s�����x�𓾂�.
		bool get_image_opacity(float& /*value*/) const noexcept;
		// �ߖT�̒��_�𓾂�.
		bool get_pos_nearest(const D2D1_POINT_2F /*pos*/, float& /*dd*/, D2D1_POINT_2F& /*value*/) const noexcept;
		// ���ʂ̈ʒu�𓾂�.
		void get_pos_anch(const uint32_t /*anch*/, D2D1_POINT_2F&/*value*/) const noexcept;
		// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
		void get_pos_min(D2D1_POINT_2F& /*value*/) const noexcept;
		// �J�n�ʒu�𓾂�.
		bool get_pos_start(D2D1_POINT_2F& /*value*/) const noexcept;
		// ���_�𓾂�.
		size_t get_verts(D2D1_POINT_2F /*v_pos*/[]) const noexcept;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F /*t_pos*/) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept;
		// �������ꂽ�����肷��.
		bool is_deleted(void) const noexcept { return m_is_deleted; }
		// �I������Ă邩���肷��.
		bool is_selected(void) const noexcept { return m_is_selected; }
		// ���������ړ�����.
		bool move(const D2D1_POINT_2F value);
		// ���̑傫���ɖ߂�.
		void resize_origin(void) noexcept;
		// �l���������ꂽ������Ɋi�[����.
		bool set_delete(const bool value) noexcept;
		// �l���摜�̕s�����x�Ɋi�[����.
		bool set_image_opacity(const float value) noexcept;
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float limit, const bool keep_aspect);
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		bool set_pos_start(const D2D1_POINT_2F /*value*/);
		// �l��I������Ă邩����Ɋi�[����.
		bool set_select(const bool /*value*/) noexcept;
		// �}�`���쐬����.
		ShapeImage(const D2D1_POINT_2F pos, const D2D1_SIZE_F view_size, const SoftwareBitmap& bitmap);
		// �}�`���쐬����.
		//ShapeImage(const D2D1_POINT_2F center_pos, DataReader const& dt_reader);
		// �f�[�^���[�_�[����ǂݍ���.
		ShapeImage(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(const wchar_t f_name[], DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// �p��
	//------------------------------
	struct ShapeSheet : Shape {

		// ��邵
		ARROW_SIZE m_arrow_size{ DEF_ARROW_SIZE };	// ��邵�̐��@
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// ��邵�̌`��

		// �p��
		D2D1_POINT_2F m_corner_rad{ DEF_GRID_LEN, DEF_GRID_LEN };	// �p�۔��a

		// �h��Ԃ�
		D2D1_COLOR_F m_fill_color{ S_WHITE };	// �h��Ԃ��̐F

		// ����
		D2D1_COLOR_F m_font_color{ S_BLACK };	// ���̂̐F (MainPage �̃R���X�g���N�^�Őݒ�)
		wchar_t* m_font_family = nullptr;	// ���̖�
		float m_font_size = DEF_FONT_SIZE;	// ���̂̑傫��
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// ���̂̐L�k
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// ���̂̎���
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// ���̂̑���

		// ���g
		CAP_STYLE m_cap_style{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };	// �[�̌`��
		D2D1_CAP_STYLE m_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// �j���̒[�̌`��
		DASH_PATT m_dash_patt{ DEF_DASH_PATT };	// �j���̗l��
		D2D1_DASH_STYLE m_dash_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// �j���̌`��
		float m_join_limit = DEF_MITER_LIMIT;	// ���̂Ȃ��̃}�C�^�[����
		D2D1_LINE_JOIN m_join_style = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER;	// ���g�̂Ȃ�
		D2D1_COLOR_F m_stroke_color{ S_BLACK };	// ���g�̐F (MainPage �̃R���X�g���N�^�Őݒ�)
		float m_stroke_width = 1.0;	// ���g�̑���

		// ������
		float m_text_line_sp = 0.0f;	// �s�� (DIPs 96dpi�Œ�)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_p = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// �i���̑���
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;	// ������̑���
		D2D1_SIZE_F m_text_padding{ DEF_TEXT_MARGIN };	// ������̍��E�Ə㉺�̗]��

		// �摜
		float m_image_opac = 1.0f;	// �摜�̕s������

		// ����
		D2D1_COLOR_F m_grid_color{ ACCENT_COLOR };	// ����̐F
		float m_grid_base = DEF_GRID_LEN - 1.0f;	// ����̊�̑傫�� (�� -1 �����l)
		GRID_SHOW m_grid_show = GRID_SHOW::BACK;	// ����̕\��
		GRID_EMPH m_grid_emph{ GRID_EMPH_0 };	// ����̋���
		bool m_grid_snap = true;	// ����ɍ��킹��

		// �p��
		D2D1_COLOR_F m_sheet_color{ S_WHITE };	// �w�i�F (MainPage �̃R���X�g���N�^�Őݒ�)
		float m_sheet_scale = 1.0f;	// �g�嗦
		D2D1_SIZE_F	m_sheet_size{ 0.0f, 0.0f };	// �傫�� (MainPage �̃R���X�g���N�^�Őݒ�)

		//------------------------------
		// shape_sheet.cpp
		//------------------------------

		// �Ȑ��̕⏕����\������.
		void draw_auxiliary_bezi(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// ���~�̕⏕����\������.
		void draw_auxiliary_elli(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// �����̕⏕����\������.
		void draw_auxiliary_line(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// ���`�̕⏕����\������.
		void draw_auxiliary_rect(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// ���p�`�̕⏕����\������.
		void draw_auxiliary_poly(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos, const POLY_OPTION& p_opt);
		// �p�ە��`�̕⏕����\������.
		void draw_auxiliary_rrect(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// �����\������,
		void draw_grid(SHAPE_DX const& dx, const D2D1_POINT_2F offset);
		// ��邵�̐��@�𓾂�.
		bool get_arrow_size(ARROW_SIZE& value) const noexcept;
		// ��邵�̌`���𓾂�.
		bool get_arrow_style(ARROW_STYLE& value) const noexcept;
		// �[�̌`���𓾂�.
		bool get_cap_style(CAP_STYLE& value) const noexcept;
		// �p�۔��a�𓾂�.
		bool get_corner_radius(D2D1_POINT_2F& value) const noexcept;
		// �j���̒[�̌`���𓾂�.
		bool get_dash_cap(D2D1_CAP_STYLE& value) const noexcept;
		// �j���̗l���𓾂�.
		bool get_dash_patt(DASH_PATT& value) const noexcept;
		// �j���̌`���𓾂�.
		bool get_dash_style(D2D1_DASH_STYLE& value) const noexcept;
		// �h��Ԃ��F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& value) const noexcept;
		// ���̂̐F�𓾂�.
		bool get_font_color(D2D1_COLOR_F& value) const noexcept;
		// ���̖��𓾂�.
		bool get_font_family(wchar_t*& value) const noexcept;
		// ���̂̑傫���𓾂�.
		bool get_font_size(float& value) const noexcept;
		// ���̂̉����𓾂�.
		bool get_font_stretch(DWRITE_FONT_STRETCH& value) const noexcept;
		// ���̂̎��̂𓾂�.
		bool get_font_style(DWRITE_FONT_STYLE& value) const noexcept;
		// ���̂̑����𓾂�.
		bool get_font_weight(DWRITE_FONT_WEIGHT& value) const noexcept;
		// ����̊�̑傫���𓾂�.
		bool get_grid_base(float& value) const noexcept;
		// ����̐F�𓾂�.
		bool get_grid_color(D2D1_COLOR_F& value) const noexcept;
		// ����̋����𓾂�.
		bool get_grid_emph(GRID_EMPH& value) const noexcept;
		// ����̕\���̏�Ԃ𓾂�.
		bool get_grid_show(GRID_SHOW& value) const noexcept;
		// ����ɍ��킹��𓾂�.
		bool get_grid_snap(bool& value) const noexcept;
		// �摜�̕s�����x�𓾂�.
		bool get_image_opacity(float& value) const noexcept;
		// �����̂Ȃ��̃}�C�^�[�����𓾂�.
		bool get_join_limit(float& value) const noexcept;
		// �����̂Ȃ��𓾂�.
		bool get_join_style(D2D1_LINE_JOIN& value) const noexcept;
		// �p���̐F�𓾂�.
		bool get_sheet_color(D2D1_COLOR_F& value) const noexcept;
		// �p���̐F�𓾂�.
		bool get_sheet_size(D2D1_SIZE_F& value) const noexcept;
		// �p���̊g�嗦�𓾂�.
		bool get_sheet_scale(float& value) const noexcept;
		// ���g�̐F�𓾂�.
		bool get_stroke_color(D2D1_COLOR_F& value) const noexcept;
		// ���̂̑����𓾂�
		bool get_stroke_width(float& value) const noexcept;
		// �i���̂��낦�𓾂�.
		bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& value) const noexcept;
		// ������̂��낦�𓾂�.
		bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& value) const noexcept;
		// �s�Ԃ𓾂�.
		bool get_text_line_sp(float& value) const noexcept;
		// ������̎��̗͂]���𓾂�.
		bool get_text_padding(D2D1_SIZE_F& value) const noexcept;
		// �f�[�^���[�_�[����ǂݍ���.
		void read(DataReader const& dt_reader);
		// �l���邵�̐��@�Ɋi�[����.
		bool set_arrow_size(const ARROW_SIZE& value);
		// �l���邵�̌`���Ɋi�[����.
		bool set_arrow_style(const ARROW_STYLE value);
		// �}�`�̑����l���i�[����.
		void set_attr_to(const Shape* s) noexcept;
		// �l���摜�̕s�����x�Ɋi�[����.
		bool set_image_opacity(const float value) noexcept;
		// �l��[�̌`���Ɋi�[����.
		bool set_cap_style(const CAP_STYLE& value);
		// �l���p�۔��a�Ɋi�[����.
		bool set_corner_radius(const D2D1_POINT_2F& value) noexcept;
		// �l��j���̒[�̌`���Ɋi�[����.
		bool set_dash_cap(const D2D1_CAP_STYLE& value);
		// �l��j���̗l���Ɋi�[����.
		bool set_dash_patt(const DASH_PATT& value);
		// �l����g�̌`���Ɋi�[����.
		bool set_dash_style(const D2D1_DASH_STYLE value);
		// �l��h��Ԃ��F�Ɋi�[����.
		bool set_fill_color(const D2D1_COLOR_F& value) noexcept;
		// �l�����̂̐F�Ɋi�[����.
		bool set_font_color(const D2D1_COLOR_F& value) noexcept;
		// ���̖��Ɋi�[����.
		bool set_font_family(wchar_t* const value);
		// ���̂̑傫���Ɋi�[����.
		bool set_font_size(const float value);
		// �l�����̂̐L�k�Ɋi�[����.
		bool set_font_stretch(const DWRITE_FONT_STRETCH value);
		// �l�����̂̎��̂Ɋi�[����.
		bool set_font_style(const DWRITE_FONT_STYLE value);
		// �l�����̂̑����Ɋi�[����.
		bool set_font_weight(const DWRITE_FONT_WEIGHT value);
		// �l�����̊�̑傫���Ɋi�[����.
		bool set_grid_base(const float value) noexcept;
		// �l�����̐F�Ɋi�[����.
		bool set_grid_color(const D2D1_COLOR_F& value) noexcept;
		// �l�����̋����Ɋi�[����.
		bool set_grid_emph(const GRID_EMPH& value) noexcept;
		// �l�����̕\���Ɋi�[����.
		bool set_grid_show(const GRID_SHOW value) noexcept;
		// �l�����ɍ��킹��Ɋi�[����.
		bool set_grid_snap(const bool value) noexcept;
		// �l������̂Ȃ��̃}�C�^�[�����Ɋi�[����.
		bool set_join_limit(const float& value);
		// �l������̂Ȃ��Ɋi�[����.
		bool set_join_style(const D2D1_LINE_JOIN& value);
		// �l��p���̐F�Ɋi�[����.
		bool set_sheet_color(const D2D1_COLOR_F& value) noexcept;
		// �l��p���̊g�嗦�Ɋi�[����.
		bool set_sheet_scale(const float value) noexcept;
		// �l��p���̐��@�Ɋi�[����.
		bool set_sheet_size(const D2D1_SIZE_F value) noexcept;
		// �l����g�̐F�Ɋi�[����.
		bool set_stroke_color(const D2D1_COLOR_F& value) noexcept;
		// �l�����̂̑����Ɋi�[����.
		bool set_stroke_width(const float value) noexcept;
		// �l��i���̂��낦�Ɋi�[����.
		bool set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT value);
		// �l�𕶎���̂��낦�Ɋi�[����.
		bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT value);
		// �l���s�ԂɊi�[����.
		bool set_text_line_sp(const float value);
		// �l�𕶎���̗]���Ɋi�[����.
		bool set_text_padding(const D2D1_SIZE_F value);
		// �f�[�^���[�_�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �O���[�v�}�`
	//------------------------------
	struct ShapeGroup : Shape {
		SHAPE_LIST m_list_grouped;	// �O���[�v�����ꂽ�}�`�̃��X�g

		// �}�`���쐬����.
		ShapeGroup(void) {};

		//------------------------------
		// shape_group.cpp
		//------------------------------

		// �}�`��j������.
		~ShapeGroup(void);
		// �}�`��\������.
		void draw(SHAPE_DX& dx);
		// �}�`���͂ޗ̈�𓾂�.
		void get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept;
		// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
		void get_pos_min(D2D1_POINT_2F& value) const noexcept;
		// �J�n�ʒu�𓾂�.
		bool get_pos_start(D2D1_POINT_2F& value) const noexcept;
		// ������}�`���܂ނ����肷��.
		bool has_text(void) noexcept;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// ��������Ă��邩���肷��.
		bool is_deleted(void) const noexcept;
		// �I������Ă��邩���肷��.
		bool is_selected(void) const noexcept;
		// ���������ړ�����.
		bool move(const D2D1_POINT_2F value);
		// �l���������ꂽ������Ɋi�[����.
		bool set_delete(const bool value) noexcept;
		// �l��I�����ꂽ������Ɋi�[����.
		bool set_select(const bool value) noexcept;
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		bool set_pos_start(const D2D1_POINT_2F value);
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapeGroup(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ���g�̂ЂȌ^
	//------------------------------
	struct ShapeStroke : Shape {
		bool m_is_deleted = false;	// �������ꂽ������
		bool m_is_selected = false;	// �I�����ꂽ������
		D2D1_POINT_2F m_pos{ 0.0f, 0.0f };	// �J�n�ʒu
		std::vector<D2D1_POINT_2F> m_diff;	// ���̈ʒu�ւ̍���
		CAP_STYLE m_cap_style{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };	// �[�̌`��
		D2D1_COLOR_F m_stroke_color{ S_BLACK };	// ���g�̐F
		D2D1_CAP_STYLE m_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// �j���̒[�̌`��
		DASH_PATT m_dash_patt{ DEF_DASH_PATT };	// �j���̗l��
		D2D1_DASH_STYLE m_dash_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// �j���̌`��
		float m_join_limit = DEF_MITER_LIMIT;		// ���̂Ȃ��̃}�C�^�[�����̔䗦
		D2D1_LINE_JOIN m_join_style = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;	// ���̂Ȃ�
		float m_stroke_width = 1.0f;	// ���g�̑���

		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_stroke_style{};	// D2D �X�g���[�N�X�^�C��

		//------------------------------
		// shape_stroke.cpp
		//------------------------------

		// �}�`��j������.
		~ShapeStroke(void);
		// �}�`���͂ޗ̈�𓾂�.
		void get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept;
		// �[�̌`���𓾂�.
		bool get_cap_style(CAP_STYLE& value) const noexcept;
		// �j���̒[�̌`���𓾂�.
		bool get_dash_cap(D2D1_CAP_STYLE& value) const noexcept;
		// �j���̗l���𓾂�.
		bool get_dash_patt(DASH_PATT& value) const noexcept;
		// �j���̌`���𓾂�.
		bool get_dash_style(D2D1_DASH_STYLE& value) const noexcept;
		// �����̂Ȃ��̃}�C�^�[�����𓾂�.
		bool get_join_limit(float& value) const noexcept;
		// �����̂Ȃ��𓾂�.
		bool get_join_style(D2D1_LINE_JOIN& value) const noexcept;
		// �ߖT�̒��_�𓾂�.
		bool get_pos_nearest(const D2D1_POINT_2F a_pos, float& dd, D2D1_POINT_2F& value) const noexcept;
		// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
		virtual void get_pos_min(D2D1_POINT_2F& value) const noexcept;
		// ���ʂ̈ʒu�𓾂�.
		virtual	void get_pos_anch(const uint32_t /*anch*/, D2D1_POINT_2F& value) const noexcept;
		// �J�n�ʒu�𓾂�.
		virtual bool get_pos_start(D2D1_POINT_2F& value) const noexcept;
		// ���g�̐F�𓾂�.
		bool get_stroke_color(D2D1_COLOR_F& value) const noexcept;
		// ���g�̑����𓾂�.
		bool get_stroke_width(float& value) const noexcept;
		// ���_�𓾂�.
		size_t get_verts(D2D1_POINT_2F v_pos[]) const noexcept;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept;
		// �������ꂽ�����肷��.
		bool is_deleted(void) const noexcept { return m_is_deleted; }
		// �I������Ă邩���肷��.
		bool is_selected(void) const noexcept { return m_is_selected; }
		// ���������ړ�����.
		virtual	bool move(const D2D1_POINT_2F value);
		// �l��[�̌`���Ɋi�[����.
		bool set_cap_style(const CAP_STYLE& value);
		// �l��j���̒[�̌`���Ɋi�[����.
		bool set_dash_cap(const D2D1_CAP_STYLE& value);
		// �l��j���̗l���Ɋi�[����.
		bool set_dash_patt(const DASH_PATT& value);
		// �l����g�̌`���Ɋi�[����.
		bool set_dash_style(const D2D1_DASH_STYLE value);
		// �l���������ꂽ������Ɋi�[����.
		bool set_delete(const bool value) noexcept { if (m_is_deleted != value) { m_is_deleted = value;  return true; } return false; }
		// �l������̂Ȃ��̃}�C�^�[�����Ɋi�[����.
		bool set_join_limit(const float& value);
		// �l������̂Ȃ��Ɋi�[����.
		bool set_join_style(const D2D1_LINE_JOIN& value);
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float limit, const bool keep_aspect);
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		virtual bool set_pos_start(const D2D1_POINT_2F value);
		// �l��I������Ă邩����Ɋi�[����.
		bool set_select(const bool value) noexcept { if (m_is_selected != value) { m_is_selected = value; return true; } return false; }
		// �l����g�̐F�Ɋi�[����.
		bool set_stroke_color(const D2D1_COLOR_F& value) noexcept;
		// �l����g�̑����Ɋi�[����.
		bool set_stroke_width(const float value) noexcept;
		// �}�`���쐬����.
		ShapeStroke(const size_t d_cnt, const ShapeSheet* s_attr);
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapeStroke(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ��������
	//------------------------------
	struct ShapeLineA : ShapeStroke {
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// ��邵�̌`��
		ARROW_SIZE m_arrow_size{ DEF_ARROW_SIZE };	// ��邵�̐��@
		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_arrow_style{ nullptr };	// ��邵�� D2D �X�g���[�N�X�^�C��
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_arrow_geom{ nullptr };	// ��邵�� D2D �p�X�W�I���g��

		//------------------------------
		// shape_line.cpp
		//------------------------------

		// �R���X�g���N�^
		ShapeLineA(const size_t d_cnt, const ShapeSheet* s_attr, const bool a_none = false) :
			ShapeStroke(d_cnt, s_attr),
			m_arrow_style(a_none ? ARROW_STYLE::NONE : s_attr->m_arrow_style),
			m_arrow_size(s_attr->m_arrow_size),
			m_d2d_arrow_geom(nullptr),
			m_d2d_arrow_style(nullptr)
		{}
		// �}�`���쐬����.
		ShapeLineA(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr);
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapeLineA(DataReader const& dt_reader);
		// �}�`��j������.
		~ShapeLineA(void);
		// �\������.
		void draw(SHAPE_DX& dx);
		// ��邵�̐��@�𓾂�.
		bool get_arrow_size(ARROW_SIZE& size) const noexcept;
		// ��邵�̌`���𓾂�.
		bool get_arrow_style(ARROW_STYLE& value) const noexcept;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �l���邵�̐��@�Ɋi�[����.
		bool set_arrow_size(const ARROW_SIZE& value);
		// �l���邵�̌`���Ɋi�[����.
		bool set_arrow_style(const ARROW_STYLE value);
		//	�l��, ���ʂ̈ʒu�Ɋi�[����. 
		bool set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float limit, const bool keep_aspect);
		// �l���n�_�Ɋi�[����.
		bool set_pos_start(const D2D1_POINT_2F value);
		// ���������ړ�����.
		bool move(const D2D1_POINT_2F value);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const;
		// �l��[�̌`���Ɋi�[����.
		bool set_cap_style(const CAP_STYLE& value);
		// �l������̂Ȃ��̃}�C�^�[�����Ɋi�[����.
		bool set_join_limit(const float& value);
		// �l������̂Ȃ��Ɋi�[����.
		bool set_join_style(const D2D1_LINE_JOIN& value);
	};

	//------------------------------
	// ���`
	//------------------------------
	struct ShapeRect : ShapeStroke {
		D2D1_COLOR_F m_fill_color{ S_WHITE };		// �h��Ԃ��F

		//------------------------------
		// shape_rect.cpp
		//------------------------------

		// �}�`���쐬����.
		ShapeRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr);
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapeRect(DataReader const& dt_reader);
		// �\������.
		void draw(SHAPE_DX& dx);
		// �ߖT�̒��_�𓾂�.
		bool get_pos_nearest(const D2D1_POINT_2F a_pos, float& dd, D2D1_POINT_2F& value) const noexcept;
		// ���_�𓾂�.
		size_t get_verts(D2D1_POINT_2F v_pos[]) const noexcept;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �}�`�̕��ʂ��ʒu���܂ނ����肷��.
		uint32_t hit_test_anch(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �h��Ԃ��̐F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& value) const noexcept;
		// �l��h��Ԃ��̐F�Ɋi�[����.
		bool set_fill_color(const D2D1_COLOR_F& value) noexcept;
		// ���ʂ̈ʒu�𓾂�.
		void get_pos_anch(const uint32_t anch, D2D1_POINT_2F& value) const noexcept;
		//	�l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float limit, const bool keep_aspect);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ��K
	// �쐬�������Ƃŕ�����̑����̕ύX�͂ł��Ȃ�.
	//------------------------------
	struct ShapeRuler : ShapeRect {
		float m_grid_base = DEF_GRID_LEN - 1.0f;	// ����̑傫�� (�� -1 �����l)
		winrt::com_ptr<IDWriteTextFormat> m_dw_text_format{};	// �e�L�X�g�t�H�[�}�b�g

		//------------------------------
		// shape_ruler.cpp
		//------------------------------

		// �}�`��j������.
		~ShapeRuler(void);
		// �}�`��\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �}�`���쐬����.
		ShapeRuler(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr);
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapeRuler(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(DataWriter const& /*dt_writer*/) const {}
	};

	//------------------------------
	// ���~
	//------------------------------
	struct ShapeElli : ShapeRect {
		// �}�`���쐬����.
		ShapeElli(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr) :
			ShapeRect::ShapeRect(b_pos, b_vec, s_attr)
		{}
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapeElli(DataReader const& dt_reader) :
			ShapeRect::ShapeRect(dt_reader)
		{}

		//------------------------------
		// shape_elli.cpp
		//------------------------------

		// �}�`��\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// �p�ە��`
	//------------------------------
	struct ShapeRRect : ShapeRect {
		D2D1_POINT_2F m_corner_rad{ DEF_GRID_LEN, DEF_GRID_LEN };		// �p�ە����̔��a

		//------------------------------
		// shape_rrect.cpp
		// �p�ە��`
		//------------------------------

		// �}�`���쐬����.
		ShapeRRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr);
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapeRRect(DataReader const& dt_reader);
		// �}�`��\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �p�۔��a�𓾂�.
		bool get_corner_radius(D2D1_POINT_2F& value) const noexcept;
		// ���ʂ̈ʒu�𓾂�.
		void get_pos_anch(const uint32_t anch, D2D1_POINT_2F& value) const noexcept;
		//	�l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float limit, const bool keep_aspect);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// �܂���̂ЂȌ^
	//------------------------------
	struct ShapePath : ShapeLineA {
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_path_geom{};	// �܂���� D2D �p�X�W�I���g��

		//------------------------------
		// shape_path.cpp
		// �܂���̂ЂȌ^
		//------------------------------

		virtual void create_path_geometry(const SHAPE_DX& /*dx*/) {}

		// �p�X�W�I���g�����쐬����.
		//virtual void create_path_geometry(ID2D1Factory3* const/*d_factory*/) {}
		// �}�`���쐬����.
		ShapePath(const size_t d_cnt, const ShapeSheet* s_attr, const bool s_closed) :
			ShapeLineA::ShapeLineA(d_cnt, s_attr, s_closed), m_d2d_path_geom(nullptr) {}
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapePath(DataReader const& dt_reader) :
			ShapeLineA::ShapeLineA(dt_reader), m_d2d_path_geom(nullptr) {}
		// �}�`��j������.
		~ShapePath(void) { if (m_d2d_path_geom != nullptr) m_d2d_path_geom = nullptr; }
		// ���������ړ�����.
		bool move(const D2D1_POINT_2F value);
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float limit, const bool keep_aspect);
		// �l���邵�̐��@�Ɋi�[����.
		bool set_arrow_size(const ARROW_SIZE& value);
		// �l���邵�̌`���Ɋi�[����.
		bool set_arrow_style(const ARROW_STYLE value);
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		bool set_pos_start(const D2D1_POINT_2F value);
		// �f�[�^���C�^�[�ɏ�������.
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

		void create_path_geometry(const SHAPE_DX& dx);


		// ���`�����Ƃɑ��p�`���쐬����.
		static void create_poly_by_bbox(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const POLY_OPTION& p_opt, D2D1_POINT_2F v_pos[], D2D1_POINT_2F& v_vec) noexcept;
		// �p�X�W�I���g�����쐬����.
		//void create_path_geometry(ID2D1Factory3* const d_factory);
		// �\������
		void draw(SHAPE_DX& dx);
		// �h��Ԃ��F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& value) const noexcept;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �l���邵�̌`���Ɋi�[����.
		bool set_arrow_style(const ARROW_STYLE value);
		// �l��h��Ԃ��F�Ɋi�[����.
		bool set_fill_color(const D2D1_COLOR_F& value) noexcept;
		// �}�`���쐬����.
		ShapePoly(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr, const POLY_OPTION& p_opt);
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapePoly(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& /*dt_writer*/) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(DataWriter const& /*dt_writer*/) const;
	};

	//------------------------------
	// �Ȑ�
	//------------------------------
	struct ShapeBezi : ShapePath {

		//------------------------------
		// shape_bezi.cpp
		// �x�W�F�Ȑ�
		//------------------------------

		void create_path_geometry(const SHAPE_DX& dx);

		// �p�X�W�I���g�����쐬����.
		//void create_path_geometry(ID2D1Factory3* const d_factory);
		// �\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �}�`���쐬����.
		ShapeBezi(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_attr);
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapeBezi(DataReader const& dt_reader);
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ������
	//------------------------------
	struct ShapeText : ShapeRect {
		static wchar_t** s_available_fonts;		// �L���ȏ��̖�

		DWRITE_TEXT_RANGE m_select_range{ 0, 0 };	// �I��͈�
		D2D1_COLOR_F m_font_color{ S_BLACK };	// ���̂̐F
		wchar_t* m_font_family = nullptr;	// ���̖�
		float m_font_size = DEF_FONT_SIZE;	// ���̂̑傫��
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// ���̂̐L�k
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// ���̂̎���
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// ���̂̑���
		wchar_t* m_text = nullptr;	// ������
		float m_text_line_sp = 0.0f;	// �s�� (DIPs 96dpi�Œ�)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_p = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// �i�����낦
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;	// ��������
		D2D1_SIZE_F m_text_padding{ DEF_TEXT_MARGIN };	// ������̂܂��̏㉺�ƍ��E�̗]��

		winrt::com_ptr<IDWriteTextLayout> m_dw_layout{};	// �������\�����邽�߂̃��C�A�E�g
		float m_dw_descent = 0.0f;	// �f�B�Z���g
		UINT32 m_dw_line_cnt = 0;	// �s�̌v�ʂ̗v�f��
		DWRITE_LINE_METRICS* m_dw_line_metrics = nullptr;	// �s�̌v��
		UINT32 m_dw_selected_cnt = 0;	// �I��͈͂̌v�ʂ̗v�f��
		DWRITE_HIT_TEST_METRICS* m_dw_selected_metrics = nullptr;	// �I��͈͂̌v��
		UINT32 m_dw_test_cnt = 0;	// �ʒu�̌v�ʂ̗v�f��
		DWRITE_HIT_TEST_METRICS* m_dw_test_metrics = nullptr;	// �ʒu�̌v��

		// �}�`��j������.
		~ShapeText(void);
		// �g�̑傫���𕶎���ɍ��킹��.
		bool adjust_bbox(const float g_len);
		// �e�L�X�g���C�A�E�g��j�����č쐬����.
		void create_text_layout(IDWriteFactory3* d_factory);
		// �v�ʂ�j�����č쐬����.
		void create_text_metrics(IDWriteFactory3* d_factory);
		// �����̋󔒂���菜��.
		void delete_bottom_blank(void) noexcept;
		// �\������.
		void draw(SHAPE_DX& dx);
		// �I�����ꂽ�����͈͂�h��.
		void fill_range(SHAPE_DX& dx, const D2D1_POINT_2F t_min);
		// ������̘g��\������.
		void draw_frame(SHAPE_DX& dx, const D2D1_POINT_2F t_min);
		// �L���ȏ��̖�����v�f�𓾂�.
		static wchar_t* get_available_font(const uint32_t i);
		// ���̂̐F�𓾂�.
		bool get_font_color(D2D1_COLOR_F& value) const noexcept;
		// ���̖��𓾂�.
		bool get_font_family(wchar_t*& value) const noexcept;
		// ���̂̑傫���𓾂�.
		bool get_font_size(float& value) const noexcept;
		// ���̂̐L�k�𓾂�.
		bool get_font_stretch(DWRITE_FONT_STRETCH& value) const noexcept;
		// ���̂̎��̂𓾂�.
		bool get_font_style(DWRITE_FONT_STYLE& value) const noexcept;
		// ���̂̑����𓾂�.
		bool get_font_weight(DWRITE_FONT_WEIGHT& value) const noexcept;
		// �i���̂��낦�𓾂�.
		bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& value) const noexcept;
		// ������̂��낦�𓾂�.
		bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& value) const noexcept;
		// ������𓾂�.
		bool get_text_content(wchar_t*& value) const noexcept;
		// �s�Ԃ𓾂�.
		bool get_text_line_sp(float& value) const noexcept;
		// ������̗]���𓾂�.
		bool get_text_padding(D2D1_SIZE_F& value) const noexcept;
		// �����͈͂𓾂�.
		bool get_text_range(DWRITE_TEXT_RANGE& value) const noexcept;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �L���ȏ��̖������肵, �L���Ȃ���Ƃ̏��̖���j����, �L���ȏ��̖��v�f�ւ̃|�C���^�[�ƒu��������.
		static bool is_available_font(wchar_t*& font);
		// �L���ȏ��̖��̔z���j������.
		static void release_available_fonts(void);
		// �L���ȏ��̖��̔z���ݒ肷��.
		static void set_available_fonts(void);
		// �l�����̂̐F�Ɋi�[����.
		bool set_font_color(const D2D1_COLOR_F& value) noexcept;
		// �l�����̖��Ɋi�[����.
		bool set_font_family(wchar_t* const value);
		// �l�����̂̑傫���Ɋi�[����.
		bool set_font_size(const float value);
		// �l�����̂̐L�k�Ɋi�[����.
		bool set_font_stretch(const DWRITE_FONT_STRETCH value);
		// �l�����̂̎��̂Ɋi�[����.
		bool set_font_style(const DWRITE_FONT_STYLE value);
		// �l�����̂̑����Ɋi�[����.
		bool set_font_weight(const DWRITE_FONT_WEIGHT value);
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float limit, const bool keep_aspect);
		// �l��i���̂��낦�Ɋi�[����.
		bool set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT value);
		// �l�𕶎���̂��낦�Ɋi�[����.
		bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT value);
		// �l�𕶎���Ɋi�[����.
		bool set_text_content(wchar_t* const value);
		// �l���s�ԂɊi�[����.
		bool set_text_line_sp(const float value);
		// �l�𕶎���̗]���Ɋi�[����.
		bool set_text_padding(const D2D1_SIZE_F value);
		// �l�𕶎��͈͂Ɋi�[����.
		bool set_text_range(const DWRITE_TEXT_RANGE value);
		// �}�`���쐬����.
		ShapeText(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, wchar_t* const text, const ShapeSheet* s_attr);
		// �f�[�^���[�_�[����}�`��ǂݍ���.
		ShapeText(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

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

	// �j���̗l�������������肷��.
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
	inline bool pt_in_anch(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos) noexcept
	{
		const double a = Shape::s_anch_len * 0.5;
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

	// ��_�̈ʒu���ׂĂ��ꂼ��傫���l�����߂�.
	// a	����̈ʒu
	// b	��������̈ʒu
	// c	����
	inline void pt_max(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x > b.x ? a.x : b.x;
		c.y = a.y > b.y ? a.y : b.y;
	}

	// ��_�̈ʒu���ׂĂ��ꂼ�ꏬ�����l�����߂�.
	// a	����̈ʒu
	// b	��������̈ʒu
	// c	����
	inline void pt_min(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x < b.x ? a.x : b.x;
		c.y = a.y < b.y ? a.y : b.y;
	}

	// �ʒu�̕����𔽑΂ɂ���.
	// a	�ʒu
	// b	����
	inline void pt_neg(const D2D1_POINT_2F a, D2D1_POINT_2F& b) noexcept
	{
		b.x = -a.x;
		b.y = -a.y;
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

	// �ʒu�ɃX�J���[���|����, �ʒu��������.
	// a	�ʒu
	// b	�X�J���[�l
	// c	������ʒu
	// d	����
	inline void pt_mul(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
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
	inline void pt_mul(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.X * b + c.x);
		d.y = static_cast<FLOAT>(a.Y * b + c.y);
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

	// �F���s���������肷��.
	// value	�F
	// �߂�l	�s�����Ȃ�� true, �����Ȃ�� false.
	inline bool is_opaque(const D2D1_COLOR_F& value) noexcept
	{
		const uint32_t a = static_cast<uint32_t>(round(value.a * 255.0f));
		return (a & 0xff) > 0;
	}

}