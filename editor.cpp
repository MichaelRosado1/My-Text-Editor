#include <sys/types.h>
#include <sys/ioctl.h>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>
//** Defines **//
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}

int getCursorPosition(int *, int *); 
void disableRawMode();
void enableRawMode();

/** Data **/

//ints so it does not conflict with char inputs 
enum keys {
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN, 
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN	
};

//use typedef to make an alias for a data type or struct
// stands for Editor Row
typedef struct erow {
	int size;
	char *chars;
} erow;
//use this struct to store the terminals original attributes
//so when the user is done, we can set the original terminal attributes back  
struct abuf {
	char *b;
	int length;
};
struct editorConfig {
	struct termios originalTermios;
	int terminalRows;
	int terminalCols;
	int cursorX, cursorY;
	int numrows;
	erow row;
};

struct editorConfig config;
/** Terminal **/


//appends terminal buffer to new character array
void abAppend(struct abuf *ab,const char *s, int len) {
	//allocates enough space for current buf + new length of buf
	char *buff = (char*) std::realloc(ab->b, ab->length + len);
	if (buff == NULL) return;
	//copies the memory from the buffer and stores it in a new location
	std::memcpy(&buff[ab->length], s, len);
	ab->b = buff;
	ab->length += len;
}


//edits config struct's terminalRow and terminalCol 
int getTerminalSize(int *rows, int *cols) {
	struct winsize windowSize;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &windowSize) == -1 || windowSize.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
			return -1;
		}	
		return getCursorPosition(rows, cols);	
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
void drawEditorRows(struct abuf *ab) {
	for (int i = 0; i < config.terminalRows; i++) {
		if (i >= config.numrows) {

			//third down the screen 
			if (i == config.terminalRows / 3) {
				char welcomeMessage[80];
				//snprintf returns the number of characters that would be printed if the buffer byte size is large enough
				int length = std::snprintf(welcomeMessage, sizeof(welcomeMessage), "Welcome to My-Text editor:)");
				//if the message is greater than the terminal screen we shorten it
				if (length > config.terminalCols) {
					length = config.terminalCols;
				}
				//half of the terminal width
				int padding = (config.terminalCols - length) / 2;
				if (padding) {
					abAppend(ab, "~", 1);
					padding--;
				}
				//while the padding isn't 0, print a space
				//	->moves to half of the terminal width
				while (padding--) abAppend(ab, " ", 1);
				abAppend(ab, welcomeMessage, length);
			} else {
				abAppend(ab, "~", 1);
			}
			//clears the line
		} else {
			int length = config.row.size;
			if (length > config.terminalCols) length = config.terminalCols;
			abAppend(ab, config.row.chars, length);
		}
		abAppend(ab, "\x1b[K", 3);
		if (i < config.terminalRows - 1) {
			abAppend(ab, "\r\n", 2);
		}	
		
	}
}
void editorRefreshScreen() {
	//*** escape sequences found using vt100.net ***
	// may not work on older terminal versions because of the 'l' and 'h' commands(hide cursor)
	struct abuf ab = ABUF_INIT;

	//hides the cursor and positions it before refreshing to prevent flickering
	abAppend(&ab, "\x1b[?25l", 6);

	abAppend(&ab, "\x1b[H", 3);	

	drawEditorRows(&ab);

	char buffer[32];
	
	//"\x1b[%d;%dH"
	//	->%d means put the following params here
	// the H command tells the terminal how much to move the cursor and in which direction
	std::snprintf(buffer, sizeof(buffer), "\x1b[%d;%dH", config.cursorY + 1, config.cursorX + 1);
	
	abAppend(&ab, buffer, std::strlen(buffer));


	abAppend(&ab, "\x1b[?25h", 6);

	write(STDOUT_FILENO,ab.b, ab.length ); 

}
/** file reading **/

