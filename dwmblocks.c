#include <X11/Xlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define SIGPLUS SIGRTMIN
#define SIGMINUS SIGRTMIN
#define LENGTH(X) (sizeof(X) / sizeof(X[0]))
#define CMDLENGTH 50
#define MIN(a, b) ((a < b) ? a : b)
#define STATUSLENGTH (LENGTH(blocks) * CMDLENGTH + 1)

typedef struct {
  char *command;
  unsigned int interval;
  unsigned int signal;
} Block;

void dummysighandler(int num);
void sighandler(int num);
void getcmds(int time);
void getsigcmds(unsigned int signal);
void setupsignals();
void sighandler(int signum);
int getstatus(char *str, char *last);
void statusloop();
void termhandler();
void pstdout();
void setroot();
static void (*writestatus)() = setroot;
static int setupX();
static Display *dpy;
static int screen;
static Window root;

static const Block blocks[] = {
    /*Command*/ /*Update Interval*/ /*Update Signal*/
    {"weth", 1800, 0},
    {"print-volume", 0, 8},
    {"date '+%-l:%M %p'", 5, 0},
};
static char delim[] = " | ";
static unsigned int delimLen = 5;

static char statusbar[LENGTH(blocks)][CMDLENGTH] = {0};
static char statusstr[2][STATUSLENGTH];
static int statusContinue = 1;

// opens process *cmd and stores output in *output
unsigned int getcmd(const Block *block, char *output) {
  unsigned int i = output[0] == ' ';
  FILE *cmdf = popen(block->command, "r");
  if (!cmdf)
    return i;
  fgets(output + i, CMDLENGTH - i - delimLen, cmdf);
  i = strlen(output);
  if (i == 0) {
    // return if block and command output are both empty
    pclose(cmdf);
    return i;
  }
  if (delim[0] != '\0') {
    // only chop off newline if one is present at the end
    i = output[i - 1] == '\n' ? i - 1 : i;
    strncpy(output + i, delim, delimLen);
  } else
    output[i++] = '\0';
  pclose(cmdf);
  return i;
}

void getcmds(int time) {
  const Block *current;
  unsigned int blocklen = LENGTH(blocks);
  for (unsigned int i = 0; i < blocklen; i++) {
    current = blocks + i;
    if (i == 0 || i == blocklen - 1) {
      statusbar[i][0] = ' ';
    }
    if ((current->interval != 0 && time % current->interval == 0) || time == -1)
      getcmd(current, statusbar[i]);
  }
}

void getsigcmds(unsigned int signal) {
  const Block *current;
  for (unsigned int i = 0; i < LENGTH(blocks); i++) {
    current = blocks + i;
    if (current->signal == signal)
      getcmd(current, statusbar[i]);
  }
}

void setupsignals() {
  /* initialize all real time signals with dummy handler */
  for (int i = SIGRTMIN; i <= SIGRTMAX; i++)
    signal(i, dummysighandler);

  for (unsigned int i = 0; i < LENGTH(blocks); i++) {
    if (blocks[i].signal > 0)
      signal(SIGMINUS + blocks[i].signal, sighandler);
  }
}

int getstatus(char *str, char *last) {
  strcpy(last, str);
  str[0] = '\0';
  for (unsigned int i = 0; i < LENGTH(blocks); i++)
    strcat(str, statusbar[i]);
  str[strlen(str) - strlen(delim)] = '\0';
  return strcmp(str, last); // 0 if they are the same
}

void setroot() {
  if (!getstatus(statusstr[0],
                 statusstr[1])) // Only set root if text has changed.
    return;
  XStoreName(dpy, root, statusstr[0]);
  XFlush(dpy);
}

int setupX() {
  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    fprintf(stderr, "dwmblocks: Failed to open display\n");
    return 0;
  }
  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);
  return 1;
}

void pstdout() {
  if (!getstatus(statusstr[0],
                 statusstr[1])) // Only write out if text has changed.
    return;
  printf("%s\n", statusstr[0]);
  fflush(stdout);
}

void statusloop() {
  setupsignals();
  int i = 0;
  getcmds(-1);
  while (1) {
    getcmds(i++);
    writestatus();
    if (!statusContinue)
      break;
    sleep(1.0);
  }
}

void dummysighandler(int signum) { return; }

void sighandler(int signum) {
  getsigcmds(signum - SIGPLUS);
  writestatus();
}

void termhandler() { statusContinue = 0; }

int main(int argc, char **argv) {
  for (int i = 0; i < argc; i++) { // Handle command line arguments
    if (!strcmp("-d", argv[i]))
      strncpy(delim, argv[++i], delimLen);
    else if (!strcmp("-p", argv[i]))
      writestatus = pstdout;
  }
  if (!setupX())
    return 1;
  delimLen = MIN(delimLen, strlen(delim));
  delim[delimLen++] = '\0';
  signal(SIGTERM, termhandler);
  signal(SIGINT, termhandler);
  statusloop();
  XCloseDisplay(dpy);
  return 0;
}
