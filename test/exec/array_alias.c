int printf(char const*,...);

void foo(int p[10][20])
{
    p[9][1] = 30;
}

int main()
{
    //CP and DCE are illegal to perform because the alias.
    int ga[10][20];
    int x;
    ga[2][1]=5678;
    ga[9][1]=4321;
    foo(&ga);
    x = ga[2][1];
    printf("\n%d\n", ga[9][1]);
    return 0;
}
