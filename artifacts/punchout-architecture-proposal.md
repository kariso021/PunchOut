# PunchOut 권장 구조 — 소유권과 호출 경계

> 초기 검토안이다. GAS와 Lyra 모티프를 채택한 현재 방향은 [피처 플러그인·GAS 설계](C:/ProjectsOfGame/PunchOut/artifacts/punchout-lyra-gas-design.md)를 따른다. 아래 단일 모듈/자체 Combat 상태 관리 권고는 현재 권장안이 아니다.

2026-11-01. 이전 정적 분석을 바탕으로 구체화한 설계 제안이다. 게임 코드는 변경하지 않았다. 현재 3인칭 멀티플레이와 플레이어·적의 공통 전투를 전제로 한다. 사망 후 리스폰, 무기 종류, 마법과 보스 채택은 결정하지 않는다.

## 선택: 얇은 공통 Character + 기능 컴포넌트 + 공격 데이터

하나의 PunchOut 모듈 안에서 기능별 폴더로 나눈다. 플레이어와 적은 공통 Character의 Health/Combat 조합을 사용하고, 공격을 결정하는 입력과 AI만 분리한다.

MainPlayer/EnemyBase 양쪽에는 현재 체력 인터페이스 전달과 전투 컴포넌트 연결이 반복되어 있다. 이 실제 공통점을 묶기 위해 공통 Character 한 단계는 유용하다. 반면 검/주먹/원거리별 Character 상속을 늘리면 장비 변경과 적 종류가 계층에 섞이므로 피한다.

```text
ACharacter
└─ APunchOutCharacter                 공통 컴포넌트 생성, 피격/사망 연결
   ├─ APunchOutPlayerCharacter        입력, 카메라, 플레이어 이동 요청
   └─ APunchOutEnemyCharacter         적 메시/애니메이션 및 AI 구성

각 Character
├─ UPunchOutHealthComponent           현재/최대 체력, 피해/회복 적용
└─ UPunchOutCombatComponent           공격·방어·회피 행동과 실행 수명
   └─ UPunchOutAttackDefinition       읽기 전용 공격 설정 참조
```

공통 Character에는 콤보 분기, 장비 종류별 공격, HUD 생성, 타겟 탐색을 넣지 않는다. 공통 연결 이외의 기능이 계속 들어오기 시작하면 분리가 잘못된 신호다. 기존 Blueprint 부모를 변경할 때는 메시·이동·입력 설정과 상속 컴포넌트 중복을 확인한다.

## 권장 폴더

실제 루트는 `C:/ProjectsOfGame/PunchOut/Source/PunchOut`이다. 아래는 목표안이며 지금 생성된 코드가 아니다. .h/.cpp는 같은 기능 폴더에 둔다.

```text
Source/PunchOut/
  PunchOut.Build.cs
  PunchOut.h / PunchOut.cpp
  Game/
    PunchOutGameMode.h/.cpp
    PunchOutGameState.h/.cpp
    PunchOutGameInstance.h/.cpp
  Characters/
    PunchOutCharacter.h/.cpp
    Player/
      PunchOutPlayerCharacter.h/.cpp
      PunchOutPlayerController.h/.cpp
    Enemies/
      PunchOutEnemyCharacter.h/.cpp
  Combat/
    PunchOutCombatTypes.h
    PunchOutDamageable.h
    PunchOutHealthComponent.h/.cpp
    PunchOutCombatComponent.h/.cpp
    Data/
      PunchOutAttackDefinition.h/.cpp
    Animation/
      PunchOutAnimNotify_Hit.h/.cpp
  AI/
    PunchOutAIController.h/.cpp
    Tasks/
      PunchOutBTTask_Attack.h/.cpp
  UI/
    ViewModels/
      PunchOutViewModel.h
    Widgets/
      PunchOutUIView.h/.cpp
  Tests/
    PunchOutHealthTests.cpp
    PunchOutCombatTests.cpp
```

앞선 초안에서 Player/AI에 나뉘어 있던 캐릭터 구성 코드를 Characters 아래로 모았다. AI에는 판단과 행동 선택 코드만 둔다.

`Combat`은 플레이어·적이 함께 쓰는 기능, `Characters`는 기능을 붙여 실제 몸을 구성하는 곳, `AI`는 행동을 선택하는 곳이다. `Components/`, `Interfaces/`, `Data/`, `Enums/`를 최상위에 모으지 않는다. 어떤 게임 기능의 코드인지 먼저 찾을 수 있게 한다. Public/Private와 별도 모듈은 모듈 간 API 경계가 생길 때 적용한다.

## 데이터마다 한 명의 소유자

