#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "uart.h"
#include "timer.h"

#define CMD_PLAY    "play"
#define CMD_EXIT    "exit"
#define CMDBUF_LEN  4

#define VEHICLE_N 10

#define CMD_DELAY 1

#define arr_len(_arr) (sizeof(_arr) / sizeof((_arr)[0]))

bool listening = true;
char choice_buf[3]  = "\0"; //2-digit dec number + terminator

struct veh {
	char vehicle_name;
	int speed;
};

struct game_state {
	int current_round;
	float player_time;
	float pc_time;
	struct veh vehicle[VEHICLE_N];
	bool used_vehicle[VEHICLE_N];
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
	g_st.current_round = 1;
	g_st.player_time = 0;
	g_st.pc_time = 0;
	for(int i = 0; i<VEHICLE_N; i++) {
		int vehicle_type = rand() % 4;
		switch(vehicle_type) {
		case 0:
			g_st.vehicle[i].speed = rand() % 51 + 90;
			g_st.vehicle[i].vehicle_name = 's';
			break;
		case 1:
			g_st.vehicle[i].speed = rand() % 21 + 70;
			g_st.vehicle[i].vehicle_name = 'm';
			break;
		case 2:
			g_st.vehicle[i].speed = rand() % 31 + 50;
			g_st.vehicle[i].vehicle_name = 'r';
			break;
		case 3:
			g_st.vehicle[i].speed = rand() % 31 + 10;
			g_st.vehicle[i].vehicle_name = 'b';
			break;
		}
		g_st.used_vehicle[i] = false;
	}
}

static
int
get_pc_choice()
{
	for(int i = 0; i<VEHICLE_N; i++) {
		if(!g_st.used_vehicle[i]) {
			g_st.used_vehicle[i] = true;
			return i;
		}
	}
	return -1;
}

static
void
calc_time(int player_choice, int pc_choice)
{
	g_st.player_time = (float) 10 / (float) g_st.vehicle[player_choice].speed;
	g_st.pc_time = (float) 10 / (float) g_st.vehicle[pc_choice].speed;
}

volatile static bool uart_int_happened = false;

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
		UART0_write_line("-> Finish 50 km in 5 rounds as fast as possible\n");
		UART0_write_line_fmt("-> Round %d", g_st.current_round);
		UART0_write_line("s - sportcar, m - motorcycle, r - rodster, b - bicycle");
		UART0_write_line("-> Pick the number of a vehicle:");
		for(int i = 0; i<VEHICLE_N; i++) {
			if(g_st.used_vehicle[i])
				continue;
			UART0_write_line_fmt("-> %d %c: %d kmph", i, g_st.vehicle[i].vehicle_name, g_st.vehicle[i].speed);
		}
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
		UART0_write_line("-> unable to read choice, try again\n");
		goto repeat_choice;
	}

	int choice = (int)strtol(choice_buf, NULL, 16);
	if(!g_st.used_vehicle[choice]) {
		g_st.used_vehicle[choice] = true;
	} else {
		UART0_write_line("-> Wrong number, choose another one\n");
		goto repeat_choice;
	}

	int pc_choice = get_pc_choice();
	assert(pc_choice != -1);

	UART0_write_line_fmt("-> I pick %d", pc_choice);
	UART0_write_line("-> the race is starting...");
	delay(CMD_DELAY);

	calc_time(choice, pc_choice);

	int covered_distance = g_st.current_round * 10;
	UART0_write_line_fmt("-> You covered %d in %f hours", covered_distance, g_st.player_time);
	UART0_write_line_fmt("-> I covered %d in %f hours", covered_distance, g_st.pc_time);

	if (g_st.current_round != 5) {
		g_st.current_round++;
		goto repeat_choice;
	} else {
		UART0_write_line_fmt("-> Score: You/Me %f/%f", g_st.player_time, g_st.pc_time);
		if(g_st.player_time < g_st.pc_time) {
			UART0_write_line("You won!");
		} else {
			UART0_write_line("You lost.");
			UART0_int_enable();
		}
	}
	cmd_not_choice = true;
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
