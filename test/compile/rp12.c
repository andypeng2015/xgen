int s61()
{
  int rc;
  char pc[6];
  for (int j = 0; j < 6; j++)
    while (pc[j])
      if (pc[j]++ < 0)
        rc = 1;
  return 8;
}
