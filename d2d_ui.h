//
// d2d_ui.h
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
	// D2D_UI �����L���Ă���A�v���P�[�V�������A�f�o�C�X������ꂽ�Ƃ��܂��͍쐬���ꂽ�Ƃ��ɒʒm���󂯂邽�߂̃C���^�[�t�F�C�X��񋟂���.
	interface IDeviceNotify {
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	//------------------------------
	// D2D
	//------------------------------
	struct D2D_UI {

		// D3D �I�u�W�F�N�g

		winrt::com_ptr<ID3D11Device3> m_d3d_device{ nullptr };
		winrt::com_ptr<ID3D11DeviceContext3> m_d3d_context{ nullptr };

		// DXGI �X���b�v�`�F�[��

		winrt::com_ptr<IDXGISwapChain3> m_dxgi_swap_chain{ nullptr };

		// D2D/DWeite �`��R���|�[�l���g

		static winrt::com_ptr<ID2D1Factory3> m_d2d_fanctory;
		static winrt::com_ptr<IDWriteFactory3> m_dwrite_factory;
		//winrt::com_ptr<ID2D1Device2> m_d2dDevice;
		winrt::com_ptr<ID2D1DeviceContext2> m_d2d_context{ nullptr };
		//winrt::com_ptr<ID2D1Bitmap1> d2d_target_bitmap;

		// XAML �R���g���[��

		//SwapChainPanel m_swapChainPanel{};	// �p�l���ւ̕ێ����ꂽ�Q��

		// �f�o�C�X����

		//FLOAT m_d3d_target_width;
		//FLOAT m_d3d_target_height;
		//FLOAT m_output_width;
		//FLOAT m_output_height;
		FLOAT m_logical_width = 0.0f;
		FLOAT m_logical_height = 0.0f;
		winrt::Windows::Graphics::Display::DisplayOrientations m_nativeOrientation = winrt::Windows::Graphics::Display::DisplayOrientations::None;
		winrt::Windows::Graphics::Display::DisplayOrientations m_currentOrientation = winrt::Windows::Graphics::Display::DisplayOrientations::None;
		float m_logical_dpi = -1.0f;
		float m_compositionScaleX = 1.0f;
		float m_compositionScaleY = 1.0f;
		D3D_DRIVER_TYPE m_d3d_driver_type = D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_NULL;

		// �X�P�[�����O�p�� DPI �ƍ����{��

		float m_effectiveDpi = -1.0f;
		float m_effectiveCompositionScaleX;
		float m_effectiveCompositionScaleY;

		// �����ϊ��p�̍s��

		//D2D1::Matrix3x2F m_orientationTransform2D;
		//DirectX::XMFLOAT4X4 m_orientationTransform3D;

		// �ʒm���󂯂邽�߂̃C���^�[�t�F�C�X.

		IDeviceNotify* m_deviceNotify = nullptr;

		// �}�`�\���p�� D2D �I�u�W�F�N�g
		winrt::com_ptr<ID2D1DrawingStateBlock1> m_state_block{ nullptr };	// �`���Ԃ̕ۑ��u���b�N
		winrt::com_ptr<ID2D1SolidColorBrush> m_range_brush{ nullptr };	// �����͈͂̕����F�u���V
		winrt::com_ptr<ID2D1SolidColorBrush> m_solid_color_brush{ nullptr };	// �}�`�̐F�u���V

		// �`�����j������.
		void Release(void)
		{
			Trim();
			m_state_block = nullptr;
			m_solid_color_brush = nullptr;
			m_range_brush = nullptr;
			//m_aux_style = nullptr;
			//m_aux_brush = nullptr;
			//m_anch_brush = nullptr;
			//m_swapChainPanel = nullptr;
			//d2d_target_bitmap = nullptr;
			m_d2d_context = nullptr;
			//m_d2dDevice = nullptr;
			m_d3d_context = nullptr;
			m_d3d_device = nullptr;
		}

		//------------------------------
		// shape_dx.cpp
		//------------------------------

		// �E�B���h�E�T�C�Y�ˑ��̃��\�[�X������������.
		void CreateWindowSizeDependentResources(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const&);
		// D3D �f�o�C�X���\����, D3D �Ƒ��݉^�p����� D2D �f�o�C�X���쐬����.
		D2D_UI(void);
		// ���ׂẴf�o�C�X ���\�[�X���č쐬��, ���݂̏�ԂɍĐݒ肷��.
		void HandleDeviceLost(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const&);
		// �X���b�v�`�F�[���̓��e����ʂɕ\������.
		void Present(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const&);
		// �f�o�C�X������ꂽ�Ƃ��ƍ쐬���ꂽ�Ƃ��ɒʒm���󂯂�悤�ɁADeviceNotify ��o�^����.
		void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		// �`����� XAML �X���b�v�`�F�[���p�l����ݒ肷��.
		// ���̃��\�b�h��, UI �R���g���[�����쐬 (�܂��͍č쐬) ���ꂽ�Ƃ��ɌĂяo�����.
		void SetSwapChainPanel(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& xamk_scp);
		// �`����ɕ\���̈�̑傫����ݒ肷��.
		// ���̃��\�b�h�́ASizeChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
		void SetLogicalSize2(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const&, const D2D1_SIZE_F logicalSize);
		// �`����� DPI ��ݒ肷��.
		// ���̃��\�b�h�́ADpiChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
		void SetDpi(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const&, const float dpi);
		// �`����Ƀf�o�C�X�̌�����ݒ肷��.
		// ���̃��\�b�h�́AOrientationChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
		void SetCurrentOrientation(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& swap_chain_panel, winrt::Windows::Graphics::Display::DisplayOrientations currentOrientation);
		// �`����ɍ����{����ݒ肷��.
		// ���̃��\�b�h�́ACompositionScaleChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
		void SetCompositionScale(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& swap_chain_panel, float compositionScaleX, float compositionScaleY);
		// �o�b�t�@�[������ł��邱�Ƃ�, �h���C�o�[�Ɏ���.
		// ���̃��\�b�h��, �A�v������~/���f�����Ƃ��ɌĂяo�����.
		void Trim();
		// �\���f�o�C�X���L���ɂȂ���.
		void ValidateDevice(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& swap_chain_panel);
	};
}

