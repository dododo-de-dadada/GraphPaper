//
// d2d_ui.h
//
// �}�`�̕`���
//
// VisualStudio �̃v���W�F�N�g�e���v���[�g�� DirectX 11 �A�v����
// �܂܂�� DeviceResources �N���X�����Ƃ�,
// 3D �\����t���[�����[�g�Ɋւ����������폜����,
// 2D �\���ɕK�v�ȕ������c���Ă���.
//
#pragma once
//#define WIN_UI	3
#define WIN_UI	2
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <dxgi1_4.h>
#include <winrt/Windows.Graphics.Display.h>
#if WIN_UI == 3
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#else
#include <winrt/Windows.UI.Xaml.Controls.h>
#endif

namespace winrt::GraphPaper::implementation
{
#if WIN_UI == 3
	using winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel;
#else
	using winrt::Windows::Graphics::Display::DisplayOrientations;
	using winrt::Windows::UI::Xaml::Controls::SwapChainPanel;
#endif
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

		static winrt::com_ptr<ID2D1Factory3> m_d2d_factory;
		winrt::com_ptr<ID2D1DeviceContext2> m_d2d_context{ nullptr };

		// XAML �R���g���[��
		SwapChainPanel m_swap_chain_panel;	// �p�l���ւ̕ێ����ꂽ�Q��

		// �f�o�C�X����

		FLOAT m_logical_width = 0.0f;
		FLOAT m_logical_height = 0.0f;
#if WIN_UI == 3
#else
		DisplayOrientations m_nativeOrientation = DisplayOrientations::None;
		DisplayOrientations m_current_orientation = DisplayOrientations::None;
#endif
		float m_logical_dpi = 96.0f;
		float m_composition_scale_x = 1.0f;
		float m_composition_scale_y = 1.0f;
		D3D_DRIVER_TYPE m_d3d_driver_type = D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_NULL;

		// �X�P�[�����O�p�� DPI �ƍ����{��

		float m_effective_dpi = 0.0f;
		float m_effectiveCompositionScaleX = 1.0f;
		float m_effectiveCompositionScaleY = 1.0f;

		// �ʒm���󂯂邽�߂̃C���^�[�t�F�C�X.

		IDeviceNotify* m_deviceNotify = nullptr;

		// �`�����j������.
		void Release(void)
		{
			Trim();
		}

		//------------------------------
		// d2d_ui.cpp
		//------------------------------

		// �E�B���h�E�T�C�Y�ˑ��̃��\�[�X������������.
		void CreateWindowSizeDependentResources(void);
		// D3D �f�o�C�X���\����, D3D �Ƒ��݉^�p����� D2D �f�o�C�X���쐬����.
		D2D_UI(void);
		// ���ׂẴf�o�C�X ���\�[�X���č쐬��, ���݂̏�ԂɍĐݒ肷��.
		void HandleDeviceLost(void);
		// �X���b�v�`�F�[���̓��e����ʂɕ\������.
		void Present(void);
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
		void SetDpi(const float dpi);
		// �`����Ƀf�o�C�X�̌�����ݒ肷��.
		// ���̃��\�b�h�́AOrientationChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
		void SetCurrentOrientation(winrt::Windows::Graphics::Display::DisplayOrientations current_orientation);
		// �`����ɍ����{����ݒ肷��.
		// ���̃��\�b�h�́ACompositionScaleChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
		void SetCompositionScale(float composition_scale_x, float composition_scale_y);
		// �o�b�t�@�[������ł��邱�Ƃ�, �h���C�o�[�Ɏ���.
		// ���̃��\�b�h��, �A�v������~/���f�����Ƃ��ɌĂяo�����.
		void Trim();
		// �\���f�o�C�X���L���ɂȂ���.
		void ValidateDevice(void);
	};
}

