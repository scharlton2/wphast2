// TimeControlMultiPropertyPage2.cpp : implementation file
//

#include "stdafx.h"
#include "WPhast.h"
#include "TimeControlMultiPropertyPage2.h"

#include "Units.h"
#include "Global.h"

// CTimeControlMultiPropertyPage2 dialog

IMPLEMENT_DYNAMIC(CTimeControlMultiPropertyPage2, baseCTimeControlMultiPropertyPage2)
CTimeControlMultiPropertyPage2::CTimeControlMultiPropertyPage2()
	: baseCTimeControlMultiPropertyPage2(CTimeControlMultiPropertyPage2::IDD)
{
	CUnits units;
	this->m_strTimeUnits = CGlobal::GetStdTimeUnits(units.time.defined ? units.time.input : units.time.si).c_str();	

	// load property descriptions
	//
	CGlobal::LoadRTFString(this->m_sTimeEndRTF,         IDR_TC_TE_TIME_END_RTF);
	CGlobal::LoadRTFString(this->m_sTimeEndUnitsRTF,    IDR_TC_TE_TIME_END_UNITS_RTF);

	CGlobal::LoadRTFString(this->m_sTSTimeRTF,          IDR_TC_TS_TIME_RTF);
	CGlobal::LoadRTFString(this->m_sTSUnitsRTF,         IDR_TC_TS_UNITS_RTF);
	CGlobal::LoadRTFString(this->m_sTSTimeStepRTF,      IDR_TC_TS_TIME_STEP_RTF);
	CGlobal::LoadRTFString(this->m_sTSTimeStepUnitsRTF, IDR_TC_TS_TIME_STEP_UNITS_RTF);
	CGlobal::LoadRTFString(this->m_sTSTimeStartRTF,     IDR_TC_START_RTF);
}

CTimeControlMultiPropertyPage2::~CTimeControlMultiPropertyPage2()
{
}

