/* Copyright (C) 2016, Edward Duffy <eduffy@clemson.edu>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */


#ifndef UTIL_H 
#define UTIL_H 

#define streq(X,Y)       (strcmp((X),(Y))==0)
#define startswith(X,P)  (strncmp((X), (P), strlen(P)) == 0)

#define likely(x)        __builtin_expect((x),1)
#define unlikely(x)      __builtin_expect((x),0)

int file_exists(const char *path);
long file_size(const char *path);
void message(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
double microtime();

#endif  /* UTIL_H  */
