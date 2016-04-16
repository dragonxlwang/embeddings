#ifndef UTIL_FILE
#define UTIL_FILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

char *FilePathExpand(char *fp) {
  wordexp_t ep;
  wordexp(fp, &ep, 0);
  char *nfp = malloc(strlen(ep.we_wordv[0]) + 1);
  strcpy(nfp, ep.we_wordv[0]);
  wordfree(&ep);
  return nfp;
}

char *FilePathSubExtension(char *fp, char *ext) {
  int i = strlen(fp) - 1;
  while (i >= 0 && fp[i] != '.') i--;
  if (i < 0) i = strlen(fp);
  char *nfp = (char *)malloc(i + 1 + strlen(ext) + 1);
  memcpy(nfp, fp, i);
  memcpy(nfp + i, ".", 1);
  memcpy(nfp + i + 1, ext, strlen(ext));
  nfp[i + 1 + strlen(ext)] = '\0';
  return nfp;
}

#endif /* ifndef UTIL_FILE */
