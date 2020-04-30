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

#include <sc3_memstamp.h>
#include <sc3_refcount.h>
#include <sc3_array.h>

struct sc3_mstamp
{
  sc3_refcount_t      rc;
  sc3_allocator_t    *aator;
  int                 setup;        /**< Boolean; is setup */
  int                 scount;       /**< Number of valid stamps */

  /* parameters fixed after setup call */
  int                 initzero;     /**< Is fill in new items with zeros. */
  int                 per_stamp;    /**< Number of items per stamp */
  size_t              esize, ssize; /**< Size per item and per stamp */

  /* member variables initialized in setup call */
  char               *cur;         /**< Memory of current stamp */
  int                 cur_snext;   /**< Next number within a stamp */
  sc3_array_t        *remember;    /**< Collects all stamps */
  sc3_array_t        *freed;       /**< Buffers the freed elements */
};

int
sc3_mstamp_is_valid (const sc3_mstamp_t * mst, char *reason)
{
  SC3E_TEST (mst != NULL, reason);
  SC3E_IS (sc3_refcount_is_valid, &mst->rc, reason);
  SC3E_IS (sc3_allocator_is_setup, mst->aator, reason);
  SC3E_IS (sc3_array_is_valid, mst->remember, reason);
  SC3E_IS (sc3_array_is_valid, mst->freed, reason);

  /* check internal allocation logic depending on setup status */
  if (!mst->setup) {
    SC3E_TEST (mst->cur == NULL, reason);
  }
  else {
    SC3E_TEST (mst->cur != NULL || mst->ssize == 0, reason);
    SC3E_TEST (mst->cur_snext < mst->per_stamp, reason);
  }
  SC3E_YES (reason);
}

int
sc3_mstamp_is_new (const sc3_mstamp_t * mst, char *reason)
{
  SC3E_IS (sc3_mstamp_is_valid, mst, reason);
  SC3E_TEST (!mst->setup, reason);
  SC3E_YES (reason);
}

int
sc3_mstamp_is_setup (const sc3_mstamp_t * mst, char *reason)
{
  SC3E_IS (sc3_mstamp_is_valid, mst, reason);
  SC3E_IS (sc3_array_is_setup, mst->remember, reason);
  SC3E_IS (sc3_array_is_setup, mst->freed, reason);
  SC3E_TEST (mst->setup, reason);
  SC3E_YES (reason);
}

sc3_error_t        *
sc3_mstamp_set_elem_size (sc3_mstamp_t * mst, size_t esize)
{
  SC3A_IS (sc3_mstamp_is_new, mst);
  mst->esize = esize;
  return NULL;
}

sc3_error_t        *
sc3_mstamp_set_stamp_size (sc3_mstamp_t * mst, size_t ssize)
{
  SC3A_IS (sc3_mstamp_is_new, mst);
  mst->ssize = ssize;
  return NULL;
}

sc3_error_t        *
sc3_mstamp_set_initzero (sc3_mstamp_t * mst, int initzero)
{
  SC3A_IS (sc3_mstamp_is_new, mst);
  mst->initzero = initzero;
  return NULL;
}

sc3_error_t        *
sc3_mstamp_get_elem_size (sc3_mstamp_t * mst, size_t *esize)
{
  SC3E_RETOPT (esize, 0);
  SC3A_IS (sc3_mstamp_is_setup, mst);

  if (esize != NULL) {
    *esize = mst->esize;
  }
  return NULL;
}

sc3_error_t        *
sc3_mstamp_get_stamp_size (sc3_mstamp_t * mst, size_t *ssize)
{
  SC3E_RETOPT (ssize, 0);
  SC3A_IS (sc3_mstamp_is_setup, mst);

  if (ssize != NULL) {
    *ssize = mst->ssize;
  }
  return NULL;
}

sc3_error_t        *
sc3_mstamp_get_stamp_count (sc3_mstamp_t * mst, int *scount)
{
  SC3E_RETOPT (scount, 0);
  SC3A_IS (sc3_mstamp_is_setup, mst);

  if (scount != NULL) {
    *scount = mst->scount;
  }
  return NULL;
}