- **HealthComponent:** 현재 체력·최대 체력. 초기에는 IsDead를 Health <= 0에서 계산한다. 살아 있음→사망 전이에서 한 번 이벤트를 낸다. 부활/다운 상태가 추가되면 그때 명시적인 생명 상태를 설계한다.
- **CombatComponent:** 현재 행동, 실행 번호, 공격 Definition, 명중 기록, 쿨다운, 회피/방어 판정 창. 초기 행동은 Idle/Attacking/Guarding/Dodging/Stunned처럼 서로 배타적으로 둔다. 사망 여부를 별도 bool로 복제하지 않고 Health를 기준으로 행동을 거절/취소한다.
- **Character:** 몸의 구성·팀 식별자·피격 연결. 이동 속도/가속도는 CharacterMovement가 가진 값을 사용하며 불필요한 사본을 두지 않는다.
- **AIController/Blackboard:** 추적 대상·관심 위치·AI 의도. Blackboard의 공격 의도와 Combat의 실제 공격 실행 상태를 구분한다. 공격 중인지 자체 bool로 다시 관리하지 않는다.
- **PlayerController:** 로컬 메뉴·입력 모드·현재 조종 Pawn 연결. Pawn이 바뀌면 UI의 구독 대상을 바꾼다.
- **GameMode:** 서버의 매치 시작·종료·승패 규칙. 탈출 규칙을 넣는다면 여기가 판정한다.
- **GameState:** 클라이언트가 알아야 할 매치 상태. 남은 시간·목표 진행처럼 매치 전체 정보가 생기면 여기에 둔다.
- **GameInstance:** 맵 전환을 넘어 유지할 연결 흐름·선호 설정. 런타임 체력이나 전투 타겟을 저장하지 않는다. 네트워크 공유 상태는 GameState 등 복제 객체가 담당한다.
- **PlayerState:** 플레이어별 점수·준비 여부처럼 다른 플레이어에게 보여줄 정보가 생길 때 커스텀 클래스를 추가한다. 현재 요구만으로 빈 클래스를 만들지는 않는다. 맵 전환 시 보존 정책은 별도로 구현/검증한다.

GameMode/GameState/GameInstance 구분은 [Epic Gameplay Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine)와 [Game Mode and Game State](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-mode-and-game-state-in-unreal-engine)에 따른다.

## 호출 경계: 명령은 직접, 결과는 이벤트

**직접 호출:** 공격 시작·취소·방어 시작처럼 즉시 성공/실패가 필요한 요청. 작은 프로젝트에 전역 EventBus나 ServiceLocator를 만들지 않는다.

**이벤트:** 체력 변경·사망·공격 완료처럼 이미 발생한 결과. UI와 AI가 필요한 결과를 구독한다. 이벤트 발행자는 구체적인 Widget/BTTask 클래스를 알지 않는다.

```text
PlayerCharacter의 공격 입력 ──┐
                            ├─ Combat.TryStartAttack(AttackId)
AI의 BTTask_Attack ──────────┘       │
                                    ├─ 승인: 실행 번호 반환, 완료 이벤트 대기
                                    └─ 거절: 실패 결과 반환

공격자 Combat
  → 서버 Trace
  → 피해 대상의 Damageable.ReceiveHit(HitRequest)
  → 대상 Character가 아래 순서로 조정
      1. Health가 살아 있는지 확인
      2. Combat.EvaluateDefense로 무적·방어·패링 판정
      3. Health.ApplyDamage로 최종 체력 적용
      4. 생존하면 피격 반응, 사망이면 공격 취소/사망 처리
  → HitResult(적용 피해, 방어/패링/무시/사망 결과) 반환
```

위 메서드명은 설계 예시다. 피해 요청에는 공격자·피해량·타격 위치/방향·반응 정보 중 실제 필요한 값만 둔다. 서버 내부에서 만든 값만 신뢰하고, 클라이언트가 넘긴 피해량이나 확정 대상을 그대로 적용하지 않는다. 인터페이스 호출 자체가 RPC는 아니다.

**의존성 방향:** Character → Combat/Health, Combat → Health 조회 및 Damageable 계약, UI → 상태 조회/이벤트 구독. Health는 Combat·AI·UI를 참조하지 않는다. Combat은 구체적인 PlayerCharacter/EnemyCharacter/BTTask/Widget 클래스를 참조하지 않는다. 방어 판정용으로 Health가 다시 Combat을 호출하는 순환을 만들지 않는다.

사망은 Health가 알리고 Character가 공통 종료를 연결한다. 공격의 FinishAttack은 여러 경로에서 호출되어도 같은 실행을 두 번 끝내지 않아야 한다. 완료 알림은 자원 정리 뒤 발행한다. 콜백이 새 공격을 시작해도 이전 공격 정리가 새 상태를 지우지 않게 하기 위해서다.

## Combat이 다시 거대해지지 않을 경계

초기에는 근접 공격·방어·회피의 상호 배타적인 실행 상태를 Combat 한 곳에서 관리한다. Combat 내부 함수는 TryStart / ExecuteHit / Cancel / Finish처럼 수명주기에 맞춘다. 타이머는 목적별로 분리한다.

아래 조건이 실제로 생기면 분리한다.

