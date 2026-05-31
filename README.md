
# MP_TriggerVolumeSystem: UE5 MassEntity Spatial Event System

UE5 MassEntity 기반 대규모 Entity가 레벨 내 Trigger Volume에 진입하거나 이탈할 때, Mass StateTree Event를 발행하기 위한 런타임 상호작용 시스템입니다.

이 샘플은 전체 플러그인 코드가 아니라, TriggerVolume 기반 레벨 이벤트 시스템의 구조와 코딩 스타일을 보여주기 위해 핵심 코드만 정리한 코드 샘플입니다.


## Table of Contents

- [Requirements](#requirements)
- [Overview](#-overview)
- [What This Sample Shows](#-이-샘플로-알-수-있는-역량)
- [C++ Implementation Points](#-c-구현-points)
- [Key Features](#-key-features)
- [Design Details](#-design-details)
  - [UE Lifecycle](#1-ue-lifecycle-고려)
  - [C++ / DataAsset Role Separation](#2-c--dataasset-역할-분리)
  - [Broad Phase / Narrow Phase](#3-broad-phase--narrow-phase-분리)
  - [Spatial Hash Grid Implementation](#4-spatial-hash-grid-구현-방식)
  - [Enter / Exit Transition Handling](#5-enter--exit-변경-시점에만-이벤트-처리)
- [Performance Notes](#-performance-notes)
- [Code Structure](#-code-structure)
- [Limitations](#limitations)
- [Note](#-note)



## Requirements

- Unreal Engine 5.x
- MassEntity
- MassAI / MassGameplay
- StateTree
- GameplayTags

이 샘플은 독립 실행 가능한 전체 플러그인이 아니라, MaaassParticle 플러그인에서 TriggerVolume Interaction 구조를 확인할 수 있도록 정리한 코드 샘플입니다.

---

## 📖 Overview

기존 Actor Overlap Event 기반 구조는 소수의 Actor에는 간단하지만, MassEntity 기반 대규모 Crowd/Agent 환경에서는 적합하지 않습니다.

MassEntity는 일반 Actor처럼 개별 Collision Event를 직접 받는 구조가 아니기 때문에, Entity 위치를 기준으로 Volume 진입/이탈을 직접 판정하고, 그 결과를 StateTree Event로 전달하는 별도의 런타임 시스템이 필요했습니다.

이 샘플에서는 다음 구조로 문제를 해결했습니다.

```text
TriggerVolumeComponent
    ↓ Register / Unregister
TriggerVolumeSubsystem
    ↓ Spatial Hash Grid Query
VolumeTriggerProcessor
    ↓ Enter / Exit Task
RequestEventFragment
    ↓ StateTree Event Flush
Mass StateTree
```

---

## 🔍 역량 요약

이 코드 샘플은 다음 역량을 보여주기 위해 정리했습니다.

* UE5 Component Lifecycle 이해
* UWorldSubsystem 기반 런타임 상태 관리
* Mass Processor 실행 그룹 및 Fragment Query 구성
* Mass StateTree Event Dispatch 구조
* DataAsset 기반 콘텐츠 설정 분리
* Spatial Hash Grid 기반 Broad-phase Query
* Enter / Exit 변경 감지 기반 이벤트 처리
* 런타임 안정성을 위한 유효성 검사와 상태 캐싱

---

## 🔍 C++ / UE 구현 Points

- `CandidateBuffer`를 Processor 멤버로 재사용하여 위치 쿼리마다 발생할 수 있는 임시 배열 할당을 줄였습니다.
- Runtime Volume 등록 시 `GridItemIndex`와 `CachedBounds`를 저장하여 해제 시 Grid에서 정확히 제거되도록 구성했습니다.
- TriggerVolume은 `OnRegister` / `OnUnregister`에서 Subsystem에 등록·해제하여 UE Component Lifecycle에 맞춰 Runtime Grid 상태를 관리합니다.
- Spatial Hash Grid는 후보 Volume을 줄이는 Broad Phase로만 사용하고, 실제 포함 여부는 Component Transform 기준의 Narrow Phase에서 다시 검사합니다.
- Task는 StateTree를 직접 실행하지 않고 Request Fragment에 이벤트를 적재하며, Processor가 Flush 시점에 StateTree Event를 전달합니다.

---

## ✨ Key Features

### 1. Component 기반 레벨 배치

`UMPTriggerVolumeComponent`는 `UBoxComponent`를 상속합니다.

레벨 디자이너는 Actor에 Component를 추가하고, Enter / Exit 이벤트 DataAsset과 Priority만 설정하면 됩니다.

* Enter / Exit Interaction DataAsset
* Priority
* Debug Draw
* PIE Visibility

### 2. WorldSubsystem 기반 중앙 관리

`UMPTriggerVolumeSubsystem`은 Runtime Trigger Volume을 등록하고, Entity별 현재 Volume 상태를 관리합니다.

* Runtime Volume 등록 / 해제
* Spatial Hash Grid 소유
* Entity → CurrentVolume Map 관리
* 위치 기반 Best Volume Query

### 3. Spatial Hash 기반 후보 Volume 탐색

모든 Volume을 매번 순회하지 않고, TriggerVolume Bounds를 Grid Cell에 등록한 뒤 Entity 위치가 속한 Cell만 조회합니다.

```text
Broad Phase:
    Spatial Hash Grid로 후보 Volume 조회

Narrow Phase:
    Entity 위치를 Component Local Space로 변환한 뒤 BoxExtent 기준으로 포함 여부 검사

Resolve:
    여러 Volume이 겹치면 Priority가 가장 높은 Volume 선택
```

### 4. Mass Processor 기반 Enter / Exit 판정

`UMPVolumeTriggerProcessor`는 MassEntity의 TransformFragment를 Chunk 단위로 읽고, 이전 Volume과 현재 Volume을 비교합니다.

Volume이 변경된 경우에만 Enter / Exit Task를 실행합니다.

### 5. Fragment Queue 기반 StateTree Event 전달

Task는 StateTree를 직접 실행하지 않습니다.
대신 `FMPTriggerVolumeRequestEventFragment`에 Event를 적재하고, Processor가 Flush 시점에 StateTree로 전달합니다.

이 구조를 통해 이벤트 생성과 StateTree 전달 시점을 분리했습니다.

---

## 🛠 설계 상세

### 1. UE Lifecycle 고려

TriggerVolume은 `OnRegister` / `OnUnregister`에서 Subsystem에 등록 및 해제됩니다.

```cpp
void UMPTriggerVolumeComponent::OnRegister()
{
    Super::OnRegister();
    UpdateSubsystemRegistration(true);
}

void UMPTriggerVolumeComponent::OnUnregister()
{
    UpdateSubsystemRegistration(false);
    Super::OnUnregister();
}
```

이 구조를 통해 Component가 월드에 등록되는 시점과 제거되는 시점에 맞춰 Spatial Grid 상태를 갱신합니다.

---

### 2. C++ / DataAsset 역할 분리

TriggerVolume 시스템은 런타임 처리와 콘텐츠 설정 영역을 분리했습니다.

C++은 성능과 실행 순서가 중요한 런타임 로직을 담당합니다.

- Volume 등록 / 해제
- Spatial Hash Grid 기반 후보 Volume 조회
- Entity 위치 평가
- Enter / Exit 판정
- StateTree Event Flush

DataAsset은 레벨별 이벤트 내용을 담당합니다.

- Enter Task List
- Exit Task List
- GameplayTag 기반 StateTree Event
- Custom TriggerVolume Task

#### DataAsset 기반 Task 구성

TriggerVolume의 Enter / Exit 동작은 C++ 코드에 직접 하드코딩하지 않고, `MPTriggerVolumeEventData`에 등록된 Task 목록으로 구성합니다.

```cpp
UPROPERTY(EditAnywhere, Instanced, Category = "TriggerVolume", meta = (DisplayName = "TriggerVolume Tasks"))
TArray<TObjectPtr<UMPTriggerVolumeTaskBase>> Tasks;
```

`UMPTriggerVolumeTaskBase`는 TriggerVolume 이벤트에서 실행될 Task의 공통 인터페이스 역할을 합니다.
예를 들어 `UMPRequestEventTask`는 StateTree를 직접 실행하지 않고, Entity의 Request Fragment에 `FStateTreeEvent`를 적재합니다.

이후 `UMPVolumeTriggerProcessor`가 Pending Event를 Flush하면서 Mass StateTree에 이벤트를 전달합니다.

```text
MPTriggerVolumeEventData
    → UMPTriggerVolumeTaskBase
        → UMPRequestEventTask
            → RequestEventFragment
                → VolumeTriggerProcessor Flush
                    → Mass StateTree Event
```

이 구조를 통해 레벨별 TriggerVolume 반응을 C++ 수정 없이 DataAsset 조합으로 구성할 수 있도록 했습니다.

---

### 3. Broad Phase / Narrow Phase 분리

Spatial Hash Grid는 후보 Volume만 빠르게 찾는 Broad Phase로 사용합니다.  
실제 포함 여부는 Entity 위치를 TriggerVolume Component의 Local Space로 변환한 뒤 BoxExtent와 비교하는 Narrow Phase에서 판정합니다.

```cpp
RuntimeVolumeGrid.FindOverlapping(Location, QueryLevel, CandidateBuffer);

for (UMPTriggerVolumeComponent* Candidate : CandidateBuffer)
{
    if (!Candidate)
    {
        continue;
    }

    if (!IsLocationInsideComponentBounds(Candidate, Location))
    {
        continue;
    }

    // Priority Resolve
}
```

---

### 4. Spatial Hash Grid 구현 방식

`MPHierarchicalBoundsHashGrid3D`는 TriggerVolume 후보군을 빠르게 줄이기 위한 Broad Phase 전용 자료구조입니다.
전체 월드 공간을 미리 분할하지 않고, TriggerVolume이 등록된 Cell만 `TMap`에 생성하여 관리합니다.

```cpp
TArray<TMap<FIntVector, FCell>> CellsByLevel;
TSparseArray<FItem> Items;
TArray<FVector::FReal> CellSizes;
```

구현 구조는 다음과 같습니다.

* `CellsByLevel`: Level별 Spatial Hash Cell 저장소
* `FIntVector`: World Location을 Cell Size로 나누어 계산한 Cell 좌표
* `FCell`: 해당 Cell에 포함된 ItemIndex 목록
* `TSparseArray<FItem>`: 등록된 TriggerVolume Item 저장소
* `CellSizes`: Level별 Cell 크기

TriggerVolume이 등록될 때는 Bounds가 차지하는 Cell 범위를 계산하고, 해당 범위의 모든 Cell에 ItemIndex를 추가합니다.

```cpp
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
```

Entity 위치를 조회할 때는 World Location을 Cell 좌표로 변환하고, 해당 Cell에 등록된 후보 Volume만 반환합니다.

```cpp
const FIntVector CellCoords = GetCellCoords(Point, Level);
const FCell* Cell = LevelCells.Find(CellCoords);
```

이때 `FindOverlapping`은 실제 충돌 판정이 아니라 후보군 조회만 담당합니다.
따라서 실제 Volume 내부 포함 여부는 `TriggerVolumeSubsystem`에서 Component Local Space 기준으로 다시 검사합니다.

```text
Spatial Hash Grid
    → 같은 Cell의 후보 Volume 조회

TriggerVolumeSubsystem
    → Component Local Space 기준 Narrow Phase 검사
    → Priority Resolve
```

이 구조를 통해 전체 Volume을 매번 순회하지 않고, Entity 위치 주변의 후보 Volume만 검사하도록 조회 범위를 줄였습니다.


---
### 5. Enter / Exit 변경 시점에만 이벤트 처리

매 프레임 이벤트를 발생시키지 않고, 이전 Volume과 현재 Volume이 달라졌을 때만 Task를 실행합니다.

```cpp
if (PrevVolume.Get() != CurrentVolume)
{
    ExecuteExitTasks();
    ExecuteEnterTasks();
    FlushStateTreeEvents();
    VolumeSubsystem->UpdateEntityCurrentVolume(Entity, CurrentVolume);
}
```
---

## 📈 성능 측정

이 샘플 코드에서는 공간 쿼리 성능 측정용 로그 코드를 제거했습니다.  
코드 샘플의 목적은 런타임 구조와 코딩 스타일을 보여주는 것이므로, 성능 측정 로직은 README에 별도로 정리했습니다.

### 측정 대상

- Entity 위치 기준 TriggerVolume 후보 조회 시간
- PreviousVolume / CurrentVolume 변경 감지 후 Event Dispatch까지의 흐름
- Spatial Hash Grid 적용 전후의 Volume Query 비용

### 측정 방식

개발 중에는 `FPlatformTime::Cycles64()`와 `FPlatformTime::Seconds()`를 사용해 Grid Query와 Event Queue → Flush 구간을 측정했습니다.  
다만 제출용 코드에서는 핵심 로직 가독성을 위해 측정 코드를 제거했습니다.

### 개선 방향

| 항목 | 기존 구조 | 개선 구조 |
|---|---|---|
| Volume 탐색 | 전체 Volume 순회 | Spatial Hash Grid 기반 후보 조회 |
| Event 처리 | 매 프레임 조건 검사 후 직접 처리 | Volume 변경 시점에만 Enter / Exit 처리 |
| StateTree 전달 | Task와 StateTree 전달 흐름 혼재 | Request Fragment에 적재 후 Processor에서 Flush |
| 임시 버퍼 | Query마다 임시 배열 생성 가능 | CandidateBuffer 재사용 |

### 참고

실제 성능 수치는 테스트 환경, Entity 수, Volume 수, Cell 크기에 따라 달라질 수 있으므로, 이 샘플에는 구조적 개선 방향만 포함했습니다.


### 내부 테스트 결과

내부 테스트 기준, TriggerVolume 이벤트 반응 지연은 평균 720ms에서 0.21ms로 감소했습니다.  
이 수치는 테스트 씬의 Entity 수, Volume 수, Cell 크기에 의존하므로, 샘플 코드에서는 측정 방식과 구조 개선 방향을 중심으로 설명합니다.

---

## 📂 코드 구조

```text
Source/Public/
├─ MPHierarchicalBoundsHashGrid3D.h      // Spatial Hash Grid
├─ MPTriggerVolumeComponent.h           // TriggerVolume 컴포넌트
├─ MPTriggerVolumeSubsystem.h           // Volume 등록/조회 관리
├─ MPVolumeTriggerProcessor.h           // Enter/Exit 판정 Processor
├─ MPTriggerVolumeTaskBase.h            // 이벤트 Task 베이스
├─ MPRequestEventTask.h                 // StateTree Event Task
└─ MPTriggerVolumeEventData.h           // Enter/Exit Task DataAsset

Source/Private/
├─ MPTriggerVolumeComponent.cpp         // Component 등록/해제 처리
├─ MPTriggerVolumeSubsystem.cpp         // Volume 조회/상태 관리 구현
├─ MPVolumeTriggerProcessor.cpp         // Enter/Exit 판정 및 Event Flush
└─ MPRequestEventTask.cpp               // Pending Event 적재
```

---

## Limitations

- 이 샘플은 정적인 Runtime Trigger Volume을 기준으로 작성했습니다.
- Runtime 중 TriggerVolume이 이동하거나 크기가 변경되는 경우, CachedBounds를 갱신하고 Grid Cell을 다시 등록하는 처리가 필요합니다.
- 전체 플러그인 빌드에 필요한 일부 Fragment / TaskBase / DataAsset 정의는 샘플 구조 설명을 위해 최소화했습니다.

---

## ⚠️ Note

이 샘플은 실제 플러그인 구현 중 TriggerVolume Interaction 흐름을 리뷰하기 쉽도록 정리한 코드입니다. 전체 플러그인 빌드보다는 시스템 설계와 코딩 스타일을 보여주는 것을 목적으로 합니다.
