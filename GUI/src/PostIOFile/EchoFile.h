#ifndef _ECHOFILE_H_
#define _ECHOFILE_H_

#include <map>
#include <string>
namespace MIOFile
{
    class EchoFile
    {
    private:
        /* data */
    public:
        EchoFile(const char* path);
        ~EchoFile();
       

        enum FILE_ERROR_CODE
        {
	        FILE_E0001 = 1,
            FILE_E0002,
            FILE_E0003,
            FILE_E0004,
            FILE_E0005,
            FILE_E0006,
            FILE_E0007,
            FILE_E0008,
            FILE_E0009,
            FILE_E0010,
            FILE_E0011,
            FILE_E0012,
        }fe_code_;

        std::map<FILE_ERROR_CODE, std::string> fe_class_
        {
            {FILE_E0001, "数据缺失"},
            {FILE_E0002, "数据冗余"},
            {FILE_E0003, "数据类型错误"},
            {FILE_E0004, "关键字格式不符合规范"},
            {FILE_E0005, "数值异常"},
            {FILE_E0006, "未识别的关键字"},
            {FILE_E0007, "名称冲突"},
            {FILE_E0008, "空指针保护"},
            {FILE_E0009, "数组溢出"},
            {FILE_E0010, "文件错误"},
            {FILE_E0011, "计算错误"},
            {FILE_E0012, "模型限制"},
        };

        FILE* fp_;
        void printEchoInfo(::MIOFile::EchoFile::FILE_ERROR_CODE fe_code, int line_no, const char* buffer, int option = 0,
            const char* buffer1 = "0", const char* buffer2 = nullptr);

        void closeEchoFile();
    };

}


#endif