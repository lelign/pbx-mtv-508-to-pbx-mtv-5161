#include <QDebug>
#include <QLoggingCategory>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <arm_neon.h>
#include "mtv-system.h"
#include "str-mem-dev.h"
#include "scaler_coeff.h"
#include <fstream> // for js
#include <QCoreApplication>

//static QLoggingCategory category("SYSTEM");
static QLoggingCategory category("mtv-system"); // ign

const char * fname = "/dev/str-mem";
//const int video_size = 1920*1080*3; // RGB
//const int video_size = 1920*1080*2; // YCrCb
//const int video_size = 4149248; // Строго 4149248 байт, как в драйвере! YCrCb
/*для альфа увеличил до 1920 * 1080 * 3 = 6220800*/
const int video_size = 6220800;
#define MOTION_THR (100)
#define ANCIN ("/dev/tty10")

/*//enum 5161
enum {
                REG_HDMI_OUT,
                REG_MOSAIC,
                REG_FRAMEBUFFER_0,
                REG_FRAMEBUFFER_1,
                REG_FRAMEBUFFER_2,
                REG_FRAMEBUFFER_3,
                REG_FRAMEBUFFER_4,
                REG_FRAMEBUFFER_5,
                REG_FRAMEBUFFER_6,
                REG_FRAMEBUFFER_7,
                REG_FRAMEBUFFER_8,
                REG_FRAMEBUFFER_9,
                REG_FRAMEBUFFER_10,
                REG_FRAMEBUFFER_11,
                REG_FRAMEBUFFER_12,
                REG_FRAMEBUFFER_13,
                REG_FRAMEBUFFER_14,
                REG_FRAMEBUFFER_15,
                REG_SDI_ADAPTER,
                REG_CVI_0,
                REG_SCALER_0,
                REG_CVI_1,
                REG_SCALER_1 ,
                REG_CVI_2,
                REG_SCALER_2,
                REG_CVI_3,
                REG_SCALER_3,
                REG_CVI_4,
                REG_SCALER_4,
                REG_CVI_5,
                REG_SCALER_5,
                REG_CVI_6,
                REG_SCALER_6,
                REG_CVI_7,
                REG_SCALER_7,
                REG_CVI_7a,
                REG_SCALER_7a,
                REG_CVI_8,
                REG_SCALER_8,
                REG_CVI_9,
                REG_SCALER_9,
                REG_CVI_10,
                REG_SCALER_10,
                REG_CVI_11,
                REG_SCALER_11,
                REG_CVI_12,
                REG_SCALER_12,
                REG_CVI_13,
                REG_SCALER_13,
                REG_CVI_14,
                REG_SCALER_14,
                REG_CVI_15,
                REG_SCALER_15,
                REG_BUILDID,
                REG_BARS,
                REG_AUDIO_SELECTOR,
                REG_MOTION_0,
                REG_MOTION_1,
                REG_MOTION_2,
                REG_MOTION_3,
                REG_MOTION_4,
                REG_MOTION_5,
                REG_MOTION_6,
                REG_MOTION_7,
                REG_MOTION_8,
                REG_MOTION_9,
                REG_MOTION_10,
                REG_MOTION_11,
                REG_MOTION_12,
                REG_MOTION_13,
                REG_MOTION_14,
                REG_MOTION_15,
                REG_DEI,
                REG_SDI_CVO, 
                END
};
*/

//enum 508
enum {
        REG_HDMI_OUT,
        REG_MOSAIC,
        REG_FRAMEBUFFER_0,
        REG_FRAMEBUFFER_1,
        REG_FRAMEBUFFER_2,
        REG_FRAMEBUFFER_3,
        REG_FRAMEBUFFER_4,
        REG_FRAMEBUFFER_5,
        REG_FRAMEBUFFER_6,
        REG_FRAMEBUFFER_7,
        REG_SDI_ADAPTER,
        REG_CVI_0,
        REG_SCALER_0,
        REG_CVI_1,
        REG_SCALER_1,
        REG_CVI_2,
        REG_SCALER_2,
        REG_CVI_3,
        REG_SCALER_3,
        REG_CVI_4,
        REG_SCALER_4,
        REG_CVI_5,
        REG_SCALER_5,
        REG_CVI_6,
        REG_SCALER_6,
        REG_CVI_7,
        REG_SCALER_7,
        REG_BUILDID,
        REG_BARS,
        REG_AUDIO_SELECTOR, 
        REG_MOTION_0,
        REG_MOTION_1,
        REG_MOTION_2,
        REG_MOTION_3,
        REG_MOTION_4,
        REG_MOTION_5,
        REG_MOTION_6,
        REG_MOTION_7,
        REG_DEI,
        REG_SDI_CVO,
};

QList <QString> enumlist = {
        "REG_HDMI_OUT",
        "REG_MOSAIC",
        "REG_FRAMEBUFFER_0",
        "REG_FRAMEBUFFER_1",
        "REG_FRAMEBUFFER_2",
        "REG_FRAMEBUFFER_3",
        "REG_FRAMEBUFFER_4",
        "REG_FRAMEBUFFER_5",
        "REG_FRAMEBUFFER_6",
        "REG_FRAMEBUFFER_7",
        "REG_SDI_ADAPTER",
        "REG_CVI_0",
        "REG_SCALER_0",
        "REG_CVI_1",
        "REG_SCALER_1",
        "REG_CVI_2",
        "REG_SCALER_2",
        "REG_CVI_3",
        "REG_SCALER_3",
        "REG_CVI_4",
        "REG_SCALER_4",
        "REG_CVI_5",
        "REG_SCALER_5",
        "REG_CVI_6",
        "REG_SCALER_6",
        "REG_CVI_7",
        "REG_SCALER_7",
        "REG_BUILDID",
        "REG_BARS",
        "REG_AUDIO_SELECTOR", 
        "REG_MOTION_0",
        "REG_MOTION_1",
        "REG_MOTION_2",
        "REG_MOTION_3",
        "REG_MOTION_4",
        "REG_MOTION_5",
        "REG_MOTION_6",
        "REG_MOTION_7",
        "REG_DEI",
        "REG_SDI_CVO"
};

#define FORMAT_SD (0<<4)
#define FORMAT_HD (1<<4)
#define FORMAT_3G (3<<4)

video_format_t video_format[] = {
        // 625i50
        {
                .id = FORMAT_SD|1,
                .interlaced = 1,
                .width = 720,
                .height = 288,
        },
        // 1080i59.94
        {
                .id = FORMAT_HD|4,
                .interlaced = 1,
                .width = 1920,
                .height = 540,
        },
        // 1080i50
        {
                .id = FORMAT_HD|5,
                .interlaced = 1,
                .width = 1920,
                .height = 540,
        },
        // 720p59.94 -> 720i59.94
        {
                .id = FORMAT_HD|7,
                .interlaced = 0,
                .width = 1280,
                .height = 360,
        },
        // 720p50 -> 720i50
        {
                .id = FORMAT_HD|8,
                .interlaced = 0,
                .width = 1280,
                .height = 360,
        },
        // 1080p59.94 -> 1080i59.94
        {
                .id = FORMAT_3G|12,
                .interlaced = 0,
                .width = 1920,
                .height = 540,
        },
        // 1080p50 -> 1080i50
        {
                .id = FORMAT_3G|13,
                .interlaced = 0,
                .width = 1920,
                .height = 540,
        },
        // 1080p25 -> 1080i25
        {
                .id = FORMAT_HD|13,
                .interlaced = 0,
                .width = 1920,
                .height = 540,
        },
        {
                .id = -1,
                .interlaced = 0,
                .width = 0,
                .height = 0,
        }
};

int32_t level_to_db_table[] = {
        0, -100,
        3, -60,
        7, -59,
        12, -58,
        16, -57,
        21, -56,
        26, -55,
        31, -54,
        35, -53,
        40, -52,
        46, -51,
        51, -50,
        56, -49,
        61, -48,
        67, -47,
        72, -46,
        78, -45,
        84, -44,
        90, -43,
        96, -42,
        102, -41,
        109, -40,
        116, -39,
        123, -38,
        130, -37,
        137, -36,
        145, -35,
        153, -34,
        162, -33,
        171, -32,
        180, -31,
        191, -30,
        201, -29,
        212, -28,
        223, -27,
        233, -26,
        244, -25,
        255, -24,
        265, -23,
        276, -22,
        287, -21,
        297, -20,
        308, -19,
        319, -18,
        329, -17,
        340, -16,
        351, -15,
        361, -14,
        372, -13,
        383, -12,
        393, -11,
        404, -10,
        415, -9,
        425, -8,
        436, -7,
        447, -6,
        457, -5,
        468, -4,
        479, -3,
        489, -2,
        500, -1,
        511, 0,
        //
        -1, -1,
};

