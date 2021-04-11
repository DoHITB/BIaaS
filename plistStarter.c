#include <sys/types.h>
#include <dirent.h>
#include<unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

pid_t proc_find(const char* name);

pid_t proc_find(const char* name) {
  DIR* dir;
  struct dirent* ent;
  char buf[512];

  long  pid;
  char pname[100] = {0,};
  char state;
  FILE *fp = NULL; 

  if (!(dir = opendir("/proc"))) {
    perror("can't open /proc");
    return -1;
  }

  while((ent = readdir(dir)) != NULL) {
    long lpid = atol(ent->d_name);
        
    if(lpid < 0)
      continue;
    
    snprintf(buf, sizeof(buf), "/proc/%ld/stat", lpid);
    fp = fopen(buf, "r");

    if (fp) {
      if ((fscanf(fp, "%ld (%[^)]) %c", &pid, pname, &state)) != 3 ){
        printf("fscanf failed \n");
        fclose(fp);
        closedir(dir);
        return -1; 
      }
      
      if (!strcmp(pname, name)) {
        fclose(fp);
        closedir(dir);
        
        return (pid_t)lpid;
      }
            
      fclose(fp);
    }
  }

  closedir(dir);
  return -1;
}

int main(int argc, char* argv[]) {
  //-1: plist is out
  return proc_find("plist");
}