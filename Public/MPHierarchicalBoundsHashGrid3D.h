#pragma once

#include "CoreMinimal.h"

/**
 * TriggerVolume 후보군을 빠르게 줄이기 위한 3D Spatial Hash Grid입니다.
 *
 * 이 클래스는 Broad Phase 전용 구조입니다.
 * 실제 Volume 내부 포함 여부는 호출자가 별도의 Narrow Phase에서 검사합니다.
 */
template<typename ItemType>
class TMPHierarchicalBoundsHashGrid3D
{
public:
    struct FCell
    {
        TArray<int32> ItemIndices;
    };

    struct FItem
    {
        ItemType ID;

        FItem() = default;
        explicit FItem(const ItemType& InID) : ID(InID) {}
    };

    TMPHierarchicalBoundsHashGrid3D() = default;

    void Init(int32 InNumLevels, const TArray<FVector::FReal>& InCellSizes);
    void Reset();

    int32 Add(const ItemType& Item, const FBox& Bounds, int32 Level = 0);
    void Remove(int32 ItemIndex, const FBox& Bounds, int32 Level = 0);

    void FindOverlapping(const FVector& Point, int32 Level, TArray<ItemType>& OutOverlappingItems) const;

    int32 GetNumLevels() const { return CellSizes.Num(); }
    bool IsValidLevel(int32 Level) const { return CellsByLevel.IsValidIndex(Level) && CellSizes.IsValidIndex(Level); }

    FVector::FReal GetCellSize(int32 Level) const
    {
        check(IsValidLevel(Level));
        return CellSizes[Level];
    }

private:
    struct FCellBox
    {
        FIntVector Min = FIntVector::ZeroValue;
        FIntVector Max = FIntVector::ZeroValue;
    };

private:
    FCellBox CalcQueryBounds(const FBox& QueryBox, int32 Level) const;
    FIntVector GetCellCoords(const FVector& Location, int32 Level) const;

    void AddItemToCell(int32 ItemIndex, const FIntVector& CellCoords, int32 Level);
    void RemoveItemFromCell(int32 ItemIndex, const FIntVector& CellCoords, int32 Level);

private:
    TArray<TMap<FIntVector, FCell>> CellsByLevel;
    TSparseArray<FItem> Items;
    TArray<FVector::FReal> CellSizes;
};

template<typename ItemType>
void TMPHierarchicalBoundsHashGrid3D<ItemType>::Init(int32 InNumLevels, const TArray<FVector::FReal>& InCellSizes)
{
    Reset();

    if (InNumLevels <= 0 || InCellSizes.Num() < InNumLevels)
    {
        return;
    }

    CellSizes.Reserve(InNumLevels);

    for (int32 Level = 0; Level < InNumLevels; ++Level)
    {
        if (InCellSizes[Level] <= 0)
        {
            Reset();
            return;
        }

        CellSizes.Add(InCellSizes[Level]);
    }

    CellsByLevel.SetNum(InNumLevels);
}

template<typename ItemType>
void TMPHierarchicalBoundsHashGrid3D<ItemType>::Reset()
{
    CellsByLevel.Reset();
    Items.Reset();
    CellSizes.Reset();
}

template<typename ItemType>
int32 TMPHierarchicalBoundsHashGrid3D<ItemType>::Add(const ItemType& Item, const FBox& Bounds, int32 Level)
{
    if (!IsValidLevel(Level) || !Bounds.IsValid)
    {
        return INDEX_NONE;
    }

    const int32 ItemIndex = Items.Emplace(Item);
    const FCellBox CellBox = CalcQueryBounds(Bounds, Level);

    for (int32 Z = CellBox.Min.Z; Z <= CellBox.Max.Z; ++Z)
    {
        for (int32 Y = CellBox.Min.Y; Y <= CellBox.Max.Y; ++Y)
        {
            for (int32 X = CellBox.Min.X; X <= CellBox.Max.X; ++X)
            {
                AddItemToCell(ItemIndex, FIntVector(X, Y, Z), Level);
            }
        }
    }

    return ItemIndex;
}

