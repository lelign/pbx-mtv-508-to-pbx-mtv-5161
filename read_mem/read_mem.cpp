#include <QDebug>
#include <QLoggingCategory>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <arm_neon.h>
#include "read_mem.h"
#include "str-mem-dev.h"
#include "scaler_coeff.h"
#include <QFile>
#define BUF_SIZE (16*1024)
#include <QList>

static QLoggingCategory category("\033[35m mtv-system\033[0m");

const char * fname = "/dev/str-mem";
const int video_size = 1920*1080*3;
#define MOTION_THR (100)
#define ANCIN ("/dev/tsin1")

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
        REG_BARS, // audio meters
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


QList<QString> reglist = {
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
        "REG_BARS", // audio meters
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
        "REG_SDI_CVO"};



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

ReadMem::ReadMem()
{
    buffer = (char*) malloc(video_size); // Выделить  6220800 байт памяти = 1920*1080*3 = 6220800 
        /*dei = 0; // dei <= deinterlasing 
        reconfigure_timer.setSingleShot(true);
        connect(&reconfigure_timer, &QTimer::timeout, this, &ReadMem::reconfigure_timeout);
        // reconfigure_timer will be start at 100ms via set_dei()
        
        connect(&sdi_format_timer, &QTimer::timeout, this, &ReadMem::sdi_format_timeout);
        sdi_format_timer.start(1000); // 100  <= check inputs status every 100msec
        memset(&image_config, 0, sizeof(image_config));
        sdi_format_notify_timer.setSingleShot(true);
        connect(&sdi_format_notify_timer, &QTimer::timeout, this, &ReadMem::sdi_format_notify_timeout);
        // sdi_format_notify_timer will be start at 500msec if inputs status changes
        set_audio_source(0);
        reconfigure(); /* REG_MOSAIC, REG_DEI, REG_HDMI_OUT, REG_SDI_CVO, REG_MOSAIC*/
        //anc_reader = new AncReader(ANCIN, this);
        //anc_reader->start(); // start as thread ts_reader
        //ignvgpiserver = new IgnVgpiServer();
        //ignvgpiserver->start();*/
}

ReadMem::~ReadMem()
{
        free(buffer);
        //anc_reader->stop();
        //delete anc_reader;
}

void ReadMem::framebuffer_start(int index, int value)
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

void ReadMem::framebuffer_reconfigure(int index, int width, int height)
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

void ReadMem::mosaic_reconfigure(int index, int x, int y, int width, int height, int enable)
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