void CTimeControlMultiPropertyPage2::DoDataExchange(CDataExchange* pDX)
{
	baseCTimeControlMultiPropertyPage2::DoDataExchange(pDX);

	DDX_GridControl(pDX, IDC_TIMEEND_GRID, m_gridTimeEnd);
	DDX_GridControl(pDX, IDC_TIMESTEP_GRID, m_gridTimeStep);

	DDX_Control(pDX, IDC_STARTTIME_COMBO, m_cboStartTime);

	DDX_Control(pDX, IDC_DESC_RICHEDIT, m_wndRichEditCtrl);
	if (this->m_bFirstSetActive)
	{
		// add items to combo
		CGlobal::AddTimeUnits(&this->m_cboStartTime);

		// wrap richedit to window
		this->m_wndRichEditCtrl.SetTargetDevice(NULL, 0);
		this->m_wndRichEditCtrl.SetWindowText(this->m_sTimeEndRTF.c_str());
	}

	if (this->m_bFirstSetActive)
	{
		this->SetupGrids();

		// time_end
		//
		std::set<Ctime>::iterator tIter = this->m_tc2.m_timeEnd.begin();
		for (int nRow = 1; tIter != this->m_tc2.m_timeEnd.end(); ++tIter, ++nRow)
		{
			ASSERT(tIter->value_defined);
			::DDX_TextGridControl(pDX, IDC_TIMEEND_GRID, nRow, 0, tIter->value);
			if (tIter->input)
			{
				CString str(tIter->input);
				::DDX_TextGridControl(pDX, IDC_TIMEEND_GRID, nRow, 1, str);
			}
		}

		// time_step
		//
		CTimeSeries<Ctime>::iterator tsIter = this->m_tc2.m_timeStep.begin();
		for (int nRow = 1; tsIter != this->m_tc2.m_timeStep.end(); ++tsIter, ++nRow)
		{
			Ctime t(tsIter->first);
			ASSERT(t.value_defined);
			::DDX_TextGridControl(pDX, IDC_TIMESTEP_GRID, nRow, 0, t.value);
			if (t.input)
			{
				CString str(t.input);
				::DDX_TextGridControl(pDX, IDC_TIMESTEP_GRID, nRow, 1, str);
			}

			ASSERT(tsIter->second.value_defined);
			::DDX_TextGridControl(pDX, IDC_TIMESTEP_GRID, nRow, 2, tsIter->second.value);
			if (tsIter->second.input)
			{
				CString str(tsIter->second.input);
				::DDX_TextGridControl(pDX, IDC_TIMESTEP_GRID, nRow, 3, str);
			}
		}

		// start_time
		//
		if (this->m_tc2.m_timeStart.value_defined)
		{
			::DDX_Text(pDX, IDC_STARTTIME_EDIT, this->m_tc2.m_timeStart.value);
		}
		else
		{
			double d = 0;
			::DDX_Text(pDX, IDC_STARTTIME_EDIT, d);
		}

		if (this->m_tc2.m_timeStart.input)
		{
			VERIFY(this->m_cboStartTime.SelectString(0,
				CGlobal::GetStdTimeUnits(this->m_tc2.m_timeStart.input).c_str()) != CB_ERR);
		}
		/***
		else
		{
			VERIFY(this->m_cboStartTime.SelectString(0,
				CGlobal::GetStdTimeUnits(this->m_strTimeUnits).c_str()) != CB_ERR);
		}
		***/
	}

	if (pDX->m_bSaveAndValidate)
	{
		CTimeControl2 tc2;

		// time_end
		//
		int nIDC = IDC_TIMEEND_GRID;
		CModGridCtrlEx* pGrid = &this->m_gridTimeEnd;

		for (int iRow = pGrid->GetFixedRowCount(); iRow < pGrid->GetRowCount(); ++iRow)
		{
			// time start
			//
			CString start;
			::DDX_TextGridControl(pDX, nIDC, iRow, 0, start);
			if (start.IsEmpty())
			{
				continue;
			}

			double d;
			::DDX_TextGridControl(pDX, nIDC, iRow, 0, d);
			Ctime t;
			t.type = TT_UNITS;
			t.SetValue(d);

			// time units
			//
			::DDX_TextGridControl(pDX, nIDC, iRow, 1, start);
			if (!start.IsEmpty())
			{
				t.SetInput(start);
			}
			tc2.m_timeEnd.insert(t);
		}

		// time_step
		//
		nIDC = IDC_TIMESTEP_GRID;
		pGrid = &this->m_gridTimeStep;

		for (int iRow = pGrid->GetFixedRowCount(); iRow < pGrid->GetRowCount(); ++iRow)
		{
			// time start
			//
			CString start;
			::DDX_TextGridControl(pDX, nIDC, iRow, 0, start);
			if (start.IsEmpty())
			{
				continue;
			}

			double d;
			::DDX_TextGridControl(pDX, nIDC, iRow, 0, d);
			Ctime t;
			t.type = TT_UNITS;
			t.SetValue(d);

			// time units
			//
			::DDX_TextGridControl(pDX, nIDC, iRow, 1, start);
			if (!start.IsEmpty())
			{
				t.SetInput(start);
			}

			::DDX_TextGridControl(pDX, nIDC, iRow, 2, d);
			Ctime t2;
			t2.type = TT_UNITS;
			t2.SetValue(d);

			// time units
			//
			::DDX_TextGridControl(pDX, nIDC, iRow, 3, start);
			if (!start.IsEmpty())
			{
				t2.SetInput(start);
			}
			tc2.m_timeStep[t] = t2;
		}

		// start time
		//

		// value
		//
		::DDX_Text(pDX, IDC_STARTTIME_EDIT, tc2.m_timeStart.value);
		tc2.m_timeStart.value_defined = TRUE;
		tc2.m_timeStart.type = TT_UNITS;

		// units
		//
		int nCurSel = this->m_cboStartTime.GetCurSel();
		if (nCurSel != CB_ERR)
		{
			CString sVal;
			this->m_cboStartTime.GetLBText(nCurSel, sVal);
			///if (sVal.Compare(this->m_strTimeUnits) != 0)
			{
				VERIFY(tc2.m_timeStart.SetInput(sVal) == OK);
			}
		}

		this->m_tc2 = tc2;
	}
}


BEGIN_MESSAGE_MAP(CTimeControlMultiPropertyPage2, baseCTimeControlMultiPropertyPage2)
	ON_NOTIFY(GVN_SELCHANGED, IDC_TIMESTEP_GRID, OnSelChangedTimeStep)
	ON_NOTIFY(GVN_SETFOCUS, IDC_TIMESTEP_GRID, OnSelChangedTimeStep)

	ON_NOTIFY(GVN_SELCHANGED, IDC_TIMEEND_GRID, OnSelChangedTimeEnd)
	ON_NOTIFY(GVN_SETFOCUS, IDC_TIMEEND_GRID, OnSelChangedTimeEnd)

	ON_EN_SETFOCUS(IDC_STARTTIME_EDIT, &CTimeControlMultiPropertyPage2::OnEnSetfocusStarttimeEdit)
	ON_CBN_SETFOCUS(IDC_STARTTIME_COMBO, &CTimeControlMultiPropertyPage2::OnCbnSetfocusStarttimeCombo)
