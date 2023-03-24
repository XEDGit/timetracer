/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   timetracer.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lmuzio <lmuzio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/03/21 16:58:47 by lmuzio            #+#    #+#             */
/*   Updated: 2023/03/21 21:13:05 by lmuzio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/types.h>
#include <sys/ptrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#define __USE_GNU
#define _GNU_SOURCE
#include <dlfcn.h>

#define	ENTER 0
#define	EXIT 1

typedef struct funcdata {
	void			*address;
	clock_t			time;
	char			type;
	struct funcdata	*next;
} t_funcdata;

t_funcdata *data = 0;

typedef struct dladdr_result {
	int						str_id;
	clock_t					time;
	struct dladdr_result	*next;
}	t_dlret;

typedef struct string_storage {
	char					*str;
	struct string_storage	*next;
}	t_strstore;

pid_t child_pid;

__attribute__((no_instrument_function)) static int add_node(t_funcdata tmp, t_funcdata **og)
{
	t_funcdata	*new = malloc(sizeof(t_funcdata));
	if (!new)
	{
		return (-1);
	}
	new->address = tmp.address;
	new->next = 0;
	new->time = tmp.time;
	new->type = tmp.type;
	if (*og)
	{
		t_funcdata *copy = *og;
		while (copy->next)
			copy = copy->next;
		copy->next = new;
	}
	else
		*og = new;
	return (0);
}

__attribute__((no_instrument_function)) static int	add_dlret(t_dlret tmp, t_dlret **og)
{
	if (tmp.str_id == -1)
		return (-1);
	t_dlret	*new = malloc(sizeof(t_dlret));
	if (!new)
	{
		return (-1);
	}
	new->next = 0;
	new->time = tmp.time;
	new->str_id = tmp.str_id;
	if (*og)
	{
		t_dlret *copy = *og;
		while (copy->next)
			copy = copy->next;
		copy->next = new;
	}
	else
		*og = new;
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
		return (iter);
	}
	else
		*og = new;
	return (0);
}

__attribute__((no_instrument_function)) static int	search_strstore(char *to_search, t_strstore *store)
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

__attribute__((no_instrument_function)) void __cyg_profile_func_enter (void *called_fn, void *call_site)
{
	add_node((t_funcdata){.address = called_fn, .time = clock(), .type = ENTER}, &data);
}

__attribute__((no_instrument_function)) void __cyg_profile_func_exit (void *called_fn, void *call_site)
{
	add_node((t_funcdata){.address = called_fn, .time = clock(), .type = EXIT}, &data);
}

__attribute__((no_instrument_function)) int on_exit(void (*function)(int, void *), void *arg)
{
	clock_t	end_time = clock(), start_time = data->time;
	t_dlret		*func_info;
	t_strstore	*func_names;

	//	organizing informations in t_dlret
	t_funcdata	*copy;
	Dl_info info;
	int	func_index;
	while (data)
	{
		dladdr(data->address, &info);
		func_index = search_strstore(info.dli_sname, func_names);
		if (func_index == -1)
			func_index = add_strstore((t_strstore){.str = strdup(info.dli_sname)}, func_names);
		if (add_dlret((t_dlret){.str_id = func_index, .time = data->time}, &func_info) == -1)
		{
			perror("Error parsing function informations: ");
			exit(1);
		}
		copy = data->next;
		free(data);
		data = copy;
	}

	// creating data to display

}
