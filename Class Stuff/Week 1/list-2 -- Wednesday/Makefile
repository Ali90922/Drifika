# COMP 3430 Operating Systems
# Winter 2025
# Franklin Bristow
#
# Students registered in this offering of the course are explicitly permitted
# to copy and use this Makefile for their own work.

CC = clang

# You can read more about -D_FORTIFY_SOURCE at
# https://developers.redhat.com/articles/2023/07/04/developers-guide-secure-coding-fortifysource
CFLAGS = -Wall -Werror -Wextra -Wpedantic -D_FORTIFY_SOURCE=3 -g

.PHONY: clean

all: main 

main: list.o

README.pdf: README.md
	pandoc $< -o $@ --template="https://raw.githubusercontent.com/Wandmalfarbe/pandoc-latex-template/refs/heads/master/eisvogel.tex"

clean:
	rm -rf main list.o main.dSYM
