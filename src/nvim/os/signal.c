// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <assert.h>
#include <stdbool.h>
#include <uv.h>
#ifndef WIN32
# include <signal.h>  // for sigset_t
#endif

#include "nvim/ascii.h"
#include "nvim/eval.h"
#include "nvim/event/loop.h"
#include "nvim/event/signal.h"
#include "nvim/fileio.h"
#include "nvim/globals.h"
#include "nvim/log.h"
#include "nvim/main.h"
#include "nvim/memline.h"
#include "nvim/memory.h"
#include "nvim/misc1.h"
#include "nvim/os/signal.h"
#include "nvim/vim.h"

static SignalWatcher spipe, shup, squit, sterm, susr1;
#ifdef SIGPWR
static SignalWatcher spwr;
#endif

static bool rejecting_deadly;

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "os/signal.c.generated.h"
#endif

void signal_init(void)
{
#ifndef WIN32
  // Ensure a clean slate by unblocking all signals. For example, if SIGCHLD is
  // blocked, libuv may hang after spawning a subprocess on Linux. #5230
  sigset_t mask;
  sigemptyset(&mask);
  if (pthread_sigmask(SIG_SETMASK, &mask, NULL) != 0) {
    ELOG("Could not unblock signals, nvim might behave strangely.");
  }
#endif

  signal_watcher_init(&main_loop, &spipe, NULL);
  signal_watcher_init(&main_loop, &shup, NULL);
  signal_watcher_init(&main_loop, &squit, NULL);
  signal_watcher_init(&main_loop, &sterm, NULL);
#ifdef SIGPWR
  signal_watcher_init(&main_loop, &spwr, NULL);
#endif
#ifdef SIGUSR1
  signal_watcher_init(&main_loop, &susr1, NULL);
#endif
  signal_start();
}

void signal_teardown(void)
{
  signal_stop();
  signal_watcher_close(&spipe, NULL);
  signal_watcher_close(&shup, NULL);
  signal_watcher_close(&squit, NULL);
  signal_watcher_close(&sterm, NULL);
#ifdef SIGPWR
  signal_watcher_close(&spwr, NULL);
#endif
#ifdef SIGUSR1
  signal_watcher_close(&susr1, NULL);
#endif
}

void signal_start(void)
{
#ifdef SIGPIPE
  signal_watcher_start(&spipe, on_signal, SIGPIPE);
#endif
  signal_watcher_start(&shup, on_signal, SIGHUP);
#ifdef SIGQUIT
  signal_watcher_start(&squit, on_signal, SIGQUIT);
#endif
  signal_watcher_start(&sterm, on_signal, SIGTERM);
#ifdef SIGPWR
  signal_watcher_start(&spwr, on_signal, SIGPWR);
#endif
#ifdef SIGUSR1
  signal_watcher_start(&susr1, on_signal, SIGUSR1);
#endif
}

void signal_stop(void)
{
#ifdef SIGPIPE
  signal_watcher_stop(&spipe);
#endif
  signal_watcher_stop(&shup);
#ifdef SIGQUIT
  signal_watcher_stop(&squit);
#endif
  signal_watcher_stop(&sterm);
#ifdef SIGPWR
  signal_watcher_stop(&spwr);
#endif
#ifdef SIGUSR1
  signal_watcher_stop(&susr1);
#endif
}

void signal_reject_deadly(void)
{
  rejecting_deadly = true;
}

void signal_accept_deadly(void)
{
  rejecting_deadly = false;
}

static char * signal_name(int signum)
{
  switch (signum) {
#ifdef SIGPWR
  case SIGPWR:
    return "SIGPWR";
#endif
#ifdef SIGPIPE
  case SIGPIPE:
    return "SIGPIPE";
#endif
  case SIGTERM:
    return "SIGTERM";
#ifdef SIGQUIT
  case SIGQUIT:
    return "SIGQUIT";
#endif
  case SIGHUP:
    return "SIGHUP";
#ifdef SIGUSR1
  case SIGUSR1:
    return "SIGUSR1";
#endif
  default:
    return "Unknown";
  }
}

// This function handles deadly signals.
// It tries to preserve any swap files and exit properly.
// (partly from Elvis).
// NOTE: Avoid unsafe functions, such as allocating memory, they can result in
// a deadlock.
static void deadly_signal(int signum)
{
  // Set the v:dying variable.
  set_vim_var_nr(VV_DYING, 1);
  v_dying = 1;

  WLOG("got signal %d (%s)", signum, signal_name(signum));

  snprintf((char *)IObuff, sizeof(IObuff), "Vim: Caught deadly signal '%s'\r\n",
           signal_name(signum));

  // Preserve files and exit.
  preserve_exit();
}

static void on_signal(SignalWatcher *handle, int signum, void *data)
{
  assert(signum >= 0);
  switch (signum) {
#ifdef SIGPWR
  case SIGPWR:
    // Signal of a power failure(eg batteries low), flush the swap files to
    // be safe
    ml_sync_all(false, false, true);
    break;
#endif
#ifdef SIGPIPE
  case SIGPIPE:
    // Ignore
    break;
#endif
  case SIGTERM:
#ifdef SIGQUIT
  case SIGQUIT:
#endif
  case SIGHUP:
    if (!rejecting_deadly) {
      deadly_signal(signum);
    }
    break;
#ifdef SIGUSR1
  case SIGUSR1:
    apply_autocmds(EVENT_SIGNAL, (char_u *)"SIGUSR1", curbuf->b_fname, true,
                   curbuf);
    break;
#endif
  default:
    ELOG("invalid signal: %d", signum);
    break;
  }
}
