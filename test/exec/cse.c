extern void exit(int);
extern void abort();
void printf(char const*,...);
int main()
{
    int a,b,c,d;
    if (b > d) {
        b = a + 2;
        c = 4 * b;
        if (b < c) {
            b = 1;
        }
        d = a + 2;
    } else {
        a = 3;
        d = a + 2;
        if (d == 2) {
            while (1) {
                c = a + 2;
            }
        }
    }
    return d;
}
