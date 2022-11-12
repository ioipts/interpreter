#include "interpreter.h"
#include "pythonparser.h"
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>

#define ISALPHA(S) (((S)>='a') && ((S)<='z')) || (((S)>='A') && ((S)<='Z'))
#define ISNUM(S) ((((S)>='0') && ((S)<='9')))
#define ISREALNUM(S) (((S)>='0') && ((S)<='9')) || ((S)=='.')
#define ISNAME(S) ((((S)>='a') && ((S)<='z')) || (((S)>='A') && ((S)<='Z')) || (((S)>='0') && ((S)<='9')) || ((S)=='_'))
#define ISCOMPARE(S)  ((strstr((S),"==")==(S)) || (strstr((S),"<")==(S))  || (strstr((S),">")==(S)) || (strstr((S),"<=")==(S)) || (strstr((S),">=")==(S)) || (strstr((S),"!=")==(S)))
#define ISOPERATOR(S) (((S)=='+') || ((S)=='-') || ((S)=='*') || ((S)=='/') || ((S)=='%'))
#define ISLOGIC(S) ((strstr((S),"&&")==(S)) || (strstr((S),"||")==(S))) 
#define ISSPACE(S) ((S)==' ') 
#define ISTAB(S) ((S)=='\t')
#define ISQUOTE(S) ((S)=='"')
#define ISNEWLINE(S) (((S)=='\n') || ((S)==0))

#define NUMTYPECODE 1
#define ALPHATYPECODE 2
#define STRINGTYPECODE 3
#define ARRAYTYPECODE 4
//' ' or '\t'
#define BLOCKCODE 5
#define LOGICCODE 6
//\n
#define DELIMITERCODE 7
//no more
#define ENDCODE 8
//,
#define ARRAYDELIMITERCODE 10
//+-*/
#define OPERATORTYPECODE 11
// > < 
#define COMPARETYPECODE 12
// || &&
#define LOGICTYPECODE 13
//#
#define COMMENTCODE 14
// [ 
#define LEFTBRACKETCODE 15
// ]
#define RIGHTBRACKETCODE 16
//True False true false TRUE FALSE
#define BOOLEANCODE 17

typedef struct PythonSubprocedureS PythonSubprocedure;
struct PythonSubprocedureS {
	char* name;
	int go;
	int numstack;
	int numparam;
	char** param;
};

typedef struct PythonCodeS* PythonCode;
struct PythonCodeS {
	char* sourcecode;
	bool globalOrLocal;
	PythonSubprocedure* currentSub;
	std::vector<std::string> globalVar;
	std::vector<std::string> localVar;
	std::vector<Statement> statement;
	std::vector<PythonSubprocedure*> subprocedure;
	//std::vector<int> laststatement;
};

char* pythonCompareParser(char* statement, PythonCode code, int* result);
char* pythonLogicParser(char* statement, PythonCode code, int* result);
char* pythonExpressionParser(char* statement, PythonCode code, int* result);
char* pythonExecuteParser(char* statement, char* next, PythonCode code, int* result,int* resultfirst);
char* pythonVariableParser(char* statement,  PythonCode code, int* result);
char* pythonRangeParser(char* statement, PythonCode code, int* result,int* startid,int* stopid,int* stepid);
char* pythonArrayParser(char* statement, PythonCode code, int* result, int* resultfirst, int* varid);
char* pythonBindingParser(char* statement, BindingStatement func, PythonCode code, int* result);
char* pythonIfParser(char* statement, PythonCode code, int currentblock, int* resultif, int* resultlast);
char* pythonForParser(char* statement, PythonCode code, int currentblock, int* result,int* resultfirst);
char* pythonWhileParser(char* statement, PythonCode code, int currentblock, int* result);
char* pythonRoutineParser(char* statement, PythonCode code, int currentblock, int* result, int* resultfirst);

/**
* count how many lines
*/
int getLines(char* statement)
{
	int num = 0;
	int n = strlen(statement);
	if (n == 0) return 0;
	for (int i = 0; i < n; i++)
	{
		if (statement[i] == '\n') num++;
	}
	return num + 1;
}

/**
* python left trim
*/
char* trimToken(char* statement)
{
	int i = 0;
	while (statement[i] == ' ') i++;
	return &statement[i];
}

/**
* until see p or \n or 0
*/
char* nextTokenTo(char* statement, char p)
{
	while ((*statement != 0) && (*statement != '\n') && (*statement != p))
	{
		statement++;
	}
	return statement;
}

/**
* get token
* 
* @param statement
* @param type can be error (negative value)
* @param token 
* @return next statement
*/
char* pythonToken(char* statement, int* type, char** token)
{
	int i = 0, j;
	char* ptr;
	char* ptr1;
	float testnum;
	//trim
	*type = 0;
	*token = NULL;
	ptr1 = statement;
	if (ISSPACE(*statement))					//space
	{
		*type = BLOCKCODE;
		return &statement[1];
	}
	else if (ISALPHA(*statement))				//name
	{
		j = i;
		while (ISNAME(statement[i])) i++;
		char* name = (char*)ALLOCMEM(i - j + 1);
		CPYMEM(name, &statement[j], i - j);
		name[i - j] = 0;
		*token = name;
		*type = ALPHATYPECODE;
		if ((strlen(name) == 4) && ((strcmp(name, "TRUE") == 0) || (strcmp(name, "true") == 0) || (strcmp(name, "True") == 0)))
		{
			*type = NUMTYPECODE;
			strcpy(name, "1");
		}
		else if ((strlen(name) == 5) && ((strcmp(name, "FALSE") == 0) || (strcmp(name, "false") == 0) || (strcmp(name, "False") == 0)))
		{
			*type = NUMTYPECODE;
			strcpy(name, "0");
		}
		return &statement[i];
	}
	else if (ISNUM(*statement))					//number
	{
		j = i;
		if (statement[i] == '-') i++;
		while (ISREALNUM(statement[i])) i++;
		char* num = (char*)ALLOCMEM(i - j + 1);
		CPYMEM(num, &statement[j], i - j);
		num[i - j] = 0;
		*token = num;
		if (sscanf(num, "%f", &testnum) == 0) { FREEMEM(num); *type = ERRORNUMBERIC; return ptr1; }
		*type = NUMTYPECODE;
		return &statement[i];
	}
	else if (ISOPERATOR(*statement))
	{
		char* name = (char*)ALLOCMEM(10);
		name[0] = *statement;
		name[1] = 0;
		j = i;
		i++;
		*token = name;
		*type = OPERATORTYPECODE;
		return &statement[i];

	}
	else if (*statement == '(')
	{
		*type = LOGICCODE;
		return &statement[1];
	}
	else if (*statement == '"')
	{
		statement++;
		ptr = nextTokenTo(statement, '\"');
		if (*ptr != '\"') { *type = ERRORSTRINGQUOTE; return ptr1; }
		*ptr = 0;
		char* str = (char*)ALLOCMEM(ptr - statement + 1);
		strcpy(str, statement);
		*token = str;
		*type = STRINGTYPECODE;
		return ptr + 1;
	}
	else if (*statement == '[')
	{
		*type = ARRAYTYPECODE;
		return ptr1;
	}
	else if (*statement == ',')
	{
		*type = ARRAYDELIMITERCODE;
		return ptr1;
	}
	else if ((*statement == '\n') || (*statement == '\r'))
	{
		*type = DELIMITERCODE;
		return statement + 1;
	}
	else if (*statement == '#')
	{
		*type = COMMENTCODE;
		return statement + 1;
	}
	else if (*statement == 0)
	{
		*type = ENDCODE;
		return NULL;
	}
	*type = ERRORSYNTAX;
	return statement;
}