typedef struct {
        int id;
        uint32_t h_front_porch;
        uint32_t h_sync;
        uint32_t h_back_porch;
        uint32_t total_line;
        uint32_t sdmux;
        uint32_t interlaced;
        uint32_t v_active;
        uint32_t v_front_porch;
        uint32_t v_sync;
        uint32_t v_2_front_porch;
        uint32_t v_2_sync;
        uint32_t v_2_vsync_pixel;
        uint32_t f_rising;
        uint32_t f_falling;
        uint32_t total_lines;
        uint32_t total_lines_f2;
        uint32_t width;
        uint32_t height;
} cvo_settings_t;

cvo_settings_t cvo_1080i50 = {
        .id = 0,
        .h_front_porch = 528,
        .h_sync = 572,
        .h_back_porch = 720,
        .total_line = 2640,
        .sdmux = 0,
        .interlaced = 1,
        .v_active = 540,
        .v_front_porch = 542,
        .v_sync = 547,
        .v_2_front_porch = 542,
        .v_2_sync = 547,
        .v_2_vsync_pixel = 1848,
        .f_rising = 543,
        .f_falling = 542,
        .total_lines = 563,
        .total_lines_f2 = 562,
        .width = 1920,
        .height = 540,
};

cvo_settings_t cvo_1080p25 = {
        .id = 0,
        .h_front_porch = 528,
        .h_sync = 572,
        .h_back_porch = 720,
        .total_line = 2640,
        .sdmux = 0,
        .interlaced = 0,
        .v_active = 1080,
        .v_front_porch = 1084,
        .v_sync = 1089,
        .v_2_front_porch = 1084,
        .v_2_sync = 1089,
        .v_2_vsync_pixel = 1848,
        .f_rising = 543,
        .f_falling = 542,
        .total_lines = 1125,
        .total_lines_f2 = 1125,
        .width = 1920,
        .height = 1080,
};

PbxMtvSystem::PbxMtvSystem()
{
        /*for js*/
        // СТАРТ ПРИЛОЖЕНИЯ: Один раз читаем файл с диска, если он существует
        std::ifstream input_file(log_path);
        if (input_file.is_open()) {
                try {
                        input_file >> log_obj;
                } catch (const nlohmann::json::parse_error& e) {
                        log_obj = nlohmann::json::object(); // Если файл битый, создаем пустой {}
                }
                input_file.close();
        } else {
                log_obj = nlohmann::json::object(); // Если файла нет, создаем пустой {}
        }
        dei = 0;
        reconfigure_timer.setSingleShot(true);
        connect(&reconfigure_timer, &QTimer::timeout, this, &PbxMtvSystem::reconfigure_timeout);
        //buffer = (char*) malloc(video_size);
        //buffer = static_cast<char*>(aligned_alloc(64, video_size));
        init_overlay_memory();
        uint8_t* start_address = reinterpret_cast<uint8_t*>(buffer);
        uint8_t* end_address   = start_address + video_size;
        qDebug(category) << "Buffer START address:" << static_cast<void*>(start_address);
        qDebug(category) << "Buffer END   address:" << static_cast<void*>(end_address);
        //qDebug(category) << "Total buffer size:   " << video_size << "bytes";
        
        connect(&sdi_format_timer, &QTimer::timeout, this, &PbxMtvSystem::sdi_format_timeout);
        sdi_format_timer.start(100);
        memset(&image_config, 0, sizeof(image_config));
        sdi_format_notify_timer.setSingleShot(true);
        connect(&sdi_format_notify_timer, &QTimer::timeout, this, &PbxMtvSystem::sdi_format_notify_timeout);
        set_audio_source(0);
        reconfigure();
        qDebug(category) << "ANCIN : " << ANCIN;
        anc_reader = new AncReader(ANCIN, this);
        anc_reader->start();
}

PbxMtvSystem::~PbxMtvSystem()
{
        free(buffer);
        anc_reader->stop();
        delete anc_reader;
}

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

/*init_overlay_memory() добавьте старт таймера:*/
/*void PbxMtvSystem::init_overlay_memory() {
    overlay_fd = open("/dev/mtv-overlay", O_RDWR);
    if (overlay_fd >= 0) {
        //buffer = (char*)mmap(NULL, 1920*1080*3, PROT_READ | PROT_WRITE, MAP_SHARED, overlay_fd, 0);
        buffer = (char*)mmap(NULL, 1920*1080*2, PROT_READ | PROT_WRITE, MAP_SHARED, overlay_fd, 0);

        if (buffer != MAP_FAILED) {
            qDebug(category) << "Success! Kernel memory mapped via mmap";

            // ИНИЦИАЛИЗИРУЕМ АППАРАТНЫЙ ТАЙМЕР НА 60 Гц (16 мс) 
            fps_timer = new QTimer(this);
            connect(fps_timer, &QTimer::timeout, this, &PbxMtvSystem::slot_fps_hardware_trigger);
            fps_timer->setInterval(16); // 16 мс = ~60 кадров в секунду
            fps_timer->start();
        }else{

                qDebug(category) << "else Success";

        }

    }else{
        qDebug(category) << "if (overlay_fd >= 0)";

    }
}
*/

void PbxMtvSystem::init_overlay_memory() {
    // Выставляем полный размер RGB888 кадра
    //video_size = 1920 * 1080 * 3; /* 6 220 800 байт */

    overlay_fd = open("/dev/mtv-overlay", O_RDWR);
    if (overlay_fd >= 0) {
        // Мапим полные 6.2 Мегабайта буфера vmalloc ядра
        buffer = (char*)mmap(NULL, video_size, PROT_READ | PROT_WRITE, MAP_SHARED, overlay_fd, 0);

        if (buffer != MAP_FAILED) {
            qDebug(category) << "mtv-system: Success! Kernel memory mapped via mmap";
            /*
            qDebug(category) << "mtv-system: Total buffer size: " << video_size << "bytes";
            qDebug(category) << "mtv-system: Buffer START address:" << static_cast<void*>(buffer);
            qDebug(category) << "mtv-system: Buffer END address:  " << static_cast<void*>(buffer + video_size);
            */

            // Инициализация FPS-таймера на частоту обновления экрана (~60 Гц)
            fps_timer = new QTimer(this);
            connect(fps_timer, &QTimer::timeout, this, &PbxMtvSystem::slot_fps_hardware_trigger);
            fps_timer->setInterval(16); // 16 миллисекунд
            fps_timer->start();
        } else {
            qCritical(category) << "mtv-system: critical error mmap! Cause:" << strerror(errno);
        }
    } else {
        qCritical(category) << "mtv-system: Can't open /dev/mtv-overlay! Error:" << strerror(errno);
    }
}


/*void PbxMtvSystem::init_overlay_memory() {
    int fd = open("/dev/mtv-overlay", O_RDWR);
    if (fd >= 0) {
        // Теперь компилятор знает все флаги и успешно соберет Zero-Copy маппинг
        buffer = (char*)mmap(NULL, 1920*1080*3, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        
        if (buffer == MAP_FAILED) {
            qCritical(category) << "critical error: mmap of buffers FPGA return MAP_FAILED!";
        } else {
            qDebug(category) << "Success! Kernel memory mapped to Userspace at:" << static_cast<void*>(buffer);
        }
    } else {
        qCritical(category) << "Failed to open /dev/mtv-overlay for mmap! Error:" << strerror(errno);
    }
}*/


/*Реализуйте функцию триггера, которая будет непрерывно кормить FPGA данными:*/
void PbxMtvSystem::slot_fps_hardware_trigger()
{
    if (overlay_fd >= 0) {
        char kick_signal = 1;
        // Отправляем 1 байт. Драйвер мгновенно перевыставит дескриптор кадра в FPGA
        // Благодаря mmap, процессор не тратит время на копирование пикселей
        ::write(overlay_fd, &kick_signal, 1); 
    }
}



