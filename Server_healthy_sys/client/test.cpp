/*************************************************************************
	> File Name: test.cpp
	> Author: 
	> Mail: 
	> Created Time: 2020年09月08日 星期二 03时52分36秒
 ************************************************************************/

#include<iostream>
using namespace std;


int main() {

    char *p = (char *)malloc(sizeof(char) * 10);
    char *s;
    p = "dfdf";
    s = "ddd";
    cout << sizeof(p) << endl;
    cout << sizeof(s) << endl;
    cout << p << endl;
    return 0;
}
