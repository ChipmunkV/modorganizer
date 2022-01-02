#include "multiprocess.h"
#include <QLibraryInfo> // UNUSED
#include <iostream>
#include "loglist.h"
#include "moapplication.h"
#include "organizercore.h"
#include "commandline.h"
#include "env.h"
#include "instancemanager.h"
#include "thread_utils.h"
#include "shared/util.h"
#include <report.h>
#include <log.h>

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386))
#include <signal.h>
#include <bits/sigcontext.h>
#include <unistd.h>
#include <sys/prctl.h>
#include "printformatbacktrace.h"

/* Signal handler to get crash backtraces on Linux */
void bt_sighandler(int sig, struct sigcontext ctx)
{
# if defined(__x86_64__)
  if (sig == SIGSEGV)
      fprintf(stderr, "Got signal %d, faulty address is %p, from %p\n", sig, (void *) ctx.cr2, (void *) ctx.rip);
  else
      fprintf(stderr, "Got signal %d\n", sig);

  void *ip = (void *) ctx.rip;
# elif defined(__i386)
  if (sig == SIGSEGV)
  fprintf(stderr, "Got signal %d, faulty address is %p, from %p\n", sig, (void *) ctx.cr2, (void *) ctx.eip);
  else
  fprintf(stderr, "Got signal %d\n", sig);

  void *ip = (void *) ctx.eip;
# endif

  printFormatBacktrace("", ip, 1);
}
#endif

using namespace MOBase;

//thread_local LPTOP_LEVEL_EXCEPTION_FILTER g_prevExceptionFilter = nullptr;
thread_local std::terminate_handler g_prevTerminateHandler = nullptr;

int run(int argc, char *argv[]);

int main(int argc, char *argv[])
{
#if defined(__linux__) && (defined(__x86_64__) || defined(__i386))
  /* Install our signal handler */
  struct sigaction sa;

  typedef void (*sigaction_int_handler_t)(int);
  sa.sa_handler = (sigaction_int_handler_t) bt_sighandler;

  /**
   * Allow the standard handler to execute after the ours.
   * It will dump the core and end the program without clearing static objects.
   */
  sa.sa_flags = SA_RESETHAND;
  sigemptyset(&sa.sa_mask);

  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGABRT, &sa, NULL);

  /* ... add any other signal here */
#endif

  const int r = run(argc, argv);
  std::cout << "mod organizer done\n";
  return r;
}

