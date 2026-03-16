1. Group은 Merge 시 무조건 필요함, 허나 모든 CameraCard 는 Crop 정보와 각 CameraID 를 기반으로 동작하게 함 CameraWindow 하고 Synchronized 가 안되고 있음, Dashboard 의 CameraCard 의 정보가 바뀌면 CameraWindow 의 정보는 자동으로 CameraCard 와 Synchronize 되어서 바뀌어야함. Split 을 한 CameraCard 에 Split 되지 않은 다른 CameraCard 와 Swap 시 split 의 정보들 마저 교환이 되어서 다른 CameraCard 가 Split 이 되어버리고 기존의 CameraCard 의 Split 은 Split 되지 않은 다른 CameraCard 처럼 변해버림.
2. adminpage 의 Graph 는 나오는데 buffer 에 있는 값을 전부 출력하는 것이 아닌 가장 최근 값만 출력하게끔 되어있음 모든 기록을 Graph 로 표현 DevicePage 의 Graph 도 Buffer 에 있는 모든 값을 출력하는지지 확인

CameraCard 의 동작과 CameraWindow 의 동작은 이걸 지켜야함
Group과 Split 을 떠나서 모든 Card 는 서로서로 Swap 이 가능해야함
그렇기에 각 Card 는 Crop 이 되는 위치와 CameraId, group 을 가지고 있음
Card 가 swap 이 된다면 Crop, CameraId, Group 3가지 전부 swap 을 실행을 함
Card 가 바뀐다면 CameraWindow 도 바뀌어야함