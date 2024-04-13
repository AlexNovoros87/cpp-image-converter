#include "bmp_image.h"
#include "pack_defines.h"
#include "img_lib.h"

#include <array>
#include <fstream>
#include <string_view>
#include <cassert>
#include <vector>
#include <iostream>
/*
  ĞÀÑØÈĞÅÍÛÉ ÂÀĞÈÀÍÒ BMP.CPP
  ÊÀÊ ÄÎÏÎËÍÅÍÈÅ ÌÍÎÃÎ ÏĞÎÂÅĞÎÊ
  ÎÑÍÎÂÍÎÉ ÂÀĞÈÀÍÒ ÓĞÅÇÀË ÄËß 
  ÓÄÎÁÑÒÂÀ, ÍÎ ÕÎ×Ó ÂÀÌ ÏÎÊÀÇÀÒÜ
  ×ÒÎ ÌÎÆÍÎ È ÒÀÊ!!!
*/


//#define PR 1
//#define PRX 1
using namespace std;

namespace img_lib
{

    PACKED_STRUCT_BEGIN BitmapFileHeader{
        char autograph[2] = {'B', 'M'};
        unsigned int sum_size;
        char reserve_space[4] = {'\0', '\0', '\0', '\0'};
        unsigned int padding_from_filebegin = 54;
    }
        PACKED_STRUCT_END

        PACKED_STRUCT_BEGIN BitmapInfoHeader{
           unsigned int second_h_size = 40; //Ğàçìåğ ıòîãî õåäåğà
           int picture_Width;  //Øèğíà 
           int picture_Height;  //Âûñîòà
           uint16_t flatness_count = 1; //êîë-âî ïëîñêîñòåé
           uint16_t bit_per_pixel = 24; //êîë-âî áèò íà ïèêñåëü
           unsigned int compress_type = 0; // òèï ñæàòèÿ
           unsigned int bytes_count; //êîë- âî áàéò äàííûõ
           int gorizontal_resolution = 11811; // ãîğèçîíòàëüíîå ğàçğåøåíèå
           int vertical_resolution = 11811; // âåğòèêàëüíîå ğàçğåøåíèå
           unsigned int used_color = 0; //êîë-âî èñïîëüçóåìûõ öâåòîâ 
           unsigned int main_color = 0x1000000; //êîë-âî çíà÷èìûõ öâåòîâ   
    }
        PACKED_STRUCT_END

        // ôóíêöèÿ âû÷èñëåíèÿ îòñòóïà ïî øèğèíå
        static int GetBMPStride(int w) {
        return 4 * ((w * 3 + 3) / 4);
    };

    // íàïèøèòå ıòó ôóíêöèş
    bool SaveBMP(const Path& file, const Image& image) {

        ofstream out(file, ios::binary);
        BitmapFileHeader header;
        BitmapInfoHeader info;

        int stride = GetBMPStride(image.GetWidth());
        header.sum_size = stride * image.GetHeight() + 54;
        info.bytes_count = stride * image.GetHeight();
        info.picture_Width = image.GetWidth();
        info.picture_Height = image.GetHeight();

        /*header*/
        out.write(header.autograph, 2).
            write(reinterpret_cast<const char*>(&header.sum_size), 4).
            write(header.reserve_space, 4).
            write(reinterpret_cast<const char*>(&header.padding_from_filebegin), 4).
            /*info*/
            write(reinterpret_cast<const char*>(&info.second_h_size), 4).
            write(reinterpret_cast<const char*>(&info.picture_Width), 4).
            write(reinterpret_cast<const char*>(&info.picture_Height), 4).
            write(reinterpret_cast<const char*>(&info.flatness_count), 2).
            write(reinterpret_cast<const char*>(&info.bit_per_pixel), 2).
            write(reinterpret_cast<const char*>(&info.compress_type), 4).
            write(reinterpret_cast<const char*>(&info.bytes_count), 4).
            write(reinterpret_cast<const char*>(&info.gorizontal_resolution), 4).
            write(reinterpret_cast<const char*>(&info.vertical_resolution), 4).
            write(reinterpret_cast<const char*>(&info.used_color), 4).
            write(reinterpret_cast<const char*>(&info.main_color), 4);

        vector<char>buffer(GetBMPStride(info.picture_Width));

        for (int y = info.picture_Height - 1; y >= 0; --y) {
            int x = 0;

            for (x; x < info.picture_Width; ++x) {
                Color color = image.GetPixel(x, y);
                buffer[x * 3] = static_cast<char>(color.b);
                buffer[x * 3 + 1] = static_cast<char>(color.g),
                    buffer[x * 3 + 2] = static_cast<char>(color.r);
            }
            x *= 3;
            for (x; x < stride; ++x) { buffer[x] = '\0'; }
            out.write(buffer.data(), stride);
        };
        if (!out.good()) return false;
        return true;
    }


