void main() {
  const short color = 0x0F00;
  const char *hello = "Hello world!";
  short *vga = (short *)0xb8000;
  for (int i = 0; i < 16; i++)
    vga[i] = color | hello[i];
}
