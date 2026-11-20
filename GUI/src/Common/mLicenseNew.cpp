#include "mLicenseNew.h"
#include<cstdlib>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <QDir>
using namespace std;
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable :4995)
#pragma warning(disable:4996)

vector<uint8_t> base64_decode(const string& base64_str) {
    BIO* bio_mem = nullptr, * bio_b64 = nullptr;
    vector<uint8_t> decoded_data;
    try {
        // 初始化Base64解码BIO链
        bio_b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(bio_b64, BIO_FLAGS_BASE64_NO_NL);
        bio_mem = BIO_new_mem_buf(base64_str.c_str(), static_cast<int>(base64_str.size()));
        bio_mem = BIO_push(bio_b64, bio_mem);

        // 计算解码后最大可能长度，分配缓冲区
        size_t max_len = base64_str.size() * 3 / 4 + 1;
        decoded_data.resize(max_len);
        int actual_len = BIO_read(bio_mem, decoded_data.data(), static_cast<int>(max_len));
        if (actual_len < 0) {
            throw runtime_error("Base64解码失败：无效的Base64字符串");
        }
        decoded_data.resize(static_cast<size_t>(actual_len)); // 裁剪为实际长度
    }
    catch (...) {
        // 释放资源，避免内存泄漏
        if (bio_mem) BIO_free_all(bio_mem);
        if (bio_b64) BIO_free(bio_b64);
        throw;
    }
    if (bio_mem) BIO_free_all(bio_mem);
    return decoded_data;
}

// 拼接并解码 AES 密钥
vector<uint8_t> get_aes_key() {
    vector<uint8_t> key = base64_decode(AES_KEY_BASE64);
    return key;
}

//python加密的格式，参考此格式进行解密
vector<uint8_t> pkcs7_unpad(const vector<uint8_t>& padded_data, size_t block_size = 16) {
    if (padded_data.empty()) {
        throw runtime_error("error");
    }
    uint8_t pad_len = padded_data.back(); // 最后1字节=需要移除的填充字节数
    // 合法性校验：避免密文被篡改或填充规则不匹配
    if (pad_len == 0 || pad_len > block_size || pad_len > padded_data.size()) {
        throw runtime_error("error");
    }
    for (size_t i = padded_data.size() - pad_len; i < padded_data.size(); ++i) {
        if (padded_data[i] != pad_len) {
            throw runtime_error("error");
        }
    }
    // 移除填充字节，返回原始明文字节
    return vector<uint8_t>(padded_data.begin(), padded_data.end() - pad_len);
}

// 根据 key 长度选择 AES-128/192/256 CBC ----------
static const EVP_CIPHER* cipher_from_keylen(size_t keylen) {
    if (keylen == 16) return EVP_aes_128_cbc();
    if (keylen == 24) return EVP_aes_192_cbc();
    if (keylen == 32) return EVP_aes_256_cbc();
    throw runtime_error("Unsupported AES key length (must be 16/24/32 bytes).");
}

// 读取二进制密文文件
vector<uint8_t> read_ciphertext_file(const string& file_path) {
    ifstream file(file_path, ios::in | ios::binary | ios::ate);
    if (!file.is_open()) {
        throw runtime_error("无法打开密文文件：" + file_path);
    }
    // 获取文件大小（密文长度），分配缓冲区
    streampos file_size = file.tellg();
    vector<uint8_t> ciphertext(static_cast<size_t>(file_size));
    // 回到文件开头，读取所有二进制数据
    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char*>(ciphertext.data()), file_size);
    file.close();
    // 验证密文长度：AES密文必须是16字节的整数倍
    if (ciphertext.size() % AES_BLOCK_SIZE != 0) {
        throw runtime_error("密文文件损坏：长度不是16字节的整数倍");
    }
    return ciphertext;
}


