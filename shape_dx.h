//
// shape_dx.h
//
// �}�`�̕`���
//
// VisualStudio �̃v���W�F�N�g�e���v���[�g�� DirectX 11 �A�v����
// �܂܂�� DeviceResources �N���X�����Ƃ�,
// 3D �\����t���[�����[�g�Ɋւ����������폜����, 2D �\����
// �K�v�ȕ������c���Ă���.
// �}�`��\�����邽��, �F�u���V�Ȃ� D2D �I�u�W�F�N�g��
// �ǉ�����Ă���.
//
#pragma once
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <dxgi1_4.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.UI.Xaml.Controls.h>

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::SwapChainPanel;
	using winrt::Windows::Graphics::Display::DisplayOrientations;

	// ��
	constexpr D2D1_COLOR_F S_WHITE{ 1.0f, 1.0f, 1.0f, 1.0f };
	// ��
	constexpr D2D1_COLOR_F S_BLACK{ 0.0f, 0.0f, 0.0f, 1.0f };
	// �⏕���̕s�����x
	constexpr float AUX_OPAC = 0.975f;
	// �⏕���̔j���̔z�u
	constexpr FLOAT AUX_DASHES[] = { 4.0f, 4.0f };
	// �⏕���̔j���̔z�u�̗v�f��
	constexpr UINT32 AUX_DASHES_CONT = sizeof(AUX_DASHES) / sizeof(AUX_DASHES[0]);
	// �⏕���̐��̓���
	constexpr D2D1_STROKE_STYLE_PROPERTIES1 AUX_STYLE {
		D2D1_CAP_STYLE_FLAT,	// startCap
		D2D1_CAP_STYLE_FLAT,	// endCap
		D2D1_CAP_STYLE_ROUND,	// dashCap
		D2D1_LINE_JOIN_MITER_OR_BEVEL,	// lineJoin
		1.0f,	// miterLimit
		D2D1_DASH_STYLE_CUSTOM,	// dashStyle
		0.0f,	// dashOffset
		D2D1_STROKE_TRANSFORM_TYPE_NORMAL
	};
	// �����͈͂̔w�i�F
	constexpr D2D1_COLOR_F RNG_BACK = { 0.0f, 2.0f / 3.0f, 1.0f, 1.0f };
	// �����͈͂̕����F
	constexpr D2D1_COLOR_F RNG_TEXT = { 1.0f, 1.0f, 1.0f, 1.0f };

	// SHAPE_DX �����L���Ă���A�v���P�[�V�������A�f�o�C�X������ꂽ�Ƃ��܂��͍쐬���ꂽ�Ƃ��ɒʒm���󂯂邽�߂̃C���^�[�t�F�C�X��񋟂���.
	interface IDeviceNotify {
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	//------------------------------
	// �}�`�̕`���
	//------------------------------
	struct SHAPE_DX {

		// D3D �I�u�W�F�N�g

		winrt::com_ptr<ID3D11Device3> m_d3dDevice;
		winrt::com_ptr<ID3D11DeviceContext3> m_d3dContext;

		// DXGI �X���b�v�`�F�[��

		winrt::com_ptr<IDXGISwapChain3> m_dxgi_swap_chain;

		// D2D/DWeite �`��R���|�[�l���g

		static winrt::com_ptr<ID2D1Factory3> m_d2dFactory;
		static winrt::com_ptr<IDWriteFactory3> m_dwriteFactory;
		winrt::com_ptr<ID2D1Device2> m_d2dDevice;
		winrt::com_ptr<ID2D1DeviceContext2> m_d2dContext;
		winrt::com_ptr<ID2D1Bitmap1> m_d2dTargetBitmap;

		// XAML �R���g���[��

		SwapChainPanel m_swapChainPanel{};	// �p�l���ւ̕ێ����ꂽ�Q��

		// �f�o�C�X����

		D3D_FEATURE_LEVEL m_d3dFeatureLevel{ D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_1 };
		FLOAT m_d3d_target_width;
		FLOAT m_d3d_target_height;
		FLOAT m_output_width;
		FLOAT m_output_height;
		FLOAT m_logical_width = 0.0f;
		FLOAT m_logical_height = 0.0f;
		DisplayOrientations	m_nativeOrientation = DisplayOrientations::None;
		DisplayOrientations	m_currentOrientation = DisplayOrientations::None;
		float m_logical_dpi = -1.0f;
		float m_compositionScaleX = 1.0f;
		float m_compositionScaleY = 1.0f;
		D3D_DRIVER_TYPE m_d3d_driver_type{ D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_NULL };

		// �X�P�[�����O�p�� DPI �ƍ����{��

		float m_effectiveDpi = -1.0f;
		float m_effectiveCompositionScaleX;
		float m_effectiveCompositionScaleY;

		// �����ϊ��p�̍s��

		D2D1::Matrix3x2F m_orientationTransform2D;
		DirectX::XMFLOAT4X4 m_orientationTransform3D;

		// �ʒm���󂯂邽�߂̃C���^�[�t�F�C�X.

		IDeviceNotify* m_deviceNotify = nullptr;

		// �}�`�\���p�� D2D �I�u�W�F�N�g

		double m_anchor_len = 6.0;	// ���ʂ̕��`�̑傫��
		winrt::com_ptr<ID2D1StrokeStyle1> m_aux_style;	// �⏕���̌`��
		D2D1_COLOR_F m_range_background = RNG_BACK;	// �����͈͂̔w�i�F
		D2D1_COLOR_F m_range_foreground = RNG_TEXT;	// �����͈͂̕����F
		D2D1_COLOR_F m_theme_background = S_WHITE;	// �O�i�F
		D2D1_COLOR_F m_theme_foreground = S_BLACK;	// �w�i�F
		winrt::com_ptr<ID2D1SolidColorBrush> m_range_brush;	// �����͈͂̕����F�u���V
		winrt::com_ptr<ID2D1SolidColorBrush> m_shape_brush;	// �}�`�̐F�u���V
		winrt::com_ptr<ID2D1DrawingStateBlock1> m_state_block;	// �`���Ԃ̕ۑ��u���b�N

		// �`�����j������.
		void Release(void)
		{
			Trim();
			m_state_block = nullptr;
			m_shape_brush = nullptr;
			m_range_brush = nullptr;
			m_aux_style = nullptr;
			//m_aux_brush = nullptr;
			//m_anch_brush = nullptr;
			m_swapChainPanel = nullptr;
			m_d2dTargetBitmap = nullptr;
			m_d2dContext = nullptr;
			m_d2dDevice = nullptr;
			m_d3dContext = nullptr;
			m_d3dDevice = nullptr;
		}

		//------------------------------
		// shape_dx.cpp
		//------------------------------

		// �E�B���h�E�T�C�Y�ˑ��̃��\�[�X������������.
		void CreateWindowSizeDependentResources();
		// D3D �f�o�C�X���\����, D3D �Ƒ��݉^�p����� D2D �f�o�C�X���쐬����.
		SHAPE_DX(void);
		// ���ׂẴf�o�C�X ���\�[�X���č쐬��, ���݂̏�ԂɍĐݒ肷��.
		void HandleDeviceLost();
		// �X���b�v�`�F�[���̓��e����ʂɕ\������.
		void Present();
		// �f�o�C�X������ꂽ�Ƃ��ƍ쐬���ꂽ�Ƃ��ɒʒm���󂯂�悤�ɁADeviceNotify ��o�^����.
		void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		// �`����� XAML �X���b�v�`�F�[���p�l����ݒ肷��.
		// ���̃��\�b�h��, UI �R���g���[�����쐬 (�܂��͍č쐬) ���ꂽ�Ƃ��ɌĂяo�����.
		void SetSwapChainPanel(SwapChainPanel const& xamk_scp);
		// �`����ɕ\���̈�̑傫����ݒ肷��.
		// ���̃��\�b�h�́ASizeChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
		void SetLogicalSize2(const D2D1_SIZE_F logicalSize);
		// �`����� DPI ��ݒ肷��.
		// ���̃��\�b�h�́ADpiChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
		void SetDpi(float dpi);
		// �`����Ƀf�o�C�X�̌�����ݒ肷��.
		// ���̃��\�b�h�́AOrientationChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
		void SetCurrentOrientation(DisplayOrientations currentOrientation);
		// �`����ɍ����{����ݒ肷��.
		// ���̃��\�b�h�́ACompositionScaleChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
		void SetCompositionScale(float compositionScaleX, float compositionScaleY);
		// �o�b�t�@�[������ł��邱�Ƃ�, �h���C�o�[�Ɏ���.
		// ���̃��\�b�h��, �A�v������~/���f�����Ƃ��ɌĂяo�����.
		void Trim();
		// �\���f�o�C�X���L���ɂȂ���.
		void ValidateDevice();
	};
}

