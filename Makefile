main: 
	cc -Wall -Wextra -Werror -lcurl -lpthread -o c main.c

fclean:
	rm -f c
