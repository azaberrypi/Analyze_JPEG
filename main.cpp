#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <iostream>

#define gps_byte 256

uint32_t binary_to_sequence(const uint8_t *str, int big_endian, uintptr_t first_address, int offset) {
    uint32_t sequence = 0;
    uint32_t last_address = first_address + offset;

    if (big_endian == 1) {
        for (uint64_t cnt = 0; cnt < offset; cnt++) {
            sequence <<= 8;
            sequence |= str[first_address++];
        }
    } else {    // I haven't check completely yet.
        for (uint64_t cnt = 0; cnt < offset; cnt++) {
            sequence *= 256;
            sequence += str[last_address--];
        }
    }
    return sequence;
}

uint16_t get_IFD_type(uint32_t sequence) {
    switch (sequence) {
        case 1:
            //BYTE
            return 1;
        case 2:
            //ASCII
            return 1;
        case 3:
            //SHORT
            return 2;
        case 4:
            //LONG
            return 4;
        case 5:
            //RATIONAL
            return 8;
        case 7:
            //UNDEFINED
            return 1;
        case 9:
            //SLONG
            return 4;
        case 10:
            //SRATIONAL
            return 8;
    }
}

int main(int argc, char *argv[]) {
    using namespace std;
    FILE *fp;

    if (argc != 2) {
        cerr << "Invalid arguments!";
        exit(1);
    }
    //char *filename = "C:\\Users\\aomid\\OneDrive\\Desktop\\DSC_0002.JPG";
    char *filename = argv[1];
    uint8_t buff[8192];
    uint8_t gps[gps_byte];

    if ((fp = fopen(filename, "rb")) == NULL) {
        printf("%s couldn't be opened\n", filename);
        exit(1);
    } else {
        puts("succsess");
    }

    int cnt;
    for (cnt = 0; cnt < 8192; cnt++) {
        buff[cnt] = fgetc(fp);
    }


    /*Checking whether jpeg or not.*/
    if (buff[6] != 'E' || buff[7] != 'x' || buff[8] != 'i' || buff[9] != 'f') {
        printf("%s is not jpeg.\n", filename);
        exit(1);
    }


    /*Veryfing endianness*/
    int big_endian = -1;
    int tiff_header_offset = 0;
    for (cnt = 0; cnt < 20; cnt++) {
        if (buff[cnt] == 0x4d && buff[cnt + 1] == 0x4d) {
            big_endian = 1;
            tiff_header_offset = cnt;
            cnt += 2;
            break;
        } else if (buff[cnt] == 0x49 && buff[cnt + 1] == 0x49) {
            big_endian = 0;
            tiff_header_offset = cnt;
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
    if (buff[cnt++] == 0x00 && buff[cnt++] == 0x2A) {
        puts("This is TIFF file.");
    } else {
        puts("This is BigTIFF file.");
        puts("Learn first about it.");
        exit(-1);
    }


    /*Getting 0th IFD's offset*/
    int _0th_IFD_offset = binary_to_sequence(buff, big_endian, cnt, 4);
    printf("0th IFD offset : %d Byte\n", _0th_IFD_offset);


    /*Getting IFD offset*/
    int ifd_offset = tiff_header_offset + _0th_IFD_offset;
    printf("IFD offset : 0x%x\n", ifd_offset);


    /*Getting number of entry*/
    int number_of_entry = binary_to_sequence(buff, big_endian, ifd_offset, 2);
    printf("number_of_entry : 0x%x\n", number_of_entry);
    cnt = ifd_offset + 2;

    uint8_t gps_IFD_offset = 0;

    // todo : make function
    do {
        if (buff[cnt] == 0x88 && buff[cnt + 1] == 0x25) {
            puts("got it !");
            gps_IFD_offset = cnt;
            break;
        }
        cnt += 12;
        number_of_entry--;
    } while (number_of_entry != 0);

    uint32_t gps_info_IFD_Pointer = tiff_header_offset + binary_to_sequence(buff, big_endian, gps_IFD_offset + 8, 4);

    printf("gps_pointer : 0x%x\n", gps_info_IFD_Pointer);
    cnt = gps_info_IFD_Pointer;

    while (1) {
        if (buff[cnt + 1] > 0x1f) {
            puts("GPS longitude/latitude doesn't exist.");
            break;
        } else if (buff[cnt] == 0x00 && buff[cnt + 1] == 0x01) {
            //GPS Latitude Ref
            puts("got gps latitude ref");
            break;
        } else if (buff[cnt] == 0x00 && buff[cnt + 1] == 0x03) {
            //GPS Longitude Ref
            puts("got gps longitude ref");
            break;
        } else {
            //Other info
            uint16_t ifd_type = get_IFD_type(binary_to_sequence(buff, big_endian, cnt + 2, 2));
            uint16_t ifd_count = binary_to_sequence(buff, big_endian, cnt + 4, 2);
            cnt += (ifd_type * ifd_count);
        }
        //get_IFD_type()
    }

    //printf("gps_offset : 0x%x\n", gps_offset);

    //printf("https://www.google.com/maps?q=%d,%d\n", ,);

    fclose(fp);
    return 0;
}