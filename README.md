File Binder - A Utility for Keeping Pages Resident
(c) 2016-2017 - Chris Kennelly (chris@ckennelly.com)

Overview
========

File Binder opens one or more files (such as the critical executables in
`/bin`, etc.) and pages them into memory.  It monitors these files and their
dependent libraries for updates via Linux's `inotify` interface.

This is intended for the cases where a hard drive may not be responsive (due to
IO overload or imminent failure), but having important binaries paged in (such
as `sshd`, `bash`, etc.) enables basic commands to be run and responsiveness
restored.

Building
========

File Binder depends on `bazel` (https://github.com/bazelbuild/bazel) and
`googletest` (https://github.com/google/googletest).

The `rlimit` utility is meant to be setuid for `root` to increase the memory
lock limit available to the process.