void ReadMem::scaler_scaler_config(int index, int bypass, int width, int height, 
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

void ReadMem::scaler_coeff(int index, uint32_t * coeff)
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

void ReadMem::scaler_reconfigure(int index, int width_in, int height_in, int width_out, int height_out)
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

void ReadMem::reg_write(uint32_t block, uint32_t addr, uint32_t data)
{
        strmem_reg_data reg_data;
        int ret;
        int fd;

        reg_data.block = block;
        reg_data.address = addr*4;
        reg_data.data = data;
        reg_data.rw = STR_REG_WRITE;
        //qDebug(category) << "\t\t reg_write fname" << fname << "block" << block << "addr"<< addr << "data" << data;
        //qDebug(category) << "\t\t reg_write fname" << fname;
        /*        uint8_t buf[BUF_SIZE];
        int f_ts;
        //int thread_exit;
        f_ts = open(fname, O_RDONLY);
        
        ret = read(f_ts, buf, BUF_SIZE);
        //push_data(buf, ret);
        
        close(f_ts);
        qDebug(category) << "HERE READ ret" << ret;*/



        fd = open(fname, O_RDONLY); // fd file
        if(fd<0){
                return;
        }
        ret = ioctl(fd, STRMEM_IOCTL_REG, &reg_data);
	if(ret<0){
	        printf("ioctl error\n");
	}
        close(fd);
        //if (some_changed){
        //QList <int> formatlist = {20, 21, 23, 24, 29, 60, 61};
        //if(formatlist.contains(get_sdi_format(index))){
        if(reglist[block].contains("REG_MOSAIC")){
               /* qDebug(category) << "\t\treg_write" 
                                << reglist[block] << addr << data
                                << QString("%1").arg(QString::number(data, 2), 32, QChar('0'));*/
                QString row = QString("%1 | %2 | %3 | %4 | %5")
                        .arg("reg_write", -5)       // Left-aligned, 5 chars
                        .arg(reglist[block], -18)    // Left-aligned, 15 chars
                        .arg(addr, 3)   // Right-aligned, 10 chars
                        .arg(data, 10)
                        .arg(QString("%1").arg(QString::number(data, 2), 32, QChar('0'))); 
                qDebug(category).noquote() << row;
                //qDebug(category) << "here" << QString("%1").arg(data), -20, QChar('0');
                
                /*if(reglist[block].contains("REG_BARS")){
                        qDebug(category)<< "here check" << reg_read(block, addr);

                }*/
        };
        

}

void ReadMem::bars_configure(int index, int x, int x2, int y, int scale, int enable_1, int enable_2)
{
        if(dei){
                y = y*2;
                scale = (scale + 1) * 2 - 1;
        }

        /*        qDebug(category) << "\t\t bars_configure reg_write  " << index << x << x2 << y << scale << enable_1 << enable_2;
        qDebug(category) << "\t\t bars_configure reg_write 1" <<  QString::number(index*2+0, 2) << "\t\t"<< QString::number (0
                |(x<<0)
                |((y/2)<<11)
                |((scale&0x0f)<<22)
                |(enable_1<<26), 2);*/

        reg_write(REG_BARS, index*2+0, 0
                |(x<<0)
                |((y/2)<<11)
                |((scale&0x0f)<<22)
                |(enable_1<<26)
        );
        /*        qDebug(category) << "\t\t bars_configure reg_write 2" << QString::number(index*2+1, 2)  << "\t\t"<< QString::number (0
                |(x2<<0)
                |((y/2)<<11)
                |((scale&0x0f)<<22)
                |(enable_2<<26), 2);*/


        reg_write(REG_BARS, index*2+1, 0
                |(x2<<0)
                |((y/2)<<11)
                |((scale&0x0f)<<22)
                |(enable_2<<26)
        );
        /*
                scale = 5;
        enable_2 = 0;
        qDebug(category) << "\t\t enable_2 = 0 scale = 5" << QString::number(index*2+1, 2)  << "\t\t"<< QString::number (0
                |(x2<<0)
                |((y/2)<<11)
                |((scale&0x0f)<<22)
                |(enable_2<<26), 2);
                */

}


uint32_t ReadMem::reg_read(uint32_t block, uint32_t addr)
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
        /*ign*/
        /*
                QFile file("/mnt/ramdisk/read.txt");
        if (file.open(QIODevice::WriteOnly)) {
        QByteArray data = ret;
        file.write(data);
        file.close();
        }

        QFile file("binary.dat");
        if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);
        out << QString("User Name") << (qint32)42; // Serializes data
        file.close();
        }
        */

        return reg_data.data;
}

QString ReadMem::get_build_id()
{
        uint32_t reg;
        if(!reg_mem){
                reg_mem = reg;
        }
        reg = reg_read(REG_BUILDID, 0);
        /*
                if(reg_mem != reg ){
                qDebug(category) << "get_build_id()" << reg << QString("%1%2%3").arg((reg>>16)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>8)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>0)&0xFF, 2, 10, QLatin1Char('0'));
                reg_mem = reg;
        }
        */

        /*
        Этот фрагмент кода преобразует 24-битное целое число (например, значение цвета RGB) в отформатированную строку, гарантируя, 
        что каждый 8-битный сегмент будет иметь длину не менее 2 цифр и будет дополнен ведущим нулем при необходимости
        QString return_string;
        return_string = QString("%1%2%3").arg((reg>>16)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>8)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>0)&0xFF, 2, 10, QLatin1Char('0'));
        qDebug(category) << "get_build_id()" << return_string;*/
        
        return QString("%1%2%3").arg((reg>>16)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>8)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>0)&0xFF, 2, 10, QLatin1Char('0'));
}

int ReadMem::limit_color(int value)
{
        if(value > 255)
                value = 255;
        if(value < 0)
                value = 0;
        return value;
}

QRgb ReadMem::rgb_to_ycrcb(QRgb value)
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

QImage * ReadMem::image_to_prpb(QImage * image)
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

void ReadMem::convert_line(QImage * img, int y, int width, uint8_t * buffer)
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

void ReadMem::draw_overlay(QImage * image)
{
        draw_overlay(image, 0, 0);
}

