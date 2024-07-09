#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

#define CLIENT_NBR 1024
#define BUFFER_SIZE 4096

typedef	struct s_client {
	char*	buffer_in;
	char*	buffer_out;
	int		id;
}	t_client;

static t_client*	clients[CLIENT_NBR] = {0};
static fd_set		rfds, wfds, fds_models = {0};
static int			max_fd = 0, passive_sock = -1;

char*	str_join(char* dest, char* append) {
	size_t	l_dest = 0, l_append = 0;
	char*	new_buffer = NULL;

	if (append == NULL || append[0] == '\0')
		return (dest);
	if (dest == NULL)
		l_dest = 0;
	else
		l_dest = strlen(dest);
	l_append = strlen(append);
	new_buffer = calloc(l_dest + l_append + 1, 1);
	if (new_buffer == NULL)
		return (NULL);
	if (dest) {
		strcpy(new_buffer, dest);
		free(dest);
	}
	strcpy(new_buffer + l_dest, append);
	return (new_buffer);
}

int	extract_msg(char** buffer_in, char** msg) {
	size_t	i = 0;
	char*	new_buffer_in = NULL;

	if (*buffer_in == NULL || msg == NULL)
		return (0);
	*msg = NULL;
	while ((*buffer_in)[i]){
		if ((*buffer_in)[i] == '\n') {
			new_buffer_in = malloc(1 + strlen(*buffer_in) - (i + 1)); 
			if (new_buffer_in == NULL)
				return (1);
			*msg = *buffer_in;
			(*buffer_in)[i] = '\0';
			strcpy(new_buffer_in, (*buffer_in) + i + 1);
			(*msg)[i] = '\n';
			(*msg)[i + 1] = '\0';
			*buffer_in = new_buffer_in;
			return (1);
		}
		i++;
	}
	return (0);
}

void	free_client(int fd) {
	if (clients[fd]) {
		if (fd >= 0)
			close (fd);
		if (clients[fd]->buffer_in) {
			free(clients[fd]->buffer_in);
			clients[fd]->buffer_in = NULL;
		}
		if (clients[fd]->buffer_out) {
			free(clients[fd]->buffer_out);
			clients[fd]->buffer_out = NULL;
		}
		free (clients[fd]);
		clients[fd] = NULL;
	}
}

void	free_clients() {
	for (int i = 0; i < CLIENT_NBR; i++) {
		if (clients[i])
			free_client(i);
	}
}

void	error(char* msg) {
	if (msg) {
		write(2, msg, strlen(msg));
	} else {
		write(2, "Fatal error\n", strlen("Fatal error\n"));
	}
	free_clients();
	if (passive_sock >= 0)
		close(passive_sock);
	exit(1);
}

int	send_all(char* msg, int except) {
	if (msg == NULL)
		return (0);
	for (int fd = 0; fd < max_fd; fd++) {
		if (clients[fd] && fd != passive_sock && fd != except) {
			clients[fd]->buffer_out = str_join(clients[fd]->buffer_out, msg);
			if (clients[fd]->buffer_out == NULL)
				error(NULL);
		}
	}
	return (0);
}

char*	format_client_left(int fd) {
	static char	buffer[64];

	sprintf(buffer, "server: client %d just left\n", clients[fd]->id);
	return (buffer);
}

char*	format_new_client(int fd) {
	static char	buffer[64];

	sprintf(buffer, "server: client %d just arrived\n", clients[fd]->id);
	return (buffer);
}

char*	format_msg_client(char* msg, int fd) {
	static char	buffer[BUFFER_SIZE];

	sprintf(buffer, "client %d: %s", clients[fd]->id, msg);
	return (buffer);
}

int	create_passive_sock(int port) {
	struct sockaddr_in	addr = {0};
	
	passive_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (passive_sock < 0)
		return (1);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(passive_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		return (1);
	if (listen(passive_sock, 10) < 0)
		return (1);
	FD_SET(passive_sock, &fds_models);
	max_fd = passive_sock + 1;
	return (0);
}

int	handleNewClient() {
	static int	client_id = 0;

	int	sock = accept(passive_sock, NULL, NULL);
	if (sock < 0)
		return (0);
	if (sock + 1 > max_fd)
		max_fd = sock + 1;
	clients[sock] = calloc(1, sizeof(t_client));
	if (clients[sock] == NULL)
		error(NULL);
	clients[sock]->id = client_id++;
	FD_SET(sock, &fds_models);
	send_all(format_new_client(sock), sock);
	return (0);
}

int	handleClientLeft(int fd) {

	FD_CLR(fd, &fds_models);
	send_all(format_client_left(fd), fd);
	free_client(fd);
	return (0);
}

int	handleReadEvent(int fd) {
	char	buffer[BUFFER_SIZE + 1];
	int		nread = 0;

	if (fd == passive_sock) {
		handleNewClient();
		return (0);
	}
	nread = recv(fd, buffer, BUFFER_SIZE, 0);
	if (nread <= 0) {
		handleClientLeft(fd);
		return (0);
	}
	buffer[nread] = '\0';
	clients[fd]->buffer_in = str_join(clients[fd]->buffer_in, buffer);
	if (clients[fd]->buffer_in == NULL)
		error(NULL);
	char*	msg = NULL;
	while (extract_msg(&clients[fd]->buffer_in, &msg)) {
		if (msg == NULL)
			error(NULL);
		send_all(format_msg_client(msg, fd), fd);
		free(msg);
	}
	// printf("len = %lu\n", strlen(clients[fd]->buffer_in));
	// printf("bufferin=|%s|\n", clients[fd]->buffer_in);
	return (0);
}

int	handleWriteEvent(int fd) {
	if (clients[fd] == NULL || clients[fd]->buffer_out == NULL)
		return (0);
	if (send(fd, clients[fd]->buffer_out, strlen(clients[fd]->buffer_out), 0) < 0) {
		handleClientLeft(fd);
		return (0);
	}
	free(clients[fd]->buffer_out);
	clients[fd]->buffer_out = NULL;
	return (0);
}

int	event_loop() {
	while (1) {
		rfds = wfds = fds_models;
		FD_CLR(passive_sock, &wfds);
		select(max_fd, &rfds, &wfds, NULL, NULL);
		for (int fd = 0; fd < max_fd; fd++) {
			if (FD_ISSET(fd, &rfds))
				handleReadEvent(fd);
			if (FD_ISSET(fd, &wfds))
				handleWriteEvent(fd);
		}
	}
	return (0);
}

int	main(int argc, char** argv) {
	if (argc != 2) {
		error("Invalid number of arguments\n");
	}
	if (create_passive_sock(atoi(argv[1])))
		error(NULL);
	if (event_loop())
		error(NULL);
	free_clients();
	close(passive_sock);
	return (0);
}

