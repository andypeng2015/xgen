
static int AAA[] = {
    1, 2, 3, 4
};

int sum;
int main (void)
{
    sum = AAA[0]; //�����ǰ�ļ��У�AAA��static�ģ�����û����Ϊ�쳣��ָ��
                           //e.g: int *p=100;  ��AAA[0] Ӧ�ñ��滻��1
    return 0;
}

