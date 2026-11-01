# UnrealReboot → PunchOut 리팩토링 분석

> 코드 문제 분석은 참고 자료로 유지한다. 현재 구조 결정은 [PunchOut Lyra·GAS 설계](C:/ProjectsOfGame/PunchOut/artifacts/punchout-lyra-gas-design.md)이며, 아래의 GAS 미채택/단일 모듈 제안은 초기 검토안이다.

작성: 2026-11-01. 대상: `C:/ProjectsOfGame/UnrealReboot/Source` 및 현재 PunchOut의 Source, 프로젝트 설정, README.

후속 구조 제안: [소유권과 호출 경계](C:/ProjectsOfGame/PunchOut/artifacts/punchout-architecture-proposal.md). 아래 폴더 초안을 구체화한 권장안이며, 얇은 공통 Character와 컴포넌트 사이의 피해 처리 경계를 추가로 정의한다.

소스 정적 분석이다. Blueprint 그래프·에셋 참조 전체를 에디터에서 검사하거나 빌드·PIE를 실행한 결과는 아니다. 멀티플레이를 유지한다는 가정이며, 무기·마법·보스의 최종 채택 여부는 아직 미정이다. 제안된 클래스와 폴더는 새 설계이며 현재 구현되어 있다는 뜻이 아니다.

## 1. 권장 방향

현재 PunchOut 프로젝트 안에 작은 전투 기반을 만들고 UnrealReboot의 기능을 단위별로 이식한다. UnrealReboot 전체를 먼저 정리한 뒤 복제하는 방식은 사용하지 않을 기능까지 수정하게 된다.

핵심은 세 가지다.

1. **소유권:** 체력, 실행 중인 공격, AI 의사결정, UI를 각각 한 곳에서 관리한다.
2. **설정과 실행의 분리:** 공격 에셋에는 수치·몽타주만, 실행 중 대상·타이머·명중 목록은 컴포넌트에 둔다.
3. **실행의 시작과 종료:** 성공·실패·중단·사망에서 상태와 자원을 일관되게 정리한다.

처음에는 PunchOut 런타임 모듈 하나와 Health/Combat 컴포넌트 두 개면 충분하다. GAS, 범용 Ability 프레임워크, 기능별 플러그인, 대규모 상속 계층은 현재 요구만으로 도입하지 않는다. 예측·다수 효과 중첩·복잡한 스킬 비용이 실제 요구가 되면 GAS 도입과 직접 구현 비용을 다시 비교한다.

## 2. 실제 코드에서 확인한 구조 문제

- `MainPlayer.cpp` 1,111줄: 이동·입력, 세 가지 자세, 장비, 회피, 방어·패링, 콤보, 사격, HUD 생성, 카메라, 전역 슬로모션을 소유한다. 플레이어 변경이 전투와 표현을 동시에 건드린다.
- `CP_Attacks.cpp` 721줄: 판정·데미지 전달·점프 이동·발사체·광역기·AI Damage Sense·몽타주 디스패치를 포함한다. 이미 컴포넌트로 나누었지만 책임은 여전히 크다.
- `AIC_EnemyBaseCPP.cpp` 531줄: 감지 설정·타겟 관리·상태 전환·Blackboard 갱신이 모여 있다. 감지와 Blackboard 연결은 유지할 수 있으나 공격 실행 책임은 공통 Combat으로 내려야 한다.
- `DamageableInterface.h`: 체력·피격뿐 아니라 팀, 공격 여부, AI 공격 토큰, 인터럽트 설정까지 노출한다. 피해를 받을 수 있는 오브젝트에 AI 정책까지 요구한다.
- `FAttackInfo`: DamageInfo·Montage와 실행 중 AttackTarget이 함께 있다. 에셋 정의와 인스턴스 상태로 나눌 지점이다.
- 현재 PunchOut도 `UI/PunchOutUIFlow.h` 안에 GameInstance·GameState·GameMode·PlayerController가 함께 있다. 게임 규칙 클래스가 UI 파일에 들어 있는 구조부터 정리할 수 있다.

줄 수는 공백·주석 포함이며, 크기만으로 문제를 판단하지 않았다. 위 책임 결합과 실행 흐름이 분리 근거다.

