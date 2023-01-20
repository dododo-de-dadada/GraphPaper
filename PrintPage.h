#pragma once

#include "PrintPage.g.h"

namespace winrt::GraphPaper::implementation
{
    struct PrintPage : PrintPageT<PrintPage>
    {
        PrintPage() 
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::GraphPaper::factory_implementation
{
    struct PrintPage : PrintPageT<PrintPage, implementation::PrintPage>
    {
    };
}
