#include "StdAfx.h"
#include "SetScaleAction.h"

#include <map>
#include "WPhastDoc.h"
#include "WPhastView.h"


CSetScaleAction::CSetScaleAction(CWPhastDoc* pDoc, double xScale, double yScale, double zScale)
: m_pDoc(pDoc)
{
	this->m_Scale[0] = xScale;
	this->m_Scale[1] = yScale;
	this->m_Scale[2] = zScale;
	this->m_pDoc->GetScale(this->m_OriginalScale);
}

CSetScaleAction::~CSetScaleAction(void)
{
}

void CSetScaleAction::Execute()
{
	ASSERT_VALID(this->m_pDoc);
	this->m_pDoc->SetScale(this->m_Scale[0], this->m_Scale[1], this->m_Scale[2]);
}

void CSetScaleAction::UnExecute()
{
	ASSERT_VALID(this->m_pDoc);
	this->m_pDoc->SetScale(this->m_OriginalScale[0], this->m_OriginalScale[1], this->m_OriginalScale[2]);
}

void CSetScaleAction::Apply(double xScale, double yScale, double zScale)
{
	this->m_Scale[0] = xScale;
	this->m_Scale[1] = yScale;
	this->m_Scale[2] = zScale;
	this->Execute();
}
