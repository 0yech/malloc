NAME      	= xalloc
CC			= cc
CFLAGS		= -Wextra \
			-Werror \
			-g

RM       	= rm
RMFLAGS   	= -f

SRCDIR    	= ./src
CFILES    	= $(SRCDIR)/allocator.c \
			  $(SRCDIR)/block.c \
			  $(SRCDIR)/debug.c \
			  $(SRCDIR)/free.c \
			  $(SRCDIR)/freelist.c \
			  $(SRCDIR)/main.c \
			  $(SRCDIR)/physlist.c \
			  $(SRCDIR)/realloc.c \
			  $(SRCDIR)/returnable.c \
			  $(SRCDIR)/state.c \
			  $(SRCDIR)/utils.c \
			  $(SRCDIR)/zones.c \
			  
OBJS      	= $(CFILES:.c=.o)

INCLDIR   	= include/
IFILES    	= $(INCLDIR)xalloc.h

all:      	$(NAME)

%.o:		%.c $(IFILES)
			@$(CC) $(CFLAGS) -I$(INCLDIR) -c $< -o $@

$(NAME):	$(OBJS)
			@$(CC) $(CFLAGS) $(OBJS) -o $(NAME)
			@printf "xalloc built successfully\n"

clean:		
			@$(RM) $(RMFLAGS) $(OBJS)
			@printf "xalloc cleaned successfully\n"

re:			clean all

.PHONY:		all clean re