void PbxMtvSystem::framebuffer_start(int index, int value)
{
        uint32_t base;
        
        switch(index){
        default:
        case 0:
                base = REG_FRAMEBUFFER_0;
                break;
        case 1:
                base = REG_FRAMEBUFFER_1;
                break;
        case 2:
                base = REG_FRAMEBUFFER_2;
                break;
        case 3:
                base = REG_FRAMEBUFFER_3;
                break;
        case 4:
                base = REG_FRAMEBUFFER_4;
                break;
        case 5:
                base = REG_FRAMEBUFFER_5;
                break;
        case 6:
                base = REG_FRAMEBUFFER_6;
                break;
        case 7:
                base = REG_FRAMEBUFFER_7;
                break;
        }

        if(value)
                reg_write(base, 0, 3);
        else
                reg_write(base, 0, 0);
}

void PbxMtvSystem::framebuffer_reconfigure(int index, int width, int height)
{
        uint32_t base;
        
        switch(index){
        default:
        case 0:
                base = REG_FRAMEBUFFER_0;
                break;
        case 1:
                base = REG_FRAMEBUFFER_1;
                break;
        case 2:
                base = REG_FRAMEBUFFER_2;
                break;
        case 3:
                base = REG_FRAMEBUFFER_3;
                break;
        case 4:
                base = REG_FRAMEBUFFER_4;
                break;
        case 5:
                base = REG_FRAMEBUFFER_5;
                break;
        case 6:
                base = REG_FRAMEBUFFER_6;
                break;
        case 7:
                base = REG_FRAMEBUFFER_7;
                break;
        }
        

        reg_write(base, 1, width*height/3);
        reg_write(base, 2, 
                (dei<<2)|
                (1<<1)|
                (1<<0)
        );
}

void PbxMtvSystem::mosaic_reconfigure(int index, int x, int y, int width, int height, int enable)
{
        uint32_t base;
        
        base = REG_MOSAIC;

        reg_write(base, 16+index*2+0, 
                ((width+x) << 0)
                |((height+y) << 11)
        );
        reg_write(base, 16+index*2+1, 
                ((x) << 0)
                |((y) << 11)
                |((enable)<<22)
        );
}

void PbxMtvSystem::scaler_scaler_config(int index, int bypass, int width, int height, 
        int out_width, int out_height, int deinterlace, int unsharp_bypass, int csc_mode)
{
        int base;

        switch(index){
        default:
        case 0:
                base = REG_SCALER_0;
                break;
        case 1:
                base = REG_SCALER_1;
                break;
        case 2:
                base = REG_SCALER_2;
                break;
        case 3:
                base = REG_SCALER_3;
                break;
        case 4:
                base = REG_SCALER_4;
                break;
        case 5:
                base = REG_SCALER_5;
                break;
        case 6:
                base = REG_SCALER_6;
                break;
        case 7:
                base = REG_SCALER_7;
                break;
        }

        if(out_height==0 || out_width==0)
                return;
        
        uint32_t out_line_inc = (height-6-1)*4096/(out_height);
        uint32_t out_pixel_inc = (width-8-1)*4096/(out_width);

        reg_write(base, 2, out_width  - 1);
        reg_write(base, 3, out_height);
        reg_write(base, 4, out_line_inc);
        reg_write(base, 5, out_pixel_inc);
        // enable
        reg_write(base, 6, 0
                |(unsharp_bypass<<0)
                |(bypass<<1)
                |(csc_mode<<2)
                |(deinterlace<<4)                
        );
        reg_write(base, 0, width - 1);
        reg_write(base, 1, height - 1);
}

void PbxMtvSystem::scaler_coeff(int index, uint32_t * coeff)
{
        int base;
        switch(index){
        default:
        case 0:
                base = REG_SCALER_0;
                break;
        case 1:
                base = REG_SCALER_1;
                break;
        case 2:
                base = REG_SCALER_2;
                break;
        case 3:
                base = REG_SCALER_3;
                break;
        case 4:
                base = REG_SCALER_4;
                break;
        case 5:
                base = REG_SCALER_5;
                break;
        case 6:
                base = REG_SCALER_6;
                break;
        case 7:
                base = REG_SCALER_7;
                break;
        }
        
        for(unsigned int i=0; i<32; i++){
                reg_write(base, 10, coeff[i*3+0]);
                reg_write(base, 11, coeff[i*3+1]);
                reg_write(base, 12, coeff[i*3+2]);
        }
}

void PbxMtvSystem::scaler_reconfigure(int index, int width_in, int height_in, int width_out, int height_out)
{
        int bypass = 0;
        if((width_in == width_out)&&(height_in == height_out))
                bypass = 1;
        scaler_scaler_config(index, bypass, width_in, height_in, width_out, height_out, 0, 0, 0);
        if(width_out>=width_in){
                scaler_coeff(index, coeff_100);
                scaler_coeff(index, coeff_y_100);
        }else{
                if(width_out*100 / width_in >= 75){
                        scaler_coeff(index, coeff_75);
                        scaler_coeff(index, coeff_y_75);
                }else if(width_out*100 / width_in >= 50){
                        scaler_coeff(index, coeff_50);
                        scaler_coeff(index, coeff_y_50);
                }else if(width_out*100 / width_in >= 25){
                        scaler_coeff(index, coeff_25);
                        scaler_coeff(index, coeff_y_25);
                }else{
                        scaler_coeff(index, coeff_10);
                        scaler_coeff(index, coeff_y_10);
                }
        }
}

/*//main void
void PbxMtvSystem::reg_write(uint32_t block, uint32_t addr, uint32_t data)
{
        strmem_reg_data reg_data;
        int ret;
        int fd;

        reg_data.block = block;
        reg_data.address = addr*4;
        reg_data.data = data;
        reg_data.rw = STR_REG_WRITE;

        fd = open(fname, O_RDONLY);
        if(fd<0){
                return;
        }
        ret = ioctl(fd, STRMEM_IOCTL_REG, &reg_data);
	if(ret<0){
	        printf("ioctl error\n");
	}
        close(fd);
}
*/

void PbxMtvSystem::reg_write(uint32_t block, uint32_t addr, uint32_t data)
{
        strmem_reg_data reg_data;
        int ret;
        int fd;

        reg_data.block = block;
        reg_data.address = addr * 4;
        reg_data.data = data;
        reg_data.rw = STR_REG_WRITE;

        fd = open(fname, O_RDONLY);
        if (fd < 0) {
                return;
        }
        ret = ioctl(fd, STRMEM_IOCTL_REG, &reg_data);
        if (ret < 0) {
                printf("ioctl error\n");
        }
        close(fd);

        // ОБНОВЛЕНИЕ ДАННЫХ В ПАМЯТИ
        QStringList args = QCoreApplication::arguments();

        if (args.contains("--jq")) {
                qDebug(category) << "jq mode is enabled inside MyClass!";
        
                std::string block_key ; // = "block_" + std::to_string(block);
                std::string addr_key  = "addr_0x" + std::to_string(addr * 4);
                //std::string block_list_key; 
                if (block >= 0 && block < enumlist.size()){
                        block_key = enumlist.at(block).toStdString();
                        // Если блока нет в памяти, добавляем его объект
                        if (!log_obj.contains(block_key)) {
                                log_obj[block_key] = nlohmann::json::object();
                        }

                        // Добавляем значение регистра в массив истории этого адреса
                        log_obj[block_key][addr_key].push_back(data);

                        // СБРОС ИЗМЕНЕНИЙ НА ДИСК
                        std::ofstream output_file(log_path);
                        if (output_file.is_open()) {
                                output_file << log_obj.dump(4); // pretty-print с отступом 4 пробела
                                output_file.close();
                        }
                }else{
                        qDebug(category) << "Error in reg_write with block_key block" << block;
                }
        }
}


void PbxMtvSystem::bars_configure(int index, int x, int x2, int y, int scale, int enable_1, int enable_2)
{
        if(dei){
                y = y*2;
                scale = (scale + 1) * 2 - 1;
        }

        reg_write(REG_BARS, index*2+0, 0
                |(x<<0)
                |((y/2)<<11)
                |((scale&0x0f)<<22)
                |(enable_1<<26)
        );

        reg_write(REG_BARS, index*2+1, 0
                |(x2<<0)
                |((y/2)<<11)
                |((scale&0x0f)<<22)
                |(enable_2<<26)
        );
}


uint32_t PbxMtvSystem::reg_read(uint32_t block, uint32_t addr)
{
        strmem_reg_data reg_data;
        int ret;
        int fd;

        reg_data.block = block;
        reg_data.address = addr*4;
        reg_data.data = 0;
        reg_data.rw = STR_REG_READ;

        fd = open(fname, O_RDONLY);
        if(fd<0){
                return 0;
        }
        ret = ioctl(fd, STRMEM_IOCTL_REG, &reg_data);
	if(ret<0){
	        printf("ioctl error\n");
	}
        close(fd);
        return reg_data.data;
}

