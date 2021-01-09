navipage: main.c run.c rogueutil.h
	cc --std=c99 -Wall -Wextra -pedantic -O3 -D_POSIX_C_SOURCE=200809L *c -o navipage
