#pragma once
#include <map>
#include <vtkLODActor.h>
// #include "structs.h"
///#include "Grid.h"
#include "GridKeyword.h"
#include "Units.h"
#include "Snap.h"
#include "tree.h"           // CTreeCtrlNode

class vtkGeometryFilter;
class vtkFeatureEdges;
class vtkPolyDataMapper;
class vtkImplicitPlaneWidget;
class CGridLineWidget;

class CZone;

class CGridLODActor : public vtkLODActor
{
public:
	vtkTypeRevisionMacro(CGridLODActor,vtkLODActor);

	static CGridLODActor *New();

	enum EventIds {
		DeleteGridLineEvent = vtkCommand::UserEvent + 600,
		InsertGridLineEvent,
		MoveGridLineEvent,
	};

	void SetGrid(const CGrid& x, const CGrid& y, const CGrid& z, const CUnits& units);
	void GetGrid(CGrid& x, CGrid& y, CGrid& z)const;

	//void SetFlowOnly(BOOL bFlowOnly);
	//BOOL GetFlowOnly(void);
	void GetDefaultZone(CZone& rZone);

	void GetSnap(CSnap &snap)const { snap = this->m_gridKeyword.m_snap; };
	void SetSnap(const CSnap &snap) { this->m_gridKeyword.m_snap = snap; };

	void GetAxes(int axes[3])const
	{
		axes[0] = this->m_gridKeyword.m_axes[0];
		axes[1] = this->m_gridKeyword.m_axes[1];
		axes[2] = this->m_gridKeyword.m_axes[2];
	};
	void SetAxes(const int axes[3])
	{
		this->m_gridKeyword.m_axes[0] = axes[0];
		this->m_gridKeyword.m_axes[1] = axes[1];
		this->m_gridKeyword.m_axes[2] = axes[2];
	};

	int GetAxisIndex(void)           { return AxisIndex; }
	int GetPlaneIndex(void)          { return PlaneIndex; }
	double GetLastDeletedValue(void) { return m_dDeletedValue; }

	void GetGridKeyword(CGridKeyword& gridKeyword)const;
	void SetGridKeyword(const CGridKeyword& gridKeyword, const CUnits& units);

	BOOL DeleteLine(int nAxisIndex, int nPlaneIndex);
	int InsertLine(int nAxisIndex, double dValue);

	void Serialize(bool bStoring, hid_t loc_id);

	virtual void SetInteractor(vtkRenderWindowInteractor* iren);
	vtkGetObjectMacro(Interactor, vtkRenderWindowInteractor);

	void SetEnabled(int enabling);

	// Description:
	// Main process event method
	static void ProcessEvents(vtkObject* object, unsigned long event, void* clientdata, void* calldata);

	// ProcessEvents() dispatches to these methods.
	void OnMouseMove(void);
	void OnLeftButtonDown(void);
	void OnLeftButtonUp(void);
	void OnKeyPress(void);
	void OnInteraction(void);
	void OnEndInteraction(void);


#if defined(WIN32)
	BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
#endif


#ifdef _DEBUG
	void Dump(CDumpContext& dc)const;
#endif
	void Insert(CTreeCtrl* pTreeCtrl, HTREEITEM htiGrid);
	void Insert(CTreeCtrlNode node);

	void UpdateNode(void);

	////void SetDimensions(int dims[3])    { m_nDimensions[0] = dims[0]; m_nDimensions[1] = dims[1]; m_nDimensions[2] = dims[2]; }
	////void SetDimensions(int nx, int ny, int nz)    { m_nDimensions[0] = nx; m_nDimensions[1] = ny; m_nDimensions[2] = nz; }
	////void SetCoordinates(float *xCoords, float *yCoords, float *zCoords);


protected:
	CGridLODActor(void);
	virtual ~CGridLODActor(void);

	int State;
	enum WidgetState
	{
		Start=0,
		Pushing,
	};


	void Setup(const CUnits& units);

	vtkGeometryFilter*  m_pGeometryFilter;
	vtkFeatureEdges*    m_pFeatureEdges;
	vtkPolyDataMapper*  m_pPolyDataMapper;

	// Used to associate observers with the interactor
	vtkRenderWindowInteractor *Interactor;

	// Used to process events
	vtkCallbackCommand* EventCallbackCommand;

	// vtkImplicitPlaneWidget    *PlaneWidget;	
	CGridLineWidget           *PlaneWidget;

	int                 Enabled;

	// Internal ivars for processing events
	vtkRenderer        *CurrentRenderer;

	int                    AxisIndex;
	int                    PlaneIndex;
	double                 CurrentPoint[3];
	std::map<double, int>  ValueToIndex[3];


	///CGrid               m_grid[3];
	///CSnap               m_snap;
	///int                 m_axes[3];
	///bool                m_print_input_xy;
	CGridKeyword        m_gridKeyword;
	CUnits              m_units;
	// HTREEITEM           m_htiGrid;
	CTreeCtrlNode       m_node;

	float               m_min[3];
	float               m_max[3];

	double              m_dDeletedValue;

private:
  CGridLODActor(const CGridLODActor&);  // Not implemented.
  void operator=(const CGridLODActor&);  // Not implemented.

};
