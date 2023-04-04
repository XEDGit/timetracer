#include "timetracer.h"

__attribute__((no_instrument_function)) static pthread_t	*add_thread(t_threadlist **og)
{
	t_threadlist	*new = malloc(sizeof(t_threadlist));
	if (!new)
		return (0);
	new->next = 0;
	int iter = 0;
	if (*og)
	{
		t_threadlist *copy = *og;
		while (copy->next)
		{
			copy = copy->next;
			iter++;
		}
		copy->next = new;
	}
	else
		*og = new;
	return (&new->thread);
}

__attribute__((no_instrument_function)) static void	*search_matching_branches(void *data)
{
	t_dlret	*func_info = (t_dlret *)data;
	t_dlret	*pattern_start = 0;
	t_dlret	*pattern_end = 0;
	pattern_start = func_info;
	pattern_end = func_info->right;
	//	search for two matches and start traversing
	while (pattern_end && pattern_start->str_id != pattern_end->str_id)
		pattern_end = pattern_end->right;
	if (!pattern_end)
		pthread_exit((void *)0);
	//	check if the two branches are identical
	t_dlret *end = pattern_end;
	while (pattern_end && pattern_start != end)
	{
		if (pattern_start->str_id != pattern_end->str_id)
			pthread_exit((void *)0);
		pattern_end = pattern_end->next;
		pattern_start = pattern_start->next;
	}
	return ((void *)1);
}

__attribute__((no_instrument_function)) static void	free_branch(t_dlret *tofree, t_dlret *toconnect, t_dlret *toadd)
{
	int	depth = tofree->depth;
	toadd->time += tofree->time;
	t_dlret *copy = tofree;
	tofree = tofree->next;
	free(copy);
	while (tofree && tofree->depth > depth)
	{
		copy = tofree;
		tofree = tofree->next;
		toadd->time += copy->time;
		if (toadd->min > copy->time)
			toadd->min = copy->time;
		if (toadd->max < copy->time)
			toadd->max = copy->time;
		toadd = toadd->next;
		free(copy);
	}
	toconnect->next = tofree;
}

__attribute__((no_instrument_function)) void	group_functions(t_dlret *func_info)
{
	t_threadlist	*threads = 0;
	int				curr_depth = 0, err = 0;
	while (func_info)
	{
		//	check if all the branch has been grouped
		if (!func_info->right && !func_info->inside)
		{
			func_info = func_info->next;
			continue;
		}
		//	start checking for same tree
		pthread_t *last = add_thread(&threads);
		if (!last && (err = 1))
			break ;
		pthread_create(last, 0, &search_matching_branches, func_info);
		//	join thread and elaborate result
		void	*return_val;
		pthread_join(*last, &return_val);
		//	if match is found delete identical branch and reconnect
		if (return_val)
		{
			//	find match and calculate horizontal distance
			int	distance = 0;
			t_dlret	*cursor = func_info, *last_node = func_info;
			cursor->times++, distance++;
			cursor = cursor->right;
			while (cursor && cursor->str_id != func_info->str_id)
			{
				cursor->times++;
				distance++;
				last_node = cursor;
				cursor = cursor->right;
			}
			//	find last node of the first identical branch to reconnect after freeing
			while (last_node->inside)
			{
				last_node = last_node->inside;
				while (last_node->right)
					last_node = last_node->right;
			}
			//	free tree branches and reset next of last node
			t_dlret *copy = func_info;
			for (int i = 0; i < distance; i++)
			{
				t_dlret *tofree = cursor;
				cursor = cursor->right;
				free_branch(tofree, last_node, copy);
				copy = copy->right;
			}
			//	set new right to last identical element
			copy = func_info;
			for (int i = 0; i < distance - 1; i++)
				copy = copy->right;
			copy->right = cursor;
		}
		//	snippet for resetting linked list while not storing more than 1 node of t_threadlist
		t_threadlist *tofree = threads;
		threads = 0;
		free(tofree);
		//	change starting node (func_info) and check again
		if (!return_val)
			func_info = func_info->next;
	}
	if (err)
	{
		perror("Malloc fail");
		exit(1);
	}
}