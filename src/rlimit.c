/**
 * File Binder - A File Loading Utility
 * (c) 2016-2017 Chris Kennelly <chris@ckennelly.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc <= 1) {
        fprintf(stderr,
            "Usage: %s <program to run> [args...]\n\n"
            "%s reconfigures the memlock limit to the maximum possible value,\n"
            "drops privileges, then runs the program specified.\n", argv[0],
            argv[0]);
        return 1;
    }

    struct rlimit l;
    l.rlim_cur = RLIM_INFINITY;
    l.rlim_max = RLIM_INFINITY;

    int ret = setrlimit(RLIMIT_MEMLOCK, &l);
    if (ret != 0) {
        fprintf(stderr, "Unable to reconfigure rlimit: Error %d (%s)\n",
            errno, strerror(errno));
        return 2;
    }

    if (setgid(getgid()) == -1) {
        fprintf(stderr, "Unable to drop group.\n");
        return 3;
    }
    if (setuid(getuid()) == -1) {
        fprintf(stderr, "Unable to drop user.\n");
        return 4;
    }

    ret = execvp(argv[1], argv + 1);

    // We're still here somehow, report the error.
    fprintf(stderr, "Unable to exec: Error %d (%s)\n", errno,
            strerror(errno));
    return 5;
}