    template<typename T>
    T GetDigit(const vector<char>& buff) {
        assert(buff.size() <= 4);
        const int bit_padding = 8;
        int this_padding = 0;
        T res = 0;
        for (int i = 0; i < buff.size(); ++i) {
            res |= static_cast<uint8_t>(buff[i]) << this_padding;
            this_padding += bit_padding;
        }
        return res;
    }

    bool IsZeroSpace(const vector<char>& buff) {
        assert(buff.size() <= 4);
        for (int i = 0; i < buff.size(); ++i) {
            if (buff[i] != '\0') return false;
        }
        return true;
    }

    void FillZero(char spacefield[], size_t size) {
        for (int i = 0; i < size; ++i) {
            spacefield[i] = '\0';
        }
    }

    std::pair<BitmapFileHeader, bool> ReadeFileHeader(istream& ifs) {
        BitmapFileHeader bm_fileheader;
        vector<char> buffer(4, 'w');



        //ÇÀÃÎËÎÂÎÊ
        if (!ifs) return { std::move(bm_fileheader) , false };
        ifs.read(buffer.data(), 2);
        if (buffer[0] != 'B' && buffer[1] != 'M') return { std::move(bm_fileheader) , false }; //OFF 2
        bm_fileheader.autograph[0] = 'B';  bm_fileheader.autograph[1] = 'M';

#ifdef PR
        cout << bm_fileheader.autograph[0] << " " << bm_fileheader.autograph[1] << endl;
#endif

        //ĞÀÇÌÅĞ  
        ifs.read(buffer.data(), 4);
        bm_fileheader.sum_size = GetDigit<unsigned int>(buffer); //OFF 6

#ifdef PR
        cout << "bm_fileheader.sum_size " << bm_fileheader.sum_size << endl;
#endif

        //ÏÓÑÒÀß ÎÁËÀÑÒÜ   
        ifs.read(buffer.data(), 4);
        if (!IsZeroSpace(buffer)) return { std::move(bm_fileheader) , false }; //OFF 10

        FillZero(bm_fileheader.reserve_space, buffer.size());

#ifdef PR
        cout << "bm_fileheader.reserve_space " << bm_fileheader.reserve_space[0] << bm_fileheader.reserve_space[1] <<
            bm_fileheader.reserve_space[2] << bm_fileheader.reserve_space[3] << endl;
#endif

        //ÍÎÌÅĞ ÁÀÉÒÀ ÍÀ×ÀËÀ ĞÈÑÓÍÊÀ
        ifs.read(buffer.data(), 4);
        bm_fileheader.padding_from_filebegin = GetDigit<unsigned int>(buffer); //OFF 14

#ifdef PR
        cout << "bm_fileheader.padding_from_filebegin " << bm_fileheader.padding_from_filebegin << endl;
#endif

        if (!ifs.good())return { std::move(bm_fileheader) , false };
        return { std::move(bm_fileheader), true };
    }


