// MediaPropsPage2.cpp : implementation file
//

#include "stdafx.h"
#include "WPhast.h"
#include "MediaPropsPage2.h"

#include "Global.h"


// CMediaPropsPage2 dialog

const TCHAR ACTIVE[]            = _T("Active");
const TCHAR KX[]                = _T("Kx");
const TCHAR KY[]                = _T("Ky");
const TCHAR KZ[]                = _T("Kz");
const TCHAR POROSITY[]          = _T("Porosity");
const TCHAR STORAGE[]           = _T("Specific storage");
const TCHAR ALPHA_LONG[]        = _T("Longitudinal dispersivity");
const TCHAR ALPHA_HORIZONTAL[]  = _T("Horizontal dispersivity");
const TCHAR ALPHA_VERTICAL[]    = _T("Vertical dispersivity");
const TCHAR TORTUOSITY[]        = _T("Tortuosity");


const int SELECTED = 0;
const int SINGLE = 0;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CMediaPropsPage2, CPropsPropertyPage)

CMediaPropsPage2::CMediaPropsPage2()
	: CPropsPropertyPage(CMediaPropsPage2::IDD)
	, ActiveProperty(this, true, false)
	, KxProperty(this, true, false)
	, KyProperty(this, true, false)
	, KzProperty(this, true, false)
	, PorosityProperty(this, true, false)
	, StorageProperty(this, true, false)
	, AlphaLongProperty(this, true, false)
	, AlphaHorizontalProperty(this, true, false)
	, AlphaVerticalProperty(this, true, false)
	, TortuosityProperty(this, true, false)
	, FlowOnly(false)
	, Default(false)
{
	TRACE("In %s\n", __FUNCTION__);

	// load property descriptions
	CGlobal::LoadRTFString(this->m_sDescriptionRTF,     IDR_DESCRIPTION_RTF);
	CGlobal::LoadRTFString(this->m_sActiveRTF,          IDR_MEDIA_ACTIVE_RTF);
	CGlobal::LoadRTFString(this->m_sKxRTF,              IDR_MEDIA_KX_RTF);
	CGlobal::LoadRTFString(this->m_sKyRTF,              IDR_MEDIA_KY_RTF);
	CGlobal::LoadRTFString(this->m_sKzRTF,              IDR_MEDIA_KZ_RTF);
	CGlobal::LoadRTFString(this->m_sPorosityRTF,        IDR_MEDIA_POROSITY_RTF);
	CGlobal::LoadRTFString(this->m_sStorageRTF,         IDR_MEDIA_SPEC_STORAGE_RTF);
	CGlobal::LoadRTFString(this->m_sAlphaLongRTF,       IDR_MEDIA_LONG_DISP_RTF);
	CGlobal::LoadRTFString(this->m_sAlphaHorizontalRTF, IDR_MEDIA_ALPHA_HORZ_RTF);
	CGlobal::LoadRTFString(this->m_sAlphaVerticalRTF,   IDR_MEDIA_ALPHA_VERT_RTF);
	CGlobal::LoadRTFString(this->m_sShellRTF,           IDR_MEDIA_SHELL_RTF);
	CGlobal::LoadRTFString(this->m_sTortuosityRTF,      IDR_MEDIA_TORTUOSITY_RTF);

	TRACE("Out %s\n", __FUNCTION__);
}

CMediaPropsPage2::~CMediaPropsPage2()
{
	TRACE("In %s\n", __FUNCTION__);
	TRACE("Out %s\n", __FUNCTION__);
}

