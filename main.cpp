#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <iostream>

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
        default:
            printf("Out of type!!\n");
            return 1;
    }
}

int main(int argc, char *argv[]) {
    using namespace std;
    FILE *fp;

    puts("----------debug info ----------");

    if (argc != 2) {
        cerr << "Invalid arguments!!";
        exit(1);
    }
    //char *filename = "C:\\Users\\aomid\\OneDrive\\Desktop\\DSC_0002.JPG";
    char *filename = argv[1];
    uint8_t buff[8192];

    if ((fp = fopen(filename, "rb")) == NULL) {
        printf("%s couldn't be opened!!\n", filename);
        exit(1);
    }

    uint16_t cnt;
    for (cnt = 0; cnt < 8192; cnt++) {
        buff[cnt] = fgetc(fp);
    }


    /*Check whether jpeg or not.*/
    if (buff[6] != 'E' || buff[7] != 'x' || buff[8] != 'i' || buff[9] != 'f') {
        printf("%s is not jpeg!!\n", filename);
        exit(1);
    }


    /*Verify endianness*/
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
        puts("This program cannot handle little endian file at this moment!!");
        exit(1);
    }


    /*Verify the type of TIFF*/
    if (buff[cnt++] == 0x00 && buff[cnt++] == 0x2A) {
        puts("This is TIFF file.");
    } else {
        puts("This is BigTIFF file.");
        puts("Learn first about it!!");
        exit(1);
    }


    /*Get the 0th IFD's offset*/
    int _0th_ifd_offset = binary_to_sequence(buff, big_endian, cnt, 4);
    printf("0th IFD offset : 0x%x\n", _0th_ifd_offset);


    /*Get IFD offset*/
    int ifd_offset = tiff_header_offset + _0th_ifd_offset;
    printf("IFD offset : 0x%x\n", ifd_offset);


    /*Get the number of entry*/
    int number_of_0th_ifd_entry = binary_to_sequence(buff, big_endian, ifd_offset, 2);
    printf("number of 0th ifd entry : 0x%x\n", number_of_0th_ifd_entry);
    cnt = ifd_offset + 2;

    uint8_t gps_ifd_offset = 0;

    // todo : make function
    do {
        if (buff[cnt] == 0x88 && buff[cnt + 1] == 0x25) {
            gps_ifd_offset = cnt;
            break;
        }
        cnt += 12;
        number_of_0th_ifd_entry--;
    } while (number_of_0th_ifd_entry != 0);

    uint32_t gps_info_ifd_pointer = tiff_header_offset + binary_to_sequence(buff, big_endian, gps_ifd_offset + 8, 4);

    printf("gps_pointer : 0x%x\n", gps_info_ifd_pointer);
    cnt = gps_info_ifd_pointer;

    uint16_t number_of_gps_ifd_entry = binary_to_sequence(buff, big_endian, cnt, 2);
    printf("number of gps ifd entry : 0x%X\n", number_of_gps_ifd_entry);
    cnt += 2;

    /*Get the latituderef/longituderef and pointers of latitude/longitude */
    uint32_t gps_latitude_pointer = 0;
    uint32_t gps_longitude_pointer = 0;
    char8_t latitude_ref;
    char8_t longitude_ref;

    printf("\n");

    for (uint16_t cnt1 = 0; cnt1 < number_of_gps_ifd_entry; cnt1++) {
        printf("Tag : [0x%X 0x%X]\n", buff[cnt], buff[cnt + 1]);
        if (buff[cnt + 1] > 0x1f) {
            puts("GPS longitude/latitude doesn't exist!!");
            exit(1);
        } else if (buff[cnt] == 0x00 && buff[cnt + 1] == 0x01) {
            //GPS Latitude Ref
            latitude_ref = (char8_t) buff[cnt + 8];
            printf("GPSLatitudeRef : %c\n", latitude_ref);
        } else if (buff[cnt] == 0x00 && buff[cnt + 1] == 0x03) {
            //GPS Longitude Ref
            longitude_ref = (char8_t) buff[cnt + 8];
            printf("GPSLongitudeRef : %c\n", longitude_ref);
        } else if (buff[cnt] == 0x00 && buff[cnt + 1] == 0x02) {
            //GPS Latitude
            gps_latitude_pointer = tiff_header_offset + binary_to_sequence(buff, big_endian, cnt + 8, 4);
        } else if (buff[cnt] == 0x00 && buff[cnt + 1] == 0x04) {
            //GPS Longitude
            gps_longitude_pointer = tiff_header_offset + binary_to_sequence(buff, big_endian, cnt + 8, 4);
        }

        //info
        uint16_t ifd_type = get_IFD_type(binary_to_sequence(buff, big_endian, cnt + 2, 2));
        uint16_t ifd_count = binary_to_sequence(buff, big_endian, cnt + 4, 4);
        printf("Type : 0x%X, Count : 0x%X\n\n", ifd_type, ifd_count);
        cnt += 12;
    }

    printf("latitude pointer : 0x%x\n", gps_latitude_pointer);
    printf("longitude pointer : 0x%x\n", gps_longitude_pointer);


    /*Get latitude/longitude*/
    uint32_t gps_latitude[3] = {
            binary_to_sequence(buff, big_endian, gps_latitude_pointer, 4) /
            binary_to_sequence(buff, big_endian, gps_latitude_pointer + 4, 4),
            binary_to_sequence(buff, big_endian, gps_latitude_pointer + 8, 4) /
            binary_to_sequence(buff, big_endian, gps_latitude_pointer + 12, 4),
            binary_to_sequence(buff, big_endian, gps_latitude_pointer + 16, 4) /
            binary_to_sequence(buff, big_endian, gps_latitude_pointer + 20, 4)
    };
    uint32_t gps_longitude[3] = {
            binary_to_sequence(buff, big_endian, gps_longitude_pointer, 4) /
            binary_to_sequence(buff, big_endian, gps_longitude_pointer + 4, 4),
            binary_to_sequence(buff, big_endian, gps_longitude_pointer + 8, 4) /
            binary_to_sequence(buff, big_endian, gps_longitude_pointer + 12, 4),
            binary_to_sequence(buff, big_endian, gps_longitude_pointer + 16, 4) /
            binary_to_sequence(buff, big_endian, gps_longitude_pointer + 20, 4)
    };

    uint32_t gps_latitude_dms[3] = {
            binary_to_sequence(buff, big_endian, gps_latitude_pointer, 4),
            binary_to_sequence(buff, big_endian, gps_latitude_pointer + 8, 4),
            binary_to_sequence(buff, big_endian, gps_latitude_pointer + 16, 4)
    };
    uint32_t gps_longitude_dms[3] = {
            binary_to_sequence(buff, big_endian, gps_longitude_pointer, 4),
            binary_to_sequence(buff, big_endian, gps_longitude_pointer + 8, 4),
            binary_to_sequence(buff, big_endian, gps_longitude_pointer + 16, 4)
    };

    puts("-------------------------------");
    printf("\n");


    /*Make URL of Google Map*/
    printf("https://www.google.com/maps?q=%2.6f,%3.6f\n",
           (float) gps_latitude[0] + (float) gps_latitude[1] / 60 + (float) gps_latitude[2] / 3600,
           (float) gps_longitude[0] + (float) gps_longitude[1] / 60 + (float) gps_longitude[2] / 3600);

    printf("https://www.google.com/maps/place/%d%%C2%%B0%d'%d\"%c+%d%%C2%%B0%d'%d\"%c\n", gps_latitude_dms[0],
           gps_latitude_dms[1], gps_latitude_dms[2], latitude_ref,
           gps_longitude_dms[0], gps_longitude_dms[1], gps_longitude_dms[2], longitude_ref);

    printf("\n");


    /*Open browser automatically*/  //todo : discriminate OS and make several move
    char google_map_url[300];
    sprintf(google_map_url, "rundll32.exe url.dll,FileProtocolHandler https://www.google.com/maps?q=%2.6f,%3.6f",
            (float) gps_latitude[0] + (float) gps_latitude[1] / 60 + (float) gps_latitude[2] / 3600,
            (float) gps_longitude[0] + (float) gps_longitude[1] / 60 + (float) gps_longitude[2] / 3600);

    printf("execute command : %s\n", google_map_url);
    system(google_map_url);


    fclose(fp);
    return 0;
}