//  读取文件并解密（验证码）
string decrypt_file(const string& file_path, const vector<unsigned char>& aes_key) {
    // 1. 二进制方式打开文件（原逻辑正确，保留）
    ifstream file(file_path, ios::binary);
    if (!file.is_open()) {
        throw runtime_error("无法打开文件：" + file_path);
    }
    //string file = file_path;
    file.seekg(0, ios::end);
    streamsize file_size = file.tellg();
    file.seekg(0, ios::beg);

    if (file_size < 16) { // 提前校验，避免后续拆分越界
        file.close();
        throw runtime_error("文件数据过短，缺少16字节IV（文件大小：" + to_string(file_size) + "字节）");
    }

    vector<unsigned char> bin(file_size);
    if (!file.read(reinterpret_cast<char*>(bin.data()), file_size)) {
        file.close();
        /*throw runtime_error("读取文件二进制数据失败");*/
    }
    file.close();

    // 3. 拆分IV（前16字节）+ 密文（剩余字节）（原逻辑正确，保留）
    vector<unsigned char> iv(bin.begin(), bin.begin() + 16);
    vector<unsigned char> ciphertext(bin.begin() + 16, bin.end());

    // 4. AES-CBC 解密（原逻辑正确，保留）
    return aes_cbc_decrypt(aes_key, iv, ciphertext);
}

string aes_cbc_decrypt(const vector<unsigned char>& key, const vector<unsigned char>& iv, const vector<unsigned char>& ciphertext)
{
    const EVP_CIPHER* cipher = cipher_from_keylen(key.size());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw runtime_error("EVP_CIPHER_CTX_new failed");

    if (1 != EVP_DecryptInit_ex(ctx, cipher, nullptr, key.data(), iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw runtime_error("EVP_DecryptInit_ex failed");
    }

    vector<unsigned char> plaintext(ciphertext.size() + EVP_CIPHER_block_size(cipher));
    int outlen1 = 0, outlen2 = 0;

    if (1 != EVP_DecryptUpdate(ctx,
        plaintext.data(), &outlen1,
        ciphertext.data(), (int)ciphertext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw runtime_error("EVP_DecryptUpdate failed");
    }

    // 这里会校验并移除 PKCS#7 padding，padding 不对会失败
    //if (1 != EVP_DecryptFinal_ex(ctx, plaintext.data() + outlen1, &outlen2)) {
    //    EVP_CIPHER_CTX_free(ctx);
    //    throw runtime_error("EVP_DecryptFinal_ex failed (bad key/iv or corrupted data).");
    //}

    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(outlen1 + outlen2);

    return string(reinterpret_cast<const char*>(plaintext.data()), plaintext.size());
}

string decrypt_file_bin(const string& file_path, const vector<unsigned char>& aes_key) {
    // 1. 二进制打开文件，读取全部纯二进制数据（IV+密文，无任何Base64编码）
    ifstream file(file_path, ios::binary);
    if (!file.is_open()) {
        throw runtime_error("无法打开文件：" + file_path);
    }

    // 获取文件大小，预分配二进制缓冲区
    file.seekg(0, ios::end);
    streamsize file_size = file.tellg();
    file.seekg(0, ios::beg);

    //// 提前校验：至少包含16字节IV，避免拆分越界
    //if (file_size < 16) {
    //    file.close();
    //    throw runtime_error("文件数据过短，缺少16字节IV（文件大小：" + to_string(file_size) + "字节）");
    //}
    vector<unsigned char> bin_data(file_size);
    if (!file.read(reinterpret_cast<char*>(bin_data.data()), file_size)) {
        file.close();
        throw runtime_error("读取文件二进制数据失败：" + file_path);
    }
    file.close();
    const vector<unsigned char> iv = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10 };
    // 2. 拆分IV（前16字节）+ 密文（剩余字节）- 与原有逻辑一致
    //vector<unsigned char> iv(bin_data.begin(), bin_data.begin() + 16);
    vector<unsigned char> ciphertext(bin_data.begin(), bin_data.end());

    // 3. 调用原有AES-CBC解密函数（无修改，复用原有逻辑）
    return aes_cbc_decrypt(aes_key, iv, ciphertext);
}

//OpenSSL 3.0 新版 EVP 接口 - AES-256-CBC 解密 =====================
string aes_256_cbc_decrypt(const vector<uint8_t>& ciphertext) {
    // 生成实际32字节AES-256密钥（Base64解码后，与Python一致）
    vector<uint8_t> key = get_aes_key();
    // 校验IV长度（固定16字节，CBC模式强制要求，与Python一致）
    if (IV_LEN != AES_BLOCK_SIZE) {
        throw invalid_argument("AES-CBC IV length error: required 16 bytes");
    }
    if (ciphertext.empty()) return "";
    if (ciphertext.size() % AES_BLOCK_SIZE != 0) {
        throw runtime_error("Ciphertext length error: must be multiple of 16 bytes");
    }

    // 1. 初始化EVP加密上下文（OpenSSL 3.0 推荐）
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
        throw runtime_error("EVP context create failed: out of memory");
    }

    vector<uint8_t> plaintext;
    try {
        // 2. 初始化解密操作：指定算法(AES-256-CBC)、密钥、IV
        // EVP_aes_256_cbc()：固定AES-256-CBC，与Python一致
        // key.data()：32字节原始密钥（Base64解码后）
        // AES_IV：16字节固定IV，与Python完全一致
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), AES_IV) != 1) {
            throw runtime_error("EVP decrypt init failed: invalid key/IV");
        }

        // 3. 配置解密上下文：关闭自动填充（关键！）
        // 原因：Python加密时手动做了PKCS7填充，需在解密后手动去填充，禁止OpenSSL自动处理
        EVP_CIPHER_CTX_set_padding(ctx, 0);

        // 4. 执行分段解密（EVP接口推荐分段处理，兼容大密文）
        plaintext.resize(ciphertext.size() + AES_BLOCK_SIZE); // 预留最大可能长度
        int update_len = 0, final_len = 0;
        // 解密主数据
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &update_len, ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) {
            throw runtime_error("EVP decrypt update failed: invalid ciphertext");
        }
        // 解密收尾（CBC模式必须调用，处理最后一个分组）
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + update_len, &final_len) != 1) {
            throw runtime_error("EVP decrypt final failed: ciphertext corrupted or wrong key");
        }
        // 裁剪为实际解密长度（带PKCS7填充的明文字节）
        plaintext.resize(static_cast<size_t>(update_len + final_len));

        // 5. 核心步骤：PKCS7去填充（解决末尾缺失3字符问题，与Python填充规则一致）
        plaintext = pkcs7_unpad(plaintext);

    }
    catch (...) {
        // 异常时释放上下文，避免内存泄漏
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }
    // 释放EVP上下文
    EVP_CIPHER_CTX_free(ctx);

    // 转换为UTF-8字符串（匹配Python的json.dumps(data).encode()）
    return string(plaintext.begin(), plaintext.end());
};

