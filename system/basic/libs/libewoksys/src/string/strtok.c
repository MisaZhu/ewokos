

char *strrchr(const char *s, int i) {
  const char *last = 0;

  for (; *s; ++s)
    if (*s == i) last = s;

  return (char *)last;
}

char *strerror(int errnum) {
  if (errnum == 0) return "no error";
  return "unexpected error";
}

int strspn(const char *s, const char *accept) {
  const char *p;
  const char *a;
  int count = 0;

  for (p = s; *p != '\0'; ++p) {
    for (a = accept; *a != '\0'; ++a)
      if (*p == *a) break;
    if (*a == '\0')
      return count;
    else
      ++count;
  }

  return count;
}

 const void *rawmemchr(const void *spp, int c) {
   const unsigned char *s = spp;
 
   while (*s != (unsigned char)c) s++;
 
   return s;
 }

const char *strpbrk(const char *s, const char *accept) {
  while (*s != '\0') {
    const char *a = accept;
    while (*a != '\0')
      if (*a++ == *s) return s;
    ++s;
  }

  return 0;
}


char *strtok(char *s, const char *delim, char **saveptr) {
  char *token;

  if (s == 0) s = *saveptr;

  /* Scan leading delimiters.  */
  s += strspn(s, delim);
  if (*s == '\0') {
    *saveptr = s;
    return 0;
  }

  /* Find the end of the token.  */
  token = s;
  s = (char *)strpbrk(token, delim);
  if (s == 0) {
    /* This token finishes the string.  */
    *saveptr = (char *)rawmemchr(token, '\0');
  } else {
    /* Terminate the token and make *saveptr point past it.  */
    *s = '\0';
    *saveptr = s + 1;
  }
  return token;
}
