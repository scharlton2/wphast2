#pragma once
#include "ICZoneActor.h"

class CICChemZoneActor : public CICZoneActor
{
public:
	vtkTypeRevisionMacro(CICChemZoneActor,CICZoneActor);
	static CICChemZoneActor *New();
	static void Create(CWPhastDoc* pWPhastDoc, const CZone& zone, const CChemIC& chemIC, LPCTSTR desc);

	CChemIC GetData(void)const;
	void SetData(const CChemIC& rChemIC);

	virtual void Insert(CPropertyTreeControlBar* pTreeControlBar, HTREEITEM hInsertAfter = TVI_LAST);
	virtual void Update(CTreeCtrl* pTreeCtrl, HTREEITEM htiParent);
	virtual void Edit(CTreeCtrl* pTreeCtrl);
	virtual void Add(CWPhastDoc *pWPhastDoc);
	virtual void Remove(CWPhastDoc *pWPhastDoc);

	static const char szHeading[];

	void Serialize(bool bStoring, hid_t loc_id, const CUnits& units);

protected:
	CICChemZoneActor(void);
	virtual ~CICChemZoneActor(void);
	CChemIC m_chemIC;

private:
	CICChemZoneActor(const CICChemZoneActor&);  // Not implemented.
	void operator=(const CICChemZoneActor&);  // Not implemented.
};
