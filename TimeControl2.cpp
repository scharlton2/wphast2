#include "StdAfx.h"
#include "TimeControl2.h"

#include "SetTimeControl2Action.h"
#include "WPhastDoc.h"
#include "TimeControlMultiPropertyPage2.h"

CTimeControl2::CTimeControl2(void)
{
}

CTimeControl2::CTimeControl2(const struct time_series& timeStep, const struct time *timeEnd, int nTimeEnd)
{
	this->m_timeStep = timeStep;
	ASSERT(this->m_timeStep.size() == (size_t)timeStep.count_properties);

	for (int i = 0; i < nTimeEnd; ++i)
	{
		Ctime t(timeEnd[i]);
		this->m_timeEnd.insert(t);
	}
	ASSERT(this->m_timeEnd.size() == (size_t)nTimeEnd);
}

CTimeControl2::~CTimeControl2(void)
{
}

void CTimeControl2::Edit(CTreeCtrl* pTreeCtrl)
{
	ASSERT(this);
	ASSERT(pTreeCtrl);

	CFrameWnd *pFrame = (CFrameWnd*)::AfxGetApp()->m_pMainWnd;
	ASSERT_VALID(pFrame);
	CWPhastDoc* pDoc = reinterpret_cast<CWPhastDoc*>(pFrame->GetActiveDocument());
	ASSERT_VALID(pDoc);

	// show property sheet
	//
	ETSLayoutPropertySheet propSheet("Time Control", ::AfxGetMainWnd());

	CTimeControlMultiPropertyPage2 tcPage;
	tcPage.SetProperties(*this);

	propSheet.AddPage(&tcPage);

	switch (propSheet.DoModal())
	{
		case IDOK:
			{
				CTimeControl2 newTimeControl;
				tcPage.GetProperties(newTimeControl);
				pDoc->Execute(new CSetTimeControl2Action(pDoc, newTimeControl));
			}
			break;
		case IDCANCEL:
			break;
		default:
			ASSERT(FALSE);
	}
}

void CTimeControl2::Insert(CTreeCtrl* pTreeCtrl, HTREEITEM htiTimeControl)
{
	// remove all previous items
	//
	while (HTREEITEM hChild = pTreeCtrl->GetChildItem(htiTimeControl))
	{
		pTreeCtrl->DeleteItem(hChild);
	}

	pTreeCtrl->SetItemText(htiTimeControl, "TIME_CONTROL");
	pTreeCtrl->SetItemData(htiTimeControl, (DWORD_PTR)this);
	this->m_htiTimeControl = htiTimeControl;

	// time_step
	//
	CString str;
	CString str2;
	str.Format("time_step");
	HTREEITEM hTimeStep = pTreeCtrl->InsertItem(str, htiTimeControl);
	CTimeSeries<Ctime>::const_iterator tsIter = this->m_timeStep.begin();
	for(; tsIter != this->m_timeStep.end(); ++tsIter)
	{
		str.Format("%g", tsIter->first.value);
		ASSERT(tsIter->first.type == UNITS);
		if (tsIter->first.type == UNITS)
		{
			str += " ";
			str += tsIter->first.input;
		}
		str2.Format(" %g", tsIter->second.value);
		str += str2;
		ASSERT(tsIter->second.type == UNITS);
		if (tsIter->second.type == UNITS)
		{
			str += " ";
			str += tsIter->second.input;
		}
		pTreeCtrl->InsertItem(str, hTimeStep);
	}


	// time_end
	//
	str.Format("time_end");
	HTREEITEM hTimeEnd = pTreeCtrl->InsertItem(str, htiTimeControl);
	std::set<Ctime>::const_iterator tIter = this->m_timeEnd.begin();
	for(; tIter != this->m_timeEnd.end(); ++tIter)
	{
		str.Format("%g", tIter->value);
		ASSERT(tIter->type == UNITS);
		if (tIter->type == UNITS)
		{
			str += " ";
			str += tIter->input;
		}
		pTreeCtrl->InsertItem(str, hTimeEnd);
	}
}