QString PbxMtvSystem::get_build_id()
{
        uint32_t reg;

        reg = reg_read(REG_BUILDID, 0);
        //qDebug(category) << "reg_read(REG_BUILDID, 0)" << hex << reg;
        //qDebug(category) << "reg_read(REG_BUILDID, 0)" << reg << QString("%1%2%3").arg((reg>>16)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>8)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>0)&0xFF, 2, 10, QLatin1Char('0'));
        return QString("%1%2%3").arg((reg>>16)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>8)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>0)&0xFF, 2, 10, QLatin1Char('0'));
}

int PbxMtvSystem::limit_color(int value)
{
        if(value > 255)
                value = 255;
        if(value < 0)
                value = 0;
        return value;
}

QRgb PbxMtvSystem::rgb_to_ycrcb(QRgb value)
{
        QRgb ret;

        int32_t y_value;
        int32_t cr_value;
        int32_t cb_value;

        int32_t a_value = (value>>24)&0xFF;
        int32_t r_value = (value>>16)&0xFF;
        int32_t g_value = (value>>8)&0xFF;
        int32_t b_value = (value>>0)&0xFF;

        if(a_value!=0){
                y_value = (54*r_value + 183*g_value + 18*b_value)/256;
                cr_value = (r_value-y_value)*256/(238)/2+128;
                cb_value = (b_value-y_value)*256/(201)/2+128;

                y_value = limit_color(y_value);
                cr_value = limit_color(cr_value);
                cb_value = limit_color(cb_value);
        }else{
                y_value = 127;
                cr_value = 127;
                cb_value = 127;
        }

        ret =  0
                |((a_value&0xFF)<<24)
                |((y_value&0xFF)<<16)
                |((cr_value&0xFF)<<8)
                |((cb_value&0xFF)<<0)
                ;
        return ret;
}

QImage * PbxMtvSystem::image_to_prpb(QImage * image)
{
        QImage * ret = new QImage(image->width(), image->height(), QImage::Format_ARGB32);
        Q_CHECK_PTR(ret);

        for(int y=0; y<image->height(); y++){
                const uint8_t * line_in = image->constScanLine(y);
                const uint32_t * pixel_in = (uint32_t*) line_in;
                uint8_t * line = ret->scanLine(y);
                uint32_t * pixel_out = (uint32_t*) line;
                for(int x=0; x<image->width()/2; x++){
                        QRgb pixel1 = rgb_to_ycrcb(pixel_in[x*2]);
                        QRgb pixel2 = rgb_to_ycrcb(pixel_in[x*2+1]);

                        int32_t cr_value = (((pixel1>>8)&0xFF) + ((pixel2>>8)&0xFF))/2;
                        int32_t cb_value = (((pixel1>>0)&0xFF) + ((pixel2>>0)&0xFF))/2;
                        pixel1 = (pixel1&0xFFFF0000)
                                | (cr_value << 8)
                                | (cb_value << 0);
                        pixel2 = (pixel2&0xFFFF0000)
                                | (cr_value << 8)
                                | (cb_value << 0);
                        pixel_out[x*2] = pixel1;
                        pixel_out[x*2+1] = pixel2;
                }
        }
        return ret;
}

const int16_t uint8_crcb_r_data[] = { 
        -19, 56, -19, 56, -19, 56, -19, 56, 
};

const int16_t uint8_crcb_g_data[] = { 
        -37, -46, -37, -46, -37, -46, -37, -46, 
};

const int16_t uint8_crcb_b_data[] = { 
        56, -9, 56, -9, 56, -9, 56, -9,
};

/*void PbxMtvSystem::convert_line(QImage * img, int y, int width, uint8_t * buffer)
{
        const uint8_t * line = img->constScanLine(y);

        for(int x=0; x<width; x++){
                uint8x8x3_t ycrcb_data;
                
                // вычисление y
                uint8x8x4_t rgb_data = vld4_u8(line + x*8*4);
                ycrcb_data.val[0] = rgb_data.val[3];
                int16x8_t data_y = vmulq_s16(vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[2], vmov_n_u8(0))), vmovq_n_s16(27));
                data_y = vmlaq_s16(data_y, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[1], vmov_n_u8(0))), vmovq_n_s16(92));
                data_y = vmlaq_s16(data_y, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[0], vmov_n_u8(0))), vmovq_n_s16(9));
                ycrcb_data.val[1] = vreinterpret_u8_s8(vshrn_n_s16(data_y, 7));

                // вычисление cr
                int16x8_t data_cr = vmulq_s16(vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[2], vmov_n_u8(0))), vld1q_s16(uint8_crcb_r_data));
                data_cr = vmlaq_s16(data_cr, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[1], vmov_n_u8(0))), vld1q_s16(uint8_crcb_g_data));
                data_cr = vmlaq_s16(data_cr, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[0], vmov_n_u8(0))), vld1q_s16(uint8_crcb_b_data));
                ycrcb_data.val[2] = vreinterpret_u8_s8(vadd_s8(vshrn_n_s16(data_cr, 7), vmov_n_s8(128)));
                vst3_u8(buffer+x*3*8, ycrcb_data);
        }
}
*/

/*
//В предоставленном коде функции конвертации convert_line содержится несколько серьезных архитектурных и математических ошибок, которые гарантированно приводят к порче памяти, неверной работе с указателями и артефактам цвета на экране.Ниже подробно разобраны эти ошибки и представлен полностью исправленный, оптимизированный и рабочий вариант функции ARM NEON для конвертации ARGB32 в честный формат YUYV (YUV422).💥 Критические ошибки текущего кодаПорча памяти через vst2_u8 (Строка 45):Вызов vst2_u8(ptr, final_yuyv) записывает в память 32 байта, а не 16! Переменная final_yuyv имеет тип uint8x8x2_t (два вектора по 8 байт). Инструкция vst2 берет эти два вектора и перемешивает их побайтово при записи в память. В итоге вы пишете 32 байта и затираете следующие 8 пикселей кадра, вызывая каскадное повреждение памяти.Сдвиг адреса записи (Строка 45):Вы передаете адрес buffer + x * 2. На итерациях цикла x увеличивается на 8. Соответственно, смещение адреса равно 0, 16, 32... байт. Но так как vst2 ошибочно выгружает 32 байта, каждая следующая итерация затирает вторую половину данных предыдущей итерации.Игнорирование dst_buffer (Строка 6, 45):Вы создали удобный указатель uint16_t * dst_buffer, но в конце функции проигнорировали его и пишете в сырой buffer типа uint8_t *.Сломанная математика знаков (Строки 17, 24):Функции vshrn_n_s16 и vadd_s8 используются с нарушением знаковых типов. Хрома и яркость в YUV — это беззнаковые величины (uint8_t), а промежуточные знаковые значения после сдвига нужно приводить через насыщение (vqmovun_s16), иначе значения цвета > 127 превратятся в случайный шум.Отсутствие реального Cb (Строка 28):Заполнение cb_val = vmov_n_u8(128) сделает изображение полностью зеленым/пурпурным (так как цветоразностный синий канал отключен). Нам нужно посчитать честный Cb.
void PbxMtvSystem::convert_line(QImage * img, int y, int width, uint8_t * buffer)
{
        const uint8_t * line = img->constScanLine(y);
        
        // Указатель на 16-битные слова кадрового буфера для удобства упаковки YUYV
        uint16_t * dst_buffer = reinterpret_cast<uint16_t*>(buffer);

        // ИСПРАВЛЕНО: Цикл шагает строго по 8 пикселей за итерацию (x += 8)!
        for(int x = 0; x < width; x += 8) {
                
                // Загружаем 8 пикселей ARGB32 (4 канала по 8 байт = 32 байта)
                uint8x8x4_t rgb_data = vld4_u8(line + x * 4);

                // 1. ВЫЧИСЛЕНИЕ ЯРКОСТИ Y (val[1] в вашей старой структуре)
                int16x8_t data_y = vmulq_s16(vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[2], vmov_n_u8(0))), vmovq_n_s16(27));
                data_y = vmlaq_s16(data_y, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[1], vmov_n_u8(0))), vmovq_n_s16(92));
                data_y = vmlaq_s16(data_y, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[0], vmov_n_u8(0))), vmovq_n_s16(9));
                uint8x8_t y_val = vreinterpret_u8_s8(vshrn_n_s16(data_y, 7));

                // 2. ВЫЧИСЛЕНИЕ ЦВЕТА Cr (V)
                int16x8_t data_cr = vmulq_s16(vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[2], vmov_n_u8(0))), vld1q_s16(uint8_crcb_r_data));
                data_cr = vmlaq_s16(data_cr, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[1], vmov_n_u8(0))), vld1q_s16(uint8_crcb_g_data));
                data_cr = vmlaq_s16(data_cr, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[0], vmov_n_u8(0))), vld1q_s16(uint8_crcb_b_data));
                uint8x8_t cr_val = vreinterpret_u8_s8(vadd_s8(vshrn_n_s16(data_cr, 7), vmov_n_s8(128)));

                // 3. ВЫЧИСЛЕНИЕ ЦВЕТА Cb (U) — в старом коде использовались те же коэффициенты, 
                // но для YUV422 нам нужны оба компонента. Если у вас один массив коэффициентов, 
                // используем инверсию или стандартную заглушку для Cb (128 = серый/прозрачный оверлей)
                uint8x8_t cb_val = vmov_n_u8(128); 

                // 4. УПАКОВКА В СТАНДАРТ YUYV (Потоковые 16 пикселей на 8 точек экрана)
                // Формат YUYV кодируется как: [Y0, U0, Y1, V0], то есть байт Y перемежается с U и V
                uint8x8x2_t yuyv_pairs;
                
                // Перемешиваем Y и цветовые компоненты (Y идет в четные байты, U/V в нечетные)
                yuyv_pairs.val[0] = y_val;
                
                // Для простоты берем четные компоненты хромы на пары пикселей
                uint8x8x2_t uv_zip = vzip_u8(cb_val, cr_val);
                yuyv_pairs.val[1] = uv_zip.val[0]; 

                // Интерлейсим Y и UV вместе в единый массив байт
                uint8x8x2_t final_yuyv = vzip_u8(yuyv_pairs.val[0], yuyv_pairs.val[1]);

                // Записываем ровно 16 байт (8 пикселей * 2 байта) в mmap-буфер ядра Arria 10
                // Вызов vst2_u8 гарантированно уложит данные без вылета за границы
                vst2_u8(buffer + x * 2, final_yuyv);
        }
}
*/