int run(int argc, char *argv[])
{
  MOShared::SetThisThreadName("main");
  setExceptionHandlers();

  cl::CommandLine cl;
//  if (auto r=cl.process(GetCommandLineW())) {
//    return *r;
//  }
  std::cerr << "FIXME: command line" + std::string(" \e]8;;eclsrc://") + __FILE__ + ":" + std::to_string(__LINE__) + "\a" + __FILE__ + ":" + std::to_string(__LINE__) + "\e]8;;\a\n";

  initLogging();

  // must be after logging
  TimeThis tt("main() multiprocess");

  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  MOApplication app(argc, argv);
  std::cerr << "FIXME: applicationDirPath: '" +  QCoreApplication::applicationDirPath().toStdString() + "'" + std::string(" \e]8;;eclsrc://") + __FILE__ + ":" + std::to_string(__LINE__) + "\a" + __FILE__ + ":" + std::to_string(__LINE__) + "\e]8;;\a\n";
  std::cerr << "FIXME: qt.conf exists: '" +  std::string(QDir(QCoreApplication::applicationDirPath()).exists("qt.conf") ? "true" : "false") + "'" + std::string(" \e]8;;eclsrc://") + __FILE__ + ":" + std::to_string(__LINE__) + "\a" + __FILE__ + ":" + std::to_string(__LINE__) + "\e]8;;\a\n";
  std::cerr << "FIXME: LibraryExecutablesPath: '" + QLibraryInfo::location(QLibraryInfo::LibraryExecutablesPath).toStdString() + "', DataPath: '" + QLibraryInfo::location(QLibraryInfo::DataPath).toStdString() + "'" + std::string(" \e]8;;eclsrc://") + __FILE__ + ":" + std::to_string(__LINE__) + "\a" + __FILE__ + ":" + std::to_string(__LINE__) + "\e]8;;\a\n";


  // check if the command line wants to run something right now
  if (auto r=cl.runPostApplication(app)) {
    return *r;
  }


  // check if there's another process running
  MOMultiProcess multiProcess(cl.multiple());

  if (multiProcess.ephemeral()) {
    // this is not the primary process

    if (cl.forwardToPrimary(multiProcess)) {
      // but there's something on the command line that could be forwarded to
      // it, so just exit
      return 0;
    }

    QMessageBox::information(
      nullptr, QObject::tr("Mod Organizer"),
      QObject::tr("An instance of Mod Organizer is already running"));

    return 1;
  }


  // check if the command line wants to run something right now
  if (auto r=cl.runPostMultiProcess(multiProcess)) {
    return *r;
  }

  tt.stop();


  // stuff that's done only once, even if MO restarts in the loop below
  app.firstTimeSetup(multiProcess);

  // force the "Select instance" dialog on startup, only for first loop or when
  // the current instance cannot be used
  bool pick = cl.pick();

  // MO runs in a loop because it can be restarted in several ways, such as
  // when switching instances or changing some settings
  for (;;)
  {
    try
    {
      auto& m = InstanceManager::singleton();

      if (cl.instance()) {
        m.overrideInstance(*cl.instance());
      }

      if (cl.profile()) {
        m.overrideProfile(*cl.profile());
      }


      // set up plugins, OrganizerCore, etc.
      {
        const auto r = app.setup(multiProcess, pick);
        pick = false;

        if (r == RestartExitCode || r == ReselectExitCode) {
          // resets things when MO is "restarted"
          app.resetForRestart();

          // don't reprocess command line
          cl.clear();

          if (r == ReselectExitCode) {
            pick = true;
          }

          continue;
        } else if (r != 0) {
          // something failed, quit
          return r;
        }
      }


      // check if the command line wants to run something right now
      if (auto r=cl.runPostOrganizer(app.core())) {
        return *r;
      }


      // run the main window
      const auto r = app.run(multiProcess);


      if (r == RestartExitCode) {
        // resets things when MO is "restarted"
        app.resetForRestart();

        // don't reprocess command line
        cl.clear();

        continue;
      }

      return r;
    }
    catch (const std::exception &e)
    {
      reportError(e.what());
      return 1;
    }
  }
}

//LONG WINAPI onUnhandledException(_EXCEPTION_POINTERS* ptrs)
//{
//  const auto path = OrganizerCore::getGlobalCoreDumpPath();
//  const auto type = OrganizerCore::getGlobalCoreDumpType();
//
//  const auto r = env::coredump(path.empty() ? nullptr : path.c_str(), type);
//
//  if (r) {
//    log::error("ModOrganizer has crashed, core dump created.");
//  } else {
//    log::error("ModOrganizer has crashed, core dump failed");
//  }
//
//  // g_prevExceptionFilter somehow sometimes point to this function, making this
//  // recurse and create hundreds of core dump, not sure why
//  if (g_prevExceptionFilter && ptrs && g_prevExceptionFilter != onUnhandledException)
//    return g_prevExceptionFilter(ptrs);
//  else
//    return EXCEPTION_CONTINUE_SEARCH;
//}

void onTerminate() noexcept
{
//  __try
//  {
//    // force an exception to get a valid stack trace for this thread
//    *(int*)0 = 42;
//  }
//  __except
//    (
//      onUnhandledException(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER
//      )
//  {
//  }
//
//  if (g_prevTerminateHandler) {
//    g_prevTerminateHandler();
//  } else {
//    std::abort();
//  }
  std::cerr << "FIXME: Not implemented" + std::string(" \e]8;;eclsrc://") + __FILE__ + ":" + std::to_string(__LINE__) + "\a" + __FILE__ + ":" + std::to_string(__LINE__) + "\e]8;;\a\n"; assert(false && "Not implemented");
}

void setExceptionHandlers()
{
//  if (g_prevExceptionFilter) {
//    // already called
//    return;
//  }
//
//  g_prevExceptionFilter = SetUnhandledExceptionFilter(onUnhandledException);
//  g_prevTerminateHandler = std::set_terminate(onTerminate);
  std::cerr << "FIXME: cpp excption handlers" + std::string(" \e]8;;eclsrc://") + __FILE__ + ":" + std::to_string(__LINE__) + "\a" + __FILE__ + ":" + std::to_string(__LINE__) + "\e]8;;\a\n";
}
