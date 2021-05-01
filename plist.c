#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <BOperation.h>
#include <unistd.h>

static int list_dir(const char *path, memory *, char *st1, char *st2);
static void store(const char *path, const char *file);
static int treat(const char *path, const char *out, memory *m, char *st1, char *st2);
static int operate(FILE* fl, const char* name, memory *m, char *st1, char *st2);
static int moveOut(const char* name, char* result);

typedef struct files{
  char* fn;
  char* fo;
}files;

static int fsi = 0;
static files fs[1024];
static int max_value = 1024;

int main(int argc, char** argv) {
  int idx = 0;
  memory* m = malloc(getMemorySize());
  char* st1;
  char* st2;

  init(m);
  iniStr(&st1);
  iniStr(&st2);

  if(m == NULL || st1 == NULL || st2 == NULL)
    return 0;

  for(idx = 0;idx < 1024;idx++){
    fs[idx].fn = malloc(sizeof(char) * 256);
    fs[idx].fo = malloc(sizeof(char) * 256);
  }

  while(list_dir("f", m, st1, st2));

  return 0;
}

static int list_dir(const char *path, memory *m, char *st1, char *st2) {
  struct dirent *entry;
  DIR *dir = opendir(path);
  int count = 0;
  int li = 0;
  int efl;

  if (dir == NULL)
    return 0;

  fsi = 0;

  //Store up to 1024 files on "fs". They include input name and output name
  while (((entry = readdir(dir)) != NULL) && count < max_value)
    if(entry->d_name[0] != '.'){
      store(path, entry->d_name);
      ++count;
    }

  closedir(dir);

  if (count == 0)
    return 0;

  //for each stored file, calculate. Then, delete the file
  for(li = 0;li < fsi;li++){
    efl = treat(fs[li].fn, fs[li].fo, m, st1, st2);
    efl = remove(fs[li].fn);

    if(efl != 0)
      return 0;
  }

  //we will return the number of treated files to keep service iterating itself
  return count;
}

//store function. Populates fs structure with input and output route
static void store(const char *path, const char *file){
  char* fname = malloc(sizeof(char) * 256);

  //Memory checking first
  if(fname == NULL)
    return;

  //input file
  snprintf(fname, strlen(path) + strlen(file) + 4, "./%s/%s", path, file);
  memcpy(fs[fsi].fn, fname, sizeof(char) * strlen(fname) + 1);

  //output file
  snprintf(fname, strlen(file) + 7, "./r/r_%s", file);
  memcpy(fs[fsi].fo, fname, sizeof(char) * strlen(fname) + 1);

  fsi++;

  free(fname);
}

//treat function. Opens a file, operate, and then closes it.
static int treat(const char *file, const char* out, memory *m, char *st1, char *st2){
  FILE* fl;
  int opr;

  fl = fopen(file, "r");

  if(fl != NULL){
    opr = operate(fl, out, m, st1, st2);

    if(opr < 0)
      return opr;

    fclose(fl);
  }else
    return -10;

  return 0;
}

//operate function. Reads a file, performs operation and write output if OK
static int operate(FILE* fl, const char* name, memory *m, char *st1, char *st2){
  BigDouble* a = malloc(sizeof(BigDouble));
  BigDouble* b = malloc(sizeof(BigDouble));
  BigInteger* bi = malloc(sizeof(BigInteger));
  int ret;

  int aux;
  char op;

  //Memory checking
  if(a == NULL || b == NULL || bi == NULL){
    moveOut(name, "BI_operation_error_80");
    return -100;
  }

  //get operator
  fscanf(fl, "%c", &op);

  //get values
  if(op == '^' || op == 's'){
    //pow or root.
    fscanf(fl, "%s %i", st1, &aux);

    newBI(bi, st1, 0);
  }else{
    fscanf(fl, "%s %s", st1, st2);

    newBD(a, st1, 0);

    if(getReturnCode() == 0){
      newBD(b, st2, 0);

      if(getReturnCode() == 0){
        toString(a, st1);

        if(getReturnCode() == 0)
          toString(b, st2);
      }
    }
  }

  //operate (only if data creation was OK)
  if(getReturnCode() == 0){
    if(op == '+')
      add(a, b, m);
    else if(op == '-')
      sub(a, b, m);
    else if(op == '*')
      mul(a, b, m);
    else if(op == '/')
      dvs(a, b, m);
    else if(op == '^')
      bipow(bi, aux, m);
    else if(op == 's')
      nqrt(bi, aux, m);
    else{
      moveOut(name, "BI_operation_error_81");
      return -101;
    }
  }else{
    moveOut(name, "BI_operation_error_82");
    return -102;
  }

  //translate to string (only if operation went OK)
  if(getReturnCode() == 0){
    if(op == '^' || op == 's')
      toString(bi, st1);
    else
      toString(a, st1);
  }else{
    moveOut(name, "BI_operation_error_83");
    return -103;
  }

  //write it (only if operation went OK)
  if(getReturnCode() ==  0)
    ret = moveOut(name, st1);
  else{
    moveOut(name, "BI_operation_error_84");
    return -104;
  }

  free(a);
  free(b);
  free(bi);

  return ret;
}

//moveOut function. Writes the result of an operation
static int moveOut(const char* name, char* result){
  FILE* ofl;

  ofl = fopen(name, "w");

  if(ofl == NULL)
    return -1;

  fputs(result, ofl);
  fclose(ofl);

  return 0;
}
