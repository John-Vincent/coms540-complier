int i[5], j[6];
char c;
float f;

int main()
{
    /*
    Read two digits and sum them.
    No spaces between the digits on input.
    Also no error checking, if the two characters
    are not digits, we get interesting results.
    Also, we do a lot more casting than normal C,
    in case student compilers don’t automatically
    coerce types from int to char or char to int.
    */
    char d1, d2, sum;
    d1 = (char) getchar() - '0';
    d2 = (char) getchar() - '0';
    sum = d1 + d2;
    putchar((int)(d1 + '0'));
    putchar((int)'+');
    putchar((int)(d2 + '0'));
    putchar((int)'=');
    /*
    Print the two digit sum, one digit at a time.
    Leading digit might be zero, but there’s no
    way to suppress that without using if.
    */
    putchar((int)(sum / 10 + '0'));
    putchar((int)(sum % 10 + '0'));
    putchar((int)'\n');
    return sum; /* The stack VM will show the value returned by main() */
}

void memes(int i, char asdf, float f[5])
{
    char c[20], v;
    int k;

    c[k+10] = 'c';
    k = -5;
    "hello";
    k++;
}
