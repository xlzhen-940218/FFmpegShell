#include "encrypt.h"

// TEA加密核心函数（一次处理64位）
static void tea_encrypt(uint32_t v[2], const uint32_t k[4]) {
    uint32_t v0 = v[0], v1 = v[1];
    uint32_t sum = 0;
    const uint32_t delta = 0x9E3779B9;

    for (int i = 0; i < 32; i++) {
        sum += delta;
        v0 += ((v1 << 4) + k[0]) ^ (v1 + sum) ^ ((v1 >> 5) + k[1]);
        v1 += ((v0 << 4) + k[2]) ^ (v0 + sum) ^ ((v0 >> 5) + k[3]);
    }

    v[0] = v0;
    v[1] = v1;
}

// 加密函数：data为输入输出缓冲区，len为数据长度，key为16字节密钥，nonce为8字节随机数
void tea_ctr_encrypt(uint8_t* data, size_t len, const uint8_t key[16], const uint8_t nonce[8]) {
    uint32_t tea_key[4];
    memcpy(tea_key, key, 16);  // 拷贝密钥

    // 初始化计数器 (nonce + 32位块计数器)
    uint64_t counter_block;
    memcpy(&counter_block, nonce, 8);  // 前8字节用随机数
    uint32_t block_counter = 0;         // 后4字节计数器从0开始

    uint8_t key_stream[8];  // 密钥流缓冲区
    size_t pos = 0;

    while (len > 0) {
        // 构造计数器块 (nonce + block_counter)
        uint32_t ctr_block[2] = {
            (uint32_t)(counter_block),       // 低32位
            (uint32_t)(counter_block >> 32)  // 高32位
        };
        ctr_block[0] ^= block_counter++;  // 合并计数器

        // 加密计数器生成密钥流
        tea_encrypt(ctr_block, tea_key);
        memcpy(key_stream, ctr_block, 8);

        // 处理当前块 (最多8字节)
        size_t bytes_this_block = (len < 8) ? len : 8;
        for (size_t i = 0; i < bytes_this_block; i++) {
            data[pos + i] ^= key_stream[i];  // 异或加密
        }

        // 更新位置和剩余长度
        pos += bytes_this_block;
        len -= bytes_this_block;
    }
}
