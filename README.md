# BaseballGame — 언리얼 멀티플레이 숫자 야구

서버가 숨긴 3자리 비밀 숫자를 상대보다 먼저 맞히는 멀티플레이어 숫자 야구 게임.
데디케이티드 서버 구조에서 RPC와 Property Replication으로 상태를 동기화한다.

- 엔진: Unreal Engine 5.5
- 모듈/접두사: C++ `BaseballGame` 모듈, 클래스 접두사 `BG`

## 게임 규칙

- 서버가 1~9 사이, 서로 중복되지 않는 3자리 정답을 생성한다.
- 플레이어는 채팅창에 3자리 숫자를 입력해 추측한다.
  - S(Strike): 숫자와 자리가 모두 일치
  - B(Ball): 숫자는 있으나 자리가 다름
  - OUT: 일치하는 숫자가 하나도 없음
- 각 플레이어 3회 기회. 먼저 3S 달성 시 승리, 전원 소진 시 무승부. 이후 자동 리셋.
- 예) 정답 386 → `167`은 `0S 1B`, `386`은 `3S 0B`(승리).

## 실행 방법

1. `BaseballGame.uproject` 우클릭 → Generate Visual Studio project files → 빌드.
2. 에디터 툴바 Play → Net Mode `Play As Client`, Number of Players `2` → Play.
3. 정답은 Output Log에 `[BaseballGame] Secret Number = ###`로 출력된다(테스트용).

## 프로젝트 구조

| 클래스 | 부모 | 역할 |
|---|---|---|
| `ABGGameModeBase` | `AGameModeBase` | 정답 생성·입력 검증·판정·승패·리셋 (서버 권위) |
| `ABGGameStateBase` | `AGameStateBase` | 접속 알림 NetMulticast |
| `ABGPlayerController` | `APlayerController` | 입력 처리, Server/Client RPC, 공지 |
| `ABGPlayerState` | `APlayerState` | 플레이어 이름·시도 횟수(복제) |
| `UBGChatInput` | `UUserWidget` | 채팅 입력 위젯 |
| `BGFunctionLibrary` | - | 출력/네트워크 헬퍼 |

## 핵심 설계

- 서버 권위: 정답과 모든 판정은 GameMode(서버)에만 존재하므로 클라이언트가 조작할 수 없다.
- 전달 방식 구분
  - 채팅·판정 결과·안내: RPC (Server/Client), 접속 알림: NetMulticast
  - 시도 횟수·공지: Property Replication (`CurrentGuessCount`, `NotificationText`)
- 데이터 흐름

  ```
  [클라] 입력 → ServerRPC → [서버] ProcessGuess
    → 검증/기회 검사 (실패 시 본인에게만 안내)
    → 횟수 증가(복제) → 판정 → 전원에게 결과 방송(ClientRPC)
    → 승패 판정 → 공지(NotificationText 복제) → 리셋
  ```

- 표시용 `Player1(1/3)`은 서버가 횟수를 증가시킨 직후 직접 조립해 방송하므로 복제 지연 없이 즉시 반영된다.

## 구현 기능

- 멀티플레이 채팅(RPC)
- 정답 3자리 난수 생성, S/B/OUT 판정
- 입력 검증(3자리·숫자·중복) 사유별 안내 및 기회 소진 방어
- PlayerState 기반 시도 횟수 관리 및 표시
- 승리·무승부 공지 위젯, 게임 자동 리셋
