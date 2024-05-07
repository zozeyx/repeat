#include "Common.h"

char *SERVERIP = (char *)"127.0.0.1"; 

#define SERVERPORT 9000     
#define BUFSIZE    512      
#define ACK_SIZE	5

int packet[9] = {0,1,2,3,4,5,6,7,8};	//패킷선언
char packet_str[9][20];
int sender_window = 4;	//한번에 응답없이 보낼 수 있는 패킷 개수
int h = 0;
int t = -1;
int ackCount = -1;	//받은 ack의 숫자. 하나씩 받을 때 마다 1개씩 증가한다. packet 0에 대한 ack을 받았으면 ackCount = 0으로 증가
char ack[100];
int timeCount[9] = {0};
int ackRecvCheck[9] = {0};	//보낸 패킷에 대한 ack을 받았는지 확인. 받았으면 1로 표시.
int breakCon = 0;	//반복문을 빠져나올 스위치. 1이면 빠져나온다.
int willAckNumber = 0;
int receivedAckNumber = -1;
char ack_str[9][20];

ssize_t recv_fixlen(int sockfd, char *buf, size_t len, int flags) {     //패킷 데이터가 정확하게 버퍼에 안 떨어지기 때문에 while문 돌리면서 정확하게 8바이트가 떨어질때까지 돌린다.
	int received_len = 0;
	int ret;
	while(received_len < len) {
		ret = recv(sockfd, &buf[received_len], len - received_len, flags);
		if (ret == SOCKET_ERROR) {
			return ret;
		} else if (ret == 0) { // 클라이언트가 연결을 종료한 경우 반복문 종료
			return ret;
		} else {
			received_len += ret;
			buf[received_len] = '\0';
		}
	}
}

int main(int argc, char *argv[]){

	for (int i = 0; i < 9; i++) {	//packet_str에 packet 0 이런식으로 저장
        sprintf(packet_str[i], "packet %d", packet[i]);
    }
	for(int i = 0; i < 9; i++){
		sprintf(ack_str[i], "ACK %d", i);
	}

	int retval;

	if (argc > 1) SERVERIP = argv[1];

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0); 
	if (sock == INVALID_SOCKET) err_quit("socket()");

	struct sockaddr_in serveraddr;  
	memset(&serveraddr, 0, sizeof(serveraddr));     
	serveraddr.sin_family = AF_INET;    
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);   
	serveraddr.sin_port = htons(SERVERPORT);    

	retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));    
	if (retval == SOCKET_ERROR) err_quit("connect()");  

	//우선 먼저 4개를 보냄
	for(int i=0; i<sender_window; i++){		//4개를 보내는데 보낼때 마다 각 패킷의 timeCount++
		t++;
		retval = send(sock, packet_str[t], strlen(packet_str[t]), 0);
    	if (retval == SOCKET_ERROR) {
   			err_display("send()");
        	break;
    	}

		for(int j=0; j<=t; j++){
			if(ackRecvCheck[j]==1){		//ack를 받았으면 그에 해당하는 timeCount는 증가하지 않도록 한다.
				continue;
			}
			timeCount[j]++;
			if(timeCount[j]>4){
				printf("\"%s\" is timeout.\n", packet_str[j]);
				break;
			}
		}
		printf("\"%s\" is transmitted.\n", packet_str[t]);		//한번 보낼때마다 출력함
	}	

	for(int i=0; i<5; i++){
		// 데이터 받기
		char buf[BUFSIZE + 1];

		retval = recv_fixlen(sock, buf, ACK_SIZE, 0);  	//상대가 보낸 데이터 ack을 버퍼에 저장
		if (retval == SOCKET_ERROR) {   
			err_display("recv()");
			break;
		} else if (retval == 0) {   
			break;
		} else {	//받아야할 ack넘버랑 받은 ack넘버가 같으면 
			if(strcmp(ack_str[willAckNumber], buf)==0){
				strcpy(ack,buf);
				printf("\"%s\" is received.\n", ack);	
				ackRecvCheck[willAckNumber] = 1;
				willAckNumber++;
				h++;
				t++;
				retval = send(sock, packet_str[t], strlen(packet_str[t]), 0);	//	무사히 ack을 받았다면 다음 패킷을 보낸다.
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
				printf("\"%s\" is transmitted.\n", packet_str[t]);		// 패킷을 보내고 출력

			} else {
				printf("\"%s\" is recevied and recorded.\n", buf);		// 패킷을 보내고 출력
				
			}
			
			for(int j=0; j<=t; j++){		//ackRecvCheck배열을 현재 패킷 배열을 가리키는 tail포인터가 위치하는 곳 까지 돌면서 받은 위치가 있는지 확인한다. (1인 부분)
				if(ackRecvCheck[j]==1){		//ack를 받았으면 그에 해당하는 timeCount는 증가하지 않도록 한다.
					continue;
				}
				timeCount[j]++;
				if(timeCount[j]>4){
					printf("\"%s\" is timeout.\n", packet_str[j]);		//만약 timeout된 패킷이 있다면 출력하고 breakCon을 1로 만든다 그리고 해당 for문을 빠져나온다.
					for (int p = j; p < sizeof(timeCount) / sizeof(timeCount[0]); p++) {
    					timeCount[p] = 0;
					}
					breakCon = 1;
					break;
				}
			}

			if(breakCon == 1){		//위에서 timeout이 발생하였기 때문에 큰 while문을 빠져나간다.
				break;
			}

		}		
	}
	/*
	for (int i = 0; i < 9; ++i) {
        printf("timeCount[%d] = %d\n", i, timeCount[i]);
		printf("ackRecv[%d] = {%d}\n", i, ackRecvCheck[i]);
		printf("----------------------\n");
    }
	*/
	
	for(int i=0; i<sender_window; i++){
		retval = send(sock, packet_str[willAckNumber], strlen(packet_str[willAckNumber]), 0);
    	if (retval == SOCKET_ERROR) {
   			err_display("send()");
        	break;
    	}

		printf("\"%s\" is transmitted.\n", packet_str[willAckNumber]);		//한번 보낼때마다 출력함
		break;
	}
	
	// 소켓 닫기
	sleep(1);
	close(sock);
	return 0;
}
