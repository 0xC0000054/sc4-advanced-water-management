////////////////////////////////////////////////////////////////////////////
//
// This file is part of sc4-advanced-water-management, a DLL Plugin for
// SimCity 4 that improves interoperability with advanced water management.
//
// Copyright (c) 2024, 2025 Nicholas Hayes
//
// This file is licensed under terms of the MIT License.
// See LICENSE.txt for more information.
//
////////////////////////////////////////////////////////////////////////////

#include "WaterBuildingOccupantFilter.h"
#include "cISC4Occupant.h"

static const uint32_t GZIID_WaterBuildingOccupantFilter = 0x8E0E791E;

static const uint32_t kOccupantType_Building = 0x278128A0;

static const uint32_t kOccupantGroup_Building_Water = 0x1401;

WaterBuildingOccupantFilter::WaterBuildingOccupantFilter()
	: refCount(0)
{
}

bool WaterBuildingOccupantFilter::QueryInterface(uint32_t riid, void** ppvObj)
{
	if (riid == GZIID_WaterBuildingOccupantFilter)
	{
		*ppvObj = this;
		AddRef();

		return true;
	}
	else if (riid == GZIID_cISC4OccupantFilter)
	{
		*ppvObj = static_cast<cISC4OccupantFilter*>(this);
		AddRef();

		return true;
	}
	else if (riid == GZIID_cIGZUnknown)
	{
		*ppvObj = static_cast<cIGZUnknown*>(this);
		AddRef();

		return true;
	}

	return false;
}

uint32_t WaterBuildingOccupantFilter::AddRef()
{
	return ++refCount;
}

uint32_t WaterBuildingOccupantFilter::Release()
{
	if (refCount > 0)
	{
		--refCount;
	}

	return refCount;
}

bool WaterBuildingOccupantFilter::IsOccupantIncluded(cISC4Occupant* pOccupant)
{
	uint32_t dwType = pOccupant->GetType();

	return IsOccupantTypeIncluded(dwType)
		&& pOccupant->IsOccupantGroup(kOccupantGroup_Building_Water);
}

bool WaterBuildingOccupantFilter::IsOccupantTypeIncluded(uint32_t dwType)
{
	return dwType == kOccupantType_Building;
}

bool WaterBuildingOccupantFilter::IsPropertyHolderIncluded(cISCPropertyHolder* pProperties)
{
	return true;
}
