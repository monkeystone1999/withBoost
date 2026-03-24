 1. Image 를 받을 것임 한번에 5개가 연속해서 들어옴 각 Image 를 나누는 기준은 Frame Index 로 구분을 함 매 Image 마다의 들어오는 값은 이러함
 0a 88 00 00 00 <- 이게 들어온다 PacketHeader 를 보면 0a = IMAGE Type 이고 나머지는 Length 인 것을 볼 수 있다. 그리고 이미지 값들이 들어온다 일단 ip 를 기점으로 camera id 에 맞게 queue 형태로 집어넣는다. 그리고 UI 에 표현이 가능해야함 최대 20개까지 가능해야하고 
Time Stamp 를 찍어야하는데 Qt 에서 시간을 찍어서 Frame 위에 표현하고 AI Page 의 WheelHandler 에 쭉 나열을 할 것임 주석에 있는 내용처럼 // Wheel scroll: up=newer, down=older
본디 Header 를 읽고 Body 읽고 끝나는 흐름과는 다르게 동작을 하는데 Header -> Header -> Body 이런식에 가깝다. 첫번째 0a 88 00 00 00 이 값은 {"device_id":"SubPi_192.168.0.43","frame_index":0,"ip":"192.168.0.43","jpeg_size":25120,"timestamp_ms":8,"total_frames":5,"track_id":1} 이 json 형태의 크기를 의미하고 jpeg_size 가 Byte 단위의 추후에 들어오는 데이터들의 양을 의미한다.


8,g0E
@@:hN Ho< ]P{"device_id":"SubPi_192.168.0.43","frame_index":0,"ip":"192.168.0.43","jpeg_size":25120,"timestamp_ms":8,"total_frames":5,"track_id":1}JFIFC

	!"$"$C"	

}!1AQa"q2#BR$3br	
%&'()*456789:CDEFGHIJSTUVWXYZcdefghijstuvwxyz	

w!1AQaq"2B	#3Rbr
$4%&'()*56789:CDEFGHIJSTUVWXYZcdefghijstuvwxyz?p)'4* Q.04R)NsIX(&
a)I@ h4iH( tc;PKE%!=)sHM 4q-BqKMa@
ai)XZ,+\ciy4 w#zLnM4zSH
(PFiES/Z`jF;p2q*(bP+ qNeiR
q>,I=?:n(y~wLKyAgbL+WEn _q[p087jW'$Nq!\*A2sS13#Ji4p2iI=
^3M@0HfrqJIJwsM{qn%ltu$v=mN	MPq)ASIM!4r
084l4uh
)9L'9tL$-mb!9"31f8U	S[y9XS74
ZA<zREM1XSIGC{7 )
)=Z4KJF@ozp4HO))HM1	M=iza4s@n9@ojcs<

