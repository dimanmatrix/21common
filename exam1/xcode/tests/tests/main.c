//
//  main.c
//  tests
//
//  Created by Dmitry on 9/23/24.
//

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 20

#define FALSE 0
#define TRUE 1

typedef char bool;

void my_strcpy(char *dest, const char *src) {
	while (*src != '\0')
	{
		*dest = *src;
		dest++;
		src++;
	}
	*dest = '\0';
}

size_t my_strlen(const char *str) {
	const char *s;
	
	if (str == NULL)
	{
		return 0;
	}
	s = str;
	while(*s)
	{
		++s;
	}
	return (s - str);
}

bool has_eol_last_symbol(const char *str) {
//	const char *s;
	size_t len;
	
	len = my_strlen(str);
	if (str == NULL || len == 0)
	{
		return (FALSE);
	}
//	s = str;
//	printf("%s\n", str);
//	printf("%d%c", len, *(str + len - 1));
	if(*(str + len - 1) == '\n')
	{
		return (TRUE);
	}
	return (FALSE);
//	while(*s)
//	{
//		++s;
//		if(*s == "\n")
//		{
//			return (TRUE);
//		}
//	}
//	return (FALSE);
}

bool change(char **str, char *new) {
	size_t new_len;
	
	if (str == NULL || *str == NULL || new == NULL) {
		return (FALSE);
	}
	new_len = my_strlen(new);
	char *new_str = realloc(*str, (new_len + 1) * sizeof(char));
	if(new_str == NULL)
	{
		return (FALSE); // В случае ошибки realloc, старая память остается неизменной
	}
	my_strcpy(new_str, new);
	*str = new_str;
	printf("change. %s\n", *str);
	return (TRUE);
}

bool read_string(char **str, int i) {
	
	char	buffer[BUFFER_SIZE];
	char	*tmp;
	char	*new_str;	// собираем здесь все что считали
	ssize_t	bytes_read;
	size_t	total_read;
	
	total_read = 0;
	new_str = NULL;
	while(1)
	{
		bytes_read = read(0, buffer, BUFFER_SIZE - 1);
//		if(i == 1)
//		{
//			bytes_read = 6;
//			my_strcpy(buffer, "hello\n\0");
//		}
//		else
//		{
//			bytes_read = 16;
//			my_strcpy(buffer, "ZZZZZZZZZZZZZZZ\n\0");
//		}
		tmp = realloc(new_str, (total_read + bytes_read + 1) * sizeof(char));
		if (bytes_read == -1 || tmp == NULL) {
			free(new_str);
			return FALSE;
		}
		new_str = tmp;
		buffer[bytes_read] = '\0';
		my_strcpy(new_str + total_read, buffer);
		total_read += bytes_read;
		if(has_eol_last_symbol(new_str))
		{
			*(new_str + total_read - 1) = '\0'; // заменяем перевод строки
			free(*str);
			*str = new_str;
//			my_strcpy(*str, new_str);
			break;
		}
	}
	
	printf("after. %s\n", *str);
	return (TRUE);
}


void print_memory_address(char *str) {
	printf("Адрес str: %p\n", (void*)str);
}


int main(void) {
	char	*str;
	bool	res;
	
//	printf("sizeof size_t. %lu\n", sizeof(size_t));
//	printf("sizeof ssize_t. %lu\n", sizeof(ssize_t));
	//	printf("sizeof bool. %lu\n", sizeof(bool));
	//	printf("sizeof char. %lu\n", sizeof(char));
	//	printf("sizeof short. %lu\n", sizeof(short));
	//	printf("sizeof int. %lu\n", sizeof(int));
	
	
	str = calloc(20, sizeof(char));
	my_strcpy(str, "TEST");
	printf("start. %s\n", str);
	print_memory_address(str);
	
	int i;
//	for(i=0;i<1000000;i++)
//	{
		printf("input: ");
		res = read_string(&str, 1);
		printf("after2. %s\n", str);
		print_memory_address(str);
		
		char *t;
		t = malloc(10 * sizeof(char));
		my_strcpy(t, "AAABBBCCCDD");
		printf("t1. %s\n", t);
		free(t);
		
//		res = read_string(str, 2);
//		printf("after3. %s\n", str);
//		print_memory_address(str);
		
		res = change(&str, "NEW");
		//	printf("4res=%d\n", res);
		
		printf("final. %s\n", str);
//	}
}


