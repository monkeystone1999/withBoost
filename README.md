### 1. theme/ (디자인 시스템 코어 - 단일 진실 공급원)

포함 파일: Theme.qml, Typography.qml, Icon.qml
역할 (Role): 프로젝트 전체의 시각적 요소(색상, 폰트 크기, 마진, 패딩, 애니메이션 속도 등)를 정의하는 **단일 진실 공급원(Single Source of Truth)**입니다.

### 2. components/ (멍청한 컴포넌트 / Presentational Components)

포함 하위 폴더: camera/, dashboard/, device/, user/
역할 (Role): 버튼, 카드, 텍스트 입력창 등 재사용 가능한 가장 작은 UI 블록입니다.

제약 사항 (매우 중요): 이 폴더의 파일들은 C++ 백엔드(QObject/Model)의 존재를 전혀 몰라야 합니다. 오직 부모가 넣어주는 Property 데이터만 받아서 수동적으로 화면을 그리는 역할만 해야 합니다. (이것을 "Dumb Component" 원칙이라 합니다.)
C++ 로직에 의존하는 코드를 모두 없애고, 오직 외부에서 열려있는 속성(Property)과 이벤트(Signal)만 호출하도록 업데이트했습니다.

### 3. pages/ (스마트 컴포넌트 / Pages)

포함 파일: DashboardPage.qml, SettingsPage.qml, LoginPage.qml 등
역할 (Role): 라우팅(화면 전환)의 대상이 되는 전체 화면 단위입니다. **C++ 백엔드와 직접 소통하는 "스마트 컴포넌트(Smart Component)"**의 역할을 맡습니다.
페이지 파일들에서 직접 UI를 그리는(Rectangle, Text 등을 마구 나열하는) 행위를 최소화해야 합니다.
대신 C++에서 주입된 데이터 모델(예: DashboardController, CameraListModel)을 가져와서 Layout과 Component 파일들에게 데이터를 '뿌려주는(Props Drilling)' 컨트롤 타워 역할만 수행하도록 코드를 정리합니다.

### 4. layouts/ (구조 및 배치 컨트롤러)

포함 파일: DashboardLayout.qml, CameraSplitLayout.qml 등
역할 (Role): 여러 개의 Component들을 모아서 특정 규칙(Grid, List, Split)에 맞게 화면에 배치하는 역할을 합니다.

### 5. Assets/ (정적 리소스)
포함 하위 폴더: Fonts/, Icons/, Logos/
역할 (Role): 변하지 않는 폰트 파일(.ttf)이나 벡터 그래픽(.svg)을 보관합니다.

### 6. Main.qml 및 TitleBar.qml (껍데기 / App Shell)
역할 (Role): 프로그램의 최상위 윈도우 프레임워크, 페이지 간의 내비게이션(Stackpages/SwipeView)을 관리하는 진입점(Entry Point)입니다.

cmake --preset anomap-ninja-debug

# build
cmake --build --preset anomap-ninja-debug

# configure + build (using workflow presets)
cmake --workflow --preset my-workflow