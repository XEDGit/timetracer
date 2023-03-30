NAME := test

SRC := timetracer.c func_grouping.c

$(NAME): $(SRC)
	gcc $^ -o $@ -finstrument-functions -g -ldl -fPIC $(PROD_FLAGS) $(DBG)

debug: DBG = -fsanitize=address
debug: re

prod: PROD_FLAGS = -Wall -Wextra -Werror -Wpedantic
prod: NAME = timetracer
prod: re

clean:
	rm -f $(NAME)

re: clean $(NAME)