# PunchOut — Lyra 모티프의 피처 플러그인 + GAS 설계

## 구현 진행 상황

2026-11-01 첫 적용: PunchOutCore 런타임 플러그인, PlayerState 소유 ASC, Pawn Avatar 연결/해제, 기존 BP_ThirdPersonCharacter의 Core 부모 연결을 구현했다. Editor 빌드와 PunchOut.Core.AvatarOwnership 자동화 테스트 1개가 통과했다. BP 컴파일 및 메시·AnimBP·이동 속도·점프·복제 기본값 보존을 확인했다. 백업은 Saved/CoreMigration에 있다.

Combat/OfficeGame/FrontEnd 피처, AttributeSet/Effect/AbilitySet, Experience, ModularGameplay 준비 상태, 입력 태그 연결은 아직 구현 전이다. 현재 Pawn 소유권 테스트는 단일 프로세스 게임 World 테스트이며 2인 PIE 네트워크 검증을 대신하지 않는다. 아래의 '게임 코드를 변경하지 않는다'는 설계 단계 당시 기록이다.

2026-11-01. 현재 합의: 이 채팅에서 PunchOut을 먼저 설계한다. 새 프로젝트·새 에이전트 생성은 중단한다. GAS는 채택한다. UnrealReboot는 기존 구현과 동작을 분석하는 참고 자료다.

이 문서는 초기의 단일 모듈/자체 Combat 상태 머신 제안을 대체한다. 아래 파일·클래스·에셋 이름은 목표 설계다. 구현 완료 목록이 아니다. 앞서 잠깐 추가한 GAS 초안은 직전 전환 요청 당시 이미 철회되었고, 기존 PunchOut의 복구 빌드는 성공했다. 이번 설계 단계에서는 게임 코드를 변경하지 않는다.

## 1. 선택하는 구조

**상시 로드되는 공통 기반 + 코드와 에셋을 함께 묶는 Game Feature 플러그인 + 피처를 조합하는 Experience**를 채택한다.

