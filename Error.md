이 문서가 내 프로젝트에 명확히 접합이 되는지 확인하고 프로젝트 파일별 명확히 뭘 해야할지를 명시된 요구사항 보고서를 만들어

1. 프로젝트 목표 (Objective)

FFmpeg의 하드웨어 디코딩(NV12) 결과물을 CPU 연산(libyuv, sws_scale) 없이 순수 YUV 포맷 그대로 GPU 메모리에 업로드하고, Qt 6 RHI 기반의 **커스텀 셰이더(Fragment Shader)**를 통해 렌더링 시점에 즉시 RGB로 변환하여 화면에 출력한다. (목표 CPU 점유율: 5% 미만)

2. 단계별 핵심 요구사항 (Core Requirements)

Phase 1: 비디오 데이터 공급부 수정 (Video.cpp)

요구사항 1-1. 변환 로직의 완전한 제거

기존에 사용하던 sws_scale 또는 libyuv 라이브러리 의존성과 관련 코드를 전면 삭제한다.

요구사항 1-2. NV12 Raw 데이터 추출 및 전달

GPU에서 다운로드한 swFrame (포맷: AV_PIX_FMT_NV12)의 Y 평면(Plane)과 UV 평면 데이터를 가공 없이 연속된 하나의 std::vector<uint8_t> 버퍼에 복사(Memcpy)한다.

onFrameReady 콜백을 통해 프레임 데이터 포인터, 해상도(width, height), 그리고 각 평면의 Stride(행의 바이트 길이) 정보를 함께 Qt 프론트엔드로 전달한다.

Phase 2: RHI 다중 텍스처 업로드 (VideoFrameTexture)

요구사항 2-1. 단일 텍스처에서 다중 텍스처로 분리

기존의 단일 QRhiTexture* rhiTex_ 구조를 버리고, 포맷에 맞게 텍스처를 분리한다.

NV12 지원 시: QRhiTexture* texY_ (포맷: QRhiTexture::R8), QRhiTexture* texUV_ (포맷: QRhiTexture::RG8) 총 2개 생성.

요구사항 2-2. 평면별(Plane) 독립 업로드 구현

commitTextureOperations 함수 내부에서 넘겨받은 원시 버퍼의 오프셋(Offset)을 계산하여, Y 데이터는 texY_에, UV 데이터는 texUV_에 각각 독립적으로 uploadTexture를 수행한다.

Phase 3: Qt 6 Scene Graph 커스텀 셰이더 구축 (신규 개발)

요구사항 3-1. 커스텀 QSGMaterial 클래스 개발

기본 제공되는 QSGImageNode를 폐기하고, 2개(또는 3개)의 텍스처 바인딩(Binding)을 지원하는 나만의 C++ 재질(Material) 클래스를 상속 및 구현한다.

요구사항 3-2. 커스텀 QSGGeometryNode 생성

렌더링될 사각형(Quad) 정점(Vertex)을 정의하고, 위에서 만든 커스텀 Material을 씌우는 노드 클래스를 작성한다.

VideoSurfaceItem::updatePaintNode가 이 커스텀 노드를 반환하도록 수정한다.

요구사항 3-3. 픽셀 셰이더(Fragment Shader) 작성 및 빌드

GLSL 또는 HLSL 문법을 사용하여, 샘플링된 Y값과 UV값을 수학적 변환 매트릭스(BT.601 또는 BT.709)를 통해 RGB로 변환하는 .frag 파일을 작성한다.

Qt 6의 Shader Tools (qsb)를 이용하여 셰이더 소스를 크로스 플랫폼에서 돌아가는 .qsb 파일로 컴파일한다.

3. 핵심 검색/학습 키워드 (Keywords for Research)

성공적인 개발을 위해 구글링 및 Qt 공식 문서에서 중점적으로 파고들어야 할 핵심 키워인인 목록 및 조건입니다.

FFmpeg 으로부터 받는 자동으로 설정되는 값들을 최대한 활용할 것 
하드코딩방식은 최대한 지양하며 필요시에 함수로 따로 빼내서 코드 리팩토링
해당 Decoding 을 하고 난뒤에 QML 및 Qt class 들이 제대로 받을 수 있는지 타입 및 호환성 확인
이미 만들어져있는 shaders 들의 정보가 현 프로젝트에 적합한지 확인
CMakeLists.txt 에서 제대로 찾고 있는지 확인
Buil 는 절대 금물 하지만 Debugging message 를 최대한 넣어 문제 개선에 일조
조조
🔑 비디오 디코딩 & 데이터 구조

FFmpeg AV_PIX_FMT_NV12 data layout (NV12 메모리 구조 이해)

YUV to RGB Conversion Matrix (BT.601 / BT.709 수학적 변환 공식)

🔑 Qt 6 Scene Graph (가장 중요)

Qt 6 Custom QSGMaterial (커스텀 재질 생성 기본 튜토리얼)

QSGMaterialShader (C++ 코드와 셰이더 변수를 연결하는 클래스)

QSGGeometryNode (화면에 사각형을 그리기 위한 기하 구조 노드)

Qt6 updatePaintNode custom material example

🔑 Qt 6 RHI & 셰이더 프로그래밍

QRhiTexture::R8 / QRhiTexture::RG8 (Y, UV 포맷용 1채널, 2채널 텍스처 포맷)

Qt Shader Tools (qsb) (Qt 6 환경에서의 셰이더 빌드 시스템)

Vulkan GLSL Sampler2D binding (셰이더 코드 내에서 여러 개의 텍스처를 받는 방법)