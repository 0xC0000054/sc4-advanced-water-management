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

#pragma once
#include "cISC4OccupantFilter.h"

class WaterBuildingOccupantFilter final : public cISC4OccupantFilter
{
public:

	WaterBuildingOccupantFilter();

	bool QueryInterface(uint32_t riid, void** ppvObj) override;

	uint32_t AddRef() override;

	uint32_t Release() override;

	bool IsOccupantIncluded(cISC4Occupant* pOccupant) override;

	bool IsOccupantTypeIncluded(uint32_t dwType) override;

	bool IsPropertyHolderIncluded(cISCPropertyHolder* pProperties) override;

private:

	uint32_t refCount;
};