template<typename ItemType>
void TMPHierarchicalBoundsHashGrid3D<ItemType>::Remove(int32 ItemIndex, const FBox& Bounds, int32 Level)
{
    if (!IsValidLevel(Level) || !Bounds.IsValid || !Items.IsAllocated(ItemIndex))
    {
        return;
    }

    const FCellBox CellBox = CalcQueryBounds(Bounds, Level);

    for (int32 Z = CellBox.Min.Z; Z <= CellBox.Max.Z; ++Z)
    {
        for (int32 Y = CellBox.Min.Y; Y <= CellBox.Max.Y; ++Y)
        {
            for (int32 X = CellBox.Min.X; X <= CellBox.Max.X; ++X)
            {
                RemoveItemFromCell(ItemIndex, FIntVector(X, Y, Z), Level);
            }
        }
    }

    Items.RemoveAt(ItemIndex);
}

template<typename ItemType>
void TMPHierarchicalBoundsHashGrid3D<ItemType>::FindOverlapping(
    const FVector& Point,
    int32 Level,
    TArray<ItemType>& OutOverlappingItems) const
{
    OutOverlappingItems.Reset();

    if (!IsValidLevel(Level))
    {
        return;
    }

    const FIntVector CellCoords = GetCellCoords(Point, Level);
    const TMap<FIntVector, FCell>& LevelCells = CellsByLevel[Level];

    const FCell* Cell = LevelCells.Find(CellCoords);
    if (!Cell)
    {
        return;
    }

    for (const int32 ItemIndex : Cell->ItemIndices)
    {
        if (Items.IsAllocated(ItemIndex))
        {
            OutOverlappingItems.Add(Items[ItemIndex].ID);
        }
    }
}

template<typename ItemType>
typename TMPHierarchicalBoundsHashGrid3D<ItemType>::FCellBox TMPHierarchicalBoundsHashGrid3D<ItemType>::CalcQueryBounds(
    const FBox& QueryBox,
    int32 Level) const
{
    check(IsValidLevel(Level));

    const FVector::FReal CellSize = CellSizes[Level];
    const FVector::FReal Epsilon = static_cast<FVector::FReal>(KINDA_SMALL_NUMBER);

    auto CalcMaxCoord = [CellSize, Epsilon](FVector::FReal MinValue, FVector::FReal MaxValue)
    {
        if (FMath::IsNearlyEqual(MinValue, MaxValue))
        {
            return FMath::FloorToInt(MaxValue / CellSize);
        }

        return FMath::FloorToInt((MaxValue - Epsilon) / CellSize);
    };

    FCellBox Result;
    Result.Min = GetCellCoords(QueryBox.Min, Level);
    Result.Max = FIntVector(
        CalcMaxCoord(QueryBox.Min.X, QueryBox.Max.X),
        CalcMaxCoord(QueryBox.Min.Y, QueryBox.Max.Y),
        CalcMaxCoord(QueryBox.Min.Z, QueryBox.Max.Z)
    );

    return Result;
}

template<typename ItemType>
FIntVector TMPHierarchicalBoundsHashGrid3D<ItemType>::GetCellCoords(const FVector& Location, int32 Level) const
{
    check(IsValidLevel(Level));

    const FVector::FReal CellSize = CellSizes[Level];

    return FIntVector(
        FMath::FloorToInt(Location.X / CellSize),
        FMath::FloorToInt(Location.Y / CellSize),
        FMath::FloorToInt(Location.Z / CellSize)
    );
}

template<typename ItemType>
void TMPHierarchicalBoundsHashGrid3D<ItemType>::AddItemToCell(int32 ItemIndex, const FIntVector& CellCoords, int32 Level)
{
    FCell& Cell = CellsByLevel[Level].FindOrAdd(CellCoords);
    Cell.ItemIndices.Add(ItemIndex);
}

template<typename ItemType>
void TMPHierarchicalBoundsHashGrid3D<ItemType>::RemoveItemFromCell(int32 ItemIndex, const FIntVector& CellCoords, int32 Level)
{
    FCell* Cell = CellsByLevel[Level].Find(CellCoords);
    if (!Cell)
    {
        return;
    }

    Cell->ItemIndices.RemoveSingleSwap(ItemIndex, EAllowShrinking::No);

    if (Cell->ItemIndices.IsEmpty())
    {
        CellsByLevel[Level].Remove(CellCoords);
    }
}
