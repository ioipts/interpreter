/**
* compile to bytecode.
* save the bytecode.
* load the bytecode.
* then execute it.
*/
#include "interpreter.h"
#include "binaryparser.h"
#include "pythonparser.h"
#include <iostream>
#include <string>
#include <sstream>
#include <fstream> 

using namespace std;

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

string readfile(string fileName)
{
	ifstream inFile;
    inFile.open(fileName); 
    stringstream strStream;
    strStream << inFile.rdbuf();
	inFile.close(); 
    return strStream.str(); 
}

void writefile(string fileName,void* bin,size_t len)
{
	ofstream wf(fileName, ios::out | ios::binary);
    if(!wf) {
      cout << "Cannot open file!" << endl;
      return ;
    }
    wf.write((char *) bin, len);
    wf.close();
}

int main(int argc, char** argv)
{
	int result, pos;

	if (argc<3) {
		cout << "usage: testinterpreter <input.py> <output.bin>" << endl;
		return 0;
	}
	string codestr=readfile(argv[1]);
	CodeBlock c = pythonParser(codestr.c_str(),&result,&pos);
	if (c == NULL) {
		cout << "failed\n" << endl;
		return 0;
	}
	size_t len;
	void* bin = savecodeblock(c, &len);
	writefile(string(argv[2]),bin,len);
	CodeBlock ctest=loadcodeblock(bin, len);
	runCodeBlock(ctest, pythoncallback, NULL);
	destroyCodeBlock(ctest);
	destroyCodeBlock(c);
	free(bin);
	return 0;
}


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