void CMediaPropsPage2::DoDataExchange(CDataExchange* pDX)
{
	TRACE("In %s\n", __FUNCTION__);
	CPropsPropertyPage::DoDataExchange(pDX);

	if (this->TreeCtrl.GetCount() == 0)
	{
		// setup tree
		this->ActiveProperty.treeitem          = this->TreeCtrl.InsertItem(ACTIVE,           TVI_ROOT, TVI_LAST);
		this->KxProperty.treeitem              = this->TreeCtrl.InsertItem(KX,               TVI_ROOT, TVI_LAST);
		this->KyProperty.treeitem              = this->TreeCtrl.InsertItem(KY,               TVI_ROOT, TVI_LAST);
		this->KzProperty.treeitem              = this->TreeCtrl.InsertItem(KZ,               TVI_ROOT, TVI_LAST);
		this->PorosityProperty.treeitem        = this->TreeCtrl.InsertItem(POROSITY,         TVI_ROOT, TVI_LAST);
		this->StorageProperty.treeitem         = this->TreeCtrl.InsertItem(STORAGE,          TVI_ROOT, TVI_LAST);
		this->AlphaLongProperty.treeitem       = this->TreeCtrl.InsertItem(ALPHA_LONG,       TVI_ROOT, TVI_LAST);
		this->AlphaHorizontalProperty.treeitem = this->TreeCtrl.InsertItem(ALPHA_HORIZONTAL, TVI_ROOT, TVI_LAST);
		this->AlphaVerticalProperty.treeitem   = this->TreeCtrl.InsertItem(ALPHA_VERTICAL,   TVI_ROOT, TVI_LAST);		
		this->TortuosityProperty.treeitem      = this->TreeCtrl.InsertItem(TORTUOSITY,       TVI_ROOT, TVI_LAST);

		// setup tree selection
		this->ItemDDX = this->ActiveProperty.treeitem;
		this->TreeCtrl.SelectItem(this->ItemDDX);
		this->TreeCtrl.ModifyStyle(TVS_TRACKSELECT, TVS_FULLROWSELECT|TVS_SHOWSELALWAYS, 0);

		// set points grid for each propety
		this->ActiveProperty.SetPointsGrid(&this->PointsGrid);
		this->KxProperty.SetPointsGrid(&this->PointsGrid);
		this->KyProperty.SetPointsGrid(&this->PointsGrid);
		this->KzProperty.SetPointsGrid(&this->PointsGrid);
		this->PorosityProperty.SetPointsGrid(&this->PointsGrid);
		this->StorageProperty.SetPointsGrid(&this->PointsGrid);
		this->AlphaLongProperty.SetPointsGrid(&this->PointsGrid);
		this->AlphaHorizontalProperty.SetPointsGrid(&this->PointsGrid);
		this->AlphaVerticalProperty.SetPointsGrid(&this->PointsGrid);
		this->TortuosityProperty.SetPointsGrid(&this->PointsGrid);
	}

	// description
	::DDX_Text(pDX, IDC_DESC_EDIT, this->Description);

	// properties
	if (pDX->m_bSaveAndValidate)
	{
		this->DDV_SoftValidate();
	}
	this->DDX_Single(pDX);

	// shell
	if (pDX->m_bSaveAndValidate)
	{
		if (this->IsDlgButtonChecked(IDC_CHECK_SHELL))
		{
			this->GridElt.shell = true;
			
			CString str;

			// x width
			::DDX_Text(pDX, IDC_EDIT_XW, str);
			if (str.IsEmpty())
			{
				 this->GridElt.shell_width[0] = 0;
			}
			else
			{
				::DDX_Text(pDX, IDC_EDIT_XW, this->GridElt.shell_width[0]);
			}

			// y width
			::DDX_Text(pDX, IDC_EDIT_YW, str);
			if (str.IsEmpty())
			{
				 this->GridElt.shell_width[1] = 0;
			}
			else
			{
				::DDX_Text(pDX, IDC_EDIT_YW, this->GridElt.shell_width[1]);
			}

			// z width
			::DDX_Text(pDX, IDC_EDIT_ZW, str);
			if (str.IsEmpty())
			{
				 this->GridElt.shell_width[2] = 0;
			}
			else
			{
				::DDX_Text(pDX, IDC_EDIT_ZW, this->GridElt.shell_width[2]);
			}
		}
		else
		{
			this->GridElt.shell = false;
		}
	}
	else
	{
		if (this->GridElt.shell)
		{
			this->CheckDlgButton(IDC_CHECK_SHELL, BST_CHECKED);
		}
		else
		{
			this->CheckDlgButton(IDC_CHECK_SHELL, BST_UNCHECKED);
		}
		::DDX_Text(pDX, IDC_EDIT_XW, this->GridElt.shell_width[0]);
		::DDX_Text(pDX, IDC_EDIT_YW, this->GridElt.shell_width[1]);
		::DDX_Text(pDX, IDC_EDIT_ZW, this->GridElt.shell_width[2]);

		// shell controls
		this->OnBnClickedCheckShell();
	}

	TRACE("Out %s\n", __FUNCTION__);
}

