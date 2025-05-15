/* File: palindrome.c 
   Author: Richard Hsin
   
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> //allows to use "bool" as a boolean type
#include <ctype.h>
#include <string.h>

/*Optional functions, uncomment the next two lines
 * if you want to create these functions after main: */
//bool readLine(char** line, size_t* size, size_t* length);

/* 
  * NOTE that I used char** for the line above... this is a pointer to
  * a char pointer.  I used this because of the availability of
  * a newer function getline which takes 3 arguments (you should look it
  * up) and the first argument is a char**.  You can create a char*, say
  * called var, and to make it a char** just use &var when calling this
  * function.  If this is too confusing, you can use fgets instead.  Feel
  * free to change the function prototypes as you need them.
  * Also, note the use of size_t as a type.  You can look this up, but
  * essentially, this is just a special type of int to track sizes of
  * things like strings...
*/

//bool isPalindrome(const char* line, size_t len);

bool readLine(char** line, size_t* size, size_t* length) {
  size_t read = getline(line, size, stdin);
  if (read == -1) {
    return false;
  }
  *length = (size_t)read;
  if ((*line)[*length - 1] == '\n') {
    (*line)[*length - 1] = '\0';
    (*length)--;
  }
  return true;
}

bool isPalindrome(const char* line, size_t len) {
  char filtered[100];
  size_t f_len = 0;
  
  // Filter only alphabetic characters
  for (size_t i = 0; i < len; i++) {
    if (isalpha(line[i])) {
      filtered[f_len++] = tolower(line[i]); 
    }
  }
  filtered[f_len] = '\0';
  
  // Check if it's a palindrome
  for (size_t i = 0, j = f_len - 1; i < j; i++, j--) {
    if (filtered[i] != filtered[j]) {
      return false;
    }
  }
  return true;
}

int main() {
  char* input = NULL;
  size_t size = 0, length = 0;
  
  while (1) {
    printf("Enter a word (or '.' to quit): ");
    if (!readLine(&input, &size, &length)) {
      break;
    }
    
    if (strcmp(input, ".") == 0) {
      break;
    }
    
    if (isPalindrome(input, length)) {
      printf("'%s' is a palindrome!\n", input);
    } else {
      printf("'%s' is not a palindrome.\n", input);
    }
  }
  
  printf("Never back down never what?\n");
  free(input);
  return 0;
}
