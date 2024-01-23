char * strdup(char * source){
  char * pnt = 0;
  int len = strlen(source);
  if(len > 0){
	pnt = (char *) malloc(strlen(source) + 1);
	strcpy(pnt, source);
  }
  return pnt;
}