void CMediaPropsPage2::DDV_SoftValidate()
{
	// description
	CDataExchange dx(this, TRUE);
	::DDX_Text(&dx, IDC_DESC_EDIT, this->Description);

	if (this->ItemDDX)
	{
		if (this->ItemDDX == this->ActiveProperty.treeitem)
		{
			this->ActiveProperty.DDV_SoftValidate();
		}
		else if (this->ItemDDX == this->KxProperty.treeitem)
		{
			this->KxProperty.DDV_SoftValidate();
		}
		else if (this->ItemDDX == this->KyProperty.treeitem)
		{
			this->KyProperty.DDV_SoftValidate();
		}
		else if (this->ItemDDX == this->KzProperty.treeitem)
		{
			this->KzProperty.DDV_SoftValidate();
		}
		else if (this->ItemDDX == this->PorosityProperty.treeitem)
		{
			this->PorosityProperty.DDV_SoftValidate();
		}
		else if (this->ItemDDX == this->StorageProperty.treeitem)
		{
			this->StorageProperty.DDV_SoftValidate();
		}
		else if (this->ItemDDX == this->AlphaLongProperty.treeitem)
		{
			this->AlphaLongProperty.DDV_SoftValidate();
		}
		else if (this->ItemDDX == this->AlphaHorizontalProperty.treeitem)
		{
			this->AlphaHorizontalProperty.DDV_SoftValidate();
		}
		else if (this->ItemDDX == this->AlphaVerticalProperty.treeitem)
		{
			this->AlphaVerticalProperty.DDV_SoftValidate();
		}
		else if (this->ItemDDX == this->TortuosityProperty.treeitem)
		{
			this->TortuosityProperty.DDV_SoftValidate();
		}
	}
}

void CMediaPropsPage2::DDX_Single(CDataExchange* pDX)
{
	if (this->ItemDDX)
	{
		if (!pDX->m_bSaveAndValidate)
		{
			// display single
			CGridTimeSeries::ShowSingleProperty(this, SW_SHOW);
		}

		// DDX_Single selected property
		if (this->ItemDDX == this->ActiveProperty.treeitem)
		{
			this->ActiveProperty.DDX_Single(pDX, this->Default);
		}
		else if (this->ItemDDX == this->KxProperty.treeitem)
		{
			this->KxProperty.DDX_Single(pDX, false);
		}
		else if (this->ItemDDX == this->KyProperty.treeitem)
		{
			this->KyProperty.DDX_Single(pDX, false);
		}
		else if (this->ItemDDX == this->KzProperty.treeitem)
		{
			this->KzProperty.DDX_Single(pDX, false);
		}
		else if (this->ItemDDX == this->PorosityProperty.treeitem)
		{
			this->PorosityProperty.DDX_Single(pDX, false);
		}
		else if (this->ItemDDX == this->StorageProperty.treeitem)
		{
			this->StorageProperty.DDX_Single(pDX, false);
		}
		else if (this->ItemDDX == this->AlphaLongProperty.treeitem)
		{
			this->AlphaLongProperty.DDX_Single(pDX, false);
		}
		else if (this->ItemDDX == this->AlphaHorizontalProperty.treeitem)
		{
			this->AlphaHorizontalProperty.DDX_Single(pDX, false);
		}
		else if (this->ItemDDX == this->AlphaVerticalProperty.treeitem)
		{
			this->AlphaVerticalProperty.DDX_Single(pDX, false);
		}
		else if (this->ItemDDX == this->TortuosityProperty.treeitem)
		{
			this->TortuosityProperty.DDX_Single(pDX, false);
		}
	}
}

