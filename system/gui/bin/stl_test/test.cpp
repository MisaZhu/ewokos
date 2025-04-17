#include <string>
#include <vector>
#include <stdio.h>

using namespace std;

int main(int argc, char** argv) {
	vector<int> v;
	v.push_back(100);
	string str = "";
	str += "aaa";
	str += "xxx";
	printf("%s, %d\n", str.c_str(), v.at(0));

	return 0;
}