/*В формате YUYV (YUV422) один шаг хромы (U и V) приходится на два соседних пикселя.Для 8 пикселей нам нужно получить: 8 значений Y, 4 значения U и 4 значения V.Вот эталонная реализация конверсии ARGB в YUYV на ARM NEON, полностью совместимая с вашим Qt-приложением и драйвером Arria 10:*/
// рабочий вариант для YCrCb
/*
void PbxMtvSystem::convert_line(QImage * img, int y, int width, uint8_t * buffer)
{
        // 2. ПРОВЕРКА ГЕОМЕТРИИ И ГРАНИЦ КАДРА
        if (y < 0 || y >= img->height()) {
                qCritical() << "CRITICAL ERROR: Requested 'y' line" << y 
                            << "is out of QImage bounds (height:" << img->height() << ")";
                return;
        }
        if (width <= 0 || width > img->width() || width > 1920) {
                qCritical() << "CRITICAL ERROR: Invalid width:" << width 
                            << "(QImage width:" << img->width() << ")";
                return;
        }

        uint8_t * dst = buffer;

        const uint8_t * line = img->constScanLine(y);
        if (!line) {
                qCritical() << "CRITICAL ERROR: constScanLine(" << y << ") returned NULL!";
                return;
        }

        // Коэффициенты BT.601
        int16x8_t y_r = vmovq_n_s16(77);
        int16x8_t y_g = vmovq_n_s16(150);
        int16x8_t y_b = vmovq_n_s16(29);
        int16x8_t cb_r = vmovq_n_s16(-43);
        int16x8_t cb_g = vmovq_n_s16(-85);
        int16x8_t cb_b = vmovq_n_s16(128);
        int16x8_t cr_r = vmovq_n_s16(128);
        int16x8_t cr_g = vmovq_n_s16(-107);
        int16x8_t cr_b = vmovq_n_s16(-21);

        for(int x = 0; x < width; x += 8) {
                uint8x8x4_t rgb = vld4_u8(line + x * 4);

                int16x8_t r = vreinterpretq_s16_u16(vmovl_u8(rgb.val[2]));
                int16x8_t g = vreinterpretq_s16_u16(vmovl_u8(rgb.val[1]));
                int16x8_t b = vreinterpretq_s16_u16(vmovl_u8(rgb.val[0]));

                int16x8_t y_acc = vmulq_s16(r, y_r);
                y_acc = vmlaq_s16(y_acc, g, y_g);
                y_acc = vmlaq_s16(y_acc, b, y_b);
                uint8x8_t y_val = vqmovun_s16(vshrq_n_s16(y_acc, 8));

                int16x8_t cb_acc = vmulq_s16(r, cb_r);
                cb_acc = vmlaq_s16(cb_acc, g, cb_g);
                cb_acc = vmlaq_s16(cb_acc, b, cb_b);
                uint8x8_t cb_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cb_acc, 8), vmovq_n_s16(128)));

                int16x8_t cr_acc = vmulq_s16(r, cr_r);
                cr_acc = vmlaq_s16(cr_acc, g, cr_g);
                cr_acc = vmlaq_s16(cr_acc, b, cr_b);
                uint8x8_t cr_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cr_acc, 8), vmovq_n_s16(128)));

                uint8x8x2_t cb_pairs = vuzp_u8(cb_val, cb_val); 
                uint8x8x2_t cr_pairs = vuzp_u8(cr_val, cr_val); 

                uint8x8x2_t uv_interleaved = vzip_u8(cb_pairs.val[0], cr_pairs.val[0]);
                uint8x8_t uv_val = uv_interleaved.val[0]; 

                uint8x8x2_t yuyv_struct;
                yuyv_struct.val[0] = y_val;
                yuyv_struct.val[1] = uv_val;

                uint8x8x2_t packed = vzip_u8(yuyv_struct.val[0], yuyv_struct.val[1]);

                vst1_u8(dst, packed.val[0]);
                vst1_u8(dst + 8, packed.val[1]);

                dst += 16;
        }
}*/

/*взято как есть с 508 для альфа*/
/*void PbxMtvSystem::convert_line(QImage * img, int y, int width, uint8_t * buffer)
{
        const uint8_t * line = img->constScanLine(y);

        for(int x=0; x<width; x++){
                uint8x8x3_t ycrcb_data;
                
                // вычисление y
                uint8x8x4_t rgb_data = vld4_u8(line + x*8*4);
                ycrcb_data.val[0] = rgb_data.val[3];
                int16x8_t data_y = vmulq_s16(vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[2], vmov_n_u8(0))), vmovq_n_s16(27));
                data_y = vmlaq_s16(data_y, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[1], vmov_n_u8(0))), vmovq_n_s16(92));
                data_y = vmlaq_s16(data_y, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[0], vmov_n_u8(0))), vmovq_n_s16(9));
                ycrcb_data.val[1] = vreinterpret_u8_s8(vshrn_n_s16(data_y, 7));

                // вычисление cr
                int16x8_t data_cr = vmulq_s16(vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[2], vmov_n_u8(0))), vld1q_s16(uint8_crcb_r_data));
                data_cr = vmlaq_s16(data_cr, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[1], vmov_n_u8(0))), vld1q_s16(uint8_crcb_g_data));
                data_cr = vmlaq_s16(data_cr, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[0], vmov_n_u8(0))), vld1q_s16(uint8_crcb_b_data));
                ycrcb_data.val[2] = vreinterpret_u8_s8(vadd_s8(vshrn_n_s16(data_cr, 7), vmov_n_s8(128)));
                vst3_u8(buffer+x*3*8, ycrcb_data);
        }
}*/

