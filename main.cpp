#include <iostream>
#include <fstream>
#include <cmath>

using namespace std;

template <typename T>
T readVariableLength(std::fstream &stream, int length)
{
    T value = 0;
    stream.read(reinterpret_cast<char *>(&value), length);
    return value;
}

typedef struct ntfs_boot_sector
{

    uint8_t jump_code[3];
    uint8_t oem_id[8];

    // BPB
    uint8_t byte_per_sector[2]; // 2byte
    uint8_t sector_per_cluster;
    uint8_t rev[2];
    uint8_t unused[5];
    uint8_t media;
    uint8_t unused_2[18];
    uint8_t total_sectors[8];

    uint64_t mft_start_cluster;
    uint64_t mft_start_mirror;

    uint32_t cluster_per_entry;

    uint8_t unused_3[3];

    uint8_t cluster_idx;

    uint8_t unused_4[3];

    uint64_t vol_serial_num;

    uint8_t unused_5[4];

} BootSector;

typedef struct mft_entry_header
{

    uint32_t signature;
    uint16_t fixarr_offset;
    uint16_t fixarr_entries;

    uint64_t lsn;
    uint16_t seqnum;
    uint16_t hard_link_count;

    uint16_t file_attr_offset;
    uint16_t flags;

    uint32_t real_entry_size;
    uint32_t alloc_entry_size;

    uint64_t base_entry_addr;
    uint16_t next_attr_id;

} MFTHeader;

typedef struct common_header
{
    uint32_t attr_type_id;
    uint32_t attr_length;
    uint8_t non_resident_flag;
    uint8_t name_length;
    uint16_t name_offset;
    uint16_t flags;
    uint16_t attr_id;
} COMMON_HEADER;

typedef struct non_resident_header
{
    uint64_t start_runlist_vcn;
    uint64_t end_runlist_vcn;
    uint16_t runlist_offset;
} NON_RESIDENT_HEADER;

