#include "printformatbacktrace.h"

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386))
#include <execinfo.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/prctl.h>
#include <sys/syscall.h>

void printFormatBacktrace(const std::string& debuginfoPath, void *ip, size_t framesToSkip, bool printThreadId)
{
	const size_t bt_depth = 128;
	const size_t cmd_buff_len = 4096;
	const size_t thr_id_buff_len = 64;

	void *trace[bt_depth];
	char **messages = (char **) NULL;
	int i, trace_size = 0;
	const int stderr_term = isatty(2);

	trace_size = backtrace(trace, bt_depth);
	/* overwrite sigaction with caller's address */
	if (ip)
		trace[1] = ip;

	messages = backtrace_symbols(trace, trace_size);

	char tidstr[thr_id_buff_len];
	char tidsub[thr_id_buff_len];
	tidstr[0] = '\0';
	tidsub[0] = '\0';

	if (printThreadId)
	{
		const pid_t tid = syscall(SYS_gettid);
		snprintf(tidstr, thr_id_buff_len, " (tid=%d)", tid);
		snprintf(tidsub, thr_id_buff_len, "| sed -r 's_$_ (tid=%d)_'", tid);
	}

	/* skip first stack frame (points here) */
	fprintf(stderr, "Execution path:\n");
	for (i = 1 + framesToSkip; i < trace_size; ++i)
	{
		char syscom[cmd_buff_len];

		syscom[0] = '\0';
		snprintf(syscom, cmd_buff_len, "echo '#%d %s%s'"
			/**
			 * Remove path from fthe binary file name.
			 */
			"| sed -r 's_^(#[0-9]+ )(.*/([^/]*)|[^/]*)\\((([^/]*)(\\+0x[0-9a-f]*))?\\)( \\[0x[0-9a-f]*\\])_\\1\\3 \\5 \\6\\7_'"
			/**
			 * Make the binary file name bold.
			 */
			"%s"
			/**
			 * Demangle.
			 */
			"| c++filt"
			/**
			 * Colorize function names and the class they belong to.
			 * Colorize stack frame number.
			 */
			"%s%s"
			/**
			 * Shorten std::string.
			 */
			"| sed -r 's/std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >( (>))?/std::string\\2/g'"
			/**
			 * Shorten std::allocator.
			 */
			"| { command -v perl >/dev/null 2>&1 && perl -pe 's/, std::(?:allocator|default_delete)(<(?:[^<>]++|(?1))*+>) ?//g' || cat; }"
			">&2\n",
			i,
			messages[i],
			tidstr,
			/**
			 * Make the binary file name bold.
			 */
			stderr_term ? "| sed -r 's_^(#[0-9]+ )([^[:space:]]*)_\\1\\x1B[1m\\2\\x1B[0m_'" : "",
			/**
			 * Colorize function names and the class they belong to.
			 */
			stderr_term ? "| sed -r 's_([^<])(\\b[a-zA-Z_]([a-zA-Z0-9_]+::)?[a-zA-Z0-9_\\.@]+)( ?)\\(_\\1\\x1B[1;96m\\2\\x1B[0m\\4(_g'" : "",
			/**
			 * Colorize stack frame number.
			 */
			stderr_term ? "| sed -r 's_^(#[0-9]*)_\\x1B[1;31m\\1\\x1B[0m_'" : ""
			);

		if (system(syscom) != 0)
			fprintf(stderr, "print failed\n");

		syscom[0] = '\0';
		snprintf(syscom, cmd_buff_len, "eu-addr2line '%p'%s%s --pid=%d"
			/**
			 * Colorize file path and line number.
			 */
			"%s"
			/**
			 * Add thread ID.
			 */
			"%s"
			">&2\n", trace[i], debuginfoPath.empty() ? "" : " --debuginfo-path=", debuginfoPath.c_str(), getpid(),
			/**
			 * Colorize file path and line number, add eclsrc:// hyperlink.
			 */
			stderr_term ? "| sed -r 's_(\\.*[/A-Za-z0-9\\+_\\.\\-]*):([0-9]+)(:[0-9]+)?$_\\x1B]8;;eclsrc://\\1:\\2\\3\\x7\\x1B[1;34m\\1\\x1B[0m:\\x1B[33m\\2\\x1B[0m\\3\\x1B]8;;\\x7_'" : "",
			tidsub
			);

		if (system(syscom) != 0)
			fprintf(stderr, "eu-addr2line failed\n");
	}
}
#else
void printFormatBacktrace(const std::string&, void*, size_t, bool)
{
	fprintf(stderr, "Unable to get stack trace. Getting stack trace is not implemented.\n");
}
#endif
