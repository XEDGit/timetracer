/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   timetracer.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lmuzio <lmuzio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/03/21 16:58:47 by lmuzio            #+#    #+#             */
/*   Updated: 2023/03/28 21:31:42 by lmuzio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#define __USE_GNU
#define _GNU_SOURCE
__attribute__((no_instrument_function)) static void	report(void);
#define	MAX_DEPTH 4
#define	POOL_SIZE 100000
unsigned long	base_address_offset = 0;
#ifdef __APPLE__
#define OFFSET_FUNC_NAME 1
# include <mach-o/dyld.h>

__attribute__((no_instrument_function)) void callback(const struct mach_header *info, intptr_t vmaddr_slide)
{
	if (!base_address_offset)
		base_address_offset = (unsigned long)vmaddr_slide;
}

__attribute__((no_instrument_function)) __attribute__((constructor)) void on_start(void)
{
    _dyld_register_func_for_add_image(&callback);
	atexit(&report);
}

#else
#define OFFSET_FUNC_NAME 0
# include <link.h>

__attribute__((no_instrument_function)) int callback(struct dl_phdr_info *info, size_t size, void *data)
{
	if (!base_address_offset)
		base_address_offset = (unsigned long)info->dlpi_addr;
    return 0;
}

__attribute__((no_instrument_function)) __attribute__((constructor)) void on_start(void)
{
    dl_iterate_phdr(&callback, 0);
	atexit(&report);
}

#endif
#include <dlfcn.h>


#define	ENTER 0
#define	EXIT 1

typedef struct funcdata {
	void			*address;
	clock_t			time;
	char			type;
	char			pool_head;
	int				depth;
	struct funcdata	*next;
	struct funcdata	*last;
} t_funcdata;

t_funcdata *data = 0;

typedef struct dladdr_result {
	int						str_id;
	clock_t					time;
	char					type;
	int						times;
	clock_t					max;
	clock_t					min;
	int						depth;
	struct dladdr_result	*right;
	struct dladdr_result	*inside;
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

int				depth = 0;
t_funcdata		*pool = 0;
int				pool_index = POOL_SIZE;

__attribute__((no_instrument_function)) static int add_node(t_funcdata *new, t_funcdata **og)
{
	if (*og)
	{
		t_funcdata *copy = *og;
		copy->last->next = new;
		copy->last = new;
	}
	else
	{
		new->last = new;
		*og = new;
	}
	return (0);
}

__attribute__((no_instrument_function)) static int	add_dlret(t_dlret tmp, t_dlret **og)
{
	static t_dlret	*curr_traversal[MAX_DEPTH + 1] = {0};
	static int		curr_depth = 0;
	if (tmp.str_id == -1)
		return (-1);
	t_dlret	*new = malloc(sizeof(t_dlret));
	if (!new)
		return (-1);
	new->inside = 0;
	new->right = 0;
	new->time = tmp.time;
	new->type = tmp.type;
	new->depth = tmp.depth;
	new->times = 1;
	new->str_id = tmp.str_id;
	if (*og)
	{
		t_dlret *copy;
		if (new->depth - 1 > curr_depth)
		{
			copy = curr_traversal[curr_depth++];
			copy->inside = curr_traversal[curr_depth] = new;
		}
		else
		{
			curr_depth = new->depth - 1;
			copy = curr_traversal[new->depth - 1];
			copy->right = curr_traversal[new->depth - 1] = new;
		}
	}
	else
	{
		curr_traversal[new->depth - 1] = new;
		*og = new;
	}
	return (0);
}

__attribute__((no_instrument_function)) static int	add_strstore(t_strstore tmp, t_strstore **og)
{
	t_strstore	*new = malloc(sizeof(t_strstore));
	if (!new)
	{
		return (-1);
	}
	new->next = 0;
	new->str = tmp.str;
	int iter = 0;
	if (*og)
	{
		t_strstore *copy = *og;
		while (copy->next)
		{
			copy = copy->next;
			iter++;
		}
		copy->next = new;
		return (iter + 1);
	}
	else
		*og = new;
	return (0);
}

__attribute__((no_instrument_function)) static int	search_strstore(const char *to_search, t_strstore *store)
{
	int	i = 0;
	while (store)
	{
		if (!strcmp(to_search, store->str))
			return (i);
		i++;
		store = store->next;
	}
	return (-1);
}

__attribute__((no_instrument_function)) static char	**copy_strstore(t_strstore *store)
{
	int	i = 0;
	t_strstore *copy = store;
	while (store)
	{
		i++;
		store = store->next;
	}
	char **return_array = malloc(sizeof(char *) * (i + 1));
	return_array[i] = 0;
	store = copy;
	i = 0;
	while (store)
	{
		return_array[i++] = store->str;
		copy = store->next;
		free(store);
		store = copy;
	}
	return (return_array);
}

__attribute__((no_instrument_function)) static int	add_strptrstore(t_strptrstore tmp, t_strptrstore **og)
{
	t_strptrstore	*new = malloc(sizeof(t_strptrstore));
	if (!new)
	{
		return (-1);
	}
	new->next = 0;
	new->str = tmp.str;
	new->ptr = tmp.ptr;
	int iter = 0;
	if (*og)
	{
		t_strptrstore *copy = *og;
		while (copy->next)
		{
			copy = copy->next;
			iter++;
		}
		copy->next = new;
		return (iter);
	}
	else
		*og = new;
	return (0);
}

__attribute__((no_instrument_function)) static void	manage_pool(void)
{
	if (pool_index == POOL_SIZE)
	{
		pool = malloc(sizeof(t_dlret) * POOL_SIZE);
		if (!pool)
			exit((perror("Malloc error"), 10));
		pool_index = 0;
		pool[pool_index].pool_head = 1;
	}
}

__attribute__((no_instrument_function)) void __cyg_profile_func_enter (void *called_fn, void *call_site)
{
	if (depth++ > MAX_DEPTH)
		return ;
	clock_t time = clock();
	manage_pool();
	if (pool_index)
		pool[pool_index].pool_head = 0;
	pool[pool_index].address = called_fn - base_address_offset;
	pool[pool_index].depth = depth;
	pool[pool_index].time = time;
	pool[pool_index].type = ENTER;
	pool[pool_index].next = 0;
	add_node(&pool[pool_index++], &data);
	(void) call_site;
}

__attribute__((no_instrument_function)) void __cyg_profile_func_exit (void *called_fn, void *call_site)
{
	if (--depth > MAX_DEPTH)
		return ;
	clock_t time = clock();
	manage_pool();
	if (pool_index)
		pool[pool_index].pool_head = 0;
	pool[pool_index].address = called_fn - base_address_offset;
	pool[pool_index].depth = depth + 1;
	pool[pool_index].time = time;
	pool[pool_index].type = EXIT;
	pool[pool_index].next = 0;
	add_node(&pool[pool_index++], &data);
	(void) call_site;
}

__attribute__((no_instrument_function)) static char	*get_next_line(FILE *f)
{
	char *s = malloc(1000);
	int i = 0;
	if (!s)
	{
		perror("Malloc fail: ");
		exit(1);
	}
	char ch = 1;
	while (ch)
	{
		ch = fgetc(f);
		if (ch == '\n' || ch == -1)
			break ;
		if (i == 999)
		{
			while ((ch = fgetc(f)) != '\n' && ch != 0);
			break ;
		}
		s[i++] = ch;
	}
	if (!i)
	{
		free(s);
		return (0);
	}
	s[i] = 0;
	return (s);
}

__attribute__((no_instrument_function)) static char	*find_symbol(void *addr)
{
	static t_strptrstore *db = 0;
	if (!db)
	{
		FILE *nm_out_stream = popen("nm --defined-only $(pwd)/test", "r");
		char *str_buff = 0, *str_copy;
		void *func_ptr;


		while ((str_buff = get_next_line(nm_out_stream)))
		{
			func_ptr = (void *)strtol(str_buff, 0, 16);
			str_copy = strchr(str_buff, ' ') + 3;
			add_strptrstore((t_strptrstore){.str = strdup(str_copy), .ptr = func_ptr}, &db);
			free(str_buff);
			str_buff = 0;
		}
		pclose(nm_out_stream);
	}
	t_strptrstore *copy = db;
	if (!addr && db)
	{
		while (db)
		{
			free(db->str);
			copy = db->next;
			free(db);
			db = copy;
		}
	}
	while (copy && addr != copy->ptr)
		copy = copy->next;
	if (!copy)
		return (0);
	return (copy->str);
}

__attribute__((no_instrument_function)) static clock_t	find_total_time(t_funcdata *d)
{
	clock_t	start = d->time;
	void	*caller = d->address;
	int		recursive_depth = 0;

	d = d->next;
	while (d)
	{
		if (d->address == caller && d->type == ENTER)
			recursive_depth++;
		if (d->address == caller && d->type == EXIT)
			if (recursive_depth-- == 0)
				return (d->time - start);
		d = d->next;
	}
	return (-1);
}

__attribute__((no_instrument_function)) static void	report(void)
{
	t_dlret		*func_info = 0;
	t_strstore	*func_names = 0;
	//	organizing informations in t_dlret
	printf("ORGANISE DATA\n");
	t_funcdata	*copy = 0;
	char		*name;
	int			func_index = 0;

	int fd = open("timetracer_out.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd > 0)
		dup2(fd, 1);
	else
		goto free_all;

	while (data)
	{
		if (data->type == ENTER)
		{
			name = find_symbol(data->address);
			if (name)
			{
				func_index = search_strstore(name, func_names);
				if (func_index == -1)
					func_index = add_strstore((t_strstore){.str = strdup(name)}, &func_names);
				if (add_dlret((t_dlret){.str_id = func_index, .depth = data->depth, .times = 0, .time = find_total_time(data), .type = data->type}, &func_info) == -1)
				{
					perror("Error parsing function informations: ");
					exit(1);
				}
			}
		}
		if (data->pool_head)
		{
			free(copy);
			copy = data;
		}
		data = data->next;
	}
	if (copy)
		free(copy);
	//	creating allocated 2d array of strings and freeing linked list
	char **func_names_arr = copy_strstore(func_names);
	//	grouping function patterns
	// t_dlret *copy_func_info = func_info;
	// t_dlret	*curr_travered_tree[MAX_DEPTH] = {[0] = func_info};
	// int		curr_depth = 1;
	// while (1)
	// {
	// 	// search for identical sequences of functions
	// 	t_dlret	pattern = {.right = 0, .inside = 0};
		
	// 	// search for function before
	// 	while (func_info && func_info->depth != curr_depth)
	// 		func_info = func_info->next;
	// 	if (!func_info)
	// 	{
	// 		func_info = copy_func_info;
	// 		continue;
	// 	}
	// 	if (func_info->next && func_info->next->depth == max_depth)
	// 	{
	// 		pattern[0] = func_info;
	// 		pattern[1] = func_info->next;
	// 	}
	// 	int times = 1;
	// 	while (pattern[1]->next && pattern[0]->str_id == pattern[1]->next->str_id \
	// 		&& pattern[1]->next->next && pattern[1]->str_id == pattern[1]->next->next->str_id)
	// 	{
	// 		t_dlret	*pattern_copy[2] = {pattern[0], pattern[1]};
	// 		pattern[0] = pattern[1]->next;
	// 		pattern[1] = pattern[1]->next->next;
	// 		free(pattern_copy[0]);
	// 		free(pattern_copy[1]);
	// 		times++;
	// 	}
	// 	add info to linked list somehow
		
	// 	func_info =	pattern[1]->next;
	// }
	// func_info = copy_func_info;
	//	displaying data
	dprintf(2, "DISPLAY DATA\n");
	t_dlret	*curr_traversed_nodes[MAX_DEPTH + 1];
	int		curr_depth = 0;
	curr_traversed_nodes[curr_depth] = func_info;
	printf("Report:\n");
	char	*indentation = malloc(MAX_DEPTH + 2);
	memset(indentation, '\t', MAX_DEPTH);
	indentation[MAX_DEPTH + 1] = 0;
	while (func_info)
	{
		indentation[func_info->depth] = 0;
		if (func_info->times == 1)
			printf("%s%s:%10.3fms\n", indentation, func_names_arr[func_info->str_id] + OFFSET_FUNC_NAME, (float)func_info->time / (float)1000);
		else
			printf("%s%s:%10.3f ms/%d calls = ~%.3fms per call, peak: %.3f, min: %.3f\n", indentation, func_names_arr[func_info->str_id] + OFFSET_FUNC_NAME, (float)func_info->time / (float)1000, func_info->times, (float)(func_info->time / func_info->times) / (float)1000, (float)func_info->max  / (float)1000, (float)func_info->min / (float)1000);
		indentation[func_info->depth] = '\t';
		if (func_info->inside)
			func_info = curr_traversed_nodes[++curr_depth] = func_info->inside;
		else if (func_info->right)
			func_info = curr_traversed_nodes[curr_depth] = func_info->right;
		else
		{
			if (curr_depth)
			{
				while (curr_depth)
				{
					func_info = curr_traversed_nodes[--curr_depth];
					if (func_info->right)
						break ;
				}
				func_info = curr_traversed_nodes[curr_depth] = func_info->right;
			}
			else
				break ;
		}
	}
	free(indentation);
	//	free func_info and func_names_arr
	dprintf(2, "FREEING DATA\n");
	free_all:
	func_info = curr_traversed_nodes[0];
	while (func_info)
	{
		t_dlret	*to_free = func_info;
		if (func_info->inside)
			func_info = curr_traversed_nodes[++curr_depth] = func_info->inside;
		else if (func_info->right)
			func_info = func_info->right;
		else
		{
			if (curr_depth)
			{
				while (curr_depth && !(func_info = curr_traversed_nodes[--curr_depth])->right);
				func_info = func_info->right;
			}
			else
			{
				func_info = 0;
				break ;
			}
		}
		free(to_free);
	}
	int i = 0;
	while (func_names_arr[i])
		free(func_names_arr[i++]);
	free(func_names_arr);
	close(fd);
}

// void a();
// void b();
// void c();
// void d();

// void a(){b();b();}
// void b(){}
// void c(){d();}
// void d(){a();b();}
// // a-b-b-b-c-d-a-b-b-b-d-a-b-b-b
// // 1 2 2 1 1 2 3 4 4 3 1 2 3 3 2
// int main()
// {
// 	a();
// 	b();
// 	c();
// 	d();
// }
