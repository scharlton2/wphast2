#include "StdAfx.h"
#include "GridActor.h"

#include <vtkGeometryFilter.h>
#include <vtkFeatureEdges.h>
#include <vtkPolyDataMapper.h>
#include <vtkObjectFactory.h>

#include <vtkPointPicker.h>

#include <vtkFloatArray.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkStructuredGrid.h>
#include <vtkStructuredGridGeometryFilter.h>
#include <vtkThreshold.h>
#include <vtkPolyData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkImplicitPlaneWidget.h>
#include "GridLineWidget.h"
#include "GridLineMoveMemento.h"

#include "Global.h"
#include "Units.h"
#include "Zone.h"
#include "DelayRedraw.h"

#if defined(WIN32)
#include "Resource.h"
#endif

//{{
static HCURSOR s_hcur = 0;
//}}


vtkCxxRevisionMacro(CGridActor, "$Revision: 418 $");
vtkStandardNewMacro(CGridActor);

// Note: No header files should follow the next three lines
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CGridActor::CGridActor(void)
	: m_pGeometryFilter(0)
	, m_pFeatureEdges(0)
	, m_pPolyDataMapper(0)
	, Actor(0)
	, Interactor(0)
	, CurrentRenderer(0)
	, EventCallbackCommand(0)
	, PlanePicker(0)
	, Enabled(0)
	, PlaneWidget(0)
	, State(CGridActor::Start)
	, AxisIndex(-1)
	, ScaleTransform(0)
	, UnitsTransform(0)
	, PosXLineSources(0)
	, PosXLineFilters(0)
	, PosXLineMappers(0)
	, PosXLineActors(0)
	, LinePicker(0)
{
	this->m_pGeometryFilter = vtkGeometryFilter::New();

	this->m_pFeatureEdges = vtkFeatureEdges::New();
	this->m_pFeatureEdges->BoundaryEdgesOn();
	this->m_pFeatureEdges->ManifoldEdgesOn();
	this->m_pFeatureEdges->FeatureEdgesOn();
	this->m_pFeatureEdges->NonManifoldEdgesOn();
	this->m_pFeatureEdges->ColoringOff();

	this->m_pPolyDataMapper = vtkPolyDataMapper::New();
	this->m_pPolyDataMapper->SetInput(this->m_pFeatureEdges->GetOutput());
	this->m_pPolyDataMapper->SetResolveCoincidentTopologyToPolygonOffset();

	this->Actor = vtkLODActor::New();
	this->Actor->SetMapper(this->m_pPolyDataMapper);
	this->Actor->GetProperty()->SetColor(1.0, 0.8, 0.6);
	this->AddPart(this->Actor);

	this->m_gridKeyword.m_grid[0].c = 'X';
	this->m_gridKeyword.m_grid[1].c = 'Y';
	this->m_gridKeyword.m_grid[2].c = 'Z';

	this->EventCallbackCommand = vtkCallbackCommand::New();
	this->EventCallbackCommand->SetClientData(this); 
	this->EventCallbackCommand->SetCallback(CGridActor::ProcessEvents);


	this->PlanePicker = vtkCellPicker::New();
	this->PlanePicker->PickFromListOn();

	for (int i = 0; i < 6; ++i)
	{
		this->PlaneSources[i] = vtkPlaneSource::New();
		this->PlaneMappers[i] = vtkPolyDataMapper::New();
		this->PlaneMappers[i]->SetInput(this->PlaneSources[i]->GetOutput());
		this->PlaneActors[i]  = vtkLODActor::New();
		this->PlaneActors[i]->SetMapper(this->PlaneMappers[i]);
		this->AddPart(this->PlaneActors[i]);
		this->PlanePicker->AddPickList(this->PlaneActors[i]);
	}

	this->ScaleTransform = vtkTransform::New();
	this->UnitsTransform = vtkTransform::New();

	for (int i = 0; i < 3; ++i)
	{
		this->m_min[i] = 0.0;
		this->m_max[i] = 1.0;
	}
	this->LinePicker = vtkCellPicker::New();
	this->LinePicker->SetTolerance(0.002);
}

CGridActor::~CGridActor(void)
{
	if ( this->PosXLineCount > 0 )
	{
		for (int i = 0; i < this->PosXLineCount; ++i)
		{
			this->RemovePart(this->PosXLineActors[i]);
			this->PosXLineSources[i]->Delete();
			this->PosXLineFilters[i]->Delete();
			this->PosXLineMappers[i]->Delete();
			this->PosXLineActors[i]->Delete();
		}
		delete [] this->PosXLineSources; this->PosXLineSources = NULL;
		delete [] this->PosXLineFilters; this->PosXLineFilters = NULL;
		delete [] this->PosXLineMappers; this->PosXLineMappers = NULL;
		delete [] this->PosXLineActors; this->PosXLineActors = NULL;
		this->PosXLineCount = 0;
	}
	if (this->LinePicker)
	{
		this->LinePicker->Delete();
	}
	if (this->GetEnabled())
	{
		this->SetEnabled(0);
	}
	for (int i = 0; i < 6; ++i)
	{
		this->PlaneSources[i]->Delete();
		this->PlaneMappers[i]->Delete();
		this->PlaneActors[i]->Delete();
	}
	if (this->Actor)
	{
		this->Actor->Delete();
	}
	this->m_pGeometryFilter->Delete();
	this->m_pFeatureEdges->Delete();
	this->m_pPolyDataMapper->Delete();
	if (this->PlaneWidget)
	{
		if (this->PlaneWidget->GetEnabled())
		{
			this->PlaneWidget->SetEnabled(0);
		}
		this->PlaneWidget->Delete();
	}
	if (this->Interactor)
	{
		this->Interactor->RemoveObserver(this->EventCallbackCommand);
		this->Interactor->UnRegister(this);
	}
	this->EventCallbackCommand->Delete();
	if (this->PlanePicker)
	{
		this->PlanePicker->Delete();
	}

	this->ScaleTransform->Delete();
	this->UnitsTransform->Delete();
}

#ifdef _DEBUG
void CGridActor::Dump(CDumpContext& dc) const
{
	dc << "<CGridActor>\n";
	this->m_gridKeyword.m_grid[0].Dump(dc);
	this->m_gridKeyword.m_grid[1].Dump(dc);
	this->m_gridKeyword.m_grid[2].Dump(dc);
	dc << "</CGridActor>\n";
}
#endif