END_MESSAGE_MAP()


// CTimeControlMultiPropertyPage2 message handlers

BOOL CTimeControlMultiPropertyPage2::OnInitDialog()
{
	baseCTimeControlMultiPropertyPage2::OnInitDialog();

	// Layout controls
	this->CreateRoot(VERTICAL)
		<< 	( pane(HORIZONTAL, GREEDY, 0, 0, 0 )
			<< item(IDC_TIMEEND_GRID, GREEDY)
			<< itemFixed(HORIZONTAL, 20)
			<< item(IDC_TIMESTEP_GRID, GREEDY)
			)
		<< 	( pane(HORIZONTAL, GREEDY, 0, 0, 0 )
			<< item(IDC_STARTTIME_STATIC, NORESIZE|ALIGN_VCENTER)
			<< item(IDC_STARTTIME_EDIT, NORESIZE|ALIGN_VCENTER)
			<< itemFixed(HORIZONTAL, 10)
			<< item(IDC_STARTTIME_COMBO, NORESIZE|ALIGN_VCENTER)
			)
		<< item(IDC_DESC_RICHEDIT, ABSOLUTE_VERT)		
		;
	UpdateLayout();

	/*** TESTING {{
	CWnd* pWnd = this->GetDlgItem(IDC_DESC_STATIC);
	pWnd->ShowWindow(SW_SHOW);
	// Layout controls
	this->CreateRoot(VERTICAL)
		<< 	( pane(HORIZONTAL, GREEDY, 0, 0, 0 )
			<< item(IDC_TIMEEND_GRID, GREEDY)
			<< itemFixed(HORIZONTAL, 20)
			<< item(IDC_TIMESTEP_GRID, GREEDY)
			)
		<< itemFixed(VERTICAL, 10)
		<< (paneCtrl(IDC_DESC_STATIC, HORIZONTAL, GREEDY, nDefaultBorder, 10, 10)
			<< item(IDC_DESC_RICHEDIT, ABSOLUTE_VERT)
			)
		;
	UpdateLayout();
	}} TESTING ***/

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CTimeControlMultiPropertyPage2::SetupGrids(void)
{
#if defined(_DEBUG)
	const int MIN_ROW_COUNT = 655;
#else
	const int MIN_ROW_COUNT = 65536;
#endif

	int nTimeEndRows  = max(2 * this->m_tc2.m_timeEnd.size(), MIN_ROW_COUNT);
	int nTimeStepRows = max(2 * this->m_tc2.m_timeStep.size(), MIN_ROW_COUNT);;

	try
	{
		this->m_gridTimeEnd.SetRowCount(nTimeEndRows);
		this->m_gridTimeEnd.SetColumnCount(2);
		this->m_gridTimeEnd.SetFixedRowCount(1);
		this->m_gridTimeEnd.SetFixedColumnCount(0);
		this->m_gridTimeEnd.EnableTitleTips(FALSE);
		this->m_gridTimeEnd.SetCurrentFocusCell(1, 0);


		this->m_gridTimeStep.SetRowCount(nTimeStepRows);
		this->m_gridTimeStep.SetColumnCount(4);
		this->m_gridTimeStep.SetFixedRowCount(1);
		this->m_gridTimeStep.SetFixedColumnCount(0);
		this->m_gridTimeStep.EnableTitleTips(FALSE);
		this->m_gridTimeStep.SetCurrentFocusCell(1, 2);
	}
	catch (CMemoryException *e)
	{
		e->ReportError();
		e->Delete();
		return FALSE;
	}

	std::vector<CString> vecTimeUnits;
	vecTimeUnits.push_back(_T("seconds"));
	vecTimeUnits.push_back(_T("minutes"));
	vecTimeUnits.push_back(_T("hours"));
	vecTimeUnits.push_back(_T("days"));
	vecTimeUnits.push_back(_T("years"));

	this->m_gridTimeEnd.SetItemText(0, 0, "Time end");
	this->m_gridTimeEnd.SetItemText(0, 1, "Units");
	this->m_gridTimeEnd.SetColumnOptions(1, vecTimeUnits);

	this->m_gridTimeEnd.SetColumnWidth(0, 106);
	this->m_gridTimeEnd.SetColumnWidth(1, 106);

	this->m_gridTimeStep.SetItemText(0, 0, "Time");
	this->m_gridTimeStep.SetItemText(0, 1, "Units");
	this->m_gridTimeStep.SetItemText(0, 2, "Time step");
	this->m_gridTimeStep.SetItemText(0, 3, "Units");

	this->m_gridTimeStep.SetColumnOptions(1, vecTimeUnits);
	this->m_gridTimeStep.SetColumnOptions(3, vecTimeUnits);

	this->m_gridTimeStep.SetColumnWidth(0, 85);
	this->m_gridTimeStep.SetColumnWidth(1, 85);
	this->m_gridTimeStep.SetColumnWidth(2, 85);
	this->m_gridTimeStep.SetColumnWidth(3, 85);


	this->m_gridTimeStep.SetItemText(1, 0, "0"); this->m_gridTimeStep.DisableCell(1, 0);
	this->m_gridTimeStep.SetItemText(1, 1, "");  this->m_gridTimeStep.DisableCell(1, 1);

	return TRUE;
}

void CTimeControlMultiPropertyPage2::SetProperties(const CTimeControl2& r_tc2)
{
	this->m_tc2 = r_tc2;
}

void CTimeControlMultiPropertyPage2::GetProperties(CTimeControl2& r_tc2)
{
	r_tc2 = this->m_tc2;
}

void CTimeControlMultiPropertyPage2::SetUnits(const CUnits& units)
{
	this->m_strTimeUnits = CGlobal::GetStdTimeUnits(units.time.defined ? units.time.input : units.time.si).c_str();
}

void CTimeControlMultiPropertyPage2::OnSelChangedTimeEnd(NMHDR *pNotifyStruct, LRESULT *result)
{
	CCellID focus = this->m_gridTimeEnd.GetFocusCell();

	if (!this->m_gridTimeEnd.IsValid(focus)) return;

	switch (focus.col)
	{
	case 0:
		this->m_wndRichEditCtrl.SetWindowText(this->m_sTimeEndRTF.c_str());
		break;
	case 1:
		this->m_wndRichEditCtrl.SetWindowText(this->m_sTimeEndUnitsRTF.c_str());
		break;
	default:
		ASSERT(FALSE);
	}
}

void CTimeControlMultiPropertyPage2::OnSelChangedTimeStep(NMHDR *pNotifyStruct, LRESULT *result)
{
	CCellID focus = this->m_gridTimeStep.GetFocusCell();

	if (!this->m_gridTimeStep.IsValid(focus)) return;

	switch (focus.col)
	{
	case 0:
		this->m_wndRichEditCtrl.SetWindowText(this->m_sTSTimeRTF.c_str());
		break;
	case 1:
		this->m_wndRichEditCtrl.SetWindowText(this->m_sTSUnitsRTF.c_str());
		break;
	case 2:
		this->m_wndRichEditCtrl.SetWindowText(this->m_sTSTimeStepRTF.c_str());
		break;
	case 3:
		this->m_wndRichEditCtrl.SetWindowText(this->m_sTSTimeStepUnitsRTF.c_str());
		break;
	default:
		ASSERT(FALSE);
	}
}

BOOL CTimeControlMultiPropertyPage2::PreTranslateMessage(MSG* pMsg)
{
	// Add your specialized code here and/or call the base class
	if (pMsg->hwnd == this->m_cboStartTime.GetSafeHwnd())
	{
		if (pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_DELETE || pMsg->wParam == VK_BACK))
		{
			// allow clearing of combo
			//
			TRACE("cbo recd delete\n");
			this->m_cboStartTime.SetCurSel(-1);
		}
	}
	return ETSLayoutPropertyPageXP::PreTranslateMessage(pMsg);
}

void CTimeControlMultiPropertyPage2::OnEnSetfocusStarttimeEdit()
{
	this->m_wndRichEditCtrl.SetWindowText(this->m_sTSTimeStartRTF.c_str());
}

void CTimeControlMultiPropertyPage2::OnCbnSetfocusStarttimeCombo()
{
	this->m_wndRichEditCtrl.SetWindowText(this->m_sTSTimeStartRTF.c_str());
}
