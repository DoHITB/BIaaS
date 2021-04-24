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

  //printf("Service is awake!\n");

  if(m == NULL || st1 == NULL || st2 == NULL){
    //printf("Memory error\n");

    return 0;
  }

  for(idx = 0;idx < 1024;idx++){
    fs[idx].fn = malloc(sizeof(char) * 256);
    fs[idx].fo = malloc(sizeof(char) * 256);
  }

  while(list_dir("f", m, st1, st2));

  //printf("Service is done\n");

  return 0;
}

static int list_dir(const char *path, memory *m, char *st1, char *st2) {
  struct dirent *entry;
  DIR *dir = opendir(path);
  int count = 0;
  int li = 0;
  int efl;

  if (dir == NULL){
    //printf("\tDirectory %s does not exist!\n\tExiting...\n");
    return 0;
  }

  //printf("\tDirectory open...\n");

  fsi = 0;

  //Store up to 1024 files on "fs". They include input name and output name
  while (((entry = readdir(dir)) != NULL) && count < max_value)
    if(entry->d_name[0] != '.'){
      //printf("\t\tStoring: %s\n",entry->d_name);

      store(path, entry->d_name);
      ++count;
    }

  closedir(dir);

  if (count == 0){
    //printf("\tNo files found\n");
    return 0;
  }

  //printf("\t%i files stored\n", count);
  //printf("\tTreating files\n");

  //for each stored file, calculate. If it goes OK, delete the file
  for(li = 0;li < fsi;li++){
    efl = treat(fs[li].fn, fs[li].fo, m, st1, st2);

    if(efl == 0){
      //printf("\t\tRemoving... %s\n", fs[li].fn);

      efl = remove(fs[li].fn);

      if(efl != 0){
        //printf("\t\t\tSomething went wrong. Status: %i\n", efl);

        return 0;
      }
    }else{
      //printf("\t\tError on calculation\n");

      return 0;
    }
  }

  //printf("\t%i files were successfully removed\n", li);

  //we will return the number of treated files to keep service iterating itself
  return count;
}

//store function. Populates fs structure with input and output route
static void store(const char *path, const char *file){
  char* fname = malloc(sizeof(char) * 256);

  //Memory checking first
  if(fname == NULL){
    return;
  }

  //input file
  snprintf(fname, strlen(path) + strlen(file) + 4, "./%s/%s", path, file);
  memcpy(fs[fsi].fn, fname, sizeof(char) * strlen(fname) + 1);

  //output file
  snprintf(fname, strlen(file) + 7, "./r/r_%s", file);
  memcpy(fs[fsi].fo, fname, sizeof(char) * strlen(fname) + 1);

  //printf("\t\t\tFile added: {%s - %s}\n", fs[fsi].fn, fs[fsi].fo);

  fsi++;

  free(fname);
}

//treat function. Opens a file, operate, and then closes it.
static int treat(const char *file, const char* out, memory *m, char *st1, char *st2){
  FILE* fl;
  int efl = -1;
  int opr;

  //printf("\t\t\tOpening: %s\n", file);

  fl = fopen(file, "r");

  if(fl != NULL){
   //printf("\t\t\tTreating...\n");

    opr = operate(fl, out, m, st1, st2);

    if(opr < 0){
      //printf("\t\t\tError %i on operate\n", opr);

      return opr;
    }

    fclose(fl);
  }else{
    //printf("\t\t\tError!\n");

    return -10;
  }

  return 0;
}

//operate function. Reads a file, performs operation and write output if OK
static int operate(FILE* fl, const char* name, memory *m, char *st1, char *st2){
  BigDouble* a = malloc(sizeof(BigDouble));
  BigDouble* b = malloc(sizeof(BigDouble));
  BigInteger* bi = malloc(sizeof(BigInteger));

  int aux;
  char op;
  int ret;

  //Memory checking
  if(a == NULL || b == NULL || bi == NULL)
    return -10;

  //printf("\t\t\t\tGather data\n");

  //get operator
  fscanf(fl, "%c", &op);

  //printf("\t\t\t\t\tOperation: %c\n", op);

  //get values
  if(op == '^' || op == 's'){
    //pow or root.
    fscanf(fl, "%s %i", st1, &aux);

    //printf("\t\t\t\t\tOperators: <%s>, <%i>\n", st1, aux);

    newBI(bi, st1, 0);
  }else{
    fscanf(fl, "%s %s", st1, st2);

    //printf("\t\t\t\t\tOperators: <%s>, <%s>\n", st1, st2);

    newBD(a, st1, 0);
    newBD(b, st2, 0);

    toString(a, st1);
    toString(b, st2);
  }

  //operate
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
  else
    return -11;

  //translate to string
  if(op == '^' || op == 's')
    toString(bi, st1);
  else
    toString(a, st1);

  //write it
  ret = moveOut(name, st1);

  free(a);
  free(b);
  free(bi);

  return ret;
}

//moveOut function. Writes the result of an operation
static int moveOut(const char* name, char* result){
  FILE* ofl;

  //printf("\t\t\t\tCreating... %s\n", name);

  ofl = fopen(name, "w");

  if(ofl == NULL){
    //printf("\t\t\t\tError creating file %s\n", name);

    return -1;
  }

  fputs(result, ofl);
  fclose(ofl);

  //printf("\t\t\t\tFile created: %s\n", name);

  return 0;
}
