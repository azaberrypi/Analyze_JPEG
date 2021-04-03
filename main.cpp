#include <stdio.h>
#include <stdlib.h>

#define gps_byte 256

int binary_to_sequence(char *str, int big_endian, int first_address, int offset) {
    int sequence = 0;
    int last_address = first_address + offset;

    if (big_endian == 1) {
        for (int cnt = 0; cnt < offset; cnt++) {
            sequence *= 256;
            sequence += str[first_address++];
        }
    } else {    // I haven't check completely yet.
        for (int cnt = 0; cnt < offset; cnt++) {
            sequence *= 256;
            sequence += str[last_address--];
        }
    }
    return sequence;
}

int main(void) {
    FILE *fp;

    //char *filename = "C:\\Users\\aomid\\OneDrive\\Desktop\\DSC_0002.JPG";
    char *filename = "C:\\Users\\aomid\\DSC_0002.JPG";
    char str[8192];
    char gps[gps_byte];

    if ((fp = fopen(filename, "rb")) == NULL) {
        printf("%s couldn't be opened\n", filename);
        exit(1);
    } else {
        puts("succsess");
    }

    int cnt;
    for (cnt = 0; cnt < 8192; cnt++) {
        str[cnt] = fgetc(fp);
    }


    /*Checking whether jpeg or not.*/
    if (str[6] != 'E' || str[7] != 'x' || str[8] != 'i' || str[9] != 'f') {
        printf("%s is not jpeg.\n", filename);
        exit(1);
    }


    /*Veryfing endianness*/
    int big_endian = -1;
    int TIFF_header_offset = 0;
    for (cnt = 0; cnt < 20; cnt++) {
        if (str[cnt] == 0x4d && str[cnt + 1] == 0x4d) {
            big_endian = 1;
            TIFF_header_offset = cnt;
            cnt += 2;
            break;
        } else if (str[cnt] == 0x49 && str[cnt + 1] == 0x49) {
            big_endian = 0;
            TIFF_header_offset = cnt;
            cnt += 2;
            break;
        }
    }

    if (big_endian) {
        puts("big endian");
    } else {
        puts("little endian");
    }


    /*Verifying the type of TIFF*/
    if (str[cnt++] == 0x00 && str[cnt++] == 0x2A) {
        puts("This is TIFF file.");
    } else {
        puts("This is BigTIFF file.");
        puts("Learn first about it.");
        exit(-1);
    }


    /*Getting 0th IFD's offset*/
    int _0th_IFD_offset = binary_to_sequence(str, big_endian, cnt, 4);
    printf("0th IFD offset : %d Byte\n", _0th_IFD_offset);


    /*Getting IFD offset*/
    int IFD_offset = TIFF_header_offset + _0th_IFD_offset;
    printf("IFD offset : 0x%x\n", IFD_offset);


    /*Getting number of entry*/
    int number_of_entry = binary_to_sequence(str, big_endian, IFD_offset, 2);
    printf("number_of_entry : 0x%x\n", number_of_entry);
    cnt = IFD_offset + 2;

    do {

        number_of_entry--;
    } while (number_of_entry == 0);

    //printf("gps_offset : 0x%x\n", gps_offset);

    //printf("https://www.google.com/maps?q=%d,%d\n", ,);

    fclose(fp);
    return 0;
}