NAME := timetracer

SRC := timetracer.c

$(NAME): $(SRC)
	gcc $< -o $@ -finstrument-functions -g
clean:
	rm -f $(NAME)

re: clean $(NAME)