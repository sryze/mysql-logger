/*
 * Copyright 2019 Sergey Zolotarev
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BOOL_H
#define BOOL_H

#if defined __STDC__ && defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
  #include <stdbool.h>
#else
  typedef unsigned char bool;
  #define true 1
  #define false 0
  #define __bool_true_false_are_defined
#endif

#endif /* BOOL_H */
