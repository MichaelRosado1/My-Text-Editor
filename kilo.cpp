#include <iostream>
#include <unistd.h>
#include <termios.h>

//function which causes turns off echo in terminal
void enableRawMode() {
	// this will store all of the terminal attributes
	struct termios t;
	//gets the terminals attributes and stores them in the termios struct
	tcgetattr(STDIN_FILENO, &t);
	//turns off the echo feature in the terminal
	t.c_lflag &=~(ECHO);
	//sets the new attributes of the terminal
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);


}

int main() {
	enableRawMode();
	//letter typed into terminal
	char c;
	//while chars are still being typed in terminal and the letter q hasn't been typed
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');

}
