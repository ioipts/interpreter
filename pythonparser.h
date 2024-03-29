/*
 * Copyright (c) 2022 Pit Suwongs, พิทย์ สุวงศ์ (Thailand)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * By Pit Suwongs <admin@ornpit.com>
 *
 * Simple Python parser!
 * Written in C++11
 * 
 * Example:
 * def my_function():
 *  print("Hello from a function")
 * 
 * Feature and Limitation:
 * - binding function
 * - + - * / % and or 
 * - support multi dimension of array
 * - subroutine
 * - always pass by value
 * - return a value, structure or array 
 *
 * 11/11/2022 fix 'set next' 
 * 31/07/2022 multi dimension array, for x in range():, for x in array,  tested
 * 15/07/2022 def: tested
 * 15/05/2022 completed
 */

#ifndef PYTHONPARSER_H_
#define PYTHONPARSER_H_

#include "interpreter.h"

/**
* Parser for Python
* @params statement Python statement
*/
CodeBlock pythonParser(const char* statement, int* result, int* pos);

#endif