void CMediaPropsPage2::SetProperties(const CGridElt& rGridElt)
{
	TRACE("In %s\n", __FUNCTION__);

	// copy grid_elt
	this->GridElt = rGridElt;

	// single properties
	this->ActiveProperty.SetProperty(rGridElt.active);
	this->KxProperty.SetProperty(rGridElt.kx);
	this->KyProperty.SetProperty(rGridElt.ky);
	this->KzProperty.SetProperty(rGridElt.kz);
	this->PorosityProperty.SetProperty(rGridElt.porosity);
	this->StorageProperty.SetProperty(rGridElt.storage);
	this->AlphaLongProperty.SetProperty(rGridElt.alpha_long);
	this->AlphaHorizontalProperty.SetProperty(rGridElt.alpha_horizontal);
	this->AlphaVerticalProperty.SetProperty(rGridElt.alpha_vertical);
	this->TortuosityProperty.SetProperty(rGridElt.tortuosity);

	TRACE("Out %s\n", __FUNCTION__);
}

void CMediaPropsPage2::GetProperties(CGridElt& rGridElt)const
{
	TRACE("In %s\n", __FUNCTION__);

	rGridElt = this->GridElt;

	// single properties
	this->ActiveProperty.GetProperty(rGridElt.active);
	this->KxProperty.GetProperty(rGridElt.kx);
	this->KyProperty.GetProperty(rGridElt.ky);
	this->KzProperty.GetProperty(rGridElt.kz);
	this->PorosityProperty.GetProperty(rGridElt.porosity);
	this->StorageProperty.GetProperty(rGridElt.storage);
	this->AlphaLongProperty.GetProperty(rGridElt.alpha_long);
	this->AlphaHorizontalProperty.GetProperty(rGridElt.alpha_horizontal);
	this->AlphaVerticalProperty.GetProperty(rGridElt.alpha_vertical);
	this->TortuosityProperty.GetProperty(rGridElt.tortuosity);

	TRACE("Out %s\n", __FUNCTION__);
}

BEGIN_MESSAGE_MAP(CMediaPropsPage2, CPropsPropertyPage)
	// IDC_DESC_EDIT
	ON_EN_SETFOCUS(IDC_DESC_EDIT, OnEnSetfocusDescEdit)

	// IDC_BUTTON_XYZ
	ON_BN_CLICKED(IDC_BUTTON_XYZ, OnBnClickedButtonXYZ)

	// IDC_COMBO_PROPTYPE
	ON_CBN_SELCHANGE(IDC_COMBO_PROPTYPE, OnCbnSelchangeComboProptype)

	// IDC_CHECK_MIXTURE
	ON_BN_CLICKED(IDC_CHECK_MIXTURE, OnBnClickedCheckMixture)

	// IDC_CHECK_SHELL
	ON_BN_CLICKED(IDC_CHECK_SHELL, OnBnClickedCheckShell)

	// DDX failure
	ON_MESSAGE(UM_DDX_FAILURE, OnUM_DDXFailure)

	// shell
	ON_EN_SETFOCUS(IDC_EDIT_XW, &CMediaPropsPage2::OnEnSetfocusEditXw)
	ON_EN_SETFOCUS(IDC_EDIT_YW, &CMediaPropsPage2::OnEnSetfocusEditYw)
	ON_EN_SETFOCUS(IDC_EDIT_ZW, &CMediaPropsPage2::OnEnSetfocusEditZw)
	ON_BN_SETFOCUS(IDC_CHECK_SHELL, &CMediaPropsPage2::OnBnSetfocusCheckShell)
END_MESSAGE_MAP()


// CMediaPropsPage2 message handlers

#define COMPARE_SET(S, R) \
do { \
	if (strItem.Compare(S) == 0) { \
		this->RichEditCtrl.SetWindowText(R.c_str()); \
	} \
} while (0)

void CMediaPropsPage2::SetPropertyDescription()
{
	CString strItem = this->TreeCtrl.GetItemText(this->ItemDDX);

	COMPARE_SET(ACTIVE,           this->m_sActiveRTF);
	COMPARE_SET(KX,               this->m_sKxRTF);
	COMPARE_SET(KY,               this->m_sKyRTF);
	COMPARE_SET(KZ,               this->m_sKzRTF);
	COMPARE_SET(POROSITY,         this->m_sPorosityRTF);
	COMPARE_SET(STORAGE,          this->m_sStorageRTF);
	COMPARE_SET(ALPHA_LONG,       this->m_sAlphaLongRTF);
	COMPARE_SET(ALPHA_HORIZONTAL, this->m_sAlphaHorizontalRTF);
	COMPARE_SET(ALPHA_VERTICAL,   this->m_sAlphaVerticalRTF);
	COMPARE_SET(TORTUOSITY,       this->m_sTortuosityRTF);
}

