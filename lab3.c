#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "uart.h"
#include "timer.h"

#define CMD_PLAY    "play"
#define CMD_EXIT    "exit"
#define CMDBUF_LEN  4

#define MAX_LEN 15

#define CHAR_EQ '-'

#define CMD_DELAY 3

#define arr_len(_arr) (sizeof(_arr) / sizeof((_arr)[0]))

bool listening = true;

char choice_buf[2]  = "\0"; //2-digit dec number + terminator
char current_word[MAX_LEN];
struct game_state {
	char chosen_word[MAX_LEN];
	char current_word[MAX_LEN];
	char current_char;
};

static struct game_state g_st;

/* Forward declarations */
static
void
gen_game_values(void);

static
void
uart_int(void) __irq;

static
void
gen_game_values(void)
{
	char bag_of_words[3][MAX_LEN] = {"first", "abrakadabra", "difficult"};
	strcpy(g_st.chosen_word, gets(bag_of_words[rand() % 2 + 0]));
	UART0_write_line(g_st.chosen_word);
	for (int i = 0; i<arr_len(g_st.chosen_word);i++){
		g_st.current_word[i] = CHAR_EQ;
	}
	UART0_write_line(g_st.chosen_word);
	UART0_write_line(g_st.current_word);
}

volatile static bool uart_int_happened = false;

void
string_compare(struct game_state g_st){
	strcpy(current_word, g_st.current_word);
	for (int i=0; i<sizeof(g_st.chosen_word); i++){
			if (g_st.chosen_word[i]==g_st.current_char){
				current_word[i] = g_st.current_char;
		}
	};
}

void
uart_int(void) __irq
{
	uart_int_happened = true;
}

static bool cmd_not_choice = true;

void
handle_game(void)
{
	while(!uart_int_happened);
	
	UART0_int_disable();
	if(!cmd_not_choice)
		goto repeat_choice;
	
	char cmd_buf[CMDBUF_LEN+1] = "\0";
	int err = UART0_read_line(cmd_buf, arr_len(cmd_buf));
	if (err)
		goto error;

	if (!memcmp(cmd_buf, CMD_EXIT, CMDBUF_LEN)) {
		UART0_write_line("-> Good bye!\n");
		listening = false;
		goto cleanup;
	} else if (!memcmp(cmd_buf, CMD_PLAY, CMDBUF_LEN)) {
		gen_game_values();
		UART0_write_line("-> Guess the word as fast as possible\n");
		cmd_not_choice = false;
		goto cleanup;
	} else {
		goto error;
	}
	
error:
	UART0_write_line("-> unknown command\n");
cleanup:
	UART0_int_enable();
	return;

repeat_choice:	
	if (UART0_read_line(choice_buf, arr_len(choice_buf))) {
		UART0_write_line("-> unable to read char, try again\n");
		goto repeat_choice;
	}

	if(choice_buf[0]!=' ') {
		g_st.current_char = choice_buf[0];
	} else {
		UART0_write_line("-> Wrong char, choose another one\n");
		goto repeat_choice;
	}

	UART0_write_line_fmt("-> You choose char %c", g_st.current_char);
	delay(CMD_DELAY);

	string_compare(g_st);
	strcpy(g_st.current_word, current_word);
	
	if (strcmp(g_st.current_word, g_st.chosen_word)==0) {
		UART0_write_line_fmt("-> The word is %s", g_st.chosen_word);
		cmd_not_choice = true;
	} else {
		UART0_write_line_fmt("-> The word you guessed is %s", g_st.current_word);
		goto repeat_choice;
	}
}

int
main(void)
{
	assert(!UART_init());
	timer0_init();
	srand(0);
	UART0_reg_int_handler(uart_int);
	UART0_int_enable();
	while(listening)
		handle_game();
	exit(0);
}
