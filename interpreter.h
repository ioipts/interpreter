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
 */

#ifndef INTERPRETER_H_
#define INTERPRETER_H_

/**
* @file interpreter.h
* @brief interpreter
* 
* Bytecode interpreter with binding and debug information.
* Written in standard C for the best portabilily
*
* Feature and Limitation 
* - binding function
* - + - * / and or 
* - support multi dimension of array
* - subroutine
* - always pass by value
* - return a value, structure or array 
*  
* 31/07/2022 remove decimal when print integer
* 08/08/2021 mem leak for Python checked, able to run 
*/

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//Configure
#define ALLOCMEM malloc
#define REALLOCMEM realloc
#define FREEMEM free
#define CPYMEM memcpy
#define SETMEM memset
#define PACKED 

// disable deprecated warning for Windows
#if defined(_MSC_VER)
#pragma warning( disable : 4996 )				
#define _CRT_SECURE_NO_WARNINGS
#endif

/********* STATEMENT TYPE *************/
#define ASSIGNSTATEMENT 1
#define IFSTATEMENT 2
#define CONSTANTSTATEMENT 3
#define LOGICSTATEMENT 4
#define COMPARESTATEMENT 5
#define EXPRESSIONSTATEMENT 6
#define BINDINGSTATEMENT 7
#define VARIABLESTATEMENT 8
#define WHILESTATEMENT 9
#define CALLSTATEMENT 11
#define RETURNSTATEMENT 12


/********* VARIABLE TYPE *************/
#define GLOBALVAR 1
#define LOCALVAR 2
#define STATICVARPROPERTY 1
#define DYNAMICVARPROPERTY 2

/********* PARSE ERROR *************/
#define PARSEOK 0
//numberic expected
#define ERRORNUMBERIC -1
//space expected (python)
#define ERRORTAB -3
//syntax error (unknown error)
#define ERRORSYNTAX -4
//" expected
#define ERRORSTRINGQUOTE -5
#define ERROREXPECTVARIABLE -6
//in expected
#define ERRORLOOPIN -7
#define ERROREXPECTFUNCTION -8
//expect )
#define ERROREXPECTBRACKET -9
//range expected (python)
#define ERROREXPECTLOOPRANGE -10
#define ERROREXPECTOPERATOR -11
#define ERROREXPECTCOLON -12
//= expected
#define ERROREXPECTASSIGN -13
//no if
#define ERRORREQUIREIF -14
//subroutine dup
#define ERRORSUBPROCEDUREDUP -15
#define ERRORSUBPROCEDURENOTFOUND -16
#define ERRORSUBPROCEDUREINVALID -17

#define ERRORBRACKET -18
#define ERRORNOTEXPECTCOLON -19
//in
#define ERROREXPECTIN -20
//\n
#define ERROREXPECTENTER -21
// [ ]
#define ERROREXPECTARRAY -22


#pragma pack(push,1)

typedef struct StatementS* Statement;
typedef struct PropertiesS Properties;
typedef struct ValueS Value;
typedef struct ValueS* Variable;

/**
* x.b for subproperties 
* 10,10 for array 
* NULL for main
*/
typedef struct PropertiesS
{
	char* name;					//name can be NULL
	char* value;
};

typedef struct ValueS {
	int numproperties;			//0 for NULL, 1 for STRING VAR
	Properties* properties;
};


/**
* can be a structure constant
* For example 10.01 "10"
*/
typedef struct ConstantStatementS* ConstantStatement;
struct ConstantStatementS
{
	char type;
	int next;
	int starttext;
	int endtext;
	Value constant;
};

typedef struct VariablePropertyS VariableProperty;
struct VariablePropertyS
{
	char type;						//STATICVARPROPERTY, DYNAMICVARPROPERTY
	int dynamicid;
	char* staticproperty;
};

