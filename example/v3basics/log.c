/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

  The SC Library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the SC Library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#include <sc3_log.h>
#include <stdarg.h>

static int          provoke_fatal, provoke_leaks, provoke_which;
static int          main_log_bare;

static const char  *main_log_user = "sc3_log";

static void
main_log (void * user, const char *msg,
          sc3_log_role_t role, int rank, int tid,
          sc3_log_level_t level, int spaces, FILE *outfile)
{
  /* example of custom log function */
  fprintf (outfile, "%s: %s\n", (const char *) user, msg);
}

static void
main_exit_failure (sc3_error_t **e, const char *prefix)
{
  char                flatmsg[SC3_BUFSIZE];

  sc3_error_destroy_noerr (e, flatmsg);
  fprintf (stderr, "%s: %s\n", prefix, flatmsg);

  exit (EXIT_FAILURE);
}

static int
work_error (sc3_error_t **e, sc3_log_t *log, const char *prefix)
{
  char                flatmsg[SC3_BUFSIZE];

  /* bad call convention is reported and accepted */
  if (e == NULL || *e == NULL) {
    sc3_logf (log, 0, SC3_LOG_THREAD0, SC3_LOG_ERROR,
              "%s: NULL error", prefix);
    return 0;
  }

  /* if needed, treat user/recoverable errors here */

  /* leak error we just report and then continue */
  if (sc3_error_is_leak (*e, NULL)) {
    sc3_error_destroy_noerr (e, flatmsg);
    sc3_logf (log, 0, SC3_LOG_THREAD0, SC3_LOG_ERROR,
              "%s: %s", prefix, flatmsg);
    return 0;
  }

  /* fatal error out of an sc3 call, unsafe to continue */
  sc3_error_destroy_noerr (e, flatmsg);
  sc3_logf (log, 0, SC3_LOG_THREAD0, SC3_LOG_ERROR,
            "%s: %s", prefix, flatmsg);
  return 1;
}

static sc3_error_t *
work_init_allocator (sc3_allocator_t ** alloc, int align)
{
  SC3E (sc3_allocator_new (sc3_allocator_nothread (), alloc));
  SC3E (sc3_allocator_set_align (*alloc, align));
  SC3E (sc3_allocator_setup (*alloc));

  if (provoke_leaks && provoke_which == 1) {
    /* Provoke leak */
    SC3E (sc3_allocator_ref (*alloc));
  }

  return NULL;
}

static sc3_error_t *
work_init_log (sc3_MPI_Comm_t mpicomm,
               sc3_log_t ** log, sc3_allocator_t * alloc, int indent)
{
  SC3E (sc3_log_new (alloc, log));
  SC3E (sc3_log_set_level (*log, SC3_LOG_INFO));
  SC3E (sc3_log_set_comm (*log, mpicomm));
  SC3E (sc3_log_set_indent (*log, indent));
  if (main_log_bare) {
    SC3E (sc3_log_set_function (*log, main_log, (void *) main_log_user));
  }
  SC3E (sc3_log_setup (*log));
  return NULL;
}

static sc3_error_t *
work_init (int *pargc, char ***pargv, sc3_MPI_Comm_t mpicomm,
           sc3_allocator_t ** alloc, sc3_log_t ** log)
{
  char                tmp[SC3_BUFSIZE];

  SC3E (work_init_allocator (alloc, 16));
  SC3E (work_init_log (mpicomm, log, *alloc, 3));
  sc3_logf (*log, 0, SC3_LOG_PROCESS0, SC3_LOG_ESSENTIAL,
            "Command line flags %s%s%s%s",
            provoke_fatal ? "F" : "", provoke_leaks ? "L" : "",
            provoke_which > 0 ?
            (snprintf (tmp, SC3_BUFSIZE, "%d", provoke_which), tmp) : "",
            main_log_bare ? "B" : "");
  sc3_log (*log, 0, SC3_LOG_THREAD0, SC3_LOG_TOP, "Leave work_init");
  return NULL;
}

