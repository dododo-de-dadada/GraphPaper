#pragma once
//------------------------------
// Summary.h
//
// �ꗗ�̍���
//------------------------------
#include "Summary.g.h"
#include "shape.h"
#include <winrt/Windows.ApplicationModel.Resources.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <MainPage.g.h>
#include <typeinfo>

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::ResourceDictionary;

	// �ꗗ�̗v�f
	struct Summary : SummaryT<Summary>
	{
		// �ꗗ�̗v�f���쐬����.
		// s	�\������}�`.
		// r	���C���y�[�W�̃��\�[�X�f�B�N�V���i��.
		// �}�`�̖��O��, �p�X�A�C�R���̈ړ��ƕ`��̃R�}���h�������ݒ肷��.
		Summary(Shape* s, const ResourceDictionary& r) :
			m_shape(s)
		{
			auto const& t_id = typeid(*s);
			if (t_id == typeid(ShapeBezier)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_bezier")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_bezier/Text");
			}
			else if (t_id == typeid(ShapeEllipse)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_elli")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_ellipse/Text");
			}
			else if (t_id == typeid(ShapeGroup)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_group")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"str_grouped");
			}
			else if (t_id == typeid(ShapeLine)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_line")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_line/Text");
			}
			else if (t_id == typeid(ShapePolygon)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_polygon")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_poly/Text");
			}
			else if (t_id == typeid(ShapeQEllipse)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_qellipse")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_qellipse/Text");
			}
			else if (t_id == typeid(ShapeRect)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_rect")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_rect/Text");
			}
			else if (t_id == typeid(ShapeRRect)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_rrect")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_rrect/Text");
			}
			else if (t_id == typeid(ShapeRuler)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_ruler")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_ruler/Text");
			}
			else if (t_id == typeid(ShapeText)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_text")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_text/Text");
			}
			else if (t_id == typeid(ShapeImage)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_image")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"str_image");
			}
			else if (t_id == typeid(ShapeQEllipse)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_qellipse")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_qellipse/Text");
			}
			else {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_select")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"str_unknown");
			}
		}

		Shape* const get_shape(void) const noexcept
		{
			return m_shape;
		}

		// �ړ��ƕ`��̃R�}���h��������W�I���g���ɕϊ�����.
		static winrt::Windows::UI::Xaml::Media::Geometry Geom(winrt::hstring const& move_and_draw_command)
		{
			using winrt::Windows::UI::Xaml::Markup::XamlReader;
			using winrt::Windows::UI::Xaml::Controls::PathIcon;

			constexpr auto START_TAG = L"<PathIcon xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" Width=\"24\" Height=\"24\"><PathIcon.Data>";
			constexpr auto END_TAG = L"</PathIcon.Data></PathIcon>";
			winrt::hstring xaml = START_TAG + move_and_draw_command + END_TAG;
			auto icon = unbox_value<PathIcon>(XamlReader::Load(xaml));
			auto geom = icon.Data();
			icon.Data(nullptr);
			icon = nullptr;
			return geom;
		}

		// �W�I���g���𓾂�.
		winrt::Windows::UI::Xaml::Media::Geometry Geom()
		{
			return Geom(m_icon);
		}

		// �W�I���g���Ɋi�[����.
		void Geom(winrt::Windows::UI::Xaml::Media::Geometry const& /*geo*/)
		{
		}

		// 16 �i������𓾂�.
		auto Hexa()
		{
			wchar_t buf[64];
			swprintf_s(buf, L"%p", m_shape);
			wchar_t* t = buf;
			while (*t == L'0') {
				t++;
			}
			return winrt::hstring(t);
		}

		// 16 �i������ɒl���i�[����.
		void Hexa(winrt::hstring const& val)
		{
			if (Hexa() == val) {
				return;
			}
			m_changed(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{ L"Hexa" });
		}

		// ���O�𓾂�.
		auto Name()
		{
			return m_name;
		}

		// ���O�ɒl���i�[����.
		void Name(winrt::hstring const& val)
		{
			if (Name() == val) {
				return;
			}
			m_changed(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{ L"Name" });
		}

		winrt::event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
		{
			return m_changed.add(handler);
		}

		void PropertyChanged(winrt::event_token const& token) noexcept
		{
			m_changed.remove(token);
		}

	private:
		Shape* m_shape;	// �}�`
		winrt::hstring m_icon;	// �p�X�A�C�R���̃f�[�^
		winrt::hstring m_name;	// �}�`�̖��O
		winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_changed;
	};
}
