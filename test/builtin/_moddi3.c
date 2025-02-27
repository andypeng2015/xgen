unsigned int clz64 (unsigned long long a);
unsigned long long  __moddi3(unsigned long long a, unsigned long long b)
{
    unsigned int leading_zero_a;
    unsigned int leading_zero_b;
    unsigned int diff_leading_zero = 0;
    unsigned long long quotient = 0;
    int n;
    long long sign_a, sign_b, sign = 0;

    sign_a = 0x8000000000000000 & a;
    sign_b = 0x8000000000000000 & b;

    if (sign_a != sign_b)
    sign = 0x8000000000000000;

    if (sign_a < 0)
    a = sign - a;
    if (sign_b < 0)
    b = sign - b;

    a = a & 0x7fffffffffffffff;
    b = b & 0x7fffffffffffffff;

    leading_zero_a = clz64(a);
    leading_zero_b = clz64(b);

    if (b == 0)
    {
    if (sign < 0)
    return 0x8000000000000000;
    else
    return 0x7fffffffffffffff;
    }

    if (leading_zero_a == leading_zero_b)
    {
        if (a >= b)
        {
            if (sign_a < 0)
                    return b - a;
            else
            return a - b;
        }
        else // a < b
        {
            if (sign_a < 0)
                return -a;
            else
                return a;
        }

    }
    else if (leading_zero_a > leading_zero_b)
    {
        if (sign_a < 0)
            return -a;
        else
            return a;
    }
    else
    {
        diff_leading_zero = (leading_zero_b - leading_zero_a);
        b = b << diff_leading_zero;

        for (n = 0; n < diff_leading_zero + 1; n++)
        {
            quotient = quotient << 1;
            if (a >= b)
            {
                a = a - b;
                quotient++;
            }
            a = a << 1;
        }
    }
    a = a >> (diff_leading_zero + 1);
    if (sign_a < 0)
        return  - a;
    return a;
}