모듈은 C++ 빌드와 공개 API의 경계다. 플러그인은 모듈·에셋·설정을 묶는 배포/관리 경계다. Game Feature는 플러그인의 등록·로딩·활성화 수명주기를 제공한다. 컴포넌트는 특정 Actor의 런타임 기능이다. 따라서 피처가 최상위 설계 단위이고, 컴포넌트는 필요할 때 피처 내부에서 사용한다. [Epic Modules](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules), [Plugins](https://dev.epicgames.com/documentation/en-us/unreal-engine/plugins-in-unreal-engine), [Game Features](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-features-and-modular-gameplay-in-unreal-engine)

Lyra의 설계 원리를 참고하되, 클래스 이름만 바꾸거나 샘플 전체를 복제하는 작업으로 정의하지 않는다. 현재 로컬 ProjectsOfGame 폴더에서는 Lyra 체크아웃을 찾지 못했으므로, Lyra 관련 판단은 공식 문서에 근거한다. 실제 소스 차용 전에는 사용할 엔진 버전과 맞는 Lyra 소스를 확인한다.

## 2. 최상위 폴더

```text
PunchOut/
  Source/
    PunchOut/                         앱 진입점과 프로젝트 설정 연결
      PunchOut.Build.cs
      PunchOut.cpp

  Plugins/
    PunchOutCore/                     상시 사용하는 공통 기반
      PunchOutCore.uplugin
      Source/PunchOutCore/
        PunchOutCore.Build.cs
        Public/
          AbilitySystem/             ASC, AbilitySet, Ability 기반 계약
          Experiences/               ExperienceDefinition, 로딩 계약
          Pawn/                      기본 Character, PawnData, 초기화 연결
          Player/                    PlayerState, Controller 기반
          Input/                     InputConfig, 입력 태그 연결
          UI/                        화면 삽입/제거와 로컬 UI 연결 계약
        Private/                     위 API의 구현

    GameFeatures/
      PunchOutCombat/
        PunchOutCombat.uplugin
        Source/PunchOutCombat/
          PunchOutCombat.Build.cs
          Public/                    외부에서 필요한 전투 계약만 공개
          Private/
            Abilities/               공격, 회피, 방어, 피격/사망
            Attributes/              HealthSet
            Effects/                 피해 계산
            Animation/               명중 시점/구간 연결
            AI/                      Ability를 요청하는 공격 Task
            UI/                      전투 ViewModel과 표시 연결
        Content/
          Abilities/                 GA_*
          Effects/                   GE_*
          AbilitySets/               DA_AbilitySet_*
          Animation/                 몽타주, 전투 애니메이션
          UI/                        전투 위젯

      PunchOutOfficeGame/             사무실 플레이 규칙과 실제 게임 구성
        Source/PunchOutOfficeGame/
        Content/
          Characters/                플레이어/적 BP와 PawnData
          AI/                        추적·순찰·교전 BT/BB
          Rules/                     목표/종료/탈출 규칙

      PunchOutFrontEnd/               메뉴·로비 화면과 연결 흐름
        Source/PunchOutFrontEnd/
        Content/UI/

  Content/PunchOut/
    Experiences/                     DA_Experience_Office, FrontEnd
    Maps/                            시작 맵과 실제 플레이 맵
```

OfficeGame·FrontEnd의 상세 기능은 현재 게임 요구 안에서 구현한다. 탈출·승패 조건을 이 설계에서 임의로 확정하지 않는다. Maps를 별도 피처로 분리할 필요가 생기기 전까지 일반 프로젝트 Content에 둔다. 기존 에셋은 설계만으로 이동하지 않는다.

공통 코드는 현재의 PunchOut 게임 모듈에 둘 수도 있다. 여기서는 피처들이 게임 진입 모듈에 의존하지 않도록 PunchOutCore라는 상시 플러그인을 선택한다. Core는 GAS 연결·초기화·Experience·UI 확장 계약까지만 포함한다. 공격 수치, 사무실 목표, 특정 무기, 특정 Widget은 Core에 넣지 않는다.

## 3. 의존성 — 컴파일과 활성화를 구분

화살표는 '왼쪽 코드가 오른쪽 모듈의 API를 사용'한다는 뜻이다.

```text
PunchOut             → PunchOutCore
PunchOutCombat       → PunchOutCore
PunchOutOfficeGame   → PunchOutCore + PunchOutCombat
PunchOutFrontEnd     → PunchOutCore
```

Core는 Combat/OfficeGame/FrontEnd 헤더를 include하지 않는다. OfficeGame은 구체적인 전투 게임이므로 Combat을 명시적으로 사용할 수 있다. FrontEnd는 전투 내부를 읽지 않는다. 모든 것을 Common 모듈로 옮겨 순환 의존을 숨기지 않는다.

각 모듈에서 외부 사용 헤더만 Public으로 노출하고, 다른 모듈의 Private 헤더를 include하지 않는다. Build.cs의 Public/Private 의존성은 공개 헤더에 타입을 노출하는지에 따라 선택한다. DLL 경계의 공개 C++ 타입에는 해당 모듈의 API export를 적용한다.

활성화는 별도다. Office Experience가 Combat을 준비하고 OfficeGame을 활성화한다. GameFeatureData, 플러그인 의존성, 필요한 Action이 준비되어야 한다. Build.cs에 넣었다고 해당 피처가 자동으로 게임플레이에 연결되는 것은 아니다.

## 4. Experience의 책임

Experience는 '이번 플레이에서 어떤 기능과 구성을 사용할지'를 나타내는 선언이다. Lyra 전용 Experience 시스템은 GameFeatures 플러그인만 켠다고 자동 제공되지 않는다. 필요한 Definition·Manager·Action을 구현하거나 검토된 Lyra 소스에서 가져와야 한다. [Epic Lyra Sample](https://dev.epicgames.com/documentation/unreal-engine/lyra-sample-game-in-unreal-engine?lang=en-US)

초기 데이터는 아래 정도로 제한한다.

- 활성화할 Game Feature 이름/식별자.
- 사용할 기본 PawnData의 soft reference.
- 필요한 기능 부여·입력·UI 연결 Action 또는 ActionSet.

Office/FrontEnd Experience는 처음에는 항상 검색 가능한 프로젝트 Content에 둔다. 아직 등록되지 않은 피처 안에서 Experience를 찾으려는 부트스트랩 순환을 피한다. Asset Manager의 Experience 및 GameFeatureData 등록 규칙과 cook 포함 여부를 검증한다.

```text
서버가 Experience 선택
  → 식별자를 클라이언트에 공유
  → 각 실행 환경에서 필요한 피처 등록/로드/활성화
  → 서버가 기본 Pawn 준비
  → PawnData·PlayerState·ASC·입력의 준비 상태 확인
  → GameplayReady 이후 기능 실행/UI 연결
```

UI 전용 작업은 로컬 플레이어가 있을 때만 실행하고, 서버 전용 부여와 로컬 입력 바인딩을 구분한다. 로딩 실패 시 무한 대기를 만들지 않고 실패 결과를 표시한다. 시작 시 한 번 고른 Experience로 먼저 검증하며, 플레이 중 임의 Experience 전환은 초기 범위에 넣지 않는다.

## 5. GAS가 기존 시스템을 대체하는 위치

- MainPlayer/CP_Attacks의 공격 실행 → GameplayAbility.
- 공격 비용·쿨다운·무적·버프 → GameplayEffect와 GameplayTag.
- DamageSystem의 Health/MaxHealth → Combat 피처의 AttributeSet.
- 공격·방어·회피·기절 등 활성화 제한 → Ability의 태그 요구/차단/취소 규칙.
- 몽타주 대기·명중 시점 대기 → AbilityTask 또는 Ability 수명과 연결된 Notify 처리.
- 피격 이펙트·사운드 등 표현 → GameplayCue 또는 피처의 표현 코드.
- 공격 종류별 수치·몽타주 → Ability/Effect 기본값과 필요한 DataAsset.

GAS와 별도로 같은 공격 상태를 가진 CombatManager를 병행하지 않는다. UI를 위한 Health 표시 객체를 만들더라도 값의 원본은 AttributeSet이며, 독립 Health float를 유지하지 않는다.

전투 Damage Effect는 서버가 승인한 피해 정보를 사용한다. 예를 들어 Damage 메타 Attribute로 받은 값을 Health에서 차감한 뒤 Damage는 소비/초기화한다. 방어·패링 결과와 피해 0 처리, 사망 전이의 책임을 Combat 피처 안에서 확정한다. 사용자가 정하지 않은 밸런스 값은 이 설계에서 결정하지 않는다.

입력 태그 연결은 프로젝트 코드로 제공해야 한다. 예: IA_Attack → InputTag.Attack.Light → 부여된 AbilitySpec → GA_PunchLight. 이동·시점 입력까지 반드시 Ability로 바꿀 필요는 없다. [Epic Abilities in Lyra](https://dev.epicgames.com/documentation/en-us/unreal-engine/abilities-in-lyra-in-unreal-engine)

## 6. ASC·Attribute·Ability 수명

**플레이어:** Core의 PlayerState가 ASC를 소유하고 현재 Character를 Avatar로 연결한다. Pawn 교체를 지원할 수 있지만, 체력·효과를 무엇까지 유지할지는 별도 정책이다. PlayerState에 ASC가 있다고 사망 후 새 Pawn이 자동으로 완전 회복되는 것은 아니다.

**일반 적:** 초기에는 Pawn 소유 ASC를 권장한다. 적이 죽을 때 함께 제거되는 수명이 자연스럽다. 플레이어와 동일한 리스폰·점수·팀 참가자 모델을 쓰는 봇만 PlayerState 소유를 검토한다. Ability 코드는 IAbilitySystemInterface 등 공통 ASC 접근 계약을 사용해 두 경우를 지원한다.

**부여 수명:** 플레이어 세션, Pawn, 장비/피처별 부여를 구분한다. 각각 AbilitySpecHandle·EffectHandle·추가 AttributeSet을 추적한다. Pawn 해제 때 세션 전체 능력을 무조건 CancelAllAbilities로 지우지 않는다.

**Combat HealthSet:** Core는 구체적인 HealthSet 클래스를 생성하지 않는다. Combat의 AbilitySet/Action이 서버에서 부여하고 연결한다. 피처 비활성화 시 이를 참조하는 Ability/Effect·UI를 정리한 다음 AttributeSet을 안전하게 회수한다. 공유로 쓰는 AttributeSet은 한 부여자가 임의로 제거하지 않도록 소유권을 관리한다. 늦게 참가한 클라이언트의 동적 AttributeSet 복제도 검사한다.

**재소유:** 이전 Pawn이 늦게 종료되어 새 Pawn의 Avatar 연결을 지우지 않게 검사한다. OnRep_PlayerState 한 곳만으로 준비 완료를 가정하지 않고 초기화 상태를 관찰한다.

Lyra의 PlayerState ASC 구조는 Pawn 교체 시 GAS 상태 유지에 유용하되 Pawn 관련 상태의 초기화/해제가 필요하다. [Epic Abilities in Lyra](https://dev.epicgames.com/documentation/en-us/unreal-engine/abilities-in-lyra-in-unreal-engine)

## 7. 초기화와 피처 정리

ModularGameplay의 준비 상태 흐름을 채택한다.

`Spawned → DataAvailable → DataInitialized → GameplayReady`

이 흐름은 로딩/복제/소유자 준비 순서를 조정하는 초기화 상태다. 전투의 Idle/Attacking 같은 게임플레이 상태 머신과 혼동하지 않는다. 늦게 등록된 구독자도 현재 준비 상태를 확인할 수 있게 한다. [Epic Component Manager](https://dev.epicgames.com/documentation/unreal-engine/game-framework-component-manager-in-unreal-engine)

피처 활성화 시 만든 입력 바인딩, UI 인스턴스, delegate, Ability 부여를 반환 핸들로 기록한다. 비활성화/World 종료 시 실행 중 Ability를 취소하고, 해당 피처 소유 부여·입력·UI·콜백을 해제한다. 실행 데이터는 World/Actor/LocalPlayer 단위로 보관한다. IModuleInterface 전역 싱글턴에 현재 적·체력·타겟을 저장하지 않는다. 여러 PIE World가 같은 모듈 객체를 공유한다는 점을 고려한다.

엔진 기본 GameFeatureAction과 Lyra의 프로젝트 전용 Action을 구분한다. 능력 세트 부여·입력 태그 바인딩·UI 삽입 같은 작업은 필요한 범위에서 프로젝트 Action으로 구현한다. 컴포넌트 추가 Action은 엔진 기능을 재사용한다.

## 8. 피처 단위가 실제로 의미하는 것

**Combat:** 공격 Ability·피해 Effect·HealthSet·명중 Notify·전투 UI·AI의 공격 요청 Task를 함께 소유한다. UI와 AI라는 이유만으로 다른 전역 폴더로 흩어놓지 않는다.

**OfficeGame:** 적의 순찰/추격/교전 순서, 사무실 목표, 실제 PawnData, 플레이어와 적의 BP를 소유한다. 공격 실행은 Combat API를 쓴다. 구체적인 승패/탈출 규칙은 다음 게임 디자인 단계에서 정한다.

**FrontEnd:** 메뉴·로비 화면과 사용자 연결 명령을 소유한다. 게임 세션의 장기 연결 상태는 Core의 서비스/엔진 계층과 연동한다. FrontEnd 비활성화 때 네트워크 연결 자체까지 사라지게 만들지 않는다.

**UI 기반:** Core에는 화면 삽입 지점과 등록/해제 계약만 둔다. 기존 MVVM은 피처 내부 표시 방식으로 계속 사용할 수 있다. CommonUI/UIExtension 채택은 GAS 채택과 별개이며 현재 단계에서 기존 UI 전체 교체를 강제하지 않는다.

## 9. 첫 구현 순서와 검증 기준

1. **Core 모듈 분리:** 현재 Game/Player/Pawn 공통 틀을 옮기고 기존 Blueprint/메뉴/이동 동작을 유지. 클래스 이동은 Core Redirects와 BP 부모 참조를 확인. 완료: Editor/Game 빌드, BP 컴파일, 기존 맵 실행.
2. **GAS·초기화 기반:** PlayerState ASC, Pawn Avatar 연결, AbilitySet 부여/회수, 입력 태그 연결. 완료: 서버/클라이언트, 늦은 PlayerState 준비, 재소유, 중복 초기화에서 능력 중복 없음.
3. **Combat 피처 한 개:** HealthSet + 피해 Effect + 기본 공격 Ability + 테스트 타겟. 완료: 서버 판정·HP 복제·공격 취소·연속 입력·중복 명중 방지.
4. **Office Experience 연결:** Combat/OfficeGame 준비와 PawnData 로딩을 연결. 완료: 로딩 완료 이전 입력 차단, 실패 처리, 패키징에서 에셋 누락 없음.
5. **기존 기능 단계 이식:** 방어→패링→회피→적 AI 순으로 UnrealReboot 동작을 재현하며 정리. 완료: 버그 재현 테스트와 기능별 PIE 통과.
6. **FrontEnd 피처 이전:** 메뉴와 로비를 마지막에 분리. 완료: FrontEnd↔Office 맵 이동, 재접속, 입력/UI 중복 없음.

처음부터 모든 피처를 동시에 옮기지 않는다. 목표 폴더는 확정하되 실제 구현은 전투 한 사이클이 빌드와 네트워크 검증을 통과하는 단위로 진행한다. GameFeature 플러그인 생성만으로 완료라고 판단하지 않는다.

## 10. UnrealReboot에서 가져올 것

판정 알고리즘, 공격/방어 의도, 몽타주와 애니메이션, 적 패턴을 참고한다. MainPlayer·CP_Attacks·DamageSystem의 클래스 형태는 새로운 시스템에 맞게 재구성한다.

알려진 회귀 항목: 회피 타이머 덮어쓰기, 인터럽트 인자 무시, AI 공격 시작 실패 무시, BT 인스턴스 상태 공유, 종료 시 공격 토큰 반환 누락, 발사체 공용 피해 데이터. 이 항목들을 GAS로 옮겼다는 이유만으로 해결되었다고 가정하지 않는다.

이번 결정에서 확정한 것은 **PunchOut을 설계 대상, Lyra를 모티프, GAS를 전투 실행 기반, 피처 플러그인을 상위 코드/에셋 경계로 삼는 것**이다. 보스·무기 종류·승패 조건·CommonUI 전환은 아직 확정하지 않는다.