void pythonDestroyVariableParser(std::vector<VariableProperty> property)
{
	for (int i = 0; i < property.size(); i++)
	{
		VariableProperty v = property[i];
		if (v.staticproperty != NULL) FREEMEM(v.staticproperty);
	}
}

void pythonDestroySubprocedure(PythonSubprocedure* sub)
{
	for (int i = 0; i < sub->numparam; i++)
	{
		FREEMEM(sub->param[i]);
	}
	FREEMEM(sub);
}

int pythonGetVar(PythonCode code,const char* varname,bool* isLocal)
{
	int vid = -1;
	if (!code->globalOrLocal) //find in local if not found then global
	{
		for (int i = 0; i < code->localVar.size(); i++) {
			if (strcmp(code->localVar[i].c_str(), varname) == 0) {
				vid = i;
				*isLocal = true;
				break;
			}
		}
	}
	if (vid == -1) {		//if not found then find in global
		for (int i = 0; i < code->globalVar.size(); i++) {
			if (strcmp(code->globalVar[i].c_str(), varname) == 0) {
				vid = i;
				*isLocal = false;
				break;
			}
		}
	}
	if (vid == -1) { 
		if (code->globalOrLocal) { 
			code->globalVar.push_back(varname); 
			vid = code->globalVar.size() - 1; 
			*isLocal = false;
		} else { 
			code->localVar.push_back(varname); 
			vid = code->localVar.size() - 1; 
			*isLocal = true;
			//code->currentSub->numstack++;		//no need to add stack
		} 
	}
	return vid;
}

/**
* get variable simple, array or structure 
* support a, a.b.c, a[10], a[b] 
*
* 9/5/2022
*/
char* pythonVariableParser(char* statement, PythonCode code, int* result)
{
	char* token;
	int type;
	bool res = true;
	statement = trimToken((char*)statement);
	char* ptr = statement;
	statement = pythonToken((char*)statement, &type, &token);
	if (type != ALPHATYPECODE) { FREEMEM(token); *result = ERROREXPECTVARIABLE;  return statement; }
	statement = trimToken((char*)statement);
	VariableStatement var = (VariableStatement)initStatement(VARIABLESTATEMENT);
	var->type = VARIABLESTATEMENT;
	bool isLocal=false;
	var->vid = pythonGetVar(code,token,&isLocal);
	var->vartype = (isLocal) ? LOCALVAR : GLOBALVAR;
	var->starttext = ptr - code->sourcecode;
	var->endtext = statement - code->sourcecode;
	*result = code->statement.size();
	code->statement.push_back((Statement)var);
	bool end = false;
	std::vector<VariableProperty> properties;
	while (!end) {			//loop through 
		VariableProperty p;
		if ((statement != NULL) && (*statement == '['))
		{
			int r;
			statement++;
			char* ptr = statement;
			statement = pythonExpressionParser(statement, code, &r);
			if (r < 0)
			{
				*result = r;		//Error
				pythonDestroyVariableParser(properties);
				return ptr;
			}
			if (*statement != ']')
			{
				*result = ERROREXPECTBRACKET;
				pythonDestroyVariableParser(properties);
				return statement;
			}
			statement++;
			p.type = DYNAMICVARPROPERTY;
			p.staticproperty = NULL;
			p.dynamicid = r;
			properties.push_back(p);
		}
		else if ((statement != NULL) && (statement[0] == '.'))
		{
			statement++;
			statement = pythonToken((char*)statement, &type, &token);
			if (type != ALPHATYPECODE) { 
				res = false; 
				FREEMEM(token);  
				*result = ERROREXPECTVARIABLE; 
			}
			p.type = STATICVARPROPERTY;
			p.staticproperty = token;
			p.dynamicid = 0;
			properties.push_back(p);
		}
		else {
			end = true;
		}
	}
	//convert
	var->numproperty = properties.size();
	var->property = (VariableProperty*)ALLOCMEM(sizeof(VariableProperty) * var->numproperty);
	for (int i = 0; i < properties.size(); i++)
	{
		var->property[i] = properties[i];
	}
	return statement;
}