void CGridActor::Serialize(bool bStoring, hid_t loc_id)
{
	static const char szGrid[]     = "Grid";
	static const char szX[]        = "X";
	static const char szY[]        = "Y";
	static const char szZ[]        = "Z";
	static const char szScale[]    = "Scale";
	static const char szSnap[]     = "Snap";
	static const char szPrintXY[]  = "PrintXYOrient";
	static const char szChemDims[] = "ChemDims";
	
	vtkFloatingPointType scale[3];
	double double_scale[3];

	herr_t status;

	if (bStoring)
	{
		if (loc_id > 0)
		{
			hid_t grid_gr_id = H5Gcreate(loc_id, szGrid, 0);
			if (grid_gr_id > 0)
			{

				// X
				//
				hid_t x_gr_id = ::H5Gcreate(grid_gr_id, szX, 0);
				if (x_gr_id > 0)
				{
					this->m_gridKeyword.m_grid[0].Serialize(bStoring, x_gr_id);
					::H5Gclose(x_gr_id);
				}

				// Y
				//
				hid_t y_gr_id = ::H5Gcreate(grid_gr_id, szY, 0);
				if (y_gr_id > 0)
				{
					this->m_gridKeyword.m_grid[1].Serialize(bStoring, y_gr_id);
					::H5Gclose(y_gr_id);
				}

				// Z
				//
				hid_t z_gr_id = ::H5Gcreate(grid_gr_id, szZ, 0);
				if (z_gr_id > 0)
				{
					this->m_gridKeyword.m_grid[2].Serialize(bStoring, z_gr_id);
					::H5Gclose(z_gr_id);
				}

				// Scale
				//
				this->GetScale(scale);
				double_scale[0] = scale[0];    double_scale[1] = scale[1];    double_scale[2] = scale[2];
				status = CGlobal::HDFSerialize(bStoring, grid_gr_id, szScale, H5T_NATIVE_DOUBLE, 3, double_scale);
				ASSERT(status >= 0);

				// Snap
				//
				status = CGlobal::HDFSerialize(bStoring, grid_gr_id, szSnap, H5T_NATIVE_DOUBLE, 3, this->m_gridKeyword.m_snap.m_snap);
				ASSERT(status >= 0);

				// print_orientation
				//
				status = CGlobal::HDFSerializeBool(bStoring, grid_gr_id, szPrintXY, this->m_gridKeyword.m_print_input_xy);
				ASSERT(status >= 0);

				// chemistry_dimensions
				//
				ASSERT(this->m_gridKeyword.m_axes[0] == 0 || this->m_gridKeyword.m_axes[0] == 1);
				ASSERT(this->m_gridKeyword.m_axes[1] == 0 || this->m_gridKeyword.m_axes[1] == 1);
				ASSERT(this->m_gridKeyword.m_axes[2] == 0 || this->m_gridKeyword.m_axes[2] == 1);
				status = CGlobal::HDFSerialize(bStoring, grid_gr_id, szChemDims, H5T_NATIVE_INT, 3, this->m_gridKeyword.m_axes);
				ASSERT(status >= 0);

				// close grid group
				//
				status = ::H5Gclose(grid_gr_id);
				ASSERT(status >= 0);
			}
		}
	}
	else
	{
		if (loc_id > 0)
		{
			hid_t grid_gr_id = H5Gopen(loc_id, szGrid);
			if (grid_gr_id > 0)
			{
				CGrid x, y, z;

				// X
				//
				hid_t x_gr_id = H5Gopen(grid_gr_id, szX);
				if (x_gr_id > 0)
				{
					x.Serialize(bStoring, x_gr_id);
					::H5Gclose(x_gr_id);
				}

				// Y
				//
				hid_t y_gr_id = H5Gopen(grid_gr_id, szY);
				if (y_gr_id > 0)
				{
					y.Serialize(bStoring, y_gr_id);
					::H5Gclose(y_gr_id);
				}

				// Z
				//
				hid_t z_gr_id = H5Gopen(grid_gr_id, szZ);
				if (z_gr_id > 0)
				{
					z.Serialize(bStoring, z_gr_id);
					::H5Gclose(z_gr_id);
				}
				this->m_gridKeyword.m_grid[0] = x;
				this->m_gridKeyword.m_grid[1] = y;
				this->m_gridKeyword.m_grid[2] = z;
				ASSERT(this->m_gridKeyword.m_grid[0].elt_centroid == 0);
				ASSERT(this->m_gridKeyword.m_grid[1].elt_centroid == 0);
				ASSERT(this->m_gridKeyword.m_grid[2].elt_centroid == 0);

				// Scale
				//
				scale[0] = 1;
				scale[1] = 1;
				scale[2] = 1;
				status = CGlobal::HDFSerialize(bStoring, grid_gr_id, szScale, H5T_NATIVE_DOUBLE, 3, double_scale);
				ASSERT(status >= 0);
				if (status >= 0)
				{
					scale[0] = double_scale[0];    scale[1] = double_scale[1];    scale[2] = double_scale[2];
				}
				this->SetScale(scale);

				// Snap
				//
				status = CGlobal::HDFSerialize(bStoring, grid_gr_id, szSnap, H5T_NATIVE_DOUBLE, 3, this->m_gridKeyword.m_snap.m_snap);
				ASSERT(status >= 0);

				// print_orientation
				//
				status = CGlobal::HDFSerializeBool(bStoring, grid_gr_id, szPrintXY, this->m_gridKeyword.m_print_input_xy);
				ASSERT(status >= 0);

				// chemistry_dimensions
				//
				status = CGlobal::HDFSerialize(bStoring, grid_gr_id, szChemDims, H5T_NATIVE_INT, 3, this->m_gridKeyword.m_axes);
				ASSERT(status >= 0);

				ASSERT(this->m_gridKeyword.m_axes[0] == 0 || this->m_gridKeyword.m_axes[0] == 1);
				ASSERT(this->m_gridKeyword.m_axes[1] == 0 || this->m_gridKeyword.m_axes[1] == 1);
				ASSERT(this->m_gridKeyword.m_axes[2] == 0 || this->m_gridKeyword.m_axes[2] == 1);
				if (this->m_gridKeyword.m_axes[0] == 0 || this->m_gridKeyword.m_axes[0] == 1) this->m_gridKeyword.m_axes[0] = 1;
				if (this->m_gridKeyword.m_axes[1] == 0 || this->m_gridKeyword.m_axes[1] == 1) this->m_gridKeyword.m_axes[1] = 1;
				if (this->m_gridKeyword.m_axes[2] == 0 || this->m_gridKeyword.m_axes[2] == 1) this->m_gridKeyword.m_axes[2] = 1;

				// close grid group
				//
				status = ::H5Gclose(grid_gr_id);
				ASSERT(status >= 0);
			}
		}
	}
}

void CGridActor::Insert(CTreeCtrlNode node)
{
	ASSERT(node);

	// delay the refresh
	//
	CDelayRedraw redraw(node.GetWnd());

	// store expanded states
	bool bMainExpanded = false;
	if (node.HasChildren())
	{
		bMainExpanded = ((node.GetState(TVIS_EXPANDED) & TVIS_EXPANDED) != 0);
	}

	// remove all previous items
	//
	while (node.HasChildren())
	{
		node.GetChild().Delete();
	}

	// insert
	this->m_gridKeyword.m_grid[0].Insert(node);
	this->m_gridKeyword.m_grid[1].Insert(node);
	this->m_gridKeyword.m_grid[2].Insert(node);

	if (bMainExpanded)
	{
		node.Expand(TVE_EXPAND);
	}
	this->m_node = node;
}

void CGridActor::Insert(CTreeCtrl* pTreeCtrl, HTREEITEM htiGrid)
{
	while (HTREEITEM hChild = pTreeCtrl->GetChildItem(htiGrid))
	{
		pTreeCtrl->DeleteItem(hChild);
	}

	// insert
	this->m_gridKeyword.m_grid[0].Insert(pTreeCtrl, htiGrid);
	this->m_gridKeyword.m_grid[1].Insert(pTreeCtrl, htiGrid);
	this->m_gridKeyword.m_grid[2].Insert(pTreeCtrl, htiGrid);

	// snap
	CSnap defaultSnap;
	if (this->m_gridKeyword.m_snap[0] != defaultSnap[0])
	{
		CString str;
		str.Format("snap X %g", this->m_gridKeyword.m_snap[0]);
		pTreeCtrl->InsertItem(str, htiGrid);
	}
	if (this->m_gridKeyword.m_snap[1] != defaultSnap[1])
	{
		CString str;
		str.Format("snap Y %g", this->m_gridKeyword.m_snap[1]);
		pTreeCtrl->InsertItem(str, htiGrid);
	}
	if (this->m_gridKeyword.m_snap[2] != defaultSnap[2])
	{
		CString str;
		str.Format("snap Z %g", this->m_gridKeyword.m_snap[2]);
		pTreeCtrl->InsertItem(str, htiGrid);
	}

	// chem dims
	if (!this->m_gridKeyword.m_axes[0] || !this->m_gridKeyword.m_axes[1] || !this->m_gridKeyword.m_axes[2])
	{
		CString str("chemistry_dimensions ");
		if (this->m_gridKeyword.m_axes[0]) str += _T("X");
		if (this->m_gridKeyword.m_axes[1]) str += _T("Y");
		if (this->m_gridKeyword.m_axes[2]) str += _T("Z");
		pTreeCtrl->InsertItem(str, htiGrid);
	}

	// set data
	pTreeCtrl->SetItemData(htiGrid, (DWORD_PTR)this);
	this->m_node = CTreeCtrlNode(htiGrid, (CTreeCtrlEx*)pTreeCtrl);
}

void CGridActor::SetGrid(const CGrid& x, const CGrid& y, const CGrid& z, const CUnits& units)
{
	this->m_gridKeyword.m_grid[0] = x;
	this->m_gridKeyword.m_grid[1] = y;
	this->m_gridKeyword.m_grid[2] = z;

	this->Setup(units);
}

void CGridActor::GetGrid(CGrid& x, CGrid& y, CGrid& z)const
{
	x = this->m_gridKeyword.m_grid[0];
	y = this->m_gridKeyword.m_grid[1];
	z = this->m_gridKeyword.m_grid[2];
}

void CGridActor::GetGridKeyword(CGridKeyword& gridKeyword)const
{
	gridKeyword = this->m_gridKeyword;
}

void CGridActor::SetGridKeyword(const CGridKeyword& gridKeyword, const CUnits& units)
{
	this->m_gridKeyword = gridKeyword;
	this->m_units = units;
	this->Setup(units);
}