static sc3_error_t *
work_work (sc3_allocator_t * alloc, sc3_log_t * log)
{
  sc3_log (log, 0, SC3_LOG_PROCESS0, SC3_LOG_TOP, "In work_work");
  sc3_log (log, 0, SC3_LOG_THREAD0, SC3_LOG_TOP, "In work_work");

  if (provoke_fatal && provoke_which == 1) {
    int                 bogus = 1;
    SC3E (sc3_allocator_free (alloc, &bogus));
  }
  if (provoke_leaks && provoke_which == 2) {
    int                *bogus;
    SC3E (sc3_allocator_malloc (alloc, sizeof (int), &bogus));
  }

  return NULL;
}

static sc3_error_t *
work_finalize (sc3_allocator_t ** alloc, sc3_log_t ** log)
{
  sc3_error_t        *leak = NULL;

  sc3_log (*log, 0, SC3_LOG_PROCESS0, SC3_LOG_TOP, "Enter work_finalize");

  if (provoke_leaks && provoke_which == 3) {
    /* Provoke leak */
    SC3E (sc3_log_ref (*log));
  }

  /* if we find any leaks, propagate them to the outside */
  SC3L (&leak, sc3_log_destroy (log));

  if (provoke_fatal && provoke_which == 2) {
    int               a = 1;
    int              *bogus = &a;
    SC3E (sc3_allocator_free (*alloc, bogus));
  }

  /* the allocator is destroyed last */
  SC3L (&leak, sc3_allocator_destroy (alloc));
  return leak;
}

int
main (int argc, char **argv)
{
  int                 i;
  int                 scdead = 0;
  sc3_MPI_Comm_t      mpicomm = SC3_MPI_COMM_WORLD;
  sc3_allocator_t    *alloc;
  sc3_log_t          *log;
  sc3_error_t        *e;

  /* Initialize MPI.  This is representative of any external startup code */
  if ((e = sc3_MPI_Init (&argc, &argv)) != NULL) {
    main_exit_failure (&e, "Main init");
  }

  /* Testing predefined static logger: It has no concept of MPI */
  {
    /* No error checking for the MPI calls; valgrind will tell */
    int rank;
    sc3_MPI_Comm_rank (mpicomm, &rank);
    if (rank == 0) {
      sc3_log (sc3_log_predef (), 8, SC3_LOG_PROCESS0, SC3_LOG_TOP,
               "sc3_log example begin: calling static log");
    }
    sc3_MPI_Barrier (mpicomm);
  }

  /* Process command line options */
  if (argc == 2) {
    if (strchr (argv[1], 'F')) {
      provoke_fatal = 1;
    }
    if (strchr (argv[1], 'L')) {
      provoke_leaks = 1;
    }
    for (i = 1; i <= 3; ++i) {
      if (strchr (argv[1], '0' + i)) {
        provoke_which = i;
      }
    }
    if (strchr (argv[1], 'B')) {
      main_log_bare = 1;
    }
  }
  /* there is no logger object yet */

  /* Initialization of toplevel allocator and logger.
     We initialize logging and basic allocation here, on errors we exit.
     This is representative of entering sc3 code from any larger program */
  if (!scdead && (e = work_init (&argc, &argv, mpicomm,
                                 &alloc, &log)) != NULL) {
    main_exit_failure (&e, "Work init");
  }

  /* This is representative of calling sc3 code from any larger program */
  for (i = 0; i < 2; ++i) {
    if (!scdead && (e = work_work (alloc, log)) != NULL) {
      /* The logger is alive so we use it inside the following function */
      scdead = work_error (&e, log, "Work work");
    }
  }

  /* Free toplevel allocator and logger.
     This is representative of leaving sc3 code from any larger program */
  if (!scdead && (e = work_finalize (&alloc, &log)) != NULL) {
    /* The allocator and logger are likely no longer valid */
    scdead = work_error (&e, NULL, "Work finalize");
  }

  /* Application reporting on fatal sc3 error status */
  if (scdead) {
    fprintf (stderr, "%s", "Main fatal work error\n");
  }

  /* Finalize MPI.  This is representative of any external cleanup code */
  if ((e = sc3_MPI_Barrier (mpicomm)) != NULL ||
      (e = sc3_MPI_Finalize ()) != NULL) {
    main_exit_failure (&e, "Main finalize");
  }
  return EXIT_SUCCESS;
}