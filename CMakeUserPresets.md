# CMakeUserPresets.json 레퍼런스

`CMakeUserPresets.json`은 **개발자 개인** preset을 정의하는 파일입니다.  
`.gitignore`에 추가하여 저장소에 커밋하지 않는 것이 일반적입니다.  
`CMakePresets.json`의 hidden preset을 `inherits`로 재사용하여 중복 없이 구성합니다.

---

## CMakePresets.json과의 차이점

| 항목 | `CMakePresets.json` | `CMakeUserPresets.json` |
|---|---|---|
| 목적 | 팀 공유 공통 설정 | 개인 로컬 설정 |
| Git 커밋 | ✅ 커밋 | ❌ `.gitignore` 처리 |
| `include` 가능 | v4+ | v4+ |
| 공유 preset 참조 | — | `CMakePresets.json` preset을 `inherits` 가능 |

---

## 최상위 (Root) Properties

`CMakePresets.json`과 동일한 스키마를 공유합니다.

| Property | 타입 | 필수 | 설명 |
|---|---|---|---|
| `version` | integer | ✅ | 스키마 버전 (`CMakePresets.json`과 일치 권장) |
| `cmakeMinimumRequired` | object | ❌ | 최소 CMake 버전 요건 |
| `include` | string[] | ❌ | 외부 preset 파일 경로 (v4+) |
| `vendor` | object | ❌ | IDE/도구 전용 확장 데이터 |
| `configurePresets` | object[] | ❌ | configure 단계 preset |
| `buildPresets` | object[] | ❌ | build 단계 preset |
| `testPresets` | object[] | ❌ | CTest 단계 preset |
| `packagePresets` | object[] | ❌ | CPack 단계 preset (v6+) |
| `workflowPresets` | object[] | ❌ | 파이프라인 preset (v6+) |

---

## `configurePresets[]` Properties

| Property | 타입 | 필수 | 설명 |
|---|---|---|---|
| `name` | string | ✅ | preset 고유 식별자 (공백 불가) |
| `displayName` | string | ❌ | IDE/CLI UI 표시 이름 |
| `description` | string | ❌ | 설명 |
| `hidden` | boolean | ❌ | `true`이면 상속 전용, 직접 선택 불가 |
| `inherits` | string \| string[] | ❌ | 상속할 preset. `CMakePresets.json`의 hidden preset 포함 가능 |
| `condition` | object | ❌ | 활성화 조건 (OS, 환경변수 등) |
| `generator` | string | ❌ | 빌드 시스템 생성기 (상속으로 전파 가능) |
| `architecture` | string \| object | ❌ | 타겟 아키텍처 |
| `toolset` | string \| object | ❌ | 컴파일러 toolset |
| `binaryDir` | string | ❌ | 빌드 출력 경로 (매크로 사용 가능) |
| `installDir` | string | ❌ | install 대상 경로 |
| `toolchainFile` | string | ❌ | CMake toolchain 파일 |
| `cacheVariables` | object | ❌ | `-D` 캐시 변수. 값은 string 또는 `{type, value}` 객체 |
| `environment` | object | ❌ | 환경 변수 (`null`이면 해당 변수 제거) |
| `warnings` | object | ❌ | CMake 경고 옵션 |
| `errors` | object | ❌ | CMake 오류 처리 옵션 |
| `debug` | object | ❌ | 디버그 출력 옵션 |
| `vendor` | object | ❌ | 도구 전용 임의 데이터 |

### `cacheVariables` 값 형식

```jsonc
// 단순 문자열
"CMAKE_BUILD_TYPE": "Debug"

// 타입 명시 객체
"MY_FLAG": { "type": "BOOL", "value": "ON" }

// 제거 (null 불가 — 대신 빈 문자열 사용)
"MY_VAR": ""
```

### `environment` 값 형식

```jsonc
"environment": {
  "MY_VAR": "value",        // 설정
  "ANOTHER": null           // 상속된 값 제거
}
```

---

## `buildPresets[]` Properties

| Property | 타입 | 필수 | 설명 |
|---|---|---|---|
| `name` | string | ✅ | preset 고유 식별자 |
| `displayName` | string | ❌ | UI 표시 이름 |
| `description` | string | ❌ | 설명 |
| `hidden` | boolean | ❌ | 상속 전용 여부 |
| `inherits` | string \| string[] | ❌ | 상속할 buildPreset |
| `condition` | object | ❌ | 활성화 조건 |
| `configurePreset` | string | ❌ | 연결된 configurePreset 이름 |
| `inheritConfigureEnvironment` | boolean | ❌ | configurePreset 환경 변수 상속 여부 (기본 `true`) |
| `jobs` | integer | ❌ | 병렬 빌드 수 (`--parallel N`) |
| `targets` | string \| string[] | ❌ | 빌드할 타겟 |
| `configuration` | string | ❌ | 멀티-config 제너레이터용 구성 이름 (`Debug`, `Release`) |
| `cleanFirst` | boolean | ❌ | 빌드 전 clean 실행 |
| `verbose` | boolean | ❌ | 상세 출력 |
| `nativeToolOptions` | string[] | ❌ | 네이티브 도구 추가 인수 |
| `environment` | object | ❌ | 환경 변수 |
| `vendor` | object | ❌ | 도구 전용 데이터 |

---

## `testPresets[]` Properties

| Property | 타입 | 설명 |
|---|---|---|
| `name` | string | preset 식별자 |
| `configurePreset` | string | 연결된 configurePreset |
| `configuration` | string | 멀티-config 구성 |
| `output` | object | 출력 상세도, 진행 표시 등 |
| `filter` | object | `include` / `exclude` 테스트 필터 |
| `execution` | object | `jobs`, `timeout`, `stopOnFailure` 등 |
| `environment` | object | 환경 변수 |

---

## 이 파일의 Preset 구조

```
CMakePresets.json (공유)        CMakeUserPresets.json (개인)
─────────────────────────       ────────────────────────────────────
base (hidden)                   
 └── ninja (hidden)    ──────▶  ninja-debug   (configurePreset)
 └── msvc  (hidden)    ──────▶  ninja-release (configurePreset)
                                msvc-debug    (configurePreset)
                                msvc-release  (configurePreset)

                                ninja-debug   (buildPreset)
                                ninja-release (buildPreset)
                                msvc-debug    (buildPreset)
                                msvc-release  (buildPreset)
```

---

## 사용 예시

```bash
# configure
cmake --preset ninja-debug

# build
cmake --build --preset ninja-debug

# configure + build (workflow preset 사용 시)
cmake --workflow --preset my-workflow
```

---

## .gitignore 설정 권장

```gitignore
CMakeUserPresets.json
```

> 공식 문서: <https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html>
