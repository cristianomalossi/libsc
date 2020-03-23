/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

/** \file sc3_error.h \ingroup sc3
 * The error object is fundamental to the design of the library.
 *
 * We propagate errors up the call stack by returning a pointer of type
 * \ref sc3_error_t from most library functions.
 * If a function returns NULL, it has executed successfully.
 * Otherwise, it encountered an error condition encoded in the return value.
 *
 * The error object may be used to indicate both fatal and non-fatal errors.
 * The fatal sort arises when out of memory, encountering unreachable code
 * and on assertion failures, or whenever a function decides to return fatal.
 * If a function returns any of these hard error conditions, the application
 * must consider that it returned prematurely and may have left resources open.
 * Even worse, communication and I/O may be left open and cause blocking.
 * It is the application's responsibility to take this into account.
 *
 * Logically, a function returns non-fatal => it must not leak any resources.
 * This invariant is preserved if non-fatal errors are propagated upward as
 * fatal.  It is incorrect to propagate a fatal error up as non-fatal.
 *
 * In other words, a function may return not fully cleaned up only if it
 * returns an error of the fatal kind.  This only-if-condition is one-way.
 *
 * To propagate fatal errors up the call stack, the SC3A and SC3E types of
 * macros are provided.
 * The SC3A macros (Assert) are only active when configured --enable-debug.
 * The SC3E macros (Execute) are always active.
 * Both check conditions or error returns and propagate fatal errors upward.
 * These macros are understood to return prematurely on error.
 * When used on non-fatal conditions, they create fatal errors themmselves.
 * An application should use them on any condition it considers fatal.
 * To handle some error gracefully, an application should *not* use them.
 *
 * An application may imply any additional condition as a fatal error.
 * This is fine if it never propagates a fatal error up as non-fatal.
 *
 * The library functions return an error of kind \ref SC3_ERROR_LEAK
 * when encountering leftover memory, references, or other resources,
 * but ensure that the program may continue cleanly.
 * Thus, an application is free to treat leaks as fatal or not.
 * The library functions may return leaks on any sc3_object_destroy call.
 *
 * Non-fatal errors are imaginable when accessing files on disk or parsing
 * input.  It is up to the application to define and implement error handling.
 *
 * The object query functions sc3_object_is_* shall return cleanly on
 * any kind of input parameters, even those that do not make sense.
 * Query functions must be designed not to crash under any circumstance.
 * On incorrect or undefined input, they must return false.
 * On defined input, they return the proper query result.
 */

#ifndef SC3_ERROR_H
#define SC3_ERROR_H

#include <sc3_base.h>

/** The sc3 error object is an opaque structure. */
typedef struct sc3_error sc3_error_t;

#include <sc3_alloc.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/*** DEBUG macros do nothing unless configured with --enable-debug. ***/

#ifndef SC_ENABLE_DEBUG

/** Assertion macro to require a true sc3_object_is_* return value.
 * The argument \a f is such a function and \a o the object to query. */
#define SC3A_IS(f,o) SC3_NOOP

/** Assertion macro requires some condition \a x to be true. */
#define SC3A_CHECK(x) SC3_NOOP

/** Assertion macro checks an error expression \a f and returns if set.
 * The error object returned is stacked into a new fatal error.
 */
#define SC3A_STACK(f) SC3_NOOP

#define SC3A_ONULL(r) SC3_NOOP
#else
#define SC3A_IS(f,o) SC3E_DEMIS(f,o)
#define SC3A_CHECK(x) do {                                              \
  if (!(x)) {                                                           \
    return sc3_error_new_bug ( __FILE__, __LINE__, #x);                 \
  }} while (0)
#define SC3A_STACK(f) do {                                              \
  sc3_error_t *_e = (f);                                                \
  if (_e != NULL) {                                                     \
    return sc3_error_new_stack (&_e, __FILE__, __LINE__, #f);           \
  }} while (0)
#define SC3A_ONULL(r) do {                                              \
  SC3A_CHECK ((r) != NULL);                                             \
  *(r) = NULL;                                                          \
  } while (0)
#endif

/*** ERROR macros.  They are always active and return fatal errors. ***/