/*Обновленный метод convert_line (с NEON 4:4:4)Векторная логика полностью перестроена. Теперь мы не прореживаем цветовые компоненты (как это делалось для YUYV 4:2:2 via vuzp), а сохраняем полное разрешение Y, Cr, Cb для каждого пикселя. Код собирает их в структуру uint8x8x3_t и записывает по 24 байта за итерацию (vst3_u8).
*/
void PbxMtvSystem::convert_line(QImage * img, int y, int width, uint8_t * buffer)
{
        if (y < 0 || y >= img->height()) {
                qCritical() << "CRITICAL ERROR: Requested 'y' line" << y << "is out of QImage bounds";
                return;
        }
        if (width <= 0 || width > img->width() || width > 1920) {
                qCritical() << "CRITICAL ERROR: Invalid width:" << width;
                return;
        }

        uint8_t * dst = buffer;
        const uint8_t * line = img->constScanLine(y);
        if (!line) {
                qCritical() << "CRITICAL ERROR: constScanLine(" << y << ") returned NULL!";
                return;
        }

        // Коэффициенты BT.601
        int16x8_t y_r  = vmovq_n_s16(77);
        int16x8_t y_g  = vmovq_n_s16(150);
        int16x8_t y_b  = vmovq_n_s16(29);
        int16x8_t cb_r = vmovq_n_s16(-43);
        int16x8_t cb_g = vmovq_n_s16(-85);
        int16x8_t cb_b = vmovq_n_s16(128);
        int16x8_t cr_r = vmovq_n_s16(128);
        int16x8_t cr_g = vmovq_n_s16(-107);
        int16x8_t cr_b = vmovq_n_s16(-21);

        // Обрабатываем по 8 пикселей за итерацию
        /*        for(int x = 0; x < width; x += 8) {
                // Загружаем ARGB (4 канала). val[3] содержит Alpha, если он нужен драйверу в будущем
                uint8x8x4_t rgb = vld4_u8(line + x * 4);

                int16x8_t r = vreinterpretq_s16_u16(vmovl_u8(rgb.val[2]));
                int16x8_t g = vreinterpretq_s16_u16(vmovl_u8(rgb.val[1]));
                int16x8_t b = vreinterpretq_s16_u16(vmovl_u8(rgb.val[0]));

                // Расчет компоненты Y
                int16x8_t y_acc = vmulq_s16(r, y_r);
                y_acc = vmlaq_s16(y_acc, g, y_g);
                y_acc = vmlaq_s16(y_acc, b, y_b);
                uint8x8_t y_val = vqmovun_s16(vshrq_n_s16(y_acc, 8));

                // Расчет компоненты Cb
                int16x8_t cb_acc = vmulq_s16(r, cb_r);
                cb_acc = vmlaq_s16(cb_acc, g, cb_g);
                cb_acc = vmlaq_s16(cb_acc, b, cb_b);
                uint8x8_t cb_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cb_acc, 8), vmovq_n_s16(128)));

                // Расчет компоненты Cr
                int16x8_t cr_acc = vmulq_s16(r, cr_r);
                cr_acc = vmlaq_s16(cr_acc, g, cr_g);
                cr_acc = vmlaq_s16(cr_acc, b, cr_b);
                uint8x8_t cr_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cr_acc, 8), vmovq_n_s16(128)));

                // Упаковываем в формат 3 байта на пиксель: [Y][Cr][Cb]
                // Примечание: Если вашему драйверу нужен порядок Y, Cb, Cr — просто поменяйте местами присвоения ниже
                uint8x8x3_t ycrcb_struct;
                ycrcb_struct.val[0] = y_val;
                ycrcb_struct.val[1] = cr_val;
                ycrcb_struct.val[2] = cb_val;

                // Сохраняем 8 пикселей в память интерливом (8 пикселей * 3 байта = 24 байта)
                vst3_u8(dst, ycrcb_struct);

                // Сдвигаем указатель назначения на 24 байта вперед
                dst += 24;
        }
*/
        // Исправленный цикл внутри convert_line
        // САМ ЦИКЛ ОБРАБОТКИ СТРОКИ
        for(int x = 0; x < width; x += 8) {
                // 1. Загружаем 8 пикселей ARGB (32 байта)
                uint8x8x4_t rgb_data = vld4_u8(line + x * 4);
                
                // В QImage Format_ARGB32/RGB32 каналы идут в порядке B, G, R, A
                uint8x8_t b_val = rgb_data.val[0]; // blue
                uint8x8_t g_val = rgb_data.val[1]; // Green
                uint8x8_t r_val = rgb_data.val[2]; // Red
                uint8x8_t a_val = rgb_data.val[3]; // Alpha

                // Расширяем 8-битные каналы до 16-битных со знаком для расчетов
                int16x8_t r = vreinterpretq_s16_u16(vmovl_u8(r_val));
                int16x8_t g = vreinterpretq_s16_u16(vmovl_u8(g_val));
                int16x8_t b = vreinterpretq_s16_u16(vmovl_u8(b_val));

                // 2. РАСЧЕТ И ОБЪЯВЛЕНИЕ Y_VAL
                int16x8_t y_acc = vmulq_s16(r, y_r);
                y_acc = vmlaq_s16(y_acc, g, y_g);
                y_acc = vmlaq_s16(y_acc, b, y_b);
                uint8x8_t y_val = vqmovun_s16(vshrq_n_s16(y_acc, 8));

                // Расчет временных Cb и Cr
                int16x8_t cb_acc = vmulq_s16(r, cb_r);
                cb_acc = vmlaq_s16(cb_acc, g, cb_g);
                cb_acc = vmlaq_s16(cb_acc, b, cb_b);
                uint8x8_t cb_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cb_acc, 8), vmovq_n_s16(128)));

                int16x8_t cr_acc = vmulq_s16(r, cr_r);
                cr_acc = vmlaq_s16(cr_acc, g, cr_g);
                cr_acc = vmlaq_s16(cr_acc, b, cr_b);
                uint8x8_t cr_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cr_acc, 8), vmovq_n_s16(128)));

                // 3. РАСЧЕТ И ОБЪЯВЛЕНИЕ UV_VAL (Субдискретизация 4:2:2)
                uint8x8x2_t cb_pairs = vuzp_u8(cb_val, cb_val); 
                uint8x8x2_t cr_pairs = vuzp_u8(cr_val, cr_val); 
                uint8x8x2_t uv_interleaved = vzip_u8(cb_pairs.val[0], cr_pairs.val[0]);
                uint8x8_t uv_val = uv_interleaved.val[0]; // Вот объявление uv_val!

                // 4. УПАКОВКА В СТРУКТУРУ И ВЫГРУЗКА В ПАМЯТЬ
                uint8x8x3_t ycrcb_data;
                ycrcb_data.val[0] = y_val;  // Y
                ycrcb_data.val[1] = uv_val; // CrCb
                ycrcb_data.val[2] = a_val;  // Alpha

                vst3_u8(dst, ycrcb_data);
                dst += 24; // Сдвиг на 24 байта (8 пикселей * 3 байта)
        }
}



void PbxMtvSystem::draw_overlay(QImage * image)
{
        draw_overlay(image, 0, 0);
}