int main()
{

    fstream source;

    source.open("ntfs.dd", ios::binary | ios::in | ios::out);
    if (!source)
    {
        puts("파일 열기 오류");
        return 1;
    }

    BootSector boot;

    // 파일 시작
    source.seekg(ios::beg);

    source.read((char *)&boot, sizeof(BootSector));

    printf("jump code: ");
    for (int i = 0; i < 3; i++)
        printf("%02X ", boot.jump_code[i]);
    putchar('\n');

    printf("oem id:: ");
    for (int i = 0; i < 8; i++)
        printf("%02X ", boot.oem_id[i]);
    putchar('\n');

    printf("byte per sector: ");
    for (int i = 0; i < 2; i++)
        printf("%02X ", boot.byte_per_sector[i]);
    putchar('\n');

    printf("sector per cluster: %02X\n", boot.sector_per_cluster);

    printf("MFT start cluster: %llX\n", boot.mft_start_cluster);

    int32_t tmp = boot.cluster_per_entry;
    tmp = ~tmp + 1 + 256;
    printf("clusters per MFT entry: %d\n", int(std::pow(2, tmp)));

    // MFT #0 주소 계산
    int64_t mft_start_offset = boot.mft_start_cluster * boot.sector_per_cluster * 0x200;
    printf("MFT start offset: %llX * %02x * 0x200 = %llX\n", boot.mft_start_cluster, boot.sector_per_cluster, mft_start_offset);
    printf("%llX\n", mft_start_offset);

    // MFT #0으로 이동
    source.seekg(mft_start_offset);

    MFTHeader mftheader;
    source.read((char *)&mftheader, sizeof(MFTHeader));

    printf("MFT header Signature: %X\n", mftheader.signature);
    printf("MFT fixupArray offset: %X\n", mftheader.fixarr_offset);
    printf("MFT fixupArray Entries: %X\n", mftheader.fixarr_entries);
    printf("file attribute offset: %X\n", mftheader.file_attr_offset);

    // fixupArray 오프셋으로 이동
    source.seekg(mft_start_offset + mftheader.fixarr_offset);

    // fixupArray와 각 섹터 마지막 2바이트 비교해서 다르면 에러 발생시키기.
    uint16_t *fixupArr = new uint16_t[mftheader.fixarr_entries];
    source.read((char *)fixupArr, sizeof(uint16_t) * 3);

    for (int i = 0; i < mftheader.fixarr_entries; i++)
        printf("%04X ", fixupArr[i]);
    putchar('\n');

    uint16_t fix_tmp;
    for (int i = 0; i < 2; i++)
    {
        // sector 끝의 2바이트 읽어오기
        source.seekg(mft_start_offset + (0x200 * (i + 1)) - 2);
        source.read((char *)&fix_tmp, sizeof(uint16_t));
        printf("sector %d sign: %04X\n", i + 1, fix_tmp);

        if (fixupArr[0] == fix_tmp)
        {
            // 해당 섹터 끝으로 가서 원래의 값 저장
            source.seekg(mft_start_offset + (0x200 * (i + 1)) - 2);
            source.write((char *)&fixupArr[i + 1], sizeof(uint16_t));

            // fixup Array에는 다시 서명 저장
            source.seekg(mft_start_offset + mftheader.fixarr_offset + 2 * (i + 1));
            source.write((char *)&fixupArr[0], sizeof(uint16_t));
        }
        else
        {
            puts("fixup Array 불일치. 파일 손상됨.");
            return 1;
        }
    }

    uint16_t *fixupArrPost = new uint16_t[mftheader.fixarr_entries];
    source.seekg(mft_start_offset + mftheader.fixarr_offset);
    source.read((char *)fixupArr, sizeof(uint16_t) * 3);
    for (int i = 0; i < mftheader.fixarr_entries; i++)
        printf("%04X ", fixupArr[i]);
    putchar('\n');

    // file attribute offset으로 가서 $DATA까지 간 다음에 start VCN runlist, end VCN runlist 이용해서 start와 size(클러스터 수) 출력하면 끝.

    // file attribute로 이동
    uint64_t attr_offset = mft_start_offset + mftheader.file_attr_offset;
    source.seekg(attr_offset);
    COMMON_HEADER common;
    source.read((char *)&common, sizeof(COMMON_HEADER));
    // $DATA 영역까지 찾기
    puts("seek for $DATA area...");
    while (common.attr_type_id != 0x80)
    {
        printf("attribute id: %X\n", common.attr_type_id);
        printf("attribute length: %X\n", common.attr_length);
        attr_offset += common.attr_length;
        source.seekg(attr_offset);
        source.read((char *)&common, sizeof(COMMON_HEADER));
    }

    printf("attribute id: %X\n", common.attr_type_id);
    printf("attribute length: %X\n", common.attr_length);

    NON_RESIDENT_HEADER non_resident;
    source.read((char *)&non_resident, sizeof(NON_RESIDENT_HEADER));
    printf("start VCN of runlist: %llX\n", non_resident.start_runlist_vcn);
    printf("end VCN of runlist: %llX\n", non_resident.end_runlist_vcn);

    uint64_t runlist_start = attr_offset + non_resident.runlist_offset;
    printf("runlist offset: %02X\n", non_resident.runlist_offset);

    // runlist 시작 주소로 이동
    source.seekg(runlist_start);
    uint8_t info;
    uint32_t run_leng;
    uint32_t run_offset;
    while (true)
    {
        source.read((char *)&info, sizeof(uint8_t));
        if (info == 0x00)
            break;
        printf("%02X\n", info);
        int run_leng_byte = info & 0x0F;
        int run_offset_byte = (info >> 4) & 0x0F;
        printf("%d %d\n", run_leng_byte, run_offset_byte);

        // source.read((char*)&run_leng, run_leng_byte);
        // source.read((char*)&run_offset, run_offset_byte);
        run_leng = readVariableLength<uint32_t>(source, run_leng_byte);
        run_offset = readVariableLength<uint32_t>(source, run_offset_byte);
        printf("cluster start: %d\n", run_offset);
        printf("size: %d\n", run_leng);
    }

    // printf("cluster start: %llX\n", runlist_start);
    // printf("size: %lld\n", non_resident.end_runlist_vcn - non_resident.start_runlist_vcn + 1);

    delete[] fixupArr;
    delete[] fixupArrPost;
    source.close();

    return 0;
}