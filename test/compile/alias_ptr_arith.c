//lda.gpr.nram %r0, N1��                        r0 -> MDx1 (MD_base(MDx1) = N1, ofst = 0, size = N1�ռ��С)
//add.gpr.s48 %r0, %r0, -64;                    r0 -> MDx1 (MD_base(MDx1) = N1, ofst = -64, size = N1�ռ��С)
//add.gpr.s48 %r1, %r0, 128;                   r1 -> MDx1 (MD_base(MDx1) = N1, ofst = 64, size = N1�ռ��С)
int N1[100];
void * foo()
{
    void * r0;
    void * r1;
    r0 = &N1;
    r0 = r0 - 64;
    r1 = r0 + 128;
    return r1;
}