// 去除字符串首尾的双引号
string trim_quotation(const string& str) {
    if (!str.empty() && str.front() == '"' && str.back() == '"') {
        // 从第1位开始，截取长度为 总长度-2 的内容（去掉首尾引号）
        return str.substr(1, str.size() - 2);
    }
    // 不满足条件则直接返回原字符串
    return str;
};

//结构化输出格式
map<string, string> parse_license_to_kv(const string& decrypted_message) {
    // 1. 定义固定字段名列表（与Python格式化的14个字段严格一一对应，顺序不可变）
    const vector<string> field_names = {
        "LicenseType",
        "MacAddress",
        "ProjectName",
        "LicenseVersion",
        "StartTime",
        "EndTime",
        "MaintenancePeriod",
        "VerSion",
        "SupportFunction",
        "SupportCore",
        "SupportSubprocess",
        "SupportEquation",
        "LicenseNumber",
        "IsSuper"
    };

    // 2. 初始化键值对map、字符串流、临时token
    map<string, string> kv_map;
    stringstream ss(decrypted_message);
    string token;
    int index = 0;

    // 3. 按冒号分割字符串，逐字段解析
    while (getline(ss, token, ':') && index < field_names.size()) {
        kv_map[field_names[index]] = token; // 键=字段名，值=解析出的token
        index++;
    }

    return kv_map;
};;

/**
 * @brief 字符串转小写
 * @param str 输入字符串
 * @return 小写后的字符串
 */
inline std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](int c) { return std::tolower(c); }); // C++11 lambda，兼容C++11
    return str;
}

