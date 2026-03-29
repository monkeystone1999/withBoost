# Qml <-> QObject <-> ~Bridge <-> Manager <-> Engine



# Network Read it
MessageType::Image(Server -> Client){이미지 정보} -> MessageType::Image(Client -> Server){port 번호} -> MessageType::Image(Client -> Server) {loss 된 이미지 frame 및 sequence 번호}