# ⚾ BaseballGame — 언리얼 멀티플레이 숫자 야구

서버가 숨긴 **3자리 비밀 숫자**를 상대보다 먼저 맞히는 멀티플레이어 숫자 야구 게임입니다.
**데디케이티드 서버** 구조에서 **RPC**와 **Property Replication**으로 게임 상태를 동기화합니다.

- **엔진 버전**: Unreal Engine 5.5
- **언어/모듈**: C++ (`BaseballGame` 모듈, 클래스 접두사 `BG`)

---

## 🎮 게임 규칙

- 게임 시작 시 서버가 **1~9 사이, 서로 중복되지 않는 3자리** 정답을 생성합니다.
- 플레이어는 채팅창에 3자리 숫자를 입력해 추측합니다.
- 판정 결과
  | 결과 | 의미 |
  |---|---|
  | **S (Strike)** | 숫자와 자리가 모두 일치 |
  | **B (Ball)** | 숫자는 있으나 자리가 다름 |
  | **OUT** | 일치하는 숫자가 하나도 없음 |
- 예시 (정답 `386`): `127` → OUT, `167` → `0S 1B`, `386` → `3S 0B` (승리!)
- **승리**: 3번의 기회 안에 먼저 `3S` 달성
- **무승부**: 모든 플레이어가 3회를 모두 소진했는데도 정답자가 없을 때
- 승리/무승부 시 정답을 새로 생성하고 시도 횟수를 초기화하여 **자동 리셋**됩니다.

---

## 🕹 실행 방법

1. `BaseballGame.uproject` 우클릭 → **Generate Visual Studio project files**
2. `.sln`을 열어 **Development Editor / Win64** 구성으로 빌드 (또는 `.uproject` 더블클릭 후 rebuild)
3. 에디터 툴바 **Play ▾ → Advanced Settings**
   - **Net Mode**: `Play As Client` (데디케이티드 서버 + 클라이언트)
   - **Number of Players**: `2`
4. **Play** → 클라이언트 창 2개에서 각각 숫자 입력 후 Enter

> 💡 정답은 화면이 아니라 **Output Log**에 `[BaseballGame] Secret Number = ###`로 출력됩니다(테스트용).

---

## 🧩 프로젝트 구조

```
Source/BaseballGame/
├─ BGFunctionLibrary.h          # 출력/네트워크 헬퍼 (C++ 라이브러리)
├─ Game/
│  ├─ BGGameModeBase.*          # 정답 생성·입력 검증·S/B/OUT 판정·승패·리셋 (서버 권위)
│  └─ BGGameStateBase.*         # 접속 알림 NetMulticast
├─ Player/
│  ├─ BGPlayerController.*      # 입력 처리, Server/Client RPC, 공지(NotificationText)
│  └─ BGPlayerState.*           # 플레이어 이름·시도 횟수(복제), 표시용 Getter
└─ UI/
   └─ BGChatInput.*             # 채팅 입력 위젯 베이스(UEditableTextBox 바인딩)
```

| 클래스 | 부모 | 존재 위치 | 역할 |
|---|---|---|---|
| `ABGGameModeBase` | `AGameModeBase` | 서버 only | 게임 규칙 전체 제어 |
| `ABGGameStateBase` | `AGameStateBase` | 서버+클라 | 접속 알림 멀티캐스트 |
| `ABGPlayerController` | `APlayerController` | 서버+소유 클라 | 입력/통신/공지 |
| `ABGPlayerState` | `APlayerState` | 서버+모든 클라 | 플레이어 공개 데이터 |
| `UBGChatInput` | `UUserWidget` | 클라 UI | 채팅 입력 |

---

## 🔑 핵심 설계

### 1. 서버 권위 (Server Authority)
정답 생성과 모든 판정은 **GameMode(서버)** 에서만 수행됩니다. 정답(`SecretNumberString`)은 서버에만 존재하므로 클라이언트가 훔쳐보거나 조작할 수 없습니다.

### 2. RPC vs Property Replication
| 데이터 | 전달 방식 |
|---|---|
| 채팅/판정 결과/안내 메시지 | **RPC** (`ServerRPC` → 처리 → `ClientRPC`) |
| 접속 알림 | **NetMulticast RPC** |
| 시도 횟수 `(x/3)` | **Property Replication** (`PlayerState::CurrentGuessCount`) |
| 승리/무승부 공지 | **Property Replication** (`PlayerController::NotificationText`) + UMG 바인딩 |

### 3. 데이터 흐름
```
[클라] 채팅 입력 (WBP_ChatInput, Enter)
  └▶ BGPlayerController::SetChatMessageString (로컬, 원문만 전송)
       └▶ ServerRPCPrintChatMessageString ───(Server RPC)──▶ [서버]
            └▶ BGGameModeBase::ProcessGuess
                 ├ 기회 소진 검사 / 입력 검증 → 실패 시 본인에게만 안내 (Client RPC)
                 ├ IncreaseGuessCount → CurrentGuessCount++ ──(복제)──▶ [클라] (x/3) 즉시 갱신
                 ├ JudgeResult → "1S 1B"
                 ├ BroadcastChatMessage ──(Client RPC, 전원)──▶ [클라] 화면 출력
                 └ JudgeGame
                      ├ 승리/무승부 → NotificationText ──(복제)──▶ [클라] 공지 위젯 자동 표시
                      └ ResetGame → 정답 재생성 + 전원 카운트 0
```

> **카운트 즉시 반영**: 표시용 문자열 `Player1(1/3)`은 클라가 아니라 **서버가 횟수를 증가시킨 직후 직접 조립**해 방송합니다. 그래서 복제 지연 없이 그 줄에서 바로 갱신됩니다.

---

## ✅ 구현 기능 (필수)

- [x] 기본 멀티플레이 채팅 (RPC 기반)
- [x] 정답 3자리 난수 생성 (서버, 1~9 중복 없음)
- [x] 입력 검증 (3자리 / 숫자 / 중복) + **사유별 안내 메시지**
- [x] S / B / OUT 판정 로직
- [x] PlayerState 기반 시도 횟수 관리 (기본 3회)
- [x] 현재/최대 시도 횟수 표시 (`GetPlayerInfoString`)
- [x] 승리 판정 + 공지 위젯
- [x] 무승부 판정 + 공지 위젯
- [x] 게임 리셋 (정답 재생성 + 데이터 초기화)
- [x] C++ 라이브러리(`BGFunctionLibrary`) 작성

---

## 🛡 예외 처리 (방어 코드)

| 상황 | 처리 |
|---|---|
| 3자리가 아님 (`12`) | "세 자리 숫자를 입력해주세요." · 기회 미소진 |
| 숫자 아님 / 0 포함 (`abc`, `102`) | "1~9 사이의 숫자만 입력할 수 있어요." · 기회 미소진 |
| 중복 숫자 (`112`) | "중복되지 않은 숫자를 입력해주세요." · 기회 미소진 |
| 기회 0회인데 입력 | "기회를 모두 소진했습니다. 다음 라운드를 기다려주세요." · 무시 |

---

## 🚧 미구현 / TODO (도전 기능)

- [ ] 턴제 규칙 (턴 전환, 활동 체크)
- [ ] 실시간 타이머 위젯 + 서버 시간 동기화
- [ ] 시간 초과 시 기회 소진 / 입력 차단
