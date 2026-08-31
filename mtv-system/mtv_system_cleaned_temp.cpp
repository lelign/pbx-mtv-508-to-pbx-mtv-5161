void PbxMtvSystem::draw_overlay_fast(QImage *image, int offset_x, int offset_y)
{
    QElapsedTimer timer;
    timer.start();  

    if (!image) {
        qCritical(category) << ANSI_RED << "CRITICAL ERROR: QImage pointer is NULL!" << ANSI_RESET;
        return;
    }
    if (!buffer) {
        qCritical(category) << ANSI_RED << "CRITICAL ERROR: Output buffer pointer is NULL!" << ANSI_RESET;
        return;
    }
    if (image->format() != QImage::Format_ARGB32 && image->format() != QImage::Format_RGB32) {
        qCritical(category) << ANSI_RED << "WARNING: QImage format is not ARGB32/RGB32! Current format:" << image->format() << ANSI_RESET;
    }

    int aligned_offset_x = (offset_x / 2) * 2;

    Q_ASSERT(image->width() + aligned_offset_x <= 1920);
    Q_ASSERT(image->height() + offset_y <= 1080);

    int row_stride = 1920 * 3;
    int img_w = image->width();
    int img_h = image->height();

    if ((aligned_offset_x + img_w) > 1920 || (offset_y + img_h) > 1080) {
        qCritical(category) << ANSI_RED << "CRITICAL ERROR: Image with offsets goes out of Full HD bounds!";
        return;
    }
    uint8_t* start_address = reinterpret_cast<uint8_t*>(buffer + (this->current_buffer_index * (video_size / 2)));

    for (int y = 0; y < img_h; ++y) {
        int screen_y = y + offset_y;
        uint8_t * row_start_address = start_address + (screen_y * row_stride);
        uint8_t * current_row_with_offset = row_start_address + (aligned_offset_x * 3);

        convert_line(image, y, img_w, current_row_with_offset);
    }
    
    int64_t current_elapsed = timer.elapsed();
    // Выводим лог только если время изменилось по сравнению с прошлым кадром
        if (current_elapsed != last_elapsed_time && !current_elapsed == 0 ) {
                qCDebug(category) << ANSI_MAGENTA << "draw_overlay_fast();" << ANSI_RESET
                                << current_elapsed << "milliseconds" 
                                << ANSI_MAGENTA "\tcurrent_idx" << ANSI_RESET
                                << this->current_buffer_index;
                             
                last_elapsed_time = current_elapsed; // Запоминаем новое значение
        }
}


void PbxMtvSystem::convert_line(QImage * img, int y, int width, uint8_t * buffer) {
    if (y < 0 || y >= img->height()) {
        qCritical() << "CRITICAL ERROR: Requested y line" << y << "is out of QImage bounds";
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

    // Коэффициенты BT.601.
    // Y считаем БЕЗ ЗНАКА: коэффициенты все положительные и в сумме дают 256,
    // поэтому для яркого пикселя (255,255,255) сумма произведений доходит до
    // 255*256 = 65280 — это переполняет int16 (макс. 32767), но укладывается
    // в uint16 (макс. 65535). Со знаковой арифметикой здесь было переполнение,
    // из-за которого белый текст на видео давал "шумные"/битые пиксели.
    uint16x8_t y_r = vmovq_n_u16(77);
    uint16x8_t y_g = vmovq_n_u16(150);
    uint16x8_t y_b = vmovq_n_u16(29);

    // Cb/Cr коэффициенты знакопеременные, их сумма по модулю не превышает 128,
    // так что максимум |255*128| = 32640 спокойно помещается в int16 —
    // тут знаковая арифметика корректна и переполнения нет.
    int16x8_t cb_r = vmovq_n_s16(-43);
    int16x8_t cb_g = vmovq_n_s16(-85);
    int16x8_t cb_b = vmovq_n_s16(128);
    
    int16x8_t cr_r = vmovq_n_s16(128);
    int16x8_t cr_g = vmovq_n_s16(-107);
    int16x8_t cr_b = vmovq_n_s16(-21);

    // Округляем основную сетку обработки вниз до ближайшего кратного 8
    int x = 0;
    int vector_width = width & ~7; 

    // Основной цикл: обрабатываем по 8 пикселей за раз
    for(; x < vector_width; x += 8) {
        uint8x8x4_t rgb_data = vld4_u8(line + x * 4);
        
        uint8x8_t b_val = rgb_data.val[0];
        uint8x8_t g_val = rgb_data.val[1];
        uint8x8_t r_val = rgb_data.val[2];
        uint8x8_t a_val = rgb_data.val[3];

        uint16x8_t r_u = vmovl_u8(r_val);
        uint16x8_t g_u = vmovl_u8(g_val);
        uint16x8_t b_u = vmovl_u8(b_val);

        // Y: беззнаковая арифметика (см. комментарий у коэффициентов выше)
        uint16x8_t y_acc = vmulq_u16(r_u, y_r);
        y_acc = vmlaq_u16(y_acc, g_u, y_g);
        y_acc = vmlaq_u16(y_acc, b_u, y_b);
        uint8x8_t y_val = vqshrn_n_u16(y_acc, 8); // насыщающий сдвиг+сужение до 8 бит

        // Cb/Cr: знаковая арифметика (коэффициенты знакопеременные)
        int16x8_t b = vreinterpretq_s16_u16(b_u);
        int16x8_t g = vreinterpretq_s16_u16(g_u);
        int16x8_t r = vreinterpretq_s16_u16(r_u);

        int16x8_t cb_acc = vmulq_s16(r, cb_r);
        cb_acc = vmlaq_s16(cb_acc, g, cb_g);
        cb_acc = vmlaq_s16(cb_acc, b, cb_b);
        uint8x8_t cb_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cb_acc, 8), vmovq_n_s16(128)));

        int16x8_t cr_acc = vmulq_s16(r, cr_r);
        cr_acc = vmlaq_s16(cr_acc, g, cr_g);
        cr_acc = vmlaq_s16(cr_acc, b, cr_b);
        uint8x8_t cr_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cr_acc, 8), vmovq_n_s16(128)));

        // Субдискретизация 4:2:2 с усреднением соседних пикселей по горизонтали
        uint8x8x2_t cb_split = vuzp_u8(cb_val, cb_val);
        uint8x8x2_t cr_split = vuzp_u8(cr_val, cr_val);
        uint8x8_t cb_down = vrhadd_u8(cb_split.val[0], cb_split.val[1]); // 4 усреднённых значения Cb
        uint8x8_t cr_down = vrhadd_u8(cr_split.val[0], cr_split.val[1]); // 4 усреднённых значения Cr

        // Собираем итоговую тройку векторов для vst3_u8, дающую при записи
        // порядок байт Cb0 Y0 A0 Cr0 Y1 A1 Cb1 Y2 A2 Cr1 Y3 A3 ...
        uint8x8x3_t out_p1;
        out_p1.val[0] = vzip_u8(cb_down, cr_down).val[0]; // Cb0, Cr0, Cb1, Cr1...
        out_p1.val[1] = y_val;                            // Y0,  Y1,  Y2,  Y3...
        out_p1.val[2] = a_val;                            // A0,  A1,  A2,  A3...

        vst3_u8(dst, out_p1);
        dst += 24;
    }