/**
* a, a[10], a.b
*/
typedef struct VariableStatementS* VariableStatement;
struct VariableStatementS
{
	 char type;
	 int next;
	 int starttext;
	 int endtext;
	 int vid;						//variable id
	 int vartype;					//variable type LOCAL, GLOBAL
	 int numproperty;
	 VariableProperty* property;	//name of property (can be NULL) For example, 1.1 for array, x.b for properties
};

/**
* a.trim(10)
*/
typedef struct BindingStatementS* BindingStatement;
struct BindingStatementS
{
	 char type;
	 int next;
	 int starttext;
	 int endtext;
	 int left;					//variable statement
	 char* varname;				//variable name
	 char* func;				//function name
	 char numparam;
	 int* param;				//point to variable, constant, another binding or function
	 bool isexecute;
};

typedef struct CompareStatementS* CompareStatement;
struct CompareStatementS
{ 
 char type;
 int next;
 int starttext;
 int endtext;
 char compare[4];	//== > >= < <= !
 int left;			//left statement
 int right;			//right statement
};

typedef struct LogicStatementS* LogicStatement;
struct LogicStatementS
{
 char type;
 int next;
 int starttext;
 int endtext;
 char logic;	//& | !
 int left;
 int right;
};

typedef struct ExpressionStatementS* ExpressionStatement;
struct ExpressionStatementS
{
 char type;
 int next;
 int starttext;
 int endtext;
 char op;	//+ - * / % & | xor
 int left;
 int right;
};

typedef struct WhileStatementS* WhileStatement;
struct WhileStatementS
{
	char type;
	int next;
	int starttext;
	int endtext;
	int logic;
	int dowhile;
};

typedef struct IfStatementS* IfStatement;
struct IfStatementS
{
 char type;
 int next;		//no else
 int starttext;
 int endtext;
 int logic;		//0= else 
 int ifthen;
 //int ifelse;
};

typedef struct AssignmentStatementS *AssignmentStatement;
struct AssignmentStatementS {
 char type;
 int next;
 int starttext;
 int endtext;
 int left;
 int right;		//expression 
};

typedef struct CallStatementS* CallStatement;
struct CallStatementS {
	char type;
	int next;
	int starttext;
	int endtext;
	int go;						//goto					
	int numparam;
	int* param;					//point to variable or binding
	int stack;					//increase stack and load param in variable
	bool isexecute;
};

typedef struct ReturnStatementS* ReturnStatement;
struct ReturnStatementS {
	char type;
	int next;
	int starttext;
	int endtext;
	int stack;					//decrease stack
	int ret;					//variable or constant
};

struct StatementS
{
 char type;
 int next;
 int starttext;
 int endtext;
};

/**
* Callback func
* 
* @return true if ok
*/
typedef bool Binding(Variable data, const char* varname,const char* properties, const char* funcname, Value** param, int numparam, void* id,Value* result);

/**
* Byte code for interpreter
*/
typedef struct CodeBlockS* CodeBlock;
struct CodeBlockS
{
 int type;			    //always 0
 int start;			    //index start always 1
 int maxstack;			//max stack level
 int currentstack;		//current stack for return statement
 int* stack;			//return statement
 int currentvarstack;	//current stack for variable
 int* varstack;			//num of var stack
 int numvar;			//num of var array 
 Variable* var;			//variable stack
 int numglobalvar;		//num of global variable
 int numstatement;
 Statement* statement;	//all statement
 void *id;			    //for callback
 Binding *func;
 char* sourcecode;		//original
};

#pragma pack(pop)

/**
* init code block
*/
CodeBlock initCodeBlock(int maxstack);
/**
* destroy code block
*/
void destroyCodeBlock(CodeBlock c);

Statement initStatement(char type);
void destroyStatement(Statement s);

Value* initValue(const char* value);
Variable initVariable();
void setVariable(Value* v, const char* propertie, Value* value);
Value* getVariable(Value* v, const char* propertie);
void destroyVariable(Variable v);
void destroyValue(Value* v);

/**
* run
*/
void runCodeBlock(CodeBlock code, Binding* func,void* id);

#endif