void CMediaPropsPage2::OnEnSetfocusDescEdit()
{
	TRACE("In %s\n", __FUNCTION__);
	this->RichEditCtrl.SetWindowText(this->m_sDescriptionRTF.c_str());
	TRACE("Out %s\n", __FUNCTION__);
}

LRESULT CMediaPropsPage2::OnUM_DDXFailure(WPARAM wParam, LPARAM lParam)
{
	CWnd* pFocus = (CWnd*)wParam;
	if (pFocus && pFocus->GetSafeHwnd())
	{
		ASSERT_KINDOF(CWnd, pFocus);
		pFocus->SetFocus();
	}
	return 0;
}

void CMediaPropsPage2::OnBnClickedButtonXYZ()
{
	if (this->ItemDDX)
	{
		if (this->ItemDDX == this->ActiveProperty.treeitem)
		{
			this->ActiveProperty.OnBnClickedButtonXYZ();
		}
		else if (this->ItemDDX == this->KxProperty.treeitem)
		{
			this->KxProperty.OnBnClickedButtonXYZ();
		}
		else if (this->ItemDDX == this->KyProperty.treeitem)
		{
			this->KyProperty.OnBnClickedButtonXYZ();
		}
		else if (this->ItemDDX == this->KzProperty.treeitem)
		{
			this->KzProperty.OnBnClickedButtonXYZ();
		}
		else if (this->ItemDDX == this->PorosityProperty.treeitem)
		{
			this->PorosityProperty.OnBnClickedButtonXYZ();
		}
		else if (this->ItemDDX == this->StorageProperty.treeitem)
		{
			this->StorageProperty.OnBnClickedButtonXYZ();
		}
		else if (this->ItemDDX == this->AlphaLongProperty.treeitem)
		{
			this->AlphaLongProperty.OnBnClickedButtonXYZ();
		}
		else if (this->ItemDDX == this->AlphaHorizontalProperty.treeitem)
		{
			this->AlphaHorizontalProperty.OnBnClickedButtonXYZ();
		}
		else if (this->ItemDDX == this->AlphaVerticalProperty.treeitem)
		{
			this->AlphaVerticalProperty.OnBnClickedButtonXYZ();
		}
		else if (this->ItemDDX == this->TortuosityProperty.treeitem)
		{
			this->TortuosityProperty.OnBnClickedButtonXYZ();
		}
		else
		{
			ASSERT(FALSE);
		}
	}
}

void CMediaPropsPage2::SetUnits(const CUnits &u)
{
	this->ActiveProperty.SetUnits(u);
	this->KxProperty.SetUnits(u);
	this->KyProperty.SetUnits(u);
	this->KzProperty.SetUnits(u);
	this->PorosityProperty.SetUnits(u);
	this->StorageProperty.SetUnits(u);
	this->AlphaLongProperty.SetUnits(u);
	this->AlphaHorizontalProperty.SetUnits(u);
	this->AlphaVerticalProperty.SetUnits(u);
	this->TortuosityProperty.SetUnits(u);
}

void CMediaPropsPage2::OnCbnSelchangeComboProptype()
{
	if (this->ItemDDX)
	{
		if (this->ItemDDX == this->ActiveProperty.treeitem)
		{
			this->ActiveProperty.OnCbnSelchangeComboProptype();
		}
		else if (this->ItemDDX == this->KxProperty.treeitem)
		{
			this->KxProperty.OnCbnSelchangeComboProptype();
		}
		else if (this->ItemDDX == this->KyProperty.treeitem)
		{
			this->KyProperty.OnCbnSelchangeComboProptype();
		}
		else if (this->ItemDDX == this->KzProperty.treeitem)
		{
			this->KzProperty.OnCbnSelchangeComboProptype();
		}
		else if (this->ItemDDX == this->PorosityProperty.treeitem)
		{
			this->PorosityProperty.OnCbnSelchangeComboProptype();
		}
		else if (this->ItemDDX == this->StorageProperty.treeitem)
		{
			this->StorageProperty.OnCbnSelchangeComboProptype();
		}
		else if (this->ItemDDX == this->AlphaLongProperty.treeitem)
		{
			this->AlphaLongProperty.OnCbnSelchangeComboProptype();
		}
		else if (this->ItemDDX == this->AlphaHorizontalProperty.treeitem)
		{
			this->AlphaHorizontalProperty.OnCbnSelchangeComboProptype();
		}
		else if (this->ItemDDX == this->AlphaVerticalProperty.treeitem)
		{
			this->AlphaVerticalProperty.OnCbnSelchangeComboProptype();
		}
		else if (this->ItemDDX == this->TortuosityProperty.treeitem)
		{
			this->TortuosityProperty.OnCbnSelchangeComboProptype();
		}
		else
		{
			ASSERT(FALSE);
		}
	}
}

