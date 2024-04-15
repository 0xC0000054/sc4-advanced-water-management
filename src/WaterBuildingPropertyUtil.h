////////////////////////////////////////////////////////////////////////////
//
// This file is part of sc4-advanced-water-management, a DLL Plugin for
// SimCity 4 that improves interoperability with advanced water management.
//
// Copyright (c) 2024 Nicholas Hayes
//
// This file is licensed under terms of the MIT License.
// See LICENSE.txt for more information.
//
////////////////////////////////////////////////////////////////////////////

#pragma once
#include <cstdint>

class cISC4Occupant;

namespace WaterBuildingPropertyUtil
{
	enum class WaterSource : uint8_t
	{
		FreshWater = 0,
		SaltWater,
		Aquifer
	};

	bool IsWaterProducer(cISC4Occupant* pOccupant, WaterSource& waterSource);

	bool IsNonAquiferWaterSource(cISC4Occupant* pOccupant);
}