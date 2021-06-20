FROM alpine:3.13.5
USER root
WORKDIR /root/dumboss

RUN apk update
RUN apk add llvm10 clang lld nasm mtools binutils

ENV CFLAGS='-c -Os --std=gnu17 -target x86_64-unknown-elf -ffreestanding \
        -mno-red-zone -nostdlib -fPIC -Iinclude \
        -Wall -Wextra -Werror -Wconversion'

ENV BOOT_CFLAGS='-Os --std=gnu17 -target x86_64-unknown-windows -ffreestanding \
        -fshort-wchar -mno-red-zone \
        -nostdlib -Wall -Wextra -Werror -Wconversion \
        -Iinclude'

ENV BOOT_LDFLAGS='-target x86_64-unknown-windows -nostdlib -Wl,-entry:efi_main \
        -Wl,-subsystem:efi_application -fuse-ld=lld-link'

COPY ./include ./include
COPY ./bootloader ./bootloader
COPY ./common ./common
RUN clang $BOOT_CFLAGS $BOOT_LDFLAGS -o BOOTX64.EFI ./bootloader/*.c ./common/*.c

COPY ./src ./src
RUN nasm -f elf64 -o kmain.o src/entry.asm
RUN clang $CFLAGS src/*.c common/*.c
RUN ld.lld --oformat binary --pie --script src/link.ld -o os ./*.o

RUN dd if=/dev/zero of=kernel bs=1k count=1440
RUN mformat -i kernel -f 1440 ::
RUN mmd -i kernel ::/EFI
RUN mmd -i kernel ::/EFI/BOOT
RUN mcopy -i kernel BOOTX64.EFI ::/EFI/BOOT
RUN mcopy -i kernel os ::/kernel

ENTRYPOINT ["cat", "kernel"]
