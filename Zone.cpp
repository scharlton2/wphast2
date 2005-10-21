#include "StdAfx.h"
#include "Zone.h"

#include "Global.h"
#include <ostream> // std::ostream

// Note: No header files should follow the following three lines
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/* ---------------------------------------------------------------------- 
 *   Zone
 * ---------------------------------------------------------------------- */
CZone::CZone()
{
	this->zone_defined = UNDEFINED;
}

CZone::~CZone()
{
}

CZone::CZone(const struct zone& src)
{
	this->InternalCopy(src);
}

CZone::CZone(const CZone& src)
{
	this->InternalCopy(src);
}

void CZone::InternalCopy(const struct zone& src)
{
	this->zone_defined = src.zone_defined;
	this->x1           = src.x1;
	this->y1           = src.y1;
	this->z1           = src.z1;
	this->x2           = src.x2;
	this->y2           = src.y2;
	this->z2           = src.z2;
}

void CZone::Serialize(bool bStoring, hid_t loc_id)
{
	static const char szZone[] = "zone";

	herr_t status;
	double xyz[6];

	if (bStoring)
	{
#ifdef _DEBUG
		this->AssertValid();
#endif
		ASSERT(this->zone_defined == TRUE);
		xyz[0] = this->x1;
		xyz[1] = this->y1;
		xyz[2] = this->z1;
		xyz[3] = this->x2;
		xyz[4] = this->y2;
		xyz[5] = this->z2;

		status = CGlobal::HDFSerialize(bStoring, loc_id, szZone, H5T_NATIVE_DOUBLE, 6, xyz);
		ASSERT(status >= 0);

	}
	else
	{
		this->zone_defined = FALSE;
		status = CGlobal::HDFSerialize(bStoring, loc_id, szZone, H5T_NATIVE_DOUBLE, 6, xyz);
		ASSERT(status >= 0);
		if (status >= 0) {
			this->zone_defined = TRUE;
			this->x1 = xyz[0];
			this->y1 = xyz[1];
			this->z1 = xyz[2];
			this->x2 = xyz[3];
			this->y2 = xyz[4];
			this->z2 = xyz[5];
		}
#ifdef _DEBUG
		this->AssertValid();
#endif
	}
}

void CZone::Serialize(CArchive& ar)
{
	static const char szZone[] = "zone";
	static int version = 1;

	CString type;
	int ver;

	if (ar.IsStoring())
	{
		// store type as string
		//
		type = szZone;
		ar << type;

		// store version in case changes need to be made
		ar << version;
	}
	else
	{
		// read type as string
		//
		ar >> type;
		ASSERT(type.Compare(szZone) == 0);

		// read version in case changes need to be made
		ar >> ver;
		ASSERT(ver == version);
	}


	if (ar.IsStoring())
	{
		// zone_defined
        ASSERT(this->zone_defined == TRUE);
		ar << this->zone_defined;

		// x1
		ar << this->x1;

		// y1
		ar << this->y1;

		// z1
		ar << this->z1;

		// x2
		ar << this->x2;

		// y2
		ar << this->y2;

		// z2
		ar << this->z2;
	}
	else
	{
		// zone_defined
        ASSERT(this->zone_defined == TRUE);
		ar >> this->zone_defined;

		// x1
		ar >> this->x1;

		// y1
		ar >> this->y1;

		// z1
		ar >> this->z1;

		// x2
		ar >> this->x2;

		// y2
		ar >> this->y2;

		// z2
		ar >> this->z2;
	}
}


#ifdef _DEBUG
void CZone::AssertValid() const
{
	ASSERT(this);
	ASSERT(this->zone_defined == TRUE);
	ASSERT(this->x1 <= this->x2);
	ASSERT(this->y1 <= this->y2);
	ASSERT(this->z1 <= this->z2);
}

void CZone::Dump(CDumpContext& dc)const
{
	dc << "<CZone>\n";
	switch (zone_defined) {
		case UNDEFINED:
			dc << "\tUNDEFINED\n";
			break;
		case TRUE:
			dc << "(" 
				<< this->x1 << ", " << this->y1 << ", " << this->z1
				<< ")-("
				<< this->x2 << ", " << this->y2 << ", " << this->z2
				<< ")\n";
				break;
		default:
			ASSERT(FALSE);
	}
	dc << "</CZone>\n";
}
#endif

std::ostream& operator<< (std::ostream &os, const CZone &a)
{
	ASSERT(a.zone_defined != UNDEFINED);
	os << "\t" << "-zone"
		<< " " << a.x1
		<< " " << a.y1
		<< " " << a.z1
		<< " " << a.x2
		<< " " << a.y2
		<< " " << a.z2
		<< "\n";
	return os;
}

bool CZone::operator==(const struct zone& rhs)const throw()
{
	if (this->zone_defined == UNDEFINED || rhs.zone_defined == UNDEFINED) return false;
	if (this->zone_defined == FALSE || rhs.zone_defined == FALSE) return false;
	return (
		this->x1 == rhs.x1 &&
		this->x2 == rhs.x2 &&
		this->y1 == rhs.y1 &&
		this->y2 == rhs.y2 &&
		this->z1 == rhs.z1 &&
		this->z2 == rhs.z2
		);
}