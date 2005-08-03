#include "StdAfx.h"
#include "GridLODActor.h"

#include <vtkGeometryFilter.h>
#include <vtkFeatureEdges.h>
#include <vtkPolyDataMapper.h>
#include <vtkObjectFactory.h>


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

#include "Global.h"
#include "Units.h"
#include "Zone.h"
#include "DelayRedraw.h"

#if defined(WIN32)
#include "Resource.h"
#endif


vtkCxxRevisionMacro(CGridLODActor, "$Revision$");
vtkStandardNewMacro(CGridLODActor);

// Note: No header files should follow the next three lines
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CGridLODActor::CGridLODActor(void)
	: m_pGeometryFilter(0)
	, m_pFeatureEdges(0)
	, m_pPolyDataMapper(0)
	, Interactor(0)
	, CurrentRenderer(0)
	, EventCallbackCommand(0)
	, Enabled(0)
	, PlaneWidget(0)
	, State(CGridLODActor::Start)
	, AxisIndex(-1)
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

	this->SetMapper(this->m_pPolyDataMapper);
	this->m_gridKeyword.m_grid[0].c = 'X';
	this->m_gridKeyword.m_grid[1].c = 'Y';
	this->m_gridKeyword.m_grid[2].c = 'Z';

	this->EventCallbackCommand = vtkCallbackCommand::New();
	this->EventCallbackCommand->SetClientData(this); 
	this->EventCallbackCommand->SetCallback(CGridLODActor::ProcessEvents);
}

CGridLODActor::~CGridLODActor(void)
{
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
}

#ifdef _DEBUG
void CGridLODActor::Dump(CDumpContext& dc) const
{
	dc << "<CGridLODActor>\n";
	this->m_gridKeyword.m_grid[0].Dump(dc);
	this->m_gridKeyword.m_grid[1].Dump(dc);
	this->m_gridKeyword.m_grid[2].Dump(dc);
	dc << "</CGridLODActor>\n";
}
#endif

void CGridLODActor::Serialize(bool bStoring, hid_t loc_id)
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
				/// this->SetGrid(x, y, z, 0);
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

void CGridLODActor::Insert(CTreeCtrlNode node)
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

void CGridLODActor::Insert(CTreeCtrl* pTreeCtrl, HTREEITEM htiGrid)
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
	//this->m_htiGrid = htiGrid;
	this->m_node = CTreeCtrlNode(htiGrid, (CTreeCtrlEx*)pTreeCtrl);
}

void CGridLODActor::SetGrid(const CGrid& x, const CGrid& y, const CGrid& z, const CUnits& units)
{
	this->m_gridKeyword.m_grid[0] = x;
	this->m_gridKeyword.m_grid[1] = y;
	this->m_gridKeyword.m_grid[2] = z;

	this->Setup(units);
}

void CGridLODActor::GetGrid(CGrid& x, CGrid& y, CGrid& z)const
{
	x = this->m_gridKeyword.m_grid[0];
	y = this->m_gridKeyword.m_grid[1];
	z = this->m_gridKeyword.m_grid[2];
}

void CGridLODActor::GetGridKeyword(CGridKeyword& gridKeyword)const
{
	gridKeyword = this->m_gridKeyword;
}

void CGridLODActor::SetGridKeyword(const CGridKeyword& gridKeyword, const CUnits& units)
{
	this->m_gridKeyword = gridKeyword;
	this->m_units = units;
	this->Setup(units);
}

void CGridLODActor::Setup(const CUnits& units)
{
	ASSERT(this->m_gridKeyword.m_grid[0].uniform_expanded == FALSE);
	ASSERT(this->m_gridKeyword.m_grid[1].uniform_expanded == FALSE);
	ASSERT(this->m_gridKeyword.m_grid[2].uniform_expanded == FALSE);

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
	float x[3];
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
				points->InsertPoint(offset, x);
			}
		}
	}

	for (i = 0; i < 3; ++i)
	{
		if (this->m_gridKeyword.m_grid[i].uniform)
		{
			this->m_min[i] = min[i];
		}
		else
		{
			if (i == 2)
			{
				this->m_min[i] = this->m_gridKeyword.m_grid[i].coord[0] * units.vertical.input_to_si;
			}
			else
			{
				this->m_min[i] = this->m_gridKeyword.m_grid[i].coord[0] * units.horizontal.input_to_si;
			}
		}
		this->m_max[i] = x[i];
	}

	sgrid->SetPoints(points);
	points->Delete();

	//{{
	if (!this->PlaneWidget)
	{
		//this->PlaneWidget = vtkImplicitPlaneWidget::New();
		this->PlaneWidget = CGridLineWidget::New();

		vtkCallbackCommand* Callback = vtkCallbackCommand::New();
		Callback->SetClientData(this); 
		Callback->SetCallback(CGridLODActor::ProcessEvents);

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
	//}}

	this->m_pGeometryFilter->SetInput(sgrid);
	sgrid->Delete();

	this->m_pFeatureEdges->SetInput(this->m_pGeometryFilter->GetOutput());
}

