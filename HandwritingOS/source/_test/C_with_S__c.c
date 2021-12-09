/*
* @Author: Yooj
* @Date:   2021-12-09 19:48:05
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-09 20:05:39
*/
extern void asm_print(char*, int);

void c_print(char* str)
{
    int len = 0;
    while(str[len++]);
    asm_print(str, len); // 调用汇编文件中函数
}