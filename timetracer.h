#ifndef TIMETRACER_H
# define TIMETRACER_H
# include <sys/types.h>
# include <fcntl.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <time.h>
# include <pthread.h>
# define __USE_GNU
# define _GNU_SOURCE
# include <dlfcn.h>
# define	MAX_DEPTH 5
# define	ENTER 0
# define	EXIT 1
# ifndef POOL_SIZE
#  define POOL_SIZE 100000
# endif
# ifndef COLORS
#  define COLORS 1
# endif

typedef struct funcdata {
	void			*address;
	clock_t			time;
	char			type;
	char			pool_head;
	int				depth;
	struct funcdata	*next;
	struct funcdata	*last;
} t_funcdata;

typedef struct grouptimes {
	int					calls;
	struct grouptimes	*back;
}	t_grouptimes;

typedef struct dladdr_result {
	int						str_id;
	clock_t					time;
	char					type;
	t_grouptimes			*times;
	int						*times_len;
	clock_t					max;
	clock_t					min;
	int						depth;
	struct dladdr_result	*right;
	struct dladdr_result	*inside;
	struct dladdr_result	*next;
}	t_dlret;

typedef struct string_storage {
	char					*str;
	struct string_storage	*next;
	struct string_storage	*last;
}	t_strstore;

typedef struct strptr_storage {
	char					*str;
	void					*ptr;
	struct strptr_storage	*next;
}	t_strptrstore;

//	for pthread implementation

typedef struct threadlist {
	pthread_t			thread;
	struct threadlist	*next;
}	t_threadlist;

__attribute__((no_instrument_function)) static void	report(void);
__attribute__((no_instrument_function)) int		group_functions(t_dlret *func_info);

# define DEF_COLOR "\e[0;39m"
# define GRAY "\e[0;90m"
# define RED "\e[0;91m"
# define GREEN "\e[0;92m"
# define YELLOW "\e[0;93m"
# define BLUE "\e[0;94m"
# define MAGENTA "\e[0;95m"
# define CYAN "\e[0;96m"

#endif