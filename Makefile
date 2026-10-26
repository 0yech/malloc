HOSTTYPE	= $(shell uname -m)_$(shell uname -s)

NAME      	= libft_malloc_$(HOSTTYPE).so
LINK		= libft_malloc.so

CC			= cc
CFLAGS		= -Wextra \
			-Werror \
			-Wall \
			-g \
			-fPIC \
			-pthread

LDFLAGS		= -shared \
			  -pthread

RM       	= rm
RMFLAGS   	= -f

SRCDIR    	= ./src
CFILES    	= $(SRCDIR)/allocator.c \
			  $(SRCDIR)/block.c \
			  $(SRCDIR)/debug.c \
			  $(SRCDIR)/free.c \
			  $(SRCDIR)/freelist.c \
			  $(SRCDIR)/physlist.c \
			  $(SRCDIR)/realloc.c \
			  $(SRCDIR)/returnable.c \
			  $(SRCDIR)/state.c \
			  $(SRCDIR)/utils.c \
			  $(SRCDIR)/zones.c
			  
OBJS      	= $(CFILES:.c=.o)

INCLDIR   	= include/
IFILES    	= $(INCLDIR)xalloc.h

all:      	$(NAME)

%.o:		%.c $(IFILES)
			@$(CC) $(CFLAGS) -I$(INCLDIR) -c $< -o $@

$(NAME): $(OBJS)
			@$(CC) $(LDFLAGS) $(OBJS) -o $(NAME)
			@ln -sf $(NAME) $(LINK)
			@printf "$(NAME) built successfully\n"

clean:		
			@$(RM) $(RMFLAGS) $(OBJS)
			@printf "xalloc cleaned successfully\n"

fclean:		clean
			@$(RM) $(RMFLAGS) $(NAME) $(LINK)
			@printf "library cleaned successfully\n"

re:			fclean all

.PHONY: all clean fclean re