void ReadMem::draw_overlay(QImage * image, int offset_x, int offset_y)
{
        Q_ASSERT(image->width()+offset_x<=1920);
        Q_ASSERT(image->height()+offset_y<=1080);

        offset_x -= offset_x % 2;
        offset_y -= offset_y % 2;

        if(!dei){
                const int height = image->height()/2;
                int width = image->width()/8;
                for(int y=0; y<height; y++){
                        unsigned int buffer_offset = (offset_y/2+y+0)*1920 + offset_x;
                        convert_line(image, y*2+0, width, (uint8_t*)buffer+(buffer_offset)*3);
                }
                for(int y=0; y<height; y++){
                        unsigned int buffer_offset = (offset_y/2+y+540)*1920 + offset_x;
                        convert_line(image, y*2+1, width, (uint8_t*)buffer+(buffer_offset)*3);
                }
        } else {
                const int height = image->height();
                int width = image->width()/8;
                for(int y=0; y<height; y++){
                        unsigned int buffer_offset = (offset_y+y+0)*1920 + offset_x;
                        convert_line(image, y, width, (uint8_t*)buffer+(buffer_offset)*3);
                }
        }
}

void ReadMem::overlay_sync()
{
        int f;

        Q_CHECK_PTR(buffer);
        f = open("/dev/mtv-overlay", O_WRONLY);
        Q_ASSERT(f>0);

        write(f, buffer, video_size);

        close(f);
}

void ReadMem::configure_image(int index, int width, int height, int x, int y, int enable)
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