void CGridLODActor::GetDefaultZone(CZone& rZone)
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

void CGridLODActor::SetInteractor(vtkRenderWindowInteractor* iren)
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

void CGridLODActor::SetEnabled(int enabling)
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
		vtkDebugMacro(<<"Disabling river");

		if ( ! this->Enabled ) //already disabled, just return
		{
			return;
		}

		this->Enabled = 0;

		// don't listen for events any more
		if (this->Interactor)
		{
			if (this->PlaneWidget)
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

//void CGridLODActor::ProcessEvents(vtkObject* vtkNotUsed(object), 
//									   unsigned long event,
//									   void* clientdata, 
//									   void* vtkNotUsed(calldata))
void CGridLODActor::ProcessEvents(vtkObject* object, unsigned long event, void* clientdata, void* calldata)
{
	CGridLODActor* self 
		= reinterpret_cast<CGridLODActor *>( clientdata );

	switch(event)
	{
	case vtkCommand::MouseMoveEvent:
		self->OnMouseMove();
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
		TRACE("InteractionEvent\n");
		ASSERT(object == self->PlaneWidget);
		self->State = CGridLODActor::Pushing;
		break;

	case vtkCommand::EndInteractionEvent:
		TRACE("EndInteractionEvent\n");
		ASSERT(object == self->PlaneWidget);
		{
			self->PlaneIndex = -1;
			CGrid grid = self->m_gridKeyword.m_grid[self->AxisIndex];
			grid.Setup();
			for (int pi = 0; pi < grid.count_coord; ++pi)
			{
				if (grid.coord[pi] == self->CurrentPoint[self->AxisIndex])
				{
					self->PlaneIndex = pi;
					break;
				}
			}
			if (self->PlaneIndex == -1)
			{
				TRACE("Warning self->PlaneIndex is -1\n");
			}
			std::set<double> coordinates;
			coordinates.insert(grid.coord, grid.coord + grid.count_coord);
			double value = self->PlaneWidget->GetOrigin()[self->AxisIndex] / self->GetScale()[self->AxisIndex];
			if (coordinates.insert(value).second)
			{
				if (!(::GetAsyncKeyState(VK_CONTROL) < 0))
				{
					coordinates.erase(self->CurrentPoint[self->AxisIndex]);
				}
			}
			std::set<double>::iterator i = coordinates.begin();
			for (; i != coordinates.end(); ++i)
			{
				TRACE("%g\n", *i);
			}
			self->m_gridKeyword.m_grid[self->AxisIndex].insert(coordinates.begin(), coordinates.end());
			self->Setup(self->m_units);
			self->Insert(self->m_node);
			self->PlaneWidget->Off();
		}
		self->State = CGridLODActor::Start;
		break;

	case vtkCommand::StartInteractionEvent:
		TRACE("StartInteractionEvent\n");
		ASSERT(object == self->PlaneWidget);
		self->State = CGridLODActor::Pushing;
		break;
	}
}


void CGridLODActor::OnLeftButtonDown()
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

void CGridLODActor::OnMouseMove()
{
	if (!this->Interactor) return;

	int X = this->Interactor->GetEventPosition()[0];
	int Y = this->Interactor->GetEventPosition()[1];

	vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
	if ( ren != this->CurrentRenderer )
	{
		return;
	}

	//{{
	if (this->State != CGridLODActor::Start)
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
	TRACE("OnMouseMove X = %d, Y= %d\n", X, Y);
	//}}

	vtkCellPicker* pCellPicker = vtkCellPicker::New();
	pCellPicker->SetTolerance(0.001);
	//pCellPicker->SetTolerance(0.004);
	//pCellPicker->SetTolerance(0.5);
	pCellPicker->PickFromListOn();
	this->PickableOn();
	pCellPicker->AddPickList(this);

	if (pCellPicker->Pick(X, Y, 0.0, this->CurrentRenderer))
	{
		ASSERT(pCellPicker->GetPath()->GetFirstNode()->GetProp() == this);

		vtkIdType n = pCellPicker->GetCellId();
		TRACE("CellId = %d\n", n);
		vtkFloatingPointType* pt = pCellPicker->GetPickPosition();
		if (vtkDataSet* pDataSet = pCellPicker->GetDataSet())
		{
			vtkCell* pCell = pDataSet->GetCell(n);
			if (pCell->GetCellType() == VTK_LINE)
			{
				ASSERT(pCell->GetNumberOfPoints() == 2);				
				if (vtkPoints* pPoints = pCell->GetPoints())
				{
					vtkFloatingPointType* pt0 = pPoints->GetPoint(0);
					vtkFloatingPointType* pt1 = pPoints->GetPoint(1);
					TRACE("pt0[0] = %g, pt0[1] = %g, pt0[2] = %g\n", pt0[0], pt0[1], pt0[2]);
					TRACE("pt1[0] = %g, pt1[1] = %g, pt1[2] = %g\n", pt1[0], pt1[1], pt1[2]);

					if ((pt0[2] == this->m_min[2] && pt1[2] == this->m_min[2]) || (pt0[2] == this->m_max[2] && pt1[2] == this->m_max[2]))
					{
						// +z / -z
						//
						if (pt0[0] == pt1[0])
						{
							if (this->PlaneWidget /** && !this->PlaneWidget->GetEnabled() **/)
							{
								TRACE("this->PlaneWidget->NormalToXAxisOn()\n");
							}
						}
						else if (pt0[1] == pt1[1])
						{
							if (this->PlaneWidget)
							{
								TRACE("this->PlaneWidget->NormalToYAxisOn()\n");
							}
						}
					}
					else if ((pt0[1] == this->m_min[1] && pt1[1] == this->m_min[1]) || (pt0[1] == this->m_max[1] && pt1[1] == this->m_max[1]))
					{
						// -y / +y
						//
						if (pt0[0] == pt1[0])
						{
							if (this->PlaneWidget)
							{
								TRACE("this->PlaneWidget->NormalToXAxisOn()\n");
							}
						}
						else if (pt0[2] == pt1[2])
						{
							if (this->PlaneWidget)
							{
								TRACE("this->PlaneWidget->NormalToZAxisOn()\n");
							}
						}
					}
					else if ((pt0[0] == this->m_max[0] && pt1[0] == this->m_max[0]) || (pt0[0] == this->m_min[0] && pt1[0] == this->m_min[0]))
					{
						// +x / -x
						//
						if (pt0[1] == pt1[1])
						{
							if (this->PlaneWidget)
							{
								TRACE("this->PlaneWidget->NormalToYAxisOn()\n");
							}
						}
						else if (pt0[2] == pt1[2])
						{
							if (this->PlaneWidget)
							{
								TRACE("this->PlaneWidget->NormalToZAxisOn()\n");
							}
						}
					}
				}
			}
		}
	}

	if (pCellPicker->Pick(X, Y, 0.0, this->CurrentRenderer))
	{
		vtkIdType n = pCellPicker->GetCellId();
		TRACE("CellId = %d\n", n);
		vtkFloatingPointType* pt = pCellPicker->GetPickPosition();
		if (vtkDataSet* pDataSet = pCellPicker->GetDataSet())
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
					vtkFloatingPointType* scale = this->GetScale();

					vtkFloatingPointType length;
					vtkFloatingPointType bounds[6];
					this->GetBounds(bounds);

					if ((pt0[2] == this->m_min[2] && pt1[2] == this->m_min[2]) || (pt0[2] == this->m_max[2] && pt1[2] == this->m_max[2]))
					{
						// +z / -z
						//
						if (pt0[0] == pt1[0])
						{
							if (this->PlaneWidget)
							{
								this->AxisIndex = 0;
								//this->CurrentPoint;
								this->PlaneWidget->SetInteractor(this->Interactor);
								this->PlaneWidget->NormalToXAxisOn();
								length = (bounds[1] - bounds[0]);
								bounds[0] -= length * 4;
								bounds[1] += length * 4;
								this->PlaneWidget->PlaceWidget(bounds);
								this->PlaneWidget->SetOrigin(pt0[0] * scale[0], 0, 0);
								this->PlaneWidget->EnabledOn();
								this->Interactor->Render();
							}
						}
						else if (pt0[1] == pt1[1])
						{
							if (this->PlaneWidget)
							{
								this->AxisIndex = 1;
								this->PlaneWidget->SetInteractor(this->Interactor);
								this->PlaneWidget->NormalToYAxisOn();
								length = (bounds[3] - bounds[2]);
								bounds[2] -= length * 4;
								bounds[3] += length * 4;
								this->PlaneWidget->PlaceWidget(bounds);
								this->PlaneWidget->SetOrigin(0, pt0[1] * scale[1], 0);
								this->PlaneWidget->EnabledOn();
								this->Interactor->Render();
							}
						}
						else
						{
							this->PlaneWidget->SetInteractor(this->Interactor);
							this->PlaneWidget->EnabledOff();
							this->Interactor->Render();
						}
					}
					else if ((pt0[1] == this->m_min[1] && pt1[1] == this->m_min[1]) || (pt0[1] == this->m_max[1] && pt1[1] == this->m_max[1]))
					{
						// -y / +y
						//
						if (pt0[0] == pt1[0])
						{
							if (this->PlaneWidget)
							{
								this->AxisIndex = 0;
								this->PlaneWidget->SetInteractor(this->Interactor);
								this->PlaneWidget->NormalToXAxisOn();
								length = (bounds[1] - bounds[0]);
								bounds[0] -= length * 4;
								bounds[1] += length * 4;
								this->PlaneWidget->PlaceWidget(bounds);
								this->PlaneWidget->SetOrigin(pt0[0] * scale[0], 0, 0);
								this->PlaneWidget->EnabledOn();
								this->Interactor->Render();
							}
						}
						else if (pt0[2] == pt1[2])
						{
							if (this->PlaneWidget)
							{
								this->AxisIndex = 2;
								this->PlaneWidget->SetInteractor(this->Interactor);
								this->PlaneWidget->NormalToZAxisOn();
								length = (bounds[5] - bounds[4]);
								bounds[4] -= length * 4;
								bounds[5] += length * 4;
								this->PlaneWidget->PlaceWidget(bounds);
								this->PlaneWidget->SetOrigin(0, 0, pt0[2] * scale[2]);
								this->PlaneWidget->EnabledOn();
								this->Interactor->Render();
							}
						}
						else
						{
							this->AxisIndex = -1;
							this->PlaneWidget->SetInteractor(this->Interactor);
							this->PlaneWidget->EnabledOff();
							this->Interactor->Render();
						}
					}
					else if ((pt0[0] == this->m_max[0] && pt1[0] == this->m_max[0]) || (pt0[0] == this->m_min[0] && pt1[0] == this->m_min[0]))
					{
						// +x / -x
						//
						if (pt0[1] == pt1[1])
						{
							if (this->PlaneWidget)
							{
								this->AxisIndex = 1;
								this->PlaneWidget->SetInteractor(this->Interactor);
								this->PlaneWidget->NormalToYAxisOn();
								length = (bounds[3] - bounds[2]);
								bounds[2] -= length * 4;
								bounds[3] += length * 4;
								this->PlaneWidget->PlaceWidget(bounds);
								this->PlaneWidget->SetOrigin(0, pt0[1] * scale[1], 0);
								this->PlaneWidget->EnabledOn();
								this->Interactor->Render();
							}
						}
						else if (pt0[2] == pt1[2])
						{
							if (this->PlaneWidget)
							{
								this->AxisIndex = 2;
								this->PlaneWidget->SetInteractor(this->Interactor);
								this->PlaneWidget->NormalToZAxisOn();
								length = (bounds[5] - bounds[4]);
								bounds[4] -= length * 4;
								bounds[5] += length * 4;
								this->PlaneWidget->PlaceWidget(bounds);
								this->PlaneWidget->SetOrigin(0, 0, pt0[2] * scale[2]);
								this->PlaneWidget->EnabledOn();
								this->Interactor->Render();
							}
						}
						else
						{
							this->AxisIndex = -1;
							this->PlaneWidget->SetInteractor(this->Interactor);
							this->PlaneWidget->EnabledOff();
							this->Interactor->Render();
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
		this->Interactor->Render();
	}
	pCellPicker->Delete();
	this->PickableOff();
}

void CGridLODActor::OnLeftButtonUp()
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

void CGridLODActor::OnKeyPress()
{
	if (!this->Interactor) return;

	char* keysym = this->Interactor->GetKeySym();
	TRACE("OnKeyPress %s\n", keysym);

	
	if (this->PlaneWidget->GetEnabled())
	{
		if (::strcmp(keysym, "Delete") == 0)
		{
			this->PlaneIndex = -1;
			CGrid grid = this->m_gridKeyword.m_grid[this->AxisIndex];
			grid.Setup();
			for (int pi = 0; pi < grid.count_coord; ++pi)
			{
				if (grid.coord[pi] == this->CurrentPoint[this->AxisIndex])
				{
					this->PlaneIndex = pi;
					break;
				}
			}
			ASSERT(this->PlaneIndex != -1);
			std::set<double> coordinates;
			coordinates.insert(grid.coord, grid.coord + grid.count_coord);
			coordinates.erase(this->CurrentPoint[this->AxisIndex]);
			if (coordinates.size() >= 2)
			{
				this->m_gridKeyword.m_grid[this->AxisIndex].insert(coordinates.begin(), coordinates.end());
				this->Setup(this->m_units);
				this->Insert(this->m_node);
			}
			else
			{
				::AfxMessageBox("TODO");
			}
			this->PlaneWidget->Off();
			this->State = CGridLODActor::Start;
		}
	}


// COMMENT: {8/2/2005 9:25:12 PM}	if (this->PlaneWidget->GetEnabled())
// COMMENT: {8/2/2005 9:25:12 PM}	{
// COMMENT: {8/2/2005 9:25:12 PM}		if (::strcmp(keysym, "Escape") == 0)
// COMMENT: {8/2/2005 9:25:12 PM}		{
// COMMENT: {8/2/2005 9:25:12 PM}			if (this->PlaneWidget)
// COMMENT: {8/2/2005 9:25:12 PM}			{
// COMMENT: {8/2/2005 9:25:12 PM}				if (this->PlaneWidget->GetEnabled())
// COMMENT: {8/2/2005 9:25:12 PM}				{
// COMMENT: {8/2/2005 9:25:12 PM}					this->PlaneWidget->SetEnabled(0);
// COMMENT: {8/2/2005 9:25:12 PM}				}
// COMMENT: {8/2/2005 9:25:12 PM}				this->PlaneWidget->Delete();
// COMMENT: {8/2/2005 9:25:12 PM}				this->PlaneWidget = 0;
// COMMENT: {8/2/2005 9:25:12 PM}			}
// COMMENT: {8/2/2005 9:25:12 PM}			this->Setup(this->m_units);
// COMMENT: {8/2/2005 9:25:12 PM}
// COMMENT: {8/2/2005 9:25:12 PM}			// this->PlaneWidget->Off();
// COMMENT: {8/2/2005 9:25:12 PM}			this->AxisIndex = -1;
// COMMENT: {8/2/2005 9:25:12 PM}			this->PlaneWidget->SetInteractor(this->Interactor);
// COMMENT: {8/2/2005 9:25:12 PM}			this->PlaneWidget->EnabledOff();
// COMMENT: {8/2/2005 9:25:12 PM}			this->Interactor->Render();
// COMMENT: {8/2/2005 9:25:12 PM}			this->State = CGridLODActor::Start;
// COMMENT: {8/2/2005 9:25:12 PM}		}
// COMMENT: {8/2/2005 9:25:12 PM}	}

// COMMENT: {7/26/2005 7:01:34 PM}	if (this->State == CRiverActor::CreatingRiver)
// COMMENT: {7/26/2005 7:01:34 PM}	{
// COMMENT: {7/26/2005 7:01:34 PM}		char* keysym = this->Interactor->GetKeySym();		
// COMMENT: {7/26/2005 7:01:34 PM}		// if (::strcmp(keysym, "Escape") == 0)
// COMMENT: {7/26/2005 7:01:34 PM}		this->Interactor->Render();
// COMMENT: {7/26/2005 7:01:34 PM}	}
}

#if defined(WIN32)
BOOL CGridLODActor::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (!this->Interactor) return FALSE;

	int X = this->Interactor->GetEventPosition()[0];
	int Y = this->Interactor->GetEventPosition()[1];

	vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
	if ( ren != this->CurrentRenderer )
	{
		return FALSE;
	}

// COMMENT: {7/28/2005 3:26:36 PM}	vtkCellPicker* pCellPicker = vtkCellPicker::New();
// COMMENT: {7/28/2005 3:26:36 PM}	pCellPicker->SetTolerance(0.004);
// COMMENT: {7/28/2005 3:26:36 PM}	pCellPicker->PickFromListOn();
// COMMENT: {7/28/2005 3:26:36 PM}	this->PickableOn();
// COMMENT: {7/28/2005 3:26:36 PM}	pCellPicker->AddPickList(this);
// COMMENT: {7/28/2005 3:26:36 PM}	if (pCellPicker->Pick(X, Y, 0.0, this->CurrentRenderer))
// COMMENT: {7/28/2005 3:26:36 PM}	{
// COMMENT: {7/28/2005 3:26:36 PM}		vtkIdType n = pCellPicker->GetCellId();
// COMMENT: {7/28/2005 3:26:36 PM}		if (vtkDataSet* pDataSet = pCellPicker->GetDataSet())
// COMMENT: {7/28/2005 3:26:36 PM}		{
// COMMENT: {7/28/2005 3:26:36 PM}			vtkCell* pCell = pDataSet->GetCell(n);
// COMMENT: {7/28/2005 3:26:36 PM}			if (pCell->GetCellType() == VTK_LINE)
// COMMENT: {7/28/2005 3:26:36 PM}			{
// COMMENT: {7/28/2005 3:26:36 PM}				ASSERT(pCell->GetNumberOfPoints() == 2);				
// COMMENT: {7/28/2005 3:26:36 PM}				if (vtkPoints* pPoints = pCell->GetPoints())
// COMMENT: {7/28/2005 3:26:36 PM}				{
// COMMENT: {7/28/2005 3:26:36 PM}					float* pt0 = pPoints->GetPoint(0);
// COMMENT: {7/28/2005 3:26:36 PM}					float* pt1 = pPoints->GetPoint(1);
// COMMENT: {7/28/2005 3:26:36 PM}					TRACE("pt0[0] = %g, pt0[1] = %g\n", pt0[0], pt0[1]);
// COMMENT: {7/28/2005 3:26:36 PM}					TRACE("pt1[0] = %g, pt1[1] = %g\n", pt1[0], pt1[1]);
// COMMENT: {7/28/2005 3:26:36 PM}					if (pt0[1] == pt1[1])
// COMMENT: {7/28/2005 3:26:36 PM}					{
// COMMENT: {7/28/2005 3:26:36 PM}						::SetCursor(AfxGetApp()->LoadCursor(IDC_HO_SPLIT));
// COMMENT: {7/28/2005 3:26:36 PM}						pCellPicker->Delete();
// COMMENT: {7/28/2005 3:26:36 PM}						this->PickableOff();
// COMMENT: {7/28/2005 3:26:36 PM}						return TRUE;
// COMMENT: {7/28/2005 3:26:36 PM}					}
// COMMENT: {7/28/2005 3:26:36 PM}					else if (pt0[0] == pt1[0])
// COMMENT: {7/28/2005 3:26:36 PM}					{
// COMMENT: {7/28/2005 3:26:36 PM}						::SetCursor(AfxGetApp()->LoadCursor(IDC_VE_SPLIT));
// COMMENT: {7/28/2005 3:26:36 PM}						pCellPicker->Delete();
// COMMENT: {7/28/2005 3:26:36 PM}						this->PickableOff();
// COMMENT: {7/28/2005 3:26:36 PM}						return TRUE;
// COMMENT: {7/28/2005 3:26:36 PM}					}
// COMMENT: {7/28/2005 3:26:36 PM}				}
// COMMENT: {7/28/2005 3:26:36 PM}			}
// COMMENT: {7/28/2005 3:26:36 PM}		}
// COMMENT: {7/28/2005 3:26:36 PM}	}
// COMMENT: {7/28/2005 3:26:36 PM}	pCellPicker->Delete();
// COMMENT: {7/28/2005 3:26:36 PM}	this->PickableOff();
	return FALSE;
}
#endif