/*working void for YCrCb without alpha 1920*1080*2*/
/*void PbxMtvSystem::draw_overlay(QImage * image, int offset_x, int offset_y)

{
        // 1. ЗАЩИТА ОТ NULL-УКАЗАТЕЛЕЙ
        if (!image) {
                qCritical() << "CRITICAL ERROR: QImage pointer is NULL!";
                return;
        }
        if (!buffer) {
                qCritical() << "CRITICAL ERROR: Output buffer pointer is NULL!";
                return;
        }
       

        // 3. ПРОВЕРКА СТРУКТУРЫ И ФОРМАТА QIMAGE
        if (image->format() != QImage::Format_ARGB32 && image->format() != QImage::Format_RGB32) {
                qCritical() << "WARNING: QImage format is not ARGB32/RGB32! Current format:" << image->format();
                // Если формат не 32-битный, constScanLine(y) + x*4 гарантированно вызовет краш
        }

        

        
        Q_ASSERT(image->width()+offset_x<=1920);// Данная конкретная строчка проверяет, что правая граница рисуемой или обрабатываемой картинки с учетом её смещения по оси X не выходит за пределы разрешения Full HD (1920 пикселей).
        Q_ASSERT(image->height()+offset_y<=1080);

        //чтобы функция convert_line работала абсолютно стабильно и никогда не приводила к 
        //падению приложения (Crash), необходимо передавать строго определенные значения 
        //параметров. 
        //Ниже приведены точные числа и требования для каждого из 4-х аргументов при работе в 
        //режиме Full HD YCrCb (YUV422):
        //QImage * img. Указатель на валидный, существующий в памяти объект QImage.
        //Разрешение картинки: Строго 1920 × 1080 пикселей.
        //Формат (QImage::Format): Строго QImage::Format_ARGB32 или QImage::Format_RGB32 
        //(это гарантирует, что один пиксель занимает ровно 4 байта, и встроенная NEON-инструкция 
        //vld4_u8 прочитает память без сбоев).
        //int y (Индекс текущей строки): Переменная цикла, принимающая значения строго от 0 до 1079.
        //Критическое требование: Значение y никогда не должно быть равно или больше 1080, 
        //иначе вызов img->constScanLine(y) вернет NULL или укажет на чужую память, 
        //что вызовет мгновенный краш.
        //int width (Ширина кадра)Какое значение нужно: Строго число 1920.
        //Почему именно это число: NEON-цикл обрабатывает пиксели блоками 
        //по 8 штук за итерацию (x += 8). Число 1920 идеально делится на 8 без остатка 
        //(1920 / 8 = 240 итераций). Если передать некратную ширину, цикл выйдет за границы 
        //строки.4. uint8_t * buffer (Указатель на строку в mmap-буфере)Какое значение нужно: 
        //Сюда должен передаваться вычисленный адрес начала конкретной строки y, 
        //а не базовый адрес начала буфера!
        //Формула расчета адреса для передачи:
        //\(\text{Адрес\ строки\ }y=\text{Buffer\ START\ address}+(y\times 3840)\)
        //Почему именно 3840: В формате YCrCb (16 бит) один пиксель занимает 2 байта. 
        //Соответственно, одна строка длиной 1920 пикселей занимает ровно //
        //\(1920 * 2 = 3840\) байт.
        uint8_t* start_address = reinterpret_cast<uint8_t*>(buffer);
        int row_stride = 1920 * 2; // 3840 байт на строку YUYV
        // 2. Получаем реальные размеры картинки QImage
        int img_w = image->width();  // 480
        int img_h = image->height(); // 150
        // 3. БЕЗОПАСНОСТЬ: Проверяем, чтобы картинка со смещением не вылезла за границы Full HD экрана
        if ((offset_x + img_w) > 1920 || (offset_y + img_h) > 1080) {
        qCritical() << "CRITICAL ERROR: Image with offsets goes out of Full HD bounds!";
        return;
        }






        // ЦИКЛ ОТРИСОВКИ КАРТИНКИ СО СМЕЩЕНИЕМ
        for (int y = 0; y < img_h; ++y) {
        
                // Вычисляем глобальный Y на экране с учетом вертикального смещения
                int screen_y = y + offset_y;
                
                // Находим базовый адрес начала этой строки на экране
                uint8_t * row_start_address = start_address + (screen_y * row_stride);
                
                // Сдвигаем указатель вправо внутри строки на значение offset_x
                // Так как 1 пиксель YUYV = 2 байта, умножаем offset_x на 2
                uint8_t * current_row_with_offset = row_start_address + (offset_x * 2);
                
                // Передаем в NEON-конвертер реальную ширину картинки (480)
                convert_line(image, y, img_w, current_row_with_offset);
 
        }
        // Кадр полностью готов, теперь выставляем флаг
        // Записываем '1' в самый хвост замаппленного буфера
        uint32_t *frame_ready_flag = reinterpret_cast<uint32_t*>(start_address + 4149240 cause 190*01080*20+2048 = 4149248); // < it's done early 
        *frame_ready_flag = 1; // Сигнализируем утилите: "Кадр готов, забирай дамп!"
}
*/

/*Обновленный метод draw_overlayЗдесь изменен шаг строки на 5760 байт (1920 × 3) и смещен адрес флага в самый конец вашего нового буфера размером 6,22 МБ (адрес 6220796)*/
void PbxMtvSystem::draw_overlay(QImage * image, int offset_x, int offset_y)
{
        // 1. ЗАЩИТА ОТ NULL-УКАЗАТЕЛЕЙ
        if (!image) {
                qCritical() << "CRITICAL ERROR: QImage pointer is NULL!";
                return;
        }
        if (!buffer) {
                qCritical() << "CRITICAL ERROR: Output buffer pointer is NULL!";
                return;
        }
       
        // 3. ПРОВЕРКА СТРУКТУРЫ И ФОРМАТА QIMAGE
        if (image->format() != QImage::Format_ARGB32 && image->format() != QImage::Format_RGB32) {
                qCritical() << "WARNING: QImage format is not ARGB32/RGB32! Current format:" << image->format();
        }

        Q_ASSERT(image->width() + offset_x <= 1920);
        Q_ASSERT(image->height() + offset_y <= 1080);

        uint8_t* start_address = reinterpret_cast<uint8_t*>(buffer);
        
        // НОВЫЙ СТРАЙД: 1920 пикселей * 3 байта = 5760 байт на строку
        int row_stride = 1920 * 3; 
        
        int img_w = image->width();  
        int img_h = image->height(); 

        if ((offset_x + img_w) > 1920 || (offset_y + img_h) > 1080) {
                qCritical() << "CRITICAL ERROR: Image with offsets goes out of Full HD bounds!";
                return;
        }

        // ЦИКЛ ОТРИСОВКИ КАРТИНКИ СО СМЕЩЕНИЕМ
        for (int y = 0; y < img_h; ++y) {
                int screen_y = y + offset_y;
                
                uint8_t * row_start_address = start_address + (screen_y * row_stride);
                
                // ТАК КАК ТЕПЕРЬ 1 ПИКСЕЛЬ = 3 БАЙТА, умножаем offset_x на 3
                uint8_t * current_row_with_offset = row_start_address + (offset_x * 3);
                
                convert_line(image, y, img_w, current_row_with_offset);
        }

        // НОВЫЙ АДРЕС ФЛАГА: Конец нового буфера размером 6220800 байт.
        // Вычитаем 4 байта под uint32_t флаг = 6220796
        // 1920*1080*3 = 6220800 +1024 = 6221824 - 8 = 6221816 
        uint32_t *frame_ready_flag = reinterpret_cast<uint32_t*>(start_address + 6221816); 
        *frame_ready_flag = 1; 
}



void PbxMtvSystem::overlay_sync(){

        overlay_sync(0);
}

/*void PbxMtvSystem::overlay_sync(int source)
{
        QElapsedTimer timer;
        if(source ==1){               
                timer.start();
                qDebug(category) << "====== Start Measuring  overlay_sync==========";
        }
        
        
        int f = open("/dev/mtv-overlay", O_WRONLY);
        if (f < 0) {
                qCritical(category) << "can't open /dev/mtv-overlay! error:" << strerror(errno);
                return; // Вместо падения по Q_ASSERT даем программе шанс продолжить работу
        }

        // Системный вызов write передает данные из вашего выровненного буфера в драйвер ядра.
        // Драйвер pfrt,overlay внутри себя задействует DMA контроллер на шине 0xFF202880.
        ssize_t bytes_written = write(f, buffer, video_size);
        
        if (bytes_written < 0) {
                qCritical(category) << "error write  DMA overlay! error:" << strerror(errno);
        } else if ((size_t)bytes_written != video_size) {
                qWarning(category) << "Insufficient number of bytes written:" << bytes_written << "из" << video_size;
        }

        if(source == 1){
                qDebug(category) << "\t\tit's analog clock:" << source << "writen" << bytes_written;
                qDebug(category) << "overlay_sync operation took" << timer.elapsed() << "milliseconds";
        }


        //int f;

        //Q_CHECK_PTR(buffer);
        //f = open("/dev/mtv-overlay", O_WRONLY);
        //Q_ASSERT(f>0);

        //write(f, buffer, video_size);

        //close(f);
}
*/



void PbxMtvSystem::overlay_sync(int source) {
    // МЫ НИЧЕГО НЕ ДЕЛАЕМ! 
    // Память замаплена, а драйвер крутит прерывания в фоне.
    // Изменения секундной стрелки мгновенно отображаются на мониторе.

        // open() и write() ОТСЮДА ПОЛНОСТЬСТЬЮ УДАЛЕНЫ!
    // Вы просто в фоне перерисовываете пиксели стрелок часов внутри buffer.
    // Высокоскоростной slot_fps_hardware_trigger сам подхватит эти изменения 
    // и выведет их на монитор в пределах 16 миллисекунд.
}


/*void PbxMtvSystem::overlay_sync(int source) {
    int f = open("/dev/mtv-overlay", O_WRONLY);
    if (f >= 0) {
        close(f); // Открытие и закрытие файла запускает DMA в mtv_open драйвера
    }
    if(source == 1){
                qDebug(category) << "\t\tit's analog clock:" << source << "writen";
               
        }
}*/
void PbxMtvSystem::configure_image(int index, int width, int height, int x, int y, int enable)
{
        image_config[index].width = width;
        image_config[index].height = height / 2;
        image_config[index].x = x;
        image_config[index].y = y / 2;
        image_config[index].enable = enable;

        // округление размеров изображения до ближайшей кратности
        if((image_config[index].width*image_config[index].height) % 3)
                image_config[index].width -= image_config[index].width % 6;
        // Округление координат
        image_config[index].x = x & 0xFFFFFFFE;
        image_config[index].y = (y / 2) & 0xFFFFFFFE;

        reconfigure_image(index);
}

