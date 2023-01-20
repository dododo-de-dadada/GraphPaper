#include "pch.h"
#include "PrintPage.h"
#if __has_include("PrintPage.g.cpp")
#include "PrintPage.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::GraphPaper::implementation
{
    int32_t PrintPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void PrintPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
    /*
    void PrintPage::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        //Button().Content(box_value(L"Clicked"));
    }
    */
}
