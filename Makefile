SRCS =	main.cpp \
		Server.cpp \
		custom_exception.cpp \
		User.cpp \
		Message.cpp \
		string_func.cpp

OBJS = 	${SRCS:.cpp=.o}

TOTAL_OBJS = ${OBJS}

CXX = c++

CXXFLAGS = -std=c++98 # -fsanitize=address -g3 #-Wall -Wextra -Werror

NAME = ircserv

${NAME} : ${TOTAL_OBJS}
	${CXX} ${CXXFLAGS} ${TOTAL_OBJS} -o $@

%.o :%.cpp
	${CXX} ${CXXFLAGS} -c -I. $< -o $@

all : ${NAME}

clean:
	rm -f ${OBJS}

fclean:
	make clean
	rm -rf ${NAME}

re: 
	make fclean
	make all

.PHONY: all clean fclean re
