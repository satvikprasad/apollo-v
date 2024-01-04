#include "ffmpeg.h"
#include "handmademath.h"
#include "raylib.h"
#include <assert.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

i32 ffmpeg_start(HMM_Vec2 size, u32 fps, const char *music) {
  i32 pipefd[2];

  if (pipe(pipefd) < 0) {
    fprintf(stderr, "ERROR: could not create a pipe: %s\n", strerror(errno));
    return -1;
  }

  pid_t child = fork();
  if (child < 0) {
    fprintf(stderr, "ERROR: could not fork a child: %s\n", strerror(errno));
    return -1;
  }

  if (child == 0) {
    if (dup2(pipefd[0], STDIN_FILENO) < 0) {
      fprintf(stderr, "ERROR: could not reopen end of pipe as stdin: %s\n", strerror(errno));
      return -1;
    }
    close(pipefd[1]);

    char resolution[64];
    snprintf(resolution, sizeof(resolution), "%zux%zu", size.Width, size.Height);
    char framerate[64];
    snprintf(framerate, sizeof(resolution), "%zu", fps);

    i32 err = execlp("ffmpeg", 
        "ffmpeg",
        "-loglevel", "verbose"
        "-y",
        "-f", "rawvideo",
        "-pix_fmt", "rgba",
        "-s", resolution,
        "-r", framerate,
        "-i", "-",
        "-i", music,
        "-c:v", "libx264",
        "-vb", "2500k",
        "c:a", "aac",
        "-pix_fmt", "yuv420p",
        "output.mp4",
        NULL
    );

    if (err < 0) {
      fprintf(stderr, "ERROR: could not run ffmpeg. Please ensure ffmpeg is installed in your system: %s\n", strerror(errno));
      return -1;
    }

    assert(0 && "unreachable");
  }
  
  close(pipefd[0]);

  return pipefd[1];
}

void ffpmeg_end(i32 pipe) {
  close(pipe);
  wait(NULL);
}
