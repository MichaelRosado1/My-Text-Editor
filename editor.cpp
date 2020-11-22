#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <termios.h>

void disableRawMode();
//use this struct to store the terminals original attributes
//so when the user is done, we can set the original terminal attributes back  
struct termios originalTermios;

//function which causes turns off echo in terminal
void enableRawMode() {
	tcgetattr(STDIN_FILENO, &originalTermios);
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
   */
	
	t.c_lflag &= ~(ECHO | ISIG| ICANON);
	
	// input flag which prevents cntrl s and q from being pressed	
	t.c_iflag &= ~(IXON);

	//TCSAFLUSH specifies when to apply the changes made
	//	-> makes the change after queued output has been written	
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);

}

void disableRawMode() {
	//sets the terminal attributes back to the users original settings 
	tcsetattr(STDIN_FILENO,TCSAFLUSH, &originalTermios);
	
}



int main() {
	enableRawMode();
	//letter typed into terminal
	char c;

	//while chars are still being typed in terminal and the letter q hasn't been typed
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
		//if the char c is a control(non-printable) character
		if (iscntrl(c)) {
			//we only want to print the ascii value of the character
			printf("%d\n", c);
		} else {
			//print the ascii value and the letter inputted surrounded by ' 
			printf("%d ('%c')\n", c, c);
		}

	}
}
