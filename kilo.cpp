#include <iostream>
#include <unistd.h>
#include <termios.h>

	
int main() {
	//letter typed into terminal
	char c;
	//while chars are still being typed in terminal and the letter q hasn't been typed
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');

}