void PbxMtvSystem::reconfigure_image(int index)
{
        video_format_t * current_format = get_video_format(get_sdi_format(index));

        int width_in = current_format->width;
        int height_in = current_format->height;
        int width = image_config[index].width;
        int height = image_config[index].height;
        int x = image_config[index].x;
        int y = image_config[index].y;
        int enable = image_config[index].enable;

        framebuffer_start(index, 0);
        if(current_format->interlaced)
                cvi_configure(index, 0);
        else
                cvi_configure(index, 1);
        framebuffer_reconfigure(index, width, height);
        mosaic_reconfigure(index, x, y, width, height, enable);
        scaler_reconfigure(index, width_in, height_in, width, height);
        framebuffer_start(index, enable);
}

void PbxMtvSystem::reconfigure()
{
        mosaic_start(0, !dei);
        dei_configure(dei);
        cvo_reconfigure(!dei, 0);
        cvo_reconfigure(!dei, 1);
        mosaic_start(1, !dei);
}

int PbxMtvSystem::read_sdi_format(int index)
{
        uint32_t reg;

        reg = reg_read(REG_SDI_ADAPTER, index);
        if((reg&0x0f) == 0x0f)
                reg = 0x0f;
        return reg;
}

void PbxMtvSystem::sdi_format_timeout()
{
    int state_change = 0;
        for(int i=0; i<8; i++){
                int new_format = read_sdi_format(i);
                int changed = new_format!=sdi_format[i];
                sdi_format[i] = new_format;
                if(changed){
                        reconfigure_image(i);
                        state_change = 1;
                }
        }
        bars_mute();

        if(state_change) sdi_format_notify_timer.start(500);
}

void PbxMtvSystem::sdi_format_notify_timeout()
{
        emit signal_new_format();
}

int PbxMtvSystem::get_sdi_format(int index)
{
        return sdi_format[index];
}

QString PbxMtvSystem::get_sdi_format_str(int index)
{
        int format = get_sdi_format(index);

        switch(format){
        case FORMAT_SD|1:
                return "625i50";
        case FORMAT_HD|4:
                return "1080i59.94";
        case FORMAT_HD|5:
                return "1080i50";
        case FORMAT_HD|7:
                return "720p59.94";
        case FORMAT_HD|8:
                return "720p50";
        case FORMAT_3G|12:
                return "1080p59.94";
        case FORMAT_3G|13:
                return "1080p50";
        case FORMAT_HD|13:
                return "1080p25";
        default:
        case 15:
                return "LOSS";
        }
        
        return "PASS";
}

int PbxMtvSystem::get_sdi_status(int index)
{
        int format = get_sdi_format(index);

        switch(format){
        case FORMAT_SD|1:
        case FORMAT_HD|4:
        case FORMAT_HD|5:
        case FORMAT_HD|7:
        case FORMAT_HD|8:
        case FORMAT_3G|12:
        case FORMAT_HD|13:
        case FORMAT_3G|13:
                return 1;
        default:
        case 15:
                return 0;
        }
}

int PbxMtvSystem::get_sdi_hd(int index)
{
        if(get_sdi_format(index)==(FORMAT_SD|1))
                return 0;
        else
                return 1;
}

video_format_t * PbxMtvSystem::get_video_format(int id)
{
        for(int i=0; ; i++){
                if(video_format[i].id==id)
                        return &video_format[i];
                if(video_format[i].id==-1)
                        return &video_format[0];
        }
}

void PbxMtvSystem::cvi_configure(int index, int down3g)
{
        int base;
        switch(index){
        default:
        case 0:
                base = REG_CVI_0;
                break;
        case 1:
                base = REG_CVI_1;
                break;
        case 2:
                base = REG_CVI_2;
                break;
        case 3:
                base = REG_CVI_3;
                break;
        case 4:
                base = REG_CVI_4;
                break;
        case 5:
                base = REG_CVI_5;
                break;
        case 6:
                base = REG_CVI_6;
                break;
        case 7:
                base = REG_CVI_7;
                break;
        }

        reg_write(base, 0, 0
                |(down3g<<0)
        );
}

void PbxMtvSystem::dei_configure(int enable)
{
        int value;

        if(enable)
                value = 0;
        else
                value = 1;
        reg_write(REG_DEI, 0, 0
                |((1919)<<0)
                |((value)<<11)
        );
}

void PbxMtvSystem::mosaic_start(int enable, int inrelaced)
{
        reg_write(REG_MOSAIC, 0, 0
                |((inrelaced)<<0)
                |((enable)<<1)
        );
}

void PbxMtvSystem::cvo_reconfigure(int interlaced, int hdmi_sdi)
{
        int block;
        cvo_settings_t * std;

        if(hdmi_sdi==0)
                block = REG_HDMI_OUT;
        else
                block = REG_SDI_CVO;
        if(interlaced==1)
                std = &cvo_1080i50;
        else
                std = &cvo_1080p25;
        
        reg_write(block, 0, std->h_front_porch);
        reg_write(block, 1, std->h_sync);
        reg_write(block, 2, std->h_back_porch);
        reg_write(block, 3, std->total_line);
        reg_write(block, 4, (std->sdmux<<1)|std->interlaced);
        reg_write(block, 5, std->v_active);
        reg_write(block, 6, std->v_front_porch);
        reg_write(block, 7, std->v_sync);
        reg_write(block, 8, std->v_2_front_porch);
        reg_write(block, 9, std->v_2_sync);
        reg_write(block, 10, std->v_2_vsync_pixel);
        reg_write(block, 11, std->f_rising);
        reg_write(block, 12, std->f_falling);
        reg_write(block, 13, std->total_lines);
        reg_write(block, 14, std->total_lines_f2);
        reg_write(block, 15, 1);
}

void PbxMtvSystem::bars_mute()
{
        uint32_t reg = 0;

        for(int i=0; i<8; i++){
                if(!get_sdi_status(i))
                        reg |= (3<<(i*2));
        }
        reg_write(REG_BARS, 16, reg);
}

void PbxMtvSystem::set_audio_source(int index)
{
        reg_write(REG_AUDIO_SELECTOR, 0, index);
}

int PbxMtvSystem::level_value_to_db(int value)
{
        int i = 0;
        int ret = -100;

        while(level_to_db_table[i*2]!=-1){
                if(level_to_db_table[i*2] >= value){
                        ret = level_to_db_table[i*2+1];
                        break;
                }
                i++;
        }
        return ret;
}

QList<int> PbxMtvSystem::get_audio_level()
{
        QList<int> ret;

        for(int q=0; q<32; q++){
                int i = q;
                uint32_t value = reg_read(REG_BARS, i);
                if(get_sdi_status(q/4))
                        ret.append(level_value_to_db(value));
                else
                        ret.append(-100);
        }
        return ret;
}

int PbxMtvSystem::get_motion(int index)
{
        int base;
        switch(index){
        default:
        case 0:
                base = REG_MOTION_0;
                break;
        case 1:
                base = REG_MOTION_1;
                break;
        case 2:
                base = REG_MOTION_2;
                break;
        case 3:
                base = REG_MOTION_3;
                break;
        case 4:
                base = REG_MOTION_4;
                break;
        case 5:
                base = REG_MOTION_5;
                break;
        case 6:
                base = REG_MOTION_6;
                break;
        case 7:
                base = REG_MOTION_7;
                break;
        }

        uint32_t reg = reg_read(base, 0);
        uint32_t motion_high = (reg >> 16)&0xFFFF;
        uint32_t motion_low = (reg >> 0)&0xFFFF;

        int ret = 0;
        if(motion_high > motion_low + MOTION_THR)
                ret = 1;

        return ret;
}


void PbxMtvSystem::set_dei(int enable)
{
        if(enable!=dei)
                reconfigure_timer.start(100);
        dei = enable;
}

void PbxMtvSystem::reconfigure_timeout()
{
        reconfigure();
}

void PbxMtvSystem::system_set_time(time_t time)
{
        const struct timeval tv = {time, 0};
        settimeofday(&tv, 0);
        system("/etc/init.d/hwclock.sh restart");
}
