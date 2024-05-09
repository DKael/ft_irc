SRCS =	main_bot.cpp \
		Bot.cpp \
		../Message.cpp \
		../string_func.cpp \
		../util.cpp \

OBJS = 	${SRCS:.cpp=.o}

TOTAL_OBJS = ${OBJS}

CXX = c++

CXXFLAGS = -std=c++98 -Wall -Wextra -Werror #-fsanitize=address -g3 

NAME = bot

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

bonus:
	@make WITH_BONUS=1

.PHONY: all clean fclean re