void CMediaPropsPage2::OnBnClickedCheckMixture()
{
	if (this->ItemDDX)
	{
		if (this->ItemDDX == this->ActiveProperty.treeitem)
		{
			this->ActiveProperty.OnBnClickedCheckMixture();
		}
		else if (this->ItemDDX == this->KxProperty.treeitem)
		{
			this->KxProperty.OnBnClickedCheckMixture();
		}
		else if (this->ItemDDX == this->KyProperty.treeitem)
		{
			this->KyProperty.OnBnClickedCheckMixture();
		}
		else if (this->ItemDDX == this->KzProperty.treeitem)
		{
			this->KzProperty.OnBnClickedCheckMixture();
		}
		else if (this->ItemDDX == this->PorosityProperty.treeitem)
		{
			this->PorosityProperty.OnBnClickedCheckMixture();
		}
		else if (this->ItemDDX == this->StorageProperty.treeitem)
		{
			this->StorageProperty.OnBnClickedCheckMixture();
		}
		else if (this->ItemDDX == this->AlphaLongProperty.treeitem)
		{
			this->AlphaLongProperty.OnBnClickedCheckMixture();
		}
		else if (this->ItemDDX == this->AlphaHorizontalProperty.treeitem)
		{
			this->AlphaHorizontalProperty.OnBnClickedCheckMixture();
		}
		else if (this->ItemDDX == this->AlphaVerticalProperty.treeitem)
		{
			this->AlphaVerticalProperty.OnBnClickedCheckMixture();
		}
		else if (this->ItemDDX == this->TortuosityProperty.treeitem)
		{
			this->TortuosityProperty.OnBnClickedCheckMixture();
		}
		else
		{
			ASSERT(FALSE);
		}
	}
}

void CMediaPropsPage2::OnBnClickedCheckShell()
{
	static UINT IDs[] =
	{
		IDC_STATIC_XW,
		IDC_EDIT_XW,
		IDC_STATIC_YW,
		IDC_EDIT_YW,
		IDC_STATIC_ZW,
		IDC_EDIT_ZW,
	};

	size_t n = sizeof(IDs) / sizeof(IDs[0]);

	CWnd *pWnd;
	BOOL bEnable = this->IsDlgButtonChecked(IDC_CHECK_SHELL);
	for (size_t i = 0; i < n; ++i)
	{
		pWnd = this->GetDlgItem(IDs[i]);
		ASSERT(pWnd && pWnd->GetSafeHwnd());
		if (pWnd)
		{
			pWnd->EnableWindow(bEnable);
		}
	}
	this->RichEditCtrl.SetWindowText(this->m_sShellRTF.c_str());
}
void CMediaPropsPage2::OnEnSetfocusEditXw()
{
	this->RichEditCtrl.SetWindowText(this->m_sShellRTF.c_str());
}

void CMediaPropsPage2::OnEnSetfocusEditYw()
{
	this->RichEditCtrl.SetWindowText(this->m_sShellRTF.c_str());
}

void CMediaPropsPage2::OnEnSetfocusEditZw()
{
	this->RichEditCtrl.SetWindowText(this->m_sShellRTF.c_str());
}

void CMediaPropsPage2::OnBnSetfocusCheckShell()
{
	this->RichEditCtrl.SetWindowText(this->m_sShellRTF.c_str());
}