- **장비 장착/해제와 슬롯 상태가 생김:** EquipmentComponent를 추가한다. 현재 장비를 보관하고 사용 가능한 공격을 제공한다. 공격 실행 상태와 피해 계산은 Combat에 남긴다.
- **발사체가 채택됨:** Projectile Actor가 이동·충돌·발사 시점 피해 데이터를 소유한다. Combat은 발사 요청만 한다. 근접 공격과 공용 CurrentDamageInfo를 쓰지 않는다.
- **같은 대상에 대한 동시 공격 수 제한이 필요함:** 대상에 AttackSlotsComponent를 붙인다. Health에서 토큰 정책을 제거한다. 예약량은 공격 실행과 연결해 실패/중단에서 정확히 한 번 반환한다.
- **회피가 복잡한 네트워크 이동이 됨:** 이동 실행은 CharacterMovement 확장 쪽으로 옮긴다. 행동 승인과 다른 공격의 차단은 Combat이 결정한다.

회피·패링·콤보마다 독립 StateManager를 만들지는 않는다. 이 행동들은 서로 막아야 해서 별도 관리자를 만들면 조정 비용이 커진다. 모든 공격을 처리하는 범용 효과 그래프도 첫 근접 공격에 필요하지 않다.

## C++, Blueprint, DataAsset 분담

- **C++:** 서버 검증·상태 전이·판정·체력 적용·복제·실행 종료. 같은 동작 규칙을 여러 BP에 복제하지 않는다.
- **Character Blueprint:** Mesh·AnimBP·컴포넌트 기본값·입력 에셋·사용 가능한 공격 목록을 구성한다. 몸의 조립과 튜닝을 맡는다.
- **AttackDefinition:** 피해량·반경·거리·쿨다운·몽타주 같은 공격별 불변 데이터를 보관한다. 실행 중 Actor 참조와 타이머는 넣지 않는다.
- **AnimBP/Notify:** 상태를 표현하고 명중 시점을 전달한다. 서버의 공격 실행 번호·판정 창 검증을 통과해야 피해를 적용한다. Notify에 캐릭터별 상태를 저장하지 않는다.
- **Behavior Tree:** 감지/추적/접근/공격 선택 순서를 편집한다. 실제 공격·체력 처리는 공통 C++ 경로를 사용한다.
- **Widget Blueprint 또는 현재 C++ Widget:** 레이아웃과 표시. 두 UI 구현 방식을 동시에 다시 만들 필요는 없다. 기존 MVVM을 유지한다.

수치만 다른 적은 같은 EnemyCharacter 기반 BP와 데이터로 만든다. 이동 모델·상호작용·전투 규칙이 본질적으로 다른 경우에만 별도 C++ 클래스를 도입한다.

## 소스와 에셋의 폴더를 대응

```text
Content/PunchOut/
  Characters/Player/       BP_Player, ABP_Player
  Characters/Enemies/     적별 BP/AnimBP
  Combat/Attacks/          DA_Attack_PunchLight 등
  AI/                     BT/BB
  Input/                  IA/IMC
  UI/                     Widget 및 UI 리소스
  Maps/                   신규 맵
```

모든 Blueprint를 Blueprints 폴더 하나에, 모든 DataAsset을 Data 폴더 하나에 몰지 않는다. 기존 /Game/Levels/L_OfficePrototype과 ThirdPerson 에셋은 이 설계 논의만으로 이동하지 않는다.

## 구조가 적절한지 판단하는 변경 시나리오

1. **펀치 피해량 조정:** 공격 데이터 하나를 수정한다. Player/Enemy/AI 코드를 수정하지 않는다.
2. **같은 공격을 쓰는 적 추가:** 적 BP 구성과 공격 목록·BT 설정을 추가한다. Combat 구현은 그대로 쓴다.
3. **체력 UI 교체:** Widget/ViewModel 연결만 변경한다. Health는 UI 클래스를 모른다.
4. **공격 중 사망:** Health → Character → Combat.Cancel로 끝나며 타이머·판정·AI 예약이 남지 않는다.
5. **공격 요청 거절:** 플레이어와 AI가 같은 거절 결과를 받는다. AI가 완료 이벤트를 영원히 기다리지 않는다.
6. **Pawn 재소유:** Controller가 이전 Health 구독을 해제하고 새 Pawn을 연결한다. 이전 캐릭터 체력이 HUD에 남지 않는다.

새 기능 하나를 추가할 때 Character·AI·Widget을 모두 수정해야 한다면 경계를 다시 검토한다. 실제 두 번째 사용처가 생기기 전에는 추가 인터페이스나 모듈로 일반화하지 않는다.

## 첫 구현 범위

Health, Combat, AttackDefinition, 얇은 Character 기반과 기존 플레이어 BP 연결까지만 먼저 만든다. 더미 피격 대상에 공격 하나를 적용하고 서버/클라이언트 결과 및 취소 처리를 검증한다. 이후 같은 Combat으로 적 한 종류를 연결하면 공통화가 유효한지 확인할 수 있다.

기존 BP를 새 부모로 전환할 때 재부모화·상속 컴포넌트·에셋 참조를 별도 검증한다. 이름 변경과 동작 변경은 분리한다. 이 문서는 이전 분석의 폴더 초안을 구체화하며, 현재 게임 디자인에 없는 시스템을 추가 구현하자는 뜻은 아니다.