## 3. 리팩토링보다 먼저 고정할 동작

### A. 회피 타이머 덮어쓰기 — 확인된 코드 문제

`MainPlayer.cpp:319`에서 무적 종료를 0.2초 후에 예약하고, `:323`에서 같은 DelayTimerHandle로 회피 초기화를 0.8초 후에 다시 예약한다. 앞선 예약이 대체된다. 패링과 슬로모션도 같은 핸들을 사용한다(`:1075`, `:1090`).

기능별 핸들과 종료 경로를 분리한다. 회피 무적은 0.2초, 재사용 대기는 0.8초라는 기존 의도를 먼저 테스트하고, 사망·취소·EndPlay에서도 복구하도록 한다. 향후 다른 무적 원인이 추가되면 회피 종료가 다른 무적을 지우지 않도록 원인별 소유권을 도입한다.

### B. 인터럽트 설정값 무시 — 확인된 코드 문제

`DamageSystem.cpp:153`의 SetIsInterruptible_FromComponent는 인자를 사용하지 않고 항상 true를 넣는다. false/true 설정을 각각 검증한 뒤 이식한다.

### C. AI 공격 시작 실패 무시 / 이동 완료 혼동

`BTT_MeleeAttackCPP.cpp`의 AttackStart 실패 분기는 Failed 반환이 주석 처리되어 있어 계속 실행된다. MoveToLocation의 RequestSuccessful도 도착 완료와 함께 처리해 즉시 공격한다. 이동 요청 수락과 목적지 도착은 분리해야 한다.

트리는 `접근 → 사거리 조건 → 공격 실행 → 완료 대기`로 구성한다. 공격 거절 시 즉시 실패하고, 성공한 실행의 완료 이벤트만 기다린다.

### D. 여러 AI의 Task 실행 상태 공유

`BTT_MeleeAttackCPP.h`는 CachedOwnerComp와 EnemyMelee를 객체 멤버로 저장한다. 생성자에는 bCreateNodeInstance 설정이 없고 NodeMemory도 사용하지 않는다. 동일 트리를 여러 AI가 사용하면 실행 상태가 섞일 위험이 있다. 현재 트리 배치는 에디터 검증이 필요하다.

