#include <iostream>
#include <fstream>

using namespace std;

struct ntfs_boot_sector
{
    uint8_t mft_start[8];
};

int main()
{

    ntfs_boot_sector ntfs_sector;

    fstream file;
    file.open("ntfs.dd", ios::binary);

    file.seekg(ios::beg);
    file.read((char *)&ntfs_sector, sizeof(ntfs_sector));

    return 0;
}