void ReadMem::reconfigure_image(int index)
{
               
        video_format_t * current_format = get_video_format(get_sdi_format(index));
        QList <int>formatlist;
        formatlist = {20, 21, 23, 24, 29, 60, 61};
        if(formatlist.contains(get_sdi_format(index))){
                qDebug(category) << "\n\t\treconfigure_image("<< index <<")" 
                << "get_sdi_format("<< index << "):" 
                << get_sdi_format(index) 
                << "get_video_format" 
                <<  current_format << "\n";
        }
        
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

void ReadMem::reconfigure()
{
        qDebug(category) << "\n\t\treconfigure()\n";
        mosaic_start(0, !dei);
        dei_configure(dei);
        cvo_reconfigure(!dei, 0);
        cvo_reconfigure(!dei, 1);
        mosaic_start(1, !dei);
}

int ReadMem::read_sdi_format(int index)
{
        uint32_t reg;

        reg = reg_read(REG_SDI_ADAPTER, index);
        //qDebug(category) << "read_sdi_format() reg:" << reg << "index" << index;
        if((reg&0x0f) == 0x0f)
                reg = 0x0f;
        return reg;
}

void ReadMem::sdi_format_timeout()
{

    //qDebug(category) << "sdi_format_timeout () timer 1000";
    int state_change = 0;
        for(int i=0; i<8; i++){
                int new_format = read_sdi_format(i);
                int changed = new_format!=sdi_format[i];
                sdi_format[i] = new_format;
                some_changed = false;
                if(changed){
                        some_changed = true;
                        qDebug(category) << "\n\t\tsdi_format_timeout() some_changed" << some_changed << "reconfigure_image{"<< i <<")";
                        reconfigure_image(i);
                        state_change = 1;
                }
        }
        bars_mute();

        if(state_change) 
                {
                qDebug(category) << "\t\t sdi_format_timeout() state_change" << state_change << "sdi_format_notify_timer.start(500);";                
                sdi_format_notify_timer.start(500); // it's here
                }
}

void ReadMem::sdi_format_notify_timeout()
{
        qDebug(category) << "\t\t sdi_format_notify_timeout emit signal_new_format()";
        emit signal_new_format();
}

int ReadMem::get_sdi_format(int index){
        
        return sdi_format[index];
}

QString ReadMem::get_sdi_format_str(int index)
{
        int format = get_sdi_format(index);

        switch(format){
        case FORMAT_SD|1:
                return "625i50";
        case FORMAT_HD|4: //20
                return "1080i59.94";
        case FORMAT_HD|5: //21
                return "1080i50";
        case FORMAT_HD|7: //23
                return "720p59.94";
        case FORMAT_HD|8: //24
                return "720p50";
        case FORMAT_3G|12: //60
                return "1080p59.94";
        case FORMAT_3G|13: //61
                return "1080p50";
        case FORMAT_HD|13: //29
                return "1080p25";
        default:
        case 15:
                return "LOSS";
        }

        return "";
}

int ReadMem::get_sdi_status(int index)
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

int ReadMem::get_sdi_hd(int index)
{
        if(get_sdi_format(index)==(FORMAT_SD|1))
                return 0;
        else
                return 1;
}

video_format_t * ReadMem::get_video_format(int id)
{
        for(int i=0; ; i++){
                if(video_format[i].id==id)
                        return &video_format[i];
                if(video_format[i].id==-1)
                        return &video_format[0];
        }
}

void ReadMem::cvi_configure(int index, int down3g)
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

void ReadMem::dei_configure(int enable)
{
        int value;

        if(enable)
                value = 0;
        else
                value = 1;
        uint32_t i_reg = 0; 
        i_reg = 0
                |((1919)<<0)
                |((value)<<11);
        qDebug(category) << "\n\t\t\tdei_configure(" << enable <<")\n";
        reg_write(REG_DEI, 0, 0
                |((1919)<<0)
                |((value)<<11)
        );
}

void ReadMem::mosaic_start(int enable, int inrelaced)
{
        uint32_t i_reg = 0;
        i_reg = 0
                |((inrelaced)<<0)
                |((enable)<<1);
        qDebug(category) << "\n\t\t\tmosaic_start(" << enable << inrelaced <<")\n" ;
        reg_write(REG_MOSAIC, 0, 0
                |((inrelaced)<<0)
                |((enable)<<1)
        );
}

void ReadMem::cvo_reconfigure(int interlaced, int hdmi_sdi)
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
        
        qDebug(category) << "\n\t\t\tcvo_reconfigure(" << interlaced <<  hdmi_sdi << ")\n";
        
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

void ReadMem::bars_mute()
{
        uint32_t reg = 0;

        for(int i=0; i<8; i++){
                // i = 0 => ...0011
                // i = 1 => ...1100
                // i = 2 => ...00110000
                if(!get_sdi_status(i))
                        reg |= (3<<(i*2)); // установки определенных битов флага или регистра 1 без влияния на другие биты
        }
        //qDebug(category) << "\tbars_mute()";
        //reg = 0; // подавление градусника
        reg_write(REG_BARS, 16, reg);
}

void ReadMem::set_audio_source(int index)
{
        uint32_t i_reg = 0;
        i_reg = index; 
        qDebug(category) << "\n\t\tset_audio_source("<< index << ")\n";
        reg_write(REG_AUDIO_SELECTOR, 0, index);
}

int ReadMem::level_value_to_db(int value)
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

QList<int> ReadMem::get_audio_level()
{
        QList<int> ret;
        //qDebug(category) << "get_audio_level"; 

        for(int q=0; q<32; q++){
                int i = q;
                uint32_t value = reg_read(REG_BARS, i);

                /*if(value != 0){
                        qDebug(category) << "get_audio_level()" << value;
                }*/
                
                if(get_sdi_status(q/4))
                        ret.append(level_value_to_db(value));
                else
                        ret.append(-100);
        }
        return ret;
}

int ReadMem::get_motion(int index)
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
        qDebug(category) << "get_motion()" << reg;
        uint32_t motion_high = (reg >> 16)&0xFFFF;
        uint32_t motion_low = (reg >> 0)&0xFFFF;

        int ret = 0;
        if(motion_high > motion_low + MOTION_THR)
                ret = 1;

        return ret;
}


void ReadMem::set_dei(int enable)
{
        //qDebug(category) << "\n\t\t\t set_dei enable(" << enable << ")\n";
        if(enable!=dei)
                reconfigure_timer.start(100);
        dei = enable;
}

void ReadMem::reconfigure_timeout()
{
        qDebug(category) << "reconfigure_timeout()";
        reconfigure();
}

void ReadMem::system_set_time(time_t time)
{
        const struct timeval tv = {time, 0};
        settimeofday(&tv, 0);
        system("/etc/init.d/hwclock.sh restart");
}

