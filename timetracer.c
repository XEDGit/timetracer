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
#include <libunwind.h>
#include <unistd.h>

#include <mach/mach_init.h>
#include <sys/sysctl.h>
#include <mach/mach_vm.h>

#define PGREP_CMD "pgrep -x "

typedef struct data {
	void	*address;
	clock_t	time;
	char	type;
} t_data;

t_data data[100000] = {0};
int data_index = 0;

 __attribute__((no_instrument_function)) void __cyg_profile_func_enter (void *this_fn, void *call_site)
{
	printf("this_fn: %p call_site: %p\n", this_fn, call_site);
}
 __attribute__((no_instrument_function)) void __cyg_profile_func_exit  (void *this_fn, void *call_site)
{

}

void lol()
{
	
}

int main()
{
	// printf("offset: %llu\n", vmoffset);
	while (1);
}
