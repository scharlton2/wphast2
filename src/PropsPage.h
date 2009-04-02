#pragma once

#include <vector>

#include "TreePropSheetEx/ResizablePage.h"
#include "TreePropSheetEx/TreePropSheetUtil.hpp"
#include "TreePropSheetEx/TreePropSheetEx.h"
#include "PropNone.h"
#include "PropConstant.h"
#include "TreePropSheetEx/TreePropSheetSplitter.h"
#include "TreePropSheetExSRC.h"

#include "PropStruct.h"
#include "GridElt.h"
#include "afxcmn.h"


// CPropsPage dialog

class CPropsPage // : public CPropertyPage
: public CResizablePage
, public TreePropSheet::CWhiteBackgroundProvider
{
	DECLARE_DYNAMIC(CPropsPage)

public:
	CPropsPage();
	virtual ~CPropsPage();

	void CommonConstruct(void);

	void SetProperties(const CGridElt& rGridElt);
	void GetProperties(CGridElt& rGridElt)const;

	void SetFlowOnly(bool flowOnly)      { this->bFlowOnly = flowOnly; }
	bool GetFlowOnly(void)const          { return this->bFlowOnly; }

	void SetDefault(bool bDefault)       { this->bDefault = bDefault; }
	bool GetDefault(void)const           { return bDefault; }

	void SetDesc(LPCTSTR desc)           { this->sDesc = desc; }
	LPCTSTR GetDesc()                    { return sDesc.c_str(); }


// Dialog Data
	enum { IDD = IDD_PROPS_PAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

 	afx_msg void OnPageTreeSelChanging(NMHDR *pNotifyStruct, LRESULT *plResult);
 	afx_msg void OnPageTreeSelChanged(NMHDR *pNotifyStruct, LRESULT *plResult);
	afx_msg LRESULT OnUM_DDXFailure(WPARAM wParam, LPARAM lParam);



protected:
	bool RegisterSheet(UINT nID, CPropertySheet& rSheet);
// COMMENT: {4/1/2009 3:46:35 PM}	TreePropSheet::CTreePropSheetEx          SheetTop;
	CTreePropSheetExSRC                      SheetTop;
	TreePropSheet::CTreePropSheetSplitter    Splitter;
	CPropNone                                PropNone;
	CPropConstant                            PropConstant;

	CGridElt GridElt;
	CGridElt GridEltFixed;
	CGridElt GridEltLinear;
	CRichEditCtrl RichEditCtrl;

	HTREEITEM ItemDDX;

	bool bFlowOnly;
	bool bDefault;

	std::string m_sDescriptionRTF;       // IDR_DESCRIPTION_RTF
	std::string m_sActiveRTF;            // IDR_MEDIA_ACTIVE_RTF
	std::string m_sKxRTF;                // IDR_MEDIA_KX_RTF
	std::string m_sKyRTF;                // IDR_MEDIA_KY_RTF
	std::string m_sKzRTF;                // IDR_MEDIA_KZ_RTF
	std::string m_sAlphaLongRTF;         // IDR_MEDIA_LONG_DISP_RTF
	std::string m_sPorosityRTF;          // IDR_MEDIA_POROSITY_RTF
	std::string m_sStorageRTF;           // IDR_MEDIA_SPEC_STORAGE_RTF
	std::string m_sAlphaHorizontalRTF;   // IDR_MEDIA_ALPHA_HORZ_RTF
	std::string m_sAlphaVerticalRTF;     // IDR_MEDIA_ALPHA_VERT_RTF

	std::string sDesc;

public:
	virtual BOOL OnInitDialog();

};
