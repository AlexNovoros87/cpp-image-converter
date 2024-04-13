#include "bmp_image.h"
#include "pack_defines.h"
#include "img_lib.h"

#include <array>
#include <fstream>
#include <string_view>
#include <cassert>
#include <vector>

using namespace std;

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
          unsigned int second_h_size = 40; //Размер этого хедера
          int picture_Width;  //Ширна 
          int picture_Height;  //Высота
          uint16_t flatness_count = 1; //кол-во плоскостей
          uint16_t bit_per_pixel = 24; //кол-во бит на пиксель
          unsigned int compress_type = 0; // тип сжатия
          unsigned int bytes_count; //кол- во байт данных
          int gorizontal_resolution = 11811; // горизонтальное разрешение
          int vertical_resolution = 11811; // вертикальное разрешение
          unsigned int used_color = 0; //кол-во используемых цветов 
          unsigned int main_color = 0x1000000; //кол-во значимых цветов   
  }
      PACKED_STRUCT_END

      // функция вычисления отступа по ширине
      static int GetBMPStride(int w) {
      return 4 * ((w * 3 + 3) / 4);
  };

    
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

   std::pair<BitmapFileHeader, bool> ReadeFileHeader(istream& ifs) {
        BitmapFileHeader bm_fileheader;
        /*
        Уважаемый(ая) код-ревьюер, по большому счету можно было бы заголовок 
        считать полностью... но считаю что пэтапность и сфокусированнось 
        облегчает диагностику
        */
        vector<char> buffer(4);
        //ЗАГОЛОВОК
        if (!ifs) return { std::move(bm_fileheader) , false };
        ifs.read(buffer.data(), 2);
        if (buffer[0] != 'B' && buffer[1] != 'M') return { std::move(bm_fileheader) , false }; //OFF 2

        //РАЗМЕР  
        ifs.read(buffer.data(), 4);
        bm_fileheader.sum_size = GetDigit<unsigned int>(buffer); //OFF 6

        //ПУСТАЯ ОБЛАСТЬ   
        ifs.read(buffer.data(), 4);
        if (!IsZeroSpace(buffer)) return { std::move(bm_fileheader) , false }; //OFF 10
        
        //НОМЕР БАЙТА НАЧАЛА РИСУНКА
        ifs.read(buffer.data(), 4);  //OFF 14

        if (!ifs.good())return { std::move(bm_fileheader) , false };
        return { std::move(bm_fileheader), true };
    }


    std::pair<BitmapInfoHeader, bool> ReadFileInfo(ifstream& ifs) {
        BitmapInfoHeader bm_info;
         /*
        Так же можно было бы считывать большими кусками до нужного
        казателя... причина поэтапности выше 
        */
        
        vector<char> buffer(4);
        //РАЗМЕР ЗАГОЛОВКА
        ifs.read(buffer.data(), 4);   //OFF 18
        //ШИРИНА
        ifs.read(buffer.data(), 4);   //OFF 22
        bm_info.picture_Width = GetDigit<unsigned int>(buffer);
        //ВЫСОТА
        ifs.read(buffer.data(), 4);  //OFF 26
        bm_info.picture_Height = GetDigit<unsigned int>(buffer);
        //ПЛОСКОСТИ
        ifs.read(buffer.data(), 2);  //OFF 28
        //БИТ/ПИКСЕЛЬ
        ifs.read(buffer.data(), 2);  //OFF 30
        //ТИП СЖАТИЯ
        ifs.read(buffer.data(), 4); //OFF 34
        //СКОЛЬКО БАЙТ ДАННЫХ
        ifs.read(buffer.data(), 4);  //OFF 38
        bm_info.bytes_count = GetDigit<unsigned int>(buffer);
        //ГОРИЗОНТАЛЬНОЕ РАЗРЕШЕНИЕ
        ifs.read(buffer.data(), 4);   //OFF 42
        //ВЕРТИКАЛЬНОЕ РАЗРЕШЕНИЕ
        ifs.read(buffer.data(), 4);   //OFF 46
        //КОЛИЧЕСТВО ИСПОЛЬЗОВАННЫХ ЦВЕТОВ
        ifs.read(buffer.data(), 4);  if (!ifs.good())return {};  //OFF 50
        //КОЛИЧЕСТВО ЗНАЧИМЫХ ЦВЕТОВ
        ifs.read(buffer.data(), 4);  if (!ifs.good())return {};  //OFF 54
        if (!ifs.good())return { std::move(bm_info) , false };
        return { std::move(bm_info) , true };
    }

    // напишите эту функцию
    Image LoadBMP(const Path& file) {

        ifstream ifs(file, ios::binary);
        //УЗНАЕМ ДЛИНУ ПОТОКА
        ifs.seekg(0, ios::end);
        int streamsize = static_cast<int>(ifs.tellg());
        ifs.seekg(0, ios::beg);

        //ЧИТАЕМ ХЕДЕРЫ
        std::pair<BitmapFileHeader, bool> fileheader = ReadeFileHeader(ifs);
        if (!fileheader.second) return {};
        std::pair<BitmapInfoHeader, bool> fileinfo = ReadFileInfo(ifs);
        if (!fileinfo.second)  return {};

        int bah = static_cast<int>(ifs.tellg()); /*bytes after header*/
        const int stride = GetBMPStride(fileinfo.first.picture_Width); /*отступ*/
        //КАРТИНКА
        Image image(fileinfo.first.picture_Width, fileinfo.first.picture_Height, Color::Black());
        vector<char>imagebuf(stride);  /*буфер*/
        int seeker = streamsize - stride; /*указатель на место в потоке*/
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