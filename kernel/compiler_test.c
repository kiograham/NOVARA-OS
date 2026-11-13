void kernel_test(void)
{
    volatile unsigned short *video =
        (unsigned short *)0xB8000;

    video[0] = 0x0F4E; // N
    video[1] = 0x0F4F; // O
    video[2] = 0x0F56; // V
    video[3] = 0x0F41; // A
}