void CTimeControl2::Serialize(bool bStoring, hid_t loc_id)
{
	static const char szTimeControl2[] = "TimeControl2";
	static const char szTimeStep[]     = "time_step";
	static const char szTimeEnd[]      = "time_end";
	

	hid_t timecontrol_id = 0;
	hid_t timestep_id    = 0;
	hid_t timeend_id     = 0;

	herr_t status;

	if (bStoring)
	{
		// Create the szTimeControl2 group
		timecontrol_id = ::H5Gcreate(loc_id, szTimeControl2, 0);
		ASSERT(timecontrol_id > 0);

		// Create the szTimeStep group
		timestep_id = ::H5Gcreate(timecontrol_id, szTimeStep, 0);
		ASSERT(timestep_id > 0);

		// Create the szTimeEnd group
		timeend_id = ::H5Gcreate(timecontrol_id, szTimeEnd, 0);
		ASSERT(timeend_id > 0);

	}
	else
	{
		// Open the szTimeControl group
		timecontrol_id = ::H5Gopen(loc_id, szTimeControl2);
		ASSERT(timecontrol_id > 0);

		// Open the szTimeStep group
		timestep_id = ::H5Gopen(timecontrol_id, szTimeStep);
		ASSERT(timestep_id > 0);

		// Open the szTimeEnd group
		timeend_id = ::H5Gopen(timecontrol_id, szTimeEnd);
		ASSERT(timeend_id > 0);
	}

	//{{
	if (timecontrol_id > 0)
	{
		if (timestep_id > 0)
		{
			// serialize this->m_timeStep
			this->m_timeStep.Serialize(bStoring, timestep_id);
			status = ::H5Gclose(timestep_id);
			ASSERT(status >= 0);
		}

		if (timeend_id > 0)
		{
			// serialize this->m_timeEnd
			//////this->m_timeEnd.Serialize(bStoring, timestep_id);
			status = ::H5Gclose(timeend_id);
			ASSERT(status >= 0);
		}

		// close the szTimeControl2 group
		status = ::H5Gclose(timecontrol_id);
		ASSERT(status >= 0);
	}
	//}}

// COMMENT: {4/22/2005 4:29:51 PM}	if (timecontrol_id > 0) {
// COMMENT: {4/22/2005 4:29:51 PM}
// COMMENT: {4/22/2005 4:29:51 PM}		if (timestep_id > 0) {
// COMMENT: {4/22/2005 4:29:51 PM}			// serialize this->m_timeStep
// COMMENT: {4/22/2005 4:29:51 PM}			this->m_timeStep.Serialize(bStoring, timestep_id);
// COMMENT: {4/22/2005 4:29:51 PM}			status = ::H5Gclose(timestep_id);
// COMMENT: {4/22/2005 4:29:51 PM}			ASSERT(status >= 0);
// COMMENT: {4/22/2005 4:29:51 PM}		}
// COMMENT: {4/22/2005 4:29:51 PM}		if (timechange_id > 0) {
// COMMENT: {4/22/2005 4:29:51 PM}			// serialize this->m_timeEnd
// COMMENT: {4/22/2005 4:29:51 PM}			this->m_timeEnd.Serialize(bStoring, timechange_id);
// COMMENT: {4/22/2005 4:29:51 PM}			status = ::H5Gclose(timechange_id);
// COMMENT: {4/22/2005 4:29:51 PM}			ASSERT(status >= 0);
// COMMENT: {4/22/2005 4:29:51 PM}		}
// COMMENT: {4/22/2005 4:29:51 PM}
// COMMENT: {4/22/2005 4:29:51 PM}		// close the szFlowOnly group
// COMMENT: {4/22/2005 4:29:51 PM}		status = ::H5Gclose(timecontrol_id);
// COMMENT: {4/22/2005 4:29:51 PM}		ASSERT(status >= 0);
// COMMENT: {4/22/2005 4:29:51 PM}	}
}