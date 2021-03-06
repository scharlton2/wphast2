#include "StdAfx.h"
#include "vtkCellPicker2.h"

#include <vtkLODProp3D.h>
#include <vtkProp3DCollection.h>
#include <vtkVolumeMapper.h>
#include "vtkBox.h"


vtkCxxRevisionMacro(vtkCellPicker2, "$Revision$");
vtkStandardNewMacro(vtkCellPicker2);


vtkCellPicker2::vtkCellPicker2(void)
{
}

vtkCellPicker2::~vtkCellPicker2(void)
{
}

// mod'd from vtkPicker::Pick
//   prop3D don't need to be visible to be picked (nor have an opacity > 0)
//   see code marked by /***SRC SRC***/
int vtkCellPicker2::Pick(double selectionX, double selectionY, double selectionZ,
                    vtkRenderer *renderer)
{
  int i;
  vtkProp *prop;
  vtkCamera *camera;
  vtkAbstractMapper3D *mapper = NULL;
  double p1World[4], p2World[4], p1Mapper[4], p2Mapper[4];
  int picked=0;
  int *winSize;
  double x, y, t;
  double *viewport;
  double cameraPos[4], cameraFP[4];
  double *displayCoords, *worldCoords;
  double *clipRange;
  double ray[3], rayLength;
  int pickable;
  int LODId;
  double windowLowerLeft[4], windowUpperRight[4];
  double bounds[6], tol;
  double tF, tB;
  double hitPosition[3];
  double cameraDOP[3];
  
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;
  this->SelectionPoint[0] = selectionX;
  this->SelectionPoint[1] = selectionY;
  this->SelectionPoint[2] = selectionZ;

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent,NULL);

  if ( renderer == NULL )
    {
    vtkErrorMacro(<<"Must specify renderer!");
    return 0;
    }

  // Get camera focal point and position. Convert to display (screen) 
  // coordinates. We need a depth value for z-buffer.
  //
  camera = renderer->GetActiveCamera();
  camera->GetPosition(cameraPos);
  cameraPos[3] = 1.0;
  camera->GetFocalPoint(cameraFP);
  cameraFP[3] = 1.0;

  renderer->SetWorldPoint(cameraFP[0],cameraFP[1],cameraFP[2],cameraFP[3]);
  renderer->WorldToDisplay();
  displayCoords = renderer->GetDisplayPoint();
  selectionZ = displayCoords[2];

  // Convert the selection point into world coordinates.
  //
  renderer->SetDisplayPoint(selectionX, selectionY, selectionZ);
  renderer->DisplayToWorld();
  worldCoords = renderer->GetWorldPoint();
  if ( worldCoords[3] == 0.0 )
    {
    vtkErrorMacro(<<"Bad homogeneous coordinates");
    return 0;
    }
  for (i=0; i < 3; i++) 
    {
    this->PickPosition[i] = worldCoords[i] / worldCoords[3];
    }
  
  //  Compute the ray endpoints.  The ray is along the line running from
  //  the camera position to the selection point, starting where this line
  //  intersects the front clipping plane, and terminating where this
  //  line intersects the back clipping plane.
  for (i=0; i<3; i++)
    {
    ray[i] = this->PickPosition[i] - cameraPos[i];
    }
  for (i=0; i<3; i++)
    {
    cameraDOP[i] = cameraFP[i] - cameraPos[i];
    }

  vtkMath::Normalize(cameraDOP);

  if (( rayLength = vtkMath::Dot(cameraDOP,ray)) == 0.0 ) 
    {
    vtkWarningMacro("Cannot process points");
    return 0;
    } 

  clipRange = camera->GetClippingRange();

  if ( camera->GetParallelProjection() )
    {
    tF = clipRange[0] - rayLength;
    tB = clipRange[1] - rayLength;
    for (i=0; i<3; i++) 
      {
      p1World[i] = this->PickPosition[i] + tF*cameraDOP[i];
      p2World[i] = this->PickPosition[i] + tB*cameraDOP[i];
      }
    }
  else
    {
    tF = clipRange[0] / rayLength;
    tB = clipRange[1] / rayLength;
    for (i=0; i<3; i++) 
      {
      p1World[i] = cameraPos[i] + tF*ray[i];
      p2World[i] = cameraPos[i] + tB*ray[i];
      }
    }
  p1World[3] = p2World[3] = 1.0;

  // Compute the tolerance in world coordinates.  Do this by
  // determining the world coordinates of the diagonal points of the
  // window, computing the width of the window in world coordinates, and 
  // multiplying by the tolerance.
  //
  viewport = renderer->GetViewport();
  winSize = renderer->GetRenderWindow()->GetSize();
  x = winSize[0] * viewport[0];
  y = winSize[1] * viewport[1];
  renderer->SetDisplayPoint(x, y, selectionZ);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(windowLowerLeft);

  x = winSize[0] * viewport[2];
  y = winSize[1] * viewport[3];
  renderer->SetDisplayPoint(x, y, selectionZ);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(windowUpperRight);

  for (tol=0.0,i=0; i<3; i++) 
    {
    tol += (windowUpperRight[i] - windowLowerLeft[i]) *
      (windowUpperRight[i] - windowLowerLeft[i]);
    }
  
  tol = sqrt (tol) * this->Tolerance;

  //  Loop over all props.  Transform ray (defined from position of
  //  camera to selection point) into coordinates of mapper (not
  //  transformed to actors coordinates!  Reduces overall computation!!!).
  //  Note that only vtkProp3D's can be picked by vtkPicker.
  //
  vtkPropCollection *props;
  vtkProp *propCandidate;
  if ( this->PickFromList ) 
    {
    props = this->GetPickList();
    }
  else 
    {
    props = renderer->GetViewProps();
    }

  vtkActor *actor;
  vtkLODProp3D *prop3D;
  vtkVolume *volume;
  vtkImageActor *imageActor = 0;
  vtkAssemblyPath *path;
  vtkProperty *tempProperty;
  this->Transform->PostMultiply();
  vtkCollectionSimpleIterator pit;
  double scale[3];
  for ( props->InitTraversal(pit); (prop=props->GetNextProp(pit)); )
    {
    for ( prop->InitPathTraversal(); (path=prop->GetNextPath()); )
      {
      pickable = 0;
      actor = NULL;
      propCandidate = path->GetLastNode()->GetViewProp();
      if ( propCandidate->GetPickable() /***SRC && propCandidate->GetVisibility() SRC***/ )
        {
        pickable = 1;
        if ( (actor=vtkActor::SafeDownCast(propCandidate)) != NULL )
          {
          mapper = actor->GetMapper();
/***SRC
          if ( actor->GetProperty()->GetOpacity() <= 0.0 )
            {
            pickable = 0;
            }
SRC***/
          }
        else if ( (prop3D=vtkLODProp3D::SafeDownCast(propCandidate)) != NULL )
          {
          LODId = prop3D->GetPickLODID();
          mapper = prop3D->GetLODMapper(LODId);

          // if the mapper is a vtkMapper (as opposed to a vtkVolumeMapper), 
          // then check the transparency to see if the object is pickable
          if ( vtkMapper::SafeDownCast(mapper) != NULL)
            {
            prop3D->GetLODProperty(LODId, &tempProperty);
/***SRC
            if ( tempProperty->GetOpacity() <= 0.0 )
              {
              pickable = 0;
              }
SRC***/
            }
          }
        else if ( (volume=vtkVolume::SafeDownCast(propCandidate)) != NULL )
          {
          mapper = volume->GetMapper();
          }
        else if ( (imageActor=vtkImageActor::SafeDownCast(propCandidate)) )
          {
          mapper = 0;
          }
        else 
          {
          pickable = 0; //only vtkProp3D's (actors and volumes) can be picked
          }
        }

      //  If actor can be picked, get its composite matrix, invert it, and
      //  use the inverted matrix to transform the ray points into mapper
      //  coordinates. 
      if ( pickable  &&  mapper != NULL )
        {
        vtkMatrix4x4 *lastMatrix = path->GetLastNode()->GetMatrix();
        if (lastMatrix == NULL)
          {
          vtkErrorMacro (<< "Pick: Null matrix.");
          return 0;
          }
        this->Transform->SetMatrix(lastMatrix);
        this->Transform->Push();
        this->Transform->Inverse();
        this->Transform->GetScale(scale); //need to scale the tolerance

        this->Transform->TransformPoint(p1World,p1Mapper);
        this->Transform->TransformPoint(p2World,p2Mapper);

        for (i=0; i<3; i++) 
          {
          ray[i] = p2Mapper[i] - p1Mapper[i];
          }

        this->Transform->Pop();

        //  Have the ray endpoints in mapper space, now need to compare this
        //  with the mapper bounds to see whether intersection is possible.
        //
        //  Get the bounding box of the modeller.  Note that the tolerance is
        //  added to the bounding box to make sure things on the edge of the
        //  bounding box are picked correctly.
        mapper->GetBounds(bounds);
        bounds[0] -= tol; bounds[1] += tol; 
        bounds[2] -= tol; bounds[3] += tol; 
        bounds[4] -= tol; bounds[5] += tol; 
        if ( vtkBox::IntersectBox(bounds, p1Mapper, 
                                  ray, hitPosition, t) )
          {
          t = this->IntersectWithLine(p1Mapper, p2Mapper, 
                                      tol*0.333*(scale[0]+scale[1]+scale[2]), path, 
                                      static_cast<vtkProp3D *>(propCandidate), mapper);
          if ( t < VTK_DOUBLE_MAX )
            {
            picked = 1;
            if ( ! this->Prop3Ds->IsItemPresent(prop) )
              {
              this->Prop3Ds->AddItem(static_cast<vtkProp3D *>(prop));
              }
            this->PickedPositions->InsertNextPoint
              ((1.0 - t)*p1World[0] + t*p2World[0],
               (1.0 - t)*p1World[1] + t*p2World[1],
               (1.0 - t)*p1World[2] + t*p2World[2]);

            // backwards compatibility: also add to this->Actors
            if (actor)
              {
              this->Actors->AddItem(actor);
              }
            }
          }
        }
      else if ( pickable && imageActor )
        { // special case for imageActor, which has no mapper
        vtkMatrix4x4 *lastMatrix = path->GetLastNode()->GetMatrix();
        if (lastMatrix == NULL)
          {
          vtkErrorMacro (<< "Pick: Null matrix.");
          return 0;
          }
        this->Transform->SetMatrix(lastMatrix);
        this->Transform->Push();
        this->Transform->Inverse();

        this->Transform->TransformPoint(p1World,p1Mapper);
        this->Transform->TransformPoint(p2World,p2Mapper);

        this->Transform->Pop();

        //  Have the ray endpoints in data space, now need to compare this
        //  with the displayed image bounds.
        imageActor->GetDisplayBounds(bounds);

        t = VTK_DOUBLE_MAX;

        for (i = 0; i < 3; i++)
          {
          if (bounds[2*i] == bounds[2*i+1] && p2Mapper[i] != p1Mapper[i])
            {
            t = (p2World[i] - bounds[2*i])/(p2Mapper[i] - p1Mapper[i]);
            break;
            }
          }

        if (t < VTK_DOUBLE_MAX)
          {
          hitPosition[0] = (1.0 - t)*p1Mapper[0] + t*p2Mapper[0];
          hitPosition[1] = (1.0 - t)*p1Mapper[1] + t*p2Mapper[1];
          hitPosition[2] = (1.0 - t)*p1Mapper[2] + t*p2Mapper[2];
          if ((bounds[0] == bounds[1] || (hitPosition[0] >= bounds[0]-tol &&
                                          hitPosition[0] <= bounds[1]+tol)) &&
              (bounds[2] == bounds[3] || (hitPosition[1] >= bounds[2]-tol &&
                                          hitPosition[1] <= bounds[3]+tol)) &&
              (bounds[4] == bounds[5] || (hitPosition[2] >= bounds[4]-tol &&
                                          hitPosition[2] <= bounds[5]+tol)))
            {
            picked = 1;

            // the following code is handled in MarkPicked for other Prop3Ds
            this->Mapper = mapper; // mapper is null
            this->DataSet = imageActor->GetInput();
            this->MapperPosition[0] = hitPosition[0];
            this->MapperPosition[1] = hitPosition[1];
            this->MapperPosition[2] = hitPosition[2];
            this->Transform->TransformPoint(hitPosition,this->PickPosition);
            imageActor->Pick();
            this->InvokeEvent(vtkCommand::PickEvent,NULL);

            this->Prop3Ds->AddItem(imageActor);
            this->PickedPositions->InsertNextPoint
              ((1.0 - t)*p1World[0] + t*p2World[0],
               (1.0 - t)*p1World[1] + t*p2World[1],
               (1.0 - t)*p1World[2] + t*p2World[2]);
            }
          }
        }//if visible and pickable not transparent and has mapper
      }//for all parts
    }//for all actors

  // Invoke end pick method if defined
  this->InvokeEvent(vtkCommand::EndPickEvent,NULL);

  return picked;
}
