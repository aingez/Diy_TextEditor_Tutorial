#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

// define ctrl-q  to quit
#define CTRL_KEY(k) ((k) & 0x1f)

// disable raw mode
struct termios orig_termios; // store global variable

// editor terminal data global variable
struct editorConfig {
	int screenrows;
	int screencols;
	struct termios orig_termios;
};

struct editorConfig E;

void die(const char *s) {
	perror(s);
	exit(1);
}

void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) die("tcsetattr");
}

// disable echo feature
void enableRawMode() { 
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcsetattr");
	atexit(disableRawMode);

	struct termios raw = E.orig_termios;
	// disable software control flow ctrl-q, ctrl-s, I input flag & ctrl-m (carriage return new line)
	raw.c_lflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
	// disable output procession "\n" to "\r\n"
	raw.c_lflag &= ~(OPOST);
	// I input flag CANON(NICAL), diable ctrl-c(SIGNIT), ctrl-z(SIGSTP), ctrl-v/o IEXTEN
	raw.c_cflag= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) die("tcsetattr");
}

void editorDrawRows() {
	int y;
	for (y = 0; y < E.screenrows; y++) {
		write(STDOUT_FILENO, "~\r\n", 3);
	}
}

void editorRefreshScreen() {
	write(STDOUT_FILENO, "\x1b[2J", 4); // writing 4 bytes to terminal  \x1b (27 or ESC) + [ common starting escape seq. + J clear screen + 2 clear entire screen 
	write(STDOUT_FILENO, "\x1b[H", 3); // move cursor up to position
	
	editorDrawRows();

	write(STDOUT_FILENO, "\x1b[H", 3); // reposition after tilde draw
}

// wait for one keypress and return, handle escape sequences
char editorReadKey() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) die("read");
	}
	return c;
}

int getCursorPosition(int *rows, int *cols) {
	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

	printf("\r\n");
	char c;
	while (read(STDIN_FILENO, &c, 1) == 1) {
		if (iscntrl(c)) {
			printf("%d\r\n", c);
		} else {
			printf("%d ('%c')\r\n", c, c);
		}
	}

	editorReadKey();

	return -1;
}

int getWindowSize(int *rows, int *cols) {
	struct winsize ws;
	
	if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		// measure terminal row, moving cursor to the bottom right
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return 1;
		return getCursorPosition(rows, cols);
		return -1;
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

// wait for keypress then handle, mapping key combinations
void editorProcessKeypress() {
	char c = editorReadKey();

	switch (c) {
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4); 
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
	}
}

void initEditor() {
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main() {
	enableRawMode();
	initEditor();

	// read and display keypress
	while (1) {
		// clear screen in case of error 
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}
