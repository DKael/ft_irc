CXX=c++
CFLAGS=-std=c++98 -Wall -Wextra -Werror #-fsanitize=address -g3 
NAME=ircserv

# Create a variable for the object directory
OBJDIR=obj

# List of source files
SRCS =	main.cpp \
		Channel.cpp \
		Server.cpp \
		custom_exception.cpp \
		User.cpp \
		Message.cpp \
		Channel.cpp \
		string_func.cpp

# Generate a list of object files with the obj/ prefix
OBJS=$(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(NAME)

# Create the obj/ directory if it doesn't exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Update the pattern rule to place .o files in the obj/ directory
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -rf $(OBJDIR)
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
