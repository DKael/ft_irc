# List of source files
SRCS =	main_bot.cpp \
		Bot.cpp \
		Message_bot.cpp \
		Message_rpl_bot.cpp \
		string_func_bot.cpp \
		util_bot.cpp



# Create a variable for the object directory
OBJDIR = obj

# Generate a list of object files with the obj/ prefix
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))

CXX = c++

CXXFLAGS = -std=c++98 -Wall -Wextra -Werror #-fsanitize=address -g3 

NAME = bot

DEBUG_FLAG = 0

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

# Create the obj/ directory if it doesn't exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Update the pattern rule to place .o files in the obj/ directory
ifeq ($(DEBUG_FLAG),1)
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c -D DEBUG -I. $< -o $@
else 
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c -I. $< -o $@
endif

all: $(NAME)

debug:
	make DEBUG_FLAG=1

clean:
	rm -rf $(OBJDIR)

fclean:
	make clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re debug
