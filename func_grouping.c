#include "timetracer.h"

t_threadlist	*add_thread(t_threadlist **og)
{
	t_threadlist	*new = malloc(sizeof(t_threadlist));
	if (!new)
	{
		return (-1);
	}
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
		return (iter);
	}
	else
		*og = new;
	return (0);
}

void	*search_matching_branches(void *data)
{
	t_dlret	*func_info = (t_dlret *)data;
	t_dlret	*pattern_start[MAX_DEPTH + 1] = {0};
	t_dlret	*pattern_end[MAX_DEPTH + 1] = {0};
	int		p_depth = 0;
	pattern_start[p_depth] = func_info;
	pattern_end[p_depth] = func_info->right;
	//	search for two matches and start traversing
	while (pattern_end[p_depth] && pattern_start[p_depth]->str_id != pattern_end[p_depth]->str_id)
		pattern_end[p_depth] = pattern_end[p_depth]->right;
	if (!pattern_end[p_depth])
		pthread_exit(1);
	//  TODO add actual traversal search
}

void	group_functions(t_dlret *func_info)
{
	t_threadlist	*threads = 0;
	t_dlret			*curr_traversal[MAX_DEPTH + 1] = {[0] = func_info, 0};
	int				curr_depth = 0, err = 0;
	while (1)
	{
		//	change starting node (func_info) and check again
		if (func_info->inside)
		{
			curr_traversal[curr_depth++] = func_info;
			func_info = func_info->inside;
		}
		else if (func_info->right)
		{
			curr_traversal[curr_depth] = func_info;
			func_info = func_info->right;
		}
		else
		{
			while (curr_depth >= 0 && !curr_traversal[curr_depth]->right) curr_depth--;
			//	break ends main loop when there's no nodes left to analyze
			if (curr_depth <= 0)
				break ;
			func_info = curr_traversal[curr_depth]->right;
			continue ;
		}
		//	start checking for same tree
		pthread_t *last = add_thread(&threads);
		if (!last && (err = 1))
			break ;
		pthread_create(last, &search_matching_branches, 0, func_info);
	}
	t_dlret	*cursor;
	curr_depth = 0;
	cursor = curr_traversal[curr_depth];
	while (threads)
	{
		void	*return_val;
		pthread_join(threads->thread, &return_val);
		if (return_val)
		{
			int	len = 0;
			t_dlret	*og = cursor;
			og->times++, len++;
			cursor = cursor->right;
			while (og->str_id != cursor->right->str_id)
				cursor->times++, len++;
			//	free tree branch and reconnect from og
			while (len--)
			{
				//	todo store to_free
				free_branch(cursor);
			}
		}
		threads = threads->next;
	}
	if (err)
	{
		perror("Malloc fail");
		exit(1);
	}
}