/* 
 * p00.c
 */

/* 
 * usage:
 * 
 *   ./a.out
#include 
 *
 * Intented behavior:
 * It should print the content of this file.
 *
 */
#include <stdio.h>
int main()
{
  FILE * fp = fopen("problems/00/p00.c", "rb");
  while (1) {
    int c = fgetc(fp);
    if (c == EOF) break;
    fputc(c, stdout);
  }
  return 0;
}