/**
 * @brief 字符串替换
 * @param str 原字符串
 * @param old_val 要替换的子串
 * @param new_val 新子串
 * @return 替换后的字符串
 */
inline std::string str_replace(std::string str, const std::string& old_val, const std::string& new_val) {
    size_t pos = 0;
    while ((pos = str.find(old_val, pos)) != std::string::npos) {
        str.replace(pos, old_val.length(), new_val);
        pos += new_val.length();
    }
    return str;
}


inline std::string str_strip(std::string str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

inline std::vector<std::string> str_split(const std::string& str, char delim) {
    std::vector<std::string> res;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim)) {
        res.push_back(token);
    }
    return res;
}

inline void vec_remove(std::vector<std::string>& vec, const std::string& val) {
    auto it = std::find(vec.begin(), vec.end(), val);
    if (it != vec.end()) {
        vec.erase(it);
    }
}

bool mac_address_check(const std::string& license_mac_str, const std::vector<std::string>& server_mac_list) {
    std::vector<std::string> license_mac_split;
    // 按,/:/;分割,判断分隔符
    if (license_mac_str.find(',') != std::string::npos) {
        license_mac_split = str_split(license_mac_str, ',');
    }
    else if (license_mac_str.find(':') != std::string::npos) {
        license_mac_split = str_split(license_mac_str, ':');
    }
    else if (license_mac_str.find(';') != std::string::npos) {
        license_mac_split = str_split(license_mac_str, ';');
    }
    else {
        // 无分隔符：视为单个MAC，直接加入list
        license_mac_split.push_back(license_mac_str);
    }

    // 步骤2：格式化许可证MAC list → 统一格式
    std::vector<std::string> license_mac_list;
    for (const std::string& mac : license_mac_split) {
        std::string clean_mac = str_strip(mac);
        if (clean_mac.empty()) continue;
        clean_mac = to_lower(clean_mac);
        clean_mac = str_replace(clean_mac, "-", ""); // 去除可能的-分隔符
        license_mac_list.push_back(clean_mac);
    }

    if (license_mac_list.empty()) {
        throw std::runtime_error("许可证MAC地址格式化后为空，请检查配置");
    }

    // 步骤3：格式化本机MAC list → 统一格式兼容可能的大小写/空白
    std::vector<std::string> local_mac_list;
    for (const std::string& mac : server_mac_list) {
        std::string clean_mac = str_strip(mac);
        if (clean_mac.empty()) continue;
        clean_mac = to_lower(clean_mac);
        local_mac_list.push_back(clean_mac);
    }

    if (local_mac_list.empty()) {
        throw std::runtime_error("本机MAC地址列表为空，请检查获取逻辑");
    }

    // 两个list求交集
    std::set<std::string> license_mac_set(license_mac_list.begin(), license_mac_list.end());
    std::set<std::string> local_mac_set(local_mac_list.begin(), local_mac_list.end());
    std::set<std::string> common_macs;

    // 求两个set的交集（结果存入common_macs）
    std::set_intersection(
        license_mac_set.begin(), license_mac_set.end(),
        local_mac_set.begin(), local_mac_set.end(),
        std::inserter(common_macs, common_macs.begin())
    );

    // 判断交集是否为空
    if (!common_macs.empty()) {
        std::cout << "MAC地址验证通过，匹配项：";
        for (const std::string& mac : common_macs) {
            std::cout << mac << " ";
        }
        std::cout << std::endl;
        return true;
    }
    else {
        // 拼接错误信息（列出两个list的内容，方便排查）
        std::stringstream err_ss;
        err_ss << "MAC地址无匹配项！\n许可证MAC列表：[";
        for (size_t i = 0; i < license_mac_list.size(); ++i) {
            if (i > 0) err_ss << ", ";
            err_ss << license_mac_list[i];
        }
        err_ss << "]\n本机MAC列表：[";
        for (size_t i = 0; i < local_mac_list.size(); ++i) {
            if (i > 0) err_ss << ", ";
            err_ss << local_mac_list[i];
        }
        err_ss << "]";
        std::cout << err_ss.str() << std::endl;
        return false;
    }
}


