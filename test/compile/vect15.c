int arr[10];
int sum = 0;
void foo()
{
    for (int i = 0; i < 10; i += 2) {
        sum += arr[i];
        //Dependence between store of 'arr' and load of 'arr' prevents
        //vectorization.
        //Report the actions.
        arr[i+1] += sum;
    }
} 