/**
* in case token ( found 
* variable.binding(expression,expression,)
* 9/5/2022
*/
char* pythonBindingParser(char* statement,BindingStatement func, PythonCode code, int* result)
{
	char* token = NULL;
	int type;
	statement = pythonToken((char*)statement, &type, &token);
	if (type != LOGICCODE) {
		if (token != NULL) FREEMEM(token);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement = trimToken(statement);
	func->numparam = 0;
	int maxparam = 1;
	int countbracket = 0;
	char* s = statement;
	bool end = false;
	//count max param
	while (!end)
	{
		if (*s == ',') maxparam++;
		else if (*s == '(') countbracket++;
		else if ((*s == ')') && (countbracket <= 0))  end = true;
		else if (*s == ')') countbracket--;
		else if ((*s == '\n') || (*s == 0)) end = true;
		s++;
	}
	func->param = (int*)ALLOCMEM(sizeof(int) * maxparam);
	//get param
	while (*statement != ')')
	{
		int r1;
		statement = pythonExpressionParser(statement, code, &r1);
		if (r1 < 0) { 
			destroyStatement((Statement)func);  
			*result = r1;  
			return statement; 
		}
		statement = trimToken(statement);
		if (*statement == ',') {
			statement++;
			statement = trimToken(statement);
		}
		func->param[func->numparam] = r1;
		func->numparam++;
	}
	statement = trimToken(statement);
	if (*statement != ')') {
		destroyStatement((Statement)func);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	*result = 0;
	return statement;
}

/**
* in case token ( found
*
* function(expression,expression,)
* 10/05/2022
*/
char* pythonCallingParser(char* statement, CallStatement func, PythonCode code, int* result)
{
	char* token = NULL;
	int type;
	statement=trimToken(statement);
	*result = 0;
	if (*statement != '(') {
		if (token != NULL) FREEMEM(token);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	statement = trimToken(statement);
	func->numparam = 0;
	int maxparam = 1;
	int countbracket = 0;
	char* s = statement;
	bool end = false;
	//count max param
	while (!end)
	{
		if (*s == ',') maxparam++;
		else if (*s == '(') countbracket++;
		else if ((*s == ')') && (countbracket <= 0))  end = true;
		else if (*s == ')') countbracket--;
		else if ((*s == '\n') || (*s == 0)) end = true;
		s++;
	}
	func->param = (int*)ALLOCMEM(sizeof(int) * maxparam);
	//get param
	while (*statement != ')')
	{
		int r1;
		statement = pythonExpressionParser(statement, code, &r1);
		if (r1 < 0) {
			destroyStatement((Statement)func);
			*result = r1;
			return statement;
		}
		statement = trimToken(statement);
		if (*statement == ',') {
			statement++;
			statement = trimToken(statement);
		}
		func->param[func->numparam] = r1;
		func->numparam++;
	}
	statement = trimToken(statement);
	if (*statement != ')') {
		destroyStatement((Statement)func);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	return statement;
}

/**
* execute
* assignment or binding or function
* a=10+20
* a.b(10)
* a[1].walk(10,20)
* a[10].walk(10,10)
* func(19)
* a=[10,20]
* 
* 9/5/2022
*/
char* pythonExecuteParser(char* statement, char* next,PythonCode code, int* result,int* resultfirst)
{
	*resultfirst = 0;
	char* ptr1 = statement;
	if (*next == '(') //function case
	{
		int type;
		char* token;
		int sid = -1;
		statement = pythonToken(statement, &type, &token);
		for (int i = 0; i < code->subprocedure.size(); i++)
		{
			if (strcmp(code->subprocedure[i]->name, token) == 0) {
				sid = i;
				break;
			}
		}
		if (sid == -1) {	// Binding without variable
			int rl;
			BindingStatement b = (BindingStatement)initStatement(BINDINGSTATEMENT);
			b->isexecute = true;
			b->left = 0;
			b->varname = NULL;
			b->func = token;
			*result = code->statement.size();
			code->statement.push_back((Statement)b);
			statement = pythonBindingParser(statement, b, code,&rl);
			if (rl < 0) {
				*result = rl;
			}
		}
		else {				// Call subprocedure
			int rl;
			FREEMEM(token);
			CallStatement b = (CallStatement)initStatement(CALLSTATEMENT);
			b->isexecute = true;
			b->go = code->subprocedure[sid]->go;
			b->numparam = code->subprocedure[sid]->numparam;
			b->stack = code->subprocedure[sid]->numstack;
			*result = code->statement.size();
			code->statement.push_back((Statement)b);
			statement = pythonCallingParser(statement, b, code, &rl);
			if (rl < 0) {
				*result = rl;
			}
		}
		return statement;
	}
	int rl;	
	statement=pythonVariableParser(statement,  code, &rl);
	if (rl < 0) {
		*result = rl;
		return statement;		//Parse error
	}
	statement = trimToken(statement);
	if (*statement == '(') {			//binding
		VariableStatement vstate = (VariableStatement)code->statement[rl];
		if (vstate->property[vstate->numproperty - 1].type == DYNAMICVARPROPERTY) {
			*result = ERRORSYNTAX;
			return statement;
		}
		//get var name 
		char* token;
		int type;
		pythonToken(ptr1, &type, &token);
		BindingStatement b = (BindingStatement)initStatement(BINDINGSTATEMENT);
		b->left = rl;
		b->varname = token;
		b->func = NULL;
		//incase id( no property
		if (vstate->numproperty > 0) {
			b->func = vstate->property[vstate->numproperty - 1].staticproperty;
			vstate->numproperty--;
		}
		*result = code->statement.size();
		code->statement.push_back((Statement)b);
		int rp;
		statement=pythonBindingParser(statement, b, code, &rp);
		if (rp < 0) {
			*result = rp;
		}
	}
	else if (*statement == '=') {		//assignment
		int r,varid;
		statement++;
		statement = trimToken(statement);
		if (*statement == '[') {
			statement = pythonArrayParser(statement, code, &r, resultfirst,&varid);
			if (r < 0) {
				*result = r;
				return statement;
			}
			statement = trimToken(statement);
			if (!ISNEWLINE(*statement)) {
				*result = ERROREXPECTENTER;
				return statement;
			}
			if (*statement != 0) statement++;
			AssignmentStatement a = (AssignmentStatement)initStatement(ASSIGNSTATEMENT);
			a->starttext = ptr1 - code->sourcecode;
			a->endtext = statement - code->sourcecode;
			VariableStatement v1 = (VariableStatement)initStatement(VARIABLESTATEMENT);
			v1->endtext = v1->starttext = 0;
			v1->numproperty = 0;
			v1->property = NULL;
			v1->vid = varid;
			v1->vartype = GLOBALVAR;
			a->right = code->statement.size();
			code->statement.push_back((Statement)v1);
			a->left = rl;
			*result = code->statement[r]->next = code->statement.size();
			code->statement.push_back((Statement)a);
		} else {
			statement = pythonExpressionParser(statement, code, &r);
			if (r < 0) {
				*result = r;
				return statement;
			}
			AssignmentStatement a = (AssignmentStatement)initStatement(ASSIGNSTATEMENT);
			a->starttext = ptr1 - code->sourcecode;
			a->endtext = statement - code->sourcecode;
			a->right = r;
			a->left = rl;
			*result = code->statement.size();
			code->statement.push_back((Statement)a);
		}
	}
	return statement;
}

ConstantStatement pythonNewConstant(const char* token)
{
	ConstantStatement con = (ConstantStatement)initStatement(CONSTANTSTATEMENT);
	con->constant.numproperties = 1;
	con->constant.properties = (Properties*)ALLOCMEM(sizeof(Properties));
	con->constant.properties[0].name = (char*)ALLOCMEM(1);
	con->constant.properties[0].name[0] = 0;
	con->constant.properties[0].value = (char*)ALLOCMEM(strlen(token) + 1);
	strcpy(con->constant.properties[0].value, token);
	return con;
}

/**
* get array []
*/
char* pythonArrayParser(char* statement, PythonCode code, int* result,int *resultfirst,int* varid)
{
	int rl = 0;
	int type;
	char* token;
	statement = trimToken(statement);
	char* ptr1 = statement;
	statement = pythonToken(statement, &type, &token);
	if (type != ARRAYTYPECODE) {
		*result = ERROREXPECTARRAY;
		return statement;
	}
	//convert multi dimension array to assignment into a variable
	//let's count the dimension by [ first
	int dimension = 1;
	statement++;
	while ((*statement == ' ') || (*statement == '['))
	{
		if (*statement == '[') dimension++;
		statement++;
	}
	int allDimension = dimension;
	std::vector<int> dindex;
	for (int i = 0; i < dimension; i++)
		dindex.push_back(0);
	*resultfirst = 0;
	*result = 0;
	code->globalVar.push_back("");
	//empty array-> constant
	if (*statement == ']') {
		ConstantStatement right = (ConstantStatement)initStatement(CONSTANTSTATEMENT);
		right->constant.numproperties = 1;
		right->constant.properties = (Properties*)ALLOCMEM(sizeof(Properties));
		right->constant.properties[0].name = (char*)ALLOCMEM(1);
		right->constant.properties[0].name[0] = 0;
		right->constant.properties[0].value = (char*)ALLOCMEM(2);
		strcpy(right->constant.properties[0].value, "1");
		right->starttext = 0;
		right->endtext = 0;
		int rl = code->statement.size();
		code->statement.push_back((Statement)right);

		AssignmentStatement a = (AssignmentStatement)initStatement(ASSIGNSTATEMENT);
		a->endtext = a->starttext = 0;
		VariableStatement var = (VariableStatement)initStatement(VARIABLESTATEMENT);
		var->type = VARIABLESTATEMENT;
		*varid = var->vid = code->globalVar.size() - 1;
		var->vartype = GLOBALVAR;
		var->starttext = var->endtext = 0;
		var->numproperty = 1;
		var->property = (VariableProperty*)ALLOCMEM(sizeof(VariableProperty) * 1);
		var->property[0].type = STATICVARPROPERTY;
		var->property[0].staticproperty = (char*)ALLOCMEM(10);
		sprintf(var->property[0].staticproperty, "0");
		a->left = code->statement.size();
		code->statement.push_back((Statement)var);
		a->right = rl;
		*result=*resultfirst = code->statement.size();
		code->statement.push_back((Statement)a);
		statement++;
		return statement;
	}
	while (dimension > 0)
	{
		int rl;
		statement = pythonExpressionParser(statement, code, &rl);
		if (rl < 0) {
			*result = rl;
			return statement;
		}
		AssignmentStatement a = (AssignmentStatement)initStatement(ASSIGNSTATEMENT);
		a->endtext = a->starttext = 0;
		VariableStatement var = (VariableStatement)initStatement(VARIABLESTATEMENT);
		var->type = VARIABLESTATEMENT;
		*varid = var->vid = code->globalVar.size() - 1;
		var->vartype = GLOBALVAR;
		var->starttext = var->endtext = 0;
		var->numproperty = allDimension;
		var->property = (VariableProperty*)ALLOCMEM(sizeof(VariableProperty) * allDimension);
		for (int j = 0; j < allDimension; j++) {
			var->property[j].type = STATICVARPROPERTY;
			var->property[j].staticproperty = (char*)ALLOCMEM(10);
			sprintf(var->property[j].staticproperty, "%d", dindex[j]);
		}
		a->left = code->statement.size();
		code->statement.push_back((Statement)var);
		a->right = rl;
		if (*resultfirst == 0) *resultfirst = code->statement.size();
		if (*result != 0) {
			AssignmentStatement as = (AssignmentStatement)code->statement[*result];
			as->next = code->statement.size();
		}
		*result = code->statement.size();
		code->statement.push_back((Statement)a);
		bool expectColon = true;
		while ((dimension>0) && (*statement == ' ') || (*statement == ',') || (*statement == '[') || (*statement == ']'))
		{
			if (*statement == ',') {
				if (!expectColon) {
					*result = ERRORNOTEXPECTCOLON;
					return statement;
				}
				dindex[dimension - 1]++;
				for (int k = dimension; k < allDimension; k++)
				{
					dindex[k] = 0;
				}
				expectColon = false;
			}
			else
				if (*statement == ']') {
					expectColon = true;
					dimension--;
					if (dimension < 0) {
						*result = ERRORBRACKET;
						return ptr1;
					}
				}
				else if (*statement == '[') {
					if (expectColon) {
						*result = ERROREXPECTCOLON;
						return statement;
					}
					dimension++;
					if (dimension > allDimension) {
						*result = ERRORBRACKET;
						return ptr1;
					}
				}
			statement++;
		}
		if ((dimension>0) && (expectColon)) {
			*result = ERROREXPECTCOLON;
			return statement;
		}
	}
	return statement;
}

/**
* get range() for loop
*/
char* pythonRangeParser(char* statement, PythonCode code,int* result,int *startid,int *stopid,int* stepid)
{
	int type;
	char* token;
	statement = trimToken(statement);
	char* ptr1 = statement;
	statement = pythonToken(statement, &type, &token);
	if ((type != ALPHATYPECODE) && (strcmp(token, "range") != 0)) {
	
		*result = ERROREXPECTLOOPRANGE;
		return ptr1;
	}
	statement = trimToken(statement);
	if (*statement != '(') {
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	int numrl=0;
	int rl[3];
	while ((*statement != 0) && (*statement!=')') && (numrl<3)) {
		statement = pythonExpressionParser(statement, code, &rl[numrl]);
		if (rl[numrl] < 0) {
			*result = rl[numrl];
			return statement;
		}
		statement = trimToken(statement);
		numrl++;
		if (*statement == ',') statement++;
	}
	if (*statement != ')') {
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	//convert start, stop, step to assignment into a variable
	if (numrl==0) {
		ConstantStatement c = pythonNewConstant("0");
		rl[0] = code->statement.size();
		code->statement.push_back((Statement)c);
		c = pythonNewConstant("1");
		rl[1] = code->statement.size();
		code->statement.push_back((Statement)c);
		c = pythonNewConstant("1");
		rl[2] = code->statement.size();
		code->statement.push_back((Statement)c);
	}
	else if (numrl == 1) {
		rl[1] = rl[0];
		ConstantStatement c = pythonNewConstant("0");
		rl[0] = code->statement.size();
		code->statement.push_back((Statement)c);
		c = pythonNewConstant("1");
		rl[2] = code->statement.size();
		code->statement.push_back((Statement)c);
	}
	else if (numrl == 2) {
		ConstantStatement c = pythonNewConstant("1");
		rl[2] = code->statement.size();
		code->statement.push_back((Statement)c);						
	}
	*startid = rl[0];
	*stopid = rl[1];
	*stepid = rl[2];
	*result = 0;
	return statement;
}

/**
* get Operand
* Constant or variable or binding or function
* no negative
* 
* ex.
* a.func(1)
* a.b
* a
* 10
* a[0]
* 
* 30/10/2022 operand can be calling
* 9/5/2022
*/
char* pythonOperandParser(char* statement, PythonCode code, int* result)
{
	int rl = 0;
	int type;
	char* token;
	statement = trimToken(statement);
	char* ptr1 = statement;
	statement = pythonToken(statement, &type, &token);
	if (type == ALPHATYPECODE) {
		statement = ptr1;		//back
		statement = pythonVariableParser(statement, code, &rl);
		if (rl < 0)
		{
			*result = rl;
			return ptr1;
		}
		//binding (can be variable) fix 24/7/22
		if (*statement == '(') {
			bool replace = false;
			VariableStatement vstate = (VariableStatement)code->statement[rl];
			if (vstate->numproperty == 0) {
				replace = true;
			} else 
			if (vstate->property[vstate->numproperty - 1].type == DYNAMICVARPROPERTY) {
				*result = ERRORSYNTAX;
				return statement;
			}
			//get var name 
			char* token;
			int type;
			pythonToken(ptr1, &type, &token);
			//can be calling
			int sid = -1;
			for (int i = 0; i < code->subprocedure.size(); i++)
			{
				if (strcmp(code->subprocedure[i]->name, token) == 0) {
					sid = i;
					break;
				}
			}
			if (sid == -1) {	// Binding without variable
				BindingStatement b = (BindingStatement)initStatement(BINDINGSTATEMENT);
				b->isexecute = false;
				if (replace) {
					b->left = 0;
					b->varname = NULL;
					b->func = token;
					code->statement[rl] = (Statement)b;
				}
				else {
					b->left = rl;
					b->varname = token;
					b->func = vstate->property[vstate->numproperty - 1].staticproperty;
					vstate->numproperty--;
					rl = code->statement.size();
					code->statement.push_back((Statement)b);
				}
				int rp;
				statement = pythonBindingParser(statement, b, code, &rp);
				if (rp < 0) {
					*result = rp;
				}
			}
			else {	//calling 
				CallStatement b = (CallStatement)initStatement(CALLSTATEMENT);
				b->isexecute = false;
				if (replace) {
					b->go = code->subprocedure[sid]->go;
					b->numparam = code->subprocedure[sid]->numparam;
					b->stack = code->subprocedure[sid]->numstack;
					code->statement[rl] = (Statement)b;
				}
				else {
					b->go = code->subprocedure[sid]->go;
					b->numparam = code->subprocedure[sid]->numparam;
					b->stack = code->subprocedure[sid]->numstack;
					*result = code->statement.size();
					code->statement.push_back((Statement)b);
				}
				int rc = -1;
				*result = rl;
				statement = pythonCallingParser(statement, b, code, &rc);
				if (rc < 0) {
					*result = rc;
				}
			}
		}
	}
	else if ((type == NUMTYPECODE) || (type==STRINGTYPECODE))  {
		ConstantStatement con = (ConstantStatement)initStatement(CONSTANTSTATEMENT);
		con->constant.numproperties = 1;
		con->constant.properties = (Properties*)ALLOCMEM(sizeof(Properties));
		con->constant.properties[0].name = (char*)ALLOCMEM(1);
		con->constant.properties[0].name[0] = 0;
		con->constant.properties[0].value = (char*)ALLOCMEM(strlen(token)+1);
		strcpy(con->constant.properties[0].value, token);
		con->starttext = ptr1 - code->sourcecode;
		con->endtext = statement - code->sourcecode;
		rl = code->statement.size();
		code->statement.push_back((Statement)con);
	}
	else {
		*result = ERROREXPECTVARIABLE;
		return ptr1;
	}
	*result = rl;
	return statement;
}

/**
* get single comparation
* ex.
* operand compare operand
* a<b
* b<=c
* a(x,y) 
* can be "0" or "1" 
* 
* 
* 9/5/2022
*/
char* pythonCompareParser(char* statement, PythonCode code, int* result)
{
	int type;
	int r1, r2;
	char* token1, *token2,*token;
	char* ptr = statement;

	//left 
	statement = pythonExpressionParser(statement, code, &r1);
	if (r1 < 0) {
		*result = r1;
		return statement;
	}
	//operator
	statement = trimToken(statement);
	char* ns = statement + 1;
	char op[4]; op[0] = 0;
	if ((*statement == '=') && (*ns == '='))
	{
		strcpy(op, "=");
		statement += 2;
	}
	else if ((*statement == '!') && (*ns == '='))
	{
		strcpy(op, "!");
		statement += 2;
	}
	else if ((*statement == '<') && (*ns == '='))
	{
		strcpy(op, "<=");
		statement += 2;
	}
	else if ((*statement == '>') && (*ns == '='))
	{
		strcpy(op, ">=");
		statement += 2;
	}
	else if (*statement == '<')
	{
		strcpy(op, "<");
		statement ++;
	}
	else if (*statement == '>')
	{
		strcpy(op, ">");
		statement ++;
	}
	else {	
		// just add == 1 
		ConstantStatement right = (ConstantStatement)initStatement(CONSTANTSTATEMENT);
		right->constant.numproperties = 1;
		right->constant.properties = (Properties*)ALLOCMEM(sizeof(Properties));
		right->constant.properties[0].name = (char*)ALLOCMEM(1);
		right->constant.properties[0].name[0] = 0;
		right->constant.properties[0].value = (char*)ALLOCMEM(2);
		strcpy(right->constant.properties[0].value, "1");
		right->starttext = 0;
		right->endtext = 0;
		r2 = code->statement.size();
		code->statement.push_back((Statement)right);
		CompareStatement com = (CompareStatement)initStatement(COMPARESTATEMENT);
		com->left = r1;
		com->right = r2;
		com->starttext = ptr - code->sourcecode;
		com->endtext = statement - code->sourcecode;
		strcpy(com->compare, "=");
		*result = code->statement.size();
		code->statement.push_back((Statement)com);
		return statement;
	} 
	//right
	statement=pythonExpressionParser(statement, code, &r2);
	if (r2 < 0) {
		*result = r2;
		return statement;
	}
	CompareStatement com = (CompareStatement)initStatement(COMPARESTATEMENT);
	com->left = r1;
	com->right = r2;
	com->starttext = ptr-code->sourcecode;
	com->endtext = statement-code->sourcecode;
	strcpy(com->compare, op);
	*result = code->statement.size();
	code->statement.push_back((Statement)com);
	return statement;
}

/**
* logic statement for if, while, for
* ex.
* (a<=b) and (b>c) or !(e==f)
*
* 9/5/2022
*/
char* pythonLogicParser(char* statement, PythonCode code, int* result)
{
	int r1 = 0;
	int r2 = 0;
	char operand[12];
	int type;
	char* token;
	statement = trimToken(statement);
	//left
	if (*statement == '(') {
		statement++;
		statement = pythonLogicParser(statement, code, &r1);
		if (r1 < 0) { *result = r1;  return statement; }
		if (*statement != ')') { *result = r1; return statement; }
		statement++;
	} else if (*statement == '!') {
		statement++;
		statement = pythonLogicParser(statement, code, &r1);
		if (r1 < 0) { *result = r1;  return statement; }
		LogicStatement logicstatement = (LogicStatement)initStatement(LOGICSTATEMENT);
		logicstatement->left = r1;
		logicstatement->logic = '!';
		r1 = code->statement.size();
		code->statement.push_back((Statement)logicstatement);
	}
	else {
		statement = pythonCompareParser(statement, code, &r1);
	}
	//operation
	char* ptr1 = statement;
	statement = trimToken(statement);
	char* ns = statement + 1;
	char op[4];
	if (strstr(statement,"or")==statement)
	{
		strcpy(op, "|");
		statement += 2;
	}
	else if (strstr(statement,"and")==statement)
	{
		strcpy(op, "&");
		statement += 3;
	}
	else {
		//only single compare
		*result = r1;
		return statement;
	}
	//result
	statement = pythonLogicParser(statement, code, &r2);
	if (r2 < 0) { *result = r2; return statement; }
	LogicStatement logicstatement = (LogicStatement)initStatement(LOGICSTATEMENT);
	logicstatement->left = r1;
	logicstatement->right = r2;
	logicstatement->logic = op[0];
	*result = code->statement.size();
	code->statement.push_back((Statement)logicstatement);
	return statement;
}

/**
* get expression can be range
* support complex ( and multiple operand
* ex.
* a
* a + b
* a+(b*10)+(c*20)
* -a 
* -10
* a(20)+a.walk(20)+;
* 
* 9/5/2022
*/
char* pythonExpressionParser(char* statement, PythonCode code, int* result)
{
	int r1 = 0;
	int r2 = 0;
	char op[12];
	int type;
	char* token;
	statement = trimToken(statement);
	//left
	if (*statement == '(') {
		statement++;
		statement = pythonExpressionParser(statement, code, &r1);
		if (r1 < 0) { *result = r1;  return statement; }
		statement = trimToken(statement);
		if (*statement != ')') { *result = r1; return statement; }
		statement++;
	}
	//negative (left is zero)
	else if (*statement == '-') {
		ConstantStatement con = (ConstantStatement)initStatement(CONSTANTSTATEMENT);
		con->constant.numproperties = 1;
		con->constant.properties = (Properties*)ALLOCMEM(sizeof(Properties));
		con->constant.properties[0].name = (char*)ALLOCMEM(1);
		con->constant.properties[0].name[0] = 0;
		con->constant.properties[0].value = (char*)ALLOCMEM(2);
		strcpy(con->constant.properties[0].value, "0");
		con->starttext = statement - code->sourcecode;
		con->endtext = statement - code->sourcecode;
		r1 = code->statement.size();
		code->statement.push_back((Statement)con);
	}
	else {
		statement = pythonOperandParser(statement, code, &r1);
		if (r1 < 0) {
			*result = r1;
			return statement;
		}
	}
	//operation
	statement = trimToken(statement);
	char* ptr1 = statement;
	if (ISOPERATOR(*statement))
	{
		statement = pythonToken(statement, &type, &token);
		strcpy(op, token);
		FREEMEM(token);
	}
	else
	{	//only variable
		*result = r1;
		return statement;
	}
	//right
	char* ptr2 = statement;
	statement = pythonExpressionParser(statement, code, &r2);
	if (r2 < 0)
	{
		*result = r2;
		return ptr2;
	}
	//result
	ExpressionStatement expressstatement = (ExpressionStatement)initStatement(EXPRESSIONSTATEMENT);
	expressstatement->left = r1;
	expressstatement->right = r2;
	expressstatement->op = op[0];
	*result = code->statement.size();
	code->statement.push_back((Statement)expressstatement);
	return statement;
}

/**
* set next for
* def, if, while, for
*/
void pythonSetNext(PythonCode code, int start, int end, int next)
{
	for (int i = start; i < end; i++)
	{
		Statement s = code->statement[i];
		if (s->next == 0) {
			if (s->type == ASSIGNSTATEMENT) {
				s->next = next;
			}
			else if (s->type == IFSTATEMENT) {
				IfStatement ifs = (IfStatement)code->statement[i];
				if (ifs->ifthen == 0) ifs->ifthen = next;
				if (ifs->next == 0) ifs->next = next;
			}
			else if (s->type == WHILESTATEMENT) {
				s->next = next;
			}
			else if (s->type == CALLSTATEMENT) {
				CallStatement c = (CallStatement)code->statement[i];
				if (c->isexecute) c->next = next;
			}
			else if (s->type == BINDINGSTATEMENT) {
				BindingStatement c = (BindingStatement)code->statement[i];
				if (c->isexecute) c->next = next;
			}
		}
	}
}

/**
* Finish 31/07/2022
*/
char* pythonForParser(char* statement, PythonCode code,int currentblock, int* result,int *resultfirst)
{
	int type;
	int r1,r2;
	char* name;
	char* range;
	char* token;
	char* ptr1 = statement;
	bool isLocal = false;
	bool isRange = false;
	int startid;
	int stopid;
	int stepid;
	int varrl; //for varrl in varid
	int varid; //for varrl in varid
	int fw;	   //while statement
	//variable
	statement = pythonVariableParser(statement, code, &varrl);
	if (varrl < 0) {
		*result = varrl;
		return statement;		//Parse error
	}
	//in
	ptr1 = statement;
	statement = pythonToken(statement, &type, &token);
	if ((type != ALPHATYPECODE) || (strcmp(token, "in") != 0)) {
		*result = ERROREXPECTIN;
		return ptr1;
	}
	statement = trimToken(statement);
	ptr1 = statement;
	statement = pythonToken(statement, &type, &range);
	if ((type == ARRAYTYPECODE) || ((type == ALPHATYPECODE) && (strcmp(range, "range") == 0)))
	{
		isRange = true;
		statement = pythonRangeParser(ptr1, code, result, &startid, &stopid,&stepid);
		if (*result < 0) {
			return statement;
		}
	}
	else if (type == ALPHATYPECODE) {
		varid=pythonGetVar(code, range,&isLocal);
	}
	else {
		*result = ERROREXPECTLOOPRANGE;
		return ptr1;
	}

	statement = trimToken(statement);
	if (*statement != ':') {
		*result = ERROREXPECTCOLON;
		return statement;
	}
	statement++;
	statement = trimToken(statement);
	if (!ISNEWLINE(*statement)) {
		*result = ERROREXPECTENTER;
		return statement;
	}
	if (*statement != 0) statement++;
	//compile for range
	if (isRange) {	
		AssignmentStatement a1 = (AssignmentStatement)initStatement(ASSIGNSTATEMENT);
		a1->endtext = a1->starttext = 0;
		a1->left = varrl;
		a1->right = startid;
		*resultfirst = code->statement.size();
		code->statement.push_back((Statement)a1);

		WhileStatement whilestatement = (WhileStatement)initStatement(WHILESTATEMENT);
		whilestatement->starttext = whilestatement->endtext = 0;

		CompareStatement c1 = (CompareStatement)initStatement(COMPARESTATEMENT);
		c1->endtext = c1->starttext = 0;
		c1->left = varrl;
		c1->compare[0] = '<';
		c1->compare[1] = 0;
		c1->right = stopid;
		whilestatement->logic = code->statement.size();
		code->statement.push_back((Statement)c1);
		a1->next = fw = code->statement.size();
		code->statement.push_back((Statement)whilestatement);
		statement = pythonRoutineParser((char*)statement, code, currentblock + 1, &r1, &r2);
		if (r1 < 0) {
			*result = r1;
			return statement;
		}
		whilestatement->dowhile = r2;
		AssignmentStatement a2 = (AssignmentStatement)initStatement(ASSIGNSTATEMENT);
		a2->endtext = a2->starttext = 0;
		a2->left = varrl;
		a2->next = fw;
		ExpressionStatement e1 = (ExpressionStatement)initStatement(EXPRESSIONSTATEMENT);
		e1->endtext = e1->starttext = 0;
		e1->left = varrl;
		e1->op = '+';
		e1->right = stepid;
		a2->right = code->statement.size();
		code->statement.push_back((Statement)e1);
		int fa2 = code->statement.size();
		code->statement.push_back((Statement)a2);
		pythonSetNext(code, r2, r1+1, fa2);
		//code->statement[r1]->next = fa2;
	}
	//compile for array
	else {		
		VariableStatement vstep = (VariableStatement)initStatement(VARIABLESTATEMENT);
		vstep->starttext = vstep->endtext = 0;
		code->globalVar.push_back("");
		vstep->vid = code->globalVar.size() - 1;
		vstep->vartype = GLOBALVAR;
		vstep->numproperty = 0;
		vstep->property = NULL;
		stepid = code->statement.size();
		code->statement.push_back((Statement)vstep);

		//Can be optimized
		ConstantStatement c0=pythonNewConstant("0");
		int c0id = code->statement.size();
		code->statement.push_back((Statement)c0);
		ConstantStatement cone = pythonNewConstant("1");
		int c1id = code->statement.size();
		code->statement.push_back((Statement)cone);
		ConstantStatement cnull = pythonNewConstant("");
		int cnullid = code->statement.size();
		code->statement.push_back((Statement)cnull);

		AssignmentStatement a1 = (AssignmentStatement)initStatement(ASSIGNSTATEMENT);
		a1->endtext = a1->starttext = 0;
		a1->left = stepid;
		a1->right = c0id;
		*resultfirst = code->statement.size();
		code->statement.push_back((Statement)a1);

		AssignmentStatement a2 = (AssignmentStatement)initStatement(ASSIGNSTATEMENT);
		a2->endtext = a2->starttext = 0;
		a2->left = varrl;		
		VariableStatement v2s = (VariableStatement)initStatement(VARIABLESTATEMENT);
		v2s->endtext = v2s->starttext = 0;
		v2s->vid = varid;
		v2s->vartype = isLocal;
		v2s->numproperty = 1;
		v2s->property = (VariableProperty*)ALLOCMEM(sizeof(struct VariablePropertyS));
		v2s->property[0].dynamicid = stepid;
		v2s->property[0].staticproperty = NULL;
		v2s->property[0].type = DYNAMICVARPROPERTY;
		a2->right = code->statement.size();
		code->statement.push_back((Statement)v2s);
		int fa=a1->next = code->statement.size();
		code->statement.push_back((Statement)a2);

		WhileStatement whilestatement = (WhileStatement)initStatement(WHILESTATEMENT);
		whilestatement->starttext = whilestatement->endtext = 0;
		CompareStatement c1 = (CompareStatement)initStatement(COMPARESTATEMENT);
		c1->endtext = c1->starttext = 0;
		c1->left = varrl;
		c1->compare[0] = '!';
		c1->compare[1] = '=';
		c1->compare[2] = 0;
		c1->right = cnullid;
		whilestatement->logic = code->statement.size();
		code->statement.push_back((Statement)c1);
		a2->next = fw = code->statement.size();
		code->statement.push_back((Statement)whilestatement);
		statement = pythonRoutineParser((char*)statement, code, currentblock + 1, &r1, &r2);
		if (r1 < 0) {
			*result = r1;
			return statement;
		}
		whilestatement->dowhile = r2;

		AssignmentStatement a3 = (AssignmentStatement)initStatement(ASSIGNSTATEMENT);
		a3->endtext = a3->starttext = 0;
		a3->left = stepid;
		a3->next = fa;
		ExpressionStatement e1 = (ExpressionStatement)initStatement(EXPRESSIONSTATEMENT);
		e1->endtext = e1->starttext = 0;
		e1->left = stepid;
		e1->op = '+';
		e1->right = c1id;
		a3->right = code->statement.size();
		code->statement.push_back((Statement)e1);
		int fa3 = code->statement.size();
		code->statement.push_back((Statement)a3);
		pythonSetNext(code, r2, r1+1, fa3);
		//code->statement[r1]->next = fa3;

	}
	*result = fw;		//last
	return statement;
}

/**
*
* 10/5/2022
*/
char* pythonWhileParser(char* statement, PythonCode code,int currentblock, int* result)
{
	int r1,r2;
	WhileStatement whilestatement = (WhileStatement)initStatement(WHILESTATEMENT);
	int rw = code->statement.size();
	code->statement.push_back((Statement)whilestatement);
	statement = pythonLogicParser(statement, code, &r1);
	whilestatement->logic = r1;
	if (r1 < 0) {
		*result = r1;
		return statement;
	}
	statement = trimToken(statement);
	if (*statement != ':') {
		*result = ERROREXPECTCOLON;
		return statement;
	}
	statement++;
	statement = trimToken(statement);
	if (!ISNEWLINE(*statement)) {
		*result = ERROREXPECTENTER;
		return statement;
	}
	if (*statement != 0) statement++;
	statement = pythonRoutineParser((char*)statement, code, currentblock + 1, &r1,&r2);
	if (r1 < 0) {
		*result = r1;
		return statement;
	}
	whilestatement->dowhile = r2;
	*result = rw;
	pythonSetNext(code, r2, code->statement.size(), rw);
	return statement;
}

/**
* 29/10/2022 fix bug end if
* 10/05/2022
*/
char* pythonIfParser(char* statement, PythonCode code, int currentblock,int* resultif,int* resultlast)
{
	int rl,r1,r2;
	IfStatement ifstatement = (IfStatement)initStatement(IFSTATEMENT);
	int is = code->statement.size();
	code->statement.push_back((Statement)ifstatement);
	statement = pythonLogicParser((char*)statement, code, &rl);
	if (rl < 0) {
		*resultif = rl;
		return statement;
	}
	statement = trimToken(statement);
	if (*statement == ':') {
		statement++;
		statement = trimToken(statement);
	}
	statement = pythonRoutineParser(statement, code, currentblock + 1, &r1,&r2);
	ifstatement->logic = rl;
	ifstatement->ifthen = r2;
	ifstatement->next = 0;
	*resultif = is;
	*resultlast = r1;
	return statement;
}

/*
*
* 10/5/2022
*/
char* pythonElseIfParser(char* statement, PythonCode code,int currentblock, int* resultif,int* resultlast)
{
	int rl,r1,r2;
	IfStatement elseifstatement = (IfStatement)initStatement(IFSTATEMENT);
	int is = code->statement.size();
	code->statement.push_back((Statement)elseifstatement);
	statement = pythonLogicParser((char*)statement, code, &rl);
	if (rl < 0) {
		*resultif = rl;
		return statement;
	}
	statement = trimToken(statement);
	if (*statement == ':') {
		statement++;
		statement = trimToken(statement);
	}
	statement = pythonRoutineParser(statement, code, currentblock + 1, &r1,&r2);
	elseifstatement->logic = rl;
	elseifstatement->ifthen = r2;
	elseifstatement->next = 0;
	*resultif = is;
	*resultlast = r1;
	return statement;
}

/*
* 
* 10/5/2022
*/
char* pythonElseParser(char* statement, PythonCode code, int currentblock,int* resultif,int* resultlast)
{
	int r1,r2;
	IfStatement elsestatement = (IfStatement)initStatement(IFSTATEMENT);
	elsestatement->logic = 0;
	int is = code->statement.size();
	code->statement.push_back((Statement)elsestatement);
	statement = trimToken(statement);
	if (*statement == ':') {
		statement++;
		statement = trimToken(statement);
	}
	statement = pythonRoutineParser(statement, code, currentblock + 1, &r1,&r2);
	elsestatement->ifthen = r2;
	elsestatement->next = 0;
	*resultif = is;
	*resultlast = r1;
	return statement;
}

/**
* Subprocedure
* 10/05/2022
*/
char* pythonDefParser(char* statement, PythonCode code,int currentblock, int* result)
{
	char* token = NULL;
	int type;
	
	statement=trimToken(statement);
	statement = pythonToken((char*)statement, &type, &token);
	if (type != ALPHATYPECODE) {
		if (token != NULL) FREEMEM(token);
		*result = ERROREXPECTFUNCTION;
	}
	if (*statement != '(') {
		if (token != NULL) FREEMEM(token);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	statement = trimToken(statement);

	PythonSubprocedure* sub = (PythonSubprocedure*)ALLOCMEM(sizeof(struct PythonSubprocedureS));
	sub->name = token;
	sub->numparam = 0;
	int maxparam = 1;	//at least
	int countbracket = 0;
	char* s = statement;
	bool end = false;
	//count max param
	while (!end)
	{
		if (*s == ',') maxparam++;
		else if (*s == '(') countbracket++;
		else if ((*s == ')') && (countbracket <= 0))  end = true;
		else if (*s == ')') countbracket--;
		else if ((*s == '\n') || (*s == 0)) end = true;
		s++;
	}
	sub->param = (char**)ALLOCMEM(sizeof(char*)*maxparam);
	//get param
	while (*statement != ')')
	{
		statement = pythonToken((char*)statement, &type, &token);
		if (type != ALPHATYPECODE) {
			if (token != NULL) FREEMEM(token);
			*result = ERROREXPECTVARIABLE;
		}
		statement = trimToken(statement);
		if (*statement == ',') {
			statement++;
			statement = trimToken(statement);
		}
		else if (*statement != ')') {
			if (token != NULL) FREEMEM(token);
			pythonDestroySubprocedure(sub);
			*result = ERROREXPECTBRACKET;  
			return statement;
		}
		sub->param[sub->numparam]=token;
		sub->numparam++;
	}
	statement = trimToken(statement);
	if (*statement != ')') {
		pythonDestroySubprocedure(sub);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	statement = trimToken(statement);
	if (*statement != ':') {
		pythonDestroySubprocedure(sub);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	int rl;
	code->globalOrLocal = false;
	//load localvar
	code->localVar.clear();
	for (int i = 0; i < sub->numparam; i++)
	{
		code->localVar.push_back(sub->param[i]);
	}
	int startstatement = code->statement.size();
	code->subprocedure.push_back(sub);
	code->currentSub = sub;
	sub->numstack = sub->numparam;
	sub->go = -1;	//mark
	statement=pythonRoutineParser(statement, code, currentblock + 1, result, &rl);
	sub->go = rl;
	//replace
	for (int i = 1; i < code->statement.size(); i++)
	{
		if (code->statement[i]->type == CALLSTATEMENT) {
			CallStatement cs = (CallStatement)code->statement[i];
			if (cs->go==-1) cs->go = rl;
		}
	}
	code->globalOrLocal = true;
	if (*result < 0) {
		return statement;
	}
	//add return null here, check ->next=0 to go to return
	rl = code->statement.size();
	ReturnStatement rs = (ReturnStatement)initStatement(RETURNSTATEMENT);
	rs->ret = 0;
	rs->stack=sub->numstack;
	code->statement.push_back((Statement)rs);
	pythonSetNext(code, startstatement, rl, rl);	//set next
	return statement;
}

char* pythonReturnParser(char* statement, PythonCode code, int* result)
{
	int r = -1;
	statement = pythonExpressionParser(statement, code, &r);
	if (r < 0)
	{
		*result = r;
		return statement;
	}
	ReturnStatement s = (ReturnStatement)initStatement(RETURNSTATEMENT);
	s->ret = r;
	s->next = 0;
	s->stack = code->currentSub->numstack;
	*result = code->statement.size();
	code->statement.push_back((Statement)s);
	return statement;
}

/**
* get block (tab)
* 10/5/2022
*/
char* pythonBlock(char* statement, int* block)
{
	*block = 0;
	while ((*statement == ' ') || (*statement=='\t')) {
		statement++;
		*block = (*block) + 1;
	}
	return statement;
}

/**
* parsing source code to codeblock
* 
* @param statement source code
* @param code the binary code
* @param result is the first statement
* @param resultlast is the last statement
*
* @return next in the block
* 
* 10/5/2022
*/
char* pythonRoutineParser(char* statement, PythonCode code, int currentblock,int* result,int* resultfirst)
{
	int type=0;
	int r = 0;
	char* token=NULL;
	Statement last = NULL;
	int ifstatement=0;
	int fw=0,lw=0;
	*resultfirst = 0;
	while (*statement != 0)
	{
		int rf = 0;
		int block=0;
		char* ptrblock = statement;
		statement = pythonBlock((char*)statement, &block);
		char* ptr = statement;
		statement = pythonToken(statement, &type, &token);
		if (type == COMMENTCODE) {
			statement=nextTokenTo(statement,'\n');
		}
		else if (type == DELIMITERCODE) {			//do nothing
		}
		else if (type == ENDCODE) {
			*result = r;  
			return statement;
		} else {
			if (block > currentblock) {
				*result = ERRORTAB;
				return statement;
			}
			else if (block<currentblock) {			//less tab
				*result = r;
				return ptrblock;
			}
			else if (block == currentblock) {
				if (type == ALPHATYPECODE) {
					fw = lw = 0;
					if (strcmp(token, "if") == 0)
					{
						FREEMEM(token);
						statement = pythonIfParser(statement, code, currentblock, &fw, &lw);
						if (fw > 0) {
							code->statement[fw]->starttext = ptr-code->sourcecode;
							code->statement[fw]->endtext = statement-code->sourcecode;
							//11 nov not clear it yet							
							if (ifstatement!=0) pythonSetNext(code, ifstatement,fw, fw);
							r = ifstatement = fw;
						} else {
							*result = fw;
							return statement;
						}
					}
					else if (strcmp(token, "elif") == 0)
					{
						FREEMEM(token);
						if (ifstatement==0)
						{
							*result = ERRORREQUIREIF;
							return statement;
						}
						statement = pythonElseIfParser(statement, code, currentblock, &fw, &lw);
						if (fw > 0) {
							code->statement[fw]->starttext = ptr - code->sourcecode;
							code->statement[fw]->endtext = statement - code->sourcecode;
							r = fw; 
							// ifstatement = fw;
							//code->laststatement.push_back(lw);
						} else {
							*result = fw;
							return statement;
						}
					}
					else if (strcmp(token, "else") == 0)
					{
						FREEMEM(token);
						if (ifstatement==0)
						{
							*result = ERRORREQUIREIF;
							return statement;
						}
						statement = pythonElseParser(statement, code, currentblock, &fw, &lw);
						if (fw > 0) {
							code->statement[fw]->starttext = ptr - code->sourcecode;
							code->statement[fw]->endtext = statement - code->sourcecode;
							r = fw; 
							// ifstatement = fw;
							//code->laststatement.push_back(lw);
						} else {
							*result = fw;
							return statement;
						}
					}
					else if (strcmp(token, "for") == 0)
					{
						FREEMEM(token);
						statement = pythonForParser(statement, code, currentblock, &r,&rf);
						if (r > 0) {
							code->statement[r]->starttext = ptr - code->sourcecode;
							code->statement[r]->endtext = statement - code->sourcecode;
						} else {
							*result = r;
							return statement;
						}
						if (ifstatement != 0) pythonSetNext(code, ifstatement, r, r);
						ifstatement = 0;
					}
					else if (strcmp(token, "while") == 0)
					{
						FREEMEM(token);
						statement = pythonWhileParser(statement, code, currentblock, &r);
						if (r > 0) {
							code->statement[r]->starttext = ptr - code->sourcecode;
							code->statement[r]->endtext = statement - code->sourcecode;
						} else {
							*result = r;
							return statement;
						}
						if (ifstatement != 0) pythonSetNext(code, ifstatement, r, r);
						ifstatement = 0;
					}
					else if (strcmp(token, "def") == 0)
					{
						FREEMEM(token);
						if (!code->globalOrLocal) {
							*result = ERRORSUBPROCEDUREINVALID;
							return ptr;
						}
						int r;
						statement = pythonDefParser((char*)statement, code, currentblock, &r);
						if (r < 0) {
							*result = r;
							return statement;
						}
						r = 0;
						ifstatement = 0;
					}
					else if (strcmp(token, "return") == 0) 
					{
						FREEMEM(token);
						if (code->globalOrLocal) {
							*result = ERRORSUBPROCEDUREINVALID;
							return ptr;
						}
						statement=pythonReturnParser((char*)statement, code, &r);
						if (r < 0) {
							*result = r;
							return statement;
						}
						if (ifstatement != 0) pythonSetNext(code, ifstatement, r, r);
						ifstatement = 0;
					}
					else {
						char* next = statement;
						statement = ptr; //back
						int end = code->statement.size();
						statement = pythonExecuteParser(statement, next, code, &r,&rf);
						if (r < 0) {
							*result = r;
							return statement;
						}
						if (ifstatement != 0) pythonSetNext(code, ifstatement, end, (rf!=0)?rf:r);
						ifstatement = 0;
					}
					if ((*resultfirst == 0) && (rf > 0)) {
						*resultfirst = rf;
					} else 
					if ((*resultfirst==0) && (r>0)) {
						*resultfirst = r;
					}	
				}
				else {
					*result = ERRORSYNTAX;
					return statement;
				}
			}
			//set next 
			if ((last != NULL) && (rf != 0)) {
				last->next = rf;
			}
			else if ((last != NULL) && (r != 0)) {
				last->next = r;
			}
			last = (r != 0)?(Statement)code->statement[r]:NULL;
		}
	}
	*result = (lw>r)?lw:r;
	return statement;	
}

CodeBlock pythonParser(const char* statement,int* result,int* pos)
{
	int i;	
	CodeBlock c;
	char* sta = (char*)ALLOCMEM(strlen(statement) + 1);
	strcpy(sta, statement);
	int numline = getLines((char*)statement);
	PythonCode pc = new PythonCodeS();
	pc->globalOrLocal = true;
	pc->sourcecode = sta;
	pc->statement.push_back((Statement)initStatement(0));					//dummy
	*result = -1;
	int resultfirst;
	char* p = pythonRoutineParser(sta, pc, 0, result,&resultfirst);		//go go go!
	if (*result < 0) {
		c = NULL;
		*pos = p - sta;
	} else {
		c = initCodeBlock(10000);
		c->start = resultfirst;
		c->numstatement = pc->statement.size();
		c->statement = (Statement*)ALLOCMEM(sizeof(struct StatementS) * c->numstatement);
		c->statement[0] = NULL;
		for (int i = 1; i < pc->statement.size(); i++)
		{
			c->statement[i] = pc->statement[i];
		}
		c->numglobalvar = pc->globalVar.size();
		c->sourcecode = (char*)ALLOCMEM(strlen(statement) + 1);
		strcpy(c->sourcecode, statement);
	}
	delete pc;
	FREEMEM(sta); 
	return c;
}


/**
* operand or range
*
char* pythonOperandOrRangeParser(char* statement, PythonCode code, int* result)
{
	int rl = 0;
	int type;
	char* token;
	statement = trimToken(statement);
	char* ptr1 = statement;
	int resultfirst;
	int varid;
	statement = pythonToken(statement, &type, &token);
	if (type == ALPHATYPECODE) {
		if (strcmp(token, "range") == 0) {
			statement=pythonRangeParser(ptr1, code, result,&resultfirst,&varid);
			if (*result < 0) {
				return statement;
			}
			VariableStatement vstate = (VariableStatement)initStatement(VARIABLESTATEMENT);

			vstate->numproperty = 0;
			//TODO:
			return statement;
		}
		else
			return pythonOperandParser(ptr1, code, result);
	}
	else if (type == LEFTBRACKETCODE)
	{
		statement=pythonRangeParser(ptr1, code, result,&resultfirst,&varid);
		if (*result < 0) {
			return statement;
		}

		return statement;
	}
	return pythonOperandParser(ptr1, code, result);
}*/