/**
 * @brief 字符串日期转std::tm（适配"YYYY-MM-DD"格式，核心工具函数）
 * @param date_str 日期字符串，如"2025-12-03"
 * @return std::tm 转换后的日期结构体（仅日期，时间为00:00:00）
 * @throw std::runtime_error 格式错误/日期无效时抛出异常
 */
tm string_to_tm(const std::string& date_str) {
    std::tm tm_date = { 0 }; // 初始化所有字段为0，避免随机值
    int year, month, day;
    // 按"YYYY-MM-DD"格式解析，提取年、月、日
    if (sscanf(date_str.c_str(), "%d-%d-%d", &year, &month, &day) != 3) {
        throw std::runtime_error("日期格式错误，需为YYYY-MM-DD：" + date_str);
    }
    // 校验日期合法性（基础校验，避免明显无效值）
    if (year < 2000 || month < 1 || month > 12 || day < 1 || day > 31) {
        throw std::runtime_error("日期值无效：" + date_str);
    }
    // std::tm的年份是「从1900年开始的偏移量」，月份是0-11（需减1）
    tm_date.tm_year = year - 1900;
    tm_date.tm_mon = month - 1;
    tm_date.tm_mday = day;
    tm_date.tm_hour = 0;
    tm_date.tm_min = 0;
    tm_date.tm_sec = 0;
    return tm_date;
}

/**
 * @param start_time_str 开始时间字符串（YYYY-MM-DD），如"2025-12-03"
 * @param end_time_str 结束时间字符串（YYYY-MM-DD），如"2026-12-03"
 * @param license_time 输出参数：剩余天数（赋值给你的key.license_time）
 * @return bool 时间在有效期内返回true，否则false
 * @throw std::runtime_error 日期格式错误时抛出异常
 */
bool check_license_time(const std::string& start_time_str,const std::string& end_time_str) {
    try {
        // 1. 字符串转std::tm日期结构体
        std::tm start_tm = string_to_tm(start_time_str);
        std::tm end_tm = string_to_tm(end_time_str);
        // 2. 获取当前系统时间
        time_t now_time_t = time(nullptr);
        // 转换为本地时间
        std::tm now_tm = *localtime(&now_time_t);

        // 3. 转换为time_t（秒数），方便时间比较和差值计算
        time_t start_time_t = mktime(&start_tm);
        time_t end_time_t = mktime(&end_tm);
        time_t now_time_t_date = mktime(&now_tm);

        // 4. 时间范围判断（复刻Python: if start_time < now_time < end_time）
        if (now_time_t_date > start_time_t && now_time_t_date < end_time_t) {
            // 5. 计算剩余时间（秒数差值）
            double time_diff_sec = difftime(end_time_t, now_time_t_date);
            // 转换为天、时、分、秒（与Python原逻辑完全一致）
            long long days = static_cast<long long>(time_diff_sec / (24 * 3600));
            long long remaining_sec = static_cast<long long>(time_diff_sec) % (24 * 3600);
            //int hours = static_cast<int>(remaining_sec / 3600);
            //int minutes = static_cast<int>((remaining_sec % 3600) / 60);
            //int seconds = static_cast<int>(remaining_sec % 60);

            // 6. 构建剩余时间格式化字符串（复刻Python的拼接逻辑）
            std::string remaining_time;
            if (days > 0) {
                remaining_time += std::to_string(days) + " 天 ";
            }
            //if (hours > 0 || days > 0) {
            //    remaining_time += std::to_string(hours) + " 小时 ";
            //}
            //if (minutes > 0 || hours > 0 || days > 0) {
            //    remaining_time += std::to_string(minutes) + " 分 ";
            //}
            //remaining_time += std::to_string(seconds) + " 秒";

            // 8. 打印剩余时间（与Python原输出一致）
            std::cout << "许可剩余时间: " << remaining_time << std::endl;
            return true;
        }
        else {
            // 未在有效期内：判断未开始/已过期
            if (now_time_t_date < start_time_t) {
                std::cout << "未到许可开始时间" << std::endl;
            }
            else {
                std::cout << "许可证文件已过期，请重新申请！" << std::endl;
            }
            return false;
        }
    }
    catch (const std::runtime_error& e) {
        throw std::runtime_error("许可证时间校验失败：" + std::string(e.what()));
    }
}