/** Execute an expression \a f that produces an \ref sc3_error_t object.
 * If that error is not NULL, create a fatal error and return it.
 * This macro stacks every non-NULL error returned into a fatal error.
 * For graceful handling of non-fatal errors, please use something else.
 */
#define SC3E(f) do {                                                    \
  sc3_error_t *_e = (f);                                                \
  if (_e != NULL) {                                                     \
    return sc3_error_new_stack (&_e, __FILE__, __LINE__, #f);           \
  }} while (0)

/** If a condition \a x is not met, create a fatal error and return it.
 * The error created is of type \ref SC3_ERROR_BUG
 * and its message is set to the failed condition and argument \a s.
 * For graceful handling of non-fatal errors, please use something else.
 */
#define SC3E_DEMAND(x,s) do {                                           \
  if (!(x)) {                                                           \
    char _errmsg[SC3_BUFSIZE];                                          \
    sc3_snprintf (_errmsg, SC3_BUFSIZE, "%s: %s", #x, (s));             \
    return sc3_error_new_bug (__FILE__, __LINE__, _errmsg);             \
  }} while (0)

#define SC3E_DEMIS(f,o) do {                                            \
  char _r[SC3_BUFSIZE];                                                 \
  if (!(f ((o), _r))) {                                                 \
    char _errmsg[SC3_BUFSIZE];                                          \
    sc3_snprintf (_errmsg, SC3_BUFSIZE, "%s(%s): %s", #f, #o, _r);      \
    return sc3_error_new_bug (__FILE__, __LINE__, _errmsg);             \
  }} while (0)

/** When encountering unreachable code, return a fatal error.
 * The error created is of type \ref SC3_ERROR_BUG
 * and its message is provided as parameter \a s.
 */
#define SC3E_UNREACH(s) do {                                            \
  char _errmsg[SC3_BUFSIZE];                                            \
  sc3_snprintf (_errmsg, SC3_BUFSIZE, "Unreachable: %s", (s));          \
  return sc3_error_new_bug (__FILE__, __LINE__, _errmsg);               \
  } while (0)

/** Assert a pointer parameter \a r not to be NULL
 * and initialize its value to \a v. */
#define SC3E_RETVAL(r,v) do {                                           \
  SC3A_CHECK ((r) != NULL);                                             \
  *(r) = (v);                                                           \
  } while (0)

/** Assign a return value \a r to value \a v only
 * if the variable pointer \a r is not NULL. */
#define SC3E_RETOPT(r,v) do {                                           \
  if ((r) != NULL) *(r) = (v);                                          \
  } while (0)

#define SC3E_INOUTP(pp,p) do {                                          \
  SC3A_CHECK ((pp) != NULL && *(pp) != NULL);                           \
  (p) = *(pp);                                                          \
  } while (0)
#define SC3E_INULLP(pp,p) do {                                          \
  SC3A_CHECK ((pp) != NULL && *(pp) != NULL);                           \
  (p) = *(pp);                                                          \
  *(pp) = NULL;                                                         \
  } while (0)
#define SC3E_ONULLP(pp,p) do {                                          \
  SC3A_CHECK ((pp) != NULL);                                            \
  (p) = *(pp);                                                          \
  *(pp) = NULL;                                                         \
  } while (0)

/*** ERROR macros for proceeding without returning in case of errors. ***/

/** Initialize an error object \a e using the result of an expression \a f.
 * If the result is NULL, that becomes the value of the error object.
 * If the result is not NULL, it is stacked into a fatal error,
 * which is then assigned to the error object.
 */
#define SC3E_SET(e,f) do {                                              \
  sc3_error_t *_e = (f);                                                \
  if (_e != NULL) {                                                     \
    (e) = sc3_error_new_stack (&_e, __FILE__, __LINE__, #f);            \
  } else { (e) = NULL; }                                                \
  } while (0)

/** Set an error object \a e that is NULL using the result of an expression.
 * The update is perforrmed as in \ref SC3E_SET with expression \a f.
 * If the error object is not NULL, the macro does not evaluate \a f.
 */
#define SC3E_NULL_SET(e,f) do {                                         \
  if ((e) == NULL) SC3E_SET(e,f);                                       \
  } while (0)