처음에는 해당 비동기 Task를 인스턴스화하는 단순한 방법을 권장한다. AbortTask와 정상 종료에서 델리게이트를 해제하고, 캐시된 참조를 비운다. 다수 AI 성능 요구가 확인되면 NodeMemory 방식으로 바꾼다. 근거: [Epic BT Node Instancing Policy](https://dev.epicgames.com/documentation/en-us/unreal-engine/behavior-tree-node-reference-in-unreal-engine).

### E. 토큰 반환 경로 손실

`CP_Attacks.cpp:715`는 AttackEnd에 nullptr를 전달한다. `EnemyBase.cpp:261`의 AttackEnd는 유효한 Target이 있어야 토큰을 반환하고 Attacking을 false로 만든다. 해당 경로는 종료 알림만 보내고 정리는 건너뛴다.

실행 컨텍스트에 예약 대상·예약량을 보관하고 FinishAttack 한 곳에서 정확히 한 번 반환한다. 정리 후 완료 이벤트를 보낸다. 실패·중단·사망·대상 파괴에서도 누락과 이중 반환을 검사한다.

### F. 발사체별 데이터가 발사자 공용 변수에 있음

`CP_Attacks.cpp:222`에서 CurrentDamageInfo를 덮어쓰고 `:527`에서 충돌 시 다시 읽는다. 서로 다른 데미지의 발사체가 동시에 살아 있으면 이전 발사체에 새 공격 값이 적용될 수 있다.

발사체를 채택할 때 발사 시점의 DamageSpec·Instigator·Team을 각 발사체에 보관한다. 광역 Actor도 같은 기준으로 검토한다. 지금 근접 공격만 이식한다면 이 문제를 해결하려고 마법 시스템 전체를 가져오지는 않는다.

### G. 동일 타입 인자 순서 혼동

`CP_Attacks.h:82`의 SphereTraceDamage는 Length, Radius 순서지만 구현(`.cpp:314`)의 이름은 Radius, Length 순서다. 둘 다 float이므로 컴파일로 검출되지 않는다. 현재 호출이 모두 틀렸다는 뜻은 아니지만 변경 시 오용하기 쉽다. 판정 설정 구조체의 명명된 필드로 바꾼다.

## 4. 권장 코드 폴더

아래는 첫 근접 전투와 적 한 종류까지의 목표 구조다. 확장용 빈 폴더를 미리 만들지는 않는다. 단일 게임 모듈이므로 .h/.cpp를 기능별로 함께 두어도 충분하다. Public/Private는 다른 모듈이 사용할 API가 생길 때 적용한다.

```text
Source/PunchOut/
  PunchOut.Build.cs
  PunchOut.h / PunchOut.cpp
  Game/
    PunchOutGameInstance.h/.cpp
    PunchOutGameMode.h/.cpp
    PunchOutGameState.h/.cpp
  Player/
    PunchOutPlayerController.h/.cpp
    PunchOutPlayerCharacter.h/.cpp
  Combat/
    PunchOutCombatTypes.h
    PunchOutDamageable.h
    PunchOutHealthComponent.h/.cpp
    PunchOutCombatComponent.h/.cpp
    Data/
      PunchOutAttackDefinition.h/.cpp
    Animation/
      PunchOutAnimNotify_Hit.h/.cpp
      PunchOutAnimNotifyState_HitWindow.h/.cpp  [연속 판정이 필요할 때]
  AI/
    PunchOutEnemyCharacter.h/.cpp
    PunchOutAIController.h/.cpp
    Tasks/
      PunchOutBTTask_Attack.h/.cpp
    Decorators/                              [커스텀 조건이 필요한 경우]
  UI/
    ViewModels/
      PunchOutViewModel.h
    Widgets/
      PunchOutUIView.h/.cpp
  Tests/
    PunchOutHealthTests.cpp
    PunchOutCombatTests.cpp
```

추후 필요가 확인되면 Combat/Weapons, Combat/Projectiles, AI/AttackSlotsComponent를 추가한다. MainPlayer의 모든 함수를 각각 별도 컴포넌트로 만드는 방식은 피한다. 회피도 첫 구현은 Character/Combat의 명확한 메서드로 충분하고, 여러 캐릭터가 공유하거나 독립 수명주기가 커지면 추출한다.

명명은 CPP 접미사를 없애고 역할을 드러낸다. 예: UCP_Attacks → UPunchOutCombatComponent, UDamageSystem → UPunchOutHealthComponent. 단, 기존 UCLASS를 단순 이름 치환하면 Blueprint 참조가 깨질 수 있으므로 이식 단계와 함께 처리한다.

## 5. 각 클래스의 책임과 데이터 흐름

- **PlayerCharacter:** 이동·카메라·입력 바인딩 및 컴포넌트 조립. Attack 입력을 Combat에 전달한다. HUD를 생성하지 않는다.
- **PlayerController:** 로컬 화면/입력 모드·메뉴 명령·소유 플레이어 연결. 체력 계산이나 AI 공격을 맡지 않는다.
- **CombatComponent:** 공격 가능 여부, 실행 중 공격, 방어·패링 창, 판정, 취소/종료를 소유한다. 초기에는 판정 헬퍼도 여기에 두어 과도한 컴포넌트 분리를 피한다.
- **HealthComponent:** MaxHealth/Health와 피해·회복·사망 전이를 소유한다. 음수 피해·초과 회복 정책을 정하고 0 ≤ Health ≤ MaxHealth를 보장한다. 방어 가능 여부는 Combat이 제공하고, 최종 적용은 하나의 경로에서 처리한다.
- **AttackDefinition:** 변경하지 않는 공격 설정. 현재 타겟·명중 목록·타이머는 포함하지 않는다.
- **EnemyCharacter:** Mesh/AnimBP와 Health/Combat을 구성한다. 적 종류별 수치 차이는 Blueprint 기본값 또는 공유 데이터로 표현한다. 실제 행동이 다른 적만 C++ 파생 클래스를 만든다.
- **AIController/BT:** 누구를 언제 공격할지 결정하고 Combat에 요청한다. BTTask는 적 파생 클래스의 ShortRangeAttack/SpinningAttack을 직접 호출하지 않는다.
- **UI ViewModel/Widget:** Health 변경과 매치 상태를 구독해서 표시한다. UI에서 게임 규칙을 결정하지 않는다.

```text
플레이어 입력 / 서버 AI
  → Combat.TryStartAttack(선택한 공격)
  → 서버가 상태·쿨다운·허용 공격을 검증
  → 실행 컨텍스트 생성 + 공격 시작 상태 전달
  → 판정 시점/구간에서 서버 Trace
  → 대상의 피해 수신 경로 → 방어 판정 → Health 변경
  → Health 복제/변경 이벤트 → ViewModel → Widget
  → 성공·중단·실패 모두 FinishAttack → 정리 후 완료 통지
```

Health와 Combat 사이의 복잡한 범용 이벤트 버스는 필요하지 않다. 소유 Character 또는 좁은 Damageable API가 두 컴포넌트를 연결하면 된다. 기존 DamageableInterface에서 AI 토큰 API를 제거하고, 팀 판정은 최소한의 팀 식별자로 시작한다.

## 6. 하드코딩을 옮길 기준

모든 숫자를 DataAsset으로 옮기면 오히려 편집 지점이 늘어난다. 변경 이유에 따라 구분한다.

- **공격별 수치/에셋 → AttackDefinition:** Damage, RangeCm, RadiusCm, CooldownSeconds, Montage, 방어·패링 허용 여부. 기존 MainPlayer.cpp:844의 10 데미지·20/50 판정, EnemyMelee.cpp의 10/25 데미지·20/200/300 판정이 대상이다.
- **한 캐릭터의 기본값 → EditDefaultsOnly 설정:** 걷기 속도, 회피 거리·무적 시간, 카메라 거리. 여러 캐릭터가 같은 프리셋을 공유하게 될 때만 CharacterDefinition 에셋으로 묶는다.
- **AI 조정값 → AIController 기본값 또는 공유 프로필:** AIC_EnemyBaseCPP.cpp:148의 SightRadius 1500, LoseSightRadius 2000, 시야각 60, 검사 주기 0.5. Blackboard 키 선택은 FBlackboardKeySelector로 노출한다.
- **모델 결합 → Blueprint/장비 설정:** Gun 본 숨김, RightHand/hand_r/hand_r_sword_socket을 사용할 메시 기준으로 설정한다. 잘못된 소켓은 에셋 검사에서 찾아낸다.
- **애니메이션 이벤트 → 역할이 명확한 Notify:** Slash/AOESlash/Jump 문자열 분기 대신 Hit/HitWindow와 같이 Combat 호출에 집중한 Notify를 사용한다. 기존 몽타주의 문자열은 이식 중 호환 단계에서 유지할 수 있다.
- **엔진 식별자·알고리즘 상수 → 코드 유지:** 서브오브젝트 이름, 단위 변환, 0/1 경계값 등은 설정 에셋으로 옮길 이유가 없다. 필요한 문자열은 명명된 상수로 모은다.

첫 AttackDefinition은 UDataAsset이면 충분하다. PrimaryDataAsset은 Asset Manager를 통한 ID·로딩 정책이 실제로 필요할 때 선택한다. 수많은 같은 형태의 공격을 표로 편집할 필요가 생기면 DataTable을 검토한다. 동일 값을 DataAsset과 DataTable 양쪽에서 관리하지 않는다. [Epic Data Assets](https://dev.epicgames.com/documentation/unreal-engine/data-assets-in-unreal-engine)

**설정 예시:** DA_Attack_PunchLight에 Damage=10, RangeCm=50, RadiusCm=20, Montage 참조를 둔다. 이는 기존 코드 수치를 옮기는 예시이며 PunchOut 최종 밸런스 결정은 아니다.

**실행 상태 예시:** 실행 번호, 사용 중 Definition 참조, 약한 Target 참조, 이번 타격 구간에서 맞은 Actor 집합, 시작 시간, 취소 여부. 콤보 여러 타는 타격 구간마다 명중 집합을 새로 시작한다. 공유 DataAsset에 이 상태를 쓰지 않는다.

## 7. 상태와 애니메이션

현재 canMove/attacking/bMeleeAttackExcute/bRangeAttackExcute/readyToDodge/bHasDodged 등의 중복 상태는 값이 어긋나기 쉽다. 서로 배타적인 행동은 Idle/Attacking/Blocking/Dodging/Stunned/Dead와 같은 작은 enum으로 관리하고, CanMove/CanAttack은 상태에서 계산한다. 동시에 존재할 수 있는 무적·팀·쿨다운은 별도 의미로 유지한다.

TryStartAttack은 성공 여부를 반환해야 한다. 몽타주 없음·재생 실패·쿨다운·사망이면 상태를 남기지 않고 실패한다. End/Interrupted/Death/Abort/EndPlay는 같은 정리 경로로 모으고 실행 번호로 늦게 온 이전 콜백을 거른다.

몽타주 에셋 포인터만 실행 ID로 쓰지 않는다. 동일 공격 재실행, 중단 후 재시작을 구분한다. UObject를 오래 보관하는 필드는 의도에 따라 UPROPERTY/TObjectPtr 또는 TWeakObjectPtr를 사용하고 람다의 raw Actor 캡처 수명을 점검한다.

Notify 객체 자체에는 캐릭터별 실행 상태를 저장하지 않는다. Notify는 소유자의 Combat에 이벤트를 전달하고, 실제 명중 기록은 Combat이 가진다. 중단 시 NotifyEnd만 믿지 않고 FinishAttack에서도 판정 창을 닫는다.

## 8. 멀티플레이를 전제로 바꿀 부분

UnrealReboot Private 소스에서 Server/NetMulticast/Replicated/HasAuthority/GetLifetimeReplicatedProps 패턴을 찾지 못했다. 이는 Blueprint 전체를 포함한 네트워크 감사 결과는 아니지만 C++ 전투를 그대로 가져올 수 없다는 근거다.

- 클라이언트는 소유 Pawn/Combat을 통해 허용된 공격 ID와 필요한 조준 입력만 요청한다. 피해량·피해 대상 확정을 신뢰하지 않는다.
- 서버가 공격 상태·쿨다운·판정·피해·사망을 결정한다. AI도 이 실행 경로를 사용한다.
- Health/사망/지속되는 공격 상태를 복제하고 UI는 변경 알림으로 갱신한다. 일회성 효과는 별도 표현 이벤트로 처리한다. 복제 속성을 모든 타격 이벤트의 기록처럼 사용하지 않는다.
- Actor와 Component 모두 복제 설정이 필요하다. [Epic Actor Component Replication](https://dev.epicgames.com/documentation/en-us/unreal-engine/replicating-actor-components-in-unreal-engine)
- 서버에서도 판정용 애니메이션·본 갱신이 실행되는지 확인한다. 클라이언트 Notify가 피해를 적용하게 두지 않는다. 전용 서버를 지원하면 렌더링이 없는 경우도 검증한다. 서버 타이머 기반 판정을 택한다면 애니메이션과 별도의 중복 타이밍 값을 방치하지 않는다.
- 회피의 SetActorLocation 직접 이동은 네트워크 이동 정책을 다시 정해야 한다. 첫 단계에서는 서버 검증 이동, 이후 요구에 따라 CharacterMovement/Root Motion 기반 예측을 검토한다. [Epic Networked Movement](https://dev.epicgames.com/documentation/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine)
- MainPlayer.cpp:1085의 SetGlobalTimeDilation을 패링의 기본 표현으로 이식하지 않는다. 로컬 카메라·사운드·효과로 먼저 표현하고, 전체 게임 시간을 늦추는 것이 실제 게임 규칙일 때 별도로 설계한다.

## 9. Content와 기존 PunchOut 연결

```text
Content/PunchOut/
  Characters/Player/        BP, AnimBP, 캐릭터별 애니메이션
  Characters/Enemies/       적별 BP와 애니메이션
  Combat/Attacks/           DA_Attack_*, 공유 몽타주
  Input/                   IA_*, IMC_*
  AI/                      BT_*, BB_*
  UI/                      WBP_*
  Maps/                    새 맵을 만들 때 적용
```

이 구조는 신규·이식 에셋부터 적용한다. 현재 /Game/Levels/L_OfficePrototype과 ThirdPerson 에셋을 먼저 이동할 필요는 없다. 특히 PunchOutUIFlow.cpp:54는 BP_ThirdPersonCharacter를 기본 Pawn으로 지정한다. C++ PunchOutPlayerCharacter만 변경해도 실제 플레이 Pawn에 반영된다고 가정해서는 안 된다. BP 부모 클래스·입력 경로를 먼저 확인한다.

기존 PunchOutUIFlow는 Game 계열 클래스를 개별 파일로 분리하되 UCLASS 이름은 우선 유지한다. UIPlayerController 이름 변경은 별도 단계로 미룰 수 있다. 맵 문자열은 한 설정 지점에서 참조하고, 입력 MappingContext도 Pawn 소유/재소유 흐름에 맞춰 설정한다.

현재 0.1초마다 RefreshViewModel하는 경로는 기능별로 변경 이벤트로 대체한다. Health는 OnHealthChanged/OnRep, MatchStarted는 RepNotify와 서버 로컬 갱신을 연결한다. PlayerCount처럼 별도 수명주기 처리가 필요한 항목은 하나씩 교체하며 일괄 삭제하지 않는다.

에셋은 필요한 Blueprint와 종속 몽타주·스켈레톤·입력·BT를 함께 이식한다. UE 5.2 원본은 별도로 보존하고 PunchOut 5.8 쪽에서 컴파일·리타게팅·충돌 채널·소켓을 확인한다. C++ 모듈/클래스/구조체/프로퍼티 이름 변경 시 정확한 Core Redirects를 설정한다. 큰 클래스를 여러 컴포넌트로 분해하는 작업은 이름 Redirect만으로 자동 변환되지 않으므로 BP 재부모화와 값 재연결이 필요하다. [Epic Core Redirects](https://dev.epicgames.com/documentation/en-us/unreal-engine/core-redirects-in-unreal-engine)

## 10. 실행 순서와 완료 기준

1. **기준 확보:** 현재 맵·실제 Pawn·입력·BP 부모·필수 에셋 확인. 기존 변경을 보존한 작업 단위 확보. 완료: 이동·메뉴·2인 접속 기준 상태 기록.
2. **Health 이식:** 피해 데이터 최소화, 체력·사망·복제, 인터럽트 버그 수정. 완료: 피해/회복 경계, 사망 1회, 서버·클라이언트 HP 일치 검증.
3. **공격 하나 이식:** AttackDefinition + Combat, 근접 판정·몽타주·공통 종료. 완료: 1회 타격 중복 없음, 공격 거절/중단/죽음에서 상태 복구, 다른 공격 데이터로 바꿔도 코드 변경 없음.
4. **회피·방어·패링:** 타이머와 상태 소유권 분리. 완료: 무적 종료/회피 쿨다운, 연속 입력과 중단 시 복구, 공격·방어 충돌 규칙 검증.
5. **적 한 종류:** 감지·이동·공격 BT, Task 인스턴스 및 Abort 정리. 필요하면 공격 슬롯 추가. 완료: 같은 BT의 적 두 마리 이상 동시 실행, 토큰 거절·중단·대상 파괴 시 예약량 복구.
6. **UI와 에셋 정리:** Game/UI 파일 분리, 체력 이벤트 연결, 필요한 에셋만 이동. 완료: BP 컴파일·에디터 재실행·기본 맵 로드·Cook/패키징 확인.
7. **선택 기능:** 무기/원거리/광역/보스가 채택되면 해당 실행 데이터 수명과 네트워크 정책을 검증하며 추가.

새 코드에 기존 장애를 재현하는 의미 있는 테스트를 먼저 둔다. 대표 회귀 테스트는 회피 무적 타이머, 인터럽트 false, 공격 실패 시 실행 금지, 두 AI의 콜백 분리, 공격 취소 시 토큰 1회 반환이다. 발사체를 채택한다면 서로 다른 데미지의 두 발사체를 역순으로 명중시키는 테스트를 추가한다.

폴더 이동·이름 변경과 동작 변경은 가능한 한 별도 변경 단위로 나눈다. 각 단계에서 빌드와 해당 기능 PIE를 통과한 뒤 다음 기능으로 진행한다.