template <typename Container, typename T>
inline bool contains(const Container& c, const T& value) {
    using std::begin;
    using std::end;
    return std::find(begin(c), end(c), value) != end(c);
}
LicenseNew::LicenseNew(QString workdir) :mLicense(workdir)
{
#if defined(_WIN32)
    char exePath[512];
    GetModuleFileNameA(nullptr, exePath, 512);
    (strrchr(exePath, '\\'))[0] = 0;

    licenseFile = string(exePath) + "\\..\\License\\license.lic";			//激活码文件位置；
    configureFile = string(exePath) + "\\..\\License\\MxConfigure.conf";	//生成的本机信息文件的位置

    //创建文件夹
    string  licFolderPath = string(exePath) + "\\..\\License"; //License文件夹位置
    if (access(licFolderPath.c_str(), 0) != 0) {
        mkdir(licFolderPath.c_str());
    }
#else
    auto wk = QDir(workdir);
    auto licFolderPath = QDir(wk.filePath("../License"));
    licenseFile = licFolderPath.filePath("MxLicense.lic").toStdString();
    configureFile = licFolderPath.filePath("MxConfigure.conf").toStdString();

    if (!licFolderPath.exists()) {
        licFolderPath.mkpath(licFolderPath.path());
    }
    qputenv("MXLICENSE_DIR", licFolderPath.absolutePath().toLocal8Bit());

#endif
}
LicenseNew::~LicenseNew()
{
}

bool LicenseNew::check_localfile()
{
    fstream _localfile;
    _localfile.open(licenseFile, ios::in);
    if (!_localfile)
    {
        return false;
    }
    return true;
}

int LicenseNew::Licensecheck() {
        //写本机信息文件
        //if (!writeConfigureInfor())
        //{
        //    //cout<<"写入配置文件失败！";
        //    return 1;
        //}

        //检查本地激活码文件是否存在
        if (!check_localfile())
        {
            //cout << "本机无激活码文件，请联系软件管理员！";
            //return 2;
            return 2;
        }

        //检查激活码的合法性以及有效期;
        int sign = newLicenseCheck();
        if (sign == 3)
        {
            //cout << "激活码文件与系统配置文件不匹配！";
            return 3;
        }
        if (sign == 4)
        {
            //cout << "激活码已过期！";
            return 4;
        }
        if (sign == 5)
        {
            return 5;
        }
        else
        {
            //cout << "激活码有效！";
            return 0;
        }
};

int LicenseNew::newLicenseCheck() {
        //文件解密
        vector<string> mac_address;
        vector<uint8_t> ciphertext = read_ciphertext_file(licenseFile);
        string plaintext = aes_256_cbc_decrypt(ciphertext);
        string a = trim_quotation(plaintext);
        map<string, string> license_map = parse_license_to_kv(a);
        string isSuper = license_map["IsSuper"];         
        string server_mac = license_map["MacAddress"];
        string start_time = license_map["StartTime"];
        string end_time = license_map["EndTime"];
        string Product = license_map["ProjectName"];
        string LicenseType = license_map["LicenseType"];
        trim_quotation(server_mac);
        trim_quotation(Product);
        if (LicenseType != "Single") {
            return 5;
        }
        vector<string> product_list = str_split(Product, ',');
        bool product_in = contains(product_list, "MxSim.Mechanical.GUI");
        bool prodcut_in1 = contains(product_list, "MxSim.Mechanical");
        if (product_in !=true && prodcut_in1 != true) {
            cout << "当前产品不可使用该许可\n";
            cout << "当前许可支持:" << Product << "\n" << endl;
            return 3;
        }
        if (isSuper == "True") {
        }
        else {
            getMacByCmd(mac_address);

            if (mac_address_check(server_mac, mac_address)) {
            }
            else{
                return 3;
            };
        };
        string_to_tm(start_time);
        string_to_tm(end_time);
        bool license_time_check = check_license_time(start_time, end_time);
        if (license_time_check != 1) {
            return 4;
        };
        return 1;
};