void editorOpen() {
	char *line = (char *)"Hello, world!";
	ssize_t linelength = 13;

	config.row.size = linelength;
	config.row.chars =(char *)std::malloc(linelength + 1);
	std::memcpy(config.row.chars, line, linelength);
	config.row.chars[linelength] = '\0';
	config.numrows = 1;
}
/** user input **/

//implementing vim style keypresses

void moveCursor(int keypress) {
	switch (keypress) {
		case ARROW_DOWN :
			if (config.cursorY != config.terminalRows- 1) {
				config.cursorY--;
			}
			break;
		case ARROW_UP:
			if (config.cursorY != 0) {
				config.cursorY++;
			}
			break;
		case ARROW_LEFT:
			if (config.cursorX != 0) {
				config.cursorX--;
			}
			break;
		case ARROW_RIGHT:
			if (config.cursorX != config.terminalCols - 1) {
				config.cursorX++;
			}
			break;
	}
}

int readKey() {
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

	}
	//if teh read  character is the escape sequence
	if (c =='\x1b') {
		//storing 3 bytes to account for longer sequences
		char sequence[3];
		if (read(STDIN_FILENO, &sequence[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &sequence[1], 1) != 1) return '\x1b';	

		if (sequence[0] == '[') {
			if (sequence[1] >= '0' && sequence[1] <= '9') {
				if (read(STDIN_FILENO, &sequence[2], 1) != 1) return '\x1b';
				if (sequence[2] == '~') {
					switch (sequence[1]) {
						case '1': return HOME_KEY;
						case '3': return DEL_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
				}
			} else {
				switch(sequence[1]) {
					case 'A': return ARROW_RIGHT;
					case 'B': return ARROW_DOWN;
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'H': return HOME_KEY;
					case 'F': return END_KEY;
				}
			}	
		} else if (sequence[0] == 'O') {
			switch(sequence[1]) {
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
			}
		}
		return '\x1b';
	} else {
			return c;
	}
}	

void processKeypress() {
//used switch case because if else was not working with CTRL_KEY I will change this later for easier readability	
	int c = readKey();
	switch (c) {
		case CTRL_KEY('q'):
		//if cntrl q is pressed clear the screen
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
		case HOME_KEY:
			//home key sets cursor to the top of the terminal screen 
			config.cursorY = 0;
			break;
		case END_KEY:
			//end key sets cursor to the end of the terminal window
			config.cursorX = config.terminalCols - 1;
			break;
		case ARROW_DOWN:
		case ARROW_UP:
		case ARROW_RIGHT:
		case ARROW_LEFT:
			moveCursor(c);
			break;
		case PAGE_UP:
		case PAGE_DOWN:
			{
				int rows = config.terminalRows;
				while (rows--) {
					moveCursor(c == PAGE_UP ? ARROW_UP: ARROW_DOWN);
				}
			}
			break;
	}
}

//uses the cursor to find the hight of the users terminal
int getCursorPosition(int *rows, int *cols) {
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) !=4) {
		return -1;
	}
	// reading the terminal buffer	
	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break;
	
		i++;	
	}
	//sets the buf[i] to null
	//clears the terminal buffer
	buf[i] = '\0';

	//if the first char isn't the escape key or the second input isn't the escape sequence symbol return -1
	if (buf[0] != '\x1b' || buf[1] != '[') return -1;

	//if there aren't two argument lists  successfully filled, return -1
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
	

	return 0;
}


/** INITIALIZE  **/

//initializes the row and col size of the editor config struct
void initEditor() {
	config.cursorX = 0;
	config.cursorY = 0;
	config.numrows = 0;
	if (getTerminalSize(&config.terminalRows, &config.terminalCols) == -1) {
		killPgrm("getTerminaSize");
	}
}

int main() {
	enableRawMode();
	initEditor();
	editorOpen();
	//letter typed into terminal
	while (1) {
		editorRefreshScreen();
		processKeypress();
	}
}

