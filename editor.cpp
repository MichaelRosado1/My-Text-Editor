#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

#define CTRL_KEY(k) ((k) & 0x1f)

void disableRawMode();
void enableRawMode();


/** Data **/

//use this struct to store the terminals original attributes
//so when the user is done, we can set the original terminal attributes back  

struct termios originalTermios;

/** Terminal **/
void killPgrm(const char *s) {
	perror(s);
	exit(1);
}


//function which causes turns off echo in terminal
void enableRawMode() {
	if (tcgetattr(STDIN_FILENO, &originalTermios) == -1) {
		killPgrm("tcgetattr");
	}
	atexit(disableRawMode);

	//gets the curent terminal attributes
	tcgetattr(STDIN_FILENO, &originalTermios);
		
	// this will store all of the terminal attributes
	struct termios t = originalTermios;
	/*
		Echo and Icanon are both bit flags
			->both are represented by binary digits
			->the | operator goes through the bits and does an OR operation on them.
			->this essentially will flip both of the bitflags or turn them off
		Echo = printing in terminal
		ISIG = control c or z(we need to turn this off to prevent the program from ending
		ICANON = allows us to turn off canonical mode
		IEXTEN = disables cntrl v
   */
	
	t.c_lflag &= ~(ECHO | ISIG| IEXTEN | ICANON);
	

	// input flag which prevents cntrl s and q from being pressed	
	//also fixes cntrl m problem
	t.c_iflag &= ~(IXON | BRKINT | INPCK |ISTRIP |ICRNL);

	//disables the processing features "\n and \r\n"
	t.c_oflag &= ~(OPOST);

	t.c_cflag |= (CS8);

	//vmin sets amount of bytes needed  before read() returns anything
	t.c_cc[VMIN] = 0;
	//VTIME sets teh amount of time to wait before read() returns anything
	t.c_cc[VTIME] = 1;

	//TCSAFLUSH specifies when to apply the changes made
	//	-> makes the change after queued output has been written	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) == -1) {
		killPgrm("tcsetattr");
	}

}


void disableRawMode() {
	//sets the terminal attributes back to the users original settings 
	if (tcsetattr(STDIN_FILENO,TCSAFLUSH, &originalTermios)) {
		killPgrm("tcsetattr");
	}	
}
/** user input **/

char readKey() {
	char c;
	int charToRead;

	/**
	  while the terminal is still reading characters
		-> check if the character returns and error and if that given error is NOT a EAGAIN(time out error) then
			-> call killPgrm() which exits the program
		->if not then return the inputted character
	  **/
	while ((charToRead= read(STDIN_FILENO, &c, 1)) != 1) {
		if (charToRead== -1 && errno != EAGAIN) {
			killPgrm("read");
		}
		return c;
	}	
	return c;
}


void processKeypress() {
	
	char c = readKey();

	switch (c) {
		case CTRL_KEY('q'):
			exit(0);
			break;
	}
}

/** INITIALIZE  **/

int main() {
	enableRawMode();
	//letter typed into terminal
	while (1) {
		processKeypress();
	}
}
