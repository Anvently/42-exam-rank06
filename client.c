#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

int	main(int argc, char** argv) {
	if (argc != 2) {
		printf("Invalid number of arguments\n");
		return (1);
	}
	struct sockaddr_in	addr = {0};
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return (1);
	if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
		return (1);
	// const char*	str = "Hi this is a wfbewufnewiuf weiufwe fuywebf uwefbweufb weufb weuf weufb wqeufb wufb wqefubwfwewegewgwegwgwegwef wef qwefwefywe wey y";
	char	str[100000];
	memset(str, 'n', 100000);
	while (1) {
		if (send(sock, str, 100000, 0) < 0) {
			close(sock);
			return (0);
		}
		// sleep(3);
	}
	close(sock);
}