#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

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
	strcat(new_buffer, append);
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

int	main(void) {
	char*	buffer = strdup("This is a test\n fewf\need\ndefe\n");
	char*	msg = NULL;

	while (extract_msg(&buffer, &msg)) {
		printf("msg = |%s|\n", msg);
		free(msg);
	}
	// printf("buffer = |%s|\n", buffer);
	free(buffer);
	return (0);

}
