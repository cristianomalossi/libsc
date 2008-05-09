/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008 Carsten Burstedde, Lucas Wilcox.

  The SC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the SC Library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sc_dummympi.h>

int
MPI_Init (int *argc, char ***argv)
{
  return MPI_SUCCESS;
}

int
MPI_Finalize (void)
{
  return MPI_SUCCESS;
}

int
MPI_Comm_size (MPI_Comm comm, int *size)
{
  *size = 1;

  return MPI_SUCCESS;
}

int
MPI_Comm_rank (MPI_Comm comm, int *rank)
{
  *rank = 0;

  return MPI_SUCCESS;
}

/* EOF sc_dummympi.c */