void CGridActor::Setup(const CUnits& units)
{
	ASSERT(this->m_gridKeyword.m_grid[0].uniform_expanded == FALSE);
	ASSERT(this->m_gridKeyword.m_grid[1].uniform_expanded == FALSE);
	ASSERT(this->m_gridKeyword.m_grid[2].uniform_expanded == FALSE);

	this->UnitsTransform->Identity();
	this->UnitsTransform->Scale(units.horizontal.input_to_si, units.horizontal.input_to_si, units.vertical.input_to_si);

	register int i;
	double inc[3];
	double min[3];
	for (i = 0; i < 3; ++i)
	{
		if (this->m_gridKeyword.m_grid[i].uniform)
		{
			inc[i] = (this->m_gridKeyword.m_grid[i].coord[1] - this->m_gridKeyword.m_grid[i].coord[0]) / (this->m_gridKeyword.m_grid[i].count_coord - 1);
			if (i == 2)
			{
				min[2] = this->m_gridKeyword.m_grid[2].coord[0] * units.vertical.input_to_si;
				inc[2] *= units.vertical.input_to_si;
			}
			else
			{
				min[i] = this->m_gridKeyword.m_grid[i].coord[0] * units.horizontal.input_to_si;
				inc[i] *= units.horizontal.input_to_si;
			}
		}
	}

	vtkStructuredGrid *sgrid = vtkStructuredGrid::New();
	sgrid->SetDimensions(this->m_gridKeyword.m_grid[0].count_coord, this->m_gridKeyword.m_grid[1].count_coord, this->m_gridKeyword.m_grid[2].count_coord);

	vtkPoints *points = vtkPoints::New();
	points->Allocate(this->m_gridKeyword.m_grid[0].count_coord * this->m_gridKeyword.m_grid[1].count_coord * this->m_gridKeyword.m_grid[2].count_coord);

	// load grid points
	//
	this->ValueToIndex[0].clear();
	this->ValueToIndex[1].clear();
	this->ValueToIndex[2].clear();
	vtkFloatingPointType x[3];
	vtkFloatingPointType t[3];
	register int j, k, offset, jOffset, kOffset;
	for (k = 0; k < this->m_gridKeyword.m_grid[2].count_coord; ++k)
	{
		if (this->m_gridKeyword.m_grid[2].uniform)
		{
			x[2] = min[2] + k * inc[2];
		}
		else
		{
			x[2] = this->m_gridKeyword.m_grid[2].coord[k] * units.vertical.input_to_si;
		}
		kOffset = k * this->m_gridKeyword.m_grid[0].count_coord * this->m_gridKeyword.m_grid[1].count_coord;
		for (j = 0; j < this->m_gridKeyword.m_grid[1].count_coord; ++j)
		{
			if (this->m_gridKeyword.m_grid[1].uniform)
			{
				x[1] = min[1] + j * inc[1];
			}
			else
			{
				x[1] = this->m_gridKeyword.m_grid[1].coord[j] * units.horizontal.input_to_si;
			}
			jOffset = j * this->m_gridKeyword.m_grid[0].count_coord;
			for (i = 0; i < this->m_gridKeyword.m_grid[0].count_coord; ++i)
			{
				if (this->m_gridKeyword.m_grid[0].uniform)
				{
					x[0] = min[0] + i * inc[0];
				}
				else
				{
					x[0] = this->m_gridKeyword.m_grid[0].coord[i] * units.horizontal.input_to_si;
				}
				offset = i + jOffset + kOffset;
				this->ScaleTransform->TransformPoint(x, t);
				if (i == 0 && j == 0)
				{
					VERIFY(this->ValueToIndex[2].insert(std::map<vtkFloatingPointType, int>::value_type(t[2], k)).second);
				}
				if (i == 0 && k == 0)
				{
					VERIFY(this->ValueToIndex[1].insert(std::map<vtkFloatingPointType, int>::value_type(t[1], j)).second);
				}
				if (j == 0 && k == 0)
				{
					VERIFY(this->ValueToIndex[0].insert(std::map<vtkFloatingPointType, int>::value_type(t[0], i)).second);
				}
				points->InsertPoint(offset, t);
			}
		}
	}
	ASSERT(this->ValueToIndex[0].size() == this->m_gridKeyword.m_grid[0].count_coord);
	ASSERT(this->ValueToIndex[1].size() == this->m_gridKeyword.m_grid[1].count_coord);
	ASSERT(this->ValueToIndex[2].size() == this->m_gridKeyword.m_grid[2].count_coord);

	sgrid->SetPoints(points);
	points->Delete();

	for (i = 0; i < 3; ++i)
	{
		this->m_min[i] = this->ValueToIndex[i].begin()->first;
		this->m_max[i] = this->ValueToIndex[i].rbegin()->first;
	}

	this->UpdatePoints();
	
// COMMENT: {8/12/2005 4:49:00 PM}	//{{
// COMMENT: {8/12/2005 4:49:00 PM}	if ( this->PosXLineCount > 0 )
// COMMENT: {8/12/2005 4:49:00 PM}	{
// COMMENT: {8/12/2005 4:49:00 PM}		for (int i = 0; i < this->PosXLineCount; ++i)
// COMMENT: {8/12/2005 4:49:00 PM}		{
// COMMENT: {8/12/2005 4:49:00 PM}			this->RemovePart(this->PosXLineActors[i]);
// COMMENT: {8/12/2005 4:49:00 PM}			this->PosXLineSources[i]->Delete();
// COMMENT: {8/12/2005 4:49:00 PM}			this->PosXLineFilters[i]->Delete();
// COMMENT: {8/12/2005 4:49:00 PM}			this->PosXLineMappers[i]->Delete();
// COMMENT: {8/12/2005 4:49:00 PM}			this->PosXLineActors[i]->Delete();
// COMMENT: {8/12/2005 4:49:00 PM}		}
// COMMENT: {8/12/2005 4:49:00 PM}		delete [] this->PosXLineSources; this->PosXLineSources = NULL;
// COMMENT: {8/12/2005 4:49:00 PM}		delete [] this->PosXLineFilters; this->PosXLineFilters = NULL;
// COMMENT: {8/12/2005 4:49:00 PM}		delete [] this->PosXLineMappers; this->PosXLineMappers = NULL;
// COMMENT: {8/12/2005 4:49:00 PM}		delete [] this->PosXLineActors; this->PosXLineActors = NULL;
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineCount = 0;
// COMMENT: {8/12/2005 4:49:00 PM}	}
// COMMENT: {8/12/2005 4:49:00 PM}
// COMMENT: {8/12/2005 4:49:00 PM}	int num = this->m_gridKeyword.m_grid[0].count_coord + this->m_gridKeyword.m_grid[1].count_coord;
// COMMENT: {8/12/2005 4:49:00 PM}	this->PosXLineSources = new vtkLineSource* [num];
// COMMENT: {8/12/2005 4:49:00 PM}	this->PosXLineFilters = new vtkTubeFilter* [num];
// COMMENT: {8/12/2005 4:49:00 PM}	this->PosXLineMappers = new vtkPolyDataMapper* [num];
// COMMENT: {8/12/2005 4:49:00 PM}	this->PosXLineActors = new vtkActor* [num];
// COMMENT: {8/12/2005 4:49:00 PM}
// COMMENT: {8/12/2005 4:49:00 PM}	float pt1[3];
// COMMENT: {8/12/2005 4:49:00 PM}	float pt2[3];
// COMMENT: {8/12/2005 4:49:00 PM}	for (int i = 0; i < this->m_gridKeyword.m_grid[0].count_coord; ++i)
// COMMENT: {8/12/2005 4:49:00 PM}	{
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineSources[i] = vtkLineSource::New();
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineFilters[i] = vtkTubeFilter::New();
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineFilters[i]->SetRadius(250);
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineFilters[i]->SetNumberOfSides(8);
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineFilters[i]->SetInput(this->PosXLineSources[i]->GetOutput());
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineMappers[i] = vtkPolyDataMapper::New();
// COMMENT: {8/12/2005 4:49:00 PM}		//this->PosXLineMappers[i]->SetInput(this->PosXLineSources[i]->GetOutput());
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineMappers[i]->SetInput(this->PosXLineFilters[i]->GetOutput());
// COMMENT: {8/12/2005 4:49:00 PM}
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineActors[i] = vtkLODActor::New();
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineActors[i]->SetMapper(this->PosXLineMappers[i]);
// COMMENT: {8/12/2005 4:49:00 PM}		{
// COMMENT: {8/12/2005 4:49:00 PM}			if (this->m_gridKeyword.m_grid[0].uniform)
// COMMENT: {8/12/2005 4:49:00 PM}			{
// COMMENT: {8/12/2005 4:49:00 PM}				x[0] = min[0] + i * inc[0];
// COMMENT: {8/12/2005 4:49:00 PM}			}
// COMMENT: {8/12/2005 4:49:00 PM}			else
// COMMENT: {8/12/2005 4:49:00 PM}			{
// COMMENT: {8/12/2005 4:49:00 PM}				x[0] = this->m_gridKeyword.m_grid[0].coord[i] * units.horizontal.input_to_si;
// COMMENT: {8/12/2005 4:49:00 PM}			}
// COMMENT: {8/12/2005 4:49:00 PM}			//{{
// COMMENT: {8/12/2005 4:49:00 PM}			pt1[1] = this->m_min[1];
// COMMENT: {8/12/2005 4:49:00 PM}			pt1[2] = this->m_max[2];
// COMMENT: {8/12/2005 4:49:00 PM}			pt2[1] = this->m_max[1];
// COMMENT: {8/12/2005 4:49:00 PM}			pt2[2] = this->m_max[2];
// COMMENT: {8/12/2005 4:49:00 PM}			//}}
// COMMENT: {8/12/2005 4:49:00 PM}			pt1[0] = pt2[0] = x[0];
// COMMENT: {8/12/2005 4:49:00 PM}			this->ScaleTransform->TransformPoint(pt1, pt1);
// COMMENT: {8/12/2005 4:49:00 PM}			this->ScaleTransform->TransformPoint(pt2, pt2);
// COMMENT: {8/12/2005 4:49:00 PM}			this->PosXLineSources[i]->SetPoint1(pt1);
// COMMENT: {8/12/2005 4:49:00 PM}			this->PosXLineSources[i]->SetPoint2(pt2);
// COMMENT: {8/12/2005 4:49:00 PM}		}
// COMMENT: {8/12/2005 4:49:00 PM}// COMMENT: {8/12/2005 1:34:28 PM}		this->AddPart(this->PosXLineActors[i]);
// COMMENT: {8/12/2005 4:49:00 PM}	}
// COMMENT: {8/12/2005 4:49:00 PM}
// COMMENT: {8/12/2005 4:49:00 PM}	for (int i = this->m_gridKeyword.m_grid[0].count_coord; i < num; ++i)
// COMMENT: {8/12/2005 4:49:00 PM}	{
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineSources[i] = vtkLineSource::New();
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineFilters[i] = vtkTubeFilter::New();
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineFilters[i]->SetRadius(250);
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineFilters[i]->SetNumberOfSides(8);
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineFilters[i]->SetInput(this->PosXLineSources[i]->GetOutput());
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineMappers[i] = vtkPolyDataMapper::New();
// COMMENT: {8/12/2005 4:49:00 PM}		//this->PosXLineMappers[i]->SetInput(this->PosXLineSources[i]->GetOutput());
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineMappers[i]->SetInput(this->PosXLineFilters[i]->GetOutput());
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineActors[i] = vtkLODActor::New();
// COMMENT: {8/12/2005 4:49:00 PM}		this->PosXLineActors[i]->SetMapper(this->PosXLineMappers[i]);
// COMMENT: {8/12/2005 4:49:00 PM}		{
// COMMENT: {8/12/2005 4:49:00 PM}			int j = i - this->m_gridKeyword.m_grid[0].count_coord;
// COMMENT: {8/12/2005 4:49:00 PM}			if (this->m_gridKeyword.m_grid[1].uniform)
// COMMENT: {8/12/2005 4:49:00 PM}			{
// COMMENT: {8/12/2005 4:49:00 PM}				x[1] = min[1] + j * inc[1];
// COMMENT: {8/12/2005 4:49:00 PM}			}
// COMMENT: {8/12/2005 4:49:00 PM}			else
// COMMENT: {8/12/2005 4:49:00 PM}			{
// COMMENT: {8/12/2005 4:49:00 PM}				x[1] = this->m_gridKeyword.m_grid[1].coord[j] * units.horizontal.input_to_si;
// COMMENT: {8/12/2005 4:49:00 PM}			}
// COMMENT: {8/12/2005 4:49:00 PM}			//{{
// COMMENT: {8/12/2005 4:49:00 PM}			pt1[0] = this->m_min[0];
// COMMENT: {8/12/2005 4:49:00 PM}			pt1[2] = this->m_max[2];
// COMMENT: {8/12/2005 4:49:00 PM}			pt2[0] = this->m_max[0];
// COMMENT: {8/12/2005 4:49:00 PM}			pt2[2] = this->m_max[2];
// COMMENT: {8/12/2005 4:49:00 PM}			//}}
// COMMENT: {8/12/2005 4:49:00 PM}			pt1[1] = pt2[1] = x[1];
// COMMENT: {8/12/2005 4:49:00 PM}			this->ScaleTransform->TransformPoint(pt1, pt1);
// COMMENT: {8/12/2005 4:49:00 PM}			this->ScaleTransform->TransformPoint(pt2, pt2);
// COMMENT: {8/12/2005 4:49:00 PM}			this->PosXLineSources[i]->SetPoint1(pt1);
// COMMENT: {8/12/2005 4:49:00 PM}			this->PosXLineSources[i]->SetPoint2(pt2);
// COMMENT: {8/12/2005 4:49:00 PM}		}
// COMMENT: {8/12/2005 4:49:00 PM}// COMMENT: {8/12/2005 1:34:22 PM}		this->AddPart(this->PosXLineActors[i]);
// COMMENT: {8/12/2005 4:49:00 PM}	}
// COMMENT: {8/12/2005 4:49:00 PM}
// COMMENT: {8/12/2005 4:49:00 PM}	this->PosXLineCount = num;
// COMMENT: {8/12/2005 4:49:00 PM}	//}}


// COMMENT: {8/10/2005 5:14:49 PM}	// +z plane
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[0]->SetOrigin(this->m_min[0], this->m_min[1], this->m_max[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[0]->SetPoint1(this->m_min[0], this->m_max[1], this->m_max[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[0]->SetPoint2(this->m_max[0], this->m_min[1], this->m_max[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneActors[0]->GetProperty()->SetColor(0, 0, 1);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneActors[0]->GetProperty()->SetOpacity(0.1);
// COMMENT: {8/10/2005 5:14:49 PM}
// COMMENT: {8/10/2005 5:14:49 PM}	// -z plane
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[1]->SetOrigin(this->m_min[0], this->m_min[1], this->m_min[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[1]->SetPoint1(this->m_min[0], this->m_max[1], this->m_min[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[1]->SetPoint2(this->m_max[0], this->m_min[1], this->m_min[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneActors[1]->GetProperty()->SetColor(0, 0, 1);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneActors[1]->GetProperty()->SetOpacity(0.1);
// COMMENT: {8/10/2005 5:14:49 PM}
// COMMENT: {8/10/2005 5:14:49 PM}
// COMMENT: {8/10/2005 5:14:49 PM}	// +y plane
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[2]->SetOrigin(this->m_min[0], this->m_max[1], this->m_min[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[2]->SetPoint1(this->m_min[0], this->m_max[1], this->m_max[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[2]->SetPoint2(this->m_max[0], this->m_max[1], this->m_min[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneActors[2]->GetProperty()->SetColor(0, 1, 0);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneActors[2]->GetProperty()->SetOpacity(0.1);
// COMMENT: {8/10/2005 5:14:49 PM}
// COMMENT: {8/10/2005 5:14:49 PM}	// -y plane
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[3]->SetOrigin(this->m_min[0], this->m_min[1], this->m_min[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[3]->SetPoint1(this->m_min[0], this->m_min[1], this->m_max[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[3]->SetPoint2(this->m_max[0], this->m_min[1], this->m_min[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneActors[3]->GetProperty()->SetColor(0, 1, 0);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneActors[3]->GetProperty()->SetOpacity(0.1);
// COMMENT: {8/10/2005 5:14:49 PM}
// COMMENT: {8/10/2005 5:14:49 PM}	// +x plane
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[4]->SetOrigin(this->m_max[0], this->m_min[1], this->m_min[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[4]->SetPoint1(this->m_max[0], this->m_min[1], this->m_max[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[4]->SetPoint2(this->m_max[0], this->m_max[1], this->m_min[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneActors[4]->GetProperty()->SetColor(1, 0, 0);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneActors[4]->GetProperty()->SetOpacity(0.1);
// COMMENT: {8/10/2005 5:14:49 PM}
// COMMENT: {8/10/2005 5:14:49 PM}	// -x plane
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[5]->SetOrigin(this->m_min[0], this->m_min[1], this->m_min[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[5]->SetPoint1(this->m_min[0], this->m_min[1], this->m_max[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneSources[5]->SetPoint2(this->m_min[0], this->m_max[1], this->m_min[2]);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneActors[5]->GetProperty()->SetColor(1, 0, 0);
// COMMENT: {8/10/2005 5:14:49 PM}	this->PlaneActors[5]->GetProperty()->SetOpacity(0.1);


	if (!this->PlaneWidget)
	{
		//this->PlaneWidget = vtkImplicitPlaneWidget::New();
		this->PlaneWidget = CGridLineWidget::New();

		vtkCallbackCommand* Callback = vtkCallbackCommand::New();
		Callback->SetClientData(this); 
		Callback->SetCallback(CGridActor::ProcessEvents);

		this->PlaneWidget->AddObserver(vtkCommand::InteractionEvent, Callback);
		this->PlaneWidget->AddObserver(vtkCommand::EndInteractionEvent, Callback);
		this->PlaneWidget->AddObserver(vtkCommand::StartInteractionEvent, Callback);
		Callback->Delete();

		//this->PlaneWidget->SetInteractor(this->Interactor);
		this->PlaneWidget->SetInput(sgrid);
// COMMENT: {8/1/2005 6:36:30 PM}		this->PlaneWidget->PlaceWidget();
		//this->PlaneWidget->NormalToZAxisOn();
		//this->PlaneWidget->EnabledOn();

		//{{
		this->PlaneWidget->SetPlaceFactor(1);
		//}}
	}

	this->m_pGeometryFilter->SetInput(sgrid);
	sgrid->Delete();

	this->m_pFeatureEdges->SetInput(this->m_pGeometryFilter->GetOutput());
}

void CGridActor::GetDefaultZone(CZone& rZone)
{
	ASSERT(this->m_gridKeyword.m_grid[0].uniform_expanded == FALSE);
	ASSERT(this->m_gridKeyword.m_grid[1].uniform_expanded == FALSE);
	ASSERT(this->m_gridKeyword.m_grid[2].uniform_expanded == FALSE);

	rZone.zone_defined = TRUE;
	rZone.x1 = this->m_gridKeyword.m_grid[0].coord[0];
	rZone.y1 = this->m_gridKeyword.m_grid[1].coord[0];
	rZone.z1 = this->m_gridKeyword.m_grid[2].coord[0];

	if (this->m_gridKeyword.m_grid[0].uniform)
	{
		rZone.x2 = this->m_gridKeyword.m_grid[0].coord[1];
	}
	else
	{
		rZone.x2 = this->m_gridKeyword.m_grid[0].coord[this->m_gridKeyword.m_grid[0].count_coord - 1];
	}

	if (this->m_gridKeyword.m_grid[1].uniform)
	{
		rZone.y2 = this->m_gridKeyword.m_grid[1].coord[1];
	}
	else
	{
		rZone.y2 = this->m_gridKeyword.m_grid[1].coord[this->m_gridKeyword.m_grid[1].count_coord - 1];
	}

	if (this->m_gridKeyword.m_grid[2].uniform)
	{
		rZone.z2 = this->m_gridKeyword.m_grid[2].coord[1];
	}
	else
	{
		rZone.z2 = this->m_gridKeyword.m_grid[2].coord[this->m_gridKeyword.m_grid[2].count_coord - 1];
	}
}

void CGridActor::SetInteractor(vtkRenderWindowInteractor* iren)
{
	if (iren == this->Interactor)
	{
		return;
	}

	// if we already have an Interactor then stop observing it
	if (this->Interactor)
	{
		this->Interactor->UnRegister(this);
		this->Interactor->RemoveObserver(this->EventCallbackCommand);
	}

	this->Interactor = iren;
	this->Interactor->Register(this);

	if (iren)
	{
		this->SetEnabled(1);
	}
	this->Modified();
}

void CGridActor::SetEnabled(int enabling)
{
	if (!this->Interactor)
	{
		vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
		return;
	}

	if ( enabling ) //------------------------------------------------------------
	{
		vtkDebugMacro(<<"Enabling grid interaction");

		if ( this->Enabled ) //already enabled, just return
		{
			return;
		}

		if ( ! this->CurrentRenderer )
		{
			this->CurrentRenderer = this->Interactor->FindPokedRenderer(
				this->Interactor->GetLastEventPosition()[0],
				this->Interactor->GetLastEventPosition()[1]);
			if (this->CurrentRenderer == NULL)
			{
				return;
			}
		}

		this->Enabled = 1;

		// listen to the following events
		vtkRenderWindowInteractor *i = this->Interactor;
		i->AddObserver(vtkCommand::MouseMoveEvent,
			this->EventCallbackCommand, 10);
		i->AddObserver(vtkCommand::LeftButtonPressEvent, 
			this->EventCallbackCommand, 10);
		i->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
			this->EventCallbackCommand, 10);
		i->AddObserver(vtkCommand::KeyPressEvent, 
			this->EventCallbackCommand, 10);

	}

	else //disabling-------------------------------------------------------------
	{
		vtkDebugMacro(<<"Disabling grid interaction");

		if ( ! this->Enabled ) //already disabled, just return
		{
			return;
		}

		this->Enabled = 0;

		// don't listen for events any more
		if (this->Interactor)
		{
			if (this->PlaneWidget && this->PlaneWidget->GetEnabled())
			{
				this->PlaneWidget->SetEnabled(enabling);
			}
			this->Interactor->RemoveObserver(this->EventCallbackCommand);
			this->Interactor->UnRegister(this);
			this->Interactor = 0;
		}
	}

	if (this->Interactor)
	{
		this->Interactor->Render();
	}
}

//void CGridActor::ProcessEvents(vtkObject* vtkNotUsed(object), 
//									   unsigned long event,
//									   void* clientdata, 
//									   void* vtkNotUsed(calldata))
void CGridActor::ProcessEvents(vtkObject* object, unsigned long event, void* clientdata, void* calldata)
{
	CGridActor* self 
		= reinterpret_cast<CGridActor *>( clientdata );

	switch(event)
	{
	case vtkCommand::MouseMoveEvent:
		if (true)
		{
			self->OnMouseMove();
		}
		else
		{
			if (!self->Interactor) return;

			int X = self->Interactor->GetEventPosition()[0];
			int Y = self->Interactor->GetEventPosition()[1];

			vtkRenderer *ren = self->Interactor->FindPokedRenderer(X,Y);
			if ( ren != self->CurrentRenderer )
			{
				return;
			}

			if (self->PlanePicker->Pick(X, Y, 0.0, self->CurrentRenderer))
			{
				for (int i = 0; i < 6; ++i)
				{
					if (self->PlanePicker->GetActor() == self->PlaneActors[i])
					{
						self->PlaneActors[i]->GetProperty()->SetOpacity(1.0);
						//{{
						if (i == 0)
						{
							//vtkCellPicker* pCellPicker = vtkCellPicker::New();
							//pCellPicker->PickFromListOn();
							self->LinePicker->PickFromListOn();
							for (int j = 0; j < self->PosXLineCount; ++j)
							{
								//pCellPicker->AddPickList(self->PosXLineActors[j]);
								self->LinePicker->AddPickList(self->PosXLineActors[j]);
								self->PosXLineActors[j]->GetProperty()->SetColor(1, 1, 1);
							}
							if (self->LinePicker->Pick(X, Y, 0.0, self->CurrentRenderer))
							{
								//TRACE("Line Picked\n");
								//pCellPicker->GetActor()->GetProperty()->SetColor(1, 0, 0);
								vtkActorCollection *col = self->LinePicker->GetActors();
								col->InitTraversal();
								int item;
								vtkActor* pItem = self->LinePicker->GetActor();
								while (vtkActor *pActor = col->GetNextActor())
								{
									for (int j = 0; j < self->PosXLineCount; ++j)
									{
										if (pActor == self->PosXLineActors[j])
										{
											item = j;
										}
										if (pActor == self->PosXLineActors[j])
										{
											self->PosXLineActors[j]->GetProperty()->SetColor(1, 0, 0);
											break;
										}
									}
								}
								////self->LinePicker->GetActor()->GetProperty()->SetColor(1, 1, 0);
								//{{
								vtkFloatingPointType length;
								vtkFloatingPointType bounds[6];
								self->GetBounds(bounds);

								self->PlaneWidget->SetInteractor(self->Interactor);
								self->PlaneWidget->NormalToXAxisOn();
								length = (bounds[1] - bounds[0]);
								bounds[0] -= length * 4;
								bounds[1] += length * 4;
								self->PlaneWidget->PlaceWidget(bounds);
								///this->PlaneWidget->SetOrigin(pt0[0] * scale[0], 0, 0);
								vtkFloatingPointType pt1[3];
								self->PosXLineSources[item]->GetPoint1(pt1);
								self->PlaneWidget->SetOrigin(pt1[0], 0, 0);
								self->PlaneWidget->EnabledOn();
								self->Interactor->Render();
								//}}
							}
							//pCellPicker->Delete();
						}
						//}}
					}
					else
					{
						self->PlaneActors[i]->GetProperty()->SetOpacity(0.1);
					}
				}
			}
			else
			{
				for (int i = 0; i < 6; ++i)
				{
					self->PlaneActors[i]->GetProperty()->SetOpacity(0.1);
				}
				for (int j = 0; j < self->PosXLineCount; ++j)
				{
					self->PosXLineActors[j]->GetProperty()->SetColor(1, 1, 1);
				}
			}
			if (self->Interactor)
			{
				self->Interactor->Render();
			}
		}

		break;

	case vtkCommand::LeftButtonPressEvent: 
		self->OnLeftButtonDown();
		break;

	case vtkCommand::LeftButtonReleaseEvent:
		self->OnLeftButtonUp();
		break;

	case vtkCommand::KeyPressEvent:
		self->OnKeyPress();
		break;

	case vtkCommand::InteractionEvent:
		ASSERT(object == self->PlaneWidget);
		self->OnInteraction();
		break;

	case vtkCommand::EndInteractionEvent:
		ASSERT(object == self->PlaneWidget);
		self->OnEndInteraction();
		break;

	case vtkCommand::StartInteractionEvent:
		TRACE("StartInteractionEvent\n");
		ASSERT(object == self->PlaneWidget);
		self->State = CGridActor::Pushing;
		break;
	}
}


void CGridActor::OnLeftButtonDown()
{
	if (!this->Interactor) return;

	int X = this->Interactor->GetEventPosition()[0];
	int Y = this->Interactor->GetEventPosition()[1];

	vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
	if ( ren != this->CurrentRenderer )
	{
		return;
	}
	TRACE("OnLeftButtonDown X = %d, Y= %d\n", X, Y);
}

void CGridActor::OnMouseMove()
{
	if (!this->Interactor) return;

	int X = this->Interactor->GetEventPosition()[0];
	int Y = this->Interactor->GetEventPosition()[1];

	vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
	if ( ren != this->CurrentRenderer )
	{
		return;
	}
	if (this->State != CGridActor::Start)
	{
		return;
	}
#if defined(WIN32)
	if (::GetAsyncKeyState(VK_LBUTTON) < 0)
	{
		// ignore if right mouse button is down
		return;
	}
	if (::GetAsyncKeyState(VK_RBUTTON) < 0)
	{
		// ignore if right mouse button is down
		return;
	}
	if (::GetAsyncKeyState(VK_MBUTTON) < 0)
	{
		// ignore if right mouse button is down
		return;
	}
#endif
	TRACE("CGridActor::OnMouseMove X = %d, Y= %d\n", X, Y);

	int nPlane;
	if (this->PlanePicker->Pick(X, Y, 0.0, this->CurrentRenderer))
	{
		vtkActor* pActor = this->PlanePicker->GetActor();
		for (int i = 0; i < 6; ++i)
		{
			if (this->PlaneActors[i] == pActor)
			{
                nPlane = i;
				break;
			}
		}
	}
	else
	{
		this->AxisIndex = -1;
		this->PlaneWidget->SetInteractor(this->Interactor);
		this->PlaneWidget->EnabledOff();
		for (int i = 0; i < 6; ++i)
		{
			this->PlaneActors[i]->GetProperty()->SetOpacity(0.1);
		}
		this->Interactor->Render();
		return;
	}

// COMMENT: {8/16/2005 4:36:46 PM}	this->LinePicker->Pick(X, Y, 0.0, ren);
	this->LinePicker->PickFromListOn();
	this->LinePicker->AddPickList(this->Actor);
	//{{
// COMMENT: {8/16/2005 4:36:19 PM}	vtkCellPicker* pCellPicker = this->LinePicker;
	//}}


// COMMENT: {8/16/2005 4:33:08 PM}	vtkCellPicker* pCellPicker = vtkCellPicker::New();
// COMMENT: {8/16/2005 4:33:08 PM}	//pCellPicker->SetTolerance(0.001);
// COMMENT: {8/16/2005 4:33:08 PM}	pCellPicker->SetTolerance(0.002);
// COMMENT: {8/16/2005 4:33:55 PM}	pCellPicker->PickFromListOn();
// COMMENT: {8/16/2005 4:33:55 PM}	this->PickableOn();
// COMMENT: {8/9/2005 8:09:18 PM}	pCellPicker->AddPickList(this);
	this->Actor->PickableOn();
// COMMENT: {8/16/2005 4:34:02 PM}	pCellPicker->AddPickList(this->Actor);

	//{{
	vtkAssemblyPath *path;
	this->LinePicker->Pick(X, Y, 0.0, ren);
	path = this->LinePicker->GetPath();
	//}}

// COMMENT: {8/11/2005 3:29:09 PM}	if (pCellPicker->Pick(X, Y, 0.0, this->CurrentRenderer))
	if (path != 0)
	{
		//{{
		vtkActor* pActor = vtkActor::SafeDownCast(path->GetFirstNode()->GetProp());
		ASSERT(path->GetNumberOfItems() == 1);
		ASSERT(pActor);
		ASSERT(pActor == this->Actor);
// COMMENT: {8/11/2005 3:35:26 PM}		return;
		//}}

		vtkIdType n = this->LinePicker->GetCellId();
		TRACE("CellId = %d\n", n);
		vtkFloatingPointType* pt = this->LinePicker->GetPickPosition();
		if (vtkDataSet* pDataSet = this->LinePicker->GetDataSet())
		{
			vtkCell* pCell = pDataSet->GetCell(n);
			if (pCell->GetCellType() == VTK_LINE)
			{
				ASSERT(pCell->GetNumberOfPoints() == 2);				
				if (vtkPoints* pPoints = pCell->GetPoints())
				{
					pPoints->GetPoint(0, this->CurrentPoint);
					vtkFloatingPointType* pt0 = this->CurrentPoint;
					vtkFloatingPointType* pt1 = pPoints->GetPoint(1);
					TRACE("pt0[0] = %g, pt0[1] = %g, pt0[2] = %g\n", pt0[0], pt0[1], pt0[2]);
					TRACE("pt1[0] = %g, pt1[1] = %g, pt1[2] = %g\n", pt1[0], pt1[1], pt1[2]);

					vtkFloatingPointType length;
					vtkFloatingPointType bounds[6];
					this->GetBounds(bounds);

					if (nPlane == 0 && ( pt0[2] == this->m_max[2] && pt1[2] == this->m_max[2] )
						||
						nPlane == 1 && ( pt0[2] == this->m_min[2] && pt1[2] == this->m_min[2] ) )
					{
						// +z / -z
						//
						if (pt0[0] == pt1[0])
						{
							if (this->PlaneWidget)
							{
								this->AxisIndex = 0;
								this->PlaneWidget->SetInteractor(this->Interactor);
								this->PlaneWidget->SetInitialPickPosition(this->CurrentPoint);
								this->PlaneWidget->NormalToXAxisOn();
								length = (bounds[1] - bounds[0]);
								bounds[0] -= length * 4;
								bounds[1] += length * 4;
								this->PlaneWidget->PlaceWidget(bounds);
								this->PlaneWidget->SetOrigin(pt0[0], 0, 0);
								this->PlaneWidget->EnabledOn();
								this->PlaneWidget->GetPlaneProperty()->SetColor(1, 0, 0);
								this->PlaneWidget->GetSelectedPlaneProperty()->SetColor(1, 0, 0);
								this->PlanePicker->GetActor()->GetProperty()->SetOpacity(0.20);
								this->Interactor->Render();
							}
						}
						else if (pt0[1] == pt1[1])
						{
							if (this->PlaneWidget)
							{
								this->AxisIndex = 1;
								this->PlaneWidget->SetInteractor(this->Interactor);
								this->PlaneWidget->SetInitialPickPosition(this->CurrentPoint);
								this->PlaneWidget->NormalToYAxisOn();
								length = (bounds[3] - bounds[2]);
								bounds[2] -= length * 4;
								bounds[3] += length * 4;
								this->PlaneWidget->PlaceWidget(bounds);
								this->PlaneWidget->SetOrigin(0, pt0[1], 0);
								this->PlaneWidget->EnabledOn();
								this->PlaneWidget->GetPlaneProperty()->SetColor(0, 1, 0);
								this->PlaneWidget->GetSelectedPlaneProperty()->SetColor(0, 1, 0);
								this->PlanePicker->GetActor()->GetProperty()->SetOpacity(0.20);
								this->Interactor->Render();
							}
						}
						else
						{
							ASSERT(FALSE);
						}
					}
					if (nPlane == 2 && ( pt0[1] == this->m_max[1] && pt1[1] == this->m_max[1] )
						||
						nPlane == 3 && ( pt0[1] == this->m_min[1] && pt1[1] == this->m_min[1] ) )
					{
						// -y / +y
						//
						if (pt0[0] == pt1[0])
						{
							if (this->PlaneWidget)
							{
								this->AxisIndex = 0;
								this->PlaneWidget->SetInteractor(this->Interactor);
								this->PlaneWidget->SetInitialPickPosition(this->CurrentPoint);
								this->PlaneWidget->NormalToXAxisOn();
								length = (bounds[1] - bounds[0]);
								bounds[0] -= length * 4;
								bounds[1] += length * 4;
								this->PlaneWidget->PlaceWidget(bounds);
								this->PlaneWidget->SetOrigin(pt0[0], 0, 0);
								this->PlaneWidget->EnabledOn();
								this->PlaneWidget->GetPlaneProperty()->SetColor(1, 0, 0);
								this->PlaneWidget->GetSelectedPlaneProperty()->SetColor(1, 0, 0);
								this->PlanePicker->GetActor()->GetProperty()->SetOpacity(0.20);
								this->Interactor->Render();
							}
						}
						else if (pt0[2] == pt1[2])
						{
							if (this->PlaneWidget)
							{
								this->AxisIndex = 2;
								this->PlaneWidget->SetInteractor(this->Interactor);
								this->PlaneWidget->SetInitialPickPosition(this->CurrentPoint);
								this->PlaneWidget->NormalToZAxisOn();
								length = (bounds[5] - bounds[4]);
								bounds[4] -= length * 4;
								bounds[5] += length * 4;
								this->PlaneWidget->PlaceWidget(bounds);
								this->PlaneWidget->SetOrigin(0, 0, pt0[2]);
								this->PlaneWidget->EnabledOn();
								this->PlaneWidget->GetPlaneProperty()->SetColor(0, 0, 1);
								this->PlaneWidget->GetSelectedPlaneProperty()->SetColor(0, 0, 1);
								this->PlanePicker->GetActor()->GetProperty()->SetOpacity(0.20);
								this->Interactor->Render();
							}
						}
						else
						{
							ASSERT(FALSE);
						}
					}
					if (nPlane == 4 && ( pt0[0] == this->m_max[0] && pt1[0] == this->m_max[0] )
						||
						nPlane == 5 && ( pt0[0] == this->m_min[0] && pt1[0] == this->m_min[0] ) )
					{
						// +x / -x
						//
						if (pt0[1] == pt1[1])
						{
							if (this->PlaneWidget)
							{
								this->AxisIndex = 1;
								this->PlaneWidget->SetInteractor(this->Interactor);
								this->PlaneWidget->SetInitialPickPosition(this->CurrentPoint);
								this->PlaneWidget->NormalToYAxisOn();
								length = (bounds[3] - bounds[2]);
								bounds[2] -= length * 4;
								bounds[3] += length * 4;
								this->PlaneWidget->PlaceWidget(bounds);
								this->PlaneWidget->SetOrigin(0, pt0[1], 0);
								this->PlaneWidget->EnabledOn();
								this->PlaneWidget->GetPlaneProperty()->SetColor(0, 1, 0);
								this->PlaneWidget->GetSelectedPlaneProperty()->SetColor(0, 1, 0);
								this->PlanePicker->GetActor()->GetProperty()->SetOpacity(0.20);
								this->Interactor->Render();
							}
						}
						else if (pt0[2] == pt1[2])
						{
							if (this->PlaneWidget)
							{
								this->AxisIndex = 2;
								this->PlaneWidget->SetInteractor(this->Interactor);
								this->PlaneWidget->SetInitialPickPosition(this->CurrentPoint);
								this->PlaneWidget->NormalToZAxisOn();
								length = (bounds[5] - bounds[4]);
								bounds[4] -= length * 4;
								bounds[5] += length * 4;
								this->PlaneWidget->PlaceWidget(bounds);
								this->PlaneWidget->SetOrigin(0, 0, pt0[2]);
								this->PlaneWidget->EnabledOn();
								this->PlaneWidget->GetPlaneProperty()->SetColor(0, 0, 1);
								this->PlaneWidget->GetSelectedPlaneProperty()->SetColor(0, 0, 1);
								this->PlanePicker->GetActor()->GetProperty()->SetOpacity(0.20);
								this->Interactor->Render();
							}
						}
						else
						{
							ASSERT(FALSE);
						}
					}
				}
			}
		}
		TRACE("pt[0] = %g, pt[1] = %g, pt[2] = %g\n", pt[0], pt[1], pt[2]);
	}
	else
	{
		this->AxisIndex = -1;
		this->PlaneWidget->SetInteractor(this->Interactor);
		this->PlaneWidget->EnabledOff();
		for (int i = 0; i < 6; ++i)
		{
			this->PlaneActors[i]->GetProperty()->SetOpacity(0.1);
		}
		this->Interactor->Render();
	}
	this->LinePicker->DeletePickList(this->Actor);
// COMMENT: {8/16/2005 4:34:51 PM}	pCellPicker->Delete();
// COMMENT: {8/16/2005 4:35:02 PM}	this->PickableOff();
	this->Actor->PickableOff();
}

void CGridActor::OnLeftButtonUp()
{
	if (!this->Interactor) return;

	int X = this->Interactor->GetEventPosition()[0];
	int Y = this->Interactor->GetEventPosition()[1];

	vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
	if ( ren != this->CurrentRenderer )
	{
		return;
	}
}

void CGridActor::OnKeyPress()
{
	if (!this->Interactor) return;

	char* keysym = this->Interactor->GetKeySym();
	TRACE("OnKeyPress %s\n", keysym);
	
	if (this->PlaneWidget->GetEnabled())
	{
		ASSERT(this->AxisIndex != -1);
		if (::strcmp(keysym, "Delete") == 0 && this->AxisIndex != -1)
		{
			this->PlaneIndex = -1;
			std::map<vtkFloatingPointType, int>::iterator i = this->ValueToIndex[this->AxisIndex].find(this->CurrentPoint[this->AxisIndex]);
			if (i != this->ValueToIndex[this->AxisIndex].end())
			{
				if (this->m_gridKeyword.m_grid[this->AxisIndex].count_coord > 2)
				{
					this->PlaneIndex = i->second;
					VERIFY(this->DeleteLine(this->AxisIndex, this->PlaneIndex));

					// notify listeners
					this->InvokeEvent(CGridActor::DeleteGridLineEvent, &this->m_dDeletedValue);
				}
				else
				{
					::AfxMessageBox("There must be at least two nodes in each coordinate direction.");
				}
				this->State = CGridActor::Start;
			}
		}
	}
}

void CGridActor::OnInteraction(void)
{
	TRACE("CGridActor::OnInteraction\n");
	// set state
	this->State = CGridActor::Pushing;

	// HACK {{
	extern HCURSOR Test();
	if (s_hcur == 0)
	{
		s_hcur = Test();
	}
	if (s_hcur && (::GetAsyncKeyState(VK_CONTROL) < 0))
	{
		::SetCursor(s_hcur);
	}
	else
	{
		::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	}
	// HACK }}
}

void CGridActor::OnEndInteraction(void)
{
	if (0 <= this->AxisIndex && this->AxisIndex < 3)
	{
		// if Ctrl is pressed copy line otherwise move line
		//
		bool bMoving = !(::GetAsyncKeyState(VK_CONTROL) < 0);

		// lookup current point and convert to grid index
		//
		std::map<vtkFloatingPointType, int>::iterator setIter = this->ValueToIndex[this->AxisIndex].find(this->CurrentPoint[this->AxisIndex]);
		if (setIter != this->ValueToIndex[this->AxisIndex].end())
		{
			int originalPlaneIndex = setIter->second;
			// be careful here the iterator setIter should not be used below here
			// because DeleteLine/InsertLine refill ValueToIndex making the
			// interator no longer valid
			struct GridLineMoveMemento memento;
			memento.Uniform = this->m_gridKeyword.m_grid[this->AxisIndex].uniform;
			if (bMoving)
			{
				ASSERT(this->DeleteLine(this->AxisIndex, originalPlaneIndex));
			}
			double value = this->PlaneWidget->GetOrigin()[this->AxisIndex] / this->GetScale()[this->AxisIndex];
			this->PlaneIndex = this->InsertLine(this->AxisIndex, value);
			if (this->PlaneIndex != -1)
			{
				if (bMoving)
				{
					// the memento is the only information listeners
					// need in order to undo/redo
					memento.AxisIndex          = this->AxisIndex;
					memento.OriginalPlaneIndex = originalPlaneIndex;
					memento.NewPlaneIndex      = this->PlaneIndex;
					memento.OriginalCoord      = this->m_dDeletedValue;
					memento.NewCoord           = value;

					// notify listeners
					this->InvokeEvent(CGridActor::MoveGridLineEvent, &memento);
				}
				else
				{
					// at this point the variables:
					//     AxisIndex
					//     PlaneIndex
					// should be valid for the use of the listeners use

					// notify listeners
					this->InvokeEvent(CGridActor::InsertGridLineEvent, &value);
				}
			}
			else
			{
				// PlaneIndex well be -1 if the moved line lands exactly
				// on an existing line.  Therefore this just becomes a deleted line.
				this->PlaneIndex = originalPlaneIndex;

				// notify listeners
				this->InvokeEvent(CGridActor::DeleteGridLineEvent, &this->m_dDeletedValue);
			}
		}
	}

	// reset state
	//
	this->State = CGridActor::Start;
	this->AxisIndex = -1;
	this->PlaneIndex = -1;
	::SetCursor(::LoadCursor(NULL, IDC_ARROW));
}

#if defined(WIN32)
BOOL CGridActor::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (!this->Interactor) return FALSE;

	int X = this->Interactor->GetEventPosition()[0];
	int Y = this->Interactor->GetEventPosition()[1];

	vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
	if ( ren != this->CurrentRenderer )
	{
		return FALSE;
	}

	extern HCURSOR Test();
	if (s_hcur == 0)
	{
		s_hcur = Test();
	}
	if (s_hcur && (::GetAsyncKeyState(VK_CONTROL) < 0))
	{
		::SetCursor(s_hcur);
		return TRUE;
	}
	return FALSE;
}
#endif

BOOL CGridActor::DeleteLine(int nAxisIndex, int nPlaneIndex)
{
	if ( (0 <= nAxisIndex) && (nAxisIndex < 3) )
	{
		CGrid grid = this->m_gridKeyword.m_grid[nAxisIndex];
		if ( (grid.count_coord > 2) && (0 <= nPlaneIndex) && (nPlaneIndex < grid.count_coord) )
		{
			// expand uniform grids
			grid.Setup();

			// store coordinates
			std::set<double> coordinates;
			coordinates.insert(grid.coord, grid.coord + grid.count_coord);

			this->m_dDeletedValue = grid.coord[nPlaneIndex];
			TRACE("CGridActor::DeleteLine %d = %g from grid[%d]\n", nPlaneIndex, this->m_dDeletedValue, nAxisIndex);

			// remove coordinate
			VERIFY(coordinates.erase(grid.coord[nPlaneIndex]) == 1);

			// insert new coordinates
			this->m_gridKeyword.m_grid[nAxisIndex].insert(coordinates.begin(), coordinates.end());

			// update grid data
			this->Setup(this->m_units);

			// update tree
			this->UpdateNode();

			// turn off widget
			if (this->PlaneWidget) this->PlaneWidget->Off();

			return TRUE; // success
		}
		else
		{
			TRACE("CGridActor::DeleteLine: Bad Plane Index => %d for grid[%d](0..%d)\n", nPlaneIndex, nAxisIndex, grid.count_coord);
		}
	}
	else
	{
		TRACE("CGridActor::DeleteLine: Bad Axis Index => %d\n", nAxisIndex);
	}
	return FALSE; // failure
}

int CGridActor::InsertLine(int nAxisIndex, double dValue)
{
	if ( (0 <= nAxisIndex) && (nAxisIndex < 3) )
	{
		// copy and expand uniform grids
		CGrid grid = this->m_gridKeyword.m_grid[nAxisIndex];
		grid.Setup();

		// store coordinates
		std::set<double> coordinates;
		coordinates.insert(grid.coord, grid.coord + grid.count_coord);

		// insert plane
		std::pair< std::set<double>::iterator, bool > ins = coordinates.insert(dValue);
		if (ins.second)
		{
			TRACE("CGridActor::InsertLine %g into grid[%d]\n", dValue, nAxisIndex);

			// insert new coordinates
			this->m_gridKeyword.m_grid[nAxisIndex].insert(coordinates.begin(), coordinates.end());

			// update grid data
			this->Setup(this->m_units);

			// update tree
			this->UpdateNode();

			// turn off widget
			if (this->PlaneWidget) this->PlaneWidget->Off();

			int i = 0;
			std::set<double>::iterator it = coordinates.begin();
			for (; it != ins.first; ++it, ++i);
			ASSERT( (0 <= i) && (i < grid.count_coord + 1) );
			return i; // success
		}
		else
		{
			TRACE("CGridActor::InsertLine: Bad Value %g already exists\n", dValue);
		}
	}
	else
	{
		TRACE("CGridActor::InsertLine: Bad Axis Index => %d\n", nAxisIndex);
	}
	return -1; // failure
}

#if defined(SAVE)
BOOL CGridActor::MoveLine(int nAxisIndex, int nPlaneIndex, double dValue)
{
	if (0 <= nAxisIndex && nAxisIndex < 3)
	{
		CGrid grid = this->m_gridKeyword.m_grid[nAxisIndex];
		if ( (0 <= nPlaneIndex) && (nPlaneIndex < grid.count_coord) )
		{
			// expand uniform grids
			grid.Setup();

			// store coordinates
			std::set<double> coordinates;
			coordinates.insert(grid.coord, grid.coord + grid.count_coord);

			this->m_dDeletedValue = grid.coord[nPlaneIndex];

			// remove coordinate
			VERIFY(coordinates.erase(this->m_dDeletedValue) == 1);

			// insert plane
			if (coordinates.insert(dValue).second)
			{
				// insert new coordinates
				this->m_gridKeyword.m_grid[nAxisIndex].insert(coordinates.begin(), coordinates.end());

				// update grid data
				this->Setup(this->m_units);

				// update tree
				this->UpdateNode();

				// turn off widget
				if (this->PlaneWidget) this->PlaneWidget->Off();

				return TRUE; // success
			}
			else
			{
				TRACE("CGridActor::MoveLine(%d, %d, %g) Unable to insert %g\n", nAxisIndex, nPlaneIndex, dValue, dValue);
			}
		}
		else
		{
			TRACE("CGridActor::MoveLine(%d, %d, %g) Bad Plane Index(0..%d)\n", nAxisIndex, nPlaneIndex, dValue, grid.count_coord);
		}
	}
	return FALSE; // failure
}
#endif

void CGridActor::UpdateNode(void)
{
	// update tree
	this->Insert(this->m_node);
}

void CGridActor::UpdatePoints(void)
{
	// +z plane
	this->PlaneSources[0]->SetOrigin(this->m_min[0], this->m_min[1], this->m_max[2]);
	this->PlaneSources[0]->SetPoint1(this->m_min[0], this->m_max[1], this->m_max[2]);
	this->PlaneSources[0]->SetPoint2(this->m_max[0], this->m_min[1], this->m_max[2]);
	this->PlaneActors[0]->GetProperty()->SetColor(0, 0, 1);
	this->PlaneActors[0]->GetProperty()->SetOpacity(0.1);

	// -z plane
	this->PlaneSources[1]->SetOrigin(this->m_min[0], this->m_min[1], this->m_min[2]);
	this->PlaneSources[1]->SetPoint1(this->m_min[0], this->m_max[1], this->m_min[2]);
	this->PlaneSources[1]->SetPoint2(this->m_max[0], this->m_min[1], this->m_min[2]);
	this->PlaneActors[1]->GetProperty()->SetColor(0, 0, 1);
	this->PlaneActors[1]->GetProperty()->SetOpacity(0.1);

	// +y plane
	this->PlaneSources[2]->SetOrigin(this->m_min[0], this->m_max[1], this->m_min[2]);
	this->PlaneSources[2]->SetPoint1(this->m_min[0], this->m_max[1], this->m_max[2]);
	this->PlaneSources[2]->SetPoint2(this->m_max[0], this->m_max[1], this->m_min[2]);
	this->PlaneActors[2]->GetProperty()->SetColor(0, 1, 0);
	this->PlaneActors[2]->GetProperty()->SetOpacity(0.1);

	// -y plane
	this->PlaneSources[3]->SetOrigin(this->m_min[0], this->m_min[1], this->m_min[2]);
	this->PlaneSources[3]->SetPoint1(this->m_min[0], this->m_min[1], this->m_max[2]);
	this->PlaneSources[3]->SetPoint2(this->m_max[0], this->m_min[1], this->m_min[2]);
	this->PlaneActors[3]->GetProperty()->SetColor(0, 1, 0);
	this->PlaneActors[3]->GetProperty()->SetOpacity(0.1);

	// +x plane
	this->PlaneSources[4]->SetOrigin(this->m_max[0], this->m_min[1], this->m_min[2]);
	this->PlaneSources[4]->SetPoint1(this->m_max[0], this->m_min[1], this->m_max[2]);
	this->PlaneSources[4]->SetPoint2(this->m_max[0], this->m_max[1], this->m_min[2]);
	this->PlaneActors[4]->GetProperty()->SetColor(1, 0, 0);
	this->PlaneActors[4]->GetProperty()->SetOpacity(0.1);

	// -x plane
	this->PlaneSources[5]->SetOrigin(this->m_min[0], this->m_min[1], this->m_min[2]);
	this->PlaneSources[5]->SetPoint1(this->m_min[0], this->m_min[1], this->m_max[2]);
	this->PlaneSources[5]->SetPoint2(this->m_min[0], this->m_max[1], this->m_min[2]);
	this->PlaneActors[5]->GetProperty()->SetColor(1, 0, 0);
	this->PlaneActors[5]->GetProperty()->SetOpacity(0.1);
}

void CGridActor::SetScale(vtkFloatingPointType x, vtkFloatingPointType y, vtkFloatingPointType z)
{
	this->ScaleTransform->Identity();
	this->ScaleTransform->Scale(x, y, z);
	this->Setup(this->m_units);
}

void CGridActor::SetScale(vtkFloatingPointType scale[3])
{
	this->ScaleTransform->Identity();
	this->ScaleTransform->Scale(scale);
	///this->UpdatePoints();
	///this->Actor->SetScale(scale);
	this->Setup(this->m_units);
}

vtkFloatingPointType* CGridActor::GetScale(void)
{
	ASSERT(this->ScaleTransform);
	return this->ScaleTransform->GetScale();
}

void CGridActor::GetScale(vtkFloatingPointType scale[3])
{
	ASSERT(this->ScaleTransform);
	return this->ScaleTransform->GetScale(scale);
}