    std::pair<BitmapInfoHeader, bool> ReadFileInfo(ifstream& ifs) {
        BitmapInfoHeader bm_info;

        vector<char> buffer(4);
        //ĞÀÇÌÅĞ ÇÀÃÎËÎÂÊÀ
        ifs.read(buffer.data(), 4);  if (!ifs.good())return {}; //OFF 18
        bm_info.second_h_size = GetDigit<unsigned int>(buffer);
        if (!ifs.good())return { std::move(bm_info) , false };

#ifdef PRX
        cout << "bm_info.second_h_size " << bm_info.second_h_size << endl;
#endif

        //ØÈĞÈÍÀ
        ifs.read(buffer.data(), 4);  if (!ifs.good())return {}; //OFF 22
        bm_info.picture_Width = GetDigit<unsigned int>(buffer);
        if (!ifs.good())return { std::move(bm_info) , false };

#ifdef PRX
        cout << " bm_info.picture_Width " << bm_info.picture_Width << endl;
#endif

        //ÂÛÑÎÒÀ
        ifs.read(buffer.data(), 4);  if (!ifs.good())return {};  //OFF 26
        bm_info.picture_Height = GetDigit<unsigned int>(buffer);
        if (!ifs.good())return { std::move(bm_info) , false };
#ifdef PRX
        cout << " bm_info.picture_Height " << bm_info.picture_Height << endl;
        //system("pause"); 
#endif


//ÂÍÈÌÀÍÈÅ!!!!!!
        buffer.resize(2);


        //ÏËÎÑÊÎÑÒÈ REMEMBER!!! SMALL BUFFER!!!
        ifs.read(buffer.data(), 2);  if (!ifs.good())return {};  //OFF 28
        bm_info.flatness_count = GetDigit<uint16_t>(buffer);
        if (!ifs.good())return { std::move(bm_info) , false };

#ifdef PRX
        cout << " bm_info.flatness_count " << bm_info.flatness_count << endl;
        //system("pause"); 
#endif


//ÁÈÒ ÍÀ ÏÈÊÑÅËÜ REMEMBER!!! SMALL BUFFER!!!
        ifs.read(buffer.data(), 2);  if (!ifs.good())return {};  //OFF 30
        bm_info.bit_per_pixel = GetDigit<uint16_t>(buffer);

#ifdef PRX
        cout << " bm_info.bit_per_pixel " << bm_info.bit_per_pixel << endl;
        //system("pause"); 
#endif

//ÂÍÈÌÀÍÈÅ!!!!!!
        buffer.resize(4);

        //ÒÈÏ ÑÆÀÒÈß
        ifs.read(buffer.data(), 4);  if (!ifs.good())return {};  //OFF 34
        bm_info.compress_type = GetDigit<unsigned int>(buffer);


#ifdef PRX
        cout << " bm_info.compress_type " << bm_info.compress_type << endl;
#endif

        //ÑÊÎËÜÊÎ ÁÀÉÒ ÄÀÍÍÛÕ
        ifs.read(buffer.data(), 4);  if (!ifs.good())return {};  //OFF 38
        bm_info.bytes_count = GetDigit<unsigned int>(buffer);


#ifdef PRX
        cout << " bm_info.bytes_count " << bm_info.bytes_count << endl;
        //system("pause"); 
#endif

 //ÃÎĞÈÇÎÍÒÀËÜÍÎÅ ĞÀÇĞÅØÅÍÈÅ
        ifs.read(buffer.data(), 4);  if (!ifs.good())return {};  //OFF 42
        bm_info.gorizontal_resolution = GetDigit<int>(buffer);


#ifdef PRX
        cout << " bm_info.gorizontal_resolution " << bm_info.gorizontal_resolution << endl;
#endif

        //ÂÅĞÒÈÊÀËÜÍÎÅ ĞÀÇĞÅØÅÍÈÅ
        ifs.read(buffer.data(), 4);  if (!ifs.good())return {};  //OFF 46
        bm_info.vertical_resolution = GetDigit<int>(buffer);


#ifdef PRX 
        cout << " bm_info.vertical_resolution " << bm_info.vertical_resolution << endl;
#endif

        //ÊÎËÈ×ÅÑÒÂÎ ÈÑÏÎËÜÇÎÂÀÍÍÛÕ ÖÂÅÒÎÂ
        ifs.read(buffer.data(), 4);  if (!ifs.good())return {};  //OFF 50
        bm_info.used_color = GetDigit<int>(buffer);


#ifdef PRX 
        cout << "bm_info.used_color " << bm_info.used_color << endl;
#endif

        //ÊÎËÈ×ÅÑÒÂÎ ÇÍÀ×ÈÌÛÕ ÖÂÅÒÎÂ
        ifs.read(buffer.data(), 4);  if (!ifs.good())return {};  //OFF 54
        bm_info.main_color = 0x1000000;

#ifdef PRX 
        cout << "bm_info.main_color " << bm_info.main_color << endl;
#endif
        if (!ifs.good())return { std::move(bm_info) , false };
        return { std::move(bm_info) , true };
    }

    // íàïèøèòå ıòó ôóíêöèş
    Image LoadBMP(const Path& file) {

        //SaveBMP(Path(), Image());  

        ifstream ifs(file, ios::binary);
        //ÓÇÍÀÅÌ ÄËÈÍÓ ÏÎÒÎÊÀ
        ifs.seekg(0, ios::end);
        int streamsize = static_cast<int>(ifs.tellg());
        ifs.seekg(0, ios::beg);

        //×ÈÒÀÅÌ ÕÅÄÅĞÛ
        std::pair<BitmapFileHeader, bool> fileheader = ReadeFileHeader(ifs);
        if (!fileheader.second) return {};
        std::pair<BitmapInfoHeader, bool> fileinfo = ReadFileInfo(ifs);
        if (!fileinfo.second)  return {};

        int bah = static_cast<int>(ifs.tellg()); /*bytes after header*/
        const int stride = GetBMPStride(fileinfo.first.picture_Width);
        //ÊÀĞÒÈÍÊÀ
        Image image(fileinfo.first.picture_Width, fileinfo.first.picture_Height, Color::Black());
        /*index bmp stream*/

        vector<char>imagebuf(stride);  /*áóôåğ*/
        int seeker = streamsize - stride;
        int pixel_y = 0;
        if (seeker < 0) return{};

        while (seeker >= bah) {
            ifs.seekg(seeker, ios::beg);
            ifs.read(imagebuf.data(), stride);

            for (int indbmp = 0; indbmp < fileinfo.first.picture_Width * 3; indbmp += 3) {
                int pixel_x = indbmp / 3;

                int b = static_cast<int>(imagebuf[indbmp]),
                    g = static_cast<int>(imagebuf[indbmp + 1]),
                    r = static_cast<int>(imagebuf[indbmp + 2]);

                image.GetPixel(pixel_x, pixel_y) = { byte(r), byte(g), byte(b) };
            }
            ++pixel_y;
            seeker -= stride;
        }
        return image;
    }

}// namespace img_lib