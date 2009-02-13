/*
  This file is part of p4est.
  p4est is a C library to manage a parallel collection of quadtrees and/or
  octrees.

  Copyright (C) 2007,2008 Carsten Burstedde, Lucas Wilcox.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef P4_TO_P8
#include <p8est_algorithms.h>
#else
#include <p4est_algorithms.h>
#endif

typedef struct
{
  p4est_topidx_t      a;
  int64_t             sum;
}
user_data_t;

static int          weight_counter;
static int          weight_index;

static void
init_fn (p4est_t * p4est, p4est_topidx_t which_tree,
         p4est_quadrant_t * quadrant)
{
  user_data_t        *data = quadrant->p.user_data;

  data->a = which_tree;
  data->sum = quadrant->x + quadrant->y + quadrant->level;
}

static int
refine_fn (p4est_t * p4est, p4est_topidx_t which_tree,
           p4est_quadrant_t * quadrant)
{
  if (quadrant->level >= 6) {
    return 0;
  }
#ifdef P4_TO_P8
  if (quadrant->level >= 5 && quadrant->z <= P4EST_QUADRANT_LEN (3)) {
    return 0;
  }
#endif

  if (quadrant->x == P4EST_LAST_OFFSET (2) &&
      quadrant->y == P4EST_LAST_OFFSET (2)) {
    return 1;
  }
  if (quadrant->x >= P4EST_QUADRANT_LEN (2)) {
    return 0;
  }

  return 1;
}

static int
weight_one (p4est_t * p4est, p4est_topidx_t which_tree,
            p4est_quadrant_t * quadrant)
{
  return 1;
}

static int
weight_once (p4est_t * p4est, p4est_topidx_t which_tree,
             p4est_quadrant_t * quadrant)
{
  if (weight_counter++ == weight_index) {
    return 1;
  }

  return 0;
}

int
main (int argc, char **argv)
{
  int                 rank;
  int                 num_procs;
  int                 mpiret;
  MPI_Comm            mpicomm;
  p4est_t            *p4est, *copy;
  p4est_connectivity_t *connectivity;
  int                 i;
  p4est_topidx_t      t;
  size_t              qz;
  p4est_locidx_t      num_quadrants_on_last;
  p4est_locidx_t     *num_quadrants_in_proc;
  p4est_quadrant_t   *quad;
  p4est_tree_t       *tree;
  user_data_t        *user_data;
  int64_t             sum;
  unsigned            crc;

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = MPI_COMM_WORLD;
  mpiret = MPI_Comm_rank (mpicomm, &rank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpicomm, true, true, NULL, SC_LP_DEFAULT);

  /* create connectivity and forest structures */
#ifdef P4_TO_P8
  connectivity = p8est_connectivity_new_twocubes ();
#else
  connectivity = p4est_connectivity_new_corner ();
#endif
  p4est = p4est_new (mpicomm, connectivity, 15,
                     sizeof (user_data_t), init_fn, NULL);

  num_procs = p4est->mpisize;
  num_quadrants_in_proc = P4EST_ALLOC (p4est_locidx_t, num_procs);

  /* refine and balance to make the number of elements interesting */
  p4est_refine (p4est, true, refine_fn, init_fn);

  /* Set an arbitrary partition.
   *
   * Since this is just a test we assume the global number of
   * quadrants will fit in an int32_t
   */
  num_quadrants_on_last = (p4est_locidx_t) p4est->global_num_quadrants;
  for (i = 0; i < num_procs - 1; ++i) {
    num_quadrants_in_proc[i] = (p4est_locidx_t) i + 1;  /* type ok */
    num_quadrants_on_last -= (p4est_locidx_t) i + 1;    /* type ok */
  }
  num_quadrants_in_proc[num_procs - 1] = num_quadrants_on_last;
  SC_CHECK_ABORT (num_quadrants_on_last > 0,
                  "Negative number of quadrants on the last processor");

  /* Save a checksum of the original forest */
  crc = p4est_checksum (p4est);

  /* partition the forest */
  (void) p4est_partition_given (p4est, num_quadrants_in_proc);

  /* Double check that we didn't loose any quads */
  SC_CHECK_ABORT (crc == p4est_checksum (p4est),
                  "bad checksum, missing a quad");

  /* count the actual number of quadrants per proc */
  SC_CHECK_ABORT (num_quadrants_in_proc[rank]
                  == p4est->local_num_quadrants,
                  "partition failed, wrong number of quadrants");

  /* check user data content */
  for (t = p4est->first_local_tree; t <= p4est->last_local_tree; ++t) {
    tree = p4est_array_index_topidx (p4est->trees, t);
    for (qz = 0; qz < tree->quadrants.elem_count; ++qz) {
      quad = sc_array_index (&tree->quadrants, qz);
      user_data = (user_data_t *) quad->p.user_data;
      sum = quad->x + quad->y + quad->level;

      SC_CHECK_ABORT (user_data->a == t, "bad user_data, a");
      SC_CHECK_ABORT (user_data->sum == sum, "bad user_data, sum");
    }
  }

  /* do a weighted partition with uniform weights */
  p4est_partition (p4est, weight_one);
  SC_CHECK_ABORT (crc == p4est_checksum (p4est),
                  "bad checksum after uniformly weighted partition");

  /* copy the p4est */
  copy = p4est_copy (p4est, 1);
  SC_CHECK_ABORT (crc == p4est_checksum (copy), "bad checksum after copy");

  /* do a weighted partition with many zero weights */
  weight_counter = 0;
  weight_index = (rank == 1) ? 1342 : 0;
  p4est_partition (copy, weight_once);
  SC_CHECK_ABORT (crc == p4est_checksum (copy),
                  "bad checksum after unevenly weighted partition 1");

  /* do a weighted partition with many zero weights */
  weight_counter = 0;
  weight_index = 0;
  p4est_partition (copy, weight_once);
  SC_CHECK_ABORT (crc == p4est_checksum (copy),
                  "bad checksum after unevenly weighted partition 2");

  /* do a weighted partition with many zero weights
   *
   * Since this is just a test we assume the local number of
   * quadrants will fit in an int
   */
  weight_counter = 0;
  weight_index =
    (rank == num_procs - 1) ? ((int) copy->local_num_quadrants - 1) : 0;
  p4est_partition (copy, weight_once);
  SC_CHECK_ABORT (crc == p4est_checksum (copy),
                  "bad checksum after unevenly weighted partition 3");

  /* check user data content */
  for (t = copy->first_local_tree; t <= copy->last_local_tree; ++t) {
    tree = p4est_array_index_topidx (copy->trees, t);
    for (qz = 0; qz < tree->quadrants.elem_count; ++qz) {
      quad = sc_array_index (&tree->quadrants, qz);
      user_data = (user_data_t *) quad->p.user_data;
      sum = quad->x + quad->y + quad->level;

      SC_CHECK_ABORT (user_data->a == t, "bad user_data, a");
      SC_CHECK_ABORT (user_data->sum == sum, "bad user_data, sum");
    }
  }

  /* clean up and exit */
  P4EST_FREE (num_quadrants_in_proc);
  p4est_destroy (p4est);
  p4est_destroy (copy);
  p4est_connectivity_destroy (connectivity);
  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}

/* EOF test_partition.c */