int main() {
    try {

        static QLoggingCategory category("\033[35m read_mem\033[0m");
        
//static constexpr size_t REG_SPAN = 0x80;
        /*        QTextStream qin(stdin);
        QTextStream qout(stdout);
        qout << "Enter REG: ";
        qout.flush();
        QString line = qin.readLine();
        bool ok = false;
        
        if(reglist.contains(line)){
            ok = true;
            qDebug(category) << "reglist.contains" << line;
        }*/
        
        /*        
        uintptr_t REG_BASE; // = 0xFF202780;
        REG_BASE = static_cast<uintptr_t>(line.toULongLong(&ok, 0)); // >> REG_BASE;

        if (ok) {
            qDebug(category) << "line" << line << REG_BASE;
        // Success: 'address' contains the value
        } else {
        // Handling error: Conversion failed
        }*/
        
        // here stop
        ReadMem read_mem;
        uint32_t base;
        /*
        REG_MOSAIC
        index << x << y << width << height << enable
        0 640 180 640 180 1 
        1 0 0 640 180 0 
        2 0 360 640 180 1 
        3 0 0 640 180 0 
        4 0 0 640 180 0 
        5 0 0 640 180 0 
        6 0 0 640 180 0 
        7 1280 360 640 180 1 
        */

        /*
        ok with mosaic
        base = REG_MOSAIC;
        int index = 0;
        int x = 320; // 640
        int y = 180;
        int width = 640;
        int height = 180;
        int enable = 1;


        read_mem.reg_write(base, 16+index*2+0, 
                ((width+x) << 0)
                |((height+y) << 11)
        );
        read_mem.reg_write(base, 16+index*2+1, 
                ((x) << 0)
                |((y) << 11)
                |((enable)<<22)
        );
        */
        
        
        /*        // REG_BARS OK
        // bars_configure 0 647 1248 387 4 1 1
        //              int index, int x, int x2, int y, int scale, int enable_1, int enable_2

        int index = 0;
        int x = 647; // 640
        int x2 = 1248;
        int y = 387;
        int scale = 4;
        int enable_1 = 1;
        int enable_2 = 1;
        read_mem.reg_write(REG_BARS, index*2+0, 0
                |(x<<0)
                |((y/2)<<11)
                |((scale&0x0f)<<22)
                |(enable_1<<26)
        );

        read_mem.reg_write(REG_BARS, index*2+1, 0
                |(x2<<0)
                |((y/2)<<11)
                |((scale&0x0f)<<22)
                |(enable_2<<26)
        );
        */


        // REG_SCALER_0
        // int index, int bypass, int width, int height, 
        // int out_width, int out_height, int deinterlace, int unsharp_bypass, int csc_mode
        // 0 0 1920 540 1280 360 0 0 0
        base = REG_SCALER_0;
        /*        int index = 0;
        int bypass = 0;
        int width = 1920;
        int height = 540;
        int out_width = 1280;
        int out_height = 360;
        int deinterlace = 0;
        int unsharp_bypass = 0;
        int csc_mode = 0;
        read_mem.scaler_scaler_config( index,  bypass,  width,  height, 
         out_width,  out_height,  deinterlace,  unsharp_bypass,  csc_mode);*/

        // scaler_reconfigure 7 1920 540 640 180
        // scaler_reconfigure 0 1920 540 1280 360
        // scaler_reconfigure(int index, int width_in, int height_in, int width_out, int height_out)
        int index = 0; 
        int width_in = 1920;
        int height_in = 540;
        int width_out = 1280;
        int height_out = 360;

        //read_mem.scaler_reconfigure(index, width_in, height_in, width_out, height_out);
        uint32_t reg = read_mem.reg_read(REG_BUILDID, 0);
        qDebug(category) << reglist[REG_BUILDID] << "Value:" << reg;
        for(int i = 0; i < 8; i++){
                // Fixed: Added address offset 'i' to the second parameter
                reg = read_mem.reg_read(REG_SDI_ADAPTER, i); 
                
                // Formatted output using Qt's streams
                qDebug(category) << reglist[REG_SDI_ADAPTER] 
                                      << "Offset:" << i 
                                      << "Value:" << reg;
        }
        //read_mem.reg_write(REG_BUILDID, 0);

        /*uint32_t out_line_inc = (height-6-1)*4096/(out_height);
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

        for(unsigned int i=0; i<32; i++){
            reg_write(base, 10, coeff[i*3+0]);
            reg_write(base, 11, coeff[i*3+1]);
            reg_write(base, 12, coeff[i*3+2]);
        }*/

        // Читаем значение
        //uint32_t data = fpga.read(REG_BASE);


        //qDebug(category) << "1 Read value: 0x" << std::hex << data;
        //qDebug(category) << "2 Read value: 0x" << data << QString::number(data,2);

        // Записываем значение
        // fpga.write(0x1);
        
    } catch (const std::exception& e) {
        //std::cerr << "Error: " << e.what() << std::endl;
        // std::tera << "Error: " << e.what() << std::endl;
        qDebug() <<  "Error: " << e.what();
        return 1;
    }
    return 0;
}