/** If an error object \a e is NULL, set it if condition \a x fails.
 * In this case, we set the object to a new \ref SC3_ERROR_BUG.
 */
#define SC3E_NULL_REQ(e,x) do {                                         \
  if ((e) == NULL && !(x)) {                                            \
    (e) = sc3_error_new_bug (__FILE__, __LINE__, #x);                   \
  }} while (0)

/** If an error object \a e is not NULL, break a loop. */
#define SC3E_NULL_BREAK(e) do {                                         \
  if ((e) != NULL) { break; }} while (0)

/*** TEST macros.  Always executed. */

/** Set the reason output parameter \a r inside an sc3_object_is_* furction
 * to a given value \a reason before returning false.
 * \a r may be NULL in which case it is not updated.
 */
#define SC3E_NO(r,reason) do {                                          \
  if ((r) != NULL) { SC3_BUFCOPY ((r), (reason)); } return 0; } while (0)

/** Set the reason output parameter \a r inside an sc3_object_is_* function
 * to "" before returning true.
 * \a r may be NULL in which case it is not updated.
 */
#define SC3E_YES(r) do {                                                \
  if ((r) != NULL) { (r)[0] = '\0'; } return 1; } while (0)

/** Query condition \a x to be true, in which case the macro does nothing.
 * If the condition is false, the put the reason into \a r, a pointer
 * to a string of size \ref SC3_BUFSIZE, and return 0.
 */
