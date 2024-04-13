    #include "ppm_image.h"
    #include <array>
    #include <fstream>
    #include <stdio.h>
    #include <setjmp.h>
    #include <iostream>
    #include <jpeglib.h>

    using namespace std;

    namespace img_lib {

        struct my_error_mgr {
            struct jpeg_error_mgr pub;
            jmp_buf setjmp_buffer;
        };

        typedef struct my_error_mgr* my_error_ptr;

        METHODDEF(void)
            my_error_exit(j_common_ptr cinfo) {
            my_error_ptr myerr = (my_error_ptr)cinfo->err;
            (*cinfo->err->output_message) (cinfo);
            longjmp(myerr->setjmp_buffer, 1);
        }

        
        bool SaveJPEG(const Path& file, const Image& image) {
            
            jpeg_compress_struct cinfo;
            jpeg_error_mgr jerr;
            
            FILE* outfile;    /* target file */
            JSAMPROW row_pointer[1];  /* pointer to JSAMPLE row[s] */
            int row_stride;       /* physical row width in image buffer */


            /* ШАГ 1 ОПИСАНИЕ ВНИЗУ */
            cinfo.err = jpeg_std_error(&jerr);
            /* Now we can initialize the JPEG compression object. */
            jpeg_create_compress(&cinfo);
            /* ШАГ 2 ОПИСАНИЕ ВНИЗУ */
        
    #ifdef _MSC_VER
            if (outfile = _wfopen(file.wstring().c_str(), "wb")) == NULL) { 
    #else
            if ((outfile = fopen(file.string().c_str(), "wb")) == NULL) { 
        
    #endif
        return false;
    }
        jpeg_stdio_dest(&cinfo, outfile);
            
            /* ШАГ 3 ОПИСАНИЕ ВНИЗУ */
            /* ширина и высота изображения в пикселях */
            cinfo.image_width = static_cast<JDIMENSION>(image.GetWidth());
            cinfo.image_height = static_cast<JDIMENSION>(image.GetHeight());
            cinfo.input_components = 3; /* количество цветовых компонентов на пиксель */   
            cinfo.in_color_space = JCS_RGB;  /* цветовое пространство входного изображения */
            // 3 - 1
            jpeg_set_defaults(&cinfo);
            //3 - 2
            
            /* ШАГ 4 ОПИСАНИЕ ВНИЗУ */
            jpeg_start_compress(&cinfo, TRUE);
            /* ШАГ 5 ОПИСАНИЕ ВНИЗУ */
            row_stride = image.GetWidth() * 3; /* Пикселей в строке */

    for(int y=0; y< image.GetHeight(); ++y ) {       
            std::vector<JSAMPLE> image_buffer(row_stride);
            
            for (int x = 0; x < image.GetWidth(); ++x) {
                image_buffer[3 * x] = static_cast<JSAMPLE>(image.GetPixel(x,y).r);
                image_buffer[1 + 3 * x] = static_cast<JSAMPLE>(image.GetPixel(x,y).g);
                image_buffer[2 + 3 * x] = static_cast<JSAMPLE>(image.GetPixel(x,y).b);
            }
            
            row_pointer[0] = &image_buffer[0];
            (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }
            /* Шаг 6: Завершение сжатия */
            jpeg_finish_compress(&cinfo);
            /* После finish_compress мы можем закрыть выходной файл. */
            fclose(outfile);
            /* ШАГ 7 ОПИСАНИЕ ВНИЗУ */
            jpeg_destroy_compress(&cinfo);
        return true;
        }

        // тип JSAMPLE фактически псевдоним для unsigned char
        void SaveSсanlineToImage(const JSAMPLE* row, int y, Image& out_image) {
            Color* line = out_image.GetLine(y);
            for (int x = 0; x < out_image.GetWidth(); ++x) {
                const JSAMPLE* pixel = row + x * 3;
                line[x] = Color{ byte{pixel[0]}, byte{pixel[1]}, byte{pixel[2]}, byte{255} };
            }
        }

    Image LoadJPEG(const Path& file) {
        jpeg_decompress_struct cinfo;
        my_error_mgr jerr;

        FILE* infile;
        JSAMPARRAY buffer;
        int row_stride;

        // Тут не избежать функции открытия файла из языка C,
        // поэтому приходится использовать конвертацию пути к string.
        // Под Visual Studio это может быть опасно, и нужно применить
        // нестандартную функцию _wfopen
    #ifdef _MSC_VER
        if ((infile = _wfopen(file.wstring().c_str(), "rb")) == NULL) {
    #else
        if ((infile = fopen(file.string().c_str(), "rb")) == NULL) {
    #endif
            return {};
        }

        /* Шаг 1: выделяем память и инициализируем объект декодирования JPEG */
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;

        if (setjmp(jerr.setjmp_buffer)) {
            jpeg_destroy_decompress(&cinfo);
            fclose(infile);
            return {};
        }
        jpeg_create_decompress(&cinfo);

        /* Шаг 2: устанавливаем источник данных */
        jpeg_stdio_src(&cinfo, infile);
    
        /* Шаг 3: читаем параметры изображения через jpeg_read_header() */
        (void)jpeg_read_header(&cinfo, TRUE);
    
        /* Шаг 4: устанавливаем параметры декодирования */
        // установим желаемый формат изображения
        cinfo.out_color_space = JCS_RGB;
        cinfo.output_components = 3;

        /* Шаг 5: начинаем декодирование */
        (void)jpeg_start_decompress(&cinfo);
        row_stride = cinfo.output_width * cinfo.output_components;
       buffer = (*cinfo.mem->alloc_sarray)
            ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

        /* Шаг 5a: выделим изображение ImgLib */
        Image result(cinfo.output_width, cinfo.output_height, Color::Black());

        /* Шаг 6: while (остаются строки изображения) jpeg_read_scanlines(...); */
        while (cinfo.output_scanline < cinfo.output_height) {
            int y = cinfo.output_scanline;
            (void)jpeg_read_scanlines(&cinfo, buffer, 1);
            SaveSсanlineToImage(buffer[0], y, result);
        }

        /* Шаг 7: Останавливаем декодирование */
        (void)jpeg_finish_decompress(&cinfo);
        /* Шаг 8: Освобождаем объект декодирования */
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        
        return result;
    }

    } // of namespace img_lib

    ////////////////////////////////////SAVEJPEG///////////////////////////////////////////////////

    // В эту функцию вставлен код примера из библиотеки libjpeg.
    // Измените его, чтобы адаптировать к переменным file и image.
    // Задание качества уберите - будет использовано качество по умолчанию
    /* 
    jpeg_compress_struct cinfo;
    Эта структура содержит параметры сжатия JPEG и указатели на
    рабочее пространство (которое выделяется библиотекой JPEG по мере необходимости).
    Возможно, что одновременно существует несколько таких структур, представляющих несколько
    процессов сжатия/распаковки. Мы ссылаемся на
    к любой структуре (и связанным с ней рабочим данным) в качестве "объекта JPEG".

    jpeg_error_mgr jerr;
    Эта структура представляет обработчик ошибок в формате JPEG. Она объявлена отдельно
    , потому что приложения часто хотят предоставить специализированный обработчик ошибок
    (пример смотрите во второй половине этого файла). Но здесь мы просто
    воспользуйтесь простым способом и используйте стандартный обработчик ошибок, который
    напечатает сообщение в stderr и вызовет exit() в случае сбоя сжатия.
    Обратите внимание, что эта структура должна работать столько же, сколько и основной параметр JPEG
    struct, чтобы избежать проблем с зависанием указателя.
    
        
    Шаг 1: выделите и инициализируйте объект сжатия JPEG 
    Сначала мы должны настроить обработчик ошибок на случай, если инициализация
    завершится неудачно. (Маловероятно, но это может произойти, если у вас не хватает памяти.)
    Эта процедура заполняет содержимое struct jerr и возвращает адрес jerr
    , который мы помещаем в поле ссылки в cinfo.
    
    ШАГ 2 
    укажите место назначения данных (например, файл) 
    Примечание: шаги 2 и 3 можно выполнять в любом порядке. 
    Здесь мы используем предоставленный библиотекой код для отправки сжатых данных в поток
    stdio. Вы также можете написать свой собственный код для выполнения чего-либо еще.
    ОЧЕНЬ ВАЖНО: используйте опцию "b" для fopen(), если вы работаете на компьютере, который
    требует этого для записи двоичных файлов.
    
    Шаг 3: задаем параметры сжатия 
    Сначала мы предоставляем описание входного изображения.
    Необходимо заполнить четыре поля структуры cinfo:
    
    3-1
    Теперь используйте библиотечную процедуру для установки параметров сжатия по умолчанию.
    (Перед вызовом этой функции вы должны установить как минимум cinfo.in_color_space,
    поскольку значения по умолчанию зависят от исходного цветового пространства.)
    3-2
    Теперь вы можете задать любые параметры, отличные от параметров по умолчанию.
    * Здесь мы просто иллюстрируем использование масштабирования по качеству (таблица квантования)
    
    Шаг 4: Запустите компрессор
    Значение TRUE гарантирует, что мы создадим полноценный файл формата interchange-JPEG.
    Укажите значение TRUE, если вы не очень уверены в том, что делаете
    
    Шаг 5: пока (остается записать строки сканирования) 
    Здесь мы используем библиотечную переменную состояния cinfo.next_scanline в качестве
    счетчика циклов, чтобы нам не приходилось самим отслеживать.
    Чтобы упростить задачу, мы передаем одну строку сканирования за вызов; вы можете передать
    впрочем, если хотите, то больше.
    
    Шаг 7: освободите объект сжатия JPEG 
    Это важный шаг, поскольку он освободит значительный объем памяти. */
    
    
    
    
    
    
    
    








    
    
    
    