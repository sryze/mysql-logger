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

#ifndef JSON_H
#define JSON_H

#include "bool.h"
#include "strbuf.h"

int json_encode_bool(struct strbuf *json, bool value);
int json_encode_double(struct strbuf *json, double value);
int json_encode_string(struct strbuf *json, const char *str);
int json_encode(struct strbuf *json, const char *format, ...);

#endif /* JSON_H */
