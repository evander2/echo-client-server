#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <thread>
#include <mutex>
#include <set>

std::set<int> sdset;
std::mutex sdset_lock;

void usage(void){
    	printf("echo-server:\n");
	printf("syntax : echo-server <port> [-e[-b]]\n");
    	printf("sample : echo-server 1234 -e -b\n");
}

struct Param {
	bool e{false};
	bool b{false};
	uint16_t port{0};

	bool parse(int argc, char* argv[]) {
		for (int i = 1; i < argc; i++) {
			if (!strcmp(argv[i], "-e")) {
				e = true;
				continue;
			}
			if (!strcmp(argv[i], "-b")) {
                                b = true;
                                continue;
                        }
			port = atoi(argv[i]);
		}
		return port != 0;
	}
} param;



void clientThread(int sd){
    	
	printf("%d connected(socket descriptor number)\n", sd);
	static const int BUFSIZE = 65536;
	char buf[BUFSIZE];
	while (true) {
		ssize_t res = ::recv(sd, buf, BUFSIZE - 1, 0);
		if (res == 0 || res == -1) {
			fprintf(stderr, "recv return %ld", res);
			perror(" ");
			break;
		}
		buf[res] = '\0';
		printf("%s", buf);
		fflush(stdout);

		if (!param.b && param.e) {
			res = ::send(sd, buf, res, 0);
			if (res == 0 || res == -1) {
				fprintf(stderr, "send return %ld", res);
				perror(" ");
				break;
			}
		}
		if (param.b) {
			sdset_lock.lock();
			for(int s : sdset){
				res = ::send(s, buf, res, 0);
				if (res == 0 || res == -1) {
					fprintf(stderr, "send return %ld", res);
					perror(" ");
					break;
				}
			}
			sdset_lock.unlock();

		}
	}
	printf("%d disconnected\n", sd);
	sdset_lock.lock();
	sdset.erase(sd);
	sdset_lock.unlock();
	::close(sd);
}



int main(int argc, char* argv[]){


	if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}

	int sd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
		return -1;
	}

	int res;
	int optval = 1;
	res = ::setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (res == -1) {
		perror("setsockopt");
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(param.port);

	ssize_t res2 = ::bind(sd, (struct sockaddr *)&addr, sizeof(addr));
	if (res2 == -1) {
		perror("bind");
		return -1;
	}
	
	res = listen(sd, 5);
	if (res == -1) {
		perror("listen");
		return -1;
	}


	while (true) {
		struct sockaddr_in cli_addr;
		socklen_t len = sizeof(cli_addr);
		int cli_sd = ::accept(sd, (struct sockaddr *)&cli_addr, &len);
		if (cli_sd == -1) {
			perror("accept");
			break;
		}
		sdset_lock.lock();
		sdset.insert(cli_sd);
		sdset_lock.unlock();
		std::thread* t = new std::thread(clientThread, cli_sd);
		t->detach();
	}
	::close(sd);
    	
    
    	return 0;
}
