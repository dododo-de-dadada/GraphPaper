#pragma once
#include "pch.h"
#include "SummaryViewModel.g.h"

namespace winrt::GraphPaper::implementation
{
	struct SummaryViewModel : SummaryViewModelT<SummaryViewModel>
	{
		SummaryViewModel()
		{
			m_summaries = winrt::single_threaded_observable_vector<GraphPaper::Summary>();
		}
		Windows::Foundation::Collections::IObservableVector<GraphPaper::Summary> Summaries()
		{
			return m_summaries;
		}
	private:
		GraphPaper::Summary m_smry{ nullptr };
		Windows::Foundation::Collections::IObservableVector<GraphPaper::Summary> m_summaries;
	};
}
