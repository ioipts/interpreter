#include "interpreter.h"
#include "binaryparser.h"
#include "pythonparser.h"

#if defined(_MSC_VER)
	#define FILE_NAME "C:\\Users\\Pit\\Downloads\\l3.bin"
	//#define FILE_NAME "D:\\Downloads\\f2.bin"
#else
	#define FILE_NAME "k10.bin"
#endif

bool __cdecl pythoncallback(Variable data, const char* varname, const char* properties, const char* funcname, Value** param, int numparam, void* id,Value* result)
{
	if (strcmp(funcname, "print") == 0) {
		printf("print %s\n", param[0]->properties[0].value);
	}
	else {
		printf("call %s\n", funcname);
	}
	Value* r = initValue("1");
	setVariable(result, "", r);
	destroyValue(r);
	return true;
}

int main(int argc, char** argv)
{
	const char* w1 =
		"x=0"
		"while (x<10):\n"
		"	x=x+1\n"
		"print(x)\n";
	const char* s1 = 
		"x=6\n"
		"if x > 10 :\n"
		"	print(\"Above 10,\")\n"
		"elif x > 50 :\n"
		"	print(\"and also above 50!\")\n"
		"else :\n"
		"	print(\"but not above 10.\")\n";
	const char* t1 =
		"tractor.move(0,0)\n"
		"tractor[10].move(10,8)\n";
	const char* f1 =
		"def test(b) :\n"
		"	print(b)\n"
		"x=\"test\"\n"
		"test(x)\n";
	const char* f2 =
		"def test(b):\n"
		"	if (b < 15):\n"
		"		test(b + 1)\n"
		"		print(b)\n"	
		"	else:\n"
		"		print(\"ret\")\n"	
		"x = 10\n"
		"test(x)\n"
		"print(\"end\")\n";
	const char* k1 =
		"k=tractor.move(0,0)\n"
		"if (k==\"2\"):\n"
		"	tractor[10].move(10, 8)\n";
	const char* l1 =
		"for x in range(0, 10, 1) :\n"
		"	print(x+1)\n";
	const char* l2 =
		"a = [ 1 , 4 , 5 , 6]\n"
		"print(a[2])\n"
		"for x in a :\n"
		"	print(x+1)\n";
	const char* l3 =
		"a = [ [1 , 4] , [5 , 6]]\n"
		"print(a[1][1])\n";
	int result, pos;
	CodeBlock c = pythonParser(l3,&result,&pos);
	//runCodeBlock(c, pythoncallback, NULL);
	/*char debug[60000];
	for (int i = 1; i < c->numstatement; i++)
	{
		Statement s = c->statement[i];
		int len = s->endtext - s->starttext;
		strncpy(debug, &c->sourcecode[s->starttext],len);
		debug[len] = 0;
		printf("%d : %s\n",s->type,debug);
	}*/
	if (c == NULL) {
		printf("failed\n");
		return 0;
	}
	size_t len;
	void* bin = savecodeblock(c, &len);
	CodeBlock ctest=loadcodeblock(bin, len);
	runCodeBlock(ctest, pythoncallback, NULL);
	FILE* f = fopen(FILE_NAME,"wb");
	fwrite(bin, len, 1, f);
	fclose(f);
	destroyCodeBlock(ctest);
	destroyCodeBlock(c);

	return 0;
}

/*const char* s2 = "a=((10+20)+30)/(2)"
	"print(a)";
CodeBlock c1=pythonParser(s2);
runCodeBlock(c1,pythoncallback,NULL);
destroyCodeBlock(c1);

CodeBlock c3 = pythonParser("print(\"test\"+\" \"+\"string\")");
runCodeBlock(c3,pythoncallback,NULL);
destroyCodeBlock(c3);*/

//int leak = getdebug();
//printf("leak: %d\n", leak);

