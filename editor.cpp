#include <sys/ioctl.h>
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



struct editorConfig {
	struct termios originalTermios;
	int terminalRows;
	int terminalCols;
};

struct editorConfig config;

/** Terminal **/
int getTerminalSize(int *rows, int *cols) {
	struct winsize windowSize;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &windowSize) == -1 || windowSize.ws_col == 0) {
		return -1;
	} else {
		*cols = windowSize.ws_col;
		*rows = windowSize.ws_row;
		return 0;	
	}
}
void killPgrm(const char *s) {
	//if an error occurs, clear the terminal and place the cursor in the top left corner
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	perror(s);
	exit(1);
}


//function which causes turns off echo in terminal
void enableRawMode() {
	if (tcgetattr(STDIN_FILENO, &config.originalTermios) == -1) {
		killPgrm("tcgetattr");
	}
	atexit(disableRawMode);

	struct termios t = config.originalTermios;
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
	if (tcsetattr(STDIN_FILENO,TCSAFLUSH, &config.originalTermios)){
		killPgrm("tcsetattr");
	}	
}
/** Terminal Output **/
void drawEditorRows() {
	/*
	Does not work, does not stop printing ~
	struct winsize userTerminal;

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &userTerminal);

	int rows =(int) (userTerminal.ws_col);

	for (int i = 0; i < rows; i++) {
		write(STDOUT_FILENO, "~\r\n", 3); 
	}	
	*/

	for (int i = 0; i < config.terminalRows; i++) {
		write(STDOUT_FILENO, "~\r\n", 3);
	}
}

void editorRefreshScreen() {

	//*** escape sequences found using vt100.net ***

	//writing 4 bytes to the terminal 
	//	-> "\x1b" is the escape character
	//		->"\x1b[" tells the terminal to begin an escape sequence which instructs the terminal to do things like color text
	write(STDOUT_FILENO, "\x1b[2J", 4);

	//writes 3 bytes to the terminal
	//	->places the cursor in the "home" position or the top left corner
	write(STDOUT_FILENO, "\x1b[H", 3);

	drawEditorRows();

	write(STDOUT_FILENO, "\x1b[H", 3); 

}
/** user input **/

char readKey() {
	char c;
	int readStatus;

	/**
	  while the terminal is still reading characters
		-> check if the character returns and error and if that given error is NOT a EAGAIN(time out error) then
			-> call killPgrm() which exits the program
		->if not then return the inputted character
	  **/
	while ((readStatus= read(STDIN_FILENO, &c, 1)) != 1) {
		if (readStatus== -1 && errno != EAGAIN) {
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
		//if cntrl q is pressed clear the screen
		write(STDOUT_FILENO, "\x1b[2J", 4);
		write(STDOUT_FILENO, "\x1b[H", 3);
		exit(0);
		break;
	}
}

/** INITIALIZE  **/

//initializes the row and col size of the editor config struct
void initEditor() {
	if (getTerminalSize(&config.terminalRows, &config.terminalCols) == -1) {
		killPgrm("getTerminaSize");
	}
}


int main() {
	enableRawMode();
	initEditor();
	//letter typed into terminal
	while (1) {
		editorRefreshScreen();
		processKeypress();
	}
}
