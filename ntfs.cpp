#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdint>

using namespace std;

void *little_endian_address(void *big_endian_address, size_t address_size);
struct ntfs_boot_sector
{

    uint8_t bytes_per_sector[2];
    uint8_t sector_per_cluster[1];
    uint8_t reserved_sectors[2];
    uint8_t reserved1[5];
    uint8_t reserved2[18];
    uint8_t total_sectors[8];
    uint8_t start[8];
    uint8_t mirr[8];
};

int main()
{

    ntfs_boot_sector ntfs_sector;

    fstream file;
    file.open("./ntfs.dd", ios::binary | ios::in | ios::out);

    if (!file)
    {
        cout << "파일을 열지 못했습니다." << endl;
        return -1;
    }

    // 원하는 위치로 이동
    int offset = 11;
    file.seekg(offset, ios::beg);

    // 데이터 읽어오기
    /*
        1. 읽어온 데이터를 ntfs_sector라는 변수에 저장한다.

    */
    file.read(reinterpret_cast<char *>(&ntfs_sector), sizeof(ntfs_sector));

    printf("bytes per sector ");
    for (int i = 0; i < 2; i++)
    {
        printf("%02X ", ntfs_sector.bytes_per_sector[i]);
    }
    printf("\n");

    printf("sector per cluster ");
    for (int i = 0; i < 1; i++)
    {
        printf("%02X ", ntfs_sector.sector_per_cluster[i]);
    }
    printf("\n");

    printf("ntfs_sector start ");
    // little_endian_address(ntfs_sector.start, sizeof(ntfs_sector.start));
    for (int i = 0; i < 8; i++)
    {
        printf("%02X ", ntfs_sector.start[i]);
    }
    printf("\n");

    // mtf 첫번째 주소 계산
    uint64_t ntfs_address;
    ntfs_address = ntfs_sector.bytes_per_sector[0] + (ntfs_sector.bytes_per_sector[1] << 8);
    ntfs_address *= ntfs_sector.sector_per_cluster[0];
    ntfs_address *= *reinterpret_cast<uint64_t *>(ntfs_sector.start);
    printf("ntfs_address %llx\n", ntfs_address);

    return 0;
}

void *little_endian_address(void *big_endian_address, size_t address_size)
{
    uint8_t *big_endian_bytes = static_cast<uint8_t *>(big_endian_address);

    // 바이트 순서를 뒤집은 새로운 바이트 배열을 만듭니다.
    uint8_t little_endian_bytes[address_size];
    for (size_t i = 0; i < address_size; ++i)
    {
        little_endian_bytes[i] = big_endian_bytes[address_size - i - 1];
    }

    // 새로운 바이트 배열을 다시 주소값으로 변환해서 반환합니다.
    void *little_endian_address;
    std::memcpy(&little_endian_address, little_endian_bytes, address_size);
    return little_endian_address;
}
