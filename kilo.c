#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

// disable raw mode
struct termios orig_termios; // store global variable

void die(const char *s) {
	perror(s);
	exit(1);
}

void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) die("tcsetattr");
}

// disable echo feature
void enableRawMode() { 
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcsetattr");
	atexit(disableRawMode);

	struct termios raw = orig_termios;
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

int main() {
	enableRawMode();

	// read and display keypress
	while (1) {
		char c = '\0';
		if (read(STDIN_FILENO,&c, 1) == -1 && errno != EAGAIN) die("read");
		if (iscntrl(c)) { // check if a control character (nonprintable ASCII 0-31,127)
			printf("%d\r\n", c);
		}else{ // printable 32 - 126
			printf("%d ('%c')\r\n", c, c);
		}

		if (c == 'q') break;
	}

	return 0;
}
