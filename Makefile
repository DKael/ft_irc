SRCS =	main.cpp \
		Server.cpp \
		cutsom_exception.cpp\

OBJS = 	${SRCS:.cpp=.o}

TOTAL_OBJS = ${OBJS}

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98

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