#define SC3E_TEST(x,r)                                                  \
  do { if (!(x)) {                                                      \
    if ((r) != NULL) { SC3_BUFCOPY ((r), #x); }                         \
    return 0; }} while (0)

/** Query an sc3_object_is_* function.
 * The argument \a f shall be such a function and \a o an object to query.
 * If the test returns false, the function puts the reason into \a r,
 * a pointer to a string of size \ref SC3_BUFSIZE, and returns 0.
 * Otherwise the function does nothing.  \a r may be NULL.
 */
#define SC3E_IS(f,o,r)                                                  \
  do { if ((r) == NULL) {                                               \
    if (!(f ((o), NULL))) { return 0; }} else {                         \
    char _r[SC3_BUFSIZE];                                               \
    if (!(f ((o), _r))) {                                               \
      sc3_snprintf ((r), SC3_BUFSIZE, "%s(%s): %s", #f, #o, _r);        \
      return 0; }}} while (0)

#define SC3E_TERR(f,r) do {                                             \
  sc3_error_t * _e = (f);                                               \
  if (_e != NULL) {                                                     \
    sc3_error_destroy_noerr (&_e, r); return 0; }} while (0)

#if 0
/** The severity of an error used to be a proper enum.
 * We have introduced \ref sc3_error_kind_t to be used instead.
 * \deprecated We may not be using this property any longer.
 */
typedef enum sc3_error_severity
{
  SC3_ERROR_SEVERITY_LAST,
}
sc3_error_severity_t;
#endif

/** We indicate the kind of an error condition;
 * see also \ref sc3_error_is_fatal and \ref sc3_error_is_leak. */
typedef enum sc3_error_kind
{
  SC3_ERROR_FATAL,      /**< Generic error indicating a failed program. */
  SC3_ERROR_WARNING,    /**< Generic warning that is not a fatal error. */
  SC3_ERROR_RUNTIME,    /**< Generic runtime error that is recoverable. */
  SC3_ERROR_BUG,        /**< A failed pre-/post-condition or assertion.
                             May also be a violation of call convention.
                             The program may be in an undefined state. */
  SC3_ERROR_MEMORY,     /**< Out of memory or related error.
                             Memory subsystem may be in undefined state. */
  SC3_ERROR_NETWORK,    /**< Network error, possibly unrecoverable.
                             Network subsystem is assumed dysfunctional. */
  SC3_ERROR_LEAK,       /**< Leftover allocation or reference count.
                             The library does not consider this error fatal,
                              but the application should report it. */
  SC3_ERROR_IO,         /**< Input/output error due to external reasons.
                             For example, file permissions may be missing.
                             The application should attempt to recover. */
  SC3_ERROR_USER,       /**< Interactive usage or configuration error.
                             The application must handle this cleanly
                             without producing leaks or incensistencies. */
  SC3_ERROR_KIND_LAST   /**< Guard range of possible enumeration values. */
}
sc3_error_kind_t;

#if 0
/** Errors may be synchronized between multiple programs or threads.
 * \deprecated We are not sure how we will be indicating synchronization.
 *             Other approaches (PETSc) use MPI communicators for programs.
 *             Unclear how a communicator can indicate thread synchronization.
 *             We might demand that all errors from threads must be synced.
 */
typedef enum sc3_error_sync
{
  SC3_ERROR_LOCAL,      /**< Error just occurred in local program/thread.*/
  SC3_ERROR_SYNCED,     /**< Error is synchronized between processes. */
  SC3_ERROR_DISAGREE,   /**< We know the error has distinct values
                             among multiple processes. */
  SC3_ERROR_SYNC_LAST   /**< Guard range of possible enumeration values. */
}
sc3_error_sync_t;
#endif

/** The error handler callback can be invoked on some errors.
 * It takes ownership of the passed in error \a e.
 * It may return NULL if the error was handled or an error object
 * whose ownership is passed to the caller.
 * Thus, when returning NULL, the input error should be unrefd.
 * When not returning NULL, the simplest is to return \a e.
 * Otherwise, \a e must be unrefd and an error with a one up reference
 * returned.
 * In other words, one reference goes in and one goes out.
 */
typedef sc3_error_t *(*sc3_error_handler_t)
                    (sc3_error_t * e, const char *funcname, void *user);

/** Check whether an error is not NULL and internally consistent.
 * The error may be valid in both its setup and usage phases.
 * \param [in] e        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer is not NULL and error consistent.
 */
int                 sc3_error_is_valid (const sc3_error_t * e, char *reason);

/** Check whether an error is not NULL, consistent and not setup.
 * This means that the error is not (yet) in its usage phase.
 * \param [in] e        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, error consistent, not setup.
 */
int                 sc3_error_is_new (const sc3_error_t * e, char *reason);

/** Check whether an error is not NULL, internally consistent and setup.
 * This means that the error is in its usage phase.
 * \param [in] e        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, error consistent and setup.
 */
int                 sc3_error_is_setup (const sc3_error_t * e, char *reason);

/** Check an error object to be setup and fatal.
 * An error is considered fatal if it is of the kind \ref SC3_ERROR_FATAL,
 * \ref SC3_ERROR_BUG, \ref SC3_ERROR_MEMORY or \ref SC3_ERROR_NETWORK.
 * An application may implicitly define any other condition as fatal.
 * \param [in] e        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if error is not NULL, setup, and is of a
 *                      fatal kind, false otherwise.
 */
int                 sc3_error_is_fatal (const sc3_error_t * e, char *reason);

/** Check an error object to be setup and of kind \ref SC3_ERROR_LEAK.
 * \param [in] e        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff error is not NULL, setup, and a leak.
 */
int                 sc3_error_is_leak (const sc3_error_t * e, char *reason);

/*** TODO error functions shall not throw new errors themselves?! ***/

/** Create a new error object in its setup phase.
 * It begins with default parameters that can be overridden explicitly.
 * Setting and modifying parameters is only allowed in the setup phase.
 * The default settings are documented with the sc3_error_set_* functions.
 * Call \ref sc3_error_setup to change the error into its usage phase.
 * After that, no more parameters may be set.
 * \param [in,out] eator    An allocator that is setup.
 *                          The allocator is refd and remembered internally
 *                          and will be unrefd on error destruction.
 * \param [out] ep      Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to an error with default values.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_new (sc3_allocator_t * eator,
                                   sc3_error_t ** ep);

/** Set the error to be the top of a stack of existing errors.
 * The default stack is NULL.
 * \param [in,out] e        Error object before \ref sc3_error_setup.
 * \param [in,out] stack    This function takes ownership of stack
 *                          (i.e. does not ref it), the pointer is NULLed.
 *                          The stack may be NULL or must be setup.
 *                          If called multiple times, a stack passed earlier
 *                          is unrefd internally.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_stack (sc3_error_t * e,
                                         sc3_error_t ** stack);

/** Set the filename and line number of an error.
 * The default location is ("", 0).
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] filename Null-terminated string.  Pointer must not be NULL.
 * \param [in] line     Line number, non-negative.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_location (sc3_error_t * e,
                                            const char *filename, int line);

/** Set the message of an error.
 * The default message is "".
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] errmsg   Null-terminated string.  Pointer must not be NULL.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_message (sc3_error_t * e,
                                           const char *errmsg);

/** Set the kind of an error.
 * The default kind is the generic \ref SC3_ERROR_FATAL.
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] kind     Enum value in [0, \ref SC3_ERROR_KIND_LAST).
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_kind (sc3_error_t * e,
                                        sc3_error_kind_t kind);

#if 0
/** Set the severity of an error.
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] sev      Enum value in [0, \ref SC3_ERROR_SEVERITY_LAST).
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_severity (sc3_error_t * e,
                                            sc3_error_severity_t sev);
void                sc3_error_set_sync (sc3_error_t * e,
                                        sc3_error_sync_t syn);
sc3_error_t        *sc3_error_set_msgf (sc3_error_t * e,
                                        const char *errfmt, ...)
  __attribute__((format (printf, 2, 3)));
#endif

/** Setup an error and put it into its usable phase.
 * \param [in,out] e    This error must not yet be setup.
 *                      Internal storage is allocated, the setup phase ends,
 *                      and the error is put into its usable phase.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_setup (sc3_error_t * e);

/** Increase the reference count on an error object by 1.
 * This is only allowed after the error has been setup.
 * Does nothing if error has not been created by \ref sc3_error_new.
 * \param [in,out] e    This error must be setup.  Its refcount is increased.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_ref (sc3_error_t * e);

/** Decrease the reference of an error object by 1.
 * If the count reaches zero the error object is deallocated.
 * Does nothing if error has not been created by \ref sc3_error_new.
 * \param [in,out] ep   Pointer must not be NULL and the error valid.
 *                      The refcount is decreased.  If it reaches zero,
 *                      the error is deallocated and the value to NULL.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_unref (sc3_error_t ** ep);

/** Takes an error object with one remaining reference and deallocates it.
 * Destroying an error that is multiply refd produces a reference leak.
 * The caller can provide an error handler for this case to avoid
 * returning a fatal error, which would occur with it being NULL.
 * Does nothing if error has not been created by \ref sc3_error_new,
 * i.e. if it is a static fallback.
 * \param [in,out] ep       Setup error with one reference.  NULL on output.
 * \return                  An error object or NULL without errors.
 *                          When the error had more than one reference,
 *                          return an error of kind \ref SC3_ERROR_LEAK.
 */
sc3_error_t        *sc3_error_destroy (sc3_error_t ** ep);

/** Destroy an error object and condense its messages into a string.
 * \param [in,out] pe   This error will be destroyed and pointer NULLed.
 *                      If any errors occur in the process, they are ignored.
 *                      One would be that the error has multiple references.
 * \param [out] flatmsg If not NULL, existing string of length SC3_BUFSIZE
 *                      is filled with the error stack messages flattened.
 */
void                sc3_error_destroy_noerr (sc3_error_t ** pe,
                                             char *flatmsg);

/** Create a new error of parameterizable kind.
 * If internal allocation fails, return a working static error object.
 * This is useful when allocating an error with \ref sc3_error_new might fail.
 * This should generally be used when the error indicates a buggy program.
 * \param [in] kind     Any valid \ref sc3_error_kind_t.
 * \param [in] filename The filename is copied into the error object.
 *                      Pointer not NULL, string null-terminated.
 * \param [in] line     Line number set in the error.
 * \param [in] errmsg   The error message is copied into the error.
 *                      Pointer not NULL, string null-terminated.
 * \return          The new error object; there is no other error handling.
 */
sc3_error_t        *sc3_error_new_kind (sc3_error_kind_t kind,
                                        const char *filename,
                                        int line, const char *errmsg);

/** Create a new error of the kind \ref SC3_ERROR_BUG.
 * If internal allocation fails, return a working static error object.
 * This is useful when allocating an error with \ref sc3_error_new might fail.
 * This should generally be used when the error indicates a buggy program.
 * \param [in] filename The filename is copied into the error object.
 *                      Pointer not NULL, string null-terminated.
 * \param [in] line     Line number set in the error.
 * \param [in] errmsg   The error message is copied into the error.
 *                      Pointer not NULL, string null-terminated.
 * \return              The new error object or a static fallback.
 */
sc3_error_t        *sc3_error_new_bug (const char *filename,
                                       int line, const char *errmsg);

/** Stack a given error into a new one of the kind \ref SC3_ERROR_FATAL.
 * If internal allocation fails, return a working static error object.
 * This is useful when allocating an error with \ref sc3_error_new might fail.
 * This function expects a setup error as stack, which may in turn have stack.
 * It should generally be used when the error indicates a buggy program.
 * \param [in] pstack   We take owership of the stack, pointer is NULLed.
 * \param [in] filename The filename is copied into the error object.
 *                      Pointer not NULL, string null-terminated.
 * \param [in] line     Line number set in the error.
 * \param [in] errmsg   The error message is copied into the error.
 *                      Pointer not NULL, string null-terminated.
 * \return              The new error object or a static fallback.
 */
sc3_error_t        *sc3_error_new_stack (sc3_error_t ** pstack,
                                         const char *filename,
                                         int line, const char *errmsg);

#if 0
/** Create a new error without relying on dynamic memory allocation.
 * This is useful when allocating an error with \ref sc3_error_new might fail.
 * This function expects a setup error as stack and uses its severity value.
 * \todo Do we need this function?
 * \param [in] pstack   We take owership of the stack, pointer is NULLed.
 *                      The severity of *pstack is used for the result error.
 * \param [in] filename The filename is copied into the error object.
 *                      Pointer not NULL, string null-terminated.
 * \param [in] line     Line number set in the error.
 * \param [in] errmsg   The error message is copied into the error.
 *                      Pointer not NULL, string null-terminated.
 * \return          The new error object; there is no other error handling.
 */
sc3_error_t        *sc3_error_new_inherit (sc3_error_t ** pstack,
                                           const char *filename,
                                           int line, const char *errmsg);
#endif

/** Access location string and line number in a setup error object.
 * The filename output pointer is only valid as long as the error is alive.
 * This can be ensured, optionally, by refing and later unrefing the error.
 * We do not ensure this by default to simplify short-time usage.
 * \param [in] e            Setup error object.
 * \param [out] filename    Pointer may be NULL, then it is not assigned.
 *                          On success, set to error's filename.
 * \param [out] line        Pointer may be NULL, then it is not assigned.
 *                          On success, set to error's line number.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_get_location (sc3_error_t * e,
                                            const char **filename, int *line);

/** Access pointer to the message string in a setup error object.
 * The message output pointer is only valid as long as the error is alive.
 * This can be ensured, optionally, by refing and later unrefing the error.
 * We do not ensure this by default to simplify short-time usage.
 * \param [in] e        Setup error object.
 * \param [out] errmsg  Pointer may be NULL, then it is not assigned.
 *                      On success, set to error's message.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_get_message (sc3_error_t * e,
                                           const char **errmsg);

/** Access the kind of a setup error object.
 * \param [in] e        Setup error object.
 * \param [out] kind    Pointer may be NULL, then it is not assigned.
 *                      On success, set to error's kind.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_get_kind (sc3_error_t * e,
                                        sc3_error_kind_t * kind);

#if 0
/** Access the severity of a setup error object.
 * \param [in] e        Setup error object.
 * \param [out] sev     Pointer may be NULL, then it is not assigned.
 *                      On success, set to error's severity.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_get_severity (sc3_error_t * e,
                                            sc3_error_severity_t * sev);
#endif

/** Return the next deepest error stack with an added reference.
 * The input error object must be setup.  Its stack is allowed to be NULL.
 * It is not changed by the call except for its stack to get referenced.
 * \param [in,out] e    The error object must be setup.
 * \param [out] pstack  Pointer may be NULL, then it is not assigned.
 *                      When function returns cleanly, set to input error's
 *                      stack object.  If the stack is not NULL, it is refd.
 *                      In this case it must be unrefd when no longer needed.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_get_stack (sc3_error_t * e,
                                         sc3_error_t ** pstack);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ERROR_H */